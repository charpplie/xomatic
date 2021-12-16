// ResourceCompilerPC.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "StatCGFCompiler.h"
#include "CharacterCompiler.h"
#include "ColladaCompiler.h"
#include "CGA\AnimationCompiler.h"
#include "ICryXML.h"

#include "ResourceCompilerPC.h"

// Must be included only once in DLL module.
#include <platform_implRC.h>


//#ifdef CRY_STRING
//int sEmptyStringBuffer[] = { -1, 0, 0, 0 };
//string::StrHeader* string::m_emptyStringData = (string::StrHeader*)&sEmptyStringBuffer;
//wstring::StrHeader* wstring::m_emptyStringData = (wstring::StrHeader*)&sEmptyStringBuffer;
//#endif //CRY_STRING


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
	//// Find the directory of the current dll.
	//char szBuffer[1024];
	//GetModuleFileName(g_hInst, szBuffer, sizeof(szBuffer));
	//string sPath = Path::GetPath(szBuffer);

	//SetDllDirectory(Path::AddBackslash(sPath) + "..\\tools");
	HMODULE hXMLLibrary = LoadLibrary("CryXML.dll");
	if (NULL == hXMLLibrary)
	{
		char szCurrentDirectory[512];

		GetCurrentDirectory(512,szCurrentDirectory);

		RCLogError("Unable to load xml library (CryXML.dll)");
		RCLogError("  Current Directory: %s",szCurrentDirectory);		// useful to track down errors
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

// This is an example of an exported function.
void __stdcall RegisterConvertors(IResourceCompiler* const pRC)
{
	SetRCLog(pRC->GetIRCLog());

	pRC->RegisterKey("ext_cgf","ignore or copy cgf files");	
	pRC->RegisterKey("ext_cga","ignore or copy cga files");	
	pRC->RegisterKey("ext_chr","ignore or copy chr files");	
	pRC->RegisterKey("ext_cba","ignore or copy animations batch processing file");	
	pRC->RegisterKey("ext_dae","ignore or copy dae files");	
	pRC->RegisterKey("ext_zip","ignore or copy zip files");	

	ICryXML* const pCryXML = LoadICryXML();

	pRC->RegisterConvertor(new CStatCGFCompiler());

	if (pCryXML == 0)
	{
		RCLogError("Loading xml library failed - not registering collada converter.");
	}
	else
	{
		pRC->RegisterConvertor(new CharacterCompiler(pCryXML));
		pRC->RegisterConvertor(g_pAnimationCompiler = new CAnimationCompiler(pCryXML));
		pRC->RegisterConvertor(new ColladaCompiler(pCryXML, pRC->GetPakSystem()));
	}

	pRC->RegisterKey("createmtl","[DAE] 0=don't create .mtl files (default), 1=create .mtl files");

	pRC->RegisterKey("file","animation file for processing");	
	pRC->RegisterKey("dest","destination folder for the results");	
	pRC->RegisterKey("report","report mode");	
	pRC->RegisterKey("cleanupfast","erase uncompressed chunks without recalculation");	
	pRC->RegisterKey("cleanup","erase uncompressed chunks with recalculation");	
	pRC->RegisterKey("SkipDba","skip build dba");	
	pRC->RegisterKey("checkloco",
		"should be used with report mode.\n"
		"Compare locomotion_locator motion with recalculated root motion");	

	pRC->RegisterKey("qtangents","0=use vectors to represent tangent space(default), 1=use quaternions");

	pRC->RegisterKey("debugdump","[CGF] dump contents of source .cgf file instead of compiling it");
	pRC->RegisterKey("debugvalidate","[CGF] validate source .cgf file instead of compiling it");

	pRC->RegisterKey("StripMesh","[CGF] Strip mesh chunks from output files");
	pRC->RegisterKey("SplitLODs","[CGF] Auto split LODs into the separate files");
	pRC->RegisterKey("SkipMissing","[CGF] Skip missing cgf files, do not treat them as error");
	pRC->RegisterKey("RealignChunks","[CGF,CHR] Realign chunks in output files (alignment is 4)");
}

HMODULE g_hInst;

#include "Cry_Vector3_info.h"
#include "Cry_Quat_info.h"
#include "Cry_Matrix_info.h"
#include "Cry_Color_info.h"
#include "Cry_Geo_info.h"
#include "CryHeaders_info.h"
#include "IIndexedMesh_info.h"
#include "CGFContent_info.h"
#include "CryHalf_info.h"
