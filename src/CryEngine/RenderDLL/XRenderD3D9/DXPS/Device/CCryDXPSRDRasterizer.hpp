#ifndef __CRYDXPSRDRASTERIZER__
#define __CRYDXPSRDRASTERIZER__


typedef float						tdDXPSRDepth;

class CDXPSRProf
{
	SPU_DOMAIN_LOCAL uint32* m_pValue;
	uint32	m_T0;
public:
					CDXPSRProf(SPU_DOMAIN_LOCAL uint32* pValue):
					m_pValue(pValue)
					{
						m_T0	= rdtsc();
					}
					~CDXPSRProf()
					{
						const uint32 T1 = rdtsc();
						*m_pValue	+=	m_T0-T1;
					}
};

struct SDXPSRasStats
{
	uint32											m_TimeClear;
	uint32											m_TimeRasVertex;
	uint32											m_TimeRasTriangle;
	uint32											m_TimeFlush;
	uint32											m_CountDrawcalls;
	uint32											m_CountTriangles;
	uint32											m_VBCacheHit;
	uint32											m_VBCacheMiss;
	uint32											m_ZBCacheHit;
	uint32											m_ZBCacheMiss;
	uint32											m_ZBStall;
	uint32											m_ZBCoarseFail;
	uint32											m_ZBCoarsePass;
	uint32											m_ZBCoarseFineTest;
	uint32											m_VBVertexAssembly;
	uint32											m_VBVertexAssemblyStall;
	uint32											m_ZCUpdate;
	uint32											m_ZCCulled;
	uint32											m_ZCPassed;
}_ALIGN(128);

//#define DXPSR_PROFILE_TIMINGS
#ifdef DXPSR_PROFILE_TIMINGS
	//#define DXPSR_PROFILE_DETAILED
#endif

//#define DXPSR_PROFILE_ZC
//#define DXPSR_PROFILE_VB

#define	DXPSR_FAST_ZINTERP
//#define RILNE		NO_INLINE
//#define RILNM		NO_INLINE
#define RILNE		ILINE
#define RILNM		ILINE



//#define DXPSR_PROFILE_VBCACHE(X) X++;
#define DXPSR_PROFILE_VBCACHE(X)

#ifdef DXPSR_PROFILE_TIMINGS
//#define DXPSR_PROFILE_VASTALL(X) CDXPSRProf Prof##__LINE__(&X);
#endif
#define DXPSR_PROFILE_VASTALL(X)

//#define DXPSR_PROFILE_ZBCACHE(X) X++;
#define DXPSR_PROFILE_ZBCACHE(X)
//#define DXPSR_PROFILE_ZBSTALL(X) CDXPSRProf Prof##__LINE__(&X);
#define DXPSR_PROFILE_ZBSTALL(X)





const tdDXPSRDepth DXPSRDEPTH_MAX	=	1.f;

enum{DXPS_RASTERIZER_JOBCOUNT=4};
enum{DXPS_RASTERIZER_DRAWCALLCOUNT=1024};
enum{DXPS_RASTERIZER_RESOLUTION=512};

