#ifndef __TACTICALMANAGER_H__
#define __TACTICALMANAGER_H__

//////////////////////////////////////////////////////////////////////////


class CTacticalManager
{

public:

	enum ETacticalEntityType
	{
		eTacticalEntity_Story,
		eTacticalEntity_Item,
		eTacticalEntity_Unit,
	};

	enum EScanningCount
	{
		eScanned_None,
		eScanned_Once,
		eScanned_Max
	};

	typedef uint8 TScanningCount;

	struct STacticalInterestPoint
	{
		STacticalInterestPoint(const EntityId id)
			: m_entityId(id)
			, m_visible(false)
			, m_scanned(eScanned_None)
		{

		}

		EntityId				m_entityId;
		TScanningCount	m_scanned;
		bool						m_visible;
	};

	typedef std::vector<STacticalInterestPoint> TInterestPoints;

public:

														CTacticalManager();
	virtual										~CTacticalManager();

	void											AddEntity(const EntityId id, ETacticalEntityType type);
	void											RemoveEntity(const EntityId id, ETacticalEntityType type);
	void											SetEntityScanned(const EntityId id);
	void											SetEntityScanned(const EntityId id, ETacticalEntityType type);

	const TInterestPoints&		GetTacticalPoints(const ETacticalEntityType type) const;

private:

	TInterestPoints*					GetTacticalPointsInternal(const ETacticalEntityType type);

	TInterestPoints						m_story;
	TInterestPoints						m_items;
	TInterestPoints						m_units;
};


//////////////////////////////////////////////////////////////////////////

#endif

