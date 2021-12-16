/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   PathObstacles.cpp
$Id$
Description: 

-------------------------------------------------------------------------
History:
- ?
- 4 May 2009  : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#include "StdAfx.h"
#include "PathObstacles.h"
#include "AICollision.h"
#include "DebugDrawContext.h"
#include "Puppet.h"

#include "I3DEngine.h"

#include <numeric>

static float criticalTopAlt = 0.5f;
static float criticalBaseAlt = 2.0f;

// NOTE Jun 4, 2007: <pvl> the comparison operators are used just for debugging ATM.
inline bool operator== (const SPathObstacleCircle2D & lhs, const SPathObstacleCircle2D & rhs)
{
	return lhs.center == rhs.center && lhs.radius == rhs.radius;
}

inline bool operator== (const SPathObstacleShape2D & lhs, const SPathObstacleShape2D & rhs)
{
	return IsEquivalent (lhs.aabb, rhs.aabb, 0.0f) && lhs.pts == rhs.pts;
}

bool operator== (const CPathObstacle & lhs, const CPathObstacle & rhs)
{
	if (lhs.GetType () != rhs.GetType ())
		return false;

	CPathObstacle::EPathObstacleType type = lhs.GetType ();

	switch (type)
	{
	case CPathObstacle::ePOT_Circle2D:
		return lhs.GetCircle2D () == rhs.GetCircle2D ();
		break;
	case CPathObstacle::ePOT_Shape2D:
		return lhs.GetShape2D () == rhs.GetShape2D ();
		break;
	default:
		AIAssert (0 && "Unhandled obstacle type in operator==(CPathObstacle, CPathObstacle)");
		break;
	}
	return false;
}

inline bool operator== (const TPathObstacles & lhs, const TPathObstacles & rhs)
{
	if (lhs.size () != rhs.size ())
		return false;

	const int numElems = lhs.size	();

	for (int i=0; i < numElems; ++i)
		if (lhs[i] != rhs[i])
			return false;

	return true;
}

// NOTE Jun 4, 2007: <pvl> for consistency checking while debugging
unsigned int SPathObstacleShape2D::GetHash () const
{
	unsigned int hash = 0;
	TVectorOfVectors::const_iterator it = pts.begin ();
	TVectorOfVectors::const_iterator end = pts.end ();
	for ( ; it != end; ++it)
		hash += HashFromVec3 (*it, 0.0f);
	return hash;
}

unsigned int SPathObstacleCircle2D::GetHash () const
{
	return HashFromVec3 (center, 0.0f) + HashFromFloat (radius, 0.0f);
}

unsigned int CPathObstacle::GetHash () const
{
	unsigned int hash = HashFromFloat (float (m_type), 0.0f);

	switch (m_type)
	{
	case CPathObstacle::ePOT_Circle2D:
		return hash + GetCircle2D ().GetHash ();
		break;
	case CPathObstacle::ePOT_Shape2D:
		return hash + GetShape2D ().GetHash ();
		break;
	default:
		AIAssert (0 && "Unhandled obstacle type in CPathObstacle::GetHash()");
		break;
	}
	return 0;
}
/*
static unsigned int GetHash (const TPathObstacles & obstacles)
{
	unsigned int hash = 0;

	TPathObstacles::const_iterator it = obstacles.begin ();
	TPathObstacles::const_iterator end = obstacles.end ();
	for ( ; it != end; ++it)
		hash += (*it)->GetHash ();
	return hash;
}
*/
static void ClearObstacles (TPathObstacles & obstacles)
{
	obstacles.resize(0);
}

static void DeepCopyObstacles (TPathObstacles & dst, const TPathObstacles & src)
{
	for (uint32 i=0; i < src.size(); ++i)
	{
		dst.push_back (new CPathObstacle (*src[i].get()));
	}
}

static bool ObstacleDrawingIsOnForPuppet (const CPuppet * puppet)
{
	if (gAIEnv.CVars.DebugDraw > 0)
	{
		const char *pathName = gAIEnv.CVars.DrawPathAdjustment;
		if (!strcmp(pathName, "all") || (puppet && !strcmp(puppet->GetName(), pathName)))
			return true;
	}
	return false;
}

//===================================================================
// CPathObstacle
//===================================================================
CPathObstacle::CPathObstacle(EPathObstacleType type) : m_type(type) 
{
  switch (m_type)
  {
  case ePOT_Circle2D: m_pData = new SPathObstacleCircle2D; break;
  case ePOT_Shape2D: m_pData = new SPathObstacleShape2D; break;
  case ePOT_Unset: m_pData = 0; break;
  default: m_pData = 0; AIError("CPathObstacle: Unhandled type: %d", type);
  }
}

//===================================================================
// CPathObstacle
//===================================================================
CPathObstacle::~CPathObstacle() 
{
	Free();
}

void CPathObstacle::Free()
{
	if (m_pData)
	{
		switch (m_type)
		{
    case ePOT_Circle2D: delete &GetCircle2D(); break;
    case ePOT_Shape2D: delete &GetShape2D(); break;
		default: AIError("~CPathObstacle Unhandled type: %d", m_type); break;
		}
		m_type = ePOT_Unset;
		m_pData = 0;
	}
}

//===================================================================
// operator=
//===================================================================
CPathObstacle & CPathObstacle::operator=(const CPathObstacle &other)
{
  if (this == &other)
    return *this;
  Free();
  m_type = other.GetType();
  switch (m_type)
  {
  case ePOT_Circle2D:
    m_pData = new SPathObstacleCircle2D; 
    *((SPathObstacleCircle2D *) m_pData) = other.GetCircle2D();
    break;
  case ePOT_Shape2D: 
    m_pData = new SPathObstacleShape2D; 
    *((SPathObstacleShape2D *) m_pData) = other.GetShape2D();
    break;
  default: AIError("CPathObstacle::operator= Unhandled type: %d", m_type); return *this;
  }
  return *this;
}

//===================================================================
// CPathObstacle
//===================================================================
CPathObstacle::CPathObstacle(const CPathObstacle &other)
{
  m_type = ePOT_Unset;
  m_pData = 0;
  *this = other;
}


//===================================================================
// SCachedObstacle
//===================================================================
//#define CHECK_CACHED_OBST_CONSISTENCY
struct SCachedObstacle
{
  void Reset() {
		entity = 0; entityHash = 0; extraRadius = 0.0f; shapes.clear();
#ifdef CHECK_CACHED_OBST_CONSISTENCY
		shapesHash = 0;
#endif
		debugBoxes.clear();
	}

