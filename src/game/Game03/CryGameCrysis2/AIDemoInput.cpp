#include "StdAfx.h"
#include "AIDemoInput.h"
#include "GameRulesModules/IGameRulesSpawningModule.h"
#include "GameCVars.h"
#include "Player.h"
#include "GameRules.h"
#include "Weapon.h"

static const float THETA = 3.14159265358979323f / 180.0f * 3;

CDedicatedInput::CDedicatedInput(CPlayer* pPlayer) :
		m_pPlayer(pPlayer),
		m_deltaMovement(0.0f,1.0f,0.0f),
		m_deltaRotation(0.0f,0.0f,THETA),
		m_rand( (uint32)gEnv->pTimer->GetAsyncCurTime() ),
		m_movingTime(0.0f),
		m_grenadeStateTime(0.0f),
		m_stance(STANCE_STAND),
		m_jump(0),
		m_grenadeState(GRENADE_PREPARE),
		m_timeToReachTarget(0),
		m_fire(eDB_Default),
		m_move(eDB_Default),
		m_dummyPlayerBehaviour(false)
{
	IEntity* pEntity = pPlayer->GetEntity();
	Vec3 pos = pEntity->GetPos();

	m_spawnPosition = pos;
	m_targetPosition = pos;
}

void CDedicatedInput::PreUpdate()	
{
#if !defined(_RELEASE)
	if (m_dummyPlayerBehaviour)
	{

	}
	else if (g_pGameCVars->g_playerEnableDedicatedInput)
	{
		if (m_movingTime <= 0.f)
		{
			m_deltaMovement.Set(0.f, (float) ((rand() % 10) - 2), 0.f);
			m_deltaRotation.Set(0.f, 0.f, THETA * ((rand() % 5) - 2.5f));

			m_movingTime = (float) ((rand() % 10) + 5);

			m_mode = rand() % NUM_MODES; 
			IItem* pItem = m_pPlayer->GetCurrentItem();
			if (pItem)
			{
				IWeapon* pWeapon = pItem->GetIWeapon();
				if (pWeapon)
				{
					pWeapon->StopFire();
				}
			}
			switch (m_mode)
			{
				case E_EXPLOSIVE:
					m_grenadeState = GRENADE_PREPARE;
					m_grenadeStateTime = 0.f;
					m_pPlayer->SelectNextItem(1, true, "explosive");
					break;
				case E_SMALLMEDIUM:
				{
					int looptimes = (rand() % 2) + 1;
					while(looptimes--)
						m_pPlayer->SelectNextItem(1, true, "primary|secondary");
					break;
				}
				case E_MELEE:
					m_pPlayer->SelectNextItem(1, true, "primary|secondary");
					break;
			}
		}

		if (fmod(m_movingTime, 2.0f) > 1.0f)
		{
			if (m_stance == STANCE_STAND)
			{
				m_stance = STANCE_CROUCH;
			}
			else
			{
				m_jump = 1;
				m_stance = STANCE_STAND;
			}
		}

		float frametime = gEnv->pTimer->GetFrameTime();
		m_movingTime -= frametime;
		m_grenadeStateTime += frametime;
	}
	else
	{
		m_deltaMovement.Set(0.0f, 0.0f, 0.0f);
	}
#endif

	CHANGED_NETWORK_STATE(m_pPlayer,  CPlayer::ASPECT_INPUT_CLIENT );
}

