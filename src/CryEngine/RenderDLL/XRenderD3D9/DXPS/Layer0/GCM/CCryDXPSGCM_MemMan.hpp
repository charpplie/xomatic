#ifndef __CRYDXPSGCM_MEMMAN__
#define __CRYDXPSGCM_MEMMAN__

#include "CCryDXPSGCM_MemConfig.hpp"
#include "CCryDXPSGCM_MemItemList.hpp"
#include "CCryDXPSGCM_MemBank.hpp"

class CCryDXPSGCMMemMan
{
private:
	CCryDXPSGCMMemItemList	m_ItemFreelist;
	CCryDXPSGCMMemBank			m_Bank[CRY_MM_BANK_COUNT];
	void*										m_pMemory;
	uint32									m_Size;

#if defined(CRY_MM_DEBUG_DEFRAG)
	void*										m_pTarget[2];
#endif
#if defined(CRY_MM_PROFILE)
	uint64									m_AllocTime;
	uint64									m_FreeTime;
	uint64									m_Allocs;
	uint64									m_Frees;
#endif

	//reserved for animations
	void*										m_pMemoryAni;
	uint32									m_SizeAni;

	uint32									m_InUse;

	uint32									AllocatePages(uint32 Count,uint32 Allign);
	void										Merge(CCryDXPSGCMMemItem*	pItem);
public:
	void										Init(void* pMemory,uint32 Size,uint32 Alloc4Animation=CRY_MM_RESERVED4ANIMATION);

	uint32									Allocate(uint32 Size,uint32 Allign=CRY_MM_PAGE_SIZE);
	void										Free(uint32 ID);

//	void* 								AllocateHuge(uint32 Size);
//	void									FreeHuge(void* pMem);

	void										Lock(uint32 ID);
	uint32									InUse()const{return m_InUse;}
	void*										ResolveHandle(uint32 ID)
													{
														uint8* pData=&reinterpret_cast<uint8*>(m_pMemory)[CCryDXPSGCMMemItem::ID2Pointer(ID)->StartPage()*CRY_MM_PAGE_SIZE];
#if defined(CRY_MM_DEBUG_BOUNDCHECK)
														if(!CCryDXPSGCMMemItem::ID2Pointer(ID)->Alignement())
														{
															pData+=CRY_MM_PAGE_SIZE;
															if(reinterpret_cast<uint32*>(pData)[-1]!=0xCAFEFACE)
																snPause();
															const uint32_t Size	=	reinterpret_cast<uint32*>(pData)[-2];
															if(*reinterpret_cast<uint32*>(pData+Size)!=0xCAFEFACE)
																snPause();
														}
#endif
														return pData;
													}
	void*										Memory()
													{
														return m_pMemory;
													}
	void*										MemoryAni()
													{
														return m_pMemoryAni;
													}
	uint32									SizeAni()const{return m_SizeAni;}


#if defined(CRY_MM_DEBUG_DEFRAG)
	void										DrawDebug(uint32 DebugMode,void* pTarget);
#endif

	void										Size(class ICrySizer* Sizer);
};


#endif