  IPhysicalEntity *entity;
  unsigned entityHash;
  float extraRadius;
  TPathObstacles shapes;
#ifdef CHECK_CACHED_OBST_CONSISTENCY
  unsigned shapesHash;
#endif
  std::vector<CPathObstacles::SDebugBox> debugBoxes;
};

/// In a suit_demonstration level 32 results in about 80% cache hits
int CPathObstacles::s_obstacleCacheSize = 32;
CPathObstacles::TCachedObstacles CPathObstacles::s_cachedObstacles;

//===================================================================
// GetOrClearCachedObstacle
//===================================================================
SCachedObstacle * CPathObstacles::GetOrClearCachedObstacle(IPhysicalEntity *entity, float extraRadius)
{
  if (s_obstacleCacheSize == 0)
    return 0;

  unsigned entityHash = GetHashFromEntities(&entity, 1);

  const TCachedObstacles::reverse_iterator itEnd = s_cachedObstacles.rend();
  const TCachedObstacles::reverse_iterator itBegin = s_cachedObstacles.rbegin();
  for (TCachedObstacles::reverse_iterator it = s_cachedObstacles.rbegin() ; it != itEnd ; ++it)
  {
    SCachedObstacle &cachedObstacle = **it;
    if (cachedObstacle.entity != entity || cachedObstacle.extraRadius != extraRadius)
      continue;
    if (cachedObstacle.entityHash == entityHash)
    {
      s_cachedObstacles.erase(it.base()-1);
      s_cachedObstacles.push_back(&cachedObstacle);
#ifdef CHECK_CACHED_OBST_CONSISTENCY
      if (cachedObstacle.shapesHash != GetHash (cachedObstacle.shapes))
        AIError ("corrupted obstacle!");
#endif
      return &cachedObstacle;
    }
    s_cachedObstacles.erase(it.base()-1);
    return 0;
  }
  return 0;
}

//===================================================================
// AddCachedObstacle
//===================================================================
void CPathObstacles::AddCachedObstacle(struct SCachedObstacle *obstacle)
{
  if (s_cachedObstacles.size() == s_obstacleCacheSize)
  {
    delete s_cachedObstacles.front();
    s_cachedObstacles.erase(s_cachedObstacles.begin());
  }
  if (s_obstacleCacheSize > 0)
    s_cachedObstacles.push_back(obstacle);
}

//===================================================================
// PopOldestCachedObstacle
//===================================================================
struct SCachedObstacle *CPathObstacles::GetNewCachedObstacle()
{
  if (s_obstacleCacheSize == 0)
    return 0;
  if ((int)s_cachedObstacles.size() < s_obstacleCacheSize)
    return new SCachedObstacle();
  SCachedObstacle *obs = s_cachedObstacles.front();
  s_cachedObstacles.erase(s_cachedObstacles.begin());
  obs->Reset();
  return obs;
}


//===================================================================
// CombineObstacleShape2DPair
//===================================================================
static bool CombineObstacleShape2DPair(SPathObstacleShape2D &shape1, SPathObstacleShape2D  &shape2)
{
  if (!Overlap::Polygon_Polygon2D<TVectorOfVectors>(shape1.pts, shape2.pts, &shape1.aabb, &shape2.aabb))
    return false;

  shape2.pts.insert(shape2.pts.end(), shape1.pts.begin(), shape1.pts.end());
  ConvexHull2D(shape1.pts, shape2.pts);
  shape1.CalcAABB();
  return true;
}

//===================================================================
// CombineObstaclePair
// If possible adds the second obstacle (which may become unuseable) to 
// the first and returns true.
// If not possible nothing gets modified and returns false
//===================================================================
static bool CombineObstaclePair(CPathObstaclePtr ob1, CPathObstaclePtr ob2)
{
  if (ob1->GetType() != ob2->GetType())
    return false;
  if (ob1->GetType() == CPathObstacle::ePOT_Shape2D)
    return CombineObstacleShape2DPair(ob1->GetShape2D(), ob2->GetShape2D());
  return false;
}

//===================================================================
// CombineObstacles
//===================================================================
static void CombineObstacles(TPathObstacles &combinedObstacles, const TPathObstacles &obstacles)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);
  combinedObstacles = obstacles;

  for (TPathObstacles::iterator it = combinedObstacles.begin() ; it != combinedObstacles.end() ; ++it)
  {
    if ((*it)->GetType() == CPathObstacle::ePOT_Shape2D)
      (*it)->GetShape2D().CalcAABB();
  }
StartAgain:
  for (TPathObstacles::iterator it = combinedObstacles.begin() ; it != combinedObstacles.end() ; ++it)
  {
    for (TPathObstacles::iterator itOther = it+1 ; itOther != combinedObstacles.end() ; ++itOther)
    {
      if (CombineObstaclePair(*it, *itOther))
      {
        combinedObstacles.erase(itOther);
        goto StartAgain;
      }
    }
  }
}

//===================================================================
// SimplifyObstacle
//===================================================================
static void SimplifyObstacle(CPathObstaclePtr ob)
{
  if (ob->GetType() == CPathObstacle::ePOT_Circle2D)
  {
    static const float diagScale = 1.0f / sqrtf(2.0f);
    const SPathObstacleCircle2D &circle = ob->GetCircle2D();
    CPathObstacle newOb(CPathObstacle::ePOT_Shape2D);
    SPathObstacleShape2D &shape2D = newOb.GetShape2D();
    shape2D.pts.push_back(circle.center + diagScale * Vec3(-circle.radius, -circle.radius, 0.0f));
    shape2D.pts.push_back(circle.center +             Vec3( 0.0f, -circle.radius, 0.0f));
    shape2D.pts.push_back(circle.center + diagScale * Vec3( circle.radius,  -circle.radius, 0.0f));
    shape2D.pts.push_back(circle.center +             Vec3(circle.radius, 0.0f, 0.0f));
    shape2D.pts.push_back(circle.center + diagScale * Vec3(circle.radius, circle.radius, 0.0f));
    shape2D.pts.push_back(circle.center +             Vec3( 0.0f, circle.radius, 0.0f));
    shape2D.pts.push_back(circle.center + diagScale * Vec3( -circle.radius,  circle.radius, 0.0f));
    shape2D.pts.push_back(circle.center +             Vec3(-circle.radius,  0.0f, 0.0f));
    *ob = newOb;
  }
}

//===================================================================
// SimplifyObstacles
//===================================================================
static void SimplifyObstacles(TPathObstacles &obstacles)
{
  for (TPathObstacles::iterator it = obstacles.begin() ; it != obstacles.end() ; ++it)
    SimplifyObstacle(*it);
}

