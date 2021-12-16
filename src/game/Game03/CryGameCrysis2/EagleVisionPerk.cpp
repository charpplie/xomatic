#include "StdAfx.h"
#include "EagleVisionPerk.h"

#include "Player.h"
#include "HUD/HUD.h"

#include "GameCodeCoverage/GameCodeCoverageTracker.h"

EagleVisionPerk::EagleVisionPerk()
{
	m_seenTarget = false;
}

void EagleVisionPerk::Update(const float dt)
{
	if(ShouldHighlight())
	{
		IActorSystem *pActorSystem=gEnv->pGame->GetIGameFramework()->GetIActorSystem();
		IActorIteratorPtr pIter = pActorSystem->CreateActorIterator();

		Vec3 viewDir;
		Vec3 playerPos;
		float fov;
		if(!m_seenTarget)
		{
			viewDir = gEnv->pRenderer->GetCamera().GetViewdir();
			playerPos = m_ownerPlayer->GetEntity()->GetWorldPos();
			fov = gEnv->pRenderer->GetCamera().GetFov();
		}

		while (CActor* pActor = (CActor*)pIter->Next())
		{
			EntityId actorId = pActor->GetEntityId();
			if(!m_ownerPlayer->IsFriendlyEntity(actorId))
			{
				g_pGame->GetHUD()->GetSilhouettes()->SetSilhouette(pActor,1.0f,0.0f,0.0f,1.0f,1.0f);
				CCCPOINT(Perk_EagleVision_Highlight);
				if(!m_seenTarget)
				{
					Vec3 actorPos = pActor->GetEntity()->GetWorldPos();

					if(viewDir.Dot((actorPos-playerPos).normalized()) > fov &&
						g_pGame->GetPlayerVisTable()->CanLocalPlayerSee(actorId, 5))
					{
						m_seenTarget = true;
						CAudioSignalPlayer::JustPlay("Perk_EagleVision", m_ownerPlayer->GetEntityId());
					}
				}
			}
		}
	}
	else
	{
		m_seenTarget = false;
	}
}

bool EagleVisionPerk::ShouldHighlight()
{
	if(m_ownerPlayer->IsClient())
	{
		IItem *pItem = m_ownerPlayer->GetCurrentItem();
		if(pItem)
		{
			IWeapon *pWeapon = pItem->GetIWeapon();
			if(pWeapon && pWeapon->IsZoomed())
			{
				return true;
			}
		}
	}

	return false;
}