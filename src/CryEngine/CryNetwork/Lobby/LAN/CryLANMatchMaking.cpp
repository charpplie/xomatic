
#include "StdAfx.h"
#include "CryLANLobby.h"
#include "CryLANMatchMaking.h"
#include "Network.h"

#define LAN_SEARCH_INTERVAL	1000
#define MAX_SEARCH_RETRIES	5

CCryLANMatchMaking::CCryLANMatchMaking(CCryLobby* lobby, CCryLobbyService* service) : CCryMatchMaking(lobby, service)
{
	// Make the CCryMatchMaking base pointers point to our data so we can use the common code in CCryMatchMaking
	for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
	{
		CCryMatchMaking::m_sessions[i] = &m_sessions[i];
		CCryMatchMaking::m_sessions[i]->localConnection = &m_sessions[i].localConnection;

		for (uint32 j = 0; j < MAX_LOBBY_CONNECTIONS; j++)
		{
			CCryMatchMaking::m_sessions[i]->remoteConnection[j] = &m_sessions[i].remoteConnection[j];
		}
	}

	for (uint32 i = 0; i < MAX_MATCHMAKING_TASKS; i++)
	{
		CCryMatchMaking::m_task[i] = &m_task[i];
	}
}

ECryLobbyError CCryLANMatchMaking::StartTask(ETask etask, CryMatchMakingTaskID* mmTaskID, CryLobbyTaskID* lTaskID, CrySessionHandle h, void* cb, void* cbArg)
{
	CryMatchMakingTaskID tmpMMTaskID;
	CryMatchMakingTaskID* useMMTaskID = mmTaskID ? mmTaskID : &tmpMMTaskID;
	ECryLobbyError error = CCryMatchMaking::StartTask(etask, false, useMMTaskID, lTaskID, h, cb, cbArg);

	if (error == eCLE_Success)
	{
		STask* task = &m_task[*useMMTaskID];

		task->lastUpdate.SetValue(0);
		task->ticks = 0;
	}

	return error;
}

void CCryLANMatchMaking::StartTaskRunning(CryMatchMakingTaskID mmTaskID)
{
	LOBBY_AUTO_LOCK;

	STask* task = &m_task[mmTaskID];

	if (task->used)
	{
		task->running = true;

		switch (task->startedTask)
		{
		case eT_SessionRegisterUserData:
		case eT_SessionCreate:
		case eT_SessionUpdate:
		case eT_SessionStart:
		case eT_SessionEnd:
			// All of these tasks are either synchronous or not needed for LAN but forcing them to wait a tick before succeeding 
			// should ensure less surprises when the asynchronous online versions come along
			StopTaskRunning(mmTaskID);
			break;

		case eT_SessionDelete:
			StartSessionDelete(mmTaskID);
			break;

		case eT_SessionJoin:
			StartSessionJoin(mmTaskID);
			break;

#if NETWORK_HOST_MIGRATION
		case eT_SessionMigrateHostServer:
			HostMigrationServerNT(mmTaskID);
			break;

		case eT_SessionMigrateHostClient:
			TickHostMigrationClientNT(mmTaskID);
			break;
#endif
		}
	}
}

void CCryLANMatchMaking::EndTask(CryMatchMakingTaskID mmTaskID)
{
	LOBBY_AUTO_LOCK;

	STask* task = &m_task[mmTaskID];

	if (task->used)
	{
		if (task->cb)
		{
			switch (task->startedTask)
			{
			case eT_SessionRegisterUserData:
			case eT_SessionUpdate:
			case eT_SessionStart:
			case eT_SessionEnd:
			case eT_SessionDelete:
				((CryMatchmakingCallback)task->cb)(task->lTaskID, task->error, task->cbArg);
				break;

			case eT_SessionCreate:
				((CryMatchmakingSessionCreateCallback)task->cb)(task->lTaskID, task->error, CreateGameSessionHandle(task->session, m_sessions[task->session].localConnection.uid), task->cbArg);
				break;

			case eT_SessionSearch:
				((CryMatchmakingSessionSearchCallback)task->cb)(task->lTaskID, task->error, NULL, task->cbArg);
				break;

			case eT_SessionJoin:
				((CryMatchmakingSessionJoinCallback)task->cb)(task->lTaskID, task->error, CreateGameSessionHandle(task->session, m_sessions[task->session].localConnection.uid), m_sessions[task->session].id.m_ip, m_sessions[task->session].id.m_port, task->cbArg);
				break;
			}
		}

		FreeTask(mmTaskID);

		CryLogAlways("[Lobby] EndTask %d (%d) error %d", task->startedTask, task->subTask, task->error);
	}
}

void CCryLANMatchMaking::StopTaskRunning(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];

	if (task->used)
	{
		task->running = false;
		TO_GAME(&CCryLANMatchMaking::EndTask, this, mmTaskID);
	}
}

ECryLobbyError CCryLANMatchMaking::CreateSessionHandle(CrySessionHandle *h, bool host, uint32 flags, int numUsers)
{
	ECryLobbyError error = CCryMatchMaking::CreateSessionHandle(h, host, numUsers);

	if (error == eCLE_Success)
	{
		SSession* session = &m_sessions[*h];

		session->data.m_data = session->userData;
		session->flags = flags;
		session->numFilledSlots = numUsers;
		session->data.m_numData = m_registeredUserData.num;

		for (uint32 j = 0; j < m_registeredUserData.num; j++)
		{
			session->data.m_data[j] = m_registeredUserData.data[j];
		}
	}

	return error;
}

CryMatchMakingConnectionID CCryLANMatchMaking::AddRemoteConnection(CrySessionHandle h, CryLobbyConnectionID connectionID, CryMatchMakingConnectionUID uid, uint32 ip, uint16 port, uint32 numUsers)
{
	if (connectionID == CryLobbyInvalidConnectionID)
	{
		TNetAddress netAddr = TNetAddress(SIPv4Addr(ntohl(ip), ntohs(port)));

		connectionID = m_lobby->FindConnection(netAddr);

		if (connectionID == CryLobbyInvalidConnectionID)
		{
			connectionID = m_lobby->CreateConnection(netAddr);
		}
	}

	CryMatchMakingConnectionID id = CCryMatchMaking::AddRemoteConnection(h, connectionID, uid, numUsers);

	if (id != CryMatchMakingInvalidConnectionID)
	{
		SSession* session = &m_sessions[h];
		SSession::SRConnection* connection = &session->remoteConnection[id];

		connection->ip = ip;
		connection->port = port;
		connection->m_migrated = false;
		session->numFilledSlots += numUsers;
	}

	return id;
}

