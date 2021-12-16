#ifndef __cryengine_h__
#define __cryengine_h__ 

#include "CryEngineApp.h"
#include "../Resources/Resource.h"
#include "../LevelLoader/LevelLoader.h"

// Engine includes.
#include <IInput.h>
#include <IRenderer.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// We need shell api for Current Root Extrection.
#include "shlwapi.h"
#pragma comment(lib, "shlwapi.lib")




#define CRY_SYSTEM_DLL "CrySystem.dll"
#define DLL_INITFUNC_SYSTEM "CreateSystemInterface"

#define CRYENGINE_WINDOW_CLASSNAME "CryENGINE"

// Simple function to display error dialog
void Error( const char* sErrorText );


class CCryEngineApp : 
	public IInputEventListener
{
public:
	CCryEngineApp();
	  ~CCryEngineApp();

	  bool						InitCrySystem( const char* sInCmdLine );
	  bool						InitWindowClass();
	  static LRESULT CALLBACK	WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);	  

	  // Main Engine loop.
	  int	MainLoop();
	  void	UpdateCamera();

	  virtual bool	OnInputEvent( const SInputEvent &event );
	  virtual bool	OnInputEventUI( const SInputEvent &event );

private:
	HMODULE m_hSystemHandle;
	ISystem* m_pISystem;
	Vec3 m_vCamPos;
	Ang3 m_vCamAng;
};



#endif //__cryengine_h__