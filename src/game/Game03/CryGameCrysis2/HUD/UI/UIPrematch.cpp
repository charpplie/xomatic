#include "StdAfx.h"

#include "UIPrematch.h"

#include "ILevelSystem.h"
#include "HUD/HUD.h"
#include "HUD/HUD_Impl.h"
#include "HUD/UIElementManager.h"
#include "Graphics/2DRenderUtils.h"
#include "Game.h"
#include "GameRules.h"
#include "GameRulesModules/GameRulesMPSpawning.h"
#include "Actor.h"

static const float k_UIPrematch_SpecCamFontSize = 15.0f;
static const float k_UIPrematch_ypscale = 600.0f/768.0f;

string /*CryFixedStringT<128>*/ UIPrematch::s_strBuffer("");

UIPrematch::UIPrematch()
 : TPrematchParent()
{
	m_joinGameButtonTexture = 0;

	Reinit();

	CHUD::AddHUDEventListener(this,"OnUpdateGameStartMessage");
	CHUD::AddHUDEventListener(this,"OnWaitingForPlayers");
	CHUD::AddHUDEventListener(this,"OnGameStart");
	CHUD::AddHUDEventListener(this,"OnUpdateGameResumeMessage");
}

UIPrematch::~UIPrematch( void )
{
	CHUD::RemoveHUDEventListener(this);
}

void UIPrematch::Initialize( const IItemParamsNode* element, IUIElement* parent )
{
	TPrematchParent::Initialize(element, parent);
}

void UIPrematch::Update(float frameTime)
{
	if (m_joinGameButtonTexture == 0)
	{
		const ITexture * tex = g_pGame->GetHUD()->GetElementManager()->GetTexture("controller_face_down");
		assert (tex);
		if (tex)
		{
			m_joinGameButtonTexture = tex->GetTextureID();
		}
	}
}

