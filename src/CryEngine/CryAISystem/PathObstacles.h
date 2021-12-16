/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   PathObstacles.h
$Id$
Description: interface for the CGraph class.

-------------------------------------------------------------------------
History:
- ?
- 4 May 2009  : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#ifndef PATHOBSTACLE_H
#define PATHOBSTACLE_H

#if _MSC_VER > 1000
#pragma once
#endif

#include "platform.h"
#include "AILog.h"
#include "smartptr.h"

#include <vector>
#include <set>

typedef std::vector<Vec3> TVectorOfVectors;

//===================================================================
// SPathObstacleShape2D
//===================================================================
struct SPathObstacleShape2D
{
  SPathObstacleShape2D() {aabb.Reset();}
  void CalcAABB() {if (pts.empty()) aabb.Reset(); else aabb = AABB(&pts[0], pts.size());}
	/// Debugging consistency check.
  unsigned int GetHash () const;

  TVectorOfVectors pts;
  AABB aabb;
};

// NOTE Jun 1, 2007: <pvl> used just for debugging ATM
bool operator== (const SPathObstacleShape2D & lhs, const SPathObstacleShape2D & rhs);

//===================================================================
// SPathObstacleCircle2D
//===================================================================
struct SPathObstacleCircle2D
{
	/// Debugging consistency check.
  unsigned int GetHash () const;

  Vec3 center;
  float radius;
};

// NOTE Jun 1, 2007: <pvl> used just for debugging ATM
bool operator== (const SPathObstacleCircle2D & lhs, const SPathObstacleCircle2D & rhs);

//===================================================================
// CPathObstacle
//===================================================================
class CPathObstacle : public _reference_target_t
{
public:
  enum EPathObstacleType
  {
    ePOT_Unset,
    ePOT_Circle2D,
    ePOT_Shape2D,
    ePOT_Sphere3D
  };

  CPathObstacle(EPathObstacleType type = ePOT_Unset);
  ~CPathObstacle();
  void Free();
  CPathObstacle &operator=(const CPathObstacle &other);
  CPathObstacle(const CPathObstacle &other);

  EPathObstacleType GetType() const {return m_type;}
  SPathObstacleShape2D &GetShape2D() {AIAssert(m_type == ePOT_Shape2D); return *((SPathObstacleShape2D *) m_pData);}
  const SPathObstacleShape2D &GetShape2D() const {AIAssert(m_type == ePOT_Shape2D); return *((SPathObstacleShape2D *) m_pData);}
  SPathObstacleCircle2D &GetCircle2D() {AIAssert(m_type == ePOT_Circle2D); return *((SPathObstacleCircle2D *) m_pData);}
  const SPathObstacleCircle2D &GetCircle2D() const {AIAssert(m_type == ePOT_Circle2D); return *((SPathObstacleCircle2D *) m_pData);}

	/// Debugging consistency check.
  unsigned int GetHash () const;

private:
  EPathObstacleType m_type;
  void * m_pData;
};

// NOTE Jun 1, 2007: <pvl> used just for debugging ATM
bool operator== (const CPathObstacle & lhs, const CPathObstacle & rhs);

typedef _smart_ptr<CPathObstacle> CPathObstaclePtr;

// NOTE Jun 1, 2007: <pvl> used just for debugging ATM
// NOTE Jun 1, 2007: <pvl> we need deep comparison here, not just the pointer
// values as the generic version of operator==() does.  Overloading global
// operator==() couldn't cut it, it just created ambiguity with the member
// version.  Thus specialization.
template <>
inline bool _smart_ptr<CPathObstacle>::operator== (const _smart_ptr<CPathObstacle> & rhs) const
{
	return *p == *rhs.p;
}
template <>
inline bool _smart_ptr<CPathObstacle>::operator!= (const _smart_ptr<CPathObstacle> & rhs) const
{
	return ! (*this == rhs);
}

typedef std::vector<CPathObstaclePtr> TPathObstacles;

inline bool operator== (const TPathObstacles & lhs, const TPathObstacles & rhs);


class CNavPath;

//===================================================================
// CPathObstacles
//===================================================================
class CPathObstacles
{
public:
  CPathObstacles();
  ~CPathObstacles();

  /// collect all the obstacles around the puppet. If this is called in quick succession
  /// when the puppet hasn't moved then the old result will be used
  void CalculateObstaclesAroundPuppet(const class CPuppet *pPuppet);
	void CalculateObstaclesAroundLocation(const Vec3& location, const AgentMovementAbility& movementAbility, const CNavPath* pNavPath);

  /// Gets the result
  const TPathObstacles &GetCombinedObstacles() const {return m_combinedObstacles;}

