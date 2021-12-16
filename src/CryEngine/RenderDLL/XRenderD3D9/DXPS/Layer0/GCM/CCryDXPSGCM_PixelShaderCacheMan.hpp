#ifndef __CRYDXPSGCM_PIXELSHADERCACHEMAN__
#define __CRYDXPSGCM_PIXELSHADERCACHEMAN__

class CCryDXPSBuffer;

enum {CRY_PSCACHE_COUNT				=	4};
//enum {CRY_PSCACHE_BUFFERSIZE	=	64*1024};	//256KB
enum {CRY_PSCACHE_BUFFERSIZE	=	128*1024};	//4096KB, now shared pixelshader + VB/IB buffer

class CDXPSRDWorker;

class CCryDXPSGCMPixelshaderCacheMan
{
private:
	CCryDXPSBuffer*			m_pCache[CRY_PSCACHE_COUNT];
	uint32							m_CurrentUsedCache;
	uint32							m_CurrentBufferOffset;
	int									m_Initialized;

	void								Init();
	void								NextBuffer(tdResHandle DrawCallCounter);
public:
											CCryDXPSGCMPixelshaderCacheMan():
											m_Initialized(0){}

	void*								Alloc(uint32 Size,tdResHandle DrawCallCounter);
	
	void								Size(class ICrySizer* Sizer);
	friend class CDXPSRDWorker;
} _ALIGN(32);//put into a single cache line

#endif

