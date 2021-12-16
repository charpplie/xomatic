#include "StdAfx.h"
#include "CryMatchMaking.h"
#include "IGame.h"
#include "IGameFramework.h"

const uint32 CryMatchMakingNubConnectionPingTime = 250;
const uint32 CryMatchMakingNubConnectionTimeOut = 10000;

CCryMatchMaking::CCryMatchMaking(CCryLobby* lobby, CCryLobbyService* service)
{
	assert(lobby);
	assert(service);

	m_connectionUIDCounter = 0;
	m_lobby = lobby;
	m_pService = service;

	for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
	{
		m_sessions[i] = NULL;
	}

	for (uint32 i = 0; i < MAX_MATCHMAKING_TASKS; i++)
	{
		m_task[i] = NULL;
	}
}

#define GAME_SESSION_HANDLE_HANDLE_MASK		0xffff0000
#define GAME_SESSION_HANDLE_HANDLE_SHIFT	16
#define GAME_SESSION_HANDLE_UID_MASK			0x0000ffff
#define GAME_SESSION_HANDLE_UID_SHIFT			0
CrySessionHandle CCryMatchMaking::CreateGameSessionHandle(CrySessionHandle h, CryMatchMakingConnectionUID uid)
{
	return ((h<<GAME_SESSION_HANDLE_HANDLE_SHIFT)&GAME_SESSION_HANDLE_HANDLE_MASK) | ((uid<<GAME_SESSION_HANDLE_UID_SHIFT)&GAME_SESSION_HANDLE_UID_MASK);
}

CryMatchMakingConnectionUID CCryMatchMaking::CreateConnectionUID()
{
	do 
	{
		m_connectionUIDCounter = (m_connectionUIDCounter + 1)&(GAME_SESSION_HANDLE_UID_MASK>>GAME_SESSION_HANDLE_UID_SHIFT);
	} while ((m_connectionUIDCounter == 0) || (m_connectionUIDCounter == (GAME_SESSION_HANDLE_UID_MASK>>GAME_SESSION_HANDLE_UID_SHIFT)));

	return m_connectionUIDCounter;
}

CrySessionHandle CCryMatchMaking::GetSessionHandleFromGameSessionHandle(CrySessionHandle gh)
{
	return ((gh&GAME_SESSION_HANDLE_HANDLE_MASK) == (CrySessionInvalidHandle&GAME_SESSION_HANDLE_HANDLE_MASK))
		? CrySessionInvalidHandle
		: (gh&GAME_SESSION_HANDLE_HANDLE_MASK)>>GAME_SESSION_HANDLE_HANDLE_SHIFT;
}

CryMatchMakingConnectionUID CCryMatchMaking::GetConnectionUIDFromGameSessionHandle(CrySessionHandle gh)
{
	return (gh&GAME_SESSION_HANDLE_UID_MASK)>>GAME_SESSION_HANDLE_UID_SHIFT;
}

void CCryMatchMaking::Tick(CTimeValue tv)
{
}

bool CCryMatchMaking::GetAddressFromChannelID(TNetChannelID channelID, TNetAddress& address)
{
	bool succeeded = false;

	CrySessionHandle sessionHandle = CrySessionInvalidHandle;
	CryMatchMakingConnectionID connectionID = CryMatchMakingInvalidConnectionID;

	if (FindConnectionFromUID((CryMatchMakingConnectionUID)channelID, &sessionHandle, &connectionID))
	{
		succeeded = m_lobby->AddressFromConnection(address, m_sessions[sessionHandle]->remoteConnection[connectionID]->connectionID);
	}

	return succeeded;
}

bool CCryMatchMaking::GetChannelIDFromAddress(const TNetAddress& address, TNetChannelID* pChannelID)
{
	bool succeeded = false;

	if (address.GetPtr<TLocalNetAddress>())
	{
		// This is a local address, so get the local channel id
		*pChannelID = (TNetChannelID)GetLocalConnectionUID();
		succeeded = true;
	}
	else
	{
		CryLobbyConnectionID connectionID = m_lobby->FindConnection(address);

		if (connectionID != CryLobbyInvalidConnectionID)
		{
			succeeded = FindConnectionFromLobbyConnectionID(connectionID, (CryMatchMakingConnectionUID*)pChannelID);
		}
	}

	return succeeded;
}

CrySessionHandle CCryMatchMaking::GetCurrentSessionHandle(void) const
{
	// TODO: Possibly a better/more robust method of determining the session handle
	// This method assumes sessions are created and then destroyed in reverse order
	// which may not be true - Paul?
	CrySessionHandle handle = CrySessionInvalidHandle;

	for (uint32 index = 0; index < MAX_MATCHMAKING_SESSIONS; ++index)
	{
		if (m_sessions[index]->used)
		{
			handle = index;
		}
	}

	return handle;
}

