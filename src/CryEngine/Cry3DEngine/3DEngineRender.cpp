////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   3denginerender.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: rendering
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "3dEngine.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "terrain_water.h"
#include "IParticles.h"
#include "DecalManager.h"
#include "SkyLightManager.h"
#include "terrain.h"
#include "CullBuffer.h"
#include "WaterVolumes.h"
#include "LightEntity.h"
#include "FogVolumeRenderNode.h"
#include "ObjectsTree.h"
#include "WaterWaveRenderNode.h"
#include "CloudsManager.h"
#include "MatMan.h"
#include "VolumeObjectRenderNode.h"
#include "CryPath.h"
#include "VoxTerrain.h"
#include "ILocalMemoryUsage.h"
#include "BitFiddling.h"
#include "ObjMan.h"

#include "IGameFramework.h"
#include "IRealtimeRemoteUpdate.h"

#ifdef GetCharWidth
#undef GetCharWidth
#endif //GetCharWidth





////////////////////////////////////////////////////////////////////////////////////////
// RenderScene
////////////////////////////////////////////////////////////////////////////////////////

#define DLD_SET_CVAR(VarName,FlagName)\
	int VarName = GetCVars()->VarName;\
	if(!(dwDrawFlags & FlagName))\
	GetCVars()->VarName = 0;\

#define DLD_SET_CVAR_FLOAT(VarName,FlagName)\
  float VarName = GetCVars()->VarName;\
  if(!(dwDrawFlags & FlagName))\
  GetCVars()->VarName = 0;\

#define DLD_RESTORE_CVAR(VarName) \
	GetCVars()->VarName = VarName; \

// for panorama screenshots
class CStitchedImage : public Cry3DEngineBase
{
public:
	CStitchedImage(				C3DEngine&	rEngine, 
									const uint32			dwWidth,
									const uint32			dwHeight,
									const uint32			dwVirtualWidth,
									const uint32			dwVirtualHeight,
									const uint32			dwSliceCount,
									const f32					fTransitionSize,
									const bool				bMetaData=false):
	m_rEngine(rEngine),
	m_dwWidth(dwWidth),
	m_dwHeight(dwHeight),
	m_fInvWidth(1.f/static_cast<f32>(dwWidth)),
	m_fInvHeight(1.f/static_cast<f32>(dwHeight)),
	m_dwVirtualWidth(dwVirtualWidth),
	m_dwVirtualHeight(dwVirtualHeight),
	m_fInvVirtualWidth(1.f/static_cast<f32>(dwVirtualWidth)),
	m_fInvVirtualHeight(1.f/static_cast<f32>(dwVirtualHeight)),
	m_nFileId(0),
	m_dwSliceCount(dwSliceCount),
	m_fHorizFOV(2*gf_PI/dwSliceCount),
	m_bFlipY(true),
	m_fTransitionSize(fTransitionSize),
	m_bMetaData(bMetaData)
	{
		assert(dwWidth);
		assert(dwHeight);

		const char *szExtension = m_rEngine.GetCVars()->e_ScreenShotFileFormat->GetString();

		m_bFlipY=(stricmp(szExtension,"tga")==0);

		m_RGB.resize(m_dwWidth*3*m_dwHeight);

		// ratio between width and height defines angle 1 (angle from mid to cylinder edges)
		float fVert1Frac = (2*gf_PI*m_dwHeight)/m_dwWidth;

		// slice count defines angle 2
		float fHorizFrac = tanf(GetHorizFOVWithBorder()*0.5f);
		float fVert2Frac = 2.0f * fHorizFrac / rEngine.GetRenderer()->GetWidth() * rEngine.GetRenderer()->GetHeight();
//		float fVert2Frac = 2.0f * fHorizFrac / rEngine.GetRenderer()->GetWidth() * rEngine.GetRenderer()->GetHeight();

		// the bigger one defines the needed angle
		float fVertFrac = max(fVert1Frac,fVert2Frac);

		// planar image becomes a barrel after projection and we need to zoom in to only utilize the usable part (inner rect)
		// this is not always needed - for quality with low slice count we could be save some quality here
		fVertFrac /= cosf(GetHorizFOVWithBorder()*0.5f);

		// compute FOV from Frac
		float fVertFOV = 2*atanf(0.5f*fVertFrac);

		m_fPanoramaShotVertFOV = fVertFOV;

		CryLog("RenderFov = %f degrees (%f = max(%f,%f)*fix)",RAD2DEG(m_fPanoramaShotVertFOV), fVertFrac,fVert1Frac,fVert2Frac);
		Clear();
	}

	void Clear()
	{
		memset(&m_RGB[0],0,m_dwWidth*m_dwHeight*3);
	}

	// szDirectory + "/" + file_id + "." + extension
	// logs errors in the case there are problems
	bool SaveImage( const char *szDirectory )
	{
		assert(szDirectory);

		const char *szExtension = m_rEngine.GetCVars()->e_ScreenShotFileFormat->GetString();

		if(	stricmp(szExtension,"dds")!=0	&&
				stricmp(szExtension,"tga")!=0	&&
				stricmp(szExtension,"jpg")!=0)
		{
			gEnv->pLog->LogError("Format e_ScreenShotFileFormat='%s' not supported",szExtension);
			return false;
		}

		char sFileName[_MAX_PATH];

		snprintf(sFileName, sizeof(sFileName), "%s/ScreenShots/%s", PathUtil::GetGameFolder().c_str(), szDirectory);
		CryCreateDirectory(sFileName,0);

		// find free file id
		for(;;)
		{
			snprintf(sFileName, sizeof(sFileName), "%s/ScreenShots/%s/%.5d.%s",PathUtil::GetGameFolder().c_str(), szDirectory,m_nFileId,szExtension);
	
			FILE * fp = gEnv->pCryPak->FOpen(sFileName,"rb");

			if (!fp)
				break; // file doesn't exist

			gEnv->pCryPak->FClose(fp);
			m_nFileId++;
		}

		bool bOk;

		if(stricmp(szExtension,"dds")==0)
			bOk = gEnv->pRenderer->WriteDDS((byte *)&m_RGB[0],m_dwWidth,m_dwHeight,4,sFileName,eTF_DXT5,1);
		else
		if(stricmp(szExtension,"tga")==0)
			bOk = gEnv->pRenderer->WriteTGA((byte *)&m_RGB[0],m_dwWidth,m_dwHeight,sFileName,24,24);
		else
			bOk = gEnv->pRenderer->WriteJPG((byte *)&m_RGB[0],m_dwWidth,m_dwHeight,sFileName,24);

		if(!bOk)
			gEnv->pLog->LogError("Failed to write '%s' (not supported on this platform?)",sFileName);
		else //write meta data
		{
			if(m_bMetaData)
			{
				const f32	nTSize		=	static_cast<f32>(gEnv->p3DEngine->GetTerrainSize());
				const f32	fSizeX	=	GetCVars()->e_ScreenShotMapSizeX;
				const f32	fSizeY	=	GetCVars()->e_ScreenShotMapSizeY;
				const f32	fTLX	=	GetCVars()->e_ScreenShotMapCenterX-fSizeX;
				const f32	fTLY	=	GetCVars()->e_ScreenShotMapCenterY-fSizeY;
				const f32	fBRX	=	GetCVars()->e_ScreenShotMapCenterX+fSizeX;
				const f32	fBRY	=	GetCVars()->e_ScreenShotMapCenterY+fSizeY;

				snprintf(sFileName, sizeof(sFileName), "%s/ScreenShots/%s/%.5d.%s",PathUtil::GetGameFolder().c_str(),szDirectory,m_nFileId,"xml");

				FILE * metaFile = gEnv->pCryPak->FOpen(sFileName,"wt");
				if(metaFile)
				{
					char sFileData[1024];
					snprintf(sFileData, sizeof(sFileData), "<MiniMap Filename=\"%.5d.%s\" startX=\"%f\" startY=\"%f\" endX=\"%f\" endY=\"%f\"/>",
						m_nFileId,szExtension,fTLX,fTLY,fBRX,fBRY);
					string data(sFileData);
					gEnv->pCryPak->FWrite(data.c_str(), data.size(), metaFile);
					gEnv->pCryPak->FClose(metaFile);
				}
			}
		}

		return bOk;
	}

	// rasterize rectangle
	// Arguments:
	//   x0 - <x1, including
	//   y0 - <y1, including
	//   x1 - >x0, excluding
	//   y1 - >y0, excluding
	void RasterizeRect(			const uint32*	pRGBAImage,
													const uint32	dwWidth,
													const uint32	dwHeight,
													const uint32	dwSliceX,
													const uint32	dwSliceY,
													const f32			fTransitionSize,
													const bool		bFadeBordersX,
													const bool		bFadeBordersY)
	{
//		if(bFadeBordersX|bFadeBordersY)
		{
			//calculate rect inside the whole image
			const int32 OrgX0	=	static_cast<int32>(static_cast<f32>((dwSliceX*dwWidth)*m_dwWidth)*m_fInvVirtualWidth);
			const int32 OrgY0	=	static_cast<int32>(static_cast<f32>((dwSliceY*dwHeight)*m_dwHeight)*m_fInvVirtualHeight);
			const int32 OrgX1	=	min(static_cast<int32>(static_cast<f32>(((dwSliceX+1)*dwWidth)*m_dwWidth)*m_fInvVirtualWidth),static_cast<int32>(m_dwWidth))-(m_rEngine.GetCVars()->e_ScreenShotDebug==1?1:0);
			const int32 OrgY1	=	min(static_cast<int32>(static_cast<f32>(((dwSliceY+1)*dwHeight)*m_dwHeight)*m_fInvVirtualHeight),static_cast<int32>(m_dwHeight))-(m_rEngine.GetCVars()->e_ScreenShotDebug==1?1:0);
			//expand bounds for borderblending
			const int32 CenterX	=	(OrgX0+OrgX1)/2;
			const int32 CenterY	=	(OrgY0+OrgY1)/2;
			const int32 X0	=	static_cast<int32>(static_cast<f32>(OrgX0-CenterX)*(1.f+fTransitionSize))+CenterX;
			const int32 Y0	=	static_cast<int32>(static_cast<f32>(OrgY0-CenterY)*(1.f+fTransitionSize))+CenterY;
			const int32 X1	=	static_cast<int32>(static_cast<f32>(OrgX1-CenterX)*(1.f+fTransitionSize))+CenterX;
			const int32 Y1	=	static_cast<int32>(static_cast<f32>(OrgY1-CenterY)*(1.f+fTransitionSize))+CenterY;
			//clip bounds -- disabled due to some artifacts on edges
			//X0	=	min(max(X0,0),static_cast<int32>(m_dwWidth-1));
			//Y0	=	min(max(Y0,0),static_cast<int32>(m_dwHeight-1));
			//X1	=	min(max(X1,0),static_cast<int32>(m_dwWidth-1));
			//Y1	=	min(max(Y1,0),static_cast<int32>(m_dwHeight-1));
			const f32 InvBlendX	=	1.f/max(static_cast<f32>(X1-OrgX1), 0.01f);//0.5 is here because the border is two times wider then the border of the single segment in total 
			const f32 InvBlendY	=	1.f/max(static_cast<f32>(Y1-OrgY1), 0.01f);
			const int32 DebugScale=	(m_rEngine.GetCVars()->e_ScreenShotDebug==2)?65536:0;
			for(int32 y=max(Y0,0);y<Y1 && y<(int)m_dwHeight;y++)
			{
				const f32 WeightY	=	bFadeBordersY?min(1.f,static_cast<f32>(min(y-Y0,Y1-y))*InvBlendY):1.f;
				for(int32 x=max(X0,0);x<X1 && x<(int)m_dwWidth;x++)
				{
					uint8 *pDst = &m_RGB[m_bFlipY?3*(x+(m_dwHeight-y-1)*m_dwWidth):3*(x+y*m_dwWidth)];
					const f32 WeightX	=	bFadeBordersX?min(1.f,static_cast<f32>(min(x-X0,X1-x))*InvBlendX):1.f;
					GetBilinearFilteredBlend(	static_cast<int32>(static_cast<f32>(x-X0)/static_cast<f32>(X1-X0)*static_cast<f32>(dwWidth)*16.f),
																		static_cast<int32>(static_cast<f32>(y-Y0)/static_cast<f32>(Y1-Y0)*static_cast<f32>(dwHeight)*16.f),
																		pRGBAImage,dwWidth,dwHeight,
																		max(static_cast<int32>(WeightX*WeightY*65536.f),DebugScale),pDst);
				}
			}
		}
/*		else
		{
			const int32 X0	=	static_cast<int32>(static_cast<f32>((dwSliceX*dwWidth)*m_dwWidth)*m_fInvVirtualWidth);
			const int32 Y0	=	static_cast<int32>(static_cast<f32>((dwSliceY*dwHeight)*m_dwHeight)*m_fInvVirtualHeight);
			const int32 X1	=	min(static_cast<int32>(static_cast<f32>(((dwSliceX+1)*dwWidth)*m_dwWidth)*m_fInvVirtualWidth),static_cast<int32>(m_dwWidth))-(m_rEngine.GetCVars()->e_ScreenShotDebug==1?1:0);
			const int32 Y1	=	min(static_cast<int32>(static_cast<f32>(((dwSliceY+1)*dwHeight)*m_dwHeight)*m_fInvVirtualHeight),static_cast<int32>(m_dwHeight))-(m_rEngine.GetCVars()->e_ScreenShotDebug==1?1:0);
			for(int32 y=Y0;y<Y1;y++)
				for(int32 x=X0;x<X1;x++)
				{
					uint8 *pDst = &m_RGB[m_bFlipY?3*(x+(m_dwHeight-y-1)*m_dwWidth):3*(x+y*m_dwWidth)];
					GetBilinearFiltered(static_cast<int32>(static_cast<f32>(x*m_dwVirtualWidth*16)*m_fInvWidth)-((dwSliceX*dwWidth)*16),
																	static_cast<int32>(static_cast<f32>(y*m_dwVirtualHeight*16)*m_fInvHeight)-((dwSliceY*dwHeight)*16),
																	pRGBAImage,dwWidth,dwHeight,pDst);
				}
		}*/
	}

	void RasterizeCylinder( const uint32 *pRGBAImage, 
													const uint32 dwWidth,
													const uint32 dwHeight, 
													const uint32 dwSlice, 
													const bool bFadeBorders)
	{
		float fSrcAngleMin = GetSliceAngle(dwSlice-1);
		float fFractionVert = tanf(m_fPanoramaShotVertFOV*0.5f);
		float fFractionHoriz = fFractionVert * gEnv->pRenderer->GetCamera().GetProjRatio();
		float fInvFractionHoriz = 1.0f / fFractionHoriz;
		float fCameraHorizFov = atanf(fFractionHoriz)*2.f;

		// for soft transition
		float fFadeOutFov = GetHorizFOVWithBorder();
		float fFadeInFov = GetHorizFOV();

		int x0=0, y0=0, x1=m_dwWidth, y1=m_dwHeight;

		float fScaleX = 1.0f/m_dwWidth;
		float fScaleY = 0.5f*fInvFractionHoriz/(m_dwWidth/(2*gf_PI))/dwHeight*dwWidth;								// this value is not correctly computed yet - but using many slices reduced the problem

		if(m_bFlipY)
			fScaleY=-fScaleY;


		// it's more efficient to process colums than lines
		for(int x=x0;x<x1;++x)
		{
			uint8 *pDst = &m_RGB[3*(x+y0*m_dwWidth)];
			float fSrcX = x*fScaleX-0.5f;		// -0.5 .. 0.5
			float fSrcAngleX = fSrcAngleMin+2*gf_PI*fSrcX;

			if(fSrcAngleX>gf_PI)
				fSrcAngleX-=2*gf_PI;
			if(fSrcAngleX<-gf_PI)
				fSrcAngleX+=2*gf_PI;

			if(fabs(fSrcAngleX)>fFadeOutFov*0.5f)
				continue;															// clip away curved parts of the barrel

			float fScrPosX = (tanf(fSrcAngleX) * 0.5f * fInvFractionHoriz + 0.5f) * dwWidth;
//			float fInvCosSrcX = 1.0f / cos(fSrcAngleX);
			float fInvCosSrcX = 1.0f / cosf(fSrcAngleX);

			if(fScrPosX>=0 && fScrPosX<=(float)dwWidth)		// this is an optimization - but it could be done even more efficient
			if(fInvCosSrcX>0)															// don't render the viewer opposing direction
			{
				int iSrcPosX16 = (int)(fScrPosX*16.0f);

				float fYOffset = 16*0.5f*dwHeight - 16*0.5f*m_dwHeight*fScaleY*fInvCosSrcX*dwHeight;
				float fYMul = 16*fScaleY*fInvCosSrcX*dwHeight;

				float fSrcY = y0*fYMul+fYOffset;

				uint32 dwLerp64k = 256*256-1;

				if(!bFadeBorders)
				{
					// first pass - every second image without soft borders
					for(int y=y0;y<y1;++y,fSrcY+=fYMul,pDst+=m_dwWidth*3)
						GetBilinearFiltered(iSrcPosX16,(int)fSrcY,pRGBAImage,dwWidth,dwHeight,pDst);
				}
				else
				{
					// second pass - do all the inbetween with soft borders
					float fOffSlice = fabs(fSrcAngleX/fFadeInFov)-0.5f;					
				
					if(fOffSlice<0)
						fOffSlice=0;			// no transition in this area

					float fBorder = (fFadeOutFov-fFadeInFov)*0.5f;
					
					if(fBorder<0.001f)
						fBorder=0.001f;		// we do not have border
					
					float fFade = 1.0f-fOffSlice*fFadeInFov/fBorder;

					if(fFade<0.0f)
						fFade=0.0f;				// don't use this slice here

					dwLerp64k = (uint32)(fFade*(256.0f*256.0f-1.0f));		// 0..64k

					if(dwLerp64k)															// optimization
						for(int y=y0;y<y1;++y,fSrcY+=fYMul,pDst+=m_dwWidth*3)
							GetBilinearFilteredBlend(iSrcPosX16,(int)fSrcY,pRGBAImage,dwWidth,dwHeight,dwLerp64k,pDst);
				}
			}
		}
	}

