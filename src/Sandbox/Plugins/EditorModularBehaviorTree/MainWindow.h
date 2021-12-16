// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#ifndef MainWindow_h
#define MainWindow_h

#pragma once

#include <QMainWindow>
#include <IEditor.h>
#include <Include/IPlugin.h>

#include "TreePanel.h"

class MainWindow : public QMainWindow, public IEditorNotifyListener
{
	Q_OBJECT
public:
	MainWindow();
	~MainWindow();

	void OnEditorNotifyEvent( EEditorNotifyEvent editorNotifyEvent ) override;

protected:
	void closeEvent( QCloseEvent* closeEvent ) OVERRIDE;

protected slots:
	void OnMenuActionNew();
	void OnMenuActionOpen();
	void OnMenuActionSave();
	void OnMenuActionSaveToFile();

private:
	TreePanel* m_treePanel;
};

#endif // MainWindow_h