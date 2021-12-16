////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terrain_det_tex.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: terrain detail textures
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

//#include "VoxMan.h"

/*
#include "terrain.h"
#include "terrain_sector.h"
#include "3dEngine.h"
#include "VoxMan.h"
#include "IndexedMesh.h"
#include "MeshCompiler/MeshCompiler.h"
#include "VoxTerrain.h"

CVoxDataNode::CVoxDataNode(short nMatId) 
{ 
  ZeroStruct(*this);
  m_nMatId = nMatId;
}

void CVoxDataNode::ReleaseChilds()
{
  for(int nChildId=0; nChildId<8; nChildId++)
    SAFE_DELETE(m_arrChilds[nChildId]);
}

CVoxDataNode::~CVoxDataNode()
{
  ReleaseChilds();
}

Vec3 ColorB_To_Vec3(ColorB color)
{
  Vec3 vCol;
  vCol.x = 1.f/255.f*color.r;
  vCol.y = 1.f/255.f*color.g;
  vCol.z = 1.f/255.f*color.b;
  return vCol;
}

ColorB Vec3_To_ColorB(Vec3 vec)
{
  vec.CheckMin(Vec3(1,1,1));
  ColorB col = vec*255;
  return col;
}

void CVoxDataNode::UpdateStateFromChilds(int nMatId, EVoxelEditOperation eOperation)
{
  FUNCTION_PROFILER_3DENGINE;

  m_fIsoVal = 0;

  for(int nChildId=0; nChildId<8; nChildId++)
  {
    if(m_arrChilds[nChildId])
      m_fIsoVal += m_arrChilds[nChildId]->m_fIsoVal;
  }

  m_fIsoVal /= 8.f;

  assert(m_fIsoVal>=0.f && m_fIsoVal<=1.f);

  m_nMatId = nMatId;

  if(m_fIsoVal == 0)
  {
    ReleaseChilds();
  }
}

void CVoxDataNode::Submit(Sphere sp, EVoxelBrushShape eShape, const AABB & nodeBox)
{
  bool bOverlap = false;

  if(eShape == evbsSphere)
  {
    bOverlap = Overlap::Sphere_AABB(sp,nodeBox);
  }
  else if(eShape == evbsBox)
  {
    Vec3 r(sp.radius,sp.radius,sp.radius);
    AABB brushBox(sp.center-r, sp.center+r);
    bOverlap = Overlap::AABB_AABB(brushBox,nodeBox);
  }

  if(!bOverlap)
    return;

  m_fIsoValPrev = m_fIsoVal;

  for(int nChildId=0; nChildId<8; nChildId++)
    if(m_arrChilds[nChildId])
      m_arrChilds[nChildId]->Submit(sp, eShape, GetChildBBox(nodeBox, nChildId));
}

void CVoxDataNode::DoVoxelShape(Sphere sp, EVoxelBrushShape eShape, EVoxelEditOperation eOperation, int nMatId, 
                                const AABB & nodeBox, int nDepth, int nMaxDepth)
{
  FUNCTION_PROFILER_3DENGINE;

  nDepth++;

  if(nDepth < nMaxDepth)
  {
    for(int nChildId=0; nChildId<8; nChildId++)
    {
      AABB aabb = GetChildBBox(nodeBox, nChildId);

      bool bOverlap = false;

      if(eOperation == eveoCopyTerrain && aabb.GetSize().z < 64.f)
      {
        sp = Sphere(aabb.GetCenter(), 2);
        sp.center.z = 2;
      }

      if(eShape == evbsSphere)
      {
        bOverlap = Overlap::Sphere_AABB(sp,aabb);
      }
      else if(eShape == evbsBox)
      {
        float r = sp.radius;
        AABB brushBox(sp.center-Vec3(r,r,r), sp.center+Vec3(r,r,r));
        bOverlap = Overlap::AABB_AABB(brushBox,aabb);
      }

      if(bOverlap)
      {
        if(!m_arrChilds[nChildId] && (eOperation==eveoCreate || eOperation==eveoCopyTerrain))
        {
          m_arrChilds[nChildId] = new CVoxDataNode(nMatId);
        }

        if(m_arrChilds[nChildId])
          m_arrChilds[nChildId]->DoVoxelShape(sp, eShape, eOperation, nMatId, aabb, nDepth, nMaxDepth);
      }
    }

    UpdateStateFromChilds(nMatId, eOperation);
  }
  else
  {
    float t = 1.f;

    if(eShape == evbsSphere) 
      t = sqrt(SATURATE(2.f - 2.f * nodeBox.GetCenter().GetDistance(sp.center) / sp.radius));

    if(eOperation == eveoCreate)
    {
      m_fIsoVal += t;
      if(t > 0.25f)
        m_nMatId = nMatId;
    }
    else if(eOperation == eveoSubstract)
    {
      m_fIsoVal -= t;
    }
    else if(eOperation == eveoMaterial || eOperation == eveoBaseColor)
    {
      if(t > 0.25f)
        m_nMatId = nMatId;
    }
    else if(eOperation == eveoCopyTerrain)
    {
      m_fIsoVal = 1.f;
    }
    else if(eOperation == eveoBlur)
    {
      Vec3 vPos = nodeBox.GetCenter();
      float fNewVal = 0;
      short nMatId = 0;
      float fCount = 0;

      for(int x=-1; x<=1; x++)
      {
        for(int y=-1; y<=1; y++)
        {
          for(int z=-1; z<=1; z++)
          {
            float fNodeSize = 2.f;
            fNewVal += m_pVoxTerrain->m_pRootData->GetVoxelValueF(vPos + Vec3(x,y,z)*fNodeSize, fNodeSize, &nMatId, true, m_pVoxTerrain->m_pRootMesh->GetNodeAABB());
            fCount += 1.f;
          }
        }
      }

      fNewVal = fNewVal / fCount;

      float t2 = t*t;

      m_fIsoVal = min(m_fIsoVal, t2*fNewVal + (1.f-t2)*m_fIsoVal);
    }

    m_fIsoVal = SATURATE(m_fIsoVal);
  }
}

AABB CVoxDataNode::GetChildBBox(const AABB & parentBox, int nChildId)
{
  int x = (nChildId/4);
  int y = (nChildId-x*4)/2;
  int z = (nChildId-x*4-y*2);
  Vec3 vSize = parentBox.GetSize()*0.5f;
  Vec3 vOffset = vSize;
  vOffset.x *= x;
  vOffset.y *= y;
  vOffset.z *= z;
  AABB childBox;
  childBox.min = parentBox.min + vOffset;
  childBox.max = childBox.min + vSize;
  return childBox;
}

void CVoxDataNode::RenderDebug(const AABB & nodeBox)
{
  if(!GetCamera().IsAABBVisible_E(nodeBox))
    return;

  FUNCTION_PROFILER_3DENGINE;

  float fDistance = sqrtf(Distance::Point_AABBSq(GetCamera().GetPosition(), nodeBox));

  for(int nChildId=0; nChildId<8; nChildId++)
  {
    if(m_arrChilds[nChildId])
      m_arrChilds[nChildId]->RenderDebug(GetChildBBox(nodeBox, nChildId));
  }

  short nMatId=0;
  AABB aabb = nodeBox;
  aabb.min -= Vec3(1,1,1);
  aabb.max -= Vec3(1,1,1);
  DrawBBox(aabb, (m_fIsoVal>.5) ? Col_White : Col_Green);
}

float CVoxDataNode::GetVoxelValueF(Vec3 vPos, float fGridSize, short * pMatId, bool bPrev, const AABB & nodeBox)
{
  if(pMatId) 
    *pMatId = -1;

  const float & fIsoVal = bPrev ? m_fIsoValPrev : m_fIsoVal;

  if(fIsoVal == 0.f)
    return 0;

  if(!HasChilds())
  {
    if(fIsoVal>0)
      if(pMatId) 
        *pMatId = m_nMatId;

    return fIsoVal;
  }
  else
  {
    Vec3 vCenter = nodeBox.GetCenter();

    int nFirst = 
      ((vPos.x > vCenter.x) ? 4 : 0) |
      ((vPos.y > vCenter.y) ? 2 : 0) |
      ((vPos.z > vCenter.z) ? 1 : 0);

    if(m_arrChilds[nFirst])
      return m_arrChilds[nFirst]->GetVoxelValueF(vPos, fGridSize, pMatId, bPrev, GetChildBBox(nodeBox, nFirst));
  }

  return 0;
}

int CVoxDataNode::GetTreeDepth(Vec3 vPos, int nDepth, const AABB & nodeBox)
{
  nDepth++;

  if(!HasChilds())
  {
    return nDepth;
  }
  else
  {
    Vec3 vCenter = nodeBox.GetCenter();

    int nFirst = 
      ((vPos.x > vCenter.x) ? 4 : 0) |
      ((vPos.y > vCenter.y) ? 2 : 0) |
      ((vPos.z > vCenter.z) ? 1 : 0);

    if(m_arrChilds[nFirst])
      return m_arrChilds[nFirst]->GetTreeDepth(vPos, nDepth, GetChildBBox(nodeBox, nFirst));
  }

  return 0;
}


bool IsAABBInsideHull(SPlaneObject * pHullPlanes, int nPlanesNum, const AABB & aabbBox);
bool IsSphereInsideHull(SPlaneObject * pHullPlanes, int nPlanesNum, const Sphere & objSphere);

ILINE char AABB_AABB_Inside_Ex( const AABB& aabb1, const AABB& aabb2 ) {
  if ( Overlap::AABB_AABB(aabb1,aabb2) ) {
    if  (aabb1.min.x<aabb2.min.x) return 1;
    if  (aabb1.min.y<aabb2.min.y) return 1; 
    if 	(aabb1.min.z<aabb2.min.z) return 1; 
    if  (aabb1.max.x>aabb2.max.x) return 1;
    if  (aabb1.max.y>aabb2.max.y) return 1;
    if  (aabb1.max.z>aabb2.max.z) return 1;
    //yes, its inside 
    return 2;
  }
  return 0;
}

CVoxDataNode * CVoxDataNode::GetMinNodeContainingAABB(const AABB & box, const AABB & nodeBox)
{
  if(AABB_AABB_Inside_Ex(box, nodeBox) == 2)
  {
    for(int nChildId=0; nChildId<8; nChildId++)
      if(m_arrChilds[nChildId])
        if(CVoxDataNode * pNode = m_arrChilds[nChildId]->GetMinNodeContainingAABB(box, GetChildBBox(nodeBox,nChildId)))
          return pNode;

    return this;
  }

  return NULL;
}

bool CVoxDataNode::HasChilds()
{ 
  const int tmp[8] = {0,0,0,0,0,0,0,0}; 
  return memcmp(m_arrChilds, tmp, sizeof(tmp)) != 0;
}
*/