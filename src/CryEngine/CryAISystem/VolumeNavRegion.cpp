/********************************************************************
CryGame Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   VolumeNavRegion.cpp
Version:     v1.00
Description: 

-------------------------------------------------------------------------
History:
- 21:7:2004   14:15 : Created by Kirill Bulatsev
- 4 May 2009        : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#include "StdAfx.h"

#include "VolumeNavRegion.h"
#include "CTriangulator.h"
#include "CAISystem.h"
#include "AILog.h"
#include "AIObject.h"
#include "AICollision.h"

#include "IPhysics.h"
#include "ISystem.h"
#include "DebugDrawContext.h"
#include "CryFile.h"
#include "ITimer.h"
#include "I3DEngine.h"
#include "ISerialize.h"
#include "IEntitySystem.h"
#include "IConsole.h"
#include "CalculationStopper.h"
#include <algorithm>
#include <limits>

#define BAI_3D_FILE_VERSION 21

#ifdef DYNAMIC_3D_NAVIGATION
#define PASSABILITY_FLAGS AICE_ALL
#else
#define PASSABILITY_FLAGS AICE_STATIC
#endif

struct SGravityModifier
{
  IPhysicalEntity* pEntity;
  AABB aabb;
  Vec3 aabbCentre;
  float gravityStrength;

	SGravityModifier() : aabb(Vec3(0,0,0),Vec3(0,0,0)),aabbCentre(0,0,0),gravityStrength(0) {}
};

typedef std::vector<SGravityModifier> GravityModifiers;

//====================================================================
//	some generic helper functions
//====================================================================

//====================================================================
// OverlapLinesegBlockers
//====================================================================
inline bool OverlapLinesegBlockers(const Lineseg& lineseg, const NavigationBlockers& navigationBlockers, 
                                   IAISystem::ENavigationType navType)
{
  unsigned nBlockers = navigationBlockers.size();
  for (unsigned i = 0 ; i < nBlockers ; ++i)
  {
    const NavigationBlocker& blocker = navigationBlockers[i];
    if (blocker.navType != navType)
      continue;
    if (Overlap::Lineseg_Sphere(lineseg, blocker.sphere))
      return true;
  }
  return false;
}

//====================================================================
// IsPointInsideGravityModifier
//====================================================================
static bool IsPointInsideGravityModifier(const Vec3 &pt, const SGravityModifier &gm)
{
  Vec3 testPt = pt + 100.0f * (pt - gm.aabbCentre).GetNormalizedSafe();

  ray_hit rayHit;
  int result = gEnv->pPhysicalWorld->RayTraceEntity(gm.pEntity, testPt, pt - testPt, &rayHit);
  return result != 0;
}

//====================================================================
// DoesLinkIntersectGravityModifier
//====================================================================
static bool DoesLinkIntersectGravityModifier(const Vec3 &p1, const Vec3 &p2, 
                                             const GravityModifiers &gravityModifiers)
{
  unsigned num = gravityModifiers.size();
  int result;
  ray_hit rayHit;
  Vec3 linkDelta = p2 - p1;

  Vec3 linkDir = linkDelta.GetNormalizedSafe();
  AABB linkAABB(AABB::RESET);
  linkAABB.Add(p1);
  linkAABB.Add(p2);

  Vec3 pt = 0.5f * (p1 + p2);
  Vec3 gravity;
  pe_params_buoyancy junk;
  gEnv->pPhysicalWorld->CheckAreas(pt, gravity, &junk);

  for (unsigned i = 0 ; i < num ; ++i)
  {
    const SGravityModifier &gm = gravityModifiers[i];
    if (!Overlap::AABB_AABB(linkAABB, gm.aabb))
      continue;

    // check gravity dir against link
    static float criticalGravityValue = 0.0f;
    static float criticalGravityMag = 1.0f;

    float gravStrength = gravity.NormalizeSafe();
    if (gravStrength < criticalGravityMag)
      continue;

    if (linkDir.Dot(gravity) > criticalGravityValue)
      continue;

    result = gEnv->pPhysicalWorld->RayTraceEntity(gm.pEntity, p1, linkDelta, &rayHit);
    if (result)
      return true;

    static bool checkPoints = true;
    if (checkPoints)
    {
      if (IsPointInsideGravityModifier(p1, gm))
        return true;
      if (IsPointInsideGravityModifier(p2, gm))
        return true;
    }
  }
  return false;
}

//====================================================================
// DoesLinkIntersectGravityModifier
//====================================================================
static bool DoesLinkIntersectGravityModifier(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3,
                                             const GravityModifiers &gravityModifiers)
{
  return DoesLinkIntersectGravityModifier(p1, p2, gravityModifiers) || DoesLinkIntersectGravityModifier(p2, p3, gravityModifiers);
}

//====================================================================
// GetGravityModifiers
//====================================================================
void GetGravityModifiers(GravityModifiers &gravityModifiers, const AABB &aabb)
{
  //  IPhysicalEntity ** ppList = 0;
  //  int	numEntites = gEnv->pPhysicalWorld->GetEntitiesInBox(aabb.min, aabb.max, ppList, ent_areas);

  IPhysicalEntity *pArea = gEnv->pPhysicalWorld->GetNextArea(0);

  while (pArea)
  {
    pe_params_area gravityParams;
    pArea->GetParams(&gravityParams);
    if (!is_unused(gravityParams.gravity) && gravityParams.gravity.GetLengthSquared() > 1.0f)
    {
      gravityModifiers.push_back(SGravityModifier());
      SGravityModifier &gm = gravityModifiers.back();
      gm.pEntity = pArea;

      pe_params_bbox params;
      pArea->GetParams(&params);
      gm.aabb.min = params.BBox[0];
      gm.aabb.max = params.BBox[1];
      gm.aabbCentre = 0.5f * (gm.aabb.min + gm.aabb.max);

      pe_status_pos posParams;
      gm.gravityStrength = gravityParams.gravity.GetLength();

      if (gm.gravityStrength < 0.01f || gm.aabb.GetSize().GetLengthSquared() < 0.01f)
      {
        gravityModifiers.pop_back();
      }
    }

    pArea = gEnv->pPhysicalWorld->GetNextArea(pArea);
  }
}

//====================================================================
// PathSegmentWorldIntersection
//====================================================================
bool CVolumeNavRegion::PathSegmentWorldIntersection(const Vec3& _start,
                                                    const Vec3& _end,
                                                    float _radius,
                                                    bool intersectAtStart,
                                                    bool intersectAtEnd, 
                                                    EAICollisionEntities aiCollisionEntities) const
{
  Vec3 start(_start);
  Vec3 end(_end);
  float radius(_radius);

  Vec3 segDir = end - start;
  float segLen = segDir.NormalizeSafe(Vec3Constants<float>::fVec3_OneX);

  float effLen = segLen;
  if (!intersectAtEnd)
    effLen -= radius;
  if (!intersectAtStart)
    effLen -= radius;

  if (effLen < 0.0f)
  {
    // just do sphere test at the mid-point
    if (!intersectAtStart && intersectAtEnd)
      start += min(radius, segLen) * segDir;
    else if (!intersectAtEnd && intersectAtStart)
      end -= min(radius, segLen) * segDir;
    Vec3 pos = 0.5f * (start + end);
    return OverlapSphere(pos, radius, aiCollisionEntities);
  }
  else
  {
    if (!intersectAtEnd)
      end -= radius * segDir;
    if (!intersectAtStart)
      start += radius * segDir;
    return OverlapCapsule(Lineseg(start, end), radius, aiCollisionEntities);
  }
}

//====================================================================
// MultiRayWorldIntersection
// does a raycast from start to end, as well as numExtraRays from 
// points around start/end to approximate a cylinder. Returns true
// if any of these rays intersect
// numExtraRays should either be 0 or >= 3
//====================================================================
static bool MultiRayWorldIntersection(const Vec3 & start,
                                      const Vec3 & end,
                                      float radius,
                                      int numExtraRays = 0)
{
  AIAssert(gEnv->pPhysicalWorld);
  if (!gEnv->pPhysicalWorld)
    return false;

  // get normalised dir from start to end
  Vec3 lookDir = end - start;
  float len = lookDir.GetLength();
  if (len <= radius)
    return false;
  lookDir /= len;

  ray_hit hit;
  int rayResult = gEnv->pPhysicalWorld->RayWorldIntersection(
    start, end - start,
    ent_static, rwi_stop_at_pierceable,&hit,1);
  if (rayResult)
    return true;

  // if numExtraRays > 0 then it should be >= 3
  if (0 == numExtraRays)
    return false;

  if (radius <= 0.0f)
    return false;

  AIAssert(3 <= numExtraRays);
  if (3 > numExtraRays)
    return false;

  // get normalised "up" vector - arbitrary except it's normal to lookDir
  Vec3 upDir;
  if ( abs(lookDir * Vec3(1.0f, 0.0f, 0.0f)) > 
    abs(lookDir * Vec3(0.0f, 1.0f, 0.0f)) )
  {
    upDir = lookDir ^ Vec3(0.0f, 1.0f, 0.0f);
  }
  else
  {
    upDir = lookDir ^ Vec3(1.0f, 0.0f, 0.0f);
  }

  // do the extra rays to approximate a swept volume, but start/end them
  // forward/back (by 45 deg) from the original start/end point
  float newLen = len - 2.0f * radius;
  if (newLen <= 0.01f)
    return false;
  Vec3 extraRayDir = (newLen / len) * (end - start);
  for (int iRay = 0 ; iRay < numExtraRays ; ++iRay)
  {
    float angle = (gf_PI * iRay) / numExtraRays;
    Vec3 offset = radius * (Matrix33::CreateRotationAA(angle, lookDir) * upDir);
    offset += radius * lookDir;
    rayResult = gEnv->pPhysicalWorld->RayWorldIntersection(
      start, offset,
      ent_static, rwi_stop_at_pierceable,&hit,1);
    if (rayResult)
      return true;
    rayResult = gEnv->pPhysicalWorld->RayWorldIntersection(
      start + offset, extraRayDir,
      ent_static, rwi_stop_at_pierceable,&hit,1);
    if (rayResult)
      return true;
  }
  return false;
}

//====================================================================
// DoesPathIntersectWorld
//====================================================================
bool CVolumeNavRegion::DoesPathIntersectWorld(std::vector<Vec3> & path,
                                              float radius, 
                                              EAICollisionEntities aiCollisionEntities)
{
  AIAssert(gEnv->pPhysicalWorld);
  if (!gEnv->pPhysicalWorld)
    return false;

  unsigned numPts = path.size();
  if (numPts <= 1)
    return false;
  for (unsigned i = 0 ; i < numPts-1 ; ++i)
  {
    unsigned iNext = i+1;
    if (PathSegmentWorldIntersection(path[i], path[iNext], radius, false, true, aiCollisionEntities))
      return true;
  }
  return false;
}

//
//	some generic helper functions over
//
//-----------------------------------------------------------------------------------------------------
//***********************************************************************************************************************************

//====================================================================
// CVolumeNavRegion
//====================================================================
CVolumeNavRegion::CVolumeNavRegion(CGraph *pGraph):
m_pGraph(pGraph),
m_hideSpots(Vec3(30, 30, 30), 4096)
{
  AIAssert(m_pGraph);
  Clear();
}

//====================================================================
// CVolumeNavRegion
//====================================================================
CVolumeNavRegion::~CVolumeNavRegion(void)
{
  Clear();
}

//====================================================================
// Clear
//====================================================================
void CVolumeNavRegion::Clear()
{
  while(!m_volumes.empty())
  {
    unsigned nodeIndex = m_volumes.back()->m_graphNodeIndex;
    m_pGraph->Disconnect(nodeIndex, true);
    delete m_volumes.back();
    m_volumes.pop_back();
  }
  m_maxVolumeRadius = 0.0f;
  m_portals.clear();
}

//====================================================================
// IsPointInVolume
//====================================================================
inline bool CVolumeNavRegion::IsPointInVolume(const CVolume & vol, const Vec3 & pos, float extra) const
{
  float distSq = (vol.Center(m_pGraph->GetNodeManager()) - pos).GetLengthSquared();
  if (distSq > square(vol.m_radius + extra))
    return false;

  if (MultiRayWorldIntersection(vol.Center(m_pGraph->GetNodeManager()), pos, 0.0f))
    return false;

  return true;
}

//====================================================================
// GetVoxelValue
//====================================================================
inline CVolumeNavRegion::TVoxelValue CVolumeNavRegion::GetVoxelValue(const SVoxelData &voxelData, int xIdx, int yIdx, int zIdx) const
{
  return voxelData.voxels[(zIdx*voxelData.dim.y + yIdx)*voxelData.dim.x + xIdx];
}

//====================================================================
// CalcClosestPointFromVolumeToPos
//====================================================================
Vec3 CVolumeNavRegion::CalcClosestPointFromVolumeToPos(const CVolume &volume, const Vec3 &pos, float passRadius) const
{
  float hitDist;
  Lineseg lineseg(volume.Center(m_pGraph->GetNodeManager()), pos);
  if (IntersectSweptSphere(0, hitDist, lineseg, passRadius, AICE_ALL))
  {
    float linesegLen = Distance::Point_Point(lineseg.start, lineseg.end);
    float frac = hitDist / linesegLen;
    return frac * lineseg.end + (1.0f - frac) * lineseg.start;
  }
  else
  {
    return pos;
  }
}

//====================================================================
// GetEnclosing
//====================================================================
unsigned CVolumeNavRegion::GetEnclosing(const Vec3& pos, float passRadius, unsigned startIndex,
                                          float range, Vec3 * closestValid, bool returnSuspect, const char *requesterName)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

  if(m_volumes.empty())
    return NULL;

	// A range value less than zero results in get search based on m_maxVolumeRadius.
	if (range < 0.0f)
		range = 0.0f;

  if (closestValid)
    *closestValid = pos;

  static std::vector< std::pair<float, const CVolume*> > distVolumePairs;
  distVolumePairs.resize(0);

  static bool doNew = true;
  if (doNew)
  {
    // for big levels this is faster. For small levels brute force is faster...
    typedef std::vector< std::pair<float, unsigned> > TNodes;
    static TNodes nodes;
    nodes.resize(0);
    CAllNodesContainer &allNodes = m_pGraph->GetAllNodes();
    float totalDist = 2.0f * m_maxVolumeRadius + range;
    allNodes.GetAllNodesWithinRange(nodes, pos, totalDist, IAISystem::NAV_VOLUME);

    unsigned nNodes = nodes.size();
    for (unsigned iNode = 0 ; iNode < nNodes ; ++iNode)
    {
      std::pair<float, unsigned> &nodePair = nodes[iNode];
      const GraphNode *pNode = m_pGraph->GetNodeManager().GetNode(nodePair.second);
      const CVolume* pVolume = GetVolume(m_volumes, pNode->GetVolumeNavData()->nVolimeIdx);

			float distSq = nodePair.first;

			// TODO/workaround! This code is potentially costly. It is here to solve some specific
			// problems in one of the Crysis levels. Ideally this should not be called.
			if (AreAllLinksBlocked(pVolume->m_graphNodeIndex, passRadius))
				distSq += sqr(1000);
			// End workaround.

      distVolumePairs.push_back(std::pair<float, const CVolume*>(distSq, pVolume));
    }
  }
  else
  {
    unsigned numVolumes = m_volumes.size();
    // first time - do quick check - this should get it almost every time.
    for (unsigned iVolume = 0 ; iVolume < numVolumes ; ++iVolume)
    {
      const CVolume & volume = *m_volumes[iVolume];
      float radiusExtraSq = square(2.0f * volume.m_radius + range); // be generous with the volumes found
      float distSq = (volume.Center(m_pGraph->GetNodeManager()) - pos).GetLengthSquared();
			if (distSq < radiusExtraSq)
      {
        distVolumePairs.push_back(std::pair<float, const CVolume*>(distSq, &volume));
      }
    }
  }

  unsigned numDistVolumePairs = distVolumePairs.size();
  if (numDistVolumePairs == 0)
    return 0;

  // sorting on pairs is done by the first value
  std::sort(distVolumePairs.begin(), distVolumePairs.end());

  // first try to get a node considering all entities. If that fails then hope
  // that ignoring dynamic entities (e.g. frozen bodies) is ok
  EAICollisionEntities collTypes[2] = {AICE_ALL, AICE_STATIC};
  for (int iCollType = 0 ; iCollType < 2 ; ++iCollType)
  {
    EAICollisionEntities collType = collTypes[iCollType];

    for (unsigned iVolume = 0 ; iVolume < numDistVolumePairs ; ++iVolume)
    {
      const CVolume & volume = *distVolumePairs[iVolume].second;
      if (PathSegmentWorldIntersection(volume.Center(m_pGraph->GetNodeManager()), pos, passRadius, false, false, collType))
        continue;
      if (passRadius > m_pGraph->GetNodeManager().GetNode(volume.m_graphNodeIndex)->GetMaxLinkRadius(m_pGraph->GetLinkManager()))
        continue;
      if (closestValid)
        *closestValid = CalcClosestPointFromVolumeToPos(volume, pos, passRadius);
			return volume.m_graphNodeIndex;
    }
	}

  if (!returnSuspect)
    return 0;

  for (unsigned iVolume = 0 ; iVolume < numDistVolumePairs ; ++iVolume)
  {
    const CVolume & volume = *distVolumePairs[iVolume].second;
    if (MultiRayWorldIntersection(volume.Center(m_pGraph->GetNodeManager()), pos, 0.0f))
      continue;
    if (passRadius > m_pGraph->GetNodeManager().GetNode(volume.m_graphNodeIndex)->GetMaxLinkRadius(m_pGraph->GetLinkManager()))
      continue;
    if (closestValid)
      *closestValid = CalcClosestPointFromVolumeToPos(volume, pos, passRadius);
		return volume.m_graphNodeIndex;
  }

  for (unsigned iVolume = 0 ; iVolume < numDistVolumePairs ; ++iVolume)
  {
    const CVolume & volume = *distVolumePairs[iVolume].second;
    if (passRadius > m_pGraph->GetNodeManager().GetNode(volume.m_graphNodeIndex)->GetMaxLinkRadius(m_pGraph->GetLinkManager()))
      continue;
    AIWarning("Cannot find node for %s position (%5.2f, %5.2f, %5.2f) - returning closest even though no line of sight",
      requesterName, pos.x, pos.y, pos.z);
    if (closestValid)
      *closestValid = CalcClosestPointFromVolumeToPos(volume, pos, passRadius);
		return volume.m_graphNodeIndex;
  }

  AIWarning("Cannot find any node for %s position (%5.2f, %5.2f, %5.2f) - returning closest (no line of sight and radius is bad)",
    requesterName, pos.x, pos.y, pos.z);
  if (closestValid)
    *closestValid = CalcClosestPointFromVolumeToPos(*distVolumePairs[0].second, pos, passRadius);
	return distVolumePairs[0].second->m_graphNodeIndex;
}

//====================================================================
// AreAllLinksBlocked
//====================================================================
bool CVolumeNavRegion::AreAllLinksBlocked(unsigned nodeIndex, float passRadius)
{
	const GraphNode* pNode = m_pGraph->GetNodeManager().GetNode(nodeIndex);
	if (!pNode)
		return false;

	unsigned blocked = 0;
	unsigned count = 0;
	for (unsigned link = pNode->firstLinkIndex; link; link = m_pGraph->GetLinkManager().GetNextLink(link))
	{
		unsigned connectedIndex = m_pGraph->GetLinkManager().GetNextNode(link);
		GraphNode *pConnectedNode = m_pGraph->GetNodeManager().GetNode(connectedIndex);
		if (!pConnectedNode)
			continue;

		count++;

		if (m_pGraph->GetLinkManager().GetRadius(link) < passRadius)
		{
			blocked++;
			continue;
		}

		float extraLinkFactor = gAIEnv.pNavigation->GetExtraLinkCost(pNode->GetPos(), pConnectedNode->GetPos());
		if (extraLinkFactor < 0.0f)
		{
			blocked++;
			continue;
		}
	}

	return blocked == count;
}

//====================================================================
// CheckPassability
//====================================================================
bool CVolumeNavRegion::CheckPassability(const Vec3& from, const Vec3& to, float radius, const NavigationBlockers& navigationBlockers, IAISystem::tNavCapMask ) const
{
  return !PathSegmentWorldIntersection(from, to, radius, true, true, AICE_ALL);
}

//====================================================================
// GetConnectionPoint
//====================================================================
Vec3 CVolumeNavRegion::GetConnectionPoint(TVolumes &volumes, TPortals &portals, int volumeIdx1, int volumeIdx2) const
{
  if(volumeIdx1<0 || static_cast<size_t>(volumeIdx1)>=volumes.size() ||
    volumeIdx2<0 || static_cast<size_t>(volumeIdx2)>=volumes.size()	)
  {
    AIWarning("[AISystem] CVolumeNavRegion::GetConnectionPoint error  >> %d %d ", volumeIdx1, volumeIdx2);	
    return Vec3(0,0,0);
  }
  return GetVolume(volumes, volumeIdx1)->GetConnectionPoint(this, GetVolume(volumes, volumeIdx2));
}

//====================================================================
// GetVolume
//====================================================================
CVolumeNavRegion::CVolume* CVolumeNavRegion::GetVolume(TVolumes &volumes, int volumeIdx) const
{
  if(volumeIdx<0 || static_cast<size_t>(volumeIdx)>=volumes.size())
    return NULL;
  return volumes[volumeIdx];
}

//====================================================================
// GetVolumeIdx
//====================================================================
int CVolumeNavRegion::GetVolumeIdx(const TVolumes &volumes, const CVolumeNavRegion::CVolume* pVolume) const
{
  TVolumes::const_iterator itr=std::find(volumes.begin(), volumes.end(), pVolume);
  if(itr == volumes.end())
    return -1;
  return (int)(itr - volumes.begin());
}

//====================================================================
// GetPortal
//====================================================================
CVolumeNavRegion::CPortal* CVolumeNavRegion::GetPortal(TPortals &portals, int portalIdx)
{
  if(portalIdx<0 || static_cast<size_t>(portalIdx)>=portals.size())
    return NULL;
  return &portals[portalIdx];
}

//====================================================================
// GetPortal
//====================================================================
const CVolumeNavRegion::CPortal* CVolumeNavRegion::GetPortal(const TPortals &portals, int portalIdx) const
{
  if(portalIdx<0 || static_cast<size_t>(portalIdx)>=portals.size())
    return NULL;
  return &portals[portalIdx];
}

//====================================================================
// GetPortalIdx
//====================================================================
int CVolumeNavRegion::GetPortalIdx(const TPortals &portals, const CVolumeNavRegion::CPortal* pPortal) const
{
  if (portals.empty())
    return -1;
  int index = (int)(pPortal - &portals[0]);
  if (index < 0)
    return -1;
  else if (index >= (int) portals.size())
    return -1;
  else
    return index;
}

//====================================================================
// GetNavigableSpaceEntities
//====================================================================
void CVolumeNavRegion::GetNavigableSpaceEntities(std::vector<const IEntity*> &entities)
{
  entities.resize(0);

  // Need points that are definitely in valid space
  IEntityItPtr entIt = gEnv->pEntitySystem->GetEntityIterator();
  const char* navName = "NavigableSpace";
  size_t navNameLen = strlen(navName);
  if (entIt)
  {
    entIt->MoveFirst();
    while (!entIt->IsEnd())
    {
      IEntity* ent = entIt->Next();
      if (ent)
      {
        const char* name = ent->GetName();
        if (strnicmp(name, navName, navNameLen) == 0)
        {
          entities.push_back(ent);
        }
      }
    }
  }
}


//====================================================================
// GetNavigableSpacePoint
//====================================================================
bool CVolumeNavRegion::GetNavigableSpacePoint(const Vec3 &refPos, Vec3 &pos, const std::vector<const IEntity*> &entities)
{
  // use the closest NavigableSpace point
  const IEntity* ent = 0;
  float bestDistSq = std::numeric_limits<float>::max();
  unsigned nEntities = entities.size();
  for (unsigned i = 0 ; i < nEntities ; ++i)
  {
    float distSq = (entities[i]->GetPos() - refPos).GetLengthSquared();
    if (distSq < bestDistSq)
    {
      bestDistSq = distSq;
      ent = entities[i];
    }
  }
  if (!ent)
    return false;

  pos = ent->GetPos();
  return true;
}

//====================================================================
// CountHits
//====================================================================
int CountHits(IPhysicalWorld* pWorld, const Vec3& from, const Vec3& to, bool front)
{
  Vec3 begin = front ? from : to;
  Vec3 end = front ? to : from;
  static const int maxHits = 2;
  ray_hit hits[maxHits];
  Vec3 dir = (end - begin).GetNormalizedSafe();
  unsigned flags = rwi_ignore_noncolliding | rwi_ignore_back_faces;
  int count = 0;
  std::vector<IPhysicalEntity*> ignoreEnts;
  std::vector<float> ignoreEntDistances;
  while (count < 1000)
  {
    int rayresult = pWorld->RayWorldIntersection(begin, end - begin, ent_static, flags, &hits[0], maxHits,
      ignoreEnts.empty() ? 0 : &ignoreEnts[0], ignoreEnts.size());
    if (!rayresult)
      return count;
    ++count;
    if (rayresult == 2 && hits[1].dist > 0 && hits[1].dist < hits[0].dist)
      hits[0] = hits[1];
    int index = hits[0].dist < 0 ? 1 : 0;
    begin = hits[index].pt;// + dir * 0.01f;

    AIAssert(hits[index].dist >= 0.0f);
    // avoid hitting the same entity again until we've moved a significant distance
    bool foundEnt = false;
    for (unsigned i = 0 ; i < ignoreEnts.size() ; )
    {
      ignoreEntDistances[i] += hits[index].dist;
      if (ignoreEntDistances[i] > 0.01f)
      {
        ignoreEntDistances.erase(ignoreEntDistances.begin() + i);
        ignoreEnts.erase(ignoreEnts.begin() + i);
      }
      else
      {
        if (ignoreEnts[i] == hits[index].pCollider)
          foundEnt = true;
        ++i;
      }
    }
    if (!foundEnt)
    {
      ignoreEnts.push_back(hits[index].pCollider);
      ignoreEntDistances.push_back(0);
    }
  }
  return count;
}

//====================================================================
// CheckVoxel
//====================================================================
CVolumeNavRegion::ECheckVoxelResult CVolumeNavRegion::CheckVoxel(const Vec3& pos, const Vec3& definitelyValid) const
{
  static const int maxHits = 16;
  ray_hit hits[maxHits];

  // shoot rays around
  float tol = 0.01f;
  int rayresult = gEnv->pPhysicalWorld->RayWorldIntersection(pos, definitelyValid - pos, ent_static, rwi_ignore_noncolliding, &hits[0], maxHits);
  Vec3 dir = definitelyValid - pos;
  float dirLen = dir.GetLength();
  dir.NormalizeSafe();
  if (!rayresult)
    return VOXEL_VALID;
  int index = hits[0].dist < 0 ? 1 : 0;
  if(hits[index].dist <= dirLen && hits[index].n.Dot(dir) > tol)
    return VOXEL_INVALID;

  // hit a front-face first - need to count the hits
  int nFront = CountHits(gEnv->pPhysicalWorld, pos, definitelyValid, true);
  int nBack  = CountHits(gEnv->pPhysicalWorld, pos, definitelyValid, false);

  return (nFront == nBack) ? VOXEL_VALID : VOXEL_INVALID;
}

//====================================================================
// CreateVolume
//====================================================================
CVolumeNavRegion::CVolume* CVolumeNavRegion::CreateVolume(TVolumes &volumes, const Vec3& pos, float radius, float distanceToGeometry)
{
  CVolume	*pNewVolume = new CVolume();
  // add newly generated volume to the graph
  pNewVolume->m_graphNodeIndex = m_pGraph->CreateNewNode(IAISystem::NAV_VOLUME, pos);
  pNewVolume->m_radius = radius;
  pNewVolume->m_distanceToGeometry = distanceToGeometry;

  // add newly generated volume to the overall volumes list
  volumes.push_back(pNewVolume);

  // this index will need to be adjusted when all volumes are ready
  m_pGraph->GetNodeManager().GetNode(pNewVolume->m_graphNodeIndex)->GetVolumeNavData()->nVolimeIdx = volumes.size()-1;

  if (radius > m_maxVolumeRadius)
    m_maxVolumeRadius = radius;
  return pNewVolume;

}

//====================================================================
// UglyPath
//====================================================================
void CVolumeNavRegion::UglifyPath(const VectorConstNodeIndices& inPath, TPathPoints& outPath, 
                                  const Vec3& startPos, const Vec3& startDir, 
                                  const Vec3& endPos, const Vec3 & endDir)
{
  outPath.push_back(PathPointDescriptor(IAISystem::NAV_VOLUME, startPos));

  for(VectorConstNodeIndices::const_iterator itrCurNode = inPath.begin() ; itrCurNode != inPath.end() ; ++itrCurNode)
  {
    const GraphNode& curNode=*m_pGraph->GetNodeManager().GetNode(*itrCurNode);
    outPath.push_back(PathPointDescriptor(IAISystem::NAV_VOLUME, curNode.GetPos()));
    // the connections
    VectorConstNodeIndices::const_iterator itrNextNode = itrCurNode;
    ++itrNextNode;
    if (itrNextNode != inPath.end())
    {
      const GraphNode& nextNode=*m_pGraph->GetNodeManager().GetNode(*itrNextNode);
      Vec3 pt = GetConnectionPoint(m_volumes, m_portals, curNode.GetVolumeNavData()->nVolimeIdx, 
        nextNode.GetVolumeNavData()->nVolimeIdx);
      outPath.push_back(PathPointDescriptor(IAISystem::NAV_VOLUME, pt));
    }
  }
  outPath.push_back(PathPointDescriptor(IAISystem::NAV_VOLUME, endPos));
  return;
}

//====================================================================
// BeautifyPath
//====================================================================
void CVolumeNavRegion::BeautifyPath(const VectorConstNodeIndices& inPath, TPathPoints& outPath, 
                                    const Vec3& startPos, const Vec3& startDir, 
                                    const Vec3& endPos, const Vec3 & endDir, 
                                    float radius, const AgentMovementAbility & movementAbility,
                                    const NavigationBlockers& navigationBlockers)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );
  float extraR = gAIEnv.CVars.ExtraRadiusDuringBeautification;
  radius += extraR;

  outPath.clear();
  if (inPath.empty())
    return;

  outPath.push_back(PathPointDescriptor(IAISystem::NAV_VOLUME, startPos));

  GravityModifiers gravityModifiers;
  GetGravityModifiers(gravityModifiers, m_AABB);

  // for debugging
  static bool doCutting = true;
  static bool doCorners = true;

  // origPath is our first path, which then gets beautified
  std::vector<Vec3> origPath;
  origPath.push_back(outPath.back().vPos);

  VectorConstNodeIndices::const_iterator itrCurNode=inPath.begin();
  for(;itrCurNode!=inPath.end();++itrCurNode)
  {
    Vec3 &lastPos = origPath.back();
    VectorConstNodeIndices::const_iterator itrNextNode=itrCurNode;
    ++itrNextNode;
    const GraphNode& curNode=*m_pGraph->GetNodeManager().GetNode(*itrCurNode);

    Vec3 nextPoint = curNode.GetPos();

    CVolume *pCurVolume = GetVolume(m_volumes, curNode.GetVolumeNavData()->nVolimeIdx);
    if ( (itrNextNode == inPath.end()) ||
      (IsPointInVolume(*pCurVolume, endPos )) )
    {
      nextPoint = endPos;
    }
    else
    {
      const GraphNode& nextNode=*m_pGraph->GetNodeManager().GetNode(*itrNextNode);
      nextPoint = GetConnectionPoint(m_volumes, m_portals, curNode.GetVolumeNavData()->nVolimeIdx, nextNode.GetVolumeNavData()->nVolimeIdx);
    }

    if ( PathSegmentWorldIntersection(lastPos, nextPoint, radius, false, true, PASSABILITY_FLAGS) ||
      gAIEnv.pNavigation->OverlapExtraCostAreas(Lineseg(lastPos, nextPoint)) ||
      OverlapLinesegBlockers(Lineseg(lastPos, nextPoint), navigationBlockers, IAISystem::NAV_VOLUME) ||
      DoesLinkIntersectGravityModifier(lastPos, nextPoint, gravityModifiers))
      origPath.push_back(curNode.GetPos());

    if(IsPointInVolume(*pCurVolume, endPos ))
      break;
    origPath.push_back(nextPoint);
  }
  if ( (origPath.back() - endPos).GetLengthSquared() > 0.01f )
    origPath.push_back(endPos);

  // walk through the points attempting to take a shortcut.
  if (doCutting)
  {
		static int numCuts = 1;
    for (int iCut = 0 ; iCut < numCuts ; ++iCut)
    {
      std::vector<Vec3>::iterator it, itNext, itNextNext;
      for (it = origPath.begin(); it != origPath.end() ; ++it)
      {
        const Vec3 & pt = *it;
        itNext = it+1;
        if (itNext != origPath.end())
        {
          itNextNext = itNext + 1;
//          while ( origPath.end() != itNextNext )
					if ( origPath.end() != itNextNext )		// [Mikko] Cut only once.
          {
            const Vec3 & ptNext = *itNext;
            const Vec3 & ptNextNext = *itNextNext;
            if (PathSegmentWorldIntersection(pt, ptNextNext, radius, false, true, PASSABILITY_FLAGS) ||
              gAIEnv.pNavigation->OverlapExtraCostAreas(Lineseg(pt, ptNextNext)) ||
              OverlapLinesegBlockers(Lineseg(pt, ptNextNext), navigationBlockers, IAISystem::NAV_VOLUME) ||
              DoesLinkIntersectGravityModifier(pt, ptNextNext, gravityModifiers))
            {
              // OK - can't cut this corner completely, but maybe we can move the middle
              // point in to straighten things a bit
              Vec3 trialPt = 0.5f * (0.5f * (pt + ptNextNext) + ptNext);
              if (!PathSegmentWorldIntersection(pt, trialPt, radius, false, true, PASSABILITY_FLAGS) &&
                !PathSegmentWorldIntersection(trialPt, ptNextNext, radius, false, true, PASSABILITY_FLAGS) &&
                !gAIEnv.pNavigation->OverlapExtraCostAreas(Lineseg(pt, trialPt)) &&
                !gAIEnv.pNavigation->OverlapExtraCostAreas(Lineseg(trialPt, ptNextNext)) &&
                !OverlapLinesegBlockers(Lineseg(pt, trialPt), navigationBlockers, IAISystem::NAV_VOLUME) &&
                !OverlapLinesegBlockers(Lineseg(trialPt, ptNextNext), navigationBlockers, IAISystem::NAV_VOLUME) &&
                !DoesLinkIntersectGravityModifier(pt, trialPt, ptNextNext, gravityModifiers) )
              {
                // move this middle one in
                *itNext = trialPt;
              }
              break;
            }
            else
            {
              origPath.erase(itNext++);
              if (itNext == origPath.end())
                break;
              itNextNext = itNext + 1;
            }
          }
        }
      }
    } // loop over iCut
  }

  // Now smooth the corners. This is done by adding an extra two points either side of a corner
  // and removing the original. Could do this multiple times.
  std::vector<Vec3> pathPts;
  if (doCorners)
  {
    static int numCorners = 3;
    for (int iCorner = 0 ; iCorner < numCorners ; ++iCorner)
    {
      pathPts.resize(0);
      // This is the basic distance that will be used to smooth the corners. 
      static float basicDist = 10.0f;

      unsigned origNum = origPath.size();
      for (unsigned iPt = 0 ; iPt < origNum ; ++iPt)
      {
        const Vec3 & pt = origPath[iPt];
        // first and last are special
        if (0 == iPt)
        {
          pathPts.push_back(pt);
        }
        else if (origNum-1 == iPt) 
        {
          pathPts.push_back(pt);
        }
        else
        {
          // know that iPt-1 and iPt+1 are valid
          const Vec3 & prev = origPath[iPt-1];
          const Vec3 & next = origPath[iPt+1];
          // if we get two chops in a row we'll end up with a duplicate point - so be careful to
          // not add it, and make sure chopAmount is not > 0.5
          float chopAmount = 0.5f;
          float prevDist = min(chopAmount * (pt - prev).GetLength(), basicDist);
          float nextDist = min(chopAmount * (pt - next).GetLength(), basicDist);

          // the actual points to insert
          Vec3 ptPrev = pt + prevDist * (prev - pt).GetNormalizedSafe();
          Vec3 ptNext = pt + nextDist * (next - pt).GetNormalizedSafe();

          // only use the shortcut if there's line-of-sight, otherwise fall back to 
          // the original. Do less checks here because we're not modifying the path much
          // - just smoothing the corners.
          if (prevDist < 1.0f || nextDist < 1.0f || 
            PathSegmentWorldIntersection(ptPrev, ptNext, radius, false, true, PASSABILITY_FLAGS) ||
            OverlapLinesegBlockers(Lineseg(ptPrev, ptNext), navigationBlockers, IAISystem::NAV_VOLUME))
          {
            pathPts.push_back(pt);
          }
          else
          {
            if (!ptPrev.IsEquivalent(pathPts.back(), 0.1f))
              pathPts.push_back(ptPrev);
            pathPts.push_back(ptNext);
          }
        }
      }
      origPath = pathPts;
    }
  }
  else
  {
    // no corner cutting
    pathPts = origPath;
  }

  outPath.clear();
  unsigned numPts = pathPts.size();
  for (unsigned iPt = 0 ; iPt < numPts ; ++iPt)
    outPath.push_back(PathPointDescriptor(IAISystem::NAV_VOLUME, pathPts[iPt]));

  return;
}

//====================================================================
// IsConnecting
//====================================================================
bool CVolumeNavRegion::CPortal::IsConnecting(const CVolume* pOne, const CVolume* pTwo) const
{
  if(m_pVolumeOne==pOne && m_pVolumeTwo==pTwo)
    return true;
  if(m_pVolumeOne==pTwo && m_pVolumeTwo==pOne)
    return true;
  return false;
}

//====================================================================
// GetConnectionPoint
//====================================================================
Vec3 CVolumeNavRegion::CVolume::GetConnectionPoint(const CVolumeNavRegion * pVolumeNavRegion, const CVolume* otherVolume) const
{
  for(TPortalIndices::const_iterator itrPortal=m_portalIndices.begin(); itrPortal!=m_portalIndices.end(); ++itrPortal)
  {
    const CPortal & portal= *pVolumeNavRegion->GetPortal(pVolumeNavRegion->m_portals, *itrPortal);
    if(portal.IsConnecting(this, otherVolume))
    {
      return portal.m_passPoint;
    }
  }
  AIWarning("[AISystem] CVolume::GetConnectionPoint - not connected");
  return Center(pVolumeNavRegion->m_pGraph->GetNodeManager());
}

//====================================================================
// LoadPortal
//====================================================================
bool CVolumeNavRegion::LoadPortal(TVolumes &volumes, TPortals &portals, CCryFile &cryFile)
{
  portals.push_back(CPortal(0, 0));
  CPortal &portal = portals.back();

  int volumeIdx1;
  cryFile.ReadType(&volumeIdx1);
  portal.m_pVolumeOne = GetVolume(volumes, volumeIdx1);
  int volumeIdx2;
  cryFile.ReadType(&volumeIdx2);
  portal.m_pVolumeTwo = GetVolume(volumes, volumeIdx2);

  if (!portal.m_pVolumeOne || !portal.m_pVolumeTwo)
  {
    AIWarning("Error loading portal");
    portals.pop_back();
    return false;
  }

  cryFile.ReadType(&portal.m_passPoint);
  cryFile.ReadType(&portal.m_passRadius);

  m_pGraph->Connect(portal.m_pVolumeOne->m_graphNodeIndex, portal.m_pVolumeTwo->m_graphNodeIndex, portal.m_passRadius, portal.m_passRadius);
  return true;
}

//====================================================================
// LoadVolume
//====================================================================
bool CVolumeNavRegion::LoadVolume(TVolumes &volumes, CCryFile &cryFile)
{
  Vec3 center(ZERO);
  cryFile.ReadType(&center);
  float radius = 0.0f;
  cryFile.ReadType(&radius);
  float distanceToGeometry = 0.0f;
  cryFile.ReadType(&distanceToGeometry);

  CVolume *pVolume = CreateVolume(volumes, center, radius, distanceToGeometry);

  // read portals
  uint32 counter = 0;
  cryFile.ReadType(&counter);
  pVolume->m_portalIndices.resize(counter);
  if (counter > 0)
    cryFile.ReadType(&pVolume->m_portalIndices[0], counter);

  return true;
}

//====================================================================
// LoadHideSpots
//====================================================================
bool CVolumeNavRegion::LoadHideSpots(std::vector<SVolumeHideSpot>& hideSpots, CCryFile &cryFile)
{
  uint32 nHideSpots = 0;
  cryFile.ReadType(&nHideSpots);
  hideSpots.resize(nHideSpots);
  if (nHideSpots > 0)
    cryFile.ReadType(&hideSpots[0], nHideSpots);
  return true;
}

//====================================================================
// SaveHideSpots
//====================================================================
bool CVolumeNavRegion::SaveHideSpots(const std::vector<SVolumeHideSpot>& hideSpots, CCryFile &cryFile)
{
  uint32 nHideSpots = hideSpots.size();
  cryFile.Write(&nHideSpots, sizeof(nHideSpots));
  if (nHideSpots > 0)
    cryFile.Write(&hideSpots[0], nHideSpots * sizeof(hideSpots[0]));
  return true;
}

//====================================================================
// LoadNavigation
//====================================================================
bool CVolumeNavRegion::LoadNavigation(TVolumes &volumes, TPortals &portals, CCryFile& cryFile)
{
  int nFileVersion = BAI_3D_FILE_VERSION;
  cryFile.ReadType(&nFileVersion);

  if (nFileVersion != BAI_3D_FILE_VERSION)
  {
    AIWarning("Wrong BAI file version (found %d expected %d)!! Regenerate 3d navigation volumes in the editor.",
      nFileVersion, BAI_3D_FILE_VERSION);
    return false;
  }

  // load volumes
  uint32 volumeCount = 0;
  cryFile.ReadType(&volumeCount);
  volumes.reserve(volumeCount);
  for (uint32 iVolume = 0 ; iVolume < volumeCount ; ++iVolume)
  {
    if (!LoadVolume(volumes, cryFile))
    {
      AIWarning("Error loading volume navigation");
      return false;
    }
  }

  // load portals
  uint32 portalCount = 0;
  cryFile.ReadType(&portalCount);
  portals.reserve(portalCount);
  for(uint32 iPortal = 0 ; iPortal < portalCount ; ++iPortal)
  {
    if (!LoadPortal(volumes, portals, cryFile))
    {
      AIWarning("Error loading volume (portals) navigation");
      return false;
    }
  }

  for (uint32 iVolume = 0 ; iVolume < volumeCount ; ++iVolume)
  {
    const CVolume & volume = *volumes[iVolume];
    for (uint32 i = 0 ; i < volume.m_portalIndices.size() ; ++i)
      AIAssert(volume.m_portalIndices[i] < (int)portals.size());
  }

  return true;
}

//====================================================================
// LoadRegion
//====================================================================
void CVolumeNavRegion::LoadRegion(const char * szLevel, const char * szMission, SRegionData &regionData)
{
  char fileName[1024];
  sprintf(fileName, "%s/v3d%s-region-%s.bai",szLevel,szMission, regionData.name.c_str());

  CCryFile file;
  if (!file.Open(fileName, "rb"))
  {
    AIWarning("Failed to open %s - this region will not be calculated/loaded", fileName);
    return;
  }

  file.ReadType(&regionData.volumeRadius);
  if (!LoadNavigation(regionData.volumes, regionData.portals, file))
    AIWarning("Failed to load region data from %s", fileName);

  if (!LoadHideSpots(regionData.hideSpots, file))
    AIWarning("Failed to load region hidespots from %s", fileName);
}

//====================================================================
// ReadFromFile
//====================================================================
bool CVolumeNavRegion::ReadFromFile(const char *pName)
{
  float fStartTime = gEnv->pTimer->GetAsyncCurTime();
  AILogLoading("Reading AI volumes [%s]", pName);

  CCryFile file;;
  if (!file.Open( pName,"rb"))
  {
    AIWarning("could not read AI volume. [%s]", pName);
    return false;
  }

  int nFileVersion = BAI_3D_FILE_VERSION;
  file.ReadType(&nFileVersion);

  if (nFileVersion != BAI_3D_FILE_VERSION)
  {
    AIWarning("Wrong BAI file version (found %d expected %d)!! Regenerate 3d navigation volumes in the editor.",
      nFileVersion, BAI_3D_FILE_VERSION);
    return false;
  }

  file.ReadType(&m_AABB);

  if (!LoadNavigation(m_volumes, m_portals, file))
  {
    AIWarning("Failed to load volume navigation data");
    Clear();
    return false;
  }

	// [Mikko] 10.9.07 - The volume navigation hide points are not going to be used
	// in the production level, so I commented this out for now. The reason is that
	// the hash space serialization changed.

  // read hide spots
//  m_hideSpots.ReadFromFile(file);

	AILogLoading("Read %d hidespots from file", m_hideSpots.GetNumObjects());

  AILogLoading("AI volumes loaded  successfully in %6.3f sec. [%s]", 
    gEnv->pTimer->GetAsyncCurTime() - fStartTime, pName);
  AILogLoading("AI num volumes = %d, num portals = %d", 
    m_volumes.size(), m_portals.size());
  return true;
}

//====================================================================
// DebugDraw
//====================================================================
void CVolumeNavRegion::DebugDraw() const
{
  IEntity* ent1;
  IEntity* ent2;
  ent1 = gEnv->pEntitySystem->FindEntityByName("GravityTestStart");
  ent2 = gEnv->pEntitySystem->FindEntityByName("GravityTestEnd");
  if( ent1 && ent2 )
  {
    Vec3 start = ent1->GetPos();
    Vec3 end = ent2->GetPos();

    GravityModifiers gravityModifiers;
    GetGravityModifiers(gravityModifiers, m_AABB);

    ColorB color = DoesLinkIntersectGravityModifier(start, end, gravityModifiers) ? ColorB(255, 0, 0) : ColorB(0, 255, 0);

    CDebugDrawContext dc;
    dc->DrawLine(start, color, end, color);
  }
}

//====================================================================
// DebugDrawVoxels
//====================================================================
void CVolumeNavRegion::DebugDrawVoxels() const
{
  CDebugDrawContext dc;
  Vec3 pos = dc->GetCameraPos();
  static float dist = 50.0f;

  int testVal = gAIEnv.CVars.DebugDrawVolumeVoxels;
  if (testVal == 0)
    return;

  ColorB color(255, 255, 0);
	
  for (TDebugVoxels::const_iterator it = m_debugVoxels.begin() ; it != m_debugVoxels.end() ; ++it)
  {
    const SVoxelData &voxelData = *it;

    Vec3i minIdx(
      (int)((pos.x - voxelData.corner.x) / voxelData.size), 
      (int)((pos.y - voxelData.corner.y) / voxelData.size), 
      (int)((pos.z - voxelData.corner.z) / voxelData.size));
    Vec3i maxIdx = minIdx;
    minIdx.x -= (int)(0.5f * dist / voxelData.size);
    minIdx.y -= (int)(0.5f * dist / voxelData.size);
    minIdx.z -= (int)(0.5f * dist / voxelData.size);

    maxIdx.x += (int)(0.5f * dist / voxelData.size);
    maxIdx.y += (int)(0.5f * dist / voxelData.size);
    maxIdx.z += (int)(0.5f * dist / voxelData.size);

    Limit(minIdx.x, 0, voxelData.dim.x);
    Limit(minIdx.y, 0, voxelData.dim.y);
    Limit(minIdx.z, 0, voxelData.dim.z);

    Limit(maxIdx.x, 0, voxelData.dim.x);
    Limit(maxIdx.y, 0, voxelData.dim.y);
    Limit(maxIdx.z, 0, voxelData.dim.z);

    for (int i = minIdx.x ; i < maxIdx.x ; ++i)
    {
      for (int j = minIdx.y ; j < maxIdx.y ; ++j)
      {
        for (int k = minIdx.z ; k < maxIdx.z ; ++k)
        {
          TVoxelValue val = GetVoxelValue(voxelData, i, j, k);
          if ( (testVal > 0 && val >= testVal) ||
            (testVal < 0 && val == -testVal) )
          {
            Vec3 voxelPos = voxelData.corner + Vec3(i * voxelData.size, j * voxelData.size, k * voxelData.size);
            dc->DrawSphere(voxelPos, 0.1f, color);
          }
        }
      }
    }
  }
}

//====================================================================
// DebugDrawConnections
//====================================================================
void CVolumeNavRegion::DebugDrawConnections() const
{
  for(size_t volumeIdx=0; volumeIdx<m_volumes.size(); ++volumeIdx)
    DebugDrawConnections(volumeIdx, false);
}


//====================================================================
// DebugDrawConnections
//====================================================================
void CVolumeNavRegion::DebugDrawConnections(size_t volumeIdx, bool showOtherLabel) const
{
  if(volumeIdx >= m_volumes.size())
    return;
  const CVolume	&curVolume = *m_volumes[volumeIdx];

  static float maxDistForLabelsSq = square(50.0f);
  float distFromPlayerToVol = std::numeric_limits<float>::max();
  CDebugDrawContext dc;
  distFromPlayerToVol = (dc->GetCameraPos() - curVolume.Center(m_pGraph->GetNodeManager())).GetLength();

  if (distFromPlayerToVol > gAIEnv.CVars.DrawStats)
    return;

  bool renderLabels = false;
  static float maxDistForLabels = 50.0f;
  if (distFromPlayerToVol < maxDistForLabels)
    renderLabels = true;

  static bool renderDistance = false;

  char	buff[16];
  ColorB color(255, 255, 255);

  if (renderLabels)
  {
    Vec3 pzpos = curVolume.Center(m_pGraph->GetNodeManager());
    pzpos.z += 1.0f;
    sprintf(buff, "%d", volumeIdx);
    dc->Draw3dLabelEx(pzpos, 6, color, false, false, buff);
  }

  if (renderLabels && renderDistance)
  {
    Vec3 pzpos = curVolume.Center(m_pGraph->GetNodeManager());
    pzpos.z -= 1.0f;
    sprintf(buff, "%3.0f", curVolume.m_distanceToGeometry);
    dc->Draw3dLabelEx(pzpos, 6, color, false, false, buff);
  }

  if (curVolume.m_portalIndices.empty())
    dc->DrawSphere(curVolume.Center(m_pGraph->GetNodeManager()), 0.4f, ColorB(255, 0, 0));

  for(TPortalIndices::const_iterator itrLink=curVolume.m_portalIndices.begin(); 
    itrLink != curVolume.m_portalIndices.end(); 
    ++itrLink)
  {
    const CPortal &portal = *GetPortal(m_portals, *itrLink);
    const Vec3 &pos1 = portal.m_pVolumeOne->Center(m_pGraph->GetNodeManager());
    const Vec3 &pos2 = portal.m_pVolumeTwo->Center(m_pGraph->GetNodeManager());

    ColorB color = ColorF(.3f, .3f, .7f);
    dc->DrawSphere(pos1, 0.4f, color);
    dc->DrawSphere(pos2, 0.4f, color);

    color.Set(.2f, .7f,  .2f, 1.0f);
    dc->DrawSphere(portal.m_passPoint, 0.2f, color);

    ColorF passCol(1, 1, 0, 1);
    ColorF noPassCol(1, 0, 0, 1);
    ColorF col1 = noPassCol;
    ColorF col2 = noPassCol;
		unsigned link1 = m_pGraph->GetNodeManager().GetNode(portal.m_pVolumeOne->m_graphNodeIndex)->GetLinkTo(m_pGraph->GetNodeManager(), m_pGraph->GetLinkManager(), m_pGraph->GetNodeManager().GetNode(portal.m_pVolumeTwo->m_graphNodeIndex));
    if (link1 && m_pGraph->GetLinkManager().GetRadius(link1) > 0.0f)
      col1 = passCol;
		unsigned link2 = m_pGraph->GetNodeManager().GetNode(portal.m_pVolumeTwo->m_graphNodeIndex)->GetLinkTo(m_pGraph->GetNodeManager(), m_pGraph->GetLinkManager(), m_pGraph->GetNodeManager().GetNode(portal.m_pVolumeOne->m_graphNodeIndex));
    if (link2 && m_pGraph->GetLinkManager().GetRadius(link2) > 0.0f)
      col2 = passCol;

    dc->DrawLine(pos1, col1, portal.m_passPoint, col1);
    dc->DrawLine(portal.m_passPoint, col2, pos2, col2);

    if (renderLabels)
    {
      sprintf(buff, "%5.2f", portal.m_passRadius);
      dc->Draw3dLabelEx(portal.m_passPoint + Vec3(0, 0, 0.5f), 6, color, false, false, buff);
    }

    if (showOtherLabel && renderLabels)
    {
      const CVolume * otherVol = portal.m_pVolumeOne == &curVolume ? portal.m_pVolumeTwo : portal.m_pVolumeOne;
      unsigned i;
      for (i = 0 ; i < m_volumes.size() ; ++i)
        if (m_volumes[i] == otherVol)
          break;
      sprintf(buff, "%d", i);
      dc->Draw3dLabelEx(otherVol->Center(m_pGraph->GetNodeManager()) + Vec3(0, 0, 1.0f), 6, color, false, false, buff);
    }
  }
}

//====================================================================
// DebugDrawNavProblems
//====================================================================
void CVolumeNavRegion::DebugDrawNavProblems()
{
  CDebugDrawContext dc;
  Vec3 playerPos = dc->GetCameraPos();
  playerPos.x = (float) ( (int) playerPos.x );
  playerPos.y = (float) ( (int) playerPos.y );
  playerPos.z = (float) ( (int) playerPos.z );

  // Need point that is definitely in valid space
  std::vector<IEntity*> entities;
  IEntityItPtr entIt = gEnv->pEntitySystem->GetEntityIterator();
  const char* navName = "NavigableSpace";
  size_t navNameLen = strlen(navName);
  IEntity *bestEntity = 0;
  float bestDist = std::numeric_limits<float>::max();
  if (entIt)
  {
    entIt->MoveFirst();
    while (!entIt->IsEnd())
    {
      IEntity* ent = entIt->Next();
      if (ent)
      {
        const char* name = ent->GetName();
        if (strnicmp(name, navName, navNameLen) == 0)
        {
          float dist = (ent->GetPos() - playerPos).GetLength();
          if (dist < bestDist)
          {
            bestDist = dist;
            bestEntity = ent;
          }
        }
      }
    }
  }
  if (bestEntity == 0)
  {
    AIError("CVolumeNavRegion Need at least one point called NavigableSpace that indicates valid volume navigation space [Design bug]");
    return;
  }

  static float range = 25.0f;
  static float delta = 0.5f;
  static float testRadius = 1.0f;
  static std::vector<Vec3> problems;
  static std::vector<Vec3> newProblems;

  static int counter = 0;

  Vec3 tmpPos;

  int num = (int) (range/delta);
  CCalculationStopper stopper("VolumeNavProblems", 0.05f, 10000);
  while (counter < num*num*num && !stopper.ShouldCalculationStop())
  {
    int tmp = counter;
    int countX = tmp % num;
    tmp = (tmp - countX) / num;
    int countY = tmp % num;
    tmp = (tmp - countY) / num;
    int countZ = tmp % num;

    Vec3 pos(((float) countX) / num, ((float) countY) / num, ((float) countZ) / num);
    pos *= num * delta;
    pos += playerPos - 0.5f * num * Vec3(delta, delta, delta);

    bool ptInNavigableSpace = CheckVoxel(pos, bestEntity->GetPos()) == VOXEL_VALID;
    if (ptInNavigableSpace)
    {
      if (0 == GetEnclosing(pos, testRadius, 0, 5.0f, &tmpPos, true))
      {
        newProblems.push_back(pos);
      }
    }
    ++counter;
  }

  if (counter >= num*num*num)
    counter = 0;

  if (0 == counter)
  {
    problems.swap(newProblems);
    newProblems.resize(0);
    AILogComment("Updating Volume nav problems: num = %d", problems.size());
  }

  ColorB color(255, 0, 0);
  for (unsigned i = 0 ; i < problems.size() ; ++i)
    dc->DrawSphere(problems[i], 0.05f, color);
}

//====================================================================
// Serialize
//====================================================================
void CVolumeNavRegion::Serialize(TSerialize ser, CObjectTracker& objectTracker)
{
  ser.BeginGroup("VolumeNavRegion");

  ser.EndGroup();
}

//====================================================================
// ISVolumeHideSpotGood
//====================================================================
bool CVolumeNavRegion::ISVolumeHideSpotGood(SVolumeHideSpot& hideSpot, const Vec3& triPos)
{
  static float testRadius = 1.5f; //2.5f;
  unsigned numRadialTests = 6;
  static float radialDist = 4.0f; //10.0f;
  static float radialDistClose = 3.0f; //1.75f; //4.0f;
  static float fwdDist = 7.0f; //15.0f;

  if( hideSpot.dir.GetLengthSquared() < 0.001f )
    return false;

  // check we can actually get here
  if (!GetEnclosing(hideSpot.pos, 1.0f))
    return false;

  Vec3 dirToTri = triPos - hideSpot.pos;
  float distToTri = dirToTri.GetLength();
  if (distToTri < 0.1f)
    return false;
  dirToTri /= distToTri;

  Vec3 testTriPos = hideSpot.pos + dirToTri * (distToTri - 0.1f);

  if (PathSegmentWorldIntersection(hideSpot.pos, testTriPos, testRadius, true, false, AICE_ALL))
    return false;

  static std::vector<Vec3> radialPts;
  static std::vector<Vec3> enemyPts;
  static std::vector<bool> results;

  Matrix33 mat = Matrix33::CreateOrthogonalBase(hideSpot.dir);

  Vec3 finalDir(0, 0, 0);
  unsigned failedCount = 0;
  float angleOffset = (float)ai_rand() / (float)RAND_MAX * gf_PI;
	const float numRadialTestsInv = 1.f / (float)numRadialTests;
	float fI = 0.f;//avoid LHS
  for (unsigned i = 0 ; i < numRadialTests ; ++i)
  {
		fI += 1.f;
    float angle = (gf_PI2 * fI) * numRadialTestsInv;
    float x = cosf(angle);
    float y = sinf(angle);

    Vec3 radialPt = hideSpot.pos + radialDist * (x * mat.GetColumn(1) + y * mat.GetColumn(2));
    Vec3 radialPtClose = hideSpot.pos + radialDistClose * (x * mat.GetColumn(1) + y * mat.GetColumn(2));
    Vec3 enemyPt = radialPt + fwdDist * hideSpot.dir;
    Vec3 enemyPtClose = radialPtClose + fwdDist * hideSpot.dir;

    if (!PathSegmentWorldIntersection(hideSpot.pos, radialPt, testRadius, false, true, AICE_ALL))
    {
      // clear out to radial position
      if (PathSegmentWorldIntersection(radialPtClose, enemyPtClose, testRadius, false, true, AICE_ALL))
      {
        // the close position is occluded
        if (!PathSegmentWorldIntersection(radialPt, enemyPt, testRadius, false, true, AICE_ALL))
        {
          // no direct los to target
          if (PathSegmentWorldIntersection(hideSpot.pos, enemyPt, testRadius, false, true, AICE_ALL))
          {
            finalDir += (enemyPt - hideSpot.pos);
          }
        }
      }
      else
      {
        failedCount++;
        if( failedCount >= numRadialTests )
          return false;
      }
    }
  }

  if (finalDir.GetLengthSquared() < 0.01f)
    return false;

  finalDir.NormalizeSafe();
  hideSpot.dir += finalDir;
  hideSpot.dir.NormalizeSafe();

  return true;
}

//====================================================================
// CalculateHideSpots
// just recalculates within one region
//====================================================================
void CVolumeNavRegion::CalculateHideSpots()
{
  m_hideSpots.Clear(true);

  // todo Danny/Mikko return early since there are some issues with calculating
  // hidespots, and anyway they're not used.
//  return;

  SRegionData regionData;
  regionData.aabb = m_AABB;
  regionData.name = string("All navigation");
  regionData.sa = 0;

  std::vector<SVolumeHideSpot> hideSpots;
  CalculateHideSpots(regionData, hideSpots);

  unsigned nHideSpots = hideSpots.size();
  for (unsigned i = 0 ; i < nHideSpots ; ++i)
  {
    m_hideSpots.AddObject(hideSpots[i]);
  }
}

inline bool CheckTriangleArea( const Vec3& p0, const Vec3& p1, const Vec3& p2 )
{
  Vec3 triDir = (p1 - p0).Cross(p2 - p0);
  float	len = triDir.GetLengthSquared();
  if( len < 0.0001f )
    return false;
  return true;
}

void SubdivideTriangle( std::deque<Triangle>& triangles, const Vec3& p0, const Vec3& p1, const Vec3& p2, float tesSize )
{
  // The triangle is tesselated by first finding the longest edge and the subidiving that.
  // The same amount of points are distributed on the two other edges and vertical stripes
  // are formed.
  //
  // Finally the vertical stripes are subdivided too and triangles are formed between the two
  // neighbor stripes. Each triangle pair is further optimized to create as uniform triangles
  // as possible.
  //
  //   top
  //       o.
  //     .'| '|..
  //   .|  |  |  |'.
  //  o.|..|..|..|..|:o
  //   bottom
  //
  //       o.
  //     .'| '|..
  //   .|--|--|--|'.
  //  o.|..|..|..|..|:o

  // Calculate the triangle edges and find the longest edge.
  const Vec3	points[3] = { p0, p1, p2 };
  int		longest = 0;
  float	longestLen = 0;
  Vec3	edge[3];
  float	edgeLen[3];

  for( int i = 0; i < 3; i++ )
  {
    edge[i] = points[(i + 1) % 3] - points[i];
    edgeLen[i] = edge[i].len();
    if( edgeLen[i] > longestLen )
    {
      longestLen = edgeLen[i];
      longest = i;
    }
  }

  const Vec3&	edgeA = edge[(longest + 1) % 3];
  const Vec3&	edgeB = edge[(longest + 2) % 3];

  // Project the point opposite the longest edge on the longest edge.
  Vec3	dir = edge[longest].normalized();
  float	dot = dir.Dot( -edgeA );
  Vec3	proj = points[longest] + dir * dot;

  std::vector<Vec3>	top;
  std::vector<Vec3>	bottom;
  std::vector<int>	pointCount;

  int	nPtLongest = (int)floor( edgeLen[longest] / tesSize ) + 2;

  top.resize( nPtLongest );
  bottom.resize( nPtLongest );
  pointCount.resize( nPtLongest );

  // Calculate the bottom set of points.
  for( int i = 0; i < nPtLongest; i++ )
  {
    float	t = (float)i / (float)(nPtLongest - 1);
    bottom[i] = points[longest] + edge[longest] * (1 - t);
  }

  float	edgeALen = edgeA.len();
  float	edgeBLen = edgeB.len();

  int	nExtra = max( 0, (nPtLongest - 3) );
  int	nPtEdgeA = max( 0, (int)floor((dot / edgeLen[longest]) * nExtra + 0.5f) );
  int	nPtEdgeB = nExtra - nPtEdgeA;
  nPtEdgeA += 1;
  nPtEdgeB += 2;

  // Calculate the top set of points.
  for( int i = 0; i < nPtEdgeA; i++ )
  {
    float	t = (float)i / (float)nPtEdgeA;
    top[i] = points[(longest + 1) % 3] + edgeA * t;
  }

  for( int i = 0; i < nPtEdgeB; i++ )
  {
    float	t = (float)i / (float)(nPtEdgeB - 1);
    top[i + nPtEdgeA] = points[(longest + 2) % 3] + edgeB * t;
  }

  for( int i = 0; i < nPtLongest; i++ )
  {
    Vec3	seg = top[i] - bottom[i];
    pointCount[i] = (int)floor( seg.len() / tesSize ) + 2;
  }

  int	triCount = 0;

  // Calculate the triangles.
  for( int i = 0; i < nPtLongest - 1; i++ )
  {
    // 'a' and 'b' are clamped counters. That is either of them can reach
    // the target too early, but clamped last point is used as the pair until
    // all the iterations are done.
    int	a = 0;
    int	b = 0;
    int	m = max( pointCount[i], pointCount[i + 1] );

    Vec3	seg0 = top[i] - bottom[i];
    Vec3	seg1 = top[i + 1] - bottom[i + 1];

    for( int j = 0; j < m - 1; j++ )
    {
      // Calculate the four points required to create he two or one triangle.
      const float	t0 = (float)a / (float)(pointCount[i] - 1);
      const float	t1 = (float)b / (float)(pointCount[i + 1] - 1);
      const float	t2 = (float)(b + 1) / (float)(pointCount[i + 1] - 1);
      const float	t3 = (float)(a + 1) / (float)(pointCount[i] - 1);

      const Vec3	pt0 = bottom[i] + seg0 * t0;
      const Vec3	pt1 = bottom[i + 1] + seg1 * t1;
      const Vec3	pt2 = bottom[i + 1] + seg1 * t2;
      const Vec3	pt3 = bottom[i] + seg0 * t3;

      // Build the triangles so that
      float	lenDiagA = (pt0 - pt2).len2();
      float	lenDiagB = (pt1 - pt3).len2();
      if( lenDiagA > 0 && lenDiagA < lenDiagB )
      {
        // Handle the case that sometimes there needs to be only one triangle.
        if( b + 1 < pointCount[i + 1] )
        {
          if( CheckTriangleArea(pt2, pt1, pt0) )
            triangles.push_back(Triangle(pt2, pt1, pt0));
          triCount++;
          if( a + 1 < pointCount[i] )
          {
            if( CheckTriangleArea(pt3, pt2, pt0)  )
              triangles.push_back(Triangle(pt3, pt2, pt0));
            triCount++;
          }
        }
        else if( a + 1 < pointCount[i] )
        {
          if( CheckTriangleArea(pt3, pt1, pt0)  )
            triangles.push_back(Triangle(pt3, pt1, pt0));
          triCount++;
        }
      }
      else
      {
        // Handle the case that sometimes there needs to be only one triangle.
        if( a + 1 < pointCount[i] )
        {
          if( CheckTriangleArea(pt3, pt1, pt0) )
            triangles.push_back(Triangle(pt3, pt1, pt0));
          triCount++;
          if( b + 1 < pointCount[i + 1] )
          {
            if( CheckTriangleArea(pt3, pt2, pt1) )
              triangles.push_back(Triangle(pt3, pt2, pt1));
            triCount++;
          }
        }
        else if( b + 1 < pointCount[i + 1] )
        {
          if( CheckTriangleArea(pt2, pt1, pt0) )
            triangles.push_back(Triangle(pt2, pt1, pt0));
          triCount++;
        }
      }

      // Increment and clamp the point iterators.
      a++; b++;
      if( a >= pointCount[i] ) a = pointCount[i] - 1;
      if( b >= pointCount[i + 1] ) b = pointCount[i + 1] - 1;
    }
  }
}

//====================================================================
// CalculateHideSpots
//====================================================================
void CVolumeNavRegion::CalculateHideSpots(SRegionData &regionData, std::vector<SVolumeHideSpot> &hideSpots)
{
  AILogProgress("Obtaining the triangles for hidespots...");

	AILogProgress("voxels = %d\n", regionData.voxelData.voxels.size());

/*	for (TVolumes::iterator it = regionData.volumes.begin(), end = regionData.volumes.end(); it != end; ++it)
	{
	}*/

