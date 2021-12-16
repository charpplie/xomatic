#ifndef __HUD_RADAR_H__
#define __HUD_RADAR_H__

struct IActor;
#include "HUDObject.h"

#include "HUD/HUD.h"
#include "HUD/HUD_RadarIcon.h"

//-----------------------------------------------------------------------------------------------------

struct IFlashPlayer;                                 

class CHUD_Radar : public CHUDObject
{
private:

	const static int k_maxFlashIconsSupported = 32;

	enum GlobalDirtyFlags
	{
		eGDF_None				= 0,
		eGDF_Rotation		= BIT(0),
		EGDF_All				= 0xFFFF
	};

	struct SHUDLocalInfo
	{
		SHUDLocalInfo()
			: m_size(-1.0f, -1.0f)
			, m_relPos(0.0f, 0.0f)
		{
		}

		void Read(const IItemParamsNode* node);

		Vec2 m_size;
		Vec2 m_relPos;
	};



	struct SHUDRadar_FlashData
	{	
		struct SHUD_RadarEntVarObj
		{
			struct SHUD_RadarAlertness
			{
				SHUD_RadarAlertness();
				void Init( IHUDAsset* pAsset, const int i, const int frameNum );
				void Clear();
				bool IsValid() const;
				
				//-----
				IFlashVariableObject* m_up;
				IFlashVariableObject* m_ground;
				IFlashVariableObject* m_down;			
			};
			
			//-----
			SHUD_RadarEntVarObj();
			void Init( IHUDAsset* pAsset, const int id );
			void Clear();
			bool IsValid() const;

			//-----
			SHUD_RadarAlertness   m_alertness[4];
			IFlashVariableObject* m_ent;
			IFlashVariableObject* m_icons;
		};

		//-----
		SHUDRadar_FlashData();
		void Init( IHUDAsset* pAsset );
		void Clear();
		bool IsValid() const;
		
		//-----
		SHUD_RadarEntVarObj   m_entReps[k_maxFlashIconsSupported];
		IFlashVariableObject* m_player;
		IFlashVariableObject* m_scan;
		IFlashVariableObject* m_root;
	};

public:
	CHUD_Radar();
	virtual ~CHUD_Radar();

	virtual void Init();
	virtual void PreDelete();
	virtual void Initialize(const IItemParamsNode * object);

	void Update	(float frameTime);
	void Draw		();
	void Reload	();

	void OnHUDEvent(const SHUDEvent& event);

	void ScheduleRemoveEntity(EntityId id);

private:

	void UpdateClientStats();
	void UpdateClientMatrices( float worldAngle, Vec3 worldPos, const Vec3& mapCenter, float mapRange);

	void UpdateEntityStats(CHUD_RadarIcon& entry, IEntity* pEntity);
	void UpdateEdgeEntity(CHUD_RadarIcon& entry);
	void UpdateRadarEntities(const float frameTime);

	void SwitchTeam(EntityId id);
	void RescanActors();

	void AddEntity(const EntityId id, const float temporary, const bool firing);
	void RemoveEntity(EntityId id);

	bool ShouldAppearOnRadar(CHUD_RadarIcon& entry, IEntity* pEntity, const float frameTime);

	bool SetVisible(const float frameTime, const int index, CHUD_RadarIcon& entry, IEntity* pEntity);

	bool ShouldBeAddedOnRadar(const EntityId entity, const bool noEnemies, const bool onlyHuman = false);
	void SetupEntity(CHUD_RadarIcon& outEntry, const EntityId entityId, const float time, const bool firing, const bool friendly);
	CHUD_RadarIcon::EIcon GetIconFromEntity(EntityId id, const bool friendly);

	bool IsTeamRadarActive(EntityId clientId) const;
	void OnTeamRadarChanged(const bool isActive);
	void UpdateActorSpotting(IActor* pActor);

	bool IsFriendlyToClient(const EntityId id) const;

	void ToggleMapMode( const bool showMenuMap, const float frameTime );

	CHUD_RadarIcon					m_entries[k_maxFlashIconsSupported];
	std::vector<EntityId>		m_scheduledRemoves;

	SHUDLocalInfo						m_mapInfo;
	SHUDLocalInfo						m_radarInfo;

	Matrix33								m_worldRotationMat;
	Matrix33								m_worldTranslationMat;
	Matrix33								m_screenPointMat;

	Vec2										m_mapCenter;

	const CHUD_RadarIcon*		m_clientEntry;
	IHUDAsset*							m_radar;

	IFlashVariableObject*		m_compass;
	IFlashVariableObject*		m_player;
	IFlashVariableObject*		m_scan;
	IFlashVariableObject*		m_root;

	float										m_levelSpecificCompassRotation;
	float										m_clientRotation;
	float										m_clientHeight;
	float										m_mapRotation;
	float										m_mapScale;

	int											m_sizeOfEntities;

	uint16									m_globalDirty;

	bool										m_visibility[k_maxFlashIconsSupported];
	bool										m_useMenuMap;
	bool                    m_showMenuMapNextFrame;
	bool                    m_transitioningBetweenMaps;
	bool                    m_blind;

#if ENABLE_HUD_EXTRA_DEBUG
	void debugRadar(int i, CHUD_RadarIcon& entry, IEntity* pEntity, IActor* client);
#endif
};


#endif

