#include "StdAfx.h"
#include "Flash/Flash.h"
#include "HUD_Radar.h"
#include "HUD/HUD.h"
#include "HUD/HUDDefines.h"
#include "HUD/HUDCVars.h"
#include "IActorSystem.h"
#include "Player.h"
#include "GameCVars.h"
#include "Audio/AudioSignalPlayer.h"
#include "Utility/CryWatch.h"
#include "Utility/StringUtils.h"
#include "TeamPerks.h"
#include "GameRules.h"
#include "IWorldQuery.h"
#include "HUD/HUDMissionObjectiveSystem.h"

static IEntityClass* sFlagClass = NULL;
static IEntityClass* sPlayerClass = NULL;
static IEntityClass* sDummyPlayerClass = NULL;
static IEntityClass* sHologramClass = NULL;
static IEntityClass* sPSLCapturePointClass = NULL;
static IEntityClass* sCSLocationClass = NULL;
static IEntityClass* sCDTimeBonusLocationClass = NULL;
static IEntityClass* sASIntelPointClass = NULL;

//////////////////////////////////////////////////////////////////////////


CHUD_Radar::CHUD_Radar() :
m_globalDirty(CHUD_Radar::eGDF_None),
m_clientRotation(0.0f),
m_clientHeight(0.0f),
m_levelSpecificCompassRotation(0.0f),
m_useMenuMap(false),
m_showMenuMapNextFrame(false),
m_transitioningBetweenMaps(false),
m_player(NULL),
m_scan(NULL),
m_root(NULL),
m_compass(NULL)
{
	m_sizeOfEntities = 0;
	m_radar = NULL;
	sFlagClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("CTFFlag");
	sPlayerClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Player");
	sDummyPlayerClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("DummyPlayer");
	sHologramClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Hologram");
	sPSLCapturePointClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("PSLCapturePoint");
	sCSLocationClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("CrashSiteLocation");
	sCDTimeBonusLocationClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("TimeBonusLocation");
	sASIntelPointClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("AssaultIntel");
	for(int i=0; i<k_maxFlashIconsSupported; ++i)
	{
		m_visibility[i] = false;
	}
	m_mapCenter.zero();
	m_mapRotation = 0.0f;
	m_mapScale = 1.0f;
	m_clientEntry = NULL;
}

void CHUD_Radar::Init()
{
	m_radar = GetAsset();

	SHUDEvent relaignRadarTextureEvent(eHUDEvent_AlignToRadarAsset);
	relaignRadarTextureEvent.AddData(m_radar);
	CHUD::CallEvent(relaignRadarTextureEvent);

	IEntity* pEntity = gEnv->pEntitySystem->FindEntityByName("ThisIsNorth");
	if(pEntity)
	{
		const Vec3&	dir  = pEntity->GetForwardDir();
		m_levelSpecificCompassRotation = atan2f( -dir.x, dir.y );
	}

	m_radar->Update(0.0f);

	HUD_FLASVAROBJ_REG(m_radar, "Root_Radar.Green.Compass", m_compass);
	HUD_FLASVAROBJ_REG(m_radar, "Root_Radar.Player", m_player);
	HUD_FLASVAROBJ_REG(m_radar, "Root_Radar.Scan", m_scan);
	HUD_FLASVAROBJ_REG(m_radar, "Root_Radar", m_root);

	Vec2 innerOffset(0.0f, 0.0f);

	SFlashDisplayInfo rdi;
	m_player->GetDisplayInfo(rdi);
	innerOffset.x = rdi.GetX();
	innerOffset.y = rdi.GetY();	

	for(int i=0; i<k_maxFlashIconsSupported; ++i)
	{
		m_entries[i].Init(this, m_radar, i, innerOffset);
	}
}

void CHUD_Radar::PreDelete()
{
	HUD_FLASHOBJ_SAFERELEASE(m_compass);
	HUD_FLASHOBJ_SAFERELEASE(m_root);
	HUD_FLASHOBJ_SAFERELEASE(m_player);
	HUD_FLASHOBJ_SAFERELEASE(m_scan);

	for(int i=0; i<k_maxFlashIconsSupported; ++i)
	{
		m_entries[i].PreDelete();
	}
}

void CHUD_Radar::Initialize(const IItemParamsNode * object)
{
	CHUDObject::Initialize(object);

	const IItemParamsNode* data = object->GetChild("Data");
	if(data)
	{
		const IItemParamsNode* mapSettings = data->GetChild("FullscreenMapSettings");
		if(mapSettings)
		{
			m_mapInfo.Read(mapSettings);
		}
	}

	const IItemParamsNode* assets = object->GetChild("Assets");
	if(assets)
	{
		const IItemParamsNode* gfx = assets->GetChild("Gfx");
		if(gfx)
		{
			m_radarInfo.Read(gfx);
		}
	}

}

void CHUD_Radar::SHUDLocalInfo::Read(const IItemParamsNode* node)
{
	const IItemParamsNode * pos = node->GetChild("Position");
	if(pos)
	{
		pos->GetAttribute("x", m_relPos.x);
		pos->GetAttribute("y", m_relPos.y);
	}

	const IItemParamsNode * size = node->GetChild("Size");
	if(size)
	{
		size->GetAttribute("x", m_size.x);
		size->GetAttribute("y", m_size.y);
	}
}

