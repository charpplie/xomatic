/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010
-------------------------------------------------------------------------

Helper class to record telemetry for actors

-------------------------------------------------------------------------
History:
- 02-3-2010		Steve Humphreys

*************************************************************************/

#include "StdAfx.h"
#include "ActorTelemetry.h"

#include "Actor.h"
#include "Player.h"
#include "StatsRecordingMgr.h"
#include "StatHelpers.h"
#include "Weapon.h"

static const float POSITION_DUMP_INTERVAL = 1.0f;
static const float POSITION_THRESHOLD = 0.5f;
static const float HEALTH_DUMP_INTERVAL =  2.0f;
static const float LOOKDIR_DUMP_INTERVAL = 0.33f;
static const float LOOKDIR_THRESHOLD = 0.1f;
static const float SUITENERGY_DUMP_INTERVAL = 2.0f;
static const float SUITENERGY_THRESHOLD = 1.0f;

CActorTelemetry::CActorTelemetry()
: m_lastPosition(ZERO)
, m_lastHealth(-1)
, m_lastLookDir(ZERO)
, m_lastSuitEnergy(0.0f)
, m_currentWeaponId(0)
, m_isNanoSuitListener(false)
{
	IItemSystem		*is=g_pGame->GetIGameFramework()->GetIItemSystem();
	if(is && gEnv->bServer)
	{
		is->RegisterListener(this);
	}
}

CActorTelemetry::~CActorTelemetry()
{
	UnsubscribeFromWeapon();

	IItemSystem		*is=g_pGame->GetIGameFramework()->GetIItemSystem();
	if (is && gEnv->bServer)
	{
		is->UnregisterListener(this);
	}
}

void CActorTelemetry::SetOwner(const CActor* pActor)
{
	CRY_ASSERT(pActor);
	m_ownerRef = pActor->GetWeakRef();
}

IStatsTracker* CActorTelemetry::GetStatsTracker()
{
	CStatsRecordingMgr *sr=g_pGame->GetStatsRecorder();
	if (sr)
	{
		return sr->GetStatsTracker(m_ownerRef.Get());
	}

	return NULL;
}

void CActorTelemetry::Update()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	CActor* pOwner = m_ownerRef.Get();
	IStatsTracker *tracker = GetStatsTracker();
	CStatsRecordingMgr* pMgr = g_pGame->GetStatsRecorder();
	if(tracker && pOwner && pMgr)
	{
		IGameStatistics		*gs=g_pGame->GetIGameFramework()->GetIGameStatistics();
		CTimeValue time = gEnv->pTimer->GetFrameStartTime();
		assert(gs);

		if (TimePassedCheck(m_lastPositionTime, time, POSITION_DUMP_INTERVAL) && pMgr->ShouldRecordEvent(eSE_Position, pOwner))
		{
			Vec3 pos = pOwner->GetEntity()->GetWorldPos();
			if(!pos.IsEquivalent(m_lastPosition, POSITION_THRESHOLD))
			{
				m_lastPosition = pos;
				tracker->Event(eSE_Position, new CPositionStats(pos, gEnv->p3DEngine->GetTerrainElevation(pos.x, pos.y, true) ));
			}
		}

		if (TimePassedCheck(m_lastHealthTime, time, HEALTH_DUMP_INTERVAL) && pMgr->ShouldRecordEvent(eSE_Health, pOwner))
		{
			int32 health = pOwner->GetHealth();
			if(health != m_lastHealth)
			{
				m_lastHealth = health;
				tracker->Event(eSE_Health, health);
			}
		}

		if (pMgr->ShouldRecordEvent(eSE_LookDir, pOwner))
		{
			CPlayer* pPlayerOwner = static_cast<CPlayer*>(pOwner);
			IPlayerInput* pInput = pPlayerOwner->GetPlayerInput();
			if ( pInput && TimePassedCheck(m_lastLookDirTime, time, LOOKDIR_DUMP_INTERVAL))
			{
				SSerializedPlayerInput serInp;
				pInput->GetState(serInp);
				const Vec3& look = serInp.lookDirection;

				if(!look.IsEquivalent(m_lastLookDir, LOOKDIR_THRESHOLD))
				{
					m_lastLookDir = look;

					tracker->Event(eSE_LookDir, new CLookDirStats(look));
				}
			}
		}
	}

	if(pOwner->IsPlayer() && !m_isNanoSuitListener)
	{
		if(CNanoSuit* pSuit = pOwner->GetNanoSuit())
		{
			pSuit->AddListener(this);
			m_isNanoSuitListener = true;

			// record initial mode
			ModeChanged(pSuit->GetGameParams().GetMode());
		}
	}
}