//===================================================================
// DebugDraw
//===================================================================
void CPathObstacles::DebugDraw() const
{
	CDebugDrawContext dc;

  for (unsigned i = 0 ; i < m_debugPathAdjustmentBoxes.size() ; ++i)
  {
    const SDebugBox &box = m_debugPathAdjustmentBoxes[i];
    Matrix34 mat34;
    mat34.SetRotation33(Matrix33(box.q));
    mat34.SetTranslation(box.pos);
    dc->DrawOBB(box.obb, mat34, false, ColorB(0, 1, 0), eBBD_Extremes_Color_Encoded);
		dc->DrawOBB(box.obb, mat34, false, ColorB(0, 1, 0), eBBD_Extremes_Color_Encoded);
	}

  if (gAIEnv.CVars.DebugDraw != 0)
  {
    for (TPathObstacles::const_iterator it = m_simplifiedObstacles.begin() ; it != m_simplifiedObstacles.end() ; ++it)
    {
      ColorF col(1, 1, 0);
      const CPathObstacle &obstacle = **it;
      if (obstacle.GetType() == CPathObstacle::ePOT_Shape2D && !obstacle.GetShape2D().pts.empty())
      {
        const TVectorOfVectors &pts = obstacle.GetShape2D().pts;
        for (unsigned i = 0 ; i < pts.size() ; ++i)
        {
          unsigned j = (i + 1) % pts.size();
          Vec3 v1 = pts[i];
          Vec3 v2 = pts[j];
          v1.z = dc->GetDebugDrawZ(v1, true);
          v2.z = dc->GetDebugDrawZ(v2, true);
          dc->DrawLine( v1, col, v2, col );
        }
      }
    }
    for (TPathObstacles::const_iterator it = m_combinedObstacles.begin() ; it != m_combinedObstacles.end() ; ++it)
    {
      ColorF col(1, 1, 1);
      const CPathObstacle &obstacle = **it;
      if (obstacle.GetType() == CPathObstacle::ePOT_Shape2D && !obstacle.GetShape2D().pts.empty())
      {
        const TVectorOfVectors &pts = obstacle.GetShape2D().pts;
        for (unsigned i = 0 ; i < pts.size() ; ++i)
        {
          unsigned j = (i + 1) % pts.size();
          Vec3 v1 = pts[i];
          Vec3 v2 = pts[j];
          v1.z = dc->GetDebugDrawZ(v1, true);
          v2.z = dc->GetDebugDrawZ(v2, true);
          dc->DrawLine( v1, col, v2, col );
        }
      }
    }
  }
}

//===================================================================
// AddEntityBoxesToObstacles
//===================================================================
bool CPathObstacles::AddEntityBoxesToObstacles(IPhysicalEntity *entity, TPathObstacles &obstacles, float extraRadius, float terrainZ, bool debug) const
{
  static int cacheHits = 0;
  static int cacheMisses = 0;

  static int numPerOutput = 100;
  if (cacheHits + cacheMisses > numPerOutput)
  {
    AILogComment("PathObstacles: cache hits = %d misses = %d", cacheHits, cacheMisses);
    cacheMisses = cacheHits = 0;
  }

  SCachedObstacle *cachedObstacle = GetOrClearCachedObstacle(entity, extraRadius);
  if (cachedObstacle)
  {
		if (debug)
			m_debugPathAdjustmentBoxes.insert(m_debugPathAdjustmentBoxes.end(), cachedObstacle->debugBoxes.begin(), cachedObstacle->debugBoxes.end());

    obstacles.insert(obstacles.end(), cachedObstacle->shapes.begin(), cachedObstacle->shapes.end());
    ++cacheHits;
    return true;
  }
  ++cacheMisses;

  // put the obstacles into a temporary and then combine/copy at the end
  static TPathObstacles pathObstacles;
	ClearObstacles (pathObstacles);

  pe_status_nparts statusNParts;
  int nParts = entity->GetStatus(&statusNParts);
  pe_status_pos statusPos;
  if (0 == entity->GetStatus(&statusPos))
    return false;

  unsigned origDebugBoxesSize = m_debugPathAdjustmentBoxes.size();

  pe_params_part paramsPart;
  for (statusPos.ipart = 0, paramsPart.ipart = 0 ; statusPos.ipart < nParts ; ++statusPos.ipart, ++paramsPart.ipart)
  {
    if (0 == entity->GetParams(&paramsPart))
      continue;
    if (!(paramsPart.flagsAND & geom_colltype_player))
      continue;

    if (0 == entity->GetStatus(&statusPos))
      continue;
    if (!statusPos.pGeomProxy)
      continue;

    primitives::box box;
    statusPos.pGeomProxy->GetBBox(&box);

    box.center *= statusPos.scale;
    box.size *= statusPos.scale;

    Vec3 worldC = statusPos.pos + statusPos.q * box.center;
    Matrix33 worldM = Matrix33(statusPos.q) * box.Basis.GetTransposed();

    static std::vector<Vec3> pts;
    pts.resize(0);
	pts.reserve(64);

    pts.push_back(worldC + worldM * Vec3(box.size.x, box.size.y, box.size.z));
    pts.push_back(worldC + worldM * Vec3(box.size.x, box.size.y, -box.size.z));
    pts.push_back(worldC + worldM * Vec3(box.size.x, -box.size.y, box.size.z));
    pts.push_back(worldC + worldM * Vec3(box.size.x, -box.size.y, -box.size.z));
    pts.push_back(worldC + worldM * Vec3(-box.size.x, box.size.y, box.size.z));
    pts.push_back(worldC + worldM * Vec3(-box.size.x, box.size.y, -box.size.z));
    pts.push_back(worldC + worldM * Vec3(-box.size.x, -box.size.y, box.size.z));
    pts.push_back(worldC + worldM * Vec3(-box.size.x, -box.size.y, -box.size.z));

    float minZ = pts[0].z;
    float maxZ = pts[0].z;
    unsigned nPts = pts.size();
    bool onePointOutside = false;
	const float extraRadiusSin = extraRadius * 0.707106781186f;
    for (unsigned i = 0 ; i < nPts ; ++i)
    {
      // if all OBB points are inside FA then ignore this object. TODO cache this result (also the entire shape?)
      // alongside the transformation matrix as the FA checks or not very cheap.
      if (!onePointOutside && !gAIEnv.pNavigation->IsPointInForbiddenRegion(pts[i], 0, false))
        onePointOutside = true;

      if (pts[i].z > maxZ)
        maxZ = pts[i].z;
      if (pts[i].z < minZ)
        minZ = pts[i].z;
      pts.push_back(pts[i] + Vec3(extraRadius, 0.0f, 0.0f));
      pts.push_back(pts[i] + Vec3(-extraRadius, 0.0f, 0.0f));
      pts.push_back(pts[i] + Vec3(0.0f, extraRadius, 0.0f));
      pts.push_back(pts[i] + Vec3(0.0f, -extraRadius, 0.0f));
      pts.push_back(pts[i] + Vec3(extraRadiusSin, extraRadiusSin, 0.0f));
      pts.push_back(pts[i] + Vec3(-extraRadiusSin, -extraRadiusSin, 0.0f));
      pts.push_back(pts[i] + Vec3(extraRadiusSin, -extraRadiusSin, 0.0f));
      pts.push_back(pts[i] + Vec3(-extraRadiusSin, extraRadiusSin, 0.0f));
    }
    if (!onePointOutside)
      continue;

/*    float extra = 0.5f * min(min(box.size.x, box.size.y), box.size.z);
    const float rScale = sqrt(2.0f);
    minZ += rScale * extra;
    maxZ -= rScale * extra;*/
    if (minZ > terrainZ + criticalBaseAlt)
      continue;
    if (maxZ < terrainZ + criticalTopAlt)
      continue;

    nPts = pts.size();
    for (unsigned i = 0 ; i < nPts ; ++i)
      pts[i].z = terrainZ + 0.1f;

    pathObstacles.push_back(new CPathObstacle(CPathObstacle::ePOT_Shape2D));
    ConvexHull2D(pathObstacles.back()->GetShape2D().pts, pts);
  }

  if (pathObstacles.empty())
    return false;

  static bool combine = true;
  if (combine && pathObstacles.size() > 1)
  {
    static TPathObstacles tempObstacles;
    CombineObstacles(tempObstacles, pathObstacles);
    pathObstacles.swap(tempObstacles);
    ClearObstacles (tempObstacles);
  }
  AIAssert(!pathObstacles.empty());

  obstacles.insert(obstacles.end(), pathObstacles.begin(), pathObstacles.end());

  // update cache
  cachedObstacle = GetNewCachedObstacle();
  if (cachedObstacle)
  {
    cachedObstacle->entity = entity;
    cachedObstacle->entityHash = GetHashFromEntities(&entity, 1);
    cachedObstacle->extraRadius = extraRadius;

    cachedObstacle->shapes.insert(cachedObstacle->shapes.end(), pathObstacles.begin(), pathObstacles.end());
#ifdef CHECK_CACHED_OBST_CONSISTENCY
    cachedObstacle->shapesHash = GetHash (cachedObstacle->shapes);
#endif

		if (debug)
			cachedObstacle->debugBoxes.insert(cachedObstacle->debugBoxes.end(), m_debugPathAdjustmentBoxes.begin() + origDebugBoxesSize, m_debugPathAdjustmentBoxes.end());

    AddCachedObstacle(cachedObstacle);
  }
  // NOTE Jun 3, 2007: <pvl> mostly for debugging - so that pathObstacles
  // doesn't confuse reference counts.  Shouldn't cost more than clearing an
  // already cleared container.  Can be removed if necessary.
  ClearObstacles (pathObstacles);
  return true;
}