CHUD_Radar::~CHUD_Radar()
{
}

void CHUD_Radar::ToggleMapMode(const bool showMenuMap, const float frameTime)
{
	m_useMenuMap = showMenuMap;

	SHUDLocalInfo newInfo = m_useMenuMap ? m_mapInfo : m_radarInfo;

	m_radar->SetPositionAndSize(newInfo.m_relPos.x, newInfo.m_relPos.y, newInfo.m_size.x, newInfo.m_size.y);

	// Update bg texture.
	SHUDEvent relaignRadarTextureEvent(eHUDEvent_AlignToRadarAsset);
	relaignRadarTextureEvent.AddData(m_radar);
	CHUD::CallEvent(relaignRadarTextureEvent);

	m_radar->Invoke("gotoAndStop", m_useMenuMap ? "Map" : "Radar");

	//Need to reset everything so that flash is correct
	for(int i=0; i<k_maxFlashIconsSupported; ++i)
	{
		m_visibility[i] = false;
	}

	// Update entities
	for(int i = 0; i < m_sizeOfEntities; i++)
	{
		CHUD_RadarIcon& entry = m_entries[i];
		entry.m_dirty = CHUD_RadarIcon::eDF_All;
	}

	// Update BG UI-Texture
	m_globalDirty |= CHUD_Radar::eGDF_Rotation;

	m_transitioningBetweenMaps = true;
}


void CHUD_Radar::Update(float frameTime)
{
	if( m_showMenuMapNextFrame != m_useMenuMap )
	{
		// need to change.
		ToggleMapMode(m_showMenuMapNextFrame, frameTime);
	}

	IActor* pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor)
		return;

	UpdateClientStats();

	if(m_globalDirty & CHUD_Radar::eGDF_Rotation)
	{
		const float compassRotation = RAD2DEG(m_clientRotation - m_levelSpecificCompassRotation);
		SFlashDisplayInfo info;
		info.SetRotation(compassRotation);
		m_compass->SetDisplayInfo(info);

		const float mapRotation = RAD2DEG(m_clientRotation);
		SHUDEvent globalRadarRotateEvent(eHUDEvent_RadarRotation);
		globalRadarRotateEvent.AddData(mapRotation);
		CHUD::CallEvent(globalRadarRotateEvent);
	}

	PrefetchLine(&m_entries[0], 0);
	for(int i = 0; i < m_sizeOfEntities; i++)
	{
		PrefetchLine(&m_entries[i], 128);
		CHUD_RadarIcon& entry = m_entries[i];
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entry.m_entityId);
		if(!pEntity)
		{
			ScheduleRemoveEntity(entry.m_entityId);
			continue;
		}

#if ENABLE_HUD_EXTRA_DEBUG
		debugRadar(i, entry, pEntity, pClientActor);
#endif

		if(!SetVisible(frameTime, i, entry, pEntity))
		{
			continue;
		}

		UpdateEntityStats(entry, pEntity);
		entry.Update(frameTime);
	}

	const int removeSize = m_scheduledRemoves.size();
	for(int i=0; i<removeSize; ++i)
	{
		RemoveEntity(m_scheduledRemoves[i]);
	}
	m_scheduledRemoves.clear();

	m_globalDirty = CHUD_Radar::eGDF_None;
}

bool CHUD_Radar::SetVisible(const float frameTime, const int index, CHUD_RadarIcon& entry, IEntity* pEntity)
{
	bool visible = true;
	
	if(entry.m_icon != CHUD_RadarIcon::eIcon_Objective && !pEntity->IsActive() && !gEnv->bMultiplayer)	//not multiplayer because CaptureObjectives aren't 'Active'
	{
		visible = false;
	}

	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entry.m_entityId);
	if (pActor && pActor->GetHealth()<=0.0f)
	{
		entry.m_timeVisible = 0.0f;	//Stop dead actors being visible when they respawn
		visible = false;
	}

	if (visible)
	{
		if(entry.m_icon == CHUD_RadarIcon::eIcon_Flag || entry.m_icon == CHUD_RadarIcon::eIcon_Objective)
		{
			entry.m_flags |= CHUD_RadarIcon::eRF_NoRot;
			visible = true;
		}
		else if(gEnv->bMultiplayer)
		{
			IActor* pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
			if (!pClientActor || !pClientActor->IsPlayer())
				return false;

			bool teamRadarActive = IsTeamRadarActive(pClientActor->GetEntityId()) || g_pGameCVars->g_mpAllSeeingRadar || g_pGameCVars->g_mpAllSeeingRadarSv;

			if (pActor)
			{
				bool friendly = IsFriendlyToClient(entry.m_entityId);

				if(friendly)
				{
					//This isn't the most obvious place for spotting but it already loop through the actors, getting IActor and checking for friendlies
					UpdateActorSpotting(pActor);
				}

				visible = friendly || (teamRadarActive && ShouldBeAddedOnRadar(entry.m_entityId, false));

				if(entry.m_timeVisible > 0.0f)
				{
					visible = true;
				}

				if(teamRadarActive && !friendly)	//need to set time visible for team radar so enemies alpha is 1
				{
					entry.m_timeVisible = g_pGame->GetHUD()->GetCVars()->hud_radarOnShootTimeOnRadar;
				}

				if(pClientActor->GetEntityId() == entry.m_entityId)
				{
					assert (friendly);
					m_player->SetVisible(false);
					visible = true;
				}
				else
				{
					if (g_pGameCVars->g_mpNoEnemiesOnRadar && !friendly)
					{
						visible = false;
					}
				}
			}
		}
		else
		{
			IActor* pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
			if(pClientActor)
			{
				IEntity* pClientEntity = pClientActor->GetEntity();
				if(pClientEntity)
				{
					IAIObject* pClientAI = pClientEntity->GetAI();
					if(pClientAI)
					{
						IAIObject* pAI = pEntity->GetAI();
						if(!pAI || !pAI->IsHostile(pClientAI, false))
						{
							visible = false;
						}
					}
				}
			}
		}
	}

	if(m_visibility[index] != visible)
	{
		m_visibility[index] = visible;
		entry.m_pFlashData->m_object->SetVisible(visible);
	}

	return visible;
}

