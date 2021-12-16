#include "StdAfx.h"

#include "CryLobby.h"
#include "LAN/CryLANLobby.h"
#if USE_LIVE
#include "Live/CryLiveLobby.h"
#endif
#if USE_PSN
#include "PSN/CryPSNLobby.h"
#endif
#include "Protocol/NetChannel.h"
#include "Protocol/NetNub.h"
#include "IGame.h"
#include "IGameFramework.h"

const int LOBBY_UPDATE_FREQUENCY = 10;
const float LOBBY_UPDATE_INTERVAL = 1.0f/((float)LOBBY_UPDATE_FREQUENCY);

#define CONNECTION_COUNTER_MAX	255
#define CONNECTION_COUNTER_HALF	128

ICryLobby* CCryLobby::m_lobby = NULL;

CCryLobby::CCryLobby()
{
	for (uint32 i = 0; i < eCLS_NumServices; i++)
	{
		m_services[i] = NULL;
		m_task[i].cb = NULL;
		m_task[i].cbArg = NULL;
	}

	m_callbacks.reserve(10);			// pre-reserve space for callback event data
	m_updateTimer = 0;
	m_service = eCLS_LAN;
	m_socket = NULL;
	m_pExternalSocketListener = NULL;
	m_MMM = NULL;
	m_socketService = eCLS_NumServices;
	m_socketListenPort = 0;
	m_socketConnectPort = 0;
	m_socketServerCount = 0;
	m_socketServer = false;
	m_socketExternalRefCount = 0;

#if USE_GFWL
	m_gfwlExtras = new CCryLobbyGFWLExtras;
#else
	m_gfwlExtras = NULL;
#endif

	m_pRebroadcaster = new CCryRebroadcaster(this);
	NET_ASSERT(m_pRebroadcaster != NULL);

#if NETWORK_HOST_MIGRATION
	CNetwork::Get()->AddEventListener(this, "CryLobby");
#endif
}

CCryLobby::~CCryLobby()
{
#if NETWORK_HOST_MIGRATION
	CNetwork::Get()->RemoveEventListener(this);
#endif

	if (m_pRebroadcaster)
	{
		delete m_pRebroadcaster;
		m_pRebroadcaster = NULL;
	}

	if (m_updateTimer)
	{
		TIMER.CancelTimer(m_updateTimer);
		m_updateTimer = 0;
	}

	InternalSocketFree();

	if (m_MMM)
	{
		delete m_MMM;
		m_MMM = NULL;
	}
}

ECryLobbyService CCryLobby::SetLobbyService(ECryLobbyService service)
{
	ECryLobbyService ret = m_service;

	assert(service<eCLS_NumServices);
	m_service = service;

	return ret;
}

#if NETWORK_HOST_MIGRATION
bool CCryLobby::OnInitiate(SHostMigrationInfo& hostMigrationInfo)
{
	CryLog("Host Migration: CCryLobby::OnInitiate() started");
	CCryMatchMaking* pMatchMaking = (CCryMatchMaking*)GetMatchMaking();
	if (pMatchMaking)
	{
		pMatchMaking->HostMigrationInitiate();
	}

	if (hostMigrationInfo.IsNewHost())
	{
		m_lobbyHMStatus = eLHMS_MigrateServer;
	}
	else
	{
		m_lobbyHMStatus = eLHMS_MigrateClient;
	}

	CryLog("Host Migration: CCryLobby::OnInitiate() finished");
	return true;
}

bool CCryLobby::OnDisconnectClient(SHostMigrationInfo& hostMigrationInfo)
{
	return true;
}

bool CCryLobby::OnDemoteToClient(SHostMigrationInfo& hostMigrationInfo)
{
#if defined(WIN32)
	// Need to do this on Windows because you can run multiple instances
	// on the same PC so you need to differentiate port numbers for client
	// and server
	InternalSocketCreate(false);
#endif
	return true;
}

