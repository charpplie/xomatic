/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   Graph.h
$Id$
Description: interface for the CGraph class.

-------------------------------------------------------------------------
History:
- ?
- 4 May 2009      : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#if !defined(AFX_GRAPH_H__6D059D2E_5A74_4352_B3BF_2C88D446A2E1__INCLUDED_)
#define AFX_GRAPH_H__6D059D2E_5A74_4352_B3BF_2C88D446A2E1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IAISystem.h"
#include "IAgent.h"
#include "Heuristic.h"
#include "NavPath.h"
#include "AILog.h"
#include "ISerialize.h"
#include "AllNodesContainer.h"
#include "AutoTypeStructs.h"

#include <list>
#include <map>
#include <set>
#include <vector>
#include <CryArray.h>
#include <VectorMap.h>

class CCryFile;
class CGraphLinkManager;

enum EPathfinderResult
{
  PATHFINDER_STILLFINDING,
  PATHFINDER_BEAUTIFYINGPATH,
  PATHFINDER_POPNEWREQUEST,
  PATHFINDER_PATHFOUND,
  PATHFINDER_NOPATH,
  PATHFINDER_ABORT,
  PATHFINDER_MAXVALUE
};
class CAISystem;
struct IStatObj;
class ICrySizer;
class CAIObject;
class CVolumeNavRegion;
class CFlightNavRegion;
class CGraphNodeManager;

class CSmartObject;
struct CCondition;

struct IVisArea;

inline int TypeIndexFromType(IAISystem::tNavCapMask type)
{
	if (type & IAISystem::NAV_LAYERED_NAV_MESH)
		return IAISystem::NAV_TYPE_COUNT + type.GetLnmCaps();

	int typeIndex;
	for (typeIndex = IAISystem::NAV_TYPE_COUNT - 1; typeIndex >= 0 && ((1 << typeIndex) & type) == 0; --typeIndex);
	return typeIndex;
}

inline const char* StringFromTypeIndex(int typeIndex)
{
	static const char* navTypeStrings[] = {
		"NAV_UNSET",
		"NAV_TRIANGULAR",
		"NAV_WAYPOINT_HUMAN",
		"NAV_WAYPOINT_3DSURFACE",
		"NAV_FLIGHT",
		"NAV_VOLUME",
		"NAV_ROAD",
		"NAV_SMARTOBJECT",
		"NAV_FREE_2D",
		"NAV_LAYERED_NAV_MESH",
		"NAV_CUSTOM_NAVIGATION",
	};
	const int STRING_COUNT = sizeof(navTypeStrings) / sizeof(navTypeStrings[0]);

	COMPILE_TIME_ASSERT(STRING_COUNT == static_cast<int>(IAISystem::NAV_TYPE_COUNT));

	if (typeIndex	< 0)
		return "<Invalid Nav Type>";
	else if (typeIndex >= STRING_COUNT)
		return "NAV_LAYERED_NAV_MESH"; // TypeIndexFromType() converts NAV_LAYERED_NAV_MESH to something greater than NAV_TYPE_COUNT
	else
		return navTypeStrings[typeIndex];
}

inline const char* StringFromType(IAISystem::ENavigationType type)
{
	return StringFromTypeIndex(TypeIndexFromType(type));
}

//====================================================================
// CObstacleRef
//====================================================================
class CObstacleRef
{
protected:
  CWeakRef<CAIObject>	m_refAnchor;		// designer defined hiding point
  int			m_vertexIndex;	// index of vertex
  unsigned	m_nodeIndex;		// for indoors nodes could be hide points
	GraphNode* m_pNode;
  CSmartObject*	m_pSmartObject;	// pointer to smart object to be used for hiding
  CCondition*	m_pRule;		// pointer to smart object rule to be used for hiding

public:
  CAIObject* GetAnchor() const { return m_refAnchor.GetAIObject(); }
  int GetVertex() const { return m_vertexIndex; }
  const unsigned GetNodeIndex() const { return m_nodeIndex; }
	const GraphNode* GetNode() const {return m_pNode;}
  CSmartObject* GetSmartObject() const { return m_pSmartObject; }
  const CCondition* GetRule() const { return m_pRule; }