CryMatchMakingConnectionUID CCryMatchMaking::GetLocalConnectionUID(void) const
{
	CryMatchMakingConnectionUID uid = CryMatchMakingInvalidConnectionUID;
	CrySessionHandle handle = GetCurrentSessionHandle();

	if (handle != CrySessionInvalidHandle)
	{
		uid = m_sessions[handle]->localConnection->uid;
	}

	return uid;
}

CryMatchMakingConnectionUID CCryMatchMaking::GetHostConnectionUID(void) const
{
	CryMatchMakingConnectionUID uid = CryMatchMakingInvalidConnectionUID;
	CrySessionHandle handle = GetCurrentSessionHandle();

	if (handle != CrySessionInvalidHandle)
	{
		if (m_sessions[handle]->host == true)
		{
			uid = m_sessions[handle]->localConnection->uid;
		}
		else
		{
			uid = m_sessions[handle]->remoteConnection[m_sessions[handle]->hostConnectionID]->uid;
		}
	}

	return uid;
}

ECryLobbyError CCryMatchMaking::StartTask(uint32 etask, bool startRunning, CryMatchMakingTaskID* mmTaskID, CryLobbyTaskID* lTaskID, CrySessionHandle h, void* cb, void* cbArg)
{
	CryLobbyTaskID lobbyTaskID = m_lobby->CreateTask();

	if (lobbyTaskID != CryLobbyInvalidTaskID)
	{
		for (uint32 i = 0; i < MAX_MATCHMAKING_TASKS; i++)
		{
			STask* task = m_task[i];

			assert(task);

			if (!task->used)
			{
				task->timer.SetValue(0);
				task->lTaskID = lobbyTaskID;
				task->error = eCLE_Success;
				task->startedTask = etask;
				task->subTask = etask;
				task->session = h;
				task->cb = cb;
				task->cbArg = cbArg;
				task->sendID = CryLobbyInvalidSendID;
				task->sendStatus = eCLE_Success;
				task->used = true;
				task->running = startRunning;
				task->timerStarted = false;
				task->canceled = false;

				for (uint32 j = 0; j < MAX_MATCHMAKING_PARAMS; j++)
				{
					task->params[j] = TMemInvalidHdl;
					task->numParams[j] = 0;
				}

				if (mmTaskID)
				{
					*mmTaskID = i;
				}

				if (lTaskID)
				{
					*lTaskID = lobbyTaskID;
				}

				return eCLE_Success;
			}
		}
	}

	return eCLE_TooManyTasks;
}

void	CCryMatchMaking::StartSubTask(uint32 etask, CryMatchMakingTaskID mmTaskID)
{
	STask* task = m_task[mmTaskID];
	task->subTask = etask;
}

void CCryMatchMaking::FreeTask(CryMatchMakingTaskID mmTaskID)
{
	STask* task = m_task[mmTaskID];

	assert(task);

	for (uint32 i = 0; i < MAX_MATCHMAKING_PARAMS; i++)
	{
		if (task->params[i] != TMemInvalidHdl)
		{
			m_lobby->MemFree(task->params[i]);
			task->params[i] = TMemInvalidHdl;
			task->numParams[i] = 0;
		}
	}

	m_lobby->ReleaseTask(task->lTaskID);

	task->used = false;
}

void CCryMatchMaking::CancelTask(CryLobbyTaskID lTaskID)
{
	LOBBY_AUTO_LOCK;

	NetLog("[Lobby]Try cancel task %d", lTaskID);

	if (lTaskID != CryLobbyInvalidTaskID)
	{
		for (uint32 i = 0; i < MAX_MATCHMAKING_TASKS; i++)
		{
			STask* task = m_task[i];

			assert(task);

			if (task->used && (task->lTaskID == lTaskID))
			{
				NetLog("[Lobby] Task %d canceled", lTaskID);
				task->cb = NULL;
				task->canceled = true;

				break;
			}
		}
	}
}

void CCryMatchMaking::SessionJoinFromConsole(void)
{
}

ECryLobbyError CCryMatchMaking::CreateTaskParam(CryMatchMakingTaskID mmTaskID, uint32 param, const void* paramData, uint32 numParams, size_t paramSize)
{
	STask* task = m_task[mmTaskID];

	assert(task);

	task->params[param] = m_lobby->MemAlloc(paramSize);
	void* p = m_lobby->MemGetPtr(task->params[param]);

	if (p)
	{
		if (paramData)
		{
			memcpy(p, paramData, paramSize);
		}

		task->numParams[param] = numParams;

		return eCLE_Success;
	}

	return eCLE_OutOfMemory;
}

