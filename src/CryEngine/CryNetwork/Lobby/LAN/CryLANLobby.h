#ifndef __CRYLANLOBBY_H__
#define __CRYLANLOBBY_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "Lobby/CryLobby.h"
#include "Lobby/CryTelemetry.h"
#include "Lobby/LAN/CryLANMatchMaking.h"

enum CryLANLobbyPacketType
{
	// Keep LAN packet types away from online types to stop packets being processed incorrectly when service is changed
	// and any tasks running not being canceled correctly.
	eLANPT_StartType = 200,

	eLANPT_MM_RequestServerData = eLANPT_StartType,
	eLANPT_MM_ServerData,
	eLANPT_SessionRequestJoin,
	eLANPT_SessionRequestJoinResult,
	eLANPT_SessionAddRemoteConnections,
	eLANPT_HostMigrationServer,
	eLANPT_HostMigrationClient,

	eLANPT_EndType
};

class CCryLANLobbyService : public CCryLobbyService
{
public:
	virtual ECryLobbyError		Initialise(CryLobbyServiceCallback cb, CCryLobby* cbArg, ECryLobbyService service);
	virtual ECryLobbyError		Terminate(CryLobbyServiceCallback cb, CCryLobby* cbArg, ECryLobbyService service);
	virtual void							Tick(CTimeValue tv);
	virtual ICryMatchMaking*	GetMatchMaking()										{ return m_matchmaking; }
	virtual ICryVoice*				GetVoice()													{ return NULL; }
	virtual ICryTelemetry*		GetTelemetry()											{ return m_telemetry; }
	virtual ICryStats*				GetStats()													{ return NULL; }
	virtual	CryUserID					GetUserID(uint32 user)							{ return CryUserInvalidID; }

	virtual void							OnPacket(const TNetAddress& addr, const uint8* data, uint32 length);
	virtual void							OnError(const TNetAddress& addr, ESocketError error, CryLobbySendID sendID);
	virtual void							OnSendComplete(const TNetAddress& addr, CryLobbySendID sendID);

private:
	_smart_ptr<CCryLANMatchMaking>	m_matchmaking;
	_smart_ptr<CCryTelemetry>				m_telemetry;
};

inline void StartAddPacket(uint8* buffer, uint32 bufferSize, uint32* bufferPos, CryLANLobbyPacketType type)
{
	StartAddPacket(buffer, bufferSize, bufferPos, (CryLobbyPacketType)type);
}

inline void StartRemovePacket(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, CryLANLobbyPacketType* type)
{
	StartRemovePacket(buffer, bufferSize, bufferPos, (CryLobbyPacketType*)type);
}

#endif // __CRYLANLOBBY_H__
