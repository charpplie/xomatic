/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 06:11:2009   Created for power struggle lite

*************************************************************************/

#include "StdAfx.h"
#include "JavelinMissile.h"
#include "Actor.h"
#include "Game.h"
#include "GameCVars.h"
#include "WeaponSystem.h"
#include "Single.h"
#include "NetInputChainDebug.h"
#include "Chaff.h"


//------------------------------------------------------------------------
CJavelinMissile::CJavelinMissile()
{

}

//------------------------------------------------------------------------
CJavelinMissile::~CJavelinMissile()
{

}

//------------------------------------------------------------------------
bool CJavelinMissile::Init(IGameObject *pGameObject)
{
	if(CHomingMissile::Init(pGameObject))
	{
		const char* launchEffectName = NULL;
		launchEffectName = GetParam("launch_effect", launchEffectName);

		if(launchEffectName && launchEffectName[0])
		{
			m_launchEffect = gEnv->pParticleManager->FindEffect(launchEffectName);
		}
		return true;
	}

	return false;
}

//------------------------------------------------------------------------
void CJavelinMissile::Launch(const Vec3 &pos, const Vec3 &dir, const Vec3 &velocity, float speedScale)
{
	//addition effect when launching
	if(m_launchEffect)
	{
		m_launchEffect->Spawn(true, IParticleEffect::ParticleLoc(pos, -dir));
	}

	//kill players near where the missile is launched from
	CRY_TODO(9, 11, 2009, "Kill players near missile launch position");

	//cruise altitude is local to fire position (so it'll work at any height)
	m_cruiseAltitude += pos.z;

	CHomingMissile::Launch(pos, dir, velocity, speedScale);
}

//------------------------------------------------------------------------
void CJavelinMissile::Update(SEntityUpdateContext &ctx, int updateSlot)
{
	float frameTime = ctx.fFrameTime;

	pe_status_dynamics status;
	if (!GetEntity()->GetPhysics()->GetStatus(&status))
		return;

	float currentSpeed = status.v.len();
	Vec3 currentPos = GetEntity()->GetWorldPos();
	

	if (!m_destination.IsZero())
	{
		float heightDiff = m_cruiseAltitude - currentPos.z;

		if (!m_isCruising && heightDiff > 0.f)
		{
			Vec3 currentDir = status.v.GetNormalizedSafe(FORWARD_DIRECTION);
			Vec3 targetDir = currentDir;

			//wait for the initial delay and then climb to target altitude
			if (m_lockedTimer > 0.0f)
			{
				m_lockedTimer = m_lockedTimer - frameTime;
			}
			else
			{
				Vec3 goalDir(0.0f, 0.0f, 1.0f);

				float cosine = max(min(currentDir.Dot(goalDir), 0.999f), -0.999f);
				float goalAngle = RAD2DEG(acos_tpl(cosine));
				float maxAngle = m_turnSpeed * frameTime;

				if (goalAngle > maxAngle+0.05f)
				{
					targetDir = (Vec3::CreateSlerp(currentDir, goalDir, maxAngle/goalAngle)).normalize();
				}
				else
				{
					targetDir = goalDir;
				}
			}

			float desiredSpeed = currentSpeed;
			if (currentSpeed < m_maxSpeed-0.1f)
			{
				desiredSpeed = min(m_maxSpeed, desiredSpeed + m_accel*frameTime);
			}

			pe_action_set_velocity action;
			action.v = targetDir * desiredSpeed;
			GetEntity()->GetPhysics()->Action(&action);
		}
		//Once at target altitude then let normal Cruise take over
		else
		{
			m_isCruising = true;
			UpdateCruiseMissile(frameTime);
			return;
		}
	}
}


void CJavelinMissile::SetDestination(EntityId targetId)
{
	IEntity* pEntity = gEnv->pEntitySystem->GetEntity(targetId);
	assert(pEntity);
	if(pEntity)
	{
		AABB box;
		pEntity->GetWorldBounds(box);
		CHomingMissile::SetDestination( box.GetCenter() );
	}
	m_targetId = targetId;
}

void CJavelinMissile::SetDestination(const Vec3& pos)
{
	
}