  CObstacleRef() : m_vertexIndex(-1), m_nodeIndex(0), m_pNode(0), m_pSmartObject(NULL), m_pRule(NULL) {}
  CObstacleRef(const CObstacleRef& other) : m_refAnchor(other.m_refAnchor), m_vertexIndex(other.m_vertexIndex), m_nodeIndex(other.m_nodeIndex), m_pNode(other.m_pNode),
	  m_pSmartObject(other.m_pSmartObject), m_pRule(other.m_pRule) {}
  CObstacleRef(CWeakRef<CAIObject> refAnchor) : m_refAnchor(refAnchor), m_vertexIndex(-1), m_nodeIndex(0), m_pNode(0), m_pSmartObject(NULL), m_pRule(NULL) {}
  CObstacleRef(int vertexIndex) : m_vertexIndex(vertexIndex), m_nodeIndex(0), m_pSmartObject(NULL), m_pRule(NULL) {}
  CObstacleRef(unsigned nodeIndex, GraphNode* pNode) : m_vertexIndex(-1), m_nodeIndex(nodeIndex), m_pNode(pNode), m_pSmartObject(NULL), m_pRule(NULL) {}
  CObstacleRef(CSmartObject* pSmartObject, CCondition* pRule) : m_vertexIndex(-1), m_nodeIndex(0), m_pNode(0)
	  , m_pSmartObject(pSmartObject), m_pRule(pRule) {}

  void Serialize(TSerialize ser, class CObjectTracker& objectTracker);
  Vec3 GetPos() const;
	float GetApproxRadius() const;
  const CObstacleRef& operator = (const CObstacleRef& other)
  {
    m_refAnchor = other.m_refAnchor;
    m_vertexIndex = other.m_vertexIndex;
    m_nodeIndex = other.m_nodeIndex;
		m_pNode = other.m_pNode;
    m_pSmartObject = other.m_pSmartObject;
    m_pRule = other.m_pRule;
    return *this;
  }

  bool operator == (const CObstacleRef& other) const
  {
    return m_refAnchor == other.m_refAnchor && m_vertexIndex == other.m_vertexIndex && m_nodeIndex == other.m_nodeIndex
      && m_pNode == other.m_pNode && m_pSmartObject == other.m_pSmartObject && m_pRule == other.m_pRule;
  }
  bool operator != (const CObstacleRef& other) const
  {
    return m_refAnchor != other.m_refAnchor || m_vertexIndex != other.m_vertexIndex || m_nodeIndex != other.m_nodeIndex
      || m_pNode != other.m_pNode || m_pSmartObject != other.m_pSmartObject || m_pRule != other.m_pRule;
  }
  bool operator < (const CObstacleRef& other) const
  {
    return
      m_nodeIndex < other.m_nodeIndex || m_nodeIndex == other.m_nodeIndex &&
      (	m_refAnchor < other.m_refAnchor || m_refAnchor == other.m_refAnchor &&
      ( m_vertexIndex < other.m_vertexIndex || m_vertexIndex == other.m_vertexIndex &&
      ( m_pSmartObject < other.m_pSmartObject || m_pSmartObject < other.m_pSmartObject && 
	    m_pRule < other.m_pRule ) ) );
  }
  operator bool () const
  {
    return m_refAnchor.IsValid() || m_vertexIndex >= 0 || m_nodeIndex || m_pSmartObject && m_pRule;
  }
  bool operator ! () const
  {
	  return !m_refAnchor.IsValid() && m_vertexIndex < 0 && !m_nodeIndex && (!m_pSmartObject || !m_pRule);
  }

private:
  operator int () const
  {
    // it is illegal to cast CObstacleRef to an int!!!
    // are you still using old code?
    AIAssert(0);
    return 0;
  }
};

// NOTE: int64 here avoids a tiny performance impact on 32-bit platform
// for the cost of loss of full compatibility: 64-bit generated BAI files
// can't be used on 32-bit platform safely. Change the key to int64 to 
// make it fully compatible. The code that uses this map will be recompiled
// to use the full 64-bit key on both 32-bit and 64-bit platforms.

typedef std::multimap<int64,unsigned> EntranceMap;