/*
  IPhysicalEntity ** ppList = 0;
  int	numEntites = gEnv->pPhysicalWorld->GetEntitiesInBox(regionData.aabb.min, regionData.aabb.max,
    ppList, ent_static | ent_ignore_noncolliding);

  std::deque<Triangle> triangles;

  for( int i = 0; i < numEntites; ++i )
  {
    pe_status_pos status;
    status.ipart = 0;
    ppList[i]->GetStatus( &status );

    Vec3 pos = status.pos;
    Matrix33 mat(status.q);
    mat *= status.scale;

    IGeometry *geom = status.pGeom;
    int type = geom->GetType();
    switch (type)
    {
    case GEOM_BOX:
      {
        const primitives::primitive * prim = geom->GetData();
        const primitives::box* bbox = static_cast<const primitives::box*>(prim);
        Matrix33 rot = bbox->Basis.T();

        Vec3	boxVerts[8];
        int		sides[6 * 4] = {
          3, 2, 1, 0,
            0, 1, 5, 4,
            1, 2, 6, 5,
            2, 3, 7, 6,
            3, 0, 4, 7,
            4, 5, 6, 7,
        };

        boxVerts[0] = bbox->center + rot * Vec3( -bbox->size.x, -bbox->size.y, -bbox->size.z );
        boxVerts[1] = bbox->center + rot * Vec3( bbox->size.x, -bbox->size.y, -bbox->size.z );
        boxVerts[2] = bbox->center + rot * Vec3( bbox->size.x, bbox->size.y, -bbox->size.z );
        boxVerts[3] = bbox->center + rot * Vec3( -bbox->size.x, bbox->size.y, -bbox->size.z );
        boxVerts[4] = bbox->center + rot * Vec3( -bbox->size.x, -bbox->size.y, bbox->size.z );
        boxVerts[5] = bbox->center + rot * Vec3( bbox->size.x, -bbox->size.y, bbox->size.z );
        boxVerts[6] = bbox->center + rot * Vec3( bbox->size.x, bbox->size.y, bbox->size.z );
        boxVerts[7] = bbox->center + rot * Vec3( -bbox->size.x, bbox->size.y, bbox->size.z );

        for( int j = 0; j < 8; j++ )
          boxVerts[j] = pos + mat * boxVerts[j];

        for( int j = 0; j < 6; j++ )
        {
          // subdivide the side if necessary.
          Vec3	edgeU = boxVerts[sides[j*4 + 1]] - boxVerts[sides[j*4 + 0]];
          Vec3	edgeV = boxVerts[sides[j*4 + 2]] - boxVerts[sides[j*4 + 1]];
          const Vec3&	base = boxVerts[sides[j*4 + 0]];

          float	lenU = edgeU.GetLength();
          float	lenV = edgeV.GetLength();

          const float	quadSize = 1.5f;
          int		subdivU = (int)floorf( lenU / quadSize );
          int		subdivV = (int)floorf( lenV / quadSize );
          if( subdivU < 1 ) subdivU = 1;
          if( subdivV < 1 ) subdivV = 1;

          float	deltaU = 1.0f / (float)subdivU;
          float	deltaV = 1.0f / (float)subdivV;

          for( int v = 0; v < subdivV; v++ )
          {
            for( int u = 0; u < subdivU; u++ )
            {
              float	ut = (float)u * deltaU;
              float	vt = (float)v * deltaV;

              // Create two triangle per each quad.
              // 3 +-----+ 2
              //   |   / |
              //   | /   |
              // 0 x-----x 1
              const Vec3	v0 = base + edgeU * ut + edgeV * vt;
              const Vec3	v1 = v0 + edgeU * deltaU;
              const Vec3	v2 = v0 + edgeU * deltaU + edgeV * deltaV;
              const Vec3	v3 = v0 + edgeV * deltaV;

              if (!regionData.aabb.IsContainPoint(v0))
                continue;
              if (!regionData.aabb.IsContainPoint(v1))
                continue;
              if (!regionData.aabb.IsContainPoint(v2))
                continue;
              if (!regionData.aabb.IsContainPoint(v3))
                continue;
              triangles.push_back(Triangle(v0, v1, v2));
              triangles.push_back(Triangle(v0, v2, v3));
            }
          }
        }

      }
      break;

    case GEOM_TRIMESH:
    case GEOM_VOXELGRID:
      {
        const primitives::primitive * prim = geom->GetData();
        const mesh_data * mesh = static_cast<const mesh_data *>(prim);

        int numVerts = mesh->nVertices;
        int numTris = mesh->nTris;

        for (int j = 0; j < numTris; ++j)
        {
          const Vec3 v0 = pos + mat * mesh->pVertices[mesh->pIndices[j*3 + 0]];
          if (!regionData.aabb.IsContainPoint(v0))
            continue;
          const Vec3 v1 = pos + mat * mesh->pVertices[mesh->pIndices[j*3 + 1]];
          if (!regionData.aabb.IsContainPoint(v1))
            continue;
          const Vec3 v2 = pos + mat * mesh->pVertices[mesh->pIndices[j*3 + 2]];
          if (!regionData.aabb.IsContainPoint(v2))
            continue;

          const float minEdgeLen = 0.25f * 0.25f;
          Vec3	e0 = v1 - v0;
          Vec3	e1 = v2 - v1;
          Vec3	e2 = v0 - v2;
          const float e0LenSqr = e0.GetLengthSquared();
          const float e1LenSqr = e1.GetLengthSquared();
          const float e2LenSqr = e2.GetLengthSquared();
          if( e0LenSqr < minEdgeLen && e1LenSqr < minEdgeLen && e2LenSqr < minEdgeLen )
            continue;

          const float subDivSize = 2.0f;
          const float subDivSizeSqr = subDivSize * subDivSize;

          if( e0LenSqr < subDivSizeSqr && e1LenSqr < subDivSizeSqr && e2LenSqr < subDivSizeSqr )
            triangles.push_back(Triangle(v0, v1, v2));
          else
            SubdivideTriangle( triangles, v0, v1, v2, subDivSize );
        }
        break;
      }
    default:
      break;
    }
  }

  unsigned nTriangles = triangles.size();
  AILogProgress("Finished obtaining %d triangles...", nTriangles);

  float hideTriDist = 2.0f;

  float lastPC = 0.0f;
  for (unsigned iTriangle = 0 ; iTriangle < nTriangles ; iTriangle += 2)
  {
    float pc = (100.0f * iTriangle) / nTriangles;
    if (pc > lastPC + 2.0f)
    {
      AILogProgress("Completed %5.2f%%%% of hidespots", pc);
      lastPC = pc;
    }
    const Triangle& tri = triangles[iTriangle];
    Vec3 midPt = (tri.v0 + tri.v1 + tri.v2 ) / 3.0f;
    Vec3 triDir = (tri.v1 - tri.v0).Cross(tri.v2 - tri.v0);
    triDir.NormalizeSafe();
    SVolumeHideSpot hideSpot(midPt + hideTriDist * triDir, -triDir);
    if (ISVolumeHideSpotGood(hideSpot, midPt))
      hideSpots.push_back(hideSpot);
  }
  AILogProgress("Completed 100%%%% of hidespots");*/
}

