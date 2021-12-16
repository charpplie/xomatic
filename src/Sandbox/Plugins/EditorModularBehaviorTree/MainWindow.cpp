// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "MainWindow.h"

#include <IEditor.h>
#include <ICryPak.h>

#include <QDockWidget>
#include <QAction>

#include <QFileDialog>

#include <QMenuBar>

#include "TreePanel.h"

MainWindow::MainWindow()
{
	setWindowFlags( Qt::Widget );

	QMenuBar* menu = new QMenuBar( this );
	QMenu* fileMenu = menu->addMenu( "&File" );
	connect( fileMenu->addAction( "&New" ), SIGNAL( triggered() ), this, SLOT( OnMenuActionNew() ) );
	connect( fileMenu->addAction( "&Open" ), SIGNAL( triggered() ), this, SLOT( OnMenuActionOpen() ) );
	// todo : add recent files menu
	connect( fileMenu->addAction( "&Save" ), SIGNAL( triggered() ), this, SLOT( OnMenuActionSave() ) );
	connect( fileMenu->addAction( "Save &as..." ), SIGNAL( triggered() ), this, SLOT( OnMenuActionSaveToFile() ) );

	setMenuBar(menu);

	GetIEditor()->RegisterNotifyListener( this );

	m_treePanel = new TreePanel();
	setCentralWidget( m_treePanel );
}

MainWindow::~MainWindow()
{
	GetIEditor()->UnregisterNotifyListener( this );
}

void MainWindow::OnEditorNotifyEvent( EEditorNotifyEvent editorNotifyEvent )
{
}

void MainWindow::closeEvent( QCloseEvent* closeEvent )
{
	if( !m_treePanel->HandleCloseEvent() )
		return;

	QMainWindow::closeEvent( closeEvent );
}

void MainWindow::OnMenuActionNew()
{
	m_treePanel->OnWindowEvent_NewFile();
}

void MainWindow::OnMenuActionOpen()
{
	m_treePanel->OnWindowEvent_OpenFile();
}

void MainWindow::OnMenuActionSave()
{
	m_treePanel->OnWindowEvent_Save();
}

void MainWindow::OnMenuActionSaveToFile()
{
	m_treePanel->OnWindowEvent_SaveToFile();
}

#include "moc_MainWindow.cpp"