typedef std::list<Vec3> ListPositions;
typedef std::list<ObstacleData> ListObstacles;
typedef std::multimap<float, ObstacleData> MultimapRangeObstacles;
typedef std::vector<NodeDescriptor> NodeDescBuffer;
typedef std::vector<LinkDescriptor> LinkDescBuffer;
typedef std::list<GraphNode *> ListNodes;
typedef std::list<unsigned> ListNodeIds;
typedef std::set<GraphNode*> SetNodes;
typedef std::set<const GraphNode*> SetConstNodes;
typedef std::list<const GraphNode *> ListConstNodes;
typedef std::vector<const GraphNode *> VectorConstNodes;
typedef std::vector<unsigned> VectorConstNodeIndices;
typedef std::multimap<float,GraphNode*> CandidateMap;
typedef std::multimap<float,unsigned> CandidateIdMap;
typedef std::set< CObstacleRef > SetObstacleRefs;
typedef VectorMap<unsigned, SCachedPassabilityResult> PassabilityCache;

// [Mikko] Note: The Vector map is faster when traversing, and the normal map with pool allocator seems
// to be a little faster in CGraph.GetNodesInRange. Both are faster than normal std::map.
//typedef stl::STLPoolAllocator< std::pair<const GraphNode*, float> > NodeMapAllocator;
//typedef std::map<const GraphNode*, float, std::less<const GraphNode*>, NodeMapAllocator> MapConstNodesDistance;
typedef VectorMap<const GraphNode*, float> MapConstNodesDistance;


//====================================================================
// CGraph 
//====================================================================
class CGraph
{
public:
  CGraph(CAISystem* pAISystem);
  ~CGraph();

	CGraphLinkManager& GetLinkManager() {return *m_pGraphLinkManager;}
	const CGraphLinkManager& GetLinkManager() const {return *m_pGraphLinkManager;}

	CGraphNodeManager& GetNodeManager() {return *m_pGraphNodeManager;}
	const CGraphNodeManager& GetNodeManager() const {return *m_pGraphNodeManager;}

	/// Restores the graph to the initial state (i.e. restores pass radii etc). 
  void Reset();

  /// removes all nodes and stuff associated with navTypes matching the bitmask
  void Clear(IAISystem::tNavCapMask navTypeMask);

  /// Connects (two-way) two nodes, optionally returning pointers to the new links
  void Connect(unsigned oneIndex, unsigned twoIndex, float radiusOneToTwo = 100.0f, float radiusTwoToOne = 100.0f,
    unsigned* pLinkOneTwo = 0, unsigned* pLinkTwoOne = 0);

  /// Disconnects a node from its neighbours. if bDelete then pNode will be deleted. Note that 
  /// the previously connected nodes will not be deleted, even if they
  /// end up with no nodes.
  void Disconnect(unsigned nodeIndex, bool bDelete = true);

  /// Removes an individual link from a node (and removes the reciprocal link) - 
  /// doesn't delete it.
  void Disconnect(unsigned nodeIndex, unsigned linkId);

  /// Checks the graph is OK (as far as possible). Asserts if not, and then
  /// returns true/false to indicate if it's OK
  /// msg should indicate where this is being called from (for writing error msgs)
  bool Validate(const char * msg, bool checkPassable) const;

  /// Checks that a node exists (should be quick). If fullCheck is true it will do some further 
  /// checks which will be slower
  bool ValidateNode(unsigned nodeIndex, bool fullCheck) const;
	bool ValidateHashSpace() { return m_allNodes.ValidateHashSpace(); }

	unsigned int LinkId (unsigned link) const;

  /// General function to get the graph node enclosing a position - the type depends
  /// on the navigation modifiers etc and the navigation capabilities. If you know what 
  /// navigation type you want go via the specific NavRegion class. If there are a number of nodes
  /// that could be returned, only nodes that have at least one link with radius > passRadius will 
	/// be returned. Range determines the search range for the enclosing node: (range < 0) indicates 
	/// a default search radius will be used determined by the navigation type. (range >= 0) will be  
	/// taken into account depending on the navigation type.
	unsigned int GetEnclosing(const Vec3& pos, IAISystem::tNavCapMask navCapMask, float passRadius = 0.0f,
		unsigned int startIndex = 0, float range = 0.0f, Vec3* closestValid = 0, bool returnSuspect = false, const char* requesterName = "");

  /// Restores all node/links
  void RestoreAllNavigation( );

  /// Reads the AI graph from a specified file
  bool ReadFromFile(const char * szName, int lnmModif = 0 );

  /// Returns all nodes that are in the graph - not all nodes will be
  /// connected (even indirectly) to each other
  CAllNodesContainer& GetAllNodes() {return m_allNodes;}
  const CAllNodesContainer& GetAllNodes() const {return m_allNodes;}