void CCryLANMatchMaking::FreeRemoteConnection(CrySessionHandle h, CryMatchMakingConnectionID id)
{
	SSession* session = &m_sessions[h];
	SSession::SRConnection* connection = &session->remoteConnection[id];

	session->numFilledSlots -= connection->numUsers;
	CCryMatchMaking::FreeRemoteConnection(h, id);
}

CrySessionHandle CCryLANMatchMaking::FindSessionFromServerID(CrySessionHandle h)
{
	for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
	{
		if (m_sessions[i].used)
		{
			if (m_sessions[i].id.m_h == h)
			{
				return i;
			}
		}
	}

	return CrySessionInvalidHandle;
}

ECryLobbyError CCryLANMatchMaking::CreateSessionSearchHandle(CrySessionHandle *h)
{
	for (uint32 i = 0; i < MAX_LAN_SEARCHES; i++)
	{
		if (!m_search[i].used)
		{
			m_search[i].used = true;
			m_search[i].numServers = 0;

			*h = i;

			return eCLE_Success;
		}
	}

	return eCLE_TooManyTasks;
};

ECryLobbyError CCryLANMatchMaking::SetSessionUserData(CrySessionHandle h, SCrySessionUserData* data, uint32 numData)
{
	if (m_sessions[h].used)
	{
		for (uint32 i = 0; i < numData; i++)
		{
			uint32 j;

			for (j = 0; j < m_registeredUserData.num; j++)
			{
				if (data[i].m_id == m_registeredUserData.data[j].m_id)
				{
					if (data[i].m_type == m_registeredUserData.data[j].m_type)
					{
						m_sessions[h].data.m_data[j] = data[i];
						break;
					}
					else
					{
						return eCLE_UserDataTypeMissMatch;
					}
				}
			}

			if (j == m_registeredUserData.num)
			{
				return eCLE_UserDataNotRegistered;
			}
		}

		return eCLE_Success;
	}

	return eCLE_InvalidSession;
}

ECryLobbyError CCryLANMatchMaking::SetSessionData(CrySessionHandle h, SCrySessionData* data)
{
	if (m_sessions[h].used)
	{
		m_sessions[h].data.m_numPublicSlots = data->m_numPublicSlots;
		m_sessions[h].data.m_numPrivateSlots = data->m_numPrivateSlots;
		strcpy(m_sessions[h].data.m_name, data->m_name);
		m_sessions[h].data.m_ranked = data->m_ranked;

		return SetSessionUserData(h, data->m_data, data->m_numData);
	}

	return eCLE_InvalidSession;
}

void CCryLANMatchMaking::Tick(CTimeValue tv)
{
	if (m_lobby->MutexTryLock())
	{
		CCryMatchMaking::Tick(tv);

		for (uint32 i = 0; i < MAX_MATCHMAKING_TASKS; i++)
		{
			STask* task = &m_task[i];

			if (task->used && task->running)
			{
				switch (task->subTask)
				{
				case eT_SessionSearch:
					if (task->lastUpdate.GetMilliSecondsAsInt64() + LAN_SEARCH_INTERVAL < tv.GetMilliSecondsAsInt64())
					{
						task->lastUpdate = tv;

						if ((task->ticks == MAX_SEARCH_RETRIES) || task->canceled)
						{
							CryLogAlways("[Lobby] Stop LAN search ticks %d canceled %d", task->ticks, task->canceled);
							m_search[task->session].used = false;
							StopTaskRunning(i);
						}
						else
						{
							uint8 data[3];

							data[0] = Frame_IDToHeader[eH_CryLobby];
							data[1] = eLANPT_MM_RequestServerData;
							data[2] = i;

							CryLogAlways("[Lobby] Send LAN search broadcast %d", task->ticks);
							Send(CryMatchMakingInvalidTaskID, data, 3, TNetAddress(SIPv4Addr(0xffffffffu, m_lobby->GetInternalSocketPort())), false);
						}

						task->ticks++;
					}

					break;

				case eT_SessionJoin:
					TickSessionJoin(i);
					break;

#if NETWORK_HOST_MIGRATION
				case eT_SessionMigrateHostClient:
					TickHostMigrationClientNT(i);
					break;

				case eT_SessionMigrateHostFinish:
					TickHostMigrationFinishNT(i);
					break;
#endif
				}
			}
		}

		m_lobby->MutexUnlock();
	}
}

ECryLobbyError CCryLANMatchMaking::Initialise()
{
	ECryLobbyError error = CCryMatchMaking::Initialise();

	if (error == eCLE_Success)
	{
		m_registeredUserData.num = 0;

		for (uint32 i = 0; i < MAX_LAN_SEARCHES; i++)
		{
			m_search[i].used = false;
		}
	}

	return error;
}

ECryLobbyError CCryLANMatchMaking::SessionRegisterUserData(SCrySessionUserData* data, uint32 numData, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	if (numData < MAX_MATCHMAKING_SESSION_USER_DATA)
	{
		CryMatchMakingTaskID tid;
		
		error = StartTask(eT_SessionRegisterUserData, &tid, taskID, 0, (void*)cb, cbArg);

		if (error == eCLE_Success)
		{
			memcpy(m_registeredUserData.data, data, numData*sizeof(data[0]));
			m_registeredUserData.num = numData;

			FROM_GAME(&CCryLANMatchMaking::StartTaskRunning, this, tid);
		}
	}
	else
	{
		error = eCLE_OutOfSessionUserData;
	}

	CryLogAlways("[Lobby] Start SessionRegisterUserData error %d", error);

	return error;
}

ECryLobbyError CCryLANMatchMaking::SessionCreate(uint32* users, int numUsers, uint32 flags, SCrySessionData* data, CryLobbyTaskID* taskID, CryMatchmakingSessionCreateCallback cb, void* cbArg)
{
	LOBBY_AUTO_LOCK;

	CrySessionHandle h;
	ECryLobbyError error = CreateSessionHandle(&h, true, flags, numUsers);

	if (error == eCLE_Success)
	{
		SSession* session = &m_sessions[h];

		error = SetSessionData(h, data);

		if (error == eCLE_Success)
		{
			CryMatchMakingTaskID tid;

			error = StartTask(eT_SessionCreate, &tid, taskID, h, (void*)cb, cbArg);

			if (error == eCLE_Success)
			{
				session->localConnection.uid = CreateConnectionUID();
				CryLogAlways("[Lobby] Created local connection uid %d", session->localConnection.uid);
				FROM_GAME(&CCryLANMatchMaking::StartTaskRunning, this, tid);
			}
			else
			{
				FreeSessionHandle(h);
			}
		}
		else
		{
			FreeSessionHandle(h);
		}
	}

	CryLogAlways("[Lobby] Start SessionCreate error %d", error);

	return error;
}

