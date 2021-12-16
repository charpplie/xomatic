////////////////////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// ----------------------------------------------------------------------------------------
//  File name:   PropertiesPanel
//  Description: Panel which shows properties of selected item
//
//////////////////////////////////////////////////////////////////////////////////////////// 

#ifndef _PROPERTIES_PANEL_H_
#define _PROPERTIES_PANEL_H_

#pragma once

#include <QDockWidget>

class QPropertyTree;
class QSplitter;
class AppDocument;

class PropertiesPanel : public QDockWidget
{
	Q_OBJECT

public:
	PropertiesPanel(QWidget* pParent, AppDocument& appDocument);

public slots:
	void OnSignalExplorerSelectionChanged();
	void OnSignalExplorerSelectionRefresh();

private:
	
	QPropertyTree*    m_pPropertyTree;

	AppDocument& m_appDocument;
};

#endif // _PROPERTIES_PANEL_H_