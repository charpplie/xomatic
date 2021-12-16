#include "StdAfx.h"
#include "NetCVars.h"
#include "Lobby/CryLobby.h"

CNetCVars * CNetCVars::s_pThis;

CNetCVars::CNetCVars()
{
	memset(this, 0, sizeof(*this));

	NET_ASSERT(!s_pThis);
	s_pThis = this;

	IConsole * c = gEnv->pConsole;

#if defined(_DEBUG) || defined(DEBUG)
	REGISTER_COMMAND("net_ExpKeyXch_test", CExponentialKeyExchange::Test,0,"");
#endif
	REGISTER_CVAR2("cl_tokenid", &TokenId, 0, 0, "Token id expected from client player during connection");

//	REGISTER_CVAR2( "sys_network_CPU", &CPU, 1, 0, "Run network multithreaded" );
	pSchedulerDebug = REGISTER_STRING( "net_scheduler_debug", "0", 0, "Show scheduler debugger for some channel" );
	REGISTER_CVAR2( "net_log", &LogLevel, 0, 0, "Logging level of network system" );

#if ENABLE_DEBUG_KIT
	REGISTER_CVAR2( "net_inspector", &NetInspector, 0, 0, "Logging level of network system" );
	REGISTER_CVAR2( "net_meminfo", &MemInfo, 0, 0, "Display memory usage information" );
	REGISTER_CVAR2( "net_logcomments", &LogComments, 0, 0, "Enable ultra verbose logging" );
	REGISTER_CVAR2( "net_showobjlocks", &ShowObjLocks, 0, 0, "show lock debugging display");
	REGISTER_CVAR2( "net_pq_log", &EndpointPendingQueueLogging, 0, 0, "Log updates to the pending queue for each channel");
	REGISTER_CVAR2( "net_disconnect_on_uncollected_breakage", &DisconnectOnUncollectedBreakage, 0, 0, "Disconnect on uncollected breakage" );
	REGISTER_CVAR2( "net_debug_connection_state", &DebugConnectionState, 0, 0, "Show current connecting status as a head-up display" );
	REGISTER_CVAR2( "net_log_dropped_messages", &LogDroppedMessagesVar, 0, 0, "Log debug information for dropped messages" );
	REGISTER_CVAR2( "net_perfcounters", &PerfCounters, 0, 0, "Display some dedicated server performance counters" );
	REGISTER_CVAR2( "net_gamespy_debug",&GSDebugOutput,0,VF_NULL,"Sets debug output level for GameSpy SDKs");
	REGISTER_CVAR2( "net_random_packet_corruption", &RandomPacketCorruption, 0, VF_CHEAT, "" );
#endif

	REGISTER_CVAR2( "net_channelstats", &ChannelStats, 0, 0, "Display bandwidth statistics per-channel" );
	REGISTER_CVAR2( "net_bw_aggressiveness", &BandwidthAggressiveness, 0.5f, 0, "Balances TCP friendlyness versus prioritization of game traffic");
	REGISTER_CVAR2( "net_inactivitytimeout", &InactivityTimeout, 30, 0, "Sets how many seconds without receiving a packet a connection will stay alive for (can only be set on server)" );
	REGISTER_CVAR2( "net_inactivitytimeoutDevmode", &InactivityTimeoutDevmode, 600, 0, "Sets how many seconds without receiving a packet a connection will stay alive for while in devmode (can only be set on server)" );
	REGISTER_CVAR2( "net_backofftimeout", &BackoffTimeout, 6*60, 0, "Maximum time to allow a remote machine to stall for before disconnecting" );
	REGISTER_CVAR2( "sv_maxmemoryusage", &MaxMemoryUsage, 0, 0, "Maximum memory a dedicated server is allowed to use" );








#if NET_ASSERT_LOGGING
	REGISTER_CVAR2( "net_assertlogging", &AssertLogging, 0, VF_DUMPTODISK, "Log network assertations" );
#endif

#if LOG_INCOMING_MESSAGES || LOG_OUTGOING_MESSAGES
	REGISTER_CVAR2( "net_logmessages", &LogNetMessages, 0, VF_NULL,"" );
#endif
#if LOG_BUFFER_UPDATES
	REGISTER_CVAR2( "net_logbuffers", &LogBufferUpdates, 0, VF_NULL,"" );
#endif
#if STATS_COLLECTOR_INTERACTIVE
	REGISTER_CVAR2( "net_showdatabits", &ShowDataBits, 0, 0, "show bits used for different data" );
#endif
#if ENABLE_DEBUG_KIT
	static const int DEFAULT_CHEAT_PROTECTION = 3;
#else
	static const int DEFAULT_CHEAT_PROTECTION = 0;//0 for GameK01 for now
#endif
	REGISTER_CVAR2( "sv_cheatprotection", &CheatProtectionLevel, DEFAULT_CHEAT_PROTECTION, 0, "" );
	pVoiceCodec=REGISTER_STRING( "sv_voicecodec", "speex", VF_REQUIRE_LEVEL_RELOAD,"" );
	REGISTER_CVAR2("sv_voice_enable_groups",&EnableVoiceGroups,1,VF_NULL,"");

	REGISTER_CVAR2("net_enable_voice_chat",&EnableVoiceChat,1,VF_REQUIRE_APP_RESTART,"");
	REGISTER_CVAR2("net_rtt_convergence_factor", &RTTConverge, 995,VF_NULL,"");
#if !ALWAYS_CHECK_CD_KEYS
  REGISTER_CVAR2( "sv_check_cd_keys",&CheckCDKeys, 0, VF_NULL, "Sets CDKey validation mode:\n 0 - no check\n 1 - check&disconnect unverified\n 2 - check only\n");
#endif
  REGISTER_CVAR2( "sv_ranked", &RankedServer, 0, VF_NULL, "Enable statistics report, for official servers only.");
	// network visualizationsv_
#if ENABLE_DEBUG_KIT
	REGISTER_CVAR2( "net_vis_mode", &VisMode, 0,VF_NULL,"" );
	REGISTER_CVAR2( "net_vis_window", &VisWindow, 0/*.3f*/,VF_NULL,"" );
	REGISTER_CVAR2( "net_show_ping", &ShowPing, 0,VF_NULL,"" );
#endif

#if INTERNET_SIMULATOR
	REGISTER_CVAR2( "net_packetlossrate", &PacketLossRate, 0.0f, VF_CHEAT, "")->SetOnChangeCallback(OnPacketLossRateChange );
	REGISTER_CVAR2( "net_packetextralag", &PacketExtraLag, 0.0f, VF_CHEAT, "")->SetOnChangeCallback(OnPacketExtraLagChange );
#endif

	REGISTER_CVAR2( "net_highlatencythreshold", &HighLatencyThreshold, 0.5f, VF_NULL, "" ); // must be net synched
	REGISTER_CVAR2( "net_highlatencytimelimit", &HighLatencyTimeLimit, 1.5f, VF_NULL, "" ); // must be net synched

	REGISTER_CVAR2( "net_remotetimeestimationwarning", &RemoteTimeEstimationWarning, 0, VF_NULL, "" ); 

	REGISTER_CVAR2( "net_enable_tfrc", &EnableTFRC, 1,VF_NULL,"" );

	//REGISTER_CVAR2( "net_wmi_check_interval", &WMICheckInterval, gEnv->pSystem->IsDedicated() ? 0.0f : 2.0f, 0 );
	REGISTER_CVAR2("net_connectivity_detection_interval", &NetworkConnectivityDetectionInterval, 1.0f,VF_NULL,"");

	// TODO: remove/cleanup
	REGISTER_FLOAT( "net_defaultChannelBitRateDesired", 200000.0f, VF_READONLY,"" );
	REGISTER_FLOAT( "net_defaultChannelBitRateToleranceLow", 0.5f, VF_READONLY,"" );
	REGISTER_FLOAT( "net_defaultChannelBitRateToleranceHigh", 0.001f, VF_READONLY,"" );
	REGISTER_FLOAT( "net_defaultChannelPacketRateDesired", 50.0f, VF_READONLY,"" );
	REGISTER_FLOAT( "net_defaultChannelPacketRateToleranceLow", 0.1f, VF_READONLY,"" );
	REGISTER_FLOAT( "net_defaultChannelPacketRateToleranceHigh", 2.0f, VF_READONLY,"" );
	REGISTER_FLOAT( "net_defaultChannelIdlePacketRateDesired", 0.05f, VF_READONLY,"" );

  REGISTER_CVAR2("net_voice_lead_packets", &VoiceLeadPackets, 5,VF_NULL,"" );
  REGISTER_CVAR2( "net_voice_trail_packets", &VoiceTrailPackets, 5,VF_NULL,"" );
	REGISTER_CVAR2( "net_voice_proximity", &VoiceProximity, 0.0f, VF_NULL,"");

  REGISTER_CVAR2( "net_lan_scanport_first", &LanScanPortFirst,  64087, VF_DUMPTODISK, "Starting port for LAN games scanning");
  REGISTER_CVAR2( "net_lan_scanport_num", &LanScanPortNum, 5, VF_DUMPTODISK, "Num ports for LAN games scanning" );

#if ENABLE_DEBUG_KIT
	REGISTER_COMMAND("net_reload_scheduler", ReloadScheduler, 0, "Reload game/scripts/network/scheduler.xml");
#endif

  REGISTER_COMMAND("net_set_cdkey", SetCDKey,VF_DUMPTODISK,"");
	StatsLogin = REGISTER_STRING( "net_stats_login", "", VF_DUMPTODISK, "Login for reporting stats on dedicated server");
	StatsPassword = REGISTER_STRING( "net_stats_pass", "", VF_DUMPTODISK, "Password for reporting stats on dedicated server");

	REGISTER_COMMAND("net_dump_object_state", DumpObjectState, 0,"");

#if NETWORK_REBROADCASTER
	REGISTER_COMMAND("net_rebroadcaster", RebroadcasterCmd, 0, "net_rebroadcaster on/off - enables/disables the rebroadcaster\n"
																														 "net_rebroadcaster debug on/off - enables/disables debug info\n"
																														 "net_rebroadcaster <from channel id> <to channel id> on/off - enables/disables individual connections\n"
																														 "net_rebroadcaster - displays status");
	REGISTER_INT("net_enable_rebroadcaster", 1, VF_DUMPTODISK, "enables/disables rebroadcaster");
#endif
#if NETWORK_HOST_MIGRATION
	REGISTER_FLOAT("net_migrate_timeout", 30.0f, VF_DUMPTODISK, "timeout for host migration failure");
	REGISTER_INT("net_automigrate_host", 1, VF_DUMPTODISK, "Toggles whether server migration is automatically attempted");
	REGISTER_COMMAND("net_migrate_host", MigrateHostCmd, 0, "net_migrate_host - tell this client to become the new host\n"
																															"net_migrate_host <machine> - Attempt to rejoin a migrated game on the specified host");
#endif

#if ENABLE_DEBUG_KIT
	REGISTER_COMMAND("net_stall", Stall, VF_CHEAT, "stall the network thread for a time (default 1 second, but can be passed in as a parameter");
#endif

	REGISTER_CVAR2("net_minTCPFriendlyBitRate", &MinTCPFriendlyBitRate, 30000.f, VF_NULL, "Minimum bitrate allowed for TCP friendly transmission");

  REGISTER_CVAR2( "net_UseDeprecatedVoiceSystem", &useDeprecatedVoiceSystem, 1, VF_DUMPTODISK, "Use deprecated voice system" );

	NET_PROFILE_REG_CVARS();
}