bool CHUD_Radar::IsTeamRadarActive(EntityId clientId) const
{
	CGameRules *pGameRules=g_pGame->GetGameRules();
	if (pGameRules)
	{
		int teamId=pGameRules->GetTeam(clientId);
		const CTeamPerks *pTeamPerks=pGameRules->GetTeamPerks(teamId);
		if (pTeamPerks && pTeamPerks->IsTeamPerkActive(eTeamPerk_Radar))
		{
			return true;
		}
	}

	return false;
}

void CHUD_Radar::OnTeamRadarChanged(const bool isActive)
{
	if(!isActive)
	{
		PrefetchLine(&m_entries[m_sizeOfEntities-1], 0);
		EntityId clientId = g_pGame->GetIGameFramework()->GetClientActorId();
		for(int i = (m_sizeOfEntities-1); i>=0; --i)
		{
			PrefetchLine(&m_entries[i], -128);
			if(m_entries[i].m_entityId == clientId)
				continue;

			if(m_entries[i].m_timeVisible > 0.0f)
				continue;
				
			RemoveEntity(m_entries[i].m_entityId);
		}
	}
	RescanActors();
}


bool CHUD_Radar::IsFriendlyToClient(const EntityId id) const
{
	IActor* pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor || !pClientActor->IsPlayer())
		return true;

	CPlayer* pPlayer = static_cast<CPlayer*>(pClientActor);
	return pPlayer->IsFriendlyEntity(id)!=0;
}


void CHUD_Radar::UpdateActorSpotting(IActor* pActor)
{
	CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
	EntityId targetEntityId = pPlayer->GetCurrentTargetEntityId();
	if(targetEntityId != 0)
	{
		const static float k_aimTimeBeforeSpotting = 0.5f;
		if(pPlayer->GetCurrentTargetTime() > k_aimTimeBeforeSpotting)
		{
			const static float k_TimeSpottedPlayersAppearFor = 5.0f;

			SHUDEvent eventTempAddToRadar(eHUDEvent_TemporarilyTrackEntity);
			eventTempAddToRadar.AddData(SHUDEventData((int)targetEntityId));
			eventTempAddToRadar.AddData(SHUDEventData(k_TimeSpottedPlayersAppearFor));
			OnHUDEvent(eventTempAddToRadar);
		}
	}
}

void CHUD_Radar::Draw()
{
	if ( (gEnv->bMultiplayer && g_pGameCVars->g_mpDisableRadar) )
	{
		return;
	}

	// Don't draw when transitioning between states
	if( m_transitioningBetweenMaps )
	{
		m_transitioningBetweenMaps = false;
		return;
	}
}

void CHUD_Radar::UpdateClientStats()
{
	const CCamera& camera = gEnv->pRenderer->GetCamera();

	const Vec3&	clientDir  = camera.GetViewdir();
	float rotation = 0.0f;

	if(!m_useMenuMap)
	{
		rotation = atan2f( -clientDir.x, clientDir.y );
	}
	else
	{
#if ENABLE_HUD_EXTRA_DEBUG
		if(g_pGame->GetHUD()->GetCVars()->hud_radarOverrideMapRotation > 0.0f)
		{
			m_mapRotation = DEG2RAD(g_pGame->GetHUD()->GetCVars()->hud_radarOverrideMapRotation);
		}
#endif
		rotation = m_mapRotation;
	}

	if(rotation != m_clientRotation)
	{
		m_clientRotation = rotation;
		m_globalDirty |= CHUD_Radar::eGDF_Rotation;
	}

	m_clientHeight = camera.GetPosition().z;

	float mapRange = 35.0f;
	if(g_pGame->GetHUD()->GetCVars()->hud_radarOverrideScale > 0.0f)
	{
		mapRange = g_pGame->GetHUD()->GetCVars()->hud_radarOverrideScale;
	}

	Vec3 worldPos;

	if(m_useMenuMap)
	{
#if ENABLE_HUD_EXTRA_DEBUG
		if(g_pGameCVars->hud_mapOverrideCenterX != 0.0f)
		{
			m_mapCenter.x = g_pGameCVars->hud_mapOverrideCenterX;
		}

		if(g_pGameCVars->hud_mapOverrideCenterY != 0.0f)
		{
			m_mapCenter.y = g_pGameCVars->hud_mapOverrideCenterY;
		}
#endif
		worldPos.Set(m_mapCenter.x, m_mapCenter.y, 0.0f);
#if ENABLE_HUD_EXTRA_DEBUG
		if(g_pGame->GetHUD()->GetCVars()->hud_radarOverrideMapScale > 0.0f)
		{
			m_mapScale = g_pGame->GetHUD()->GetCVars()->hud_radarOverrideMapScale;
		}
#endif
		mapRange *= m_mapScale;
	}
	else
	{
		worldPos = camera.GetPosition();
	}
	UpdateClientMatrices(m_clientRotation, worldPos, Vec3(0,0,0), mapRange);
}



