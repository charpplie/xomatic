#include "stdafx.h"
#include <afxwin.h>
#include <afxdllx.h>

static AFX_EXTENSION_MODULE TelemetryPluginDLL = { NULL, NULL };

extern "C" int APIENTRY DllMain2(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		if (!AfxInitExtensionModule(TelemetryPluginDLL, hInstance))
			return 0;

		new CDynLinkLibrary(TelemetryPluginDLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		AfxTermExtensionModule(TelemetryPluginDLL);
	}
	return 1;   // ok
}
