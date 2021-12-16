//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "PropertiesPanel.h"

#include <Descriptor/IObject.h>
#include <Descriptor/IDatabase.h>
#include <QSplitter>

#include "../EditorCommon/QPropertyTree/QPropertyTree.h"

#include "AppDocument.h"
#include "Database.h"

PropertiesPanel::PropertiesPanel( QWidget* pParent, AppDocument& appDocument )
 : QDockWidget("Properties")
 , m_appDocument(appDocument)
{
	m_pPropertyTree =  new QPropertyTree(this);

	setWidget(m_pPropertyTree);
	setFeatures((QDockWidget::DockWidgetFeatures)(QDockWidget::AllDockWidgetFeatures & ~(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetFloatable|QDockWidget::DockWidgetMovable)));

	connect(&m_appDocument, SIGNAL(SignalExplorerSelectionChanged()), this, SLOT(OnSignalExplorerSelectionChanged()));
	connect(&m_appDocument, SIGNAL(SignalExplorerSelectionRefresh()), this, SLOT(OnSignalExplorerSelectionRefresh()));

	connect(m_pPropertyTree, SIGNAL(signalChanged()), &m_appDocument, SLOT(OnSignalPropertyTreeDataChanged()));
	connect(m_pPropertyTree, SIGNAL(signalContinuousChange()), &m_appDocument, SLOT(OnSignalPropertyTreeDataChanged()));
}

void PropertiesPanel::OnSignalExplorerSelectionChanged()
{
	const DatabaseEntryNodePtr pSelectedEntryNode = m_appDocument.GetSelectedExplorerRecord();

	const bool isLeafNode = (pSelectedEntryNode != NULL) && (pSelectedEntryNode->type == DatabaseEntryNode::Leaf); 
	const DatabaseEntry* pDatabaseEntry = isLeafNode ? m_appDocument.GetDatabase().GetDatabaseEntryById(pSelectedEntryNode->databaseEntryId) : NULL;
	if (pDatabaseEntry)
	{
		setWindowTitle( QString("Properties: ") + QString(pDatabaseEntry->path.c_str()) );
		m_pPropertyTree->detach();

		m_pPropertyTree->setExpandLevels(2);
		m_pPropertyTree->attach(Serialization::SStruct(*pDatabaseEntry->Get()->pObject.get()));
		m_pPropertyTree->setCompact(false);
		
		const bool canEditEntry = pSelectedEntryNode->flags.AreAllFlagsActive(DatabaseEntryNode::FlagInFolder);
		m_pPropertyTree->setEnabled(canEditEntry);
	}
	else
	{
		setWindowTitle( QString("Properties") );
		m_pPropertyTree->detach();
	}
}

void PropertiesPanel::OnSignalExplorerSelectionRefresh()
{
	m_pPropertyTree->revert();
}

#include <moc_PropertiesPanel.cpp>