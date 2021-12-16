/*=============================================================================
  CREMesh.cpp : implementation of OcLeaf Render Element.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honitch Andrey

=============================================================================*/

#include "StdAfx.h"
#include <ICryAnimation.h>

void CREMesh::mfReset()
{
}

void CREMesh::mfCenter(Vec3& Pos, CRenderObject*pObj)
{
  Vec3 Mins = m_pRenderMesh->m_vBoxMin;
  Vec3 Maxs = m_pRenderMesh->m_vBoxMax;
  Pos = (Mins + Maxs) * 0.5f;
  if (pObj)
    Pos += pObj->GetTranslation();
}

void CREMesh::mfGetBBox(Vec3& vMins, Vec3& vMaxs)
{
  vMins = m_pRenderMesh->_GetVertexContainer()->m_vBoxMin;
  vMaxs = m_pRenderMesh->_GetVertexContainer()->m_vBoxMax;
}

float CREMesh::mfDistanceToCameraSquared(Matrix34& matInst)
{
  CRenderer *rd = gRenDev;

  Vec3 Mins = m_pRenderMesh->_GetVertexContainer()->m_vBoxMin;
  Vec3 Maxs = m_pRenderMesh->_GetVertexContainer()->m_vBoxMax;
  //Vec3 Center = (Mins + Maxs) * 0.5f;
  //Center += thisObject.GetTranslation();
  //Vec3 delta = rd->GetRCamera().Orig - Center;
  AABB aabb(Mins, Maxs);
  float fDistSq = Distance::Point_AABBSq(rd->GetRCamera().Orig-matInst.GetTranslation(), aabb);
  return fDistSq;

/*	float dist = delta.GetLength();
	dist += 60.f;//TODO: remove me, i am a workaround for cxp to let the particle effect be in front of the sphere surrounding it
	return sqr(dist);
*/
  //return (delta).GetLengthSquared();
}


