////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   Vegetation.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: Loading trees, buildings, ragister/unregister entities for rendering
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "terrain.h"
#include "StatObj.h"
#include "ObjMan.h" 
#include "CullBuffer.h"
#include "3dEngine.h"
#include "Vegetation.h"

//volatile int g_lockVegetationPhysics = 0;

float CVegetation::g_scBoxDecomprTable[256] _ALIGN(128);

// g_scBoxDecomprTable heps on consoles (no difference on PC)
void CVegetation::InitVegDecomprTable()
{
	const float cDecompFactor = (VEGETATION_CONV_FACTOR / 255.f);
	for(int i=0; i<256; ++i)
		g_scBoxDecomprTable[i] = (float)i * cDecompFactor;
}

CVegetation::CVegetation() 
{
	Init();

	GetInstCount(eERType_Vegetation)++;
}

void CVegetation::Init()
{
	m_nObjectTypeID=0;
	m_vPos.Set(0,0,0);
	m_ucScale=0;
	m_pPhysEnt=0;
	m_ucAngle = 0;
	m_ucSunDotTerrain = 255;
	m_pRNTmpData = NULL;
  m_ucInSpritesList = false;

  m_HMAIndex = 0;
  m_alignNormal[0]=m_alignNormal[1]=127;

  // assert(sizeof(CVegetation) == 64 || sizeof(m_pRNTmpData) != 4);
}

//////////////////////////////////////////////////////////////////////////
void CVegetation::CalcMatrix( Matrix34A &tm, int * pFObjFlags )
{
  FUNCTION_PROFILER_3DENGINE;

	Matrix34A matTmp;
	int orFlags = 0;

	matTmp.SetIdentity();

	if (float fAngle = GetZAngle())
	{
		matTmp.SetRotationZ( fAngle );
		orFlags |= FOB_TRANS_ROTATE;
	}

	StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID];

	if(vegetGroup.bAlignToTerrain)
	{
    Vec3 vTerrainNormal;

    if(!GetCVars()->e_VoxTer || m_bEditor)
    {
			//All paths from GetTerrainSurfaceNormal() return a normalized vector
		  Vec3 vTempTerrainNormal = GetTerrain()->GetTerrainSurfaceNormal(m_vPos, 0.5f);
      vTempTerrainNormal = LERP(Vec3(0,0,1.f), vTempTerrainNormal, GetCVars()->e_VegetationAlignToTerrainAmount);
      vTerrainNormal = vTempTerrainNormal.GetNormalizedFloat();
      m_alignNormal[0] = (byte)SATURATEB((0.5f + 0.5f*vTerrainNormal.x)*255.f);
      m_alignNormal[1] = (byte)SATURATEB((0.5f + 0.5f*vTerrainNormal.y)*255.f);
    }
    else
    {
      vTerrainNormal.x = ((float)m_alignNormal[0] - 127.f) / 127.f;
      vTerrainNormal.y = ((float)m_alignNormal[1] - 127.f) / 127.f;
      vTerrainNormal.z = sqrtf(1.f - vTerrainNormal.x*vTerrainNormal.x + vTerrainNormal.y*vTerrainNormal.y);
    }

		Vec3 vDir = Vec3(-1,0,0).Cross(vTerrainNormal);
		
		Matrix33 m33;

		m33 = m33.CreateOrientation(vDir, -vTerrainNormal, 0);

		matTmp = m33 * matTmp;

		orFlags |= FOB_TRANS_ROTATE;
	}

  float fScale = GetScale();
	if (fScale != 1.0f)
	{
		Matrix33 m33; 
		m33.SetIdentity();
		m33.SetScale(Vec3(fScale,fScale,fScale));
		matTmp = m33*matTmp;
		orFlags |= FOB_TRANS_SCALE;
	}

	matTmp.SetTranslation( m_vPos );

	if(pFObjFlags)
	{
		*pFObjFlags |= (FOB_TRANS_TRANSLATE|orFlags);
	}

	tm = matTmp;
}

//////////////////////////////////////////////////////////////////////////
AABB CVegetation::CalcBBox()
{
  AABB WSBBox = GetStatObj()->GetAABB();
  WSBBox.min.z = 0;

	if (m_ucAngle)
	{
		Matrix34A tm;
		CalcMatrix( tm );
		WSBBox.SetTransformedAABB( tm, WSBBox );
	}
	else
	{
    float fScale = GetScale();
		WSBBox.min = m_vPos + WSBBox.min*fScale;
		WSBBox.max = m_vPos + WSBBox.max*fScale;
	}

  SetBBox(WSBBox);

  return WSBBox;
}

void CVegetation::CalcTerrainAdaption()
{
	FUNCTION_PROFILER_3DENGINE;

  assert(m_pRNTmpData);

  AABB WSBBox = GetBBox();

	const	f32 minX		=	WSBBox.min.x;
	const	f32 minY		=	WSBBox.min.y;
	const	f32 maxX		=	WSBBox.max.x;
	const	f32 maxY		=	WSBBox.max.y;
	f32	HeightMax	=	m_vPos.z;
	f32	HeightMin	=	m_vPos.z;
	float H0,H1,H2,H3;
	H0=H1=H2=H3=m_vPos.z;
  if(minX>=0.f && maxX<CTerrain::GetTerrainSize() && (!GetCVars()->e_VoxTer || m_bEditor))
	{
    StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID];
		H0	=	Get3DEngine()->GetTerrainElevation(maxX,m_vPos.y,vegetGroup.bAffectedByVoxels);
		H1	=	Get3DEngine()->GetTerrainElevation(minX,m_vPos.y,vegetGroup.bAffectedByVoxels);
	}
	if(minY>=0.f && maxY<CTerrain::GetTerrainSize() && (!GetCVars()->e_VoxTer || m_bEditor))
	{
    StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID];
    H2	=	Get3DEngine()->GetTerrainElevation(m_vPos.x,maxY,vegetGroup.bAffectedByVoxels);
		H3	=	Get3DEngine()->GetTerrainElevation(m_vPos.x,minY,vegetGroup.bAffectedByVoxels);
	}

  CRNTmpData::SRNUserData & userData = m_pRNTmpData->userData;

	HeightMax		=		max(max(H0,H1),max(H2,H3));
	HeightMin		=		min(min(H0,H1),min(H2,H3));
	m_HMARange	=		max(max(HeightMax-m_vPos.z,m_vPos.z-HeightMin),FLT_EPSILON);

	m_HMAIndex	|=0xfff;	//to round up...ceil... the integer part
	m_HMAIndex++;

	const float RANGE	=	static_cast<float>((1<<2)-1);
	int H0Idx		=		static_cast<int>((H0-m_vPos.z)/m_HMARange*RANGE+RANGE+0.5f);//0.5for rounding
	int H1Idx		=		static_cast<int>((H1-m_vPos.z)/m_HMARange*RANGE+RANGE+0.5f);
	int H2Idx		=		static_cast<int>((H2-m_vPos.z)/m_HMARange*RANGE+RANGE+0.5f);
	int H3Idx		=		static_cast<int>((H3-m_vPos.z)/m_HMARange*RANGE+RANGE+0.5f);

	m_HMAIndex	=		(m_HMAIndex&0xfffff000)|H0Idx|(H1Idx<<3)|(H2Idx<<6)|(H3Idx<<9);