	// fast, rgb only
	static inline ColorB lerp( const ColorB x, const ColorB y, const uint32 a, const uint32 dwBase )
	{
		const int32 b = dwBase-a;
		const int32	RC	=	dwBase/2;	//rounding correction


		return ColorB(((int)x.r*b+(int)y.r*a+RC)/dwBase,
									((int)x.g*b+(int)y.g*a+RC)/dwBase,
									((int)x.b*b+(int)y.b*a+RC)/dwBase);
	}

	static inline ColorB Mul( const ColorB x, const int32 a,const int32 dwBase )
	{
		return ColorB(((int)x.r*(int)a)/dwBase,
									((int)x.g*(int)a)/dwBase,
									((int)x.b*(int)a)/dwBase);
	}
	static inline ColorB MadSaturate( const ColorB x, const int32 a,const int32 dwBase , const ColorB y )
	{
		const int32 MAX_COLOR	=	0xff;
		const ColorB PreMuled	=	Mul(x,a,dwBase);
		return ColorB(min((int)PreMuled.r+(int)y.r,MAX_COLOR),
									min((int)PreMuled.g+(int)y.g,MAX_COLOR),
									min((int)PreMuled.b+(int)y.b,MAX_COLOR));
	}

	// bilinear filtering in fixpoint,
	// 4bit fractional part -> multiplier 16 
	// --lookup outside of the image is not defined
	//	lookups outside the image are now clamped, needed due to some float inaccuracy while rasterizing a rect-screenshot
	// Arguments:
	//   iX16 - fX mul 16
	//   iY16 - fY mul 16
	//   result - [0]=red, [1]=green, [2]=blue
	static inline bool GetBilinearFilteredRaw(	const int iX16, const int iY16, 
																							const uint32 *pRGBAImage,
																							const uint32 dwWidth, const uint32 dwHeight,
																							ColorB& result)
	{
		int iLocalX = min(max(iX16/16,0),static_cast<int>(dwWidth-1));
		int iLocalY = min(max(iY16/16,0),static_cast<int>(dwHeight-1));

		int iLerpX = iX16&0xf;			// 0..15
		int iLerpY = iY16&0xf;			// 0..15

		ColorB colS[4];

		const uint32 *pRGBA = &pRGBAImage[iLocalX+iLocalY*dwWidth];

		colS[0] = pRGBA[0];
		colS[1] = pRGBA[1];
		colS[2] = pRGBA[iLocalY+1uL<dwHeight?dwWidth:0];
		colS[3] = pRGBA[(iLocalX+1uL<dwWidth?1:0)+(iLocalY+1uL<dwHeight?dwWidth:0)];

		ColorB colTop,colBottom;
		
		colTop = lerp(colS[0],colS[1],iLerpX,16);
		colBottom = lerp(colS[2],colS[3],iLerpX,16);

		result = lerp(colTop,colBottom,iLerpY,16);
		return true;
	}


	// blend with background
	static inline bool GetBilinearFiltered(	const int iX16, const int iY16, 
																					const uint32 *pRGBAImage,
																					const uint32 dwWidth, const uint32 dwHeight,
																					uint8 result[3] )
	{
		ColorB colFiltered;
		if(GetBilinearFilteredRaw(iX16,iY16,pRGBAImage,dwWidth,dwHeight,colFiltered))
		{
			result[0]=colFiltered.r;
			result[1]=colFiltered.g;
			result[2]=colFiltered.b;
			return true;
		}
		return false;
	}

	static inline bool GetBilinearFilteredBlend(const int iX16, const int iY16, 
																							const uint32 *pRGBAImage,
																							const uint32 dwWidth, const uint32 dwHeight,
																							const uint32 dwLerp64k, 
																							uint8 result[3] )
	{
		ColorB colFiltered;
		if(GetBilinearFilteredRaw(iX16,iY16,pRGBAImage,dwWidth,dwHeight,colFiltered))
		{
			ColorB colRet = lerp(ColorB(result[0],result[1],result[2]),colFiltered,dwLerp64k,256*256);

			result[0]=colRet.r;
			result[1]=colRet.g;
			result[2]=colRet.b;
			return true;
		}
		return false;
	}

	static inline bool GetBilinearFilteredAdd(const int iX16, const int iY16, 
																						const uint32 *pRGBAImage,
																						const uint32 dwWidth, const uint32 dwHeight,
																						const uint32 dwLerp64k, 
																						uint8 result[3] )
	{
		ColorB colFiltered;
		if(GetBilinearFilteredRaw(iX16,iY16,pRGBAImage,dwWidth,dwHeight,colFiltered))
		{
			ColorB colRet = MadSaturate(colFiltered,dwLerp64k,256*256,ColorB(result[0],result[1],result[2]));

			result[0]=colRet.r;
			result[1]=colRet.g;
			result[2]=colRet.b;
			return true;
		}
		return false;
	}


	float GetSliceAngle( const uint32 dwSlice ) const
	{
		uint32 dwAlternatingSlice = (dwSlice*2)%m_dwSliceCount;

		float fAngleStep = m_fHorizFOV;

		float fRet = fAngleStep*dwAlternatingSlice;

		if(dwSlice*2>=m_dwSliceCount)
			fRet += fAngleStep;

		return fRet;
	}

	float GetHorizFOV() const
	{
		return m_fHorizFOV;
	}

	float GetHorizFOVWithBorder() const
	{
		return m_fHorizFOV*(1.0f+m_fTransitionSize);
	}

	void* GetBuffer(){ return &m_RGB[0]; }
	uint32 GetWidth() { return m_dwWidth; }
	uint32 GetHeight() { return m_dwHeight; }


	//private: // -------------------------------------------------------------------

	uint32									m_dwWidth;									// >0
	uint32									m_dwHeight;									// >0
	f32											m_fInvWidth;								// >0
	f32											m_fInvHeight;								// >0
	uint32									m_dwVirtualWidth;						// >0
	uint32									m_dwVirtualHeight;					// >0
	f32											m_fInvVirtualWidth;					// >0
	f32											m_fInvVirtualHeight;				// >0
	std::vector<uint8>			m_RGB;											// [channel + x*3 + m_dwWidth*3*y], channel=0..2, x<m_dwWidth, y<m_dwHeight, no alpha channel to occupy less memory
	uint32									m_nFileId;									// counts up until it finds free file id
	bool										m_bFlipY;										// might be useful for some image formats
	bool										m_bMetaData;								// output additional metadata

	float										m_fPanoramaShotVertFOV;			// -1 means not set yet - in radians

private:

	uint32									m_dwSliceCount;							//
	C3DEngine &							m_rEngine;									//
	float										m_fHorizFOV;								// - in radians
	float										m_fTransitionSize;					// [0..1], 0=no transition, 1.0=full transition
};

enum EScreenShotType
{
	ESST_NONE			=	0,
	ESST_HIGHRES	=	1,
	ESST_PANORAMA,
	ESST_MAP_DELAYED,
	ESST_MAP,
};

void C3DEngine::ScreenshotDispatcher(const int nRenderFlags, const CCamera &_cam, const int dwDrawFlags, const int nFilterFlags)
{
	CStitchedImage*	pStitchedImage=0;
	const uint32	dwPanWidth			= max(1,GetCVars()->e_ScreenShotWidth);
	const uint32	dwPanHeight			= max(1,GetCVars()->e_ScreenShotHeight);
	const f32			fTransitionSize = min(1.f,abs(GetCVars()->e_ScreenShotQuality) * 0.01f);
	const uint32	MinSlices				=	max(max(1,GetCVars()->e_ScreenShotMinSlices),
																			max(	static_cast<int>((dwPanWidth+GetRenderer()->GetWidth()-1)/GetRenderer()->GetWidth()),
																						static_cast<int>((dwPanHeight+GetRenderer()->GetHeight()-1)/GetRenderer()->GetHeight())));
	const uint32	dwVirtualWidth	=	GetRenderer()->GetWidth()*MinSlices;
	const uint32	dwVirtualHeight	=	GetRenderer()->GetHeight()*MinSlices;

	switch(abs(GetCVars()->e_ScreenShot))
	{
		case ESST_HIGHRES:
  			GetConsole()->ShowConsole(false);
				pStitchedImage	=	new CStitchedImage(*this,dwPanWidth,dwPanHeight,dwVirtualWidth,dwVirtualHeight,MinSlices,fTransitionSize);
				ScreenShotHighRes(pStitchedImage,nRenderFlags, _cam, dwDrawFlags, nFilterFlags,MinSlices,fTransitionSize);
				pStitchedImage->SaveImage("HiRes");
				pStitchedImage->Clear();		// good for debugging
				delete pStitchedImage;
				if(GetCVars()->e_ScreenShot>0)			// <0 is used for multiple frames (videos)
					GetCVars()->e_ScreenShot=0;	
			break;
		case ESST_PANORAMA:
				GetConsole()->ShowConsole(false);
				pStitchedImage	=	new CStitchedImage(*this,dwPanWidth,dwPanHeight,dwVirtualWidth,dwVirtualHeight,MinSlices,fTransitionSize);
				ScreenShotPanorama(pStitchedImage,nRenderFlags,_cam,dwDrawFlags,nFilterFlags,MinSlices,fTransitionSize);
				pStitchedImage->SaveImage("Panorama");
				pStitchedImage->Clear();		// good for debugging
				delete pStitchedImage;
				if(GetCVars()->e_ScreenShot>0)			// <0 is used for multiple frames (videos)
					GetCVars()->e_ScreenShot=0;	
			break;
		case ESST_MAP_DELAYED:
			{
				GetCVars()->e_ScreenShot	=	sgn(GetCVars()->e_ScreenShot)*ESST_MAP;		// sgn() to keep sign bit , <0 is used for multiple frames (videos)
        GetTerrain()->ResetTerrainVertBuffers(NULL, GetDefSID());
			}
			break;
		case ESST_MAP:
			{
				static const unsigned int nMipMapSnapshotSize = 512;
				GetRenderer()->ChangeViewport(0,0,nMipMapSnapshotSize,nMipMapSnapshotSize);
				uint32 TmpHeight,TmpWidth,TmpVirtualHeight,TmpVirtualWidth;
				TmpHeight=TmpWidth=TmpVirtualHeight=TmpVirtualWidth=1;

				while((TmpHeight<<1)<=dwPanHeight)
				{
					TmpHeight<<=1;
				}
				while((TmpWidth<<1)<=dwPanWidth)
				{
					TmpWidth<<=1;
				}
				const uint32	TmpMinSlices				=	max(max(1,GetCVars()->e_ScreenShotMinSlices),
																								max(	static_cast<int>((TmpWidth+nMipMapSnapshotSize-1)/nMipMapSnapshotSize),
																											static_cast<int>((TmpHeight+nMipMapSnapshotSize-1)/nMipMapSnapshotSize)));
				while((TmpVirtualHeight<<1)<=TmpMinSlices*nMipMapSnapshotSize)
				{
					TmpVirtualHeight<<=1;
				}
				while((TmpVirtualWidth<<1)<=TmpMinSlices*nMipMapSnapshotSize)
				{
					TmpVirtualWidth<<=1;
				}

				GetConsole()->ShowConsole(false);
				pStitchedImage	=	new CStitchedImage(*this,TmpWidth,TmpHeight,TmpVirtualWidth,TmpVirtualHeight,TmpMinSlices,fTransitionSize,true);
				ScreenShotMap(pStitchedImage,nRenderFlags, _cam, dwDrawFlags, nFilterFlags,TmpMinSlices,fTransitionSize);
				pStitchedImage->SaveImage("Map");

				if(m_pScreenshotCallback)
				{
					const f32	fSizeX	=	GetCVars()->e_ScreenShotMapSizeX;
					const f32	fSizeY	=	GetCVars()->e_ScreenShotMapSizeY;
					const f32	fTLX	=	GetCVars()->e_ScreenShotMapCenterX-fSizeX;
					const f32	fTLY	=	GetCVars()->e_ScreenShotMapCenterY-fSizeY;
					const f32	fBRX	=	GetCVars()->e_ScreenShotMapCenterX+fSizeX;
					const f32	fBRY	=	GetCVars()->e_ScreenShotMapCenterY+fSizeY;

					m_pScreenshotCallback->SendParameters(pStitchedImage->GetBuffer(), pStitchedImage->GetWidth(), pStitchedImage->GetHeight(), fTLX, fTLY, fBRX, fBRY);
				}

				pStitchedImage->Clear();		// good for debugging
				delete pStitchedImage;
			}
			if(GetCVars()->e_ScreenShot>0)			// <0 is used for multiple frames (videos)
				GetCVars()->e_ScreenShot=0;	

      GetTerrain()->ResetTerrainVertBuffers(NULL,GetDefSID());
			break;
		default:
			GetCVars()->e_ScreenShot=0;
	}
}



struct SDebugFrustrum
{
	Vec3						m_vPos[8];
	const char *		m_szName;
	CTimeValue			m_TimeStamp;
	ColorB					m_Color;
	float						m_fQuadDist;		// < 0 if not used
};

static std::vector<SDebugFrustrum> g_DebugFrustrums;

void C3DEngine::DebugDraw_Draw()
{
	CTimeValue CurrentTime = gEnv->pTimer->GetFrameStartTime();

	IRenderAuxGeom *pAux = GetRenderer()->GetIRenderAuxGeom();

	SAuxGeomRenderFlags	oldFlags = pAux->GetRenderFlags();
	SAuxGeomRenderFlags	newFlags;
	newFlags.SetAlphaBlendMode(e_AlphaBlended);
	newFlags.SetCullMode(e_CullModeNone);
	newFlags.SetDepthWriteFlag(e_DepthWriteOff);
	pAux->SetRenderFlags(newFlags);
	std::vector<SDebugFrustrum>::iterator it;
	
	for(it=g_DebugFrustrums.begin();it!=g_DebugFrustrums.end();)
	{
		SDebugFrustrum &ref = *it;

		float fRatio = (CurrentTime-ref.m_TimeStamp).GetSeconds()*2.0f;

		if(fRatio>1.0f)
		{
			it = g_DebugFrustrums.erase(it);
			continue;
		}

		uint16 pnInd[8] = {	0,4,1,5, 2,6,3,7	};

		float fRadius = ((ref.m_vPos[0]+ref.m_vPos[1]+ref.m_vPos[2]+ref.m_vPos[3])-(ref.m_vPos[4]+ref.m_vPos[5]+ref.m_vPos[6]+ref.m_vPos[7])).GetLength()*0.25f;
		float fDistance = min(fRadius,33.0f);		// in meters

		float fRenderRatio = fRatio * fDistance / fRadius;

		if(ref.m_fQuadDist>0)
			fRenderRatio = ref.m_fQuadDist/fRadius;

		Vec3 vPos[4];

		for(uint32 i=0;i<4;++i)
			vPos[i]=ref.m_vPos[i]*fRenderRatio + ref.m_vPos[i+4]*(1.0f-fRenderRatio);

		Vec3 vMid = (vPos[0]+vPos[1]+vPos[2]+vPos[3])*0.25f;

		ColorB col = ref.m_Color;

		if(ref.m_fQuadDist<=0)
		{
			for(uint32 i=0;i<4;++i)
				vPos[i]=vPos[i]*0.95f + vMid*0.05f;

			// quad
      if(ref.m_fQuadDist != -999.f)
      {
			  pAux->DrawTriangle(vPos[0],col,vPos[2],col,vPos[1],col);
			  pAux->DrawTriangle(vPos[2],col,vPos[0],col,vPos[3],col);
      }
			// projection lines
			pAux->DrawLines(ref.m_vPos, 8, pnInd, 2, RGBA8(0xff,0xff,0x1f,0xff));
			pAux->DrawLines(ref.m_vPos, 8, pnInd+2, 2, RGBA8(0xff,0xff,0x1f,0xff));
			pAux->DrawLines(ref.m_vPos, 8, pnInd+4, 2, RGBA8(0xff,0xff,0x1f,0xff));
			pAux->DrawLines(ref.m_vPos, 8, pnInd+6, 2, RGBA8(0xff,0xff,0x1f,0xff));
		}
		else
		{
			// rectangle
			pAux->DrawPolyline(vPos, 4, true, RGBA8(0xff,0xff,0x1f,0xff));
		}

		++it;
	}

	pAux->SetRenderFlags(oldFlags);


	if(GetCVars()->e_DebugDraw==16)
	{
		DebugDraw_UpdateDebugNode();
	}
	else
	{
		GetRenderer()->SetDebugRenderNode(NULL);
	}
}