const float DXPS_RASTERIZE_HALFPIXELOFFSET	=	0.5f/static_cast<float>(DXPS_RASTERIZER_RESOLUTION);

	class CDXPSRJob
	{
		uint32*										m_pDepthBuffer;
		uint32										m_DrawcallStart;
		uint32										m_DrawcallEnd;
		uint32										m_ViewPortSizeX;
		uint32										m_ViewPortSizeY;

	public:
		RILNE void								ViewPort(int X0,int Y0,int X1,int Y1)
															{
																m_ViewPortSizeX	=	X1-X0;
																m_ViewPortSizeY	=	Y1-Y0;
															}
		RILNE void								DepthBuffer(CCryDXPSDepthStencilView* pDepthBuffer)
															{
																CCryDXPSTexture2D* pTex	=	pDepthBuffer->Texture();
																m_pDepthBuffer	=	pTex?reinterpret_cast<uint32*>(pTex->RawPointer()):0;
															}

		RILNE void								DrawcallStart(uint32 DS)
															{
																m_DrawcallStart=DS;
															}
		RILNE void								DrawcallEnd(uint32 DE)
															{
																m_DrawcallEnd=DE;
															}

		RILNE uint32*							DepthBuffer(){return m_pDepthBuffer;}
		RILNE uint32							DrawcallStart()const{return m_DrawcallStart;}
		RILNE uint32							DrawcallEnd()const{return m_DrawcallEnd;}
		RILNE uint32							ViewPortSizeX()const{return m_ViewPortSizeX;}
		RILNE uint32							ViewPortSizeY()const{return m_ViewPortSizeY;}

	} _ALIGN(16);

	class CDXPSRDrawCall
	{
		Matrix44A									m_ViewMat;
		void*											m_pVtxPos;
		uint16*										m_pIndices;
		uint32										m_IndexCount;
		uint16										m_VertexCount;
		uint16										m_VertPosStride;
	public:
		RILNE	void								WorldViewProjMatrix(const Matrix44A& rViewMat)
															{
																m_ViewMat	=	rViewMat;
															}
		RILNE void								IndexBuffer(uint16* pIdxBuffer,uint32 IndexCount)
															{
																m_pIndices				=	pIdxBuffer;
																m_IndexCount			=	IndexCount;
															}
		RILNE void								VertexBuffer(void* pVtxPos,uint32 VertPosStride,uint32 VertCount)
															{
																m_pVtxPos					=	pVtxPos;
																m_VertexCount			=	VertCount>(1<<(sizeof(m_VertexCount)*8))-1?(1<<(sizeof(m_VertexCount)*8))-1:VertCount;
																m_VertPosStride		=	VertPosStride;
															}


		const Matrix44A&					ViewMat()const{return m_ViewMat;}
		const void*								VtxBufferPos()const{return m_pVtxPos;}
		const uint16*							IndexBuffer()const{return m_pIndices;}
		uint32										IndexCount()const{return m_IndexCount;}
		uint32										VertexCount()const{return m_VertexCount;}
		uint32										VertexStride()const{return m_VertPosStride;}
	} _ALIGN(128);

class CDXPSRDRasterizer
{
	CDXPSRDrawCall							m_DrawCalls[DXPS_RASTERIZER_DRAWCALLCOUNT] _ALIGN(128);
	CDXPSRDrawCall							m_DrawCall;
	tdDXPSRDepth								m_Buffer[DXPS_RASTERIZER_RESOLUTION*DXPS_RASTERIZER_RESOLUTION]	_ALIGN(128);
	const void*									m_DrawCallsVtxPos[DXPS_RASTERIZER_DRAWCALLCOUNT] _ALIGN(128);
	CDXPSRJob										m_Job;
	Matrix44A										m_ViewProj;
	uint32											m_CMDBPut;
#ifdef DXPSR_PROFILE_TIMINGS
	SDXPSRasStats								m_Stats;
#endif
//	void												Clear(CDXPSRJob& rJob);
//	void												Copy(CDXPSRJob& rJob);
//	void												ZBCacheFlush();
//	void												ZBCacheTile(uint32 x,uint32 y,SPU_DOMAIN_LOCAL tdDXPSRDepth*& pIn,SPU_DOMAIN_LOCAL tdDXPSRDepth*& pOut,bool& NeedSync);
//	void												Triangulate(float x0,float y0,float z0,float x1,float y1,float z1,float x2,float y2,float z2);
//	void												Triangle(qword qV0,qword qV1,qword qV2);























	RILNE CDXPSRJob&						ThisJob()
															{
																return	m_Job;
															}
	RILNE CDXPSRDrawCall&				ThisDrawCall()
															{
																return	m_DrawCall;
															}
	RILNE	uint32								NextDrawcallID(uint32 ID)
															{
																return (ID+1)%DXPS_RASTERIZER_DRAWCALLCOUNT;
															}

