#include "StdAfx.h"
#include "HUD.h"

#include "HUD/HUDAsset.h"
#include "HUD/HUDEventDispatcher.h"
#include "HUD/HUDEventTranslator.h"
#include "HUD/HUDObjectManager.h"
#include "HUD/HUDState.h"
#include "HUD/HUDCVars.h"
#include "HUD/HUDStateManager.h"
#include "HUD/UIElementManager.h"
#include "HUD/HUD_Impl.h" // TODO : Remove
#include "HUD/HUDTester.h"
#include "HUD/ScreenLayoutManager.h"
#include "HUD/HUDNULLFlashVariableObject.h"
#include "Graphics/2DRenderUtils.h"
#include "Menus/FlashMenuObject.h"
#include "FrontEnd/WarningsManager.h"
#include "FrontEnd/FrontEnd.h"
#include "FrontEnd/FlashFrontEnd.h"
#include "HUDMissionObjectiveSystem.h"

#include "Game.h"
#include "GameRules.h"
#include "IRenderer.h"
#include "GameActions.h"
#include <ITweakMenuController.h>

#include "GameRulesModules/GameRulesModulesManager.h"
#include "GameRulesModules/IGameRulesSpectatorModule.h"

#include "ILocalizationManager.h"
#include "Utility/StringUtils.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IHUDAsset * CHUD::s_pHUDAssetNULL = NULL;
IFlashVariableObject * CHUD::s_pHUDFlashvariableObjectNULL = NULL;

#if ENABLE_HUD_TESTS
HUDTester * CHUD::s_hudTester = NULL;
#endif // ENABLE_HUD_EXTRA_DEBUG

// ---------------------------------

CHUD::CHUD() :
m_pEventTranslator(NULL),
m_pImpl(NULL),
m_p2DRenderUtils(NULL),
m_pSilhouettes(NULL),
m_elementsDrawn(0),
m_lastElementsDrawn(0),
m_isLoaded(false)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "HUD initialisation");

	// CVARS
	m_pHUDCVars = new CHUDCVars();

	m_canvasSize = Vec2i( gEnv->pRenderer->GetWidth(), gEnv->pRenderer->GetHeight() );
	m_sizeObjects.clear();

	m_pEventTranslator = new CHUDEventTranslator();
	m_layoutManager = new ScreenLayoutManager();
	m_p2DRenderUtils = new C2DRenderUtils(m_layoutManager);
	m_pSilhouettes = new CHUDSilhouettes();
	m_pImpl          = new CHUD_Impl(this, m_p2DRenderUtils);

	s_pHUDAssetNULL = new CHUDAssetNull();
//	s_pHUDFlashvariableObjectNULL = new CNULLFlashVariableObject(); //Diesel cut

	m_objectManager = new CHUDObjectManager();
	m_elementManager = new CUIElementManager();
	m_stateManager = new CHUDStateManager(m_objectManager,m_elementManager);

	m_pTweakMenuController = gEnv->pGame->GetIGameFramework()->CreateITweakMenuController();
	CHUDEventDispatcher::CheckRegisteredEvents( ); // sanity check.
}

CHUD::~CHUD()
{
	SAFE_DELETE(m_pTweakMenuController);

	// CVARS
	SAFE_DELETE(m_pHUDCVars);

	Unload();
	
	SAFE_DELETE(m_pImpl);

	SAFE_DELETE( m_layoutManager );
	SAFE_DELETE(m_p2DRenderUtils);
	SAFE_DELETE(m_pSilhouettes);

	SAFE_DELETE( m_objectManager );
	SAFE_DELETE( m_stateManager );

	SAFE_DELETE(m_pEventTranslator);

	SAFE_DELETE(s_pHUDFlashvariableObjectNULL);
	SAFE_DELETE(s_pHUDAssetNULL); 
}



#if ENABLE_HUD_TESTS
/*static*/ void CHUD::CmdTestHud(IConsoleCmdArgs *pArgs)
{
	if( pArgs->GetArgCount() != 2 )
	{
		CryLogAlways("hud_test accepts a file path as it's parameter.");
		return;
	}

	if(!s_hudTester)
	{
		s_hudTester = new HUDTester(g_pGame->GetHUD()->m_stateManager, g_pGame->GetHUD()->m_objectManager);
	}

	s_hudTester->Start(pArgs->GetArg(1));
}