//===================================================================
// SSphereSorter
// Arbitrary sort order - but ignores z.
//===================================================================
struct SSphereSorter
{
  bool operator()(const Sphere &lhs, const Sphere &rhs) const
  {
    if (lhs.center.x < rhs.center.x)
      return true;
    else if (lhs.center.x > rhs.center.x)
      return false;
    else if (lhs.center.y < rhs.center.y)
      return true;
    else if (lhs.center.y > rhs.center.y)
      return false;
    else if (lhs.radius < rhs.radius)
      return true;
    else if (lhs.radius > rhs.radius)
      return false;
    else 
      return false;
  }
};

//===================================================================
// SSphereEq
//===================================================================
struct SSphereEq
{
  bool operator()(const Sphere &lhs, const Sphere &rhs) const
  {
    return IsEquivalent(lhs.center, rhs.center, 0.1f) && fabs(lhs.radius - rhs.radius) < 0.1f;
  }
};

//===================================================================
// IsTriangularPt
//===================================================================
inline bool IsTriangularPt(const Vec3 &pt)
{
  int nBuildingID;
  IVisArea *pArea;
  IAISystem::ENavigationType navType = gAIEnv.pNavigation->CheckNavigationType(pt, nBuildingID, pArea, IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_TRIANGULAR);
  return (navType & (IAISystem::NAV_TRIANGULAR | IAISystem::NAV_ROAD)) != 0;
}

//===================================================================
// GetPathObstacles_AIObject
//===================================================================
void CPathObstacles::GetPathObstacles_AIObject(CAIObject *pObject, SPathObstaclesInfo &pathObstaclesInfo) const
{
	assert(pObject);

	CPuppet::ENavInteraction navInteraction = pathObstaclesInfo.pPuppet ? CPuppet::GetNavInteraction(pathObstaclesInfo.pPuppet, pObject) : CPuppet::NI_IGNORE;
	if (navInteraction == CPuppet::NI_STEER)
	{
		static const float maxSpeedSq = square(0.3f);
		if (pObject->GetVelocity().GetLengthSquared() <= maxSpeedSq)
		{
			const Vec3 objectPos = pObject->GetPhysicsPos();
			if (IsTriangularPt(objectPos))
			{
				Vec3 pathPos(ZERO);
				float distAlongPath = 0.0f;
				const float distToPath = pathObstaclesInfo.pNavPath->GetDistToPath(pathPos, distAlongPath, objectPos, pathObstaclesInfo.maxDistToCheckAhead, true);
				if (distToPath >= 0.0f && distToPath <= pathObstaclesInfo.maxPathDeviation)
				{
					//FIXME: Don't allocate at run-time for something like this...
					CPathObstaclePtr pPathObstacle = new CPathObstacle(CPathObstacle::ePOT_Circle2D);
					SPathObstacleCircle2D &pathObstacleCircle2D = pPathObstacle->GetCircle2D();
					pathObstacleCircle2D.center = objectPos;
					pathObstacleCircle2D.radius = pathObstaclesInfo.minAvRadius + 0.5f;

					pathObstaclesInfo.dynamicObstacleSpheres.push_back(Sphere(pathObstacleCircle2D.center, pathObstacleCircle2D.radius));

					pathObstaclesInfo.foundObjectsAABB.Add(objectPos, pathObstaclesInfo.minAvRadius + pObject->GetRadius());
					pathObstaclesInfo.outObstacles.push_back(pPathObstacle);
				}
			}
		}
	}
}