void C3DEngine::DebugDraw_PushFrustrum( const char *szName, const CRenderCamera &rCam, const ColorB col, const float fQuadDist )
{
	if(GetCVars()->e_DebugDraw!=8 && GetCVars()->e_DebugDraw!=9)
		return;

	Vec3 pvFrust[8];

	rCam.CalcVerts(pvFrust);

	SDebugFrustrum one;

	one.m_szName=szName;
	one.m_Color=col;
	one.m_TimeStamp=gEnv->pTimer->GetFrameStartTime();
	one.m_fQuadDist=fQuadDist;

	one.m_vPos[0]=pvFrust[4];
	one.m_vPos[1]=pvFrust[5];
	one.m_vPos[2]=pvFrust[6];
	one.m_vPos[3]=pvFrust[7];

	one.m_vPos[4]=pvFrust[0];
	one.m_vPos[5]=pvFrust[1];
	one.m_vPos[6]=pvFrust[2];
	one.m_vPos[7]=pvFrust[3];

	g_DebugFrustrums.push_back(one);
}


void C3DEngine::DebugDraw_PushFrustrum( const char *szName, const CCamera &rCam, const ColorB col, const float fQuadDist )
{
	if(GetCVars()->e_DebugDraw!=8 && GetCVars()->e_DebugDraw!=9 && !GetCVars()->e_CameraFreeze)
		return;
	
	SDebugFrustrum one;

	one.m_szName=szName;
	one.m_Color=col;
	one.m_fQuadDist=fQuadDist;
	one.m_TimeStamp=gEnv->pTimer->GetFrameStartTime();
	rCam.GetFrustumVertices(one.m_vPos);

	g_DebugFrustrums.push_back(one);
}

void C3DEngine::DebugDraw_UpdateDebugNode()
{
  ray_hit rayHit;
  
  // use cam, no need for firing pos/dir
  CCamera& cam = GetISystem()->GetViewCamera();
  const unsigned int flags = rwi_stop_at_pierceable|rwi_colltype_any;
	const float hitRange = 2000.f;

  if( gEnv->pPhysicalWorld->RayWorldIntersection(cam.GetPosition()+cam.GetViewdir(), cam.GetViewdir()*hitRange, ent_all, flags, &rayHit, 1) )
	{
		int type = rayHit.pCollider->GetiForeignData();

		if( type == PHYS_FOREIGN_ID_STATIC ) //|| type == PHYS_FOREIGN_ID_ENTITY )
    {
			IRenderNode *pRenderNode = (IRenderNode*)rayHit.pCollider->GetForeignData(PHYS_FOREIGN_ID_STATIC);
    
			if (pRenderNode)
			{
				gEnv->pRenderer->SetDebugRenderNode(pRenderNode);

				//gEnv->pLog->LogToConsole(	"Hit: %s, ipart: %d, partid: %d, surafce_idx: %d, iNode: %d, \n", 
				//													/*pRenderNode->GetName()*/"", rayHit.ipart, rayHit.partid, rayHit.surface_idx, rayHit.iNode);
			}
		}
	}
}

void C3DEngine::RenderWorld(const int nRenderFlags, 
                            const CCamera * pCameras, int nCamerasNum, 
                            const char *szDebugName, const int dwDrawFlags, const int nFilterFlags)
{
	assert(szDebugName);

	if (!GetCVars()->e_Render)
		return;

	FUNCTION_PROFILER_3DENGINE;

	if(m_nRenderStackLevel<0)
	{
		if(GetCVars()->e_ScreenShot)
			ScreenshotDispatcher(nRenderFlags,pCameras[0],dwDrawFlags,nFilterFlags);

		if(GetCVars()->e_DefaultMaterial)
		{
			_smart_ptr<IMaterial> pMat = GetMaterialManager()->LoadMaterial("Materials/material_default");
      _smart_ptr<IMaterial> pTerrainMat = GetMaterialManager()->LoadMaterial("Materials/material_terrain_default");
			GetRenderer()->SetDefaultMaterials(pMat,pTerrainMat);
		}
		else
			GetRenderer()->SetDefaultMaterials(NULL,NULL);
	}

  if(GetCVars()->e_SplitScreenTest && m_nRenderStackLevel<0)
  {
    int w = GetRenderer()->GetWidth();
    int h = GetRenderer()->GetHeight();

    for(int x=0; x<2; x++)
    {
      for(int y=0; y<2; y++)
      {
        GetRenderer()->SetViewport(x*w/2, y*h/2, w/2, h/2);

        CCamera newCam = pCameras[0];
        Matrix34 mat = pCameras[0].GetMatrix();
        Matrix33 matRot; matRot.SetRotationZ((x*2+y)*gf_PI/2);
        newCam.SetMatrix(mat*matRot);

        RenderInternal(nRenderFlags,newCam,szDebugName,dwDrawFlags,nFilterFlags); 
      }
    }
  }
  else if(nCamerasNum>1)
  {
    for(int i=0; i<nCamerasNum; i++)
    {
      const CCamera & rCam = pCameras[i];
      int nPosX, nPosY, nSizeX, nSizeY;
      rCam.GetViewPort(nPosX, nPosY, nSizeX, nSizeY);
      if(!nSizeX || !nSizeY)
      { Error("I3DEngine::RenderWorld: Camera viewport is not specified"); assert(!"Invalid camera viewport"); }
      GetRenderer()->SetViewport(nPosX, nPosY, nSizeX, nSizeY);
      RenderInternal(nRenderFlags,rCam,szDebugName,dwDrawFlags,nFilterFlags); 
    }
  }
  else
  {
    RenderInternal(nRenderFlags,pCameras[0],szDebugName,dwDrawFlags,nFilterFlags); 
  }

	if(GetCVars()->e_DebugDraw)
	{
		f32 fColor[4] = {1,1,0,1};
		
		float fYLine=8.0f, fYStep=20.0f;

		GetRenderer()->Draw2dLabel(8.0f,fYLine+=fYStep,2.0f,fColor,false,"e_DebugDraw = %d",GetCVars()->e_DebugDraw);

		char *szMode = "";

		switch(GetCVars()->e_DebugDraw)
		{
			case  1:	szMode="name of the used cgf, polycount, used LOD";break;
			case  2:	szMode="color coded polygon count";break;
			case  3:	szMode="show color coded LODs count, flashing color indicates LOD";break;
			case  4:	szMode="object texture memory usage in KB";break;
			case  5:	szMode="number of render materials (color coded)";break;
			case  6:	szMode="ambient color (R,G,B,A)";break;
			case  7:	szMode="triangle count, number of render materials, texture memory in KB";break;
			case  8:	szMode="RenderWorld statistics (with view cones)";break;
			case  9:	szMode="RenderWorld statistics (with view cones without lights)";break;
			case 10:	szMode="geometry with simple lines and triangles";break;
			case 11:	szMode="occlusion geometry additionally";break;
			case 12:	szMode="occlusion geometry without render geometry";break;
			case 13:	szMode="occlusion amount (used during AO computations)";break;
//			case 14:	szMode="";break;
			case 15:	szMode="display helpers";break;			
			case 16:	szMode="Debug Gun";break;
			case 17:	szMode="streaming: buffer sizes (black: geometry, blue: texture)";
        if (gEnv->pLocalMemoryUsage) gEnv->pLocalMemoryUsage->OnRender(GetRenderer(), &pCameras[0]);
								break;
			case 18:	szMode="streaming: required streaming speed (black: geometry, blue: texture)";
				if (gEnv->pLocalMemoryUsage) gEnv->pLocalMemoryUsage->OnRender(GetRenderer(), &pCameras[0]);
								break;

			default: assert(0);
		}

		GetRenderer()->Draw2dLabel(8.0f,fYLine+=fYStep,2.0f,fColor,false,"   %s",szMode);

		if( GetCVars()->e_DebugDraw == 17 )
		{
			GetRenderer()->Draw2dLabel(8.0f,fYLine+=fYStep,2.0f,fColor,false,"   StatObj geometry used: %.2fMb / %dMb",CObjManager::s_nLastStreamingMemoryUsage/(1024.f*1024.f), GetCVars()->e_StreamCgfPoolSize);
		} 
		if( (GetCVars()->e_DebugDraw == 17 || GetCVars()->e_DebugDraw == 18) )
		{
			ICVar* cVar = GetConsole()->GetCVar("r_TexturesStreaming");
			if (!cVar || !cVar->GetIVal())
			{
				GetRenderer()->Draw2dLabel(8.0f,fYLine+=fYStep,2.0f,fColor,false,"   You have to set r_TexturesStreaming = 1 to see texture information!");
			}
		} 
}
}

void C3DEngine::RenderInternal(const int nRenderFlags, const CCamera &_cam, const char *szDebugName, const int dwDrawFlags, const int nFilterFlags)
{
	m_bProfilerEnabled = gEnv->pFrameProfileSystem->IsProfiling();

  // test if recursion causes problems
	if(m_nRenderStackLevel>=0 && !GetCVars()->e_Recursion)
		return;

	// clamp to prevent cheating (implementation could be improved)
//	GetCVars()->e_LodMin = clamp_tpl(GetCVars()->e_LodMin,0,2);
//	GetCVars()->e_LodRatio = clamp_tpl(GetCVars()->e_LodRatio,3.0f,6.0f);

	// Update particle system as late as possible, only renderer is dependent on it.
	if(m_nRenderStackLevel == -1 && m_pPartManager)
		m_pPartManager->Update();

	if (m_nRenderStackLevel == -1 && GetCVars()->e_Clouds)
	{
		if (m_pCloudsManager)
			m_pCloudsManager->MoveClouds();

		CVolumeObjectRenderNode::MoveVolumeObjects();
	}

	m_nRenderStackLevel++;

	if(m_nRenderStackLevel!=0)		// record all except main rendering
		DebugDraw_PushFrustrum(szDebugName,_cam,ColorB(0xff,0xff,0x1f,0x50));

  if(GetCVars()->e_SplitScreenTest)
  {
    m_nRenderFrameID++;
    if(m_nRenderStackLevel==0)
      m_nRenderMainFrameID++;
  }
  else
  {
    m_nRenderFrameID = GetRenderer()->GetFrameID();
    m_nRenderMainFrameID = GetRenderer()->GetFrameID(false);
    //m_nRenderFrameID++;
    //if(m_nRenderStackLevel==0)
    //  m_nRenderMainFrameID++;
  }

  m_nRenderThreadListID = (int)GetRenderer()->EF_Query(EFQ_MainThreadList);

	m_fZoomFactor = 0.5f + 0.5f*(RAD2DEG(_cam.GetFov())/60.f);

	if(m_pObjManager)
	{
		m_pObjManager->m_fMaxViewDistanceScale = 1.f;
		m_pObjManager->m_fGSMMaxDistance = GetCVars()->e_GsmRange * powf((float)GetCVars()->e_GsmRangeStep, 
			(float)(GetCVars()->e_GsmLodsNum - (GetCVars()->e_ShadowsFromTerrain ?  1 : 0)));
	}
	
	assert((UINT_PTR)m_nRenderStackLevel == (UINT_PTR)GetRenderer()->EF_Query(EFQ_RecurseLevel));

	if(m_nRenderStackLevel<0 || m_nRenderStackLevel>=MAX_RECURSION_LEVELS)
  { 
    assert(!"Recursion depther than MAX_RECURSION_LEVELS is not supported"); 	
    m_nRenderStackLevel--;
    return; 
  }

	// set cvars from flags
	DLD_SET_CVAR(e_Shadows,DLD_SHADOW_MAPS);
	DLD_SET_CVAR(e_Brushes,DLD_STATIC_OBJECTS);
	DLD_SET_CVAR(e_Vegetation,DLD_STATIC_OBJECTS);
	DLD_SET_CVAR(e_Entities,DLD_ENTITIES);
	DLD_SET_CVAR(e_Terrain,DLD_TERRAIN);
	DLD_SET_CVAR(e_WaterOcean,DLD_TERRAIN_WATER);
	DLD_SET_CVAR(e_Particles,DLD_PARTICLES);	
	DLD_SET_CVAR(e_Decals,DLD_DECALS);	
  DLD_SET_CVAR(e_TerrainDetailMaterials,DLD_DETAIL_TEXTURES);	
  float fOldZoomFactor = m_fZoomFactor;

	if(m_nRenderStackLevel)
	{
		GetCVars()->e_Shadows = 0;
    m_fZoomFactor /= GetCVars()->e_RecursionViewDistRatio;
	}

  if(GetCVars()->e_DefaultMaterial || (GetCVars()->e_Voxel && GetCVars()->e_VoxTer && !m_bEditor))
  {
    //GetCVars()->e_Decals = 0;
    GetCVars()->e_TerrainDetailMaterials = 0;
  }

	CCamera oldCamera = GetCamera();

	if(!SetupCamera(_cam, szDebugName))
  { 
    m_nRenderStackLevel--;
    return; 
  }

  // force preload terrain data if camera was teleported more than 32 meters
  if(m_nRenderStackLevel==0)
  {
    if(GetCVars()->e_LevelAutoPrecacheCameraJumpDist && m_vPrevMainFrameCamPos.GetDistance(GetCamera().GetPosition()) > GetCVars()->e_LevelAutoPrecacheCameraJumpDist)
    {
      PrintMessage("Preparing textures, procedural objects and shaders for position (%.1f,%.1f,%.1f)",
        GetCamera().GetPosition().x, GetCamera().GetPosition().y, GetCamera().GetPosition().z);
      m_bContentPrecacheRequested = true;
    }

    m_vPrevMainFrameCamPos = GetCamera().GetPosition();
  }

	if(!m_nRenderStackLevel)
		UpdateScene();

  CheckPhysicalized(GetCamera().GetPosition()-Vec3(2,2,2),GetCamera().GetPosition()+Vec3(2,2,2));

	RenderScene(nRenderFlags, GetSystem()->IsDedicated() ? 0 : dwDrawFlags, nFilterFlags);

	// c-buffer debug info
  if(m_pObjManager && m_pCoverageBuffer && GetCVars()->e_CoverageBufferDebug)
  {
    m_pCoverageBuffer->DrawDebug(GetCVars()->e_CoverageBufferDebug);
  }

	DLD_RESTORE_CVAR(e_Shadows);
	DLD_RESTORE_CVAR(e_Brushes);
	DLD_RESTORE_CVAR(e_Vegetation);
	DLD_RESTORE_CVAR(e_Entities);
	DLD_RESTORE_CVAR(e_Terrain);
	DLD_RESTORE_CVAR(e_WaterOcean);
	DLD_RESTORE_CVAR(e_Particles);	
	DLD_RESTORE_CVAR(e_Decals);	
  DLD_RESTORE_CVAR(e_TerrainDetailMaterials);	
  m_fZoomFactor = fOldZoomFactor;
  
	// Unload old stuff
//  if(GetCVars()->e_stream_areas)
  {
//    if(m_pTerrain)
  //    m_pTerrain->CheckUnload();
  //  if(m_pVisAreaManager)
//      m_pVisAreaManager->CheckUnload();
  }

	if(m_pObjManager && !m_nRenderStackLevel)
  {
		m_pObjManager->CheckTextureReadyFlag();
		if(GetCVars()->e_StreamCgf)
    {
      static Array2d<int> memUsage;

      int nArrayDim = 256;

      if(GetCVars()->e_StreamCgfDebugHeatMap==1)
      {
        memUsage.Allocate(nArrayDim);
        CCamera camOld = GetCamera();

        int nStep = Get3DEngine()->GetTerrainSize() / nArrayDim;

        PrintMessage("Computing mesh streaming heat map");

        for(int x=0; x<Get3DEngine()->GetTerrainSize(); x+=nStep)
        {
          for(int y=0; y<Get3DEngine()->GetTerrainSize(); y+=nStep)
          {
            CCamera camTmp = camOld;
            camTmp.SetPosition(Vec3((float)x+(float)nStep/2.f,(float)y+(float)nStep/2.f,Get3DEngine()->GetTerrainElevation((float)x,(float)y)));
            SetCamera(camTmp);
            m_pObjManager->ProcessObjectsStreaming();

						SObjectsStreamingStatus objectsStreamingStatus;
            m_pObjManager->GetObjectsStreamingStatus(objectsStreamingStatus);

            memUsage[x/nStep][y/nStep] = objectsStreamingStatus.nMemRequired;
          }

          if(!((x/nStep)&31))
            PrintMessage(" working ...");
        }

        PrintMessage(" done");

        GetCVars()->e_StreamCgfDebugHeatMap=2;
        SetCamera(camOld);
      }
      else if(GetCVars()->e_StreamCgfDebugHeatMap==2)
      {
        float fStep = (float)Get3DEngine()->GetTerrainSize() / (float)nArrayDim;

        for(int x=0; x<memUsage.GetSize(); x++)
        {
          for(int y=0; y<memUsage.GetSize(); y++)
          {
            Vec3 v0((float)x*fStep,       (float)y*fStep,       Get3DEngine()->GetTerrainElevation((float)x*fStep, (float)y*fStep));
            Vec3 v1((float)x*fStep+fStep, (float)y*fStep+fStep, v0.z+fStep);
            v0 += Vec3(.25f,.25f,.25f);
            v1 -= Vec3(.25f,.25f,.25f);
            AABB box(v0,v1);
            if(!GetCamera().IsAABBVisible_F(box))
              continue;

            int nMemUsageMB = memUsage[(int)(x)][(int)(y)]/1024/1024;

            int nOverLoad = nMemUsageMB - GetCVars()->e_StreamCgfPoolSize;
            
            ColorB col = Col_Red;

            if(nOverLoad<GetCVars()->e_StreamCgfPoolSize/2)
              col = Col_Yellow;

            if(nOverLoad<0)
              col = Col_Green;

            DrawBBox(box,col);
          }
        }
      }

      m_pObjManager->ProcessObjectsStreaming();
    }
    else
      m_pObjManager->UpdateObjectsStreamingPriority(false);
  }

/*	
	if(0 && GetCVars()->e_lm_gen_debug)
	{ // light-map map generation debug
//		uint8 * pImage = new uint8[GetCVars()->e_lm_gen_debug*GetCVars()->e_lm_gen_debug*3];

		if(CTerrainNode * pSectorInfo = m_pTerrain ? m_pTerrain->GetSecInfo(GetCamera().GetPosition()) : 0)
			if(pSectorInfo = pSectorInfo->GetTexSourceNode(false))
		{
			GetTerrain()->RenderAreaShadowsIntoTexture(pSectorInfo->m_nOriginX, pSectorInfo->m_nOriginY, 
				GetTerrain()->GetSectorSize()<<pSectorInfo->m_nTreeLevel, GetCVars()->e_lm_gen_debug);
			GetCVars()->e_lm_gen_debug = 0;
		}

//		delete [] pImage;
	}
*/

/*
	int nCount=0;
	GetVoxelRenderMeshes(NULL, nCount);
	IRenderMesh ** pNodes = new IRenderMesh * [nCount];
	nCount=0;
	GetVoxelRenderMeshes(pNodes, nCount);
	delete [] pNodes;
*/

	if(m_nRenderStackLevel)
		SetCamera(oldCamera);

  if(!m_nRenderStackLevel)
  {
    m_bContentPrecacheRequested = false;

//    COctreeNode::ReleaseEmptyNodes();
  }

	m_nRenderStackLevel--;
}

