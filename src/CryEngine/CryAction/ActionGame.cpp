#include "StdAfx.h"
#include "IGameRulesSystem.h"
#include "ActionGame.h"
#include "IGameFramework.h"
#include "Network/GameClientNub.h"
#include "Network/GameServerNub.h"
#include "Network/GameClientChannel.h"
#include "Network/GameServerChannel.h"
#include "Network/GameContext.h"
#include "Network/ServerTimer.h"
#include "Network/DeformingBreak.h" // for debug function
#include "CryAction.h"
#include "IActorSystem.h"
#include "MaterialEffects/MaterialEffectsCVars.h"
#include "ParticleParams.h"
#include "BreakablePlane.h"
#include "IPhysics.h"
#include "IFlowSystem.h"
#include "IMaterialEffects.h"
#include "IPlayerProfiles.h"
#include "ISound.h"
#include "ISoundMoodManager.h"
#include "IAISystem.h"
#include "IAgent.h"
#include "IMovementController.h"
#include "IGameSessionHandler.h"
#include "DialogSystem/DialogSystem.h"
#include "TimeOfDayScheduler.h"
#include "PersistantDebug.h"
#include "VisualLog/VisualLog.h"
#include "Network/GameStats.h"
#include "SignalTimers/SignalTimers.h"
#include <IGameTokens.h>
#include <IMovieSystem.h>
#include <IGameStatistics.h>

#include "CryAction.h"

#include "Endian.h"
#include "ICryLobby.h"
#include "ICryMatchMaking.h"

CActionGame *CActionGame::s_this=0;
int CActionGame::g_procedural_breaking = 0;
int CActionGame::g_joint_breaking = 1;
float CActionGame::g_tree_cut_reuse_dist = 0;
int CActionGame::g_no_secondary_breaking = 0;
int CActionGame::g_no_breaking_by_objects = 0;
int CActionGame::g_breakage_mem_limit = 0;
int CActionGame::g_breakage_debug = 0;
int CActionGame::s_waterMaterialId = -1;

#define MAX_ADDRESS_SIZE (256)

void CActionGame::RegisterCVars()
{
	REGISTER_CVAR2( "g_procedural_breaking", &g_procedural_breaking, 1, VF_CHEAT, "Toggles procedural mesh breaking (except explosion-breaking)" );
	REGISTER_CVAR2( "g_joint_breaking", &g_joint_breaking, 1, 0, "Toggles jointed objects breaking" );
	REGISTER_CVAR2( "g_tree_cut_reuse_dist", &g_tree_cut_reuse_dist, 0, 0, 
		"Maximum distance from a previously made cut that allows reusing" );
	REGISTER_CVAR2( "g_no_secondary_breaking", &g_no_secondary_breaking, 0, 0, 
		"Prevents secondary procedural breaks (to keep down memory usage)" );
	REGISTER_CVAR2( "g_no_breaking_by_objects", &g_no_breaking_by_objects, 0, 0, 
		"Prevents procedural breaking caused by rigid bodies" );
	REGISTER_CVAR2( "g_breakage_mem_limit", &g_breakage_mem_limit, 0, 0, 
		"Sets a budget for procedurally breakable objects (in KBs)" );
	REGISTER_CVAR2( "g_breakage_debug", &g_breakage_debug, 0, 0, 
		"Turns on debug rendering for broken objects counted against g_breakage_mem_limit" );
}

// small helper class to make local connections have a fast packet rate - during critical operations
class CAdjustLocalConnectionPacketRate
{
public:
	CAdjustLocalConnectionPacketRate( float rate )
	{
		m_old = -1;
		if (ICVar * pVar = gEnv->pConsole->GetCVar("g_localPacketRate"))
		{
			m_old = pVar->GetFVal();
			pVar->Set( rate );
		}
	}

	~CAdjustLocalConnectionPacketRate()
	{
		if (m_old > 0)
		{
			if (ICVar * pVar = gEnv->pConsole->GetCVar("g_localPacketRate"))
			{
				pVar->Set( m_old );
			}
		}
	}

private:
	float m_old;
};

CActionGame::CActionGame( CScriptRMI * pScriptRMI )
: m_pEntitySystem(gEnv->pEntitySystem)
, m_pNetwork(gEnv->pNetwork)
, m_pClientNub(0)
, m_pServerNub(0)
, m_pGameClientNub(0)
, m_pGameServerNub(0)
, m_pGameContext(0)
, m_pNetContext(0)
, m_pGameTokenSystem(0)
, m_pPhysicalWorld(0)
, m_pGameStats(0)
, m_pEntHits0(0)
,	m_pCHSlotPool(0)
, m_hostMigrationClientReconnectState(eRS_Reconnect)
{
	CRY_ASSERT(!s_this);
	s_this = this;

	m_pNetwork->AddEventListener(this, "CActionGame");

	m_pGameContext = new CGameContext(CCryAction::GetCryAction(), pScriptRMI, this);
	m_pGameTokenSystem = CCryAction::GetCryAction()->GetIGameTokenSystem();
	m_pMaterialEffects = CCryAction::GetCryAction()->GetIMaterialEffects();

	CCryAction::GetCryAction()->GetIGameSessionHandler()->StartSession();
}

CActionGame::~CActionGame()
{	
	m_pNetwork->RemoveEventListener(this);

  if(m_pGameStats)
    m_pGameStats->EndSession();

	CCryAction::GetCryAction()->GetIGameSessionHandler()->EndSession();

	if (m_pServerNub)
	{
		m_pServerNub->DeleteNub();
		m_pServerNub = 0;
	}
	else
	{
		delete m_pGameServerNub;
	}
	m_pGameServerNub = 0;

	if (m_pClientNub)
	{
		m_pClientNub->DeleteNub();
		m_pClientNub = NULL;
	}

	if (m_pNetContext)
		m_pNetContext->DeleteContext();
	m_pNetContext = 0;
	if(m_pNetwork	)
		m_pNetwork->SyncWithGame(eNGS_Shutdown);

	EnablePhysicsEvents(false);
	if (m_pGameContext && m_pGameContext->GetFramework()->GetIGameRulesSystem() && 
			m_pGameContext->GetFramework()->GetIGameRulesSystem()->GetCurrentGameRules())
		m_pGameContext->GetFramework()->GetIGameRulesSystem()->GetCurrentGameRules()->RemoveHitListener(this);

	SAFE_DELETE(m_pGameContext);
	SAFE_DELETE(m_pGameClientNub);
	SAFE_DELETE(m_pGameServerNub);

  ReleaseGameStats();

	CCryAction *pCryAction = CCryAction::GetCryAction();
	if(pCryAction)
		pCryAction->GetIGameRulesSystem()->DestroyGameRules();

	UnloadLevel();

	if(!gEnv->pSystem->IsDedicated())
	{
		gEnv->bMultiplayer = false; //reset until re-initialization
		const char* szDefaultGameRules = gEnv->pConsole->GetCVar("sv_gamerulesdefault")->GetString();
		gEnv->pConsole->GetCVar("sv_gamerules")->Set(szDefaultGameRules);
		gEnv->pConsole->GetCVar("sv_requireinputdevice")->Set("dontcare");



	}






	s_this = 0;
	gEnv->bServer = false;
	gEnv->bClient = false;
	gEnv->bMultiplayer = false;
}

//////////////////////////////////////////////////////////////////////////
void CActionGame::UnloadLevel()
{
	//AM: Flush render thread (Flush is not exposed - using EndFrame())
	//We are about to delete resources that could be in use
	gEnv->pRenderer->EndFrame();

	UnloadPhysicsData();

	CCryAction::GetCryAction()->GetILevelSystem()->UnLoadLevel();
}

//////////////////////////////////////////////////////////////////////////
void CActionGame::UnloadPhysicsData()
{
	// clean up broken object refs
	FixBrokenObjects();

	delete[] m_pCHSlotPool; m_pCHSlotPool=m_pFreeCHSlot0 = 0;

	SEntityHits *phits,*phitsPool;
	int i;
	for(i=0,phits=m_pEntHits0,phitsPool=0; phits; phits=phits->pnext,i++) if ((i & 31)==0) {
		if (phitsPool) delete[] phitsPool; phitsPool=phits;
	}
	m_vegStatus.clear();


	std::map< EntityId, SVegCollisionStatus* >::iterator it = m_treeBreakStatus.begin();
	while (it != m_treeBreakStatus.end())
	{
		SVegCollisionStatus *cur = (SVegCollisionStatus *)it->second;
		if (cur)
			delete cur;
		it++;
	}
	m_treeBreakStatus.clear();
	it = m_treeStatus.begin();
	while (it != m_treeStatus.end())
	{
		SVegCollisionStatus *cur = (SVegCollisionStatus *)it->second;
		if (cur)
			delete cur;
		it++;
	}
	m_treeStatus.clear();
}

ILINE bool CActionGame::AllowProceduralBreaking(uint8 proceduralBreakType)
{
	// bit 1 : test state
	// bit 2 : and (true), or (false)
	// bit 3 : current state
	// bit 4 : skip flag
	static const uint16 truth = 0xf0b2;
#define UPDATE_VAL(skip, isand, test) val = ((truth >> (((skip)<<3) | ((val)<<2) | ((isand)<<1) | (test)))&1)

	uint8 val = 0;
	// single player defaults to on, multiplayer to 
	UPDATE_VAL(0, 0, (m_proceduralBreakFlags&ePBF_DefaultAllow) != 0);
	UPDATE_VAL(0, 0, proceduralBreakType==ePBT_Glass);
	UPDATE_VAL((m_proceduralBreakFlags&ePBF_ObeyCVar) == 0, 1, g_procedural_breaking!=0);
	return val == 1;
}

bool CActionGame::Init( const SGameStartParams * pGameStartParams )
{
	if (!pGameStartParams)
		return false;

	// initialize client server infrastructure

	CAdjustLocalConnectionPacketRate adjustLocalPacketRate(50.0f);

	uint32 ctxFlags = 0;
	if ( (pGameStartParams->flags & eGSF_Server) == 0 || (pGameStartParams->flags & eGSF_LocalOnly) == 0 )
		ctxFlags |= INetwork::eNCCF_Multiplayer;
	if(m_pNetwork)
	{
		m_pNetContext = m_pNetwork->CreateNetContext(m_pGameContext, ctxFlags);
		m_pGameContext->Init(m_pNetContext);
	}

	// Márcio: Enabling AI in Multiplayer!
	ICVar *pAIEnabled = gEnv->pConsole->GetCVar("sv_AISystem");
	bool aiSystemEnable = ((pGameStartParams->flags & eGSF_Server) != 0) && (pAIEnabled ? pAIEnabled->GetIVal() : true);

	if (gEnv->pAISystem)
		gEnv->pAISystem->Enable(aiSystemEnable);

	CRY_ASSERT( m_pGameServerNub == NULL );
	CRY_ASSERT( m_pServerNub == NULL );

	bool ok = true;
	bool clientRequiresBlockingConnection = false;

	string connectionString = pGameStartParams->connectionString;
	if (!connectionString)
		connectionString = "";

	unsigned flags = pGameStartParams->flags;
	if (pGameStartParams->flags & eGSF_Server)
	{
		ICVar * pReqInput = gEnv->pConsole->GetCVar("sv_requireinputdevice");
		if (pReqInput)
		{
			const char * what = pReqInput->GetString();
			if (0 == strcmpi(what, "none"))
			{
				flags &= ~(eGSF_RequireKeyboardMouse | eGSF_RequireController);
			}
			else if (0 == strcmpi(what, "keyboard"))
			{
				flags |= eGSF_RequireKeyboardMouse;
				flags &= ~eGSF_RequireController;
			}
			else if (0 == strcmpi(what, "gamepad"))
			{
				flags |= eGSF_RequireController;
				flags &= ~eGSF_RequireKeyboardMouse;
			}
			else if (0 == strcmpi(what, "dontcare"))
				;
			else
			{
				GameWarning("Invalid sv_requireinputdevice %s", what);
				return false;
			}
		}
	}

	// we need to fake demo playback as not LocalOnly, otherwise things not breakable in demo recording will become breakable
	if (flags & eGSF_DemoPlayback)
		flags &= ~eGSF_LocalOnly;

	m_pGameContext->SetContextInfo( flags, pGameStartParams->port, connectionString.c_str() );

	// although demo playback doesn't allow more than one client player (the local spectator), it should still be multiplayer to be consistent
	// with the recording session (otherwise, things not physicalized in demo recording (multiplayer) will be physicalized in demo playback)
	bool isMultiplayer = !m_pGameContext->HasContextFlag(eGSF_LocalOnly) || (flags & eGSF_DemoPlayback);
	const char * configName = "singleplayer";
	if (isMultiplayer)
		configName = "multiplayer";

#ifdef SP_DEMO
	if (isMultiplayer)
		return (false);
#endif

	gEnv->pSystem->LoadConfiguration(configName);
	if (isMultiplayer)
		gEnv->pSystem->LoadConfiguration("diff_easy");	// marcio: coop uses easy settings for now

	gEnv->bClient = m_pGameContext->HasContextFlag(eGSF_Client);
	gEnv->bServer = m_pGameContext->HasContextFlag(eGSF_Server);
	gEnv->bMultiplayer = isMultiplayer;

	bool doMultithreading = isMultiplayer;// && m_pGameContext->HasContextFlag(eGSF_Server);
	if(gEnv->pNetwork)
	{
		gEnv->pNetwork->EnableMultithreading(doMultithreading);

		SNetGameInfo gameInfo;
		gameInfo.maxPlayers = pGameStartParams->maxPlayers;
		gEnv->pNetwork->SetNetGameInfo(gameInfo);

		InitImmersiveness();
	}

	// perform some basic initialization/resetting
	if (!gEnv->IsEditor())
	{
		if(!gEnv->pSystem->IsSerializingFile()) //GameSerialize will reset and reserve in the right order
			gEnv->pEntitySystem->Reset();
		gEnv->pEntitySystem->ReserveEntityId( LOCAL_PLAYER_ENTITY_ID );
	}

	m_pPhysicalWorld = gEnv->pPhysicalWorld;
	m_pFreeCHSlot0 = m_pCHSlotPool = new SEntityCollHist[32];
	int i;
	for(i=0;i<31;i++)
		m_pCHSlotPool[i].pnext = m_pCHSlotPool+i+1;
	m_pCHSlotPool[i].pnext = m_pCHSlotPool;

	m_pEntHits0 = new SEntityHits[32];
	for(i=0;i<32;i++) 
	{
		m_pEntHits0[i].pHits = &m_pEntHits0[i].hit0;
		m_pEntHits0[i].pnHits = &m_pEntHits0[i].nhits0;
		m_pEntHits0[i].nHits=0; m_pEntHits0[i].nHitsAlloc=1;
		m_pEntHits0[i].pnext = m_pEntHits0+i+1;
		m_pEntHits0[i].timeUsed=m_pEntHits0[i].lifeTime = 0;
	}
	m_pEntHits0[i-1].pnext = 0;
	s_waterMaterialId = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager()->GetSurfaceTypeByName("mat_water")->GetId();
	m_totBreakageSize = 0;

	CCryAction::GetCryAction()->AllowSave(true);
	CCryAction::GetCryAction()->AllowLoad(true);

	EnablePhysicsEvents(true);
	m_bLoading = false;

	m_nEffectCounter=0;
	m_proceduralBreakFlags = ePBF_DefaultAllow|ePBT_Glass|ePBF_ObeyCVar;

	bool hasPbSvStarted = false;

	// TODO : Server announce here
	if( m_pGameContext->HasContextFlag(eGSF_Server) )
	{
		ServerInit(pGameStartParams, &ok, &hasPbSvStarted);
	}

	CreateGameStats();//Initialize stats tracking object

	if( ok && (m_pGameContext->HasContextFlag(eGSF_Client) || m_pGameContext->HasContextFlag(eGSF_DemoRecorder)) )
	{
		ClientInit(pGameStartParams, &ok, &hasPbSvStarted, &clientRequiresBlockingConnection );
	}

	PostInit(pGameStartParams, &ok, &clientRequiresBlockingConnection);

	return ok;
}

void CActionGame::ServerInit( const SGameStartParams* pGameStartParams, bool* io_ok, bool* io_hasPbSvStarted )
{ 
	bool& ok = *io_ok; 
	bool& hasPbSvStarted = *io_hasPbSvStarted;

		CRY_ASSERT( m_pGameContext->GetServerPort() != 0 );









		m_pGameServerNub = new CGameServerNub();
		m_pGameServerNub->SetGameContext( m_pGameContext );
		m_pGameServerNub->SetMaxPlayers( pGameStartParams->maxPlayers );

		char address[256];
		if (pGameStartParams->flags & eGSF_LocalOnly)
			sprintf(address, "%s:%d", LOCAL_CONNECTION_STRING, pGameStartParams->port);
		else
		{
			ICVar* pCVar = gEnv->pConsole->GetCVar("sv_bind");
			if (pCVar && pCVar->GetString())
				sprintf(address, "%s:%d", pCVar->GetString(), pGameStartParams->port);
			else
				sprintf(address, "0.0.0.0:%d", pGameStartParams->port);
		}

		IGameQuery * pGameQuery = m_pGameContext;
		if (m_pGameContext->HasContextFlag(eGSF_NoQueries))
			pGameQuery = NULL;
		if(m_pNetwork)
			m_pServerNub = m_pNetwork->CreateNub(address, m_pGameServerNub, 0, pGameQuery);

		if (!m_pServerNub)
		{
			ok = false;
		}
}

void CActionGame::ClientInit( const SGameStartParams* pGameStartParams, bool* io_ok, bool* io_hasPbSvStarted, bool* io_requireBlockingConnection )
{
	bool& ok = *io_ok; 
	bool& hasPbSvStarted = *io_hasPbSvStarted;
	bool& clientRequiresBlockingConnection = *io_requireBlockingConnection;






		m_pGameClientNub = new CGameClientNub(CCryAction::GetCryAction());
		m_pGameClientNub->SetGameContext(m_pGameContext);

		const char * hostname;
		const char * clientname;
		if (m_pGameContext->HasContextFlag(eGSF_Server))
		{
			clientname = hostname = LOCAL_CONNECTION_STRING;
		}
		else
		{
			hostname = pGameStartParams->hostname;
			clientname = "0.0.0.0";
		}
		CRY_ASSERT(hostname);

		string addressFormatter;
    if(strchr(hostname,':')==0)
		{
			addressFormatter.Format("<session>%d,%s:%d", pGameStartParams->session, hostname, m_pGameContext->GetServerPort());
		}
    else
		{
			addressFormatter.Format("<session>%d,%s", pGameStartParams->session, hostname);
		}
		string whereFormatter;
		whereFormatter.Format("%s:0", clientname);

		ok = false;

		m_pClientNub = m_pNetwork->CreateNub(whereFormatter.c_str(), m_pGameClientNub, 0, 0);

		if (!m_pClientNub)
			return;

		if (!m_pClientNub->ConnectTo(addressFormatter.c_str(), m_pGameContext->GetConnectionString()))
			return;

		clientRequiresBlockingConnection |= m_pGameContext->HasContextFlag(eGSF_BlockingClientConnect);

		ok = true;
}

void CActionGame::PostInit( const SGameStartParams* pGameStartParams, bool* io_ok, bool* io_requireBlockingConnection )
{
	bool& ok = *io_ok; 
	bool& clientRequiresBlockingConnection = *io_requireBlockingConnection;

	if ( ok && m_pGameContext->HasContextFlag(eGSF_Server) && m_pGameContext->HasContextFlag(eGSF_Client) )
		clientRequiresBlockingConnection = true;

	if (ok && clientRequiresBlockingConnection)
	{
		ok &= BlockingConnect( &CActionGame::ConditionHaveConnection, true, "have connection" );
	}

	if ( ok && m_pGameContext->HasContextFlag(eGSF_Server) && !ChangeGameContext(pGameStartParams->pContextParams) )
	{
		ok = false;
	}

	if(ok && m_pGameStats)
		m_pGameStats->StartSession();

	if (ok && m_pGameContext->HasContextFlag(eGSF_BlockingClientConnect) && !m_pGameContext->HasContextFlag(eGSF_NoSpawnPlayer) && m_pGameContext->HasContextFlag(eGSF_Client))
	{
		ok &= BlockingConnect( &CActionGame::ConditionHavePlayer, true, "have player" );
	}

	if (ok && m_pGameContext->HasContextFlag(eGSF_BlockingMapLoad))
	{
		ok &= BlockingConnect( &CActionGame::ConditionInGame, false, "in game" );
	}

	/*
	if(ok && !gEnv->bEditor && m_pGameContext->HasContextFlag(eGSF_Server))
	{
		if (IStatsTracker *tr = CCryAction::GetCryAction()->GetIGameStatistics()->GetSessionTracker())
		{
			tr->StateValue(eSP_Map, CCryAction::GetCryAction()->GetLevelName());
		}
	}
	*/
}

