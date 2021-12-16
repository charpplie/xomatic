//////////////////////////////////////////////////////////////////////////////
// Shared resources to driver GFx
//////////////////////////////////////////////////////////////////////////////

#ifndef _SHARED_RESOURCES_H_
#define _SHARED_RESOURCES_H_

#pragma once


#ifndef EXCLUDE_SCALEFORM_SDK


#pragma warning(push)
#pragma warning(disable : 6326) // Potential comparison of a constant with another constant
#pragma warning(disable : 6011) // Dereferencing NULL pointer
#include <GFxLoader.h>
#pragma warning(pop)

#include "GAllocatorCryMem.h"
#include "GRendererXRender.h"


class GFxLoader2;
class ICrySizer;


struct IObserverLifeTime_GFxLoader
{
	virtual void OnDestroy(GFxLoader2*) = 0;
};


class CSharedFlashPlayerResources :
	public IObserverLifeTime_GFxLoader,
	public IObserverLifeTime_GRenderer
{
public:
	GFxLoader2* GetLoader(bool getRawInterface = false);
	GRendererXRender* GetRenderer(bool getRawInterface = false);

	virtual void OnDestroy(GFxLoader2*);
	virtual void OnDestroy(GRendererXRender*);

	const GSysAllocCryMem::Stats& GetSysAllocStats() const;
	void GetMemoryUsage(ICrySizer* pSizer) const;

public:
	static CSharedFlashPlayerResources& GetAccess();

private:
	CSharedFlashPlayerResources();
	~CSharedFlashPlayerResources();

private:
	GFxLoader2* m_pLoader;
	GRendererXRender* m_pRenderer;
	GSysAllocCryMem m_sysAllocCryMem;
};


class GFxLoader2 : public GFxLoader
{
public:
	GFxLoader2(IObserverLifeTime_GFxLoader* pObserver);

	// cannot configure GFC's RefCountBase via public inheritance so implement own ref counting
	virtual void AddRef();
	virtual void Release();

	void UpdateParserVerbosity();

private:
	virtual ~GFxLoader2();
	void SetupDynamicFontCache();

private:
	int m_refCount;
	IObserverLifeTime_GFxLoader* m_pObserver;
	GFxParseControl m_parserVerbosity;
};


#endif // #ifndef EXCLUDE_SCALEFORM_SDK


#endif // #ifndef _SHARED_RESOURCES_H_
