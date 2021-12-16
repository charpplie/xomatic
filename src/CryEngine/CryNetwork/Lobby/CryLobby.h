#ifndef __CRYLOBBY_H__
#define __CRYLOBBY_H__

#pragma once




#define USE_XBOXLIVE	0


#if (0 && defined(WIN32))
#define USE_GFWL			1
#else
#define USE_GFWL			0
#endif





#define USE_PSN					0
#define USE_PSN_VOICE		0	


#define USE_LIVE			(USE_XBOXLIVE || USE_GFWL)

#include "Config.h"
#include "ICryLobby.h"
#include "Socket/IDatagramSocket.h"
#include "NetTimer.h"
#include "Network.h"
#include "CryRebroadcaster.h"

enum CryLobbyPacketType
{
	eLobbyPT_Ack,
	eLobbyPT_SessionDeleteRemoteConnection,

	eLobbyPT_EndType
};

const uint32 CryLobbyTimeOut = 10000;

#define MAX_LOBBY_PACKET_SIZE		1264	// Since Live has the smallest max packet size set the lobby max packet size to the Live value.
#define MAX_LOBBY_CONNECTIONS		64
#define MAX_LOBBY_TASKS					10
#define SEND_DATA_QUEUE_SIZE		32

class CCryLobby;

typedef uint32 CryLobbyConnectionID;
const CryLobbyConnectionID CryLobbyInvalidConnectionID = 0xffffffff;
typedef uint32 CryLobbySendID;
const CryLobbySendID CryLobbyInvalidSendID = 0xffffffff;

typedef void (*CryLobbyServiceCallback)(ECryLobbyError error, CCryLobby* arg, ECryLobbyService service);

class ICryLobbyGFWLExtras
{
public:
	virtual void		OnCreateDevice(void* pD3D, void* pD3DPP) = 0;
	virtual void		OnDeviceReset(void* pD3DPP) = 0;
	virtual void		OnDeviceDestroyed() = 0;
	virtual void		Render() = 0;
	virtual uint32	PreTranslateMessage(void* msg) = 0;
};

class CCryLobbyService : public CMultiThreadRefCount, public ICryLobbyService
{
public:
	virtual										~CCryLobbyService() {}

	virtual ECryLobbyError		Initialise(CryLobbyServiceCallback cb, CCryLobby* cbArg, ECryLobbyService service) = 0;
	virtual ECryLobbyError		Terminate(CryLobbyServiceCallback cb, CCryLobby* cbArg, ECryLobbyService service) = 0;
	virtual void							Tick(CTimeValue tv) = 0;

	virtual void							OnPacket(const TNetAddress& addr, const uint8* data, uint32 length) = 0;
	virtual void							OnError(const TNetAddress& addr, ESocketError error, CryLobbySendID sendID) = 0;
	virtual void							OnSendComplete(const TNetAddress& addr, CryLobbySendID sendID) = 0;

	virtual bool							IsDead()const	{ return false; }
};

typedef _smart_ptr<CCryLobbyService> CCryLobbyServicePtr;

class CCryLobby : public ICryLobby, public IDatagramListener
#if NETWORK_HOST_MIGRATION
	, public IHostMigrationEventListener
