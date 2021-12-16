#ifndef ASTAROPENLIST_H
#define ASTAROPENLIST_H

#if _MSC_VER > 1000
#pragma once
#endif

#include "IAgent.h"
#include "GraphStructures.h"
#include <vector>
#include <set>
#include <algorithm>

// If this isn't on the OpenListMonitor isn't even mentioned in AStarOpenList.
//#define MONITOR_OPEN_LIST
/**
 * Useful for gathering info about how A* open list operates.
 *
 * Receives an event for every node that's pushed to the open list and every
 * node that popped from there.  Based on that it computes various statistics.
 *
 * TODO Mai 22, 2007: <pvl> no reasonable way of outputting the gathered stats
 * at the moment.  In fact, mainly intended to be used "manually" under
 * debugger ... :)
 */
class OpenListMonitor
{
	/// Holds any info we care to hold per every node currently on the open list.
	struct NodeInfo {
		CTimeValue m_timestamp;
		int m_frame;
		NodeInfo (const CTimeValue & time, int frame) : m_timestamp(time), m_frame(frame)
		{ }
	};
	/// Uses graph node index as a unique node ID and maps that to corresponding node info.
	typedef std::map<unsigned int, NodeInfo> NodeInfoMap;
	NodeInfoMap m_nodeInfoMap;

	/// The actual open list statistics.
	float sMin;
	float sMax;
	float sAvg;
	int sNumSamples;
	float sMinFrames;
	float sMaxFrames;
	float sAvgFrames;
	/// For each frame that's leaving the open list, this is called with the time
	/// and a number of frames that the node spent on the open list.
	void UpdateStats (float t, int frames)
	{
		if (t < sMin) sMin = t;
		if (t > sMax) sMax = t;
		sAvg = (sAvg * sNumSamples + t) / (sNumSamples + 1);

		if (t < sMinFrames) sMinFrames = (float)frames;
		if (t > sMaxFrames) sMaxFrames = (float)frames;
		sAvgFrames = (sAvgFrames * sNumSamples + frames) / (sNumSamples + 1);

		++sNumSamples;
	}
public:
	void NodePushed (unsigned int nodeIndex)
	{
		// NOTE Mai 22, 2007: <pvl> we could filter incoming nodes here if we're
		// only interested in stats for a certain node class
/*
		GraphNode* nodeptr = nodeManager.GetNode(node);
		if (nodeptr->navType != IAISystem::NAV_WAYPOINT_HUMAN)
			return;
*/
		CTimeValue now = gEnv->pTimer->GetAsyncTime();
		int currFrame = gEnv->pRenderer->GetFrameID();
		m_nodeInfoMap.insert (std::make_pair (nodeIndex, NodeInfo (now, currFrame)));
	}
	void NodePopped (unsigned int nodeIndex)
	{
		NodeInfoMap::iterator infoIt = m_nodeInfoMap.find (nodeIndex);
		if (infoIt == m_nodeInfoMap.end ()) {
			// NOTE Mai 22, 2007: <pvl> can happen if we filter nodes in NodePushed()
			return;
		}

		CTimeValue timeWhenPushed = infoIt->second.m_timestamp;
		int frameWhenPushed = infoIt->second.m_frame;

		CTimeValue now = gEnv->pTimer->GetAsyncTime();
		int currFrame = gEnv->pRenderer->GetFrameID();

		float timeSpentInList = (now - timeWhenPushed).GetMilliSeconds();
		int framesSpentInList = currFrame - frameWhenPushed;
		UpdateStats (timeSpentInList, framesSpentInList);
	}
};

struct AStarSearchNode
{
	// Cost from the starting node to this node (g)
	mutable float fCostFromStart;
	// Estimated cost from this node to the goal (h)
	mutable float fEstimatedCostToGoal;
	// Previous node in the A* path
	mutable AStarSearchNode * prevPathNodeIndex;

	//Node index
	unsigned nodeIndex;

	// Used as a flag for costs that aren't calculated yet
	static const float fInvalidCost;

	GraphNode* graphNode;

	AStarSearchNode()
	{
		fCostFromStart = fInvalidCost;
		fEstimatedCostToGoal = fInvalidCost;
		prevPathNodeIndex = 0;
		nodeIndex = 0;
		tag = 0;
		graphNode = NULL;
	}

	char IsTagged()
	{
		return tag;
	}
private:

	mutable unsigned char tag;

	friend class CAStarNodeListManager;
	AStarSearchNode(unsigned originalNodeIndex, GraphNode* graphNodePtr)
	{
		fCostFromStart = fInvalidCost;
		fEstimatedCostToGoal = fInvalidCost;
		prevPathNodeIndex = 0;
		nodeIndex = originalNodeIndex;
		tag = 0;
		graphNode = graphNodePtr;
	}
};

/// Helper class to sort the node lists
struct NodeCompareCost
{
	NodeCompareCost()
	{
	}

	bool operator()(const AStarSearchNode* node1, const AStarSearchNode* node2) const 
	{
		return ( (node1->fCostFromStart + node1->fEstimatedCostToGoal) > (node2->fCostFromStart + node2->fEstimatedCostToGoal) );
	}
};


