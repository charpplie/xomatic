#include "StdAfx.h"
#include "NetworkCVars.h"

CNetworkCVars * CNetworkCVars::s_pThis = 0;

CNetworkCVars::CNetworkCVars()
{
	CRY_ASSERT(!s_pThis);
	s_pThis = this;

	REGISTER_CVAR2("g_breakagelog", &BreakageLog, 0, VF_CHEAT, "Log break events");
	REGISTER_CVAR2("cl_voice_volume", &VoiceVolume, 1.0f, 0, "Set VOIP playback volume: 0-1");
	REGISTER_CVAR2("net_phys_pingsmooth", &PhysSyncPingSmooth, 0.1f, VF_NULL,"");
	REGISTER_CVAR2("net_phys_lagsmooth", &PhysSyncLagSmooth, 0.1f, VF_NULL,"");
	REGISTER_CVAR2( "net_phys_debug", &PhysDebug, 0, VF_CHEAT,"" );
	REGISTER_CVAR2("g_breaktimeoutframes", &BreakTimeoutFrames, 140, VF_NULL,"");
}