int __cdecl C3DEngine__Cmp_SRNInfo(const void* v1, const void* v2)
{
	SRNInfo * p1 = (SRNInfo*)v1;
	SRNInfo * p2 = (SRNInfo*)v2;

	float fViewDist1 = p1->fMaxViewDist - p1->objSphere.radius;
	float fViewDist2 = p2->fMaxViewDist - p2->objSphere.radius;

	// if same - give closest sectors higher priority
	if(fViewDist1 > fViewDist2)
		return 1;
	else if(fViewDist1 < fViewDist2)
		return -1;

	return 0;
}

bool C3DEngine::RenderVisAreaPotentialOccluders(CVisArea * pThisArea, CCullBuffer & rCB, const CCamera & viewCam, bool bResetAffectedLights)
{
	bool bOccludersFound = false;

	// render objects in current area/portal
	if(pThisArea->m_pObjectsTree)
		bOccludersFound |= pThisArea->m_pObjectsTree->Render_Occl_Nodes(viewCam, rCB, OCTREENODE_RENDER_FLAG_OCCLUDERS, -1);

	for(int ii=0; ii<pThisArea->m_lstConnections.Count(); ii++)
	{
		if(CVisArea * pNeibArea = pThisArea->m_lstConnections[ii])
		{
			// render objects in neighbor area/portal
			if(pNeibArea->m_pObjectsTree)
				bOccludersFound |= pNeibArea->m_pObjectsTree->Render_Occl_Nodes(viewCam, rCB, OCTREENODE_RENDER_FLAG_OCCLUDERS, -1);

			// render objects in neighbor of neighbor area/portal
			for(int i=0; i<pNeibArea->m_lstConnections.Count(); i++)
				if(pNeibArea->m_lstConnections[i] != pThisArea && pNeibArea->m_lstConnections[i]->m_pObjectsTree)
					bOccludersFound |= pNeibArea->m_lstConnections[i]->m_pObjectsTree->Render_Occl_Nodes(viewCam, rCB, OCTREENODE_RENDER_FLAG_OCCLUDERS, -1);
		}
	}

	return bOccludersFound;
}

bool C3DEngine::RenderPotentialOccluders(CCullBuffer & rCB, const CCamera & viewCam, bool bResetAffectedLights)
{
	FUNCTION_PROFILER_3DENGINE;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	assert(&rCB == GetCoverageBuffer() || 1);

	bool bOccludersFound = false;

	if(GetCVars()->e_CoverageBufferHW)
	{
		PodArray<SRNInfo> lstStaticsAround;

		// indoors
		PodArray<CVisArea*> * pVisibleAreas = &m_pVisAreaManager->m_lstVisibleAreas;
		for(int i=0; i<pVisibleAreas->Count(); i++)
			if(COctreeNode * pTree = pVisibleAreas->GetAt(i)->m_pObjectsTree)
				pTree->MoveObjectsIntoList(&lstStaticsAround, NULL, false, true, false);

		if(m_pTerrain)
			m_pTerrain->GetObjectsAround(viewCam.GetPosition(), 32.f, &lstStaticsAround, false, false);

		Vec3 vCamPos = viewCam.GetPosition();

		int nTrisWriten = 0;

		PodArray<IRenderNode*> lstOccluders;
		for(int i=0; i<lstStaticsAround.Count(); i++)
		{
			IRenderNode * pObj = lstStaticsAround[i].pNode;
			if(pObj->GetRndFlags()&ERF_GOOD_OCCLUDER)
			{
				float fEntDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos,pObj->GetBBox()))*m_fZoomFactor;
				assert(fEntDistance>=0 && _finite(fEntDistance));
				if(fEntDistance < pObj->m_fWSMaxViewDist*GetCVars()->e_CoverageBufferOccludersViewDistRatio)
					if(viewCam.IsAABBVisible_EM( pObj->GetBBox() ))
					{
						lstOccluders.Add(pObj);
						IRenderMesh * pMesh = pObj->GetRenderMesh(0);
						if(pMesh)
							nTrisWriten += pMesh->GetIndicesCount()/3;
					}
			}
		}

		static ICVar * p_r_Wireframe = GetConsole()->GetCVar("r_wireframe");
		int nOldWireframe = p_r_Wireframe->GetIVal();

		p_r_Wireframe->Set(R_SOLID_MODE);

#ifdef OCCLUSIONCULLER_V
		GetRenderer()->RenderOccludersIntoBuffer(viewCam, 
			rCB.m_farrBuffer.GetSize(), lstOccluders, rCB.m_farrBuffer.GetData());
#endif
		p_r_Wireframe->Set(nOldWireframe);

		bOccludersFound = lstOccluders.Count()>0;

		rCB.TrisWritten(nTrisWriten);
		rCB.ObjectsWritten(lstOccluders.Count());
	}
	else
	{
		int old_e_shadows = GetCVars()->e_Shadows;
		GetCVars()->e_Shadows = 0;

		if(GetCVars()->e_HwOcclusionCullingObjects)
		{
			ColorF GreenYellow(Col_GreenYellow);
			GetRenderer()->ClearBuffer(FRT_CLEAR, &GreenYellow);
			GetRenderer()->EnableFog(0);		
			GetRenderer()->EF_StartEf();  
		}

		if(m_pVisAreaManager)
		{
			bool bCameraInIndoor = m_pVisAreaManager->m_pCurArea || m_pVisAreaManager->m_pCurPortal;

			if(bCameraInIndoor)
			{ // indoors
				PodArray<CVisArea*> * pVisibleAreas = &m_pVisAreaManager->m_lstVisibleAreas;
				for(int i=0; i<pVisibleAreas->Count(); i++)
					if(CVisArea * pArea = pVisibleAreas->GetAt(i))
						if(pArea->m_pObjectsTree)
							for(int c=0; c<pArea->m_lstCurCameras.Count(); c++)
								bOccludersFound |= pArea->m_pObjectsTree->Render_Occl_Nodes(pArea->m_lstCurCameras[c], rCB, OCTREENODE_RENDER_FLAG_OCCLUDERS, -1);
			}

			// outdoor
      for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
      {
			  if (m_pObjectsTree[nSID] && IsOutdoorVisible())  	
			  {	
				  CCamera outdoorViewCam = GetCamera();
				  if (m_pVisAreaManager->m_lstOutdoorPortalCameras.Count()	&& 
					  (m_pVisAreaManager->m_pCurArea || m_pVisAreaManager->m_pCurPortal))
					  outdoorViewCam = m_pVisAreaManager->m_lstOutdoorPortalCameras[0];

				  bOccludersFound |= m_pObjectsTree[nSID]->Render_Occl_Nodes(outdoorViewCam, rCB, OCTREENODE_RENDER_FLAG_OCCLUDERS, -1);
			  }
      }

			if(!bCameraInIndoor)
			{ // indoors
				PodArray<CVisArea*> * pVisibleAreas = &m_pVisAreaManager->m_lstVisibleAreas;
				for(int i=0; i<pVisibleAreas->Count(); i++)
					if(CVisArea * pArea = pVisibleAreas->GetAt(i))
						if(pArea->m_pObjectsTree)
							for(int c=0; c<pArea->m_lstCurCameras.Count(); c++)
								bOccludersFound |= pArea->m_pObjectsTree->Render_Occl_Nodes(pArea->m_lstCurCameras[c], rCB, OCTREENODE_RENDER_FLAG_OCCLUDERS, -1);
			}
		}

		if(GetCVars()->e_HwOcclusionCullingObjects)
			GetRenderer()->EF_EndEf3D(SHDF_ZPASS|SHDF_ZPASS_ONLY, -1);

		GetCVars()->e_Shadows = old_e_shadows;
	}

	return bOccludersFound;
}

IMaterial* C3DEngine::GetSkyMaterial()
{
	IMaterial* pRes(0);
	if (GetCVars()->e_SkyType == 0)
	{
		if (!m_pSkyLowSpecMat)
		{
			m_pSkyLowSpecMat = m_skyLowSpecMatName.empty() ? 0 : m_pMatMan->LoadMaterial(m_skyLowSpecMatName.c_str(), false);
		}
		pRes = m_pSkyLowSpecMat;
	}
	else
	{
		if (!m_pSkyMat)
		{
			m_pSkyMat = m_skyMatName.empty() ? 0 : m_pMatMan->LoadMaterial(m_skyMatName.c_str(), false);
		}
		pRes = m_pSkyMat;
	}
	return pRes;
}

bool C3DEngine::IsHDRSkyMaterial(IMaterial* pMat) const
{
	return pMat && !stricmp(pMat->GetShaderItem().m_pShader->GetName(), "SkyHDR");
}

extern void RenderArrSRenderMeshRenderList();
extern void SyncAllTerrainNodes();

void C3DEngine::RenderScene(const int nRenderFlags, unsigned int dwDrawFlags, const int nFilterFlags)
{
	FUNCTION_PROFILER_3DENGINE;

////////////////////////////////////////////////////////////////////////////////////////
// Store draw flags
////////////////////////////////////////////////////////////////////////////////////////

	assert(Cry3DEngineBase::m_nRenderStackLevel>=0 && Cry3DEngineBase::m_nRenderStackLevel<MAX_RECURSION_LEVELS);
	m_pObjManager->m_dwRecursionDrawFlags[Cry3DEngineBase::m_nRenderStackLevel] = dwDrawFlags;

////////////////////////////////////////////////////////////////////////////////////////
// Begin scene drawing
////////////////////////////////////////////////////////////////////////////////////////

	// init coverage buffer, for USE_CULL_QUEUE this happens deferred
#ifndef USE_CULL_QUEUE
	if(!m_nRenderStackLevel)
		if(GetCVars()->e_CoverageBuffer)
			m_pCoverageBuffer->BeginFrame(GetCamera());
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Sync asynchronous cull queue processing if enabled
////////////////////////////////////////////////////////////////////////////////////////

  GetObjManager()->CullQueue().Wait();

  if(!m_nRenderStackLevel)
    COctreeNode::ReleaseEmptyNodes();

////////////////////////////////////////////////////////////////////////////////////////
// Define indoor visibility
////////////////////////////////////////////////////////////////////////////////////////

	if(m_pVisAreaManager)
		m_pVisAreaManager->CheckVis();

////////////////////////////////////////////////////////////////////////////////////////
// Draw potential occluders into z-buffer
////////////////////////////////////////////////////////////////////////////////////////

	int nOld_e_hw_occlusion_culling_objects = GetCVars()->e_HwOcclusionCullingObjects;

	GetCoverageBuffer()->SetFrameTime(GetCurAsyncTimeSec());

	if(!m_nRenderStackLevel)
	{
		if(GetCVars()->e_HwOcclusionCullingObjects	|| GetCVars()->e_CoverageBuffer)
		{
#if !defined(XENON) && !defined(PS3)
			if(!RenderPotentialOccluders(*GetCoverageBuffer(), GetCamera(), false))
      {
       // GetCVars()->e_HwOcclusionCullingObjects = 0; // disable culling for this frame if no occluders found
      }
#endif

			GetCoverageBuffer()->UpdateDepthTree();
		}

		float fRenderOccludersTime = GetCurAsyncTimeSec() - GetCoverageBuffer()->GetFrameTime();
		float fTimeRatio = fRenderOccludersTime/(0.001f*max(1,GetCVars()->e_CoverageBufferMaxAddRenderMeshTime));
		GetObjManager()->m_fOcclTimeRatio = GetObjManager()->m_fOcclTimeRatio*0.8f + fTimeRatio*0.2f;
		GetObjManager()->m_fOcclTimeRatio = CLAMP(GetObjManager()->m_fOcclTimeRatio, 1.f, 8.f);
	}
	
	if (GetCVars()->e_CoverageBufferDrawOccluders) // When drawing occluders disable time managment for occluders.
		GetObjManager()->m_fOcclTimeRatio = 1.0f;

	if(GetCVars()->e_Sleep>0)
	{
		CrySleep(GetCVars()->e_Sleep);
	}

	// todo: use only visible sectors
	if(m_pVisAreaManager)
		m_pVisAreaManager->DrawOcclusionAreasIntoCBuffer(m_pCoverageBuffer);

////////////////////////////////////////////////////////////////////////////////////////
// From here we add render elements of main scene
////////////////////////////////////////////////////////////////////////////////////////

	GetRenderer()->EF_StartEf();  

////////////////////////////////////////////////////////////////////////////////////////
// Add lsources to the renderer and register into sectors
////////////////////////////////////////////////////////////////////////////////////////

	UpdateLightSources();
	PrepareLightSourcesForRendering_0();    
  PrepareLightSourcesForRendering_1();    
	InitShadowFrustums();

////////////////////////////////////////////////////////////////////////////////////////
// Add render elements for indoor
////////////////////////////////////////////////////////////////////////////////////////

	// draw objects inside visible vis areas
	if(m_pVisAreaManager)
		m_pVisAreaManager->DrawVisibleSectors();

////////////////////////////////////////////////////////////////////////////////////////
// Clear current sprites list
////////////////////////////////////////////////////////////////////////////////////////

  for(int t=0; t<nThreadsNum; t++)
  {
    PodArray<SVegetationSpriteInfo> & rList = m_pObjManager->m_arrVegetationSprites[m_nRenderStackLevel][t];
  	rList.Clear();
  }

////////////////////////////////////////////////////////////////////////////////////////
// Add render elements for outdoor
////////////////////////////////////////////////////////////////////////////////////////

	// Current camera maybe redefined by portal system
  CCamera prevCam = GetCamera();

  // Reset ocean volume
  if( !m_nRenderStackLevel )
    m_nOceanRenderFlags[m_nRenderThreadListID] &= ~OCR_OCEANVOLUME_VISIBLE;

  if (IsOutdoorVisible())  	
	{	
		if (m_pVisAreaManager && m_pVisAreaManager->m_lstOutdoorPortalCameras.Count()	&& 
			(m_pVisAreaManager->m_pCurArea || m_pVisAreaManager->m_pCurPortal))
    { // enable multi-camera culling
      m_Camera.m_pMultiCamera = &m_pVisAreaManager->m_lstOutdoorPortalCameras;
    }

		if(IsOutdoorVisible())
		{
			RenderSkyBox(GetSkyMaterial());
		}

    // start processing terrain
		if(IsOutdoorVisible() && GetCVars()->e_Terrain && Get3DEngine()->m_bShowTerrainSurface && !gEnv->pSystem->IsDedicated() && !(GetCVars()->e_VoxTer && GetCVars()->e_Voxel))
			m_pTerrain->CheckVis();

    // process streaming and procedural vegetation distribution
    if(!m_nRenderStackLevel && m_pTerrain && !(GetCVars()->e_VoxTer && GetCVars()->e_Voxel))
      m_pTerrain->UpdateNodesIncrementaly();

    // render terrain ground
    if(m_pTerrain)
      m_pTerrain->DrawVisibleSectors(); 

    // render 3d terrain ground
    if(m_pVoxTerrain && (GetCVars()->e_VoxTer))
      m_pVoxTerrain->Render(); 

    for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
    {
		  if(m_pObjectsTree[nSID])
      {
        if(GetCVars()->e_MtTest==1)
        {
          FRAME_PROFILER( "COctreeNode::RenderNR_MT", GetSystem(), PROFILE_3DENGINE );
          m_pObjectsTree[nSID]->RenderMT(GetCamera(),OCTREENODE_RENDER_FLAG_OBJECTS, GetSkyColor());
        }
        else if(GetCVars()->e_MtTest==2)
        {
          FRAME_PROFILER( "COctreeNode::RenderNR___", GetSystem(), PROFILE_3DENGINE );

          static PodArray<COctreeNode*> arrNodes; arrNodes.Clear();
          arrNodes.Add(m_pObjectsTree[nSID]);

          while(arrNodes.Count())
          {
            COctreeNode * pLast = arrNodes.Last();
            arrNodes.DeleteLast();

            pLast->RenderNR_Object_Nodes(GetCamera(),OCTREENODE_RENDER_FLAG_OBJECTS, GetSkyColor(), &arrNodes, 0);
          }
        }
        else
        {
          FRAME_PROFILER( "COctreeNode::Render_____", GetSystem(), PROFILE_3DENGINE );
          m_pObjectsTree[nSID]->Render_Object_Nodes(false, GetCamera(),OCTREENODE_RENDER_FLAG_OBJECTS, GetSkyColor(), -1);

          /*if(GetCVars()->e_SceneMerging)
          {
            if(!m_pSceneTree)
            {
              m_pSceneTree = new CSceneTree(AABB(Vec3(0,0,0),Vec3((float)GetTerrainSize(), (float)GetTerrainSize(), (float)GetTerrainSize())), NULL);
              m_pObjectsTree[nSID]->UpdateSceneMerging();
            }

            m_pSceneTree->Render(GetCamera(), false);
          }
          else
            SAFE_DELETE(m_pSceneTree);*/
        }
      
        RenderArrSRenderMeshRenderList();
      }
    }


		// add sprites render item
    if(dwDrawFlags & DLD_FAR_SPRITES)
			m_pObjManager->RenderFarObjects();
  }
  else if(m_pVisAreaManager && m_pVisAreaManager->IsSkyVisible())
	{
		RenderSkyBox(GetSkyMaterial());
	}

  // render outdoor entities very near of camera - fix for 1p vehicle entering into indoor
  for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
  {
    if(GetCVars()->e_PortalsBigEntitiesFix)
      if(m_pObjectsTree[nSID] && !IsOutdoorVisible() && GetVisAreaManager() && GetVisAreaManager()->GetCurVisArea()) 
        if(GetVisAreaManager()->GetCurVisArea()->IsConnectedToOutdoor())
        {
          CCamera cam = GetCamera();
          cam.SetFrustum(cam.GetViewSurfaceX(), cam.GetViewSurfaceZ(), cam.GetFov(), cam.GetNearPlane(), 2.f);
          m_pObjectsTree[nSID]->Render_Object_Nodes(false, cam,
            OCTREENODE_RENDER_FLAG_OBJECTS | OCTREENODE_RENDER_FLAG_OBJECTS_ONLY_ENTITIES, 
            GetSkyColor(), -1);
        }
  }

  if(m_pTerrain)
    for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
      m_pTerrain->RenderAOSectors(nSID);

  // render special objects like laser beams intersecting entire level
  for(int i=0; i<m_lstAlwaysVisible.Count(); i++)
  {
    IRenderNode * pObj = m_lstAlwaysVisible[i];
    const AABB & objBox = pObj->GetBBox();
    if(GetCamera().IsAABBVisible_E( objBox ))
    {
      FRAME_PROFILER( "C3DEngine::RenderScene_DrawAlwaysVisible", GetSystem(), PROFILE_3DENGINE );

      Vec3 vCamPos = GetCamera().GetPosition();
      float fEntDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos,objBox))*m_fZoomFactor;
      assert(fEntDistance>=0 && _finite(fEntDistance));
      if(fEntDistance < pObj->m_fWSMaxViewDist)
        GetObjManager()->RenderObject( pObj, NULL, GetSkyColor(), objBox, fEntDistance, GetCamera(), NULL, false );
    }
  }

  ProcessOcean(dwDrawFlags);

	// restore camera
  SetCamera(prevCam);
   
	if(dwDrawFlags & DLD_DECALS && GetCVars()->e_Decals && m_pDecalManager)
	{
		m_pDecalManager->Render();
	}