//	m_WSBBox.min.z+=HeightMin-m_vPos.z;
//	m_WSBBox.max.z+=HeightMax-m_vPos.z;
}

//float arrVegetation_fSpriteSwitchState[nThreadsNum];

/*bool CVegetation::AddVegetationIntoSpritesList(float fDistance, SSectorTextureSet * pTerrainTexInfo, int8 nThreadId)
{
  StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID];

  assert(vegetGroup.m_fSpriteSwitchDistUnScaled);

  // calculate switch to sprite distance 
  float fSpriteSwitchDist = vegetGroup.m_fSpriteSwitchDistUnScaled * GetScale();

  arrVegetation_fSpriteSwitchState[nThreadId] = 0.f;

  if(vegetGroup.bUseSprites && fDistance >= fSpriteSwitchDist)
  { // just register to be rendered as sprite and return
    SVegetationSpriteInfo & si = GetObjManager()->m_arrVegetationSprites[m_nRenderStackLevel][nThreadId].AddNew();

    si.ucSpriteAlphaTestRef = 25;
    si.pTerrainTexInfoForSprite = vegetGroup.bUseTerrainColor ? pTerrainTexInfo : NULL;
    si.pVegetation = this;
    
    if(fDistance > fSpriteSwitchDist*1.3f)
      si.dwAngle = ((COctreeNode*)m_pOcNode)->m_dwSpritesAngle;
    else
    {
      const Vec3 vCamPos = GetCamera().GetPosition();
      Vec3 vD = m_vPos - vCamPos;       
      Vec3 vCenter = vegetGroup.GetStatObj()->GetCenter() * GetScale();
      Vec3 vD1 = vCenter + vD;
      si.dwAngle = ((uint32)(cry_atan2f(vD1.x, vD1.y)*(255.0f/(2*PI)))) & 0xff;
    }

    if (GetCVars()->e_BBoxes)
      DrawBBox(GetBBox(), Col_Yellow);

    arrVegetation_fSpriteSwitchState[nThreadId] = 2.f;

    return true;
  }

  return false;
}*/