void CHUD_Radar::UpdateClientMatrices( float worldAngle, Vec3 worldPos, const Vec3& mapCenter, float mapRange )
{
	float	fMapScrSize				= 80.0f;

	// calculate matrix: m_worldRotationMat;
	{
		Matrix33 MirrorMat;
		MirrorMat.SetIdentity();
		MirrorMat.m11 = -1;  // this actually converts it into an y-mirror matrix. This is needed because orientation in xy world is oposite than screen xy

		Matrix33 RotMat;
		RotMat.SetRotationZ( - worldAngle );  // we want things to rotate in oposite direction than the player
		m_worldRotationMat = MirrorMat * RotMat;
	}


	// calculate matrix: m_screenPointMat
	{
		Matrix33 CenterMat;
		CenterMat.SetIdentity();
		CenterMat.m02 = - mapCenter.x;  
		CenterMat.m12 = - mapCenter.y;  

		Matrix33 RotMat;
		RotMat.SetRotationZ( worldAngle );  // no need to invert angle because in screen it is already inverted

		Matrix33 MoveBackMat;
		MoveBackMat.SetIdentity();
		MoveBackMat.m02 = mapCenter.x; 
		MoveBackMat.m12 = mapCenter.y; 

		m_screenPointMat = MoveBackMat * ( RotMat * CenterMat );
	}


	// calculate matrix: m_worldTranslationMat
	{	
		Matrix33 CenterIntoPlayerMat;
		CenterIntoPlayerMat.SetIdentity();
		CenterIntoPlayerMat.m02 = - worldPos.x;
		CenterIntoPlayerMat.m12 = - worldPos.y;

		float fScale = fMapScrSize / ( mapRange * 2 );
		Matrix33 ScaleMat;
		ScaleMat.SetIdentity();
		ScaleMat.m00 = fScale;
		ScaleMat.m11 = -fScale;  // scale and mirror matrix in one

		Matrix33 RotMat;
		RotMat.SetRotationZ( worldAngle );  // no need to invert angle because in screen it is already inverted

		Matrix33 MoveToScreenMat;
		MoveToScreenMat.SetIdentity();
		MoveToScreenMat.m02 = mapCenter.x;
		MoveToScreenMat.m12 = mapCenter.y;

		m_worldTranslationMat = MoveToScreenMat * ( RotMat * ( ScaleMat * CenterIntoPlayerMat ) );
	}
}

