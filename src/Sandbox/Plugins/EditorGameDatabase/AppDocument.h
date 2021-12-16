////////////////////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// ----------------------------------------------------------------------------------------
//  File name:   AppDocument
//  Description: This class holds all data used by the application
//
//////////////////////////////////////////////////////////////////////////////////////////// 

#ifndef _APP_DOCUMENT_H_
#define _APP_DOCUMENT_H_

#pragma once

#include <QtCore/QObject>

#include "Database.h"

class AppDocument : public QObject
{
	Q_OBJECT

public:

	AppDocument();

	~AppDocument();

	void SetSelectedExplorerRecord( const DatabaseEntryNodePtr& selection );
	const DatabaseEntryNodePtr GetSelectedExplorerRecord() const;

	Database& GetDatabase();
	const Database& GetDatabase() const;

	bool HasUnsavedData() const;
	void SaveAll();
	void RevertAll();
signals:
	void SignalExplorerSelectionChanged();
	void SignalExplorerSelectionRefresh();

public slots:
	void OnSignalPropertyTreeDataChanged();
	void OnSignalEntryNodeReverted(DatabaseEntryNode*);

private:

	std::unique_ptr<Database> m_pDatabase;
	
	DatabaseEntryNodePtr m_pSelectedEntry;
};

#endif // _APP_DOCUMENT_H_

