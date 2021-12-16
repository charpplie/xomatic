#include "StdAfx.h"
#include "../CCryDXPS.hpp"
#include "../../CCryTypes.hpp"
#include "CCryDXPSGCM_MemMan.hpp"

#if defined(CRY_DXPS_SINGLETHREAD_OWNERSHIP)
extern int g_MAINTHREADID;
#endif

void CCryDXPSGCMMemMan::Init(void* pMemory,uint32 Size,uint32 Alloc4Animation)
{
	m_InUse=0;
#if defined(CRY_MM_DEBUG_DEFRAG)
	m_pTarget[0]=0;
	m_pTarget[1]=0;
#endif
#if defined(CRY_MM_PROFILE)
	m_AllocTime	=
	m_FreeTime	=
	m_Allocs		=
	m_Frees			=	0;
#endif

#ifdef CRY_MM_DEBUG
CRY_DEBUGOUT("--== MEMORYMANAGER INIT ==--\n");
#endif
	m_ItemFreelist.InitAsFreelist();
/*
	for(uint32 a=0,d=0;a<Size;a++)
	{
		d=a^(a<<13)^(a>>2)^d;
		reinterpret_cast<uint8*>(pMemory)[a]=d^(d>>8)^(a>>16)^(a>>24);
	}
*/
	Size					-=	Alloc4Animation;
	m_pMemoryAni	=		&reinterpret_cast<uint8*>(pMemory)[Size];
	m_SizeAni			=		Alloc4Animation;
	m_pMemory			=		pMemory;
	m_Size				=		Size;

	uint32 numPagesAvailable = Size/CRY_MM_PAGE_SIZE;

	for(uint32 a=0; a<CRY_MM_BANK_COUNT; a++)
	{
		uint32 numPagesForThisBank = MIN(numPagesAvailable, CRY_MM_PAGEBANK_COUNT);
		m_Bank[a].Init(numPagesForThisBank > 0, a * CRY_MM_PAGEBANK_COUNT, numPagesForThisBank);
		numPagesAvailable -= numPagesForThisBank;
	}
}

uint32 CCryDXPSGCMMemMan::AllocatePages(uint32 Count,uint32 Allign)
{
#if defined(CRY_DXPS_SINGLETHREAD_OWNERSHIP)
	int Th=GetCurrentThreadId();
	if(g_MAINTHREADID!=Th && g_MAINTHREADID!=-1)
	{
		snPause();
	}
#endif
#ifdef CRY_MM_DEBUG
		CRY_DEBUGOUT("Allocating %d pages..",Count);
#endif

	for(uint32 a=0;a<CRY_MM_BANK_COUNT;a++)
		if(m_Bank[a].Valid() && m_Bank[a].FreeMem()>=Count)	//just if bank is avaible try to allocate
		{
			uint32 ID	=	m_Bank[a].Alloc(Count,Allign,m_ItemFreelist);
			if(ID)
			{
				m_Bank[a].FreeMemDec(Count);
				m_InUse	+=	Count*CRY_MM_PAGE_SIZE;
#ifdef CRY_MM_DEBUG
				CRY_DEBUGOUT("successfully\n  %d%% of %dMB in use, Offset:0x%x\n",m_InUse/(m_Size/100),m_Size>>20,ID);
#endif
				return ID;
			}
		}

	CRY_DEBUGOUT_ALWAYS("****** failed to allocate %d bytes of VRAM ******\n",Count<<CRY_MM_PAGE_SHIFT);

	return 0;
}