// Helper class to sort the node lists
struct NodeCompareIndex
{
	bool operator()(const AStarSearchNode& node1, const AStarSearchNode& node2) const 
	{
		return node1.nodeIndex < node2.nodeIndex;
	}
};

typedef std::vector<AStarSearchNode*> AStarSearchNodeVector;

//====================================================================
// CAStarNodeListManager
//====================================================================
class CAStarNodeListManager
{
public:
	CAStarNodeListManager(CGraphNodeManager& nodeManager)
		:	nodeManager(nodeManager)
	{
	}

	/// Gets the best node and removes it from the list. Returns 0 if
	/// the list is empty
	AStarSearchNode* PopBestNodeFromOpenList();

	/// Adds a node to the list (shouldn't already be in the list)
	void AddNodeToOpenList(AStarSearchNode*);

	/// If the node is in the list then its position in the list gets updated.
	/// If not the list isn't changed. Either way the node itself gets
	/// modified
	void UpdateNode(unsigned node, float newCost, float newEstimatedCost);

	/// Indicates if the list is empty
	bool IsEmpty() const;

	/// Empties the list
	void Clear();

	// returns memory usage in bytes
	size_t MemStats();

	/// Reserves memory based on an estimated max list size
	void ReserveMemory(size_t estimatedMaxSize);

	bool IsTagged(unsigned nodeIndex)
	{
		std::set<AStarSearchNode, NodeCompareIndex>::iterator iter = m_currentNodes.find(
			AStarSearchNode(nodeIndex,NULL)/*should be ok as original returned??*/
		);
		return iter != m_currentNodes.end() && iter->tag;
	}

	const AStarSearchNodeVector& GetTaggedNodesVector()
	{
		return taggedNodes;
	}

	AStarSearchNode* GetAStarNode(unsigned index)
	{
		return const_cast<AStarSearchNode*>(&*m_currentNodes.insert(AStarSearchNode(index,nodeManager.GetNode(index))).first);
	}

private:

	std::set<AStarSearchNode, NodeCompareIndex> m_currentNodes;

	/// the open list
	AStarSearchNodeVector m_openList;
	CGraphNodeManager& nodeManager;
	AStarSearchNodeVector taggedNodes;

#ifdef MONITOR_OPEN_LIST
	OpenListMonitor m_monitor;
#endif // MONITOR_OPEN_LIST
};

//====================================================================
// Don't look below here - inline implementation
//====================================================================

//====================================================================
// ReserveMemory
//====================================================================
inline void CAStarNodeListManager::ReserveMemory(size_t estimatedMaxSize)
{
}


//====================================================================
// MemStats
//====================================================================
inline size_t CAStarNodeListManager::MemStats()
{
	return m_openList.capacity() * sizeof(unsigned);
}


//====================================================================
// IsEmpty
//====================================================================
inline bool CAStarNodeListManager::IsEmpty() const
{
	return m_openList.empty();
}

//====================================================================
// PopBestNode
//====================================================================
inline AStarSearchNode* CAStarNodeListManager::PopBestNodeFromOpenList()
{
	if (IsEmpty())
		return 0;

	AStarSearchNode* node = m_openList.front();
	// This "forgets about" the last node, and (partially) sorts the rest
	std::pop_heap(m_openList.begin(), m_openList.end(), NodeCompareCost());
	// remove the redundant element
	m_openList.pop_back();

#ifdef MONITOR_OPEN_LIST
	m_monitor.NodePopped (node);
#endif // MONITOR_OPEN_LIST

	return node;
}

//====================================================================
// AddNode
//====================================================================
inline void CAStarNodeListManager::AddNodeToOpenList(AStarSearchNode* node)
{
	node->tag = 1;
	taggedNodes.push_back(node);

	m_openList.push_back(node);
	std::push_heap(m_openList.begin(), m_openList.end(), NodeCompareCost());

#ifdef MONITOR_OPEN_LIST
	m_monitor.NodePushed (node.nodeIndex);
#endif // MONITOR_OPEN_LIST
}

//====================================================================
// UpdateNode
//====================================================================
inline void CAStarNodeListManager::UpdateNode(unsigned nodeIndex, float newCost, float newEstimatedCost)
{
	const AStarSearchNodeVector::const_iterator end = m_openList.end();
	for (AStarSearchNodeVector::iterator it = m_openList.begin() ; it != end ; ++it)
	{
		if ((*it)->nodeIndex == nodeIndex)
		{
			AStarSearchNode* node = (*it);
			node->fCostFromStart = newCost;
			node->fEstimatedCostToGoal = newEstimatedCost;
			std::push_heap(m_openList.begin(), it+1, NodeCompareCost());
			return;
		}
	}
	AStarSearchNode* node = GetAStarNode(nodeIndex);
	node->fCostFromStart = newCost;
	node->fEstimatedCostToGoal = newEstimatedCost;
}

//====================================================================
// Clear
//====================================================================
inline void CAStarNodeListManager::Clear()
{
	m_openList.resize(0);
	m_currentNodes.clear();
	taggedNodes.resize(0);
}


#endif
