// GraphNodeManager.cpp: Manager class for graph nodes.
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "GraphNodeManager.h"

inline IAISystem::tNavCapMask TypeFromTypeIndex (int typeIndex)
{
	if (typeIndex < IAISystem::NAV_TYPE_COUNT)
		return IAISystem::tNavCapMask (1 << typeIndex);

	IAISystem::tNavCapMask type (IAISystem::NAV_LAYERED_NAV_MESH);
	type.SetLnmCaps (typeIndex - IAISystem::NAV_TYPE_COUNT);
	return type;
}

inline int CGraphNodeManager::TypeSizeFromTypeIndex (unsigned typeIndex) const
{
	if (typeIndex < IAISystem::NAV_TYPE_COUNT)
		return m_typeSizes[typeIndex];
	else
		return sizeof(GraphNode_LayeredNavMesh);
}

bool Match (IAISystem::tNavCapMask type, IAISystem::tNavCapMask mask)
{
	if ((type & IAISystem::NAV_LAYERED_NAV_MESH) && (mask & IAISystem::NAV_LAYERED_NAV_MESH))
	{
		if (type.GetLnmCaps() == 0 || mask.GetLnmCaps() == 0)
			return true;
		else if (type.GetLnmCaps() == mask.GetLnmCaps())
			return true;
		else
			return false;
	}
	return (type & mask) != 0;
}

CGraphNodeManager::CGraphNodeManager() :
		m_lastBucket(IAISystem::NAV_TYPE_COUNT, -1),
		m_nextNode(IAISystem::NAV_TYPE_COUNT, BUCKET_SIZE)
{
	m_typeSizes[0] = sizeof(GraphNode_Unset);
	m_typeSizes[1] = sizeof(GraphNode_Triangular);
	m_typeSizes[2] = sizeof(GraphNode_WaypointHuman);
	m_typeSizes[3] = sizeof(GraphNode_Waypoint3DSurface);
	m_typeSizes[4] = sizeof(GraphNode_Flight);
	m_typeSizes[5] = sizeof(GraphNode_Volume);
	m_typeSizes[6] = sizeof(GraphNode_Road);
	m_typeSizes[7] = sizeof(GraphNode_SmartObject);
	m_typeSizes[8] = sizeof(GraphNode_Free2D);
	m_typeSizes[9] = sizeof(GraphNode_LayeredNavMesh);
	m_typeSizes[10] = sizeof(GraphNode_CustomNav);
}

CGraphNodeManager::~CGraphNodeManager()
{
	Clear(~0);
}

void CGraphNodeManager::Clear(IAISystem::tNavCapMask typeMask)
{
	for (int i = 0, count = int(m_buckets.size()); i < count; ++i)
	{
		if (m_buckets[i] && Match(m_buckets[i]->type, typeMask))
		{
			delete [] (reinterpret_cast<char*>(m_buckets[i]));
			m_buckets[i] = 0;
		}
	}

	assert (m_lastBucket.size() == m_nextNode.size());
	for (unsigned i=0, numTypes = m_lastBucket.size(); i < numTypes; ++i)
	{
		IAISystem::tNavCapMask type = TypeFromTypeIndex (i);

		// TODO Oct 22, 2009: <pvl> ugly as hell but a LNM nav type with LNM mesh
		// index (stored in LNM data) set to zero now means "all meshes" so we need
		// skip it here in order not to clear everything LNM
		if ( (type & IAISystem::NAV_LAYERED_NAV_MESH) && type.GetLnmCaps() == 0)
			continue;
		if (Match(type, typeMask))
		{
			m_lastBucket[i] = -1;
			m_nextNode[i] = BUCKET_SIZE;
		}
	}
}