/*void CVegetation::Render(const SRendParams& _EntDrawParams)
{
  //	FUNCTION_PROFILER_3DENGINE;

  // TODO: move to ocnode render
  //  if(CVegetation::AddVegetationIntoSpritesList(_EntDrawParams.fDistance, _EntDrawParams.dwFObjFlags, _EntDrawParams.pTerrainTexInfo))
  //  return true;

  float fDistance = _EntDrawParams.fDistance;

  StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID];

  float fScale = GetScale();

  // calculate switch to sprite distance 
  float fSpriteSwitchDist = vegetGroup.m_fSpriteSwitchDistUnScaled * fScale;

  //	PrintMessage("CVegetation_fSpriteSwitchState = %.2f", CVegetation_fSpriteSwitchState);

  // render object in 3d
  if(_EntDrawParams.dwFObjFlags & FOB_RENDER_INTO_SHADOWMAP)
    fDistance = min(fDistance,fSpriteSwitchDist*0.99f);

  CStatObj * pBody = vegetGroup.GetStatObj();

  if(!pBody)
    return; //false;

  Get3DEngine()->CheckCreateRNTmpData(&m_pRNTmpData, this);
  assert(m_pRNTmpData);

  CRNTmpData::SRNUserData & userData = m_pRNTmpData->userData;

  // calculate bending amount
  if(!m_nRenderStackLevel && vegetGroup.fBending && !_EntDrawParams.nTechniqueID)
  {
    const float fStiffness( 0.5f ); 

    if(GetCVars()->e_Wind && GetCVars()->e_VegetationWind) 
    {
      const float fMaxBending( 5.0f );
      const float fWindResponse( fStiffness * fMaxBending / 10.0f );

      AABB WSBBox = GetBBox();
      Vec3 vCurrWind = Get3DEngine()->GetWind(WSBBox, _EntDrawParams.m_pVisArea != 0);
      vCurrWind *= 0.5f; // scale down to half

      // bending - opt: disable at distance, based on amount of wind speed
      float fWindSpeed = clamp_tpl( ((vCurrWind.x*vCurrWind.x + vCurrWind.y * vCurrWind.y))*0.02f, 0.0f, 0.6f); 
      float fWindAtten = min( sqrtf( fSpriteSwitchDist *fSpriteSwitchDist *fSpriteSwitchDist ), 1.0f); //clamp_tpl( (1.0f - (fDistance/ (0.8f*fSpriteSwitchDist))) , 0.0f, 1.0f );

      // apply current wind
      float fFrameTime = GetTimer()->GetFrameTime();
      userData.m_vCurrentBending += fWindAtten * Vec2(vCurrWind) * (fFrameTime*fWindResponse);
      // apply restoration
      userData.m_vCurrentBending *= max(1.f - fFrameTime*fStiffness, 0.f);  

      // convert virtual bend to soft-limited real bend.		
      Vec2 vFinal = userData.m_vCurrentBending * fMaxBending / (userData.m_vCurrentBending.GetLength()+fMaxBending);
      userData.m_vFinalBending = vFinal;
    }
    else
    { 
      // original bending
      const float fMaxBending = 2.f;

      userData.m_vCurrentBending *= max(1.f - GetTimer()->GetFrameTime()*fStiffness, 0.f);
      Vec3 vWind = Get3DEngine()->m_vWindSpeed * 0.5f;           
      userData.m_vFinalBending = userData.m_vCurrentBending + Vec2(vWind);

      userData.m_vFinalBending *= vegetGroup.fBending;

      // convert virtual bend fMaxBending soft-limited real bend.
      userData.m_vFinalBending *= fMaxBending / (userData.m_vFinalBending.GetLength()+fMaxBending);

      // convert relative to size.
      userData.m_vFinalBending /= vegetGroup.fStatObjRadiusVert*fScale;
    }
    if(vegetGroup.bUseSprites)
      userData.m_vFinalBending *= (1.f - pow(fDistance/fSpriteSwitchDist, 4.f));
  }

  bool bUseTerrainColor(vegetGroup.bUseTerrainColor && GetCVars()->e_VegetationUseTerrainColor);

  SRendParams rParms;
  rParms.pMatrix = &userData.objMat;
  rParms.dwFObjFlags = userData.objMatFlags;
  rParms.nAfterWater = _EntDrawParams.nAfterWater;
  rParms.AmbientColor = _EntDrawParams.AmbientColor;
  rParms.fDistance = _EntDrawParams.fDistance;
  rParms.m_HMAData	=	userData.m_HMAIndex;
  rParms.nDLightMask = _EntDrawParams.nDLightMask;
  rParms.pRenderNode = _EntDrawParams.pRenderNode;
  rParms.nTechniqueID = _EntDrawParams.nTechniqueID;
  rParms.pInstInfo = _EntDrawParams.pInstInfo;

  if(bUseTerrainColor)
  {
    if(_EntDrawParams.pInstInfo)
    {
      AABB & aabb(_EntDrawParams.pInstInfo->aabb);
      Vec3 vTerrainNormal = GetTerrain()->GetTerrainSurfaceNormal(aabb.GetCenter(), aabb.GetRadius()/2);
      uint8 ucSunDotTerrain	 = (uint8)(CLAMP((vTerrainNormal.Dot(Get3DEngine()->GetSunDirNormalized()))*255.f, 0, 255));
      rParms.AmbientColor.a *= (1.0f/255.f*ucSunDotTerrain);
    }
    else
      rParms.AmbientColor.a *= (1.0f/255.f*m_ucSunDotTerrain);
  }
  else
    rParms.AmbientColor.a = vegetGroup.fBrightness;

  if(rParms.AmbientColor.a > 1.f)
    rParms.AmbientColor.a = 1.f;

  // set alpha blend
  if( vegetGroup.bUseAlphaBlending && !(_EntDrawParams.dwFObjFlags & FOB_RENDER_INTO_SHADOWMAP) && GetCVars()->e_VegetationAlphaBlend )
  {
    rParms.nRenderList = EFSLIST_GENERAL;
    if(vegetGroup.bUseSprites) // set fixed alpha
      rParms.fAlpha = 0.99f;
    else // fade alpha on distance
    {
      float fAlpha = min(1.f,(1.f - fDistance / m_fWSMaxViewDist)*6.f);
      rParms.fAlpha = min(0.99f,fAlpha);
    }
  }

  rParms.nMotionBlurAmount = 0;

  // Assign override material.
  rParms.pMaterial = vegetGroup.pMaterial;


  rParms.pShadowMapCasters = _EntDrawParams.pShadowMapCasters;
  rParms.dwFObjFlags |= (_EntDrawParams.dwFObjFlags & ~FOB_TRANS_MASK);

  //if it does not turn into a sprite, set the render quality to constant best
  rParms.fRenderQuality = vegetGroup.bUseSprites?min(1.f, max(1.f - fDistance/fSpriteSwitchDist,0.f)) : 1.f;
  rParms.nMaterialLayers = vegetGroup.nMaterialLayers;

  rParms.vBending = userData.m_vFinalBending;

  rParms.pTerrainTexInfo = _EntDrawParams.pTerrainTexInfo;
  rParms.nThreadId = _EntDrawParams.nThreadId;

  if(vegetGroup.bUseTerrainColor && GetCVars()->e_VegetationUseTerrainColor && _EntDrawParams.pTerrainTexInfo)
    rParms.dwFObjFlags |= FOB_BLEND_WITH_TERRAIN_COLOR;

  rParms.dwFObjFlags |= FOB_VEGETATION;

  if (rParms.pFoliage = GetFoliage())
    GetFoliage()->SetFlags(GetFoliage()->GetFlags() & ~IFoliage::FLAG_FROZEN | -(int)(rParms.nMaterialLayers&1) & IFoliage::FLAG_FROZEN);

  if(!_EntDrawParams.pInstInfo && GetCVars()->e_Dissolve && !(_EntDrawParams.dwFObjFlags & FOB_RENDER_INTO_SHADOWMAP) && !vegetGroup.bUseAlphaBlending)
  {
    rParms.ppRNTmpData = &m_pRNTmpData;

    float f3dAmmount = CLAMP(1.f-(arrVegetation_fSpriteSwitchState[_EntDrawParams.nThreadId]-1.f), 0.f, 1.f);
    rParms.nAlphaRef = max(_EntDrawParams.nAlphaRef, (byte)SATURATEB(255.f-f3dAmmount*255.f));
    if(rParms.nAlphaRef>0)
      rParms.dwFObjFlags |= FOB_DISSOLVE;
    if(f3dAmmount>0)
      pBody->Render(rParms);
  }
  else
  {
    rParms.dwFObjFlags &= ~FOB_DISSOLVE;
    pBody->Render(rParms);
  }
}*/

//////////////////////////////////////////////////////////////////////////
void CVegetation::Render(SRenderObjectModifier * pROM, int nThreadId, SSectorTextureSet * pTerrainTexInfo) const
{
  FUNCTION_PROFILER_3DENGINE;

  StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID];

  CStatObj * pStatObj = vegetGroup.GetStatObj();

  if(!pStatObj)
    return;