ECryLobbyError CCryMatchMaking::CreateSessionHandle(CrySessionHandle* h, bool host, int numUsers)
{
	for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
	{
		SSession* session = m_sessions[i];

		assert(session);

		if (!session->used)
		{
			m_lobby->InternalSocketCreate(host);

			session->used = true;
			session->host = host;

			session->localConnection->uid = 0;
			session->localConnection->numUsers = numUsers;

			for (uint32 j = 0; j < MAX_LOBBY_CONNECTIONS; j++)
			{
				session->remoteConnection[j]->used = false;
			}

			*h = i;

			return eCLE_Success;
		}
	}

	return eCLE_TooManySessions;
}

void CCryMatchMaking::FreeSessionHandle(CrySessionHandle h)
{
	SSession* session = m_sessions[h];

	assert(session);

	if (session->used)
	{
		m_lobby->InternalSocketFree(session->host);
		session->used = false;
	}

	for (uint32 index = 0; index < MAX_MATCHMAKING_SESSIONS; ++index)
	{
		if (m_sessions[index]->used)
		{
			return;
		}
	}

#if NETWORK_REBROADCASTER
	// There are no sessions active so reset the master enable
	// switch on the Rebroadcaster so it can be used in the next
	// session if required
	CCryRebroadcaster* pRebroadcaster = m_lobby->GetRebroadcaster();
	if (pRebroadcaster)
	{
		pRebroadcaster->Reset();
	}
#endif
}

