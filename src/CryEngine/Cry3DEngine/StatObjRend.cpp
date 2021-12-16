////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   statobjrend.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: prepare and add render element into renderer
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "StatObj.h"
#include "../RenderDll/Common/Shadow_Renderer.h"

#include "IndexedMesh.h"
#include "VisAreas.h"
#include "GeomQuery.h"

void CStatObj::SetupBending(CRenderObject * pObj, Vec2 const& vBending, float fRadiusVert, float fRadiusHors)
{
//  FUNCTION_PROFILER_3DENGINE;

	static float fBEND_AMPLITUDE = 0.002f;
	float fWaveAmp = fBEND_AMPLITUDE;

	const Vec3 &vObjPos = pObj->GetTranslation();
	assert(pObj->m_pBending);
  SBending *pBending = pObj->m_pBending;
  pBending->m_vBending = vBending;
	if (GetCVars()->e_VegetationWind)
	{
    float vrad = fRadiusVert;
    float hrad = fRadiusHors;
    float fBBoxRatio = vrad / hrad;
    fBBoxRatio = clamp_tpl<float>(fBBoxRatio, 0.5f, 1);

		pObj->m_pBending->m_fBendScale = fBBoxRatio * pObj->GetScaleX()  / (vrad * pObj->GetScaleZ());  
		fWaveAmp *= 1.f + 4.f * pBending->m_vBending.GetLength();
	}
	else
		// Flag for shader to use previous bending param scheme.
		pBending->m_fBendScale = 0.f;

//	SWaveForm2 *pWF[2];
	//GetRenderer()->EF_ObjAddWaves(pObj, pWF);
//	SWaveForm2 *wf = pWF[0];

  pBending->m_Waves[0].m_Level  = 0.000f; // between 0.001 and 0.1
	pBending->m_Waves[0].m_Freq   = 1.0f/fRadiusVert/8.0f+0.2f; // between 0.001 and 0.1
	pBending->m_Waves[0].m_Phase  = vObjPos.x/8.0f;
	pBending->m_Waves[0].m_Amp    = fWaveAmp;
  pBending->m_Waves[0].m_eWFType = eWF_Sin;

	pBending->m_Waves[1].m_Level  = 0.000f; // between 0.001 and 0.1
	pBending->m_Waves[1].m_Freq   = 1.0f/fRadiusVert/7.0f+0.2f; // between 0.001 and 0.1
	pBending->m_Waves[1].m_Phase  = vObjPos.y/8.0f;
	pBending->m_Waves[1].m_Amp    = fWaveAmp;
  pBending->m_Waves[1].m_eWFType = eWF_Sin;

  pObj->m_ObjFlags |= FOB_BENDED | FOB_VEGETATION;  
}

void CStatObj::Render(const SRendParams & rParams)
{
  FUNCTION_PROFILER_3DENGINE;

  if (m_nFlags & STATIC_OBJECT_HIDDEN)
    return;

	if(m_pIndexedMesh && m_pIndexedMesh->m_bInvalidated)
	{
		MakeRenderMesh();
		m_pIndexedMesh->m_bInvalidated = false;
	}

  CRenderObject * pObj = GetRenderer()->EF_GetObject(true);
  SRenderObjectModifier rom;
  FillRenderObject(rParams, rParams.pRenderNode, m_pMaterial, m_fRadiusVert, m_fRadiusHors, NULL, pObj, &rom);

  Matrix34 m34;

  if(rom.nMatricesInUse) 
    m34 = rom.mat;
  else
    m34 = pObj->m_II.m_Matrix;

  int nNewLod = GetLod(m34, pObj->m_fDistance, rParams.pRenderNode);

  return RenderInternal(pObj, rom.InUse() ? &rom : NULL, -1, nNewLod);
}

void CStatObj::RenderSubObject(CRenderObject * pRenderObject, const SRenderObjectModifier * pROM, int nThreadId, int nLod, 
															 int nSubObjId, const Matrix34A & renderTM, const Matrix34A & renderPrevTM)
{
  const SSubObject& subObj = m_subObjects[nSubObjId];

  CStatObj * const __restrict pStatObj = (CStatObj *)subObj.pStatObj;

	if( pStatObj == NULL ) 
		return;

  if (subObj.bIdentityMatrix && !subObj.pWeights)
  {
    pStatObj->RenderSubObjectInternal(pRenderObject, pROM, nThreadId, nLod );
  }
  else
  {
		const Matrix34 mat = renderTM * subObj.tm;
		const Matrix34 prevMat = (pRenderObject->m_nMotionBlurAmount)?static_cast<const Matrix34>(renderPrevTM * subObj.tm):mat;
		const SRenderObjectModifier instInfo(pROM, mat, prevMat, subObj.pWeights);
    pStatObj->RenderSubObjectInternal(pRenderObject, &instInfo, nThreadId, nLod );
  }
}