void CActionGame::LogModeInformation(const bool isMultiplayer, const char* hostname) const
{
	if(gEnv->pSystem->IsEditor())
	{
		CryLogAlways("Starting in Editor mode");
	}
	else if(gEnv->pSystem->IsDedicated())
	{
		CryLogAlways("Starting in Dedicated server mode");
	}
	else if(!isMultiplayer)
	{
		CryLogAlways("Starting in Singleplayer mode");
	}
	else if(IsServer())
	{
		CryLogAlways("Starting in Server mode");
	} 
	else if(m_pGameContext->HasContextFlag(eGSF_Client))
	{
		CryLogAlways("Starting in Client mode");
		string address = hostname;
		size_t position = address.find(":");
		if(position != string::npos)
		{
			 address= address.substr(0, position);
		}
		CryLogAlways("ServerName: %s", address.c_str());
	}
}

void CActionGame::UpdateImmersiveness()
{
	bool procMP = !m_pGameContext->HasContextFlag(eGSF_LocalOnly);
	bool immMP = m_pGameContext->HasContextFlag(eGSF_ImmersiveMultiplayer);
	m_proceduralBreakFlags =
		(int)(!procMP) * ePBF_ObeyCVar +
		(int)procMP * ePBF_AllowGlass +
		(int)(immMP || !procMP) * ePBF_DefaultAllow;
	if (!m_pGameContext->HasContextFlag(eGSF_Server))
		m_proceduralBreakFlags &= ~(ePBF_DefaultAllow | ePBF_AllowGlass);
	bool isServer = m_pGameContext->HasContextFlag(eGSF_Server);
	gEnv->pPhysicalWorld->GetPhysVars()->breakImpulseScale = 1.0f * (isServer && (immMP || !procMP && g_joint_breaking));


	if (gEnv->bMultiplayer && gEnv->bServer)
	{
		if (immMP)
		{
			static ICVar* pLength = gEnv->pConsole->GetCVar("sv_timeofdaylength");
			static ICVar* pStart = gEnv->pConsole->GetCVar("sv_timeofdaystart");
			static ICVar* pTOD = gEnv->pConsole->GetCVar("sv_timeofdayenable");
			ITimeOfDay::SAdvancedInfo advancedInfo;
			advancedInfo.fAnimSpeed = 0.0f;
			if (pTOD && pTOD->GetIVal())
			{
				advancedInfo.fStartTime = 0.0f;
				advancedInfo.fEndTime = 24.0f;
				if (pLength)
				{
					float lengthInHours = pLength->GetFVal();
					if (lengthInHours > 0.01f)
					{
						lengthInHours = CLAMP(pLength->GetFVal(), 0.2f, 24.0f);
						advancedInfo.fAnimSpeed = 1.0f / lengthInHours / 150.0f;
					}
				}
			}
			gEnv->p3DEngine->GetTimeOfDay()->SetAdvancedInfo(advancedInfo);
		}
	} 

	if (immMP && !m_pGameContext->HasContextFlag(eGSF_Server))
		gEnv->p3DEngine->GetTimeOfDay()->SetTimer(CServerTimer::Get());
	else
		gEnv->p3DEngine->GetTimeOfDay()->SetTimer(gEnv->pTimer);
}

void CActionGame::InitImmersiveness()
{
	bool immMP = m_pGameContext->HasContextFlag(eGSF_ImmersiveMultiplayer);
	UpdateImmersiveness();

	if (gEnv->bMultiplayer)
	{
		gEnv->pSystem->SetConfigSpec( immMP? CONFIG_VERYHIGH_SPEC : CONFIG_LOW_SPEC, false );
		if (immMP)
		{
			if(gEnv->pConsole->GetCVar("sv_timeofdayenable")->GetIVal())
			{
				static ICVar* pStart = gEnv->pConsole->GetCVar("sv_timeofdaystart");
				static ICVar* pTOD = gEnv->pConsole->GetCVar("e_TimeOfDay");
				if(pStart && pTOD)
					pTOD->Set(pStart->GetFVal());
			}
		}
	}
}

bool CActionGame::BlockingSpawnPlayer()
{
	CAdjustLocalConnectionPacketRate adjuster(50);

	if (!m_pGameContext)
		return false;
	if (!m_pGameContext->HasContextFlag(eGSF_BlockingClientConnect) || !m_pGameContext->HasContextFlag(eGSF_NoSpawnPlayer))
		return false;
	if (!m_pGameServerNub)
		return false;
	TServerChannelMap * pChannelMap = m_pGameServerNub->GetServerChannelMap();
	if (!pChannelMap)
		return false;
	if (pChannelMap->size() != 1)
		return false;
	CGameServerChannel * pChannel = pChannelMap->begin()->second;

	m_pGameContext->AllowCallOnClientConnect();

	return BlockingConnect( &CActionGame::ConditionHavePlayer, true, "have player" );
}

bool CActionGame::ConditionHaveConnection( CGameClientChannel * pChannel )
{
	return pChannel->GetNetChannel()->IsConnectionEstablished();
}

bool CActionGame::ConditionHavePlayer( CGameClientChannel * pChannel )
{
	return pChannel->GetPlayerId() != 0;
}

bool CActionGame::ConditionInGame( CGameClientChannel * pChannel )
{
	return CCryAction::GetCryAction()->IsGameStarted() && !CCryAction::GetCryAction()->IsGamePaused();
}

bool CActionGame::BlockingConnect( BlockingConditionFunction condition, bool requireClientChannel, const char * conditionText )
{
	bool ok = false;

	ITimer * pTimer = gEnv->pTimer;
	CTimeValue startTime = pTimer->GetAsyncTime();

	while (!ok)
	{
		m_pNetwork->SyncWithGame(eNGS_FrameStart);
		m_pNetwork->SyncWithGame(eNGS_FrameEnd);
		gEnv->pTimer->UpdateOnFrameStart();
		CGameClientChannel * pChannel = NULL;
		if (requireClientChannel)
		{
			if (!m_pGameClientNub)
			{
				GameWarning("BlockingConnect: Client nub doesn't exist while waiting for condition '%s' (after %.2f seconds)", conditionText, (pTimer->GetAsyncTime() - startTime).GetSeconds());
				break;
			}
			pChannel = m_pGameClientNub->GetGameClientChannel();
			if (!pChannel && IsStale()) // '||' => '&&' (see notes below)
			{
				GameWarning("BlockingConnect: Disconnected while waiting for condition '%s' (after %.2f seconds)", conditionText, (pTimer->GetAsyncTime() - startTime).GetSeconds());
				break;
			}
		}
		// NOTE: because now we have pre-channel hand-shaking (key exchange), it is legal that
		// a GameChannel will be created a while later than a GameNub is created - Lin
		if (!requireClientChannel || pChannel)
			ok |= (this->*condition)(pChannel);
	}

	if (ok && !gEnv->IsEditor())
	{
		float numSecondsTaken = (pTimer->GetAsyncTime() - startTime).GetSeconds();

		if (numSecondsTaken > 2.0f)
		{
			GameWarning("BlockingConnect: It's taken %.2f seconds to achieve condition '%s' - either you're on slow connection, or you're doing something intensive", numSecondsTaken, conditionText);
		}
	}

	return ok;
}

bool CActionGame::ChangeGameContext( const SGameContextParams * pGameContextParams )
{
	if (!IsServer())
	{
		GameWarning( "Can't ChangeGameContext() on client" );
		CRY_ASSERT( !"Can't ChangeGameContext() on client" );
		return false;
	}

	CRY_ASSERT(pGameContextParams);

	return m_pGameContext->ChangeContext( true, pGameContextParams );
}

IActor * CActionGame::GetClientActor()
{
	if (!m_pGameClientNub)
		return NULL;

	CGameClientChannel * pGameClientChannel = m_pGameClientNub->GetGameClientChannel();
	if (!pGameClientChannel)
		return NULL;
	EntityId playerId = pGameClientChannel->GetPlayerId();
	if (!playerId)
		return NULL;
	if( m_pGameContext->GetNetContext()->IsDemoPlayback() )
		return gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetCurrentDemoSpectator();
	return CCryAction::GetCryAction()->GetIActorSystem()->GetActor( playerId );
}

bool CActionGame::ControlsEntity(EntityId id)
{
	return m_pGameContext->ControlsEntity(id);
}

bool CActionGame::Update()
{
	_smart_ptr<CActionGame> pThis(this);

	IGameRulesSystem *pgrs;
	IGameRules *pgr;
	if (m_pGameContext && (pgrs=m_pGameContext->GetFramework()->GetIGameRulesSystem()) && (pgr=pgrs->GetCurrentGameRules()))
		pgr->AddHitListener(this);
	UpdateImmersiveness();

	CServerTimer::Get()->UpdateOnFrameStart();

	if(m_pGameTokenSystem)
		m_pGameTokenSystem->DebugDraw();

	if (g_breakage_debug)
		DrawBrokenMeshes();

  if (m_pGameStats && IsServer())
    m_pGameStats->Update();

	// if the game context returns true, then it wants to abort it's mission
	// that probably means that loading the level failed... and in any case
	// it also probably means that we've been deleted.
	// only call IsStale if the game context indicates it's not yet finished
	// (returns false) - is stale will check if we're a client based system
	// and still have an active client
	return (m_pGameContext && m_pGameContext->Update()) || IsStale();
}

IGameObject *CActionGame::GetEntityGameObject(IEntity *pEntity)
{
	return static_cast<CGameObject *>(pEntity->GetProxy(ENTITY_PROXY_USER));
}

IGameObject *CActionGame::GetPhysicalEntityGameObject(IPhysicalEntity *pPhysEntity)
{
	IEntity *pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pPhysEntity);
	if (pEntity)
		return static_cast<CGameObject *>(pEntity->GetProxy(ENTITY_PROXY_USER));
	return 0;
}


bool CActionGame::IsStale()
{
	if (m_pGameClientNub && m_pClientNub)
		if (!m_pClientNub->IsConnecting() && !m_pGameClientNub->GetGameClientChannel() && !gEnv->bHostMigrating)
			return true;

	return false;
}

/////////////////////////////////////////////////////////////////////////////
// Host Migration listener
bool CActionGame::OnInitiate(SHostMigrationInfo& hostMigrationInfo)
{
	CryLog("Host Migration: CActionGame::OnInitiate() started");

	// Save any state information that needs preserving across the migration
	IActorSystem* pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();

	// Store migrating player name (used for the migrating connection handshake string)
	CGameClientChannel* pGameClientChannel = s_this->m_pGameClientNub->GetGameClientChannel();
	if (pGameClientChannel != NULL)
	{
		hostMigrationInfo.m_playerID = pGameClientChannel->GetPlayerId();
		IActor* pActor = pActorSystem->GetActor(hostMigrationInfo.m_playerID);

		if (pActor)
		{
			hostMigrationInfo.SetMigratedPlayerName(pActor->GetEntity()->GetName());
		}
		else
		{
			CryLog("Host Migration: no actor found - aborting");
			gEnv->pNetwork->HostMigration_Terminate();
		}
	}

	m_hostMigrationClientReconnectState = eRS_Reconnect;

	CryLog("Host Migration: CActionGame::OnInitiate() started");
	return true;
}

bool CActionGame::OnDisconnectClient(SHostMigrationInfo& hostMigrationInfo)
{
	enum eDisconnectState
	{
		eDS_Disconnect,
		eDS_Disconnecting,
		eDS_Disconnected
	};

	static uint32 state = eDS_Disconnect;
	bool done = false;

	switch (state)
	{
	case eDS_Disconnect:
		CryLog("Host Migration: CActionGame::OnDisconnectClient() started");
		// Do the disconnect
		s_this->m_pGameClientNub->Disconnect(eDC_UserRequested, "Host migrating");
		s_this->m_pClientNub->DeleteNub();
		s_this->m_pClientNub = NULL;

		// TODO: Don't think SyncWithGame(eNGS_Shutdown) is necessary
		//s_this->m_pNetwork->SyncWithGame(eNGS_Shutdown);
		
		++state;
		break;

	case eDS_Disconnecting:
		CryLog("Host Migration: CActionGame::OnDisconnectClient() waiting");
		// Wait for the disconnect to complete
		if (s_this->m_pGameClientNub->GetGameClientChannel() == NULL)
		{
			++state;
		}
		break;

	case eDS_Disconnected: // Intentional fall-through
		CryLog("Host Migration: CActionGame::OnDisconnectClient() finished");
	default:
		// We're done - reset for next time
		state = eDS_Disconnect;
		done = true;
		break;
	}

	return done;
}

bool CActionGame::OnDemoteToClient(SHostMigrationInfo& hostMigrationInfo)
{
	CryLog("Host Migration: CActionGame::OnDemoteToClient() started");

	if (s_this->m_pServerNub)
	{
		s_this->m_pServerNub->DeleteNub();
		s_this->m_pServerNub = NULL;
	}

	gEnv->bServer = false;

	CryLog("Host Migration: CActionGame::OnDemoteToClient() finished");
	return true;
}

bool CActionGame::OnPromoteToServer(SHostMigrationInfo& hostMigrationInfo)
{
	CryLog("Host Migration: CActionGame::OnPromoteToServer() started");

	// Create a server on this machine
	gEnv->bServer = true;

	IConsole* pConsole = gEnv->pConsole;
	ICVar* pCVar = pConsole->GetCVar("sv_maxplayers");
	int maxPlayers = pCVar->GetIVal();

	// Set the server name
	IPlayerProfileManager* pPlayerProfileManager = gEnv->pGame->GetIGameFramework()->GetIPlayerProfileManager();
	CryFixedStringT<128> serverName(s_this->m_pNetwork->GetHostName());
	serverName.append(" ");
	serverName.append(pPlayerProfileManager->GetCurrentProfile(pPlayerProfileManager->GetCurrentUser())->GetName());
	gEnv->pConsole->GetCVar("sv_servername")->Set(serverName.c_str());

	// Create a new game server nub
	s_this->m_pGameServerNub = new CGameServerNub();
	s_this->m_pGameServerNub->SetGameContext(s_this->m_pGameContext);
	s_this->m_pGameServerNub->SetMaxPlayers(maxPlayers);

	char address[MAX_ADDRESS_SIZE];
	pCVar = gEnv->pConsole->GetCVar("sv_bind");
	if (pCVar && pCVar->GetString())
	{
		sprintf(address, "%s:%d", pCVar->GetString(), s_this->m_pGameContext->GetServerPort());
	}
	else
	{
		sprintf(address, "0.0.0.0:%d", s_this->m_pGameContext->GetServerPort());
	}

	IGameQuery * pGameQuery = s_this->m_pGameContext;
	if (s_this->m_pGameContext->HasContextFlag(eGSF_NoQueries))
	{
		pGameQuery = NULL;
	}

	if (s_this->m_pNetwork)
	{
		s_this->m_pServerNub = s_this->m_pNetwork->CreateNub(address, s_this->m_pGameServerNub, 0, pGameQuery);
	}

	if (s_this->m_pServerNub == NULL)
	{
		// Failed
		CryLogAlways("Host migration error: unable to create server");
		s_this->m_pNetwork->HostMigration_Terminate();
	}

	CryLog("Host Migration: CActionGame::OnPromoteToServer() finished");
	return true;
}

bool CActionGame::OnReconnectClient(SHostMigrationInfo& hostMigrationInfo)
{
	bool done = false;

	switch (m_hostMigrationClientReconnectState)
	{
	case eRS_Reconnect:
		{
			CryLog("Host Migration: CActionGame::OnReconnectClient() started");

			// Initiate client reconnection to the migrated server
			s_this->m_pGameClientNub->SetGameContext(s_this->m_pGameContext);

			// Create a new client nub
			CryFixedStringT<64> address;
			address.Format("%s:0", (hostMigrationInfo.IsNewHost() ? LOCAL_CONNECTION_STRING : "0.0.0.0"));
			s_this->m_pClientNub = s_this->m_pNetwork->CreateNub(address.c_str(), s_this->m_pGameClientNub, NULL, NULL);

			bool succeeded = false;
			if (s_this->m_pClientNub)
			{
				// Attempt to connect our client nub to the newly created server
				address = hostMigrationInfo.m_newServer;

				CryLog("Host Migration: Will use %s as connection address",address.c_str());

				if (s_this->m_pClientNub->ConnectTo(address.c_str(), s_this->m_pGameContext->GetConnectionString()))
				{
					succeeded = true;
					m_hostMigrationClientReconnectState = eRS_Reconnecting;
				}
			}

			if (!succeeded)
			{
				m_hostMigrationClientReconnectState = eRS_Terminated;
			}
		}
		break;

	case eRS_Reconnecting:
		CryLog("Host Migration: CActionGame::OnReconnectClient() waiting");

		// Wait for the reconnect to complete
		if (s_this->m_pGameClientNub->GetGameClientChannel() != NULL)
		{
			m_hostMigrationClientReconnectState = eRS_Reconnected;
		}
		break;

	case eRS_Terminated:
		// Failed
		CryLogAlways("Host migration error: unable to reconnect client");
		s_this->m_pNetwork->HostMigration_Terminate();
		// Intentional fall-through
		
	case eRS_Reconnected: // Intentional fall-through
		CryLog("Host Migration: CActionGame::OnReconnectClient() finished");
	default:
		// We're done - reset for next time
		m_hostMigrationClientReconnectState = eRS_Reconnect;

		if (hostMigrationInfo.IsNewHost())
		{
			// Store the server channel so that CNetContext can take ownership of game entities
			hostMigrationInfo.m_pServerChannel = s_this->m_pGameServerNub->GetLocalChannel();
		}
		done = true;
		break;
	}

	return done;
}

bool CActionGame::OnFinalise(SHostMigrationInfo& hostMigrationInfo)
{
	CryLog("Host Migration: CActionGame::OnFinalise() started");

	if (!gEnv->bServer && s_this->m_pGameServerNub)
	{
		// If this is a demoted server, deferred deletion of the game
		// server nub happens here instead of OnDemoteToClient() to
		// allow channels the chance to disconnect (different thread)
		SAFE_DELETE(s_this->m_pGameServerNub);
	}

	CryLog("Host Migration: CActionGame::OnFinalise() finished");
	return true;
}

bool CActionGame::OnTerminate(SHostMigrationInfo& hostMigrationInfo)
{
	return true;
}
/////////////////////////////////////////////////////////////////////////////

void CActionGame::AddGlobalPhysicsCallback(int event, void (*proc)(const EventPhys*, void*), void *userdata)
{
	int idx = (event&(0xff<<8))!=0;
	if (event&eEPE_OnCollisionLogged || event&eEPE_OnCollisionImmediate)
		m_globalPhysicsCallbacks.collision[idx].insert(TGlobalPhysicsCallbackSet::value_type(proc, userdata));

	if (event&eEPE_OnPostStepLogged || event&eEPE_OnPostStepImmediate)
		m_globalPhysicsCallbacks.postStep[idx].insert(TGlobalPhysicsCallbackSet::value_type(proc, userdata));

	if (event&eEPE_OnStateChangeLogged || event&eEPE_OnStateChangeImmediate)
		m_globalPhysicsCallbacks.stateChange[idx].insert(TGlobalPhysicsCallbackSet::value_type(proc, userdata));

	if (event&eEPE_OnCreateEntityPartLogged || event&eEPE_OnCreateEntityPartImmediate)
		m_globalPhysicsCallbacks.createEntityPart[idx].insert(TGlobalPhysicsCallbackSet::value_type(proc, userdata));

	if (event&eEPE_OnUpdateMeshLogged || event&eEPE_OnUpdateMeshImmediate)
		m_globalPhysicsCallbacks.updateMesh[idx].insert(TGlobalPhysicsCallbackSet::value_type(proc, userdata));
}

void CActionGame::RemoveGlobalPhysicsCallback(int event, void (*proc)(const EventPhys*, void*), void *userdata)
{
	int idx = (event&(0xff<<8))!=0;
	if (event&eEPE_OnCollisionLogged || event&eEPE_OnCollisionImmediate)
		m_globalPhysicsCallbacks.collision[idx].erase(TGlobalPhysicsCallbackSet::value_type(proc, userdata));

	if (event&eEPE_OnPostStepLogged || event&eEPE_OnPostStepImmediate)
		m_globalPhysicsCallbacks.postStep[idx].erase(TGlobalPhysicsCallbackSet::value_type(proc, userdata));

	if (event&eEPE_OnStateChangeLogged || event&eEPE_OnStateChangeImmediate)
		m_globalPhysicsCallbacks.stateChange[idx].erase(TGlobalPhysicsCallbackSet::value_type(proc, userdata));

	if (event&eEPE_OnCreateEntityPartLogged || event&eEPE_OnCreateEntityPartImmediate)
		m_globalPhysicsCallbacks.createEntityPart[idx].erase(TGlobalPhysicsCallbackSet::value_type(proc, userdata));

	if (event&eEPE_OnUpdateMeshLogged || event&eEPE_OnUpdateMeshImmediate)
		m_globalPhysicsCallbacks.updateMesh[idx].erase(TGlobalPhysicsCallbackSet::value_type(proc, userdata));
}