  /// Checks that the graph is empty. Pass in a bitmask of IAISystem::ENavigationType to 
  /// specify the types to check
  bool CheckForEmpty(IAISystem::tNavCapMask navTypeMask = IAISystem::NAVMASK_ALL) const;

  /// uses mark for internal graph operation without disturbing the pathfinder
  void MarkNode(unsigned nodeIndex) const;
  /// clears the marked nodes
  void ClearMarks() const;

  // defines bounding rectangle of this graph
  void SetBBox(const Vec3 & min, const Vec3 & max);
  // how is that for descriptive naming of functions ??
  bool InsideOfBBox(const Vec3 & pos) const; // returns true if pos is inside of bbox (but not on boundaries)

  /// Creates a new node of the specified type (which can't be subsequently changed). Note that
  /// to delete the node use the disconnect function. 
  unsigned CreateNewNode(IAISystem::tNavCapMask type, const Vec3 &pos, unsigned ID = 0);
	GraphNode* GetNode(unsigned index);

  /// Moves a node, updating spatial structures
  void MoveNode(unsigned nodeIndex, const Vec3 &newPos);

  /// finds all nodes within range of startPos and their distance from vStart. 
  /// pStart is just used as a hint.
  /// returns a reference to the input/output so it's easy to use in a test.
  /// traverseForbiddenHideLink should be true if you want to pass through 
  /// links between hide waypoints that have been marked as impassable
  /// SmartObjects will only be considered if pRequester != 0
	MapConstNodesDistance &GetNodesInRange(MapConstNodesDistance &result, const Vec3 & startPos, float maxDist, 
		IAISystem::tNavCapMask navCapMask, float passRadius, unsigned startNodeIndex = 0, const class CAIObject *pRequester = 0);

  //====================================================================
  // The following are used during serialisation
  //====================================================================
  typedef std::map<unsigned, unsigned> TGraphNodeIDs;

  /// Get an ID that is guaranteed to be the same everytime the game is
  /// run for this particular level.
  /// if graphNodeIDs is non-zero then the ID gets added to it
  unsigned GetIDFromNode(unsigned nodeIndex, TGraphNodeIDs *graphNodeIDs = 0) const;

  /// Get a node from a previously calculated ID. Returns 0 if the ID cannot be found
  /// Note that if graphNodeIDs is 0 then this is SLOW - only use it like that for a few nodes
  unsigned GetNodeFromID(unsigned ID, TGraphNodeIDs *graphNodeIDs = 0) const;

  /// Serialise the _modifications_ to the graph
  void Serialize(TSerialize ser, class CObjectTracker& objectTracker, const char* graphName);

  /// Tell the object tracker about objects we've created/own
  void PopulateObjectTracker(class CObjectTracker& objectTracker);

  /// Serialise a _pointer_ to a node. Name is the caller's name for this node
  void SerializeNodePointer(TSerialize ser, const char* name, unsigned& pNode, TGraphNodeIDs *graphNodeIDs = 0) const;

  /// Serialise a container of GraphNode pointers (only certain containers are supported)
  template<typename Container> void SerializeNodePointerContainer(TSerialize ser, const char* name, Container& container, TGraphNodeIDs *graphNodeIDs = 0) const;

  // Returns memory usage not including nodes
  size_t MemStats();
  // Returns the memory usage for nodes of the type passed in (bitmask)
  size_t NodeMemStats(unsigned navTypeMask);

  struct SBadGraphData
  {
    enum EType {BAD_PASSABLE, BAD_IMPASSABLE};
    EType mType;
    Vec3 mPos1, mPos2;
  };
  /// List of bad stuff we found during the last validation. mutable because it's
  /// debug - Validate(...) should really be a const method, since it wouldn't change
  /// any "real" data
  mutable std::vector<SBadGraphData> mBadGraphData;

  GraphNode *m_pSafeFirst;
	unsigned m_safeFirstIndex;

	CGraphNodeManager* m_pGraphNodeManager;

private:

  // maps IDs to node pointers. On Writing writes the IDs of nodes that are
  // being serialised. On reading just sets up the map for the serialised nodes
  void SerializeGraphNodeIDs(TSerialize ser, CGraph::TGraphNodeIDs &graphNodeIDs);

  /// Finds nodes within a distance of pNode that can be accessed by something with
  /// passRadius. If pRequester != 0 then smart object links will be checked as well.
  void FindNodesWithinRange(MapConstNodesDistance &result, float curDist, float maxDist, 
    const GraphNode *pNode, float passRadius, const class CAIObject *pRequester) const ;