//  if(vegetGroup.bAffectedByVoxels && Get3DEngine()->m_pVoxTerrain)
  //  return;

  CRenderObject * & pRenderObject = m_pRNTmpData->userData.pRenderObject;	

	if( !pRenderObject ) return;

  if(!m_nRenderStackLevel && !pROM)
  {
    pRenderObject->m_DissolveRef = 0;

    const Vec3 vCamPos = GetCamera().GetPosition();

    float fEntDistance2D = cry_sqrtf(vCamPos.GetSquaredDistance2D(m_vPos))*m_fZoomFactor;

    const float fSpriteSwitchDist = vegetGroup.m_fSpriteSwitchDist;

    if(GetCVars()->e_Dissolve)
    {
      float fSwitchRange = min(fSpriteSwitchDist*0.1f, 4.f);

      if(fEntDistance2D > (fSpriteSwitchDist-fSwitchRange))
      {
        float fSpriteAmount = (fEntDistance2D-(fSpriteSwitchDist-fSwitchRange))/fSwitchRange;

        pRenderObject->m_DissolveRef = (int8)SATURATEB(fSpriteAmount*255.f);

				SVegetationSpriteInfo & si = GetObjManager()->m_arrVegetationSprites[m_nRenderStackLevel][max(nThreadId,(int)0)].AddNew();

        UpdateSpriteInfo(si, fSpriteAmount, pTerrainTexInfo);
      }
    
      pRenderObject->m_DissolveRef = max(pRenderObject->m_DissolveRef, GetObjManager()->GetDissolveRef(pRenderObject->m_fDistance - m_fWSMaxViewDist, &m_pRNTmpData->userData.dissolveTransitionState));
      if(pRenderObject->m_DissolveRef == 255)
        return;
    }

    if(fEntDistance2D > fSpriteSwitchDist*1.5f)
      return;
  
    float fRenderQuality = vegetGroup.bUseSprites ? 
      min(1.f, max(1.f - pRenderObject->m_fDistance/vegetGroup.m_fSpriteSwitchDist,0.f)):min(1.f, max(1.f - pRenderObject->m_fDistance/(m_fWSMaxViewDist*0.3333f),0.f));
    pRenderObject->m_nRenderQuality = (uint16)(fRenderQuality * 65535.0f);  
  
    if (pRenderObject->m_DissolveRef)
      pRenderObject->m_ObjFlags |= FOB_DISSOLVE;
    else
      pRenderObject->m_ObjFlags &= ~FOB_DISSOLVE;
  }

  pStatObj->RenderInternal(pRenderObject, pROM, nThreadId, m_pRNTmpData->userData.nLod);

  if(GetCVars()->e_BBoxes && !IsRenderIntoShadowmap())
    GetObjManager()->RenderObjectDebugInfo((IRenderNode*)this, pRenderObject);
}

//////////////////////////////////////////////////////////////////////////

IStatObj * CVegetation::GetEntityStatObj( unsigned int nPartId, unsigned int nSubPartId, Matrix34A * pMatrix, bool bReturnOnlyVisible ) 
{ 
	if(nPartId!=0)
		return 0;

	if(pMatrix)
	{
    if(m_pRNTmpData)
    {
      *pMatrix = m_pRNTmpData->userData.objMat;
    }
    else
    {
		  Matrix34A tm;
		  CalcMatrix( tm );
		  *pMatrix = tm;
    }
	}

  return GetObjManager()->m_lstStaticTypes[m_nObjectTypeID].GetStatObj(); 
}

float CVegetation::GetMaxViewDist()
{
	StatInstGroup &group = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID];
	CStatObj *pStatObj = (CStatObj*)(IStatObj*)group.pStatObj;
  if  (pStatObj)
	{
		if (GetMinSpecFromRenderNodeFlags(m_dwRndFlags) == CONFIG_DETAIL_SPEC)
			return max(GetCVars()->e_ViewDistMin, group.fVegRadius*GetScale()*GetCVars()->e_ViewDistRatioDetail*GetViewDistRatioNormilized());

		return max(GetCVars()->e_ViewDistMin, group.fVegRadius*GetScale()*GetCVars()->e_ViewDistRatioVegetation*GetViewDistRatioNormilized());
	}
	
	return 0;
}

void CVegetation::Physicalize( bool bInstant )
{
  FUNCTION_PROFILER_3DENGINE;

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Physics, 0, "Vegetation physicalization");

	CStatObj * pBody = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID].GetStatObj();
	if(!pBody || !pBody->IsPhysicsExist() || (!gEnv->pSystem->IsDedicated() && pBody->GetRenderTrisCount()<8))
		return;

	bool bHideability = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID].bHideability;
	bool bHideabilitySecondary = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID].bHideabilitySecondary;

	//////////////////////////////////////////////////////////////////////////
	// Not create instance if no physical geometry.
	if(!pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_DEFAULT])
		//no bHidability check for the E3 demo - make all the bushes with MAT_OBSTRUCT things soft cover 
		//if(!(pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_OBSTRUCT] && (bHideability || pBody->m_nSpines)))
		if(!(pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_OBSTRUCT]))
			if(!(pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_NO_COLLIDE]))
				return;
	//////////////////////////////////////////////////////////////////////////

  AABB WSBBox = GetBBox();

//	WriteLock lock(g_lockVegetationPhysics);
	int bNoOnDemand = 
		!GetCVars()->e_OnDemandPhysics || max(WSBBox.max.x-WSBBox.min.x, WSBBox.max.y-WSBBox.min.y)>Get3DEngine()->GetCVars()->e_OnDemandMaxSize;
	if (bNoOnDemand)
		bInstant = true;

  /*if (!bInstant)
  {
    pe_status_placeholder spc;
    if (m_pPhysEnt && m_pPhysEnt->GetStatus(&spc) && spc.pFullEntity)
      GetSystem()->GetIPhysicalWorld()->DestroyPhysicalEntity(spc.pFullEntity);

    pe_params_bbox pbb;
    pbb.BBox[0] = m_WSBBox.min;
    pbb.BBox[1] = m_WSBBox.max;
    pe_params_foreign_data pfd;
    pfd.pForeignData = (IRenderNode*)this;
    pfd.iForeignData = PHYS_FOREIGN_ID_STATIC;
		pfd.iForeignFlags = PFF_VEGETATION;

    if (!m_pPhysEnt)
      m_pPhysEnt = GetSystem()->GetIPhysicalWorld()->CreatePhysicalPlaceholder(PE_STATIC,&pbb);
    else
      m_pPhysEnt->SetParams(&pbb);
    m_pPhysEnt->SetParams(&pfd);
    return;
  }*/

