#pragma once
#include <QMainWindow>
#include <IEditor.h>
#include <Include/IPlugin.h>

class QViewport;

class CExampleWindow : public QMainWindow, public IEditorNotifyListener
{
	Q_OBJECT
public:

	CExampleWindow();
	~CExampleWindow();

	void OnEditorNotifyEvent( EEditorNotifyEvent ev ) override;
	
private:
	QViewport* m_viewport;
};