//====================================================================
// SVolumeHideSpotDrawer
//====================================================================
struct SVolumeHideSpotDrawer
{
  SVolumeHideSpotDrawer() {}
  void operator()(const SVolumeHideSpot& hs, float)
  {
    static const ColorB color(0, 255, 0);
    static float size = 1.0f;
    CDebugDrawContext dc;
    dc->DrawCone(hs.pos + size * hs.dir, -hs.dir, 0.5f * size, size, color);
  }
};

//====================================================================
// DebugDrawHideSpots
//====================================================================
void CVolumeNavRegion::DebugDrawHideSpots()
{
  CDebugDrawContext dc;
  Vec3 playerPos = dc->GetCameraPos();
  SVolumeHideSpotDrawer func;
  m_hideSpots.ProcessObjectsWithinRadius(playerPos, 50.0f, func);
}

//====================================================================
// Update
//====================================================================
void CVolumeNavRegion::Update(CCalculationStopper& stopper)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

  if (m_volumes.empty())
    return;

  GravityModifiers gravityModifiers;
  GetGravityModifiers(gravityModifiers, m_AABB);
  unsigned numGravityModifiers = gravityModifiers.size();

  m_lastGravityPortalIndex = m_lastGravityPortalIndex % m_portals.size();
  unsigned origIndex = m_lastGravityPortalIndex;

  do
  {
    CPortal &portal = m_portals[m_lastGravityPortalIndex];
    unsigned link1 = m_pGraph->GetNodeManager().GetNode(portal.m_pVolumeOne->m_graphNodeIndex)->GetLinkTo(m_pGraph->GetNodeManager(), m_pGraph->GetLinkManager(), m_pGraph->GetNodeManager().GetNode(portal.m_pVolumeTwo->m_graphNodeIndex));
    unsigned link2 = m_pGraph->GetNodeManager().GetNode(portal.m_pVolumeTwo->m_graphNodeIndex)->GetLinkTo(m_pGraph->GetNodeManager(), m_pGraph->GetLinkManager(), m_pGraph->GetNodeManager().GetNode(portal.m_pVolumeOne->m_graphNodeIndex));
    if (!link1 || !link2)
      continue;
    if (DoesLinkIntersectGravityModifier(m_pGraph->GetNodeManager().GetNode(portal.m_pVolumeOne->m_graphNodeIndex)->GetPos(), 
      portal.m_passPoint, m_pGraph->GetNodeManager().GetNode(portal.m_pVolumeTwo->m_graphNodeIndex)->GetPos(), gravityModifiers))
      m_pGraph->GetLinkManager().ModifyRadius(link1, -1.0f);
    else
      m_pGraph->GetLinkManager().RestoreLink(link1);

    if (DoesLinkIntersectGravityModifier(m_pGraph->GetNodeManager().GetNode(portal.m_pVolumeTwo->m_graphNodeIndex)->GetPos(), 
      portal.m_passPoint, m_pGraph->GetNodeManager().GetNode(portal.m_pVolumeOne->m_graphNodeIndex)->GetPos(), gravityModifiers))
      m_pGraph->GetLinkManager().ModifyRadius(link2, -1.0f);
    else
      m_pGraph->GetLinkManager().RestoreLink(link2);

    m_lastGravityPortalIndex = (m_lastGravityPortalIndex + 1) % m_portals.size();
  } while (!stopper.ShouldCalculationStop() && m_lastGravityPortalIndex != origIndex);

}

//====================================================================
// GetNodeFromIndex
//====================================================================
unsigned CVolumeNavRegion::GetNodeFromIndex(int index)
{
  if (index < 0 || index > (int) m_volumes.size())
    return 0;
  else
    return m_volumes[index]->m_graphNodeIndex;
}