bool CCryLobby::OnPromoteToServer(SHostMigrationInfo& hostMigrationInfo)
{
	bool done = false;
	CCryMatchMaking* pMatchMaking = (CCryMatchMaking*)GetMatchMaking();

	if (pMatchMaking)
	{
		switch (m_lobbyHMStatus)
		{
		case eLHMS_MigrateServer:
			CryLog("Host Migration: CCryLobby::OnPromoteToServer() started");
#if defined(WIN32)
			// Need to do this on Windows because you can run multiple instances
			// on the same PC so you need to differentiate port numbers for client
			// and server
			InternalSocketCreate(true);
#endif
			pMatchMaking->HostMigrationServer();
			m_lobbyHMStatus = eLHMS_MigratingServer;
			break;
		case eLHMS_MigratingServer:
		default:
			done = pMatchMaking->IsHostMigrationFinished();
			if (done)
			{
				m_lobbyHMStatus = eLHMS_MigrateClient;
			}
			break;
		}
	}

	CryLog("Host Migration: CCryLobby::OnPromoteToServer() %s", ((done == true) ? "finished" : "waiting"));
	return done;
}

bool CCryLobby::OnReconnectClient(SHostMigrationInfo& hostMigrationInfo)
{
	bool done = false;
	CCryMatchMaking* pMatchMaking = (CCryMatchMaking*)GetMatchMaking();
	if (pMatchMaking)
	{
		switch (m_lobbyHMStatus)
		{
		case eLHMS_MigrateClient:
			CryLog("Host Migration: CCryLobby::OnReconnectClient() started");
			m_lobbyHMStatus = eLHMS_MigratingClient;
			break;
		case eLHMS_MigratingClient:
			done = pMatchMaking->IsHostMigrationFinished();
		default:
			break;
		}
	}

	CryLog("Host Migration: CCryLobby::OnReconnectClient() %s", ((done == true) ? "finished" : "waiting"));
	return done;
}

bool CCryLobby::OnFinalise(SHostMigrationInfo& hostMigrationInfo)
{
	return true;
}

bool CCryLobby::OnTerminate(SHostMigrationInfo& hostMigrationInfo)
{
	return true;
}
#endif

void CCryLobby::InviteAccepted(ECryLobbyService service, uint32 user, CrySessionID sessionID)
{
	UCryLobbyEventData					eventData;
	SCryLobbyInviteAcceptedData	inviteData;

	eventData.pInviteAcceptedData=&inviteData;
	inviteData.m_service=service;
	inviteData.m_user=user;
	inviteData.m_id=sessionID;
	DispatchEvent(eCLSE_InviteAccepted,eventData);
}

void CCryLobby::TimerCallback(NetTimerId id, void *arg, CTimeValue tv)
{
	((CCryLobby*)arg)->TimerCallback(tv);
}

