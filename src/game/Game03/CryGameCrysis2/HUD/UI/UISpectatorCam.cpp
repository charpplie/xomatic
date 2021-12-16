#include "StdAfx.h"

#include "UISpectatorCam.h"

#include "HUD/HUD.h"
#include "Graphics/2DRenderUtils.h"
#include "ILevelSystem.h"
#include "ILocalizationManager.h"
#include "GameRules.h"
#include "GameRulesModules/IGameRulesSpectatorModule.h"
#include "GameRulesModules/IGameRulesSpawningModule.h"
#include "GameRulesModules/IGameRulesStateModule.h"

#define ENABLE_SPECYCAMCODE 1

static const float k_UISpectatorCam_SpecCamFontSize = 15.0f;
static const float k_UISpectatorCam_ypscale = 600.0f/768.0f;

CryFixedStringT<128> UISpectatorCam::s_strBuffer("");

UISpectatorCam::UISpectatorCam()
{
}

UISpectatorCam::~UISpectatorCam()
{

}

void UISpectatorCam::Draw_CGameRulesMPSpectator( void ) const
{
#if ENABLE_SPECYCAMCODE
	IActor* pClient = g_pGame->GetIGameFramework()->GetClientActor();
	if(!pClient)
	{
		return;
	}

	CActor*  pActorImpl = static_cast< CActor* >( pClient );

	C2DRenderUtils* p2d = g_pGame->GetHUD()->Get2DRenderUtils();
	assert(p2d);

	ScreenLayoutManager* slm = g_pGame->GetHUD()->GetLayoutManager();
	slm->SetState(eSLO_AdaptToSafeArea|eSLO_ScaleMethod_WithY);

	IRenderer*  ren = (gEnv ? gEnv->pRenderer : NULL);
	assert(ren);

	ColorF colf(1.f, 1.f, 1.f, 1.f);

	CActor::EActorSpectatorMode  mode = (CActor::EActorSpectatorMode) pActorImpl->GetSpectatorMode();

	if (mode)
	{
		p2d->DrawText(400,0,4*k_UISpectatorCam_SpecCamFontSize,4*k_UISpectatorCam_SpecCamFontSize, g_pGame->GetHUD()->LocalizeString("@ui_hud_spec_spectating"), colf, UIDRAWHORIZONTAL_CENTER );

		CGameRules* pGameRules = g_pGame->GetGameRules();
		IGameRulesSpectatorModule *specmod = pGameRules->GetSpectatorModule();

		if (specmod->GetNextMode(pActorImpl->GetEntityId(), 1, NULL) != mode)
		{
			s_strBuffer = g_pGame->GetHUD()->LocalizeString( "@ui_hud_spec_currentmode" );
			s_strBuffer.append( g_pGame->GetHUD()->LocalizeString( specmod->GetActorSpectatorModeName(mode) ) );
			p2d->DrawText(400,k_UISpectatorCam_ypscale*60,1.5f*k_UISpectatorCam_SpecCamFontSize,1.5f*k_UISpectatorCam_SpecCamFontSize, s_strBuffer.c_str(), colf, UIDRAWHORIZONTAL_CENTER );
			p2d->DrawText(400,k_UISpectatorCam_ypscale*580,2*k_UISpectatorCam_SpecCamFontSize,2*k_UISpectatorCam_SpecCamFontSize, g_pGame->GetHUD()->LocalizeString("@ui_hud_spec_gen_controls1"), colf, UIDRAWHORIZONTAL_CENTER );
		}

		switch (mode)
		{
		case CActor::eASM_Follow:
			{
				EntityId  tid = pActorImpl->GetSpectatorTarget();
				IEntity*  t = gEnv->pEntitySystem->GetEntity(tid);
				s_strBuffer.Format( "%s%s",g_pGame->GetHUD()->LocalizeString("@ui_hud_spec_currentlywatching"), (t ? t->GetName() : "?") );
				p2d->DrawText(400,k_UISpectatorCam_ypscale*164,1.5f*k_UISpectatorCam_SpecCamFontSize,1.5f*k_UISpectatorCam_SpecCamFontSize, s_strBuffer.c_str(), colf, UIDRAWHORIZONTAL_CENTER );
				p2d->DrawText(400,k_UISpectatorCam_ypscale*632,2*k_UISpectatorCam_SpecCamFontSize,2*k_UISpectatorCam_SpecCamFontSize, g_pGame->GetHUD()->LocalizeString("@ui_hud_spec_follow_controls1"), colf, UIDRAWHORIZONTAL_CENTER );

				break;
			}
		case CActor::eASM_CCTV:
			{
				EntityId  cid = pActorImpl->GetSpectatorCCTVCam();
				IEntity*  c = gEnv->pEntitySystem->GetEntity(cid);
				s_strBuffer.Format( "%s%s", g_pGame->GetHUD()->LocalizeString("@ui_hud_spec_currentcamera"), (c ? c->GetName() : "?") );
				p2d->DrawText(400,k_UISpectatorCam_ypscale*134,1.5f*k_UISpectatorCam_SpecCamFontSize,1.5f*k_UISpectatorCam_SpecCamFontSize, s_strBuffer.c_str(), colf, UIDRAWHORIZONTAL_CENTER );

				p2d->DrawText(400,k_UISpectatorCam_ypscale*602,2*k_UISpectatorCam_SpecCamFontSize,2*k_UISpectatorCam_SpecCamFontSize, g_pGame->GetHUD()->LocalizeString("@ui_hud_spec_cctv_controls1"), colf, UIDRAWHORIZONTAL_CENTER );
				p2d->DrawText(400,k_UISpectatorCam_ypscale*624,2*k_UISpectatorCam_SpecCamFontSize,2*k_UISpectatorCam_SpecCamFontSize, g_pGame->GetHUD()->LocalizeString("@ui_hud_spec_cctv_controls2"), colf, UIDRAWHORIZONTAL_CENTER );

				// Set camera entity ID in CCTV effect
				IActor* pClient = g_pGame->GetIGameFramework()->GetClientActor();
				if(CPlayer* pPlayer=((pClient && pClient->IsPlayer()) ? static_cast< CPlayer* >( pClient ) : NULL))
				{
					pPlayer->GetCCTVScreenEffect()->SetCameraEntityId(cid);
				}

				break;
			}
		}
	}
#endif
}