unsigned CGraphNodeManager::CreateNode(IAISystem::tNavCapMask type, const Vec3 &pos, unsigned ID)
{
	int typeIndex = TypeIndexFromType(type);
	if (typeIndex < 0)
		return 0;

	assert (m_lastBucket.size() == m_nextNode.size());
	if (typeIndex >= m_lastBucket.size())
	{
		const int n = typeIndex - m_lastBucket.size() + 1;

		m_lastBucket.insert (m_lastBucket.end(), n, -1);
		m_nextNode.insert (m_nextNode.end(), n, BUCKET_SIZE);
	}

	const int typeSize = TypeSizeFromTypeIndex (typeIndex);

	if (m_nextNode[typeIndex] >= BUCKET_SIZE)
	{
		int newBucketIndex = int(m_buckets.size());
		for (int i = 0; i < int(m_buckets.size()); ++i)
		{
			if (!m_buckets[i])
				newBucketIndex = i;
		}
		int bucketAllocationSize = sizeof(BucketHeader) + typeSize * BUCKET_SIZE;
		BucketHeader* pHeader = reinterpret_cast<BucketHeader*>(new char [bucketAllocationSize]);
		pHeader->type = type.GetFullMask();
		pHeader->nodeSize = typeSize;
		pHeader->nodes = pHeader + 1;
		if (newBucketIndex >= int(m_buckets.size()))
			m_buckets.resize(newBucketIndex + 1);
		m_buckets[newBucketIndex] = pHeader;
		m_lastBucket[typeIndex] = newBucketIndex;
		m_nextNode[typeIndex] = 0;
	}

	GraphNode* pNode = reinterpret_cast<GraphNode*>(
		static_cast<char*>(m_buckets[m_lastBucket[typeIndex]]->nodes) + m_nextNode[typeIndex] * typeSize);

	IAISystem::ENavigationType actualType = (IAISystem::ENavigationType)(unsigned )type;
	switch (actualType)
	{
	case IAISystem::NAV_UNSET: pNode = new(pNode) GraphNode_Unset(actualType, pos, ID); break;
	case IAISystem::NAV_TRIANGULAR: pNode = new(pNode) GraphNode_Triangular(actualType, pos, ID); break;
	case IAISystem::NAV_WAYPOINT_HUMAN: pNode = new(pNode) GraphNode_WaypointHuman(actualType, pos, ID); break;
	case IAISystem::NAV_WAYPOINT_3DSURFACE: pNode = new(pNode) GraphNode_Waypoint3DSurface(actualType, pos, ID); break;
	case IAISystem::NAV_FLIGHT: pNode = new(pNode) GraphNode_Flight(actualType, pos, ID); break;
	case IAISystem::NAV_VOLUME: pNode = new(pNode) GraphNode_Volume(actualType, pos, ID); break;
	case IAISystem::NAV_ROAD: pNode = new(pNode) GraphNode_Road(actualType, pos, ID); break;
	case IAISystem::NAV_SMARTOBJECT: pNode = new(pNode) GraphNode_SmartObject(actualType, pos, ID); break;
	case IAISystem::NAV_FREE_2D: pNode = new(pNode) GraphNode_Free2D(actualType, pos, ID); break;
	case IAISystem::NAV_LAYERED_NAV_MESH: pNode = new(pNode) GraphNode_LayeredNavMesh(actualType, pos, ID); break;
	case IAISystem::NAV_CUSTOM_NAVIGATION: pNode = new(pNode) GraphNode_CustomNav(actualType, pos, ID); break;
	}

	return ((m_lastBucket[typeIndex] << BUCKET_SIZE_SHIFT) + m_nextNode[typeIndex]++) + 1;
}

void CGraphNodeManager::DestroyNode(unsigned index)
{
	GraphNode* pNode = GetNode(index);
	switch (pNode->navType)
	{
	case IAISystem::NAV_UNSET: static_cast<GraphNode_Unset*>(pNode)->~GraphNode_Unset(); break;
	case IAISystem::NAV_TRIANGULAR: static_cast<GraphNode_Triangular*>(pNode)->~GraphNode_Triangular(); break;
	case IAISystem::NAV_WAYPOINT_HUMAN: static_cast<GraphNode_WaypointHuman*>(pNode)->~GraphNode_WaypointHuman(); break;
	case IAISystem::NAV_WAYPOINT_3DSURFACE: static_cast<GraphNode_Waypoint3DSurface*>(pNode)->~GraphNode_Waypoint3DSurface(); break;
	case IAISystem::NAV_FLIGHT: static_cast<GraphNode_Flight*>(pNode)->~GraphNode_Flight(); break;
	case IAISystem::NAV_VOLUME: static_cast<GraphNode_Volume*>(pNode)->~GraphNode_Volume(); break;
	case IAISystem::NAV_ROAD: static_cast<GraphNode_Road*>(pNode)->~GraphNode_Road(); break;
	case IAISystem::NAV_SMARTOBJECT: static_cast<GraphNode_SmartObject*>(pNode)->~GraphNode_SmartObject(); break;
	case IAISystem::NAV_FREE_2D: static_cast<GraphNode_Free2D*>(pNode)->~GraphNode_Free2D(); break;
	case IAISystem::NAV_LAYERED_NAV_MESH: static_cast<GraphNode_LayeredNavMesh*>(pNode)->~GraphNode_LayeredNavMesh(); break;
	case IAISystem::NAV_CUSTOM_NAVIGATION: static_cast<GraphNode_CustomNav*>(pNode)->~GraphNode_CustomNav(); break;
	default: break;
	}

	// TODO: Free memory
}

size_t CGraphNodeManager::NodeMemorySize () const
{
	size_t mem = 0;

	for (int i = 0, count = int(m_buckets.size()); i < count; ++i)
	{
		if ( ! m_buckets[i])
			continue;
		unsigned typeSize = TypeSizeFromTypeIndex (TypeIndexFromType (m_buckets[i]->type));
		mem += sizeof(BucketHeader) + typeSize * BUCKET_SIZE;
	}
	return mem;
}