void CActionGame::EnablePhysicsEvents(bool enable)
{
	IPhysicalWorld *pPhysicalWorld = gEnv->pPhysicalWorld;

	if(!pPhysicalWorld)
		return;

	if (enable)
	{
		
		pPhysicalWorld->AddEventClient(EventPhysBBoxOverlap::id, OnBBoxOverlap, 1);
		pPhysicalWorld->AddEventClient(EventPhysCollision::id, OnCollisionLogged, 1);
		pPhysicalWorld->AddEventClient(EventPhysPostStep::id, OnPostStepLogged, 1);
		pPhysicalWorld->AddEventClient(EventPhysStateChange::id, OnStateChangeLogged, 1);
		pPhysicalWorld->AddEventClient(EventPhysCreateEntityPart::id, OnCreatePhysicalEntityLogged, 1, 0.1f);
		pPhysicalWorld->AddEventClient(EventPhysUpdateMesh::id, OnUpdateMeshLogged, 1, 0.1f);
		pPhysicalWorld->AddEventClient(EventPhysRemoveEntityParts::id, OnRemovePhysicalEntityPartsLogged, 1, 2.0f);
		pPhysicalWorld->AddEventClient(EventPhysCollision::id, OnCollisionImmediate, 0);
		pPhysicalWorld->AddEventClient(EventPhysPostStep::id, OnPostStepImmediate, 0);
		pPhysicalWorld->AddEventClient(EventPhysStateChange::id, OnStateChangeImmediate, 0);
		pPhysicalWorld->AddEventClient(EventPhysCreateEntityPart::id, OnCreatePhysicalEntityImmediate, 0, 10.0f);
		pPhysicalWorld->AddEventClient(EventPhysUpdateMesh::id, OnUpdateMeshImmediate, 0, 10.0f);
	}
	else
	{
		pPhysicalWorld->RemoveEventClient(EventPhysBBoxOverlap::id, OnBBoxOverlap, 1);
		pPhysicalWorld->RemoveEventClient(EventPhysCollision::id, OnCollisionLogged, 1);
		pPhysicalWorld->RemoveEventClient(EventPhysPostStep::id, OnPostStepLogged, 1);
		pPhysicalWorld->RemoveEventClient(EventPhysStateChange::id, OnStateChangeLogged, 1);
		pPhysicalWorld->RemoveEventClient(EventPhysCreateEntityPart::id, OnCreatePhysicalEntityLogged, 1);
		pPhysicalWorld->RemoveEventClient(EventPhysUpdateMesh::id, OnUpdateMeshLogged, 1);
		pPhysicalWorld->RemoveEventClient(EventPhysRemoveEntityParts::id, OnRemovePhysicalEntityPartsLogged, 1);
		pPhysicalWorld->RemoveEventClient(EventPhysCollision::id, OnCollisionImmediate, 0);
		pPhysicalWorld->RemoveEventClient(EventPhysPostStep::id, OnPostStepImmediate, 0);
		pPhysicalWorld->RemoveEventClient(EventPhysStateChange::id, OnStateChangeImmediate, 0);
		pPhysicalWorld->RemoveEventClient(EventPhysCreateEntityPart::id, OnCreatePhysicalEntityImmediate, 0);
		pPhysicalWorld->RemoveEventClient(EventPhysUpdateMesh::id, OnUpdateMeshImmediate, 0);
	}
}

int CActionGame::OnBBoxOverlap(const EventPhys * pEvent)
{
	EventPhysBBoxOverlap *pOverlap = (EventPhysBBoxOverlap*)pEvent;
	IEntity *pEnt = 0;
	pEnt = GetEntity( pOverlap->iForeignData[0], pOverlap->pForeignData[0] );

	if (pOverlap->iForeignData[1]==PHYS_FOREIGN_ID_STATIC)
	{
		pe_status_rope sr;
		IRenderNode *rn = (IRenderNode *)pOverlap->pEntity[1]->GetForeignData(PHYS_FOREIGN_ID_STATIC);
		
		if (rn != NULL && eERType_Vegetation == rn->GetRenderNodeType())
		{
			bool hit_veg = false;
			int idx = 0;
			IPhysicalEntity *phys = rn->GetBranchPhys(idx);
			while (phys != 0) 
			{
				phys->GetStatus(&sr);
				
				if (sr.nCollDyn > 0)
				{
					hit_veg = true;
					break;
				}
				//CryLogAlways("colldyn: %d collstat: %d", sr.nCollDyn, sr.nCollStat);
				idx++;
				phys = rn->GetBranchPhys(idx);
			}
			if (hit_veg && pEnt)
			{
				bool play_sound = false;
				if (s_this->m_vegStatus.find(pEnt->GetId()) == s_this->m_vegStatus.end())
				{
					play_sound = true;
					s_this->m_vegStatus[pEnt->GetId()] = pEnt->GetWorldPos();
				}
				else
				{
					float distSquared = (pEnt->GetWorldPos() - s_this->m_vegStatus[pEnt->GetId()]).GetLengthSquared();
					if (distSquared > 2.0f * 2.0f)
					{
						play_sound = true;
						s_this->m_vegStatus[pEnt->GetId()] = pEnt->GetWorldPos();
					}
				}
				if (play_sound)
				{
					TMFXEffectId effectId = s_this->m_pMaterialEffects->GetEffectIdByName("vegetation", PathUtil::GetFileName(rn->GetName()));
					if (effectId != InvalidEffectId)
					{
						SMFXRunTimeEffectParams params;
						params.pos = rn->GetBBox().GetCenter();
						params.soundSemantic = eSoundSemantic_Physics_General;

						pe_status_dynamics dyn;

						if (pOverlap->pEntity[0])
						{
							pOverlap->pEntity[0]->GetStatus(&dyn);
							const float speed = min(1.0f, dyn.v.GetLengthSquared()/(10.0f * 10.0f));
							params.AddSoundParam("speed", speed);
						}
						s_this->m_pMaterialEffects->ExecuteEffect(effectId, params);
					}
				}
			}
		}
	}
	return 1;
}

int CActionGame::OnCollisionLogged( const EventPhys * pEvent )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

	for (TGlobalPhysicsCallbackSet::const_iterator it=s_this->m_globalPhysicsCallbacks.collision[0].begin();
		it != s_this->m_globalPhysicsCallbacks.collision[0].end();
		++it)
	{
		it->first(pEvent, it->second);
	}

	const EventPhysCollision* pCollision = static_cast<const EventPhysCollision*>(pEvent);
	IGameRules::SGameCollision gameCollision;
	memset(&gameCollision, 0, sizeof(IGameRules::SGameCollision));
	gameCollision.pCollision = pCollision;
	if (pCollision->iForeignData[0]==PHYS_FOREIGN_ID_ENTITY)
	{
		//gameCollision.pSrcEntity = gEnv->pEntitySystem->GetEntityFromPhysics(gameCollision.pCollision->pEntity[0]);
		gameCollision.pSrcEntity = (IEntity*)pCollision->pForeignData[0];
		gameCollision.pSrc = GetEntityGameObject(gameCollision.pSrcEntity);
	}
	if (pCollision->iForeignData[1]==PHYS_FOREIGN_ID_ENTITY)
	{
		//gameCollision.pTrgEntity = gEnv->pEntitySystem->GetEntityFromPhysics(gameCollision.pCollision->pEntity[1]);
		gameCollision.pTrgEntity = (IEntity*)pCollision->pForeignData[1];
		gameCollision.pTrg = GetEntityGameObject(gameCollision.pTrgEntity);
	}

	SGameObjectEvent event(eGFE_OnCollision, eGOEF_ToExtensions|eGOEF_ToGameObject);
	event.ptr = (void*)pCollision;
	if (gameCollision.pSrc && gameCollision.pSrc->WantsPhysicsEvent(eEPE_OnCollisionLogged))
		gameCollision.pSrc->SendEvent(event);
	if (gameCollision.pTrg && gameCollision.pTrg->WantsPhysicsEvent(eEPE_OnCollisionLogged))
		gameCollision.pTrg->SendEvent(event);

  if (gameCollision.pSrc)
  {
    IRenderNode *pNode = NULL;
    if (pCollision->iForeignData[1] == PHYS_FOREIGN_ID_ENTITY)
    {
      IEntity *pTarget = (IEntity*)pCollision->pForeignData[1];
      if (pTarget)
      {
        IEntityRenderProxy * pRenderProxy = (IEntityRenderProxy*)pTarget->GetProxy(ENTITY_PROXY_RENDER);
        if (pRenderProxy)
          pNode = pRenderProxy->GetRenderNode();
      }
    }
    else
    if (pCollision->iForeignData[1] == PHYS_FOREIGN_ID_STATIC)
      pNode = (IRenderNode*)pCollision->pForeignData[1];
    /*else
    if (pCollision->iForeignData[1] == PHYS_FOREIGN_ID_FOLIAGE)
    {
      CStatObjFoliage *pFolliage = (CStatObjFoliage*)pCollision->pForeignData[1];
      if (pFolliage)
        pNode = pFolliage->m_pVegInst;
    }*/
    if (pNode)
      gEnv->p3DEngine->SelectEntity(pNode);
  }

	IGameRules* pGameRules = s_this->m_pGameContext->GetFramework()->GetIGameRulesSystem()->GetCurrentGameRules();
	if (pGameRules)
	{
		if(!pGameRules->OnCollision(gameCollision))
			return 0;
	}
	
	OnCollisionLogged_Breakable( pEvent );
	OnCollisionLogged_MaterialFX( pEvent );
	OnCollisionLogged_NotifyAI( pEvent );

	return 1;
}

void CActionGame::OnCollisionLogged_NotifyAI( const EventPhys * pEvent )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);
	// Skip the collision handling if there is no AI system or when in multi-player.
	if (!gEnv->pAISystem || (gEnv->bMultiplayer && !gEnv->bServer)) // Márcio: Enabling AI in Multiplayer!
		return;

	const EventPhysCollision* pCEvent = (const EventPhysCollision *) pEvent;
	IEntity* pEntityCollider = GetEntity(pCEvent->iForeignData[0], pCEvent->pForeignData[0]);

	IEntity* pEntityTarget = GetEntity(pCEvent->iForeignData[1], pCEvent->pForeignData[1]);
	if (pEntityTarget || pEntityCollider)
	{
		Vec3 velImpact = pCEvent->vloc[0] - pCEvent->vloc[1];
		float velImpactLenSq = velImpact.GetLengthSquared();
		if (velImpactLenSq > sqr(3.0f) && pCEvent->mass[0] > 0.3f && pCEvent->normImpulse > 0.01f)
		{
			float approxRadius = 0.2f;
			if (pEntityCollider)
			{
				AABB bounds;
				pEntityCollider->GetWorldBounds(bounds);
				approxRadius = bounds.GetRadius();
			}

			// Check that the object is big enough to be concerned with.
			if (pCEvent->mass[0] >= 0.3f && pCEvent->normImpulse >= 0.01f)
			{
				// Classify the object type based on mass and size.
				SAICollisionObjClassification type = AICOL_LARGE;
				if (approxRadius < 0.3f && pCEvent->mass[0] < 5.0f)
					type = AICOL_SMALL;
				else if (approxRadius < 2.0f && pCEvent->mass[0] < 100.0f)
					type = AICOL_MEDIUM;
				else
					type = AICOL_LARGE;

				// Magic formula to calculate the reaction radius from the impact params.
				const float impactSpeed = velImpact.GetLength();
				const float reactionRadSize = clamp(pCEvent->mass[0] / 250.0f, 0.0f, 1.0f);
				const float speedScale = clamp(impactSpeed / 25.0f, 0.0f, 1.0f);

				const float reactionRadius = (5.0f + reactionRadSize * 30.0f) * speedScale;
				const float soundRadius = (10.0f + sqrtf(reactionRadSize) * 90.0f) * speedScale;

				EntityId colliderId = pEntityCollider ? pEntityCollider->GetId() : 0;
				EntityId targetId = pEntityTarget ? pEntityTarget->GetId() : 0;

				if (colliderId != 0)
				{
					SAIStimulus stim(AISTIM_COLLISION, type, colliderId, targetId, pCEvent->pt, ZERO, reactionRadius);
					gEnv->pAISystem->RegisterStimulus(stim);

					SAIStimulus stimSound(AISTIM_SOUND, type == AICOL_SMALL ? AISOUND_COLLISION : AISOUND_COLLISION_LOUD, colliderId, 0,
						pCEvent->pt, ZERO, soundRadius, AISTIMPROC_FILTER_LINK_WITH_PREVIOUS);
					gEnv->pAISystem->RegisterStimulus(stimSound);
				}
			}
		}
	}
}

bool CActionGame::ProcessHitpoints(const Vec3 &pt, IPhysicalEntity *pent,int partid, ISurfaceType *pMat, int iDamage)
{
	if (m_bLoading)
		return true;
	int i,imin,id;
	Vec3 ptloc;
	SEntityHits *phits;
	pe_status_pos sp;
	std::map<int,SEntityHits*>::iterator iter;
	float curtime = gEnv->pTimer->GetCurrTime();
	sp.partid = partid;
  if (!pent->GetStatus(&sp))
    return false;
	ptloc = (pt-sp.pos)*sp.q;

	id = m_pPhysicalWorld->GetPhysicalEntityId(pent)*256+partid;
	if ((iter=m_mapEntHits.find(id))!=m_mapEntHits.end())
		phits = iter->second;
	else
	{
		for(phits=m_pEntHits0; phits->timeUsed+phits->lifeTime>curtime && phits->pnext; phits=phits->pnext);
		if (phits->timeUsed+phits->lifeTime > curtime)
		{
			phits->pnext = new SEntityHits[32];
			for(i=0;i<32;i++) 
			{
				phits->pnext[i].pHits = &phits->pnext[i].hit0;
				phits->pnext[i].pnHits = &phits->pnext[i].nhits0;
				phits->pnext[i].nHits=0; phits->pnext[i].nHitsAlloc=1;
				phits->pnext[i].pnext = phits->pnext+i+1;
				phits->pnext[i].timeUsed=phits->pnext[i].lifeTime = 0;
			}
			phits->pnext[i-1].pnext = 0;
			phits = phits->pnext;
		}
		phits->nHits = 0;
		phits->hitRadius=100.0f; phits->hitpoints=1; phits->maxdmg=100; phits->nMaxHits=64; phits->lifeTime=10.0f;
		const ISurfaceType::SPhysicalParams &physParams = pMat->GetPhyscalParams();
		phits->hitRadius	= physParams.hit_radius;
		phits->hitpoints	= (int)physParams.hit_points;
		phits->maxdmg			= (int)physParams.hit_maxdmg;
		phits->lifeTime		= physParams.hit_lifetime;
		m_mapEntHits.insert(std::pair<int,SEntityHits*>(id,phits));
	}
	phits->timeUsed = curtime;

	for(i=1,imin=0; i<phits->nHits; i++) if ((phits->pHits[i]-ptloc).len2()<(phits->pHits[imin]-ptloc).len2())
		imin = i;
	if (phits->nHits==0 || (phits->pHits[imin]-ptloc).len2()>sqr(phits->hitRadius) && phits->nHits<phits->nMaxHits) 
	{
		if (phits->nHitsAlloc==phits->nHits)
		{
			Vec3 *pts = phits->pHits;
			memcpy(phits->pHits=new Vec3[phits->nHitsAlloc=phits->nHits+1], pts, phits->nHits*sizeof(Vec3));
			if (pts!=&phits->hit0) delete[] pts;
			int *ns = phits->pnHits;
			memcpy(phits->pnHits=new int[phits->nHitsAlloc], ns, phits->nHits*sizeof(int));
			if (ns!=&phits->nhits0) delete[] ns;
		}
		phits->pHits[imin=phits->nHits] = ptloc;
		phits->pnHits[phits->nHits++] = min(phits->maxdmg,iDamage);
	}	else
	{
		iDamage = min(phits->maxdmg,iDamage);
		phits->pHits[imin] = (phits->pHits[imin]*(float)phits->pnHits[imin]+ptloc*(float)iDamage)/(float)(phits->pnHits[imin]+iDamage);
		phits->pnHits[imin] += iDamage;
	}

	if (phits->pnHits[imin]>=phits->hitpoints) 
	{
		memmove(phits->pHits+imin, phits->pHits+imin+1, (phits->nHits-imin-1)*sizeof(phits->pHits[0]));
		memmove(phits->pnHits+imin, phits->pnHits+imin+1, (phits->nHits-imin-1)*sizeof(phits->pnHits[0]));
		--phits->nHits;
		phits->hitpoints = FtoI(pMat->GetPhyscalParams().hit_points_secondary);
		return true;
	}
	return false;
}


SBreakEvent &CActionGame::RegisterBreakEvent(const EventPhysCollision *pColl, float energy)
{
	if (m_bLoading)
		return m_breakEvents[m_iCurBreakEvent];
	SBreakEvent be;	memset(&be,0,sizeof(be));
	IEntity *pEntity;
	IRenderNode *pVeg;
	IStatObj *pStatObj;
	pe_status_pos sp;

	switch (be.itype = pColl->iForeignData[1]) 
	{
		case PHYS_FOREIGN_ID_ENTITY: 
			be.idEnt = (pEntity = (IEntity*)pColl->pForeignData[1])->GetId(); 
			pColl->pEntity[1]->GetStatus(&sp);
			be.pos = sp.pos;
			be.rot = sp.q;
			be.scale = pEntity->GetScale();
			break;
		case PHYS_FOREIGN_ID_STATIC: 
			be.objPos = (pVeg = (IRenderNode*)pColl->pForeignData[1])->GetPos();
			be.objCenter = pVeg->GetBBox().GetCenter();
			be.objVolume = (pStatObj=pVeg->GetEntityStatObj(0)) && pStatObj->GetPhysGeom() ? pStatObj->GetPhysGeom()->V:0;
			break;
	}

	be.pt=pColl->pt; be.n=pColl->n;
	be.vloc[0]=pColl->vloc[0]; be.vloc[1]=pColl->vloc[1];
	be.mass[0]=pColl->mass[0]; be.mass[1]=pColl->mass[1];
	be.idmat[0]=pColl->idmat[0]; be.idmat[1]=pColl->idmat[1];
	be.partid[0]=pColl->partid[0]; be.partid[1]=pColl->partid[1];
	be.penetration = pColl->penetration;
	be.energy = energy;
	be.radius = pColl->radius;
	m_breakEvents.push_back(be);
	return m_breakEvents.back();
}