/*static*/ void CHUD::CmdTestHudDone(IConsoleCmdArgs *pArgs)
{
	assert(s_hudTester);
	SAFE_DELETE(s_hudTester);
	assert(!s_hudTester);
}
#endif // ENABLE_HUD_TESTS

#if ENABLE_HUD_EXTRA_DEBUG
/*static*/ void CHUD::CmdForceDrawAll( IConsoleCmdArgs *pArgs )
{
	gEnv->pConsole->ExecuteString( "hud_forceShowVehicleHUD 1" );
	gEnv->pConsole->ExecuteString( "hud_forceInteractive 1" );
	gEnv->pConsole->ExecuteString( "hud_forceLowAmmoOn 1" );
}
#endif // ENABLE_HUD_EXTRA_DEBUG

void CHUD::RegisterElementsDrawn( TBitfield drawnElements )
{
	m_elementsDrawn |= drawnElements;
}

void CHUD::AddHUDEventListener(IHUDEventListener* pListener, const char* eventName)
{
	CHUDEventDispatcher::AddHUDEventListener(pListener, eventName);
}



void CHUD::RemoveHUDEventListener(IHUDEventListener* pListener)
{
	CHUDEventDispatcher::RemoveHUDEventListener(pListener);
}



void CHUD::CallEvent(const SHUDEvent& event)
{
	CHUDEventDispatcher::CallEvent(event);
}

void CHUD::Reload()
{
	Unload();
	g_pGame->GetMOSystem()->DeactivateObjectives();
	Load();
	SHUDEvent event;
	event.eventType = eHUDEvent_OnHUDReload;
	CHUD::CallEvent(event);
}

void CHUD::Load()
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "HUD initialisation");

	assert( g_pGame );
	if (g_pGame)
		g_pGame->GetIGameFramework()->RegisterListener(this,"HUD", FRAMEWORKLISTENERPRIORITY_GAME);

	if (gEnv->pMovieSystem)
		gEnv->pMovieSystem->AddMovieListener(NULL, this);	//NULL means we're listening to all sequences

	m_pEventTranslator->Init();

	assert(m_objectManager);
	assert(m_stateManager);

	// UI Elements & HUDObjects need to be loaded before HUDStates
	m_elementManager->Load(); 

	m_objectManager->LoadObjectsFromFileList( "Scripts/HUD/HUDObjectFileList.xml" );

	m_stateManager->LoadStatesFromFileList( "Scripts/HUD/HUDStateFileList.xml" );


	if(CGameRules* pGameRules = g_pGame->GetGameRules())
	{
		// Set the game mode type.
		SHUDEvent gameModeSetEvent;
		gameModeSetEvent.eventType = eHUDEvent_OnGamemodeChange;
		string gameModename = pGameRules->GetEntity()->GetClass()->GetName();
		gameModeSetEvent.eventPtrData = &gameModename;
		CHUDEventDispatcher::CallEvent( gameModeSetEvent );
	}

	// Force asset updates, don't rely on detection of resolution change.
	m_canvasSize.set( gEnv->pRenderer->GetWidth(), gEnv->pRenderer->GetHeight() );
	m_layoutManager->UpdateHUDCanvasSize();

	m_isLoaded = true;

	ActivateState("no_hud");
}

void CHUD::Unload()
{
	SHUDEvent event;
	event.eventType = eHUDEvent_OnHUDUnload;
	CHUDEventDispatcher::CallEvent( event );

	m_elementManager->Clear();
	m_objectManager->ClearObjects();
	m_stateManager->ClearStates();

	m_pEventTranslator->Clear();
	
	CRY_TODO( 23, 10, 2009, "Move HUD_impl control into HUD and then remove. /FH")	
	m_pImpl->Unload();

	if (gEnv->pMovieSystem)
		gEnv->pMovieSystem->RemoveMovieListener(NULL, this);	//NULL means we're listening to all sequences

	if (g_pGame)
		g_pGame->GetIGameFramework()->UnregisterListener(this);

	m_isLoaded = false;
}



void CHUD::ActivateState( const char* stateName )
{
	if(!m_isLoaded)
		return;

	if( g_pGameCVars->g_multiplayerDefault )
	{
		CFlashFrontEnd* pMenu = g_pGame->GetFlashMenu();
		if( pMenu && pMenu->IsMenuActive(CFlashFrontEnd::eFlM_IngameMenu) )
		{
			m_stateManager->ActivateState( "mp_menus" );
			return;
		}
		else if( m_pEventTranslator->IsSelectMenuOpen() )
		{
			if( m_pEventTranslator->IsSelectmapMenuOpen() )
				m_stateManager->ActivateState( "mp_map" );
			else
				m_stateManager->ActivateState( "mp_menus" );
			return;
		}
	}

	m_stateManager->ActivateState(stateName);
}



