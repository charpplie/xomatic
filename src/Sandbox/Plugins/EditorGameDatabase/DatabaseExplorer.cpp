//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "DatabaseExplorer.h"

#include <QTreeView>
#include <QHeaderView>
#include <QBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QAction>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QRegExp>
#include <QList>
#include <QInputDialog>
#include <QIcon>

#include "AppDocument.h"
#include "DatabaseModel.h"
#include "InputDialog.h"
#include "FilterMenuButton.h"

#include <Descriptor/ObjectTypeInfo.h>
#include <ICryPak.h>

//////////////////////////////////////////////////////////////////////////
struct CompareQActionNames
{
	bool operator() (const QAction* pAction1, const QAction* pAction2) const
	{
		return pAction1->text() < pAction2->text();
	}
};

//////////////////////////////////////////////////////////////////////////

class DatabaseExplorerFilterProxyModel : public QSortFilterProxyModel
{
public:
	DatabaseExplorerFilterProxyModel(QObject* parent)
		: QSortFilterProxyModel(parent)
	{
	}

	void setFilterString(const QString& filter)
	{
		m_textFilter = filter;
		m_textFilterParts = m_textFilter.split(' ', QString::SkipEmptyParts);
		m_filterModified = (filter.compare("*") == 0);
		m_acceptedChildren.clear();
	}

	void setDescriptorFilter(const QString& filter)
	{
		m_descriptorFilter = filter;
		m_acceptedChildren.clear();
	}

	void invalidate()
	{
		m_acceptedChildren.clear();
		QSortFilterProxyModel::invalidate();
	}

	void setFilterWildcard(const QString &pattern)
	{
		m_acceptedChildren.clear();
		QSortFilterProxyModel::setFilterWildcard(pattern);
	}

	bool matchFilter(int source_row, const QModelIndex& source_parent) const
	{
		QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
		DatabaseEntryNode* pEntryNode = DatabaseModel::GetEntryNode(index);
		if (pEntryNode == NULL)
			return false;

		if (!m_descriptorFilter.isEmpty())
		{
			if ((pEntryNode->type == DatabaseEntryNode::Group) && 
				m_descriptorFilter.compare( QString(pEntryNode->path.c_str()), Qt::CaseInsensitive ) == 0)
				return true;

			if (m_descriptorFilter.compare( QString(pEntryNode->descriptorTypeName.c_str()) ) != 0)
				return false;
		}

		if (m_filterModified)
		{
			if (pEntryNode->flags.AreAnyFlagsActive(DatabaseEntryNode::FlagModified))
				return true;
		}

		QString recordPath(pEntryNode->path.c_str());
		
		if (recordPath.isEmpty())
			return m_textFilterParts.empty();

		for (int i = 0, filterPartCount = m_textFilterParts.size(); i < filterPartCount; ++i)
		{
			if (!recordPath.contains(m_textFilterParts[i], Qt::CaseInsensitive))
				return false;
		}

		return true;
	}

	bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
	{
		DatabaseEntryNode* pEntryNodeLeft = DatabaseModel::GetEntryNode(left);
		DatabaseEntryNode* pEntryNodeRight = DatabaseModel::GetEntryNode(right);
		if ((pEntryNodeLeft != NULL) && (pEntryNodeLeft->type == DatabaseEntryNode::Group) &&
			(pEntryNodeRight != NULL) && (pEntryNodeRight->type == DatabaseEntryNode::Leaf))
			return true;

		return QSortFilterProxyModel::lessThan(left, right);
	}

	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
	{
		if (matchFilter(source_row, source_parent))
			return true;

		if (hasAcceptedChildren(source_row, source_parent))
			return true;

		return false;
	}

protected:

	bool hasAcceptedChildrenCached(int source_row, const QModelIndex& source_parent) const
	{
		std::pair<QModelIndex,int> indexId = std::make_pair(source_parent, source_row);
		TAcceptedChildren::iterator it = m_acceptedChildren.find(indexId);
		if (it == m_acceptedChildren.end()) {
			const bool result = hasAcceptedChildren(source_row, source_parent);
			m_acceptedChildren[indexId] = result;
			return result;
		}
		else
		{
			return it->second;
		}
	}

