#ifndef __CRYDXPSGCM_MEMITEM__
#define __CRYDXPSGCM_MEMITEM__

#include "CCryDXPSGCM_MemConfig.hpp"


class CCryDXPSGCMMemItem
{
	friend class CCryDXPSGCMMemItemList;
	static CCryDXPSGCMMemItem	m_Items[CRY_MM_ITEM_COUNT];
	uint16																m_Prev;
	uint16																m_Next;
	tdMemItemID														m_StartPage;
	tdMemItemID														m_PageCount:sizeof(tdMemItemID)*8-3;
	tdMemItemID														m_FreeMem:1;
	tdMemItemID														m_Locked:1;	//e.g. can be set during Map/Unmap Lock/Unlock or using as current Texture,Rendertarget,Vertexbuffer etc.
	tdMemItemID														m_SpecialAlignment:1;	//e.g. can be set during Map/Unmap Lock/Unlock or using as current Texture,Rendertarget,Vertexbuffer etc.

	inline void 													ForcePrev(uint16 Prev){m_Prev=Prev;}
	inline void 													ForceNext(uint16 Next){m_Next=Next;}
	inline uint16 												ForcePrev(){return m_Prev;}
	inline uint16 												ForceNext(){return m_Next;}
public:

	inline CCryDXPSGCMMemItem*						Prev(){return m_Prev?&m_Items[m_Prev]:0;}
	inline CCryDXPSGCMMemItem*						Next(){return m_Next?&m_Items[m_Next]:0;}
	inline const CCryDXPSGCMMemItem*			Prev()const{return m_Prev?&m_Items[m_Prev]:0;}
	inline const CCryDXPSGCMMemItem*			Next()const{return m_Next?&m_Items[m_Next]:0;}
	inline void	Prev(CCryDXPSGCMMemItem*	pPrev){	m_Prev=pPrev?pPrev->ID():0;	}
	inline void Next(CCryDXPSGCMMemItem*	pNext){	m_Next=pNext?pNext->ID():0;	}
	inline uint32													Count()const{return m_PageCount;}
	inline void														StartPage(uint32 StartPage){m_StartPage=StartPage;}
	inline uint32													StartPage()const{return m_StartPage;}
	inline void														Count(uint32 Count){m_PageCount=Count;}
	inline uint32 												ID()const{return this-&m_Items[0];}
	static inline CCryDXPSGCMMemItem*			ID2Pointer(uint32 ID){return &m_Items[ID];}
	inline uint32													IsFree()const{return m_FreeMem;}
	inline void														Free()
																				{
																					m_FreeMem=1;
																					Unlock();
																				}
	inline void														InUse()
																				{
																					m_FreeMem=0;
																				}
	inline uint32													IsLocked()const{return m_Locked;}
	inline void														Lock(){m_Locked=1;}
	inline void														Unlock(){m_Locked=0;}
	inline void														Alignement(bool A){m_SpecialAlignment=A;}
	inline bool														Alignement()const{return m_SpecialAlignment;}



	//debugging
	void																	Validate();
};

#endif