void CHUD::ActivateDefaultState( void )
{
	//Currently default is based on game mode defaults.
	// This may change to be gamemode's default or spectator 
	// cam or some other state...?
	if (g_pGame->GetGameRules())
	{
		IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
		CRY_ASSERT_MESSAGE( pClientActor, "HUD: Activate default state being called when we have no actor!" );
		if( pClientActor && pClientActor->IsDead() )
		{
			// dead so we may be in spectator mode or in death cam 
			// mode in which case the state should be set by the
			// controlling module.
			IActor* pClient = g_pGame->GetIGameFramework()->GetClientActor();
			CActor* pActorImpl = pClient ? static_cast< CActor* >( pClient ) : NULL;
			CActor::EActorSpectatorMode  spetatorMode = pActorImpl ? (CActor::EActorSpectatorMode) pActorImpl->GetSpectatorMode() : CActor::eASM_None;

			CRY_ASSERT_MESSAGE( spetatorMode, "HUD: Activate default state being called when dead and _not_ spectating!" );
			if( spetatorMode )
			{
				if( pActorImpl->GetSpectatorState()==CActor::eASS_None )
				{
					ActivateState( "mp_welcome_screen" );
				}
				else
				{
					ActivateState( "mp_spectator" );
				}
			}
		}
		else
		{
			const char* sz_gameModeName = g_pGame->GetGameRules()->GetEntity()->GetClass()->GetName();
			// Set the default HUD state.
			const char* defaultHudState = CGameRulesModulesManager::GetInstance()->GetDefaultHud( sz_gameModeName );
			ActivateState( defaultHudState );
			GetHUDImpl()->ShowHUD(true,eHideHUDReason_Menu);
		}
	}
}



void CHUD::OnPostUpdate(float frameTime)
{
#if ENABLE_HUD_TESTS
	/*if( HUDTester::s_running )
	{								  
		m_p2DRenderUtils->PreRender();
		m_layoutManager->SetState( eSLO_FullScreen );
		m_p2DRenderUtils->DrawQuad(0,0,static_cast<float>(m_layoutManager->GetVirtualWidth())/2.0f, m_layoutManager->GetVirtualHeight(), ColorF(1.f,1.f,1.f,0.3f) );
		m_p2DRenderUtils->PostRender();
	}
	*/
#endif // ENABLE_HUD_TESTS

	// Check for resolution change
	Vec2i curCanvasSize( gEnv->pRenderer->GetWidth(), gEnv->pRenderer->GetHeight() );
	if( m_canvasSize.x != curCanvasSize.x || m_canvasSize.y != curCanvasSize.y  )
	{
		m_canvasSize = curCanvasSize;
		// Force update of assets and other objects on res change.
		m_layoutManager->UpdateHUDCanvasSize();
	}

	IHUDState* pCurrentState = m_stateManager->GetActiveState();
	pCurrentState->Update( frameTime );
	m_pSilhouettes->Update(frameTime);

	// Update and draw before hud
	SHUDEvent event;
	event.eventType = eHUDEvent_OnUpdate;
	event.eventFloatData = frameTime;
	CHUDEventDispatcher::CallEvent( event );

	if( ShouldShowHUD() )
	{
		if (m_pImpl)
		{
			m_pImpl->Update(frameTime);// draws as well.
		}

		m_p2DRenderUtils->PreRender();

		pCurrentState->Draw();

#if ENABLE_HUD_EXTRA_DEBUG
		m_p2DRenderUtils->RenderTest(frameTime);
#endif

		m_p2DRenderUtils->PostRender();
	}

	// Draw after hud
	event.eventType = eHUDEvent_OnPostHUDDraw;
	CHUDEventDispatcher::CallEvent( event );

#if ENABLE_HUD_EXTRA_DEBUG
	if( g_pGame->GetHUD()->GetCVars()->hud_debugActiveStates )
	{
		m_stateManager->PrintStateObjectInfo( m_stateManager->GetActiveState() );
	}
#endif

	m_pTweakMenuController->Update(frameTime);
	g_pGame->GetWarnings()->Update(frameTime);
}