CNetCVars::~CNetCVars()
{
	NET_ASSERT(s_pThis);
	s_pThis = 0;
}

void CNetCVars::DumpObjectState(IConsoleCmdArgs* pArgs)
{
	SCOPED_GLOBAL_LOCK;
	CNetwork::Get()->BroadcastNetDump( eNDT_ObjectState );
}

#if ENABLE_DEBUG_KIT
void CNetCVars::ReloadScheduler(IConsoleCmdArgs* pArgs)
{
	SCOPED_GLOBAL_LOCK;
	CNetwork::Get()->ReloadScheduler();
}
#endif

void CNetCVars::SetCDKey( IConsoleCmdArgs* pArgs)
{
  SCOPED_GLOBAL_LOCK;

  if(pArgs->GetArgCount()>1)
  {
    CNetwork::Get()->SetCDKey(pArgs->GetArg(1));
  }
  else
  {
    CNetwork::Get()->SetCDKey("");
  }
}

//------------------------------------------------------------------------
#if NETWORK_REBROADCASTER
void CNetCVars::RebroadcasterCmd(IConsoleCmdArgs* pArgs)
{
	CryFixedStringT<128> temp;
	int argCount = pArgs->GetArgCount();
	CCryRebroadcaster* pRebroadcaster = NULL;
	CCryLobby* pLobby = (CCryLobby*)CCryLobby::GetLobby();
	if (pLobby)
	{
		pRebroadcaster = pLobby->GetRebroadcaster();
	}

	// Console text colouring:
	// $0 = black | $1 = white  | $2 = blue    | $3 = green  | $4 = red
	// $5 = cyan  | $6 = yellow | $7 = magenta | $8 = orange | %9 = grey

	if (argCount == 1)
	{
		// The command with no arguments simply logs the rebroadcaster status
		if (pRebroadcaster)
		{
			pRebroadcaster->ShowMesh();
		}

		CryLog("  $3net_rebroadcaster $2from $5to $6status $1- where status is enable/disable, for a single connection");
	}
	else if (argCount == 2)
	{
		// Enable or disable the rebroadcaster
		bool enable = false;
		temp = pArgs->GetArg(1);

		if (temp.compareNoCase("disable") == 0 || temp.compareNoCase("off") == 0 || temp.compareNoCase("0") == 0 || temp.compareNoCase("n") == 0)
		{
			enable = false;
		}
		else if (temp.compareNoCase("enable") == 0 || temp.compareNoCase("on") == 0 || temp.compareNoCase("1") == 0 || temp.compareNoCase("y") == 0)
		{
			enable = true;
		}

		gEnv->pConsole->GetCVar("net_enable_rebroadcaster")->Set(enable ? 1 : 0);

		temp.Format("CryNetwork rebroadcaster %s", (enable) ? "$3enabled" : "$4disabled");
		CryLog(temp);
	}
	else if (argCount == 3)
	{
		temp = pArgs->GetArg(1);

		if (temp.compareNoCase("debug") == 0)
		{
			// Enable/disable debug info
			temp = pArgs->GetArg(2);
			bool enable = false;

			if (temp.compareNoCase("disable") == 0 || temp.compareNoCase("off") == 0 || temp.compareNoCase("0") == 0 || temp.compareNoCase("n") == 0)
			{
				enable = false;
			}
			else if (temp.compareNoCase("enable") == 0 || temp.compareNoCase("on") == 0 || temp.compareNoCase("1") == 0 || temp.compareNoCase("y") == 0)
			{
				enable = true;
			}

			if (pRebroadcaster)
			{
				pRebroadcaster->DebugMode(enable);
			}

			temp.Format("CryNetwork rebroadcaster debug info is %s", (enable) ? "$3enabled" : "$4disabled");
			CryLog(temp);
		}
	}
	else if (argCount == 4)
	{
		// Set the status of an individual connection
		temp = pArgs->GetArg(1);
		uint32 fromChannelID = atoi(temp.c_str());
		temp = pArgs->GetArg(2);
		uint32 toChannelID = atoi(temp.c_str());

		ERebroadcasterConnectionStatus status = eRCS_Unknown;
		temp = pArgs->GetArg(3);
		status = eRCS_Unknown;

		if (temp.compareNoCase("disable") == 0 || temp.compareNoCase("off") == 0 || temp.compareNoCase("0") == 0 || temp.compareNoCase("n") == 0)
		{
			status = eRCS_Disabled;
		}
		else if (temp.compareNoCase("enable") == 0 || temp.compareNoCase("on") == 0 || temp.compareNoCase("1") == 0 || temp.compareNoCase("y") == 0)
		{
			status = eRCS_Enabled;
		}

		if (pRebroadcaster)
		{
			pRebroadcaster->SetStatus(fromChannelID, toChannelID, status, true);
		}
	}
}
#endif
//------------------------------------------------------------------------

