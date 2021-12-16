#ifndef __CRYLIVEVOICE_H__
#define __CRYLIVEVOICE_H__

#pragma once

#include <xhv2.h>
#include "Lobby/CryVoice.h"

class CCryLiveMatchMaking;

#define MAX_LIVE_REMOTE_TALKERS		16
#define VOICE_BUFFER_SIZE					(XHV_VOICECHAT_MODE_PACKET_SIZE*XHV_MAX_VOICECHAT_PACKETS)

class CCryLiveVoice : public CCryVoice
{
public:
																	CCryLiveVoice(CCryLobby* lobby, CCryLobbyService* service);

					ECryLobbyError					Initialise();
					ECryLobbyError					Terminate();
					void										Tick(CTimeValue tv);
					void										OnPacket(const TNetAddress& addr, const uint8* data, uint32 length);

					void										RegisterLocalUsers(bool users[MAX_LIVE_LOCAL_USERS]);
					CryVoiceRemoteUserID		RegisterRemoteUser(XUID xuid, CryLobbyConnectionID connectionID, CryMatchMakingConnectionUID uid, uint32 localUser, uint32 remoteUser);
					void										UnregisterRemoteUser(XUID xuid);
					void										UpdateMuteList();

private:
	struct	SLocalUser
	{
					XUID										xuid;
					uint32									timeSinceSend;
					uint32									bufferSize;
					uint8										buffer[VOICE_BUFFER_SIZE];
					bool										registered;
					bool										loopback;
	};

	struct	SRemoteUser : public CCryVoice::SRemoteUser
	{
					XUID										xuid;
					uint32									timeSinceRecv;
					uint8										remoteUser;
	};

	struct	SUserRelationship : public CCryVoice::SUserRelationship
	{
					bool										sendingTo;
	};

					CryVoiceRemoteUserID		FindRemoteUser(XUID xuid);
					void										SendVoiceData(int user);
					void										ProcessVoice(const TNetAddress& addr, const uint8* data, uint32 length);

					CTimeValue							m_lastTickTime;
					SLocalUser							m_localUsers[MAX_LIVE_LOCAL_USERS];
					SRemoteUser							m_remoteUsers[MAX_LIVE_REMOTE_TALKERS];
					SUserRelationship				m_userRelationship[MAX_LIVE_LOCAL_USERS][MAX_LIVE_REMOTE_TALKERS];
					PIXHV2ENGINE						m_XHV;
};

#endif // __CRYLIVEVOICE_H__

