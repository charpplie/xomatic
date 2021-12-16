// (c) 2001-2012 Crytek GmbH
#include "SandboxAPI.h"
#include "StdAfx.h"
#include "EditorCommonAPI.h"


#include <memory>
using std::auto_ptr;

#include "platform_impl.h"
#include "Include/IEditorClassFactory.h"
#include "IEditor.h"
#include "Include/IEventLoopHook.h"

// we need this pile of crap...
class CXmlArchive;
struct SRayHitInfo;
#include <Cry_Geo.h>

#include <QApplication>
#include <QWidget>
#include <QAbstractEventDispatcher>
#include <QStyleFactory>
#if QT_VERSION >= 0x50000
#include <QFile>
#include <QAbstractNativeEventFilter>
#else
#include <QString>
#endif

#include "MFCPropertyTree.h"
#define STYLE_SHEET_PATH "Editor/Styles/stylesheet.qss"
#include "Events/EventManager.h"

static IEditor* g_editor;
IEditor* GetIEditor() { return g_editor; }

class CMFCQtApplication : public QApplication
#if QT_VERSION >= 0x50000
, public QAbstractNativeEventFilter
#endif
, public IEditorNotifyListener
{
public:
	CMFCQtApplication()
	: QApplication(__argc, __argv)
	, m_inWinEventFilter(false)
	{
#if QT_VERSION >= 0x50000
		if (QAbstractEventDispatcher::instance())
			QAbstractEventDispatcher::instance()->installNativeEventFilter(this);
#else
		QApplication::installEventFilter(this);
#endif
		GetIEditor()->RegisterNotifyListener(this);

		UpdatePalette();
	}

	~CMFCQtApplication()
	{
#if QT_VERSION >= 0x50000
		if (QAbstractEventDispatcher::instance() )
			QAbstractEventDispatcher::instance()->removeNativeEventFilter(this);
#endif
	}

#if QT_VERSION >= 0x50000
	bool nativeEventFilter(const QByteArray & eventType, void * message, long * result) override
	{
		MSG* msg = (MSG*)message;
#else
	bool winEventFilter(MSG* msg, long* result) override
	{
#endif
		if (m_inWinEventFilter)
			return false;

		m_inWinEventFilter = true;

		QWidget* widget = QWidget::find((WId)msg->hwnd);
		if (widget == 0)
		{
			if (AfxGetApp() && AfxGetApp()->PreTranslateMessage(msg))
			{
				m_inWinEventFilter = false;
				return true;
			}
		}
		
		m_inWinEventFilter = false;
#if QT_VERSION >= 0x50000
	return false;
#else
	return QApplication::winEventFilter(msg, result);
#endif
	}

	void OnEditorNotifyEvent(EEditorNotifyEvent event) override
	{
		if (event == eNotify_OnStyleChanged)
		{
			UpdatePalette();
		}
		else if (event == eNotify_OnQuit)
		{
			GetIEditor()->UnregisterNotifyListener(this);
		}
	}

	static QColor SysColorToQt(int id)
	{
		unsigned int color = GetSysColor(id);
		return QColor::fromRgba(0xff000000 | (color & 0xff00) | ((color & 0xff) << 16) | ((color & 0xff0000) >> 16));
	}

	static QColor InterpolateColors(QColor a, QColor b, float factor)
	{
		return QColor(int(a.red() * (1.0f - factor) + b.red() * factor),
					  int(a.green() * (1.0f - factor) + b.green() * factor),
					  int(a.blue() * (1.0f - factor) + b.blue() * factor),
					  int(a.alpha() * (1.0f - factor) + b.alpha() * factor));
	}

	void UpdatePalette()
	{
		QPalette palette;
		palette.setColor(QPalette::WindowText,		SysColorToQt(COLOR_WINDOWTEXT));
		palette.setColor(QPalette::ButtonText,		SysColorToQt(COLOR_BTNTEXT));
		palette.setColor(QPalette::Text,			SysColorToQt(COLOR_WINDOWTEXT));
		palette.setColor(QPalette::Light,           InterpolateColors(SysColorToQt(COLOR_3DLIGHT), SysColorToQt(COLOR_BTNFACE), 0.75f));
		palette.setColor(QPalette::Midlight,        InterpolateColors(SysColorToQt(COLOR_3DLIGHT), SysColorToQt(COLOR_BTNFACE), 0.6f));
		palette.setColor(QPalette::Button,          SysColorToQt(COLOR_BTNFACE));
		palette.setColor(QPalette::Dark,            InterpolateColors(SysColorToQt(COLOR_BTNFACE), SysColorToQt(COLOR_3DDKSHADOW), 0.33f));
		palette.setColor(QPalette::Mid,             InterpolateColors(SysColorToQt(COLOR_BTNFACE), SysColorToQt(COLOR_3DDKSHADOW), 0.66f));
		palette.setColor(QPalette::Shadow,          SysColorToQt(COLOR_3DDKSHADOW));
		palette.setColor(QPalette::BrightText,      SysColorToQt(COLOR_HIGHLIGHTTEXT));
		palette.setColor(QPalette::Base,            SysColorToQt(COLOR_WINDOW));
		palette.setColor(QPalette::AlternateBase,   SysColorToQt(COLOR_BTNSHADOW));
		palette.setColor(QPalette::Window,          SysColorToQt(COLOR_BTNFACE));
		palette.setColor(QPalette::Shadow,          SysColorToQt(COLOR_3DSHADOW));
		palette.setColor(QPalette::Highlight,       SysColorToQt(COLOR_HIGHLIGHT));
		palette.setColor(QPalette::HighlightedText, SysColorToQt(COLOR_HIGHLIGHTTEXT));

		QColor disabledTextColor = InterpolateColors(SysColorToQt(COLOR_GRAYTEXT), SysColorToQt(COLOR_BTNFACE), 0.4f);
		palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledTextColor);
		palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledTextColor);
		palette.setColor(QPalette::Disabled, QPalette::Text, disabledTextColor);

		palette.setColor(QPalette::Disabled, QPalette::Light,           InterpolateColors(SysColorToQt(COLOR_3DLIGHT), SysColorToQt(COLOR_BTNFACE), 0.5f));
		palette.setColor(QPalette::Disabled, QPalette::Midlight,        InterpolateColors(SysColorToQt(COLOR_3DLIGHT), SysColorToQt(COLOR_BTNFACE), 0.4f));
		palette.setColor(QPalette::Disabled, QPalette::Dark,            InterpolateColors(SysColorToQt(COLOR_BTNFACE), SysColorToQt(COLOR_3DDKSHADOW), 0.11f));
		palette.setColor(QPalette::Disabled, QPalette::Mid,             InterpolateColors(SysColorToQt(COLOR_BTNFACE), SysColorToQt(COLOR_3DDKSHADOW), 0.44f));

		setPalette(palette);
	}

