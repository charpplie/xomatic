#include "StdAfx.h"
#include "PlayerPlugin_Perk_AutoTarget.h"
#include "Player.h"
#include "PerkIconData.h"
#include "PerkDbgDisplay.h"
#include "IMovementController.h"

#include "GameCodeCoverage/GameCodeCoverageTracker.h"

struct SAutoTargetBestVictim
{
	Vec3 m_normalizedDir;
	float m_score, m_amountToUse;
};

CPlayerPlugin_Perk_AutoTarget::CPlayerPlugin_Perk_AutoTarget()
{
}

void CPlayerPlugin_Perk_AutoTarget::Enter()
{
	IItem * item = m_ownerPlayer->GetCurrentItem();
	IWeapon * weapon = item ? item->GetIWeapon() : NULL;

	m_ironSightOn = weapon && weapon->IsZoomed();
	m_disableIcon = true;
	m_iconIsLit = true;

	IPerk::Enter();
}

void CPlayerPlugin_Perk_AutoTarget::Leave()
{
	IPerk::Leave();
}

void CPlayerPlugin_Perk_AutoTarget::HandleEvent(EPlayerPlugInEvent perkEvent, void* data)
{
	switch(perkEvent)
	{
		case EPE_IronSightActive:
		m_ironSightOn = *(bool *)data;
		break;

		case EPE_OverrideMovementStateData:
		if (m_ownerPlayer->IsClient() && !m_ironSightOn)
		{
			CPerkIconData * iconData = CPerkIconData::GetForEntity(m_ownerPlayer->GetEntityId());
			assert (iconData);

			SMovementState * state = (SMovementState*)data;
			assert (state->aimDirection.IsUnit());
	
			SAutoTargetBestVictim best;

			if (GetBestAutoDirection(state, best))
			{
				PlayerPluginWatch("Aim %.3f %.3f %.3f, use %.3f, score %.7f", state->aimDirection.x, state->aimDirection.y, state->aimDirection.z, best.m_amountToUse, best.m_score);
				state->fireDirection = (state->fireDirection * (1.f - best.m_amountToUse) + best.m_normalizedDir * (best.m_amountToUse)).GetNormalized();
				state->aimDirection = state->fireDirection;

				CCCPOINT(Perk_AutoTarget_AdjustAimDirection);

				iconData->SetIconDrainAmount(m_perkId, 1.f - best.m_amountToUse, true);
				m_disableIcon = false;
				m_iconIsLit = true;
			}
		}
		break;

		default:
		IPerk::HandleEvent (perkEvent, data);
		break;
	}
}

EntityId CPlayerPlugin_Perk_AutoTarget::GetBestAutoDirection(const SMovementState * state, SAutoTargetBestVictim & bestOut)
{
	EntityId victimEntityId = 0;
	IActorIteratorPtr pIter = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->CreateActorIterator();
	const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
	float shrinkDistance = perkVars->perk_autoTarget_coneShrinkDistance;

	while (IActor *pActor = pIter->Next())
	{
		IEntity* pEntity = pActor->GetEntity();
		assert (pEntity);

		if (pActor->GetHealth() > 0 && pEntity->IsActive() && ! m_ownerPlayer->IsFriendlyEntity(pActor->GetEntityId()) && g_pGame->GetPlayerVisTable()->CanLocalPlayerSee(pEntity->GetId(), 5))
		{
			ISkeletonPose *pPose = pEntity->GetCharacter(0)->GetISkeletonPose();
			int16 groinID = pPose->GetJointIDByName("Bip01 Pelvis");
			int16 neckID = pPose->GetJointIDByName("Bip01 Neck");
			assert (groinID >= 0);
			assert (neckID >= 0);

			QuatT neckQuat = pPose->GetAbsJointByID(neckID);
			QuatT groinQuat = pPose->GetAbsJointByID(groinID);

			Vec3 neckPos = pEntity->GetWorldTM().TransformPoint(neckQuat.t);
			Vec3 groinPos = pEntity->GetWorldTM().TransformPoint(groinQuat.t);

			Vec3 aimAtPointOnPlayer = (neckPos * 0.5f) + (groinPos * 0.5f);
			Vec3 dirToOtherActor = aimAtPointOnPlayer - state->eyePosition;
			
			SAutoTargetBestVictim thisVictim;
			thisVictim.m_normalizedDir = dirToOtherActor.GetNormalized();
			float dot = thisVictim.m_normalizedDir.dot(state->aimDirection);

			float lenSquared = dirToOtherActor.len2();
			float scaleConeSize = shrinkDistance / (shrinkDistance + lenSquared);
			float assistanceConeSize = perkVars->perk_autoTarget_assistanceConeSize * scaleConeSize;

			PlayerPluginWatch ("%s: distanceSquared = %f scaleConeSize = %f", pEntity->GetName(), lenSquared, scaleConeSize);

			if (dot > (1.f - assistanceConeSize))
			{
				float amountOnBeforeFlatten = (1.f - (1.f - dot) / assistanceConeSize);
				thisVictim.m_score = dirToOtherActor.len2() * (1.f - amountOnBeforeFlatten);

#if defined(USER_timf)
				int greenColour = (int)(255 * amountOnBeforeFlatten);
				int blueColour = 255 - greenColour;

				IRenderAuxGeom* pRenderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
				SAuxGeomRenderFlags oldRenderFlags = pRenderAuxGeom->GetRenderFlags();
				SAuxGeomRenderFlags newRenderFlags = oldRenderFlags;
				newRenderFlags.SetDepthTestFlag(e_DepthTestOff);
				pRenderAuxGeom->SetRenderFlags(newRenderFlags);
				pRenderAuxGeom->DrawLine(groinPos, ColorB(0, greenColour / 8, blueColour / 8, 0), aimAtPointOnPlayer, ColorB(greenColour, greenColour / 2, blueColour, greenColour), 5.f);
				pRenderAuxGeom->DrawLine(neckPos, ColorB(0, greenColour / 8, blueColour / 8, 0), aimAtPointOnPlayer, ColorB(greenColour, greenColour / 2, blueColour, greenColour), 5.f);
				pRenderAuxGeom->SetRenderFlags(oldRenderFlags);
#endif

				if (victimEntityId == 0 || thisVictim.m_score < bestOut.m_score)
				{
					if (perkVars->perk_autoTarget_amountOfConeWhichGivesPerfectAim >= 1.f)
					{
						thisVictim.m_amountToUse = 1.f;
					}
					else
					{
						thisVictim.m_amountToUse = HUDCurveFraction(min (1.f, amountOnBeforeFlatten / (1.f - perkVars->perk_autoTarget_amountOfConeWhichGivesPerfectAim)));
					}
					memcpy (& bestOut, & thisVictim, sizeof(bestOut));
					victimEntityId = pEntity->GetId();
				}
			}
		}
	}

	return victimEntityId;
}

void CPlayerPlugin_Perk_AutoTarget::Update(const float dt)
{
	CPerkIconData * iconData = CPerkIconData::GetForEntity(m_ownerPlayer->GetEntityId());

	if (iconData)
	{
		PlayerPluginWatch("lit=%d disable=%d ironSight=%d", m_iconIsLit, m_disableIcon, m_ironSightOn);
		if (m_iconIsLit && m_disableIcon)
		{
			iconData->SetIconDrainAmount(m_perkId, 1.f, false);
			m_iconIsLit = false;
		}

		m_disableIcon = true;
	}

	IPerk::Update(dt);
}
