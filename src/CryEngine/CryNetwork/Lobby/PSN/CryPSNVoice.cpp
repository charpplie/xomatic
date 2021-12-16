#include "StdAfx.h"

#include "CryPSNVoice.h"
#include "NetCVars.h"

#if USE_PSN_VOICE

CCryPSNVoice::CCryPSNVoice(CCryLobby* lobby, CCryLobbyService* service) : CCryVoice(lobby, service)
{
	m_curState = ePSNAVS_None;
	m_wantedState = ePSNAVS_None;

	// Make the CCryVoice base pointers point to our data so we can use the common code in CCryVoice
	for (uint32 i = 0; i < MAX_REMOTE_TALKERS; i++)
	{
		m_pRemoteUsers[i] = &m_remoteUsers[i];
	}

	for (uint32 i = 0; i < MAX_LOCAL_TALKERS; i++)
	{
		for (uint32 j = 0; j < MAX_REMOTE_TALKERS; j++)
		{
			m_pUserRelationship[i][j] = &m_userRelationship[i][j];
		}
	}
}

ECryLobbyError CCryPSNVoice::Initialise()
{
	ECryLobbyError error = CCryVoice::Initialise();

	return error;
}

ECryLobbyError CCryPSNVoice::Terminate()
{

	return eCLE_Success;
}

void CCryPSNVoice::ChatEventCallback(CellSysutilAvc2EventId eventId, CellSysutilAvc2EventParam eventParam, void *userData )
{
	CCryPSNVoice *_this = static_cast<CCryPSNVoice*>(userData);

	switch (eventId)
	{
	case CELL_AVC2_EVENT_LOAD_SUCCEEDED:
		_this->m_curState = ePSNAVS_Initialised;
		break;
	case CELL_AVC2_EVENT_JOIN_SUCCEEDED:
		_this->m_curState = ePSNAVS_Joined;
		break;
	case CELL_AVC2_EVENT_UNLOAD_SUCCEEDED:
		_this->m_curState = ePSNAVS_None;
		break;
	case CELL_AVC2_EVENT_LEAVE_SUCCEEDED:
		_this->m_curState = ePSNAVS_Unitialise;
		break;
	default:
		fprintf(stdout,"OSAVC Got Unknown/Unhandled Event %d\n", (unsigned int) eventId);
		break;
	}
}

void CCryPSNVoice::LibraryStartup()
{
	int ret;

	// Initialize initial parameter structure
	CellSysutilAvc2InitParam avc2param;

	ret = cellSysutilAvc2InitParam(CELL_SYSUTIL_AVC2_INIT_PARAM_VERSION, &avc2param);
	if( ret != CELL_OK )
	{
		NetLog("cellSysutilAvc2InitParam() : Error %08X",ret);
		m_curState = ePSNAVS_Error;				// need to do something with this case - for now park
		return;
	}
	// Hardcoded all params for now.. 
	avc2param.media_type  = CELL_SYSUTIL_AVC2_VOICE_CHAT;
	avc2param.max_players = MAX_PSN_REMOTE_TALKERS;
	avc2param.voice_param.max_speakers  = MAX_PSN_REMOTE_TALKERS;
	avc2param.voice_param.voice_quality = CELL_SYSUTIL_AVC2_VOICE_QUALITY_NORMAL;
	const int spuLoadAverage = CNetCVars::Get().psnVoiceSpuLoadAverage;
	avc2param.spu_load_average  = spuLoadAverage;

	ret = cellSysutilAvc2LoadAsync(m_ctx,SYS_MEMORY_CONTAINER_ID_INVALID,ChatEventCallback,this,&avc2param);
	if (ret != CELL_OK)
	{
		NetLog("cellSysutilAvc2LoadAsync() : Error %08X",ret);
		m_curState = ePSNAVS_Error;
		return;
	}

	m_curState = ePSNAVS_Initialising;
}

void CCryPSNVoice::LibraryStop()
{
	int ret = cellSysutilAvc2UnloadAsync();
	if (ret != CELL_OK)
	{
		NetLog("cellSysutilAvc2UnloadAsync() : Error %08X",ret);
		m_curState = ePSNAVS_Error;
		return;
	}

	m_curState = ePSNAVS_Unitialising;
}

void CCryPSNVoice::JoinGame()
{
	int ret = cellSysutilAvc2JoinChatRequest(&m_room);
	if (ret != CELL_OK)
	{
		NetLog("cellSysutilAvc2JoinChatRequest() : Error %08X", ret);
		m_curState = ePSNAVS_Error;
		return;
	}

	m_curState = ePSNAVS_Joining;
}

void CCryPSNVoice::LeaveGame()
{
	int ret = cellSysutilAvc2LeaveChatRequest();
	if (ret != CELL_OK)
	{
		NetLog("cellSysutilAvc2LeaveChatRequest() : Error %08X",ret);
		m_curState = ePSNAVS_Error;
		return;
	}

	m_curState = ePSNAVS_Leaving;
}

void CCryPSNVoice::StartStreaming()
{
	int ret = cellSysutilAvc2StartStreaming();
	if (ret != CELL_OK)
	{
		NetLog("cellSysutilAvc2StartStreaming() : Error %08X",ret);
		m_curState = ePSNAVS_Error;
		return;
	}

	m_curState = ePSNAVS_Streaming;
}

void CCryPSNVoice::StopStreaming()
{
	int ret = cellSysutilAvc2StopStreaming();
	if (ret != CELL_OK)
	{
		NetLog("cellSysutilAvc2StopStreaming() : Error %08X",ret);
		m_curState = ePSNAVS_Error;
		return;
	}

	m_curState = ePSNAVS_Leave;
}
					
void CCryPSNVoice::StartVoice(SceNpMatching2ContextId ctx,SceNpMatching2RoomId room)
{
	m_ctx = ctx;
	m_room = room;

	m_wantedState = ePSNAVS_Streaming;
}

void CCryPSNVoice::StopVoice()
{
	m_wantedState = ePSNAVS_None;
}

void CCryPSNVoice::Tick(CTimeValue tv)
{
	switch (m_curState)
	{
	case ePSNAVS_None:
		if (m_wantedState == ePSNAVS_Streaming)
		{
			LibraryStartup();
		}
		break;
	case ePSNAVS_Initialising:
		break;
	case ePSNAVS_Unitialising:
		break;
	case ePSNAVS_Initialised:
		if (m_wantedState == ePSNAVS_Streaming)
		{
			JoinGame();
		}
		break;
	case ePSNAVS_Unitialise:
		LibraryStop();
		break;
	case ePSNAVS_Joining:
		break;
	case ePSNAVS_Leaving:
		break;
	case ePSNAVS_Joined:
		StartStreaming();
		break;
	case ePSNAVS_Leave:
		LeaveGame();
		break;
	case ePSNAVS_Streaming:
		if (m_wantedState == ePSNAVS_None)
		{
			StopStreaming();
		}
		break;
	case ePSNAVS_Error:
		break;
	}
}

#endif // USE_PSN_VOICE