void CHUD_Radar::OnHUDEvent(const SHUDEvent& event)
{
	switch( event.eventType )
	{
	case eHUDEvent_AddEntity :
		{
			EntityId id = (EntityId)event.GetData(0).GetInt();
			if(gEnv->bMultiplayer && ShouldBeAddedOnRadar(id, true))
			{
				AddEntity(id, 0.0f, false);
			}
			else if(g_pGame->GetMOSystem()->GetMissionObjectiveByEntityId(id) || id==g_pGame->GetIGameFramework()->GetClientActorId())
			{
				AddEntity(id, 0.0f, false);
			}
		}
		break;

	case eHUDEvent_RemoveEntity :
		{
			RemoveEntity((EntityId)event.GetData(0).GetInt());
			break;
		}
	case eHUDEvent_OnEntityScanned :
		{
			const EntityId id = (EntityId)event.GetData(0).GetInt();
			if(ShouldBeAddedOnRadar(id, false, true))
			{
				AddEntity(id, 0.0f, false);
			}
			break;
		}
	case eHUDEvent_OnEntitySpotted :
		{
			const EntityId id = (EntityId)(event.GetData(0).GetInt());
			if(ShouldBeAddedOnRadar(id, false, true))
			{
					AddEntity(id, 0.0f, false);
			}
			break;
		}
	case eHUDEvent_RescanActors :
		{
			RescanActors();
			break;
		}
	case eHUDEvent_OnBlind :
		//don't want to alpha out the map screen
		m_radar->SetVariable("_alpha", m_useMenuMap ? 100.0f : event.GetData(0).GetFloat() * 100.0f);
		break;
	case eHUDEvent_OnEndBlind :
	case eHUDEvent_OnSpawn :
		m_radar->SetVariable("_alpha", 100.0f);
		break;
	case eHUDEvent_OnHUDReload :
		{
			IEntity* pEntity = gEnv->pEntitySystem->FindEntityByName("ThisIsNorth");
			if(pEntity)
			{
				const Vec3&	dir  = pEntity->GetForwardDir();
				m_levelSpecificCompassRotation = atan2f( -dir.x, dir.y );
			}
		}
		break;
	case eHUDEvent_OnShoot :
		{
			EntityId shooterId = (EntityId)(event.GetData(0).GetInt());
			if(ShouldBeAddedOnRadar(shooterId, false))
			{
			AddEntity(shooterId, gEnv->bMultiplayer ? g_pGame->GetHUD()->GetCVars()->hud_radarOnShootTimeOnRadar : 0.0f, true);
		}
		}
		break;

	default :
		if(gEnv->bMultiplayer)
		{
			switch( event.eventType )
			{
			case eHUDEvent_TemporarilyTrackEntity :
				{
					EntityId id = (EntityId)event.GetData(0).GetInt();
					float time = event.GetData(1).GetFloat();
					
					if(!ShouldBeAddedOnRadar(id, false))
						return;

					AddEntity(id, time, false);
				}
				break;
			case eHUDEvent_PlayerSwitchTeams :
				{
					SwitchTeam((EntityId)event.GetData(0).GetInt());
					break;
				}
			case eHUDEvent_OnTeamRadarChanged :
				{
					OnTeamRadarChanged(event.GetData(0).GetBool());
					break;
				}
			case eHUDEvent_OnScanPerkChanged :
				{
					SFlashDisplayInfo info;
					info.SetXScale(event.eventFloatData);
					info.SetYScale(event.eventFloatData);
					m_scan->SetDisplayInfo(info);

					if(event.eventIntData==1)
					{
						if(event.eventFloat2Data > 0.0f)
						{
							m_scan->GotoAndStop(2);
							CryFixedStringT<16> dist;
							dist.Format("%.1fm",  event.eventFloat2Data);
							m_root->SetMember("DistanceVar", dist.c_str());
						}
						else
						{
							m_radar->Invoke("Root_Radar.Scan.gotoAndStop", 1);
							m_root->SetMember("DistanceVar", "");
						}
					}
					break;
				}
			case eHUDEvent_MapInfo :
				{
					//IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entry.m_entityId);
					//if( pActor && !pActor->IsDead() )					
					{
						m_mapCenter.x = event.GetData(0).GetFloat();
						m_mapCenter.y = event.GetData(1).GetFloat();
						m_mapRotation = event.GetData(2).GetFloat();
						m_mapScale = event.GetData(3).GetFloat();
					}
				}
				break;
			case eHUDEvent_ShowSelectMenuMap :
				m_showMenuMapNextFrame = true;
				break;
			case eHUDEvent_HideSelectMenuMap :
				m_showMenuMapNextFrame = false;
				break;
			}
		}// end if multiplayer
		break;
	}// end switch
}

bool CHUD_Radar::ShouldBeAddedOnRadar(const EntityId entity, const bool noEnemies, const bool onlyHuman /*= false*/)
{
	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entity);
	if(!pActor)
		return !onlyHuman;

	if(IsFriendlyToClient(entity))
		return true;

	const EntityId clientId = g_pGame->GetIGameFramework()->GetClientActorId();
	if(noEnemies && !IsTeamRadarActive(clientId))
	{
		return false;
	}

	CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
	return !pPlayer->IsPerkActive(ePerk_RadarJammer);
}



void CHUD_Radar::AddEntity(const EntityId id, const float temporary, const bool firing)
{
	PrefetchLine(&m_entries[0], 0);
	int index = m_sizeOfEntities;
	if(index == k_maxFlashIconsSupported)
	{
		CryLog("HUD_Radar::AddEntity - Failed to add entity as we're full");
		return;
	}

	bool friendly = IsFriendlyToClient(id);

	for(int i = 0; i < m_sizeOfEntities; i++)
	{
		PrefetchLine(&m_entries[i], 128);
		CHUD_RadarIcon& entry = m_entries[i];
		if(entry.m_entityId == id)
		{
			//if we are re-adding the same entity just make sure it's got the correct icon
			//(eg re-adding for change teams for local client wasn't initialized yet so rescanning all actors)
			SetupEntity(entry, id, temporary, firing, friendly);
			return;
		}
	}

	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(id);
	if(pEntity && pEntity->GetClass() != sHologramClass)
	{
		CHUD_RadarIcon& entry = m_entries[m_sizeOfEntities];
		SetupEntity(entry, id, temporary, firing, friendly);
		m_visibility[m_sizeOfEntities] = false;
		m_sizeOfEntities++;
		SetVisible(0.0f, index, entry, pEntity);
	}

	CRY_ASSERT_MESSAGE(pEntity, "[HUD]HUD_Radar; Failed to add a null entity to the radar.");
}



void CHUD_Radar::ScheduleRemoveEntity(EntityId id)
{
	m_scheduledRemoves.push_back(id);
}



