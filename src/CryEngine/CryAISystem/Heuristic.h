// Heuristic.h: interface for the CHeuristic class.
//
//////////////////////////////////////////////////////////////////////

#ifndef HEURISTIC_H
#define HEURISTIC_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AStarSolver.h"

class CGraph;
struct GraphNode;

class CLayeredNavMeshHeuristic;

/// Just uses distance
class CStandardHeuristic : public CHeuristic
{
public:
  CStandardHeuristic();

  /// Allow using a different position than the node position for start/end
  void SetStartEndData(const GraphNode *pStartNode, const Vec3 &startPos, const GraphNode *pEndNode, const Vec3 &endPos, const PathfindingExtraConstraints &extraConstraints);

  void ClearConstraints() { m_extraConstraints.resize(0); }

	virtual float EstimateCost(const AStarSearchNode & node0, const AStarSearchNode & node1) const;
  virtual float CalculateCost(const CGraph * graph,
		const AStarSearchNode & node0, unsigned linkIndex0,
		const AStarSearchNode & node1,
    const NavigationBlockers& navigationBlockers) const;
  virtual const PathfindingHeuristicProperties* GetProperties() const {return &m_properties;}
  void SetProperties(const PathfindingHeuristicProperties &properties);

private:
  /// returns either the node position, or the start/end position if it's a start/end node (from the requester)
  const Vec3 &GetNodePos(const GraphNode &node) const;

	PathfindingHeuristicProperties m_properties;
	PathfindingExtraConstraints m_extraConstraints;

  /// cache the minimum of all our extra cost factors
  float m_fMinExtraFactor;
	const GraphNode *m_pStartNode, *m_pEndNode;
  Vec3 m_startPos, m_endPos;

  friend class CLayeredNavMeshHeuristic;
  CLayeredNavMeshHeuristic * m_lnmHeuristic;
};





class CLayeredNavMeshRegion;
class CLayeredNavMeshHeuristic : public CHeuristic  
{
	const CLayeredNavMeshRegion * m_lnmRegion;
	const CGraph * m_graph;

	struct EntryPt {
		const GraphNode * m_parent;
		Vec3 m_pt;
		// ATTN Dec 16, 2008: <pvl> default ctor just to enable usage in std::vector
		EntryPt () : m_parent (0), m_pt(ZERO) { }
		EntryPt (const GraphNode * parent, const Vec3 & pt) : m_parent (parent), m_pt (pt) { }
	};
	typedef std::vector <EntryPt> PolygonEntryPts;
	typedef std::map <const GraphNode * , PolygonEntryPts> EntryPts;
	mutable EntryPts m_entryPts;

	const CStandardHeuristic * m_stdHeuristic;

	Vec3 GetEntryPoint (const AStarSearchNode & node0) const;

public:
	CLayeredNavMeshHeuristic (CGraph * graph, CLayeredNavMeshRegion * region, const CStandardHeuristic * );
	virtual ~CLayeredNavMeshHeuristic() {}

	/// Estimate the cost from one node to another - this estimate must be a minimum
	/// bound on the cost in order for AStar to work correctly (though it may run faster
	/// if the estimate more a best-guess than a lower-bound)
	virtual float EstimateCost(const AStarSearchNode & node0, const AStarSearchNode & node1) const;

	/// Calculate the cost from one node to another via a specific link. This calculation
	/// should include anything specific like terrain costs, stealth vs speed etc
	/// A return value < 0 means that this link is absolutely impassable.
	virtual float CalculateCost(const CGraph * graph,
		const AStarSearchNode & node0, unsigned linkIndex0,
		const AStarSearchNode & node1,
		const NavigationBlockers& navigationBlockers) const;

	void AddEntryPoint (const AStarSearchNode & srcNode, const AStarSearchNode & dstNode, const Vec3 & entryPt) const;
};


#endif
