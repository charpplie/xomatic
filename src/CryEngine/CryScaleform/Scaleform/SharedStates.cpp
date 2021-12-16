#include "StdAfx.h"


#ifndef EXCLUDE_SCALEFORM_SDK


#include "SharedStates.h"

#include "FlashPlayerInstance.h"
#include "GFileCryPak.h"
#include "GImageInfoXRender.h"
#include "GTextureXRender.h"
#include "GImage.h"

#include <IConsole.h>
#include <StringUtils.h>
#include <CryPath.h>
#include <ILocalizationManager.h>
#include <ITexture.h>


//////////////////////////////////////////////////////////////////////////
// CryGFxFileOpener

CryGFxFileOpener::CryGFxFileOpener()
{
}


CryGFxFileOpener& CryGFxFileOpener::GetAccess()
{
	static CryGFxFileOpener s_inst;
	return s_inst;
}


CryGFxFileOpener::~CryGFxFileOpener()
{
}


GFile* CryGFxFileOpener::OpenFile(const char* pUrl, SInt flags, SInt /*mode*/)
{
	// delegate all file I/O to ICryPak by returning a GFileCryPak object
	if (flags & ~(GFileConstants::Open_Read|GFileConstants::Open_Buffered))
		return 0;
	return new GFileCryPak(pUrl);
}


SInt64 CryGFxFileOpener::GetFileModifyTime(const char* pUrl)
{
	SInt64 fileModTime = -1;
	ICryPak* pPak = gEnv->pCryPak;
	if (pPak)
	{
		FILE* f = pPak->FOpen(pUrl, "rb");
		if (f)
		{
			fileModTime = (SInt64) pPak->GetModificationTime(f);
			pPak->FClose(f);
		}
	}
	return fileModTime;
}


//////////////////////////////////////////////////////////////////////////
// CryGFxURLBuilder

CryGFxURLBuilder::CryGFxURLBuilder()
{
}


CryGFxURLBuilder& CryGFxURLBuilder::GetAccess()
{
	static CryGFxURLBuilder s_inst;
	return s_inst;
}


CryGFxURLBuilder::~CryGFxURLBuilder()
{
}

void CryGFxURLBuilder::BuildURL(GString* pPath, const LocationInfo& loc)
{
	assert(pPath); // GFx always passes a valid dst ptr

	const char* pOrigFilePath = loc.FileName.ToCStr();
	if (pOrigFilePath && *pOrigFilePath)
	{
		// if pOrigFilePath ends with "_locfont.[swf|gfx]", we search for the font in Languages
		const char locFontPostFixSwf[] = "_locfont.swf";
		const char locFontPostFixGfx[] = "_locfont.gfx";
		assert(sizeof(locFontPostFixSwf) == sizeof(locFontPostFixGfx)); // both postfixes must be of same length
		const size_t locFontPostFixLen = sizeof(locFontPostFixSwf) - 1;

		size_t filePathLength = strlen(pOrigFilePath);
		if (filePathLength > locFontPostFixLen)
		{
			size_t offset = filePathLength - locFontPostFixLen;
			if (!strnicmp(locFontPostFixGfx, pOrigFilePath + offset, locFontPostFixLen) || !strnicmp(locFontPostFixSwf, pOrigFilePath + offset, locFontPostFixLen))
			{
				*pPath = "Languages/";
				*pPath += PathUtil::GetFile(pOrigFilePath);
				return;
			}
		}

		if (*pOrigFilePath != '/')
			*pPath = loc.ParentPath + loc.FileName;
		else
			*pPath = GString(&pOrigFilePath[1], filePathLength - 1);
	}
	else
	{
		assert(0 && "CryGFxURLBuilder::BuildURL() - no filename passed!");
		*pPath = "";
	}
}


//////////////////////////////////////////////////////////////////////////
// CryGFxTranslator

CryGFxTranslator::CryGFxTranslator()
: m_pILocMan(0)
, m_localizedString()
{
	m_pILocMan = gEnv->pSystem->GetLocalizationManager();
}


CryGFxTranslator& CryGFxTranslator::GetAccess()
{
	static CryGFxTranslator s_inst;
	return s_inst;
}


CryGFxTranslator::~CryGFxTranslator()
{
}


UInt CryGFxTranslator::GetCaps() const
{
	return Cap_StripTrailingNewLines;
}


void CryGFxTranslator::Translate(TranslateInfo* pTranslateInfo)
{
	if (!m_pILocMan || !pTranslateInfo)
		return;

	const wchar_t* pKey = pTranslateInfo->GetKey();

	if (!pKey || *pKey != L'@') // this is not a localizable label
		return;

	// Plain, quick and simple Unicode to UTF8 conversion of the key for Latin languages (our keys will always be in English!)
	char keyASCII[2048]; assert(wcslen(pKey) < sizeof(keyASCII));
	size_t idx(0);
	while (pKey[idx] /*&& pKey[idx] != L'\n'*/ && idx < sizeof(keyASCII) - 1)
	{
		keyASCII[idx] = (char) (pKey[idx] & 0xFF);
		++idx;
	}
	keyASCII[idx] = '\0';
	++idx;

	if (m_pILocMan->LocalizeLabel(keyASCII, m_localizedString))
		pTranslateInfo->SetResult(m_localizedString.c_str());
}


