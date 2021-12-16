////////////////////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// ----------------------------------------------------------------------------------------
//  File name:   DatabaseExplorer
//  Description: Qt explorer view of the database
//
//////////////////////////////////////////////////////////////////////////////////////////// 

#ifndef _DATABASE_EXPLORER_H_
#define _DATABASE_EXPLORER_H_

#pragma once

#include <QWidget>
#include "Database.h"

class QTreeView;
class QItemSelection;
class QLineEdit;
class QAction;
class QMenu;
class DatabaseModel;
class DatabaseExplorerFilterProxyModel;
class AppDocument;
class FilterMenuButton;

class DatabaseExplorer : public QWidget
{
	Q_OBJECT

private:
	typedef std::vector<QAction*> MenuActions;

public:
	DatabaseExplorer(QWidget* pParent, AppDocument& appDocument);
	~DatabaseExplorer();

public slots:
	void OnSignalTreeSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

	void OnSignalFilterMenuTypeSelectionChanged(const QString& selectionName);
	void OnSignalFilterTextChanged(const QString& text);

	void OnSignalShowContextMenu(const QPoint& position);
	void OnSignalContextMenuAddRecord();
	void OnSignalContextMenuDeleteRecord();
	void OnSignalContextMenuCloneRecord();
	void OnSignalContextMenuSaveRecord();
	void OnSignalContextMenuRevertRecord();
	void OnSignalContextMenuShowInExplorer();
	void OnSignalContextMenuExtractFileFromPak();

	void OnSignalShowHeaderContextMenu(const QPoint& position);
	void OnSignalHeaderContextMenuExpandOrCollapse();
	void OnSignalHeaderContextMenuDisplayColumn();

	void OnSignalEndAddEntryNode(DatabaseEntryNode* pEntryNode);

private:

	void PopulateContextMenuForGroupEditingAtRoot(QMenu& contextMenu);
	void PopulateContextMenuForGroupEditing(QMenu& contextMenu, const DatabaseEntryNodePtr pEntryNode);
	void PopulateContextMenuForRecordEditing(QMenu& contextMenu, const DatabaseEntryNodePtr pEntryNode);

	bool IsAnyFilteringEnabled() const;

	void ExpandTree();
	const QModelIndex GetIndexFromEntryNode(DatabaseEntryNode* pEntryNode);

	// Filtering
	QLineEdit*   m_pFilterByText;
	
	FilterMenuButton* m_pFilterMenuButton;
	QString           m_filterTypeSelectionName;

	// Tree explorer
	QTreeView*   m_pTreeView;
	DatabaseModel* m_pDatabaseModel;
	DatabaseExplorerFilterProxyModel* m_pDatabaseModelFilter;
	
	AppDocument& m_appDocument;
};

#endif 