#endif
{
public:
											CCryLobby();
											~CCryLobby();

	static ICryLobby*		GetLobby() { return m_lobby ? m_lobby : m_lobby = new CCryLobby; }

	virtual ECryLobbyError				Initialise(ECryLobbyService service, CryLobbyConfigurationCallback cfgCb, CryLobbyCallback cb, void* cbArg);
	virtual ECryLobbyError				Terminate(ECryLobbyService service, CryLobbyCallback cb, void* cbArg);
	virtual ECryLobbyService			SetLobbyService(ECryLobbyService service);
	virtual ECryLobbyService			GetLobbyServiceType()												{ return m_service; }
	virtual ICryLobbyService*			GetLobbyService()														{ return m_services[m_service]; }
	virtual ICryMatchMaking*			GetMatchMaking()														{ return m_services[m_service] ? m_services[m_service]->GetMatchMaking() : NULL; }
	virtual ICryVoice*						GetVoice()																	{ return m_services[m_service] ? m_services[m_service]->GetVoice() : NULL; }
	virtual ICryStats*						GetStats()																	{ return m_services[m_service] ? m_services[m_service]->GetStats() : NULL; }
	virtual ICryLobbyService*			GetLobbyService(ECryLobbyService service)		{ assert(service<eCLS_NumServices); return m_services[service]; }
	virtual ICryLobbyGFWLExtras*	GetLobbyExtras()														{ return m_gfwlExtras; }

#if NETWORK_HOST_MIGRATION
	virtual bool									OnInitiate(SHostMigrationInfo& hostMigrationInfo);
	virtual bool									OnDisconnectClient(SHostMigrationInfo& hostMigrationInfo);
	virtual bool									OnDemoteToClient(SHostMigrationInfo& hostMigrationInfo);
	virtual bool									OnPromoteToServer(SHostMigrationInfo& hostMigrationInfo);
	virtual bool									OnReconnectClient(SHostMigrationInfo& hostMigrationInfo);
	virtual bool									OnFinalise(SHostMigrationInfo& hostMigrationInfo);
	virtual bool									OnTerminate(SHostMigrationInfo& hostMigrationInfo);
					bool									GetNewHostAddress(char* address);
#endif

	virtual void				RegisterEventInterest(ECryLobbySystemEvent eventOfInterest, CryLobbyEventCallback cb, void *userData);
	void								DispatchEvent(ECryLobbySystemEvent evnt, UCryLobbyEventData data);

	void								GetConfigurationInformation(SConfigurationParams* infos,uint32 infoCnt);

	void								InviteAccepted(ECryLobbyService service, uint32 user, CrySessionID sessionID);

	CryLobbyTaskID			CreateTask();
	void								ReleaseTask(CryLobbyTaskID id);

	void								InternalSocketCreate(bool server);
	void								InternalSocketFree(bool server);
	uint16							GetInternalSocketPort()													{ return m_socketConnectPort; }
	IDatagramSocketPtr	ExternalSocketCreate(const TNetAddress& addr, uint32 flags);
	void								ExternalSocketSetListener(IDatagramSocketPtr socket, IDatagramListener* listener);
	void								ExternalSocketDie(IDatagramSocketPtr socket);
	ESocketError				Send(const uint8* buffer, size_t length, const TNetAddress& to, bool reliable, CryLobbySendID* id);
	ESocketError				SendVoice(const uint8* buffer, size_t length, const TNetAddress& to);
	CryLobbyConnectionID	CreateConnection(const TNetAddress& address);
	CryLobbyConnectionID	FindConnection(const TNetAddress& address);
	void								KeepConnection(CryLobbyConnectionID c);
	void								ReleaseConnection(CryLobbyConnectionID c);
	bool								ConnectionFromAddress(CryLobbyConnectionID* connection, const TNetAddress& address);
	bool								AddressFromConnection(TNetAddress& address, CryLobbyConnectionID connection);
	bool								SetConnectionAddress(CryLobbyConnectionID connection, TNetAddress& address);

	//start of IDatagramListener
	virtual void				OnPacket(const TNetAddress& addr, const uint8* data, uint32 length);
	virtual void				OnError(const TNetAddress& addr, ESocketError error);
	//end of IDatagramListener
	void								OnSendComplete(const TNetAddress& addr);

	CryLockT<CRYLOCK_RECURSIVE>& GetMutex()													{ return m_mutex; }
	void								MutexLock()																	{ m_mutex.Lock(); }
	bool								MutexTryLock()															{ return m_mutex.TryLock(); }
	void								MutexUnlock()																{ m_mutex.Unlock(); }
	TMemHdl							MemAlloc(size_t sz)													{ return m_MMM ? m_MMM->AllocHdl(sz) : TMemInvalidHdl; }
	void								MemFree(TMemHdl h)													{ if (m_MMM) { m_MMM->FreeHdl(h); } }
	void*								MemGetPtr(TMemHdl h)												{ return m_MMM ? m_MMM->PinHdl(h) : NULL; }

	CCryRebroadcaster*	GetRebroadcaster(void)											{ return m_pRebroadcaster; }

private:
	static	void				TimerCallback(NetTimerId id, void *arg, CTimeValue tv);
	void								TimerCallback(CTimeValue tv);

	static void					InitialiseServiceCB(ECryLobbyError error, CCryLobby* arg, ECryLobbyService service);
	static void					TerminateServiceCB(ECryLobbyError error, CCryLobby* arg, ECryLobbyService service);
	void								InternalSocketFree();
	ECryLobbyError			CheckAllocGlobalResources();
	void								CheckFreeGlobalResources();
	void								FreeConnection(CryLobbyConnectionID c);

	static ICryLobby*		m_lobby;

	struct SEventCBData
	{
		ECryLobbySystemEvent	event;
		CryLobbyEventCallback	cb;
		void									*userData;
	};

	typedef std::vector<SEventCBData> EventCBList;

	EventCBList	m_callbacks;

	ECryLobbyService		m_service;
	CCryLobbyServicePtr	m_services[eCLS_NumServices];
	ICryLobbyGFWLExtras*	m_gfwlExtras;

	struct STask
	{
		CryLobbyCallback	cb;
		void*							cbArg;
	}										m_task[eCLS_NumServices];

	IDatagramSocketPtr	m_socket;
	IDatagramListener*	m_pExternalSocketListener;

	struct SConnection 
	{
		CTimeValue				timer;
		TNetAddress				addr;

		struct SData
		{
			TMemHdl					data;
			uint16					dataSize;
			uint8						counter;
		};

		MiniQueue<SData, SEND_DATA_QUEUE_SIZE>	dataQueue;

		uint8							counterIn;
		uint8							counterOut;
		bool							keep;
		bool							used;
		bool							freeing;
		bool							sendTimerStarted;
	}										m_connection[MAX_LOBBY_CONNECTIONS];

	struct SServiceTask
	{
		bool							used;
	}										m_serviceTask[MAX_LOBBY_TASKS];

	CMementoMemoryManager*	m_MMM;
	CCryRebroadcaster*			m_pRebroadcaster;
	CryLockT<CRYLOCK_RECURSIVE>	m_mutex;
	NetTimerId							m_updateTimer;
	CryLobbyConfigurationCallback m_configCB;		// allows platform specific configuration to be requested by any attached service
	ECryLobbyService		m_socketService;
	uint16							m_socketListenPort;
	uint16							m_socketConnectPort;
	uint8								m_socketServerCount;
	uint8								m_socketExternalRefCount;
	bool								m_socketServer;

	enum ELobbyHostMigrationStatus
	{
		eLHMS_MigrateServer,
		eLHMS_MigratingServer,
		eLHMS_MigrateClient,
		eLHMS_MigratingClient
	};
	ELobbyHostMigrationStatus m_lobbyHMStatus;
};