ECryLobbyError CCryLANMatchMaking::SessionMigrate(CrySessionHandle h,uint32* pUsers, int numUsers, uint32 flags, SCrySessionData* pData, CryLobbyTaskID* pTaskID, CryMatchmakingSessionCreateCallback pCB, void* pCBArg)
{
	LOBBY_AUTO_LOCK;

	ECryLobbyError error = eCLE_Success;

	h = GetSessionHandleFromGameSessionHandle(h);

	if (h != CrySessionInvalidHandle)
	{
		SSession* session = &m_sessions[h];

		error = SetSessionData(h, pData);

		if (error == eCLE_Success)
		{
			CryMatchMakingTaskID tid;

			error = StartTask(eT_SessionCreate, &tid, pTaskID, h, (void*)pCB, pCBArg);

			if (error == eCLE_Success)
			{
				session->host = true;
				session->flags = flags;
				FROM_GAME(&CCryLANMatchMaking::StartTaskRunning, this, tid);
			}
			else
			{
				FreeSessionHandle(h);
			}
		}
		else
		{
			FreeSessionHandle(h);
		}
	}

	CryLogAlways("[Lobby] Start SessionMigrate error %d", error);

	return error;
}


ECryLobbyError CCryLANMatchMaking::SessionUpdate(CrySessionHandle h, SCrySessionUserData* data, uint32 numData, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	h = GetSessionHandleFromGameSessionHandle(h);

	if ((h < MAX_MATCHMAKING_SESSIONS) && m_sessions[h].used)
	{
		if (m_sessions[h].host)
		{
			error = SetSessionUserData(h, data, numData);

			if (error == eCLE_Success)
			{
				CryMatchMakingTaskID tid;

				error = StartTask(eT_SessionUpdate, &tid, taskID, h, (void*)cb, cbArg);
				
				if (error == eCLE_Success)
				{
					FROM_GAME(&CCryLANMatchMaking::StartTaskRunning, this, tid);
				}
			}
		}
		else
		{
			error = eCLE_InvalidRequest;
		}
	}
	else
	{
		error = eCLE_InvalidSession;
	}

	CryLogAlways("[Lobby] Start SessionUpdate error %d", error);

	return error;
}

ECryLobbyError CCryLANMatchMaking::SessionStart(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	h = GetSessionHandleFromGameSessionHandle(h);

	if ((h < MAX_MATCHMAKING_SESSIONS) && m_sessions[h].used)
	{
		CryMatchMakingTaskID tid;

		error = StartTask(eT_SessionStart, &tid, taskID, h, (void*)cb, cbArg);

		if (error == eCLE_Success)
		{
			FROM_GAME(&CCryLANMatchMaking::StartTaskRunning, this, tid);
		}
	}
	else
	{
		error = eCLE_InvalidSession;
	}

	CryLogAlways("[Lobby] Start SessionStart error %d", error);

	return error;
}

ECryLobbyError CCryLANMatchMaking::SessionEnd(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	h = GetSessionHandleFromGameSessionHandle(h);

	if ((h < MAX_MATCHMAKING_SESSIONS) && m_sessions[h].used)
	{
		CryMatchMakingTaskID tid;

		error = StartTask(eT_SessionEnd, &tid, taskID, h, (void*)cb, cbArg);

		if (error == eCLE_Success)
		{
			FROM_GAME(&CCryLANMatchMaking::StartTaskRunning, this, tid);
		}
	}
	else
	{
		error = eCLE_InvalidSession;
	}

	CryLogAlways("[Lobby] Start SessionEnd error %d", error);

	return error;
}

ECryLobbyError CCryLANMatchMaking::SessionDelete(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	h = GetSessionHandleFromGameSessionHandle(h);

	if ((h < MAX_MATCHMAKING_SESSIONS) && m_sessions[h].used)
	{
		CryMatchMakingTaskID tid;

		error = StartTask(eT_SessionDelete, &tid, taskID, h, (void*)cb, cbArg);

		if (error == eCLE_Success)
		{
			FROM_GAME(&CCryLANMatchMaking::StartTaskRunning, this, tid);
		}
	}
	else
	{
		error = eCLE_InvalidSession;
	}

	CryLogAlways("[Lobby] Start SessionDelete error %d", error);

	return error;
}

void CCryLANMatchMaking::StartSessionDelete(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];

	task->subTask = eT_SessionDelete;

	// Disconnect our local connection
	SessionDisconnectRemoteConnectionViaNub(eDC_UserRequested, session->localConnection.uid, "Session deleted");

	// Free any remaining remote connections
	for (uint32 i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
	{
		SSession::SRConnection* connection = &session->remoteConnection[i];

		if (connection->used)
		{
			FreeRemoteConnection(task->session, i);
		}
	}

	FreeSessionHandle(task->session);

	StopTaskRunning(mmTaskID);
}

ECryLobbyError CCryLANMatchMaking::SessionSearch(uint32 user, SCrySessionSearchParam* param, CryLobbyTaskID* taskID, CryMatchmakingSessionSearchCallback cb, void* cbArg)
{
	LOBBY_AUTO_LOCK;

	CrySessionHandle h;
	ECryLobbyError error = CreateSessionSearchHandle(&h);

	m_lobby->InternalSocketCreate(false);

	if (error == eCLE_Success)
	{
		CryMatchMakingTaskID tid;
	
		error = StartTask(eT_SessionSearch, &tid, taskID, h, (void*)cb, cbArg);

		if (error == eCLE_Success)
		{
			FROM_GAME(&CCryLANMatchMaking::StartTaskRunning, this, tid);
		}
		else
		{
			m_search[h].used = false;
		}
	}

	CryLogAlways("[Lobby] Start SessionSearch error %d", error);

	return error;
}

