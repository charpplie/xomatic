#ifndef __CRYDXPSGCM_ZCULLMAN__
#define __CRYDXPSGCM_ZCULLMAN__

enum {CRY_ZCM_COUNT		=	8};
enum {CRY_ZCM_MEMSIZE	=	0x2fffff};
enum Notifications
{
	REPORT_ZCULL_STATS0	=	100,
	REPORT_ZCULL_STATS1,
};

class CCryDXPSGCMZCull
{
	uint32							m_Start;
	uint32							m_End;
public:

	void								Init(){m_Start=m_End=0;}
	void								Init(uint32 S,uint32 E){m_Start=S;m_End=E;}
	void								Free(){Init();}
	uint32							Start()const{return m_Start;}
	uint32							End()const{return m_End;}
	bool								InUse()const{return m_Start!=m_End;}
	bool								Overlaps(const CCryDXPSGCMZCull& rRegion)const{return !(rRegion.m_End<m_Start || rRegion.m_Start>=m_End);}

};

class CCryDXPSGCMZCullMan
{
private:
	CCryDXPSGCMZCull		m_ZCull[CRY_TRM_COUNT];
	uint32							m_Count;

	uint32							Alloc()
											{
												for(uint32 a=0;a<CRY_ZCM_COUNT;a++)
													if(!m_ZCull[a].InUse())
													{
														m_Count++;
														return a;
													}
												return CRY_ZCM_COUNT;
											}
public:
											CCryDXPSGCMZCullMan();
	void								Init();

	uint32							Create(uint32 Offset,uint32 SizeX,uint32 SizeY);
	void								Free(uint32 ID);
	void								RecalcHeuristic();
	void								Size(class ICrySizer* Sizer);
	uint32							Count()const{return m_Count;}
};

#endif

