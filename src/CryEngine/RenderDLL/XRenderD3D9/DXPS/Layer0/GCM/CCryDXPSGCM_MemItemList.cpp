#include "StdAfx.h"
#include "../../CCryTypes.hpp"
#include "CCryDXPSGCM_MemItemList.hpp"
#include "../CCryDXPS.hpp"

void CCryDXPSGCMMemItemList::InitAsFreelist()
{
	m_pFirst	=	&CCryDXPSGCMMemItem::m_Items[1];//The 0 one is skipped as it's the null-ptr of the list
	m_pLast		=	&CCryDXPSGCMMemItem::m_Items[CRY_MM_ITEM_COUNT-1];
	for(uint32 a=1;a<CRY_MM_ITEM_COUNT;a++)
	{
		CCryDXPSGCMMemItem::m_Items[a].Free();
		CCryDXPSGCMMemItem::m_Items[a].ForcePrev(a-1);//first elements sets null-ptr automatically
		CCryDXPSGCMMemItem::m_Items[a].ForceNext(a+1);
	}
	CCryDXPSGCMMemItem::m_Items[CRY_MM_ITEM_COUNT-1].ForceNext(0);
}

void CCryDXPSGCMMemItemList::Validate()
{
	//one-sided empty?
	if(!First() || !Last())
	{
		if(First()!=Last())
		{
			CRY_DEBUGOUT("ERROR validating itemlist, just one end is NULL\n");
			snPause();
		}
	}

	// endles linking?
	CCryDXPSGCMMemItem* pPrev	=	0;
	CCryDXPSGCMMemItem* pItem	=	First();
	while(pItem)
	{
		if(pPrev!=pItem->Prev())
		{
			CRY_DEBUGOUT("ERROR validating itemlist, endles linking NULL\n");
			snPause();
		}
		pPrev	=	pItem;
		pItem	=	pItem->Next();
	}

	if(pPrev!=Last())
	{
			CRY_DEBUGOUT("ERROR validating itemlist, brocken list, does not end at specified Last item\n");
			snPause();
	}

}

CCryDXPSGCMMemItem* CCryDXPSGCMMemItemList::Find(uint32 ID)
{
	for(CCryDXPSGCMMemItem* pItem=First();pItem;pItem=pItem->Next())
		if(pItem->ID()==ID)
			return pItem;
	return 0;
}



CCryDXPSGCMMemItem* CCryDXPSGCMMemItemList::Pop()
{
	CRY_MM_VALIDATE;
	if(!m_pFirst)
		return 0;

	CCryDXPSGCMMemItem* pRet	=	m_pFirst;

	m_pFirst	=	m_pFirst->Next();

	if(m_pFirst)	//if an element exists
		m_pFirst->ForcePrev(0);	//set prev ptr of this element to 0
	else
		m_pLast	=	0;	//set ptr to last element to 0 if ptr to first is zero as well

	CRY_MM_VALIDATE;
	return pRet;
}

void CCryDXPSGCMMemItemList::AddAsFirst(CCryDXPSGCMMemItem* pItem)
{
	CRY_MM_VALIDATE;
	if(!m_pFirst)
	{
		m_pFirst	=	m_pLast	=	pItem;
		pItem->ForcePrev(0);
		pItem->ForceNext(0);
	}
	else
	{
		pItem->Next(m_pFirst);
		pItem->ForcePrev(0);
		m_pFirst->Prev(pItem);
		m_pFirst	=	pItem;
	}
	CRY_MM_VALIDATE;
}

void CCryDXPSGCMMemItemList::AddAsLast(CCryDXPSGCMMemItem* pItem)
{
	CRY_MM_VALIDATE;
	if(!m_pLast)
	{
		m_pFirst	=	m_pLast	=	pItem;
		pItem->ForcePrev(0);
		pItem->ForceNext(0);
	}
	else
	{
		pItem->Prev(m_pLast);
		pItem->ForceNext(0);
		m_pLast->Next(pItem);
		m_pLast	=	pItem;
	}
	CRY_MM_VALIDATE;
}

void CCryDXPSGCMMemItemList::AddBefore(CCryDXPSGCMMemItem* pItem,CCryDXPSGCMMemItem* pPostItem)
{
	CRY_MM_VALIDATE;
#if defined(CRY_MM_DEBUG_ITEMLIST)
	CCryDXPSGCMMemItem*	pEmptyItem	=	Find(pPostItem->ID());
	if(!pEmptyItem)
	{
		CRY_DEBUGOUT("Trying to insert vmem-item before another item that is not part of this itemlist\n");
		snPause();
	}
#endif
	if(!m_pLast)
	{
		CRY_DEBUGOUT("!!!Adding item before an other in an empty ItemList!!!\n");
		m_pFirst	=	m_pLast	=	pItem;
		pItem->ForcePrev(0);
		pItem->ForceNext(0);
	}
	else
	{
		if(!pPostItem->ForcePrev())//first item?
		{
			if(pPostItem!=m_pFirst)
			{
				CRY_DEBUGOUT("!!!Adding item before an other as first Element even tho the list is not pointing to it !!!\n");
				snPause();
			}
			m_pFirst	=	pItem;
			pPostItem->ForcePrev(pItem->ID());
			pItem->ForcePrev(0);
			pItem->ForceNext(pPostItem->ID());
		}
		else
		{
			pItem->ForcePrev(pPostItem->ForcePrev());
			pItem->ForceNext(pPostItem->ID());
			pPostItem->ForcePrev(pItem->ID());
			pItem->Prev()->ForceNext(pItem->ID());
		}
	}
	CRY_MM_VALIDATE;
}