void UISpectatorCam::Draw_CGameRulesMPSpawningBase( void ) const 
{
#if ENABLE_SPECYCAMCODE
	if (IActor* pClient=g_pGame->GetIGameFramework()->GetClientActor())
	{
		CActor*  pActorImpl = static_cast< CActor* >( pClient );

		IGameRulesSpawningModule*  spawningModule = g_pGame->GetGameRules()->GetSpawningModule();
		IGameRulesStateModule*  stateModule = g_pGame->GetGameRules()->GetStateModule();

		bool  forceInstantRevive = (g_pGameCVars->g_spectate_DisableDead != 0);

		if (!forceInstantRevive && spawningModule->HandleAutoRevive() && (!stateModule || (stateModule->GetGameState() != IGameRulesStateModule::EGRS_PostGame)))
		{
			CGameRules*  pGameRules = g_pGame->GetGameRules();
			IGameRulesSpectatorModule*  specmod = pGameRules->GetSpectatorModule();
			float  autoSpectateTime = (specmod ? specmod->GetTimeFromDeathTillAutoSpectate() : 0.f);

			if ((pActorImpl->GetHealth() <= 0) && (autoSpectateTime > 0.f) && (pActorImpl->GetSpectatorMode() > 0) && (pActorImpl->GetSpectatorState() == CActor::eASS_Ingame))
			{
				if (!spawningModule->HasOptedOut() && ((spawningModule->GetNumLives() == 0) || (spawningModule->GetRemainingLives(pClient->GetEntityId()) > 0)))
				{
					IFFont*  pDefaultFont = gEnv->pCryFont->GetFont("default");

					IRenderer*  ren = (gEnv ? gEnv->pRenderer : NULL);
					assert(ren);

					ColorF colf( 1.f, 1.f, 1.f, 1.f );

					g_pGame->GetHUD()->GetLayoutManager()->SetState(eSLO_AdaptToSafeArea|eSLO_ScaleMethod_WithY);

					bool  shownWithCountdown = false;
					float  autoReviveTime = (spawningModule->GetTimeFromDeathTillAutoRevive() * 0.001f);
					if (autoReviveTime >= 0.f)
					{
						if (pActorImpl->IsPlayer())
						{
							CPlayer*  pPlyr = (CPlayer*) pActorImpl;
							float  deathTime = pPlyr->GetDeathTime();
							if (deathTime > 0.f)
							{
								float  curTime = gEnv->pTimer->GetFrameStartTime().GetSeconds();

								int  rem = (int) MAX(0.f, floorf(autoReviveTime - (curTime - deathTime)));
								rem += 1;

								g_pGame->GetHUD()->Get2DRenderUtils()->DrawText(400,k_UISpectatorCam_ypscale*481.f,2*k_UISpectatorCam_SpecCamFontSize,2*k_UISpectatorCam_SpecCamFontSize, g_pGame->GetHUD()->LocalizeString( "@ui_msg_spawn_preparingtoauto" ), colf, UIDRAWHORIZONTAL_CENTER);

								g_pGame->GetHUD()->Get2DRenderUtils()->DrawText(400,k_UISpectatorCam_ypscale*516.f,2.666f*k_UISpectatorCam_SpecCamFontSize,2.666f*k_UISpectatorCam_SpecCamFontSize, string().Format("%d", rem), colf, UIDRAWHORIZONTAL_CENTER);

								shownWithCountdown = true;
							}
						}
					}
					if (!shownWithCountdown)
					{
						g_pGame->GetHUD()->Get2DRenderUtils()->DrawText(400,k_UISpectatorCam_ypscale*505,2*k_UISpectatorCam_SpecCamFontSize,2*k_UISpectatorCam_SpecCamFontSize, g_pGame->GetHUD()->LocalizeString( "@ui_msg_spawn_preparingtoauto" ), colf, UIDRAWHORIZONTAL_CENTER);
					}

					if (g_pGameCVars->g_spectate_DisableSpawnOptOut == 0)
					{
						g_pGame->GetHUD()->Get2DRenderUtils()->DrawText(400,k_UISpectatorCam_ypscale*529,1.5f*k_UISpectatorCam_SpecCamFontSize,1.5f*k_UISpectatorCam_SpecCamFontSize, g_pGame->GetHUD()->LocalizeString( "@ui_msg_spawn_optoutcontrols" ), colf, UIDRAWHORIZONTAL_CENTER);
					}
				}
			}
		}
	}
#endif
}

void UISpectatorCam::Draw() const
{
	Draw_CGameRulesMPSpectator();
	Draw_CGameRulesMPSpawningBase();
}

void UISpectatorCam::Update( float ftime )
{

}

void UISpectatorCam::OnHUDEvent( const SHUDEvent& event )
{

}