//////////////////////////////////////////////////////////////////////////
// CryGFxLog

#if defined(WIN32) || defined(WIN64)
ICVar* CryGFxLog::CV_sys_flash_debuglog(0);
int CryGFxLog::ms_sys_flash_debuglog(0);
#endif


CryGFxLog::CryGFxLog()
: m_pLog(0)
, m_pCurFlashContext(0)
{
	m_pLog = gEnv->pLog;

#if defined(WIN32) || defined(WIN64)
	if (!CV_sys_flash_debuglog)
		CV_sys_flash_debuglog = REGISTER_CVAR2("sys_flash_debuglog", &ms_sys_flash_debuglog, 0,VF_NULL,"");
#endif
}


CryGFxLog& CryGFxLog::GetAccess()
{
	static CryGFxLog s_inst;
	return s_inst;
}


CryGFxLog::~CryGFxLog()
{
}


void CryGFxLog::LogMessageVarg(LogMessageType messageType, const char* pFmt, va_list argList)
{
	char logBuf[1024];
	{
		const size_t sizeLogBuf = sizeof(logBuf);
		int written = _vsnprintf(logBuf, sizeLogBuf, pFmt, argList);
		if (written < 0 || written == sizeLogBuf)
		{
			logBuf[sizeLogBuf-1] = '\0';
			written = sizeLogBuf - 1;
		}
		if (written && logBuf[written-1] == '\n')
			logBuf[written-1] = '\0';
	}

	char reFmt[1024];
	{
		const size_t sizeReFmt = sizeof(reFmt);
		int written = _snprintf(reFmt, sizeReFmt, "<Flash> %.896s [%s]", logBuf, m_pCurFlashContext ? m_pCurFlashContext : "#!NO_CONTEXT!#");
		if (written < 0 || written == sizeReFmt)
			reFmt[sizeReFmt-1] = '\0';
	}

#if defined(WIN32) || defined(WIN64)
	if (ms_sys_flash_debuglog)
		OutputDebugString(reFmt);
#endif

	switch(messageType & (~Log_Channel_Mask))
	{
	case Log_MessageType_Error:
		{
			m_pLog->LogError("%s", reFmt);
			break;
		}
	case Log_MessageType_Warning:
		{
			m_pLog->LogWarning("%s", reFmt);
			break;
		}
	case Log_MessageType_Message:
	default:
		{
			m_pLog->Log("%s", reFmt);
			break;
		}
	}
}


void CryGFxLog::SetContext(const char* pFlashContext)
{
	m_pCurFlashContext = pFlashContext;
}


const char* CryGFxLog::GetContext() const
{
	return m_pCurFlashContext;
}


//////////////////////////////////////////////////////////////////////////
// CryGFxFSCommandHandler

CryGFxFSCommandHandler::CryGFxFSCommandHandler()
{
}


CryGFxFSCommandHandler& CryGFxFSCommandHandler::GetAccess()
{
	static CryGFxFSCommandHandler s_inst;
	return s_inst;
}


CryGFxFSCommandHandler::~CryGFxFSCommandHandler()
{
}


void CryGFxFSCommandHandler::Callback(GFxMovieView* pMovieView, const char* pCommand, const char* pArgs)
{
	// get flash player instance this action script command belongs to and delegate it to client
	CFlashPlayer* pFlashPlayer(static_cast<CFlashPlayer*>(pMovieView->GetUserData()));
	if (pFlashPlayer)
		pFlashPlayer->DelegateFSCommandCallback(pCommand, pArgs);
}


//////////////////////////////////////////////////////////////////////////
// CryGFxExternalInterface

CryGFxExternalInterface::CryGFxExternalInterface()
{
}


CryGFxExternalInterface& CryGFxExternalInterface::GetAccess()
{
	static CryGFxExternalInterface s_inst;
	return s_inst;
}


CryGFxExternalInterface::~CryGFxExternalInterface()
{
}


void CryGFxExternalInterface::Callback(GFxMovieView* pMovieView, const char* pMethodName, const GFxValue* pArgs, UInt numArgs)
{
	CFlashPlayer* pFlashPlayer(static_cast<CFlashPlayer*>(pMovieView->GetUserData()));
	if (pFlashPlayer)
		pFlashPlayer->DelegateExternalInterfaceCallback(pMethodName, pArgs, numArgs);
	else
		pMovieView->SetExternalInterfaceRetVal(GFxValue(GFxValue::VT_Undefined));
}