//  pBody->CheckLoaded();

	if (!bInstant)
	{
		gEnv->pPhysicalWorld->RegisterBBoxInPODGrid(&WSBBox.min);
		return;
	}

  

  // create new
	//pe_status_placeholder spc;
	//int bOnDemandCallback = 0;
	pe_params_pos pp;
	Matrix34A mtx;	CalcMatrix(mtx);
  Matrix34 mtxNA = mtx;
	pp.pMtx3x4 = &mtxNA;
	float fScale = GetScale();
	//pp.pos = m_vPos;
  //pp.q.SetRotationXYZ( Ang3(0,0,GetZAngle()) );
	//pp.scale = fScale;
  if (!m_pPhysEnt)
  {
    m_pPhysEnt = GetSystem()->GetIPhysicalWorld()->CreatePhysicalEntity(PE_STATIC, (1-bNoOnDemand)*5.0f, &pp,
			(IRenderNode*)this, PHYS_FOREIGN_ID_STATIC);        
    if(!m_pPhysEnt)
      return;
  }	else
		m_pPhysEnt->SetParams(&pp,1);
	//else if (bOnDemandCallback = m_pPhysEnt->GetStatus(&spc) && !spc.pFullEntity)
  // GetSystem()->GetIPhysicalWorld()->CreatePhysicalEntity(PE_STATIC,5.0f,0,0,0,-1,m_pPhysEnt);

	pe_action_remove_all_parts remove_all;
	m_pPhysEnt->Action(&remove_all,1);

  pe_geomparams params;
	params.density = 800;
	params.flags |= geom_break_approximation;
	params.scale = fScale;

	// collidable
  if(pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_DEFAULT])
	{
		pe_params_ground_plane pgp;
		Vec3 bbox[2] = { pBody->GetBoxMin(),pBody->GetBoxMax() };
		pgp.iPlane = 0;
		pgp.ground.n.Set(0,0,1);
		(pgp.ground.origin = (bbox[0]+bbox[1])*0.5f).z -= (bbox[1].z-bbox[0].z)*0.49f;
		pgp.ground.origin *= fScale;



		m_pPhysEnt->SetParams(&pgp,1);

		if (!(m_dwRndFlags & ERF_PROCEDURAL)) 
		{
			params.idmatBreakable = pBody->m_idmatBreakable;
			if (pBody->m_bBreakableByGame)
				params.flags |= geom_manually_breakable;
		} else
			params.idmatBreakable = -1;
    m_pPhysEnt->AddGeometry(pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_DEFAULT], &params, -1, 1);
	}

	phys_geometry *pgeom;
	params.density = 5;
	params.idmatBreakable = -1;
	if(pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_NO_COLLIDE] && pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_OBSTRUCT])
	{
		params.minContactDist = GetCVars()->e_FoliageStiffness;
		params.flags = geom_squashy|geom_colltype_obstruct; 
		m_pPhysEnt->AddGeometry(pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_OBSTRUCT], &params, -1, 1);

		if (GetCVars()->e_PhysFoliage>=2) 
		{
			params.density = 0;
			params.flags = geom_log_interactions;
			params.flagsCollider = 0;
			if (pBody->m_nSpines)
				params.flags |= geom_colltype_foliage_proxy;
			m_pPhysEnt->AddGeometry(pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_NO_COLLIDE], &params, -1, 1);
		}
	}	
	else if ((pgeom=pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_NO_COLLIDE]) || (pgeom=pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_OBSTRUCT]))
	{
		params.minContactDist = GetCVars()->e_FoliageStiffness;
		params.flags = geom_log_interactions|geom_squashy;
		if (pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_OBSTRUCT])
			params.flags |= geom_colltype_obstruct;
		if (pBody->m_nSpines)
			params.flags |= geom_colltype_foliage_proxy;
		m_pPhysEnt->AddGeometry(pgeom, &params, -1, 1);
	}

  if(bHideability)
  {
    pe_params_foreign_data  foreignData;
    m_pPhysEnt->GetParams(&foreignData);
    foreignData.iForeignFlags |= PFF_HIDABLE;
    m_pPhysEnt->SetParams(&foreignData,1);
  }

	if(bHideabilitySecondary)
	{
		pe_params_foreign_data  foreignData;
		m_pPhysEnt->GetParams(&foreignData);
		foreignData.iForeignFlags |= PFF_HIDABLE_SECONDARY;
		m_pPhysEnt->SetParams(&foreignData,1);
	}

	//PhysicalizeFoliage();
}

bool CVegetation::PhysicalizeFoliage(bool bPhysicalize, int iSource)
{
	CStatObj *pBody = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID].GetStatObj();
	if(!pBody || !pBody->m_pSpines)
		return false;

	if (bPhysicalize)
	{
    Get3DEngine()->CheckCreateRNTmpData(&m_pRNTmpData, this);
    assert(m_pRNTmpData);

		Matrix34A mtx; CalcMatrix(mtx);
		if (pBody->PhysicalizeFoliage(m_pPhysEnt, mtx, m_pRNTmpData->userData.m_pFoliage, GetCVars()->e_FoliageBranchesTimeout, iSource) 
			)//&& !pBody->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_DEFAULT])
			((CStatObjFoliage*)m_pRNTmpData->userData.m_pFoliage)->m_pVegInst = this;
	} 
  else if (m_pRNTmpData && m_pRNTmpData->userData.m_pFoliage)
  {
    m_pRNTmpData->userData.m_pFoliage->Release();
    m_pRNTmpData->userData.m_pFoliage = NULL;
  }

  UpdateFoliageState();

	return m_pRNTmpData && m_pRNTmpData->userData.m_pFoliage;
}

void CVegetation::ShutDown()
{
	Dephysicalize( );
	Get3DEngine()->FreeRenderNodeState(this); // Also does unregister entity.

	if(m_pRNTmpData)
		Get3DEngine()->FreeRNTmpData(&m_pRNTmpData);
	assert(!m_pRNTmpData);
}

CVegetation::~CVegetation()
{
	ShutDown();

	GetInstCount(eERType_Vegetation)--;

  Get3DEngine()->m_lstKilledVegetations.Delete(this);
}

void CVegetation::Dematerialize( )
{
}

void CVegetation::Dephysicalize( bool bKeepIfReferenced )
{
	// delete old physics
//	WriteLock lock(g_lockVegetationPhysics);
	if(m_pPhysEnt && GetSystem()->GetIPhysicalWorld()->DestroyPhysicalEntity(m_pPhysEnt,4*(int)bKeepIfReferenced))
	{
		m_pPhysEnt = 0;
    if(m_pRNTmpData && m_pRNTmpData->userData.m_pFoliage)
    {
      m_pRNTmpData->userData.m_pFoliage->Release();
      m_pRNTmpData->userData.m_pFoliage = NULL;
    }
	}

  //UpdateFoliageState();
}

