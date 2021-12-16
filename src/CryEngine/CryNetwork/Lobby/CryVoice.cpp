#include "StdAfx.h"
#include "CryVoice.h"

CCryVoice::CCryVoice(CCryLobby* lobby, CCryLobbyService* service)
{
	assert(lobby);
	assert(service);

	m_pLobby = lobby;
	m_pService = service;

	for (uint32 i = 0; i < MAX_REMOTE_TALKERS; i++)
	{
		m_pRemoteUsers[i] = NULL;
	}

	for (uint32 i = 0; i < MAX_LOCAL_TALKERS; i++)
	{
		for (uint32 j = 0; j < MAX_REMOTE_TALKERS; j++)
		{
			m_pUserRelationship[i][j] = NULL;
		}
	}
}

ECryLobbyError CCryVoice::Initialise()
{
	for (uint32 i = 0; i < MAX_REMOTE_TALKERS; i++)
	{
		assert(m_pRemoteUsers[i]);
		m_pRemoteUsers[i]->registered = false;
	}

	return eCLE_Success;
}

CryVoiceRemoteUserID CCryVoice::RegisterRemoteUser(CryLobbyConnectionID connectionID, CryMatchMakingConnectionUID uid, uint32 localUser)
{
	for (uint32 i = 0; i < MAX_REMOTE_TALKERS; i++)
	{
		SRemoteUser* remoteUser = m_pRemoteUsers[i];

		assert(remoteUser);

		if (!remoteUser->registered)
		{
			remoteUser->registered = true;
			remoteUser->connectionID = connectionID;
			remoteUser->matchMakingUID = uid;
			remoteUser->localUser = localUser;
			remoteUser->isSpeaking = false;

			for (uint32 j = 0; j < MAX_LOCAL_TALKERS; j++)
			{
				SUserRelationship* relationship = m_pUserRelationship[j][i];

				assert(relationship);

				relationship->mutedGame = false;
				relationship->mutedExternal = false;
				relationship->playbackOn = true;
			}

			return i;
		}
	}

	return CryVoiceInvalidRemoteUserID;
}

void CCryVoice::UnRegisterRemoteUser(CryVoiceRemoteUserID user)
{
	assert(m_pRemoteUsers[user]);

	m_pRemoteUsers[user]->registered = false;
}

CryVoiceRemoteUserID CCryVoice::GetRemoteUser(uint32 channelID, uint32 user)
{
	for (uint32 i = 0; i < MAX_REMOTE_TALKERS; i++)
	{
		SRemoteUser* remoteUser = m_pRemoteUsers[i];

		assert(remoteUser);

		if (remoteUser->registered && (remoteUser->matchMakingUID == channelID) && (remoteUser->localUser == user))
		{
			return i;
		}
	}

	return CryVoiceInvalidRemoteUserID;
}

void CCryVoice::Mute(uint32 localUser, uint32 channelID, uint32 user, bool mute)
{
	CryVoiceRemoteUserID remoteUserID = GetRemoteUser(channelID, user);

	if (remoteUserID != CryVoiceInvalidRemoteUserID)
	{
		assert(m_pUserRelationship[localUser][remoteUserID]);
		m_pUserRelationship[localUser][remoteUserID]->mutedGame = mute;
	}
}

bool CCryVoice::IsMuted(uint32 localUser, uint32 channelID, uint32 user)
{
	CryVoiceRemoteUserID remoteUserID = GetRemoteUser(channelID, user);

	if (remoteUserID != CryVoiceInvalidRemoteUserID)
	{
		assert(m_pUserRelationship[localUser][remoteUserID]);
		return m_pUserRelationship[localUser][remoteUserID]->mutedGame;
	}

	return false;
}

bool CCryVoice::IsMutedExternally(uint32 localUser, uint32 channelID, uint32 user)
{
	CryVoiceRemoteUserID remoteUserID = GetRemoteUser(channelID, user);

	if (remoteUserID != CryVoiceInvalidRemoteUserID)
	{
		assert(m_pUserRelationship[localUser][remoteUserID]);
		return m_pUserRelationship[localUser][remoteUserID]->mutedExternal;
	}

	return false;
}

bool CCryVoice::IsSpeaking(uint32 localUser, uint32 channelID, uint32 user)
{
	CryVoiceRemoteUserID remoteUserID = GetRemoteUser(channelID, user);

	if (remoteUserID != CryVoiceInvalidRemoteUserID)
	{
		assert(m_pRemoteUsers[remoteUserID]);
		assert(m_pUserRelationship[localUser][remoteUserID]);

		return m_pRemoteUsers[remoteUserID]->isSpeaking && m_pUserRelationship[localUser][remoteUserID]->playbackOn;
	}

	return false;
}
