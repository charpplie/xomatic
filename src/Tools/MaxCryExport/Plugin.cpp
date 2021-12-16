//===========================================================================*\
//	Part of Crytek Character Studio and Object Export Plug-in
//
//  Copyright: © Cuneyt Ozdas 2000, 2001
//				 cuneyt@cuneytozdas.com
//===========================================================================*/

#include "StdAfx.h"
#include "CGF/Utility.h"

#include "CGF/CryImporter.h"

HINSTANCE hInstance;
int controlsInit = FALSE;

extern ClassDesc* GetCryShaderCD();
extern ClassDesc* GetBonesDefModDesc();
extern ClassDesc* GetExportStubDesc();

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) 
{
	hInstance = hinstDLL;

	// Initialize an ATL com module so that we can create instances of CComObject later.
	{
		static CComModule _Module;
		_pAtlModule=&_Module;
	}

	if ( !controlsInit )
	{
		controlsInit = TRUE;
		InitCustomControls(hInstance);
		InitCommonControls();
	}

	switch(fdwReason) 
	{
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return(TRUE);
}


const TCHAR *
LibDescription() 
{ 
	return GetString(IDS_LIBDESC); 
}


int LibNumberClasses() 
{
	return 5;
}


ClassDesc* LibClassDesc(int i) 
{
	switch(i) 
	{
		case 0: return GetCSExportUtilDesc();
		case 1: return GetCryImporterDesc();
		case 2: return GetCryShaderCD();
		case 3: return GetBonesDefModDesc();
		case 4: return GetExportStubDesc();
		default: return 0;
	}
}


ULONG LibVersion() 
{ 
	return VERSION_3DSMAX; 
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256];
	if(hInstance)
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;

	return NULL;
}
