#include "StdAfx.h"


#ifndef EXCLUDE_SCALEFORM_SDK


#include "SharedResources.h"
#include "SharedStates.h"
#include "FlashPlayerInstance.h"
#include <CrySizer.h>
#include <ISystem.h>


//////////////////////////////////////////////////////////////////////////
// CSharedFlashPlayerResources

CSharedFlashPlayerResources::CSharedFlashPlayerResources()
: m_pLoader(0)
, m_pRenderer(0)
{
	gEnv->pLog->Log("Using Scaleform GFx "GFC_FX_VERSION_STRING);
	GSystem::Init(&m_sysAllocCryMem);
}


CSharedFlashPlayerResources::~CSharedFlashPlayerResources()
{
	//assert(!m_pLoader && !m_pRenderer);
	GSystem::Destroy();
}


CSharedFlashPlayerResources& CSharedFlashPlayerResources::GetAccess()
{
	static CSharedFlashPlayerResources s_inst;
	return s_inst;
}


GFxLoader2* CSharedFlashPlayerResources::GetLoader(bool getRawInterface)
{
	if (!getRawInterface)
	{
		if (!m_pLoader)
			m_pLoader = new GFxLoader2(this);
		else
			m_pLoader->AddRef();
	}
	return m_pLoader;
}


GRendererXRender* CSharedFlashPlayerResources::GetRenderer(bool getRawInterface)
{
	if (!getRawInterface)
	{
		if (!m_pRenderer)
			m_pRenderer = new GRendererXRender(this);
		else
			m_pRenderer->AddRef();
	}
	return m_pRenderer;
}


void CSharedFlashPlayerResources::OnDestroy(GFxLoader2*)
{
	m_pLoader = 0;
}


void CSharedFlashPlayerResources::OnDestroy(GRendererXRender*)
{
	m_pRenderer = 0;
}


const GSysAllocCryMem::Stats& CSharedFlashPlayerResources::GetSysAllocStats() const
{
	return m_sysAllocCryMem.GetStats();
}


void CSharedFlashPlayerResources::GetMemoryUsage(ICrySizer* pSizer) const
{
	m_sysAllocCryMem.GetMemoryUsage( pSizer );		
}


//////////////////////////////////////////////////////////////////////////
// GFxLoader2

GFxLoader2::GFxLoader2(IObserverLifeTime_GFxLoader* pObserver)
: m_refCount(1)
, m_pObserver(pObserver)
{
	// set callbacks
	SetLog(&CryGFxLog::GetAccess());
	SetFileOpener(&CryGFxFileOpener::GetAccess());
	SetURLBuilder(&CryGFxURLBuilder::GetAccess());
	SetImageCreator(&CryGFxImageCreator::GetAccess());
	SetImageLoader(&CryGFxImageLoader::GetAccess());

	// enable dynamic font cache
	SetupDynamicFontCache();

	// set parser verbosity
	UpdateParserVerbosity();
	SetParseControl(&m_parserVerbosity);
}


GFxLoader2::~GFxLoader2()
{
	SetParseControl(0);
	if (m_pObserver)
		m_pObserver->OnDestroy(this);
}


void GFxLoader2::AddRef()
{
	++m_refCount;
}


void GFxLoader2::Release()
{
	if (--m_refCount <= 0)
		delete this;
}


void GFxLoader2::UpdateParserVerbosity()
{
	m_parserVerbosity.SetParseFlags((CFlashPlayer::GetLogOptions() & CFlashPlayer::LO_LOADING) ? 
		GFxParseControl::VerboseParseAll : GFxParseControl::VerboseParseNone);
}


void GFxLoader2::SetupDynamicFontCache()
{
	SetFontPackParams(0);

	GFxFontCacheManager* pFontCacheMan(GetFontCacheManager());
	pFontCacheMan->EnableDynamicCache(true);
	pFontCacheMan->SetMaxRasterScale(1.25f);

	struct HalfValueOnConsoles
	{
		uint32 val;
		HalfValueOnConsoles(uint32 v) : val(v) {}
		operator uint32() const
		{
#if defined(XENON) || defined(PS3)
			return val >> 1;
#else
			return val;
#endif
		}
	};

	GFxFontCacheManager::TextureConfig cfg;
	cfg.TextureWidth = HalfValueOnConsoles(1024);
	cfg.TextureHeight = HalfValueOnConsoles(1024);
	cfg.MaxNumTextures = 1;
	cfg.MaxSlotHeight = 48;
	cfg.SlotPadding = 2;
	cfg.TexUpdWidth = HalfValueOnConsoles(256);
	cfg.TexUpdHeight = HalfValueOnConsoles(512);
	pFontCacheMan->SetTextureConfig(cfg);
}


#endif // #ifndef EXCLUDE_SCALEFORM_SDK