ECryLobbyError CCryLANMatchMaking::SessionJoin(uint32* users, int numUsers, CrySessionID id, CryLobbyTaskID* taskID, CryMatchmakingSessionJoinCallback cb, void* cbArg)
{
	LOBBY_AUTO_LOCK;

	CrySessionHandle h;
	ECryLobbyError error = CreateSessionHandle(&h, false, 0, numUsers);

	if (error == eCLE_Success)
	{
		CryMatchMakingTaskID tid;

		error = StartTask(eT_SessionJoin, &tid, taskID, h, (void*)cb, cbArg);

		if (error == eCLE_Success)
		{
			m_sessions[h].id = *((SCryLANSessionID*)id.get());
			FROM_GAME(&CCryLANMatchMaking::StartTaskRunning, this, tid);
		}
		else
		{
			FreeSessionHandle(h);
		}
	}

	CryLogAlways("[Lobby] Start SessionJoin error %d", error);

	return error;
}


static void MatchmakingSessionJoinFromConsoleCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, uint32 ip, uint16 port, void* arg)
{
	if (error == eCLE_Success)
	{
		char command[128];
		_snprintf(command, sizeof(command), "connect <session>%d,%d.%d.%d.%d:%d", h, ((uint8*)&ip)[0], ((uint8*)&ip)[1], ((uint8*)&ip)[2], ((uint8*)&ip)[3], port);
		IConsole* pConsole = gEnv->pConsole;
		if (pConsole)
		{
			pConsole->ExecuteString(command, false, true);
		}
	}
}

void CCryLANMatchMaking::SessionJoinFromConsole(void)
{
	IConsole* pConsole = gEnv->pConsole;
	CryFixedStringT<32> host;
	uint16 port = SERVER_DEFAULT_PORT;

	if (pConsole)
	{
		ICVar* pCVar = pConsole->GetCVar("cl_serveraddr");
		if (pCVar)
		{
			host = pCVar->GetString();
		}

		pCVar = pConsole->GetCVar("cl_serverport");
		if (pCVar)
		{
			port = pCVar->GetIVal();
		}
	}

	if (host.find("<session>") == CryStringT<char>::npos)
	{
		TNetAddressVec addrs;
		CNetAddressResolver netAddressResolver;
		CNameRequestPtr pReq = netAddressResolver.RequestNameLookup(host.c_str());
		pReq->Wait();
		if (pReq->GetResult(addrs) != eNRR_Succeeded)
		{
			CryLogAlways("Name resolution for '%s' failed", host.c_str());
			return;
		}

		SIPv4Addr* pAddr = addrs[0].GetPtr<SIPv4Addr>();
		if (pAddr)
		{
			SCryLANSessionID* pId = new SCryLANSessionID;
			pId->m_ip = htonl(pAddr->addr);
			pId->m_port = port;
			pId->m_h = CrySessionInvalidHandle;

			uint32 users = 0;
			SessionJoin(&users, 1, pId, NULL, MatchmakingSessionJoinFromConsoleCallback, NULL);
		}
	}
}

void CCryLANMatchMaking::StartSessionJoin(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];
	const uint32 MaxBufferSize = PacketHeaderSize + PacketUINT32Size + PacketUINT8Size;
	uint8 buffer[MaxBufferSize];
	uint32 bufferSize = 0;

	StartAddPacket(buffer, MaxBufferSize, &bufferSize, eLANPT_SessionRequestJoin);
	AddPacketUINT32(buffer, MaxBufferSize, &bufferSize, session->id.m_h);
	AddPacketUINT8(buffer, MaxBufferSize, &bufferSize, session->localConnection.numUsers);

	if (Send(mmTaskID, buffer, bufferSize, TNetAddress(SIPv4Addr(ntohl(session->id.m_ip), session->id.m_port)), true) != eSE_Ok)
	{
		UpdateTaskError(mmTaskID, eCLE_ConnectionFailed);
	}
}

void CCryLANMatchMaking::TickSessionJoin(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];

	if (task->sendStatus != eCLE_Pending)
	{
		if (task->sendStatus == eCLE_Success)
		{
			// The request to join has been sent so wait for result
			if (task->TimerStarted())
			{
				if (task->GetTimer() > CryMatchMakingConnectionTimeOut)
				{
					// No response so fail connection attempt
					CryLogAlways("[Lobby] SessionJoin request session join packet sent no response received");
					UpdateTaskError(mmTaskID, eCLE_ConnectionFailed);
					StartSessionDelete(mmTaskID);
				}
			}
			else
			{
				task->StartTimer();
				CryLogAlways("[Lobby] SessionJoin request session join packet sent waiting for response");
			}
		}
		else
		{
			UpdateTaskError(mmTaskID, eCLE_ConnectionFailed);
			CryLogAlways("[Lobby] SessionJoin error sending request session join packet error %d", task->error);
			StartSessionDelete(mmTaskID);
		}
	}
}

