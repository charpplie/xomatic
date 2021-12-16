#ifndef __CRYVOICE_H__
#define __CRYVOICE_H__

#pragma once

#include "ICryVoice.h"
#include "Lobby/CryLobby.h"
#include "Lobby/CryMatchMaking.h"

#define MAX_LOCAL_TALKERS			4
#define MAX_REMOTE_TALKERS		16

typedef uint32 CryVoiceRemoteUserID;
const CryVoiceRemoteUserID CryVoiceInvalidRemoteUserID = 0xffffffff;

class CCryVoice : public CMultiThreadRefCount, public ICryVoice
{
public:
																	CCryVoice(CCryLobby* lobby, CCryLobbyService* service);

	virtual	void										Mute(uint32 localUser, uint32 channelID, uint32 user, bool mute);
	virtual	bool										IsMuted(uint32 localUser, uint32 channelID, uint32 user);
	virtual	bool										IsMutedExternally(uint32 localUser, uint32 channelID, uint32 user);
	virtual bool										IsSpeaking(uint32 localUser, uint32 channelID, uint32 user);

protected:
					ECryLobbyError					Initialise();
					CryVoiceRemoteUserID		RegisterRemoteUser(CryLobbyConnectionID connectionID, CryMatchMakingConnectionUID uid, uint32 localUser);
					void										UnRegisterRemoteUser(CryVoiceRemoteUserID user);
					CryVoiceRemoteUserID		GetRemoteUser(uint32 channelID, uint32 user);

	struct	SRemoteUser
	{
		CryLobbyConnectionID					connectionID;
		CryMatchMakingConnectionUID		matchMakingUID;
		uint8													localUser;
		bool													registered;
		bool													isSpeaking;
	};

	struct	SUserRelationship
	{
		bool													mutedGame;
		bool													mutedExternal;
		bool													playbackOn;
	};

	CCryLobby*											m_pLobby;
	CCryLobbyService*								m_pService;
	SRemoteUser*										m_pRemoteUsers[MAX_REMOTE_TALKERS];
	SUserRelationship*							m_pUserRelationship[MAX_LOCAL_TALKERS][MAX_REMOTE_TALKERS];
};

#endif // __CRYVOICE_H__

