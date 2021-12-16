#ifndef TRIANGULAR_NAVREGION_H
#define TRIANGULAR_NAVREGION_H

#if _MSC_VER > 1000
#pragma once
#endif

#include "NavRegion.h"

class CGraph;
class CVertexList;
class SimplifiedLink;

/// Handles all graph operations that relate to the outdoor/triangulation aspect
class CTriangularNavRegion : public CNavRegion
{
public:
  CTriangularNavRegion(CGraph* pGraph, CVertexList *pVertexList);
  virtual ~CTriangularNavRegion() {}

  /// inherited
  virtual unsigned GetEnclosing(const Vec3 &pos, float passRadius = 0.0f, unsigned startIndex = 0, 
    float range = -1.0f, Vec3 * closestValid = 0, bool returnSuspect = false, const char *requesterName = "");

  /// Serialise the _modifications_ since load-time
  virtual void Serialize(TSerialize ser, class CObjectTracker& objectTracker);

  /// inherited
  virtual void Clear();

  /// inherited
  virtual void Reset(IAISystem::EResetReason reason);

  /// inherited
  size_t MemStats();

private:
  // when it works this should be quicker than GreedyStep2... but it sometimes fails us
  unsigned GreedyStep1(unsigned beginIndex, const Vec3 & pos);
  unsigned GreedyStep2(unsigned beginIndex, const Vec3 & pos);

  /// If the point lies inside the obstacles at the triangle corners then it gets moved
  /// back into the triangle body
  Vec3 GetGoodPositionInTriangle(const GraphNode *pNode, const Vec3 &pos) const;

  CGraph * m_pGraph;
	CVertexList * m_pVertexList;
};

#endif