void CHUD_Radar::RemoveEntity(EntityId id)
{
	PrefetchLine(&m_entries[0], 0);
	for(int i = 0; i < m_sizeOfEntities; i++)
	{
		PrefetchLine(&m_entries[i], 128);
		CHUD_RadarIcon& entry = m_entries[i];
		if(entry.m_entityId == id)
		{
			m_sizeOfEntities--;
			// If we are not removing the last entity, we need to move the last entity into the new gap in the array
			CRY_TODO(14, 10, 2009, "[CG] Make this a bit easier to follow!");
			if (i != m_sizeOfEntities)
			{
				m_entries[i].CopyFrom(m_entries[m_sizeOfEntities]);
				m_entries[i].m_dirty = CHUD_RadarIcon::eDF_All;
				m_visibility[i] = m_visibility[m_sizeOfEntities];
				m_entries[i].m_pFlashData->m_object->SetVisible(m_visibility[i]);

				if(m_entries[i].m_entityId == gEnv->pGame->GetIGameFramework()->GetClientActorId())
				{
					m_clientEntry = &m_entries[i];
				}
			}
			m_visibility[m_sizeOfEntities] = false;
			m_entries[m_sizeOfEntities].m_pFlashData->m_object->SetVisible(false);

			m_entries[m_sizeOfEntities].Clear();
			return;
		}
	}
}

void CHUD_Radar::SwitchTeam(EntityId id)
{
	PrefetchLine(&m_entries[m_sizeOfEntities-1], 0);
	IActor* pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor)
		return;

	bool localPlayer = pClientActor->GetEntityId() == id;
	if(localPlayer)
	{
		EntityId clientId = g_pGame->GetIGameFramework()->GetClientActorId();
		for(int i = (m_sizeOfEntities-1); i>=0; --i)
		{
			PrefetchLine(&m_entries[i], -128);
			if(m_entries[i].m_entityId == clientId)
				continue;

			if(m_entries[i].m_timeVisible > 0.0f)
				continue;

			RemoveEntity(m_entries[i].m_entityId);
		}

		RescanActors();
	}

	for(int i = 0; i < m_sizeOfEntities; i++)
	{
		CHUD_RadarIcon& entry = m_entries[i];

		if(localPlayer || entry.m_entityId == id)
		{
			IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entry.m_entityId);
			if(pEntity)
			{
				SetupEntity(entry, entry.m_entityId, 0.0f, false, IsFriendlyToClient(entry.m_entityId));
			}	
		}
	}
}



void CHUD_Radar::SetupEntity(CHUD_RadarIcon& outEntry, const EntityId entityId, const float time, const bool firing, const bool friendly)
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityId);
	if(!pEntity)
	{
		CRY_ASSERT_MESSAGE(false, "[HUD]HUD_Radar: SetupEntity failed because of invalid Entity.");
		return;
	}

	if(entityId == gEnv->pGame->GetIGameFramework()->GetClientActorId())
	{
		m_clientEntry = &outEntry;
		outEntry.m_icon = CHUD_RadarIcon::eIcon_Player;
	}
	else
	{
		outEntry.m_icon = GetIconFromEntity(entityId, friendly);
	}
	
	outEntry.m_entityId = entityId;
	outEntry.m_flags |= CHUD_RadarIcon::eRF_HasEdge;
	outEntry.m_dirty = CHUD_RadarIcon::eDF_All;
	if(!gEnv->bMultiplayer)
	{
		outEntry.m_flags |= CHUD_RadarIcon::eRF_HasAlertness;
	}

	if(!friendly)
	{
		outEntry.m_timeVisible = time;
		outEntry.m_alpha = 100.0f;
		outEntry.m_flags |= CHUD_RadarIcon::eRF_HasAlpha;
		outEntry.m_dirty |= CHUD_RadarIcon::eDF_Alpha;
	}

	if(firing)
	{
		outEntry.m_dirty |= CHUD_RadarIcon::eDF_Firing;
		if(gEnv->bMultiplayer && !friendly)
		{
			outEntry.m_flags |= CHUD_RadarIcon::eRF_NoUpdatingPos;
			outEntry.m_noUpdatingPos = pEntity->GetWorldPos();
		}
	}
}



CHUD_RadarIcon::EIcon CHUD_Radar::GetIconFromEntity(EntityId id, const bool friendly)
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(id);
	if(!pEntity)
		return CHUD_RadarIcon::eIcon_None;

	IEntityClass* pEntityClass = pEntity->GetClass();
	if(pEntityClass == sFlagClass)
	{
		return CHUD_RadarIcon::eIcon_Flag;;
	}
	else if(	pEntityClass == sPSLCapturePointClass ||
						pEntityClass == sCSLocationClass ||
						pEntityClass == sCDTimeBonusLocationClass ||
						pEntityClass == sASIntelPointClass ||
						g_pGame->GetMOSystem()->GetMissionObjectiveByEntityId(id))
	{
		return CHUD_RadarIcon::eIcon_Objective;
	}
	else if(pEntityClass == sPlayerClass || pEntityClass == sDummyPlayerClass || !gEnv->bMultiplayer)
	{
		if(friendly)
		{
			return CHUD_RadarIcon::eIcon_FriendlyArrow;
		}
		else
		{
			if(gEnv->bMultiplayer)
			{
				return CHUD_RadarIcon::eIcon_EnemyNoArrow;
			}
			else
			{
				return CHUD_RadarIcon::eIcon_EnemyArrow;
			}
		}
	}
	GameWarning("[HUD]HUD_Radar: Failed to find suitable radar Icon for %s of class %s",pEntity->GetName(),  pEntityClass->GetName());
	return CHUD_RadarIcon::eIcon_None;
}



bool CHUD_Radar::ShouldAppearOnRadar(CHUD_RadarIcon& entry, IEntity* pEntity, const float frameTime)
{
	if(!pEntity->IsActive())
	{
		return false;
	}

	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entry.m_entityId);
	if (pActor && pActor->GetHealth()<=0.0f)
	{
		return false;
	}

	return true;
}

