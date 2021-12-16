//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "MainEditorWindow.h"

#include <QDockWidget>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QToolBar>
#include <QAction>
#include <QPlainTextEdit>
#include <QDir>
#include <QByteArray>

#include <Descriptor/IObject.h>

#include <Serialization/IArchive.h>
#include <Serialization/IArchiveHost.h>
#include <Serialization/Qt.h>

#include "AppDocument.h"
#include "DatabaseExplorer.h"
#include "PropertiesPanel.h"
#include "InputDialog.h"
#include "EditorLog.h"

#define APPLICATION_USER_DIRECTORY      "/HuntDatabase"
#define APPLICATION_USER_STATE_FILEPATH "/HuntDatabase/EditorState.json"

#define APPLICATION_DOCK_PANE_LOG_NAME      "LogDockPane"
#define APPLICATION_DOCK_PANE_EXPLORER_NAME "ExplorerDockPane"
#define APPLICATION_TOOL_BAR_NAME           "Toolbar"

MainEditorWindow::MainEditorWindow( )
	: m_pLoggingPanel(NULL)
{
	setWindowFlags(Qt::Widget);

	m_pLoggingPanel = new QPlainTextEdit();
	m_pLoggingPanel->setReadOnly(true);

	m_pLog.reset( new EditorLog(functor(*this, &MainEditorWindow::OnLogMessage)) );
	m_pDocument.reset( new AppDocument() );
	m_pExplorer = new DatabaseExplorer(this, *m_pDocument.get());
	m_pPropertiesPanel = new PropertiesPanel(this, *m_pDocument.get());

	// Add database explorer
	m_pExplorerDock = new QDockWidget("Explorer");
	m_pExplorerDock->setObjectName(APPLICATION_DOCK_PANE_EXPLORER_NAME);
	m_pExplorerDock->setWidget(m_pExplorer);

	addDockWidget(Qt::LeftDockWidgetArea, m_pExplorerDock);

	// Add logging area
	m_pLoggingDock  = new QDockWidget("Log");
	m_pLoggingDock->setObjectName(APPLICATION_DOCK_PANE_LOG_NAME);
	m_pLoggingDock->setWidget(m_pLoggingPanel);
	addDockWidget(Qt::BottomDockWidgetArea, m_pLoggingDock);

	// Add properties view
	setCentralWidget(m_pPropertiesPanel);

	// Add custom tool bar
	QToolBar* pToolBar = new QToolBar("Toolbar");
	pToolBar->setObjectName(APPLICATION_TOOL_BAR_NAME);
	{
		QAction* pAction = pToolBar->addAction(QIcon(":/icons/save.png"), QString());
		pAction->setToolTip(QString("Save all"));
		connect(pAction, SIGNAL(triggered()), this, SLOT(OnToolbarActionSaveAll()));

		pAction = pToolBar->addAction(QIcon(":/icons/reload.png"), QString());
		pAction->setToolTip(QString("Reload"));
		connect(pAction, SIGNAL(triggered()), this, SLOT(OnToolbarActionReloadDatabase()));

		addToolBar(pToolBar);
	}

	LoadState();

	GetIEditor()->RegisterNotifyListener(this);
}

MainEditorWindow::~MainEditorWindow()
{
	GetIEditor()->UnregisterNotifyListener(this);

	setCentralWidget(NULL);
	removeDockWidget(m_pExplorerDock);
	removeDockWidget(m_pLoggingDock);

	m_pLoggingPanel = NULL;

	// Make sure to release the document AFTER all widgets have been removed/disabled
	m_pLog.reset();
	m_pDocument.reset();
}

void MainEditorWindow::OnEditorNotifyEvent( EEditorNotifyEvent ev )
{
	if (ev == eNotify_OnQuit)
	{
		SaveState();
	}
}

void MainEditorWindow::Serialize( Serialization::IArchive& archive )
{
	int windowStateVersion = 1;

	QByteArray windowState;
	if (archive.IsOutput())
	{
		windowState = saveState(windowStateVersion);
	}
	archive(windowState, "windowState");
	if (archive.IsInput() && !windowState.isEmpty())
	{
		restoreState(windowState, windowStateVersion);
	}
}

void MainEditorWindow::OnLogMessage(const char* message)
{
	if (m_pLoggingPanel)
	{
		m_pLoggingPanel->appendPlainText(QString(message));
	}
}

void MainEditorWindow::SaveState()
{
	const stack_string saveFilePath = stack_string().Format("%s%s", GetIEditor()->GetUserFolder(), APPLICATION_USER_STATE_FILEPATH).c_str();

	// Create directory first in case it does not exist
	QDir().mkdir(stack_string().Format("%s%s", GetIEditor()->GetUserFolder(), APPLICATION_USER_DIRECTORY).c_str());

	Serialization::SaveJsonFile(saveFilePath.c_str(), *this);
}

void MainEditorWindow::LoadState()
{
	const stack_string saveFilePath = stack_string().Format("%s%s", GetIEditor()->GetUserFolder(), APPLICATION_USER_STATE_FILEPATH).c_str();

	Serialization::LoadJsonFile(*this, saveFilePath.c_str());
}

void MainEditorWindow::closeEvent(QCloseEvent* ev)
{
	SaveState();

	if (!m_pDocument->HasUnsavedData())
		return;

	ConfirmationDialog saveDialog("Save before closing", "There are some unsaved records. Do you want to save them before closing?");
	if(saveDialog.exec())
	{
		m_pDocument->SaveAll();
	}
	else
	{
		m_pDocument->RevertAll();
	}

	QMainWindow::closeEvent(ev);
}

void MainEditorWindow::OnToolbarActionSaveAll()
{
	m_pDocument->SaveAll();
}

void MainEditorWindow::OnToolbarActionReloadDatabase()
{
	ConfirmationDialog reloadDialog("Reload database", "You are about to reload the database, all not saved changes will be lost.\n Do you want to continue?");

	if(!reloadDialog.exec())
		return;

	m_pDocument->GetDatabase().Reload();
}

#include <moc_MainEditorWindow.cpp>

