#include "StdAfx.h"
#include "HUD_Impl.h"

#include "HUD/HUD.h"
#include "HUD/ScreenLayoutManager.h"
#include "HUD/ScreenLayoutUtils.h"
#include "HUD/UIElementManager.h"
#include "HUD/HUDCVars.h"

#include "Graphics/2DRenderUtils.h"

#include "HUD/UI/UIButtonPromptRegion.h"
#include "HUD/UI/UICoverEntireScreen.h"

#include "Game.h"
#include "GameCVars.h"
#include "GameActions.h"
#include "PerkDbgDisplay.h"
#include "PerkIconData.h"
#include "Menus/MPHub.h"
#include "StringUtils.h"
#include "Utility/StringUtils.h"
#include "Player.h"
#include "GameRules.h"


#include "IMovieSystem.h" // Cut-scene controls
#include "IMaterialEffects.h"
#include "IWorldQuery.h"

#include "RecordingSystem.h"
#include "Graphics/2DRenderUtils.h"

#include "ILevelSystem.h"
#include "GodMode.h"

//#include "Graphics/ScreenFader.h"

static AUTOENUM_BUILDNAMEARRAY(s_hideHUDReasonNames, HideHUDReasonList);

const static float SUIT_MODE_CHANGED_SIGNAL = 1.0f;
const static float SUIT_STATE_CHANGED_SIGNAL = 1.0f;

const static float MISSION_OBJECTIVE_ICON_OFFSCREEN_SIZE = 32.f;

#if ENABLE_HUD_IMPL_DEBUGGING
void HudLog( int level, const char* format, ... )
{
	/*if(g_pGameCVars->ui2_debug >= level )
	{
		va_list args;
		va_start(args,format);
		gEnv->pLog->LogV( ILog::eMessage,format,args );
		va_end(args);
	}*/
}
#endif

//******************************************************************************
#define C2SP_LOW_HEALTH_SCREEN_OVERLAY 1

//******************************************************************************

CHUD_Impl::CHUD_Impl(CHUD* in_pHUD, C2DRenderUtils* p2DRenderUtils) :
m_eLastSuitMode(eNanoSuitMode_Last),
m_eLastSuitState(eNanoSuitState_Normal),
m_fSuitModeChangedTimer(0.0f),
m_fSuitStateChangedTimer(0.0f),
m_fWeaponSwitchTimer(0.0f),
m_hudColor(0.8f, 0.3f, 0.3f),
editorModeActive(false),
globalAlphaModificator(1.0f),
m_lastHitPoints(0.0f),
m_mercyTimeTimer(0.0f),
m_screenRedValue(0.0f),
m_pHUD(in_pHUD),
m_pUIDraw(p2DRenderUtils)
{

	m_hideHUDFlags = eHideHUDReason_None;
	m_mostRecentElementsDrawn = kHudElementsDrawnFlag_none;

	m_safeAreas = NULL;

	if (g_pGame)
	{
		m_safeAreas = new SafeAreaRenderer( m_pUIDraw, m_pHUD->GetLayoutManager() );
		// TODO : Wait for the sw2_hud font to be in the PAKS. Keeping default for nettests.
		p2DRenderUtils->SetFont( gEnv->pCryFont->GetFont("default") );
		//m_pDefaultFont = gEnv->pCryFont->NewFont("cw2_hud");
		//m_pDefaultFont->Load("fonts/cw2_hud.xml");
	}

	m_soundSuitMercyTime.SetSignal("Player_EnterMercyTime");
	m_soundSuitMercyTimeEnd.SetSignal("Player_LeaveMercyTime");
}

//******************************************************************************

CHUD_Impl::~CHUD_Impl()
{
	delete m_safeAreas;

	m_pUIDraw = NULL;
	m_safeAreas = NULL;
}

//******************************************************************************

void CHUD_Impl::Unload()
{
	Clear();
}

//******************************************************************************

void CHUD_Impl::ShowHUD(bool onOff, EHideHUDReason reason /*= eHideHUDReason_General*/)
{
	if (onOff)
		m_hideHUDFlags &= ~reason;
	else
		m_hideHUDFlags |= reason;
}

bool CHUD_Impl::ShouldShowHUD()
{
	return (m_hideHUDFlags == eHideHUDReason_None);
}

//******************************************************************************