//////////////////////////////////////////////////////////////////////////
// CryGFxUserEventHandler

CryGFxUserEventHandler::CryGFxUserEventHandler()
{
}


CryGFxUserEventHandler& CryGFxUserEventHandler::GetAccess()
{
	static CryGFxUserEventHandler s_inst;
	return s_inst;
}


CryGFxUserEventHandler::~CryGFxUserEventHandler()
{
}


void CryGFxUserEventHandler::HandleEvent(class GFxMovieView* pMovieView, const class GFxEvent& event)
{
	// not implemented since it's not needed yet... would need to translate GFx event to independent representation
}


//////////////////////////////////////////////////////////////////////////
// CryGFxImageCreator

CryGFxImageCreator::CryGFxImageCreator()
{
}


CryGFxImageCreator& CryGFxImageCreator::GetAccess()
{
	static CryGFxImageCreator s_inst;
	return s_inst;
}


CryGFxImageCreator::~CryGFxImageCreator()
{
}


GImageInfoBase* CryGFxImageCreator::CreateImage(const GFxImageCreateInfo& info)
{
	GImageInfoBase* pImageInfo(0);
	switch (info.Type)
	{
	case GFxImageCreateInfo::Input_Image:
	case GFxImageCreateInfo::Input_File:
		{
			switch (info.Type)
			{
			case GFxImageCreateInfo::Input_Image:
				{
					// create image info and texture for internal image
					pImageInfo = new GImageInfoXRender(info.pImage);
					break;
				}
			case GFxImageCreateInfo::Input_File:
				{
					// create image info and texture for external image
					pImageInfo = new GImageInfoFileXRender(info.pFileInfo->FileName.ToCStr(), info.pFileInfo->TargetWidth, info.pFileInfo->TargetHeight);
					break;
				}
			}
			break;
		}
	default:
		{
			assert(0);
			break;
		}
	}
	return pImageInfo;
}


//////////////////////////////////////////////////////////////////////////
// CryGFxImageLoader

CryGFxImageLoader::CryGFxImageLoader()
{
}


CryGFxImageLoader& CryGFxImageLoader::GetAccess()
{
	static CryGFxImageLoader s_inst;
	return s_inst;
}


CryGFxImageLoader::~CryGFxImageLoader()
{
}


static bool LookupDDS(const char* pFilePath, uint32 &width, uint32& height)
{
	//Texture Flags required for CTexture::GenName()
	ITexture *pTex = gEnv->pRenderer->EF_GetTextureByName(pFilePath, FT_NOMIPS);

	if(pTex)
	{
		height = pTex->GetHeight();
		width = pTex->GetWidth();

		return true;
	}
	else
	{
#if defined(XENON) || defined(PS3)
		string filePath(pFilePath);
		filePath += ".0";
		pFilePath = filePath.c_str();
#endif

		ICryPak* pPak(gEnv->pCryPak);
		FILE* f(pPak->FOpen(pFilePath, "rb"));
		if (f)
		{
			pPak->FSeek(f, 12, SEEK_SET);
			pPak->FRead(&height, 1, f);
			pPak->FRead(&width, 1, f);
			pPak->FClose(f);
		}
		return f != 0;
	}
}


GImageInfoBase* CryGFxImageLoader::LoadImage(const char* pUrl)
{
	// callback for loadMovie API
	const char* pFilePath(strstr(pUrl, "://"));
	if (!pFilePath)
		return 0;
	pFilePath += 3;

	GImageInfoBase* pImageInfo(0);
	if (CFlashPlayer::GetFlashLoadMovieHandler())
	{
		IFlashLoadMovieImage* pImage(CFlashPlayer::GetFlashLoadMovieHandler()->LoadMovie(pFilePath));
		if (pImage && pImage->IsValid())
			pImageInfo = new GImageInfoILMISrcXRender(pImage);
	}

	if (!pImageInfo)
	{
		if (stricmp(pFilePath, "undefined") != 0)
		{
			// Note: We need to know the dimensions in advance as otherwise the image won't show up!
			// The seemingly more elegant approach to pass zero as w/h and explicitly call GetTexture() on the image object to 
			// query w/h won't work reliably with MT rendering (potential deadlock between Advance() and Render()). This approach 
			// works but requires that we keep in sync with any changes to the streaming system on consoles (see LookupDDS()).
			uint32 width(0), height(0);
			if (!stricmp(PathUtil::GetExt(pFilePath), "dds") && LookupDDS(pFilePath, width, height))
				pImageInfo = new GImageInfoFileXRender(pFilePath, width, height);
			else
				CryGFxLog::GetAccess().LogWarning("\"%s\" cannot be loaded by loadMovie API! Invalid file or format passed.", pFilePath);
		}
	}

	return pImageInfo;
}


#endif // #ifndef EXCLUDE_SCALEFORM_SDK
