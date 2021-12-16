
#include "StdAfx.h"
#include "CryLiveLobby.h"
#include "CryLiveMatchMaking.h"
#include "Endian.h"
#include "Protocol/NetChannel.h"
#include "Protocol/NetNub.h"

const uint32 CryLiveMatchMakingWaitJoinRemoteFromRequestJoinTimeOut = 10000;
const uint32 CryLiveMatchMakingRegistrationTimeOut = 10000;
const uint32 CryLiveMatchMakingWaitArbitrationFinishTimeOut = 10000;

CCryLiveMatchMaking::CCryLiveMatchMaking(CCryLobby* lobby, CCryLobbyService* service) : CCryMatchMaking(lobby, service)
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

ECryLobbyError CCryLiveMatchMaking::StartTask(ETask etask, bool startRunning, uint32 user, CryMatchMakingTaskID* mmTaskID, CryLobbyTaskID* lTaskID, CrySessionHandle h, void* cb, void* cbArg)
{
	CryMatchMakingTaskID tmpMMTaskID;
	CryMatchMakingTaskID* useMMTaskID = mmTaskID ? mmTaskID : &tmpMMTaskID;
	ECryLobbyError error = CCryMatchMaking::StartTask(etask, startRunning, useMMTaskID, lTaskID, h, cb, cbArg);

	if (error == eCLE_Success)
	{
		STask* task = &m_task[*useMMTaskID];

		ZeroMemory(&task->overlapped, sizeof(task->overlapped));
		task->returnTaskID = CryMatchMakingInvalidTaskID;
		task->user = user;
	}

	return error;
}

void CCryLiveMatchMaking::StartSubTask(ETask etask, CryMatchMakingTaskID mmTaskID)
{
	CCryMatchMaking::StartSubTask(etask, mmTaskID);
	
	STask* task = &m_task[mmTaskID];
	ZeroMemory(&task->overlapped, sizeof(task->overlapped));
}

void CCryLiveMatchMaking::StartTaskRunning(CryMatchMakingTaskID mmTaskID)
{
	LOBBY_AUTO_LOCK;

	STask* task = &m_task[mmTaskID];

	if (task->used)
	{
		task->running = true;

		switch (task->startedTask)
		{
		case eT_SessionRegisterUserData:
			StopTaskRunning(mmTaskID);
			break;

		case eT_SessionCreate:
			StartSessionCreate(mmTaskID);
			break;

		case eT_SessionMigrate:
			StartSessionMigrate(mmTaskID);
			break;

		case eT_SessionUpdate:
			StartSessionUpdate(mmTaskID);
			break;

		case eT_SessionStart:
			StartSessionStart(mmTaskID);
			break;

		case eT_SessionEnd:
			StartSessionEnd(mmTaskID);
			break;

		case eT_SessionDelete:
			StartSessionDelete(mmTaskID);
			break;

		case eT_SessionSearch:
			StartSessionSearch(mmTaskID);
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

void CCryLiveMatchMaking::EndTask(CryMatchMakingTaskID mmTaskID)
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
				((CryMatchmakingCallback)task->cb)(task->lTaskID, task->error, task->cbArg);
				break;

			case eT_SessionUpdate:
			case eT_SessionStart:
			case eT_SessionEnd:
			case eT_SessionDelete:
				((CryMatchmakingCallback)task->cb)(task->lTaskID, task->error, task->cbArg);
				break;

			case eT_SessionMigrate:																																// From the games point of view a migrate is the same as a create
			case eT_SessionCreate:
				((CryMatchmakingSessionCreateCallback)task->cb)(task->lTaskID, task->error, CreateGameSessionHandle(task->session, m_sessions[task->session].localConnection.uid), task->cbArg);
				break;

			case eT_SessionSearch:
				EndSessionSearch(mmTaskID);
				break;

			case eT_SessionJoin:
				EndSessionJoin(mmTaskID);
				break;
			}
		}

		CryLogAlways("[Lobby] EndTask %d (%d) error %d", task->startedTask, task->subTask, task->error);

		FreeTask(mmTaskID);
	}
}

void CCryLiveMatchMaking::StopTaskRunning(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];

	if (task->used)
	{
		task->running = false;
		TO_GAME(&CCryLiveMatchMaking::EndTask, this, mmTaskID);
	}
}

ECryLobbyError CCryLiveMatchMaking::TickTask(CryMatchMakingTaskID mmTaskID)
{
	if (XHasOverlappedIoCompleted(&m_task[mmTaskID].overlapped))
	{
		DWORD ret = XGetOverlappedExtendedError(&m_task[mmTaskID].overlapped);

		switch (ret)
		{
		case ERROR_IO_INCOMPLETE:
			return eCLE_Pending;

		default:
			return CryLiveLobbyGetErrorFromLive(ret); 
		}
	}

	return eCLE_Pending;
}

ECryLobbyError CCryLiveMatchMaking::CreateSessionHandle(CrySessionHandle* h, bool host, uint32* users, int numUsers)
{
	ECryLobbyError error = CCryMatchMaking::CreateSessionHandle(h, host, numUsers);

	if (error == eCLE_Success)
	{
		SSession* session = &m_sessions[*h];

		session->started = false;

		for (uint32 j = 0; j < numUsers; j++)
		{
			session->localConnection.users[j] = users[j];
		}

		session->remoteConnectionProcessingToDo = false;
		session->remoteConnectionTaskID = CryMatchMakingInvalidTaskID;
		session->hostConnectionID = CryMatchMakingInvalidConnectionID;
		session->unregisteredConnectionsKicked = false;
	}

	return error;
}

CryMatchMakingConnectionID CCryLiveMatchMaking::AddRemoteConnection(CrySessionHandle h, CryLobbyConnectionID connectionID, CryMatchMakingConnectionUID uid, XNADDR* xnaddr, uint16 port, uint32 numUsers, XUID* xuids, uint8* localUsers, BOOL* privateSlots)
{
	SSession* session = &m_sessions[h];

	if (connectionID == CryLobbyInvalidConnectionID)
	{
		in_addr addr;
		INT ret = XNetXnAddrToInAddr(xnaddr, &session->info.sessionID, &addr);

		if (ret == 0)
		{
			TNetAddress netAddr = TNetAddress(SIPv4Addr(addr.s_addr, port));

			connectionID = m_lobby->FindConnection(netAddr);

			if (connectionID == CryLobbyInvalidConnectionID)
			{
				connectionID = m_lobby->CreateConnection(netAddr);
			}
		}
	}

	CryMatchMakingConnectionID id = CCryMatchMaking::AddRemoteConnection(h, connectionID, uid, numUsers);

	if (id != CryMatchMakingInvalidConnectionID)
	{
		SSession::SRConnection* connection = &session->remoteConnection[id];

		connection->xnaddr = *xnaddr;
		connection->port = port;
		connection->state = SSession::SRConnection::eRCS_None;
		connection->registered = false;
		connection->m_migrated = false;

		for (uint32 i = 0; i < numUsers; i++)
		{
			connection->xuid[i] = xuids[i];
			connection->localUser[i] = localUsers[i];
			connection->localUser2Index[localUsers[i]] = i;
			connection->privateSlot[i] = privateSlots[i];
		}
	}

	assert(id != CryMatchMakingInvalidConnectionID);

	return id;
}

void CCryLiveMatchMaking::SetRemoteConnectionState(CrySessionHandle h, CryMatchMakingConnectionID id, SSession::SRConnection::ERemoteConnectionState state)
{
	SSession* session = &m_sessions[h];
	SSession::SRConnection* connection = &session->remoteConnection[id];

	connection->state = state;

	if ((state == SSession::SRConnection::eRCS_WaitingToJoin) ||
			(state == SSession::SRConnection::eRCS_WaitingToLeave) ||
			(state == SSession::SRConnection::eRCS_JoiningButWantToLeave))
	{
		session->remoteConnectionProcessingToDo = true;
	}
}

void CCryLiveMatchMaking::FreeRemoteConnection(CrySessionHandle h, CryMatchMakingConnectionID id)
{
	SSession* session = &m_sessions[h];
	SSession::SRConnection* connection = &session->remoteConnection[id];

	if ((connection->state == SSession::SRConnection::eRCS_None) || (connection->state == SSession::SRConnection::eRCS_WaitingToJoin))
	{
		// The remote players don't need removing from session so just free
		CCryMatchMaking::FreeRemoteConnection(h, id);
	}
	else
	if (connection->state == SSession::SRConnection::eRCS_Joining)
	{
		// The remote players are currently joining and will have to finish joining before they can be changed to eRCS_WaitingToLeave
		SetRemoteConnectionState(h, id, SSession::SRConnection::eRCS_JoiningButWantToLeave);
	}
	else
	if (connection->state == SSession::SRConnection::eRCS_Joined)
	{
		// The remote players have joined so change to eRCS_WaitingToLeave
		SetRemoteConnectionState(h, id, SSession::SRConnection::eRCS_WaitingToLeave);
	}
}

CrySessionHandle CCryLiveMatchMaking::FindSessionFromID(XNKID xnkid)
{
	for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
	{
		if (m_sessions[i].used)
		{
			if (memcmp(&m_sessions[i].info.sessionID, &xnkid, sizeof(xnkid)) == 0)
			{
				return i;
			}
		}
	}

	return CrySessionInvalidHandle;
}

