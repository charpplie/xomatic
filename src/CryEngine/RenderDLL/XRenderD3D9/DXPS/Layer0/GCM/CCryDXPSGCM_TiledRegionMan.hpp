#ifndef __CRYDXPSGCM_TILEDREGIONMAN__
#define __CRYDXPSGCM_TILEDREGIONMAN__

enum {CRY_TRM_COUNT		=	15};
enum {CRY_TRM_MEMSIZE	=	0x7ff};

class CCryDXPSGCMRegion
{
	uint16							m_Start;
	uint16							m_End;
public:

	void								Init(){m_Start=m_End=0;}
	void								Init(uint16 S,uint16 E){m_Start=S;m_End=E;}
	void								Free(){Init();}
	uint16							Start()const{return m_Start;}
	uint16							End()const{return m_End;}
	bool								InUse()const{return m_Start!=m_End;}
	bool								Overlaps(const CCryDXPSGCMRegion& rRegion)const{return !(rRegion.m_End<m_Start || rRegion.m_Start>=m_End);}

};

class CCryDXPSGCMTiledRegionMan
{
private:
	CCryDXPSGCMRegion		m_Region[CRY_TRM_COUNT];

	uint32							Alloc()
											{
												for(uint32 a=0;a<CRY_TRM_COUNT;a++)
													if(!m_Region[a].InUse())
														return a;
												return CRY_TRM_COUNT;
											}
public:
											CCryDXPSGCMTiledRegionMan();
	void								Init();

	uint32							Create(uint32 Offset,uint32 Pitch,uint32 Size,uint32 Compression);
	void								Free(uint32 ID);
	void								Size(class ICrySizer* Sizer);
};

#endif