////////////////////////////////////////////////////////////////////////////////////////
// Start asynchronous cull queue processing if enabled
////////////////////////////////////////////////////////////////////////////////////////

	GetObjManager()->CullQueue().Process(GetMainFrameID(), Get3DEngine()->GetCoverageBuffer(),&GetCamera());

////////////////////////////////////////////////////////////////////////////////////////
// Finalize frame
////////////////////////////////////////////////////////////////////////////////////////
  
	SetupDistanceFog();

	if(!m_nRenderStackLevel)
		SetupClearColor();

	// sync and finish all asynchron TerrainNode Updates
	SyncAllTerrainNodes();

	IRenderer * ppp = GetRenderer();
	{
		FRAME_PROFILER( "Renderer::EF_EndEf3D", GetSystem(), PROFILE_RENDERER );

		// TODO: separate SHDF_NOASYNC and SHDF_STREAM_SYNC flags
    GetRenderer()->EF_EndEf3D(IsShadersSyncLoad() ? (nRenderFlags|SHDF_NOASYNC|SHDF_STREAM_SYNC) : nRenderFlags,  GetObjManager()->m_nUpdateStreamingPrioriryRoundId);
	}

	if(!m_nRenderStackLevel)
		GetRenderer()->EnableFog(false);

  // unload old meshes
  if(!m_nRenderStackLevel && m_pTerrain)
    for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
      m_pTerrain->CheckNodesGeomUnload(nSID);

	GetCVars()->e_HwOcclusionCullingObjects = nOld_e_hw_occlusion_culling_objects;

  if(!m_nRenderStackLevel)
  {
    UpdateRNTmpDataPool(m_bResetRNTmpDataPool);
    m_bResetRNTmpDataPool = false;
  }
}

void C3DEngine::ProcessOcean(const int dwDrawFlags)
{
  if(GetOceanRenderFlags() & OCR_NO_DRAW || !GetVisAreaManager() || GetCVars()->e_DefaultMaterial)
    return;

  bool bOceanIsForcedByVisAreaFlags = GetVisAreaManager()->IsOceanVisible();

  if ( !IsOutdoorVisible() && !bOceanIsForcedByVisAreaFlags )
    return;

  float fOceanLevel = GetTerrain()->GetWaterLevel();

  // check for case when no any visible terrain sectors has minz lower than ocean level
  bool bOceanVisible = !Get3DEngine()->m_bShowTerrainSurface || (GetCVars()->e_VoxTer && GetCVars()->e_Voxel);
  bOceanVisible |= m_pTerrain->GetDistanceToSectorWithWater()>=0 && fOceanLevel && m_pTerrain->IsOceanVisible();

  if(	bOceanVisible && (dwDrawFlags & DLD_TERRAIN_WATER) && m_bOcean && GetCVars()->e_WaterOcean )
  {
    Vec3 vCamPos = GetCamera().GetPosition();
    float fWaterPlaneSize = GetCamera().GetFarPlane();

    AABB boxOcean( Vec3( vCamPos.x-fWaterPlaneSize, vCamPos.y-fWaterPlaneSize,0),
      Vec3( vCamPos.x+fWaterPlaneSize, vCamPos.y+fWaterPlaneSize,fOceanLevel+0.5f) );

    if((! bOceanIsForcedByVisAreaFlags && GetCamera().IsAABBVisible_EM(boxOcean)) ||
      (   bOceanIsForcedByVisAreaFlags && GetCamera().IsAABBVisible_E (boxOcean)))
    {
      bool bOceanIsVisibleFromIndoor = true;
      if(class PodArray<CCamera> * pMultiCamera = GetCamera().m_pMultiCamera)
      {
        for(int i=0; i<pMultiCamera->Count(); i++)
        {
          CVisArea * pExitPortal = (CVisArea *)(pMultiCamera->Get(i))->m_pPortal;
          float fMinZ = pExitPortal->GetAABBox()->min.z;
          float fMaxZ = pExitPortal->GetAABBox()->max.z;            

          if(!bOceanIsForcedByVisAreaFlags)
          {
            if(fMinZ>fOceanLevel && vCamPos.z<fMinZ)
              bOceanIsVisibleFromIndoor = false;
            
            if(fMaxZ<fOceanLevel && vCamPos.z>fMaxZ)
              bOceanIsVisibleFromIndoor = false;
          }
        }
      }

      if( bOceanIsVisibleFromIndoor && ( !GetCVars()->e_CoverageBuffer || GetCoverageBuffer()->IsObjectVisible(boxOcean,eoot_OCEAN,0) ) )
      {
        m_pTerrain->RenderTerrainWater(); 

        if( GetCVars()->e_WaterWaves && (GetOceanRenderFlags() & OCR_OCEANVOLUME_VISIBLE) )
          GetWaterWaveManager()->Update();
      }
    }
  }
}

void C3DEngine::RenderSkyBox(IMaterial * pMat)
{
	FUNCTION_PROFILER_3DENGINE;

	if(!Get3DEngine()->GetCoverageBuffer()->IsOutdooVisible())
		return;

	// hdr sky dome
	// TODO: temporary workaround to force the right sky dome for the selected shader
	if (m_pREHDRSky && IsHDRSkyMaterial(pMat))
	{
		if( GetCVars()->e_SkyBox )
		{
			// set sky light quality
			if( GetCVars()->e_SkyQuality < 1 )
				GetCVars()->e_SkyQuality = 1;
			else if( GetCVars()->e_SkyQuality > 2 )
				GetCVars()->e_SkyQuality = 2;
			m_pSkyLightManager->SetQuality( GetCVars()->e_SkyQuality ); 

			// set sky light incremental update rate and perform update
			if( GetCVars()->e_SkyUpdateRate <= 0.0f )
				GetCVars()->e_SkyUpdateRate = 0.01f;
			m_pSkyLightManager->IncrementalUpdate( GetCVars()->e_SkyUpdateRate ); 

			// prepare render object
			CRenderObject * pObj = GetRenderer()->EF_GetObject( true, -1 );
      if (!pObj)
        return;
			pObj->m_II.m_Matrix.SetTranslationMat( GetCamera().GetPosition() );
			pObj->m_ObjFlags |= FOB_TRANS_TRANSLATE;
			pObj->m_pRenderNode = 0;//m_pREHDRSky;

/*			if( 0 == m_nRenderStackLevel )
			{
				// set scissor rect
				pObj->m_nScissorX1 = GetCamera().m_ScissorInfo.x1;
				pObj->m_nScissorY1 = GetCamera().m_ScissorInfo.y1;
				pObj->m_nScissorX2 = GetCamera().m_ScissorInfo.x2;
				pObj->m_nScissorY2 = GetCamera().m_ScissorInfo.y2;
			}*/

			m_pREHDRSky->m_pRenderParams = m_pSkyLightManager->GetRenderParams();
			m_pREHDRSky->m_moonTexId = m_nNightMoonTexId;
			
			// add sky dome to render list
			GetRenderer()->EF_AddEf(m_pREHDRSky, pMat->GetShaderItem(), pObj, EFSLIST_GENERAL, 1);
		}

		GetRenderer()->SetSkyLightRenderParams( m_pSkyLightManager->GetRenderParams() );
	}
	// skybox
	else 
  {
		if (pMat && m_pRESky && GetCVars()->e_SkyBox )
		{
			CRenderObject * pObj = GetRenderer()->EF_GetObject(true, -1);
      if (!pObj)
        return;
			pObj->m_II.m_Matrix.SetTranslationMat(GetCamera().GetPosition());
			pObj->m_II.m_Matrix = pObj->m_II.m_Matrix*Matrix33::CreateRotationZ( DEG2RAD(m_fSkyBoxAngle) );
			pObj->m_ObjFlags |= FOB_TRANS_TRANSLATE | FOB_TRANS_ROTATE;
			
			m_pRESky->m_fTerrainWaterLevel = max(0.0f,m_pTerrain->GetWaterLevel());
			m_pRESky->m_fSkyBoxStretching = m_fSkyBoxStretching;

			GetRenderer()->SetSkyLightRenderParams( 0 );
			GetRenderer()->EF_AddEf(m_pRESky, pMat->GetShaderItem(), pObj, EFSLIST_GENERAL, 1);		
		}
		
		GetRenderer()->SetSkyLightRenderParams( 0 );
  }

}

void C3DEngine::DrawTextRightAligned( const float x, const float y, const char * format, ... )
{
	va_list args;
	va_start(args, format);

	SDrawTextInfo ti;
	ti.flags = eDrawText_FixedSize | eDrawText_Right | eDrawText_2D | eDrawText_Monospace;
	ti.xscale = ti.yscale = 1.3f;
	GetRenderer()->DrawTextQueued( Vec3(x,y,1.0f),ti,format,args );

	va_end(args);
}

void C3DEngine::DrawTextRightAligned( const float x, const float y, const float scale, const ColorF &color,const char * format, ... )
{
	va_list args;
	va_start(args, format);

	SDrawTextInfo ti;
	ti.flags = eDrawText_FixedSize | eDrawText_Right | eDrawText_2D | eDrawText_Monospace;
	ti.color[0] = color[0];
	ti.color[1] = color[1];
	ti.color[2] = color[2];
	ti.color[3] = color[3];
	ti.xscale = ti.yscale = scale;
	GetRenderer()->DrawTextQueued( Vec3(x,y,1.0f),ti,format,args );

	va_end(args);
}

int __cdecl C3DEngine__Cmp_FPS(const void* v1, const void* v2)
{
	float f1 = *(float*)v1;
	float f2 = *(float*)v2;

	if(f1 > f2)
		return 1;
	else if(f1 < f2)
		return -1;

	return 0;
}

inline void Blend( float& Stat, float StatCur, float fBlendCur )
{
	Stat = Stat * (1.f - fBlendCur) + StatCur * fBlendCur;
}

inline void Blend( float& Stat, int& StatCur, float fBlendCur )
{
	Blend( Stat, float(StatCur), fBlendCur );
	StatCur = int_round(Stat);
}


static void AppendString( char * &szEnd, const char *szToAppend )
{
	assert(szToAppend);

	while(*szToAppend)
		*szEnd++ = *szToAppend++;
	
	*szEnd++=' ';
	*szEnd=0;
}








void C3DEngine::DisplayInfo(float & fTextPosX, float & fTextPosY, float & fTextStepY, const bool bEnhanced)
{ 
//  FUNCTION_PROFILER_3DENGINE; causes 0 fps in stats
	static ICVar* pDisplayInfo = GetConsole()->GetCVar("r_DisplayInfo");
	if (pDisplayInfo && pDisplayInfo->GetIVal()==0)
		return;

	if (gEnv->pSystem->IsDedicated())
		return;

#if defined(INFO_FRAME_COUNTER)
  static int frameCounter = 0;
#endif
  GetRenderer()->SetState(GS_NODEPTHTEST);

	// If stat averaging is on, compute blend amount for current stats.
	float fFPS = GetTimer()->GetFrameRate();


	arrFPSforSaveLevelStats.push_back(SATURATEB((int)fFPS));

	static float fBlendTime = 0.f;
	float fFrameTime = GetTimer()->GetRealFrameTime();
	fBlendTime += fFrameTime;

	int iBlendMode = 0;
	float fBlendCur = GetTimer()->GetProfileFrameBlending(&fBlendTime, &iBlendMode);

	// make level name
	char szLevelName[128];

	*szLevelName=0;
	{
		int ii;
		for(ii=strlen(m_szLevelFolder)-2; ii>0; ii--)
			if(m_szLevelFolder[ii] == '\\' || m_szLevelFolder[ii] == '/')
				break;

		if(ii>=0)
		{
			strncpy(szLevelName,&m_szLevelFolder[ii+1],sizeof(szLevelName));
			szLevelName[sizeof(szLevelName)-1] = 0;

			for(int i=strlen(szLevelName)-1; i>0; i--)
				if(szLevelName[i] == '\\' || szLevelName[i] == '/')
					szLevelName[i]=0;
		}
	}

	fTextPosY = -10;
	fTextStepY = 13;

	//take overscan borders into account
	fTextPosX = (float)GetRenderer()->GetWidth() - 5.0f;
	Vec2 overscanBorders = *(Vec2*)gEnv->pRenderer->EF_Query(EFQ_OverscanBorders);
	fTextPosX -= ((float)gEnv->pRenderer->GetWidth()) * overscanBorders.x;
	fTextPosY += ((float)gEnv->pRenderer->GetHeight()) * overscanBorders.y;


  Matrix33 m = Matrix33(GetCamera().GetMatrix());
	//m.OrthonormalizeFast();		// why is that needed? is it?
  Ang3 aAng = RAD2DEG(Ang3::GetAnglesXYZ(m));
	Vec3 vPos = GetCamera().GetPosition();

	// display out of memory message if an allocation failed
	if( gEnv->bIsOutOfMemory )
	{
		ColorF fColor( 1.0f,0.0f,0.0f,1.0f);
		DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 4.0f, fColor, "**** Out of Memory ****" );		
		fTextPosY+=40.0f;
	}

