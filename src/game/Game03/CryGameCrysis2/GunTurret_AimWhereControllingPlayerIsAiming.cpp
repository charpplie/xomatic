/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------

GunTurret which can be activated by a player and then aims where that
player is aiming (until the player gets too far from the turret, a timer
elapses or the player dies)

-------------------------------------------------------------------------
History:
- 9:11:2009   Created by Tim Furnish

*************************************************************************/

#include "StdAfx.h"
#include "GunTurret_AimWhereControllingPlayerIsAiming.h"
#include "PerkDbgDisplay.h"
#include "GameRules.h"
#include "Player.h"

#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#define SGT_DbgDisplayForEntity(id,...)				XDbgDisplayForEntity	(g_displayDbgText_psGuns,	id,	string().Format("<GunTurret_Aim> " __VA_ARGS__))

static const float kNoAimAtPointsWithinDistanceSq = (1.5f * 1.5f);
static const float kPlayerKeepsControlWhileWithinDistanceSq = (10.f * 10.f);

CGunTurret_AimWhereControllingPlayerIsAiming::CGunTurret_AimWhereControllingPlayerIsAiming()
{
	m_beingUsedByEntity = 0;
	m_aimedOk = false;
}

CGunTurret_AimWhereControllingPlayerIsAiming::~CGunTurret_AimWhereControllingPlayerIsAiming()
{
	ActivateFeedback(m_beingUsedByEntity, false);
}

void CGunTurret_AimWhereControllingPlayerIsAiming::Update(SEntityUpdateContext& ctx, int update)
{
	switch (update)
	{
		case eIUS_General:
		m_updateTargetTimer = m_beingUsedByEntity ? GetTurretParams().update_target_time : 0.f;
		m_pauseTimer = 0.f;
		CheckControllerCanStillUseTurret();
		break;

#if 0
		case eIUS_Zooming:
		break;

		case eIUS_FireMode:
		break;

		case eIUS_Scheduler:
		break;
#endif
	}

	CGunTurret::Update(ctx, update);
}

void CGunTurret_AimWhereControllingPlayerIsAiming::CheckControllerCanStillUseTurret()
{
	IActor * validController = NULL;
	float distFromUserToGunOriginSq = 0.f;

	if (m_beingUsedByEntity)
	{
		IActor * wouldBeController = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_beingUsedByEntity);

		if (wouldBeController && wouldBeController->GetHealth() > 0)
		{
			int controllerTeam = g_pGame->GetGameRules()->GetTeam(m_beingUsedByEntity);
			if (controllerTeam && controllerTeam == GetTurretParams().team)
			{
				distFromUserToGunOriginSq = (wouldBeController->GetEntity()->GetWorldPos() - GetEntity()->GetWorldPos()).len2();
				if (distFromUserToGunOriginSq < kPlayerKeepsControlWhileWithinDistanceSq)
				{
					validController = wouldBeController;
				}
				else
				{
					CCCPOINT(SubservientGunTurret_StopControllingBecauseGotTooFarAway);
				}
			}
		}
		else
		{
			CCCPOINT(SubservientGunTurret_StopControllingBecauseDiedOrLeftGame);
		}

		if (! validController)
		{
			CryLog ("SubservientGunTurret: %s can no longer be used by %s!", GetEntity()->GetName(), wouldBeController ? wouldBeController->GetEntity()->GetName() : "non-existent actor");
			ActivateFeedback(m_beingUsedByEntity, false);
			m_beingUsedByEntity = 0;
#if GUNTURRET_NETWORK_CHANGES
			SetOwnerId(m_beingUsedByEntity);
#endif
			CHANGED_NETWORK_STATE(this, ASPECT_CONTROLLINGACTOR);
		}
	}

	SGT_DbgDisplayForEntity(m_beingUsedByEntity, "Using subservient gun turret '%s' (distance=%.2fm, team=%d counter=%d isFiring=%d)%s", GetEntity()->GetName(), sqrtf(distFromUserToGunOriginSq), GetTurretParams().team, m_shootCounter, m_isFiring, IsFiring(false) ? " $8FIRING!" : "");
}

IEntity *CGunTurret_AimWhereControllingPlayerIsAiming::GetClosestTarget() const
{
	IEntity * targetMe = NULL;

	if (m_beingUsedByEntity)
	{
		CActor * controller = (CActor *)g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_beingUsedByEntity);

		if (controller)
		{
			targetMe = gEnv->pEntitySystem->GetEntity(controller->GetCurrentTargetEntityId());
		}

		if (targetMe)
		{
			SGT_DbgDisplayForEntity (GetEntityId(), "GetClosestTarget returning %s", targetMe->GetName());
		}
	}

	return targetMe;
}