void CActorTelemetry::SubscribeToWeapon(EntityId weaponId)
{
	IWeapon				*pWeapon=NULL;
	if (weaponId)
	{
		IItem* pItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(weaponId);
		if(pItem)
		{
			pWeapon=pItem->GetIWeapon();

			pWeapon->AddEventListener(this, "CActorTelemetry");
			m_currentWeaponId = weaponId;
		}
	}

	CStatsRecordingMgr* pMgr = g_pGame->GetStatsRecorder();
	IStatsTracker* pTracker = GetStatsTracker();
	CActor* pOwner = m_ownerRef.Get();
	if(pTracker && pMgr)
	{
		if (pWeapon)
		{
			if(pMgr->ShouldRecordEvent(eSE_Weapon, pOwner))
				pTracker->Event(eSE_Weapon,static_cast<CWeapon*>(pWeapon)->GetName());

			if(pMgr->ShouldRecordEvent(eGSE_Firemode, pOwner))
			{
				IFireMode		*fireMode=pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
				if (fireMode)
				{
					pTracker->Event(eGSE_Firemode,fireMode->GetName() );
				}
				else
				{
					pTracker->Event(eGSE_Firemode,"unknown firemode");
				}
			}
		}
		else if(pMgr->ShouldRecordEvent(eSE_Weapon), pOwner)
		{
			pTracker->Event(eSE_Weapon,"unarmed");
			// until we know how we want the stats to be handled, leave the firemode timeline as it is when switching to no weapon
		}
	}
}

void CActorTelemetry::UnsubscribeFromWeapon()
{
	IItem* pItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(m_currentWeaponId);
	if(pItem)
	{
		IWeapon *pWeapon = pItem->GetIWeapon();
		if(pWeapon)
		{
			pWeapon->RemoveEventListener(this);
		}
	}
	m_currentWeaponId = 0;
}

//////////////////////////////////////////////////////////////////////////
// IItemSystemListener functions
//////////////////////////////////////////////////////////////////////////

