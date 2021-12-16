#include "StdAfx.h"
#include "GraphStructures.h"

GraphNode::GraphNode(IAISystem::ENavigationType type, const Vec3 &inpos, unsigned int _ID)
: navType(type), pos(inpos)
{
	firstLinkIndex = 0;
	mark = 0;
	nRefCount = 0;
	if (_ID == 0)
	{
		if (freeIDs.empty())
		{
			ID = maxID + 1;
		}
		else
		{
			ID = freeIDs.back();
			freeIDs.pop_back();
		}
	}
	else
	{
		ID = _ID;
	}
	if (ID > maxID)
		maxID = ID;

}

GraphNode::~GraphNode()
{
  freeIDs.push_back(ID);
}

//====================================================================
// Serialize
//====================================================================
void ObstacleData::Serialize(TSerialize ser)
{
	ser.Value("vPos", vPos);
	ser.Value("vDir", vDir);
	ser.Value("fApproxRadius",fApproxRadius);
	ser.Value("flags",flags);
	if (ser.IsReading())
	{
		navNodes.clear();
		needToEvaluateNavNodes = true;
	}
}

//===================================================================
// SetNavNodes
//===================================================================
void ObstacleData::SetNavNodes(const std::vector<const GraphNode *> &nodes)
{
	navNodes = nodes;
	needToEvaluateNavNodes = false;
}

//===================================================================
// AddNavNode
//===================================================================
void ObstacleData::AddNavNode(const GraphNode * pNode)
{
	needToEvaluateNavNodes = false;
	navNodes.push_back(pNode);
}


//===================================================================
// GetNavNodes
//===================================================================
const std::vector<const GraphNode *> &ObstacleData::GetNavNodes() const
{
	if (!needToEvaluateNavNodes)
		return navNodes;

	needToEvaluateNavNodes = false;

	// If we're in a forbidden region then sample outside the obstacle just like will be
	// done when finding hidespots
	if (IsCollidable() && fApproxRadius > 0.1f && gAIEnv.pNavigation->IsPointInForbiddenRegion(vPos))
	{
		float circ = gf_PI2 * fApproxRadius;
		int numSamples = (int)(circ * 2.f);
		Limit(numSamples, 4, 10);
		Vec3 offset(ZERO);
		Vec3 hideDir;
		std::set<GraphNode *> checkedNodes;
		unsigned navNodeIndex = 0;
		for (int iSample = 0 ; iSample < numSamples ; ++iSample)
		{
			float angle = iSample * gf_PI2 / numSamples;
			//    cry_sincosf(angle, &offset.x); // assumes a little about the layout of Vec3 :)
			sincos_tpl(angle, &offset.y,&offset.x); // assumes a little about the layout of Vec3 :)
			Vec3 pos = vPos;
			GetAISystem()->AdjustOmniDirectionalCoverPosition(pos, hideDir, max(fApproxRadius, 0.0f), 0.5f, pos + offset);
			navNodeIndex = gAIEnv.pGraph->GetEnclosing(pos, IAISystem::NAV_TRIANGULAR, 0.0f, navNodeIndex);
			GraphNode* pNavNode = gAIEnv.pGraph->GetNode(navNodeIndex);
			if (!pNavNode)
				continue;
			if (checkedNodes.insert(pNavNode).second)
				navNodes.push_back(pNavNode);
		}
	}
	else
	{
		unsigned navNodeIndex = gAIEnv.pGraph->GetEnclosing(vPos, IAISystem::NAV_TRIANGULAR);
		GraphNode *pNavNode = gAIEnv.pGraph->GetNode(navNodeIndex);
		if (!pNavNode)
			return navNodes;

		// pNavNode points to one triangle - now walk around this obstacles adding all nodes
		std::set<GraphNode *> checkedNodes;

		navNodes.push_back(pNavNode);
		checkedNodes.insert(pNavNode);

		while (true)
		{
			//unsigned nLinks = pNavNode->links.size();
			//unsigned iLink;
			//for (iLink = 0 ; iLink < nLinks ; ++iLink)
			CGraphLinkManager& linkManager = gAIEnv.pGraph->GetLinkManager();
			CGraphNodeManager& nodeManager = gAIEnv.pGraph->GetNodeManager();
			unsigned linkId;
			for (linkId = pNavNode->firstLinkIndex; linkId; linkId = linkManager.GetNextLink(linkId))
			{
				unsigned nextIndex = linkManager.GetNextNode(linkId);
				GraphNode *pNext = nodeManager.GetNode(nextIndex);

				// even if pNext turns out not to be touching the obstacle there are still
				// cases where it touches two triangles that are touching the obstacle
				if (!checkedNodes.insert(pNext).second)
					continue;
				if (pNext->navType != IAISystem::NAV_TRIANGULAR)
					continue;

				STriangularNavData *triData = pNext->GetTriangularNavData();
				unsigned nVerts = triData->vertices.size();
				unsigned iVert;
				for (iVert = 0 ; iVert < nVerts ; ++iVert)
				{
					int index = triData->vertices[iVert];
					const ObstacleData &obstacle = GetAISystem()->m_VertexList.GetVertex(index);
					if (&obstacle == this)
						break;
				}
				if (iVert != nVerts)
				{
					// found another good triangle
					pNavNode = pNext;
					navNodes.push_back(pNavNode);
					checkedNodes.insert(pNavNode);
					break;
				}
			}
			if (!linkId)
				break;
		}
	}
	return navNodes;
}