void CHUD::OnAction(const ActionId& action, int activationMode, float value)
{
	if (m_pTweakMenuController)
	{
		const CGameActions &rGameActions = g_pGame->Actions();
		bool bTweakAction = true;

		// Just forward to the HUD Tweak menu
		ETweakAction eTweakAction = eTA_Exit;
		if (action == rGameActions.tweakenter)
			eTweakAction = eTA_Enter;
		else if (action == rGameActions.tweakup)
			eTweakAction = eTA_Up;
		else if (action == rGameActions.tweakdown)
			eTweakAction = eTA_Down;
		else if (action == rGameActions.tweakleft)
			eTweakAction = eTA_Left;
		else if (action == rGameActions.tweakright)
			eTweakAction = eTA_Right;
		else if (action == rGameActions.tweakincrement)
			eTweakAction = eTA_Inc;
		else if (action == rGameActions.tweakdecrement)
			eTweakAction = eTA_Dec;
		else
			bTweakAction = false;

		if (bTweakAction)
			m_pTweakMenuController->OnAction(eTweakAction, activationMode, value);
	}
}


bool CHUD::ShouldShowHUD()
{
	return ( !GetISystem()->IsDedicated() && !(m_pHUDCVars->hud_hide) );
}



const wchar_t * CHUD::LocalizeStringW( const char *text, const char *arg1, const char *arg2 ) const
{
	if(!text)
		return L"";

	wstring localizedString, param1, param2;
	ILocalizationManager* pLocMgr = gEnv->pSystem->GetLocalizationManager();

	if(text[0]=='@')
		pLocMgr->LocalizeString(text, localizedString);
	else
		StrToWstr(text,localizedString);

	if(arg1)
	{
		if(arg1[0]=='@')
			pLocMgr->LocalizeString(arg1, param1);
		else
			StrToWstr(arg1,param1);
	}

	if(arg2)
	{
		if(arg2[0]=='@')
			pLocMgr->LocalizeString(arg2, param2);
		else
			StrToWstr(arg2,param2);
	}

	CRY_FIXME(16, 02, 2010, "This is horrible, should take a 'result' parameter");
	static wstring formatted;
	formatted.resize(0);
	gEnv->pSystem->GetLocalizationManager()->FormatStringMessage(formatted, localizedString, param1.empty()?0:param1.c_str(), param2.empty()?0:param2.c_str());

	return formatted.c_str();
}



const char * CHUD::LocalizeString( const char *text, const char *arg1, const char *arg2 ) const
{
	wstring wcharstr = LocalizeStringW( text, arg1, arg2 );
	static string charstr;
	WStrTostr(wcharstr, charstr);

	return charstr.c_str();
}



void CHUD::OnSaveGame( ISaveGame* pSaveGame )
{
	SHUDEvent hudEventSelectMenu;
	hudEventSelectMenu.eventType = eHUDEvent_OnGameSave;
	CallEvent(hudEventSelectMenu);
}



void CHUD::OnLoadGame( ILoadGame* pLoadGame )
{
	SHUDEvent hudEventSelectMenu;
	hudEventSelectMenu.eventType = eHUDEvent_OnGameLoad;
	CallEvent(hudEventSelectMenu);
}



void CHUD::OnMovieEvent(IMovieListener::EMovieEvent event, IAnimSequence* pSequence)
{
	if (event == IMovieListener::MOVIE_EVENT_START)
	{
		if(pSequence && pSequence->GetFlags() & IAnimSequence::NO_HUD)
		{
			ActivateState("no_hud");
		}
	}
	else if(event == IMovieListener::MOVIE_EVENT_STOP || event == IMovieListener::MOVIE_EVENT_ABORTED)
	{
		if(pSequence && pSequence->GetFlags() & IAnimSequence::NO_HUD)
		{
			ActivateDefaultState();
		}
	}
}
void CHUD::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject(this, sizeof(*this));
	pSizer->AddObject(m_pSilhouettes);	
	pSizer->AddObject(m_pImpl);
	pSizer->AddObject(m_pEventTranslator);
	pSizer->AddObject(m_objectManager);
	pSizer->AddObject(m_stateManager);
	pSizer->AddObject(m_elementManager);
	pSizer->AddObject(m_layoutManager);
	pSizer->AddObject(m_pHUDCVars);	
}