void UIPrematch::Draw(void) const
{
	if (gEnv->bEditor)
	{
		return;
	}

	IActor* pClient = g_pGame->GetIGameFramework()->GetClientActor();
	CActor* pActorImpl = pClient ? static_cast< CActor* >( pClient ) : NULL;
	CActor::EActorSpectatorMode  mode = pActorImpl ? (CActor::EActorSpectatorMode) pActorImpl->GetSpectatorMode() : CActor::eASM_None;

	if( mode && pActorImpl->GetSpectatorState()==CActor::eASS_None )
	{
		ColorF colf(1.f, 1.f, 1.f, 1.f);

		s_strBuffer.Format("@ui_rules_%s", g_pGame->GetGameRules()->GetEntity()->GetClass()->GetName());
		s_strBuffer = g_pGame->GetHUD()->LocalizeString(s_strBuffer.c_str(), NULL, NULL);

		C2DRenderUtils* p2d = g_pGame->GetHUD()->Get2DRenderUtils();
		p2d->DrawText(400,k_UIPrematch_ypscale*200,4*k_UIPrematch_SpecCamFontSize,4*k_UIPrematch_SpecCamFontSize, s_strBuffer.c_str(), colf, UIDRAWHORIZONTAL_CENTER);

		const char* mapName = g_pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel()->GetLevelInfo()->GetDisplayName();
		p2d->DrawText(400,k_UIPrematch_ypscale*250,3*k_UIPrematch_SpecCamFontSize,3*k_UIPrematch_SpecCamFontSize, mapName, colf, UIDRAWHORIZONTAL_CENTER);

		CGameRules *pGameRules = g_pGame->GetGameRules();
		if (pGameRules->GetHostMigrationState() == CGameRules::eHMS_NotMigrating)
		{
			bool  mpMidRoundJoinDenied = false;

			if (gEnv->bMultiplayer)
			{
				if (CGameRulesMPSpawningBase* mpSpawnMod=(CGameRulesMPSpawningBase*)pGameRules->GetSpawningModule())
				{
					mpMidRoundJoinDenied = !mpSpawnMod->CheckMidRoundJoining(pClient->GetEntityId());
				}
			}

			if (mpMidRoundJoinDenied)
			{
				s_strBuffer = g_pGame->GetHUD()->LocalizeString("@ui_hud_spec_round_in_progress_waiting", NULL, NULL);
				p2d->DrawText(400,k_UIPrematch_ypscale*540,2*k_UIPrematch_SpecCamFontSize,2*k_UIPrematch_SpecCamFontSize, s_strBuffer.c_str(), colf, UIDRAWHORIZONTAL_CENTER );
			}
			else
			{
				s_strBuffer = g_pGame->GetHUD()->LocalizeString("@ui_hud_spec_join_game", NULL, NULL);
				p2d->DrawText(400,k_UIPrematch_ypscale*540,2*k_UIPrematch_SpecCamFontSize,2*k_UIPrematch_SpecCamFontSize, s_strBuffer.c_str(), colf, UIDRAWHORIZONTAL_CENTER );

				// scaleY _is_ currently still needed to keep the width:height ratio of the texture correct! [TF]
				ScreenLayoutManager * layoutMgr = g_pGame->GetHUD()->GetLayoutManager();
				const float scaleUsingWidthValues = (gEnv->pRenderer->GetWidth() / layoutMgr->GetVirtualWidth()); 
				const float scaleUsingHeightValues = (gEnv->pRenderer->GetHeight() / layoutMgr->GetVirtualHeight());
				const float scaleY = scaleUsingWidthValues / scaleUsingHeightValues;

				const float iconWidth = 35.f;
				const float iconHeight = iconWidth * scaleY;

				p2d->DrawImage(m_joinGameButtonTexture,	400.f - (iconWidth  * 0.5f), 400.f - (iconHeight * 0.5f),	iconWidth, iconHeight, 0.0f, colf, 0.f, 1.f, 1.f, 0.f);
			}
		}
		else
		{
			const int timeRemaining = int(pGameRules->GetHostMigrationTimeTillResume() + 0.5f);

			typedef CryFixedStringT<8> TFixedString; 
			TFixedString timeString;
			timeString.Format("%.2d", timeRemaining);
	
			s_strBuffer = g_pGame->GetHUD()->LocalizeString("@ui_hud_spec_host_migrating_waiting", timeString.c_str(), NULL);
			p2d->DrawText(400,k_UIPrematch_ypscale*540,2*k_UIPrematch_SpecCamFontSize,2*k_UIPrematch_SpecCamFontSize, s_strBuffer.c_str(), colf, UIDRAWHORIZONTAL_CENTER );
		}
	}
	else
	{
		switch( m_state )
		{
		case ePMS_NONE:
			return;
		case ePMS_GameStartCountDown:
			// Pregame status - start countdown / required players
			if( m_countdown_timer >= 0 )
			{
				CHUD* pHUD = g_pGame->GetHUD();
				pHUD->GetHUDImpl()->ConvertSecondsToTimerString( m_countdown_timer, &s_strBuffer, true );
				s_strBuffer = pHUD->LocalizeString("@mp_GameStartingCountdown", s_strBuffer.c_str(), NULL);
				pHUD->GetLayoutManager()->SetState(eSLO_ScaleMethod_WithY);
				pHUD->Get2DRenderUtils()->DrawText( GetPosX(), GetPosY(), GetWidth(), GetHeight(), s_strBuffer.c_str(), textColor, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER );

				pHUD->RegisterElementsDrawn( kHudElementsDrawnFlag_startCountdown );

				return;
			}
		case ePMS_WaitingForPlayers:
			{
				if( !m_required_players )
					return;

				CHUD* pHUD = g_pGame->GetHUD();
				s_strBuffer.Format("%d", m_required_players);
				s_strBuffer = pHUD->LocalizeString("@mp_MinPlayerWarning", s_strBuffer.c_str(), NULL);
				pHUD->GetLayoutManager()->SetState(eSLO_ScaleMethod_WithY);
				pHUD->Get2DRenderUtils()->DrawText( GetPosX(), GetPosY(), GetWidth(), GetHeight(), s_strBuffer.c_str(), textColor, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER );

				pHUD->RegisterElementsDrawn( kHudElementsDrawnFlag_startCountdown );

				return;
			}
		case ePMS_WaitingForTeamPlayers:
			{
				if( !m_required_team_players )
					return;

				CHUD* pHUD = g_pGame->GetHUD();
				s_strBuffer.Format("%d", m_required_team_players);
				s_strBuffer = pHUD->LocalizeString("@mp_MinTeamWarning", s_strBuffer.c_str(), NULL);
				pHUD->GetLayoutManager()->SetState(eSLO_ScaleMethod_WithY);
				pHUD->Get2DRenderUtils()->DrawText( GetPosX(), GetPosY(), GetWidth(), GetHeight(), s_strBuffer.c_str(), textColor, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER );

				pHUD->RegisterElementsDrawn( kHudElementsDrawnFlag_startCountdown );
				return;
			}
		case ePMS_GameResumeCountDown:
			{
				CHUD* pHUD = g_pGame->GetHUD();
				pHUD->GetHUDImpl()->ConvertSecondsToTimerString( m_countdown_timer, &s_strBuffer, true );
				s_strBuffer = pHUD->LocalizeString("@mp_GameResumingCountdown", s_strBuffer.c_str(), NULL);
				pHUD->GetLayoutManager()->SetState(eSLO_ScaleMethod_WithY);
				pHUD->Get2DRenderUtils()->DrawText( GetPosX(), GetPosY(), GetWidth(), GetHeight(), s_strBuffer.c_str(), textColor, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER, UIDRAWHORIZONTAL_CENTER, UIDRAWVERTICAL_CENTER );

				pHUD->RegisterElementsDrawn( kHudElementsDrawnFlag_startCountdown );
				return;
			}
		}
	}

	//return elementsDrawn;
}

void UIPrematch::OnHUDEvent(const SHUDEvent& event)
{
	switch( event.eventType )
	{
		case eHUDEvent_OnWaitingForPlayers:
			{
				if( g_pGame->GetGameRules()->GetTeamCount())
				{
					m_required_team_players = g_pGame->GetCVars()->g_minteamlimit;
					m_state = ePMS_WaitingForTeamPlayers;
				}
				else
				{
					m_required_players = g_pGame->GetCVars()->g_minplayerlimit;
					m_state = ePMS_WaitingForPlayers;
				}
				m_countdown_timer = -1;
				return;
			}
		case eHUDEvent_OnUpdateGameStartMessage:
			{
				m_state = ePMS_GameStartCountDown;
				m_countdown_timer = event.GetData(0).GetInt();
				return;
			}
		case eHUDEvent_OnGameStart :
			{
				Reinit();
				return;
			}
		case eHUDEvent_OnUpdateGameResumeMessage :
			{
				m_state = ePMS_GameResumeCountDown;
				m_countdown_timer = event.GetData(0).GetInt();
				return;
			}
	}
}

void UIPrematch::Reinit( void )
{
	m_state = ePMS_NONE;
	m_required_team_players = 0;
	m_required_players = 0;
	m_countdown_timer = -1;
}