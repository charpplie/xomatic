#include "StdAfx.h"
#include <assert.h>													// assert()
#include <Dbghelp.h>												// GetTimestampForLoadedLibrary

#include "IRCLog.h"													// IRCLog
#include "IConfig.h"												// IConfig

#include "ImageConvertor.h"
#include "ICfgFile.h"												// ICfgFile

// constructor
CImageConvertor::CImageConvertor()
:	m_bInitialized(false)
{
}


// destructor
CImageConvertor::~CImageConvertor()
{
	if (m_bInitialized)
		DeInit();
}

void CImageConvertor::Release()
{
	delete this;
}

ICompiler* CImageConvertor::CreateCompiler()
{
	return new CImageCompiler(&m_TextureFallbackSystem);
}

bool CImageConvertor::SupportsMultithreading() const
{
	return true;
}

int CImageConvertor::GetNumPlatforms() const
{
	return 3;
}


EPlatform CImageConvertor::GetPlatform( int index ) const
{
	switch (index)
	{
		case 0:	return ePlatform_PC;
		case 1:	return ePlatform_X360;
		case 2:	return ePlatform_PS3;
			//case 3:	return ePlatform_GAMECUBE;
	};
	//assert(0);
	return ePlatform_UNKNOWN;
}


int CImageConvertor::GetNumExt() const
{
	return 1;
}


const char* CImageConvertor::GetExt( int index ) const
{
	switch (index)
	{
		// we support tif ourselves
	case 0:	return("tif");
// 	case 1:	return("dds");
		// DirectX supports bmp dds dib jpg png tga
//		case 1:	return("bmp");
//		case 2:	return("dds");
//		case 3:	return("dib");
//		case 4:	return("jpg");
//		case 5:	return("png");
//		case 6:	return("tga");
			// advance CImageConvertor::GetNumExt if you add more extensions
	};

	//assert(0);
	return 0;
}


// this should retrieve the timestamp of the convertor executable:
// when it was created by the linker, normally. This date/time is used to
// compare with the compiled file date/time and even if the compiled file
// is not older than the source file, it will be recompiled if it's older than the
// convertor
DWORD CImageConvertor::GetTimestamp() const
{
	return GetTimestampForLoadedLibrary(g_hInst);
}

void CImageConvertor::Init(IConfig* config, const char* exePath)
{
	if (m_bInitialized)
		return;

	if(config->HasKey("fallback"))
	{
		string path = PathHelpers::Join(exePath, "TextureFallbackFile.dat");
		m_TextureFallbackSystem.Init(path);
		RCLog("CTextureFallbackSystem: Init (%d Entries) '%s'",m_TextureFallbackSystem.GetEntriesCount(),path);
		RCLog("");
	}

	m_bInitialized = true;
}

void CImageConvertor::DeInit()
{
	if (!m_bInitialized)
		return;

	m_TextureFallbackSystem.SaveIfInitialized();

	m_bInitialized = false;
}