//===================================================================
// GetPathObstacles_Vehicle
//===================================================================
void CPathObstacles::GetPathObstacles_Vehicle(CAIObject *pObject, SPathObstaclesInfo &pathObstaclesInfo) const
{
	assert(pObject);
	
	IPhysicalEntity *pPhysicalEntity = pObject->GetPhysics();
	if (pPhysicalEntity)
	{
		bool bIgnore = false;

		// Ignore all vehicles if we don't care about them, per our avoidance ability, so they are ignored later! (See below)
		if ((pathObstaclesInfo.movementAbility.avoidanceAbilities & eAvoidance_Vehicles) != eAvoidance_Vehicles)
		{
			bIgnore = true;
		}
		else
		{
			static const float maxSpeedSq = square(0.3f);
			CPuppet::ENavInteraction navInteraction = pathObstaclesInfo.pPuppet ? CPuppet::GetNavInteraction(pathObstaclesInfo.pPuppet, pObject) : CPuppet::NI_IGNORE;
			if (navInteraction != CPuppet::NI_STEER || pObject->GetVelocity().GetLengthSquared() > maxSpeedSq)
			{
				bIgnore = true;
			}
		}

		// Ignoring it means we treat is as if it was checked, so physical entity check will skip it later.
		// Not ignoring means we want to use its physical check, so we get an accurate hull shape for it.
		stl::push_back_unique(bIgnore ? pathObstaclesInfo.checkedPhysicsEntities : pathObstaclesInfo.queuedPhysicsEntities, pPhysicalEntity);
	}
}

//===================================================================
// GetPathObstacles_PhysicalEntity
//===================================================================
void CPathObstacles::GetPathObstacles_PhysicalEntity(IPhysicalEntity *pEntity, SPathObstaclesInfo &pathObstaclesInfo, bool bIsPushable, bool bDebug) const
{
	assert(pEntity);

	if (pEntity && !stl::find(pathObstaclesInfo.checkedPhysicsEntities, pEntity))
	{
		pe_params_bbox params_bbox;
		pEntity->GetParams(&params_bbox);

		const Vec3 mid = 0.5f * (params_bbox.BBox[0] + params_bbox.BBox[1]);
		if (IsTriangularPt(mid))
		{
			I3DEngine *p3DEngine = gEnv->p3DEngine;
			assert(p3DEngine);

			const float terrainZ = p3DEngine->GetTerrainElevation(mid.x, mid.y);
			const float altTop = params_bbox.BBox[1].z - terrainZ;
			const float altBase = params_bbox.BBox[0].z - terrainZ;

			if (altTop >= criticalTopAlt && altBase <= criticalBaseAlt)
			{
				const float boxRadius = 0.5f * Distance::Point_Point(params_bbox.BBox[0], params_bbox.BBox[1]);

				Vec3 pathPos(ZERO);
				float distAlongPath = 0.0f;
				const float distToPath = pathObstaclesInfo.pNavPath->GetDistToPath(pathPos, distAlongPath, mid, pathObstaclesInfo.maxDistToCheckAhead, true);
				if (distToPath >= 0.0f && distToPath <= pathObstaclesInfo.maxPathDeviation)
				{
					const bool obstacleIsSmall = boxRadius < gAIEnv.CVars.ObstacleSizeThreshold;
					const int puppetType = pathObstaclesInfo.pPuppet ? pathObstaclesInfo.pPuppet->GetType() : AIOBJECT_PUPPET;

					float extraRadius = (puppetType == AIOBJECT_VEHICLE && obstacleIsSmall ? gAIEnv.CVars.ExtraVehicleAvoidanceRadiusSmall : pathObstaclesInfo.minAvRadius);
					if (bIsPushable && pathObstaclesInfo.movementAbility.pushableObstacleWeakAvoidance)
					{
						// Partial avoidance - scale down radius
						extraRadius = pathObstaclesInfo.movementAbility.pushableObstacleAvoidanceRadius;
					}

					if (AddEntityBoxesToObstacles(pEntity, pathObstaclesInfo.outObstacles, extraRadius, terrainZ, bDebug))
					{
						pathObstaclesInfo.foundObjectsAABB.Add(mid, pathObstaclesInfo.minAvRadius + boxRadius);
						pathObstaclesInfo.dynamicObstacleSpheres.push_back(Sphere(mid, boxRadius));
					}
				}
			}
		}
	}
}

//===================================================================
// GetPathObstacles_DamageRegion
//===================================================================
void CPathObstacles::GetPathObstacles_DamageRegion(const Sphere &damageRegionSphere, SPathObstaclesInfo &pathObstaclesInfo) const
{
	Vec3 pathPos(ZERO);
	float distAlongPath = 0.0f;
	float distToPath = pathObstaclesInfo.pNavPath->GetDistToPath(pathPos, distAlongPath, damageRegionSphere.center, pathObstaclesInfo.maxDistToCheckAhead, true);
	if (distToPath >= 0.0f && distToPath <= pathObstaclesInfo.maxPathDeviation)
	{
		static float safetyRadius = 2.0f;
		const float actualRadius = safetyRadius + damageRegionSphere.radius * 1.5f; // game code actually uses AABB

		//FIXME: Don't allocate at run-time for something like this...
		CPathObstaclePtr pPathObstacle = new CPathObstacle(CPathObstacle::ePOT_Circle2D);
		SPathObstacleCircle2D &pathObstacleCircle2D = pPathObstacle->GetCircle2D();
		pathObstacleCircle2D.center = damageRegionSphere.center;
		pathObstacleCircle2D.radius = actualRadius;

		pathObstaclesInfo.dynamicObstacleSpheres.push_back(Sphere(damageRegionSphere.center, damageRegionSphere.radius * 1.5f));

		pathObstaclesInfo.foundObjectsAABB.Add(damageRegionSphere.center, actualRadius);
		pathObstaclesInfo.outObstacles.push_back(pPathObstacle);
	}
}