#ifdef SP_DEMO

	static ICVar* pDisplayInfo = GetConsole()->GetCVar("r_DisplayInfo");
	if (pDisplayInfo && pDisplayInfo->GetIVal()!=2)
	{
#endif	
  DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "CamPos=%.2f %.2f %.2f Angl=%3d %2d %3d ZN=%.2f ZF=%d Zoom=%.2f", 
    vPos.x,vPos.y,vPos.z, (int)aAng.x,(int)aAng.y,(int)aAng.z,
		GetCamera().GetNearPlane(), (int)GetCamera().GetFarPlane(), m_fZoomFactor);
#ifdef SP_DEMO
	}
#endif	

	// get version
	const SFileVersion & ver = GetSystem()->GetFileVersion();
	//char sVersion[128];
	//ver.ToString(sVersion);

	// Get memory usage.
	static IMemoryManager::SProcessMemInfo processMemInfo;
	{
		static int nGetMemInfoCount = 0;
		if ((nGetMemInfoCount&0x1F) == 0 && GetISystem()->GetIMemoryManager())
		{
			// Only get mem stats every 32 frames.
			GetISystem()->GetIMemoryManager()->GetProcessMemInfo(processMemInfo);
		}
		nGetMemInfoCount++;
	}

	bool bHDREnabled = m_pRenderer->EF_Query(EFQ_HDRModeEnabled)!=0;
	bool bMultiGPU = m_pRenderer->EF_Query(EFQ_MultiGPUEnabled)!=0;
  bool bSRGBEnabled = m_pRenderer->EF_Query(EFQ_sLinearSpaceShadingEnabled)!=0;
  
	const char* pRenderType(0);
	switch(gEnv->pRenderer->GetRenderType())
	{
		case eRT_DX9: pRenderType = "DX9"; break;
		case eRT_DX11: pRenderType = "DX11"; break;
		case eRT_Xbox360: pRenderType = "X360"; break;
		case eRT_PS3: pRenderType = "PS3"; break;
		case eRT_Null: pRenderType = "Null"; break;
		case eRT_Undefined:
		default: assert(0); pRenderType = "Undefined"; break;
	}


	static int iRarelyUpdatedSpec=-2;				// for performance reasons we don't do this every frame, -2 means it' needs to be updated
	{
		static float fRarelyUpdateTimer=0;

		fRarelyUpdateTimer+=fFrameTime;

		if(fRarelyUpdateTimer>0.800f)					//  update it every once in a while
		{
			fRarelyUpdateTimer=0;
			iRarelyUpdatedSpec=-2;
		}
	}

	if(iRarelyUpdatedSpec==-2)							// update required
	{
		static ICVar *pSysSpecFull = GetConsole()->GetCVar("sys_spec_full");

		if(pSysSpecFull)
			iRarelyUpdatedSpec = pSysSpecFull->GetRealIVal();		// that can be a bit slow
	}

	const bool bCGFStreaming = GetCVars()->e_StreamCgf && m_pObjManager;
	const bool bTexStreaming = gEnv->pSystem->GetStreamEngine() && m_pRenderer->EF_Query(EFQ_TextureStreamingEnabled);

	{
		char szFlags[128], *szFlagsEnd=szFlags;

		switch(iRarelyUpdatedSpec)
		{
			case 1:		AppendString(szFlagsEnd,"LowSpec");break;
			case 2:		AppendString(szFlagsEnd,"MedSpec");break;
			case 3:		AppendString(szFlagsEnd,"HighSpec");break;
			case 4:		AppendString(szFlagsEnd,"VeryHighSpec");break;
			case -1:  AppendString(szFlagsEnd,"Custom");break;
			case -2:	break;																					// cvar doesn't exist yet or spec is -2 which doesn't make sense
			default:	assert(0);
		}

		static ICVar* pMultiThreaded = GetConsole()->GetCVar("r_MultiThreaded");
		if (pMultiThreaded && pMultiThreaded->GetIVal() > 0)
			AppendString(szFlagsEnd, "MT");

		if(bHDREnabled)
			AppendString(szFlagsEnd,"HDR");

    if(bSRGBEnabled)
      AppendString(szFlagsEnd,"SRGB");

		if(GetCVars()->e_GI != 0 && GetCVars()->e_GIAmount * GetGIAmount() > .001f)
			AppendString(szFlagsEnd,"GI");

		if(bMultiGPU)
			AppendString(szFlagsEnd,"MGPU");

		if(gEnv->pCryPak->GetLvlResStatus())
			AppendString(szFlagsEnd,"LvlRes");

		if (gEnv->pSystem->IsDevMode())
			AppendString(szFlagsEnd,"DevMode");

		if (m_pRenderer->EF_Query(EFQ_FSAAEnabled))
			AppendString(szFlagsEnd,"FSAA");

		if(bCGFStreaming || bTexStreaming)
		{
			if(bCGFStreaming && !bTexStreaming)
				AppendString(szFlagsEnd,"StmG");
			if(bTexStreaming && !bCGFStreaming)
				AppendString(szFlagsEnd,"StmT");
			if(bTexStreaming && bCGFStreaming)
				AppendString(szFlagsEnd,"StmGT");
		}

		// remove last space
		if(szFlags!=szFlagsEnd)
			*(szFlagsEnd-1)=0;

		DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "%s %dbit %s Level=%s Build=%d.%d.%d.%d",
			pRenderType, sizeof(char *) * 8, szFlags,szLevelName, ver.v[3],ver.v[2],ver.v[1],ver.v[0]);
	}

  // Polys in scene
	int nPolygons, nShadowVolPolys;
	GetRenderer()->GetPolyCount(nPolygons,nShadowVolPolys);
	int nDrawCalls = GetRenderer()->GetCurrentNumberOfDrawCalls();
	if (fBlendCur != 1.f)
	{
		// Smooth over time.
		static float fPolygons, fShadowVolPolys, fDrawCalls;
		Blend(fPolygons, nPolygons, fBlendCur);
		Blend(fShadowVolPolys, nShadowVolPolys, fBlendCur);
		Blend(fDrawCalls, nDrawCalls, fBlendCur);
	}
  // 
  static float m_lastAverageDPTime = -FLT_MAX;
  float curTime = gEnv->pTimer->GetAsyncCurTime();
  static int lastDrawCalls = 0;
  static int avgPolys = 0;
  static int sumPolys = 0;
  static int nPolysFrames = 0;
  if( curTime<m_lastAverageDPTime )
  {
    m_lastAverageDPTime = curTime;
  }
  if( curTime-m_lastAverageDPTime>1.0f )
  {
    lastDrawCalls = nDrawCalls;
    m_lastAverageDPTime = curTime;
    avgPolys = nPolysFrames ? sumPolys/nPolysFrames : 0;
    sumPolys=nPolygons;
    nPolysFrames = 1;
  }
  else
  {
    nPolysFrames++;
    sumPolys+=nPolygons;
  }
  //
	if (nShadowVolPolys>0)
  {
		DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "Tris:%2d,%03d (%2d,%03d) (SV %d) DP:%d (%d)",
			nPolygons/1000, nPolygons%1000, avgPolys/1000, avgPolys%1000, nShadowVolPolys, nDrawCalls,lastDrawCalls);
  }
	else
  {
		DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "Tris:%2d,%03d (%2d,%03d) DP:%d (%d)",
			nPolygons/1000, nPolygons%1000, avgPolys/1000, avgPolys%1000, 
      nDrawCalls,lastDrawCalls);
  }

  // print stats about CGF's streaming
  if( bCGFStreaming )
  {
    static char szCGFStreaming[256]="";
		static SObjectsStreamingStatus objectsStreamingStatus = {0};

    if(!(GetMainFrameID()&15) || !szCGFStreaming[0] || GetCVars()->e_StreamCgfDebug)
    {
      m_pObjManager->GetObjectsStreamingStatus(objectsStreamingStatus);
      sprintf_s(szCGFStreaming, 256, "CGF streaming: Loaded:%d InProg:%d All:%d Act:%d MemUsed:%2.2f MemReq:%2.2f PoolSize:%d", 
        objectsStreamingStatus.nReady, objectsStreamingStatus.nInProgress, objectsStreamingStatus.nTotal, objectsStreamingStatus.nActive, float(objectsStreamingStatus.nAllocatedBytes)/1024/1024, float(objectsStreamingStatus.nMemRequired)/1024/1024, GetCVars()->e_StreamCgfPoolSize);
    }

    bool bOutOfMem((float(objectsStreamingStatus.nMemRequired)/1024/1024)>GetCVars()->e_StreamCgfPoolSize);
    if( (pDisplayInfo->GetIVal() == 2 || GetCVars()->e_StreamCgfDebug) || bOutOfMem )
      DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 1.3f, bOutOfMem ? Col_Red : Col_White, szCGFStreaming);
  }

	// print stats about textures' streaming
	if( bTexStreaming )
	{
		static char szTexStreaming[256]="";
		static bool bCloseToOutOfMem = false;
		static bool bOutOfMem = false;

		if(!(GetMainFrameID()%30) || !szTexStreaming[0])
		{
			STextureStreamingStats stats;
			m_pRenderer->EF_Query(EFQ_GetTexStreamingInfo, (INT_PTR)&stats);
			const int iPercentage = int((float)stats.nCurrentPoolSize / stats.nMaxPoolSize * 100.f);
			sprintf_s(szTexStreaming, "TexStreaming: MemUsed:%.2fMB(%d%%%%) PoolSize:%dMB Trghput:%dKB/s", (float)stats.nCurrentPoolSize / 1024 / 1024, iPercentage, stats.nMaxPoolSize / 1024 / 1024, stats.nThroughput / 1024);

			if(!(GetMainFrameID()%150))
			{
				bCloseToOutOfMem = iPercentage >= 90;
				bOutOfMem = stats.bPoolOverflow;
			}
			else
			{
				bCloseToOutOfMem |= iPercentage >= 90;
				bOutOfMem |= stats.bPoolOverflow;
			}
		}

		DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 1.3f, bOutOfMem ? Col_Red : (bCloseToOutOfMem ? Col_Orange : Col_White) , szTexStreaming);
	}

  // print list of heavy CFG's
  if( m_pObjManager && pDisplayInfo && GetCVars()->e_StreamCgf )
  { 
    if(GetCVars()->e_StreamCgfDebug==3)
    {
      DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "----------------------------------------------------------------------------------------------------");

      for(int nObjId=0; nObjId<m_pObjManager->m_arrStreamableObjects.Count(); nObjId++)
      {
        CStatObj * pStatObj = (CStatObj *)m_pObjManager->m_arrStreamableObjects[nObjId].GetStreamAbleObject();

        int nKB = pStatObj->GetStreamableContentMemoryUsage() >> 10;
        if( nKB > GetCVars()->e_StreamCgfDebugMinObjSize )
        {
          string sName;
          pStatObj->GetStreamableName(sName);

          char * pComment = 0;
          if(pStatObj->m_pLod0) pComment = "  LOD_X";
          else if(!pStatObj->m_bLodsAreLoadedFromSeparateFile && pStatObj->m_nLoadedLodsNum>1) pComment = " SINGLE";
          else if(pStatObj->m_nLoadedLodsNum>1) pComment = "  LOD_0";
          else pComment = "NO_LODS";

          int nDiff = SATURATEB(int(float(nKB - GetCVars()->e_StreamCgfDebugMinObjSize)/GetCVars()->e_StreamCgfDebugMinObjSize*255));
          ColorB col(nDiff, 255-nDiff, 0, 255);
          ColorF fColor(col[0]/255.f,col[1]/255.f,col[2]/255.f,col[3]/255.f);
          DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 1.3f, fColor, "%s, %s, %1.2f mb, %s", 
            sName.c_str(), pComment, 1.f/1024.f*nKB, (pStatObj->m_eStreamingStatus == ecss_Ready) ? "In_Mem" : "Unload");

          if(fTextPosY > (float)gEnv->pRenderer->GetHeight())
            break;
        }
      }

      DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "----------------------------------------------------------------------------------------------------");
    }
  }


  if(pDisplayInfo)
	if(pDisplayInfo->GetIVal() == 2 || m_pVoxTerrain)	// streaming info
	{	
		if( gEnv->pSystem->GetStreamEngine())
		{
			static char szStreaming[512]="";
			if(!(GetMainFrameID()&15))
			{
				int nReady=0, nInProgress=0, nStarted=0;
				nStarted = gEnv->pSystem->GetStreamEngine()->GetNumJobs(IStreamEngine::ejtStarted);
				nInProgress = gEnv->pSystem->GetStreamEngine()->GetNumJobs(IStreamEngine::ejtPending);
				nReady = gEnv->pSystem->GetStreamEngine()->GetNumJobs(IStreamEngine::ejtFinished);
				sprintf(szStreaming, "Streaming jobs: %d/%d/%d", nStarted, nInProgress, nReady);
			}
			DrawTextRightAligned( fTextPosX, fTextPosY += fTextStepY, szStreaming);
		}
	}

	if(GetCVars()->e_VoxTer && m_pVoxTerrain)
	{
    int nLine=0;
    while(char * szStatus = m_pVoxTerrain->GetStatusString(nLine))
    {
      DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 1.3f, ColorF (0.5f, 1.0f, 1.0f), szStatus );
      nLine++;
    }

    if(GetCVars()->e_VoxTer == 2)
      for(int i=0; i<20; i++)
        DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "%d = %.2f", i, CVoxTerrain::m_arrTimeStats[i] );
	}

