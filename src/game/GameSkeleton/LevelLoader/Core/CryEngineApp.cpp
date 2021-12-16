#include "StdAfx.h"
#include "../Resources/Resource.h"
#include "../LevelLoader/LevelLoader.h"
#include "CryEngineApp.h"

// This header must be included only once! in each module or executable
#include <platform_impl.h>

extern HINSTANCE g_hInstance;

void Error( const char* sErrorText )
{
	MessageBox( 0,sErrorText, "Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// ctor & dtor

static const Vec3 s_vStartPos(1326, 1346, 140);
static const Ang3 s_aStartAng(DEG2RAD( -10 ), 0, DEG2RAD( 17 ));

CCryEngineApp::CCryEngineApp() :
	m_hSystemHandle(0),
	m_pISystem(0),
	m_vCamPos(s_vStartPos),
	m_vCamAng(s_aStartAng)
{

}

//-----------------------------------

CCryEngineApp::~CCryEngineApp()
{
	if (m_hSystemHandle)
		CryFreeLibrary(m_hSystemHandle);
}

// ctor & dtor/
//////////////////////////////////////////////////////////////////////////

bool CCryEngineApp::InitCrySystem( const char* sInCmdLine )
{
	if (!InitWindowClass())
	{
		Error( "Failed to initialize window class" );
	}

	m_hSystemHandle = LoadLibrary( "CrySystem.dll" );
	if (!m_hSystemHandle)
	{
		Error( "CrySystem.DLL Loading Failed" );
		return false;
	}

	PFNCREATESYSTEMINTERFACE pfnCreateSystemInterface = (PFNCREATESYSTEMINTERFACE)::GetProcAddress( m_hSystemHandle,DLL_INITFUNC_SYSTEM );

	SSystemInitParams sip;
	sip.bEditor = false;
	sip.bDedicatedServer = false;
	sip.bPreview = false;
	sip.bTestMode = false;
	sip.hInstance = g_hInstance;

	sip.sLogFileName = "LevelLoader.log";
	strcpy( sip.szSystemCmdLine,sInCmdLine );

	m_pISystem = pfnCreateSystemInterface( sip );
	if (!m_pISystem)
	{
		Error( "CreateSystemInterface Failed" );

		return false;
	}

	// Initialized important global variables like GetISystem() gEnv etc.. 
	ModuleInitISystem(m_pISystem,"Launcher");

	// System is fully initialized and ready by now.

	gEnv->pInput->AddEventListener(this);

	return true;
}

//-----------------------------------

bool CCryEngineApp::InitWindowClass()
{
	HINSTANCE hInstance = g_hInstance;

	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.style		 = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc   = (WNDPROC)CCryEngineApp::WndProc;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 0;
	wc.hInstance	 = GetModuleHandle(0);
	wc.hIcon		 = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SAMPLE1));
	wc.hCursor	   = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground =(HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = CRYENGINE_WINDOW_CLASSNAME;

	if (!RegisterClass(&wc))
	{
		return false;
	}
	//::ShowCursor(FALSE);
	return true;
}

//-----------------------------------

LRESULT CALLBACK CCryEngineApp::WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch(msg)
	{
	case WM_CLOSE:
		if (gEnv && gEnv->pSystem)
			gEnv->pSystem->Quit();
		return 0;
	case WM_HOTKEY:
	case WM_SYSCHAR:	// prevent ALT + key combinations from creating 'ding' sounds
		return  0;
	case WM_CHAR:
		{
			if (gEnv && gEnv->pInput)
			{
				SInputEvent event;
				event.modifiers = gEnv->pInput->GetModifiers();
				event.deviceId = eDI_Keyboard;
				event.state = eIS_UI;
				event.value = 1.0f;
				event.pSymbol = 0;//m_rawKeyboard->GetSymbol((lParam>>16)&0xff);
				if (event.pSymbol)
					event.keyId = event.pSymbol->keyId;

				wchar_t tmp[2] = { 0 };
				MultiByteToWideChar(CP_ACP, 0, (char*)&wParam, 1, tmp, 2);
				event.timestamp = tmp[0];

				char szKeyName[4] = {0};
				if (wctomb(szKeyName, (WCHAR)wParam) != -1)
				{
					if (szKeyName[1]==0 && ((unsigned char)szKeyName[0])>=32)
					{
						event.keyName = szKeyName;
						gEnv->pInput->PostInputEvent(event);
					}
				}
			}
		}
		break;
	case WM_MOVE:
		if(gEnv && gEnv->pSystem)
		{
			gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_MOVE,LOWORD(lParam), HIWORD(lParam));
		}
		break;
	case WM_SIZE:
		if(gEnv && gEnv->pSystem)
		{
			gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_RESIZE,LOWORD(lParam), HIWORD(lParam));
		}
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