void CCryLANMatchMaking::ProcessSessionRequestJoin(const TNetAddress& addr, const uint8* data, uint32 length)
{
	ECryLobbyError error = eCLE_Success;
	uint32 bufferPos = 0;
	CrySessionHandle h;
	CryLobbyConnectionID c;

	if (m_lobby->ConnectionFromAddress(&c, addr))
	{
		sockaddr_in saddr;

		if (ConvertAddr(addr, &saddr))
		{
			StartRemovePacket(data, length, &bufferPos);
			RemovePacketUINT32(data, length, &bufferPos, &h);

			// If the 'connect' command is used from the console with no session then we'll have an invalid
			// session handle here.  In this case, search for the first searchable host session and use that.
			if (h == CrySessionInvalidHandle)
			{
				for (int32 index = 0; index < MAX_MATCHMAKING_SESSIONS; ++index)
				{
					SSession* session = &m_sessions[index];
					if (session->used && session->host && (session->flags & CRYSESSION_CREATE_FLAG_SEARCHABLE))
					{
						h = index;
						break;
					}
				}
			}

			if (h != CrySessionInvalidHandle && m_sessions[h].used)
			{
				SSession* session = &m_sessions[h];
				uint8 numUsers;

				RemovePacketUINT8(data, length, &bufferPos, &numUsers);

				if (session->numFilledSlots + numUsers <= session->data.m_numPublicSlots + session->data.m_numPrivateSlots)
				{
					CryMatchMakingConnectionID id = AddRemoteConnection(h, c, CreateConnectionUID(), saddr.sin_addr.s_addr, saddr.sin_port, numUsers);

					if (id != CryMatchMakingInvalidConnectionID)
					{
						// Added to session
						TMemHdl mh = m_lobby->MemAlloc(MAX_LOBBY_PACKET_SIZE);

						if (mh != TMemInvalidHdl)
						{
							SSession::SRConnection* connection = &session->remoteConnection[id];
							uint8* buffer = (uint8*)m_lobby->MemGetPtr(mh);
							uint32 bufferSize = 0;

							StartAddPacket(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, eLANPT_SessionRequestJoinResult);
							AddPacketError(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, error);
							AddPacketUINT32(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, connection->uid);
							AddPacketUINT8(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, session->localConnection.numUsers);
							AddPacketUINT32(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, session->localConnection.uid);

							Send(CryMatchMakingInvalidTaskID, buffer, bufferSize, addr, true);

							// Send the new clients connection to the old clients
							StartAddPacket(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, eLANPT_SessionAddRemoteConnections);

							AddPacketUINT32(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, h);
							AddPacketUINT8(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, 1);

							AddPacketUINT32(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, htonl(connection->ip));
							AddPacketUINT16(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, htons(connection->port));
							AddPacketUINT32(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, connection->uid);
							AddPacketUINT8(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, connection->numUsers);

							CryLogAlways("[Lobby] Send new users to peers");
							SendToAll(CryMatchMakingInvalidTaskID, buffer, bufferSize, h, id, true);

							// Send the remote connections to the new client
							uint32 connectionToAdd = 0;

							while (true)
							{
								uint8 numConnections = 0;
								uint8 numAdded = 0;
								uint32 testSize;
								uint32 i;

								StartAddPacket(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, eLANPT_SessionAddRemoteConnections);
								AddPacketUINT32(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, h);

								// Find out how many of the remote connections can fit in the packet

								testSize = bufferSize + PacketUINT8Size;

								for (i = connectionToAdd; i < MAX_LOBBY_CONNECTIONS; i++)
								{
									SSession::SRConnection* connectionAdd = &session->remoteConnection[i];

									if (connectionAdd->used && (connectionAdd != connection))
									{
										uint32 add = PacketUINT32Size + PacketUINT16Size + PacketUINT32Size + PacketUINT8Size;

										if (testSize + add < MAX_LOBBY_PACKET_SIZE)
										{
											testSize += add;
											numConnections++;
										}
										else
										{
											break;
										}
									}
								}

								if (numConnections > 0)
								{
									// Add and send the connections
									AddPacketUINT8(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, numConnections);

									for (i = connectionToAdd, numAdded = 0; (i < MAX_LOBBY_CONNECTIONS) && (numAdded < numConnections); i++, connectionToAdd++)
									{
										SSession::SRConnection* connectionAdd = &session->remoteConnection[i];

										if (connectionAdd->used && (connectionAdd != connection))
										{
											AddPacketUINT32(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, htonl(connectionAdd->ip));
											AddPacketUINT16(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, htons(connectionAdd->port));
											AddPacketUINT32(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, connectionAdd->uid);
											AddPacketUINT8(buffer, MAX_LOBBY_PACKET_SIZE, &bufferSize, connectionAdd->numUsers);
											numAdded++;

											CryLogAlways("[Lobby] Send connection %d uid %d users to new connection %d", i, connectionAdd->uid, id);
										}
									}

									Send(CryMatchMakingInvalidTaskID, buffer, bufferSize, addr, true);
								}
								else
								{
									// No more connections to send
									break;
								}
							}

							m_lobby->MemFree(mh);
						}
						else
						{
							FreeRemoteConnection(h, id);
							error = eCLE_ConnectionFailed;
						}
					}
					else
					{
						error = eCLE_SessionFull;
					}
				}
				else
				{
					error = eCLE_SessionFull;
				}
			}
			else
			{
				error = eCLE_ConnectionFailed;
			}			
		}
		else
		{
			error = eCLE_ConnectionFailed;
		}			
	}
	else
	{
		error = eCLE_ConnectionFailed;
	}

	if (error != eCLE_Success)
	{
		// Can't add to session so send back error
		const uint32 MaxBufferSize = PacketHeaderSize + PacketErrorSize;
		uint8 buffer[MaxBufferSize];
		uint32 bufferSize = 0;

		StartAddPacket(buffer, MaxBufferSize, &bufferSize, eLANPT_SessionRequestJoinResult);
		AddPacketError(buffer, MaxBufferSize, &bufferSize, error);

		Send(CryMatchMakingInvalidTaskID, buffer, bufferSize, addr, true);
	}

	CryLogAlways("[Lobby] Processed Session request join packet error %d", error);
}

void CCryLANMatchMaking::ProcessSessionRequestJoinResult(const TNetAddress& addr, const uint8* data, uint32 length)
{
	ECryLobbyError error = eCLE_Success;
	CryLobbyConnectionID c;

	if (m_lobby->ConnectionFromAddress(&c, addr))
	{
		CryMatchMakingTaskID mmTaskID = FindTaskFromTaskConnectionID(eT_SessionJoin, c);

		if (mmTaskID != CryMatchMakingInvalidTaskID)
		{
			STask* task = &m_task[mmTaskID];
			sockaddr_in saddr;

			if (ConvertAddr(addr, &saddr))
			{
				uint32 bufferPos = 0;

				StartRemovePacket(data, length, &bufferPos);
				RemovePacketError(data, length, &bufferPos, &error);

				CryLogAlways("[Lobby] Received SessionRequestJoinResult error %d", error);

				UpdateTaskError(mmTaskID, error);

				if (task->error == eCLE_Success)
				{
					SSession* session = &m_sessions[task->session];
					uint8 numUsers;
					uint32 hostConnectionUID;

					RemovePacketUINT32(data, length, &bufferPos, &session->localConnection.uid);

					CryLogAlways("[Lobby] Created local connection uid %d", session->localConnection.uid);

					RemovePacketUINT8(data, length, &bufferPos, &numUsers);
					RemovePacketUINT32(data, length, &bufferPos, &hostConnectionUID);
					CryMatchMakingConnectionID id = AddRemoteConnection(task->session, c, hostConnectionUID, saddr.sin_addr.s_addr, saddr.sin_port, numUsers);

					if (id != CryMatchMakingInvalidConnectionID)
					{
						CryLogAlways("[Lobby] Created server connection %d uid %d", id, hostConnectionUID);
						StopTaskRunning(mmTaskID);
						session->hostConnectionID = id;
					}
					else
					{
						UpdateTaskError(mmTaskID, eCLE_ConnectionFailed);
					}
				}
			}
			else
			{
				UpdateTaskError(mmTaskID, eCLE_ConnectionFailed);
			}

			if (task->error != eCLE_Success)
			{
				StartSessionDelete(mmTaskID);
			}

			CryLogAlways("[Lobby] Processed session request join result error %d", task->error);
		}
	}
}

