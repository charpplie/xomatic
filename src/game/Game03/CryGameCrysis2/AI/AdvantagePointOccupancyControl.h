#pragma once

#ifndef AdvantagePointOccupancyControl_h
#define AdvantagePointOccupancyControl_h

class CAdvantagePointOccupancyControl
{
	typedef std::map<EntityId, Vec3> OccupiedAdvantagePoints;

public:
	void ResetAdvantagePointOccupancyControl();
	void OccupyAdvantagePoint(EntityId entityId, const Vec3& position);
	void ReleaseAdvantagePoint(EntityId entityId);
	bool IsAdvantagePointOccupied(const Vec3& position);

private:

	bool MatchAdvantagePointPosition(const Vec3& position, const Vec3& AdvantagePoint) const;

	// TODO: Investigate if we can use tAIObjectID instead of position /Mario
	OccupiedAdvantagePoints m_occupiedAdvantagePoints;
};

#endif //AdvantagePointOccupancyControl_h