void CHUD_Radar::UpdateEntityStats(CHUD_RadarIcon& entry, IEntity* pEntity)
{
	Vec3 vPos	= pEntity->GetWorldPos();

	if(entry.m_flags & CHUD_RadarIcon::eRF_NoUpdatingPos)
	{
		vPos = entry.m_noUpdatingPos;
	}

	float height = vPos.z;

	vPos.z		= 1; // to be able to use the trick of 3x3 matrix as 2d
	vPos			= m_worldTranslationMat.TransformVector( vPos );

	Vec3 vDir = m_worldRotationMat.TransformVector( pEntity->GetForwardDir() );
	vDir.z = 0;
	vDir.normalize();

	const float posX = vPos.x;
	if(entry.m_posX != posX)
	{
		entry.m_posX = posX;
		entry.m_dirty |= CHUD_RadarIcon::eDF_PosX;
	}

	const float posY = vPos.y;
	if(entry.m_posY != posY)
	{
		entry.m_posY = posY;
		entry.m_dirty |= CHUD_RadarIcon::eDF_PosY;
	}

	if(entry.m_flags & CHUD_RadarIcon::eRF_HasAlpha && entry.m_timeVisible > 0.0f)
	{
		const float alpha = clamp((entry.m_timeVisible * __fres(g_pGame->GetHUD()->GetCVars()->hud_radarOnShootTimeOnRadar)), 0.0f, 1.0f) * 100.0f;
		if(alpha != entry.m_alpha)
		{
			entry.m_alpha = alpha;
			entry.m_dirty |= CHUD_RadarIcon::eDF_Alpha;
		}
	}

	CHUD_RadarIcon::EHeightDisplay heightDisplay = CHUD_RadarIcon::eHD_Even;
	const float diff = height - m_clientHeight;
	if(diff > 2.0f)
	{
		heightDisplay = CHUD_RadarIcon::eHD_Above;
	}
	else if(diff < -2.0f)
	{
		heightDisplay = CHUD_RadarIcon::eHD_Below;
	}
	
	if(heightDisplay != entry.m_heightDisplay && !gEnv->bMultiplayer)
	{
		entry.m_heightDisplay = heightDisplay;
		entry.m_dirty |= CHUD_RadarIcon::eDF_Height;
	}

	if(!(entry.m_flags & CHUD_RadarIcon::eRF_NoRot))
	{
		const float rot = 180.0f + RAD2DEG(atan2f( -vDir.x, vDir.y ));
		if(entry.m_rotation != rot)
		{
			entry.m_rotation = rot;
			entry.m_dirty |= CHUD_RadarIcon::eDF_Rot;
		}
	}
	
	if(!gEnv->bMultiplayer)
	{
		if(IAIObject* pAIObject = pEntity->GetAI())
		{
			if(IAIActorProxy* pAIActorProxy = pAIObject->GetProxy())
			{
				const int alertness = pAIActorProxy->GetAlertnessState();
				if(entry.m_alertnessValue != alertness)
				{
					entry.m_alertnessValue = alertness;
					entry.m_dirty |= CHUD_RadarIcon::eDF_Alertness;
				}
			}
		}
	}

	UpdateEdgeEntity(entry);
}

//if an entity is off the radar they show up as an edge entity (changed into an arrow and clamped to edge)
void CHUD_Radar::UpdateEdgeEntity(CHUD_RadarIcon& entry)
{
	//currently only enemies have edge entities (grunt_mp <-> grunt)
	if(m_clientEntry && (entry.m_flags & CHUD_RadarIcon::eRF_HasEdge) && !m_useMenuMap)	//don't do this in the map mode as not centered to player
	{
		Vec3 clientPos = Vec3(m_clientEntry->m_posX, m_clientEntry->m_posY, 0.0f);
		Vec3 radarEntityPos = Vec3(entry.m_posX, entry.m_posY, 0.0f);
		float distanceSq = clientPos.GetSquaredDistance(radarEntityPos);

		static const float k_clampDistanceSq = 1600.0f;
		static const float k_clampDistance = 40.0f;
		if(distanceSq > k_clampDistanceSq)
		{
			//change the icon to an arrow
			if(!(entry.m_flags & CHUD_RadarIcon::eRF_Edge))
			{
				entry.m_tempIcon = (CHUD_RadarIcon::EIcon)((int)entry.m_icon + 1); //following frame should always be the "edged" frame
				entry.m_dirty |= CHUD_RadarIcon::eDF_Icon;
				entry.m_flags |= CHUD_RadarIcon::eRF_Edge;
			}

			//set direction to pointing away from player
			const float rotation = RAD2DEG(atan2(entry.m_posX - m_clientEntry->m_posX, m_clientEntry->m_posY - entry.m_posY));
			if(entry.m_rotation != rotation)
			{
				entry.m_rotation = rotation;
				entry.m_dirty |= CHUD_RadarIcon::eDF_Rot;
			}

			//clamp position to the edge of the radar
			float direction = atan2(m_clientEntry->m_posX - entry.m_posX, m_clientEntry->m_posY - entry.m_posY) + gf_PI;

			float sin, cos;
			cry_sincosf(direction, &sin, &cos);

			float posX = m_clientEntry->m_posX + (sin * k_clampDistance);
			if(entry.m_posX != posX)
			{
				entry.m_posX = posX;
				entry.m_dirty |= CHUD_RadarIcon::eDF_PosX;
			}

			float posY = m_clientEntry->m_posY + (cos * k_clampDistance);
			if(entry.m_posY != posY)
			{
				entry.m_posY = posY;
				entry.m_dirty |= CHUD_RadarIcon::eDF_PosY;
			}
		}
		else if(entry.m_flags & CHUD_RadarIcon::eRF_Edge)
		{
			//change back to a circle icon
			entry.m_tempIcon = CHUD_RadarIcon::eIcon_None;
			entry.m_dirty |= CHUD_RadarIcon::eDF_Icon;
			entry.m_flags &= ~CHUD_RadarIcon::eRF_Edge;
		}
	}

}


