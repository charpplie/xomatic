#ifndef __CRYDXPSGCM_MEMBANK__
#define __CRYDXPSGCM_MEMBANK__

#include "CCryDXPSGCM_MemConfig.hpp"
#include "CCryDXPSGCM_MemItemList.hpp"

class CCryDXPSGCMMemBank
{
private:
	CCryDXPSGCMMemItemList	m_ItemList;
	tdMemItemID							m_StartPage;
	uint32									m_FreeMem;
	uint8										m_Valid:1;
public:
	inline 									CCryDXPSGCMMemBank():
													m_StartPage(~0),
													m_Valid(0){};
	inline	void						Init(uint32 Valid,uint32 StartPage,uint32 FreeMem){m_Valid=Valid;m_StartPage=StartPage;m_FreeMem=FreeMem;}
	inline	uint32					Valid()const{return m_Valid;}
	inline	uint32					FreeMem()const{return m_FreeMem;}
	inline	void						FreeMemDec(uint32 Count){m_FreeMem-=Count;}
	inline	void						FreeMemInc(uint32 Count){m_FreeMem+=Count;}
	inline	void						Validate(uint32 V){m_Valid=V?1:0;}

	tdMemItemID							StartPage()const{return m_StartPage;}
	uint32 									Alloc(uint32 Count,uint32 Allign,CCryDXPSGCMMemItemList& rItemFreeList);
	uint32 									UnUsed()const{return m_ItemList.Empty();}

#if defined(CRY_MM_DEBUG_DEFRAG)
	void										DrawDebug(uint32 DebugMode,void* pTarget,uint32 Offset,int32 X0,int32 Y0,int32 X1,int32 Y1);
#endif

};


#endif