bool CREMesh::mfCullByClipPlane(CRenderObject *pObj)
{
  CRenderer *rd = gRenDev;

  if (rd->m_RP.m_ClipPlaneEnabled && CRenderer::CV_r_cullbyclipplanes)
  {
    Vec3 Mins = m_pRenderMesh->m_vBoxMin;
    Vec3 Maxs = m_pRenderMesh->m_vBoxMax;
    if (!(pObj->m_ObjFlags & FOB_TRANS_ROTATE))
    {
      // Non rotated bounding-box. Fast solution
      if (!(pObj->m_ObjFlags & FOB_TRANS_SCALE))
      {
        Mins += pObj->GetTranslation();
        Maxs += pObj->GetTranslation();
      }
      else
      {
        Matrix34A& m = pObj->GetMatrix();
        Mins = m.TransformPoint(Mins);
        Maxs = m.TransformPoint(Maxs);
      }
    }
    else
    { 
      // General slow solution (separatelly transform all 8 points of
      // non transformed cube and calculate axis aligned bounding box)
      Vec3 v[8];
      int i;
      Matrix34A& m = pObj->GetMatrix();
      for (i=0; i<8; i++)
      {
        if (i & 1)
          v[i].x = Mins.x;
        else
          v[i].x = Maxs.x;

        if (i & 2)
          v[i].y = Mins.y;
        else
          v[i].y = Maxs.y;

        if (i & 4)
          v[i].z = Mins.z;
        else
          v[i].z = Maxs.z;
        v[i] = m.TransformPoint(v[i]);
      }
      Mins=SetMaxBB();
      Maxs=SetMinBB();
      for (i=0; i<8; i++)
      {
        AddToBounds(v[i], Mins, Maxs);
      }
    }
    //if (CullBoxByPlane(&Mins.x, &Maxs.x, &rd->m_RP.m_CurClipPlaneCull) == 2)
    return false;
      return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////

void CREMesh::mfPrepare()
{
  CRenderer *rd = gRenDev;

  {
    //PROFILE_FRAME_TOTAL(Mesh_REPrepare_Ocleaf);

    rd->FX_CheckOverflow(0, 0, this);

    rd->m_RP.m_pRE = this;

    if (m_nPrimetiveType == R_PRIMV_TRIANGLE_STRIP)
    {
      rd->m_RP.m_FirstVertex = 0;
      rd->m_RP.m_FirstIndex = 0;
      rd->m_RP.m_RendNumIndices = m_nNumIndices;
      rd->m_RP.m_RendNumVerts = m_nNumVerts;
    }
    else
    {
      rd->m_RP.m_FirstVertex = m_nFirstVertId;
      rd->m_RP.m_FirstIndex = m_nFirstIndexId;
      rd->m_RP.m_RendNumIndices = m_nNumIndices;
      rd->m_RP.m_RendNumVerts = m_nNumVerts;
    }      
  }
}

PodArray<CRenderChunk> *CREMesh::mfGetMatInfoList()
{
  return &m_pRenderMesh->m_Chunks;
}

int CREMesh::mfGetMatId()
{
  return m_pChunk->m_nMatID;
}

CRenderChunk *CREMesh::mfGetMatInfo()
{
  return m_pChunk;
}

void CREMesh::mfPrecache(const SShaderItem& SH)
{
  CShader *pSH = (CShader *)SH.m_pShader;
  if (!pSH)
    return;
  if (!m_pRenderMesh)
    return;
  if (m_pRenderMesh->_HasVBStream(VSF_GENERAL))
    return;

  mfCheckUpdate(pSH->m_eVertexFormat, VSM_TANGENTS);
}

bool CREMesh::mfCheckUpdate(EVertexFormat eVertFormat, int Flags)
{
  //PROFILE_FRAME(Mesh_CheckUpdate);

  CRenderer *rd = gRenDev;
  bool bSucceed = true;
  CRenderMesh2 *pVContainer = m_pRenderMesh->_GetVertexContainer();
  if (m_pRenderMesh->m_nFrameRender != rd->m_nFrameSwapID)
  {
    m_pRenderMesh->m_nFrameRender = rd->m_nFrameSwapID;

    /*if (!(m_pRenderMesh->_GetFlags() & FRM_RELEASED))
    {
      AUTO_LOCK(CRenderMesh2::m_sResLock);
      m_pRenderMesh->Unlink();
      m_pRenderMesh->Link(&CRenderMesh2::m_Root);
    }*/
  }
  pVContainer = m_pRenderMesh->_GetVertexContainer();

  if (rd->m_RP.m_pShader && (rd->m_RP.m_pShader->m_Flags2 & EF2_DEFAULTVERTEXFORMAT))
    eVertFormat = pVContainer->_GetVertexFormat();
  bSucceed = m_pRenderMesh->RT_CheckUpdate(pVContainer, eVertFormat, Flags);

  if (!bSucceed || !pVContainer->_HasVBStream(VSF_GENERAL))
    return false;

  rd->m_RP.m_CurVFormat = pVContainer->_GetVertexFormat();

  return true;
}

void *CREMesh::mfGetPointer(ESrcPointer ePT, int *Stride, EParamType Type, ESrcPointer Dst, int Flags)
{
  CRenderMesh2 *pRM = m_pRenderMesh->_GetVertexContainer();
  byte *pD = NULL;

  switch(ePT) 
  {
    case eSrcPointer_Vert:
      pD = pRM->GetPosPtr(*Stride,  FSL_READ);
      break;
    case eSrcPointer_Tex:
      pD = pRM->GetUVPtr(*Stride,  FSL_READ);
      break;
    case eSrcPointer_Normal:
      pD = pRM->GetNormPtr(*Stride,  FSL_READ);
      break;
    case eSrcPointer_Binormal:
      pD = pRM->GetBinormalPtr(*Stride, FSL_READ);
      break;
    case eSrcPointer_Tangent:
      pD = pRM->GetTangentPtr(*Stride, FSL_READ);
      break;
    case eSrcPointer_Color:
      pD = pRM->GetColorPtr(*Stride, FSL_READ);
      break;
    default:
      assert(false);
      break;
  }
  return pD;
}

void CREMesh::mfGetPlane(Plane& pl)
{
  CRenderMesh2 *pRM = m_pRenderMesh->_GetVertexContainer();

  // fixme: plane orientation based on biggest bbox axis
  Vec3 pMin, pMax;
  mfGetBBox(pMin, pMax);
  Vec3 p0 = pMin;
  Vec3 p1 = Vec3(pMax.x, pMin.y, pMin.z);
  Vec3 p2 = Vec3(pMin.x, pMax.y, pMin.z);
  pl.SetPlane(p2, p0, p1);
}