void CActionGame::PerformPlaneBreak( const EventPhysCollision& epc, SBreakEvent * pRecordedEvent )
{
	class CRestoreLoadingFlag
	{
	public:
		CRestoreLoadingFlag() : m_bLoading(s_this->m_bLoading) {}
		~CRestoreLoadingFlag() { s_this->m_bLoading = m_bLoading; }

	private:
		bool m_bLoading;
	};
	CRestoreLoadingFlag restoreLoadingFlag;
	if (pRecordedEvent)
		s_this->m_bLoading = true; // fake loading here

	pe_params_part pp;
	IStatObj *pStatObj=0,*pStatObjNew,*pStatObjHost=0;
	IStatObj::SSubObject *pSubObj;
	IRenderNode *pBrush;
	IMaterial *pRenderMat;
	IPhysicalEntity *pPhysEnt;
	float r = epc.radius;
	pp.pMatMapping = 0;
	pp.flagsOR = geom_can_modify;
	Matrix34A mtx;
	SBrokenObjRec rec;
	rec.itype = epc.pEntity[1]->GetiForeignData();
	rec.mass = -1.0f;
	ISurfaceTypeManager *pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
	ISurfaceType *pMat = pSurfaceTypeManager->GetSurfaceType(epc.idmat[1]);

	IEntity* pEntitySrc = epc.pEntity[0] ? (IEntity*)epc.pEntity[0]->GetForeignData(PHYS_FOREIGN_ID_ENTITY) : 0;
	IEntity* pEntityTrg = epc.pEntity[1] ? (IEntity*)epc.pEntity[1]->GetForeignData(PHYS_FOREIGN_ID_ENTITY) : 0;

	if (rec.itype==PHYS_FOREIGN_ID_ENTITY)
	{
		if ((pStatObj=pEntityTrg->GetStatObj(ENTITY_SLOT_ACTUAL)) && 
			(pStatObj->GetFlags() & (STATIC_OBJECT_COMPOUND|STATIC_OBJECT_CLONE))==STATIC_OBJECT_COMPOUND)
		{
			SBrokenObjRec rec1;
			pe_params_part ppt;
			rec1.itype = PHYS_FOREIGN_ID_ENTITY;
			rec1.islot = -1;
			rec1.idEnt = pEntityTrg->GetId();
			(rec1.pStatObjOrg = pStatObj)->AddRef();
			pe_status_nparts status_nparts;
			for(rec1.mass=0,ppt.ipart=epc.pEntity[1]->GetStatus(&status_nparts)-1; ppt.ipart>=0; ppt.ipart--)
			{
				MARK_UNUSED ppt.partid;
				if (epc.pEntity[1]->GetParams(&ppt) != 0)
					rec1.mass += ppt.mass;
			}
			s_this->m_brokenObjs.push_back(rec1);
		}
		pStatObj = pEntityTrg->GetStatObj(epc.partid[1]);
		mtx = pEntityTrg->GetSlotWorldTM(epc.partid[1]);
		pRenderMat = ((IEntityRenderProxy*)pEntityTrg->GetProxy(ENTITY_PROXY_RENDER))->GetRenderMaterial(epc.partid[1]);        
		// FIXME, workaround
		if ((pRenderMat && !_stricmp(pRenderMat->GetName(), "default") || !pRenderMat) && pStatObj && pStatObj->GetMaterial())
			pRenderMat = pStatObj->GetMaterial();
		// ~FIXME
		pPhysEnt = pEntityTrg->GetPhysics();
		rec.idEnt = pEntityTrg->GetId();
		rec.islot = epc.partid[1];        
	}
	else if (rec.itype==PHYS_FOREIGN_ID_STATIC)
	{
		pStatObj = (pBrush = ((IRenderNode*)epc.pForeignData[1]))->GetEntityStatObj(0, 0, &mtx);
		if (pStatObj && pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND)
		{
			if (pSubObj = (pStatObjHost=pStatObj)->GetSubObject(epc.partid[1]))
			{
				pStatObj = pSubObj->pStatObj;
				if (!pSubObj->bIdentityMatrix)
					mtx = mtx*pSubObj->tm;
			} else
				pStatObj = 0;
		}
		pPhysEnt = ((IRenderNode*)epc.pForeignData[1])->GetPhysics();
		pRenderMat = pBrush->GetMaterial();
		rec.pBrush = pBrush;
		rec.islot = 0;
	}
	if (pStatObj)
	{
		if (!(pStatObj->GetFlags() & STATIC_OBJECT_GENERATED))
		{
			if (epc.pEntity[0] && epc.pEntity[0]->GetType()==PE_LIVING)
				return;
			(rec.pStatObjOrg = pStatObjHost ? pStatObjHost:pStatObj)->AddRef();
			s_this->m_brokenObjs.push_back(rec);
		}

		SBreakEvent& be = pRecordedEvent? *pRecordedEvent : s_this->RegisterBreakEvent(&epc, 0);
		IStatObj *pStatObjAux = 0;
		pStatObjNew = CBreakablePlane::ProcessImpact(epc.pt, epc.vloc[0],epc.mass[0],r, pStatObj,mtx, 
			pMat,pRenderMat,pPhysEnt ? pPhysEnt->GetType()==PE_STATIC:1, be, s_this->m_broken2dChunkIds,s_this->m_bLoading, pStatObjAux);
		s_this->m_pGameContext->OnBrokeSomething( be, true );
		if (pStatObjAux || pStatObjNew && !(pStatObjNew->GetFlags() & STATIC_OBJECT_GENERATED))
			MARK_UNUSED pp.pMatMapping;
		if (!pStatObjAux)
			if (!pStatObjNew)
				s_this->RegisterBrokenMesh(epc.pEntity[1], 0, epc.partid[1], 0);
			else if (!(pStatObjNew->GetFlags() & STATIC_OBJECT_COMPOUND))
			{
				phys_geometry *physGeom = pStatObjNew->GetPhysGeom();
				IGeometry *pGeom = physGeom ? physGeom->pGeom : NULL;
				s_this->RegisterBrokenMesh(epc.pEntity[1], pGeom, epc.partid[1], pStatObjNew);
			}
		if (rec.itype==PHYS_FOREIGN_ID_STATIC && pStatObjAux && !pStatObjHost)
		{
			pStatObjHost = gEnv->p3DEngine->CreateStatObj();
			pStatObjHost->SetSubObjectCount(1);
			pStatObjHost->SetFlags(pStatObjHost->GetFlags() | STATIC_OBJECT_CLONE|STATIC_OBJECT_COMPOUND);
			if (pStatObj)
				pStatObjHost->SetFilePath(pStatObj->GetFilePath());
			pStatObjHost->FreeIndexedMesh();
			pStatObjHost->SetMaterial(pStatObj->GetMaterial());
			goto ForceObjUpdate;
		}
		if (pStatObjNew!=pStatObj)
		{
			if (rec.itype==PHYS_FOREIGN_ID_ENTITY)
			{
				pp.partid = pEntityTrg->SetStatObj(pStatObjNew, epc.partid[1],true);
				if (pEntityTrg->GetPhysics())
					pEntityTrg->GetPhysics()->SetParams(&pp);
			}
			else if (rec.itype==PHYS_FOREIGN_ID_STATIC)
			{
				ForceObjUpdate:
				if (pStatObjHost)
				{
					if (!(pStatObjHost->GetFlags() & STATIC_OBJECT_CLONE))
						pStatObjHost = pStatObjHost->Clone(false,false,false);
					pp.pPhysGeom = pStatObjNew ? pStatObjNew->GetPhysGeom() : 0;
					if (pSubObj = pStatObjHost->GetSubObject(epc.partid[1]))
					{
						if (pSubObj->pStatObj)
							pSubObj->pStatObj->Release();
						if (pSubObj->pStatObj = pStatObjNew)
							pSubObj->pStatObj->AddRef();
						pStatObjHost->Invalidate(false);
					}
					pStatObjNew = pStatObjHost;
				}
				pBrush->SetEntityStatObj(0, pStatObjNew);
				if (!pStatObjHost)
					pBrush->Physicalize(true);
				pp.partid = epc.partid[1];
				if (pBrush->GetPhysics())
					if (pp.pPhysGeom)
						pBrush->GetPhysics()->SetParams(&pp);
					else
						pBrush->GetPhysics()->RemoveGeometry(pp.partid);
			}

			if (pEntityTrg && pStatObjNew)
			{
				if (IGameObject* pGameObject = gEnv->pGame->GetIGameFramework()->GetGameObject(pEntityTrg->GetId()))
				{
					SGameObjectEvent evt(eGFE_OnBreakable2d, eGOEF_ToExtensions);
					evt.ptr = (void *)&epc;
					evt.param = pStatObjNew;
					pGameObject->SendEvent(evt);
				}
			}
		}
		if (pStatObjAux) 
		{
			if (rec.itype==PHYS_FOREIGN_ID_ENTITY)
			{
				pEntityTrg->SetStatObj(pStatObjAux, -1 & ~ENTITY_SLOT_ACTUAL,true);
				SBrokenObjRec rec2;
				rec2.itype = PHYS_FOREIGN_ID_ENTITY;
				rec2.islot = -1;
				rec2.idEnt = pEntityTrg->GetId();
				rec2.pStatObjOrg = 0;
				s_this->m_brokenObjs.push_back(rec2);
				const_cast<EventPhysCollision&>(epc).partid[1] = 
					((pStatObj=pEntityTrg->GetStatObj(ENTITY_SLOT_ACTUAL)) && (pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND) ?
					 pStatObj->GetSubObjectCount() : pEntityTrg->GetSlotCount()) - 1;
			} else if (rec.itype==PHYS_FOREIGN_ID_STATIC)
			{
				pp.partid = pStatObjHost->GetSubObjectCount();
				pStatObjHost->SetSubObjectCount(pp.partid+1);
				if (pp.partid>0)
				{
					pSubObj = pStatObjHost->GetSubObject(pp.partid-1);
					if (pSubObj->nType!=STATIC_SUB_OBJECT_MESH)
					{
						for(--pp.partid; pp.partid>0 && (pSubObj=pStatObjHost->GetSubObject(pp.partid-1)) && pSubObj->nType!=STATIC_SUB_OBJECT_MESH; pp.partid--);
						pStatObjHost->CopySubObject(pStatObjHost->GetSubObjectCount()-1, pStatObjHost, pp.partid);
						pSubObj = pStatObjHost->GetSubObject(pp.partid);
					} else
						pSubObj = pStatObjHost->GetSubObject(pStatObjHost->GetSubObjectCount()-1);
				}	else
					pSubObj = pStatObjHost->GetSubObject(pp.partid);
				pSubObj->nType = STATIC_SUB_OBJECT_MESH;
				pSubObj->name = "broken";
				pSubObj->properties = "mass 0";
				pSubObj->bIdentityMatrix = 1;
				pSubObj->localTM.SetIdentity();
				(pSubObj->pStatObj = pStatObjAux)->AddRef();
				pStatObjHost->Invalidate(false);
				pe_geomparams gp; 
				gp.mass = 0;
				pBrush->GetPhysics()->AddGeometry(pStatObjAux->GetPhysGeom(), &gp, pp.partid);
				const_cast<EventPhysCollision&>(epc).partid[1] = pp.partid;
			}
			s_this->RegisterBrokenMesh(epc.pEntity[1], pStatObjAux->GetPhysGeom()->pGeom, epc.partid[1], pStatObjAux);
			//if (pMat->GetPhyscalParams().hit_radius<=0.0f)
			s_this->m_mapEntHits.erase(gEnv->pPhysicalWorld->GetPhysicalEntityId(epc.pEntity[1])*256+epc.partid[1]);
		}	

		ISurfaceType::SBreakable2DParams *pb2d = pMat->GetBreakable2DParams();
		if (pb2d && pb2d->crack_decal_scale && r>0)
		{
			CryEngineDecalInfo dcl;
			dcl.ownerInfo.pRenderNode = rec.itype==PHYS_FOREIGN_ID_ENTITY ? 
				((IEntityRenderProxy*)pEntityTrg->GetProxy(ENTITY_PROXY_RENDER))->GetRenderNode() : pBrush;
			dcl.ownerInfo.nRenderNodeSlotId = 0;
			dcl.ownerInfo.nRenderNodeSlotSubObjectId = epc.partid[1];
			dcl.vPos = epc.pt;
			dcl.vNormal = epc.n;
			dcl.vHitDirection = epc.vloc[0].normalized();
			dcl.fLifeTime = 1e10f; 
			dcl.fSize = r*pb2d->crack_decal_scale;
			dcl.fAngle = cry_frand()*2*gf_PI;
			dcl.bAdjustPos = false;
			dcl.bSkipOverlappingTest = true;
			strcpy(dcl.szMaterialName, pb2d->crack_decal_mtl);
			gEnv->p3DEngine->CreateDecal(dcl);
		}
	}
}

void CActionGame::OnCollisionLogged_Breakable( const EventPhys * pEvent )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

	const EventPhysCollision * pCEvent = (const EventPhysCollision *) pEvent;
	IEntity* pEntitySrc = pCEvent->pEntity[0] ? (IEntity*)pCEvent->pEntity[0]->GetForeignData(PHYS_FOREIGN_ID_ENTITY) : 0;
	IEntity* pEntityTrg = pCEvent->pEntity[1] ? (IEntity*)pCEvent->pEntity[1]->GetForeignData(PHYS_FOREIGN_ID_ENTITY) : 0;

	ISurfaceTypeManager *pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
  ISurfaceType *pMat = pSurfaceTypeManager->GetSurfaceType(pCEvent->idmat[1]), *pMat0;
	SmartScriptTable props;
	pe_params_part pp;
	pe_params_car pcar;
	pe_status_wheel sw;
	float energy,hitenergy;

	Vec3 dir = ZERO;
	if (pCEvent->vloc[0].GetLengthSquared() > 1e-6f)
		dir = pCEvent->vloc[0].GetNormalized();
	bool backface = (pCEvent->n.Dot(dir) >= 0);

	if (pMat)
	{
		if (Get()->AllowProceduralBreaking(ePBT_Glass) && pMat->GetBreakability()==1 && 
				 (pCEvent->mass[0]>10.0f || pCEvent->pEntity[0] && pCEvent->pEntity[0]->GetType()==PE_ARTICULATED || pCEvent->radius>0.1f ||
			   (pCEvent->vloc[0]*pCEvent->n<0 &&
			   (pCEvent->idmat[0]<0 || 
			   (energy=pMat->GetBreakEnergy())>0 &&
			   (hitenergy=max(fabs_tpl(pCEvent->vloc[0].x)+fabs_tpl(pCEvent->vloc[0].y)+fabs_tpl(pCEvent->vloc[0].z), 
				 pCEvent->vloc[0].len2())*pCEvent->mass[0]) >= energy && 
			   (!(pMat->GetHitpoints()) || 
				 s_this->ProcessHitpoints(pCEvent->pt,pCEvent->pEntity[1],pCEvent->partid[1],pMat,FtoI(min(1E6f,hitenergy/energy))))))))
		{
			PerformPlaneBreak( *pCEvent, NULL );
		}	
		else if ((pCEvent->iForeignData[1]==PHYS_FOREIGN_ID_ENTITY || pCEvent->iForeignData[1]==PHYS_FOREIGN_ID_STATIC) && 
					Get()->AllowProceduralBreaking(ePBT_Normal) && pMat->GetBreakability()==2 && 
					pCEvent->idmat[0]!=pCEvent->idmat[1] && pCEvent->idmat[0]!=-2 && (energy=pMat->GetBreakEnergy())>0 && 
					!(pCEvent->pEntity[0] && (pCEvent->pEntity[0]->GetType()==PE_WHEELEDVEHICLE && (sw.partid=pCEvent->partid[0])>=0 && 
						pCEvent->pEntity[0]->GetStatus(&sw) && pCEvent->pEntity[0]->GetParams(&pcar) && pcar.nWheels<=4 || 
						pCEvent->pEntity[0]->GetType()==PE_LIVING)) &&
					(hitenergy=max(fabs_tpl(pCEvent->vloc[0].x)+fabs_tpl(pCEvent->vloc[0].y)+fabs_tpl(pCEvent->vloc[0].z), 
							pCEvent->vloc[0].len2())*pCEvent->mass[0]) >= energy &&
					(!(pMat->GetHitpoints()) || 
					s_this->ProcessHitpoints(pCEvent->pt,pCEvent->pEntity[1],pCEvent->partid[1],pMat,FtoI(min(1E6f,hitenergy/energy)))))
		{
			pp.partid = pCEvent->partid[1];
			if (!(pMat0 = pSurfaceTypeManager->GetSurfaceType(pCEvent->idmat[0])) || !(pMat0->GetFlags() & SURFACE_TYPE_NO_COLLIDE))
			{
				// moved to create entity
				bool bBreak = false;
				pe_status_pos sp;
				pe_params_structural_joint psj;
				primitives::box bbox;
				Vec3 ptloc;
				int iaxis;
				sp.partid = pCEvent->partid[1];
				psj.idx=0; 
				if (pCEvent->pEntity[1]->GetStatus(&sp) && (!s_this->m_bLoading || !pCEvent->pEntity[1]->GetParams(&psj)))
				{
					if (sp.scale>10.0f)
						bBreak = false;
					else if (pCEvent->iForeignData[1]==PHYS_FOREIGN_ID_STATIC && pCEvent->pt.z-sp.pos.z<(sp.BBox[0].z+sp.BBox[1].z)*0.5f)
						bBreak = true; // for vegetation objects, always allow breaking when close enough to the ground
					else
					{ // check if the hit point is far enough from the geometry boundary; don't break otherwise					
						sp.pGeom->GetBBox(&bbox);
						ptloc = bbox.Basis*((pCEvent->pt-sp.pos)*sp.q*(sp.scale==1.0f ? 1.0f:1.0f/sp.scale)-bbox.center);
						iaxis = idxmax3((float*)&bbox.size);
						bBreak = fabs_tpl(ptloc[iaxis]) < bbox.size[iaxis]-max(bbox.size[iaxis]*0.15f,0.5f);              
					}
					IStatObj *pStatObj = 0;
					// GDC workaround: on consoles disable procedural breaking by objects
					if (g_no_breaking_by_objects && pCEvent->iForeignData[0] && pCEvent->pEntity[0] && pCEvent->pEntity[0]->GetType()!=PE_PARTICLE) 
						bBreak = false;
					if (g_no_secondary_breaking  && 
						pCEvent->iForeignData[1]==PHYS_FOREIGN_ID_ENTITY && (pStatObj=((IEntity*)pCEvent->pForeignData[1])->GetStatObj(pCEvent->partid[1])) &&
						pStatObj->GetFlags() & STATIC_OBJECT_GENERATED)
						bBreak = false;
					// filter for multiplayer modes
					if (!s_this->IsServer())
						bBreak = false; // no procedural breaking on the client here
					else if (!s_this->m_pGameContext->HasContextFlag(eGSF_LocalOnly) && !s_this->m_pGameContext->HasContextFlag(eGSF_ImmersiveMultiplayer))
						bBreak = false;
					if (bBreak)
					{
						const ISurfaceType::SPhysicalParams &params = pMat->GetPhyscalParams();
						energy = 0.5f;
						energy = params.hole_size;

						energy *= max(1.0f,sp.scale);
						int flags = s_this->m_bLoading ? 0:2;
						if (pCEvent->idmat[0]<0 && params.hole_size_explosion>0)
							energy = params.hole_size_explosion;
						else if (pCEvent->idmat[0]<0 && sp.pGeom->GetVolume()*cube(sp.scale)<0.5f || pCEvent->mass[0]>=1500.0f)
							energy = max(energy, min(energy*4,1.5f));//, flags=2; // for explosions
						if (pCEvent->mass[0]>=1500.0f)
							flags |= (geom_colltype_vehicle|geom_colltype6)<<16;
						SBreakEvent be = s_this->RegisterBreakEvent(pCEvent, energy);
						if (sp.pGeom->GetForeignData(DATA_MESHUPDATE) || !s_this->ReuseBrokenTrees(pCEvent,energy,flags))
						{
							LogDeformPhysicalEntity( "SERVER", pCEvent->pEntity[1], pCEvent->pt, -pCEvent->n, energy );
							if (s_this->m_pPhysicalWorld->DeformPhysicalEntity(pCEvent->pEntity[1],pCEvent->pt,-pCEvent->n,energy,flags))
							{
								if (g_tree_cut_reuse_dist>0 && !gEnv->bMultiplayer)
									s_this->RemoveEntFromBreakageReuse(pCEvent->pEntity[1], 0);
								s_this->m_pGameContext->OnBrokeSomething( be, false );
							}
						}
					}
				}
			}
		}
	}
}
		