//===================================================================
// GetPathObstacles_NearObjects
//===================================================================
void CPathObstacles::GetPathObstacles_NearObjects(SPathObstaclesInfo &pathObstaclesInfo) const
{
	CAISystem *pAISystem = GetAISystem();
	assert(pAISystem);

	Vec3 terrainPos = pathObstaclesInfo.foundObjectsAABB.GetCenter();
	terrainPos.z = gEnv->p3DEngine->GetTerrainElevation(terrainPos.x, terrainPos.y);
	pathObstaclesInfo.foundObjectsAABB.min.z = pathObstaclesInfo.foundObjectsAABB.max.z = terrainPos.z;

	static MapConstNodesDistance nodesInRange;
	nodesInRange.clear();

	static std::vector<int> addedObstacleIndices;
	addedObstacleIndices.clear();

	unsigned lastNodeIndex = 0;//pPuppet->m_lastNavNodeIndex;
	gAIEnv.pGraph->GetNodesInRange(nodesInRange, terrainPos, pathObstaclesInfo.foundObjectsAABB.GetRadius(), IAISystem::NAV_TRIANGULAR, 
		pathObstaclesInfo.movementAbility.pathRadius, lastNodeIndex, 0);

	MapConstNodesDistance::const_iterator itNodes = nodesInRange.begin();
	MapConstNodesDistance::const_iterator itNodesEnd = nodesInRange.end();
	for (; itNodes != itNodesEnd; ++itNodes)
	{
		const GraphNode *pNode = itNodes->first;
		assert(pNode);

		if (pNode && pNode->navType == IAISystem::NAV_TRIANGULAR)
		{
			const STriangularNavData *pTriangularNavData = pNode->GetTriangularNavData();
			ObstacleIndexVector::const_iterator itObstacleIndex = pTriangularNavData->vertices.begin();
			ObstacleIndexVector::const_iterator itObstacleIndexEnd = pTriangularNavData->vertices.end();
			for (; itObstacleIndex != itObstacleIndexEnd; ++itObstacleIndex)
			{
				int index = *itObstacleIndex;
				const ObstacleData &od = pAISystem->m_VertexList.GetVertex(index);
				if (od.fApproxRadius > 0.0f && od.IsCollidable())
				{
					addedObstacleIndices.push_back(index);
				}
			}
		}
	}
	std::sort(addedObstacleIndices.begin(), addedObstacleIndices.end());

	int prevIndex = -1;
	for (uint32 i = 0, ni = addedObstacleIndices.size(); i < ni; ++i)
	{
		const int index = addedObstacleIndices[i];
		if (index != prevIndex)
		{
			const ObstacleData &od = GetAISystem()->m_VertexList.GetVertex(index);

			Vec3 pathPos(ZERO);
			float distAlongPath = 0.0f;
			float distToPath = pathObstaclesInfo.pNavPath->GetDistToPath(pathPos, distAlongPath, od.vPos, pathObstaclesInfo.maxDistToCheckAhead, true);
			if (distToPath >= 0.0f && distToPath <= pathObstaclesInfo.maxPathDeviation)
			{
				bool bNearDynamicObstacleSphere = false;
				TDynamicObstacleSpheres::const_iterator itDynamicObstacleSphere = pathObstaclesInfo.dynamicObstacleSpheres.begin();
				TDynamicObstacleSpheres::const_iterator itDynamicObstacleSphereEnd = pathObstaclesInfo.dynamicObstacleSpheres.end();
				for (; itDynamicObstacleSphere != itDynamicObstacleSphereEnd; ++itDynamicObstacleSphere)
				{
					const Sphere &dynamicObstacleSphere = *itDynamicObstacleSphere;
					const float distSq = Distance::Point_Point2DSq(dynamicObstacleSphere.center, od.vPos);
					if (distSq < square(dynamicObstacleSphere.radius + od.fApproxRadius + 2.0f * pathObstaclesInfo.minAvRadius))
					{
						bNearDynamicObstacleSphere = true;
						break;
					}
				}

				if (bNearDynamicObstacleSphere && !gAIEnv.pNavigation->IsPointInForbiddenRegion(od.vPos, 0, false))
				{
					//FIXME: Don't allocate at run-time for something like this...
					CPathObstaclePtr pPathObstacle = new CPathObstacle(CPathObstacle::ePOT_Circle2D);
					SPathObstacleCircle2D &pathObstacleCircle2D = pPathObstacle->GetCircle2D();
					pathObstacleCircle2D.center.Set(od.vPos.x, od.vPos.y, 0.0f);
					pathObstacleCircle2D.radius = od.fApproxRadius + pathObstaclesInfo.minAvRadius;

					pathObstaclesInfo.outObstacles.push_back(pPathObstacle);
				}
			}
		}
		prevIndex = index;
	}
}