ECryLobbyError CCryLiveMatchMaking::SetSessionUserData(CrySessionHandle h, SCrySessionUserData* data, uint32 numData)
{
	SSession* session = &m_sessions[h];

	if (session->used)
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
						SLiveUserData liveData;
						ECryLobbyError error = CryUserDataToLiveUserData(&data[i], &liveData);

						if (error == eCLE_Success)
						{
							DWORD user = session->localConnection.users[0];

							if (liveData.typeContext)
							{
								if (liveData.context.dwContextId == X_CONTEXT_GAME_MODE)
								{
									session->gameMode = liveData.context.dwValue;
								}

								XUserSetContext(user, liveData.context.dwContextId, liveData.context.dwValue);
							}
							else
							{
								switch (liveData.property.value.type)
								{
								case XUSER_DATA_TYPE_INT32:
									XUserSetProperty(user, liveData.property.dwPropertyId, sizeof(liveData.property.value.nData), &liveData.property.value.nData);
									break;

								case XUSER_DATA_TYPE_INT64:
									XUserSetProperty(user, liveData.property.dwPropertyId, sizeof(liveData.property.value.i64Data), &liveData.property.value.i64Data);
									break;

								case XUSER_DATA_TYPE_DOUBLE:
									XUserSetProperty(user, liveData.property.dwPropertyId, sizeof(liveData.property.value.dblData), &liveData.property.value.dblData);
									break;

								case XUSER_DATA_TYPE_FLOAT:
									XUserSetProperty(user, liveData.property.dwPropertyId, sizeof(liveData.property.value.fData), &liveData.property.value.fData);
									break;
								}
							}
						}
						else
						{
							return error;
						}

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

void CCryLiveMatchMaking::Tick(CTimeValue tv)
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
				case eT_SessionCreate:
					TickSessionCreate(i);
					break;
				
				case eT_SessionMigrate:
					TickSessionMigrate(i);
					break;

				case eT_SessionUpdate:
					break;

				case eT_SessionStart:
					TickSessionStart(i);
					break;

				case eT_SessionWaitArbitrationStart:
					TickSessionWaitArbitrationStart(i);
					break;

				case eT_SessionHostWaitArbitrationStart:
					TickSessionHostWaitArbitrationStart(i);
					break;

				case eT_SessionWaitArbitrationFinish:
					TickSessionWaitArbitrationFinish(i);
					break;

				case eT_SessionArbitration:
					TickSessionArbitration(i);
					break;

				case eT_SessionEnd:
					TickSessionEnd(i);
					break;

				case eT_SessionDeleteWaitStart:
					TickSessionDeleteWaitStart(i);
					break;

				case eT_SessionDelete:
					TickSessionDelete(i);
					break;

				case eT_SessionSearch:
					TickSessionSearch(i);
					break;

				case eT_SessionJoinWaitStart:
					TickSessionJoinWaitStart(i);
					break;

				case eT_SessionJoin:
					TickSessionJoin(i);
					break;

				case eT_SessionJoinLocal:
					TickSessionJoinLocal(i);
					break;

				case eT_SessionWaitJoinRemoteFromRequestJoin:
					TickSessionWaitJoinRemoteFromRequestJoin(i);
					break;

				case eT_SessionJoinRemoteFromRequestJoin:
					TickSessionJoinRemoteFromRequestJoin(i);
					break;

				case eT_SessionRequestJoin:
					TickSessionRequestJoin(i);
					break;

				case eT_SessionJoinModifySession:
					TickSessionJoinModifySession(i);
					break;

#if NETWORK_HOST_MIGRATION
				case eT_SessionMigrateHostServer:
					TickHostMigrationServerNT(i);
					break;

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

		for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
		{
			SSession* session = &m_sessions[i];

			if (session->used)
			{
				if (session->remoteConnectionTaskID != CryMatchMakingInvalidTaskID)
				{
					ECryLobbyError error = TickTask(session->remoteConnectionTaskID);

					if (error != eCLE_Pending)
					{
						if (error == eCLE_Success)
						{
							STask* task = &m_task[session->remoteConnectionTaskID];

							if (task->startedTask == eT_SessionLeaveRemote)
							{
								// Now all players have successfully left all connections that are eRCS_Leaving can be freed
								for (uint32 j = 0; j < MAX_LOBBY_CONNECTIONS; j++)
								{
									SSession::SRConnection* connection = &session->remoteConnection[j];

									if (connection->used)
									{
										if (connection->state == SSession::SRConnection::eRCS_Leaving)
										{
											InformVoiceRemoteUsersLeft(i, j);
											SetRemoteConnectionState(i, j, SSession::SRConnection::eRCS_None);
											FreeRemoteConnection(i, j);
										}
									}
								}
							}
							else
							{
								// Now all players have successfully joined all connections that are eRCS_Joining can be set to eRCS_Joined
								// and any that were changed to eRCS_JoiningButWantToLeave can be set to eRCS_WaitingToLeave
								for (uint32 j = 0; j < MAX_LOBBY_CONNECTIONS; j++)
								{
									SSession::SRConnection* connection = &session->remoteConnection[j];

									if (connection->used)
									{
										if (connection->state == SSession::SRConnection::eRCS_Joining)
										{
											InformVoiceRemoteUsersJoined(i, j);
											SetRemoteConnectionState(i, j, SSession::SRConnection::eRCS_Joined);
										}
										else
										if (connection->state == SSession::SRConnection::eRCS_JoiningButWantToLeave)
										{
											SetRemoteConnectionState(i, j, SSession::SRConnection::eRCS_WaitingToLeave);
										}
									}
								}
							}

							// Is there any more processing to do if not don't run the remoteConnectionProcessingToDo section again until there is
							session->remoteConnectionProcessingToDo = false;

							for (uint32 j = 0; j < MAX_LOBBY_CONNECTIONS; j++)
							{
								SSession::SRConnection* connection = &session->remoteConnection[j];

								if (connection->used)
								{
									if ((connection->state == SSession::SRConnection::eRCS_WaitingToJoin) ||
											(connection->state == SSession::SRConnection::eRCS_WaitingToLeave) ||
											(connection->state == SSession::SRConnection::eRCS_JoiningButWantToLeave))
									{
										session->remoteConnectionProcessingToDo = true;
										break;
									}
								}
							}
						}

						CryLogAlways("[Lobby] %s done error %d", (m_task[session->remoteConnectionTaskID].startedTask == eT_SessionLeaveRemote) ? "XSessionLeaveRemote" : "XSessionJoinRemote", error);

						FreeTask(session->remoteConnectionTaskID);
						session->remoteConnectionTaskID = CryMatchMakingInvalidTaskID;
					}
				}
				else
				if (session->remoteConnectionProcessingToDo)
				{
					XUID xuid[MAX_LOBBY_CONNECTIONS*MAX_LIVE_LOCAL_USERS];
					BOOL privateSlot[MAX_LOBBY_CONNECTIONS*MAX_LIVE_LOCAL_USERS];
					uint32 num = 0;

					// Create XSessionLeaveRemote data
					for (uint32 j = 0; j < MAX_LOBBY_CONNECTIONS; j++)
					{
						SSession::SRConnection* connection = &session->remoteConnection[j];

						if (connection->used)
						{
							if (connection->state == SSession::SRConnection::eRCS_WaitingToLeave)
							{
								for (uint32 k = 0; k < connection->numUsers; k++)
								{
									xuid[num] = connection->xuid[k];
									num++;
									CryLogAlways("[Lobby] User %llx private %d is going to leave", connection->xuid[k], connection->privateSlot[k]);
								}
							}
						}
					}

					if (num > 0)
					{
						ECryLobbyError error = StartTask(eT_SessionLeaveRemote, true, 0, &session->remoteConnectionTaskID, NULL, i, NULL, NULL);

						if (error == eCLE_Success)
						{
							error = CreateTaskParam(session->remoteConnectionTaskID, 0, xuid, num, num*sizeof(XUID));

							if (error == eCLE_Success)
							{
								STask* task = &m_task[session->remoteConnectionTaskID];

								error = CryLiveLobbyGetErrorFromLive(XSessionLeaveRemote(session->handle, num, (XUID*)m_lobby->MemGetPtr(task->params[0]), &task->overlapped));

								CryLogAlways("[Lobby] XSessionLeaveRemote started error %d", error);

								if (error == eCLE_Success)
								{
									// Now leaving has been successfully started change state
									for (uint32 j = 0; j < MAX_LOBBY_CONNECTIONS; j++)
									{
										SSession::SRConnection* connection = &session->remoteConnection[j];

										if (connection->used)
										{
											if (connection->state == SSession::SRConnection::eRCS_WaitingToLeave)
											{
												SetRemoteConnectionState(i, j, SSession::SRConnection::eRCS_Leaving);
											}
										}
									}
								}
							}

							if (error != eCLE_Success)
							{
								FreeTask(session->remoteConnectionTaskID);
								session->remoteConnectionTaskID = CryMatchMakingInvalidTaskID;
							}
						}
					}
					else
					{
						// There wasn't any XSessionLeaveRemote data so create XSessionJoinRemote data

						for (uint32 j = 0; j < MAX_LOBBY_CONNECTIONS; j++)
						{
							SSession::SRConnection* connection = &session->remoteConnection[j];

							if (connection->used)
							{
								if (connection->state == SSession::SRConnection::eRCS_WaitingToJoin)
								{
									for (uint32 k = 0; k < connection->numUsers; k++)
									{
										xuid[num] = connection->xuid[k];
										privateSlot[num] = connection->privateSlot[k];
										CryLogAlways("[Lobby] User %llx private %d is going to join", connection->xuid[k], connection->privateSlot[k]);
										num++;
									}
								}
							}
						}

						if (num > 0)
						{
							ECryLobbyError error = StartTask(eT_SessionJoinRemote, true, 0, &session->remoteConnectionTaskID, NULL, i, NULL, NULL);

							if (error == eCLE_Success)
							{
								error = CreateTaskParam(session->remoteConnectionTaskID, 0, xuid, num, num*sizeof(XUID));

								if (error == eCLE_Success)
								{
									error = CreateTaskParam(session->remoteConnectionTaskID, 1, privateSlot, num, num*sizeof(BOOL));

									if (error == eCLE_Success)
									{
										STask* task = &m_task[session->remoteConnectionTaskID];

										error = CryLiveLobbyGetErrorFromLive(XSessionJoinRemote(session->handle, num, (XUID*)m_lobby->MemGetPtr(task->params[0]), (BOOL*)m_lobby->MemGetPtr(task->params[1]), &task->overlapped));

										CryLogAlways("[Lobby] XSessionJoinRemote started error %d", error);

										if (error == eCLE_Success)
										{
											// Now joining has been successfully started change state
											for (uint32 j = 0; j < MAX_LOBBY_CONNECTIONS; j++)
											{
												SSession::SRConnection* connection = &session->remoteConnection[j];

												if (connection->used)
												{
													if (connection->state == SSession::SRConnection::eRCS_WaitingToJoin)
													{
														SetRemoteConnectionState(i, j, SSession::SRConnection::eRCS_Joining);
													}
												}
											}
										}
									}
								}

								if (error != eCLE_Success)
								{
									FreeTask(session->remoteConnectionTaskID);
									session->remoteConnectionTaskID = CryMatchMakingInvalidTaskID;
								}
							}
						}
					}
				}
			}
		}

		m_lobby->MutexUnlock();
	}
}

ECryLobbyError CCryLiveMatchMaking::Initialise()
{
	ECryLobbyError error = CCryMatchMaking::Initialise();

	if (error == eCLE_Success)
	{
		m_registeredUserData.num = 0;
	}

	return error;
}

ECryLobbyError CCryLiveMatchMaking::SessionRegisterUserData(SCrySessionUserData* data, uint32 numData, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	if (numData < MAX_MATCHMAKING_SESSION_USER_DATA)
	{
		CryMatchMakingTaskID mmTaskID;

		error = StartTask(eT_SessionRegisterUserData, false, 0, &mmTaskID, taskID, CrySessionInvalidHandle, cb, cbArg);

		if (error == eCLE_Success)
		{
			memcpy(m_registeredUserData.data, data, numData*sizeof(data[0]));
			m_registeredUserData.num = numData;

			FROM_GAME(&CCryLiveMatchMaking::StartTaskRunning, this, mmTaskID);
		}
	}
	else
	{
		error = eCLE_OutOfSessionUserData;
	}

	CryLogAlways("[Lobby] Start SessionRegisterUserData error %d", error);

	return error;
}

ECryLobbyError CCryLiveMatchMaking::SessionMigrate(CrySessionHandle h,uint32* pUsers, int numUsers, uint32 flags, SCrySessionData* pData, CryLobbyTaskID* pTaskID, CryMatchmakingSessionCreateCallback pCB, void* pCBArg)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	// Because we simply want to re-use the session that is already available, we don't need to do much here

	h = GetSessionHandleFromGameSessionHandle(h);

	if ((h < MAX_MATCHMAKING_SESSIONS) && m_sessions[h].used)
	{
		SSession* session = &m_sessions[h];
		CryMatchMakingTaskID mmTaskID;

		error = StartTask(eT_SessionMigrate, false, session->localConnection.users[0], &mmTaskID, pTaskID, h, (void*)pCB, pCBArg);

		if (error == eCLE_Success)
		{
			FROM_GAME(&CCryLiveMatchMaking::StartTaskRunning, this, mmTaskID);
		}
	}
	else
	{
		error = eCLE_InvalidSession;
	}

	CryLogAlways("[Lobby] Start SessionMigrate error %d", error);

	return error;
}

void CCryLiveMatchMaking::StartSessionMigrate(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];

	if (task->error != eCLE_Success)
	{
		StopTaskRunning(mmTaskID);
	}
}

void CCryLiveMatchMaking::TickSessionMigrate(CryMatchMakingTaskID mmTaskID)
{
	UpdateTaskError(mmTaskID, eCLE_Success);
	StopTaskRunning(mmTaskID);
}

