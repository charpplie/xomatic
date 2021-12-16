/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id: StdAfx.cpp,v 1.0 2008/02/14 15:18:23 AntonKaplanyan Exp wwwrun $
$DateTime$
Description:  source file that includes just the standard includes
-------------------------------------------------------------------------
History:
- 14:2:2008 15:18 : Created by Anton Kaplanyan
*************************************************************************/

#include "stdafx.h"
#include "ResourceCompiler.h"
#include "CryCompressorRC.h"

// Must be included only once in DLL module.
#include <platform_implRC.h>


HMODULE g_hInst;

ICryCompressorRC* _stdcall RegisterCompressor(IResourceCompiler* rc)
{
	return CTextureCompressor::Create(rc);
}


BOOL APIENTRY DllMain( HMODULE hModule,
											DWORD  ul_reason_for_call,
											LPVOID lpReserved
											)
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
