/*=============================================================================
  NULL_RendPipeline.cpp : NULL device specific implementation using shaders pipeline.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

    Revision history:
  		* Created by Honich Andrey
    
=============================================================================*/

#include "StdAfx.h"
#include "NULL_Renderer.h"

//============================================================================================
// Init Shaders rendering

void CNULLRenderer::EF_Init()
{
  bool nv = 0;

  m_RP.m_MaxVerts = 600;
  m_RP.m_MaxTris = 300;

//==================================================

	if (!m_RP.m_TempObjects[0].Num())
		m_RP.m_TempObjects[0].Reserve(MAX_REND_OBJECTS);
	if (!m_RP.m_TempObjects[1].Num())
		m_RP.m_TempObjects[1].Reserve(MAX_REND_OBJECTS);
  if (!m_RP.m_Objects.Num())
  {
    m_RP.m_Objects.Reserve(MAX_REND_OBJECTS);
    m_RP.m_Objects.SetUse(1);
		CRenderObject::m_pPermObjData = (SRenderObjData *)CryModuleMemalign(sizeof(SRenderObjData)*MAX_PERM_OBJECTS, 16);
		memset(CRenderObject::m_pPermObjData, 0, sizeof(SRenderObjData)*MAX_PERM_OBJECTS);
		CRenderObject::m_sFreePermObjData.Alloc(MAX_PERM_OBJECTS);
    SAFE_DELETE_ARRAY(m_RP.m_ObjectsPool);
    m_RP.m_nNumObjectsInPool = 384;
    m_RP.m_ObjectsPool = new CRenderObject[m_RP.m_nNumObjectsInPool*2];
		for (uint32 j=0; j<2; j++)
		{
			for (uint32 i=0; i<m_RP.m_nNumObjectsInPool; i++)
			{
				m_RP.m_TempObjects[j][i] = &m_RP.m_ObjectsPool[j*m_RP.m_nNumObjectsInPool+i];
				m_RP.m_TempObjects[j][i]->Init(0);
				m_RP.m_TempObjects[j][i]->m_II.m_AmbColor = Col_White;
				m_RP.m_TempObjects[j][i]->m_ObjFlags = 0;
				m_RP.m_TempObjects[j][i]->m_II.m_Matrix.SetIdentity();
				m_RP.m_TempObjects[j][i]->m_RState = 0;
			}
		}
    m_RP.m_Objects[0] = &m_RP.m_ObjectsPool[0];
  }
}

void CNULLRenderer::FX_SetClipPlane (bool bEnable, float *pPlane, bool bRefract)
{
}

void CNULLRenderer::EF_PipelineShutdown()
{
  uint32 i, j;

  for (int n=0; n<2; n++)
  {
    for (j=0; j<2; j++)
    {
      for (i=0; i<CREClientPoly::m_PolysStorage[n][j].Num(); i++)
      {
        CREClientPoly::m_PolysStorage[n][j][i]->Release(false);
      }
      CREClientPoly::m_PolysStorage[n][j].Free();
    }
  }
}

void CNULLRenderer::EF_Release(int nFlags)
{
}

//==========================================================================

void CNULLRenderer::EF_SetState(int st, int AlphaRef, int RestoreState)
{
  m_RP.m_CurState = st;
  m_RP.m_CurAlphaRef = AlphaRef;
}
void CRenderer::EF_SetStencilState(int st, uint32 nStencRef, uint32 nStencMask, uint32 nStencWriteMask)
{
}

//=================================================================================

// Initialize of the new shader pipeline (only 2d)
void CRenderer::FX_Start(CShader *ef, int nTech, SRenderShaderResources *Res, CRendElementBase *re)
{
  m_RP.m_Frame++;
}

void CRenderer::FX_CheckOverflow(int nVerts, int nInds, CRendElementBase *re, int* nNewVerts, int* nNewInds)
{
}

uint32 CRenderer::EF_GetDeferredLightsNum(const eDeferredLightType eLightType)
{
  return 0;
}

int CRenderer::EF_AddDeferredLight( const CDLight &pLight, float )
{
  return 0;
}

void CRenderer::EF_ClearDeferredLightsList()
{

}

void CRenderer::EF_AddDeferredVisArea( const IVisArea *pVisArea )
{

}

void CRenderer::EF_ClearDeferredVisAreasList()
{

}

//========================================================================================

void CNULLRenderer::EF_EndEf3D(int nFlags, int nPrecacheUpdateId)
{
	//m_RP.m_TI[m_RP.m_nFillThreadID].m_RealTime = iTimer->GetCurrTime();
  EF_RemovePolysFromScene();

	int nThreadID = m_pRT->GetThreadList();
	SRendItem::m_RecurseLevel[nThreadID]--;
}

//double timeFtoI, timeFtoL, timeQRound;
//int sSome;
void CNULLRenderer::EF_EndEf2D(bool bSort)
{
}