ECryLobbyError CCryLiveMatchMaking::SessionCreate(uint32* users, int numUsers, uint32 flags, SCrySessionData* data, CryLobbyTaskID* taskID, CryMatchmakingSessionCreateCallback cb, void* cbArg)
{
	LOBBY_AUTO_LOCK;

	CrySessionHandle h;
	ECryLobbyError error = CreateSessionHandle(&h, true, users, numUsers);

	if (error == eCLE_Success)
	{
		CryMatchMakingTaskID mmTaskID;

		error = StartTask(eT_SessionCreate, false, users[0], &mmTaskID, taskID, h, cb, cbArg);

		if (error == eCLE_Success)
		{
			STask* task = &m_task[mmTaskID];
			SSession* session = &m_sessions[h];

			session->flags = XSESSION_CREATE_USES_PEER_NETWORK | XSESSION_CREATE_USES_STATS |  XSESSION_CREATE_USES_PRESENCE | XSESSION_CREATE_HOST;

			if (flags&CRYSESSION_CREATE_FLAG_SEARCHABLE)
			{
				session->flags |= XSESSION_CREATE_USES_MATCHMAKING;
			}

			if (data->m_ranked)
			{
				session->flags |= XSESSION_CREATE_USES_ARBITRATION;
				session->gameType = X_CONTEXT_GAME_TYPE_RANKED;
			}
			else
			{
				session->gameType = X_CONTEXT_GAME_TYPE_STANDARD;
			}

			session->gameMode = 0;
			session->numPublicSlots = data->m_numPublicSlots;
			session->numPrivateSlots = data->m_numPrivateSlots;
			session->localConnection.uid = CreateConnectionUID();

			CryLogAlways("[Lobby] Created local connection uid %d", session->localConnection.uid);

			for (uint32 i = 0; i < numUsers; i++)
			{
				session->localConnection.privateSlot[i] = TRUE;
			}

			error = CreateTaskParam(mmTaskID, 0, data, 1, sizeof(SCrySessionData));

			if (error == eCLE_Success)
			{
				error = CreateTaskParam(mmTaskID, 1, data->m_data, data->m_numData, data->m_numData*sizeof(data->m_data[0]));

				if (error == eCLE_Success)
				{
					FROM_GAME(&CCryLiveMatchMaking::StartTaskRunning, this, mmTaskID);
				}
			}

			if (error != eCLE_Success)
			{
				FreeTask(mmTaskID);
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

void CCryLiveMatchMaking::StartSessionCreate(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];
	SCrySessionData* data = (SCrySessionData*)m_lobby->MemGetPtr(task->params[0]);

	data->m_data = (SCrySessionUserData*)m_lobby->MemGetPtr(task->params[1]);

	XUserSetContext(task->user, X_CONTEXT_GAME_TYPE, session->gameType);
	XUserSetContext(task->user, X_CONTEXT_GAME_MODE, session->gameMode);

	UpdateTaskError(mmTaskID, SetSessionUserData(task->session, data->m_data, data->m_numData));

	if (task->error == eCLE_Success)
	{
		UpdateTaskError(mmTaskID, CryLiveLobbyGetErrorFromLive(XSessionCreate(session->flags, session->localConnection.users[0], data->m_numPublicSlots, data->m_numPrivateSlots, &session->nonce, &session->info, &task->overlapped, &session->handle)));
	}

	if (task->error != eCLE_Success)
	{
		FreeSessionHandle(task->session);
		StopTaskRunning(mmTaskID);
	}
}

void CCryLiveMatchMaking::TickSessionCreate(CryMatchMakingTaskID mmTaskID)
{
	ECryLobbyError error = TickTask(mmTaskID);

	if (error != eCLE_Pending)
	{
		STask* task = &m_task[mmTaskID];

		UpdateTaskError(mmTaskID, error);

		if (task->error == eCLE_Success)
		{
			SessionJoinLocal(mmTaskID);
		}
		else
		{
			FreeSessionHandle(task->session);
			StopTaskRunning(mmTaskID);
		}
	}
}

void CCryLiveMatchMaking::SessionJoinLocal(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];
	SSession::SLConnection* connection = &session->localConnection;

	StartSubTask(eT_SessionJoinLocal, mmTaskID);
	UpdateTaskError(mmTaskID, CryLiveLobbyGetErrorFromLive(XSessionJoinLocal(session->handle, connection->numUsers, connection->users, connection->privateSlot, &task->overlapped)));

	CryLogAlways("[Lobby] Start XSessionJoinLocal error %d", task->error);

	if (task->error != eCLE_Success)
	{
		StartSessionDelete(mmTaskID);
	}
}

void CCryLiveMatchMaking::TickSessionJoinLocal(CryMatchMakingTaskID mmTaskID)
{
	ECryLobbyError error = TickTask(mmTaskID);

	if (error != eCLE_Pending)
	{
		STask* task = &m_task[mmTaskID];

		CryLogAlways("[Lobby] XSessionJoinLocal done error %d", error);

		UpdateTaskError(mmTaskID, error);

		if (task->canceled)
		{
			StartSessionDelete(mmTaskID);
			return;
		}

		if (error == eCLE_Success)
		{
			InformVoiceLocalUsersChanged();
			StopTaskRunning(mmTaskID);
		}
		else
		{
			StartSessionDelete(mmTaskID);
		}
	}
}

ECryLobbyError CCryLiveMatchMaking::SessionUpdate(CrySessionHandle h, SCrySessionUserData* data, uint32 numData, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	h = GetSessionHandleFromGameSessionHandle(h);

	if ((h < MAX_MATCHMAKING_SESSIONS) && m_sessions[h].used)
	{
		SSession* session = &m_sessions[h];

		if (session->host)
		{
			CryMatchMakingTaskID mmTaskID;

			error = StartTask(eT_SessionUpdate, false, session->localConnection.users[0], &mmTaskID, taskID, h, cb, cbArg);

			if (error == eCLE_Success)
			{
				STask* task = &m_task[mmTaskID];

				error = CreateTaskParam(mmTaskID, 0, data, numData, numData*sizeof(data[0]));

				if (error == eCLE_Success)
				{
					FROM_GAME(&CCryLiveMatchMaking::StartTaskRunning, this, mmTaskID);
				}
				else
				{
					FreeTask(mmTaskID);
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

void CCryLiveMatchMaking::StartSessionUpdate(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SCrySessionUserData* data = (SCrySessionUserData*)m_lobby->MemGetPtr(task->params[0]);

	UpdateTaskError(mmTaskID, SetSessionUserData(task->session, data, task->numParams[0]));
	StopTaskRunning(mmTaskID);
}

ECryLobbyError CCryLiveMatchMaking::SessionStart(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	h = GetSessionHandleFromGameSessionHandle(h);

	if ((h < MAX_MATCHMAKING_SESSIONS) && m_sessions[h].used)
	{
		SSession* session = &m_sessions[h];
		CryMatchMakingTaskID mmTaskID;

		error = StartTask(eT_SessionStart, false, session->localConnection.users[0], &mmTaskID, taskID, h, cb, cbArg);

		if (error == eCLE_Success)
		{
			session->started = true;
			FROM_GAME(&CCryLiveMatchMaking::StartTaskRunning, this, mmTaskID);
		}
	}
	else
	{
		error = eCLE_InvalidSession;
	}

	CryLogAlways("[Lobby] Start SessionStart error %d", error);

	return error;
}

void CCryLiveMatchMaking::StartSessionStart(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];

	if (session->flags&XSESSION_CREATE_HOST)
	{
		// If we are host send packet to clients to say start session and then start our session

		const uint32 MaxBufferSize = PacketHeaderSize + PacketXNKIDSize + PacketUINT32Size;
		uint8 buffer[MaxBufferSize];
		uint32 bufferSize = 0;

		StartAddPacket(buffer, MaxBufferSize, &bufferSize, eLivePT_SessionStart);
		AddPacketXNKID(buffer, MaxBufferSize, &bufferSize, session->info.sessionID);
		AddPacketUINT32(buffer, MaxBufferSize, &bufferSize, mmTaskID);

		CryLogAlways("[Lobby] Send session start to peers");
		SendToAll(mmTaskID, buffer, bufferSize, task->session, CryMatchMakingInvalidConnectionID, true);
		StartSessionStartMain(mmTaskID, task->session, CryMatchMakingInvalidTaskID);
	}
	else
	{
		// If not host our session gets started after receiving a packet from the host so just end this task now
		StopTaskRunning(mmTaskID);
	}
}

void CCryLiveMatchMaking::StartSessionStartAfterArbitration(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];

	StartSubTask(eT_SessionStart, mmTaskID);
	UpdateTaskError(mmTaskID, CryLiveLobbyGetErrorFromLive(XSessionStart(session->handle, 0, &task->overlapped)));

	CryLogAlways("[Lobby] Start Call XSessionStart error %d", task->error);

	if (task->error != eCLE_Success)
	{
		session->started = false;
		StopTaskRunning(mmTaskID);
	}
}

void CCryLiveMatchMaking::StartSessionStartMain(CryMatchMakingTaskID mmTaskID, CrySessionHandle h, CryMatchMakingTaskID hostTaskID)
{
	ECryLobbyError error = eCLE_Success;
	SSession* session = &m_sessions[h];

	if (mmTaskID == CryMatchMakingInvalidTaskID)
	{
		error = StartTask(eT_SessionStart, true, session->localConnection.users[0], &mmTaskID, NULL, h, NULL, NULL);
	}

	if (error == eCLE_Success)
	{
		STask* task = &m_task[mmTaskID];

		if (session->flags&XSESSION_CREATE_USES_ARBITRATION)
		{
			// Have to do arbitration before XSessionStart can be done
			if (session->host)
			{
				// Host has to wait for other peers to do arbitration first
				StartSubTask(eT_SessionHostWaitArbitrationStart, mmTaskID);
				session->unregisteredConnectionsKicked = false;

				for (uint32 j = 0; j < MAX_LOBBY_CONNECTIONS; j++)
				{
					session->remoteConnection[j].registered = false;
				}
			}
			else
			{
				StartSubTask(eT_SessionWaitArbitrationStart, mmTaskID);
				task->returnTaskID = hostTaskID;
			}
		}
		else
		{
			// No arbitration just call XSessionStart
			StartSessionStartAfterArbitration(mmTaskID);
		}
	}
}

void CCryLiveMatchMaking::TickSessionStart(CryMatchMakingTaskID mmTaskID)
{
	ECryLobbyError error = TickTask(mmTaskID);

	if (error != eCLE_Pending)
	{
		UpdateTaskError(mmTaskID, error);

		if (m_task[mmTaskID].error != eCLE_Success)
		{
			m_sessions[m_task[mmTaskID].session].started = false;
		}

		StopTaskRunning(mmTaskID);
		CryLogAlways("[Lobby] XSessionStart Done error %d", m_task[mmTaskID].error);
	}
}

void CCryLiveMatchMaking::ProcessSessionStart(const TNetAddress& addr, const uint8* data, uint32 length)
{
	uint32 bufferPos = 0;
	XNKID xnkid;
	CrySessionHandle h;
	CryMatchMakingTaskID hostTaskID;

	StartRemovePacket(data, length, &bufferPos);
	RemovePacketXNKID(data, length, &bufferPos, &xnkid);
	RemovePacketUINT32(data, length, &bufferPos, &hostTaskID);

	h = FindSessionFromID(xnkid);

	if (h != CrySessionInvalidHandle)
	{
		StartSessionStartMain(CryMatchMakingInvalidTaskID, h, hostTaskID);
	}
}

void CCryLiveMatchMaking::TickSessionWaitArbitrationStart(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];
	bool allJoined = true;

	// Don't start arbitration until all remote players have joined the game
	for (uint32 i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
	{
		SSession::SRConnection* connection = &session->remoteConnection[i];

		if (connection->used)
		{
			if (connection->state != SSession::SRConnection::eRCS_Joined)
			{
				allJoined = false;
				break;
			}
		}
	}

	if (allJoined)
	{
		// All remote players have joined the session so it is safe to start arbitration
		CryLogAlways("[Lobby] All remote players have joined the session so start arbitration");
		StartSessionArbitration(mmTaskID);
	}
}

void CCryLiveMatchMaking::TickSessionHostWaitArbitrationStart(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];
	bool allRegistered = true;

	if (!session->unregisteredConnectionsKicked)
	{
		if (task->TimerStarted())
		{
			if (task->GetTimer() > CryLiveMatchMakingRegistrationTimeOut)
			{
				CryLogAlways("[Lobby] Registration time out kick unregistered connections");

				session->unregisteredConnectionsKicked = true;

				for (uint32 i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
				{
					SSession::SRConnection* connection = &session->remoteConnection[i];

					if (connection->used && !connection->registered)
					{
						CryLogAlways("[Lobby] kick unregistered connection %d uid %d", i, connection->uid);
						SessionDisconnectRemoteConnectionViaNub(eDC_ArbitrationFailed, connection->uid, "Arbitration Failed");
					}
				}
			}
		}
		else
		{
			task->StartTimer();
			CryLogAlways("[Lobby] Starting registration timer");
		}
	}

	// Have all connection registered
	for (uint32 i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
	{
		SSession::SRConnection* connection = &session->remoteConnection[i];

		if (connection->used)
		{
			if (!connection->registered)
			{
				allRegistered = false;
				break;
			}
		}
	}

	if (allRegistered)
	{
		// All connections have registered so now we can register ourselves
		StartSubTask(eT_SessionWaitArbitrationStart, mmTaskID);
	}
}

void CCryLiveMatchMaking::StartSessionArbitration(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];
	DWORD ret; 
	DWORD buffersize = 0;

	StartSubTask(eT_SessionArbitration, mmTaskID);

	ret = XSessionArbitrationRegister(session->handle, 0, session->nonce, &buffersize, NULL, NULL);

	if ((ERROR_INSUFFICIENT_BUFFER == ret) && (buffersize > 0))
	{
		UpdateTaskError(mmTaskID, CreateTaskParam(mmTaskID, 0, NULL, 0, buffersize));

		if (task->error == eCLE_Success)
		{
				ret = XSessionArbitrationRegister(session->handle, 0, session->nonce, &buffersize, (PXSESSION_REGISTRATION_RESULTS)m_lobby->MemGetPtr(task->params[0]), &task->overlapped);
				UpdateTaskError(mmTaskID, CryLiveLobbyGetErrorFromLive(ret));
		}
	}
	else
	{
		UpdateTaskError(mmTaskID, CryLiveLobbyGetErrorFromLive(ret));
	}

	if (task->error != eCLE_Success)
	{
		StopTaskRunning(mmTaskID);
	}

	CryLogAlways("[Lobby] XSessionArbitrationRegister Start error %d", task->error);
}

void CCryLiveMatchMaking::TickSessionArbitration(CryMatchMakingTaskID mmTaskID)
{
	ECryLobbyError error = TickTask(mmTaskID);

	if (error != eCLE_Pending)
	{
		STask* task = &m_task[mmTaskID];

		UpdateTaskError(mmTaskID, error);

		if (task->error == eCLE_Success)
		{
			SSession* session = &m_sessions[task->session];

			if (session->host)
			{
				// Registration has finished so now session can be started
				// Tell everyone arbitration has finished and the session can be started
				const uint32 MaxBufferSize = PacketHeaderSize;
				uint8 buffer[MaxBufferSize];
				uint32 bufferSize = 0;

				StartAddPacket(buffer, MaxBufferSize, &bufferSize, eLivePT_SessionRegistrationFinished);
				
				SendToAll(mmTaskID, buffer, bufferSize, task->session, CryMatchMakingInvalidConnectionID, true);

				// Start our session
				StartSessionStartAfterArbitration(mmTaskID);
			}
			else
			{
				if (session->hostConnectionID != CryMatchMakingInvalidConnectionID)
				{
					// Tell the host we have registered
					const uint32 MaxBufferSize = PacketHeaderSize + PacketUINT32Size;
					uint8 buffer[MaxBufferSize];
					uint32 bufferSize = 0;

					StartSubTask(eT_SessionWaitArbitrationFinish, mmTaskID);
					StartAddPacket(buffer, MaxBufferSize, &bufferSize, eLivePT_SessionIHaveRegistered);
					AddPacketUINT32(buffer, MaxBufferSize, &bufferSize, task->returnTaskID);
					Send(mmTaskID, buffer, bufferSize, task->session, session->hostConnectionID, true);
				}
				else
				{
					UpdateTaskError(mmTaskID, eCLE_InternalError);
				}
			}
		}

		if (task->error != eCLE_Success)
		{
			StopTaskRunning(mmTaskID);
		}

		CryLogAlways("[Lobby] XSessionArbitrationRegister Done error %d", task->error);
	}
}

void CCryLiveMatchMaking::ProcessSessionIHaveRegistered(const TNetAddress& addr, const uint8* data, uint32 length)
{
	CryLobbyConnectionID c;

	if (m_lobby->ConnectionFromAddress(&c, addr))
	{
		uint32 bufferPos = 0;
		CryMatchMakingTaskID mmTaskID;

		StartRemovePacket(data, length, &bufferPos);
		RemovePacketUINT32(data, length, &bufferPos, &mmTaskID);

		mmTaskID = FindTaskFromTaskTaskID(eT_SessionHostWaitArbitrationStart, mmTaskID);

		if (mmTaskID != CryMatchMakingInvalidTaskID)
		{
			STask* task = &m_task[mmTaskID];
			SSession* session = &m_sessions[task->session];

			for (uint32 i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
			{
				SSession::SRConnection* connection = &session->remoteConnection[i];

				if (connection->used && (connection->connectionID == c))
				{
					connection->registered = true;
					CryLogAlways("[Lobby] Connection %d uid %d has registered", i, connection->uid);
					break;
				}
			}
		}
	}
}

void CCryLiveMatchMaking::TickSessionWaitArbitrationFinish(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];

	if (task->sendStatus != eCLE_Pending)
	{
		if (task->sendStatus == eCLE_Success)
		{
			// eLivePT_SessionIHaveRegistered has been sent so wait for result  
			if (task->TimerStarted())
			{
				if (task->GetTimer() > CryLiveMatchMakingWaitArbitrationFinishTimeOut)
				{
					// No response so fail session start
					CryLogAlways("[Lobby] SessionWaitArbitrationFinish Time out waiting for arbitration finish");
					UpdateTaskError(mmTaskID, eCLE_TimeOut);
				}
			}
			else
			{
				task->StartTimer();
				CryLogAlways("[Lobby] SessionIHaveRegistered packet sent waiting arbitration finish");
			}
		}
		else
		{
			UpdateTaskError(mmTaskID, task->sendStatus);
			CryLogAlways("[Lobby] SessionWaitArbitrationFinish error sending SessionIHaveRegistered packet error %d", task->error);
		}

		if (task->error != eCLE_Success)
		{
			StopTaskRunning(mmTaskID);
		}
	}
}

void CCryLiveMatchMaking::ProcessSessionRegistrationFinished(const TNetAddress& addr, const uint8* data, uint32 length)
{
	CryLobbyConnectionID c;

	if (m_lobby->ConnectionFromAddress(&c, addr))
	{
		CryMatchMakingTaskID mmTaskID = FindTaskFromTaskConnectionID(eT_SessionWaitArbitrationFinish, c);

		if (mmTaskID != CryMatchMakingInvalidTaskID)
		{
			// Registration is finished so we can start our session
			StartSessionStartAfterArbitration(mmTaskID);
		}
	}
}

ECryLobbyError CCryLiveMatchMaking::SessionEnd(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	h = GetSessionHandleFromGameSessionHandle(h);

	if ((h < MAX_MATCHMAKING_SESSIONS) && m_sessions[h].used)
	{
		SSession* session = &m_sessions[h];
		CryMatchMakingTaskID mmTaskID;

		error = StartTask(eT_SessionEnd, false, session->localConnection.users[0], &mmTaskID, taskID, h, cb, cbArg);

		if (error == eCLE_Success)
		{
			session->started = false;
			FROM_GAME(&CCryLiveMatchMaking::StartTaskRunning, this, mmTaskID);
		}
	}
	else
	{
		error = eCLE_InvalidSession;
	}

	CryLogAlways("[Lobby] Start SessionEnd error %d", error);

	return error;
}

void CCryLiveMatchMaking::StartSessionEnd(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];

	if (session->flags&XSESSION_CREATE_HOST)
	{
		// If we are host send packet to clients to say end session and then end our session
		const uint32 MaxBufferSize = PacketHeaderSize + PacketXNKIDSize;
		uint8 buffer[MaxBufferSize];
		uint32 bufferSize = 0;

		StartAddPacket(buffer, MaxBufferSize, &bufferSize, eLivePT_SessionEnd);
		AddPacketXNKID(buffer, MaxBufferSize, &bufferSize, session->info.sessionID);

		CryLogAlways("[Lobby] Send session end to peers");
		SendToAll(CryMatchMakingInvalidTaskID, buffer, bufferSize, task->session, CryMatchMakingInvalidConnectionID, true);
		StartSessionEndMain(mmTaskID, task->session);
	}
	else
	{
		// If not host our session gets ended after receiving a packet from the host so just end this task now
		StopTaskRunning(mmTaskID);
	}
}

void CCryLiveMatchMaking::StartSessionEndMain(CryMatchMakingTaskID mmTaskID, CrySessionHandle h)
{
	ECryLobbyError error = eCLE_Success;
	SSession* session = &m_sessions[h];

	if (mmTaskID == CryMatchMakingInvalidTaskID)
	{
		error = StartTask(eT_SessionEnd, true, session->localConnection.users[0], &mmTaskID, NULL, h, NULL, NULL);
	}

	if (error == eCLE_Success)
	{
		STask* task = &m_task[mmTaskID];

		StartSubTask(eT_SessionEnd, mmTaskID);
		UpdateTaskError(mmTaskID, CryLiveLobbyGetErrorFromLive(XSessionEnd(session->handle, &task->overlapped)));

		CryLogAlways("[Lobby] Start Call XSessionEnd error %d", task->error);

		if (task->error != eCLE_Success)
		{
			session->started = true;
			StopTaskRunning(mmTaskID);
		}
	}
}

void CCryLiveMatchMaking::TickSessionEnd(CryMatchMakingTaskID mmTaskID)
{
	ECryLobbyError error = TickTask(mmTaskID);

	if (error != eCLE_Pending)
	{
		UpdateTaskError(mmTaskID, error);

		if (m_task[mmTaskID].error != eCLE_Success)
		{
			m_sessions[m_task[mmTaskID].session].started = true;
		}

		StopTaskRunning(mmTaskID);
		CryLogAlways("[Lobby] XSessionEnd Done error %d", m_task[mmTaskID].error);
	}
}

void CCryLiveMatchMaking::ProcessSessionEnd(const TNetAddress& addr, const uint8* data, uint32 length)
{
	uint32 bufferPos = 0;
	XNKID xnkid;
	CrySessionHandle h;

	StartRemovePacket(data, length, &bufferPos);
	RemovePacketXNKID(data, length, &bufferPos, &xnkid);

	h = FindSessionFromID(xnkid);

	if (h != CrySessionInvalidHandle)
	{
		StartSessionEndMain(CryMatchMakingInvalidTaskID, h);
	}
}

ECryLobbyError CCryLiveMatchMaking::SessionDelete(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	h = GetSessionHandleFromGameSessionHandle(h);

	if ((h < MAX_MATCHMAKING_SESSIONS) && m_sessions[h].used)
	{
		SSession* session = &m_sessions[h];
		CryMatchMakingTaskID mmTaskID;

		error = StartTask(eT_SessionDelete, false, session->localConnection.users[0], &mmTaskID, taskID, h, cb, cbArg);

		if (error == eCLE_Success)
		{
			FROM_GAME(&CCryLiveMatchMaking::StartTaskRunning, this, mmTaskID);
		}
	}
	else
	{
		error = eCLE_InvalidSession;
	}

	CryLogAlways("[Lobby] Start SessionDelete error %d", error);

	return error;
}

void CCryLiveMatchMaking::StartSessionDelete(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];

	StartSubTask(eT_SessionDeleteWaitStart, mmTaskID);

	CryLogAlways("[Lobby] Waiting to start XSessionDelete");
}

void CCryLiveMatchMaking::TickSessionDeleteWaitStart(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];
	bool okToStart = true;

	// Make sure all connections have been freed.
	for (uint32 i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
	{
		SSession::SRConnection* connection = &session->remoteConnection[i];

		if (connection->used)
		{
			okToStart = false;

			if (connection->state == SSession::SRConnection::eRCS_Joined)
			{
				SetRemoteConnectionState(task->session, i, SSession::SRConnection::eRCS_WaitingToLeave);
			}
		}
	}

	if (okToStart)
	{
		StartSubTask(eT_SessionDelete, mmTaskID);
		ECryLobbyError error = CryLiveLobbyGetErrorFromLive(XSessionDelete(session->handle, &task->overlapped));
		UpdateTaskError(mmTaskID, error);

		CryLogAlways("[Lobby] Start Call XSessionDelete error %d", task->error);

		if (error != eCLE_Success)
		{
			StopTaskRunning(mmTaskID);
		}
	}
}

void CCryLiveMatchMaking::TickSessionDelete(CryMatchMakingTaskID mmTaskID)
{
	ECryLobbyError error = TickTask(mmTaskID);

	if (error != eCLE_Pending)
	{
		if (error == eCLE_Success)
		{
			STask* task = &m_task[mmTaskID];
			SSession* session = &m_sessions[task->session];

			CloseHandle(session->handle);

			// Disconnect our local connection
			SessionDisconnectRemoteConnectionViaNub(eDC_UserRequested, session->localConnection.uid, "Session deleted");

			// Free any remaining remote connections
			for (uint32 i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
			{
				SSession::SRConnection* connection = &session->remoteConnection[i];

				if (connection->used)
				{
					SetRemoteConnectionState(task->session, i, SSession::SRConnection::eRCS_None);
					FreeRemoteConnection(task->session, i);
				}
			}

			InformVoiceLocalUsersChanged();
			FreeSessionHandle(task->session);
		}

		UpdateTaskError(mmTaskID, error);
		StopTaskRunning(mmTaskID);
		CryLogAlways("[Lobby] XSessionDelete Done error %d", m_task[mmTaskID].error);
	}
}

ECryLobbyError CCryLiveMatchMaking::SessionSearch(uint32 user, SCrySessionSearchParam* param, CryLobbyTaskID* taskID, CryMatchmakingSessionSearchCallback cb, void* cbArg)
{
	LOBBY_AUTO_LOCK;

	CryMatchMakingTaskID mmTaskID;

	ECryLobbyError error = StartTask(eT_SessionSearch, false, user, &mmTaskID, taskID, CrySessionInvalidHandle, cb, cbArg);

	if (error == eCLE_Success)
	{
		STask* task = &m_task[mmTaskID];

		error = CreateTaskParam(mmTaskID, 0, param, 1, sizeof(SCrySessionSearchParam));

		if (error == eCLE_Success)
		{
			error = CreateTaskParam(mmTaskID, 1, param->m_data, param->m_numData, param->m_numData*sizeof(param->m_data[0]));

			if (error == eCLE_Success)
			{
				FROM_GAME(&CCryLiveMatchMaking::StartTaskRunning, this, mmTaskID);
			}
		}

		if (error != eCLE_Success)
		{
			FreeTask(mmTaskID);
		}
	}

	CryLogAlways("[Lobby] Start SessionSearch error %d", error);

	return error;
}

void CCryLiveMatchMaking::StartSessionSearch(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SCrySessionSearchParam* param = (SCrySessionSearchParam*)m_lobby->MemGetPtr(task->params[0]);
	uint32 numProperties = 0;
	uint32 numContexts = 0;
	XUSER_PROPERTY propertiesTable[MAX_MATCHMAKING_SESSION_USER_DATA];
	XUSER_CONTEXT contextsTable[MAX_MATCHMAKING_SESSION_USER_DATA];
	DWORD procedureIndex = param->m_type;
	DWORD numUsers = param->m_numFreeSlots;
	DWORD numResults = param->m_maxNumReturn;

	param->m_data = (SCrySessionSearchData*)m_lobby->MemGetPtr(task->params[1]);

	contextsTable[numContexts].dwContextId = X_CONTEXT_GAME_TYPE;
	contextsTable[numContexts].dwValue = param->m_ranked ? X_CONTEXT_GAME_TYPE_RANKED : X_CONTEXT_GAME_TYPE_STANDARD;
	numContexts++;

	for (uint32 i = 0; i < param->m_numData; i++)
	{
		SLiveUserData liveData;

		UpdateTaskError(mmTaskID, CryUserDataToLiveUserData(&param->m_data[i].m_data, &liveData));

		if (task->error == eCLE_Success)
		{
			if (liveData.typeContext)
			{
				contextsTable[numContexts] = liveData.context;
				numContexts++;
			}
			else
			{
				propertiesTable[numProperties] = liveData.property;
				numProperties++;
			}
		}
		else
		{
			break;
		}
	}

	m_lobby->MemFree(task->params[0]);
	m_lobby->MemFree(task->params[1]);
	task->params[0] = TMemInvalidHdl;
	task->params[1] = TMemInvalidHdl;

	if (task->error == eCLE_Success)
	{
		if (numContexts > 0)
		{
			UpdateTaskError(mmTaskID, CreateTaskParam(mmTaskID, 0, contextsTable, numContexts, numContexts*sizeof(contextsTable[0])));
		}

		if ((task->error == eCLE_Success) && (numProperties > 0))
		{
			UpdateTaskError(mmTaskID, CreateTaskParam(mmTaskID, 1, propertiesTable, numProperties, numProperties*sizeof(propertiesTable[0])));
		}

		if (task->error == eCLE_Success)
		{
			DWORD buffersize = 0;
			DWORD ret;

			ret = XSessionSearchEx(procedureIndex, task->user, numResults, numUsers, 0, 0, NULL, NULL, &buffersize, NULL, NULL);

			if ((ret == ERROR_INSUFFICIENT_BUFFER) && (buffersize > 0))
			{
				UpdateTaskError(mmTaskID, CreateTaskParam(mmTaskID, 2, NULL, 0, buffersize));

				if (task->error == eCLE_Success)
				{
					XUSER_CONTEXT* contexts=NULL; 
					XUSER_PROPERTY* properties=NULL;

					if (numContexts)
					{
						contexts = (XUSER_CONTEXT*)m_lobby->MemGetPtr(task->params[0]);
					}
					if (numProperties)
					{
						properties = (XUSER_PROPERTY*)m_lobby->MemGetPtr(task->params[1]);
					}

					XSESSION_SEARCHRESULT_HEADER* results = (XSESSION_SEARCHRESULT_HEADER*)m_lobby->MemGetPtr(task->params[2]);

					ret = XSessionSearchEx(procedureIndex, task->user, numResults, numUsers, numProperties, numContexts, properties, contexts, &buffersize, results, &task->overlapped);
					UpdateTaskError(mmTaskID, CryLiveLobbyGetErrorFromLive(ret));
				}
			}
			else
			{
				UpdateTaskError(mmTaskID, CryLiveLobbyGetErrorFromLive(ret));
			}
		}
	}

	if (task->error != eCLE_Success)
	{
		StopTaskRunning(mmTaskID);
	}
}

void CCryLiveMatchMaking::TickSessionSearch(CryMatchMakingTaskID mmTaskID)
{
	ECryLobbyError error = TickTask(mmTaskID);

	if (error != eCLE_Pending)
	{
		UpdateTaskError(mmTaskID, error);
		StopTaskRunning(mmTaskID);
	}
}

void CCryLiveMatchMaking::EndSessionSearch(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];

	if (task->error == eCLE_Success)
	{
		XSESSION_SEARCHRESULT_HEADER* results = (XSESSION_SEARCHRESULT_HEADER*)m_lobby->MemGetPtr(task->params[2]);

		CryLogAlways("[Lobby] Found %d Sessions", results->dwSearchResults);
		
		for (uint32 i = 0; i < results->dwSearchResults; i++)
		{
			XSESSION_SEARCHRESULT* liveResult = &results->pResults[i];
			SCrySessionSearchResult result;
			SCrySessionUserData userData[MAX_MATCHMAKING_SESSION_USER_DATA];
			SCryLiveSessionID* id = new SCryLiveSessionID;

			id->m_info = liveResult->info;
			id->m_gameType = X_CONTEXT_GAME_TYPE_STANDARD;
			id->m_fromInvite = FALSE;
			result.m_id = id;

			result.m_numFilledSlots = liveResult->dwFilledPrivateSlots + liveResult->dwFilledPublicSlots;

			result.m_data.m_ranked = false;
			result.m_data.m_name[0] = 0;
			result.m_data.m_data = userData;
			result.m_data.m_numData = 0;

			result.m_data.m_numPublicSlots = liveResult->dwOpenPublicSlots + liveResult->dwFilledPublicSlots;
			result.m_data.m_numPrivateSlots = liveResult->dwOpenPrivateSlots + liveResult->dwFilledPrivateSlots;

			for (uint32 j = 0; j < liveResult->cContexts; j++)
			{
				if (liveResult->pContexts[j].dwContextId == X_CONTEXT_GAME_TYPE)
				{
					id->m_gameType = liveResult->pContexts[j].dwValue;
					result.m_data.m_ranked = liveResult->pContexts[j].dwValue == X_CONTEXT_GAME_TYPE_RANKED;
					break;
				}
			}

			for (uint32 j = 0; j < liveResult->cProperties; j++)
			{
				if (liveResult->pProperties[j].dwPropertyId == X_PROPERTY_GAMER_HOSTNAME)
				{
					for (uint32 k = 0; k < liveResult->pProperties[j].value.string.cbData; k++)
					{
						result.m_data.m_name[k] = (char)liveResult->pProperties[j].value.string.pwszData[k];
					}

					break;
				}
			}

			for (uint32 j = 0; j < m_registeredUserData.num; j++)
			{
				uint32 liveType = XPROPERTYTYPEFROMID(m_registeredUserData.data[j].m_id);

				if (liveType == XUSER_DATA_TYPE_CONTEXT)
				{
					for (uint32 k = 0; k < liveResult->cContexts; k++)
					{
						if (liveResult->pContexts[k].dwContextId == m_registeredUserData.data[j].m_id)
						{
							SLiveUserData liveData;

							liveData.typeContext = true;
							liveData.context = liveResult->pContexts[k];
							LiveUserDataToCryUserData(&liveData, &result.m_data.m_data[result.m_data.m_numData], m_registeredUserData.data[j].m_type);
							result.m_data.m_numData++;
							break;
						}
					}
				}
				else
				{
					for (uint32 k = 0; k < liveResult->cProperties; k++)
					{
						if (liveResult->pProperties[k].dwPropertyId == m_registeredUserData.data[j].m_id)
						{
							SLiveUserData liveData;

							liveData.typeContext = false;
							liveData.property = liveResult->pProperties[k];
							LiveUserDataToCryUserData(&liveData, &result.m_data.m_data[result.m_data.m_numData], m_registeredUserData.data[j].m_type);
							result.m_data.m_numData++;
							break;
						}
					}
				}
			}

			((CryMatchmakingSessionSearchCallback)task->cb)(task->lTaskID, eCLE_SuccessContinue, &result, task->cbArg);
		}
	}

	((CryMatchmakingSessionSearchCallback)task->cb)(task->lTaskID, task->error, NULL, task->cbArg);
}

ECryLobbyError CCryLiveMatchMaking::SessionJoin(uint32* users, int numUsers, CrySessionID id, CryLobbyTaskID* taskID, CryMatchmakingSessionJoinCallback cb, void* cbArg)
{
	LOBBY_AUTO_LOCK;

	CrySessionHandle h;
	ECryLobbyError error = CreateSessionHandle(&h, false, users, numUsers);

	if (error == eCLE_Success)
	{
		CryMatchMakingTaskID mmTaskID;

		error = StartTask(eT_SessionJoin, false, users[0], &mmTaskID, taskID, h, cb, cbArg);

		if (error == eCLE_Success)
		{
			SCryLiveSessionID* liveID = (SCryLiveSessionID*)id.get();
			SSession* session = &m_sessions[h];

			session->info = liveID->m_info;
			session->gameType = liveID->m_gameType;
			session->flags = XSESSION_CREATE_USES_PEER_NETWORK | XSESSION_CREATE_USES_MATCHMAKING | XSESSION_CREATE_USES_STATS | XSESSION_CREATE_USES_PRESENCE;

			if (session->gameType == X_CONTEXT_GAME_TYPE_RANKED)
			{
				session->flags |= XSESSION_CREATE_USES_ARBITRATION;
			}

			for (uint32 i = 0; i < numUsers; i++)
			{
				session->localConnection.privateSlot[i] = liveID->m_fromInvite;
			}

			FROM_GAME(&CCryLiveMatchMaking::StartTaskRunning, this, mmTaskID);
		}
		else
		{
			FreeSessionHandle(h);
		}
	}

	CryLogAlways("[Lobby] Start SessionJoin error %d", error);

	return error;
}

void CCryLiveMatchMaking::StartSessionJoin(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];

	StartSubTask(eT_SessionJoinWaitStart, mmTaskID);

	CryLogAlways("[Lobby] Waiting to start XSessionCreate");
}

void CCryLiveMatchMaking::TickSessionJoinWaitStart(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];
	bool okToStart = true;

	if (task->canceled)
	{
		// Task has been canceled and still waiting to start so can stop now.
		FreeSessionHandle(task->session);
		StopTaskRunning(mmTaskID);
		return;
	}

	// If running other tasks on the same session XNKID then wait for them to finish
	for (uint32 i = 0; i < MAX_MATCHMAKING_TASKS; i++)
	{
		STask* testTask = &m_task[i];

		if ((i != mmTaskID) && testTask->used && (testTask->session != CrySessionInvalidHandle))
		{
			SSession* testSession = &m_sessions[testTask->session];

			if (memcmp(&session->info.sessionID, &testSession->info.sessionID, sizeof(testSession->info.sessionID)) == 0)
			{
				okToStart = false;
				break;
			}
		}
	}

	if (okToStart)
	{
		// Check if a handle to a session with the same XNKID has already been given to user.
		for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
		{
			SSession* testSession = &m_sessions[i];
			
			if ((i != task->session) && testSession->used && (memcmp(&session->info.sessionID, &testSession->info.sessionID, sizeof(testSession->info.sessionID)) == 0))
			{
				UpdateTaskError(mmTaskID, eCLE_AlreadyInSession);
				StopTaskRunning(mmTaskID);
				return;
			}
		}

		StartSubTask(eT_SessionJoin, mmTaskID);

		XUserSetContext(task->user, X_CONTEXT_GAME_TYPE, session->gameType);
		XUserSetContext(task->user, X_CONTEXT_GAME_MODE, 0);

		UpdateTaskError(mmTaskID, CryLiveLobbyGetErrorFromLive(XSessionCreate(session->flags, session->localConnection.users[0], 16, 0, &session->nonce, &session->info, &task->overlapped, &session->handle)));

		if (task->error != eCLE_Success)
		{
			FreeSessionHandle(task->session);
			StopTaskRunning(mmTaskID);
		}

		CryLogAlways("[Lobby] SessionJoin Start XSessionCreate error %d", task->error);
	}
}

void CCryLiveMatchMaking::TickSessionJoin(CryMatchMakingTaskID mmTaskID)
{
	ECryLobbyError error = TickTask(mmTaskID);

	if (error != eCLE_Pending)
	{
		STask* task = &m_task[mmTaskID];
		SSession* session = &m_sessions[task->session];

		UpdateTaskError(mmTaskID, error);

		if (task->error == eCLE_Success)
		{
			IN_ADDR addr;

			UpdateTaskError(mmTaskID, CryLiveLobbyGetErrorFromLive(XNetXnAddrToInAddr(&session->info.hostAddress, &session->info.sessionID, &addr)));

			if (task->error == eCLE_Success)
			{
				const uint32 MaxBufferSize = PacketHeaderSize + PacketXNKIDSize + PacketUINT8Size + MAX_LIVE_LOCAL_USERS*(PacketXUIDSize + PacketUINT8Size + PacketBOOLSize);
				uint8 buffer[MaxBufferSize];
				uint32 bufferSize = 0;
				session = &m_sessions[task->session];

				StartSubTask(eT_SessionRequestJoin, mmTaskID);

				StartAddPacket(buffer, MaxBufferSize, &bufferSize, eLivePT_SessionRequestJoin);
				AddPacketXNKID(buffer, MaxBufferSize, &bufferSize, session->info.sessionID);
				AddPacketUINT8(buffer, MaxBufferSize, &bufferSize, session->localConnection.numUsers);

				for (uint32 i = 0; i < session->localConnection.numUsers; i++)
				{
					CCryLiveLobbyService* pService = (CCryLiveLobbyService*)m_pService;
					XUID xuid;

					UpdateTaskError(mmTaskID, pService->GetUserXUID(session->localConnection.users[i], &xuid));

					if (task->error == eCLE_Success)
					{
						AddPacketXUID(buffer, MaxBufferSize, &bufferSize, xuid);
						AddPacketUINT8(buffer, MaxBufferSize, &bufferSize, (uint8)session->localConnection.users[i]);
						AddPacketBOOL(buffer, MaxBufferSize, &bufferSize, session->localConnection.privateSlot[i]);
					}
					else
					{
						break;
					}
				}

				if (task->error == eCLE_Success)
				{
					if (Send(mmTaskID, buffer, bufferSize, TNetAddress(SIPv4Addr(addr.s_addr, m_lobby->GetInternalSocketPort())), true) != eSE_Ok)
					{
						task->error = eCLE_ConnectionFailed;
					}
				}
			}

			CryLogAlways("[Lobby] SessionJoin Send request session join packet error %d", task->error);

			if (task->error != eCLE_Success)
			{
				StartSessionDelete(mmTaskID);
			}
		}
		else
		{
			FreeSessionHandle(task->session);
		}
	}
}

void CCryLiveMatchMaking::TickSessionRequestJoin(CryMatchMakingTaskID mmTaskID)
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

void CCryLiveMatchMaking::TickSessionWaitJoinRemoteFromRequestJoin(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];

	ProcessSessionRequestJoin(mmTaskID, *((TNetAddress*)m_lobby->MemGetPtr(task->params[0])), (uint8*)m_lobby->MemGetPtr(task->params[1]), task->numParams[1]);
}