void CHUD_Impl::Update(float fDeltaTime)
{
	IActor * localActor = g_pGame->GetIGameFramework()->GetClientActor();
	bool gameIsStarted = g_pGame->GetIGameFramework()->IsGameStarted();
	bool gameIsPaused = g_pGame->GetIGameFramework()->IsGamePaused();

	if(localActor && gameIsStarted && !gameIsPaused)
	{
		UpdateData(localActor, fDeltaTime);

#if CRY_WATCH_ENABLED
		CryFixedStringT<128> list;
		for (int i = 0; i < UIElementItemList_numBits; ++ i)
		{
			if (BIT(i) & m_mostRecentElementsDrawn)
			{
				list.append(" ");
				list.append(CUIElement::s_drawnElementBitfieldNames[i]);
			}
		}
		HudDbgDisplay("HUD elements visible = %d:%s", m_mostRecentElementsDrawn, list.c_str());
#endif

		Draw();
	}
	else
	{
		HudDbgDisplay("Not drawing HUD because: localActor=%p started=%d paused=%d", localActor, gameIsStarted, gameIsPaused);
	}
}

//******************************************************************************

void CHUD_Impl::UpdateData(IActor * localActor, float fDeltaTime)
{
	int localActorHealth = localActor->GetHealth();

	TBitfield elementsDrawn = (localActorHealth <= 0) ? kHudElementsDrawnFlag_dead : kHudElementsDrawnFlag_none;

	CPerkIconData * perkIconData = CPerkIconData::GetLocalDataToRender();
	perkIconData->Tick(fDeltaTime);

#if C2SP_LOW_HEALTH_SCREEN_OVERLAY // TODO: Should probably all be moved into CPlayer::UpdateClientHealthFX()
	if(m_mercyTimeTimer > 0.0f)
	{
		m_mercyTimeTimer -= fDeltaTime;
		if(m_mercyTimeTimer <= 0.0f)
		{
			m_mercyTimeTimer = 0.0f;
		}
	}
	else if(m_screenRedValue > 0.0f)
	{
		float ratio = 1.5f;
		m_screenRedValue -= fDeltaTime / ratio;
		if(m_screenRedValue <= 0.0f)
		{
			m_screenRedValue = 0.0f;
			m_soundSuitMercyTime.Stop();
			m_soundSuitMercyTimeEnd.Play();
		}
	}

	if (! gEnv->bMultiplayer)
	{
		const float maxHealth = (float)localActor->GetMaxHealth();
		const float actualHealth = (float)localActorHealth;

		float healthThreshold = 20;
		if(ICVar* pCVar = gEnv->pConsole->GetCVar("g_playerLowHealthThreshold"))
		{
			healthThreshold = pCVar->GetFVal();
		}

		const float currentHitPoints = min(actualHealth, maxHealth);
		if(m_lastHitPoints != currentHitPoints)
		{
			if( (m_lastHitPoints >= healthThreshold) && (currentHitPoints < healthThreshold) )
			{
				m_mercyTimeTimer = 1.5f;
				if(ICVar* pCVar = gEnv->pConsole->GetCVar("pl_health.normal_threshold_time_to_regenerate"))
				{
					m_mercyTimeTimer = pCVar->GetFVal();
				}
				if (g_pGameCVars->hud_DeathRedScreen && !CGodMode::GetInstance().IsGod())
				{
					m_screenRedValue = 1.0f;
					m_soundSuitMercyTime.Play();
				}
			}
		}

		m_lastHitPoints = currentHitPoints;

		CUICoverEntireScreen * lowHealthOverlay = CUICoverEntireScreen::GetInstanceWithName("LowHealthOverlay");
		if (lowHealthOverlay)
		{
			lowHealthOverlay->Set(m_screenRedValue);
		}
	}
#endif

	m_mostRecentElementsDrawn = elementsDrawn;
	m_pHUD->RegisterElementsDrawn( elementsDrawn );

#if 0 // Handled by HUD states
	std::list<CUIElement*>::iterator it = m_elements.begin();
	while( it != m_elements.end() )
	{
		if( (*it)->IsDone() )
		{
			HudLog( 1, "CHUD_Impl::UpdateData() : erasing UI2 element %s", (*it)->GetName() );
			SAFE_DELETE( *it );
			it = m_elements.erase( it );
		}
		else
		{
			(*it)->Update(fDeltaTime);
			++it;
		}
	}
#endif
}