void CActionGame::OnCollisionLogged_MaterialFX( const EventPhys * pEvent )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

	const EventPhysCollision * pCEvent = (const EventPhysCollision *) pEvent;

	if ((pCEvent->idmat[1] == s_waterMaterialId) && 
		(pCEvent->pEntity[1]==gEnv->pPhysicalWorld->AddGlobalArea() && gEnv->p3DEngine->GetVisAreaFromPos(pCEvent->pt)))
		return;

	Vec3 dir = ZERO;
	if (pCEvent->vloc[0].GetLengthSquared() > 1e-6f)
	{
		dir = pCEvent->vloc[0].GetNormalized();
	}
	bool backface = (pCEvent->n.Dot(dir) >= 0);

	// track contacts info for physics sounds generation
	Vec3 vrel,r;
	float velImpact,velSlide2,velRoll2;
	pe_status_dynamics sd;
	int iop,id,i;
	SEntityCollHist *pech = 0;
	std::map<int,SEntityCollHist*>::iterator iter;

	iop = inrange(pCEvent->mass[1], 0.0f,pCEvent->mass[0]);
	id = s_this->m_pPhysicalWorld->GetPhysicalEntityId(pCEvent->pEntity[iop]);
	if ((iter=s_this->m_mapECH.find(id))!=s_this->m_mapECH.end())
		pech = iter->second;
	else if (s_this->m_pFreeCHSlot0->pnext!=s_this->m_pFreeCHSlot0) 
	{
		pech = s_this->m_pFreeCHSlot0->pnext; 
		s_this->m_pFreeCHSlot0->pnext = pech->pnext;
		pech->pnext = 0;
		pech->timeRolling=pech->timeNotRolling=pech->rollTimeout=pech->slideTimeout = 0;
		pech->velImpact=pech->velSlide2=pech->velRoll2 = 0;
		pech->imatImpact[0]=pech->imatImpact[1]=pech->imatSlide[0]=pech->imatSlide[1]=pech->imatRoll[0]=pech->imatRoll[1] = 0;
		pech->mass = 0;
		s_this->m_mapECH.insert(std::pair<int,SEntityCollHist*>(id,pech));
	}

	if (pech && pCEvent->pEntity[iop]->GetStatus(&sd))
	{
		vrel = pCEvent->vloc[iop^1]-pCEvent->vloc[iop];
		r = pCEvent->pt-sd.centerOfMass;
		if (sd.w.len2()>0.01f)
			r -= sd.w*((r*sd.w)/sd.w.len2());
		velImpact = fabs_tpl(vrel*pCEvent->n);
		velSlide2 = (vrel-pCEvent->n*velImpact).len2();
		velRoll2 = (sd.w^r).len2();
		pech->mass = pCEvent->mass[iop];

		i = isneg(pech->velImpact-velImpact);
		pech->imatImpact[0] += pCEvent->idmat[iop]-pech->imatImpact[0] & -i;
		pech->imatImpact[1] += pCEvent->idmat[iop^1]-pech->imatImpact[1] & -i;
		pech->velImpact = max(pech->velImpact,velImpact);

		i = isneg(pech->velSlide2-velSlide2);
		pech->imatSlide[0] += pCEvent->idmat[iop]-pech->imatSlide[0] & -i;
		pech->imatSlide[1] += pCEvent->idmat[iop^1]-pech->imatSlide[1] & -i;
		pech->velSlide2 = max(pech->velSlide2,velSlide2);

		i = isneg(max(pech->velRoll2-velRoll2, r.len2()*sqr(0.97f)-sqr(r*pCEvent->n)));
		pech->imatRoll[0] += pCEvent->idmat[iop]-pech->imatRoll[0] & -i;
		pech->imatSlide[1] += pCEvent->idmat[iop^1]-pech->imatRoll[1] & -i;
		pech->velRoll2 += (velRoll2-pech->velRoll2)*i;
	}
	// --- Begin Material Effects Code ---
	// Relative velocity, adjusted to be between 0 and 1 for sound effect parameters.
	const int debug = CMaterialEffectsCVars::Get().mfx_Debug & 0x1;

	float adjustedRelativeVelocity = 0.0f;
	float impactVelSquared = (pCEvent->vloc[0] - pCEvent->vloc[1]).GetLengthSquared();

	// Anything faster than 15 m/s is fast enough to consider maximum speed
	adjustedRelativeVelocity = (float)min(1.0f, impactVelSquared/(15.0f * 15.0f));

	// Relative mass, also adjusted to fit into sound effect parameters.
	// 100.0 is very heavy, the top end for the mass parameter.
	float adjustedRelativeMass = (float)min(1.0f, fabsf(pCEvent->mass[0] - pCEvent->mass[1])/100.0f);
	
	const float particleImpactThresh = CMaterialEffectsCVars::Get().mfx_ParticleImpactThresh;
	float partImpThresh = particleImpactThresh;
	Vec3 vloc0Dir = pCEvent->vloc[0];
	vloc0Dir = vloc0Dir.normalize();
	float testSpeed = (pCEvent->vloc[0] * vloc0Dir.Dot(pCEvent->n)).GetLengthSquared();

	// prevent slow objects from making too many collision events by only considering the velocity towards
	//  the surface (prevents sliding creating tons of effects)
	if (impactVelSquared < (25.0f * 25.0f) && testSpeed < (partImpThresh * partImpThresh))
	{
		impactVelSquared = 0.0f;
	}
	
	//CryLogAlways("impact energy: %f relative method: %f relative velocity: %f", impactorEnergy, (pCEvent->mass[0] * ((pCEvent->vloc[0] - pCEvent->vloc[1]) * vloc0Dir.Dot(pCEvent->n)).len2()), impactVel);

	// velocity vector
	//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(pCEvent->pt, ColorB(0,0,255,255), pCEvent->pt + pCEvent->vloc[0], ColorB(0,0,255,255));
	// surface normal
	//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(pCEvent->pt, ColorB(255,0,0,255), pCEvent->pt + (pCEvent->n * 5.0f), ColorB(255,0,0,255));
	// velocity with regard to surface normal
	//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(pCEvent->pt, ColorB(0,255,0,255), pCEvent->pt + (pCEvent->vloc[0] * vloc0Dir.Dot(pCEvent->n)), ColorB(0,255,0,255));

	if (!backface && impactVelSquared > (partImpThresh * partImpThresh))
	{
		IEntity* pEntitySrc = GetEntity( pCEvent->iForeignData[0], pCEvent->pForeignData[0] );
		IEntity* pEntityTrg = GetEntity( pCEvent->iForeignData[1], pCEvent->pForeignData[1] );

		IMaterialEffects* pMaterialEffects = s_this->m_pMaterialEffects;
		TMFXEffectId effectId = InvalidEffectId;
		const int defaultSurfaceIndex = pMaterialEffects->GetDefaultSurfaceIndex();

		SMFXRunTimeEffectParams params;
		params.src = pEntitySrc ? pEntitySrc->GetId() : 0;
		params.trg = pEntityTrg ? pEntityTrg->GetId() : 0;
		params.srcSurfaceId = pCEvent->idmat[0];
		params.trgSurfaceId = pCEvent->idmat[1];
		params.soundSemantic = eSoundSemantic_Physics_Collision;

		if (pCEvent->iForeignData[0] == PHYS_FOREIGN_ID_STATIC)
		{
			params.srcRenderNode = (IRenderNode*)pCEvent->pForeignData[0];
		}
		if (pCEvent->iForeignData[1] == PHYS_FOREIGN_ID_STATIC)
		{
			params.trgRenderNode = (IRenderNode*)pCEvent->pForeignData[1];
		}
		if (pEntitySrc && pCEvent->idmat[0] == pMaterialEffects->GetDefaultCanopyIndex())
		{
			SVegCollisionStatus *test = s_this->m_treeStatus[params.src];
			if (!test)
			{
				IEntityRenderProxy *rp = (IEntityRenderProxy *)pEntitySrc->GetProxy(ENTITY_PROXY_RENDER);
				if (rp)
				{
					IRenderNode *rn = rp->GetRenderNode();
					if (rn)
					{
						effectId = pMaterialEffects->GetEffectIdByName("vegetation", "tree_impact");
						s_this->m_treeStatus[params.src] = new SVegCollisionStatus();
					}
				}
			}
		}
		
		//Prevent the same FX to be played more than once in mfx_Timeout time interval
		float fTimeOut=CMaterialEffectsCVars::Get().mfx_Timeout;
		for (int k=0;k<MAX_CACHED_EFFECTS;k++)
		{
			SMFXRunTimeEffectParams& cachedParams=s_this->m_lstCachedEffects[k];
			if (cachedParams.src==params.src && cachedParams.trg==params.trg &&
					cachedParams.srcSurfaceId==params.srcSurfaceId && cachedParams.trgSurfaceId==params.trgSurfaceId &&
					cachedParams.srcRenderNode == params.srcRenderNode && cachedParams.trgRenderNode == params.trgRenderNode)
			{
				if (GetISystem()->GetITimer()->GetCurrTime()-cachedParams.fLastTime<=fTimeOut)
					return; // didnt timeout yet
			}
		} 

		// add it overwriting the oldest one
		s_this->m_nEffectCounter=(s_this->m_nEffectCounter+1)&(MAX_CACHED_EFFECTS-1);
		SMFXRunTimeEffectParams& cachedParams=s_this->m_lstCachedEffects[s_this->m_nEffectCounter];
		cachedParams.src=params.src;
		cachedParams.trg=params.trg;
		cachedParams.srcSurfaceId=params.srcSurfaceId;
		cachedParams.trgSurfaceId=params.trgSurfaceId;
		cachedParams.soundSemantic=params.soundSemantic;
		cachedParams.srcRenderNode=params.srcRenderNode;
		cachedParams.trgRenderNode=params.trgRenderNode;
		cachedParams.fLastTime=GetISystem()->GetITimer()->GetCurrTime();
		
		if (effectId == InvalidEffectId)
		{
			const char* pSrcArchetype = (pEntitySrc && pEntitySrc->GetArchetype()) ? pEntitySrc->GetArchetype()->GetName() : 0;
			const char* pTrgArchetype = (pEntityTrg && pEntityTrg->GetArchetype()) ? pEntityTrg->GetArchetype()->GetName() : 0;
			
			if (pEntitySrc)
			{
				if (pSrcArchetype)
					effectId = pMaterialEffects->GetEffectId(pSrcArchetype, pCEvent->idmat[1]);
				if (effectId == InvalidEffectId)
					effectId = pMaterialEffects->GetEffectId(pEntitySrc->GetClass(), pCEvent->idmat[1]);
			}
			if (effectId == InvalidEffectId && pEntityTrg)
			{
				if (pTrgArchetype)
					effectId = pMaterialEffects->GetEffectId(pTrgArchetype, pCEvent->idmat[0]);
				if (effectId == InvalidEffectId)
					effectId = pMaterialEffects->GetEffectId(pEntityTrg->GetClass(), pCEvent->idmat[0]);
			}

#if !defined(_RELEASE)
			if (effectId == InvalidEffectId)
			{
				effectId = pMaterialEffects->GetEffectId(pCEvent->idmat[0], pCEvent->idmat[1]);	
				// No effect found, our world is crumbling around us, try the default material
				if (effectId == InvalidEffectId && pEntitySrc)
				{
					if (pSrcArchetype)
						effectId = pMaterialEffects->GetEffectId(pSrcArchetype, defaultSurfaceIndex);
					if (effectId == InvalidEffectId)
						effectId = pMaterialEffects->GetEffectId(pEntitySrc->GetClass(), defaultSurfaceIndex);
					if (effectId == InvalidEffectId && pEntityTrg)
					{
						if (pTrgArchetype)
							effectId = pMaterialEffects->GetEffectId(pTrgArchetype, defaultSurfaceIndex);
						if (effectId == InvalidEffectId)
							effectId = pMaterialEffects->GetEffectId(pEntityTrg->GetClass(), defaultSurfaceIndex);
						if (effectId == InvalidEffectId)
						{
							effectId = pMaterialEffects->GetEffectId(defaultSurfaceIndex, defaultSurfaceIndex);
						}
					}
				}
			}
#endif
		}
		
		if (effectId != InvalidEffectId)
		{
			//It's a bullet if it is a particle, has small mass and flies at high speed (>100m/s)
			bool isBullet = pCEvent->pEntity[0]? (pCEvent->pEntity[0]->GetType() == PE_PARTICLE && pCEvent->vloc[0].len2() > 10000.0f && pCEvent->mass[0] < 1.0f): false;

			IActor *pActor = NULL;
			if(isBullet)
			 pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(params.trg);
			params.pos = pCEvent->pt;
			
			if (isBullet && pActor && pActor->IsClient())
			{
				Vec3 proxyOffset (ZERO);
				Matrix34 tm = pActor->GetEntity()->GetWorldTM();
				tm.Invert();

				IMovementController *pMV = pActor->GetMovementController();
				if (pMV)
				{
					SMovementState state;
					pMV->GetMovementState(state);
					params.pos = state.eyePosition + (state.eyeDirection.normalize() * 1.0f);
					params.soundProxyEntityId = params.trg;
					params.soundProxyOffset = tm.TransformVector((state.eyePosition + (state.eyeDirection * 1.0f)) - state.pos);
						//Do not play FX in FP
					params.playflags = MFX_PLAY_ALL &~ MFX_PLAY_PARTICLES;
				}
			}

			// further, prevent ALL particle effects from playing if g_blood = 0
			if (pActor && CMaterialEffectsCVars::Get().g_blood == 0)
			{
					params.playflags = MFX_PLAY_ALL &~ MFX_PLAY_PARTICLES;
			}

			// Check entity links for a 'Shooter'
			// if we find one and the local player is the shooter, we don't
			// need a raycast for sound obstruction/occlusion
			IEntityLink* pEntityLink = pEntitySrc ? pEntitySrc->GetEntityLinks() : 0;
			if (pEntityLink)
			{
				EntityId clientActorId = CCryAction::GetCryAction()->GetClientActorId();
				while (pEntityLink)
				{
					//Entity link is created in CProjectile::SetParams(), do we really need to check for the name (only the id perhaps)?
					if (strcmp("Shooter", pEntityLink->name) == 0)
					{
						if (clientActorId == pEntityLink->entityId)
							params.soundNoObstruction = true;

						// in any case, we're done
						break;
					}
					pEntityLink = pEntityLink->next;
				}
			}

			params.decalPos = pCEvent->pt;
			params.normal = pCEvent->n;
			Vec3 dir0 = pCEvent->vloc[0];
			Vec3 dir1 = pCEvent->vloc[1];

			//Water, ZeroG, ... parameters

			bool inWater = false, zeroG = false;
			float waterLevel = 0.0f;
			Vec3 gravity;
			pe_params_buoyancy pb[4];
			int nBuoys=gEnv->pPhysicalWorld->CheckAreas(pCEvent->pt, gravity, pb, 4);
			for(i=0;i<nBuoys;i++) if (pb[i].iMedium==0 && (pCEvent->pt-pb[i].waterPlane.origin)*pb[i].waterPlane.n<0)
			{
				 waterLevel = pb[i].waterPlane.origin.z;
				 break;
			}
			if (gravity.GetLength() < 0.0001f)
				zeroG = true;

			Vec3 pos=params.pos;
			if (waterLevel > 0.0f)
				inWater = (gEnv->p3DEngine->GetWaterLevel(&pos)>params.pos.z);
 
			params.inWater = inWater;
			params.inZeroG = zeroG;
			params.dir[0] = dir0.normalize();
			params.dir[1] = dir1.normalize();
			params.src = pEntitySrc ? pEntitySrc->GetId() : 0;
			params.trg = pEntityTrg ? pEntityTrg->GetId() : 0;
			params.partID = pCEvent->partid[1];


			float massMin = 0.0f;
			float massMax = 500.0f;
			float paramMin = 0.0f;
			float paramMax = 1.0f/3.0f;

			// tiny - bullets
			if ((pCEvent->mass[0] <= 0.1f) && pCEvent->pEntity[0] && pCEvent->pEntity[0]->GetType()==PE_PARTICLE)
			{
				// small
				massMin = 0.0f;
				massMax = 0.1f;
				paramMin = 0.0f;
				paramMax = 1.0f;
			}
			else if (pCEvent->mass[0] < 20.0f)
			{
				// small
				massMin = 0.0f;
				massMax = 20.0f;
				paramMin = 0.0f;
				paramMax = 1.5f/3.0f;
			}
			else if (pCEvent->mass[0] < 200.0f)
			{
				// medium
				massMin = 20.0f;
				massMax = 200.0f;
				paramMin = 1.0f/3.0f;
				paramMax = 2.0f/3.0f;
			}
			else
			{
				// ultra large
				massMin = 200.0f;
				massMax = 2000.0f;
				paramMin = 2.0f/3.0f;
				paramMax = 1.0f;
			}

			float p = min(1.0f, (pCEvent->mass[0] - massMin)/(massMax - massMin));
			float finalparam = paramMin + (p * (paramMax - paramMin));

			// need to hear bullet impacts
			params.soundDistanceMult = pCEvent->mass[0] > 1.0f ? (finalparam * finalparam) + ((1.0f - finalparam) * .05f) : 1.0f;

			params.AddSoundParam("mass", finalparam);
			params.AddSoundParam("speed", adjustedRelativeVelocity);

			pMaterialEffects->ExecuteEffect(effectId, params);

			if (debug != 0)
			{
				pEntitySrc = GetEntity( pCEvent->iForeignData[0], pCEvent->pForeignData[0] );
				pEntityTrg = GetEntity( pCEvent->iForeignData[1], pCEvent->pForeignData[1] );

				ISurfaceTypeManager *pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
				CryLogAlways("[MFX] Running effect for:");
				if (pEntitySrc)
				{
					const char* pSrcName = pEntitySrc->GetName();
					const char* pSrcClass = pEntitySrc->GetClass()->GetName();
					const char* pSrcArchetype = pEntitySrc->GetArchetype() ? pEntitySrc->GetArchetype()->GetName() : "<none>";
					CryLogAlways("      : SrcClass=%s SrcName=%s Arch=%s", pSrcClass, pSrcName, pSrcArchetype);
				}
				if (pEntityTrg)
				{
					const char* pTrgName = pEntityTrg->GetName();
					const char* pTrgClass = pEntityTrg->GetClass()->GetName();
					const char* pTrgArchetype = pEntityTrg->GetArchetype() ? pEntityTrg->GetArchetype()->GetName() : "<none>";
					CryLogAlways("      : TrgClass=%s TrgName=%s Arch=%s", pTrgClass, pTrgName, pTrgArchetype);
				}
				CryLogAlways("      : Mat0=%s", pSurfaceTypeManager->GetSurfaceType(pCEvent->idmat[0])->GetName());
				CryLogAlways("      : Mat1=%s", pSurfaceTypeManager->GetSurfaceType(pCEvent->idmat[1])->GetName());
				CryLogAlways("impact-speed=%f fx-threshold=%f mass=%f speed=%f", sqrtf(impactVelSquared),partImpThresh, finalparam, adjustedRelativeVelocity);
			}
		}
		else
		{
			if (debug != 0)
			{
				pEntitySrc = GetEntity( pCEvent->iForeignData[0], pCEvent->pForeignData[0] );
				pEntityTrg = GetEntity( pCEvent->iForeignData[1], pCEvent->pForeignData[1] );

				ISurfaceTypeManager *pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
				CryLogAlways("[MFX] Couldn't find effect for any combination of:");
				if (pEntitySrc)
				{
					const char* pSrcName = pEntitySrc->GetName();
					const char* pSrcClass = pEntitySrc->GetClass()->GetName();
					const char* pSrcArchetype = pEntitySrc->GetArchetype() ? pEntitySrc->GetArchetype()->GetName() : "<none>";
					CryLogAlways("      : SrcClass=%s SrcName=%s Arch=%s", pSrcClass, pSrcName, pSrcArchetype);
				}
				if (pEntityTrg)
				{
					const char* pTrgName = pEntityTrg->GetName();
					const char* pTrgClass = pEntityTrg->GetClass()->GetName();
					const char* pTrgArchetype = pEntityTrg->GetArchetype() ? pEntityTrg->GetArchetype()->GetName() : "<none>";
					CryLogAlways("      : TrgClass=%s TrgName=%s Arch=%s", pTrgClass, pTrgName, pTrgArchetype);
				}
				CryLogAlways("      : Mat0=%s", pSurfaceTypeManager->GetSurfaceType(pCEvent->idmat[0])->GetName());
				CryLogAlways("      : Mat1=%s", pSurfaceTypeManager->GetSurfaceType(pCEvent->idmat[1])->GetName());
			}
		}
	}
	else
	{
		/*
		if (debug != 0)
		{
			IEntity* pEntitySrc = GetEntity( pCEvent->iForeignData[0], pCEvent->pForeignData[0] );
			IEntity* pEntityTrg = GetEntity( pCEvent->iForeignData[1], pCEvent->pForeignData[1] );

			ISurfaceTypeManager *pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
			CryLogAlways("discarded:");
			if (pEntitySrc)
				CryLogAlways("      : SrcClass: %s SrcName: %s", pEntitySrc->GetClass()->GetName(), pEntitySrc->GetName());
			if (pEntityTrg)
				CryLogAlways("      : TrgClass: %s TrgName: %s", pEntityTrg->GetClass()->GetName(), pEntityTrg->GetName());
			CryLogAlways("      : Mat0: %s", pSurfaceTypeManager->GetSurfaceType(pCEvent->idmat[0])->GetName());
			CryLogAlways("      : Mat1: %s", pSurfaceTypeManager->GetSurfaceType(pCEvent->idmat[1])->GetName());
			if (backface)
				CryLogAlways("backface collision");
			CryLogAlways("impact speed squared: %f", impactVelSquared);
			}
		*/
	}

	// this might be a good spot to add dedicated rolling sounds
	//if (!backface && pech)
	//{
	//	if ((pech->timeRolling > 0) && false)
	//	{
	//		IEntity* pEntitySrc = GetEntity( pCEvent->iForeignData[0], pCEvent->pForeignData[0] );
	//		IEntity* pEntityTrg = GetEntity( pCEvent->iForeignData[1], pCEvent->pForeignData[1] );

	//		ISurfaceTypeManager *pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
	//		CryLogAlways("[MFX] Running effect for:");
	//		if (pEntitySrc)
	//			CryLogAlways("      : SrcClass: %s SrcName: %s", pEntitySrc->GetClass()->GetName(), pEntitySrc->GetName());
	//		if (pEntityTrg)
	//			CryLogAlways("      : TrgClass: %s TrgName: %s", pEntityTrg->GetClass()->GetName(), pEntityTrg->GetName());
	//		CryLogAlways("      : Mat0: %s", pSurfaceTypeManager->GetSurfaceType(pCEvent->idmat[0])->GetName());
	//		CryLogAlways("      : Mat1: %s", pSurfaceTypeManager->GetSurfaceType(pCEvent->idmat[1])->GetName());
	//		CryLogAlways("rolling speed: %f, rolling time: %f", sqrtf(pech->velRoll2), pech->timeRolling);
	//	}
	//	else
	//	{
	//	}
	//}

	// --- End Material Effects Code ---
}

int CActionGame::OnPostStepLogged( const EventPhys * pEvent )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

	for (TGlobalPhysicsCallbackSet::const_iterator it=s_this->m_globalPhysicsCallbacks.postStep[0].begin();
		it != s_this->m_globalPhysicsCallbacks.postStep[0].end();
		++it)
	{
		it->first(pEvent, it->second);
	}

	const EventPhysPostStep *pPostStep = static_cast<const EventPhysPostStep *>(pEvent);
	IGameObject *pSrc=s_this->GetPhysicalEntityGameObject(pPostStep->pEntity);

	if (pSrc && pSrc->WantsPhysicsEvent(eEPE_OnPostStepLogged))
	{
		SGameObjectEvent event(eGFE_OnPostStep, eGOEF_ToExtensions|eGOEF_ToGameObject);
		event.ptr = (void *)pPostStep;
		pSrc->SendEvent(event);
	}

	OnPostStepLogged_MaterialFX( pEvent );

	return 1;
}

