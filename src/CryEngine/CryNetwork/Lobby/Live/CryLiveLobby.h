#ifndef __CRYLIVELOBBY_H__
#define __CRYLIVELOBBY_H__

#if _MSC_VER > 1000
#pragma once
#endif

#define MAX_LIVE_PACKET_SIZE	1264
#define MAX_LIVE_LOCAL_USERS	4

#include "Lobby/CryLobby.h"
#if USE_GFWL
#include <winlive.h>
#endif
#include <Xonline.h>
#include "Network.h"
#include "Lobby/Live/CryLiveMatchMaking.h"
#include "Lobby/Live/CryLiveVoice.h"
#include "Lobby/Live/CryXLSPTelemetry.h"
#include "Lobby/Live/CryLiveStats.h"

enum CryLiveLobbyPacketType
{
	eLivePT_SessionRequestJoin = eLobbyPT_EndType,
	eLivePT_SessionRequestJoinResult,
	eLivePT_SessionAddRemoteConnections,
	eLivePT_SessionStart,
	eLivePT_SessionIHaveRegistered,
	eLivePT_SessionRegistrationFinished,
	eLivePT_SessionEnd,
	eLivePT_Voice,
	eLivePT_HostMigrationServer,
	eLivePT_HostMigrationClient,

	eLivePT_EndType
};

struct SCryLiveUserID : public SCryUserID
{
	XUID	xuid;
};

#if USE_GFWL
class CCryLobbyGFWLExtras : public ICryLobbyGFWLExtras
{
public:
									CCryLobbyGFWLExtras();
	virtual void		OnCreateDevice(void* pD3D, void* pD3DPP);
	virtual void		OnDeviceReset(void* pD3DPP);
	virtual void		OnDeviceDestroyed();
	virtual void		Render();
	virtual uint32	PreTranslateMessage(void* msg);

private:
	bool						m_initialised;
};
#endif

class CCryLiveLobbyService : public CCryLobbyService
{
public:
														CCryLiveLobbyService();

	virtual ECryLobbyError		Initialise(CryLobbyServiceCallback cb, CCryLobby* cbArg, ECryLobbyService service);
	virtual ECryLobbyError		Terminate(CryLobbyServiceCallback cb, CCryLobby* cbArg, ECryLobbyService service);
	virtual void							Tick(CTimeValue tv);
	virtual ICryMatchMaking*	GetMatchMaking()										{ return m_matchmaking; }
	virtual ICryVoice*				GetVoice()													{ return m_voice; }
	virtual ICryTelemetry*		GetTelemetry()											{ return m_telemetry; }
	virtual ICryStats*				GetStats()													{ return m_pStats; }
	virtual	CryUserID					GetUserID(uint32 user);
					ECryLobbyError		GetUserXUID(uint32 user, XUID* pXUID);

	virtual void							OnPacket(const TNetAddress& addr, const uint8* data, uint32 length);
	virtual void							OnError(const TNetAddress& addr, ESocketError error, CryLobbySendID sendID);
	virtual void							OnSendComplete(const TNetAddress& addr, CryLobbySendID sendID);

private:
	void											InviteAccepted(uint32 user, CrySessionID id);

	CCryLobby*											m_lobby;
	ECryLobbyService								m_service;
	HANDLE													m_listener;
	_smart_ptr<CCryLiveMatchMaking>	m_matchmaking;
	_smart_ptr<CCryLiveVoice>				m_voice;
	_smart_ptr<CCryTelemetry>				m_telemetry;
	_smart_ptr<CCryLiveStats>				m_pStats;
};

struct SLiveUserData 
{
	union
	{
		XUSER_CONTEXT		context;
		XUSER_PROPERTY	property;
	};

	bool typeContext;
};

extern ECryLobbyError CryUserDataToLiveUserData(SCryLobbyUserData* pCryData, SLiveUserData* pLiveData);
extern ECryLobbyError LiveUserDataToCryUserData(SLiveUserData* pLiveData, SCryLobbyUserData* pCryData, ECryLobbyUserDataType type);
extern ECryLobbyError CryLiveLobbyGetErrorFromLive(DWORD error);

inline void StartAddPacket(uint8* buffer, uint32 bufferSize, uint32* bufferPos, CryLiveLobbyPacketType type)
{
	StartAddPacket(buffer, bufferSize, bufferPos, (CryLobbyPacketType)type);
}

inline void StartRemovePacket(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, CryLiveLobbyPacketType* type)
{
	StartRemovePacket(buffer, bufferSize, bufferPos, (CryLobbyPacketType*)type);
}

const uint32 PacketDWORDSize	= PacketUINT32Size;
inline void AddPacketDWORD(uint8* buffer, uint32 bufferSize, uint32* bufferPos, DWORD data)
{
	AddPacketUINT32(buffer, bufferSize, bufferPos, (uint32)data);
}

inline void RemovePacketDWORD(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, DWORD* data)
{
	RemovePacketUINT32(buffer, bufferSize, bufferPos, (uint32*)data);
}