uint32 CCryDXPSGCMMemMan::Allocate(uint32 Size,uint32 Align)
{
#if defined(CRY_MM_PROFILE)
	uint64	T0	=	rdtsc();
#endif
#ifdef CRY_MM_DEBUG
	CRY_DEBUGOUT("Allocation request for %d bytes, rounded to %d bytes or %d pages\n",
						Size,
						((Size+CRY_MM_PAGE_SIZE-1)>>CRY_MM_PAGE_SHIFT)<<CRY_MM_PAGE_SHIFT,
						(Size+CRY_MM_PAGE_SIZE-1)>>CRY_MM_PAGE_SHIFT);
#endif
	Align	=	Align>CRY_MM_PAGE_SIZE?Align:CRY_MM_PAGE_SIZE;

#if defined(CRY_MM_DEBUG_BOUNDCHECK)
	const uint32	PageCount	=	(((Size+4+(Align-1))&~(Align-1))>>CRY_MM_PAGE_SHIFT)+1;
#else
	const uint32	PageCount	=	(((Size+(Align-1))&~(Align-1))>>CRY_MM_PAGE_SHIFT);
#endif
	const uint32	PageAlign	=	Align>>CRY_MM_PAGE_SHIFT;

	uint32 Ret	=	AllocatePages(PageCount,PageAlign);
	MEMSTAT_USAGE((void*)(0x80000000|Ret), Size);
	CCryDXPSGCMMemItem::ID2Pointer(Ret)->Alignement(Align!=CRY_MM_PAGE_SIZE);
#if defined(CRY_MM_PROFILE)
	uint64	T1	=	rdtsc();
	m_AllocTime+=T1-T0;
	m_Allocs++;
	printf("Alloctime: %lld Current: %lld Count: %lld Average: %lld\n",m_AllocTime,T1-T0,m_Allocs,m_AllocTime/m_Allocs);
#endif

#if defined(CRY_MM_DEBUG_BOUNDCHECK)
	{
		if(Align==CRY_MM_PAGE_SIZE)
		{
			uint8* pData	=	reinterpret_cast<uint8*>(&reinterpret_cast<uint8*>(m_pMemory)[CCryDXPSGCMMemItem::ID2Pointer(Ret)->StartPage()*CRY_MM_PAGE_SIZE]);
			pData+=CRY_MM_PAGE_SIZE;
			reinterpret_cast<uint32*>(pData)[-2]=Size;
			reinterpret_cast<uint32*>(pData)[-1]=0xCAFEFACE;
			*reinterpret_cast<uint32*>(pData+Size)=0xCAFEFACE;
		}
		ResolveHandle(Ret);
	}
#endif

	return Ret;
}
/*
void*  CCryDXPSGCMMemMan::AllocateHuge(uint32 Size)
{
	//fits into one bank?
	if(Size<CRY_MM_BANK_SIZE)
	{
		uint32 Handle=Allocate(Size);
		return ResolveHandle(Handle);
	}

	const uint32 Pages=(Size+CRY_MM_PAGE_SIZE-1)>>CRY_MM_PAGE_SHIFT;
	const uint32 Banks=(Pages+CRY_MM_PAGEBANK_COUNT)/CRY_MM_PAGEBANK_COUNT;

	for(uint32 a=0;a<CRY_MM_BANK_COUNT-Banks;a++)
	{
		uint32 FreeBanks=0;
		for(uint32 b=0;b<Banks;b++)
			FreeBanks+=(m_Bank[a+b].Valid() && m_Bank[a+b].UnUsed());	//Count free Banks

		if(FreeBanks==Banks)
		{
			for(uint32 b=0;b<Banks-1;b++)
				m_Bank[a+b].Validate(0);

			uint32 ID	=	m_Bank[a+Banks].Alloc(Pages&(CRY_MM_PAGEBANK_COUNT-1),m_ItemFreelist);
			if(ID)
			{
				m_InUse	+=	Pages*CRY_MM_PAGE_SIZE;
#ifdef CRY_MM_DEBUG
				CRY_DEBUGOUT("successfully\n  %d%% of %dMB in use, Offset:0x%x\n",m_InUse/(m_Size/100),m_Size>>20,ID);
#endif
				return &reinterpret_cast<uint8*>(m_pMemory)[m_Bank[a].StartPage()*CRY_MM_PAGE_SIZE];
			}
			else
			{
				CRY_DEBUGOUT("Failed to allocate mem from VMem in a free bank, SHOULD NEVER HAPPEN ");
				abort();
				return 0;
			}
		}
	}

	CRY_DEBUGOUT("Failed to allocate mem from VMem, no block found with enough mem");
	abort();
	return 0;
}

void  CCryDXPSGCMMemMan::FreeHuge(void* pMem)
{
}
*/

void CCryDXPSGCMMemMan::Merge(CCryDXPSGCMMemItem*	pItem)
{
	if(!pItem->Next())
		return;

	CCryDXPSGCMMemItem*	pNext	=	pItem->Next();
	if(!pNext	|| !pNext->IsFree())
		return;

	CRY_MM_VALIDATE_ITEM(pItem);
	CRY_MM_VALIDATE_ITEM(pNext);

	//is not last item?
	if(pNext->Next())
	{
		//merge
		pItem->Count(pNext->Count()+pItem->Count());
		//unlink
		pItem->Next(pNext->Next());
		pItem->Next()->Prev(pItem);
		//add to freelist
		m_ItemFreelist.AddAsFirst(pNext);
		CRY_MM_VALIDATE_ITEM(pItem);
		CRY_MM_VALIDATE_ITEM(pNext);
	}
	else
	if(pItem->Prev())//next item is the last one, so maybe this item is not the first one?
	{
		//merge
		pNext->Count(pNext->Count()+pItem->Count());
		pNext->StartPage(pItem->StartPage());
		//unlink
		pNext->Prev(pItem->Prev());
		pNext->Prev()->Next(pNext);
		//add to freelist
		m_ItemFreelist.AddAsFirst(pItem);
		CRY_MM_VALIDATE_ITEM(pItem);
		CRY_MM_VALIDATE_ITEM(pNext);
	}
	else//worst case, both items are on ends of list
	{
		//merge
		pNext->Count(pNext->Count()+pItem->Count());
		pNext->StartPage(pItem->StartPage());
		//as we cannot unlink, just set the size to 0, next time the itemlist is used it will be freed
		pItem->Count(0);
		CRY_MM_VALIDATE_ITEM(pItem);
		CRY_MM_VALIDATE_ITEM(pNext);
	}
}