//-----------------------------------

int CCryEngineApp::MainLoop()
{
	bool bQuit = false;
	// Main message loop and per frame engine update
	MSG msg;
	while (!bQuit)
	{
		while (PeekMessage(&msg, 0, 0, 0,PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				bQuit = true;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		UpdateCamera();

		// Basic frame update.
		m_pISystem->Update();
		m_pISystem->RenderBegin();
		m_pISystem->Render();
		m_pISystem->RenderEnd();
	}
	return 0;
}

//-----------------------------------

void CCryEngineApp::UpdateCamera()
{
	CCamera cam = GetISystem()->GetViewCamera();
	cam.SetAngles( m_vCamAng );
	cam.SetPosition( m_vCamPos );
	GetISystem()->SetViewCamera( cam );
}

//////////////////////////////////////////////////////////////////////////

bool CCryEngineApp::OnInputEvent( const SInputEvent &event )
{
	Vec3 vPosUpdate(0,0,0);
	Ang3 vAngUpdate(0,0,0);
	float fPosIncrement = 1.0f;
	float fAngIncrement = 0.05f;

	if(event.state == eIS_Down && event.deviceId == eDI_Keyboard)
	{
		if(event.modifiers & eMM_LShift)
			fPosIncrement *= 10.0f;
		if(event.modifiers & eMM_RShift)
			fPosIncrement *= 0.1f;

		//pos update
		if(event.keyId == eKI_D)
		{
			vPosUpdate.x += fPosIncrement;
		}
		if(event.keyId == eKI_A)
		{
			vPosUpdate.x -= fPosIncrement;
		}
		if(event.keyId == eKI_W)
		{
			vPosUpdate.y += fPosIncrement;
		}
		if(event.keyId == eKI_S)
		{
			vPosUpdate.y -= fPosIncrement;
		}		
		if(event.keyId == eKI_Q)
		{
			vPosUpdate.z += fPosIncrement;
		}
		if(event.keyId == eKI_E)
		{
			vPosUpdate.z -= fPosIncrement;
		}

		//ang update
		if(event.keyId == eKI_NP_8)
		{
			vAngUpdate.x += fAngIncrement;
		}
		if(event.keyId == eKI_NP_2)
		{
			vAngUpdate.x -= fAngIncrement;
		}
		if(event.keyId == eKI_NP_7)
		{
			vAngUpdate.y += fAngIncrement;
		}
		if(event.keyId == eKI_NP_9)
		{
			vAngUpdate.y -= fAngIncrement;
		}
		if(event.keyId == eKI_NP_4)
		{
			vAngUpdate.z += fAngIncrement;
		}
		if(event.keyId == eKI_NP_6)
		{
			vAngUpdate.z -= fAngIncrement;
		}
		//reset with 5
		if(event.keyId == eKI_NP_5)
		{
			m_vCamAng = s_aStartAng;
		}
	}
	m_vCamAng += vAngUpdate;

	Quat qRot;
	qRot.SetRotationXYZ(m_vCamAng);
	vPosUpdate = qRot * vPosUpdate;
	m_vCamPos += vPosUpdate;
	
	return true;
}

//-----------------------------------

bool CCryEngineApp::OnInputEventUI( const SInputEvent &event )
{
	return OnInputEvent(event);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////