void CCryLANMatchMaking::ProcessSessionAddRemoteConnections(const TNetAddress& addr, const uint8* data, uint32 length)
{
	uint32 bufferPos = 0;
	CrySessionHandle h;

	StartRemovePacket(data, length, &bufferPos);
	RemovePacketUINT32(data, length, &bufferPos, &h);

	h = FindSessionFromServerID(h);

	if (h != CrySessionInvalidHandle)
	{
		SSession* session = &m_sessions[h];
		uint8 numConnections;

		RemovePacketUINT8(data, length, &bufferPos, &numConnections);

		for (uint32 i = 0; i < numConnections; i++)
		{
			uint32 ip;
			uint16 port;
			uint8 numUsers;
			uint32 connectionUID;

			RemovePacketUINT32(data, length, &bufferPos, &ip);
			RemovePacketUINT16(data, length, &bufferPos, &port);
			RemovePacketUINT32(data, length, &bufferPos, &connectionUID);
			RemovePacketUINT8(data, length, &bufferPos, &numUsers);

			CryMatchMakingConnectionID id = AddRemoteConnection(h, CryLobbyInvalidConnectionID, connectionUID, ntohl(ip), ntohs(port), numUsers);

			if (id != CryMatchMakingInvalidConnectionID)
			{
				CryLogAlways("[Lobby] Add new connection %d uid %d", id, connectionUID);
			}
		}
	}
}

size_t CCryLANMatchMaking::CalculateServerDataSize(CrySessionHandle h)
{
	size_t size =
		PacketHeaderSize +
		PacketUINT8Size +					// Requesters task id
		PacketUINT32Size +				// Session handle
		PacketUINT32Size +				// Num filled slots
		PacketUINT32Size +				// Num public slots
		PacketUINT32Size +				// Num private slots
		MAX_SESSION_NAME_LENGTH +	// Session name
		PacketBoolSize;						// Ranked

	for (uint32 i = 0; i < m_sessions[h].data.m_numData; i++)
	{
		switch (m_sessions[h].data.m_data[i].m_type)
		{
		case eCSUDT_Int64:
		case eCSUDT_Float64:
		case eCSUDT_Int64NoEndianSwap:
			size += PacketUINT64Size;
			break;

		case eCSUDT_Int32:
		case eCSUDT_Float32:
			size += PacketUINT32Size;
			break;

		case eCSUDT_Int16:
			size += PacketUINT16Size;
			break;

		case eCSUDT_Int8:
			size += PacketUINT8Size;
			break;
		}
	}

	return size;
}

void CCryLANMatchMaking::SendServerData(const TNetAddress& addr, const uint8* datain, uint32 length)
{
	LOBBY_AUTO_LOCK;

	for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
	{
		SSession* session = &m_sessions[i];
		if (session->used && session->host && (session->flags&CRYSESSION_CREATE_FLAG_SEARCHABLE))
		{
			uint32 bufferSize = CalculateServerDataSize(i);
			TMemHdl h = m_lobby->MemAlloc(bufferSize);

			if (h != TMemInvalidHdl)
			{
				uint8 *data = (uint8*)m_lobby->MemGetPtr(h);
				uint32 size = 0;

				StartAddPacket(data, bufferSize, &size, eLANPT_MM_ServerData);
				AddPacketUINT8(data, bufferSize, &size, datain[2]);	// Requesters task id

				AddPacketUINT32(data, bufferSize, &size, i);
				AddPacketUINT32(data, bufferSize, &size, session->numFilledSlots);
				AddPacketUINT32(data, bufferSize, &size, session->data.m_numPublicSlots);
				AddPacketUINT32(data, bufferSize, &size, session->data.m_numPrivateSlots);

				memcpy(data + size, session->data.m_name, MAX_SESSION_NAME_LENGTH);
				size += MAX_SESSION_NAME_LENGTH;

				AddPacketBool(data, bufferSize, &size, session->data.m_ranked);

				for (uint32 j = 0; j < session->data.m_numData; j++)
				{
					AddPacketSCryLobbyUserData(data, bufferSize, &size, &session->data.m_data[j]);
				}

				Send(CryMatchMakingInvalidTaskID, data, size, addr, false);
				m_lobby->MemFree(h);
			}
		}
	}
}

void CCryLANMatchMaking::ServerDataToGame(CryMatchMakingTaskID mmTaskID, uint32 ip, uint16 port, TMemHdl params, uint32 length)
{
	LOBBY_AUTO_LOCK;

	STask* task = &m_task[mmTaskID];
	SCrySessionSearchResult result;
	SCrySessionUserData userData[MAX_MATCHMAKING_SESSION_USER_DATA];
	SCryLANSessionID* id = new SCryLANSessionID;
	uint8* data = (uint8*)m_lobby->MemGetPtr(params);
	uint32 index;
	uint8 taskID8;

	StartRemovePacket(data, length, &index);
	RemovePacketUINT8(data, length, &index, &taskID8);

	id->m_ip = ip;
	id->m_port = port;
	RemovePacketUINT32(data, length, &index, &id->m_h);
	result.m_id = id;

	RemovePacketUINT32(data, length, &index, &result.m_numFilledSlots);
	RemovePacketUINT32(data, length, &index, &result.m_data.m_numPublicSlots);
	RemovePacketUINT32(data, length, &index, &result.m_data.m_numPrivateSlots);

	memcpy(result.m_data.m_name, data + index, MAX_SESSION_NAME_LENGTH);
	index += MAX_SESSION_NAME_LENGTH;

	RemovePacketBool(data, length, &index, &result.m_data.m_ranked);

	result.m_data.m_data = userData;
	result.m_data.m_numData = m_registeredUserData.num;

	for (uint32 i = 0; i < m_registeredUserData.num; i++)
	{
		result.m_data.m_data[i].m_id = m_registeredUserData.data[i].m_id;
		result.m_data.m_data[i].m_type = m_registeredUserData.data[i].m_type;
		RemovePacketSCryLobbyUserData(data, length, &index, &result.m_data.m_data[i]);
	}

	if (task->cb)
	{
		((CryMatchmakingSessionSearchCallback)task->cb)(task->lTaskID, eCLE_SuccessContinue, &result, task->cbArg);
	}

	m_lobby->MemFree(params);
}