void CCryLobby::TimerCallback(CTimeValue tv)
{
	for (uint32 i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
	{
		SConnection* connection = &m_connection[i];

		if (connection->used)
		{
			if (!connection->dataQueue.Empty())
			{
				SConnection::SData& data = connection->dataQueue.Front();

				if (!connection->sendTimerStarted)
				{
					connection->timer = g_time;
					connection->sendTimerStarted = true;
				}

				CryLog("[lobby] Send reliable connection %d %d.%d.%d.%d:%d counter %d packet %d size %d",
					i, (connection->addr.GetPtr<SIPv4Addr>()->addr>>24)&0xff, (connection->addr.GetPtr<SIPv4Addr>()->addr>>16)&0xff,
					(connection->addr.GetPtr<SIPv4Addr>()->addr>>8)&0xff, (connection->addr.GetPtr<SIPv4Addr>()->addr)&0xff, connection->addr.GetPtr<SIPv4Addr>()->port,
					data.counter, ((uint8*)MemGetPtr(data.data))[1], data.dataSize);

				// Data hasn't been acknowledged so try sending again
				if (m_socket->Send((uint8*)MemGetPtr(data.data), data.dataSize, connection->addr) != eSE_Ok)
				{
					FreeConnection(i);
				}

				if ((tv - connection->timer).GetMilliSecondsAsInt64() > CryLobbyTimeOut)
				{
					// Timeout on send
					OnError(connection->addr, eSE_UnreachableAddress);
				}
			}
			else
			{
				if ((!connection->keep && ((tv - connection->timer).GetMilliSecondsAsInt64() > CryLobbyTimeOut)) ||
						connection->freeing)
				{
					CryLog("[lobby] Release inactive connection %d %d.%d.%d.%d:%d",
						i, (connection->addr.GetPtr<SIPv4Addr>()->addr>>24)&0xff, (connection->addr.GetPtr<SIPv4Addr>()->addr>>16)&0xff,
						(connection->addr.GetPtr<SIPv4Addr>()->addr>>8)&0xff, (connection->addr.GetPtr<SIPv4Addr>()->addr)&0xff, connection->addr.GetPtr<SIPv4Addr>()->port);

					FreeConnection(i);
				}
			}
		}
	}

	// Tick all lobby services.
	// Things like invite notifications for a service still need to be processed even if currently using a different service
	for (uint32 i = 0; i < eCLS_NumServices; i++)
	{
		if (m_services[i])
		{
			m_services[i]->Tick(tv);
		}
	}

	m_updateTimer = TIMER.AddTimer(tv + LOBBY_UPDATE_INTERVAL, TimerCallback, this);
}

void CCryLobby::InitialiseServiceCB(ECryLobbyError error, CCryLobby* lobby, ECryLobbyService service)
{
	if (error != eCLE_Success)
	{
		lobby->m_services[service] = NULL;
	}

	if (lobby->m_task[service].cb)
	{
		lobby->m_task[service].cb(service, error, lobby->m_task[service].cbArg);
	}
}

void CCryLobby::TerminateServiceCB(ECryLobbyError error, CCryLobby* lobby, ECryLobbyService service)
{
	// Remove our reference to the service. It will be deleted when all references are gone.
	lobby->m_services[service] = NULL;

	lobby->CheckFreeGlobalResources();

	if (lobby->m_task[service].cb)
	{
		lobby->m_task[service].cb(service, error, lobby->m_task[service].cbArg);
	}
}

void CCryLobby::InternalSocketCreate(bool server)
{
	SCOPED_GLOBAL_LOCK;

	m_socketListenPort = gEnv->pConsole->GetCVar("sv_port")->GetIVal();
	m_socketConnectPort = gEnv->pConsole->GetCVar("cl_serverport")->GetIVal();
#if defined(WIN32)
	// Need to do this on Windows because you can run multiple instances
	// on the same PC so you need to differentiate port numbers for client
	// and server
	uint16 port = server ? m_socketListenPort : 0;
#else
	uint16 port = m_socketListenPort;
#endif

#if defined(WIN32)
	if (!m_socket || (m_socketService != m_service) || ((m_socketServerCount == 0) && (m_socketServer || server)))
#else
	if (!m_socket || (m_socketService != m_service))
#endif
	{
		uint32 flags = (m_service == eCLS_LAN) ? (eSF_BroadcastSend | eSF_BroadcastReceive) : 0;

		InternalSocketFree();

		m_socket = OpenSocket(TNetAddress(SIPv4Addr(0, port)), flags);
		m_socketService = m_service;
		m_socketServer = server;

		if (m_socket)
		{
			m_socket->SetListener(this);
		}
	}

	if (server)
	{
		m_socketServerCount++;
	}
}

void CCryLobby::InternalSocketFree(bool server)
{
	if (server)
	{
		assert(m_socketServerCount != 0);

		m_socketServerCount--;

		if ((m_socketServerCount == 0) && (m_socketExternalRefCount == 0))
		{
			InternalSocketCreate(false);
		}
	}
}

void CCryLobby::InternalSocketFree()
{
	if (m_socket)
	{
		m_socket->Die();
		m_socket = NULL;
	}
}

ECryLobbyError CCryLobby::CheckAllocGlobalResources()
{
	if (!m_socket)
	{
		InternalSocketCreate(false);
	}

	if (!m_MMM)
	{
		m_MMM = new CMementoMemoryManager("Lobby");

		if (m_MMM)
		{
			for (uint32 i = 0; i < MAX_LOBBY_TASKS; i++)
			{
				m_serviceTask[i].used = false;
			}

			for (uint32 i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
			{
				m_connection[i].used = false;
			}

			if (!m_updateTimer)
			{
				m_updateTimer = TIMER.AddTimer(g_time + LOBBY_UPDATE_INTERVAL, TimerCallback, this);
			}
		}
		else
		{
			return eCLE_OutOfMemory;
		}
	}

	return eCLE_Success;
}

void CCryLobby::CheckFreeGlobalResources()
{
	uint32 i;

	for (i = 0; i < eCLS_NumServices; i++)
	{
		if (m_services[i])
		{
			break;
		}
	}

	if (i == eCLS_NumServices)
	{
		if (m_updateTimer)
		{
			TIMER.CancelTimer(m_updateTimer);
			m_updateTimer = 0;
		}

		for (i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
		{
			FreeConnection(i);
		}

		InternalSocketFree();

		if (m_MMM)
		{
			delete m_MMM;
			m_MMM = NULL;
		}
	}
}

CryLobbyTaskID CCryLobby::CreateTask()
{
	for (uint32 i = 0; i < MAX_LOBBY_TASKS; i++)
	{
		if (!m_serviceTask[i].used)
		{
			m_serviceTask[i].used = true;
			return i;
		}
	}

	return CryLobbyInvalidTaskID;
}

void CCryLobby::ReleaseTask(CryLobbyTaskID id)
{
	m_serviceTask[id].used = false;
}

ECryLobbyError CCryLobby::Initialise(ECryLobbyService service, CryLobbyConfigurationCallback cfgCb, CryLobbyCallback cb, void* cbArg)
{
	ECryLobbyError error = CheckAllocGlobalResources();

	assert(service>=0 && service<eCLS_NumServices);

	if (error == eCLE_Success)
	{
		if (m_services[service])
		{
			error = eCLE_AlreadyInitialised;
		}
		else
		{
			switch (service)
			{
			case eCLS_LAN:
				m_services[eCLS_LAN] = new CCryLANLobbyService();

				if (!m_services[eCLS_LAN])
				{
					error = eCLE_OutOfMemory;
				}

				break;

			case eCLS_Online:
	#if USE_LIVE
				m_services[eCLS_Online] = new CCryLiveLobbyService();

				if (!m_services[eCLS_Online])
				{
					error = eCLE_OutOfMemory;
				}
	#elif USE_PSN
				m_services[eCLS_Online] = new CCryPSNLobbyService();

				if (!m_services[eCLS_Online])
				{
					error = eCLE_OutOfMemory;
				}

	#else

				m_services[eCLS_Online] = NULL;
				error = eCLE_ServiceNotSupported;
	#endif
				break;
			}

			if (error == eCLE_Success)
			{
				m_task[service].cb = cb;
				m_task[service].cbArg = cbArg;
				this->m_configCB = cfgCb;

				error = m_services[service]->Initialise(InitialiseServiceCB, this, service);

				if (error != eCLE_Success)
				{
					// Task didn't start so remove our reference to the service
					m_services[service] = NULL;
				}
			}
		}
	}

	CryLog("[Lobby] Initialise service %d error %d\n", service, error);

	return error;
}

ECryLobbyError CCryLobby::Terminate(ECryLobbyService service, CryLobbyCallback cb, void* cbArg)
{
	ECryLobbyError error;

	if (m_services[service])
	{
		m_task[service].cb = cb;
		m_task[service].cbArg = cbArg;

		error = m_services[service]->Terminate(TerminateServiceCB, this, service);

		if (error != eCLE_Success)
		{
			m_services[service] = NULL;
		}
	}
	else
	{
		error = eCLE_NotInitialised;
	}

	CheckFreeGlobalResources();

	CryLog("[Lobby] Terminate service %d error %d\n", service, error);

	return error;
}

CryLobbyConnectionID CCryLobby::CreateConnection(const TNetAddress& address)
{
	for (uint32 i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
	{
		if (!m_connection[i].used)
		{
			SConnection* connection = &m_connection[i];

			connection->timer = g_time;
			connection->sendTimerStarted = false;
			connection->addr = address;
			connection->counterIn = CONNECTION_COUNTER_MAX;
			connection->counterOut = 0;
			connection->keep = false;
			connection->used = true;
			connection->freeing = false;

			return i;
		}
	}

	return CryLobbyInvalidConnectionID;
}

CryLobbyConnectionID CCryLobby::FindConnection(const TNetAddress& address)
{
	for (uint32 i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
	{
		if (m_connection[i].used && (m_connection[i].addr == address))
		{
			return i;
		}
	}

	return CryLobbyInvalidConnectionID;
}

void CCryLobby::ReleaseConnection(CryLobbyConnectionID c)
{
	// Put connection into freeing state
	// Any sends in queue will still try and send and no new sends will be accepted
	if (c != CryLobbyInvalidConnectionID)
	{
		SConnection* connection = &m_connection[c];

		if (connection->used)
		{
			connection->freeing = true;
		}
	}
}

void CCryLobby::FreeConnection(CryLobbyConnectionID c)
{
	if (c != CryLobbyInvalidConnectionID)
	{
		SConnection* connection = &m_connection[c];

		if (connection->used)
		{
			connection->used = false;

			while (!connection->dataQueue.Empty())
			{
				SConnection::SData& data = connection->dataQueue.Front();

				MemFree(data.data);
				connection->dataQueue.Pop();
			}
		}
	}
}

void CCryLobby::KeepConnection(CryLobbyConnectionID c)
{
	if (c != CryLobbyInvalidConnectionID)
	{
		SConnection* connection = &m_connection[c];

		if (connection->used)
		{
			connection->keep = true;
		}
	}
}

bool CCryLobby::ConnectionFromAddress(CryLobbyConnectionID* connection, const TNetAddress& address)
{
	*connection = FindConnection(address);

	return *connection != CryLobbyInvalidConnectionID;
}

bool CCryLobby::AddressFromConnection(TNetAddress& address, CryLobbyConnectionID c)
{
	assert(c == CryLobbyInvalidConnectionID || c < MAX_LOBBY_CONNECTIONS);

	if (c < MAX_LOBBY_CONNECTIONS)
	{
		SConnection* connection = &m_connection[c];

		if (connection->used)
		{
			address = connection->addr;
			return true;
		}
	}

	return false;
}

bool CCryLobby::SetConnectionAddress(CryLobbyConnectionID c, TNetAddress& address)
{
	if (c != CryLobbyInvalidConnectionID)
	{
		SConnection* connection = &m_connection[c];

		if (connection->used)
		{
			connection->addr = address;
			return true;
		}
	}

	return false;
}


#define PACKET_HEADER_COUNTEROUT			0
#define PACKET_HEADER_COUNTERIN				1
#define PACKET_HEADER_RELIABLEDATA		2
#define PACKET_HEADER_SIZE						3

#define PACKET_HEADER_RELIABLEDATA_RELIABLE	0x01

ESocketError CCryLobby::Send(const uint8* buffer, size_t length, const TNetAddress& to, bool reliable, CryLobbySendID* id)
{
	if (reliable)
	{
		CryLobbyConnectionID c = FindConnection(to);
		uint8 reliableData = PACKET_HEADER_RELIABLEDATA_RELIABLE;

		if (c == CryLobbyInvalidConnectionID)
		{
			c = CreateConnection(to);
		}

		if (c != CryLobbyInvalidConnectionID)
		{
			SConnection* connection = &m_connection[c];

			if (!connection->freeing && !connection->dataQueue.Full())
			{
				SConnection::SData sdata;

				sdata.data = MemAlloc(length + PACKET_HEADER_SIZE);

				if (sdata.data != TMemInvalidHdl)
				{
					uint8* data = (uint8*)MemGetPtr(sdata.data);

					connection->counterOut++;
					memcpy(data, buffer, length);

					data[length + PACKET_HEADER_COUNTEROUT] = connection->counterOut;
					data[length + PACKET_HEADER_COUNTERIN] = connection->counterIn;
					data[length + PACKET_HEADER_RELIABLEDATA] = reliableData;
					sdata.dataSize = length + PACKET_HEADER_SIZE;
					sdata.counter = connection->counterOut;
					connection->dataQueue.Push(sdata);

					if (id)
					{
						*id = CryLobbyCreateSendID(c, connection->counterOut);
					}

					return eSE_Ok;
				}
				else
				{
					return eSE_MiscFatalError;
				}
			}
			else
			{
				return eSE_MiscFatalError;
			}
		}
		else
		{
			return eSE_MiscFatalError;
		}
	}
	else
	{
		TMemHdl h = MemAlloc(length + PACKET_HEADER_SIZE);
		ESocketError ret = eSE_Ok;

		if (h != TMemInvalidHdl)
		{
			uint8* data = (uint8*)MemGetPtr(h);

			memcpy(data, buffer, length);
			data[length + PACKET_HEADER_COUNTEROUT] = 0;
			data[length + PACKET_HEADER_COUNTERIN] = 0;
			data[length + PACKET_HEADER_RELIABLEDATA] = 0;

			ret = m_socket->Send(data, length + PACKET_HEADER_SIZE, to);

			MemFree(h);
		}

		if (id)
		{
			*id = CryLobbyInvalidSendID;
		}

		return ret;
	}

	return eSE_Ok;
}

ESocketError CCryLobby::SendVoice(const uint8* buffer, size_t length, const TNetAddress& to)
{
	TMemHdl h = MemAlloc(length + PACKET_HEADER_SIZE);
	ESocketError ret = eSE_Ok;

	if (h != TMemInvalidHdl)
	{
		uint8* data = (uint8*)MemGetPtr(h);

		memcpy(data, buffer, length);
		data[length + PACKET_HEADER_COUNTEROUT] = 0;
		data[length + PACKET_HEADER_COUNTERIN] = 0;
		data[length + PACKET_HEADER_RELIABLEDATA] = 0;

		ret = m_socket->SendVoice(data, length + PACKET_HEADER_SIZE, to);

		MemFree(h);
	}

	return ret;
}

void CCryLobby::OnPacket(const TNetAddress& addr, const uint8* data, uint32 length)
{
	if (Frame_HeaderToID[data[0]] == eH_CryLobby)
	{
		CryLobbyConnectionID c = FindConnection(addr);
		uint8 reliableData;
		bool processPacket = true;

		length -= PACKET_HEADER_SIZE;
		reliableData = data[length + PACKET_HEADER_RELIABLEDATA];

		if (reliableData&PACKET_HEADER_RELIABLEDATA_RELIABLE)
		{
			if (c == CryLobbyInvalidConnectionID)
			{
				c = CreateConnection(addr);
				CryLog("[lobby] OnPacket new reliable connection %d packet %d size %d", c, data[1], length);
			}
			else
			{
				CryLog("[lobby] OnPacket reliable connection %d packet %d size %d", c, data[1], length);
			}

			if (c != CryLobbyInvalidConnectionID)
			{
				SConnection* connection = &m_connection[c];

				CryLog("[lobby] address %d.%d.%d.%d:%d",
					(connection->addr.GetPtr<SIPv4Addr>()->addr>>24)&0xff, (connection->addr.GetPtr<SIPv4Addr>()->addr>>16)&0xff,
					(connection->addr.GetPtr<SIPv4Addr>()->addr>>8)&0xff, (connection->addr.GetPtr<SIPv4Addr>()->addr)&0xff, connection->addr.GetPtr<SIPv4Addr>()->port);

				uint8 counterOut = data[length + PACKET_HEADER_COUNTEROUT];
				uint8 counterIn = data[length + PACKET_HEADER_COUNTERIN];

				if (((uint8)(connection->counterIn - counterOut)) >= CONNECTION_COUNTER_HALF)
				{
					// This is a new packet
					CryLog("[lobby] New packet counter %d", counterOut);

					connection->counterIn = counterOut;

					if (!connection->dataQueue.Empty())
					{
						SConnection::SData& qdata = connection->dataQueue.Front();

						if (counterIn >= qdata.counter)
						{
							// Other end has received the packet we are trying to send
							OnSendComplete(addr);
							CryLog("[lobby] Got ack on connection %d", c);
						}
					}
				}
				else
				{
					processPacket = false;
					CryLog("[lobby] Repeat packet counter %d", counterOut);
				}

				uint8 buffer[3];

				buffer[0] = Frame_IDToHeader[eH_CryLobby];
				buffer[1] = eLobbyPT_Ack;
				buffer[2] = connection->counterIn;

				Send(buffer, 3, addr, false, NULL);

				CryLog("[lobby] Send ack");
			}
		}

		SConnection* connection = (c != CryLobbyInvalidConnectionID) ? &m_connection[c] : NULL;

		if (connection)
		{
			connection->timer = g_time;
		}

		if (processPacket)
		{
			switch (data[1])
			{
			case eLobbyPT_Ack:
				if (connection)
				{
					uint8 counterIn = data[2];

					if (!connection->dataQueue.Empty())
					{
						SConnection::SData& qdata = connection->dataQueue.Front();

						if (counterIn >= qdata.counter)
						{
							// Other end has received the packet we are trying to send
							OnSendComplete(addr);
							CryLog("[lobby] Got ack on connection %d", c);
						}
					}
				}

				break;

			default:
				if (m_services[m_service])
				{
					// Give packet to the current service
					m_services[m_service]->OnPacket(addr, data, length);
				}

				break;
			}
		}
	}
	else
	{
		if (m_pExternalSocketListener)
		{
			m_pExternalSocketListener->OnPacket(addr, data, length);
		}
	}
}

void CCryLobby::OnError(const TNetAddress& addr, ESocketError error)
{
	CryLobbyConnectionID c;
	CryLobbySendID sendID = CryLobbyInvalidSendID;

	if (ConnectionFromAddress(&c, addr))
	{
		SConnection* connection = &m_connection[c];

		while (!connection->dataQueue.Empty())
		{
			SConnection::SData& data = connection->dataQueue.Front();

			sendID = CryLobbyCreateSendID(c, data.counter);

			MemFree(data.data);

			if (m_services[m_service])
			{
				m_services[m_service]->OnError(addr, error, sendID);
			}

			CryLog("[lobby] Socket error %d on connection %d free send data counter %d", error, c, data.counter);

			connection->dataQueue.Pop();
		}
	}
	else
	{
		if (m_services[m_service])
		{
			m_services[m_service]->OnError(addr, error, sendID);
		}
	}

	if (m_pExternalSocketListener)
	{
		m_pExternalSocketListener->OnError(addr, error);
	}
}

void CCryLobby::OnSendComplete(const TNetAddress& addr)
{
	CryLobbyConnectionID c;
	CryLobbySendID sendID = CryLobbyInvalidSendID;

	if (ConnectionFromAddress(&c, addr))
	{
		SConnection* connection = &m_connection[c];

		if (!connection->dataQueue.Empty())
		{
			SConnection::SData& data = connection->dataQueue.Front();

			sendID = CryLobbyCreateSendID(c, data.counter);

			MemFree(data.data);

			CryLog("[lobby] Send complete on connection %d free send data counter %d", c, data.counter);

			connection->dataQueue.Pop();
			connection->sendTimerStarted = false;
		}
	}

	if (m_services[m_service])
	{
		m_services[m_service]->OnSendComplete(addr, sendID);
	}
}
#if NETWORK_HOST_MIGRATION
bool CCryLobby::GetNewHostAddress(char* address)
{
	bool success = true;

	CCryMatchMaking* pMatchMaking = (CCryMatchMaking*)GetMatchMaking();

	if (pMatchMaking)
	{
		success = pMatchMaking->GetNewHostAddress(address);
	}

	return success;
}
#endif

void CCryLobby::RegisterEventInterest(ECryLobbySystemEvent eventOfInterest, CryLobbyEventCallback cb, void *userData)
{
	SEventCBData newEvent;

	newEvent.cb=cb;
	newEvent.userData=userData;
	newEvent.event=eventOfInterest;

	m_callbacks.push_back(newEvent);
}

void CCryLobby::DispatchEvent(ECryLobbySystemEvent evnt, UCryLobbyEventData data)
{
	EventCBList::iterator callbackItem;

	for (callbackItem=m_callbacks.begin();callbackItem!=m_callbacks.end();++callbackItem)
	{
		if (callbackItem->event==evnt)
		{
			callbackItem->cb(data,callbackItem->userData);
		}
	}
}

void CCryLobby::GetConfigurationInformation(SConfigurationParams* infos,uint32 infoCnt)
{
	if (m_configCB)
	{
		m_configCB(m_lobby->GetLobbyServiceType(),infos,infoCnt);
	}
}

IDatagramSocketPtr CCryLobby::ExternalSocketCreate(const TNetAddress& addr, uint32 flags)
{
	if (m_socket && addr.GetPtr<SIPv4Addr>())
	{
		m_socketExternalRefCount++;
		return m_socket;
	}
	else
	{
		return OpenSocket(addr, flags);
	}
}

void CCryLobby::ExternalSocketSetListener(IDatagramSocketPtr socket, IDatagramListener* listener)
{
	if (socket == m_socket)
	{
		assert(!listener || !m_pExternalSocketListener);

		m_pExternalSocketListener = listener;
	}
	else
	{
		socket->SetListener(listener);
	}
}

void CCryLobby::ExternalSocketDie(IDatagramSocketPtr socket)
{
	if (socket == m_socket)
	{
		assert(m_socketExternalRefCount != 0);

		m_pExternalSocketListener = NULL;
		m_socketExternalRefCount--;

		if ((m_socketServerCount == 0) && (m_socketExternalRefCount == 0))
		{
			InternalSocketCreate(false);
		}
	}
	else
	{
		socket->Die();
	}
}

void AddPacketSCryLobbyUserData(uint8* pBuffer, uint32 bufferSize, uint32* pBufferPos, SCryLobbyUserData* pData)
{
	switch (pData->m_type)
	{
	case eCSUDT_Int64:
	case eCSUDT_Float64:
		AddPacketUINT64(pBuffer, bufferSize, pBufferPos, pData->m_int64);
		break;

	case eCSUDT_Int32:
	case eCSUDT_Float32:
		AddPacketUINT32(pBuffer, bufferSize, pBufferPos, pData->m_int32);
		break;

	case eCSUDT_Int16:
		AddPacketUINT16(pBuffer, bufferSize, pBufferPos, pData->m_int16);

	case eCSUDT_Int8:
		AddPacketUINT8(pBuffer, bufferSize, pBufferPos, pData->m_int8);

	case eCSUDT_Int64NoEndianSwap:
		CRY_ASSERT_MESSAGE(pBufferPos, "AddPacketSCryLobbyUserData: Buffer pos not specified");
		CRY_ASSERT_MESSAGE(*pBufferPos + PacketUINT64Size <= bufferSize, "AddPacketSCryLobbyUserData: Buffer full");

		if (*pBufferPos + PacketUINT64Size <= bufferSize)
		{
			memcpy(&pBuffer[*pBufferPos], &pData->m_int64, PacketUINT64Size);
			*pBufferPos += PacketUINT64Size;
		}

		break;

	default:
		CRY_ASSERT_MESSAGE(0, "AddPacketSCryLobbyUserData: Undefined data type");
		break;
	}
}

void RemovePacketSCryLobbyUserData(uint8* pBuffer, uint32 bufferSize, uint32* pBufferPos, SCryLobbyUserData* pData)
{
	switch (pData->m_type)
	{
	case eCSUDT_Int64:
	case eCSUDT_Float64:
		RemovePacketUINT64(pBuffer, bufferSize, pBufferPos, (uint64*)&pData->m_int64);
		break;

	case eCSUDT_Int32:
	case eCSUDT_Float32:
		RemovePacketUINT32(pBuffer, bufferSize, pBufferPos, (uint32*)&pData->m_int32);
		break;

	case eCSUDT_Int16:
		RemovePacketUINT16(pBuffer, bufferSize, pBufferPos, (uint16*)&pData->m_int16);
		break;

	case eCSUDT_Int8:
		RemovePacketUINT8(pBuffer, bufferSize, pBufferPos, (uint8*)&pData->m_int8);
		break;

	case eCSUDT_Int64NoEndianSwap:
		CRY_ASSERT_MESSAGE(pBufferPos, "RemovePacketSCryLobbyUserData: Buffer pos not specified");
		CRY_ASSERT_MESSAGE(*pBufferPos + PacketUINT64Size <= bufferSize, "RemovePacketSCryLobbyUserData: Passed end of buffer");

		if (*pBufferPos + PacketUINT64Size <= bufferSize)
		{
			memcpy(&pData->m_int64, &pBuffer[*pBufferPos], sizeof(pData->m_int64));
			*pBufferPos += sizeof(pData->m_int64);
		}

		break;

	default:
		CRY_ASSERT_MESSAGE(0, "RemovePacketSCryLobbyUserData: Undefined data type");
		break;
	}
}
