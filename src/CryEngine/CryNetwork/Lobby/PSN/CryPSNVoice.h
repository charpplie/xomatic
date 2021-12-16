#ifndef __CRYPSNVOICE_H__
#define __CRYPSNVOICE_H__

#pragma once

#include "Lobby/CryLobby.h"

#if USE_PSN_VOICE

#include <sysutil/sysutil_avc2.h>
#include "Lobby/CryVoice.h"

#define MAX_PSN_LOCAL_TALKERS			4
#define MAX_PSN_REMOTE_TALKERS		16

enum EPSNAVState
{
	ePSNAVS_None,
	ePSNAVS_Initialising,
	ePSNAVS_Unitialising,
	ePSNAVS_Initialised,
	ePSNAVS_Unitialise,
	ePSNAVS_Joining,
	ePSNAVS_Leaving,
	ePSNAVS_Joined,
	ePSNAVS_Leave,
	ePSNAVS_Streaming,
	ePSNAVS_Error
};

class CCryPSNVoice : public CCryVoice
{
public:
																	CCryPSNVoice(CCryLobby* lobby, CCryLobbyService* service);

					ECryLobbyError					Initialise();
					ECryLobbyError					Terminate();
					void										Tick(CTimeValue tv);

					void										StartVoice(SceNpMatching2ContextId ctx,SceNpMatching2RoomId room);
					void										StopVoice();

private:

	// OS Callbacks

	static void											ChatEventCallback(CellSysutilAvc2EventId eventId, CellSysutilAvc2EventParam eventParam, void *userData );

	// State transitions
	void														LibraryStartup();
	void														LibraryStop();
	void														JoinGame();
	void														LeaveGame();
	void														StartStreaming();
	void														StopStreaming();

	struct	SLocalUser
	{
					int32										m_dummy;
	};

	struct	SRemoteUser : public CCryVoice::SRemoteUser
	{
	};

	struct	SUserRelationship : public CCryVoice::SUserRelationship
	{
	};

		EPSNAVState							m_curState;
		EPSNAVState							m_wantedState;
					
		SLocalUser							m_localUsers[MAX_PSN_LOCAL_TALKERS];
		SRemoteUser							m_remoteUsers[MAX_PSN_REMOTE_TALKERS];
		SUserRelationship				m_userRelationship[MAX_PSN_LOCAL_TALKERS][MAX_PSN_REMOTE_TALKERS];

		SceNpMatching2ContextId	m_ctx;
		SceNpMatching2RoomId		m_room;

};

#endif//USE_PSN_VOICE

#endif // __CRYPSNVOICE_H__