void CCryDXPSGCMMemMan::Free(uint32 ID)
{
#if defined(CRY_MM_DEBUG_BOUNDCHECK)
	if(!CCryDXPSGCMMemItem::ID2Pointer(ID)->Alignement())
	{
		uint8* pData	=	reinterpret_cast<uint8*>(ResolveHandle(ID));
		if(reinterpret_cast<uint32*>(pData)[-1]!=0xCAFEFACE)
			snPause();
		const uint32_t Size	=	reinterpret_cast<uint32*>(pData)[-2];
		if(*reinterpret_cast<uint32*>(pData+Size)!=0xCAFEFACE)
			snPause();
	}
#endif
#if defined(CRY_MM_PROFILE)
	uint64	T0	=	rdtsc();
#endif
#if defined(CRY_DXPS_SINGLETHREAD_OWNERSHIP)
	int Th=GetCurrentThreadId();
	if(g_MAINTHREADID!=Th && g_MAINTHREADID!=-1)
	{
		snPause();
	}
#endif
	CCryDXPSGCMMemItem*	pItem	=	CCryDXPSGCMMemItem::ID2Pointer(ID);
#if defined(CRY_MM_DEBUG_ITEMLIST)
	if(ID==0 || pItem->IsFree() || pItem->Count()==0)
	{
		CRY_DEBUGOUT("Trying to free an already freed vmem-item\n");
		snPause();
	}
	CCryDXPSGCMMemItem*	pEmptyItem	=	m_ItemFreelist.Find(ID);
	if(pEmptyItem)
	{
		CRY_DEBUGOUT("Trying to free an already freed vmem-item\n");
		snPause();
	}
#endif

	for(uint32 a=0;a<CRY_MM_BANK_COUNT;a++)
		if(m_Bank[a].Valid() && m_Bank[a].StartPage()<=pItem->StartPage() && m_Bank[a].StartPage()+CRY_MM_PAGE_SIZE>pItem->StartPage())
		{
			m_Bank[a].FreeMemInc(pItem->Count());
			break;
		}
	m_InUse	-=	pItem->Count()*CRY_MM_PAGE_SIZE;
	pItem->Free();

#ifdef CRY_MM_DEBUG
	CRY_DEBUGOUT("%dByte freed  %d%% of %dMB in use, Offset:0x%x\n",pItem->Count()*CRY_MM_PAGE_SIZE,m_InUse/(m_Size/100),m_Size>>20,ID);
#endif
	//if next memblock is not allocated as well, merge blocks to lower the fragmentation
	CCryDXPSGCMMemItem*	pPrev	=	pItem->Prev();
	Merge(pItem);
	if(pPrev && pPrev->IsFree())
		Merge(pPrev);

#if defined(CRY_MM_PROFILE)
	uint64	T1	=	rdtsc();
	m_FreeTime+=T1-T0;
	m_Frees++;
	printf("Freetime: %lld Current: %lld Count: %lld Average: %lld\n",m_FreeTime,T1-T0,m_Frees,m_FreeTime/m_Frees);
#endif


}

void CCryDXPSGCMMemMan::Lock(uint32 ID)
{
	CCryDXPSGCMMemItem::ID2Pointer(ID)->Lock();
}

void CCryDXPSGCMMemMan::Size(ICrySizer* Sizer)
{
	{
		SIZER_COMPONENT_NAME(Sizer,"DXPS VMem manager");		
		{
			SIZER_COMPONENT_NAME(Sizer,"DXPS VMem manager Items");			
		}
	}
}

#if defined(CRY_MM_DEBUG_DEFRAG)
void CCryDXPSGCMMemMan::DrawDebug(uint32 DebugMode,void* pTarget)
{
	if(!m_pTarget[0])
	{
		m_pTarget[0]=pTarget;
		return;
	}
	if(!m_pTarget[1])
	{
		m_pTarget[1]=pTarget;
	}

	pTarget=(pTarget==m_pTarget[0])?m_pTarget[1]:m_pTarget[0];

	for(uint32 a=0;a<CRY_MM_BANK_COUNT;a++)
		if(m_Bank[a].Valid())
			m_Bank[a].DrawDebug(DebugMode,pTarget,a*CRY_MM_PAGEBANK_COUNT,0,a*720/CRY_MM_BANK_COUNT,1024,(a+1)*720/CRY_MM_BANK_COUNT-4);

}
#endif



