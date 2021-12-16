//////////////////////////////////////////////////////////////////////
//
//  CryFont Source Code
//
//  File: CryFont.cpp
//  Description: CCryFont class.
//
//  History:
//  - August 18, 2001: Created by Alberto Demichelis
//  - June	 28, 2003: Added r_DumpFontTexture and r_DumpFontNames CVARs
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CryFont.h"
#include "FBitmap.h"
#include "FFont.h"


///////////////////////////////////////////////
static void DumpFontTexture( IConsoleCmdArgs *pArgs )
{
	if(pArgs->GetArgCount()!=2)
		return;

	const char *szName = pArgs->GetArg(1);

	if ((szName) && (*szName != 0) && (*szName != '0'))
	{
		string szFontName(szName);
		string szFontFile(szName);
		szFontFile += ".bmp";

		CFFont *pFont = (CFFont *)gEnv->pCryFont->GetFont(szName);

		if(pFont)
		{
			pFont->m_pFontTexture->WriteToFile(szFontFile.c_str());

			gEnv->pLog->LogWithType(IMiniLog::eInputResponse,"Dumped '%s' texture to '%s'!", szName, szFontFile.c_str());
		}
	}

}

///////////////////////////////////////////////
static void DumpFontNames( IConsoleCmdArgs *pArgs )
{
	string sNames = gEnv->pCryFont->GetLoadedFontNames();

	gEnv->pLog->LogWithType(IMiniLog::eInputResponse,"Currently Loaded Fonts: %s",sNames.c_str());
}


///////////////////////////////////////////////
string CCryFont::GetLoadedFontNames()
{
	string sRet;
	FontMapItor pItor;
	CFFont *pFont;

	for (pItor=m_mapFonts.begin(); pItor != m_mapFonts.end(); ++pItor)
	{
		pFont = (CFFont *)pItor->second;		

		if(!sRet.empty())			// separators
			sRet+=",";

		sRet += pFont->m_sName;
	}

	return sRet;
}


///////////////////////////////////////////////
CCryFont::CCryFont(ISystem *pSystem)
{
  m_pISystem = pSystem;

	REGISTER_COMMAND("r_DumpFontTexture",DumpFontTexture, 0, 
		"Dumps the specified font's texture to a bitmap file\n"
		"Use r_DumpFontTexture to get the loaded font names\n"
		"Usage: r_DumpFontTexture <fontname>");
	REGISTER_COMMAND("r_DumpFontNames", DumpFontNames, 0, 
		"Logs a list of fonts currently loaded");
}

///////////////////////////////////////////////
CCryFont::~CCryFont()
{
	FontMap tmpMap;
	tmpMap.swap(m_mapFonts);

	for (FontMapItor itor=tmpMap.begin();itor!=tmpMap.end(); ++itor)
	{
		IFFont *pFont=itor->second;
		pFont->Release();
	}
}

///////////////////////////////////////////////
void CCryFont::Release()
{
	delete this;
}


///////////////////////////////////////////////
IFFont* CCryFont::NewFont(const char *pszName)
{
	string sName=pszName;
	sName.MakeLower();				// we will removed it soon

	// check if font already created, if so return it
	FontMapItor itor = m_mapFonts.find(CONST_TEMP_STRING(sName.c_str()));
	if (itor!=m_mapFonts.end())
		return itor->second;

	CFFont *pFont=new CFFont(m_pISystem, this, sName.c_str());
	m_mapFonts.insert(FontMapItor::value_type(sName.c_str(), pFont));
	return (IFFont*)pFont;
}

///////////////////////////////////////////////
void CCryFont::UnregisterFont(const char* pszName)
{
	FontMapItor iter = m_mapFonts.find(CONST_TEMP_STRING(pszName));

	if(iter!=m_mapFonts.end())
		m_mapFonts.erase(iter);
}

///////////////////////////////////////////////
IFFont* CCryFont::GetFont(const char *pszName)
{
	FontMapItor itor = m_mapFonts.find(CONST_TEMP_STRING(pszName));

	if(itor!=m_mapFonts.end())
		return itor->second;
	
	return 0;
}

void CCryFont::GetMemoryUsage(class ICrySizer* pSizer)
{
	if(!pSizer->Add (*this))
		return;

	FontMapItor it;
	pSizer->AddObject(m_mapFonts);
	
#ifndef _LIB // Only when compiling as dynamic library
	{
		//SIZER_COMPONENT_NAME(pSizer,"Strings");
		//pSizer->AddObject( (this+1),string::_usedMemory(0) );
	}
	{
		SIZER_COMPONENT_NAME(pSizer,"STL Allocator Waste");
		CryModuleMemoryInfo meminfo;
		ZeroStruct(meminfo);
		CryGetMemoryInfoForModule( &meminfo );
		pSizer->AddObject( (this+2),(size_t)meminfo.STL_wasted );
	}
#endif
}


#include <CrtDebugStats.h>