#define LOBBY_AUTO_LOCK AUTO_LOCK_T(CryLockT<CRYLOCK_RECURSIVE>, ((CCryLobby*)CCryLobby::GetLobby())->GetMutex())

inline CryLobbySendID CryLobbyCreateSendID(CryLobbyConnectionID connection, uint32 count)
{
	return connection|(count<<16);
}

inline CryLobbyConnectionID CryLobbyGetConnectionIDFromSendID(CryLobbySendID id)
{
	return id&0xffff;
}

const uint32 PacketHeaderSize	= 2;
inline void StartAddPacket(uint8* buffer, uint32 bufferSize, uint32* bufferPos, CryLobbyPacketType type)
{
	assert(PacketHeaderSize <= bufferSize);

	if (PacketHeaderSize <= bufferSize)
	{
		buffer[0] = Frame_IDToHeader[eH_CryLobby];
		buffer[1] = type;
		assert(bufferPos);
		*bufferPos = PacketHeaderSize;
	}
}

inline void StartRemovePacket(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, CryLobbyPacketType* type)
{
	assert(PacketHeaderSize <= bufferSize);

	if (PacketHeaderSize <= bufferSize)
	{
		if (type)
		{
			*type = (CryLobbyPacketType)buffer[1];
		}

		assert(bufferPos);
		*bufferPos = PacketHeaderSize;
	}
}