void CDedicatedInput::Update()
{
#if !defined(_RELEASE)
	if (m_dummyPlayerBehaviour)
	{
		float frametime = gEnv->pTimer->GetFrameTime();

		bool shouldFire = (g_pGameCVars->g_dummyPlayersFire != 0);
		bool shouldMove = (g_pGameCVars->g_dummyPlayersMove != 0);

		if (m_fire != eDB_Default)
			shouldFire = (m_fire == eDB_True);

		if (m_move != eDB_Default)
			shouldMove = (m_move == eDB_True);

		if(!gEnv->bServer)
			return;

		IEntity* pEntity = m_pPlayer->GetEntity();

		Vec3 pos = pEntity->GetPos();

		m_timeToReachTarget += frametime;

		Vec3 delta = m_targetPosition - pos;
		float distSquared = delta.GetLengthSquared2D();
		if (distSquared < 1 || m_timeToReachTarget > 3)
		{
			// Choose a new target position
			Vec3 randomVector = Vec3(Random(-5, 5), Random(-5, 5), 0);
			m_targetPosition = m_spawnPosition + randomVector;
			m_timeToReachTarget = 0;
		}

		Vec3 dir = delta;
		dir.z = 0;
		dir.Normalize();

		CMovementRequest request;

		request.SetStance(STANCE_STAND);
		if (shouldMove)
		{
			Quat worldRot = m_pPlayer->GetBaseQuat();
			Vec3 deltaMovement = worldRot.GetInverted().GetNormalized() * dir;

			request.AddDeltaMovement(deltaMovement);
			request.SetLookTarget( pos + 10.0f * dir );
			request.SetPseudoSpeed(1);
		}
		else
		{
			request.SetPseudoSpeed(0);
		}
		m_pPlayer->GetMovementController()->RequestMovement(request);

		if (IItem * pItem = m_pPlayer->GetCurrentItem())
		{
			CWeapon *cw = static_cast<CWeapon *>(pItem->GetIWeapon());

			if (cw)
			{
				IFireMode *ifm = cw->GetFireMode(cw->GetCurrentFireMode());
				if (ifm)
				{
					if (shouldFire && !ifm->IsFiring())
					{
						ifm->StartFire();
					}
					else if (!shouldFire && ifm->IsFiring())
					{
						ifm->StopFire();
					}
				}
			}
		}
	}
	else if (g_pGameCVars->g_playerEnableDedicatedInput)
	{
		CMovementRequest request;
		request.SetStance((EStance) m_stance);
		if (m_jump)
		{
			request.SetJump();
			m_jump = 0;
		}
		request.AddDeltaMovement(m_deltaMovement);
		request.AddDeltaRotation(m_deltaRotation);
		float pseudoSpeed = m_pPlayer->CalculatePseudoSpeed(false);
		request.SetPseudoSpeed(pseudoSpeed);
		m_pPlayer->GetMovementController()->RequestMovement(request);

		IItem* pItem = m_pPlayer->GetCurrentItem();
		if (pItem)
		{
			IWeapon* pWeapon = pItem->GetIWeapon();
			if (pWeapon)
			{
				IFireMode *cfm = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
				if ( cfm )
				{
					pWeapon->SetAmmoCount( cfm->GetAmmoType(), 100 );
				}

				CWeapon *pcweap = (CWeapon*) pWeapon;

				switch (m_mode)
				{
					case E_EXPLOSIVE:
					{
						if (cfm)
						{
							switch(m_grenadeState)
							{
								case GRENADE_PREPARE:
									if (m_grenadeStateTime > 0.5f)
									{
										m_grenadeStateTime = 0.f;
										pWeapon->StartFire();
										m_grenadeState = GRENADE_PREPARE_WAIT;
									}
									break;
								case GRENADE_PREPARE_WAIT:
									if (m_grenadeStateTime > 0.5f)
									{
										m_grenadeStateTime = 0.f;
										pWeapon->StopFire();
										m_grenadeState = GRENADE_THROW_WAIT;
									}
									break;
								case GRENADE_THROW_WAIT:
									if (m_grenadeStateTime > 0.5f)
									{
										m_grenadeStateTime = 0.f;
										m_pPlayer->SelectNextItem(1, true, "explosive");
										m_grenadeState = GRENADE_PREPARE;
									}
									break;
							}
						}

						break;
					}
					case E_SMALLMEDIUM:
					{
						pWeapon->StartFire();
						break;
					}
					case E_MELEE:
						pWeapon->MeleeAttack();
						break;
				}
			}
		}
	}
#endif

	if (m_pPlayer->IsDead())
	{
		CryLogAlways("CDedicatedInput::Update() detected that our player is dead - requesting revive");

		IGameRulesSpawningModule *pSpawningModule = g_pGame->GetGameRules()->GetSpawningModule();
		if (pSpawningModule)
		{
			CryLog("CDedicatedInput::Update() Requesting revive");
			pSpawningModule->ClRequestRevive(m_pPlayer->GetEntityId());
		}
	}

}

void CDedicatedInput::PostUpdate()
{
	if (m_dummyPlayerBehaviour)
	{
	}
	else
	{
		SMovementState movementState;
		m_pPlayer->GetMovementController()->GetMovementState(movementState);
		m_serializedInput.deltaMovement = m_deltaMovement;
		m_serializedInput.lookDirection = movementState.eyeDirection;
		m_serializedInput.bodyDirection = movementState.bodyDirection;
		m_serializedInput.stance = m_stance;
	}
}

void CDedicatedInput::OnAction( const ActionId& action, int activationMode, float value )
{
}

void CDedicatedInput::SetState( const SSerializedPlayerInput& input )
{
	GameWarning("CDedicatedInput::SetState called - should not happen");
}

void CDedicatedInput::GetState( SSerializedPlayerInput& input )
{
	input = m_serializedInput;
}

void CDedicatedInput::Reset()
{
	m_deltaMovement.Set(0.0f, 1.0f, 0.0f);
	m_deltaRotation.Set(0.0f, 0.0f, THETA);
}

void CDedicatedInput::DisableXI(bool disabled)
{
}

IPlayerInput::EInputType CDedicatedInput::GetType() const
{
	return DEDICATED_INPUT;
}

void CDedicatedInput::GetMemoryUsage(ICrySizer * pSizer) const
{
	pSizer->Add(*this);
}