#if NETWORK_HOST_MIGRATION
void CNetCVars::MigrateHostCmd(IConsoleCmdArgs* pArgs)
{
	// Check whether server migration is allowed
	if (!gEnv->bMultiplayer)
	{
		NetWarning("Multiplayer mode only");
		return;
	}

	IConsole *pConsole = gEnv->pConsole;
	ICVar* pCVar = pConsole->GetCVar("sv_ranked");
	if (pCVar->GetIVal() != 0)
	{
		NetWarning("Migration of ranked game is not permitted");
		return;
	}

	// Essentially, we can migrate the server to any client that isn't already a server,
	// and we can migrate any client (even if it's the original server) to another server.
	if (gEnv->bClient)
	{
		switch (pArgs->GetArgCount())
		{
		case 1:
			if (gEnv->bServer)
			{
				NetWarning("Already hosting game");
			}
			else
			{
				// Attempt to migrate the server to this client
				CNetwork::Get()->HostMigration_Start(LOCAL_CONNECTION_STRING, true);
			}
			break;

		case 2:
			// Attempt to reconnect this client to a migrated server
			CNetwork::Get()->HostMigration_Start(pArgs->GetArg(1), false);
			break;

		default:
			// Malformed command
			NetWarning("Malformed command");
			break;
		}
	}
	else
	{
		NetWarning("Illegal command on dedicated server");
	}
}
#endif

//------------------------------------------------------------------------
#if ENABLE_DEBUG_KIT
void CNetCVars::Stall( IConsoleCmdArgs* pArgs )
{
	float stallTime = 1.0f;
	if (pArgs && pArgs->GetArgCount()>=2)
	{
		if (!sscanf(pArgs->GetArg(1), "%f", &stallTime))
			stallTime = 1.0f;
	}
	if (stallTime > 0.0f)
	{
		NetLogAlways("Stalling all sending for %f seconds", stallTime);
		Get().StallEndTime = g_time + stallTime;
	}
	else
	{
		NetLogAlways("Sleeping for %f seconds", -stallTime);
		SCOPED_GLOBAL_LOCK;
		Sleep((DWORD)(-1000.0f*stallTime));
	}
}
#endif
