#include "stdafx.h"
#include "IResCompiler.h"
#include "XMLConverter.h"
#include "IRCLog.h"
#include "XML/xml.h"

// Must be included only once in DLL module.
#include <platform_implRC.h>

HMODULE g_hInst;

XmlStrCmpFunc g_pXmlStrCmp = &stricmp;

BOOL APIENTRY DllMain( HANDLE hModule, 
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

ICryXML* LoadICryXML()
{
	// Find the directory of the current dll.
	char szBuffer[1024];
	GetModuleFileName(g_hInst, szBuffer, sizeof(szBuffer));

	HMODULE hXMLLibrary = LoadLibrary("CryXML.dll");
	if (NULL == hXMLLibrary)
	{
		RCLogError("Unable to load xml library (CryXML.dll).");
		return 0;
	}

	FnGetICryXML pfnGetICryXML = (FnGetICryXML)GetProcAddress(hXMLLibrary, "GetICryXML");
	if (pfnGetICryXML == 0)
	{
		RCLogError("Unable to load xml library (CryXML.dll) - cannot find exported function GetICryXML().");
		return 0;
	}

	return pfnGetICryXML();
}

void __stdcall RegisterConvertors( IResourceCompiler *pRC )
{
	SetRCLog(pRC->GetIRCLog());

	ICryXML* pCryXML = LoadICryXML();
	if (pCryXML == 0)
	{
		RCLogError("Loading xml library failed - not registering xml converter.");
		return;
	}

	pCryXML->AddRef();

	pRC->RegisterConvertor(new XMLConverter(pCryXML));

	pRC->RegisterKey("ext_xml","ignore or copy xml files");	

	pCryXML->Release();
}