void CVegetation::GetMemoryUsage(ICrySizer * pSizer)
{
	SIZER_COMPONENT_NAME(pSizer, "Vegetation");
	pSizer->AddObject(this, sizeof(*this));
}

void CVegetation::UpdateRndFlags()
{
	const uint32 dwFlagsToUpdate = 
    ERF_CASTSHADOWMAPS | ERF_CASTSHADOWINTORAMMAP | ERF_HIDABLE | ERF_PICKABLE | ERF_SPEC_BITS_MASK | ERF_USE_TERRAIN_COLOR | ERF_INTEGRATION_TYPE_BIT_1 | ERF_INTEGRATION_TYPE_BIT_2 | ERF_AFFECTED_BY_VOXELS;
	m_dwRndFlags &= ~dwFlagsToUpdate;
	m_dwRndFlags |= GetObjManager()->m_lstStaticTypes[m_nObjectTypeID].m_dwRndFlags & dwFlagsToUpdate;
	m_dwRndFlags |= ERF_OUTDOORONLY;

  SetLodRatio((int)(GetObjManager()->m_lstStaticTypes[m_nObjectTypeID].fLodDistRatio * 100.f));
  SetViewDistRatio((int)(GetObjManager()->m_lstStaticTypes[m_nObjectTypeID].fMaxViewDistRatio * 100.f));
}

void CVegetation::UpdateSunDotTerrain()
{
  float fRadius = GetBBox().GetRadius();

	Vec3 vTerrainNormal = GetTerrain()->GetTerrainSurfaceNormal(m_vPos, fRadius);
	m_ucSunDotTerrain	 = (uint8)(CLAMP((vTerrainNormal.Dot(Get3DEngine()->GetSunDirNormalized()))*255.f, 0, 255));

	// TODO: take sky access into account
/*
	StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID];
	assert(vegetGroup.m_fSpriteSwitchDistUnScaled);
	if(vegetGroup.m_fSpriteSwitchDistUnScaled && vegetGroup.GetStatObj())
	{
		float fSpriteSwitchDist = vegetGroup.m_fSpriteSwitchDistUnScaled * GetScale();
		float fLodRatioNorm = cry_sqrtf((vegetGroup.GetStatObj()->m_nLoadedLodsNum+1)*(GetCVars()->e_LodRatio*fRadius)/fSpriteSwitchDist);
		m_ucLodRatio = (uint8)SATURATEB(fLodRatioNorm*100.f);
	}*/
}

const char *CVegetation::GetName() const
{
	return (GetObjManager()->m_lstStaticTypes.size() && GetObjManager()->m_lstStaticTypes[m_nObjectTypeID].GetStatObj() )? 
		GetObjManager()->m_lstStaticTypes[m_nObjectTypeID].GetStatObj()->GetFilePath() : "StatObjNotSet";
}
//////////////////////////////////////////////////////////////////////////
IRenderMesh *		CVegetation::GetRenderMesh(int nLod)
{
	// For now.
	if (nLod>0)
	{
		return NULL;
	}

	CStatObj*	pStatObj(GetStatObj());

	if (!pStatObj)
	{
		return NULL;
	}

	return pStatObj->GetRenderMesh();
}
//////////////////////////////////////////////////////////////////////////
IMaterial* CVegetation::GetMaterial(Vec3 * pHitPos)
{
	StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID];

	if(vegetGroup.pMaterial)
		return vegetGroup.pMaterial;

	if (CStatObj *pBody = vegetGroup.GetStatObj())
		return pBody->GetMaterial();

	return NULL;
}

IMaterial* CVegetation::GetMaterialOverride()
{
	StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID];

	if(vegetGroup.pMaterial)
		return vegetGroup.pMaterial;

	return NULL;
}

/*
float CVegetation::GetLodForDistance(float fDistance)
{
	StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID];

	assert(vegetGroup.m_fSpriteSwitchDist);

	float fSpriteSwitchDist = vegetGroup.m_fSpriteSwitchDist;

	CStatObj * pBody = vegetGroup.GetStatObj();

//	if(fDistance > fSpriteSwitchDist)
//		if(((CStatObj*)pBody->GetLodObject(pBody->m_nLoadedLodsNum-1))->GetRenderTrisCount()>8)
	//		return -1;

	float fLod = (fDistance/fSpriteSwitchDist)*pBody->m_nLoadedLodsNum;

	if(fLod > pBody->m_nLoadedLodsNum-1)
		fLod = (float)(pBody->m_nLoadedLodsNum-1);

	return fLod;
}
*/
float CVegetation::GetZAngle() const 
{ 
	if(m_ucAngle)
	{
		return DEG2RAD( 360.0f/255.0f*m_ucAngle ); 
	}

	return 0;
}

