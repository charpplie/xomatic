// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "TreePanel.h"

#include <QPropertyTree/QPropertyTree.h>
#include <QPropertyTree/QPropertyTreeStyle.h>

TreePanel::TreePanel()
	: propertiesAttachedToDocument( false )
{
	m_propertyTree = new QPropertyTree( this );
	QPropertyTreeStyle treeStyle;
	treeStyle.levelIndent = 1.0f;
	treeStyle.firstLevelIndent = 1.0f;

	m_propertyTree->setTreeStyle( treeStyle );

	setWidget( m_propertyTree );
	setFeatures( ( QDockWidget::DockWidgetFeatures )( QDockWidget::AllDockWidgetFeatures & ~( QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable ) ) );
	connect( m_propertyTree, SIGNAL( signalChanged() ), this, SLOT( OnPropertyTreeDataChanged() ) );
}

void TreePanel::Reset()
{
	m_propertyTree->detach();
	propertiesAttachedToDocument = false;
	m_behaviorTreeDocument.Reset();
}

void TreePanel::OnWindowEvent_NewFile()
{
	if( !CheckForUnsavedDataAndSave() )
		return;

	QString gameFolder = QString::fromLocal8Bit( GetIEditor()->GetSystem()->GetIPak()->GetGameFolder() );
	QDir behaviorFolder( QDir::fromNativeSeparators( gameFolder + "/libs/ai/behavior_trees/" ) );

	QString absoluteFilePath = QFileDialog::getSaveFileName( this, "Modular Behavior Tree", behaviorFolder.absolutePath(), "XML files (*.xml)" );

	if( absoluteFilePath.isEmpty() )
		return;

	Reset();

	QFileInfo fileInfo( absoluteFilePath );

	m_behaviorTreeDocument.NewFile( fileInfo.baseName().toStdString().c_str(), absoluteFilePath.toStdString().c_str() );

	if( !propertiesAttachedToDocument )
	{
		m_propertyTree->attach( Serialization::SStruct( m_behaviorTreeDocument ) );
		propertiesAttachedToDocument = true;
	}
}

void TreePanel::OnWindowEvent_OpenFile()
{
	if( !CheckForUnsavedDataAndSave() )
		return;

	QString gameFolder = QString::fromLocal8Bit( GetIEditor()->GetSystem()->GetIPak()->GetGameFolder() );
	QDir behaviorFolder( QDir::fromNativeSeparators( gameFolder + "/libs/ai/behavior_trees/" ) );

	QString absoluteFilePath = QFileDialog::getOpenFileName( this, "Modular Behavior Tree", behaviorFolder.absolutePath(), "XML files (*.xml)" );

	if( absoluteFilePath.isEmpty() )
		return;

	Reset();

	QFileInfo fileInfo( absoluteFilePath );

	if( !m_behaviorTreeDocument.OpenFile( fileInfo.baseName().toStdString().c_str(), absoluteFilePath.toStdString().c_str() ) )
		return;

	if( !propertiesAttachedToDocument )
	{
		m_propertyTree->attach( Serialization::SStruct( m_behaviorTreeDocument ) );
		propertiesAttachedToDocument = true;
	}
}

void TreePanel::OnWindowEvent_Save()
{
	if( m_behaviorTreeDocument.Loaded() && m_behaviorTreeDocument.Changed() )
		m_behaviorTreeDocument.Save();
}

void TreePanel::OnWindowEvent_SaveToFile()
{
	if( !m_behaviorTreeDocument.Loaded() && !m_behaviorTreeDocument.Changed() )
		return;

	QString gameFolder = QString::fromStdString( GetIEditor()->GetSystem()->GetIPak()->GetGameFolder() );
	QDir behaviorFolder( QDir::fromNativeSeparators( gameFolder + "/libs/ai/behavior_trees/" ) );

	QString absoluteFilePath = QFileDialog::getSaveFileName( this, "Modular Behavior Tree", behaviorFolder.absolutePath(), "XML files (*.xml)" );

	if( absoluteFilePath.isEmpty() )
		return;

	QFileInfo fileInfo( absoluteFilePath );

	m_behaviorTreeDocument.SaveToFile( fileInfo.baseName().toStdString().c_str(), absoluteFilePath.toStdString().c_str() );

	m_propertyTree->revert();
}

void TreePanel::OnPropertyTreeDataChanged()
{
	m_behaviorTreeDocument.SetChanged();
}

bool TreePanel::HandleCloseEvent()
{
	return CheckForUnsavedDataAndSave();
}

bool TreePanel::CheckForUnsavedDataAndSave()
{
	if( !m_behaviorTreeDocument.Loaded() || !m_behaviorTreeDocument.Changed() )
		return true;

	UnsavedChangesDialog saveDialog;
	saveDialog.exec();
	if( saveDialog.result == UnsavedChangesDialog::Yes )
	{
		m_behaviorTreeDocument.Save();
		return true;
	}
	else if( saveDialog.result == UnsavedChangesDialog::Cancel )
	{
		return false;
	}

	return true;
}

#include "moc_TreePanel.cpp"
