#ifndef __CRYDXPSRESOURCE__
#define __CRYDXPSRESOURCE__

#include "../../Layer0/GCM/CCryDXPSGCM_SyncMan.hpp"

enum ECryDXPSTex2DCreateFlags
{
	ECDXPSTCF_NONE = 0,
	ECDXPSTCF_DUMMY,
	ECDXPSTCF_FRAMEBUFFER,
};

enum	ECRYDXPSResType
{
	EDXPS_RT_NONE=0,
	EDXPS_RT_TEX1D,
	EDXPS_RT_TEX2D,
	EDXPS_RT_TEX2D_SWAPCHAIN,
	EDXPS_RT_TEX3D,
	EDXPS_RT_INPUTLAYOUT,
	EDXPS_RT_RENDERTARGETVIEW,
	EDXPS_RT_DEPTHSTENCILVIEW,
	EDXPS_RT_SHADERRESOURCEVIEW,
	EDXPS_RT_SHADER,
	EDXPS_RT_PIXELSHADERCACHE,
	EDXPS_RT_SHADERREFLECTION,
	EDXPS_RT_BLOB,
	EDXPS_RT_VERTEXBUFFER,
	EDXPS_RT_SAMPLERSTATE,
	EDXPS_RT_INDEXBUFFER,
	EDXPS_RT_CONSTBUFFER,
	EDXPS_RT_CONSTBUFFERTEMP,
	EDXPS_RT_BLENDSTATE,
	EDXPS_RT_RASTERIZERSTATE,
	EDXPS_RT_DEPTHSTENCILSTATE,
	EDXPS_RT_RENDERDEVICE,
	EDXPS_RT_ADAPTER,
	EDXPS_RT_OUTPUT,
	EDXPS_RT_UNUSED=~0,
};

enum	ECRYDXPSResFreq
{
	EDXPS_RF_STATIC,
	EDXPS_RF_DYNAMIC,
};

#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
class CCryLayer0GCMMemResource
{
	uint32	m_Count;
	uint32	m_MainMem;
	uint32	m_VideoMem;
	uint32	m_VideoMemAllocated;
public:
					CCryLayer0GCMMemResource():
					m_Count(0),
					m_MainMem(0),
					m_VideoMem(0),
					m_VideoMemAllocated(0){}

	void		AddCount(){m_Count++;}
	void		SubCount(){m_Count--;}
	uint32	Count()const{return m_Count;}
	void		AddMM(uint32 Size){m_MainMem+=Size;}
	void		SubMM(uint32 Size){m_MainMem-=Size;}
	uint32	MainMem()const{return m_MainMem;}
	void		AddVM(uint32 Size){m_VideoMem+=Size;m_VideoMemAllocated+=(Size+4095)&~4095;}
	void		SubVM(uint32 Size){m_VideoMem-=Size;m_VideoMemAllocated-=(Size+4095)&~4095;}
	uint32	VideoMem()const{return m_VideoMem;}
	uint32	VideoMemAllocated()const{return m_VideoMemAllocated;}
};
	#define	MMRES_PARAM_ALONE CCryLayer0GCMMemResource& rMemResource
	#define	MMRES_PARAM ,	MMRES_PARAM_ALONE
	#define	MMRES_PASS ,rMemResource
	#define	MMRES_ADDCOUNT m_rMemResource.AddCount
	#define	MMRES_SUBCOUNT m_rMemResource.SubCount
	#define	MMRES_ADDMM(X) m_rMemResource.AddMM(X)
	#define	MMRES_SUBMM(X) m_rMemResource.SubMM(X)
	#define	MMRES_ADDVM(X) m_rMemResource.AddVM(X)
	#define	MMRES_SUBVM(X) m_rMemResource.SubVM(X)
#else
	#define	MMRES_PARAM_ALONE
	#define	MMRES_PARAM
	#define	MMRES_PASS
	#define	MMRES_ADDCOUNT()
	#define	MMRES_SUBCOUNT()
	#define	MMRES_ADDMM(X)
	#define	MMRES_SUBMM(X)
	#define	MMRES_ADDVM(X)
	#define	MMRES_SUBVM(X)
#endif

class CCryDXPSResource
{
//	uint64											m_ModificationCount;
protected:

	ECRYDXPSResType							m_Type;

#ifdef MEM_MAN_ADD_SIZE_BLOCK_VMEM
	CCryLayer0GCMMemResource&		m_rMemResource;
#endif

	/*volatile*/ tdResHandle				m_ResDCLock;

															CCryDXPSResource(ECRYDXPSResType T MMRES_PARAM);
#if defined(CRY_DXPS_SINGLETHREAD_OWNERSHIP)
															~CCryDXPSResource();
#endif
#if defined(CRY_DXPS_DESTRUCTORVALIDATION)
	void												Type(ECRYDXPSResType T){m_Type=T;}
#endif
public:


	//we try to run non-virtual if possible to avoid pipeline stalls on non-ooo CPU that handle
	//virtual calls like mispredicted conditional jumps

/*
	virtual void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION *rType) = 0;
	virtual void STDMETHODCALLTYPE SetEvictionPriority(UINT EvictionPriority) = 0;
	virtual UINT STDMETHODCALLTYPE GetEvictionPriority( void) = 0;
*/
	ILINE void									DCLockTo(tdResHandle DC){m_ResDCLock=DC;}
	ILINE bool									Locked()const{return TDRES_READ(m_ResDCLock)!=0;}
	void												FreeMe();








	void												Sync();


	bool												SyncCPU();
	bool												SyncNoWait();
	bool												SyncNoWaitAll();
	inline ECRYDXPSResType			Type()const{return m_Type;}
	inline uint64								ModCount()const{return 0;/*return m_ModificationCount;*/}
	inline void									IncModCount(){/*m_ModificationCount++;*/}
	tdResHandle*								LockAddress() const {return (tdResHandle*)&m_ResDCLock;}
	const tdResHandle						LockValue() const {return (tdResHandle)m_ResDCLock;}
};

typedef CCryDXPSResource ID3D11Resource;


#endif

