#include "StdAfx.h"
#include "ExportStub.h"
#include "../CGF/PathUtil.h"
#include "ResourceCompilerHelper.h"

#define EXPORT_STUB_CLASS_ID   Class_ID(0x4ab440a8, 0x53144d7)

ExportStubGUP* g_exportStub = 0;

class ExportStubClassDesc : public ClassDesc2 
{
public:
	int 			IsPublic()					{ return 1; }
	void *			Create( BOOL loading )		{ g_exportStub = new ExportStubGUP(); return g_exportStub; } 
	const TCHAR *	ClassName()					{ return _T("CryENGINE Library"); }
	SClass_ID		SuperClassID()				{ return GUP_CLASS_ID; }
	Class_ID 		ClassID()					{ return EXPORT_STUB_CLASS_ID; }
	const TCHAR* 	Category()					{ return _T("Global Utility Plug-In");  }

	// Hardwired name, used by MAX Script as unique identifier
	const TCHAR*	InternalName()				{ return _T("cryengine2"); }
	HINSTANCE		HInstance()					{ return hInstance; }
};

static ExportStubClassDesc exportStubClassDesc;

ClassDesc* GetExportStubDesc()
{
	return &exportStubClassDesc;
}

ExportStubGUP::ExportStubGUP()
{
}

ExportStubGUP::~ExportStubGUP()
{
}

DWORD ExportStubGUP::Start()
{
	CResourceCompilerHelper rcHelper;

	for (int attempt = 1; ; ++attempt)
	{
		std::string buildPath = rcHelper.GetRootPath(true);

#if !defined WIN64
#	define EXPORT_LIBRARY_BIT_EXTENSION ""
#else
#	define EXPORT_LIBRARY_BIT_EXTENSION "_64"
#endif

		char buffer[1024];
		sprintf(buffer, "Tools\\MaxExportLibrary%d%s.dll", MAX_PRODUCT_VERSION_MAJOR, EXPORT_LIBRARY_BIT_EXTENSION);
		std::string libraryPath = PathUtil::Make(buildPath, buffer);

		m_exportLibrary = LoadLibrary(libraryPath.c_str());

		if (m_exportLibrary || attempt >= 2)
		{
			break;
		}

		rcHelper.ResourceCompilerUI(GetCOREInterface()->GetMAXHWnd());
	}

	if (m_exportLibrary)
	{
		typedef void (* InitializeFn)(ClassDesc* parentClass);
		InitializeFn initialize = (InitializeFn)GetProcAddress(m_exportLibrary, "Initialize");
		if (initialize)
		{
			initialize(&exportStubClassDesc);
		}
	}

	return GUPRESULT_KEEP;
}

void ExportStubGUP::Stop()
{
	if (m_exportLibrary)
	{
		typedef void (* FinalizeFn)();
		FinalizeFn finalize = (FinalizeFn)GetProcAddress(m_exportLibrary, "Finalize");
		if (finalize)
		{
			finalize();
		}
		
		FreeLibrary(m_exportLibrary);
	}

	m_exportLibrary = 0;
}

DWORD_PTR ExportStubGUP::Control(DWORD parameter)
{
	return 0;
}

void ExportStubGUP::DeleteThis()
{
	g_exportStub = 0;
	delete this;
}

void ExportStubGUP::DoExport(const char* parameters, INode** nodes, DWORD nodeCount)
{
	if ((parameters == 0) || (parameters[0] == 0))
	{
		MessageBox(
			GetCOREInterface()->GetMAXHWnd(), 
			"Unexpected missing parameters for exporting. Contact an exporter programmer.", 
			"Exporter", 
			MB_OK | MB_ICONERROR);
		return;
	}

	if ((nodes == 0) || (nodeCount <= 0))
	{
		MessageBox(
			GetCOREInterface()->GetMAXHWnd(), 
			"There is no nodes in the export list", 
			"Exporter", 
			MB_OK | MB_ICONERROR);
		return;
	}

	if (!m_exportLibrary)
	{
		MessageBox(
			GetCOREInterface()->GetMAXHWnd(), 
			"Tools\\MaxExportLibrary*.dll is not loaded", 
			"Exporter", 
			MB_OK | MB_ICONERROR);
		return;
	}

	typedef void (*FnType)(const char* parameters, INode** nodes, DWORD nodeCount);
	FnType const fn = (FnType)GetProcAddress(m_exportLibrary, "ExtendedExport");
	if (!fn)
	{
		MessageBox(
			GetCOREInterface()->GetMAXHWnd(), 
			"'ExtendedExport()' is missing in Tools\\MaxExportLibrary*.dll\n"
			"Use latest tools and plugins.", 
			"Exporter", 
			MB_OK | MB_ICONERROR);
		return;
	}

	fn(parameters, nodes, nodeCount);
}
