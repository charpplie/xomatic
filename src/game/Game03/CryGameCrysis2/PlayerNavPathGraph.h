/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Player nav path tag point cloud class

-------------------------------------------------------------------------
History:
- 26:01:2010: Created by Jan Neugebauer

*************************************************************************/

#ifndef __PLAYERNAVPATHGRAPH_H__
#define __PLAYERNAVPATHGRAPH_H__


//////////////////////////////////////////////////////////////////////////


class CPlayerNavPathGraph
{
public:

	struct SNavTagPoint
	{
		SNavTagPoint()
			: m_pos(ZERO)
			, m_name("Unknown")
		{
			m_links.resize(0);
		}

		SNavTagPoint(const Vec3& pos, const char* name)
			: m_pos(pos)
			, m_name(name)
		{
			m_links.resize(0);
		}

		typedef std::vector<int> TLinks;

		CryFixedStringT<32> m_name;
		TLinks		m_links;
		Vec3			m_pos;
	};

	typedef std::vector<SNavTagPoint> TCloud;

	int GetBestNodeIndex(Vec3& referencePoint) const;
	Vec3 GetNodeReferencePoint(int nodeIndex) const;

	int GetNumLinks(int nodeIndex) const;
	int GetNextNodeIndex(int graphNodeIndex, int linkIndex) const;

	CPlayerNavPathGraph();
	~CPlayerNavPathGraph();

	void GetClosestNavNode(const Vec3& position, CPlayerNavPathGraph::SNavTagPoint& outPoint) const;

	void Recalculate(const EntityId startId = 0);
	bool IsValid() const;

private:

	bool AddEntityToCloudRecursive(const EntityId entityId, int& outIndex);
	void LinkNavNodes(const int parent, const int child);

	TCloud		m_cloud;


};


//////////////////////////////////////////////////////////////////////////


struct CPlayerNavPathHeuristics
{
	float H(const CPlayerNavPathGraph* graph, int currentNodeIndex, int endNodeIndex) const;
};


//////////////////////////////////////////////////////////////////////////


struct CPlayerNavPathCostCalculator
{
	float GetCost(const CPlayerNavPathGraph* graph, int currentNodeIndex, int nextNodeIndex) const;
};


#endif //__PLAYERNAVPATHGRAPH_H__