void CCryLiveMatchMaking::ProcessSessionRequestJoin(CryMatchMakingTaskID mmTaskID, const TNetAddress& addr, const uint8* data, uint32 length)
{
	ECryLobbyError error = eCLE_Success;
	uint32 bufferPos = 0;
	XNKID xnkid;
	CrySessionHandle h;
	CryLobbyConnectionID c;

	if (m_lobby->ConnectionFromAddress(&c, addr))
	{
		StartRemovePacket(data, length, &bufferPos);
		RemovePacketXNKID(data, length, &bufferPos, &xnkid);

		h = FindSessionFromID(xnkid);

		if (h != CrySessionInvalidHandle)
		{
			SSession* session = &m_sessions[h];

			if (!(session->started && session->flags&XSESSION_CREATE_USES_ARBITRATION))
			{
				bool kickOldConnection = false;

				if (mmTaskID == CryMatchMakingInvalidTaskID)
				{
					error = StartTask(eT_SessionJoinRemoteFromRequestJoin, true, 0, &mmTaskID, NULL, h, NULL, NULL);
					kickOldConnection = true;
				}

				if (error == eCLE_Success)
				{
					STask* task = &m_task[mmTaskID];
					uint8 numUsers;
					XUID xuid[MAX_LIVE_LOCAL_USERS];
					uint8 localUser[MAX_LIVE_LOCAL_USERS];
					BOOL privateSlot[MAX_LIVE_LOCAL_USERS];
					XNADDR xnaddr;
					bool needWait = false;

					RemovePacketUINT8(data, length, &bufferPos, &numUsers);

					for (uint32 i = 0; i < numUsers; i++)
					{
						RemovePacketXUID(data, length, &bufferPos, &xuid[i]);
						RemovePacketUINT8(data, length, &bufferPos, &localUser[i]);
						RemovePacketBOOL(data, length, &bufferPos, &privateSlot[i]);
						CryLogAlways("[Lobby] New connection join user %llx local user %d private %d", xuid[i], localUser[i], privateSlot[i]);
					}

					for (uint32 i = 0; i < MAX_LOBBY_CONNECTIONS; i++)
					{
						SSession::SRConnection* connection = &session->remoteConnection[i];

						if (connection->used)
						{
							bool kicked = false;

							for (uint32 j = 0; j < connection->numUsers && !kicked; j++)
							{
								for (uint32 k = 0; k < numUsers; k++)
								{
									if (connection->xuid[j] == xuid[k])
									{
										if (kickOldConnection)
										{
											// Found a connection with the same xuid as the connection request so kick the old connection.
											CryLogAlways("[Lobby] Kick old connection %d uid %d with matching xuid %llx of new connection request", i, connection->uid, xuid[k]);
											SessionDisconnectRemoteConnectionViaNub(eDC_Kicked, connection->uid, "Kick old connection");
										}

										kicked = true;
										needWait = true;
										break;
									}
								}
							}
						}
					}

					if (needWait)
					{
						// Need to wait for old connections to be kicked so if just kicked take a copy of the packet data and start waiting else continue waiting.
						if (kickOldConnection)
						{
							StartSubTask(eT_SessionWaitJoinRemoteFromRequestJoin, mmTaskID);

							error = CreateTaskParam(mmTaskID, 0, &addr, 1, sizeof(TNetAddress));

							if (error == eCLE_Success)
							{
								error = CreateTaskParam(mmTaskID, 1, data, length, length);
							}

							if (error != eCLE_Success)
							{
								error = eCLE_ConnectionFailed;
							}

							task->StartTimer();

							CryLogAlways("[Lobby] Old connections kicked start waiting for kick complete");
						}
						else
						{
							if (task->GetTimer() > CryLiveMatchMakingWaitJoinRemoteFromRequestJoinTimeOut)
							{
								// Time out waiting to start XSessionJoinRemote so fail connection attempt
								error = eCLE_ConnectionFailed;
								CryLogAlways("[Lobby] Time out waiting for kick complete fail connection attempt");
							}
						}
					}
					else
					{
						sockaddr_in sockAddr;

						StartSubTask(eT_SessionJoinRemoteFromRequestJoin, mmTaskID);

						if (ConvertAddr(addr, &sockAddr))
						{
							XNetInAddrToXnAddr(sockAddr.sin_addr, &xnaddr, NULL);

							CryMatchMakingConnectionID id = AddRemoteConnection(h, c, CreateConnectionUID(), &xnaddr, sockAddr.sin_port, numUsers, xuid, localUser, privateSlot);

							if (id != CryMatchMakingInvalidConnectionID)
							{
								SSession::SRConnection* connection = &session->remoteConnection[id];

								// This Join remote has to be kept separate from the rest as the result needs to be sent back to the client trying to connect
								// So leave state the default eRCS_None so the main tick doesn't try to do any processing until join is complete.

								task->numParams[0] = id;

								error = CryLiveLobbyGetErrorFromLive(XSessionJoinRemote(session->handle, connection->numUsers, connection->xuid, connection->privateSlot, &task->overlapped));
								CryLogAlways("[Lobby] Created new connection %d uid %d XSessionJoinRemote started error %d", id, connection->uid, error);

								if (error != eCLE_Success)
								{
									FreeRemoteConnection(h, id);
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
				}
				else
				{
					error = eCLE_ConnectionFailed;
				}
			}
			else
			{
				// Arbitration has started so stop new connections
				// XSessionJoinRemote returns session full after arbitration has finished so do the same
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

	if (error != eCLE_Success)
	{
		// Can't Start adding to session so send back error
		const uint32 MaxBufferSize = PacketHeaderSize + PacketErrorSize;
		uint8 buffer[MaxBufferSize];
		uint32 bufferSize = 0;

		StartAddPacket(buffer, MaxBufferSize, &bufferSize, eLivePT_SessionRequestJoinResult);
		AddPacketError(buffer, MaxBufferSize, &bufferSize, error);

		Send(CryMatchMakingInvalidTaskID, buffer, bufferSize, addr, true);

		if (mmTaskID != CryMatchMakingInvalidTaskID)
		{
			// Task didn't start so free it
			FreeTask(mmTaskID);
		}
	}

	CryLogAlways("[Lobby] Processed Session request join packet error %d", error);
}

void CCryLiveMatchMaking::TickSessionJoinRemoteFromRequestJoin(CryMatchMakingTaskID mmTaskID)
{
	ECryLobbyError error = TickTask(mmTaskID);

	if (error != eCLE_Pending)
	{
		STask* task = &m_task[mmTaskID];
		SSession* session = &m_sessions[task->session];
		SSession::SRConnection* connection = &session->remoteConnection[task->numParams[0]];
		TNetAddress addr;

		CryLogAlways("[Lobby] XSessionJoinRemote done send back error %d", error);
		UpdateTaskError(mmTaskID, error);

		if (m_lobby->AddressFromConnection(addr, connection->connectionID))
		{
			TMemHdl h = m_lobby->MemAlloc(MAX_LIVE_PACKET_SIZE);
			
			if (h != TMemInvalidHdl)
			{
				uint8* buffer = (uint8*)m_lobby->MemGetPtr(h);
				uint32 bufferSize = 0;
				uint32 i;

				StartAddPacket(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, eLivePT_SessionRequestJoinResult);
				AddPacketError(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, task->error);

				if (task->error == eCLE_Success)
				{
					SSession::SLConnection* localConnection = &session->localConnection;

					AddPacketDWORD(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, session->gameMode);
					AddPacketDWORD(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, session->flags);
					AddPacketDWORD(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, session->numPublicSlots);
					AddPacketDWORD(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, session->numPrivateSlots);
					AddPacketNONCE(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, session->nonce);
					AddPacketUINT32(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, connection->uid);

					// Add servers local users
					AddPacketUINT8(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, localConnection->numUsers);
					AddPacketUINT32(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, localConnection->uid);

					for (i = 0; i < localConnection->numUsers; i++)
					{
						CCryLiveLobbyService* pService = (CCryLiveLobbyService*)m_pService;
						XUID xuid;

						UpdateTaskError(mmTaskID, pService->GetUserXUID(localConnection->users[i], &xuid));

						if (task->error == eCLE_Success)
						{
							AddPacketXUID(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, xuid);
							AddPacketUINT8(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, (uint8)localConnection->users[i]);
							AddPacketBOOL(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, localConnection->privateSlot[i]);
							CryLogAlways("[Lobby] Add server user %llx local user %d private %d to packet", xuid, localConnection->users[i], localConnection->privateSlot[i]);
						}
						else
						{
							break;
						}
					}

					if (task->error != eCLE_Success)
					{
						// Something went wrong trying to get our local player xuids so send back a connection failed error
						StartAddPacket(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, eLivePT_SessionRequestJoinResult);
						AddPacketError(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, eCLE_ConnectionFailed);
					}
				}

				Send(CryMatchMakingInvalidTaskID, buffer, bufferSize, addr, true);

				if (task->error == eCLE_Success)
				{
					// The new clients players were successfully joined so changed connection state
					SetRemoteConnectionState(task->session, task->numParams[0], SSession::SRConnection::eRCS_Joined);

					// Send the new clients connection to the old clients
					StartAddPacket(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, eLivePT_SessionAddRemoteConnections);

					AddPacketXNKID(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, session->info.sessionID);
					AddPacketUINT8(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, 1);

					AddPacketXNADDR(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, connection->xnaddr);
					AddPacketUINT16(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, connection->port);
					AddPacketUINT32(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, connection->uid);
					AddPacketUINT8(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, connection->numUsers);

					for (i = 0; i < connection->numUsers; i++)
					{
						AddPacketXUID(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, connection->xuid[i]);
						AddPacketUINT8(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, connection->localUser[i]);
						AddPacketBOOL(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, connection->privateSlot[i]);
					}

					SendToAll(CryMatchMakingInvalidTaskID, buffer, bufferSize, task->session, task->numParams[0], true);

					// Send the remote connections to the new client
					uint32 connectionToAdd = 0;

					while (true)
					{
						uint8 numConnections = 0;
						uint8 numAdded = 0;
						uint32 testSize;

						StartAddPacket(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, eLivePT_SessionAddRemoteConnections);
						AddPacketXNKID(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, session->info.sessionID);

						// Find out how many of the remote connections can fit in the packet

						testSize = bufferSize + PacketUINT8Size;

						for (i = connectionToAdd; i < MAX_LOBBY_CONNECTIONS; i++)
						{
							SSession::SRConnection* connectionAdd = &session->remoteConnection[i];

							if (connectionAdd->used && (connectionAdd != connection))
							{
								uint32 add = PacketXNADDRSize + PacketUINT16Size + PacketUINT32Size + PacketUINT8Size + connectionAdd->numUsers*(PacketXUIDSize + PacketUINT8Size + PacketBOOLSize);

								if (testSize + add < MAX_LIVE_PACKET_SIZE)
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
							AddPacketUINT8(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, numConnections);

							for (i = connectionToAdd, numAdded = 0; (i < MAX_LOBBY_CONNECTIONS) && (numAdded < numConnections); i++, connectionToAdd++)
							{
								SSession::SRConnection* connectionAdd = &session->remoteConnection[i];

								if (connectionAdd->used && (connectionAdd != connection))
								{
									AddPacketXNADDR(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, connectionAdd->xnaddr);
									AddPacketUINT16(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, connectionAdd->port);
									AddPacketUINT32(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, connectionAdd->uid);
									AddPacketUINT8(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, connectionAdd->numUsers);

									for (uint32 j = 0; j < connectionAdd->numUsers; j++)
									{
										AddPacketXUID(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, connectionAdd->xuid[j]);
										AddPacketUINT8(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, connectionAdd->localUser[j]);
										AddPacketBOOL(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, connectionAdd->privateSlot[j]);
									}

									numAdded++;

									CryLogAlways("[Lobby] Send connection %d uid %d users to new connection %d", i, connectionAdd->uid, task->numParams[0]);
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

					// Tell new client to call SessionStart if needed
					if (session->started)
					{
						CryLogAlways("[Lobby] Session already started send session start to connection %d uid %d", task->numParams[0], session->remoteConnection[task->numParams[0]].uid);

						StartAddPacket(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, eLivePT_SessionStart);
						AddPacketXNKID(buffer, MAX_LIVE_PACKET_SIZE, &bufferSize, session->info.sessionID);

						Send(CryMatchMakingInvalidTaskID, buffer, bufferSize, addr, true);
					}

					InformVoiceRemoteUsersJoined(task->session, task->numParams[0]);
				}
				else
				{
					FreeRemoteConnection(task->session, task->numParams[0]);
				}

				m_lobby->MemFree(h);
			}
			else
			{
				const uint32 MaxBufferSize = PacketHeaderSize + PacketErrorSize;
				uint8 buffer[MaxBufferSize];
				uint32 bufferSize = 0;

				StartAddPacket(buffer, MaxBufferSize, &bufferSize, eLivePT_SessionRequestJoinResult);
				AddPacketError(buffer, MaxBufferSize, &bufferSize, eCLE_ConnectionFailed);

				Send(CryMatchMakingInvalidTaskID, buffer, bufferSize, addr, true);

				FreeRemoteConnection(task->session, task->numParams[0]);
			}
		}

		FreeTask(mmTaskID);
	}
}

void CCryLiveMatchMaking::ProcessSessionRequestJoinResult(const TNetAddress& addr, const uint8* data, uint32 length)
{
	ECryLobbyError error = eCLE_Success;
	CryLobbyConnectionID c;

	if (m_lobby->ConnectionFromAddress(&c, addr))
	{
		CryMatchMakingTaskID mmTaskID = FindTaskFromTaskConnectionID(eT_SessionRequestJoin, c);

		if (mmTaskID != CryMatchMakingInvalidTaskID)
		{
			STask* task = &m_task[mmTaskID];
			uint32 bufferPos = 0;

			StartRemovePacket(data, length, &bufferPos);
			RemovePacketError(data, length, &bufferPos, &error);

			CryLogAlways("[Lobby] Received SessionRequestJoinResult error %d", error);

			UpdateTaskError(mmTaskID, error);

			if (task->error == eCLE_Success)
			{
				SSession* session = &m_sessions[task->session];
				DWORD flags;
				uint8 numUsers;
				XUID xuid[MAX_LIVE_LOCAL_USERS];
				uint8 localUser[MAX_LIVE_LOCAL_USERS];
				BOOL privateSlot[MAX_LIVE_LOCAL_USERS];
				XNADDR xnaddr;
				uint32 hostConnectionUID;

				RemovePacketDWORD(data, length, &bufferPos, &session->gameMode);
				RemovePacketDWORD(data, length, &bufferPos, &flags);
				RemovePacketDWORD(data, length, &bufferPos, &session->numPublicSlots);
				RemovePacketDWORD(data, length, &bufferPos, &session->numPrivateSlots);
				RemovePacketNONCE(data, length, &bufferPos, &session->nonce);
				RemovePacketUINT32(data, length, &bufferPos, &session->localConnection.uid);

				CryLogAlways("[Lobby] Created local connection uid %d", session->localConnection.uid);

				RemovePacketUINT8(data, length, &bufferPos, &numUsers);
				RemovePacketUINT32(data, length, &bufferPos, &hostConnectionUID);

				for (uint32 i = 0; i < numUsers; i++)
				{
					RemovePacketXUID(data, length, &bufferPos, &xuid[i]);
					RemovePacketUINT8(data, length, &bufferPos, &localUser[i]);
					RemovePacketBOOL(data, length, &bufferPos, &privateSlot[i]);
					CryLogAlways("[Lobby] Got server user %llx private %d", xuid[i], privateSlot[i]);
				}

				sockaddr_in sockAddr;

				if (ConvertAddr(addr, &sockAddr))
				{
					UpdateTaskError(mmTaskID, CryLiveLobbyGetErrorFromLive(XNetInAddrToXnAddr(sockAddr.sin_addr, &xnaddr, NULL)));

					if (task->error == eCLE_Success)
					{
						CryMatchMakingConnectionID id = AddRemoteConnection(task->session, c, hostConnectionUID, &xnaddr, sockAddr.sin_port, numUsers, xuid, localUser, privateSlot);

						if (id != CryMatchMakingInvalidConnectionID)
						{
							SSession::SRConnection* connection = &session->remoteConnection[id];

							CryLogAlways("[Lobby] Created server connection %d uid %d new users waiting to join", id, hostConnectionUID);

							session->hostConnectionID = id;
							SetRemoteConnectionState(task->session, id, SSession::SRConnection::eRCS_WaitingToJoin);

							const DWORD MODIFY_FLAGS_ALLOWED = XSESSION_CREATE_USES_ARBITRATION|XSESSION_CREATE_INVITES_DISABLED|XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED|XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED;

							session->flags &= ~MODIFY_FLAGS_ALLOWED;
							session->flags |= (flags&MODIFY_FLAGS_ALLOWED);

							// The connection request was successful so now modify our session with the correct settings
							StartSubTask(eT_SessionJoinModifySession, mmTaskID);

							XUserSetContext(session->localConnection.users[0], X_CONTEXT_GAME_MODE, session->gameMode);

							UpdateTaskError(mmTaskID, CryLiveLobbyGetErrorFromLive(XSessionModify(session->handle, session->flags, session->numPublicSlots, session->numPrivateSlots, &task->overlapped)));

							CryLogAlways("[Lobby] XSessionModify started error %d", task->error);
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
			}

			if (task->error != eCLE_Success)
			{
				StartSessionDelete(mmTaskID);
			}

			CryLogAlways("[Lobby] Processed session request join result error %d", task->error);
		}
	}
}

void CCryLiveMatchMaking::TickSessionJoinModifySession(CryMatchMakingTaskID mmTaskID)
{
	ECryLobbyError error = TickTask(mmTaskID);

	if (error != eCLE_Pending)
	{
		STask* task = &m_task[mmTaskID];

		UpdateTaskError(mmTaskID, error);

		CryLogAlways("[Lobby] SessionJoin XSessionModify done error %d", task->error);

		if (task->error == eCLE_Success)
		{
			SessionJoinLocal(mmTaskID);
		}
		else
		{
			StartSessionDelete(mmTaskID);
		}
	}
}

void CCryLiveMatchMaking::ProcessSessionAddRemoteConnections(const TNetAddress& addr, const uint8* data, uint32 length)
{
	uint32 bufferPos = 0;
	XNKID xnkid;
	CrySessionHandle h;

	StartRemovePacket(data, length, &bufferPos);
	RemovePacketXNKID(data, length, &bufferPos, &xnkid);

	h = FindSessionFromID(xnkid);

	if (h != CrySessionInvalidHandle)
	{
		SSession* session = &m_sessions[h];
		uint8 numConnections;

		RemovePacketUINT8(data, length, &bufferPos, &numConnections);

		for (uint32 i = 0; i < numConnections; i++)
		{
			XNADDR xnaddr;
			uint16 port;
			uint8 numUsers;
			XUID xuid[MAX_LIVE_LOCAL_USERS];
			uint8 localUser[MAX_LIVE_LOCAL_USERS];
			BOOL privateSlot[MAX_LIVE_LOCAL_USERS];
			uint32 connectionUID;

			RemovePacketXNADDR(data, length, &bufferPos, &xnaddr);
			RemovePacketUINT16(data, length, &bufferPos, &port);
			RemovePacketUINT32(data, length, &bufferPos, &connectionUID);
			RemovePacketUINT8(data, length, &bufferPos, &numUsers);

			for (uint32 j = 0; j < numUsers; j++)
			{
				RemovePacketXUID(data, length, &bufferPos, &xuid[j]);
				RemovePacketUINT8(data, length, &bufferPos, &localUser[j]);
				RemovePacketBOOL(data, length, &bufferPos, &privateSlot[j]);
				CryLogAlways("[Lobby] Got new connection user %llx local user %d private %d", xuid[j], localUser[j], privateSlot[j]);
			}

			CryMatchMakingConnectionID id = AddRemoteConnection(h, CryLobbyInvalidConnectionID, connectionUID, &xnaddr, port, numUsers, xuid, localUser, privateSlot);

			if (id != CryMatchMakingInvalidConnectionID)
			{
				CryLogAlways("[Lobby] Add new connection %d uid %d set to waiting to join", id, connectionUID);
				SetRemoteConnectionState(h, id, SSession::SRConnection::eRCS_WaitingToJoin);
			}
		}
	}
}

void CCryLiveMatchMaking::EndSessionJoin(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];

	if (task->error == eCLE_Success)
	{
		SSession* session = &m_sessions[task->session];
		IN_ADDR addr;

		UpdateTaskError(mmTaskID, CryLiveLobbyGetErrorFromLive(XNetXnAddrToInAddr(&session->info.hostAddress, &session->info.sessionID, &addr)));

		if (task->error == eCLE_Success)
		{
			((CryMatchmakingSessionJoinCallback)task->cb)(task->lTaskID, task->error, CreateGameSessionHandle(task->session, session->localConnection.uid), addr.s_addr, m_lobby->GetInternalSocketPort(), task->cbArg);
			return;
		}
	}

	((CryMatchmakingSessionJoinCallback)task->cb)(task->lTaskID, task->error, task->session, 0, 0, task->cbArg);
}

void CCryLiveMatchMaking::OnPacket(const TNetAddress& addr, const uint8* data, uint32 length)
{
	CryLiveLobbyPacketType type;
	uint32 pos;

	StartRemovePacket(data, length, &pos, &type);

	switch (type)
	{
	case eLivePT_SessionRequestJoin:
		ProcessSessionRequestJoin(CryMatchMakingInvalidTaskID, addr, data, length);
		break;

	case eLivePT_SessionRequestJoinResult:
		ProcessSessionRequestJoinResult(addr, data, length);
		break;

	case eLivePT_SessionAddRemoteConnections:
		ProcessSessionAddRemoteConnections(addr, data, length);
		break;

	case eLivePT_SessionStart:
		ProcessSessionStart(addr, data, length);
		break;

	case eLivePT_SessionIHaveRegistered:
		ProcessSessionIHaveRegistered(addr, data, length);
		break;

	case eLivePT_SessionRegistrationFinished:
		ProcessSessionRegistrationFinished(addr, data, length);
		break;

	case eLivePT_SessionEnd:
		ProcessSessionEnd(addr, data, length);
		break;

#if NETWORK_HOST_MIGRATION
	case eLivePT_HostMigrationServer:
		ProcessHostMigrationFromServer(addr, data, length);
		break;

	case eLivePT_HostMigrationClient:
		ProcessHostMigrationFromClient(addr, data, length);
		break;
#endif

	default:
		CCryMatchMaking::OnPacket(addr, data, length);
		break;
	}
}

HANDLE CCryLiveMatchMaking::GetLiveSessionHandle(CrySessionHandle h)
{
	if (h != CrySessionInvalidHandle)
	{
		SSession* pSession = &m_sessions[h];

		if (pSession->used)
		{
			return pSession->handle;
		}
	}

	return INVALID_HANDLE_VALUE;
}

bool CCryLiveMatchMaking::GetRemoteUserConnectionInfo(CryLobbyConnectionID connectionID, uint32 remoteUser, CryMatchMakingConnectionUID* uid, XUID* xuid, uint32* localUser)
{
	for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
	{
		SSession* session = &m_sessions[i];

		if (session->used)
		{
			for (uint32 j = 0; j < MAX_LOBBY_CONNECTIONS; j++)
			{
				SSession::SRConnection* connection = &session->remoteConnection[j];

				if (connection->used && (connection->connectionID == connectionID))
				{
					uint32 user = connection->localUser2Index[remoteUser];

					if (uid)
					{
						*uid = connection->uid;
					}

					if (xuid)
					{
						*xuid = connection->xuid[user];
					}

					if (localUser)
					{
						*localUser = user;
					}

					return true;
				}
			}
		}
	}

	return false;
}

void CCryLiveMatchMaking::InformVoiceLocalUsersChanged()
{
	// Tell the voice service which local users are currently in a session.
	CCryLiveVoice* voice = (CCryLiveVoice*)m_pService->GetVoice();

	if (voice)
	{
		bool users[MAX_LIVE_LOCAL_USERS];

		for (uint32 i = 0; i < MAX_LIVE_LOCAL_USERS; i++)
		{
			users[i] = false;
		}

		for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
		{
			SSession* session = &m_sessions[i];

			if (session->used)
			{
				for (uint32 j = 0; j < session->localConnection.numUsers; j++)
				{
					users[session->localConnection.users[j]] = true;
				}
			}
		}

		voice->RegisterLocalUsers(users);
	}
}

void CCryLiveMatchMaking::InformVoiceRemoteUsersJoined(CrySessionHandle h, CryMatchMakingConnectionID id)
{
	CCryLiveVoice* voice = (CCryLiveVoice*)m_pService->GetVoice();

	if (voice)
	{
		SSession* session = &m_sessions[h];
		SSession::SRConnection* connection = &session->remoteConnection[id];

		for (uint32 i = 0; i < connection->numUsers; i++)
		{
			voice->RegisterRemoteUser(connection->xuid[i], connection->connectionID, connection->uid, i, connection->localUser[i]);
		}
	}
}

void CCryLiveMatchMaking::InformVoiceRemoteUsersLeft(CrySessionHandle h, CryMatchMakingConnectionID id)
{
	CCryLiveVoice* voice = (CCryLiveVoice*)m_pService->GetVoice();

	if (voice)
	{
		SSession* session = &m_sessions[h];
		SSession::SRConnection* connection = &session->remoteConnection[id];

		for (uint32 i = 0; i < connection->numUsers; i++)
		{
			voice->UnregisterRemoteUser(connection->xuid[i]);
		}
	}
}

#if NETWORK_HOST_MIGRATION
void CCryLiveMatchMaking::HostMigrationInitiate()
{
	m_hostMigrationFinished = false;
}

ECryLobbyError CCryLiveMatchMaking::HostMigrationServer(void)
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
	FindLocalConnectionFromUID(hostChannelID, &h);
	h = GetSessionHandleFromGameSessionHandle(h);

	if ((h < MAX_MATCHMAKING_SESSIONS) && m_sessions[h].used)
	{
		SSession* session = &m_sessions[h];
		CryMatchMakingTaskID mmTaskID;

		session->host = true;
		session->hostConnectionID = CryMatchMakingInvalidConnectionID;
		error = StartTask(eT_SessionMigrateHostServer, false, session->localConnection.users[0], &mmTaskID, NULL, h, NULL, NULL);

		if (error == eCLE_Success)
		{
			CryLog("Host Migration: matchmaking migrating the LIVE session on the SERVER");
			FROM_GAME(&CCryLiveMatchMaking::StartTaskRunning, this, mmTaskID);
		}
	}
	else
	{
		error = eCLE_InvalidSession;
	}

	if (error != eCLE_Success)
	{
		CryLog("Host Migration: CCryLiveMatchMaking::HostMigrationServer() error %d", error);
	}

	return error;
}

void CCryLiveMatchMaking::HostMigrationServerNT(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];

	UpdateTaskError(mmTaskID, CryLiveLobbyGetErrorFromLive(XSessionMigrateHost(session->handle, task->user, &session->info, &task->overlapped)));

	if (task->error != eCLE_Success)
	{
		CryLog("Host Migration: matchmaking failed to migrate the LIVE session on the SERVER");

		StopTaskRunning(mmTaskID);
	}
}

void CCryLiveMatchMaking::TickHostMigrationServerNT(CryMatchMakingTaskID mmTaskID)
{
	ECryLobbyError error = TickTask(mmTaskID);

	if (error != eCLE_Pending)
	{
		if (error == eCLE_Success)
		{
			CryLog("Host Migration: matchmaking SERVER migrated LIVE session successfully - informing CLIENTS");

			STask* task = &m_task[mmTaskID];
			SSession* session = &m_sessions[task->session];

			const uint32 MaxBufferSize = PacketHeaderSize + PacketXSESSIONINFOSize + sizeof(uint32);
			uint8 buffer[MaxBufferSize];
			uint32 bufferSize = 0;

			StartAddPacket(buffer, MaxBufferSize, &bufferSize, eLivePT_HostMigrationServer);
			AddPacketXSessionInfo(buffer, MaxBufferSize, &bufferSize, &session->info);
			AddPacketUINT32(buffer, MaxBufferSize, &bufferSize, mmTaskID);

			SendToAll(mmTaskID, buffer, bufferSize, task->session, CryMatchMakingInvalidConnectionID, true);

			StartSubTask(eT_SessionMigrateHostFinish, mmTaskID);
			task->StartTimer();

			m_newHostAddress = TNetAddress(TLocalNetAddress());
			m_newHostAddressValid = true;
			m_hostMigrationFinished = true;
		}
		else
		{
			StopTaskRunning(mmTaskID);
		}
	}
}

ECryLobbyError CCryLiveMatchMaking::HostMigrationClient(CrySessionHandle h, XSESSION_INFO* pInfo, CryMatchMakingTaskID hostTaskID)
{
	ECryLobbyError error = eCLE_Success;

	if ((h < MAX_MATCHMAKING_SESSIONS) && m_sessions[h].used)
	{
		SSession* session = &m_sessions[h];
		CryMatchMakingTaskID mmTaskID;

		session->host = false;
		error = StartTask(eT_SessionMigrateHostClient, true, session->localConnection.users[0], &mmTaskID, NULL, h, NULL, NULL);

		if (error == eCLE_Success)
		{
			CryLog("Host Migration: matchmaking migrating to the new LIVE session on the CLIENT");

			STask* task = &m_task[mmTaskID];
			task->returnTaskID = hostTaskID;
			session->info = *pInfo;
			UpdateTaskError(mmTaskID, CryLiveLobbyGetErrorFromLive(XSessionMigrateHost(session->handle, XUSER_INDEX_NONE, &session->info, &task->overlapped)));
		}
	}
	else
	{
		error = eCLE_InvalidSession;
	}

	if (error != eCLE_Success)
	{
		CryLog("Host Migration: CCryLiveMatchMaking::HostMigrationClient() error %d", error);
	}

	return error;

}

void CCryLiveMatchMaking::TickHostMigrationClientNT(CryMatchMakingTaskID mmTaskID)
{
	ECryLobbyError error = TickTask(mmTaskID);

	if (error != eCLE_Pending)
	{
		if (error == eCLE_Success)
		{
			CryLog("Host Migration: matchmaking CLIENT migrated to new LIVE session successfully");

			STask* task = &m_task[mmTaskID];
			SSession* session = &m_sessions[task->session];

			const uint32 MaxBufferSize = PacketHeaderSize + PacketXSESSIONINFOSize + sizeof(uint32);
			uint8 buffer[MaxBufferSize];
			uint32 bufferSize = 0;

			StartAddPacket(buffer, MaxBufferSize, &bufferSize, eLivePT_HostMigrationClient);
			AddPacketUINT32(buffer, MaxBufferSize, &bufferSize, task->returnTaskID);

			Send(mmTaskID, buffer, bufferSize, task->session, session->hostConnectionID, true);
			m_hostMigrationFinished = true;
			m_newHostAddressValid = true;
		}

		StopTaskRunning(mmTaskID);
	}
}

void CCryLiveMatchMaking::ProcessHostMigrationFromServer(const TNetAddress& addr, const uint8* pData, uint32 length)
{
	uint32 bufferPos = 0;
	XSESSION_INFO info;
	CrySessionHandle h = CrySessionInvalidHandle;
	CryMatchMakingTaskID hostTaskID = CryMatchMakingInvalidTaskID;

	CryLog("Host Migration: matchmaking CLIENT received LIVE session details - migrating to new LIVE session");

	StartRemovePacket(pData, length, &bufferPos);
	RemovePacketXSessionInfo(pData, length, &bufferPos, &info);
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

			HostMigrationClient(h, &info, hostTaskID);
			m_newHostAddress = addr;
		}
	}

	// If HostMigrationClient() was not called, this client will be pruned (after timeout) on the new host
}

void CCryLiveMatchMaking::ProcessHostMigrationFromClient(const TNetAddress& addr, const uint8* pData, uint32 length)
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

void CCryLiveMatchMaking::TickHostMigrationFinishNT(CryMatchMakingTaskID mmTaskID)
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

bool CCryLiveMatchMaking::GetNewHostAddress(char* address)
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
#endif
