 /*
 *    QRAP Project
 *
 *    Version     : 0.1
 *    Date        : 2008/04/01
 *    License     : GNU GPLv3
 *    File        : cRapLinks.cpp
 *    Copyright   : (c) University of Pretoria
 *    Authors     : Roeland van Nieukerk (roeland.frans@gmail.com)
 *    Description : This class displays all the tables related to 
 *                  links and allows the user to edit the tables.
 *
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; See the GNU General Public License for      *
 *   more details                                                          *
 *                                                                         *
 ************************************************************************* */

#include "Config.h"
#include "cRapLinks.h"
#include "cSearchForm.h"

using namespace Qrap;
using namespace std;



// The constructor
cRapLinks::cRapLinks (QWidget* parent) : QWidget(parent)
{
	// Create all the widget required
	mTableList = new QListWidget(this);
	
	// Setup the mTableList
	mTableList->setMinimumSize(140,400);
	mTableList->setMaximumWidth(200);
	
	// create the mProgress bar
	mProgress = new QProgressBar(this);
	mProgress->setMaximumWidth(120);
	mProgress->setVisible(false);
	mProgress->reset();
	
	// Create the layouts
	mMainLayout = new QGridLayout(this);
	QVBoxLayout* listLayout = new QVBoxLayout();
	
	listLayout->addWidget(mTableList);
	listLayout->addWidget(mProgress);
	
	// Setup mMainLayout
	mMainLayout->addLayout(listLayout,0,0);
	mMainLayout->setSpacing(5);	
	setLayout(mMainLayout);
	
	// Create the database communicator class
	mDbCommunicator = new RapDbCommunicator(this,mProgress);
	
	// Populate the table list
	PopulateTableList();
	
	tableViewSelected = true;
	mInsertingRow = false;
	
	// Setup all the signals and slots
	connect(mTableList,SIGNAL(itemSelectionChanged()),this,SLOT(TableSelectionChanged()));
	
	// Set the default table selection to be the first row
	if(mCurrentTable->GetTable()->rowCount()!=0)
	{
		mCurrentTable->GetTable()->setCurrentCell(0,0);
		mCurrentTable->GetTable()->selectRow(0);
	} // if
}



// Destructor
cRapLinks::~cRapLinks ()
{
	// delete the RapDbCommunicator object
	delete mDbCommunicator;
}



// Insert a new row into the table and database engine
void cRapLinks::InsertRow ()
{
	// Make sure that the object knows that we're inserting a new row
	mInsertingRow = true;
	
	// Create the insert form and run the insert query
	mCurrentTable->CreateInsertForm();
	
	// Perform the insert
	//mDbCommunicator->InsertRow(mCurrentTable->GetTable(),mTables.key(mCurrentTable),"siteid",mCurrentCellId);
	
	// Let the GuiEngine object know that inserting is complete
	mInsertingRow = false;
}



// Delete the selected rows
void cRapLinks::DeleteRows ()
{
	// Perform the delete
	mDbCommunicator->DeleteRows(mCurrentTable->GetTable(),mTables.key(mCurrentTable));
}



// Reload the current table contents
void cRapLinks::ReloadTable ()
{
	// Make sure that no update signals are triggered
	disconnect(mCurrentTable->GetTable(),SIGNAL(cellChanged(int,int)),0,0);
	
	mDbCommunicator->PopulateTable(mCurrentTable->GetTable(),mTables.key(mCurrentTable),"id",-1,false,true);
	
	if (!tableViewSelected)
		mCurrentTable->CreateUpdateForm();
	// Reconnect the update signals
	connect(mCurrentTable->GetTable(),SIGNAL(cellChanged(int,int)),this,SLOT(UpdateDatabase(int,int)));
}



// Show all the contents of the current Table
void cRapLinks::ShowAllContents ()
{
	// Make sure that no update signals are triggered
	disconnect(mCurrentTable->GetTable(),SIGNAL(cellChanged(int,int)),0,0);
	
	mDbCommunicator->PopulateTable(mCurrentTable->GetTable(),mTables.key(mCurrentTable),"id",-1,false,true);
	
	// Reconnect the update signals
	connect(mCurrentTable->GetTable(),SIGNAL(cellChanged(int,int)),this,SLOT(UpdateDatabase(int,int)));
}



// Copy current cells contents into the clipboard
void cRapLinks::Copy ()
{
	if(mCurrentTable->GetTable()->currentItem()!=NULL)
	{
		QClipboard* clipboard = static_cast<MainWindow*>(window())->clipboard;
		clipboard->setText(mCurrentTable->GetTable()->currentItem()->data(Qt::DisplayRole).toString());
	}
}



// Paste the current text from the clipboard to the current cell
void cRapLinks::Paste ()
{
	if(mCurrentTable->GetTable()->currentItem()!=NULL)
	{
		QClipboard* clipboard = static_cast<MainWindow*>(window())->clipboard;
		mCurrentTable->GetTable()->currentItem()->setData(Qt::DisplayRole,QVariant(clipboard->text()));
	}
}