/*
  if(GetCVars()->e_stream_areas && m_pTerrain)
  {
    int nReady=0, nTotal=0;

    m_pTerrain->GetStreamingStatus(nReady,nTotal);
    DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY,"Terrain streaming: %d/%d", nReady, nTotal);

    m_pVisAreaManager->GetStreamingStatus(nReady,nTotal);
    DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY,"Indoor streaming: %d/%d", nReady, nTotal);
  }
*/


	//////////////////////////////////////////////////////////////////////////
	// Display Info about dynamic lights.
	//////////////////////////////////////////////////////////////////////////
	{
		float fDebugListPosY = -10;

		int nVirtMemMB = (int)(processMemInfo.PagefileUsage/(1024*1024));

		if(GetCVars()->e_DebugLights)
		{
			char sLightsList[512]="";
			for(int i=0; i<m_lstDynLights.Count(); i++)
			{
				if(m_lstDynLights[i]->m_Id>=0 && m_lstDynLights[i]->m_fRadius >= 0.5f )
					if(!(m_lstDynLights[i]->m_Flags & DLF_FAKE))
					{
						if( GetCVars()->e_DebugLights == 2 )
						{
							int nShadowCasterNumber = 0;
							for(int l=0; l<MAX_GSM_LODS_NUM; l++)
							{
								ShadowMapFrustum * pFr = m_lstDynLights[i]->m_pOwner->GetShadowFrustum(l);
								if(pFr && pFr->pCastersList)
									nShadowCasterNumber += pFr->pCastersList->Count();
							}
							DrawTextRightAligned( fTextPosX, fDebugListPosY+=fTextStepY, "%s - SM%d", m_lstDynLights[i]->m_sName, nShadowCasterNumber );
						}

						if(i<4)
						{
							char buff[32]="";
							strncat(buff,m_lstDynLights[i]->m_sName,8);
							buff[9]=0;

							if(m_lstDynLights[i]->m_Flags&DLF_CASTSHADOW_MAPS)
							{
								strcat(buff,"-SM");

								int nCastingObjects = 0;
								for(int l=0; l<MAX_GSM_LODS_NUM; l++)
								{
									ShadowMapFrustum * pFr = m_lstDynLights[i]->m_pOwner->GetShadowFrustum(l);
									if(pFr && pFr->pCastersList)
										nCastingObjects += pFr->pCastersList->Count();
								}

								if(nCastingObjects)
								{
									char tmp[32];
									sprintf(tmp,"%d",nCastingObjects);
									strcat(buff, tmp);
								}
							}

							strcat(sLightsList, buff);        
							if(i<m_lstDynLights.Count()-1)
								strcat(sLightsList,",");
						}
					}
			}

#ifdef WIN64
#pragma warning( push )									//AMD Port
#pragma warning( disable : 4267 )
#endif

			DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "DLights=%s(%d/%d/%d/%d)", sLightsList, m_nRenderLightsNum, m_nRealLightsNum, m_lstDynLights.Count(), m_nDeferredLightsNum);
		}
		else
		{












































			DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "Mem=%dMB DLights=(%d/%d/%d/%d)",nVirtMemMB,m_nRenderLightsNum, m_nRealLightsNum, (int)m_lstDynLights.Count(),m_nDeferredLightsNum );

		}

		m_nDeferredLightsNum = 0;
	}


	
	//////////////////////////////////////////////////////////////////////////
	// Display Current fps
	//////////////////////////////////////////////////////////////////////////
	if (iBlendMode)
	{
		// Track FPS frequency, report min/max.
		Blend(m_fAverageFPS, fFPS, fBlendCur);

		Blend(m_fMinFPS, fFPS, fBlendCur);
		m_fMinFPS = min(m_fMinFPS, fFPS);

		Blend(m_fMaxFPS, fFPS, fBlendCur);
		m_fMaxFPS = max(m_fMaxFPS, fFPS);

		const char* sMode = "";
		switch (iBlendMode)
		{
			case 1: sMode = "avg"; break;
			case 2: sMode = "peak wt"; break;
			case 3: sMode = "peak hold"; break;
		}
		DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 1.5f, ColorF(1.0f,1.0f,0.5f,1.0f), 
			"FPS %.1f [%.0f..%.0f], %s over %.1f s", 
			m_fAverageFPS, m_fMinFPS, m_fMaxFPS, sMode, fBlendTime );
	}
	else
  {
		const int nHistorySize = 16;
		static float arrfFrameRateHistory[nHistorySize]={0};

		static int nFrameId = 0; nFrameId++;
		int nSlotId = nFrameId%nHistorySize;
		assert(nSlotId>=0 && nSlotId<nHistorySize);
		arrfFrameRateHistory[nSlotId] = min(9999.f, GetTimer()->GetFrameRate());

		float fMinFPS = 9999.0f;
		float fMaxFPS = 0;
		for (int i = 0; i < nHistorySize; i++)
		{
			if (arrfFrameRateHistory[i] < fMinFPS)
				fMinFPS = arrfFrameRateHistory[i];
			if (arrfFrameRateHistory[i] > fMaxFPS)
				fMaxFPS = arrfFrameRateHistory[i];
		}

		float fFrameRate = 0;
		float fValidFrames = 0;
		for(int i=0; i<nHistorySize; i++)
		{
			int s = (nFrameId-i)%nHistorySize;
			fFrameRate += arrfFrameRateHistory[s];
			fValidFrames++;
		}
		fFrameRate /= fValidFrames;

		m_fAverageFPS = fFrameRate;
		m_fMinFPS = fMinFPS;
		m_fMaxFPS = fMaxFPS;



		if(bEnhanced)
		{
			DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "%6.2f ~%6.2f ms (%6.2f..%6.2f)",
				GetTimer()->GetFrameTime()*1000.0f, 1000.0f/max(0.0001f,fFrameRate),
				1000.0f/max(0.0001f,fMinFPS),
				1000.0f/max(0.0001f,fMaxFPS));
		}
		else
		{
      float fMax = (int(GetCurTimeSec()*2)&1) ? 999.f : 888.f;
			DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 1.4f,ColorF(1.0f,1.0f,0.2f,1.0f), "FPS %5.1f (%3d..%3d)",
				min(fMax, fFrameRate), (int)min(fMax, fMinFPS), (int)min(fMax, fMaxFPS) );
		}
  }

  if(GetCVars()->e_ParticlesDebug & 1)
  {
		// Show particle stats.
		static SParticleCounts Counts;
		SParticleCounts CurCounts;
		m_pPartManager->GetCounts(CurCounts);
		BlendArray(FloatArray(Counts), 1.f-fBlendCur, FloatArray(CurCounts), fBlendCur);
		float fScreenPix = (float)(GetRenderer()->GetWidth() * GetRenderer()->GetHeight());

    DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 
      "Particle %5.0f/%5.0f/%5.0f, Fill %5.2f/%5.2f, Emitter %3.0f/%3.0f/%3.0f",
				Counts.ParticlesRendered, Counts.ParticlesActive, Counts.ParticlesAlloc, 
				Counts.PixelsRendered / fScreenPix, Counts.PixelsProcessed / fScreenPix, 
				Counts.EmittersRendered, Counts.EmittersActive, Counts.EmittersAlloc);
		if (GetCVars()->e_ParticlesDebug & AlphaBit('r'))
			DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 
				"Particle: %4.0f Reiter, %4.0f Ovf, %4.0f Reject, Coll (%4.1f Tr, %4.1f Ob), %4.1f Clip",
					Counts.ParticlesReiterate, Counts.ParticlesOverflow, Counts.ParticlesReject, 
					Counts.ParticlesCollideTerrain, Counts.ParticlesCollideObjects, Counts.ParticlesClip);
  }
	m_pPartManager->RenderDebugInfo();








#ifdef WIN64
#pragma warning( pop )									//AMD Port
#endif

	if(GetCVars()->e_GsmStats)
	{
		DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "--------------- GSM Stats ---------------" );
		
		if(CLightEntity::ShadowMapInfo * pSMI = m_pSun->m_pShadowMapInfo)
		{
			int arrGSMCastersCount[MAX_GSM_LODS_NUM];
			memset(arrGSMCastersCount,0,sizeof(arrGSMCastersCount));
			char szText[256] = "Objects count per shadow map: ";
			for(int nLod=0; nLod<GetCVars()->e_GsmLodsNum && nLod<MAX_GSM_LODS_NUM; nLod++)
			{
				ShadowMapFrustum * & pLsource = pSMI->pGSM[nLod];
				
				if(nLod)
					strcat(&szText[strlen(szText)], ", ");

				sprintf(&szText[strlen(szText)], "%d", pLsource->pCastersList->Count());
			}

			DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, szText );
		}

		for(int nSunInUse=0; nSunInUse<2; nSunInUse++)
		{
			if(nSunInUse)
				DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "WithSun  ListId   FrNum UserNum" );
			else
				DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "NoSun    ListId   FrNum UserNum" );

			for(ShadowFrustumListsCache::iterator it = m_FrustumsCache[nSunInUse].begin(); it != m_FrustumsCache[nSunInUse].end(); ++it)
			{
				int nListId = (int)it->first;
				PodArray<ShadowMapFrustum*> * pList = it->second;

				DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 
					"%8d %8d %8d", 
					nListId,
					pList->Count(), m_FrustumsCacheUsers[nSunInUse][nListId]);
			}
		}
	}

	// objects counter
	if(GetCVars()->e_ObjStats)
	{
#define DRAW_OBJ_STATS(_var) DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "%s: %d", (#_var), GetInstCount(_var))

    DRAW_OBJ_STATS( eERType_Unknown );
    DRAW_OBJ_STATS( eERType_Brush );
    DRAW_OBJ_STATS( eERType_Vegetation );
    DRAW_OBJ_STATS( eERType_Light );
    DRAW_OBJ_STATS( eERType_Cloud );
    DRAW_OBJ_STATS( eERType_VoxelObject );
    DRAW_OBJ_STATS( eERType_FogVolume );
    DRAW_OBJ_STATS( eERType_Decal );
    DRAW_OBJ_STATS( eERType_ParticleEmitter );
    DRAW_OBJ_STATS( eERType_WaterVolume );
    DRAW_OBJ_STATS( eERType_WaterWave );
    DRAW_OBJ_STATS( eERType_Road );
    DRAW_OBJ_STATS( eERType_DistanceCloud );
    DRAW_OBJ_STATS( eERType_VolumeObject );
    DRAW_OBJ_STATS( eERType_AutoCubeMap );
    DRAW_OBJ_STATS( eERType_Rope );
    DRAW_OBJ_STATS( eERType_PrismObject );
    DRAW_OBJ_STATS( eERType_IsoMesh );
    DRAW_OBJ_STATS( eERType_IrradianceVolume );

    for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
      if(m_pObjectsTree[nSID])
    {
      DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "--- By list type: ---");
      DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "  Main:      %d", m_pObjectsTree[nSID]->GetObjectsCount(eMain));
      DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "Caster:      %d", m_pObjectsTree[nSID]->GetObjectsCount(eCasters));
      DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "Occlud:      %d", m_pObjectsTree[nSID]->GetObjectsCount(eOccluders));
      DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "Sprite:      %d", m_pObjectsTree[nSID]->GetObjectsCount(eSprites));
      DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "Lights:      %d", m_pObjectsTree[nSID]->GetObjectsCount(eLights));
      DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "LigAll:      %d", m_lstStaticLights.Count());
    }

		int nFree = m_LTPRootFree.Count();
		int nUsed = m_LTPRootUsed.Count();
		DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "RNTmpData(Used+Free): %d + %d = %d (%d KB)",	 
			nUsed, nFree, nUsed+nFree, (nUsed+nFree)*sizeof(CRNTmpData)/1024);

    DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "COctreeNode::m_arrEmptyNodes.Count() = %d", COctreeNode::m_arrEmptyNodes.Count());
  }

	CCullBuffer * pCB = GetCoverageBuffer();
	if(pCB && GetCVars()->e_CoverageBuffer && GetCVars()->e_CoverageBufferDebug && pCB->TrisWritten())
		DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 
		"CB: Write:%3d/%2d Test:%4d/%4d/%3d ZFarM:%.2f ZNearM:%.2f Res:%d OI:%s TimeRatio:%.1f", 
			pCB->TrisWritten(), pCB->ObjectsWritten(),
			pCB->TrisTested(), pCB->ObjectsTested(), pCB->ObjectsTestedAndRejected(),
			pCB->GetZFarInMeters(),pCB->GetZNearInMeters(),pCB->SelRes(),
			pCB->IsOutdooVisible() ? "Out" : "In",
			GetObjManager()->m_fOcclTimeRatio);
	
#if defined(INFO_FRAME_COUNTER)
	++frameCounter;
	DrawTextRightAligned( fTextPosX, fTextPosY += fTextStepY, "Frame #%d", frameCounter);
#endif

	if(GetCVars()->e_TerrainTextureStreamingDebug && m_pTerrain)
	{
		int nCacheSize[2] = {0,0};
		m_pTerrain->GetTextureCachesStatus(nCacheSize[0],nCacheSize[1]);
		DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 
			"Terrain texture streaming status: waiting=%d, all=%d, pool=(%d+%d)", 
			m_pTerrain->GetNotReadyTextureNodesCount(), m_pTerrain->GetActiveTextureNodesCount(),
			nCacheSize[0], nCacheSize[1]);	
	}

  if(GetCVars()->e_TerrainBBoxes && m_pTerrain)
  {
    DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 
      "GetDistanceToSectorWithWater() = %.2f", m_pTerrain->GetDistanceToSectorWithWater());	
  }

	if(GetCVars()->e_ProcVegetation == 2)
	{
		CProcVegetPoolMan & pool = *CTerrainNode::GetProcObjPoolMan();
		int nAll; int nUsed = pool.GetUsedInstancesCount(nAll);

		DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "---------------------------------------" );
		DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "Procedural root pool status: used=%d, all=%d, active=%d", 
			nUsed, nAll, GetTerrain()->GetActiveProcObjNodesCount());
		DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "---------------------------------------" );
		for(int i=0; i<pool.m_lstUsed.Count(); i++)
		{
			CProcObjSector * pSubPool = pool.m_lstUsed[i];
			nUsed = pSubPool ->GetUsedInstancesCount(nAll);
			DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 
				"Used sector: used=%d, all=%dx%d", nUsed, nAll, (int)GetCVars()->e_ProcVegetationMaxObjectsInChunk);
		}
		for(int i=0; i<pool.m_lstFree.Count(); i++)
		{
			CProcObjSector * pSubPool = pool.m_lstFree[i];
			nUsed = pSubPool ->GetUsedInstancesCount(nAll);
			DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 
				"Free sector: used=%d, all=%dx%d", nUsed, nAll, (int)GetCVars()->e_ProcVegetationMaxObjectsInChunk);
		}
		DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "---------------------------------------" );
		{
			SProcObjChunkPool & chunks = *CTerrainNode::GetProcObjChunkPool();
			nUsed = chunks.GetUsedInstancesCount(nAll);
			DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, 
				"chunks pool status: used=%d, all=%d, %d MB", nUsed, nAll, 
				nAll*int(GetCVars()->e_ProcVegetationMaxObjectsInChunk)*sizeof(CVegetation)/1024/1024);
		}
	}

  ITimeOfDay *pTimeOfDay = Get3DEngine()->GetTimeOfDay();
  if(GetCVars()->e_TimeOfDayDebug && pTimeOfDay)
  {
    DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "---------------------------------------" );
    DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "------------ Time of Day  -------------" );
    DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, " " );

    int nVarCount = pTimeOfDay->GetVariableCount();
    for(int v = 0; v < nVarCount; ++v)
    {
      ITimeOfDay::SVariableInfo pVar;
      pTimeOfDay->GetVariableInfo( v, pVar );
      
      if( pVar.type == ITimeOfDay::TYPE_FLOAT )
        DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, " %s: %.9f", pVar.displayName, pVar.fValue[0]);
      else
        DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, " %s: %.3f %.3f %.3f", pVar.displayName, pVar.fValue[0], pVar.fValue[1], pVar.fValue[2]);
    }
    DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "---------------------------------------" );    
  }


	// We only show memory usage in dev mode.
	if (gEnv->pSystem->IsDevMode())
	{
		if (GetCVars()->e_DisplayMemoryUsageIcon)
		{
			uint64					nAverageMemoryUsage(0);
			uint64					nHighMemoryUsage(0);
			uint64					nCurrentMemoryUsage(0);
			const		uint64	nMegabyte(1024*1024);

			// Copied from D3DDriver.cpp, function CD3D9Renderer::RT_EndFrame().
			int							nIconSize = 16;







#if defined(_WIN64) || defined(WIN64)
      nAverageMemoryUsage=3000;
      nHighMemoryUsage=6000;
#elif defined(_WIN32) && !defined(XENON)
			nAverageMemoryUsage=800;
			nHighMemoryUsage=1200;
#endif //_WIN32

			// This is the same value as measured in the editor.
			nCurrentMemoryUsage=processMemInfo.PagefileUsage/nMegabyte;

			static bool bInitialized(false);
			if (!bInitialized)
			{
				if (gEnv->pRenderer)
				{
					if (!m_ptexIconLowMemoryUsage)
					{
						m_ptexIconLowMemoryUsage=gEnv->pRenderer->EF_LoadTexture("Shaders/EngineAssets/Icons/LowMemoryUsage.tif",FT_DONT_STREAM|FT_DONT_RESIZE,eTF_Unknown);
					}

					if (!m_ptexIconAverageMemoryUsage)
					{
						m_ptexIconAverageMemoryUsage=gEnv->pRenderer->EF_LoadTexture("Shaders/EngineAssets/Icons/AverageMemoryUsage.tif",FT_DONT_STREAM|FT_DONT_RESIZE,eTF_Unknown);
					}

					if (!m_ptexIconHighMemoryUsage)
					{
						m_ptexIconHighMemoryUsage=gEnv->pRenderer->EF_LoadTexture("Shaders/EngineAssets/Icons/HighMemoryUsage.tif", FT_DONT_STREAM|FT_DONT_RESIZE,eTF_Unknown);
					}

					if (!m_ptexIconEditorConnectedToConsole)
					{
						m_ptexIconEditorConnectedToConsole=gEnv->pRenderer->EF_LoadTexture("Shaders/EngineAssets/Icons/LivePreview.TIF", FT_DONT_STREAM|FT_DONT_RESIZE , eTF_Unknown);
					}
					bInitialized=true;
				}
			}

			ITexture*	pRenderTexture(m_ptexIconAverageMemoryUsage);
			if (nCurrentMemoryUsage>nHighMemoryUsage)
			{
				pRenderTexture=m_ptexIconHighMemoryUsage;
			}
			else if (nCurrentMemoryUsage<nAverageMemoryUsage)
			{
				pRenderTexture=m_ptexIconLowMemoryUsage;
			}

			if (pRenderTexture&&gEnv->pRenderer)
			{
				float vpWidth = (float)gEnv->pRenderer->GetWidth(), vpHeight = (float)gEnv->pRenderer->GetHeight();
				float iconWidth = (float)nIconSize / vpWidth * 800.0f;
				float iconHeight = (float)nIconSize / vpHeight * 600.0f;
				gEnv->pRenderer->Draw2dImage((fTextPosX/vpWidth)*800.0f-iconWidth,((fTextPosY+=nIconSize+3)/vpHeight)*600.0f,
				                             iconWidth, iconHeight, pRenderTexture->GetTextureID(), 0, 1.0f, 1.0f, 0);
			}

			if (gEnv->pGame)
			{
				IGameFramework*	pIGame(gEnv->pGame->GetIGameFramework());
				if (pIGame)
				{
					IRealtimeRemoteUpdate	*piRealtimeRemoteUpdate(pIGame->GetIRealTimeRemoteUpdate());
					if (piRealtimeRemoteUpdate)
					{
						if (m_ptexIconEditorConnectedToConsole&&gEnv->pRenderer&&piRealtimeRemoteUpdate->IsSyncingWithEditor())
						{
							float vpWidth = (float)gEnv->pRenderer->GetWidth(), vpHeight = (float)gEnv->pRenderer->GetHeight();
							float iconWidth = (float)nIconSize / vpWidth * 800.0f;
							float iconHeight = (float)nIconSize / vpHeight * 600.0f;
							gEnv->pRenderer->Draw2dImage((fTextPosX/vpWidth)*800.0f-iconWidth,((fTextPosY+=nIconSize+3)/vpHeight)*600.0f,
								iconWidth, iconHeight, m_ptexIconEditorConnectedToConsole->GetTextureID(), 0, 1.0f, 1.0f, 0);
						}
					}
				}
			}


		}
	}
}

