#include "StdAfx.h"
#include "../../CCryTypes.hpp"
#include "CCryDXPSGCM_MemItem.hpp"

CCryDXPSGCMMemItem	CCryDXPSGCMMemItem::m_Items[CRY_MM_ITEM_COUNT];

void CCryDXPSGCMMemItem::Validate()
{
	//one-sided empty?

	if(m_Prev==m_Next)
	{
		CRY_DEBUGOUT("ERROR: vmem-item linking to self\n");
		int a=1;
		while(a)
		{
		}
	}

	// endles linking?
	CCryDXPSGCMMemItem* pPrev	=	this;
	CCryDXPSGCMMemItem* pItem	=	Next();
	while(pItem)
	{
		if(pPrev!=pItem->Prev())
		{
			CRY_DEBUGOUT("ERROR validating vmem-item, endles next-linking NULL\n");
			int a=1;
			while(a)
			{
			}
		}
		pPrev	=	pItem;
		pItem	=	pItem->Next();
	}

	pPrev	=	this;
	pItem	=	Prev();
	while(pItem)
	{
		if(pPrev!=pItem->Next())
		{
			CRY_DEBUGOUT("ERROR validating vmem-item, endles prev-linking NULL\n");
			int a=1;
			while(a)
			{
			}
		}
		pPrev	=	pItem;
		pItem	=	pItem->Prev();
	}

}