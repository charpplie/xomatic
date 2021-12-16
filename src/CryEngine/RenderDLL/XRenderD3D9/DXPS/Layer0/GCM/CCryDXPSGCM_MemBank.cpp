#include "StdAfx.h"
#include "../../CCryTypes.hpp"
#include "CCryDXPSGCM_MemBank.hpp"
#include <stdio.h>
#include "../CCryDXPS.hpp"

uint32 CCryDXPSGCMMemBank::Alloc(uint32 Count,uint32 Allign,CCryDXPSGCMMemItemList& rItemFreeList)
{
	if(Count>CRY_MM_PAGEBANK_COUNT)
	{
		CRY_DEBUGOUT("Page request exceeds max number of pages per Bank\n");
		return 0;
	}

	if(!m_Valid)
	{
		CRY_DEBUGOUT("!!!Tried to allocate pages in Invalid Bank!!!\n");
		return 0;
	}

//	CRY_DEBUGOUT("\nseeking best fitting freeItem\n");
	//if first time usage, initialise big free block
	if(m_ItemList.Empty())
	{
		CCryDXPSGCMMemItem* pItem	=	rItemFreeList.Pop();
		if(!pItem)
		{
			CRY_DEBUGOUT("Could not get Item from Freelist for this Bank\n");
			return 0;
		}
		m_ItemList.AddAsFirst(pItem);
		pItem->Count(m_FreeMem);
		if(pItem->Count()!=m_FreeMem)
		{
			CRY_DEBUGOUT("pItem->Count()==%d m_FreeMem==%d\n",pItem->Count(),m_FreeMem);
		}
		pItem->StartPage(m_StartPage);
		pItem->Free();
	}

	//garbage collect items that could not be unlinked
	if(m_ItemList.First()->IsFree() && m_ItemList.First()->Count()==0)
		rItemFreeList.AddAsFirst(m_ItemList.Pop());

	//seek smallest block with enough pages
	CCryDXPSGCMMemItem* pBestItem=0;
	if(Allign)
		for(CCryDXPSGCMMemItem* pItem=m_ItemList.First();pItem;pItem=pItem->Next())
		{
//		CRY_DEBUGOUT("Item: Free:%s PageCount:%d\n",pItem->IsFree()?"Yes":"No",pItem->Count());
			//kinda slow but helps reduce memory fragmentation
			if(pItem->IsFree()	&& (!pBestItem || pItem->Count()<pBestItem->Count()))
			{
				uint32 Offset	=	pItem->StartPage()&(Allign-1);
				if(Offset>0)
					Offset	=	Allign-Offset;
				if(Count+Offset<=pItem->Count())
					pBestItem	=	pItem;
			}
		}
	else
		for(CCryDXPSGCMMemItem* pItem=m_ItemList.First();pItem;pItem=pItem->Next())
		{
//		CRY_DEBUGOUT("Item: Free:%s PageCount:%d\n",pItem->IsFree()?"Yes":"No",pItem->Count());
			//kinda slow but helps reduce memory fragmentation
			if(pItem->IsFree() && pItem->Count()>=Count && (!pBestItem || pItem->Count()<pBestItem->Count()))
				pBestItem	=	pItem;
		}

	if(!pBestItem)
	{
//		CRY_DEBUGOUT("no block with enough space found\n");
		return 0;
	}
			

	CCryDXPSGCMMemItem* pItem	= rItemFreeList.Pop();
	if(!pItem)
	{
		CRY_DEBUGOUT("Could not get Item from Freelist for this Bank(2)\n");//should never ever happen, impossible
		return 0;	//no block left
	}

	//split space for ne allocation
	uint32 Offset	=	pBestItem->StartPage()&(Allign-1);
	if(Allign && Offset)
	{
		//if padding needed for allignment, return offseted item

		CCryDXPSGCMMemItem* pItemStart	= rItemFreeList.Pop();
		if(!pItemStart)
		{
			CRY_DEBUGOUT("Could not get Item from Freelist for this Bank(2)\n");//should never ever happen, impossible
			return 0;	//no block left
		}


		Offset	=	Allign-Offset;

		pItem->InUse();
		m_ItemList.AddBefore(pItem,pBestItem);
		m_ItemList.AddBefore(pItemStart,pItem);

		pItem->Count(Count);
		pItemStart->Count(Offset);
		pBestItem->Count(pBestItem->Count()-Count-Offset);

		pItem->StartPage(pBestItem->StartPage()+Offset);
		pItemStart->StartPage(pBestItem->StartPage());
		pBestItem->StartPage(pBestItem->StartPage()+Count+Offset);
	}
	else
	{
		pItem->InUse();
		m_ItemList.AddBefore(pItem,pBestItem);
		pItem->Count(Count);
		pBestItem->Count(pBestItem->Count()-Count);
		pItem->StartPage(pBestItem->StartPage());
		pBestItem->StartPage(pBestItem->StartPage()+Count);
	}

	return pItem->ID();
}




#if defined(CRY_MM_DEBUG_DEFRAG)
void FillRect(void* pTarget,int32 X0,int32 Y0,int32 X1,int32 Y1,uint32 C)
{
	if(X0<0)
		X0=0;
	if(X1>=1024)
		X1=1023;
	uint32* pData	=	(uint32*)pTarget;
	for(int32 y=Y0;y<Y1;y+=2)
	for(int32 x=X0;x<X1;x++)
		pData[x+y*1280]	=	C;
}

void CCryDXPSGCMMemBank::DrawDebug(uint32 DebugMode,void* pTarget,uint32 Offset,int32 X0,int32 Y0,int32 X1,int32 Y1)
{
	int32 Lines	=	CRY_MM_PAGEBANK_COUNT/(X1-X0);
	int32 YAdd	=	(Y1-Y0)/Lines;
	if(YAdd<=0)
		YAdd=1;
	uint32 Switch=0;
	for(CCryDXPSGCMMemItem* pItem=m_ItemList.First();pItem;pItem=pItem->Next())
	{
		uint32 XStart	=	pItem->StartPage()-Offset;
		uint32 XEnd		=	XStart+pItem->Count();
		uint32 C;
		if(DebugMode==1)
		{
			C	=	pItem->Count()<255?pItem->Count():255;
			C	|=	(C<<8)|0xff0000;
			C	=	pItem->IsFree()?0x0:C;
		}
		else
		if(DebugMode==2)
		{
			C	=	0xff00<<Switch;
			C	=	pItem->IsFree()?0x0:C;
			Switch	^=	8;
		}
		else
		if(DebugMode==3)
		{
			C	=	pItem->IsLocked()?0xff0000:0x00ff00;
			C	=	pItem->IsFree()?0x0:C;
		}
		for(int32 a=XStart/1024;a<=XEnd/1024 && a*YAdd<Y1;a++)
//		for(int32 a=XStart/(Lines*1024);a<=XEnd/(Lines*1024);a++)
//		for(int32 a=0;a<Lines;a++)
			FillRect(pTarget,XStart-a*1024,Y0+a*YAdd,XEnd-a*1024,Y0+(a+1)*YAdd,C);
//		FillRect(pTarget,XStart-1024,Y0+YAdd1,XEnd-1024,Y0+YAdd2,C);
//		FillRect(pTarget,XStart-2048,Y0+YAdd2,XEnd-2048,Y0+YAdd3,C);
//		FillRect(pTarget,XStart-3072,Y0+YAdd3,XEnd-3072,Y1      ,C);
	}
}
#endif


