#ifndef __SMARTOBJECTNAVREGION_H__
#define __SMARTOBJECTNAVREGION_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "NavRegion.h"

class CGraph;

/// Handles all graph operations that relate to the smart objects aspect
class CSmartObjectNavRegion : public CNavRegion
{
public:
	CSmartObjectNavRegion( CGraph* pGraph )
	{
		m_pGraph = pGraph;
	}
	virtual ~CSmartObjectNavRegion() {}

	/// inherited
	virtual unsigned GetEnclosing(const Vec3 &pos, float passRadius = 0.0f, unsigned startIndex = 0, 
		float range = -1.0f, Vec3 * closestValid = 0, bool returnSuspect = false, const char *requesterName = "")
	{
		return 0;
	}

	/// Serialise the _modifications_ since load-time
	virtual void Serialize(TSerialize ser, class CObjectTracker& objectTracker)
	{
		//ser.BeginGroup("SmartObjectNavRegion");
		//ser.EndGroup();
	}

	/// inherited
	virtual void Clear()
	{
		// remove all smart object nodes
		m_pGraph->DeleteGraph( IAISystem::NAV_SMARTOBJECT );

//		CAllNodesContainer& allNodes = m_pGraph->GetAllNodes();
//		CAllNodesContainer::Iterator it( allNodes, IAISystem::NAV_SMARTOBJECT );
//		GraphNode* pNode;
//		while ( pNode = it.GetNode() )
//		{
//			m_pGraph->Disconnect( pNode, true );
//			it = allNodes.Erase( it );
//		}
	}

	/// inherited
	virtual size_t MemStats()
	{
		size_t size = sizeof( *this );
		if ( m_pGraph )
			size += m_pGraph->NodeMemStats( IAISystem::NAV_SMARTOBJECT );
		return size;
	}

private:
	CGraph* m_pGraph;
};

#endif // __SMARTOBJECTNAVREGION_H__