  /// Indicates if pt is inside the obstacles
  bool IsPointInsideObstacles(const Vec3 &pt) const;

  /// Indicates if path crosses through obstacles
  bool IsPathIntersectingObstacles(const Vec3 &start, const Vec3 &end, float radius) const;

  /// If pt is inside the combined obstacles this returns a new pt approx extraDist 
  /// outside the combined obstacles (but beware it may be pushed into another obstacle!). Otherwise
  /// just returns pt.
  Vec3 GetPointOutsideObstacles(const Vec3 &pt, float extraDist) const;

  static void Reset();

private:
  typedef std::vector<IPhysicalEntity*> TCheckedPhysicalEntities;
  typedef std::vector<Sphere> TDynamicObstacleSpheres;
  struct SPathObstaclesInfo
  {
	float minAvRadius;
	float maxSpeed;
	float maxDistToCheckAhead;
	float maxPathDeviation;

	const CNavPath *pNavPath;
	const CPuppet *pPuppet;
	const AgentMovementAbility &movementAbility;
	
	AABB foundObjectsAABB;

	TPathObstacles &outObstacles;
	TCheckedPhysicalEntities queuedPhysicsEntities;
	TCheckedPhysicalEntities checkedPhysicsEntities;
	TDynamicObstacleSpheres dynamicObstacleSpheres;

	SPathObstaclesInfo(TPathObstacles &_obstacles, const AgentMovementAbility &_movementAbility, const CNavPath *_pNavPath, const CPuppet *_pPuppet, float _maxPathDeviation) : 
		minAvRadius(0.0f), maxSpeed(0.0f), maxDistToCheckAhead(0.0f), maxPathDeviation(_maxPathDeviation), pNavPath(_pNavPath), pPuppet(_pPuppet), 
		movementAbility(_movementAbility), foundObjectsAABB(AABB::RESET), outObstacles(_obstacles)
	{

	}
  };

  void GetPathObstacles(TPathObstacles &obstacles, const AgentMovementAbility& movementAbility, const CNavPath* pNavPath, const CPuppet* pPuppet);
  void GetPathObstacles_AIObject(CAIObject *pObject, SPathObstaclesInfo &pathObstaclesInfo) const;
  void GetPathObstacles_Vehicle(CAIObject *pObject, SPathObstaclesInfo &pathObstaclesInfo) const;
  void GetPathObstacles_PhysicalEntity(IPhysicalEntity *pEntity, SPathObstaclesInfo &pathObstaclesInfo, bool bIsPushable, bool bDebug) const;
  void GetPathObstacles_DamageRegion(const Sphere &damageRegionSphere, SPathObstaclesInfo &pathObstaclesInfo) const;
  void GetPathObstacles_NearObjects(SPathObstaclesInfo &pathObstaclesInfo) const;
  
  /// These are only used during path adjustment. However, if we were told to store debug then they won't 
  /// be cleared afterwards, and can get drawn using DrawPathAdjustmentShapes
  TPathObstacles m_combinedObstacles;
  TPathObstacles m_simplifiedObstacles;

  /// time that CalculateObstaclesAroundPuppet was last called (and a cached result wasn't used)
  CTimeValue m_lastCalculateTime;
  /// Our position when CalculateObstaclesAroundPuppet was last called (and a cached result wasn't used)
  Vec3 m_lastCalculatePos;
  /// version number of the path when last called (since the result depends on the path)
  int m_lastCalculatePathVersion;

  /// size of the cache of obstacle shapes
  static int s_obstacleCacheSize;
  typedef std::vector<struct SCachedObstacle *> TCachedObstacles;
  /// cached obstacle shapes - so that the most recently used is at the back.
  static TCachedObstacles s_cachedObstacles;

  /// If there is a cached record for entity/extraRadius and its position matches the entity position then it will 
  /// be returned, and the cache reordered.
  /// If the cached entity is found and its position doesn't match the cache will be removed and 0 returned
  /// If the cached entity isn't found 0 will be returned.
  static struct SCachedObstacle *GetOrClearCachedObstacle(IPhysicalEntity *entity, float extraRadius);
  /// pushes a obstacle into the cache, and takes ownership of it. obstacle should have been created
  /// on the heap
  static void AddCachedObstacle(struct SCachedObstacle *obstacle);
  static struct SCachedObstacle *GetNewCachedObstacle();
public:
  // debug for path adjustment
  bool AddEntityBoxesToObstacles(IPhysicalEntity *entity, TPathObstacles &obstacles, float extraRadius, float terrainZ, bool debug) const;
  void DebugDraw() const;
  struct SDebugBox
  {
    OBB obb;
    Quat q;
    Vec3 pos;
  };
  mutable std::vector<SDebugBox> m_debugPathAdjustmentBoxes;
};


#endif