void C3DEngine::DrawFarTrees()
{
  m_pObjManager->DrawFarObjects(GetMaxViewDistance());
}

void C3DEngine::GenerateFarTrees()
{
  m_pObjManager->GenerateFarObjects(GetMaxViewDistance());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void C3DEngine::SetupDistanceFog()
{
	FUNCTION_PROFILER_3DENGINE;

  GetRenderer()->SetFog(0, 0, max(GetMaxViewDistance(),0.f), &m_vFogColor[0], R_FOGMODE_LINEAR);
  GetRenderer()->EnableFog(GetCVars()->e_Fog>0);
}

void C3DEngine::ScreenShotHighRes(CStitchedImage* pStitchedImage,const int nRenderFlags, const CCamera &cam, const int dwDrawFlags, const int nFilterFlags,uint32 SliceCount,f32 fTransitionSize)
{
#if !defined(LINUX)

	// finish frame started by system
	GetRenderer()->EndFrame();

	GetConsole()->SetScrollMax(0);
	GetTimer()->EnableTimer(false);

	const uint32	ScreenWidth	=	GetRenderer()->GetWidth();
	const	uint32	ScreenHeight	=	GetRenderer()->GetHeight();
	uint32 * pImage = new uint32[ScreenWidth*ScreenHeight];
	for(uint32 yy=0; yy<SliceCount; yy++)
	for(uint32 xx=0; xx<SliceCount; xx++)
	{
		PrintMessage("Rendering tile %d of %d ... ",xx+yy*SliceCount+1,SliceCount*SliceCount);

		const int	BlendX	=	xx*2/SliceCount;
		const int	BlendY	=	yy*2/SliceCount;
		const int x	=	((xx*2)%SliceCount&~1)+BlendX;
		const int y	=	((yy*2)%SliceCount&~1)+BlendY;

		// start new frame and define needed tile
		const f32 ScreenScale	=	1.f/(1.f/static_cast<f32>(SliceCount)*(1.f+fTransitionSize));
		GetRenderer()->BeginFrame();
		GetRenderer()->SetRenderTile( 
			(static_cast<f32>(SliceCount-1-x)-fTransitionSize*0.5f)/static_cast<f32>(SliceCount)*ScreenScale,
			(static_cast<f32>(SliceCount-1-y)-fTransitionSize*0.5f)/static_cast<f32>(SliceCount)*ScreenScale,
			ScreenScale,ScreenScale);

		RenderInternal(nRenderFlags, cam, "ScreenShotHighRes",dwDrawFlags, nFilterFlags);

		GetRenderer()->EndFrame();

		PrintMessagePlus("reading frame buffer ... ");

		GetRenderer()->ReadFrameBufferFast(pImage, ScreenWidth, ScreenHeight);
		pStitchedImage->RasterizeRect(pImage,ScreenWidth,ScreenHeight,x,y,fTransitionSize,
																	fTransitionSize>0.0001f && BlendX,
																	fTransitionSize>0.0001f && BlendY);

		PrintMessagePlus("ok");
	}
	delete[] pImage;

	// re-start frame so system can safely finish it
	GetRenderer()->BeginFrame();

	// restore initial state
	GetRenderer()->SetViewport(0,0,GetRenderer()->GetWidth(),GetRenderer()->GetHeight());
	GetConsole()->SetScrollMax(300);
	GetTimer()->EnableTimer(true);
	GetRenderer()->SetRenderTile();

	// make sure folder existing
/*	CryCreateDirectory("Game\\HiResScreenShots",0);

	// find free file name
	int nFileId = 0;
	char sFileName[256];
	while(1)
	{
		snprintf(sFileName, sizeof(sFileName), "%s/HiResScreenShots/%d_%dx%d.%s", 
		  FIleUtil::GetGameFolder(),
			nFileId, 
			SliceCount, 
			SliceCount,
			stricmp(GetCVars()->e_ScreenShotFileFormat->GetString(),"tga")==0 ? "tga" : "jpg");

		FILE * fp = gEnv->pCryPak->FOpen(sFileName,"rb");
		if (!fp)
			break; // file doesn't exist

		gEnv->pCryPak->FClose(fp);
		nFileId++;
	}

	PrintMessage("Combining tiles into final image ... ");

	int nFinSize = lstSubImages.Count()*
		GetRenderer()->GetWidth()*GetRenderer()->GetHeight()*4;

	unsigned char * pFinalImage = (unsigned char *)malloc(nFinSize);
	if(!pFinalImage)
	{
		PrintMessage("Error making screen shot: memory allocation error (%d MB)", nFinSize/1024/1024);
		return;
	}

	// Save final image
	for(int nImageId=0; nImageId<lstSubImages.Count(); nImageId++)
	{
		int pos_X = (nImageId)%SliceCount*GetRenderer()->GetWidth();
		int pos_Y = (lstSubImages.Count()-nImageId-1)/SliceCount*GetRenderer()->GetHeight();

		for(int x=0; x<GetRenderer()->GetWidth(); x++)
			for(int y=0; y<GetRenderer()->GetHeight(); y++)
			{
				int nFin = ((pos_X+x) + (pos_Y+y)*GetRenderer()->GetWidth()*SliceCount);
				int nSub = ((			 x) + (			 y)*GetRenderer()->GetWidth());

				for(int c=0; c<4; c++)
					pFinalImage[nFin*4+c] = lstSubImages[nImageId][nSub*4+c];
			}

			delete [] lstSubImages[nImageId];
			lstSubImages[nImageId]=0;
	}

	PrintMessagePlus(" ok");

	PrintMessage("Writing %s ... ", sFileName);

	if(strstr(sFileName,".tga"))
	{
		GetRenderer()->WriteTGA(pFinalImage,
			GetRenderer()->GetWidth()*SliceCount, 
			GetRenderer()->GetHeight()*SliceCount, 
			sFileName, 32,32);
	}
	else
	{
		GetRenderer()->WriteJPG(pFinalImage,
			GetRenderer()->GetWidth()*SliceCount, 
			GetRenderer()->GetHeight()*SliceCount, 
			sFileName,32);
	}

	free(pFinalImage);*/

	PrintMessagePlus(" ok");

	GetTimer()->EnableTimer(true);

#endif
}



bool C3DEngine::ScreenShotMap(			CStitchedImage* pStitchedImage,
															const int							nRenderFlags,
															const CCamera&				_cam,
															const int							dwDrawFlags,
															const int							nFilterFlags,
															const uint32					SliceCount,
															const f32							fTransitionSize)
{
#if !defined(LINUX)

	const uint32	nTSize		=	GetTerrain()->GetTerrainSize();//*GetTerrain()->GetSectorSize();
//	const f32			fTLX			=	GetCVars()->e_ScreenShot_map_topleft_x*static_cast<f32>(nTSize)+fTransitionSize*GetRenderer()->GetWidth();
//	const f32			fTLY			=	(1.f-GetCVars()->e_ScreenShot_map_topleft_y)*static_cast<f32>(nTSize)+fTransitionSize*GetRenderer()->GetHeight();
//	const f32			fBRX			=	GetCVars()->e_ScreenShot_map_bottomright_x*static_cast<f32>(nTSize)+fTransitionSize*GetRenderer()->GetWidth();
//	const f32			fBRY			=	(1.f-GetCVars()->e_ScreenShot_map_bottomright_y)*static_cast<f32>(nTSize)+fTransitionSize*GetRenderer()->GetHeight();
	const f32			fTLX			=	GetCVars()->e_ScreenShotMapCenterX-GetCVars()->e_ScreenShotMapSizeX+fTransitionSize*GetRenderer()->GetWidth();
	const f32			fTLY			=	GetCVars()->e_ScreenShotMapCenterY-GetCVars()->e_ScreenShotMapSizeY+fTransitionSize*GetRenderer()->GetHeight();
	const f32			fBRX			=	GetCVars()->e_ScreenShotMapCenterX+GetCVars()->e_ScreenShotMapSizeX+fTransitionSize*GetRenderer()->GetWidth();
	const f32			fBRY			=	GetCVars()->e_ScreenShotMapCenterY+GetCVars()->e_ScreenShotMapSizeY+fTransitionSize*GetRenderer()->GetHeight();
	const f32			Height		=	GetCVars()->e_ScreenShotMapCamHeight;

	string SettingsFileName = GetLevelFilePath("ScreenshotMap.Settings");

	FILE * metaFile = gEnv->pCryPak->FOpen(SettingsFileName,"wt");
	if(metaFile)
	{
		char Data[1024*8];
		snprintf(Data,sizeof(Data), "<Map CenterX=\"%f\" CenterY=\"%f\" SizeX=\"%f\" SizeY=\"%f\" Height=\"%f\"  Quality=\"%d\" />",
												GetCVars()->e_ScreenShotMapCenterX,
												GetCVars()->e_ScreenShotMapCenterY,
												GetCVars()->e_ScreenShotMapSizeX,
												GetCVars()->e_ScreenShotMapSizeY,
												GetCVars()->e_ScreenShotMapCamHeight,
												GetCVars()->e_ScreenShotQuality);
		string data(Data);
		gEnv->pCryPak->FWrite(data.c_str(),data.size(),metaFile);
		gEnv->pCryPak->FClose(metaFile);
	}


	CCamera cam = _cam;
	Matrix34 tmX,tmY;
	tmX.SetRotationX(-gf_PI*0.5f);
	tmY.SetRotationY(-gf_PI*0.5f);
	Matrix34 tm	=	tmX*tmY;
	tm.SetTranslation(Vec3((fTLX+fBRX)*0.5f,(fTLY+fBRY)*0.5f,Height));
	cam.SetMatrix(tm);

	const f32 AngleX	=	atanf(((fBRX-fTLX)*0.5f)/Height);
	const f32 AngleY	=	atanf(((fBRY-fTLY)*0.5f)/Height);

	ICVar *r_drawnearfov = GetConsole()->GetCVar("r_DrawNearFoV");		assert(r_drawnearfov);
	const f32 drawnearfov_backup = r_drawnearfov->GetFVal();
	const f32 ViewingSize	=	(float)min(cam.GetViewSurfaceX(),cam.GetViewSurfaceZ());
  if(max(AngleX,AngleY)<=0)
    return false;
	cam.SetFrustum((int)ViewingSize,(int)ViewingSize,max(0.001f,max(AngleX,AngleY)*2.f),Height-8000.f,Height+1000.f);
	r_drawnearfov->Set(-1);
	ScreenShotHighRes(pStitchedImage,nRenderFlags, cam, dwDrawFlags, nFilterFlags,SliceCount,fTransitionSize);
	r_drawnearfov->Set(drawnearfov_backup);

	return true;
#else		// LINUX
	return false;
#endif	// LINUX
}


bool C3DEngine::ScreenShotPanorama(CStitchedImage* pStitchedImage,const int nRenderFlags, const CCamera &_cam, const int dwDrawFlags, const int nFilterFlags,uint32 SliceCount,f32 fTransitionSize)
{
#if !defined(LINUX)

	//GetRenderer()->Update();

	float r_drawnearfov_backup=-1;
	ICVar *r_drawnearfov = GetConsole()->GetCVar("r_DrawNearFoV");		assert(r_drawnearfov);

	r_drawnearfov_backup = r_drawnearfov->GetFVal();
	r_drawnearfov->Set(-1);		// means the fov override should be switched off

	GetTimer()->EnableTimer(false);

	uint32 * pImage = new uint32[GetRenderer()->GetWidth()*GetRenderer()->GetHeight()];

	for(int iSlice=SliceCount-1;iSlice>=0;--iSlice)
	{
		if(iSlice==0)												// the last one should do eye adaption
			GetTimer()->EnableTimer(true);

		GetRenderer()->BeginFrame();

		Matrix33 rot; rot.SetIdentity();

		float fAngle = pStitchedImage->GetSliceAngle(iSlice);

		rot.SetRotationZ(fAngle);

		CCamera cam = _cam;

		Matrix34 tm = cam.GetMatrix();
		tm = tm * rot;
		tm.SetTranslation(_cam.GetPosition());
		cam.SetMatrix(tm);

		cam.SetFrustum(cam.GetViewSurfaceX(),cam.GetViewSurfaceZ(),pStitchedImage->m_fPanoramaShotVertFOV,cam.GetNearPlane(),cam.GetFarPlane());

		// render scene
		RenderInternal(nRenderFlags,cam,"ScreenShotPanorama",dwDrawFlags,nFilterFlags);

		GetRenderer()->ReadFrameBufferFast(pImage, GetRenderer()->GetWidth(), GetRenderer()->GetHeight());

		GetRenderer()->EndFrame();								// show last frame (from direction)

		const bool bFadeBorders = (iSlice+1)*2<=(int)SliceCount;

		PrintMessage("PanoramaScreenShot %d/%d FadeBorders:%c (id: %d/%d)",iSlice+1,SliceCount,bFadeBorders?'t':'f',GetRenderer()->GetFrameID(false),GetRenderer()->GetFrameID(true));

		pStitchedImage->RasterizeCylinder(pImage,GetRenderer()->GetWidth(),GetRenderer()->GetHeight(),iSlice+1,bFadeBorders);

		// debug
//		m_pCurrentStitchedImage->SaveImage("PanoramaScreenShotsTest");

		if(GetCVars()->e_ScreenShotQuality<0)		// to debug FadeBorders
		if(iSlice*2==SliceCount)
		{
			pStitchedImage->Clear();
			PrintMessage("PanoramaScreenShot clear");
		}

	}
	delete [] pImage;

	r_drawnearfov->Set(r_drawnearfov_backup);

	return true;
#else		// LINUX
	return false;
#endif	// LINUX
}



void C3DEngine::SetupClearColor()
{
	FUNCTION_PROFILER_3DENGINE;

	bool bCameraInOutdoors = m_pVisAreaManager && !m_pVisAreaManager->m_pCurArea && !(m_pVisAreaManager->m_pCurPortal && m_pVisAreaManager->m_pCurPortal->m_lstConnections.Count()>1);
	GetRenderer()->SetClearColor(bCameraInOutdoors ? m_vFogColor : Vec3(0,0,0));
/*
	if(bCameraInOutdoors)
	if(GetCamera().GetPosition().z<GetWaterLevel() && m_pTerrain)
	{
		CTerrainNode * pSectorInfo = m_pTerrain ? m_pTerrain->GetSecInfo(GetCamera().GetPosition()) : 0;
		if(!pSectorInfo || !pSectorInfo->m_pFogVolume || GetCamera().GetPosition().z>pSectorInfo->m_pFogVolume->box.max.z)
		{
			if(GetCamera().GetPosition().z<GetWaterLevel() && m_pTerrain && 
				m_lstFogVolumes.Count() &&
				m_lstFogVolumes[0].bOcean)
				GetRenderer()->SetClearColor( m_lstFogVolumes[0].vColor );
		}
		//else if( pSectorInfo->m_pFogVolume->bOcean ) // makes problems if there is no skybox
			//GetRenderer()->SetClearColor( pSectorInfo->m_pFogVolume->vColor );
	}*/
}

void C3DEngine::FillDebugFPSInfo(SDebugFPSInfo& info) 
{
	size_t c = 0;
	float average = 0.0f, min = 0.0f, max = 0.0f;
	const float clampFPS = 200.0f;
	for (size_t i = 0, end = arrFPSforSaveLevelStats.size(); i < end; ++i) {
		if(arrFPSforSaveLevelStats[i] > 1.0f && arrFPSforSaveLevelStats[i] < clampFPS) {
			++c;
			average += arrFPSforSaveLevelStats[i];
			//if (arrFPSforSaveLevelStats[i] < min)
			//	min = arrFPSforSaveLevelStats[i];
			//if (arrFPSforSaveLevelStats[i] > max)
			//	max = arrFPSforSaveLevelStats[i];
		}
	}

	if (c)
		average /= (float)c;

	int minc = 0, maxc = 0;
	for (size_t i = 0, end = arrFPSforSaveLevelStats.size(); i < end; ++i) {
		if(arrFPSforSaveLevelStats[i] > average && arrFPSforSaveLevelStats[i] < clampFPS) {
			++maxc;
			max += arrFPSforSaveLevelStats[i];
		}

		if(arrFPSforSaveLevelStats[i] < average && arrFPSforSaveLevelStats[i] < clampFPS) {
			++minc;
			min += arrFPSforSaveLevelStats[i];
		}
	}

	if (minc == 0)
		minc = 1;
	if (maxc == 0)
		maxc = 1;

	info.fAverageFPS = average;
	info.fMinFPS = min/(float)minc;
	info.fMaxFPS = max/(float)maxc;
}