  bool DbgCheckList( ListNodeIds& nodesList )	const;

public:
  /// deletes (disconnects too) all nodes with a type matching the bitmask
  void DeleteGraph(IAISystem::tNavCapMask navTypeMask);

private:
  GraphNode * GetEntrance(int nBuildingID,const Vec3 &pos);
  bool GetEntrances(int nBuildingID, const Vec3& pos, std::vector<unsigned>& nodes);
  // reads all the nodes in a map
  bool ReadNodes( CCryFile &file, int lnmModif = 0 );
  /// Deletes the node, which should have been disconnected first (warning if not)
  void DeleteNode(unsigned nodeIndex);

  // helper called from ValidateNode only (to get ValidateNode to be inlined inside
  // Graph.cpp)
  bool ValidateNodeFullCheck(const GraphNode* pNode) const;

	unsigned m_currentIndex;
  GraphNode *m_pCurrent;
	unsigned m_firstIndex;
  GraphNode *m_pFirst;

  /// All the nodes we've marked
  mutable VectorConstNodeIndices m_markedNodes;
  /// All the nodes we've tagged
  mutable VectorConstNodeIndices m_taggedNodes;

  /// nodes are allocated/deleted via a single interface, so keep track of them
  /// all - for memory tracking and to allow quick iteration
  CAllNodesContainer m_allNodes;

	CGraphLinkManager* m_pGraphLinkManager;

  /// Bounding box of the triangular area
  AABB m_triangularBBox;

  EntranceMap m_mapEntrances;
  EntranceMap m_mapExits;

  friend class CTriangularNavRegion;
  friend class CWaypointHumanNavRegion;
  friend class CFlightNavRegion;
  friend class CVolumeNavRegion;
};

// Check whether a position is within a node's triangle
bool PointInTriangle(const Vec3 & pos, GraphNode * pNode);

//====================================================================
// SMarkClearer
// Helper - the constructor and destructor clear marks
//====================================================================
struct SMarkClearer
{
  SMarkClearer(const CGraph* pGraph) : m_pGraph(pGraph) {m_pGraph->ClearMarks();}
  ~SMarkClearer() {m_pGraph->ClearMarks();}
private:
  const CGraph* m_pGraph;
};

//====================================================================
// Inline implementations
//====================================================================

//====================================================================
// SerializeNodePointerContainer
//====================================================================
template<typename Container>
inline void CGraph::SerializeNodePointerContainer(TSerialize ser, const char* name, Container& container, TGraphNodeIDs *graphNodeIDs) const
{
  unsigned containerSize = container.size();
  ser.BeginGroup(name);
  ser.Value("size", containerSize);
  if (containerSize > 0)
  {
    if (ser.IsReading())
    {
      container.clear();
      for (unsigned i = 0 ; i < containerSize ; ++i)
      {
				ser.BeginGroup("n");
        unsigned nodeIndex;
        SerializeNodePointer(ser, "n", nodeIndex, graphNodeIDs);
        if (nodeIndex)
          container.push_back(nodeIndex);
        else
          AIWarning("got zero node pointer reading %s", name);
				ser.EndGroup();
      }
    }
    else
    {
      // writing
      for (typename Container::const_iterator it = container.begin() ; it != container.end() ; ++it)
      {
				ser.BeginGroup("n");
        unsigned nodeIndex = *it;
        if (nodeIndex)
          SerializeNodePointer(ser, "n", nodeIndex, graphNodeIDs);
        else
          AIWarning("got zero node pointer writing %s", name);
				ser.EndGroup();
      }
    }
  }
  else if (ser.IsReading())
  {
    container.clear();
  }
  ser.EndGroup();
}

#include "AIHash.h"

inline unsigned int CGraph::LinkId (unsigned link) const
{
	unsigned int prev = GetLinkManager().GetPrevNode (link);
	unsigned int next = GetLinkManager().GetNextNode (link);
	const GraphNode * prevNode = GetNodeManager().GetNode(prev);
	const GraphNode * nextNode = GetNodeManager().GetNode(next);
	return HashFromVec3 (prevNode->GetPos (), 0.0f) + HashFromVec3 (nextNode->GetPos (), 0.0f);
}

#endif // !defined(AFX_GRAPH_H__6D059D2E_5A74_4352_B3BF_2C88D446A2E1__INCLUDED_)
