//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "AppDocument.h"

AppDocument::AppDocument()
{
	m_pDatabase.reset( new Database() );

	connect(m_pDatabase.get(), SIGNAL(SignalEntryNodeReverted(DatabaseEntryNode*)), this, SLOT(OnSignalEntryNodeReverted(DatabaseEntryNode*)));
}

AppDocument::~AppDocument()
{

}

void AppDocument::SetSelectedExplorerRecord( const DatabaseEntryNodePtr& selection )
{
	m_pSelectedEntry = selection;

	SignalExplorerSelectionChanged();
}

const DatabaseEntryNodePtr AppDocument::GetSelectedExplorerRecord() const
{
	return m_pSelectedEntry;
}

Database& AppDocument::GetDatabase()
{
	CRY_ASSERT(m_pDatabase);

	return *m_pDatabase.get(); 
}

const Database& AppDocument::GetDatabase() const
{
	CRY_ASSERT(m_pDatabase);

	return *m_pDatabase.get(); 
}

bool AppDocument::HasUnsavedData() const
{
	return m_pDatabase->HasModifiedRecords();
}

void AppDocument::SaveAll()
{
	m_pDatabase->SaveAllModifiedRecords();
}

void AppDocument::RevertAll()
{
	m_pDatabase->RevertAllModifiedRecords();
}

void AppDocument::OnSignalPropertyTreeDataChanged()
{
	m_pDatabase->RefreshRecordModified( m_pSelectedEntry );
}

void AppDocument::OnSignalEntryNodeReverted(DatabaseEntryNode*)
{
	SignalExplorerSelectionRefresh();
}

#include <moc_AppDocument.cpp>