//===================================================================
// GetPathObstacles
//===================================================================
void CPathObstacles::GetPathObstacles(TPathObstacles &obstacles, const AgentMovementAbility& movementAbility, const CNavPath* pNavPath, const CPuppet* pPuppet)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_AI);
	AIAssert(pNavPath);

	CAISystem *pAISystem = GetAISystem();

	ClearObstacles(obstacles);

	if (gAIEnv.CVars.CrowdControlInPathfind == 0 || movementAbility.pathRegenIntervalDuringTrace <= FLT_EPSILON)
		return;

	IAISystem::ENavigationType navType = (pNavPath->Empty() ? IAISystem::NAV_UNSET : pNavPath->GetPath().front().navType);
	if (navType & (IAISystem::NAV_FLIGHT | IAISystem::NAV_VOLUME | IAISystem::NAV_UNSET))
		return;

	// The information relating to this path obstacles calculation
	static const float maxPathDeviation = 15.0f;
	SPathObstaclesInfo pathObstaclesInfo(obstacles, movementAbility, pNavPath, pPuppet, maxPathDeviation);

	const bool bDebug = ObstacleDrawingIsOnForPuppet(pPuppet); 
	if (bDebug)
		m_debugPathAdjustmentBoxes.resize(0);

	static const float minPuppetAvRadius = 0.5f;
	const float minVehicleAvRadius = gAIEnv.CVars.ExtraVehicleAvoidanceRadiusBig;

	const int puppetType = pPuppet ? pPuppet->GetType() : AIOBJECT_PUPPET;
	pathObstaclesInfo.minAvRadius = max((puppetType != AIOBJECT_PUPPET ? minVehicleAvRadius : minPuppetAvRadius), movementAbility.pathRadius);

	static const float maxSpeedScale = 1.0f;
	pathObstaclesInfo.maxSpeed = movementAbility.movementSpeeds.GetRange(AgentMovementSpeeds::AMS_COMBAT, AgentMovementSpeeds::AMU_RUN).max;
	pathObstaclesInfo.maxDistToCheckAhead = maxSpeedScale * pathObstaclesInfo.maxSpeed * movementAbility.pathRegenIntervalDuringTrace;

	// Check puppets
	if (movementAbility.avoidanceAbilities & eAvoidance_Puppets)
	{
		const CAISystem::PuppetSet& enabledPuppetsSet = pAISystem->GetEnabledPuppetSet();
		for (CAISystem::PuppetSet::const_iterator it = enabledPuppetsSet.begin(), itend = enabledPuppetsSet.end(); it != itend; ++it)
		{
			CAIObject *pObject = it->GetAIObject();
			assert(pObject);

			if (pObject && pObject != pPuppet && pObject->GetType() == AIOBJECT_PUPPET)
				GetPathObstacles_AIObject(pObject, pathObstaclesInfo);
		}
	}

	// Check vehicles (which are also physical entities)
	{
		AutoAIObjectIter pVehicleIter = pAISystem->GetFirstAIObject(IAISystem::OBJFILTER_TYPE, AIOBJECT_VEHICLE);
		while (IAIObject *pObject = pVehicleIter->GetObject())
		{
			CAIObject *pCObject = (CAIObject*)pObject;
			assert(pCObject && pCObject->GetType() == AIOBJECT_VEHICLE);

			if (pCObject && pCObject != pPuppet)
				GetPathObstacles_Vehicle(pCObject, pathObstaclesInfo);

			pVehicleIter->Next();
		}
	}

	// Check players
	if (movementAbility.avoidanceAbilities & eAvoidance_Players)
	{
		AutoAIObjectIter pPlayerIter = pAISystem->GetFirstAIObject(IAISystem::OBJFILTER_TYPE, AIOBJECT_PLAYER);
		while (IAIObject *pObject = pPlayerIter->GetObject())
		{
			CAIObject *pCObject = (CAIObject*)pObject;
			assert(pCObject && pCObject->GetType() == AIOBJECT_PLAYER);

			if (pCObject && pCObject != pPuppet)
				GetPathObstacles_AIObject(pCObject, pathObstaclesInfo);

			pPlayerIter->Next();
		}
	}

	// Check dynamic physical entities
	{
		AABB obstacleAABB = pNavPath->GetAABB(pathObstaclesInfo.maxDistToCheckAhead);
		obstacleAABB.min -= Vec3(maxPathDeviation, maxPathDeviation, maxPathDeviation);
		obstacleAABB.max += Vec3(maxPathDeviation, maxPathDeviation, maxPathDeviation);

		PhysicalEntityListAutoPtr pEntities;
		const uint32 nEntityCount = GetEntitiesFromAABB(pEntities, obstacleAABB, AICE_DYNAMIC);
		for (uint32 nEntity = 0; nEntity < nEntityCount; ++nEntity)
		{
			IPhysicalEntity *pEntity = pEntities[nEntity];
			assert(pEntity);

			if (pEntity)
			{
				pe_params_flags params_flags;
				pEntity->GetParams(&params_flags);
				const bool bIsPushable = ((params_flags.flags & pef_pushable_by_players) == pef_pushable_by_players);

				if ((bIsPushable && (movementAbility.avoidanceAbilities & eAvoidance_PushableObstacle) == eAvoidance_PushableObstacle) || 
					(!bIsPushable && (movementAbility.avoidanceAbilities & eAvoidance_StaticObstacle) == eAvoidance_StaticObstacle) ||
					stl::find(pathObstaclesInfo.queuedPhysicsEntities, pEntity))
				{
					GetPathObstacles_PhysicalEntity(pEntity, pathObstaclesInfo, bIsPushable, bDebug);
				}
			}
		}
	}

	// Check damage regions
	if (movementAbility.avoidanceAbilities & eAvoidance_DamageRegion)
	{
		const CAISystem::TDamageRegions& damageRegions = pAISystem->GetDamageRegions();
		if (!damageRegions.empty())
		{
			CAISystem::TDamageRegions::const_iterator itDamageRegion = damageRegions.begin();
			CAISystem::TDamageRegions::const_iterator itDamageRegionEnd = damageRegions.end();
			for (; itDamageRegion != itDamageRegionEnd; ++itDamageRegion)
			{
				const Sphere &damageRegionSphere = itDamageRegion->second;
				GetPathObstacles_DamageRegion(damageRegionSphere, pathObstaclesInfo);
			}
		}
	}

	// obstacles close to the entity obstacles
	if (!pathObstaclesInfo.foundObjectsAABB.IsReset())
	{
		GetPathObstacles_NearObjects(pathObstaclesInfo);
	}
}


//===================================================================
// CPathObstacles
//===================================================================
CPathObstacles::CPathObstacles()
: m_lastCalculateTime(0.0f), m_lastCalculatePos(ZERO), m_lastCalculatePathVersion(-1)
{
}

//===================================================================
// CPathObstacles
//===================================================================
CPathObstacles::~CPathObstacles()
{
}

//===================================================================
// CalculateObstaclesAroundPuppet
//===================================================================
void CPathObstacles::CalculateObstaclesAroundPuppet(const CPuppet *pPuppet)
{
  static float criticalDist = 2.0f;
  static float criticalTime = 2.0f;
  Vec3 puppetPos = pPuppet->GetPos();
  float dt = (GetAISystem()->GetFrameStartTime() - m_lastCalculateTime).GetSeconds();
  // Danny todo: this is/would be a nice optimisation to make - however when finding hidespots
  // we get here before a path is calculated, so the path obstacle list is invalid (since it 
  // depends on the path).
  if (dt < criticalTime && pPuppet->m_Path.GetVersion() == m_lastCalculatePathVersion && 
    puppetPos.IsEquivalent(m_lastCalculatePos, criticalDist))
    return;

  m_lastCalculatePos = puppetPos;
  m_lastCalculateTime = GetAISystem()->GetFrameStartTime();
  m_lastCalculatePathVersion = pPuppet->m_Path.GetVersion();

  ClearObstacles (m_combinedObstacles);
  ClearObstacles (m_simplifiedObstacles);

  GetPathObstacles(m_simplifiedObstacles, pPuppet->m_movementAbility, &pPuppet->m_Path, pPuppet);
  SimplifyObstacles(m_simplifiedObstacles);

  // NOTE Mai 24, 2007: <pvl> CombineObstacles() messes up its second argument
  // so if we're going to debug draw simplified obstacles later, we need to
  // keep m_simplifiedObstacles intact by passing a copy to CombineObstacles().
  // TPathObstacles is a container of smart pointers so we need to perform
  // a proper deep copy.
  // UPDATE Jun 4, 2007: <pvl> in fact, both arguments of CombineObstacleShape2DPair()
  // can get changed (the first one gets the convex hull of both and the second one
  // contains all the points).  So if any obstacle in m_simplifiedObstacles
  // comes from the cache and overlaps with anything else, it will be corrupted.
  // So let's just perform a deep copy here every time.
  if (true/*ObstacleDrawingIsOnForPuppet (pPuppet)*/)
  {
    TPathObstacles tempSimplifiedObstacles;
    DeepCopyObstacles (tempSimplifiedObstacles, m_simplifiedObstacles);
    // ATTN Jun 1, 2007: <pvl> costly!  Please remind me to remove this if I forget somehow! :)
    //AIAssert (tempSimplifiedObstacles == m_simplifiedObstacles);
    CombineObstacles(m_combinedObstacles, tempSimplifiedObstacles);
  } else {
    CombineObstacles(m_combinedObstacles, m_simplifiedObstacles);
  }
}


