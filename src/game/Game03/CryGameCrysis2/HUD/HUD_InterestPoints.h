#ifndef __HUDINTERESTPOINTS_H__
#define __HUDINTERESTPOINTS_H__

#include "HUDObject.h"
#include "TacticalManager.h"
#include "ContextualInfo.h"
#include "NanoSuitDefs.h"
#include "Utility/CryFixedArray.h"


//////////////////////////////////////////////////////////////////////////


class CHUD_InterestPoints : public CHUDObject
{

public:

	enum DirtyFlags
	{
		eDF_None						= 0<<0,
		eDF_Info						= BIT(0),
		eDF_All							= 0xFFFF
	};

	CHUD_InterestPoints();
	virtual ~CHUD_InterestPoints();

	virtual void		Update	(float frameTime);
	void						Reload();

	virtual void		OnHUDEvent( const SHUDEvent& event );
	virtual void		Init();
	virtual void		PreDelete();


	struct STacticalInfoDefinition
	{
		string m_primary;
		string m_secondary;
		string m_tertiary;
	};


	struct SInterestPoint
	{

	public:

		enum DirtyFlags
		{
			eDF_None						= 0<<0,
			eDF_PosX						= BIT(0),
			eDF_PosY						= BIT(1),
			eDF_Distance				= BIT(2),
			eDF_All							= 0xFFFF
		};

		SInterestPoint()
			: m_entity(0)
			, m_screenPos(0.5f, 0.5f)
			, m_screenSize(0.1f)
			, m_objective(true)
			, m_distance(0.0f)
			, m_tacticalInfoClass("")
			, m_alpha(1.0f)
			, m_dirty(SInterestPoint::eDF_None)
			, m_scanned(CTacticalManager::eScanned_None)
		{}

		void SetDistance(const float distance)
		{
			if(m_distance != distance)
			{
				m_distance = distance;
				m_dirty |= SInterestPoint::eDF_Distance;
			}
		}

		void SetPos(const Vec3& pos)
		{
			if(m_screenPos.x != pos.x)
				m_dirty |= SInterestPoint::eDF_PosX;

			if(m_screenPos.y != pos.y)
				m_dirty |= SInterestPoint::eDF_PosY;

			m_screenPos.x = pos.x;
			m_screenPos.y = pos.y;
		}

		CryFixedStringT<128>	m_tacticalInfoClass;
		Vec2				m_screenPos;
		EntityId		m_entity;

		float				m_distance;
		float				m_screenSize;
		float				m_alpha;
		CTacticalManager::TScanningCount	m_scanned;

		uint16			m_dirty;

		bool				m_objective;
	};

private:

	void UpdateAllIcons();

	void InitTacticalInfoDefinitions();
	void AddTacticalInfoDefinition(const char* infoClass, const char* primary = NULL, const char* secondary = NULL, const char* tertiary  = NULL);
	const STacticalInfoDefinition* GetTacticalInfoDefinition(const char* infoClass, const EntityId entity, const bool warn = true);

	void QueryMissionObjectives();
	void QueryAllInterestPoints();
	void QueryInterestPointsType(const CTacticalManager::ETacticalEntityType type, const float distance);
	bool QueryInterestPoint(SInterestPoint& outPoint, IEntity* pEntity, const Vec3& clientPos, const float distanceSq);

	void UpdateAllInterestPoints();
	void UpdateResolutionDependentData(const float width, const float height);

	int GetNearestToCenter();

	void SetSuitMode(const ENanoSuitMode mode);

	static const int k_maxFlashIconsSupported = 8;

	typedef std::vector<SInterestPoint> Points;
	Points	m_interestPoints;

	typedef std::map<string, STacticalInfoDefinition> TacticalInfoDefinitions;
	TacticalInfoDefinitions m_infoDefinitions;

	IFlashVariableObject* m_icons[k_maxFlashIconsSupported];
	IFlashVariableObject* m_arrows[k_maxFlashIconsSupported];

	CryFixedArray<IEntityClass*, 7> m_ignoreClasses;

#ifndef _RELEASE
	std::vector<EntityId> m_warnings;
#endif //_RELEASE

	CContextualInfo	m_contextualInfo;

	Vec2		m_edge;
	Vec2		m_sizeRatio;
	Vec2		m_maxDistanceToCenter;

	const STacticalInfoDefinition* m_nearestToCenterTacticalInfo;
	ITexture* m_pFakeTexture;
	IFlashVariableObject* m_attentionObject;

	float		m_queryTimer;
	float		m_xOffset;

	int			m_lastFrameCount;
	int			m_lastFrameNearestToCenter;

	EntityId m_nearestEntityToCenter;
	EntityId m_lookAt;

	uint16	m_dirty;

	bool		m_visorActive;

	bool		m_attentionVisible;
};


//////////////////////////////////////////////////////////////////////////

#endif