const uint32 PacketXUIDSize	= sizeof(XUID);
inline void AddPacketXUID(uint8* buffer, uint32 bufferSize, uint32* bufferPos, XUID data)
{
	assert(*bufferPos + PacketXUIDSize <= bufferSize);

	if (*bufferPos + PacketXUIDSize <= bufferSize)
	{
		memcpy(&buffer[*bufferPos], &data, sizeof(XUID));
		*bufferPos += sizeof(XUID);
	}
}

inline void RemovePacketXUID(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, XUID* data)
{
	assert(*bufferPos + PacketXUIDSize <= bufferSize);

	if (*bufferPos + PacketXUIDSize <= bufferSize)
	{
		memcpy(data, &buffer[*bufferPos], sizeof(XUID));
		*bufferPos += sizeof(XUID);
	}
}

const uint32 PacketBOOLSize	= 1;
inline void AddPacketBOOL(uint8* buffer, uint32 bufferSize, uint32* bufferPos, BOOL data)
{
	assert(*bufferPos + PacketBOOLSize <= bufferSize);

	if (*bufferPos + PacketBOOLSize <= bufferSize)
	{
		buffer[(*bufferPos)++] = data ? 1 : 0;
	}
}

inline void RemovePacketBOOL(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, BOOL* data)
{
	assert(*bufferPos + PacketBOOLSize <= bufferSize);

	if (*bufferPos + PacketBOOLSize <= bufferSize)
	{
		*data = buffer[(*bufferPos)++];
	}
}

const uint32 PacketXNKIDSize	= sizeof(XNKID);
inline void AddPacketXNKID(uint8* buffer, uint32 bufferSize, uint32* bufferPos, XNKID data)
{
	assert(*bufferPos + PacketXNKIDSize <= bufferSize);

	if (*bufferPos + PacketXNKIDSize <= bufferSize)
	{
		memcpy(&buffer[*bufferPos], &data, sizeof(XNKID));
		*bufferPos += sizeof(XNKID);
	}
}

inline void RemovePacketXNKID(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, XNKID* data)
{
	assert(*bufferPos + PacketXNKIDSize <= bufferSize);

	if (*bufferPos + PacketXNKIDSize <= bufferSize)
	{
		memcpy(data, &buffer[*bufferPos], sizeof(XNKID));
		*bufferPos += sizeof(XNKID);
	}
}

const uint32 PacketNONCESize	= sizeof(ULONGLONG);
inline void AddPacketNONCE(uint8* buffer, uint32 bufferSize, uint32* bufferPos, ULONGLONG data)
{
	assert(*bufferPos + PacketNONCESize <= bufferSize);

	if (*bufferPos + PacketNONCESize <= bufferSize)
	{
		memcpy(&buffer[*bufferPos], &data, sizeof(ULONGLONG));
		*bufferPos += sizeof(ULONGLONG);
	}
}

inline void RemovePacketNONCE(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, ULONGLONG* data)
{
	assert(*bufferPos + PacketNONCESize <= bufferSize);

	if (*bufferPos + PacketNONCESize <= bufferSize)
	{
		memcpy(data, &buffer[*bufferPos], sizeof(ULONGLONG));
		*bufferPos += sizeof(ULONGLONG);
	}
}

const uint32 PacketXNADDRSize	= sizeof(XNADDR);
inline void AddPacketXNADDR(uint8* buffer, uint32 bufferSize, uint32* bufferPos, XNADDR data)
{
	assert(*bufferPos + PacketXNADDRSize <= bufferSize);

	if (*bufferPos + PacketXNADDRSize <= bufferSize)
	{
		memcpy(&buffer[*bufferPos], &data, sizeof(XNADDR));
		*bufferPos += sizeof(XNADDR);
	}
}

inline void RemovePacketXNADDR(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, XNADDR* data)
{
	assert(*bufferPos + PacketXNADDRSize <= bufferSize);

	if (*bufferPos + PacketXNADDRSize <= bufferSize)
	{
		memcpy(data, &buffer[*bufferPos], sizeof(XNADDR));
		*bufferPos += sizeof(XNADDR);
	}
}

const uint32 PacketXSESSIONINFOSize = sizeof(XSESSION_INFO);
inline void AddPacketXSessionInfo(uint8* pBuffer, uint32 bufferSize, uint32* pBufferPos, XSESSION_INFO* pData)
{
	assert(*pBufferPos + PacketXSESSIONINFOSize <= bufferSize);

	if (*pBufferPos + PacketXSESSIONINFOSize <= bufferSize)
	{
		memcpy(&pBuffer[*pBufferPos], pData, PacketXSESSIONINFOSize);
		*pBufferPos += PacketXSESSIONINFOSize;
	}
}

inline void RemovePacketXSessionInfo(const uint8* pBuffer, uint32 bufferSize, uint32* pBufferPos, XSESSION_INFO* pData)
{
	assert(*pBufferPos + PacketXSESSIONINFOSize <= bufferSize);

	if (*pBufferPos + PacketXSESSIONINFOSize <= bufferSize)
	{
		memcpy(pData, &pBuffer[*pBufferPos], PacketXSESSIONINFOSize);
		*pBufferPos += PacketXSESSIONINFOSize;
	}
}

#endif // __CRYLIVELOBBY_H__
