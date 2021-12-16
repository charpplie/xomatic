#include "stdafx.h"
#include "platform.h"

#pragma optimize("", off)
#pragma warning(disable: 4266) // disabled warning from afk overrides

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS 
#include <afxwin.h>
#include <vector>

#include "QtViewPane.h"

#include "Include/IViewPane.h"
#include "Util/RefCountBase.h"

#include <QWidget>
#include <QEvent>
#include <QCoreApplication>
#include <QCloseEvent>
#if QT_VERSION >= 0x50000
#include <QWindow>
#include <QGuiApplication>
#include <5.2.1/QtGui/qpa/qplatformnativeinterface.h>
#endif

// ugly dependencies:
#pragma warning(push)
#pragma warning(disable: 4244) // warning C4244: 'argument' : conversion from 'A' to 'B', possible loss of data
#include "Functor.h"
class CXmlArchive;
#include <IRenderer.h>
#include "Util/PathUtil.h"
#pragma warning(pop)
#include "QtQml/QQmlEngine.h"
// ^^^

// ---------------------------------------------------------------------------
IMPLEMENT_DYNAMIC(CQtViewPaneBase, CWnd)

#define WM_FRAME_CAN_CLOSE (WM_APP + 1000)

BEGIN_MESSAGE_MAP(CQtViewPaneBase, CWnd)
	ON_MESSAGE( WM_FRAME_CAN_CLOSE, OnFrameCanClose )
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

CQtViewPaneBase::CQtViewPaneBase(QWidget* widget)
: m_window(widget)
{
	BOOL created = CreateEx(WS_EX_WINDOWEDGE, ClassName(), "Character Tool", WS_CHILD | WS_CLIPCHILDREN, CRect(0, 0, 0, 0), AfxGetMainWnd(), 0);
	if (!created)
		return;
}

CQtViewPaneBase::~CQtViewPaneBase()
{
	delete m_window;
	m_window = 0;
}

BOOL CQtViewPaneBase::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg && pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST)
	{
		// All key presses are translated by this frame window

		::TranslateMessage(pMsg);
		::DispatchMessage(pMsg);

		return TRUE;
	}
	return FALSE;
}

void CQtViewPaneBase::PostNcDestroy()
{
	CWnd::PostNcDestroy();

	DeleteThis();
}

bool CQtViewPaneBase::RegisterWindowClass()
{
	WNDCLASS windowClass = { 0 };

	windowClass.style = CS_DBLCLKS;
	windowClass.lpfnWndProc = &AfxWndProc;
	windowClass.hInstance = AfxGetInstanceHandle();
	windowClass.hIcon = NULL;
	windowClass.hCursor = NULL;
	windowClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	windowClass.lpszMenuName = NULL;

	windowClass.lpszClassName = ClassName();

	return AfxRegisterClass(&windowClass) ? true : false;
}

void CQtViewPaneBase::UnregisterWindowClass()
{
}

//--------------------------------------------------------------

int CQtViewPaneBase::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	m_window->setWindowFlags(Qt::Widget);
	HWND child = (HWND)m_window->winId();
	::SetWindowLong(child, GWL_STYLE, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
#if QT_VERSION >= 0x50000
	QWindow *qwindow = m_window->windowHandle();
	qwindow->setProperty("_q_embedded_native_parent_handle", (WId)GetSafeHwnd());
	HWND h = static_cast<HWND>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow("handle", qwindow));
	::SetParent(h, GetSafeHwnd());
	qwindow->setFlags(Qt::FramelessWindowHint);
#else
	::SetParent(child, GetSafeHwnd());
#endif

	QEvent e(QEvent::EmbeddingControl);
	QCoreApplication::sendEvent(m_window, &e);
	m_window->show();
	return 0;
}

void CQtViewPaneBase::OnSetFocus(CWnd* oldWnd)
{
	if (m_window)
		::SetFocus((HWND)m_window->winId());
}

void CQtViewPaneBase::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_window)
	{
		if (::GetFocus() != (HWND)m_window->winId()) 
		{
      m_window->setFocus(Qt::MouseFocusReason);
		}
	}
	__super::OnLButtonDown(nFlags, point);
}


void CQtViewPaneBase::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if (m_window && cx != 0 && cy != 0)
	{
		HWND hwnd = (HWND)m_window->winId();
		::MoveWindow(hwnd, 0, 0, cx, cy, TRUE);
	}
}

LRESULT CQtViewPaneBase::OnFrameCanClose(WPARAM wParam, LPARAM lParam)
{
	std::auto_ptr<QCloseEvent> closeEvent(new QCloseEvent);
	BOOL* canClose = (BOOL*)wParam;
	QCoreApplication::sendEvent(m_window, closeEvent.get());
	*canClose = closeEvent->isAccepted() ? TRUE : FALSE;
	return TRUE;
}

// ---------------------------------------------------------------------------