inline void StartRemovePacket(const uint8* buffer, uint32 bufferSize, uint32* bufferPos)
{
	assert(PacketHeaderSize <= bufferSize);

	if (PacketHeaderSize <= bufferSize)
	{
		assert(bufferPos);
		*bufferPos = PacketHeaderSize;
	}
}

const uint32 PacketUINT8Size	= 1;
inline void AddPacketUINT8(uint8* buffer, uint32 bufferSize, uint32* bufferPos, uint8 data)
{
	assert(bufferPos);
	assert(*bufferPos + PacketUINT8Size <= bufferSize);

	if (*bufferPos + PacketUINT8Size <= bufferSize)
	{
		buffer[(*bufferPos)++] = data;
	}
}

inline void RemovePacketUINT8(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, uint8* data)
{
	assert(bufferPos);
	assert(*bufferPos + PacketUINT8Size <= bufferSize);

	if (*bufferPos + PacketUINT8Size <= bufferSize)
	{
		*data = buffer[(*bufferPos)++];
	}
}

const uint32 PacketUINT16Size	= 2;
inline void AddPacketUINT16(uint8* buffer, uint32 bufferSize, uint32* bufferPos, uint16 data)
{
	assert(bufferPos);
	assert(*bufferPos + PacketUINT16Size <= bufferSize);

	if (*bufferPos + PacketUINT16Size <= bufferSize)
	{
		buffer[(*bufferPos)++] = (data>>8)&0xff;
		buffer[(*bufferPos)++] = (data)&0xff;
	}
}

inline void RemovePacketUINT16(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, uint16* data)
{
	assert(bufferPos);
	assert(*bufferPos + PacketUINT16Size <= bufferSize);

	if (*bufferPos + PacketUINT16Size <= bufferSize)
	{
		*data = buffer[(*bufferPos)++];
		*data = ((*data)<<8) | buffer[(*bufferPos)++];
	}
}

const uint32 PacketUINT32Size	= 4;
inline void AddPacketUINT32(uint8* buffer, uint32 bufferSize, uint32* bufferPos, uint32 data)
{
	assert(bufferPos);
	assert(*bufferPos + PacketUINT32Size <= bufferSize);

	if (*bufferPos + PacketUINT32Size <= bufferSize)
	{
		buffer[(*bufferPos)++] = (data>>24)&0xff;
		buffer[(*bufferPos)++] = (data>>16)&0xff;
		buffer[(*bufferPos)++] = (data>>8)&0xff;
		buffer[(*bufferPos)++] = (data)&0xff;
	}
}

inline void RemovePacketUINT32(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, uint32* data)
{
	assert(bufferPos);
	assert(*bufferPos + PacketUINT32Size <= bufferSize);

	if (*bufferPos + PacketUINT32Size <= bufferSize)
	{
		*data = buffer[(*bufferPos)++];
		*data = ((*data)<<8) | buffer[(*bufferPos)++];
		*data = ((*data)<<8) | buffer[(*bufferPos)++];
		*data = ((*data)<<8) | buffer[(*bufferPos)++];
	}
}

