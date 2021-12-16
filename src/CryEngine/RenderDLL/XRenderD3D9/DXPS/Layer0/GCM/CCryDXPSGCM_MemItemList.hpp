#ifndef __CRYDXPSGCM_MEMITEMLIST__
#define __CRYDXPSGCM_MEMITEMLIST__

#include "CCryDXPSGCM_MemConfig.hpp"
#include "CCryDXPSGCM_MemItem.hpp"


class CCryDXPSGCMMemItemList
{
	CCryDXPSGCMMemItem*	m_pFirst;
	CCryDXPSGCMMemItem*	m_pLast;
public:
	void InitAsFreelist();

	CCryDXPSGCMMemItem* First(){return m_pFirst;}
	CCryDXPSGCMMemItem* Last(){return m_pLast;}

	CCryDXPSGCMMemItem*	Pop();
	void AddBefore(CCryDXPSGCMMemItem* pItem,CCryDXPSGCMMemItem* pPostItem);
	void AddAsFirst(CCryDXPSGCMMemItem* pItem);
	void AddAsLast(CCryDXPSGCMMemItem* pItem);
	uint32 Empty()const{return m_pFirst==0;}

	//debugging
	CCryDXPSGCMMemItem* Find(uint32 ID);
	void	Validate();


};

#endif