void CGunTurret_AimWhereControllingPlayerIsAiming::Use(EntityId userId)
{
	IEntity * entity = gEnv->pEntitySystem->GetEntity(userId);
	assert (entity);

	CryLog("CGunTurret_AimWhereControllingPlayerIsAiming: '%s' using '%s' - hooray!", entity->GetName(), GetEntity()->GetName());

	assert (m_beingUsedByEntity == 0);
	m_beingUsedByEntity = userId;
#if GUNTURRET_NETWORK_CHANGES
	SetOwnerId(m_beingUsedByEntity);
#endif
	ActivateFeedback(m_beingUsedByEntity, true);
	CHANGED_NETWORK_STATE(this, ASPECT_CONTROLLINGACTOR);
	CCCPOINT(SubservientGunTurret_Use);
}

bool CGunTurret_AimWhereControllingPlayerIsAiming::CanUse(EntityId userId) const
{
	if (m_beingUsedByEntity == 0)
	{
		int userTeam = g_pGame->GetGameRules()->GetTeam(userId);
		return userTeam == GetTurretParams().team;
	}

	return false;
}

bool CGunTurret_AimWhereControllingPlayerIsAiming::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	// call base class
	bool ok = CGunTurret::NetSerialize(ser, aspect, profile, flags);

	if (ok)
	{
		if(aspect == ASPECT_CONTROLLINGACTOR)
		{
			CCCPOINT(SubservientGunTurret_SerializeCurrentController);
			EntityId previousUsedByEntity = m_beingUsedByEntity;
			ser.Value("beingUsedBy", m_beingUsedByEntity, 'eid');
			if(previousUsedByEntity != m_beingUsedByEntity)
			{
				ActivateFeedback(previousUsedByEntity, false);
				ActivateFeedback(m_beingUsedByEntity, true);
#if GUNTURRET_NETWORK_CHANGES
				SetOwnerId(m_beingUsedByEntity);
#endif
			}
		}
	}

	return ok;
}

void CGunTurret_AimWhereControllingPlayerIsAiming::UpdateAiming(IEntity * pCurrentTarget, SEntityUpdateContext& ctx)
{
	const Vec3 * aimAtPos = NULL;
	
	m_aimedOk = false;

	if (m_beingUsedByEntity)
	{
		CActor * controller = (CActor *)g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_beingUsedByEntity);

		if (controller)
		{
			const Vec3 * aimAtPos = controller->GetCurrentTargetPos();

			if (aimAtPos)
			{
				float distFromGunOriginSq = (*aimAtPos - GetEntity()->GetWorldPos()).len2(), goalYaw = 0.f, goalPitch = 0.f;
				bool doIt = distFromGunOriginSq > kNoAimAtPointsWithinDistanceSq && GetTargetAngles(*aimAtPos, goalYaw, goalPitch) && IsTargetAimable(goalYaw,goalPitch);

				SGT_DbgDisplayForEntity(m_beingUsedByEntity, "Gun turret '%s' aiming %.2fm away (%s)", GetEntity()->GetName(), sqrtf(distFromGunOriginSq), doIt ? "$3OK$o" : "$4FAIL$o");

				if (doIt)
				{
					SetGoalPitchAndYaw(goalPitch, goalYaw);
					m_aimedOk = true;
				}
			}
		}
	}
}

void CGunTurret_AimWhereControllingPlayerIsAiming::ActivateFeedback(EntityId who, bool activate)
{
	m_aimedOk = false;

	if (who)
	{
		CPlayer * player = (CPlayer *)g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(who);

		if (player)
		{
			if (activate)
			{
				CAudioSignalPlayer::JustPlay("PSLTurretActivate", GetEntity()->GetPos());
			}
			else
			{
				CAudioSignalPlayer::JustPlay("PSLTurretDeactivate", GetEntity()->GetPos());
			}
		}
	}
}

//------------------------------------------------------------------------
void CGunTurret_AimWhereControllingPlayerIsAiming::OverrideShouldShoot(IEntity* pTarget, bool & shouldShoot) const
{
	if (m_beingUsedByEntity && m_aimedOk)
	{
		shouldShoot = true;
	}
}

