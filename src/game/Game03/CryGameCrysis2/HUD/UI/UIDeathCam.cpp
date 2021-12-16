#include "StdAfx.h"

#include "UIDeathCam.h"

#include "HUD/HUD.h"
#include "Graphics/2DRenderUtils.h"

#include "GameRules.h"
#include "Player.h"
#include "HUD/UI/UITagNames.h"

CRY_TODO( 10, 02, 2010, "HUD: Use the same defines as in UITagNames, moving them to a shared location. /MS");
static const ColorF UIDC_COLOR_DEAD(0.4f,0.4f,0.4f);
static const ColorF UIDC_COLOR_ENEMY(0.9f,0.1f,0.1f);
static const ColorF UIDC_COLOR_FRIEND(0.0353f,0.6235f,0.9137f);

UIDeathCam::UIDeathCam()
	: TDeathCamParent()
	, m_pFont( gEnv->pCryFont->GetFont("default") )
	, m_pReplayActors(NULL)
	, m_killerId(0)
{
	CHUD::AddHUDEventListener(this,"OnDeathcamStartPlay");
	CHUD::AddHUDEventListener(this,"OnDeathcamStopPlay");
}

UIDeathCam::~UIDeathCam()
{
	CHUD::RemoveHUDEventListener(this);
}

void UIDeathCam::UpdateHUD(void)
{
	CRecordingSystem::TReplayActorMap::const_iterator end_iterator = m_pReplayActors->end();
	for (CRecordingSystem::TReplayActorMap::iterator thisentity = m_pReplayActors->begin(); thisentity != end_iterator; ++thisentity) 
	{
		CRecordingSystem::SReplayActorInfo *srai = &thisentity->second;
		const EntityId eid = srai->chr_id;
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(eid);
		float fDistance = 0.f;
		srai->flags &= ~eRAF_ShowName;
		if (GetTagNamePosition(pEntity, &srai->drawPos, &srai->size, &fDistance))
		{
			if (g_pGame->GetPlayerVisTable()->CanLocalPlayerSee(eid, 5))
			{
				srai->flags |= eRAF_ShowName;
			}
		}
	}
}

// Stuff that needs to be in safe areas.
void UIDeathCam::DrawHUD( C2DRenderUtils* pRenderUtils, ScreenLayoutManager* pLayoutManager ) const
{

#if !RECORDING_SYSTEM_EXTRA_DEBUG
	CRecordingSystem::TReplayActorMap::iterator itKiller = m_pReplayActors->find(m_killerId);
	if (itKiller != m_pReplayActors->end())
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(itKiller->second.chr_id);
		if (pEntity)
		{
			float width  = pLayoutManager->GetVirtualWidth();
			float height = pLayoutManager->GetVirtualHeight();
			const float x = 0.0f;//(width - 140.f)/2.f;
			const float y = -height * 1/4.f;
			const float sx = 30.0f;
			const float sy = 30.0f;
			const ColorF col(1.f, 0.f, 0.f, 1.f);
			CRY_TODO( 01, 12, 2009, "HUD: Localise death-cam text. /FH");
			CryFixedStringT<256> killCamText;
			killCamText.Format("Killed by %s", pEntity->GetName());
			pRenderUtils->DrawText( x, y, sx, sy, killCamText, col, UIDRAWHORIZONTAL_CENTER,UIDRAWVERTICAL_CENTER, UIDRAWHORIZONTAL_CENTER,UIDRAWVERTICAL_CENTER);
		}
	}
#endif

	int killerTeam = 0;
	CRecordingSystem::TReplayActorMap::iterator itKillerActor = m_pReplayActors->find(m_killerId);
	if (itKillerActor != m_pReplayActors->end())
	{
		killerTeam = itKillerActor->second.team;
	}

	CRecordingSystem::TReplayActorMap::const_iterator end_iterator = m_pReplayActors->end();

	const ColorF* pColor;
	for (CRecordingSystem::TReplayActorMap::const_iterator thisentity = m_pReplayActors->begin(); thisentity != end_iterator; ++thisentity) 
	{
		if (thisentity->first != m_killerId)
		{
			const CRecordingSystem::SReplayActorInfo srai = thisentity->second;
			if (srai.flags & eRAF_ShowName)
			{
				const EntityId eid = srai.chr_id;
				const IEntity* pEntity = gEnv->pEntitySystem->GetEntity(eid);
				const bool friendly = (srai.team == killerTeam);
				if (pEntity)
				{
					if (srai.flags & eRAF_Dead)
					{
						pColor = &UIDC_COLOR_DEAD;
					}
					else
					{
						if (friendly)
						{
							pColor = &UIDC_COLOR_FRIEND;
						}
						else
						{
							pColor = &UIDC_COLOR_ENEMY;
						}
					}
					pRenderUtils->DrawText( srai.drawPos.x, srai.drawPos.y, srai.size, srai.size, pEntity->GetName(), *pColor, UIDRAWHORIZONTAL_CENTER);
				}
			}
		}
	}
}


void UIDeathCam::Draw() const
{
	if(!m_pReplayActors)
	{
		return;
	}

	if (g_pGame->GetRecordingSystem() && g_pGame->GetRecordingSystem()->IsPlayingBack())
	{
		C2DRenderUtils* pRenderUtils = g_pGame->GetHUD()->Get2DRenderUtils();
		ScreenLayoutManager* pLayoutManager = g_pGame->GetHUD()->GetLayoutManager();

		// Should be drawn with the default (xml def'd) layout.
		DrawHUD( pRenderUtils, pLayoutManager );

		// These borders should be draw at full screen.
		ScreenLayoutStates prevLayoutState = pLayoutManager->GetState();
		pLayoutManager->SetState(eSLO_DoNotAdaptToSafeArea|eSLO_ScaleMethod_None);

		ColorF c1(0, 0, 0, 1);
		float width  = pLayoutManager->GetVirtualWidth();
		float height = pLayoutManager->GetVirtualHeight();

		gEnv->pRenderer->SetState(GS_NODEPTHTEST);
		pRenderUtils->DrawQuad(0, 0, width, height/6.f, c1);
		pRenderUtils->DrawQuad(0, height * 5.f/6.f, width, height/6.f, c1);

		pLayoutManager->SetState(prevLayoutState);

		return;
	}
}

void UIDeathCam::Update( float ftime )
{
	if(!m_pReplayActors)
	{
		return;
	}

	if ( g_pGame->GetRecordingSystem() && g_pGame->GetRecordingSystem()->IsPlayingBack())
	{
		UpdateHUD();
	}
}

void UIDeathCam::OnHUDEvent( const SHUDEvent& deathCamEvent )
{
	switch(deathCamEvent.eventType)
	{
	case eHUDEvent_OnDeathcamStartPlay:
		{
			CRecordingSystem::TReplayActorMap* pReplayActors = static_cast<CRecordingSystem::TReplayActorMap*>(deathCamEvent.GetData(0).GetPtr());
			m_pReplayActors = pReplayActors;
			m_killerId = deathCamEvent.GetData(1).GetInt();
			return;
		}
	case eHUDEvent_OnDeathcamStopPlay:
		{
			//UIDeathCam::Get()->SetReplayEntities(NULL);
			return;
		}
	}
}