void CVegetation::OnRenderNodeBecomeVisible()
{
  assert(m_pRNTmpData);
  CRNTmpData::SRNUserData & userData = m_pRNTmpData->userData;

  userData.objMatFlags = 0;
  Matrix34A mtx;
  CalcMatrix( mtx, &(userData.objMatFlags) );
  userData.objMat = mtx;
  userData.fRadius = GetBBox().GetRadius();
  
  if(!GetCVars()->e_VoxTer || m_bEditor)
    CalcTerrainAdaption();

  // init pRenderObject

  CRenderObject * & pObj = userData.pRenderObject;

  assert(!pObj);

  pObj = GetRenderer()->EF_GetObject(false);

  pObj->m_pRenderNode = this;
  SRenderObjData *pOD = NULL;
  if (m_HMAIndex)
  {
    pOD = GetRenderer()->EF_GetObjData(pObj, true);
    pOD->m_HMAData	=	m_HMAIndex;
  }
  pObj->m_II.m_Matrix = mtx;  
  pObj->m_nMotionBlurAmount = 0;
  pObj->m_II.m_AmbColor = Get3DEngine()->GetSkyColor();
  pObj->m_fAlpha = 1.f;
  pObj->m_ObjFlags |= FOB_INSHADOW | FOB_VEGETATION | FOB_TRANS_MASK;

  if( !userData.objMat.m01 && !userData.objMat.m02 && !userData.objMat.m10 && !userData.objMat.m12 && !userData.objMat.m20 && !userData.objMat.m21 )
    pObj->m_ObjFlags &= ~FOB_TRANS_ROTATE;
  else
    pObj->m_ObjFlags |= FOB_TRANS_ROTATE;

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  // Process bending
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /*
  if(GetCVars()->e_VegetationBending && pObj->m_ObjFlags & FOB_VEGETATION )
  {
  float fBendAmount = fabs_tpl(rParams.vBending.x + rParams.vBending.y) + fabs_tpl(pObj->m_fBendScale);

  // Disable bending/detail bending with barely any movement..
  if( fBendAmount >= 0.00005f)
  SetupBending(pObj, rParams.vBending, fRadiusVert, fRadiusHors);
  }
  else
  {
  pObj->m_vBending = rParams.vBending;
  }
  */

//    if( rParams.nVisionParams && !(rParams.dwFObjFlags & FOB_RENDER_INTO_SHADOWMAP) && !(pObj->m_ObjFlags & FOB_VEGETATION))
//    pObj->m_nVisionParams = rParams.nVisionParams;

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  // Set render quality
  ////////////////////////////////////////////////////////////////////////////////////////////////////

  const Vec3 vCamPos = GetCamera().GetPosition();
  float fEntDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos,GetBBox()))*m_fZoomFactor;

  pObj->m_nSort = fastround_positive(fEntDistance*2.0f);

  StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID];

  pObj->m_nMaterialLayers = vegetGroup.nMaterialLayers;

  bool bUseTerrainColor((vegetGroup.bUseTerrainColor && GetCVars()->e_VegetationUseTerrainColor) || GetCVars()->e_VegetationUseTerrainColor==2);
  if (bUseTerrainColor)
    pObj->m_ObjFlags |= FOB_BLEND_WITH_TERRAIN_COLOR;
  else
    pObj->m_ObjFlags &= ~FOB_BLEND_WITH_TERRAIN_COLOR;

  if (GetTerrain()->IsAmbientOcclusionEnabled())
    pObj->m_ObjFlags |= FOB_AMBIENT_OCCLUSION;

  if( uint8 nMaterialLayers = GetMaterialLayers() ) 
  {
    uint8 nFrozenLayer = ( nMaterialLayers&(1<<0) )? 255: 0;
    uint8 nWetLayer = ( nMaterialLayers&(1<<1) )? 255: 0;
    pObj->m_nMaterialLayers = (uint32) (nFrozenLayer<< 24) | (nWetLayer<<16);
  }
  else
    pObj->m_nMaterialLayers = 0;

  if (vegetGroup.pMaterial)
    pObj->m_pCurrMaterial = vegetGroup.pMaterial;
  else
  if (vegetGroup.GetStatObj())
    pObj->m_pCurrMaterial = vegetGroup.GetStatObj()->GetMaterial();

  if(bUseTerrainColor)
    pObj->m_II.m_AmbColor.a = (1.0f/255.f*m_ucSunDotTerrain);
  else
    pObj->m_II.m_AmbColor.a = vegetGroup.fBrightness;

  if(pObj->m_II.m_AmbColor.a > 1.f)
    pObj->m_II.m_AmbColor.a = 1.f;

  UpdateBending();

  float fEntDistance2D = cry_sqrtf(vCamPos.GetSquaredDistance2D(m_vPos))*m_fZoomFactor;
  UpdateRenderQuality(fEntDistance2D);
  UpdateSunDotTerrain();

  userData.nLod = CObjManager::GetObjectLOD(fEntDistance, GetLodRatioNormalized(), userData.fRadius);
}

void CVegetation::UpdateRenderQuality(float fDistance)
{
  if(!m_pRNTmpData)
    return;

  if(CRenderObject * pObj = m_pRNTmpData->userData.pRenderObject)
  {
    StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID];
    float fRenderQuality = vegetGroup.bUseSprites ? 
      min(1.f, max(1.f - fDistance/vegetGroup.m_fSpriteSwitchDist,0.f)) : 
		  min(1.f, max(1.f - fDistance/(m_fWSMaxViewDist*0.3333f),0.f));
    
			pObj->m_nRenderQuality = (uint16)(fRenderQuality * 65535.0f);
  }
}

void CVegetation::UpdateFoliageState()
{
  if(!m_pRNTmpData)
    return;

  if(CRenderObject * pObj = m_pRNTmpData->userData.pRenderObject)
  {
    IFoliage * pFoliage = GetFoliage();

    if(pFoliage && ((CStatObjFoliage*)pFoliage)->m_pVegInst == this)
    {
      SRenderObjData *pOD = GetRenderer()->EF_GetObjData(pObj, true);
      pOD->m_pCharInstance = pFoliage;
      pObj->m_ObjFlags |= FOB_CHARACTER|FOB_VEGETATION;
      pFoliage->SetFlags(pFoliage->GetFlags() & ~IFoliage::FLAG_FROZEN | -(int)(pObj->m_nMaterialLayers&1) & IFoliage::FLAG_FROZEN);
    }
    else
    {
      SRenderObjData *pOD = GetRenderer()->EF_GetObjData(pObj, false);
      pObj->m_ObjFlags &= ~(FOB_CHARACTER);
      if (pOD)
        pOD->m_pCharInstance = NULL;
    }
  }
}