//===================================================================
// CalculateObstaclesAroundPuppet
//===================================================================
void CPathObstacles::CalculateObstaclesAroundLocation(const Vec3& location, const AgentMovementAbility& movementAbility, const CNavPath* pNavPath)
{
	static float criticalDist = 2.0f;
	static float criticalTime = 2.0f;
	float dt = (GetAISystem()->GetFrameStartTime() - m_lastCalculateTime).GetSeconds();
	// Danny todo: this is/would be a nice optimisation to make - however when finding hidespots
	// we get here before a path is calculated, so the path obstacle list is invalid (since it 
	// depends on the path).
	if (dt < criticalTime && pNavPath->GetVersion() == m_lastCalculatePathVersion && 
		location.IsEquivalent(m_lastCalculatePos, criticalDist))
		return;

	m_lastCalculatePos = location;
	m_lastCalculateTime = GetAISystem()->GetFrameStartTime();
	m_lastCalculatePathVersion = pNavPath->GetVersion();

	ClearObstacles (m_combinedObstacles);
	ClearObstacles (m_simplifiedObstacles);

	GetPathObstacles(m_simplifiedObstacles, movementAbility, pNavPath, 0);
	SimplifyObstacles(m_simplifiedObstacles);

	// NOTE Mai 24, 2007: <pvl> CombineObstacles() messes up its second argument
	// so if we're going to debug draw simplified obstacles later, we need to
	// keep m_simplifiedObstacles intact by passing a copy to CombineObstacles().
	// TPathObstacles is a container of smart pointers so we need to perform
	// a proper deep copy.
	// UPDATE Jun 4, 2007: <pvl> in fact, both arguments of CombineObstacleShape2DPair()
	// can get changed (the first one gets the convex hull of both and the second one
	// contains all the points).  So if any obstacle in m_simplifiedObstacles
	// comes from the cache and overlaps with anything else, it will be corrupted.
	// So let's just perform a deep copy here every time.
	if (true/*ObstacleDrawingIsOnForPuppet (pPuppet)*/)
	{
		TPathObstacles tempSimplifiedObstacles;
		DeepCopyObstacles (tempSimplifiedObstacles, m_simplifiedObstacles);
		// ATTN Jun 1, 2007: <pvl> costly!  Please remind me to remove this if I forget somehow! :)
		//AIAssert (tempSimplifiedObstacles == m_simplifiedObstacles);
		CombineObstacles(m_combinedObstacles, tempSimplifiedObstacles);
	} else {
		CombineObstacles(m_combinedObstacles, m_simplifiedObstacles);
	}
}

//===================================================================
// IsPointInsideObstacles
//===================================================================
bool CPathObstacles::IsPointInsideObstacles(const Vec3 &pt) const
{
  for (TPathObstacles::const_iterator it = m_combinedObstacles.begin() ; it != m_combinedObstacles.end() ; ++it)
  {
    const CPathObstacle &ob = **it;
    if (ob.GetType() != CPathObstacle::ePOT_Shape2D)
      continue;

    const SPathObstacleShape2D &shape2D = ob.GetShape2D();
    if (shape2D.pts.empty())
      continue;

    if (Overlap::Point_Polygon2D(pt, shape2D.pts, &shape2D.aabb))
      return true;
  }
  return false;
}

//===================================================================
// IsPathIntersectingObstacles
//===================================================================
bool CPathObstacles::IsPathIntersectingObstacles(const Vec3 &start, const Vec3 &end, float radius) const
{
	bool bResult = false;

	const Lineseg lineseg(start, end);
	const float radiusSq = radius*radius;
	for (TPathObstacles::const_iterator it = m_combinedObstacles.begin() ; it != m_combinedObstacles.end() ; ++it)
	{
		const CPathObstacle &ob = **it;
		if (ob.GetType() != CPathObstacle::ePOT_Shape2D)
			continue;

		//TODO: Investigate - can we make sure no empty shapes are stored in the container, to avoid these checks?
		const SPathObstacleShape2D &shape2D = ob.GetShape2D();
		if (shape2D.pts.empty())
			continue;

		Vec3 vIntersectionPoint(ZERO);
		if (Overlap::Lineseg_AABB2D(lineseg, shape2D.aabb) && 
			Intersect::Lineseg_Polygon2D(lineseg, shape2D.pts, vIntersectionPoint))
		{
			// Ignore if shape is within our pass radius or the intersection is close enough to the end
			bool bValid = true;
			if (radius > FLT_EPSILON)
			{
				const float fDistanceToEnd = end.GetSquaredDistance2D(vIntersectionPoint);
				bValid = (fDistanceToEnd > radiusSq || !Overlap::Sphere_AABB2D(Sphere(start, radius), shape2D.aabb));
			}
			
			if (bValid)
			{
				bResult = true;
				break;
			}
		}
	}

	return bResult;
}

//===================================================================
// GetPointOutsideObstacles
//===================================================================
Vec3 CPathObstacles::GetPointOutsideObstacles(const Vec3 &pt, float extraDist) const
{
  Vec3 newPos = pt;

  for (TPathObstacles::const_iterator it = m_combinedObstacles.begin() ; it != m_combinedObstacles.end() ; ++it)
  {
    const CPathObstacle &ob = **it;
    if (ob.GetType() != CPathObstacle::ePOT_Shape2D)
      continue;

    const SPathObstacleShape2D &shape2D = ob.GetShape2D();
    if (shape2D.pts.empty())
      continue;

    if (Overlap::Point_Polygon2D(newPos, shape2D.pts, &shape2D.aabb))
    {
      (void) Distance::Point_Polygon2DSq(newPos, shape2D.pts, newPos);
      newPos.z = pt.z;
      Vec3 polyMidPoint = std::accumulate(shape2D.pts.begin(), shape2D.pts.end(), Vec3(ZERO)) / (float)shape2D.pts.size();
      Vec3 dir = (newPos - polyMidPoint);
      dir.z = 0.0f;
      dir.NormalizeSafe();
      newPos += extraDist * dir;
    }
  }
  return newPos;
}

//===================================================================
// Reset
//===================================================================
void CPathObstacles::Reset()
{
  while (!s_cachedObstacles.empty())
  {
    delete s_cachedObstacles.back();
    s_cachedObstacles.pop_back();
  }
}
