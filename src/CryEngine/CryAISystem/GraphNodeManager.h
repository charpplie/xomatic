// GraphNodeManager.h: Manager class for graph nodes.
//
//////////////////////////////////////////////////////////////////////
#ifndef __GRAPHNODEMANAGER_H__
#define __GRAPHNODEMANAGER_H__

#include "GraphStructures.h"

class CGraphNodeManager
{
	enum {BUCKET_SIZE_SHIFT = 7};
	enum {BUCKET_SIZE = 128};

public:
	CGraphNodeManager();
	~CGraphNodeManager();

	// NOTE Oct 20, 2009: <pvl> only called from Graph::Clear() and own destructor
	void Clear(IAISystem::tNavCapMask typeMask);

	unsigned CreateNode(IAISystem::tNavCapMask type, const Vec3 &pos, unsigned ID);
	void DestroyNode(unsigned index);

	GraphNode* GetNode(unsigned index)
	{
		if (!index)
			return 0;
		int bucket = (index - 1) >> BUCKET_SIZE_SHIFT;
		// FIXME Oct 20, 2009: <pvl> we need to be able to return 0 here (meaning
		// the node was likely unloaded) => get rid of the GetDummyNode() workaround
		if (!m_buckets[bucket])
			return GetDummyNode();
		return reinterpret_cast<GraphNode*>(static_cast<char*>(m_buckets[bucket]->nodes) +
			(((index - 1) & (BUCKET_SIZE - 1)) * m_buckets[bucket]->nodeSize));
	}

	const GraphNode* GetNode(unsigned index) const
	{
		if (!index)
			return 0;
		int bucket = (index - 1) >> BUCKET_SIZE_SHIFT;
		// FIXME Oct 20, 2009: <pvl> we need to be able to return 0 here (meaning
		// the node was likely unloaded) => get rid of the GetDummyNode() workaround
		if (!m_buckets[bucket])
			return GetDummyNode();
		return reinterpret_cast<GraphNode*>(static_cast<char*>(m_buckets[bucket]->nodes) +
			(((index - 1) & (BUCKET_SIZE - 1)) * m_buckets[bucket]->nodeSize));
	}

	size_t NodeMemorySize () const;

private:
	class BucketHeader
	{
	public:
		unsigned type;
		unsigned nodeSize;
		void* nodes;
	};

	// This function exists only to hide some fundamental problems in the path-finding system. Basically
	// things aren't cleaned up properly as of writing (pre-alpha Crysis 5/5/2007). Therefore we return
	// an object that looks like a graph node to stop client code from barfing.
	GraphNode* GetDummyNode() const
	{
		static char buffer[sizeof(GraphNode) + 100] = {0};
		static GraphNode* pDummy = new (buffer) GraphNode_Triangular(IAISystem::NAV_TRIANGULAR, Vec3(0, 0, 0), 0xCECECECE);
		return pDummy;
	}

	int TypeSizeFromTypeIndex (unsigned typeIndex) const;

	std::vector<BucketHeader*> m_buckets;
	std::vector<int> m_lastBucket;
	std::vector<int> m_nextNode;
	int m_typeSizes[IAISystem::NAV_TYPE_COUNT];
};

#endif //__GRAPHNODEMANAGER_H__
