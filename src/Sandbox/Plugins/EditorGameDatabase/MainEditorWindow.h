////////////////////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// ----------------------------------------------------------------------------------------
//  File name:   MainEditorWindow.h
//  Description: Main application window containing all other UI elements
//
//////////////////////////////////////////////////////////////////////////////////////////// 

#ifndef _MAIN_EDITOR_PANEL_H_
#define _MAIN_EDITOR_PANEL_H_

#pragma once

#include <QMainWindow>
#include <IEditor.h>
#include <Include/IPlugin.h>
#include <Serialization/IArchive.h>

class AppDocument;
class DatabaseExplorer;
class PropertiesPanel;
class EditorLog;
class QDockWidget;
class QPlainTextEdit;

class MainEditorWindow : public QMainWindow, public IEditorNotifyListener
{
	Q_OBJECT
public:

	MainEditorWindow();
	~MainEditorWindow();

	// IEditorNotifyListener
	void OnEditorNotifyEvent( EEditorNotifyEvent ev ) OVERRIDE;
	//~IEditorNotifyListener

	void Serialize( Serialization::IArchive& archive );

protected:
	void OnLogMessage(const char* message);
	void SaveState();
	void LoadState();

	void closeEvent(QCloseEvent* ev) OVERRIDE;

protected slots:
	void OnToolbarActionSaveAll();
	void OnToolbarActionReloadDatabase();

private:

	DatabaseExplorer* m_pExplorer;
	QDockWidget*      m_pExplorerDock;
	PropertiesPanel*  m_pPropertiesPanel;
	QDockWidget*      m_pLoggingDock;
	QPlainTextEdit*   m_pLoggingPanel;

	std::unique_ptr<AppDocument> m_pDocument;
	std::unique_ptr<EditorLog>   m_pLog;
};

#endif //_MAIN_EDITOR_PANEL_H_