void CActionGame::OnPostStepLogged_MaterialFX( const EventPhys * pEvent )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

	const EventPhysPostStep * pPSEvent = (const EventPhysPostStep *) pEvent;
	const float maxSoundDist = 30.0f;
	Vec3 pos0 = CCryAction::GetCryAction()->GetISystem()->GetViewCamera().GetPosition();

	if ((pPSEvent->pos-pos0).len2()<sqr(maxSoundDist*1.4f))
	{
		int id = s_this->m_pPhysicalWorld->GetPhysicalEntityId(pPSEvent->pEntity);
		std::map<int,SEntityCollHist*>::iterator iter;
		float velImpactThresh = 1.5f;

		if ((iter=s_this->m_mapECH.find(id))!=s_this->m_mapECH.end())
		{
			SEntityCollHist *pech = iter->second;
			bool bRemove = false;
			if ((pPSEvent->pos-pos0).len2()>sqr(maxSoundDist*1.2f))
				bRemove = true;
			else
			{
				if (pech->velRoll2<0.1f)
				{
					if ((pech->timeNotRolling+=pPSEvent->dt)>0.15f)
						pech->timeRolling = 0;
				} else
				{
					pech->timeRolling += pPSEvent->dt;
					pech->timeNotRolling = 0;
				}
				if (pech->timeRolling<0.2f)
					pech->velRoll2 = 0;

				if (pech->velRoll2>0.1f)
				{
					pech->rollTimeout = 0.7f;
					//CryLog("roll %.2f",sqrt_tpl(pech->velRoll2));
					pech->velRoll2 = 0; velImpactThresh = 3.5f;
				} else if (pech->velSlide2>0.1f)
				{
					pech->slideTimeout = 0.5f;
					//CryLog("slide %.2f",sqrt_tpl(pech->velSlide2));
					pech->velSlide2 = 0;
				}
				if (pech->velImpact>velImpactThresh)
				{ 
					//CryLog("impact %.2f",pech->velImpact);
					pech->velImpact = 0;
				}
				if (inrange(pech->rollTimeout,0.0f,pPSEvent->dt))
				{ 
					pech->velRoll2 = 0; 
					//CryLog("stopped rolling"); 
				}
				if (inrange(pech->slideTimeout,0.0f,pPSEvent->dt))
				{ 
					pech->velSlide2 = 0;
					//CryLog("stopped sliding");
				}
				pech->rollTimeout -= pPSEvent->dt;
				pech->slideTimeout -= pPSEvent->dt;
			}
			if (bRemove)
			{
				s_this->m_mapECH.erase(iter);
				pech->pnext = s_this->m_pFreeCHSlot0->pnext; s_this->m_pFreeCHSlot0->pnext = pech;
			}
		}
	}
}

int CActionGame::OnStateChangeLogged( const EventPhys * pEvent )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

	for (TGlobalPhysicsCallbackSet::const_iterator it=s_this->m_globalPhysicsCallbacks.stateChange[0].begin();
		it != s_this->m_globalPhysicsCallbacks.stateChange[0].end();
		++it)
	{
		it->first(pEvent, it->second);
	}

	const EventPhysStateChange *pStateChange = static_cast<const EventPhysStateChange *>(pEvent);
	IGameObject *pSrc=s_this->GetPhysicalEntityGameObject(pStateChange->pEntity);

	if (!gEnv->bServer && pSrc && pStateChange->iSimClass[1] > 1 && pStateChange->iSimClass[0] <= 1 && Get()->m_pNetContext)
	{
		//CryLogAlways("[0] = %d, [1] = %d", pStateChange->iSimClass[0], pStateChange->iSimClass[1]);
		Get()->m_pNetContext->RequestRemoteUpdate( pSrc->GetEntityId(), eEA_Physics );
	}

	if (pSrc && pSrc->WantsPhysicsEvent(eEPE_OnStateChangeLogged))
	{
		SGameObjectEvent event(eGFE_OnStateChange, eGOEF_ToExtensions|eGOEF_ToGameObject);
		event.ptr = (void *)pStateChange;
		pSrc->SendEvent(event);
	}

	OnStateChangeLogged_MaterialFX( pEvent );

	return 1;
}

void CActionGame::OnStateChangeLogged_MaterialFX( const EventPhys * pEvent )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

	const EventPhysStateChange * pSCEvent = (const EventPhysStateChange *) pEvent;
	if (pSCEvent->iSimClass[0]+pSCEvent->iSimClass[1]*4==6)
	{
		int id = s_this->m_pPhysicalWorld->GetPhysicalEntityId(pSCEvent->pEntity);
		std::map<int,SEntityCollHist*>::iterator iter;
		if ((iter=s_this->m_mapECH.find(id))!=s_this->m_mapECH.end())
		{
			if (iter->second->velRoll2>0)
			{}// CryLog("stopped rolling"); 
			if (iter->second->velSlide2>0)
			{}// CryLog("stopped sliding"); 
			iter->second->pnext = s_this->m_pFreeCHSlot0->pnext; s_this->m_pFreeCHSlot0->pnext = iter->second;
			s_this->m_mapECH.erase(iter);
		}
	}
}

int CActionGame::OnCreatePhysicalEntityLogged( const EventPhys * pEvent )
{
	for (TGlobalPhysicsCallbackSet::const_iterator it=s_this->m_globalPhysicsCallbacks.createEntityPart[0].begin();
		it != s_this->m_globalPhysicsCallbacks.createEntityPart[0].end();
		++it)
	{
		it->first(pEvent, it->second);
	}

	const EventPhysCreateEntityPart * pCEvent = (const EventPhysCreateEntityPart *) pEvent;
	IEntity *pEntity = GetEntity(pCEvent->iForeignData, pCEvent->pForeignData);

	// need to check what's broken (tree, glass, ....)
	if (!pEntity && pCEvent->iForeignData==PHYS_FOREIGN_ID_STATIC)
	{
		IRenderNode *rn = (IRenderNode *)pCEvent->pForeignData;
		if (eERType_Vegetation == rn->GetRenderNodeType())	
		{
			// notify AISystem 
			if (gEnv->pAISystem)
			{
				IEntity* pNewEnt = (IEntity*)pCEvent->pEntNew->GetForeignData(PHYS_FOREIGN_ID_ENTITY);
				if (pNewEnt)
				{
					AABB bounds;
					pNewEnt->GetWorldBounds(bounds);
				}

				// Classify the collision type.
				SAICollisionObjClassification type = AICOL_LARGE;
				if (pCEvent->breakSize < 0.5f)
					type = AICOL_SMALL;
				else if (pCEvent->breakSize < 2.0f)
					type = AICOL_MEDIUM;
				else
					type = AICOL_LARGE;

				// Magic formula to calculate the reaction and sound radii.
				const float impulse = pCEvent->breakImpulse.GetLength();
				const float reactionRadius = pCEvent->breakSize * 2.0f;
				const float soundRadius = 10.0f + sqrtf(clamp(impulse/800.0f, 0.0f, 1.0f)) * 60.0f;

				SAIStimulus stim(AISTIM_COLLISION, type, pNewEnt ? pNewEnt->GetId() : 0, 0, rn->GetPos(), ZERO, reactionRadius);
				gEnv->pAISystem->RegisterStimulus(stim);

				SAIStimulus stimSound(AISTIM_SOUND, type == AICOL_SMALL ? AISOUND_COLLISION : AISOUND_COLLISION_LOUD,
					pNewEnt ? pNewEnt->GetId() : 0, 0, rn->GetPos(), ZERO, soundRadius, AISTIMPROC_FILTER_LINK_WITH_PREVIOUS);
				gEnv->pAISystem->RegisterStimulus(stimSound);
			}

			TMFXEffectId effectId = s_this->m_pMaterialEffects->GetEffectIdByName("vegetation", "tree_break");
			if (effectId != InvalidEffectId)
			{
				SMFXRunTimeEffectParams params;
				params.pos = rn->GetPos();
				params.dir[0] = params.dir[1] = Vec3(0,0,1);
				params.soundSemantic = eSoundSemantic_Physics_General;
				s_this->m_pMaterialEffects->ExecuteEffect(effectId, params);
			}
		}
	}
	CryComment( "CPE: %s", pEntity ? pEntity->GetName():"Vegetation/Brush Object" );

	if (pCEvent->pEntity->GetiForeignData()==PHYS_FOREIGN_ID_ENTITY && pCEvent->pEntNew->GetiForeignData()==PHYS_FOREIGN_ID_ENTITY &&
			pCEvent->pEntity!=pCEvent->pEntNew)
	{
		SBrokenEntPart bep;
		bep.idSrcEnt = ((IEntity*)pCEvent->pEntity->GetForeignData(PHYS_FOREIGN_ID_ENTITY))->GetId();
		bep.idNewEnt = ((IEntity*)pCEvent->pEntNew->GetForeignData(PHYS_FOREIGN_ID_ENTITY))->GetId();
		s_this->m_brokenEntParts.push_back(bep);
	}

	if (g_tree_cut_reuse_dist>0 && !gEnv->bMultiplayer && pCEvent->cutRadius>0)
		s_this->RegisterEntsForBreakageReuse(pCEvent->pEntity,pCEvent->partidSrc, pCEvent->pEntNew, pCEvent->cutPtLoc[0].z,pCEvent->breakSize);

	s_this->RegisterBrokenMesh(pCEvent->pEntNew, pCEvent->pMeshNew);

	return 1;
}


void CActionGame::RegisterEntsForBreakageReuse(IPhysicalEntity *pPhysEnt,int partid, IPhysicalEntity *pPhysEntNew, float h,float size)
{
	IEntity *pEntity;
	IEntitySubstitutionProxy *pSubst;
	IRenderNode *pVeg;
	pe_params_part pp; pp.partid = partid;
	if ((pEntity = (IEntity*)pPhysEnt->GetForeignData(PHYS_FOREIGN_ID_ENTITY)) && 
			(pSubst = (IEntitySubstitutionProxy*)pEntity->GetProxy(ENTITY_PROXY_SUBSTITUTION)) &&
			(pVeg = pSubst->GetSubstitute()) && pVeg->GetRenderNodeType()==eERType_Vegetation && 
			pPhysEnt->GetParams(&pp) && pp.pPhysGeom->pGeom->GetSubtractionsCount()==1)
	{
		std::map<IPhysicalEntity*,STreeBreakInst*>::iterator iter;
		STreeBreakInst *rec;
		STreePieceThunk *thunk,*thunkPrev;
		if ((iter=m_mapBrokenTreesByPhysEnt.find(pPhysEnt))==m_mapBrokenTreesByPhysEnt.end())
		{
			float rscale = 1.0f/pEntity->GetScale().x;
			rec = new STreeBreakInst;
			rec->pPhysEntSrc = pPhysEnt;
			rec->pStatObj = pVeg->GetEntityStatObj(0);
			rec->cutHeight = h*rscale;
			rec->cutSize = size*rscale;
			rec->pPhysEntNew0 = pPhysEntNew;
			rec->pNextPiece = 0;
			rec->pThis = rec;	rec->pNext = 0;
			m_mapBrokenTreesByPhysEnt.insert(std::pair<IPhysicalEntity*,STreeBreakInst*>(pPhysEnt,rec));
			std::map<IStatObj*,STreeBreakInst*>::iterator iter2;
			if ((iter2=m_mapBrokenTreesByCGF.find(rec->pStatObj))==m_mapBrokenTreesByCGF.end())
				m_mapBrokenTreesByCGF.insert(std::pair<IStatObj*,STreeBreakInst*>(rec->pStatObj,rec));
			else {
				rec->pNext = iter2->second->pNext;
				iter2->second->pNext = rec;
			}
			thunk = (STreePieceThunk*)&rec->pPhysEntNew0;
		}	else
		{
			rec = iter->second;
			thunk = new STreePieceThunk;
			thunk->pPhysEntNew = pPhysEntNew;
			thunk->pNext = 0;
			for(thunkPrev=(STreePieceThunk*)&rec->pPhysEntNew0; thunkPrev->pNext; thunkPrev=thunkPrev->pNext);
			thunkPrev->pNext = thunk;
			thunk->pParent = rec;
		}
		m_mapBrokenTreesChunks.insert(std::pair<IPhysicalEntity*,STreePieceThunk*>(thunk->pPhysEntNew,thunk));
	}
}

int CActionGame::ReuseBrokenTrees(const EventPhysCollision *pCEvent, float size, int flags)
{
	if (g_tree_cut_reuse_dist<=0 || gEnv->bMultiplayer)
		return 0;
	IRenderNode *pVeg;
	std::map<IStatObj*,STreeBreakInst*>::iterator iter;

	if (pCEvent->iForeignData[1]==PHYS_FOREIGN_ID_STATIC && 
			(pVeg=(IRenderNode*)pCEvent->pForeignData[1])->GetRenderNodeType()==eERType_Vegetation &&
			pVeg->GetPhysics() && 
			(iter=m_mapBrokenTreesByCGF.find(pVeg->GetEntityStatObj(0)))!=m_mapBrokenTreesByCGF.end())
	{
		STreeBreakInst *rec;
		IEntity *pentSrc,*pentClone;
		IPhysicalEntity *pPhysEntSrc,*pPhysEntClone;
    
    Matrix34A objMat; 
    pVeg->GetEntityStatObj(0,0,&objMat);
    float scale = objMat.GetColumn(0).len();

    float hHit=pCEvent->pt.z-pVeg->GetPos().z;

		for(rec=iter->second; rec && (fabs_tpl(rec->cutHeight*scale-hHit)>g_tree_cut_reuse_dist || 
				fabs_tpl(rec->cutSize*scale-size)>size*scale*0.1f); rec=rec->pNext);
		if (rec && (pentSrc=(IEntity*)(pPhysEntSrc=rec->pPhysEntSrc)->GetForeignData(PHYS_FOREIGN_ID_ENTITY)))
		{
			SEntitySpawnParams esp;
			SEntityPhysicalizeParams epp;
			pe_action_remove_all_parts arap;
			pe_params_part pp,pp1;
			pe_geomparams gp;
			pe_action_impulse ai;
			Matrix34A mtx;
			SBrokenEntPart bep;
			STreePieceThunk *thunk = (STreePieceThunk*)&rec->pPhysEntNew0;
			STreeBreakInst *recClone = new STreeBreakInst;
			IStatObj *pStatObj;

			recClone->pPhysEntSrc = pVeg->GetPhysics();
			recClone->pStatObj = rec->pStatObj;
			recClone->cutHeight = rec->cutHeight;
			recClone->cutSize = rec->cutSize;
			recClone->pPhysEntNew0 = 0;
			recClone->pNextPiece = 0;
			recClone->pThis = recClone;	recClone->pNext = 0;
			m_mapBrokenTreesByPhysEnt.insert(std::pair<IPhysicalEntity*,STreeBreakInst*>(recClone->pPhysEntSrc,recClone));
			recClone->pNext = rec->pNext;
			rec->pNext = recClone;

			epp.type = PE_STATIC;
			esp.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Default");
			pStatObj = pVeg->GetEntityStatObj(0,0,&mtx);
			esp.vPosition = mtx.GetTranslation();
			esp.vScale = Vec3(mtx.GetColumn(0).len());
			esp.qRotation = Quat(Matrix33(mtx)/esp.vScale.x);
			pp1.flagsAND = ~geom_can_modify;
			if (m_bLoading)
				bep.idSrcEnt = UpdateEntityIdForVegetationBreak(pVeg);

			do {
				esp.id = 0;
				esp.sName = pentSrc->GetName();
				if (m_bLoading && epp.type==PE_RIGID)
					UpdateEntityIdForBrokenPart(bep.idSrcEnt);
				pentClone = gEnv->pEntitySystem->SpawnEntity(esp, true);
				if (epp.type == PE_STATIC)
				{
					((IEntityPhysicalProxy*)pentClone->CreateProxy(ENTITY_PROXY_PHYSICS))->AssignPhysicalEntity(pPhysEntClone = pVeg->GetPhysics());
					((IEntitySubstitutionProxy*)pentClone->CreateProxy(ENTITY_PROXY_SUBSTITUTION))->SetSubstitute(pVeg);
					pVeg->SetPhysics(0);
					gEnv->p3DEngine->DeleteEntityDecals(pVeg);
					gEnv->p3DEngine->UnRegisterEntity(pVeg);
					if (!m_bLoading)
					{
						SBrokenVegPart bvp;
						bvp.pos = pVeg->GetPos();
						bvp.volume = pStatObj->GetPhysGeom()->V;
						bvp.idNewEnt = bep.idSrcEnt = pentClone->GetId();
						m_brokenVegParts.push_back(bvp);
					}
				}	else
				{
					STreePieceThunk *thunkClone = 0;
					pentClone->Physicalize(epp);
					pPhysEntClone = pentClone->GetPhysics();
					if (!recClone->pPhysEntNew0)
						thunkClone = (STreePieceThunk*)&(recClone->pPhysEntNew0 = pPhysEntClone);
					else {
						STreePieceThunk *thunkPrev = 0;
						thunkClone = new STreePieceThunk;
						thunkClone->pPhysEntNew = pPhysEntClone;
						thunkClone->pNext = 0;
						for(thunkPrev=(STreePieceThunk*)&recClone->pPhysEntNew0; thunkPrev->pNext; thunkPrev=thunkPrev->pNext);
						thunkPrev->pNext = thunkClone;
						thunkClone->pParent = recClone;
					}
					m_mapBrokenTreesChunks.insert(std::pair<IPhysicalEntity*,STreePieceThunk*>(thunkClone->pPhysEntNew,thunkClone));
					if (!m_bLoading) 
					{
						bep.idNewEnt = pentClone->GetId();
						m_brokenEntParts.push_back(bep);
					}
				}
				
				pStatObj = pentSrc->GetStatObj(0);
				pStatObj->SetFlags(pStatObj->GetFlags() & ~(STATIC_OBJECT_GENERATED|STATIC_OBJECT_CLONE));
				pentClone->SetStatObj(pStatObj,0,false);
				pPhysEntClone->Action(&arap);
				for(pp.ipart=0; pPhysEntSrc->GetParams(&pp); pp.ipart++)
				{
					gp.pos=pp.pos; gp.q=pp.q;	gp.scale=scale;
					gp.flags=pp.flagsOR & ~(flags>>16 & 0xFFFF | geom_can_modify); gp.flagsCollider=pp.flagsColliderOR;
					gp.minContactDist=pp.minContactDist;
					gp.idmatBreakable=pp.idmatBreakable;
					gp.pMatMapping=pp.pMatMapping; gp.nMats=pp.nMats;
					gp.mass=pp.mass;
					pPhysEntClone->AddGeometry(pp.pPhysGeom, &gp, pp.partid);
					pp1.ipart=pp.ipart; pPhysEntSrc->SetParams(&pp1);

					if (epp.type==PE_RIGID && pp.flagsOR & geom_colltype_ray)
					{
						Vec3 org = esp.vPosition+esp.qRotation*(gp.pos+gp.q*pp.pPhysGeom->origin), n=pCEvent->pt-org;
						ai.impulse = pCEvent->n*-pp.mass;
						if (n.len2()>0) {
							ai.impulse -= n*((n*ai.impulse)/n.len2());
							Quat q = esp.qRotation*gp.q*pp.pPhysGeom->q;
							ai.angImpulse = q*(Diag33(pp.pPhysGeom->Ibody)*(!q*(n^pCEvent->n)))*(cube(gp.scale)*pp.density/n.len2());
						}
						pPhysEntClone->Action(&ai);
					}
				}
				((IEntityPhysicalProxy*)pentClone->GetProxy(ENTITY_PROXY_PHYSICS))->PhysicalizeFoliage(0);
				epp.type = PE_RIGID;
				if (!thunk)
					break;
				pPhysEntSrc=thunk->pPhysEntNew; thunk=thunk->pNext;
			} while(pentSrc = (IEntity*)pPhysEntSrc->GetForeignData(PHYS_FOREIGN_ID_ENTITY));

			return 1;
		}
	}

	return 0;
}

int CActionGame::OnUpdateMeshLogged( const EventPhys * pEvent )
{
	for (TGlobalPhysicsCallbackSet::const_iterator it=s_this->m_globalPhysicsCallbacks.updateMesh[0].begin();
		it != s_this->m_globalPhysicsCallbacks.updateMesh[0].end();
		++it)
	{
		it->first(pEvent, it->second);
	}

	const EventPhysUpdateMesh * pepum = (const EventPhysUpdateMesh *) pEvent;

	if (pepum->iForeignData==PHYS_FOREIGN_ID_STATIC && pepum->pEntity->GetiForeignData()==PHYS_FOREIGN_ID_ENTITY)
	{
		IEntity *pEntity = (IEntity*)pepum->pEntity->GetForeignData(PHYS_FOREIGN_ID_ENTITY);
		if (!pEntity)
			return 1;

		IRenderNode *pRenderNode = (IRenderNode*)pepum->pForeignData;
		IStatObj *pStatObj = pRenderNode->GetEntityStatObj(0);
		if (pStatObj)
		{
			phys_geometry *pPhysGeom = pRenderNode->GetEntityStatObj(0)->GetPhysGeom();
			if (pPhysGeom)
			{
				SBrokenVegPart bvp;

				bvp.pos = pRenderNode->GetPos();
				bvp.volume = pPhysGeom->V;
				bvp.idNewEnt = pEntity->GetId();
				s_this->m_brokenVegParts.push_back(bvp);
			}
		}
	}

	if (g_tree_cut_reuse_dist>0 && !gEnv->bMultiplayer && pepum->pMesh && pepum->pMesh->GetSubtractionsCount()>0)
		s_this->RemoveEntFromBreakageReuse(pepum->pEntity, pepum->pMesh->GetSubtractionsCount()<=1);

	IEntity *pEntity;
	IStatObj *pStatObj;
	if (pepum->iReason==EventPhysUpdateMesh::ReasonDeform && 
			pepum->iForeignData==PHYS_FOREIGN_ID_ENTITY && (pEntity=(IEntity*)pepum->pForeignData) && 
			(pStatObj=pEntity->GetStatObj(ENTITY_SLOT_ACTUAL)) && 
			(pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND) && 
			pepum->pMeshSkel->GetForeignData())
		{
			SBrokenObjRec rec;
			rec.itype = PHYS_FOREIGN_ID_ENTITY;
			rec.islot = -1;
			rec.idEnt = pEntity->GetId();
			if (pStatObj->GetCloneSourceObject())
				pStatObj = pStatObj->GetCloneSourceObject();
			(rec.pStatObjOrg = pStatObj)->AddRef();
			pe_params_part pp;
			for(pp.ipart=0,rec.mass=0; pepum->pEntity->GetParams(&pp); pp.ipart++)
				rec.mass += pp.mass;
			s_this->m_brokenObjs.push_back(rec);
			pepum->pMeshSkel->SetForeignData(0,0);
		}

	s_this->RegisterBrokenMesh(pepum->pEntity,pepum->pMesh,pepum->partid,0,pepum->iReason==EventPhysUpdateMesh::ReasonDeform ? pepum->pMeshSkel:0);

	return 1;
}


