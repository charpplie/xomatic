#include "platform.h"
#include "ExampleWindow.h"
#include <QDockWidget>
// #include <QWebView>

#include "../EditorCommon/QPropertyTree/QPropertyTree.h"
#include "../EditorCommon/QViewport.h"
	
CExampleWindow::CExampleWindow()
{
	m_viewport = new QViewport(gEnv, this);
	setCentralWidget(m_viewport);
	
	// QWebView* webView = new QWebView(this);
	// webView->load(QUrl("http://images.google.com"));

	// QDockWidget* webDock = new QDockWidget("WebView");
	// webDock->setWidget(webView);
	// addDockWidget(Qt::BottomDockWidgetArea, webDock, Qt::Horizontal);

	QPropertyTree* tree = new QPropertyTree(this);
	tree->attach(Serialization::SStruct(*m_viewport));

	QDockWidget* dock = new QDockWidget("Properties");
	dock->setWidget(tree);
	addDockWidget(Qt::RightDockWidgetArea, dock, Qt::Vertical);

	GetIEditor()->RegisterNotifyListener(this);
}

CExampleWindow::~CExampleWindow()
{
	GetIEditor()->UnregisterNotifyListener(this);
}

void CExampleWindow::OnEditorNotifyEvent( EEditorNotifyEvent ev )
{
	if (ev == eNotify_OnIdleUpdate)
		m_viewport->Update();
}

#include <moc_ExampleWindow.cpp>