void CActorTelemetry::OnSetActorItem(IActor *pActor, IItem *pItem )
{
	if(pActor == m_ownerRef.Get())
	{
		// stop listening to old weapon
		UnsubscribeFromWeapon();
		
		// listen to new weapon
		if(pItem)
		{
			SubscribeToWeapon(pItem->GetEntityId());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// IWeaponEventListener functions
//////////////////////////////////////////////////////////////////////////

void CActorTelemetry::OnEndReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType)
{
	CRY_ASSERT(shooterId == m_ownerRef->GetEntityId());

	CStatsRecordingMgr* pMgr = g_pGame->GetStatsRecorder();
	if(!pMgr || !pMgr->ShouldRecordEvent(eSE_Reload, m_ownerRef.Get()))
		return;

	if ( IStatsTracker *tracker = GetStatsTracker() )
	{
		tracker->Event(eSE_Reload); // TODO kiev differentiated between auto reloads and manual reloads
	}
}

void CActorTelemetry::OnMelee(IWeapon* pWeapon, EntityId shooterId)
{
	CRY_ASSERT(shooterId == m_ownerRef->GetEntityId());

	CStatsRecordingMgr* pMgr = g_pGame->GetStatsRecorder();
	if(!pMgr || !pMgr->ShouldRecordEvent(eGSE_Melee, m_ownerRef.Get()))
		return;

	if ( IStatsTracker *tracker = GetStatsTracker() )
	{
		tracker->Event(eGSE_Melee);	// TODO kiev had a melee id here, which was a rolling id stored in CPlayer. was used to link melee events to melee damage
	}
}

void CActorTelemetry::OnShoot(IWeapon *pWeapon, EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType,
																 const Vec3 &pos, const Vec3 &dir, const Vec3 &vel)
{
	CRY_ASSERT(shooterId == m_ownerRef->GetEntityId());

	CStatsRecordingMgr* pMgr = g_pGame->GetStatsRecorder();
	if(!pMgr || !pMgr->ShouldRecordEvent(eSE_Shot, m_ownerRef.Get()))
		return;

	if ( IStatsTracker *tracker = GetStatsTracker() )
	{
		int				fmidx=pWeapon->GetCurrentFireMode();
		IFireMode		*fm=pWeapon->GetFireMode(fmidx);
		IEntityClass *ammoType=fm ? fm->GetAmmoType() : NULL;
		// TODO add entity id that is being aimed at so we can calculate the % of shots that hit the intended target
		//	(SNH: should be possible to record hits, and match the projectile ids in StatsTool)
		tracker->Event( eSE_Shot, new CShotFiredStats(fm ? fm->GetProjectileId() : 0, pWeapon->GetAmmoCount(ammoType), ammoType ? ammoType->GetName() : "unknown ammo"));
	}
}

void CActorTelemetry::OnFireModeChanged(IWeapon *pWeapon, int currentFireMode)
{
	CRY_ASSERT(static_cast<CWeapon*>(pWeapon)->GetOwnerId() == m_ownerRef->GetEntityId());		// is this cast always safe?

	CStatsRecordingMgr* pMgr = g_pGame->GetStatsRecorder();
	if(!pMgr || !pMgr->ShouldRecordEvent(eGSE_Firemode, m_ownerRef.Get()))
		return;

	if ( IStatsTracker *tracker = GetStatsTracker() )
	{
		tracker->Event( eGSE_Firemode, pWeapon->GetFireMode(currentFireMode)->GetName() );
	}
}

void CActorTelemetry::OnDropped(IWeapon *pWeapon, EntityId actorId)
{
	CRY_ASSERT(actorId == m_ownerRef->GetEntityId());
	if(static_cast<CWeapon*>(pWeapon)->GetEntityId() == m_currentWeaponId)
	{
		UnsubscribeFromWeapon();
	}
}

//////////////////////////////////////////////////////////////////////////
// INanoSuitListener functions
//////////////////////////////////////////////////////////////////////////

void CActorTelemetry::ModeChanged(ENanoSuitMode mode)
{
	CStatsRecordingMgr* pMgr = g_pGame->GetStatsRecorder();
	if(!pMgr || !pMgr->ShouldRecordEvent(eGSE_NanoSuitMode, m_ownerRef.Get()))
		return;

	if (IStatsTracker* pTracker = GetStatsTracker() )
	{
		pTracker->Event(eGSE_NanoSuitMode, CNanoSuit::GetNanoSuitModeName(mode));
	}
}

void CActorTelemetry::OnSuitPowerActivated(const bool activated)
{
	CStatsRecordingMgr* pMgr = g_pGame->GetStatsRecorder();
	if(!pMgr || !pMgr->ShouldRecordEvent(eGSE_NanoSuitActivate, m_ownerRef.Get()))
		return;

	if (IStatsTracker* pTracker = GetStatsTracker() )
	{
		pTracker->Event(eGSE_NanoSuitActivate, activated);
	}
}

void CActorTelemetry::EnergyChanged(float energy)
{
	CStatsRecordingMgr* pMgr = g_pGame->GetStatsRecorder();
	if(!pMgr || !pMgr->ShouldRecordEvent(eGSE_NanoSuitEnergy, m_ownerRef.Get()))
		return;

	if (IStatsTracker* pTracker = GetStatsTracker() )
	{
		if(abs(energy - m_lastSuitEnergy) > SUITENERGY_THRESHOLD)
		{
			m_lastSuitEnergy = energy;
			pTracker->Event(eGSE_NanoSuitEnergy, (int)(energy+0.5f));	// energy is 0-100; int is easier for stat processing
		}
	}
}

void CActorTelemetry::Serialize( TSerialize ser )
{
	EntityId ownerId = m_ownerRef.Get()?m_ownerRef->GetEntityId():0;
	ser.Value("OwnerId", ownerId);
	ser.Value("WeaponId", m_currentWeaponId);

	if(ser.IsReading())
	{
		CActor *pOwnerActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(ownerId));
		CRY_ASSERT(pOwnerActor);
		m_ownerRef = pOwnerActor->GetWeakRef();
	}
}

void CActorTelemetry::PostSerialize()
{
	IItem* pItem = gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(m_currentWeaponId);
	if(pItem && pItem->GetOwnerId() != m_ownerRef->GetEntityId())
		UnsubscribeFromWeapon();

	//re-init nanosuit listener if necessary
	m_isNanoSuitListener = false;
}