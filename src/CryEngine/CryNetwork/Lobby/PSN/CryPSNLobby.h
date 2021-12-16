#ifndef __CRYPSNLOBBY_H__
#define __CRYPSNLOBBY_H__

#pragma once

#include "Lobby/CryLobby.h"

#if USE_PSN

#include "Lobby/PSN/CryPSNMatchMaking.h"
#include "Lobby/PSN/CryPSNVoice.h"

#include <np.h>

#include "Lobby/CryTelemetry.h"

class CCryPSNLobbyService : public CCryLobbyService
{
public:
														CCryPSNLobbyService();

	virtual ECryLobbyError		Initialise(CryLobbyServiceCallback cb, CCryLobby* cbArg, ECryLobbyService service);
	virtual ECryLobbyError		Terminate(CryLobbyServiceCallback cb, CCryLobby* cbArg, ECryLobbyService service);
	virtual void							Tick(CTimeValue tv);
	virtual ICryMatchMaking*	GetMatchMaking()										{ return m_matchmaking; }
	virtual ICryVoice*				GetVoice()													{
#if USE_PSN_VOICE
																																	return m_voice; 
#else//USE_PSN_VOICE
																																	return NULL;
#endif//USE_PSN_VOICE
																																}
	virtual ICryTelemetry*		GetTelemetry()											{ return m_telemetry; }
	virtual ICryStats*				GetStats()													{ return NULL; }
	virtual	CryUserID					GetUserID(uint32 user)							{ return CryUserInvalidID; }

	virtual void							OnPacket(const TNetAddress& addr, const uint8* data, uint32 length);
	virtual void							OnError(const TNetAddress& addr, ESocketError error, CryLobbySendID sendID);
	virtual void							OnSendComplete(const TNetAddress& addr, CryLobbySendID sendID);

	void											GetConfigurationInformation(SConfigurationParams* infos,uint32 cnt);

private:
	void											InviteAccepted(uint32 user, CrySessionID id);

	CCryLobby*											m_lobby;
	ECryLobbyService								m_service;
	_smart_ptr<CCryPSNMatchMaking>	m_matchmaking;
#if USE_PSN_VOICE
	_smart_ptr<CCryPSNVoice>				m_voice;
#endif//USE_PSN_VOICE
	_smart_ptr<CCryTelemetry>				m_telemetry;

	bool														m_NPInited;
	
	uint8														m_NPPool[SCE_NP_MIN_POOL_SIZE];
};

#endif // USE_PSN

#endif//__CRYPSNLOBBY_H__