//////////////////////////////////////////////////////////////////////
void CStatObj::RenderInternal(CRenderObject * pRenderObject, const SRenderObjectModifier * pROM, int nThreadId, int nLod)
{
  FUNCTION_PROFILER_3DENGINE;

	if (m_nFlags & STATIC_OBJECT_HIDDEN)
		return;

  m_nLastDrawMainFrameId = GetMainFrameID();
  if(m_pParentObject)
    m_pParentObject->m_nLastDrawMainFrameId = GetMainFrameID();

	if (pRenderObject->m_pRenderNode)
	{
		IRenderNode *pRN = (IRenderNode *)pRenderObject->m_pRenderNode;
		if(m_bEditor)
		{
			if(pRN->m_dwRndFlags & ERF_SELECTED)
				pRenderObject->m_ObjFlags |= FOB_SELECTED;
			else
				pRenderObject->m_ObjFlags &= ~FOB_SELECTED;  

			if (gEnv->bEditorGameMode && pRN->m_dwRndFlags & ERF_RAYCAST_PROXY)
			{
				return;
			}
		}
		else
		{
			if (pRN->m_dwRndFlags & ERF_RAYCAST_PROXY)
			{
				return;
			}
		}
  }

  if( !m_nRenderStackLevel )
  {
    pRenderObject->m_ObjFlags &= ~FOB_SHADERLOD0;
    if( pRenderObject->m_fDistance > GetCVars()->e_LodDistShader)
      pRenderObject->m_ObjFlags |= FOB_SHADERLOD0;
  }

  if (GetCVars()->e_StreamCgfDebug==1 && m_bUseStreaming)
  {    
    int nKB = GetStreamableContentMemoryUsage() >> 10;
    if( nKB > GetCVars()->e_StreamCgfDebugMinObjSize )
    {
      char * pComment = 0;
      if(!m_bLodsAreLoadedFromSeparateFile && m_nLoadedLodsNum) pComment = "Single";
      else if(m_nLoadedLodsNum>1) pComment = "Split";
      else pComment = "no LODs";

      int nDiff = SATURATEB(int(float(nKB - GetCVars()->e_StreamCgfDebugMinObjSize)/GetCVars()->e_StreamCgfDebugMinObjSize*255));
      DrawBBoxLabeled( AABB(m_vBoxMin,m_vBoxMax), pRenderObject->m_II.m_Matrix, ColorB(nDiff, 255-nDiff, 0, 255),
        "%.2f mb, %s", 1.f/1024.f*(float)nKB, pComment );
    }
  }

	if (m_nFlags & STATIC_OBJECT_COMPOUND)
	{ 
    // render sub objects
		if (m_pMergedObject)
    {
			m_pMergedObject->RenderInternal( pRenderObject, pROM, nThreadId, nLod );
    }
		else
		{
			//////////////////////////////////////////////////////////////////////////
			// Render SubMeshes if present.
			//////////////////////////////////////////////////////////////////////////
			if (m_nSubObjectMeshCount > 0)
			{
				Matrix34A renderTM = pRenderObject->m_II.m_Matrix;

        Matrix34A renderPrevTM;
				if( pRenderObject->m_nMotionBlurAmount )
        {
          SRenderObjData *pOD = GetRenderer()->EF_GetObjData(pRenderObject, false);
          if (pOD)
					  renderPrevTM = pOD->m_prevMatrix;
          else
            renderPrevTM = renderTM;
        }

        // make m_pSubObjectsForRendering if not ready
        if(!m_pSubObjectsForRendering && !m_bUnmergable && (m_eStreamingStatus == ecss_Ready))
        {
          m_pSubObjectsForRendering = new PodArray<int>;

          const DynArray<SSubObject>& subObjects = m_subObjects;
          for (uint32 i = 0, subObjSize=subObjects.size(); i < subObjSize; ++i)
          {
            const SSubObject& subObj = subObjects[i];
            if (subObj.nType == STATIC_SUB_OBJECT_MESH) // all the meshes are at the beginning of the array.
						{
							CStatObj * const __restrict pStatObj = (CStatObj *)subObj.pStatObj;
							if(pStatObj && !subObj.bHidden && (pStatObj->m_nRenderTrisCount >= 3) && !(pStatObj->m_nFlags & STATIC_OBJECT_HIDDEN))
							{
								m_pSubObjectsForRendering->Add(i);
							}
						}
						else
						{
							break;
						}
          }
        }
        if(m_pSubObjectsForRendering)
        {
				  for (int32 ii = 0; ii < m_pSubObjectsForRendering->Count(); ++ii)
				  {
            RenderSubObject(pRenderObject, pROM, nThreadId, nLod, m_pSubObjectsForRendering->GetAt(ii), renderTM, renderPrevTM);
				  }
        }
        else
        {
					const DynArray<SSubObject>& subObjects = m_subObjects;
					for (int32 i = 0, subObjectsSize=subObjects.size(); i < subObjectsSize; ++i)
					{
						const SSubObject& subObj = m_subObjects[i];
						if(subObj.nType == STATIC_SUB_OBJECT_MESH) // all the meshes are at the beginning of the array.
						{
							CStatObj * const __restrict pStatObj = (CStatObj *)subObj.pStatObj;

							if(pStatObj && !subObj.bHidden && (pStatObj->m_nRenderTrisCount >= 3) && !(pStatObj->m_nFlags & STATIC_OBJECT_HIDDEN))
							{
								RenderSubObject(pRenderObject, pROM, nThreadId, nLod, i, renderTM, renderPrevTM);
							}
						}
						else
						{
							break;
						}
					}
        }
			}

			if (GetCVars()->e_DebugDraw)
			{
				RenderDebugInfo( pRenderObject, pROM );
			}
		}

		//////////////////////////////////////////////////////////////////////////
	}
  else
  { // draw mesh, don't even try to render childs

    nLod = CLAMP(nLod, GetMinUsableLod(), (int)m_nMaxUsableLod);
    assert(nLod<MAX_STATOBJ_LODS_NUM);

    // try next lod's if selected one is not ready
    if((!nLod && m_pRenderMesh) || !GetCVars()->e_Lods)
    {
      RenderRenderMesh(pRenderObject, NULL, nThreadId, pROM);
    }
    else
    {
      if(m_arrpLowLODs[nLod])
      {
        m_arrpLowLODs[nLod]->m_nLastDrawMainFrameId = GetMainFrameID();
        if(m_arrpLowLODs[nLod]->m_pParentObject)
          m_arrpLowLODs[nLod]->m_pParentObject->m_nLastDrawMainFrameId = GetMainFrameID();

        if((nLod+1) < MAX_STATOBJ_LODS_NUM && m_arrpLowLODs[nLod+1])
        {
          m_arrpLowLODs[nLod+1]->m_nLastDrawMainFrameId = GetMainFrameID();
          if(m_arrpLowLODs[nLod+1]->m_pParentObject)
            m_arrpLowLODs[nLod+1]->m_pParentObject->m_nLastDrawMainFrameId = GetMainFrameID();
        }
      }

      for(; nLod<=(int)m_nMaxUsableLod; nLod++)
      {
        if(m_arrpLowLODs[nLod] && m_arrpLowLODs[nLod]->m_pRenderMesh)
        {
          m_arrpLowLODs[nLod]->RenderRenderMesh(pRenderObject, NULL, nThreadId, pROM);
          break;
        }
      }
    }
  }
}