CryMatchMakingConnectionID CCryMatchMaking::AddRemoteConnection(CrySessionHandle h, CryLobbyConnectionID connectionID, CryMatchMakingConnectionUID uid, uint32 numUsers)
{
	SSession* session = m_sessions[h];

	assert(session);

	for (uint32 i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
	{
		SSession::SRConnection* connection = session->remoteConnection[i];

		assert(connection);

		if (!connection->used)
		{
			m_lobby->KeepConnection(connectionID);

			connection->used = true;
			connection->connectionID = connectionID;
			connection->uid = uid;
			connection->numUsers = numUsers;
			connection->StartTimer();

			if (!session->host)
			{
				// Keep all remote clients in sync with the host (in case they're chosen to be host during host migration)
				m_connectionUIDCounter = uid;
			}

			return i;
		}
	}

	return CryMatchMakingInvalidConnectionID;
}

void CCryMatchMaking::FreeRemoteConnection(CrySessionHandle h, CryMatchMakingConnectionID id)
{
	assert(m_sessions[h]);

	SSession::SRConnection* connection = m_sessions[h]->remoteConnection[id];

	assert(connection);

	CryLogAlways("[Lobby] Free connection %d uid %d", id, connection->uid);
	m_lobby->ReleaseConnection(connection->connectionID);
	connection->used = false;
}

bool CCryMatchMaking::FindLocalConnectionFromUID(CryMatchMakingConnectionUID uid, CrySessionHandle* h)
{
	for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
	{
		SSession* session = m_sessions[i];

		assert(session);

		if (session->used)
		{
			SSession::SLConnection* connection = session->localConnection;

			assert(connection);

			if (connection->uid == uid)
			{
				*h = i;
				return true;
			}
		}
	}

	return false;
}

bool CCryMatchMaking::FindConnectionFromUID(CryMatchMakingConnectionUID uid, CrySessionHandle* h, CryMatchMakingConnectionID* id)
{
	for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
	{
		SSession* session = m_sessions[i];

		assert(session);

		if (session->used)
		{
			for (uint32 j = 0; j < MAX_LOBBY_CONNECTIONS; j++)
			{
				SSession::SRConnection* connection = session->remoteConnection[j];

				assert(connection);

				if (connection->used)
				{
					if (connection->uid == uid)
					{
						*h = i;
						*id = j;
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool CCryMatchMaking::FindConnectionFromLobbyConnectionID(CryLobbyConnectionID lobbyConnectionID, CryMatchMakingConnectionUID* puid)
{
	for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
	{
		SSession* session = m_sessions[i];

		assert(session);

		if (session->used)
		{
			for (uint32 j = 0; j < MAX_LOBBY_CONNECTIONS; j++)
			{
				SSession::SRConnection* connection = session->remoteConnection[j];

				assert(connection);

				if (connection->used)
				{
					if (connection->connectionID == lobbyConnectionID)
					{
						*puid = connection->uid;
						return true;
					}
				}
			}
		}
	}

	return false;
}

CryMatchMakingTaskID CCryMatchMaking::FindTaskFromSendID(CryLobbySendID sendID)
{
	if (sendID != CryLobbyInvalidSendID)
	{
		for (uint32 i = 0; i < MAX_MATCHMAKING_TASKS; i++)
		{
			STask* task = m_task[i];

			assert(task);

			if (task->used && (task->sendID == sendID))
			{
				return i;
			}
		}
	}

	return CryMatchMakingInvalidTaskID;
}

CryMatchMakingTaskID CCryMatchMaking::FindTaskFromTaskConnectionID(uint32 etask, CryLobbyConnectionID id)
{
	if (id != CryLobbyInvalidConnectionID)
	{
		for (uint32 i = 0; i < MAX_MATCHMAKING_TASKS; i++)
		{
			STask* task = m_task[i];

			assert(task);

			if (task->used && (task->subTask == etask) && (id == CryLobbyGetConnectionIDFromSendID(task->sendID)))
			{
				return i;
			}
		}
	}

	return CryMatchMakingInvalidTaskID;
}

CryMatchMakingTaskID CCryMatchMaking::FindTaskFromTaskTaskID(uint32 etask, CryMatchMakingTaskID mmTaskID)
{
	if (mmTaskID != CryMatchMakingInvalidTaskID)
	{
		STask* task = m_task[mmTaskID];

		assert(task);

		if (task->used && (task->subTask == etask))
		{
			return mmTaskID;
		}
	}

	return CryMatchMakingInvalidTaskID;
}

void CCryMatchMaking::UpdateTaskError(CryMatchMakingTaskID mmTaskID, ECryLobbyError error)
{
	STask* task = m_task[mmTaskID];

	assert(task);

	if (task->error == eCLE_Success)
	{
		task->error = error;
	}
}

ECryLobbyError CCryMatchMaking::Initialise()
{
	for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
	{
		assert(m_sessions[i]);
		m_sessions[i]->used = false;
	}

	for (uint32 i = 0; i < MAX_MATCHMAKING_TASKS; i++)
	{
		assert(m_task[i]);
		m_task[i]->used = false;
	}

#if NETWORK_HOST_MIGRATION
	m_newHostAddressValid = false;
#endif

	return eCLE_Success;
}

ECryLobbyError CCryMatchMaking::Terminate()
{
	return eCLE_Success;
}

ESocketError CCryMatchMaking::Send(CryMatchMakingTaskID mmTaskID, const uint8* buffer, size_t length, const TNetAddress& to, bool reliable)
{
	CryLobbySendID id;
	ESocketError ret = m_lobby->Send(buffer, length, to, reliable, &id);

	if (ret == eSE_Ok)
	{
		if (mmTaskID != CryMatchMakingInvalidTaskID)
		{
			STask* task = m_task[mmTaskID];

			assert(task);

			task->sendID = id;
			task->sendStatus = eCLE_Pending;
		}
	}

	return ret;
}

ESocketError CCryMatchMaking::Send(CryMatchMakingTaskID mmTaskID, const uint8* buffer, size_t length, CrySessionHandle h, CryMatchMakingConnectionID connectionID, bool reliable)
{
	TNetAddress addr;

	assert(m_sessions[h]);
	assert(m_sessions[h]->remoteConnection[connectionID]);

	if (m_lobby->AddressFromConnection(addr, m_sessions[h]->remoteConnection[connectionID]->connectionID))
	{
		return Send(mmTaskID, buffer, length, addr, reliable);
	}

	return eSE_MiscFatalError;
}

void CCryMatchMaking::SendToAll(CryMatchMakingTaskID mmTaskID, const uint8* buffer, size_t length, CrySessionHandle h, CryMatchMakingConnectionID skipID, bool reliable)
{
	SSession* session = m_sessions[h];

	assert(session);

	for (uint32 i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
	{
		SSession::SRConnection* connection = session->remoteConnection[i];

		assert(connection);

		if (connection->used && (i != skipID))
		{
			Send(CryMatchMakingInvalidTaskID, buffer, length, h, i, reliable);
		}
	}
}

void CCryMatchMaking::SessionDisconnectRemoteConnectionViaNub(EDisconnectionCause cause, CrySessionHandle gh, const char* reason)
{
	CNetNub* nub = (CNetNub*)gEnv->pGame->GetIGameFramework()->GetServerNetNub();

	if (nub)
	{
		nub->DisconnectChannel(cause, gh, reason);
	}

	SessionDisconnectRemoteConnection(gh, TNetAddress());
}

void CCryMatchMaking::SessionDisconnectRemoteConnection(CrySessionHandle gh, const TNetAddress& addr)
{
	CryMatchMakingConnectionUID uid = CryMatchMakingInvalidConnectionUID;
	CrySessionHandle h;
	CryMatchMakingConnectionID id;
	const uint32 MaxBufferSize = PacketHeaderSize + PacketUINT32Size;
	uint8 buffer[MaxBufferSize];
	uint32 bufferSize = 0;

	if (gh == CrySessionInvalidHandle)
	{
		TNetChannelID channelID;

		if (GetChannelIDFromAddress(addr, &channelID))
		{
			uid = channelID;
		}
	}
	else
	{
		uid = GetConnectionUIDFromGameSessionHandle(gh);
	}

	if (uid != CryMatchMakingInvalidConnectionUID)
	{
		StartAddPacket(buffer, MaxBufferSize, &bufferSize, eLobbyPT_SessionDeleteRemoteConnection);
		AddPacketUINT32(buffer, MaxBufferSize, &bufferSize, uid);

		if (FindLocalConnectionFromUID(uid, &h))
		{
			SSession*	pSession = m_sessions[h];

			if (pSession->host)
			{
				// If this is our local connection and we are the host tell everyone we are leaving.
				SendToAll(CryMatchMakingInvalidTaskID, buffer, bufferSize, h, CryMatchMakingInvalidConnectionID, true);
			}
			else
			{
				// If this is our local connection and we are not the host tell the host we are leaving.
				Send(CryMatchMakingInvalidTaskID, buffer, bufferSize, h, pSession->hostConnectionID, true);
			}
		}

		if (FindConnectionFromUID(uid, &h, &id))
		{
			SSession*	pSession = m_sessions[h];

			if (pSession->host)
			{
				// If this is a remote connection and we are the host tell everyone the connection is leaving.
				SendToAll(CryMatchMakingInvalidTaskID, buffer, bufferSize, h, CryMatchMakingInvalidConnectionID, true);
			}

			FreeRemoteConnection(h, id);
		}
	}
}

void CCryMatchMaking::ProcessSessionDeleteRemoteConnection(const TNetAddress& addr, const uint8* data, uint32 length)
{
	uint32 bufferPos = 0;
	CryMatchMakingConnectionUID uid;
	CrySessionHandle h;
	CryMatchMakingConnectionID id;

	StartRemovePacket(data, length, &bufferPos);
	RemovePacketUINT32(data, length, &bufferPos, &uid);

	if (FindConnectionFromUID(uid, &h, &id))
	{
		SessionDisconnectRemoteConnection(uid, TNetAddress());
	}
}

void CCryMatchMaking::OnPacket(const TNetAddress& addr, const uint8* data, uint32 length)
{
	CryLobbyPacketType type;
	uint32 pos;

	StartRemovePacket(data, length, &pos, &type);

	switch (type)
	{
	case eLobbyPT_SessionDeleteRemoteConnection:
		ProcessSessionDeleteRemoteConnection(addr, data, length);
		break;
	}
}

void CCryMatchMaking::OnError(const TNetAddress& addr, ESocketError error, CryLobbySendID sendID)
{
	CryMatchMakingTaskID mmTaskID = FindTaskFromSendID(sendID);

	if (mmTaskID != CryMatchMakingInvalidTaskID)
	{
		assert(m_task[mmTaskID]);
		m_task[mmTaskID]->sendStatus = eCLE_InternalError;
	}
}

void CCryMatchMaking::OnSendComplete(const TNetAddress& addr, CryLobbySendID sendID)
{
	CryMatchMakingTaskID mmTaskID = FindTaskFromSendID(sendID);

	if (mmTaskID != CryMatchMakingInvalidTaskID)
	{
		assert(m_task[mmTaskID]);
		m_task[mmTaskID]->sendStatus = eCLE_Success;
	}
}

#if NETWORK_HOST_MIGRATION
void CCryMatchMaking::HostMigrationInitiate(void)
{
	// Default implementation is that there are no platform specific tasks to wait for host migration to finish
	m_hostMigrationFinished = true;
}

ECryLobbyError CCryMatchMaking::HostMigrationServer(void)
{
	return eCLE_Success;
}

bool CCryMatchMaking::IsHostMigrationFinished(void)
{
	return m_hostMigrationFinished;
}

void CCryMatchMaking::HostMigrationSetNewServerAddress(TNetAddress& address)
{
}
#endif