void CHUD_Radar::RescanActors()
{
	IActor* pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (!pClientActor || !pClientActor->IsPlayer())
		return;

	CPlayer* pLocalPlayer = static_cast<CPlayer*>(pClientActor);
	const EntityId clientId = pClientActor->GetEntityId();

	if(gEnv->bMultiplayer)
	{

		IActorIteratorPtr pIter = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
		while (IActor *pActor = pIter->Next())
		{
			EntityId id = pActor->GetEntityId();

			if(!ShouldBeAddedOnRadar(id, true))
				continue;

			AddEntity(id, 0.0f, false);
		}
	}

	CHUDMissionObjectiveSystem* pMOSystem = g_pGame->GetMOSystem();
	const std::vector<CHUDMissionObjective> &objectives = pMOSystem->GetObjectives();
	const int size = objectives.size();
	for(int i = 0; i < size; ++i)
	{
		const CHUDMissionObjective &obj = objectives[i];

		if(obj.GetStatus() != CHUDMissionObjective::ACTIVATED)
			continue;

		AddEntity(obj.GetTrackedEntity(), 0.0f, false);
	}
}


#if ENABLE_HUD_EXTRA_DEBUG
void CHUD_Radar::debugRadar(int i, CHUD_RadarIcon& entry, IEntity* pEntity, IActor* pClientActor)
{
	if(g_pGame->GetHUD()->GetCVars()->hud_radarDebug > 0)
	{
		if(g_pGame->GetHUD()->GetCVars()->hud_radarDebug == 1)	//icons
		{
			CPlayer* pPlayer = static_cast<CPlayer*>(pClientActor);
			CryWatch("%s - Class %s, %s - Icon %d", pEntity->GetName(), pEntity->GetClass()->GetName(), pPlayer->IsFriendlyEntity(entry.m_entityId) ? "Friendly" : "Enemy", entry.m_icon);
		}
		else if(g_pGame->GetHUD()->GetCVars()->hud_radarDebug == 2)	//visible
		{
			SFlashVarValue currentlyVisible(false);
			m_radar->GetVariable(string().Format("Root_Radar.M%d._visible", i), currentlyVisible);
			CRY_ASSERT(currentlyVisible.GetBool() == m_visibility[i]);

			string alpha = "";
			if(entry.m_flags & CHUD_RadarIcon::eRF_HasAlpha)
			{
				alpha.Format("Alpha %.2f", entry.m_alpha);
			}
			CryWatch("%s - Time %.2f - %s %s", pEntity->GetName(), entry.m_timeVisible, currentlyVisible.GetBool() ? "Visible" : "Hidden", alpha.c_str());
		}
		else if(g_pGame->GetHUD()->GetCVars()->hud_radarDebug == 3)	//pos/rot
		{
			SFlashVarValue currentlyVisible(false);
			m_radar->GetVariable(string().Format("Root_Radar.M%d._visible", i), currentlyVisible);
			string noUpdatingPos = "";
			if(entry.m_flags & CHUD_RadarIcon::eRF_NoUpdatingPos)
			{
				noUpdatingPos.Format(" - no updating pos %.2f,  %.2f, %.2f", entry.m_noUpdatingPos.x, entry.m_noUpdatingPos.y, entry.m_noUpdatingPos.z);
			}
			CryWatch("%s - Pos %.2f.,%2.f Rot %f %s", pEntity->GetName(), entry.m_posX, entry.m_posY, entry.m_rotation, noUpdatingPos.c_str());
		}
		else if(g_pGame->GetHUD()->GetCVars()->hud_radarDebug == 4)	//player
		{
			if(pClientActor->GetEntityId() == entry.m_entityId)
			{
				SFlashVarValue currentlyVisible(false);
				m_radar->GetVariable(string().Format("Root_Radar.Player._visible", i), currentlyVisible);
				CryWatch("Local Player %s - %s%s", pEntity->GetName(), currentlyVisible.GetBool() ? "Visible" : "Hidden", ShouldBeAddedOnRadar(entry.m_entityId, false) ? " - RadarJammer" : "");
				m_radar->GetVariable(string().Format("Root_Radar.M%d._visible", i), currentlyVisible);
				CryWatch("Representation Player %s - %s", pEntity->GetName(), currentlyVisible.GetBool() ? "Visible" : "Hidden");
			}
		}
	}
}
#endif
