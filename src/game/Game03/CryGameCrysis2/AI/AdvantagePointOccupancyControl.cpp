#include "StdAfx.h"
#include "AdvantagePointOccupancyControl.h"

void CAdvantagePointOccupancyControl::ResetAdvantagePointOccupancyControl()
{
	m_occupiedAdvantagePoints.clear();
}

void CAdvantagePointOccupancyControl::OccupyAdvantagePoint( EntityId entityId, const Vec3& position )
{
	std::pair<OccupiedAdvantagePoints::iterator, bool> ret = m_occupiedAdvantagePoints.insert(std::make_pair(entityId, position));
	if(ret.second == false)
	{
		ret.first->second = position;
	}
}

void CAdvantagePointOccupancyControl::ReleaseAdvantagePoint( EntityId entityId )
{
	m_occupiedAdvantagePoints.erase(entityId);
}

bool CAdvantagePointOccupancyControl::IsAdvantagePointOccupied( const Vec3& position )
{
	OccupiedAdvantagePoints::iterator it = m_occupiedAdvantagePoints.begin();
	OccupiedAdvantagePoints::iterator itEnd = m_occupiedAdvantagePoints.end();

	while(it != itEnd)
	{
		if(MatchAdvantagePointPosition(position, (*it).second))
			return true;
		else
			++it;
	}
	return false;
}

bool CAdvantagePointOccupancyControl::MatchAdvantagePointPosition( const Vec3& position, const Vec3& AdvantagePoint ) const
{
	return ( (AdvantagePoint - position).len2() < 0.25f * 0.25f );
}