struct CrySizerNaive : ICrySizer
{
	CrySizerNaive() : m_count(0), m_size(0) {}
	virtual void Release() {}
	virtual size_t GetTotalSize() { return m_size; }
	virtual size_t GetObjectCount() { return m_count; }
	virtual IResourceCollector &GetResourceCollector() { return *(IResourceCollector*)0; }
	virtual void Push(const char*) {}
	virtual void PushSubcomponent(const char*) {}
	virtual void Pop() {}
	virtual bool AddObject(const void* id, size_t size, int count=1) { m_size += size*count; m_count++; return true; }
	virtual void Reset(){m_size=0;}
	virtual void SetResourceCollector( IResourceCollector *pColl ){};
	size_t  m_count, m_size;
};

const char *GetStatObjName(IStatObj *pStatObj)
{
	if (!pStatObj)
		return "";
	for(IStatObj *pParent=pStatObj->GetCloneSourceObject(); pParent && pParent!=pStatObj; pParent=pParent->GetCloneSourceObject())
		pStatObj = pParent;
	const char *ptr0=pStatObj->GetFilePath(),*ptr;
	for(ptr=ptr0+strlen(ptr0); ptr>ptr0 && ptr[-1]!='/'; ptr--);
	return ptr;
}

const char *GetGeomName(const SBrokenMeshSize &bms)
{
	IStatObj *pStatObj;
	if (IEntity *pEntity = (IEntity*)bms.pent->GetForeignData(PHYS_FOREIGN_ID_ENTITY))
		pStatObj = pEntity->GetStatObj(bms.partid);
	else if (IRenderNode *pBrush = (IRenderNode*)bms.pent->GetForeignData(PHYS_FOREIGN_ID_STATIC))
		pStatObj = pBrush->GetEntityStatObj(0);
	return GetStatObjName(pStatObj);
}


void CActionGame::RegisterBrokenMesh(IPhysicalEntity *pPhysEnt, IGeometry *pPhysGeom,int partid, IStatObj *pStatObj, IGeometry *pSkel)
{
	if (g_breakage_mem_limit>0)
	{
		CrySizerNaive sizer;
		if (pPhysGeom)
		{
			pPhysGeom->GetMemoryStatistics(&sizer);
			if (CBreakablePlane *pPlane = (CBreakablePlane*)pPhysGeom->GetForeignData(1))
				pPlane->m_pGrid->GetMemoryStatistics(&sizer);
		}
		if (pStatObj || pPhysGeom && (pStatObj = (IStatObj*)pPhysGeom->GetForeignData()))
		{
			pStatObj->GetMemoryUsage(&sizer);
			IRenderMesh *pRndMesh = pStatObj->GetRenderMesh();
			if (!pRndMesh && pStatObj->GetCloneSourceObject())
				pRndMesh = pStatObj->GetCloneSourceObject()->GetRenderMesh();
			if (pRndMesh)
				pRndMesh->GetMemoryUsage(&sizer, IRenderMesh::MEM_USAGE_COMBINED);
		}
		if (pSkel)
			pSkel->GetMemoryStatistics(&sizer);
		sizer.m_size = (sizer.m_size+512)>>10;
		
		int id = gEnv->pPhysicalWorld->GetPhysicalEntityId(pPhysEnt)<<8 | partid & 255;
		std::map<int,SBrokenMeshSize>::iterator iter;
		IStatObj *pStatObjNamer = pStatObj;
		if (IRenderNode *pBrush = (IRenderNode*)pPhysEnt->GetForeignData(PHYS_FOREIGN_ID_STATIC))
			pStatObjNamer = pBrush->GetEntityStatObj(0);
		if ((iter=m_mapBrokenMeshes.find(id))!=m_mapBrokenMeshes.end()) 
		{
			m_totBreakageSize -= iter->second.size;
			CryLog("Updated breakable mesh %s: %dKb->%dKb, new total size %dKb", GetStatObjName(pStatObjNamer), iter->second.size,sizer.m_size,m_totBreakageSize+sizer.m_size);
			m_mapBrokenMeshes.erase(iter);
		} else
			CryLog("New breakable mesh %s: %dKb, new total size %dKb", GetStatObjName(pStatObjNamer), sizer.m_size,m_totBreakageSize+sizer.m_size);
		if (!pPhysGeom)
			return;
		m_mapBrokenMeshes.insert(std::pair<int,SBrokenMeshSize>(id,SBrokenMeshSize(pPhysEnt,sizer.m_size,partid)));
		m_totBreakageSize += sizer.m_size;
		
		Vec3 pos0 = gEnv->pSystem->GetViewCamera().GetPosition();
		int idRndFrame = gEnv->pRenderer->GetFrameID();
		while (m_totBreakageSize > g_breakage_mem_limit)
		{
			IPhysicalEntity *pent;
			int ifd; void *pfd;
			float maxDist[2] = {0,0};
			EntityId maxDistId[2];
			IRenderNode *pRndNode;
			IEntity *pEntity;
			IStatObj::SSubObject *pSubObj;

			for(iter=m_mapBrokenMeshes.begin(); iter!=m_mapBrokenMeshes.end(); iter++)
				if ((pent = gEnv->pPhysicalWorld->GetPhysicalEntityById(iter->first>>8)) && pent==iter->second.pent &&
						((ifd=pent->GetiForeignData())==PHYS_FOREIGN_ID_ENTITY || ifd==PHYS_FOREIGN_ID_STATIC) && (pfd=pent->GetForeignData(ifd)))
				{
					pe_params_part pp; pp.partid=iter->second.partid;
					if (pent==pPhysEnt && iter->second.partid==partid || pent->GetParams(&pp) && pp.pPhysGeom->pGeom->GetForeignData(DATA_MESHUPDATE))
						continue;
					IEntityRenderProxy *pRndProxy;
					if (ifd==PHYS_FOREIGN_ID_STATIC)
						pRndNode = (IRenderNode*)pfd;
					else if (pRndProxy=(IEntityRenderProxy*)((IEntity*)pfd)->GetProxy(ENTITY_PROXY_RENDER))
						pRndNode = pRndProxy->GetRenderNode();
					else
						continue;
					int lastDrawFrame = pRndNode->GetDrawFrame();
					int bVisible = isneg(idRndFrame-lastDrawFrame-10) || lastDrawFrame<=0 || pent==pPhysEnt;
					float dist = (pos0-pRndNode->GetBBox().GetCenter()).len2();
					if (maxDist[bVisible] < dist)
						maxDist[bVisible]=dist, maxDistId[bVisible]=iter->first;
				} else 
				{	// apparently the entity was deleted in the meantime
					CryLog("Removing obsolete breakable mesh record of %dKb", iter->second.size);
					m_totBreakageSize -= iter->second.size;
					if (pent) 
						pent->Release();
					m_mapBrokenMeshes.erase(iter);
				}
			if (maxDist[0]+maxDist[1]==0.0f)
				break;

			if (!(pent = gEnv->pPhysicalWorld->GetPhysicalEntityById((id = maxDistId[maxDist[0]>0.0f ? 0:1])>>8)))
				break;
			iter = m_mapBrokenMeshes.find(id);
			pfd = pent->GetForeignData(ifd = pent->GetiForeignData());
			pRndNode = (IRenderNode*)pfd;
			pEntity = (IEntity*)pfd;

			pe_params_part pp; pp.partid=iter->second.partid;
			if (pent->GetParams(&pp) && pp.idSkeleton>=0)
			{
				pStatObj = ifd==PHYS_FOREIGN_ID_ENTITY ? pEntity->GetStatObj(ENTITY_SLOT_ACTUAL) : pRndNode->GetEntityStatObj(0);
				IStatObj *pStatObjOrg = pStatObj->GetCloneSourceObject();
				IStatObj::SSubObject *pSubObjOrg;
				pe_params_skeleton ps;
				ps.partid = iter->second.partid;
				ps.bReset = 1;
				pent->SetParams(&ps);
				if ((pSubObj=pStatObj->GetSubObject(iter->second.partid)) && (pSubObjOrg=pStatObjOrg->GetSubObject(iter->second.partid)))
				{
					if (pSubObj->pStatObj) pSubObj->pStatObj->Release();
					(pSubObj->pStatObj = pSubObjOrg->pStatObj)->AddRef();
					new(&pp) pe_params_part; 
					pp.partid = iter->second.partid;
					pp.pPhysGeom = pSubObjOrg->pStatObj->GetPhysGeom();
					pent->SetParams(&pp);
				}
			} else
			{
				pent->RemoveGeometry(iter->second.partid);

				if (ifd==PHYS_FOREIGN_ID_ENTITY && pEntity->GetProxy(ENTITY_PROXY_SUBSTITUTION))
				{ // can't delete this entity - this will restore the vegetation object through the substitution proxy
					IEntityPhysicalProxy *pPhysProxy = (IEntityPhysicalProxy*)pEntity->GetProxy(ENTITY_PROXY_PHYSICS);
					if (pPhysProxy)
						pPhysProxy->DephysicalizeFoliage(0);
					SEntityPhysicalizeParams epp; epp.type=PE_NONE;
					pEntity->Physicalize(epp);
					for(int i=pEntity->GetSlotCount()-1; i>=0; i--)
						pEntity->FreeSlot(i); 
				}	
				else if ((pStatObj = ifd==PHYS_FOREIGN_ID_ENTITY ? pEntity->GetStatObj(ENTITY_SLOT_ACTUAL) : pRndNode->GetEntityStatObj(0)) &&
								 (pSubObj = pStatObj->GetSubObject(iter->second.partid)) && pSubObj->pStatObj)
				{
					pSubObj->pStatObj->Release();
					pSubObj->pStatObj = 0;
					pSubObj->bHidden = true;
				} else if (ifd!=PHYS_FOREIGN_ID_ENTITY)
					pRndNode->SetEntityStatObj(0,0);
				else if (pEntity->GetSlotCount()<=1)
					gEnv->pEntitySystem->RemoveEntity(pEntity->GetId());
				else
					pEntity->FreeSlot(iter->second.partid);
			}

			m_totBreakageSize -= iter->second.size;
			m_mapBrokenMeshes.erase(iter);
			CryLog("Breakable memory limit breached - removing a mesh; new total size %dKb", m_totBreakageSize);
		}
	}
}


void CActionGame::DrawBrokenMeshes()
{
	IRenderer *pRnd = gEnv->pRenderer;
	std::map<int,SBrokenMeshSize>::iterator iter;
	int sizes[16],keys[16],nTop=0,i;
	pe_status_pos sp;
	float clr[] = {1.0f,0.3f,0.2f,1.0f};
	for(iter=m_mapBrokenMeshes.begin(); iter!=m_mapBrokenMeshes.end(); iter++) 
	{
		sp.partid = iter->second.partid;
		if (!iter->second.pent->GetStatus(&sp))
			continue;
		pRnd->DrawLabelEx((sp.BBox[0]+sp.BBox[1])*0.5f+sp.pos, 1.4f,clr, true,true, "%s (%d Kb)", GetGeomName(iter->second),iter->second.size);
		for(i=0; i<nTop && sizes[i]>iter->second.size; i++);
		if (i<16) {
			nTop -= nTop>>4;
			memmove(sizes+i+1, sizes+i, (nTop-i)*sizeof(sizes[0]));
			memmove(keys+i+1, keys+i, (nTop-i)*sizeof(keys[0]));
			sizes[i]=iter->second.size; keys[i]=iter->first;
			nTop++; 
		}
	}
	pRnd->Draw2dLabel( 10.0f,60.0f, 1.3f, clr,false, "Top procedurally broken objects (total size - %d Kb):",m_totBreakageSize );
	for(i=0;i<nTop;i++)
		pRnd->Draw2dLabel( 10.0f,72.0f+i*12.0f, 1.3f, clr,false, "%d Kb %s", sizes[i],GetGeomName(m_mapBrokenMeshes.find(keys[i])->second) );
}


void CActionGame::RemoveEntFromBreakageReuse(IPhysicalEntity *pEntity, int bRemoveOnlyIfSecondary)
{
	std::map<IPhysicalEntity*,STreePieceThunk*>::iterator iter1;
	STreeBreakInst *rec=0,*rec0;
	if ((iter1=m_mapBrokenTreesChunks.find(pEntity))!=m_mapBrokenTreesChunks.end())
		rec = iter1->second->pParent;
	else if (!bRemoveOnlyIfSecondary) 
	{
		std::map<IPhysicalEntity*,STreeBreakInst*>::iterator iter;
		if ((iter=m_mapBrokenTreesByPhysEnt.find(pEntity))!=m_mapBrokenTreesByPhysEnt.end())
			rec = iter->second;
	}
	if (rec)
	{
		m_mapBrokenTreesByPhysEnt.erase(rec->pPhysEntSrc);
		m_mapBrokenTreesChunks.erase(rec->pPhysEntNew0);
		STreePieceThunk *thunk,*thunkNext;
		for(thunk=rec->pNextPiece; thunk; thunk=thunkNext)
		{
			m_mapBrokenTreesChunks.erase(thunk->pPhysEntNew);
			thunkNext=thunk->pNext; delete thunk;
		}
		std::map<IStatObj*,STreeBreakInst*>::iterator iter;
		if ((iter=m_mapBrokenTreesByCGF.find(rec->pStatObj))!=m_mapBrokenTreesByCGF.end())
		{
			if (iter->second==rec) {
				if (rec->pNext)
					iter->second = rec->pNext;
				else 
					m_mapBrokenTreesByCGF.erase(rec->pStatObj);
			}	else {
				for(rec0=iter->second; rec0->pNext!=rec; rec0=rec0->pNext);
				rec0->pNext = rec->pNext;
			}
		}
		delete rec;
	}	
}

int CActionGame::OnRemovePhysicalEntityPartsLogged( const EventPhys * pEvent )
{
	for (TGlobalPhysicsCallbackSet::const_iterator it=s_this->m_globalPhysicsCallbacks.updateMesh[1].begin();
		it != s_this->m_globalPhysicsCallbacks.updateMesh[1].end();
		++it)
	{
		it->first(pEvent, it->second);
	}

	const EventPhysRemoveEntityParts *pREvent = (EventPhysRemoveEntityParts*)pEvent;
	IEntity *pEntity;
	IStatObj *pStatObj;

	if (pREvent->iForeignData==PHYS_FOREIGN_ID_ENTITY && (pEntity=(IEntity*)pREvent->pForeignData))
	{
		int idOffs = pREvent->idOffs;
		if (pREvent->idOffs>=PARTID_LINKED)
			pEntity = pEntity->UnmapAttachedChild(idOffs);
		if (pEntity && (pStatObj=pEntity->GetStatObj(ENTITY_SLOT_ACTUAL)) && pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND)
		{
			SBrokenObjRec rec;
			rec.itype = PHYS_FOREIGN_ID_ENTITY;
			rec.islot = -1;
			rec.idEnt = pEntity->GetId();
			if (pStatObj->GetCloneSourceObject())
				pStatObj = pStatObj->GetCloneSourceObject();
			(rec.pStatObjOrg = pStatObj)->AddRef();
			if( pREvent->massOrg<0.0001f )
			{
				float mass,density;
				pStatObj->GetPhysicalProperties(mass,density);
				rec.mass =  mass;
			}
			else
				rec.mass = pREvent->massOrg;
			s_this->m_brokenObjs.push_back(rec);
		}
	}

	return 1;
}


int CActionGame::OnCollisionImmediate( const EventPhys * pEvent )
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_ACTION);

	for (TGlobalPhysicsCallbackSet::const_iterator it=s_this->m_globalPhysicsCallbacks.collision[1].begin();
		it != s_this->m_globalPhysicsCallbacks.collision[1].end();
		++it)
	{
		it->first(pEvent, it->second);
	}

	const EventPhysCollision *pCollision = static_cast<const EventPhysCollision *>(pEvent);
	IGameObject *pSrc=s_this->GetPhysicalEntityGameObject(pCollision->pEntity[0]);
	IGameObject *pTrg=s_this->GetPhysicalEntityGameObject(pCollision->pEntity[1]);

	SGameObjectEvent event(eGFE_OnCollision, eGOEF_ToExtensions|eGOEF_ToGameObject);
	event.ptr = (void *)pCollision;

	if (pSrc && pSrc->WantsPhysicsEvent(eEPE_OnCollisionImmediate))
		pSrc->SendEvent(event);
	if (pTrg && pTrg->WantsPhysicsEvent(eEPE_OnCollisionImmediate))
		pTrg->SendEvent(event);

	ISurfaceTypeManager *pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
  ISurfaceType *pMat = pSurfaceTypeManager->GetSurfaceType(pCollision->idmat[1]);
	float energy,hitenergy;

	if (/*pCollision->mass[0]>=1500.0f &&*/ s_this->AllowProceduralBreaking(ePBT_Normal))
	{
		if (pMat)
			if (pMat->GetBreakability()==1 && 
					(pCollision->mass[0]>10.0f || pCollision->pEntity[0] && pCollision->pEntity[0]->GetType()==PE_ARTICULATED ||
					(pCollision->vloc[0]*pCollision->n<0 && 
					(energy=pMat->GetBreakEnergy())>0 && 
				  (hitenergy=max(fabs_tpl(pCollision->vloc[0].x)+fabs_tpl(pCollision->vloc[0].y)+fabs_tpl(pCollision->vloc[0].z), 
					pCollision->vloc[0].len2())*pCollision->mass[0]) >= energy && 
				  pMat->GetHitpoints()<=FtoI(min(1E6f,hitenergy/energy)))))
				return 0; // the object will break, tell the physics to ignore the collision
			else 
			if (pMat->GetBreakability()==2 && 
					pCollision->idmat[0]!=pCollision->idmat[1] && 
					(energy=pMat->GetBreakEnergy())>0 && 
					pCollision->mass[0]*2>energy && 
					(hitenergy=max(fabs_tpl(pCollision->vloc[0].x)+fabs_tpl(pCollision->vloc[0].y)+fabs_tpl(pCollision->vloc[0].z), 
							pCollision->vloc[0].len2())*pCollision->mass[0]) >= energy &&
					pMat->GetHitpoints()<=FtoI(min(1E6f,hitenergy/energy)) &&
					pCollision)
				return 0;
	}

	return 1;
}

int CActionGame::OnPostStepImmediate( const EventPhys * pEvent )
{
	for (TGlobalPhysicsCallbackSet::const_iterator it=s_this->m_globalPhysicsCallbacks.postStep[1].begin();
		it != s_this->m_globalPhysicsCallbacks.postStep[1].end();
		++it)
	{
		it->first(pEvent, it->second);
	}

	const EventPhysPostStep *pPostStep = static_cast<const EventPhysPostStep *>(pEvent);
	IGameObject *pSrc=s_this->GetPhysicalEntityGameObject(pPostStep->pEntity);

	if (pSrc && pSrc->WantsPhysicsEvent(eEPE_OnPostStepImmediate))
	{
		SGameObjectEvent event(eGFE_OnPostStep, eGOEF_ToExtensions|eGOEF_ToGameObject);
		event.ptr = (void *)pPostStep;
		pSrc->SendEvent(event);
	}

	return 1;
}

int CActionGame::OnStateChangeImmediate( const EventPhys * pEvent )
{
	for (TGlobalPhysicsCallbackSet::const_iterator it=s_this->m_globalPhysicsCallbacks.stateChange[1].begin();
		it != s_this->m_globalPhysicsCallbacks.stateChange[1].end();
		++it)
	{
		it->first(pEvent, it->second);
	}

	const EventPhysStateChange *pStateChange = static_cast<const EventPhysStateChange *>(pEvent);
	IGameObject *pSrc=s_this->GetPhysicalEntityGameObject(pStateChange->pEntity);

	if (pSrc && pSrc->WantsPhysicsEvent(eEPE_OnStateChangeImmediate))
	{
		SGameObjectEvent event(eGFE_OnStateChange, eGOEF_ToExtensions|eGOEF_ToGameObject);
		event.ptr = (void *)pStateChange;
		pSrc->SendEvent(event);
	}

	return 1;
}