void CStatObj::RenderSubObjectInternal(CRenderObject * pRenderObject, const SRenderObjectModifier * pROM, int nThreadId, int nLod)
{
  assert(!(m_nFlags & STATIC_OBJECT_HIDDEN));

  assert(m_nRenderTrisCount);

  m_nLastDrawMainFrameId = GetMainFrameID();
  if(m_pParentObject && (m_nFlags&STATIC_OBJECT_MULTIPLE_PARENTS))
    m_pParentObject->m_nLastDrawMainFrameId = GetMainFrameID();

  assert(!m_pParentObject || m_pParentObject->m_nLastDrawMainFrameId == GetMainFrameID());

  assert(!(m_nFlags & STATIC_OBJECT_COMPOUND));

  nLod = CLAMP(nLod, GetMinUsableLod(), (int)m_nMaxUsableLod);
  assert(nLod<MAX_STATOBJ_LODS_NUM);

  // try next lod's if selected one is not ready
  if((!nLod && m_pRenderMesh) || !GetCVars()->e_Lods)
  {
    RenderRenderMesh(pRenderObject, NULL, nThreadId, pROM);
  }
  else
  {
    if(m_arrpLowLODs[nLod])
    {
      m_arrpLowLODs[nLod]->m_nLastDrawMainFrameId = GetMainFrameID();
      if(m_arrpLowLODs[nLod]->m_pParentObject)
        m_arrpLowLODs[nLod]->m_pParentObject->m_nLastDrawMainFrameId = GetMainFrameID();

      if((nLod+1) < MAX_STATOBJ_LODS_NUM && m_arrpLowLODs[nLod+1])
      {
        m_arrpLowLODs[nLod+1]->m_nLastDrawMainFrameId = GetMainFrameID();
        if(m_arrpLowLODs[nLod+1]->m_pParentObject)
          m_arrpLowLODs[nLod+1]->m_pParentObject->m_nLastDrawMainFrameId = GetMainFrameID();
      }
    }

    for(; nLod<=(int)m_nMaxUsableLod; nLod++)
    {
      if(m_arrpLowLODs[nLod] && m_arrpLowLODs[nLod]->m_pRenderMesh)
      {
        m_arrpLowLODs[nLod]->RenderRenderMesh(pRenderObject, NULL, nThreadId, pROM);
        break;
      }
    }
  }
}