				void									Notify();

	RILNE void									PushJob()
															{
																Notify();
															}
	RILNE void									PushDrawCall()
															{
																m_DrawCalls[m_CMDBPut]=m_DrawCall;
																m_DrawCallsVtxPos[m_CMDBPut] = m_DrawCall.VtxBufferPos();
																m_CMDBPut	=	NextDrawcallID(m_CMDBPut);
															}

public:

	void												Init(){}

	void												Finish();

#if !defined(__SPU__)
															CDXPSRDRasterizer();
	RILNE void									Draw()
															{
																PushDrawCall();
															}
	RILNE void									IndexBuffer(uint16* pIdxBuffer,uint32 IndexCount)
															{
																ThisDrawCall().IndexBuffer(pIdxBuffer,IndexCount);
															}
	RILNE void									VertexBuffer(void* pVtxPos,uint32 VertPosStride,uint32 VertexCount)
															{
																ThisDrawCall().VertexBuffer(pVtxPos,VertPosStride,VertexCount);
															}
	RILNE	void									ViewProjMatrix(float* pMat)
															{
																m_ViewProj	=	*reinterpret_cast<Matrix44*>(pMat);
															}
	RILNE	void									World(float* pMat)
															{
																float Mat[16];
																Mat[0]	=	pMat[0];
																Mat[1]	=	pMat[1];
																Mat[2]	=	pMat[2];
																Mat[3]	=	pMat[3];
																Mat[4]	=	pMat[4];
																Mat[5]	=	pMat[5];
																Mat[6]	=	pMat[6];
																Mat[7]	=	pMat[7];
																Mat[8]	=	pMat[8];
																Mat[9]	=	pMat[9];
																Mat[10]	=	pMat[10];
																Mat[11]	=	pMat[11];
																Mat[12]	=	0.f;
																Mat[13]	=	0.f;
																Mat[14]	=	0.f;
																Mat[15]	=	1.f;
																const float SCALE		=	DXPS_RASTERIZER_RESOLUTION/2;
																const float OFFSET	=	SCALE-DXPS_RASTERIZE_HALFPIXELOFFSET;
																Matrix44A ScreenMat(	SCALE,0.f,0.f,OFFSET,
																										0.f,-SCALE,0.f,OFFSET,
																										0.f,0.f,1.f,0.f,
																										0.f,0.f,0.f,1.f);
																Matrix44A	WorldViewProj	=	ScreenMat*m_ViewProj* *reinterpret_cast<Matrix44*>(Mat);
																WorldViewProj.Transpose();
																ThisDrawCall().WorldViewProjMatrix(WorldViewProj);
																
															}
	RILNE void									ViewPort(int X0,int Y0,int X1,int Y1)
															{
																ThisJob().ViewPort(X0,Y0,X1,Y1);
//																NextJob(ThisJob<CDXPSRDJViewPortPPU>().Setup(rViewPort));
															}
	RILNE void									ScissorRect(const int MinX,const int MinY,const int MaxX,const int MaxY)
															{
																//NextJob(ThisJob<CDXPSRDJUpdateStateScissorPPU>().Setup(MinX,MinY,MaxX,MaxY));
															}

	RILNE void									Clear(const tdDXPSRDepth V)
															{
																ThisJob().DrawcallStart(m_CMDBPut);
															}

	RILNE void									DepthBuffer(CCryDXPSDepthStencilView* pDepthBuffer)
															{
																ThisJob().DepthBuffer(pDepthBuffer);
															}

	RILNE void									CopyBuffer()
															{
																ThisJob().DrawcallEnd(m_CMDBPut);
																PushJob();
															}
	
#endif //ndef __SPU__

} _ALIGN(128);


#endif

