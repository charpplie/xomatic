/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id: StdAfx.cpp,v 1.0 2009/01/12 15:18:23 SergeySokov Exp wwwrun $
$DateTime$
Description:  source file that includes just the standard includes
-------------------------------------------------------------------------
History:
- 12:01:2009 15:18 : Created by Sergey Sokov
*************************************************************************/

#include "stdafx.h"
#include "ResourceCompiler.h"
#include "PerforceSourceControl.h"

HMODULE g_hInst;

ICrySourceControl* _stdcall CreateSourceControl()
{
	return CPerforceSourceControl::Create();
}

void _stdcall DestroySourceControl(ICrySourceControl* pSC)
{
	CPerforceSourceControl::Destroy(pSC);
}


BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_hInst = (HMODULE)hModule;
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}