int CActionGame::OnCreatePhysicalEntityImmediate( const EventPhys * pEvent )
{
	for (TGlobalPhysicsCallbackSet::const_iterator it=s_this->m_globalPhysicsCallbacks.createEntityPart[1].begin();
		it != s_this->m_globalPhysicsCallbacks.createEntityPart[1].end();
		++it)
	{
		it->first(pEvent, it->second);
	}

	EventPhysCreateEntityPart *pepcep = (EventPhysCreateEntityPart*)pEvent;
	if (pepcep->iForeignData==PHYS_FOREIGN_ID_ENTITY && pepcep->pEntity!=pepcep->pEntNew)
		s_this->UpdateEntityIdForBrokenPart(((IEntity*)pepcep->pForeignData)->GetId());
	if (g_tree_cut_reuse_dist>0 && !gEnv->bMultiplayer && pepcep->cutRadius>0)
		s_this->RegisterEntsForBreakageReuse(pepcep->pEntity,pepcep->partidSrc,pepcep->pEntNew, pepcep->cutPtLoc[0].z,pepcep->breakSize);

	return 1;
}

EntityId CActionGame::UpdateEntityIdForBrokenPart(EntityId idSrc)
{
	std::map<EntityId,int>::iterator iter;
	EntityId id=0;
	if ((iter=m_entPieceIdx.find(idSrc))==m_entPieceIdx.end())
		iter = m_entPieceIdx.insert(std::pair<EntityId,int>(idSrc,-1)).first;
	for(++iter->second; iter->second<m_brokenEntParts.size() && m_brokenEntParts[iter->second].idSrcEnt!=idSrc; ++iter->second);
	if (iter->second < m_brokenEntParts.size())
		m_pEntitySystem->SetNextSpawnId(id=m_brokenEntParts[iter->second].idNewEnt);
	return id;
}

int CActionGame::OnUpdateMeshImmediate( const EventPhys * pEvent )
{
	EventPhysUpdateMesh *pepum = (EventPhysUpdateMesh*)pEvent;
	if (pepum->iForeignData==PHYS_FOREIGN_ID_STATIC)
		s_this->UpdateEntityIdForVegetationBreak((IRenderNode*)pepum->pForeignData);
	if (g_tree_cut_reuse_dist>0 && !gEnv->bMultiplayer && pepum->pMesh->GetSubtractionsCount()>0)
		s_this->RemoveEntFromBreakageReuse(pepum->pEntity, pepum->pMesh->GetSubtractionsCount()<=1);

	return 1;
}

EntityId CActionGame::UpdateEntityIdForVegetationBreak(IRenderNode *pVeg)
{
	Vec3 pos = pVeg->GetPos();
	float V = pVeg->GetEntityStatObj(0)->GetPhysGeom() ? pVeg->GetEntityStatObj(0)->GetPhysGeom()->V:0;
	int i;
	EntityId id=0;
	for(i=0; i<m_brokenVegParts.size() && ((m_brokenVegParts[i].pos-pos).len2()>sqr(0.03f) || 
			fabs_tpl(m_brokenVegParts[i].volume-V)>min(V,m_brokenVegParts[i].volume)*1e-4f); i++);
	if (i<m_brokenVegParts.size())
		m_pEntitySystem->SetNextSpawnId(id=m_brokenVegParts[i].idNewEnt);
	return id;
}

void CActionGame::FixBrokenObjects()
{
  //
  if( gEnv->pEntitySystem->GetBreakableManager() )
  {
    gEnv->pEntitySystem->GetBreakableManager()->ResetBrokenObjects();
  }
  //
  for(int i=m_brokenObjs.size()-1; i>=0; i--)
	{
		if (m_brokenObjs[i].itype==PHYS_FOREIGN_ID_ENTITY)
		{
			IEntity *pEnt = m_pEntitySystem->GetEntity(m_brokenObjs[i].idEnt);
			if (pEnt)
				if (m_brokenObjs[i].islot>=0)
				{
					IStatObj *pStatObj = pEnt->GetStatObj(m_brokenObjs[i].islot);
					pEnt->SetStatObj(m_brokenObjs[i].pStatObjOrg, m_brokenObjs[i].islot,true);
				}
				else if (m_brokenObjs[i].pStatObjOrg)
				{
					for(int j=pEnt->GetSlotCount()-1; j>=0; j--)
						pEnt->FreeSlot(j);
					pEnt->SetStatObj(m_brokenObjs[i].pStatObjOrg, ENTITY_SLOT_ACTUAL, true,m_brokenObjs[i].mass);
				}
				else for(int j=pEnt->GetSlotCount()-1; j>0; j--)
					pEnt->FreeSlot(j);
		}
		else
		{
			m_brokenObjs[i].pBrush->SetEntityStatObj(0, m_brokenObjs[i].pStatObjOrg);
			//m_brokenObjs[i].pBrush->GetEntityStatObj(0)->Refresh(FRO_GEOMETRY);
		}
		if (m_brokenObjs[i].pStatObjOrg)
			m_brokenObjs[i].pStatObjOrg->Release();
	}
	m_brokenObjs.clear();
	m_mapEntHits.clear();
	for(SEntityHits *phits=m_pEntHits0; phits; phits=phits->pnext)
		phits->lifeTime=phits->timeUsed=0, phits->nHits=0;
}

void CActionGame::OnEditorSetGameMode( bool bGameMode )
{
	CCryAction *pCryAction = CCryAction::GetCryAction();

	FixBrokenObjects();
	ClearBreakHistory();

	// changed mode
	if (m_pEntitySystem)
	{
		IEntityItPtr pIt = m_pEntitySystem->GetEntityIterator();
		while (IEntity *pEntity = pIt->Next())
			CallOnEditorSetGameMode(pEntity, bGameMode);
	}

	// handle the listener stuff for sound system
	ISoundSystem * pSoundSystem = gEnv->pSoundSystem;
	IActor * pClientActor = pCryAction->GetClientActor();
	EntityId clientActorId = pClientActor? pClientActor->GetEntityId() : 0;
		
	pSoundSystem->SetListenerEntity( LISTENERID_STANDARD, clientActorId );
	pSoundSystem->Pause(false, true); // reset game volume
		
	ISoundMoodManager *pMoodManager = pSoundSystem->GetIMoodManager();

	if (pMoodManager)
		pMoodManager->Reset();

	if (!bGameMode)
	{
		s_this->m_vegStatus.clear();

		std::map< EntityId, SVegCollisionStatus* >::iterator it2 = s_this->m_treeStatus.begin();
		while (it2 != s_this->m_treeStatus.end())
		{
			SVegCollisionStatus *cur = (SVegCollisionStatus *)it2->second;
			if (cur)
			{
				delete cur;
			}
			it2++;
		}
		s_this->m_treeStatus.clear();
	}

	pCryAction->AllowSave(true);
	pCryAction->AllowLoad(true);

	// reset time of day scheduler before flowsystem
	// as nodes could register in initialize
	pCryAction->GetTimeOfDayScheduler()->Reset(); 
	gEnv->pFlowSystem->Reset();
	CDialogSystem* pDS = pCryAction->GetDialogSystem();
	if (pDS)
	{
		pDS->Reset();
		if (bGameMode && CDialogSystem::sAutoReloadScripts != 0)
		{
			pDS->ReloadScripts();
		}
	}

	pCryAction->GetPersistantDebug()->Reset();
	pCryAction->GetVisualLog()->Reset();

	pCryAction->DestroyUnusedAIActorProxies();	
}

void CActionGame::CallOnEditorSetGameMode(IEntity *pEntity, bool bGameMode)
{
	IScriptTable *pScriptTable(pEntity->GetScriptTable());
	if (!pScriptTable)
		return;

	if ((pScriptTable->GetValueType("OnEditorSetGameMode") == svtFunction) && pScriptTable->GetScriptSystem()->BeginCall(pScriptTable, "OnEditorSetGameMode"))
	{
		pScriptTable->GetScriptSystem()->PushFuncParam(pScriptTable);
		pScriptTable->GetScriptSystem()->PushFuncParam(bGameMode);
		pScriptTable->GetScriptSystem()->EndCall();
	}
}

void CActionGame::ClearBreakHistory()
{
	std::map< EntityId, SVegCollisionStatus* >::iterator it = s_this->m_treeBreakStatus.begin();
	while (it != s_this->m_treeBreakStatus.end())
	{
		SVegCollisionStatus *cur = (SVegCollisionStatus *)it->second;
		if (cur)
			delete cur;
		it++;
	}
	s_this->m_treeBreakStatus.clear();
	it = s_this->m_treeStatus.begin();
	while (it != s_this->m_treeStatus.end())
	{
		SVegCollisionStatus *cur = (SVegCollisionStatus *)it->second;
		if (cur)
			delete cur;
		it++;
	}
	s_this->m_treeStatus.clear();

	s_this->m_breakEvents.clear();
	s_this->m_brokenEntParts.clear();
	s_this->m_brokenVegParts.clear();
	s_this->m_broken2dChunkIds.clear();	
	s_this->m_mapBrokenMeshes.clear();
	m_totBreakageSize = 0;
	s_this->ClearTreeBreakageReuseLog();
}

void CActionGame::ClearTreeBreakageReuseLog()
{
	for(std::map<IPhysicalEntity*,STreePieceThunk*>::iterator iter=m_mapBrokenTreesChunks.begin(); iter!=m_mapBrokenTreesChunks.end(); iter++)
		if (iter->second!=(STreePieceThunk*)&iter->second->pParent->pPhysEntNew0)
			delete iter->second;
	for(std::map<IPhysicalEntity*,STreeBreakInst*>::iterator iter=m_mapBrokenTreesByPhysEnt.begin(); iter!=m_mapBrokenTreesByPhysEnt.end(); iter++)
		delete iter->second;
	m_mapBrokenTreesByPhysEnt.clear();
	m_mapBrokenTreesByCGF.clear();
	m_mapBrokenTreesChunks.clear();
}

void CActionGame::FlushBreakableObjects()
{
	int i;
	IEntity *pent;
	for(i=0;i<m_brokenVegParts.size();i++) 
		if ((pent=m_pEntitySystem->GetEntity(m_brokenVegParts[i].idNewEnt)) && !(pent->GetFlags() & ENTITY_FLAG_MODIFIED_BY_PHYSICS))
			m_pEntitySystem->RemoveEntity(m_brokenVegParts[i].idNewEnt,true);
	for(i=0;i<m_brokenEntParts.size();i++)
		if ((pent=m_pEntitySystem->GetEntity(m_brokenEntParts[i].idNewEnt)) && !(pent->GetFlags() & ENTITY_FLAG_MODIFIED_BY_PHYSICS))
			m_pEntitySystem->RemoveEntity(m_brokenEntParts[i].idNewEnt,true);
	for(i=0;i<m_broken2dChunkIds.size();i++)
		if ((pent=m_pEntitySystem->GetEntity(m_broken2dChunkIds[i])) && !(pent->GetFlags() & ENTITY_FLAG_MODIFIED_BY_PHYSICS))
			m_pEntitySystem->RemoveEntity(m_broken2dChunkIds[i],true);
	// maybe clear vectors as well
}

void CActionGame::Serialize(TSerialize ser )
{
	SerializeBreakableObjects(ser);

	if(ser.IsReading())
	{
		for(int i = 0; i < MAX_CACHED_EFFECTS; ++i)
			m_lstCachedEffects[i].fLastTime = 0.0f; //reset timeout 
	}
}

void CActionGame::SerializeBreakableObjects(TSerialize ser)
{
	SSerializeScopedBeginGroup breakableObjectsGroup( ser,"BreakableObjects" );

	int i,j;
	if (ser.IsReading())
	{








	}/*	else
	{
		IEntity *pent;
		for(i=0;i<m_brokenVegParts.size();i++) if (pent=m_pEntitySystem->GetEntity(m_brokenVegParts[i].idNewEnt))
			pent->ClearFlags(ENTITY_FLAG_SPAWNED|ENTITY_FLAG_SLOTS_CHANGED);
		for(i=0;i<m_brokenEntParts.size();i++) if (pent=m_pEntitySystem->GetEntity(m_brokenEntParts[i].idNewEnt))
			pent->ClearFlags(ENTITY_FLAG_SPAWNED|ENTITY_FLAG_SLOTS_CHANGED);
		for(i=0;i<m_broken2dChunkIds.size();i++) if (pent=m_pEntitySystem->GetEntity(m_broken2dChunkIds[i]))
			pent->ClearFlags(ENTITY_FLAG_SPAWNED|ENTITY_FLAG_SLOTS_CHANGED);
	}*/

	ser.Value("BreakEvents", m_breakEvents);
	ser.Value("BrokenEntParts", m_brokenEntParts);
	ser.Value("BrokenVegParts", m_brokenVegParts);
	ser.Value("Broken2dChunkIds", m_broken2dChunkIds);
	ser.Value("BrokenMeshesList", m_mapBrokenMeshes);
	ser.Value("BrokenMeshesSize", m_totBreakageSize);

	if (ser.IsReading())
	{
		EventPhysCollision epc;
		IEntity *pEnt;
		IRenderNode *pVeg;
		IPhysicalEntity **pents;
		pe_params_structural_joint psj;
		m_bLoading = true;
		m_pPhysicalWorld->GetPhysVars()->bLogStructureChanges = 0;
		m_entPieceIdx.clear();
		epc.pEntity[0] = 0;
		epc.iForeignData[0] = -1;
		ClearTreeBreakageReuseLog();

		m_pPhysicalWorld->UpdateDeformingEntities(-1);
		FixBrokenObjects();

		for(i=0;i<m_breakEvents.size();i++)
		{
			m_iCurBreakEvent = i;
			if ((epc.iForeignData[1]=m_breakEvents[i].itype)==PHYS_FOREIGN_ID_ENTITY)
			{
				if (!(pEnt=m_pEntitySystem->GetEntity(m_breakEvents[i].idEnt)))
				{
					m_pPhysicalWorld->UpdateDeformingEntities();
					if (!(pEnt = m_pEntitySystem->GetEntity(m_breakEvents[i].idEnt)))
						continue;
				}
				epc.pForeignData[1] = pEnt;
				epc.pEntity[1] = pEnt->GetPhysics();
				pEnt->SetPosRotScale(m_breakEvents[i].pos, m_breakEvents[i].rot, m_breakEvents[i].scale);
				if (!epc.pEntity[1] || pEnt->GetFlags() & ENTITY_FLAG_MODIFIED_BY_PHYSICS)
					continue;
			} else if (m_breakEvents[i].itype==PHYS_FOREIGN_ID_STATIC)
			{
				j = m_pPhysicalWorld->GetEntitiesInBox(m_breakEvents[i].objCenter-Vec3(0.05f),m_breakEvents[i].objCenter+Vec3(0.05f),pents,ent_static);
				phys_geometry *pPhysGeom;
				for(--j;j>=0;j--) if (
					(pVeg = (IRenderNode*)pents[j]->GetForeignData(PHYS_FOREIGN_ID_STATIC)) && 
					(pVeg->GetPos()-m_breakEvents[i].objPos).len2()<sqr(0.03f) && 
					(m_breakEvents[i].objVolume==0 || pVeg->GetEntityStatObj(0) && (pPhysGeom=pVeg->GetEntityStatObj(0)->GetPhysGeom()) && 
					fabs_tpl(pPhysGeom->V-m_breakEvents[i].objVolume)<min(pPhysGeom->V,m_breakEvents[i].objVolume)*0.1f))
					break;
				if (j<0)
					continue;
				epc.pForeignData[1] = pVeg;
				epc.pEntity[1] = pVeg->GetPhysics();
				psj.idx=0; MARK_UNUSED psj.id;
				if (!epc.pEntity[1] || (m_breakEvents[i].idmat[0]==-1 && epc.pEntity[1]->GetParams(&psj)))
					continue;
			} else
				continue;

			if (m_breakEvents[i].idmat[0]!=-1)
			{
				epc.pt=m_breakEvents[i].pt; epc.n=m_breakEvents[i].n;
				epc.vloc[0]=m_breakEvents[i].vloc[0]; epc.vloc[1]=m_breakEvents[i].vloc[1];
				epc.mass[0]=m_breakEvents[i].mass[0]; epc.mass[1]=m_breakEvents[i].mass[1];
				epc.idmat[0]=m_breakEvents[i].idmat[0]; epc.idmat[1]=m_breakEvents[i].idmat[1];
				epc.partid[0]=m_breakEvents[i].partid[0]; epc.partid[1]=m_breakEvents[i].partid[1];
				epc.penetration = m_breakEvents[i].penetration; epc.radius = m_breakEvents[i].radius;
				OnCollisionLogged_Breakable(&epc);
			} else
				m_pPhysicalWorld->DeformPhysicalEntity(epc.pEntity[1], m_breakEvents[i].pt,m_breakEvents[i].n, m_breakEvents[i].penetration, 1);
			//This is for a purpose, some ents require 2 updates to form broken chunks
			m_pPhysicalWorld->UpdateDeformingEntities();  
			m_pPhysicalWorld->UpdateDeformingEntities();
		}
		m_pPhysicalWorld->GetPhysVars()->bLogStructureChanges = 1;
		m_entPieceIdx.clear();
		m_bLoading = false;
	}
}

void CCryAction::Serialize(TSerialize ser)
{
	if (m_pGame)
		m_pGame->Serialize(ser); 
}

void CCryAction::FlushBreakableObjects()
{
	if (m_pGame)
		m_pGame->FlushBreakableObjects(); 
}

void CCryAction::ClearBreakHistory()
{
	if (m_pGame)
		m_pGame->ClearBreakHistory(); 
}

void CActionGame::OnExplosion(const ExplosionInfo& ei)
{
	int i,j,nEnts;
	IPhysicalEntity **pents;
	pe_status_nparts snp;
	pe_status_pos sp;
	pe_params_part pp;
	SBreakEvent be; memset(&be,0,sizeof(be));
	IEntity *pEntity;
	IRenderNode *pVeg;
	IStatObj *pStatObj;

	nEnts = m_pPhysicalWorld->GetEntitiesInBox(ei.pos-Vec3(ei.radius),ei.pos+Vec3(ei.radius), pents, ent_static|ent_rigid|ent_sleeping_rigid);
	for(i=0;i<nEnts;i++) 
	{
		for(j=pents[i]->GetStatus(&snp)-1;j>=0;j--)
		{
			pp.ipart=j; MARK_UNUSED pp.partid;
			if (pents[i]->GetParams(&pp) && pp.idmatBreakable>=0)
				break;
		}
		pents[i]->GetStatus(&sp);
		if (j>=0)
		{
			switch (be.itype = pents[i]->GetiForeignData()) 
			{
				case PHYS_FOREIGN_ID_ENTITY: 
					be.idEnt = (pEntity = (IEntity*)pents[i]->GetForeignData(be.itype))->GetId(); 
					be.pos = sp.pos;
					be.rot = sp.q;
					be.scale = pEntity->GetScale();
					break;
				case PHYS_FOREIGN_ID_STATIC: 
					be.objPos = (pVeg = (IRenderNode*)pents[i]->GetForeignData(be.itype))->GetPos();
					be.objCenter = pVeg->GetBBox().GetCenter();
					be.objVolume = (pStatObj=pVeg->GetEntityStatObj(0)) && pStatObj->GetPhysGeom() ? pStatObj->GetPhysGeom()->V:0;
					break;
			}

			be.pt=ei.pos; be.n=ei.dir;
			be.idmat[0] = -1;
			be.penetration = ei.hole_size;
			m_breakEvents.push_back(be);
		}
	}
}

void CActionGame::DumpStats()
{
	if(m_pGameStats)
		m_pGameStats->Dump();
}

void CActionGame::GetMemoryStatistics(ICrySizer * s)
{
	{
		SIZER_SUBCOMPONENT_NAME(s,"Network");
    if(m_pGameClientNub)
		  m_pGameClientNub->GetMemoryStatistics(s);
    if(m_pGameServerNub)
		  m_pGameServerNub->GetMemoryStatistics(s);
    if(m_pGameContext)
		  m_pGameContext->GetMemoryStatistics(s);
	}

	{
		SIZER_SUBCOMPONENT_NAME(s,"ActionGame");
		s->Add(*this);
    if(m_pGameStats)
      m_pGameStats->GetMemoryStatistics(s);
		s->AddContainer(m_brokenObjs);
		s->AddContainer(m_breakEvents);
		s->AddContainer(m_brokenEntParts);
		s->AddContainer(m_brokenVegParts);
		s->AddContainer(m_broken2dChunkIds);
		s->AddContainer(m_entPieceIdx);
		s->AddContainer(m_mapECH);
		s->AddContainer(m_mapEntHits);
		s->AddContainer(m_vegStatus);
		s->AddContainer(m_treeStatus);
		s->AddContainer(m_treeBreakStatus);
		
	}
}


void CActionGame::CreateGameStats()
{
  m_pGameStats=new CGameStats(CCryAction::GetCryAction());
}

void CActionGame::ReleaseGameStats()
{
  delete m_pGameStats;
  m_pGameStats=0;
}