void CCryLANMatchMaking::ProcessServerData(const TNetAddress& addr, const uint8* data, uint32 length)
{
	sockaddr_in sockAddr;

	LOBBY_AUTO_LOCK;

	if (ConvertAddr(addr, &sockAddr))
	{
		uint32 taskID = data[2];

		if ((taskID < MAX_MATCHMAKING_TASKS) && m_task[taskID].used && m_task[taskID].running && (m_task[taskID].startedTask == eT_SessionSearch))
		{
			STask* task = &m_task[taskID];
			SSearch* search = &m_search[task->session];

			if (search->numServers < MAX_LAN_SEARCH_SERVERS)
			{
				uint32 ip = sockAddr.sin_addr.s_addr;
				uint16 port = ntohs(sockAddr.sin_port);
				bool newserver = true;

				for (uint32 i = 0; i < search->numServers; i++)
				{
					if ((search->servers[i].m_ip == ip) && (search->servers[i].m_port == port))
					{
						newserver = false;
						break;
					}
				}

				if (newserver)
				{
					search->servers[search->numServers].m_ip = ip;
					search->servers[search->numServers].m_port = port;
					search->numServers++;

					CryLogAlways("[Lobby] Found Session %d.%d.%d.%d:%d", ((uint8*)&ip)[0], ((uint8*)&ip)[1], ((uint8*)&ip)[2], ((uint8*)&ip)[3], port);

					TMemHdl params = m_lobby->MemAlloc(length);
					memcpy(m_lobby->MemGetPtr(params), data, length);
					TO_GAME(&CCryLANMatchMaking::ServerDataToGame, this, taskID, ip, port, params, length);				
				}
			}
		}
	}
}

void CCryLANMatchMaking::OnPacket(const TNetAddress& addr, const uint8* data, uint32 length)
{
	CryLANLobbyPacketType type;
	uint32 pos;

	StartRemovePacket(data, length, &pos, &type);

	switch (type)
	{
	case eLANPT_MM_RequestServerData:
		SendServerData(addr, data, length);
		break;

	case eLANPT_MM_ServerData:
		ProcessServerData(addr, data, length);
		break;

	case eLANPT_SessionRequestJoin:
		ProcessSessionRequestJoin(addr, data, length);
		break;

	case eLANPT_SessionRequestJoinResult:
		ProcessSessionRequestJoinResult(addr, data, length);
		break;

	case eLANPT_SessionAddRemoteConnections:
		ProcessSessionAddRemoteConnections(addr, data, length);
		break;

#if NETWORK_HOST_MIGRATION
	case eLANPT_HostMigrationServer:
		ProcessHostMigrationFromServer(addr, data, length);
		break;

	case eLANPT_HostMigrationClient:
		ProcessHostMigrationFromClient(addr, data, length);
		break;
#endif

	default:
		CCryMatchMaking::OnPacket(addr, data, length);
		break;
	}
}

#if NETWORK_HOST_MIGRATION
void CCryLANMatchMaking::HostMigrationInitiate()
{
	m_hostMigrationFinished = false;
}

ECryLobbyError CCryLANMatchMaking::HostMigrationServer(void)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	CCryRebroadcaster* pRebroadcaster = m_lobby->GetRebroadcaster();
	TNetChannelID hostChannelID = 0;
	if (pRebroadcaster)
	{
		hostChannelID = pRebroadcaster->GetLocalChannelID();
	}

	CrySessionHandle h = CrySessionInvalidHandle;
	if (FindLocalConnectionFromUID(hostChannelID, &h))
	{
		SSession* session = &m_sessions[h];
		CryMatchMakingTaskID mmTaskID;

		session->host = true;
		session->hostConnectionID = CryMatchMakingInvalidConnectionID;
		error = StartTask(eT_SessionMigrateHostServer, &mmTaskID, NULL, h, NULL, NULL);

		if (error == eCLE_Success)
		{
			CryLog("Host Migration: matchmaking migrating the session on the SERVER");
			FROM_GAME(&CCryLANMatchMaking::StartTaskRunning, this, mmTaskID);
		}
	}
	else
	{
		error = eCLE_InvalidSession;
	}

	if (error != eCLE_Success)
	{
		CryLog("Host Migration: CCryLANMatchMaking::HostMigrationServer() error %d", error);
	}

	return error;
}

void CCryLANMatchMaking::HostMigrationServerNT(CryMatchMakingTaskID mmTaskID)
{
	CryLog("Host Migration: matchmaking SERVER migrated session successfully - informing CLIENTS");

	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];

	const uint32 MaxBufferSize = PacketHeaderSize + sizeof(uint32);
	uint8 buffer[MaxBufferSize];
	uint32 bufferSize = 0;

	StartAddPacket(buffer, MaxBufferSize, &bufferSize, eLANPT_HostMigrationServer);
	AddPacketUINT32(buffer, MaxBufferSize, &bufferSize, mmTaskID);

	SendToAll(mmTaskID, buffer, bufferSize, task->session, CryMatchMakingInvalidConnectionID, true);

	StartSubTask(eT_SessionMigrateHostFinish, mmTaskID);
	task->StartTimer();

	m_newHostAddress = TNetAddress(TLocalNetAddress());
	m_newHostAddressValid = true;
	m_hostMigrationFinished = true;
}

ECryLobbyError CCryLANMatchMaking::HostMigrationClient(CrySessionHandle h, CryMatchMakingTaskID hostTaskID)
{
	ECryLobbyError error = eCLE_Success;

	if ((h < MAX_MATCHMAKING_SESSIONS) && m_sessions[h].used)
	{
		SSession* session = &m_sessions[h];
		CryMatchMakingTaskID mmTaskID;

		session->host = false;
		error = StartTask(eT_SessionMigrateHostClient, &mmTaskID, NULL, h, NULL, NULL);

		if (error == eCLE_Success)
		{
			CryLog("Host Migration: matchmaking migrating to the new session on the CLIENT");

			STask* task = &m_task[mmTaskID];
			task->returnTaskID = hostTaskID;
			StartTaskRunning(mmTaskID);
		}
	}
	else
	{
		error = eCLE_InvalidSession;
	}

	if (error != eCLE_Success)
	{
		CryLog("Host Migration: CCryLANMatchMaking::HostMigrationClient() error %d", error);
	}

	return error;

}

