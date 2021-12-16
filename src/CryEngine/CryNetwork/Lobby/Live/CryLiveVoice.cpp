#include "StdAfx.h"
#include "CryLiveLobby.h"
#include "CryLiveVoice.h"

#define MAX_VOICE_BUFFER_TIME	200
#define LIVE_VOICE_SEND_THRESHOLD ((7*VOICE_BUFFER_SIZE)/10)

CCryLiveVoice::CCryLiveVoice(CCryLobby* lobby, CCryLobbyService* service) : CCryVoice(lobby, service)
{
	m_XHV = NULL;

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

ECryLobbyError CCryLiveVoice::Initialise()
{
	ECryLobbyError error = CCryVoice::Initialise();

	if (error == eCLE_Success)
	{
		IXAudio2* xaudio2 = NULL;
		HRESULT hr;
		IXAudio2MasteringVoice* masteringVoice = 0;

		hr = XAudio2Create(&xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);

		if (xaudio2)
		{
			hr = xaudio2->CreateMasteringVoice(&masteringVoice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, 0, 0);
		}

		XHV_PROCESSING_MODE rgModes[] = {XHV_VOICECHAT_MODE, XHV_LOOPBACK_MODE};

		// Set up parameters for the voice chat engine
		XHV_INIT_PARAMS xhvParams               = {0};
		xhvParams.dwMaxRemoteTalkers            = XHV_MAX_REMOTE_TALKERS;
		xhvParams.dwMaxLocalTalkers             = XHV_MAX_LOCAL_TALKERS;
		xhvParams.localTalkerEnabledModes       = rgModes;
		xhvParams.remoteTalkerEnabledModes      = rgModes;
		xhvParams.dwNumLocalTalkerEnabledModes  = 2;
		xhvParams.dwNumRemoteTalkerEnabledModes = 1;
		xhvParams.pXAudio2                      = xaudio2;

		// Create the XHV2 engine
		hr = XHV2CreateEngine(&xhvParams, NULL, &m_XHV);

		for (uint32 i = 0; i < MAX_LIVE_LOCAL_USERS; i++)
		{
			m_localUsers[i].registered = false;
		}

		m_lastTickTime = g_time;
	}

	return error;
}

ECryLobbyError CCryLiveVoice::Terminate()
{
	if (m_XHV)
	{
		m_XHV->Release();
		m_XHV = NULL;
	}

	return eCLE_Success;
}

void CCryLiveVoice::RegisterLocalUsers(bool users[MAX_LIVE_LOCAL_USERS])
{
	if (m_XHV)
	{
		HRESULT hr;

		// Register everybody who's logged in and is capable of doing voice
		for (uint32 i = 0; i < MAX_LIVE_LOCAL_USERS; i++)
		{
			SLocalUser* user = &m_localUsers[i];

			if (users[i])
			{
				if (!user->registered)
				{
					hr = m_XHV->RegisterLocalTalker(i);

					if (hr == S_OK)
					{
						hr = m_XHV->StartLocalProcessingModes(i, &XHV_VOICECHAT_MODE, 1);

						if (hr == S_OK)
						{
							hr = m_XHV->StopLocalProcessingModes(i, &XHV_LOOPBACK_MODE, 1);

							if (hr == S_OK)
							{
								XUSER_SIGNIN_INFO info;
								DWORD error = XUserGetSigninInfo(i, XUSER_GET_SIGNIN_INFO_ONLINE_XUID_ONLY, &info);

								if (error == ERROR_SUCCESS)
								{
									user->xuid = info.xuid;
									user->registered = true;
									user->loopback = false;
									user->bufferSize = 0;
									user->timeSinceSend = 0;
								}
								else
								{
									NetLog("[Lobby] Failed to get xuid for local talker %d, error 0x%08x\n", i, error);
								}
							}
							else
							{
								NetLog("[Lobby] Failed to set processing mode for local talker %d, error 0x%08x\n", i, hr);
							}
						}
						else
						{
							NetLog("[Lobby] Failed to set processing mode for local talker %d, error 0x%08x\n", i, hr);
						}

						if (!user->registered)
						{
							// Got an error after registering so unregister
							hr = m_XHV->UnregisterLocalTalker(i);
							
							if (hr != S_OK)
							{
								NetLog("[Lobby] Failed to unregister local talker %d, error 0x%08x\n", i, hr);
							}
						}
					}
					else
					{
						NetLog("[Lobby] Failed to register local talker %d, error 0x%08x\n", i, hr);
					}
				}
			}
			else
			{
				if (user->registered)
				{
					hr = m_XHV->UnregisterLocalTalker(i);

					if (hr == S_OK)
					{
						user->registered = false;
					}
					else
					{
						NetLog("[Lobby] Failed to unregister local talker %d, error 0x%08x\n", i, hr);
					}
				}
			}
		}

		UpdateMuteList();
	}
}

CryVoiceRemoteUserID CCryLiveVoice::RegisterRemoteUser(XUID xuid, CryLobbyConnectionID connectionID, CryMatchMakingConnectionUID uid, uint32 localUser, uint32 remoteUser)
{
	if (m_XHV)
	{
		CryVoiceRemoteUserID userID = FindRemoteUser(xuid);

		if (userID != CryVoiceInvalidRemoteUserID)
		{
			// Already registered
			return userID;
		}

		userID =  CCryVoice::RegisterRemoteUser(connectionID, uid, localUser);

		if (userID != CryVoiceInvalidRemoteUserID)
		{
			SRemoteUser* user = &m_remoteUsers[userID];
			HRESULT hr = m_XHV->RegisterRemoteTalker(xuid, NULL, NULL, NULL);

			if (hr == S_OK)
			{
				hr = m_XHV->StartRemoteProcessingModes(xuid, &XHV_VOICECHAT_MODE, 1);

				if (hr == S_OK)
				{
					user->xuid = xuid;
					user->remoteUser = remoteUser;

					for (uint32 j = 0; j < MAX_LIVE_LOCAL_USERS; j++)
					{
						m_userRelationship[j][userID].sendingTo = true;
						m_userRelationship[j][userID].playbackOn = false;
					}

					UpdateMuteList();

					return userID;
				}
				else
				{
					NetLog("[Lobby] Failed to set processing modes for remote user %llx, error 0x%08x\n", xuid, hr);
				}
			}
			else
			{
				NetLog("[Lobby] Failed to register remote user %llx, error 0x%08x\n", xuid, hr);
			}

			CCryVoice::UnRegisterRemoteUser(userID);
		}
	}

	return CryVoiceInvalidRemoteUserID;
}

CryVoiceRemoteUserID CCryLiveVoice::FindRemoteUser(XUID xuid)
{
	for (uint32 i = 0; i < MAX_LIVE_REMOTE_TALKERS; i++)
	{
		if (m_remoteUsers[i].registered && (m_remoteUsers[i].xuid == xuid))
		{
			return i;
		}
	}

	return CryVoiceInvalidRemoteUserID;
}

void CCryLiveVoice::UnregisterRemoteUser(XUID xuid)
{
	if (m_XHV)
	{
		for (uint32 i = 0; i < MAX_LIVE_REMOTE_TALKERS; i++)
		{
			SRemoteUser* user = &m_remoteUsers[i];

			if (user->registered && (user->xuid == xuid))
			{
				HRESULT hr = m_XHV->UnregisterRemoteTalker(user->xuid);

				if (hr == S_OK)
				{
					CCryVoice::UnRegisterRemoteUser(i);
				}
				else
				{
					NetLog("[Lobby] Failed to unregister remote user %llx, error 0x%08x\n", user->xuid, hr);
				}

				break;
			}
		}
	}
}

void CCryLiveVoice::UpdateMuteList()
{
	if (m_XHV)
	{
		for (uint32 i = 0; i < MAX_LIVE_LOCAL_USERS; i++)
		{
			SLocalUser* localUser = &m_localUsers[i];

			if (localUser->registered)
			{
				for (uint32 j = 0; j < MAX_LIVE_REMOTE_TALKERS; j++)
				{
					SRemoteUser* remoteUser = &m_remoteUsers[j];

					if (remoteUser->registered)
					{
						SUserRelationship* relationship = &m_userRelationship[i][j];
						BOOL muted = FALSE;
						DWORD ret;

						ret = XUserMuteListQuery(i, remoteUser->xuid, &muted);

						if (ret == ERROR_SUCCESS)
						{
							relationship->mutedExternal = muted;
						}
						else
						{
							NetLog("[Lobby] XUserMuteListQuery() returned 0x%08x for user %d\n", ret, i);
						}
					}
				}
			}
		}
	}
}

void CCryLiveVoice::Tick(CTimeValue tv)
{
	if (m_XHV)
	{
		// Get the users that have voice data available
		DWORD flags = m_XHV->GetDataReadyFlags();
		uint32 tickTime = (uint32)(tv.GetMilliSecondsAsInt64() - m_lastTickTime.GetMilliSecondsAsInt64());

		m_lastTickTime = tv;

		// Look for any new incoming data
		for (uint32 i = 0; i < MAX_LIVE_LOCAL_USERS; i++)
		{
			SLocalUser* user = &m_localUsers[i];

			if (user->registered)
			{
				user->timeSinceSend += tickTime;

				if (flags&(1<<i))
				{
					// Buffer the received voice data
					DWORD numPackets;
					DWORD bytes;

					bytes = VOICE_BUFFER_SIZE - user->bufferSize;

					if (bytes >= XHV_VOICECHAT_MODE_PACKET_SIZE)
					{
						HRESULT hr = m_XHV->GetLocalChatData(i, user->buffer + user->bufferSize, &bytes, &numPackets);

						if ((hr != S_OK) && (hr != E_PENDING))
						{
							NetLog("[Lobby] GetLocalChatData Failed error 0x%08x\n", hr);
						}

						user->bufferSize += bytes;
					}
				}

				if ((user->bufferSize > LIVE_VOICE_SEND_THRESHOLD) || (user->timeSinceSend > MAX_VOICE_BUFFER_TIME))
				{
					SendVoiceData(i);
					user->bufferSize = 0;
					user->timeSinceSend = 0;
				}
			}
		}

		for (uint32 i = 0; i < MAX_LIVE_REMOTE_TALKERS; i++)
		{
			SRemoteUser* remoteUser = &m_remoteUsers[i];

			if (remoteUser->registered)
			{
				remoteUser->timeSinceRecv += tickTime;
				remoteUser->isSpeaking = m_XHV->IsRemoteTalking(remoteUser->xuid);
			}
		}
	}
}

void CCryLiveVoice::SendVoiceData(int userNum)
{
	SLocalUser* localUser = &m_localUsers[userNum];

	if (localUser->bufferSize > 0)
	{
		const uint32 bufferSize = PacketHeaderSize + PacketUINT8Size + VOICE_BUFFER_SIZE + PacketUINT8Size;
		uint8 buffer[bufferSize];
		uint32 bufferPos;
		bool stillToSend[MAX_LIVE_REMOTE_TALKERS];
		uint32 remoteUsersPos;

		StartAddPacket(buffer, bufferSize, &bufferPos, eLivePT_Voice);
		AddPacketUINT8(buffer, bufferSize, &bufferPos, localUser->bufferSize);
		AddPacketData(buffer, bufferSize, &bufferPos, localUser->buffer, localUser->bufferSize);

		remoteUsersPos = bufferPos;

		// Initialise stillToSend array
		for (uint32 i = 0; i < MAX_LIVE_REMOTE_TALKERS; i++)
		{
			SRemoteUser* remoteUser = &m_remoteUsers[i];
			SUserRelationship* relationship = &m_userRelationship[userNum][i];

			// Set to true if we want to send to this user
			stillToSend[i] = remoteUser->registered && !relationship->mutedGame && !relationship->mutedExternal && relationship->sendingTo;
		}

		for (uint32 i = 0; i < MAX_LIVE_REMOTE_TALKERS; i++)
		{
			if (stillToSend[i])
			{
				SRemoteUser* remoteUser = &m_remoteUsers[i];
				TNetAddress to;

				if (m_pLobby->AddressFromConnection(to, remoteUser->connectionID))
				{
					// Bottom 4 bits of flags holds the users on the remote machine that this voice data is for.
					// Next 2 bits hold the user number on this machine that this voice data is from.
					uint8 flags = userNum<<4;

					bufferPos = remoteUsersPos;

					flags |= (1<<remoteUser->localUser);
					stillToSend[i] = false;

					// Check to see if we should be sending to other users on the same connection as this user.
					for (uint32 j = i + 1; j < MAX_LIVE_REMOTE_TALKERS; j++)
					{
						if (stillToSend[j] && (remoteUser->connectionID == m_remoteUsers[j].connectionID))
						{
							flags |= (1<<m_remoteUsers[j].localUser);
							stillToSend[j] = false;
						}
					}

					AddPacketUINT8(buffer, bufferSize, &bufferPos, flags);

					m_pLobby->SendVoice(buffer, bufferPos, to);
				}
			}
		}
	}
}

void CCryLiveVoice::ProcessVoice(const TNetAddress& addr, const uint8* data, uint32 length)
{
	CCryLiveMatchMaking* matchMaking = (CCryLiveMatchMaking*)m_pService->GetMatchMaking();

	if (m_XHV && matchMaking)
	{
		CryLobbyConnectionID connectionID;

		if (m_pLobby->ConnectionFromAddress(&connectionID, addr))
		{
			uint8 voiceData[VOICE_BUFFER_SIZE];
			uint8 voiceDataSize;
			uint32 bufferPos;
			uint32 fromRemoteUser;
			uint32 fromLocalUser;
			XUID fromXUID;
			CryVoiceRemoteUserID remoteUserIndex;
			CryMatchMakingConnectionUID uid;
			uint8 flags;

			StartRemovePacket(data, length, &bufferPos);
			RemovePacketUINT8(data, length, &bufferPos, &voiceDataSize);
			RemovePacketData(data, length, &bufferPos, voiceData, voiceDataSize);
			RemovePacketUINT8(data, length, &bufferPos, &flags);

			fromRemoteUser = flags>>4;

			if (matchMaking->GetRemoteUserConnectionInfo(connectionID, fromRemoteUser, &uid, &fromXUID, &fromLocalUser))
			{
				remoteUserIndex = FindRemoteUser(fromXUID);

				if (remoteUserIndex == CryVoiceInvalidRemoteUserID)
				{
					// We don't have this user registered so try and register now.
					remoteUserIndex = RegisterRemoteUser(fromXUID, connectionID, uid, fromLocalUser, fromRemoteUser);
				}

				if (remoteUserIndex != CryVoiceInvalidRemoteUserID)
				{
					SRemoteUser* remoteUser = &m_remoteUsers[remoteUserIndex];

					remoteUser->timeSinceRecv = 0;

					for (uint32 i = 0; i < MAX_LIVE_LOCAL_USERS; i++)
					{
						SLocalUser* localUser = &m_localUsers[i];

						if (localUser->registered)
						{
							SUserRelationship* relationship = &m_userRelationship[i][remoteUserIndex];
							bool forThisUser = flags&(1<<i);

							if (forThisUser && !relationship->mutedGame && !relationship->mutedExternal)
							{
								if (!relationship->playbackOn)
								{
									HRESULT hr = m_XHV->SetPlaybackPriority(remoteUser->xuid, i, XHV_PLAYBACK_PRIORITY_MAX);

									if (hr == S_OK)
									{
										relationship->playbackOn = true;
									}
									else
									{
										NetLog("[Lobby] Failed to set playback priority for remote user %llx, error 0x%08x\n", remoteUser->xuid, hr);
									}
								}
							}
							else
							{
								if (relationship->playbackOn)
								{
									HRESULT hr = m_XHV->SetPlaybackPriority(remoteUser->xuid, i, XHV_PLAYBACK_PRIORITY_NEVER);

									if (hr == S_OK)
									{
										relationship->playbackOn = false;
									}
									else
									{
										NetLog("[Lobby] Failed to set playback priority for remote user %llx, error 0x%08x\n", remoteUser->xuid, hr);
									}
								}
							}
						}
					}

					DWORD size = voiceDataSize;
					HRESULT hr = m_XHV->SubmitIncomingChatData(remoteUser->xuid, voiceData, &size);

					if (hr != S_OK)
					{
						NetLog("[Lobby] SubmitIncomingChatData Failed for remote user %llx, error 0x%08x\n", remoteUser->xuid, hr);
					}
				}
			}
		}
	}
}

void CCryLiveVoice::OnPacket(const TNetAddress& addr, const uint8* data, uint32 length)
{
	CryLiveLobbyPacketType type;
	uint32 pos;

	StartRemovePacket(data, length, &pos, &type);

	switch (type)
	{
	case eLivePT_Voice:
		ProcessVoice(addr, data, length);
		break;
	}
}