private:

	bool m_inWinEventFilter;
};

// This ugly construction is used to check if QApplication actually consumes
// WM_QUIT or not Looks like there is no other way to detect it without
// modifying Qt code, or switching to QApplication::exec, i.e. replacing MFC
// message loop with the one from Qt.
static HHOOK g_getMessageHook;
static bool g_hookDetectedQuitMessage;
static LRESULT CALLBACK GetMessageHookForQuitMessge(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSG* msg = (MSG*)lParam;
	if (msg && msg->message == WM_QUIT)
	{
		g_hookDetectedQuitMessage = true;
	}

  return CallNextHookEx(g_getMessageHook, nCode, wParam, lParam);
}

#if QT_VERSION >= 0x50000
static void LogToDebug(QtMsgType Type, const QMessageLogContext& Context, const QString &message)
{
	OutputDebugStringW(L"Qt: ");
	OutputDebugStringW(reinterpret_cast<const wchar_t *>(message.utf16()));
	OutputDebugStringW(L"\n");
}
#endif

class CQtEventLoopHook : public IEventLoopHook
{
public:
	CQtEventLoopHook(IEditor* editor)
	: m_editor(editor)
	{
		m_application.reset(new CMFCQtApplication());
		qInstallMessageHandler(LogToDebug);

#if QT_VERSION >= 0x50000
		m_application->setStyle(QStyleFactory::create("Fusion"));
		qInstallMessageHandler(LogToDebug);
		QFile styleSheetFile(STYLE_SHEET_PATH);
		if (styleSheetFile.open(QFile::ReadOnly))
		{
			m_application->setStyleSheet(styleSheetFile.readAll());
		}
#else
		QAbstractEventDispatcher::instance()->setEventFilter(MFCEventFilter);
		m_application->setStyle(QStyleFactory::create("Plastique"));
#endif

		editor->RegisterEventLoopHook(this);

		g_getMessageHook = SetWindowsHookEx(WH_GETMESSAGE, GetMessageHookForQuitMessge, 0, GetCurrentThreadId());
	}

	~CQtEventLoopHook()
	{
		if (g_getMessageHook)
			UnhookWindowsHookEx(g_getMessageHook);

		if (m_editor)
		{
			m_editor->UnregisterEventLoopHook(this);
			m_editor = 0;
		}
		m_application.reset();
	}

#if QT_VERSION < 0x50000
	static bool MFCEventFilter(void* message)
	{
		long result = 0;
		return QApplication::instance()->winEventFilter((MSG*)message, &result);
	}
#endif

	bool PrePumpMessage() override
	{
		g_hookDetectedQuitMessage = false;

		// since we are running our own event loop, it is 
		// recommended to handle deferred delete events manually
		m_application->sendPostedEvents(0, QEvent::DeferredDelete);
		m_application->processEvents();

		if (g_hookDetectedQuitMessage)
		{
			// QApplication just consumed WM_QUIT, 
			// we prevent it from doing it again...
			if (m_editor)
			{
				m_editor->UnregisterEventLoopHook(this);
				m_editor = 0;
			}

			// ...and repost WM_QUIT for MFC app.
			// Previous exit code is lost here, but this should be no issue, since we
			// are using exit() for termination.
			PostQuitMessage(0);
		}
		return true;
	}

private:
	auto_ptr<CMFCQtApplication> m_application;
	IEditor* m_editor;
};

namespace
{
	CEventManager g_eventManager(gEnv);
}
static CQtEventLoopHook* g_eventLoopHook;
static int g_userCount;

bool EDITOR_COMMON_API InitializeQt(IEditor* editor)
{
	if (!AfxGetApp())
	{
		editor->GetSystem()->GetILog()->Log("EditorCommon: failed to initialize Qt support. AfxGetApp() returns NULL. Happens when Editor and plugins built in different configurations.");
		return false;
	}

	if (g_userCount == 0)
	{
		g_editor = editor;
		g_eventLoopHook = new CQtEventLoopHook(editor);

		RegisterMFCPropertyTreeClass();
	}

	++g_userCount;
	return true;
}

void EDITOR_COMMON_API FinalizeQt()
{
	if (g_userCount == 0)
	{
		ASSERT(0 && "Uneven number of InitializeQt/FinalizeQt calls");
		return;
	}

	--g_userCount;

	if (g_userCount == 0)
	{
		UnregisterMFCPropertyTreeClass();

		g_editor = 0;
		delete g_eventLoopHook;
		g_eventLoopHook = 0;
	}
}