	bool hasAcceptedChildren(int source_row, const QModelIndex& source_parent) const
	{
		QModelIndex item = sourceModel()->index(source_row, 0, source_parent);
		if (!item.isValid())
			return false;

		int childCount = item.model()->rowCount(item);
		if (childCount == 0)
			return false;

		for (int i = 0; i < childCount; ++i) 
		{
			if (filterAcceptsRow(i, item))
				return true;
		}

		return false;
	}

private:

	QString m_descriptorFilter;
	QString m_textFilter;
	QStringList m_textFilterParts;

	bool m_filterModified;

	typedef std::map<std::pair<QModelIndex, int>, bool> TAcceptedChildren;
	mutable TAcceptedChildren m_acceptedChildren;
};

//////////////////////////////////////////////////////////////////////////
namespace ExplorerHelpers
{
	void ExpandTreeFirstLevel(QTreeView& treeView)
	{
		const QModelIndex rootIndex = treeView.rootIndex();
		const int numChildren = treeView.model()->rowCount(rootIndex);
		for (int i = 0; i < numChildren; ++i)
		{
			const QModelIndex index = treeView.model()->index(i, 0, rootIndex);
			treeView.expand(index);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

DatabaseExplorer::DatabaseExplorer(QWidget* pParent, AppDocument& appDocument)
	: QWidget(pParent)
	, m_appDocument(appDocument)
	, m_pFilterByText(NULL)
	, m_pFilterMenuButton(NULL)
	, m_pTreeView(NULL)
	, m_pDatabaseModel(NULL)
	, m_pDatabaseModelFilter(NULL)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	
	QBoxLayout* pExplorerLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	pExplorerLayout->setMargin(0);
	setLayout(pExplorerLayout);

	// Filter controls 
	{
		QBoxLayout* pHorizontalLayout = new QBoxLayout(QBoxLayout::LeftToRight);

		m_pFilterByText = new QLineEdit();
		m_pFilterByText->setToolTip(QString("Filter by name (type '*' to display modified records)"));
		pHorizontalLayout->addWidget(m_pFilterByText, 1);
		connect(m_pFilterByText, SIGNAL(textChanged(const QString&)), this, SLOT(OnSignalFilterTextChanged(const QString&)));
		
		m_pFilterMenuButton = new FilterMenuButton(appDocument);
		pHorizontalLayout->addWidget(m_pFilterMenuButton, 0);
		connect( m_pFilterMenuButton, SIGNAL(SignalSelectedFilterChanged(const QString&)), this, SLOT(OnSignalFilterMenuTypeSelectionChanged(const QString&)));

		pExplorerLayout->addLayout(pHorizontalLayout);	
	}

	// Tree view 
	m_pTreeView = new QTreeView();
	{
		m_pTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
		m_pTreeView->header()->setContextMenuPolicy(Qt::CustomContextMenu);
		m_pTreeView->setIndentation(16);
		m_pTreeView->setSortingEnabled(true);
		m_pTreeView->setSelectionMode(QAbstractItemView::SingleSelection);

		m_pDatabaseModel =  new DatabaseModel(m_appDocument.GetDatabase(), this);
		m_pDatabaseModelFilter = new DatabaseExplorerFilterProxyModel(this);
		m_pDatabaseModelFilter->setDynamicSortFilter(true);

		m_pDatabaseModelFilter->setSourceModel(m_pDatabaseModel);
		m_pTreeView->setModel( m_pDatabaseModelFilter );
		m_pTreeView->sortByColumn(0, Qt::AscendingOrder);
		m_pTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

		m_pTreeView->header()->setStretchLastSection(false);
		m_pTreeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);

		const int columnCount = m_pDatabaseModel->columnCount(QModelIndex());

		for (int i = 1; i < columnCount; ++i)
		{
			m_pTreeView->header()->setSectionResizeMode(i, QHeaderView::Interactive);
			m_pTreeView->header()->resizeSection(i, 24);
			//m_pTreeView->header()->hideSection(i);
		}

		pExplorerLayout->addWidget(m_pTreeView);

		connect(m_pTreeView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(OnSignalTreeSelectionChanged(const QItemSelection&, const QItemSelection&)));
		connect(m_pTreeView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(OnSignalShowContextMenu(const QPoint&)));
		connect(m_pTreeView->header(), SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(OnSignalShowHeaderContextMenu(const QPoint&)));
	
		connect(&m_appDocument.GetDatabase(), SIGNAL(SignalEndAddEntryNode(DatabaseEntryNode*)), this, SLOT(OnSignalEndAddEntryNode(DatabaseEntryNode*)));
	}

	ExpandTree();
}

DatabaseExplorer::~DatabaseExplorer()
{
	m_pTreeView->setModel(NULL);
}

void DatabaseExplorer::OnSignalTreeSelectionChanged( const QItemSelection& selected, const QItemSelection& deselected )
{
	DatabaseEntryNodePtr pSelectedEntry;

	QItemSelection selection = m_pTreeView->selectionModel()->selection();
	if (m_pTreeView->model() == m_pDatabaseModelFilter)
	{
		selection = m_pDatabaseModelFilter->mapSelectionToSource(selection);
	}

	const QModelIndexList& indices = selection.indexes();
	if (indices.size() > 0)
	{
		const QModelIndex& index = indices[0];
		pSelectedEntry = DatabaseModel::GetEntryNode(index);
	}

	m_appDocument.SetSelectedExplorerRecord(pSelectedEntry);
}

void DatabaseExplorer::OnSignalFilterMenuTypeSelectionChanged(const QString& selectionName)
{
	m_filterTypeSelectionName = selectionName;
	
	if (m_pFilterByText)
	{
		m_pFilterByText->clear();
	}

	if (m_pDatabaseModelFilter)
	{
		m_pDatabaseModelFilter->setDescriptorFilter(selectionName);
		m_pDatabaseModelFilter->invalidate();
	}

	ExpandTree();

	OnSignalTreeSelectionChanged( QItemSelection(), QItemSelection() );
}

void DatabaseExplorer::OnSignalFilterTextChanged(const QString& text)
{
	if (m_pDatabaseModelFilter)
	{
		m_pDatabaseModelFilter->setFilterString(text);
		m_pDatabaseModelFilter->invalidate();

		ExpandTree();
	}

	OnSignalTreeSelectionChanged( QItemSelection(), QItemSelection() );
}

void DatabaseExplorer::OnSignalShowContextMenu(const QPoint& position)
{
	QModelIndex clickedIndex = m_pTreeView->indexAt(position);

	QMenu contextMenu;
	const DatabaseEntryNodePtr pSelectedEntry = (clickedIndex.isValid()) ? m_appDocument.GetSelectedExplorerRecord() : DatabaseEntryNodePtr(NULL);
	if (pSelectedEntry == NULL)
	{
		PopulateContextMenuForGroupEditingAtRoot(contextMenu);
	}
	else if (pSelectedEntry->type == DatabaseEntryNode::Group)
	{
		PopulateContextMenuForGroupEditing(contextMenu, pSelectedEntry);
	}
	else
	{
		PopulateContextMenuForRecordEditing(contextMenu, pSelectedEntry);
	}

	const QPoint globalPosition = m_pTreeView->viewport()->mapToGlobal(position);
	contextMenu.exec(globalPosition);
}

void DatabaseExplorer::OnSignalContextMenuAddRecord()
{
	QAction* pSenderAction = static_cast<QAction*>(sender());
	if (pSenderAction == NULL)
		return;

	DatabaseEntryNodePtr pParentEntryNode = m_appDocument.GetSelectedExplorerRecord();
	if (pParentEntryNode == NULL)
	{
		pParentEntryNode = m_appDocument.GetDatabase().GetRoot();
	}

	AddEntryNodeLeafDialog addRecordDialog( pParentEntryNode );
	addRecordDialog.Initialize();
	if (!addRecordDialog.exec())
		return;

	const QStringList paramList = pSenderAction->data().toStringList();
	const int count = paramList.count();
	const QString descriptorType = paramList.at(0);
	const QString folderPath = paramList.at(1);
	const QString recordName = addRecordDialog.GetProvidedName();

	const string finalRecordName = Database::GenerateRecordNameFor(folderPath.toStdString().c_str(), recordName.toStdString().c_str());
	m_appDocument.GetDatabase().AddNewRecord( descriptorType.toStdString().c_str(), finalRecordName.c_str() );

	// Force a re-selection of the just added record
	// This forces a refresh on the properties view which checks the 'file' status on disk right after adding and makes sure it is editable
	m_appDocument.SetSelectedExplorerRecord(m_appDocument.GetSelectedExplorerRecord());
}

void DatabaseExplorer::OnSignalContextMenuDeleteRecord()
{
	QAction* pSenderAction = static_cast<QAction*>(sender());
	if (pSenderAction == NULL)
		return;

	const QString fullRecordName = pSenderAction->data().toString();

	stack_string message;
	message.Format("Do you want to remove '%s'?", fullRecordName.toStdString().c_str());
	ConfirmationDialog deleteDialog("Delete record", message.c_str());
	if(!deleteDialog.exec())
		return;

	m_appDocument.GetDatabase().DeleteRecord( fullRecordName.toStdString().c_str() );
}

void DatabaseExplorer::OnSignalContextMenuCloneRecord()
{
	QAction* pSenderAction = static_cast<QAction*>(sender());
	if (pSenderAction == NULL)
		return;

	DatabaseEntryNodePtr pSelectedEntryNode = m_appDocument.GetSelectedExplorerRecord();
	if (pSelectedEntryNode == NULL)
		return;

	DatabaseEntryNodePtr pParentEntryNode = pSelectedEntryNode->pParent;

	CloneEntryNodeLeafDialog cloneRecordDialog( pParentEntryNode );
	cloneRecordDialog.Initialize();
	if (!cloneRecordDialog.exec())
		return;

	const QString sourceRecordName = pSenderAction->data().toString();
	QString clonedRecordName = QString(pParentEntryNode->path.c_str());
	if (clonedRecordName.isEmpty())
	{
		 clonedRecordName.append(cloneRecordDialog.GetProvidedName());
	}
	else
	{
		clonedRecordName.append(QString("/") + cloneRecordDialog.GetProvidedName());
	}

	m_appDocument.GetDatabase().CloneRecord( sourceRecordName.toStdString().c_str(), clonedRecordName.toStdString().c_str() );
}

void DatabaseExplorer::OnSignalContextMenuSaveRecord()
{
	QAction* pSenderAction = static_cast<QAction*>(sender());
	if (pSenderAction == NULL)
		return;

	const QString recordName = pSenderAction->data().toString();

	m_appDocument.GetDatabase().SaveRecord( recordName.toStdString().c_str() );
}

void DatabaseExplorer::OnSignalContextMenuRevertRecord()
{
	QAction* pSenderAction = static_cast<QAction*>(sender());
	if (pSenderAction == NULL)
		return;

	const QString recordName = pSenderAction->data().toString();

	m_appDocument.GetDatabase().RevertRecordChanges( recordName.toStdString().c_str() );
}

void DatabaseExplorer::OnSignalContextMenuShowInExplorer()
{
	QAction* pSenderAction = static_cast<QAction*>(sender());
	if (pSenderAction == NULL)
		return;

	const QString recordName = pSenderAction->data().toString();
	m_appDocument.GetDatabase().ShowRecordInExplorer( recordName.toStdString().c_str() );
}

void DatabaseExplorer::OnSignalContextMenuExtractFileFromPak()
{
	QAction* pSenderAction = static_cast<QAction*>(sender());
	if (pSenderAction == NULL)
		return;

	const QString recordName = pSenderAction->data().toString();
	m_appDocument.GetDatabase().ExtractRecordFromPak( recordName.toStdString().c_str() );
}

void DatabaseExplorer::OnSignalShowHeaderContextMenu(const QPoint& position)
{
	QMenu contextMenu;
	QAction* pAction = contextMenu.addAction("Expand all");
	pAction->setData(QVariant(true));
	connect(pAction, SIGNAL(triggered()), this, SLOT(OnSignalHeaderContextMenuExpandOrCollapse()));

	pAction = contextMenu.addAction("Collapse");
	pAction->setData(QVariant(false));
	connect(pAction, SIGNAL(triggered()), this, SLOT(OnSignalHeaderContextMenuExpandOrCollapse()));

	contextMenu.addSeparator();

	QMenu* pColumnsMenu = new QMenu(QString("Display extra columns"));
	const int columnCount = m_pDatabaseModel->columnCount(QModelIndex());
	for (int i = 1; i < columnCount; ++i)
	{
		pAction = pColumnsMenu->addAction(m_pDatabaseModel->GetHeaderSectionName(i));
		pAction->setCheckable(true);
		pAction->setChecked(!m_pTreeView->header()->isSectionHidden(i));
		pAction->setData(QVariant(i));
		connect(pAction, SIGNAL(triggered()), this, SLOT(OnSignalHeaderContextMenuDisplayColumn()));
	}
	contextMenu.addMenu(pColumnsMenu);

	const QPoint globalPosition = m_pTreeView->header()->viewport()->mapToGlobal(position);
	contextMenu.exec(globalPosition);
}

void DatabaseExplorer::OnSignalHeaderContextMenuExpandOrCollapse()
{
	QAction* pSenderAction = static_cast<QAction*>(sender());
	if (pSenderAction == NULL)
		return;

	const bool expandTree = pSenderAction->data().toBool();

	if (expandTree)
	{
		ExpandTree();
	}
	else
	{
		m_pTreeView->collapseAll();
	}
}

void DatabaseExplorer::OnSignalHeaderContextMenuDisplayColumn()
{
	QAction* pSenderAction = static_cast<QAction*>(sender());
	if (pSenderAction == NULL)
		return;
	
	int column = pSenderAction->data().toInt();
	m_pTreeView->header()->setSectionHidden(column, !m_pTreeView->header()->isSectionHidden(column));
}

void DatabaseExplorer::OnSignalEndAddEntryNode(DatabaseEntryNode* pEntryNode)
{
	// Select new entry 
	const QModelIndex newEntryIndex = GetIndexFromEntryNode(pEntryNode);
	if(newEntryIndex.isValid())
	{
		m_pTreeView->selectionModel()->select(newEntryIndex, QItemSelectionModel::ClearAndSelect);
	}

		// Expand parent
	if (pEntryNode->pParent != NULL)
	{
		const QModelIndex parentIndex = GetIndexFromEntryNode(pEntryNode->pParent);
		if(parentIndex.isValid() && !m_pTreeView->isExpanded(parentIndex))
		{
			m_pTreeView->expand(parentIndex);
		}
	}
}

void DatabaseExplorer::PopulateContextMenuForGroupEditingAtRoot(QMenu& contextMenu)
{
	PopulateContextMenuForGroupEditing(contextMenu, DatabaseEntryNodePtr(NULL));
}

void DatabaseExplorer::PopulateContextMenuForGroupEditing(QMenu& contextMenu, const DatabaseEntryNodePtr pEntryNode)
{
	QString groupPath = NULL;
	QString groupFilter = m_filterTypeSelectionName.toLower();
	
	if (pEntryNode != NULL)
	{
		groupPath = pEntryNode->path.c_str();
		groupFilter = pEntryNode->path.c_str();
	}
		
	const size_t typesCount = m_appDocument.GetDatabase().GetTypesCount();
	if (typesCount > 0)
	{
		QMenu* pAddMenu = contextMenu.addMenu(QIcon(":/icons/add.png"), "Add new");

		std::vector<QAction*> addNewActions;
		addNewActions.reserve(typesCount);
		for(size_t i = 0; i < typesCount; ++i)
		{
			const ObjectTypeInfoEntry& typeEntry = m_appDocument.GetDatabase().GetTypeAt(i);
			if ( !groupFilter.isEmpty() && (groupFilter.compare(typeEntry.GetGroupName().c_str()) != 0))
				continue;

			QAction* pAction = new QAction(typeEntry.pTypeInfo->name.c_str(), &contextMenu);
			QStringList paramList;
			paramList.append(typeEntry.pTypeInfo->name.c_str());
			paramList.append(!groupPath.isEmpty() ? groupPath : QString(typeEntry.GetGroupName().c_str()));
			pAction->setData(QVariant(paramList));
			connect(pAction, SIGNAL(triggered()), this, SLOT(OnSignalContextMenuAddRecord()));
			addNewActions.push_back(pAction);
		}

		std::sort(addNewActions.begin(), addNewActions.end(), CompareQActionNames());

		for (size_t i = 0, actionCount = addNewActions.size(); i < actionCount; ++i)
		{
			pAddMenu->addAction(addNewActions[i]);
		}
	}
}

void DatabaseExplorer::PopulateContextMenuForRecordEditing(QMenu& contextMenu, const DatabaseEntryNodePtr pEntryNode)
{
	CRY_ASSERT(pEntryNode != NULL);

	const QString recordPath( pEntryNode->path.c_str() );

	QAction* pAction = contextMenu.addAction(QIcon(":/icons/save.png"), QString("Save"));
	pAction->setData(recordPath);
	pAction->setEnabled(pEntryNode->flags.AreAnyFlagsActive(DatabaseEntryNode::FlagModified));
	connect(pAction, SIGNAL(triggered()), this, SLOT(OnSignalContextMenuSaveRecord()));

	pAction = contextMenu.addAction(QIcon(":/icons/revert.png"), QString("Revert"));
	pAction->setData(recordPath);
	pAction->setEnabled(pEntryNode->flags.AreAllFlagsActive(DatabaseEntryNode::FlagModified));
	connect(pAction, SIGNAL(triggered()), this, SLOT(OnSignalContextMenuRevertRecord()));

	pAction = contextMenu.addAction(QIcon(":/icons/clone.png"), QString("Clone"));
	pAction->setData(recordPath);
	connect(pAction, SIGNAL(triggered()), this, SLOT(OnSignalContextMenuCloneRecord()));

	pAction = contextMenu.addAction(QIcon(":/icons/delete.png"), QString("Delete"));
	pAction->setData(recordPath);
	pAction->setEnabled(pEntryNode->flags.AreAllFlagsActive(DatabaseEntryNode::FlagInFolder));
	connect(pAction, SIGNAL(triggered()), this, SLOT(OnSignalContextMenuDeleteRecord()));

	contextMenu.addSeparator();

	if (pEntryNode->flags.AreAllFlagsActive(DatabaseEntryNode::FlagInFolder))
	{
		pAction = contextMenu.addAction(QIcon(":/icons/show_in_explorer.png"), QString("Show in explorer"));
		pAction->setData(recordPath);
		connect(pAction, SIGNAL(triggered()), this, SLOT(OnSignalContextMenuShowInExplorer()));
	}
	else
	{
		pAction = contextMenu.addAction(QIcon(":/icons/extract_from_pak.png"), QString("Extract from pak to edit"));
		pAction->setData(recordPath);
		connect(pAction, SIGNAL(triggered()), this, SLOT(OnSignalContextMenuExtractFileFromPak()));
	}
}

bool DatabaseExplorer::IsAnyFilteringEnabled() const
{
	return (!m_filterTypeSelectionName.isEmpty()) || (!m_pFilterByText->text().isEmpty());
}

void DatabaseExplorer::ExpandTree()
{
	if (IsAnyFilteringEnabled())
	{
		m_pTreeView->expandAll();
	}
	else
	{
		ExplorerHelpers::ExpandTreeFirstLevel(*m_pTreeView);
	}
}

const QModelIndex DatabaseExplorer::GetIndexFromEntryNode( DatabaseEntryNode* pEntryNode )
{
	if (pEntryNode->pParent == NULL)
		return QModelIndex();

	const QModelIndex entryIndex = m_pDatabaseModel->ModelIndexFromNode(pEntryNode);
	QItemSelection selection(entryIndex, entryIndex);

	if (m_pTreeView->model() == m_pDatabaseModelFilter)
	{
		selection = m_pDatabaseModelFilter->mapSelectionFromSource(selection);
	}

	const QModelIndexList& selectedIndices = selection.indexes();
	
	return (selectedIndices.size() > 0) ? selectedIndices[0] : QModelIndex();
}

#include <moc_DatabaseExplorer.cpp>