void CStatObj::FillRenderObject(const SRendParams & rParams, IRenderNode * pRenderNode, IMaterial * pMaterial, 
  float fRadiusVert, float fRadiusHors, SInstancingInfo * pInstInfo, CRenderObject * pObj, SRenderObjectModifier * pROM)
{
  //  FUNCTION_PROFILER_3DENGINE;

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  // Specify transformation
  ////////////////////////////////////////////////////////////////////////////////////////////////////

  IRenderer * pRend = GetRenderer();

  assert(pObj);
  if (!pObj)
    return;

  pObj->m_pRenderNode = pRenderNode;
  pObj->m_fSort = rParams.fCustomSortOffset;
  SRenderObjData *pOD = NULL;
  if (rParams.pFoliage || rParams.m_HMAData || rParams.m_pVisArea || pInstInfo)
  {
    pOD = pRend->EF_GetObjData(pObj, true);
    pOD->m_pCharInstance = rParams.pFoliage;
    if (pOD->m_pCharInstance)
      pObj->m_ObjFlags |= FOB_CHARACTER;

    //if (pInstInfo)
    //  pOD->m_pInstancingInfo = &pInstInfo->arrMats;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // pass Heightmap adaption parameters
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    //pass Heightmap adaption coefficients
    pOD->m_HMAData	=	rParams.m_HMAData;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // pass pointer to VisArea for AmbientCube queries
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    pOD->m_pVisArea		=	rParams.m_pVisArea;
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  // Set flags
  ////////////////////////////////////////////////////////////////////////////////////////////////////

  pObj->m_ObjFlags |= FOB_TRANS_MASK;
  pObj->m_ObjFlags |= rParams.dwFObjFlags;
//  SRenderObjData *pObjData = NULL;

  assert(rParams.pMatrix);
  {
    pObj->m_II.m_Matrix = *rParams.pMatrix;  
    if( rParams.nMotionBlurAmount && rParams.pPrevMatrix && !pObj->m_II.m_Matrix.IsEquivalent(*rParams.pPrevMatrix, 0.001f) )
    {
      pObj->m_nMotionBlurAmount = 128;
      if (!pOD)
        pOD = pRend->EF_GetObjData(pObj, true);
      pOD->m_prevMatrix = *rParams.pPrevMatrix;
    }
    else
      pObj->m_nMotionBlurAmount = 0;
  }

	if( (uint64) *rParams.pLayerEffectParams )
	{
		if( !pOD )
			pOD = pRend->EF_GetObjData(pObj, true);
		pOD->m_pLayerEffectParams[0] = rParams.pLayerEffectParams[0];
		pOD->m_pLayerEffectParams[1] = rParams.pLayerEffectParams[1];
	}

  pObj->m_II.m_AmbColor = rParams.AmbientColor;

  if (GetCVars()->e_Shadows)
    pObj->m_pShadowCasters = rParams.pShadowMapCasters;
  else
    pObj->m_pShadowCasters=0;

  pObj->m_ObjFlags |= FOB_INSHADOW;
  pObj->m_fAlpha = rParams.fAlpha;
  pObj->m_DissolveRef = rParams.nDissolveRef;

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  // Process bending
  ////////////////////////////////////////////////////////////////////////////////////////////////////

  if(GetCVars()->e_VegetationBending && pObj->m_pBending)
  {
    float fBendAmount = fabs_tpl(rParams.vBending.x + rParams.vBending.y) + fabs_tpl(pObj->m_pBending->m_fBendScale);

    // Disable bending/detail bending with barely any movement..
    if( fBendAmount >= 0.00005f)
      SetupBending(pObj, rParams.vBending, fRadiusVert, fRadiusHors);
  }

  if( rParams.nVisionParams && !Get3DEngine()->_GetRenderIntoShadowmap() && !(pObj->m_ObjFlags & FOB_VEGETATION))
    pObj->m_nVisionParams = rParams.nVisionParams;

  pObj->m_ObjFlags |= (rParams.dwFObjFlags & FOB_SHADOW_DISSOLVE);


  ////////////////////////////////////////////////////////////////////////////////////////////////////
  // Set render quality
  ////////////////////////////////////////////////////////////////////////////////////////////////////

  pObj->m_nRenderQuality = (uint16)(rParams.fRenderQuality * 65535.0f);
  pObj->m_fDistance =	rParams.fDistance; 

  pObj->m_DynLMMask[m_nRenderThreadListID] = rParams.nDLightMask;

/*  if(rParams.m_pLMData && rParams.m_pLMData->m_pLMData && !(rParams.dwFObjFlags & FOB_RENDER_INTO_SHADOWMAP) && !nLod)
  {
    if(!pObjData)
      pObjData = pRend->EF_GetObjData(pObj, true);

    // set object lmaps and texture coordinates
    pObjData->m_pLMTCBufferO = rParams.m_pLMData->m_pLMTCBuffer;
    pObj->m_nRAEId = rParams.m_pLMData->m_pLMData->GetRAETex();
  }
  else*/
  {
    //clear, when exchange the state of pLightMapInfo to NULL, the pObj parameters must be update...
    pObj->m_nSort = fastround_positive(rParams.fDistance*2.0f);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  // Add render elements
  ////////////////////////////////////////////////////////////////////////////////////////////////////

  if (rParams.pMaterial)
    pMaterial = rParams.pMaterial;

  // prepare multi-layer stuff to render object
  if( !rParams.nMaterialLayersBlend && rParams.nMaterialLayers ) 
  {
    uint8 nFrozenLayer = ( rParams.nMaterialLayers&(1<<0) )? 255: 0;
    uint8 nWetLayer = ( rParams.nMaterialLayers&(1<<1) )? 255: 0;
    pObj->m_nMaterialLayers = (uint32) (nFrozenLayer<< 24) | (nWetLayer<<16);
  }
  else
    pObj->m_nMaterialLayers = rParams.nMaterialLayersBlend;

  if(rParams.pTerrainTexInfo && (rParams.dwFObjFlags & (FOB_BLEND_WITH_TERRAIN_COLOR | FOB_AMBIENT_OCCLUSION)))
  {
    pObj->m_nTextureID = rParams.pTerrainTexInfo->nTex0;
    //pObj->m_nTextureID1 = rParams.pTerrainTexInfo->nTex1;
    if (!pOD)
      pOD = pRend->EF_GetObjData(pObj, true);
    pOD->m_fTempVars[0] = rParams.pTerrainTexInfo->fTexOffsetX;
    pOD->m_fTempVars[1] = rParams.pTerrainTexInfo->fTexOffsetY;
    pOD->m_fTempVars[2] = rParams.pTerrainTexInfo->fTexScale;
    pOD->m_fTempVars[3] = rParams.pTerrainTexInfo->fTerrainMinZ - GetCamera().GetPosition().z;
    pOD->m_fTempVars[4] = rParams.pTerrainTexInfo->fTerrainMaxZ - GetCamera().GetPosition().z;
  }

  if( rParams.pFoliage )
  {
    pObj->m_ObjFlags |= FOB_VEGETATION;
  }

  pObj->m_pRenderNode = rParams.pRenderNode;
  pObj->m_pCurrMaterial = pMaterial;
  if (rParams.pWeights)
  {
    if (!pOD)
      pOD = pRend->EF_GetObjData(pObj, true);
    pOD->m_pWeights = rParams.pWeights;
  }
}


void CStatObj::RenderRenderMeshReal(CRenderObject * pObj, IRenderMesh * pRenderMesh, SInstancingInfo * pInstInfo, const SRenderObjectModifier * pROII)
{
//  FUNCTION_PROFILER_3DENGINE;

  if (pRenderMesh)
  {
/*    if(GetCVars()->e_DebugLights && rParams.pRenderNode)
      pRenderMesh->RenderDebugLightPass(pObj->m_II.m_Matrix,	pObj->m_DynLMMask, GetCVars()->e_DebugLights, GetCVars()->e_DynamicLightsMaxEntityLights);
    else*/
    {
      // Some bug here: pFoliage is existing but SkinnedChunks not.
      SRenderObjData *pOD = pObj->m_nObjDataId>=0 ? GetRenderer()->EF_GetObjData(pObj, false) : NULL;
      bool bSkinned = (pOD && pOD->m_pCharInstance && pRenderMesh->GetChunksSkinned() && (pObj->m_ObjFlags & FOB_VEGETATION));
      pRenderMesh->Render(pObj, bSkinned, pROII);
    }
  }
}

//////////////////////////////////////////////////////////////////////////
bool CStatObj::RenderDebugInfo( CRenderObject *pObj, const SRenderObjectModifier *pROM )
{
	if(IsRenderIntoShadowmap() || m_nRenderStackLevel)
		return false;

  int nThisLod = 0;
  if(m_pLod0)
  {
    for(int i=0; i<MAX_STATOBJ_LODS_NUM; i++)
      if(m_pLod0->m_arrpLowLODs[i] == this)
      {
        nThisLod = i;
        break;
      }
  }

  IRenderer * pRend = GetRenderer();
  IMaterial * pMaterial = pObj->m_pCurrMaterial;

	IRenderAuxGeom *pAuxGeom = GetRenderer()->GetIRenderAuxGeom();

	Matrix34 tm;
  if(pROM && pROM->nMatricesInUse)
    tm = pROM->mat;
  else
    tm = pObj->m_II.m_Matrix;

	AABB bbox(m_vBoxMin,m_vBoxMax);
	
	bool bOnlyBoxes = GetCVars()->e_DebugDraw == -1;

	if (GetCVars()->e_DebugDraw == 10)
	{
		SGeometryDebugDrawInfo dd;
		dd.tm = tm;
		if (m_pRenderMesh)
			m_pRenderMesh->DebugDraw( dd );
		return true;
	}
	if ((GetCVars()->e_DebugDraw == 11 || GetCVars()->e_DebugDraw == 12))
	{
		string shortName = PathUtil::GetFile(m_szFileName);
		float color[4] = {1,1,1,1};
		IRenderMesh *pRenderMeshOcclusion = m_pRenderMeshOcclusion;
		if (m_pLod0)
			pRenderMeshOcclusion = m_pLod0->m_pRenderMeshOcclusion;
		if (pRenderMeshOcclusion)
		{
			SGeometryDebugDrawInfo dd;
			dd.tm = tm;
			dd.bNoCull = true;
			pRenderMeshOcclusion->DebugDraw( dd );
			int nTris = pRenderMeshOcclusion->GetIndicesCount()/3;
			pRend->DrawLabelEx( tm.GetTranslation(),1.3f,color,true,true,"%s\n%d",shortName.c_str(),nTris);
		}
		if (GetCVars()->e_DebugDraw == 12)
			return true;
		else
			return false;
	}

	if (GetCVars()->e_DebugDraw == 1 || bOnlyBoxes)
	{
		if (!m_bMerged)
			pAuxGeom->DrawAABB( bbox,tm,false,ColorB(0,255,255,128),eBBD_Faceted );
		else
			pAuxGeom->DrawAABB( bbox,tm,false,ColorB(255,200,0,128),eBBD_Faceted );
	}

	/*
	// scaled bbox (frustum culling check)
	if(GetCVars()->e_DebugDraw!=8)
  if (GetCVars()->e_DebugDraw > 2 && !bOnlyBoxes)
  {
		AABB bbox(m_vBoxMin,m_vBoxMax);
		bbox.min *= 1.5f;
		bbox.max *= 1.5f;
		bbox.SetTransformedAABB(tm,bbox);
		pAuxGeom->DrawAABB( bbox,false,ColorB(255,0,255,128),eBBD_Faceted );
  }
	*/

	int e_DebugDraw = GetCVars()->e_DebugDraw;
	bool bNoText = e_DebugDraw < 0;
	if (e_DebugDraw < 0 && e_DebugDraw != 1)
		e_DebugDraw = -e_DebugDraw;

	if (m_nRenderTrisCount > 0 && !bOnlyBoxes)
	{ // cgf's name and tris num

		int _nMaxUsableLod = (m_pLod0) ? m_pLod0->m_nMaxUsableLod : m_nMaxUsableLod;
		int nRealNumLods = (m_pLod0) ? m_pLod0->m_nLoadedLodsNum : m_nLoadedLodsNum;
		int nNumLods = nRealNumLods;
		if (nNumLods > _nMaxUsableLod+1)
			nNumLods = _nMaxUsableLod+1;
		int nLod = nThisLod+1;
		if (nLod > nNumLods)
			nLod = nNumLods;

		Vec3 pos = tm.TransformPoint((m_vBoxMin+m_vBoxMax)*0.5f);
		float color[4] = {1,1,1,1};
		int nMats = m_pRenderMesh ? m_pRenderMesh->GetChunks().Count() : 0;
		int nRenderMats=0;

		if(nMats)
		{
			for(int i=0;i<nMats;++i)
			{	
				CRenderChunk &rc = m_pRenderMesh->GetChunks()[i];
				if(rc.pRE && rc.nNumIndices && rc.nNumVerts && ((rc.m_nMatFlags&MTL_FLAG_NODRAW)==0))
					++nRenderMats;
			}
		}

		switch (e_DebugDraw)
		{
		case 1:
			{
				const char *shortName = "";
				if (!m_szGeomName.empty())
					shortName = m_szGeomName.c_str();
				else
					shortName = PathUtil::GetFile(m_szFileName.c_str());
				if (nNumLods > 1)
					pRend->DrawLabelEx( pos,1.3f,color,true,true,"%s\n%d (LOD %d/%d)",shortName,m_nRenderTrisCount,nLod,nNumLods );
				else
					pRend->DrawLabelEx( pos,1.3f,color,true,true,"%s\n%d",shortName,m_nRenderTrisCount );
			}
			break;

		case 2:
			{
				//////////////////////////////////////////////////////////////////////////
				// Show colored poly count.
				//////////////////////////////////////////////////////////////////////////
				int fMult = 1;
				int nTris = m_nRenderTrisCount;
				ColorB clr = ColorB(255,255,255,255);
				if (nTris >= 20000*fMult)
					clr = ColorB(255,0,0,255);
				else if (nTris >= 10000*fMult)
					clr = ColorB(255,255,0,255);
				else if (nTris >= 5000*fMult)
					clr = ColorB(0,255,0,255);
				else if (nTris >= 2500*fMult)
					clr = ColorB(0,255,255,255);
				else if (nTris > 1250*fMult)
					clr = ColorB(0,0,255,255);
				else
					clr = ColorB(nTris/10,nTris/10,nTris/10,255);

				if (pMaterial)
					pMaterial = GetMatMan()->GetDefaultHelperMaterial();
				if (pObj)
				{
					pObj->m_II.m_AmbColor = ColorF(clr.r/155.0f,clr.g/155.0f,clr.b/155.0f,1);
					pObj->m_nMaterialLayers = 0;
				}
				
				if (!bNoText)
					pRend->DrawLabelEx( pos,1.3f,color,true,true,"%d",m_nRenderTrisCount );

				return false;
				//////////////////////////////////////////////////////////////////////////
			}
		case 3:
			{
				//////////////////////////////////////////////////////////////////////////
				// Show Lods
				//////////////////////////////////////////////////////////////////////////
				ColorB clr;
				if (nNumLods < 2)
				{
					if (m_nRenderTrisCount <= GetCVars()->e_LodMinTtris  || nRealNumLods > 1)
					{
						clr = ColorB(50,50,50,255);
					}
					else
					{
						clr = ColorB(255,0,0,255);
						float fAngle = gEnv->pTimer->GetFrameStartTime().GetPeriodicFraction(1.0f)*gf_PI2;
						clr.g = 127 + (int)(sinf(fAngle)*120);	// flashing color
					}
				}
				else
				{
					if (nLod == 1)
						clr = ColorB(255,0,0,255);
					else if (nLod == 2)
						clr = ColorB(0,255,0,255);
					else if (nLod == 3)
						clr = ColorB(0,0,255,255);
					else if (nLod == 4)
						clr = ColorB(0,255,255,255);
					else
						clr = ColorB(255,255,255,255);
				}

				if (pMaterial)
					pMaterial = GetMatMan()->GetDefaultHelperMaterial();
				if (pObj)
				{
					pObj->m_II.m_AmbColor = ColorF(clr.r/155.0f,clr.g/155.0f,clr.b/155.0f,1);
					pObj->m_nMaterialLayers = 0;
				}

        bool bRenderNodeValid(pObj && pObj->m_pRenderNode && ((int)(void*)(pObj->m_pRenderNode)>0));

        int nMaxUsableLod = min((int)m_nMaxUsableLod, GetCVars()->e_LodMax);

				if (nNumLods > 1 && !bNoText)
        {
          IRenderNode *pRN = (IRenderNode *)pObj->m_pRenderNode;
          pRend->DrawLabelEx( pos,1.3f,color,true,true,"%d/%d (%d/%.1f/%d)",
            nLod, nNumLods, 
            bRenderNodeValid ? pRN->GetLodRatio() : -1, pObj->m_fDistance, nMaxUsableLod);
        }

				return false;
				//////////////////////////////////////////////////////////////////////////
			}
		case 4:
			{
				// Show texture usage.
        if(m_pRenderMesh)
				{
					int nTexMemUsage = m_pRenderMesh->GetTextureMemoryUsage( pMaterial );
					pRend->DrawLabelEx( pos,1.3f,color,true,true,"%d",nTexMemUsage/1024 );		// in KByte
				}
			}
			break;

		case 5:
			{
				//////////////////////////////////////////////////////////////////////////
				// Show Num Render materials.
				//////////////////////////////////////////////////////////////////////////
				ColorB clr;
				if (nRenderMats == 1)
					clr = ColorB(0,0,255,255);
				else if (nRenderMats == 2)
					clr = ColorB(0,255,255,255);
				else if (nRenderMats == 3)
					clr = ColorB(0,255,0,255);
				else if (nRenderMats == 4)
					clr = ColorB(255,0,255,255);
				else if (nRenderMats == 5)
					clr = ColorB(255,255,0,255);
				else if (nRenderMats >= 6)
					clr = ColorB(255,0,0,255);
				else if (nRenderMats >= 11)
					clr = ColorB(255,255,255,255);

				if (pMaterial)
					pMaterial = GetMatMan()->GetDefaultHelperMaterial();
				if (pObj)
				{
					pObj->m_II.m_AmbColor = ColorF(clr.r/155.0f,clr.g/155.0f,clr.b/155.0f,1);
					pObj->m_nMaterialLayers = 0;
				}

				if (!bNoText)
					pRend->DrawLabelEx( pos,1.3f,color,true,true,"%d",nRenderMats );
			}
			break;
		
		case 6:
			{
				if (pMaterial)
					pMaterial = GetMatMan()->GetDefaultHelperMaterial();
				if (pObj)
				{
					pObj->m_nMaterialLayers = 0;
				}
				
				pRend->DrawLabelEx( pos,1.3f,color,true,true,"%d,%d,%d,%d",
					(int)(pObj->m_II.m_AmbColor.r*255.0f),(int)(pObj->m_II.m_AmbColor.g*255.0f),(int)(pObj->m_II.m_AmbColor.b*255.0f),(int)(pObj->m_II.m_AmbColor.a*255.0f)
					);
			}
			break;
		
		case 7:
      if(m_pRenderMesh)
			{
				int nTexMemUsage = m_pRenderMesh->GetTextureMemoryUsage( pMaterial );
				pRend->DrawLabelEx( pos,1.3f,color,true,true,"%d,%d,%d",m_nRenderTrisCount,nRenderMats,nTexMemUsage/1024 );
			}
			break;
		
		case 13:
			{
				float fOcclusion = GetOcclusionAmount();
				pRend->DrawLabelEx( pos,1.3f,color,true,true,"%.2f", fOcclusion);
			}
			break;
		
		case 16:
			{
				// Draw stats for object selected by debug gun
				if( GetRenderer()->IsDebugRenderNode((IRenderNode*)pObj->m_pRenderNode) )
				{
					const char *shortName = PathUtil::GetFile(m_szFileName.c_str());
			
					int texMemUsage = 0;
					
					if(m_pRenderMesh)
					{
						texMemUsage = m_pRenderMesh->GetTextureMemoryUsage( pMaterial );
					}

					pAuxGeom->DrawAABB( bbox,tm,false,ColorB(0,255,255,128),eBBD_Faceted );
				
					float yellow[4] = {1.f,1.f,0.f,1.f};

					const float yOffset = 165.f;
					const float xOffset = 970.f;

					if(m_pParentObject == NULL)
					{					
						pRend->Draw2dLabel(	xOffset, 40.f, 1.5f, yellow, false, "%s", shortName );
									
						pRend->Draw2dLabel(	xOffset, yOffset, 1.5f, color, false, 
															//"Mesh: %s\n"
															"LOD: %d/%d\n"
															"Num Instances: %d\n"
															"Num Tris: %d\n"
															"Tex Mem usage: %.2f kb\n"
															"Mesh Mem usage: %.2f kb\n"
															"Num Materials: %d\n"
															"Mesh Type: %s\n",
															//shortName,
															nLod, nNumLods,
															m_nUsers,
															m_nRenderTrisCount, 
															texMemUsage / 1024.f,
															m_nRenderMeshMemoryUsage / 1024.f,
															nRenderMats,
															m_pRenderMesh->GetTypeName());
					}
					else
					{
						for(int i=0; i<m_pParentObject->SubObjectCount(); i++)
						{
							//find subobject position
							if( m_pParentObject->SubObject(i).pStatObj == this )
							{
								//only render the header once
								if(i==0)
								{
									pRend->Draw2dLabel(	600.f, 40.f, 2.f, yellow, false, "Debug Gun: %s", shortName );
								}
								float y = yOffset + ((i%4)*150.f);
								float x = xOffset - (floor(i/4.f)*200.f);
					
								pRend->Draw2dLabel(	x, y, 1.5f, color, false, 
															"Sub Mesh: %s\n"
															"LOD: %d/%d\n"
															"Num Instances: %d\n"
															"Num Tris: %d\n"
															"Tex Mem usage: %.2f kb\n"
															"Mesh Mem usage: %.2f kb\n"
															"Num Materials: %d\n"
															"Mesh Type: %s\n",
															m_szGeomName.c_str() ? m_szGeomName.c_str() : "UNKNOWN",
															nLod, nNumLods,
															m_nUsers,
															m_nRenderTrisCount, 
															texMemUsage / 1024.f,
															m_nRenderMeshMemoryUsage / 1024.f,
															nRenderMats,
															m_pRenderMesh->GetTypeName());

								break;
							}
						}
					}
				}
			}
			break;

		}
	}

	if (GetCVars()->e_DebugDraw == 15 && !bOnlyBoxes)
	{
		// helpers
		for (int i = 0; i < m_subObjects.size(); i++)
		{
			SSubObject *pSubObject = &(m_subObjects[i]);
			if (pSubObject->nType == STATIC_SUB_OBJECT_MESH && pSubObject->pStatObj)
				continue;

			// make object matrix
			Matrix34 tMat = tm * pSubObject->tm;
			Vec3 pos = tMat.GetTranslation();

			// draw axes 
			//DrawMatrix(tMat);
			float s = 0.02f;
      ColorB col(0,255,255,255);
			pAuxGeom->DrawAABB( AABB( Vec3(-s,-s,-s),Vec3(s,s,s) ),tMat,false,col,eBBD_Faceted );      
      pAuxGeom->DrawLine(pos + s*tMat.GetColumn1(), col, pos + 3.f*s*tMat.GetColumn1(), col);
                  
			// text
			float color[4] = {0,1,1,1};
			pRend->DrawLabelEx( pos, 1.3f, color,true,true, "%s", pSubObject->name.c_str( ));
		}
	}
	return false;
}

void CStatObj::DrawMatrix(const Matrix34 & tMat)
{
  IRenderer * pRend = GetRenderer();

	Vec3 vTrans = tMat.GetTranslation();

	float color[] = {1,1,1,1};

	//CHANGED_BY_IVO
	pRend->SetMaterialColor(1,0,0,0);
	//pRend->Draw3dBBox( vTrans, vTrans+0.05f*Vec3(tMat.m_values[0]), true );
	//pRend->DrawLabelEx(vTrans+0.05f*Vec3(tMat.m_values[0]), 0.75f,color,false,true,"x");  
	pRend->Draw3dBBox( vTrans, vTrans+0.05f*tMat.GetColumn(0), true );
	        pRend->DrawLabelEx(vTrans+0.05f*tMat.GetColumn(0), 0.75f,color,false,true,"x");  

	pRend->SetMaterialColor(0,1,0,0);
	//pRend->Draw3dBBox( vTrans, vTrans+0.05f*Vec3(tMat.m_values[1]), true );
	//pRend->DrawLabelEx(vTrans+0.05f*Vec3(tMat.m_values[1]), 0.75f,color,false,true,"y");  
	pRend->Draw3dBBox( vTrans, vTrans+0.05f*tMat.GetColumn(1), true );
	        pRend->DrawLabelEx(vTrans+0.05f*tMat.GetColumn(1), 0.75f,color,false,true,"y");  

	pRend->SetMaterialColor(0,0,1,0);
	//pRend->Draw3dBBox( vTrans, vTrans+0.05f*Vec3(tMat.m_values[2]), true );
	//pRend->DrawLabelEx(vTrans+0.05f*Vec3(tMat.m_values[2]), 0.75f,color,false,true,"z");  
	pRend->Draw3dBBox( vTrans, vTrans+0.05f*tMat.GetColumn(2), true );
	        pRend->DrawLabelEx(vTrans+0.05f*tMat.GetColumn(2), 0.75f,color,false,true,"z");  
}

//
// StatObj functions.
//

float CStatObj::ComputeExtent(GeomQuery& geo, EGeomForm eForm)
{
	int nSubCount = GetSubObjectCount();
	geo.AllocParts(nSubCount+1);

	// Create parts for main and sub-objects.
	float fExtent = 0.f;
	if (m_pRenderMesh)
		fExtent += geo.GetPart(0).GetExtent(m_pRenderMesh, eForm);

	// Evaluate sub-objects.
	for (int i = 0; i < nSubCount; i++)
	{
		IStatObj::SSubObject* pSub = GetSubObject(i);
		if (pSub->nType==STATIC_SUB_OBJECT_MESH && pSub->pStatObj)
			fExtent += geo.GetPart(1+i).GetExtent(pSub->pStatObj, eForm);
	}
	return fExtent;
}

int CStatObj::GetMinUsableLod() 
{ 
  return clamp_tpl(GetCVars()->e_LodMin,0,(int)m_nMinUsableLod0);
}

int CStatObj::GetLod(Matrix34 & mat, float fEntDistance, IRenderNode * pObj)
{
/*
  int nNewLod = 0;

  float fScale = mat.GetColumn0().GetLength();
  bool bRenderNodeValid(pObj && ((int)(void*)(pObj)>0));
  nNewLod = CObjManager::GetObjectLOD(fEntDistance, bRenderNodeValid ? pObj->GetLodRatioNormalized() : 1.f, GetRadius()*fScale);
  int nMinLod = GetMinUsableLod();
  nNewLod += nMinLod;
  int nParentMaxUsableLod = m_pParentObject ? (int)m_pParentObject->m_nMaxUsableLod : 0;
  int nMaxUsableLod = min( max(nParentMaxUsableLod, (int)m_nMaxUsableLod), GetCVars()->e_LodMax );
  if (bRenderNodeValid && pObj->GetFoliage())
    nNewLod = 0;
  nNewLod = CLAMP(nNewLod, 0, nMaxUsableLod );

  return nNewLod;
*/

	float fScale = mat.GetColumn0().GetLengthFloat();			//Parent scale

	float fLodRatio;
	bool bFoliage;

	if(pObj)
	{
		fLodRatio = pObj->GetLodRatioNormalized();
		bFoliage = (pObj->GetFoliage() != 0); 
	}
	else
	{
		fLodRatio = 1.0f;
		bFoliage = false;
	}

	//return GetLodFromScale(fScale, bRenderNodeValid ? pObj->GetLodRatioNormalized() : 1.f, fEntDistance, bRenderNodeValid && pObj->GetFoliage());
	return GetLodFromScale(fScale, fLodRatio, fEntDistance, bFoliage);
}

int CStatObj::GetLodFromScale(float fScale, float fLodRatioNormalized, float fEntDistance, bool bFoliage)
{
	int nNewLod = 0;

	nNewLod = CObjManager::GetObjectLOD(fEntDistance, fLodRatioNormalized, GetRadius()*fScale);

	int nMinLod = GetMinUsableLod();
	nNewLod += nMinLod;
  int nParentMaxUsableLod = m_pParentObject ? (int)m_pParentObject->m_nMaxUsableLod : 0;
  int nMaxUsableLod = min( max(nParentMaxUsableLod, (int)m_nMaxUsableLod), GetCVars()->e_LodMax );
	nNewLod = bFoliage ? 0 : CLAMP(nNewLod, 0, nMaxUsableLod );

	return nNewLod;
}

//////////////////////////////////////////////////////////////////////////
void CStatObj::DebugDraw( const SGeometryDebugDrawInfo &info )
{
	int nFlags = 0;
	if (info.bNoLines)
	{
		nFlags |= BIT(1);
	}
	if (info.bNoCull)
	{
		nFlags |= BIT(0);
	}

	if (m_nFlags & STATIC_OBJECT_COMPOUND)
	{ 
		if (m_pMergedObject)
		{
			// If we have merged object then draw it
			m_pMergedObject->DebugDraw( info );
		}
		else
		{
			// Draw sub objects.
			for (int i = 0; i < m_subObjects.size(); i++)
			{
				if (!m_subObjects[i].pStatObj || m_subObjects[i].bHidden || m_subObjects[i].nType != STATIC_SUB_OBJECT_MESH)
					continue;

				SGeometryDebugDrawInfo subInfo = info;
				subInfo.tm = info.tm * m_subObjects[i].tm;
				m_subObjects[i].pStatObj->DebugDraw( subInfo );
			}
		}
	}
	else if (m_pRenderMesh)
	{
		m_pRenderMesh->DebugDraw( info );
	}
	else
	{
		// No RenderMesh in here, so probably no geometry in highest LOD, find it in lower LODs
		for(int nLod = 0; nLod<=(int)m_nMaxUsableLod; nLod++)
		{
			assert(nLod<MAX_STATOBJ_LODS_NUM);
			if(m_arrpLowLODs[nLod] && m_arrpLowLODs[nLod]->m_pRenderMesh)
			{
				m_arrpLowLODs[nLod]->m_pRenderMesh->DebugDraw( info );
				break;
			}
		}
	}
}