void CVegetation::UpdateBending()
{
  CRNTmpData::SRNUserData & userData = m_pRNTmpData->userData;

  CRenderObject * & pObj = userData.pRenderObject;

  StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID];

  // calculate bending amount
  if (vegetGroup.fBending)
  {
    const float fStiffness( 0.5f ); 

    float fSpriteSwitchDist = vegetGroup.m_fSpriteSwitchDist;

    float fFrameTime = 1.f;//0.03f;//GetTimer()->GetFrameTime();

    if(GetCVars()->e_Wind && GetCVars()->e_VegetationWind) 
    {
      const float fMaxBending( 5.0f );
      const float fWindResponse( fStiffness * fMaxBending / 10.0f );

      AABB WSBBox = GetBBox();
      Vec3 vCurrWind = Get3DEngine()->m_vWindSpeed* 0.5f; // Get3DEngine()->GetWind(WSBBox, pObj->m_pVisArea != 0);
      vCurrWind *= 0.5f; // scale down to half

      // bending - opt: disable at distance, based on amount of wind speed
      float fWindSpeed = clamp_tpl( ((vCurrWind.x*vCurrWind.x + vCurrWind.y * vCurrWind.y))*0.02f, 0.0f, 0.6f); 
      float fWindAtten = min( sqrtf( fSpriteSwitchDist *fSpriteSwitchDist *fSpriteSwitchDist ), 1.0f); //clamp_tpl( (1.0f - (fDistance/ (0.8f*fSpriteSwitchDist))) , 0.0f, 1.0f );

      // apply current wind
      userData.m_vCurrentBending += fWindAtten * Vec2(vCurrWind) * (fFrameTime*fWindResponse);
      // apply restoration
//      userData.m_vCurrentBending *= max(1.f - fFrameTime*fStiffness, 0.f);  

      // convert virtual bend to soft-limited real bend.		
      Vec2 vFinal = userData.m_vCurrentBending * fMaxBending / (userData.m_vCurrentBending.GetLength()+fMaxBending);
      userData.m_Bending.m_vBending = vFinal;//*fScale; //(vFinal-userData.m_vFinalBending) * 10.0f * GetTimer()->GetFrameTime();
    }
    else
    { 
      // original bending
      const float fMaxBending = 2.f;

      userData.m_vCurrentBending *= max(1.f - fFrameTime*fStiffness, 0.f);
      Vec3 vWind = Get3DEngine()->m_vWindSpeed * 0.5f;           
      userData.m_Bending.m_vBending = userData.m_vCurrentBending + Vec2(vWind);

      userData.m_Bending.m_vBending *= vegetGroup.fBending;

      // convert virtual bend fMaxBending soft-limited real bend.
      userData.m_Bending.m_vBending *= fMaxBending / (userData.m_Bending.m_vBending.GetLength()+fMaxBending);

      // convert relative to size.
      userData.m_Bending.m_vBending /= vegetGroup.fVegRadiusVert*GetScale();
    }
    //    if(vegetGroup.bUseSprites)
    //    userData.m_vFinalBending *= (1.f - pow(fDistance/fSpriteSwitchDist, 4.f));
  }

  if (GetCVars()->e_VegetationBending)
  {
    float fBendAmount = fabs_tpl(userData.m_Bending.m_vBending.x + userData.m_Bending.m_vBending.y) + fabs_tpl(userData.m_Bending.m_fBendScale);

    // Disable bending/detail bending with barely any movement..
    if( fBendAmount >= 0.00005f)
      if(CStatObj * pBody = vegetGroup.GetStatObj())
      {
        pObj->m_pBending = &userData.m_Bending;
        pBody->SetupBending(pObj, userData.m_Bending.m_vBending, pBody->GetRadiusVert(), pBody->GetRadiusHors());
      }
  }
  else
  {
    pObj->m_pBending = NULL;
  }
}

void CVegetation::AddBending(Vec3 const& v) 
{ 
  if(m_pRNTmpData)
    m_pRNTmpData->userData.m_vCurrentBending += Vec2(v); 
}

const AABB CVegetation::GetBBox() const
{ 
  AABB aabb;
  FillBBoxFromExtends(aabb, m_boxExtends, m_vPos); 
  return aabb;
}

void CVegetation::FillBBox(AABB & aabb)
{
  FillBBoxFromExtends(aabb, m_boxExtends, m_vPos); 
}

const float CVegetation::GetRadius() const
{
  const float *const __restrict cpDecompTable = g_scBoxDecomprTable;
  return cpDecompTable[m_ucRadius];
}

void CVegetation::SetBBox( const AABB& WSBBox )
{ 
	SetBoxExtends(m_boxExtends, &m_ucRadius, WSBBox, m_vPos);
}

void CVegetation::UpdateSpriteInfo(SVegetationSpriteInfo & si, float fSpriteAmount, SSectorTextureSet * pTerrainTexInfo) const
{
  const Vec3 vCamPos = GetCamera().GetPosition();
  StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[m_nObjectTypeID];

  if(GetCVars()->e_Dissolve)
  {
    const float nMin = 25;
    const float nMax = 170;
    si.ucAlphaTestRef = SATURATEB((int)((1.f-fSpriteAmount)*nMax + fSpriteAmount*nMin));
  }
  else
    si.ucAlphaTestRef = 25;

  si.pTerrainTexInfo = vegetGroup.bUseTerrainColor ? pTerrainTexInfo : NULL;
  si.pVegetation = this;
  si.fMaxViewDistSq = m_fWSMaxViewDist*m_fWSMaxViewDist;

  Vec3 vCenter = vegetGroup.GetStatObj()->GetVegCenter() * GetScale();
  Vec3 vD1 = m_vPos + vCenter - vCamPos;

  int dwAngle = ((uint32)(cry_atan2f(vD1.x, vD1.y)*(255.0f/(2*g_PI)))) & 0xff;
  si.ucSlotId = (((dwAngle+FAR_TEX_HAL_ANGLE)*FAR_TEX_COUNT)/256)%FAR_TEX_COUNT;
  assert(si.ucSlotId<FAR_TEX_COUNT);

  si.sp = Sphere(GetPos()+vCenter, GetScale()*vegetGroup.fVegRadius);

  si.ucSunDotTerrain = m_ucSunDotTerrain;
  si.pLightInfo = &vegetGroup.m_arrSSpriteLightInfo[si.ucSlotId];

  si.pStatInstGroup = &vegetGroup;
  si.fScaleV = GetScale()*si.pStatInstGroup->fVegRadiusVert;
  si.fScaleH = GetScale()*si.pStatInstGroup->fVegRadiusHor;

  if(vegetGroup.bUseTerrainColor)
    si.fAmbientValue = 1.f;
  else
    si.fAmbientValue = max(vegetGroup.fBrightness, 1.f);
};

IFoliage *CVegetation::GetFoliage() 
{
  if(m_pRNTmpData)
    return m_pRNTmpData->userData.m_pFoliage;

  return 0;
}

IPhysicalEntity* CVegetation::GetBranchPhys(int idx) 
{ 
  IFoliage * pFoliage = GetFoliage();
  return pFoliage && (unsigned int)idx < (unsigned int)((CStatObjFoliage*)pFoliage)->m_nRopes ? ((CStatObjFoliage*)pFoliage)->m_pRopes[idx] : 0; 
}