void CCryLANMatchMaking::TickHostMigrationClientNT(CryMatchMakingTaskID mmTaskID)
{
	CryLog("Host Migration: matchmaking CLIENT migrated to new session successfully");

	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];

	const uint32 MaxBufferSize = PacketHeaderSize + sizeof(uint32);
	uint8 buffer[MaxBufferSize];
	uint32 bufferSize = 0;

	StartAddPacket(buffer, MaxBufferSize, &bufferSize, eLANPT_HostMigrationClient);
	AddPacketUINT32(buffer, MaxBufferSize, &bufferSize, task->returnTaskID);

	Send(mmTaskID, buffer, bufferSize, task->session, session->hostConnectionID, true);
	m_hostMigrationFinished = true;
	m_newHostAddressValid = true;

	StopTaskRunning(mmTaskID);
}

void CCryLANMatchMaking::ProcessHostMigrationFromServer(const TNetAddress& addr, const uint8* pData, uint32 length)
{
	uint32 bufferPos = 0;
	CrySessionHandle h = CrySessionInvalidHandle;
	CryMatchMakingTaskID hostTaskID = CryMatchMakingInvalidTaskID;

	CryLog("Host Migration: matchmaking CLIENT received session details - migrating to new session");

	StartRemovePacket(pData, length, &bufferPos);
	RemovePacketUINT32(pData, length, &bufferPos, &hostTaskID);

	CryLobbyConnectionID lobbyCxID = m_lobby->FindConnection(addr);
	CryMatchMakingConnectionUID mmCxUID = CryMatchMakingInvalidConnectionID;
	CryMatchMakingConnectionID mmCxID = CryMatchMakingInvalidConnectionID;
	if (FindConnectionFromLobbyConnectionID(lobbyCxID, &mmCxUID))
	{
		if (FindConnectionFromUID(mmCxUID, &h, &mmCxID))
		{
			SSession* session = &m_sessions[h];
			session->hostConnectionID = mmCxID;

			HostMigrationClient(h, hostTaskID);
			m_newHostAddress = addr;
		}
	}

	// If HostMigrationClient() was not called, this client will be pruned (after timeout) on the new host
}

void CCryLANMatchMaking::ProcessHostMigrationFromClient(const TNetAddress& addr, const uint8* pData, uint32 length)
{
	uint32 bufferPos = 0;
	CryMatchMakingTaskID hostTaskID = CryMatchMakingInvalidTaskID;

	StartRemovePacket(pData, length, &bufferPos);
	RemovePacketUINT32(pData, length, &bufferPos, &hostTaskID);

	CryLobbyConnectionID c;
	if (m_lobby->ConnectionFromAddress(&c, addr))
	{
		hostTaskID = FindTaskFromTaskTaskID(eT_SessionMigrateHostFinish, hostTaskID);
		if (hostTaskID != CryMatchMakingInvalidTaskID)
		{
			STask* task = &m_task[hostTaskID];
			SSession* session = &m_sessions[task->session];

			for (uint32 i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
			{
				SSession::SRConnection* connection = &session->remoteConnection[i];

				if (connection->used && (connection->connectionID == c))
				{
					connection->m_migrated = true;
					CryLog("Host Migration: matchmaking SERVER received CLIENT confirmation (connection %i, uid %i)", i, connection->uid);
					return;
				}
			}
		}
	}

	CryLog("Host Migration: matchmaking SERVER received CLIENT confirmation for unknown connection");
}

void CCryLANMatchMaking::TickHostMigrationFinishNT(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];
	bool finished = true;

	for (uint32 index = 0; finished && index < MAX_LOBBY_CONNECTIONS; ++index)
	{
		if (session->remoteConnection[index].used && !session->remoteConnection[index].m_migrated)
		{
			finished = false;
		}
	}

	uint32 timeout = CryMatchMakingHostMigrationDefaultTimeout;
	if (gEnv->pConsole)
	{
		ICVar* pHostMigrationTimeout = gEnv->pConsole->GetCVar("net_migrate_timeout");
		if (pHostMigrationTimeout)
		{
			timeout = (uint32)(pHostMigrationTimeout->GetFVal() * 1000.0f);
		}
	}

	if (finished || (task->GetTimer() > timeout))
	{
		CryLog("Host Migration: matchmaking host migration finished");

		for (uint32 index = 0; index < MAX_LOBBY_CONNECTIONS; ++index)
		{
			if (session->remoteConnection[index].used)
			{
				if (session->remoteConnection[index].m_migrated)
				{
					// This connection migrated so reset for next time
					session->remoteConnection[index].m_migrated = false;
				}
				else
				{
					// Prune this connection
					CryLog("Host Migration: matchmaking pruning client uid %i", session->remoteConnection[index].uid);
					SessionDisconnectRemoteConnectionViaNub(eDC_FailedToMigrateToNewHost, session->remoteConnection[index].uid, "Failed to migrate player to new game");
				}
			}
		}

		StopTaskRunning(mmTaskID);
	}
}

bool CCryLANMatchMaking::GetNewHostAddress(char* address)
{
	bool success = m_newHostAddressValid;

	if (m_newHostAddressValid)
	{
		// Decode the address into an ip string
		if (m_newHostAddress.GetPtr<TLocalNetAddress>())
		{
			strcpy(address, "<local>:");
			strcat(address, SERVER_DEFAULT_PORT_STRING);
		}
		else
		{
			CNetwork::DecodeAddress(m_newHostAddress, address, false);
		}

		// Reset for next migration
		m_newHostAddressValid = false;
	}

	return success;
}

void CCryLANMatchMaking::HostMigrationSetNewServerAddress(TNetAddress& address)
{
	CryLobbyConnectionID connection;

	if (m_lobby->ConnectionFromAddress(&connection, address))
	{
		sockaddr_in sockAddr;

		if (ConvertAddr(address, &sockAddr))
		{
			TNetAddress newAddress = TNetAddress(SIPv4Addr(ntohl(sockAddr.sin_addr.s_addr), m_lobby->GetInternalSocketPort()));

			m_lobby->SetConnectionAddress(connection, newAddress);
		}
	}
}
#endif