const uint32 PacketUINT64Size	= 8;
inline void AddPacketUINT64(uint8* buffer, uint32 bufferSize, uint32* bufferPos, uint64 data)
{
	assert(bufferPos);
	assert(*bufferPos + PacketUINT64Size <= bufferSize);

	if (*bufferPos + PacketUINT64Size <= bufferSize)
	{
		buffer[(*bufferPos)++] = (data>>56)&0xff;
		buffer[(*bufferPos)++] = (data>>48)&0xff;
		buffer[(*bufferPos)++] = (data>>40)&0xff;
		buffer[(*bufferPos)++] = (data>>32)&0xff;
		buffer[(*bufferPos)++] = (data>>24)&0xff;
		buffer[(*bufferPos)++] = (data>>16)&0xff;
		buffer[(*bufferPos)++] = (data>>8)&0xff;
		buffer[(*bufferPos)++] = (data)&0xff;
	}
}

inline void RemovePacketUINT64(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, uint64* data)
{
	assert(bufferPos);
	assert(*bufferPos + PacketUINT64Size <= bufferSize);

	if (*bufferPos + PacketUINT64Size <= bufferSize)
	{
		*data = buffer[(*bufferPos)++];
		*data = ((*data)<<8) | buffer[(*bufferPos)++];
		*data = ((*data)<<8) | buffer[(*bufferPos)++];
		*data = ((*data)<<8) | buffer[(*bufferPos)++];
		*data = ((*data)<<8) | buffer[(*bufferPos)++];
		*data = ((*data)<<8) | buffer[(*bufferPos)++];
		*data = ((*data)<<8) | buffer[(*bufferPos)++];
		*data = ((*data)<<8) | buffer[(*bufferPos)++];
	}
}

const uint32 PacketBoolSize	= 1;
inline void AddPacketBool(uint8* buffer, uint32 bufferSize, uint32* bufferPos, bool data)
{
	assert(bufferPos);
	assert(*bufferPos + PacketBoolSize <= bufferSize);

	if (*bufferPos + PacketBoolSize <= bufferSize)
	{
		buffer[(*bufferPos)++] = data ? 1 : 0;
	}
}

inline void RemovePacketBool(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, bool* data)
{
	assert(bufferPos);
	assert(*bufferPos + PacketBoolSize <= bufferSize);

	if (*bufferPos + PacketBoolSize <= bufferSize)
	{
		*data = buffer[(*bufferPos)++] ? true : false;
	}
}

const uint32 PacketErrorSize	= PacketUINT16Size;
inline void AddPacketError(uint8* buffer, uint32 bufferSize, uint32* bufferPos, ECryLobbyError data)
{
	AddPacketUINT16(buffer, bufferSize, bufferPos, (uint16)data);
}

inline void RemovePacketError(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, ECryLobbyError* data)
{
	uint16 tmp;
	RemovePacketUINT16(buffer, bufferSize, bufferPos, &tmp);
	*data = (ECryLobbyError)tmp;
}

inline void AddPacketData(uint8* buffer, uint32 bufferSize, uint32* bufferPos, uint8* data, uint32 dataSize)
{
	assert(bufferPos);
	assert(*bufferPos + dataSize <= bufferSize);

	if (*bufferPos + dataSize <= bufferSize)
	{
		memcpy(&buffer[*bufferPos], data, dataSize);
		*bufferPos += dataSize;
	}
}

inline void RemovePacketData(const uint8* buffer, uint32 bufferSize, uint32* bufferPos, uint8* data, uint32 dataSize)
{
	assert(bufferPos);
	assert(*bufferPos + dataSize <= bufferSize);

	if (*bufferPos + dataSize <= bufferSize)
	{
		memcpy(data, &buffer[*bufferPos], dataSize);
		*bufferPos += dataSize;
	}
}

extern void AddPacketSCryLobbyUserData(uint8* pBuffer, uint32 bufferSize, uint32* pBufferPos, SCryLobbyUserData* pData);
extern void RemovePacketSCryLobbyUserData(uint8* pBuffer, uint32 bufferSize, uint32* pBufferPos, SCryLobbyUserData* pData);


#endif // __CRYLOBBY_H__