//******************************************************************************
void CHUD_Impl::Clear( void )
{
	m_soundSuitMercyTime.Stop();
	m_soundSuitMercyTimeEnd.Stop();
}
//******************************************************************************

void CHUD_Impl::Draw()
{
	if (!ShouldShowHUD())
	{
#if CRY_WATCH_ENABLED
		CryFixedStringT<128> list;
		for (int i = 0; i < HideHUDReasonList_numBits; ++ i)
		{
			if (BIT(i) & m_hideHUDFlags)
			{
				list.append(" ");
				list.append(s_hideHUDReasonNames[i]);
			}
		}
		HudDbgDisplay("Not drawing HUD because:%s", list.c_str());
#endif
		return;
	}

	CActor  *pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetClientActor());
	bool  spectating = (pActor && (pActor->GetSpectatorMode() != CActor::eASM_None));

	m_pUIDraw->PreRender();
	m_pHUD->GetLayoutManager()->SetState(eSLO_FullScreen);

	//activate alpha blending before rendering
	gEnv->pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);

#if HIDE_ENTIRE_HUD_WHILE_SPECTATING
	CRY_FIXME( 23, 10, 2009, "[HUD] Spectator HUD mode should be implemented properly. /PJH")
	if (spectating)
	{
		m_pUIDraw->PostRender();
		return;
	}
#endif

#if 0 // Handled by HUD states
	DrawElements(gEnv->pTimer->GetFrameTime());
#endif

	if( g_pGame->GetHUD()->GetCVars()->hud_drawSafeAreas )
	{
		m_safeAreas->DrawSafeAreas();
	}

	//DrawSuitModeSpecifics();

#if ENABLE_HUD_EXTRA_DEBUG
	if (g_pGame->GetHUD()->GetCVars()->hud_drawCentreOfScreen != 0)
	{
		ColorF dbgColour(1.f, 1.f, 1.f, 1.f);
		CHUD* pHud = g_pGame->GetHUD();
		ScreenLayoutManager* pLayoutManager = pHud->GetLayoutManager();
		ScreenLayoutStates prevStates = pLayoutManager->GetState();
		pLayoutManager->SetState(eSLO_ScaleMethod_None|eSLO_DoNotAdaptToSafeArea);
		gEnv->pRenderer->SetState(GS_NODEPTHTEST);
		m_pUIDraw->DrawQuad(  399.5f,    0.f,   1.f,  600.f, dbgColour); // T2B
		m_pUIDraw->DrawQuad(     0.f, 299.5f, 800.f,    1.f, dbgColour); // L2R
		pLayoutManager->SetState(prevStates);

	}
#endif

	m_pUIDraw->PostRender();

}

ColorF CHUD_Impl::GetBaseColor() const
{
	return m_hudColor;
}

#if 0 // Handled by HUD states
void CHUD_Impl::DrawElements( float frameTime ) const 
{
	// Why not draw all Elements?
	/*CUIElement *pRoot = FindElement( rootName.c_str() );
	if(pRoot)
		pRoot->Draw(this);*/

	std::list<CUIElement*>::const_iterator it = m_elements.begin();
	std::list<CUIElement*>::const_iterator end = m_elements.end();
	while(it!=end)
	{
		CUIElement* el = (*it);
		HudLog( 3, "CHUD_Impl::DrawElements() : Drawing '%s'", el->GetName() );
		el->Draw();
		++it;
	}
}
#endif

void CHUD_Impl::ConvertSecondsToTimerString( const int s, string* in_out_string, bool stripZeroElements )
{
	int hours=0, mins=0, secs=0;
	secs = s;
	hours = (int)floor(((float)secs)*(1/60.0f)*(1/60.0f));
	secs -= hours*60*60;
	mins = (int)floor(((float)secs)*(1/60.0f));
	secs -= mins*60;
	string& l_time = (*in_out_string);
	if (stripZeroElements)
	{
		if (hours > 0)
		{
			l_time.Format( "%.2d:%.2d:%.2d", hours, mins, secs );
		}
		else if (mins > 0)
		{
			l_time.Format( "%.2d:%.2d", mins, secs );
		}
		else
		{
			l_time.Format( "%.2d", secs );
		}
	}
	else
	{
		l_time.Format( "%.2d:%.2d:%.2d", hours, mins, secs );
	}
	return;
}