// Cut the data from the current cell
void cRapLinks::Cut ()
{
	if(mCurrentTable->GetTable()->currentItem()!=NULL)
	{
		QClipboard* clipboard = static_cast<MainWindow*>(window())->clipboard;
		clipboard->setText(mCurrentTable->GetTable()->currentItem()->data(Qt::DisplayRole).toString());
		mCurrentTable->GetTable()->currentItem()->setData(Qt::DisplayRole,QVariant(""));
	}
}



// Search in the currently selected table
void cRapLinks::Search ()
{
	cSearchForm* searchForm = new cSearchForm(0,mCurrentTable->mTableName,this);
	
	searchForm->show();
}



// Excute the search
void cRapLinks::ExecuteSearch (string search)
{
	disconnect(mCurrentTable->GetTable(),SIGNAL(cellChanged(int,int)),0,0);
	
	mDbCommunicator->PopulateTable(mCurrentTable->GetTable(),mCurrentTable->mTableName,"id",-1,false,true,search);
	
	connect(mCurrentTable->GetTable(),SIGNAL(cellChanged(int,int)),this,SLOT(UpdateDatabase(int,int)));
}



// Make sure that the correct table is made visible
void cRapLinks::TableSelectionChanged ()
{

	QListWidgetItem* selectedItem = mTableList->currentItem();
	
	// Remove the previous table from the Grid layout
	mMainLayout->removeWidget(mCurrentTable);
	mCurrentTable->setVisible(false);
	
	// Make sure that no update signals are triggered
	disconnect(mCurrentTable->GetTable(),SIGNAL(cellChanged(int,int)),0,0);
	
	mCurrentTable = mTables[selectedItem->data(Qt::UserRole).toString()];
	mMainLayout->addWidget(mCurrentTable,0,1);
	
	// populate the current table
	if(mCurrentTable->GetTable()->rowCount()==0)
		mDbCommunicator->PopulateTable(mCurrentTable->GetTable(),"links","id",-1,false,true);

	
	// Reconnect the update signals
	connect(mCurrentTable->GetTable(),SIGNAL(cellChanged(int,int)),this,SLOT(UpdateDatabase(int,int)));
	
	// Check whether the form view is selected or the table view
	if(tableViewSelected)
		mCurrentTable->setCurrentIndex(0);
	else
		mCurrentTable->setCurrentIndex(1);
	
	
	// Make sure that the MainWindow object knows what the current table is
	static_cast<MainWindow*>(window())->currentTable = getCurrentTableName();
	
	mCurrentTable->setVisible(true);
}



// Update the database
void cRapLinks::UpdateDatabase(int row, int col)
{
	if(!mInsertingRow)
	{
		// Check whether the 
		// Perform the update
		mDbCommunicator->UpdateDatabase(mCurrentTable->GetTable(),mTables.key(mCurrentTable),row,col);
	} // if
}



// Populate the mTableList
void cRapLinks::PopulateTableList ()
{
	// Create the items
	QListWidgetItem* linksItem = new QListWidgetItem("Links");
	
	// Setup the items
	linksItem->setData(Qt::UserRole,QVariant("links"));
	linksItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
	
	// Add the items to the mTableList
	mTableList->addItem(linksItem);
	
	// Create the different table widgets
	mTables.insert("links",new cRapTableTab(this,"links"));
	
	QMapIterator<QString,cRapTableTab*> it(mTables);
	
	// Setup the different tables
	while(it.hasNext())
	{
		it.next();
		
		// Setup the table
		it.value()->GetTable()->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
		it.value()->GetTable()->setSelectionBehavior(QAbstractItemView::SelectRows);
		it.value()->GetTable()->setSelectionMode(QAbstractItemView::ContiguousSelection);
		it.value()->GetTable()->verticalHeader()->setVisible(false);
		it.value()->GetTable()->setMinimumSize(600,400);
		it.value()->setVisible(false);
		it.value()->mTableViewSelected = &tableViewSelected;
		
		// Load the headers for each table
		mDbCommunicator->LoadTableHeaders(it.value()->GetTable(),it.key(),"id",true);
	} // while
	
	// Make sure that the site table is visible
	mMainLayout->addWidget(mTables["links"],0,1);
	mTables["links"]->show();
	mCurrentTable = mTables["links"];
	
	// Make sure that no update signals are triggered
	disconnect(mCurrentTable->GetTable(),SIGNAL(cellChanged(int,int)),0,0);
	
	// Populate the site table first
	mDbCommunicator->PopulateTable(mCurrentTable->GetTable(),"links","",-1,false,true);
	
	// Reconnect the update signals
	connect(mCurrentTable->GetTable(),SIGNAL(cellChanged(int,int)),this,SLOT(UpdateDatabase(int,int)));
}
