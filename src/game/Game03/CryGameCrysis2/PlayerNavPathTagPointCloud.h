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

#ifndef __PLAYERNAVPATHTAGPOINTCLOUD_H__
#define __PLAYERNAVPATHTAGPOINTCLOUD_H__


class CPlayerNavPathTagPointCloud
{
public:

	struct SNavTagPoint
	{
		SNavTagPoint()
			: m_entityId(0)
			, m_pos(ZERO)
			, m_navNodeId(0)
		{
			m_links.resize(0);
		}

		SNavTagPoint(const EntityId entityId, const Vec3& pos, const uint32 navNodeId)
			: m_entityId(entityId)
			, m_pos(pos)
			, m_navNodeId(navNodeId)
		{
			m_links.resize(0);
		}

		typedef std::vector<uint32> TLinks;

		TLinks		m_links;
		Vec3			m_pos;
		EntityId	m_entityId;
		uint32		m_navNodeId;
	};

	typedef std::vector<SNavTagPoint> TCloud;

	CPlayerNavPathTagPointCloud();
	~CPlayerNavPathTagPointCloud();

	void Recalculate(const EntityId startId = 0);
	void Clear();
	bool IsValid() const;

	void GetClosestNavNode(const Vec3& position, CPlayerNavPathTagPointCloud::SNavTagPoint& outPoint) const;

private:

	bool AddEntityToCloudRecursive(const EntityId entityId, uint32& outNavNodeId);
	void LinkNavNodes(const uint32 parent, const uint32 child);

	bool IsAlreadyRegistered(const EntityId entityId) const;
	
	TCloud		m_cloud;
	EntityId	m_startEntity;

};

#endif //__PLAYERNAVPATH_H__
