
#include "StdAfx.h"

#include "CryPSNLobby.h"
#include "CryPSNMatchMaking.h"

#if USE_PSN

#include "Endian.h"
#include "Protocol/NetChannel.h"
#include "Protocol/NetNub.h"
#include "IGame.h"
#include "IGameFramework.h"


#include <netex/net.h>
#include <netex/libnetctl.h>

#define HOST_MIGRATION_SAFETY_SLEEP (200)

CryMatchMakingConnectionID CCryPSNMatchMaking::FindConnectionIDFromMemberID(SSession* session,SceNpMatching2RoomMemberId memberId)
{
	uint32 a;

	for (a=0;a<MAX_LOBBY_CONNECTIONS;a++)
	{
		SSession::SRConnection* connection = &session->remoteConnection[a];
		if (connection->registered && (connection->memberId == memberId))
		{
			return a;
		}
	}

	return CryMatchMakingInvalidConnectionID;
}


void CCryPSNMatchMaking::RemoveRemote(const CrySessionHandle sessionHandle, const SRoomMember* member)
{
	if (member && ((member->m_valid & ePSNMI_Me) == 0))
	{
		if (sessionHandle != CrySessionInvalidHandle)
		{
			SSession* session = &m_sessions[sessionHandle];

			if (session->used)
			{
				// Remove the player (if we have connection information we will need to drop that here too)
				CryMatchMakingConnectionID id = FindConnectionIDFromMemberID(session,member->m_memberId);

				if (id != CryMatchMakingInvalidConnectionID)
				{
					SSession::SRConnection* connection = &session->remoteConnection[id];

					connection->registered=false;

					CCryMatchMaking::FreeRemoteConnection(sessionHandle, id);
				}
			}
		}
	}

}

void CCryPSNMatchMaking::AddRemote(const CrySessionHandle sessionHandle, const SRoomMember* member)
{
	if (member && ((member->m_valid & ePSNMI_Me) == 0))
	{
		if (sessionHandle != CrySessionInvalidHandle)
		{
			SSession* session = &m_sessions[sessionHandle];

			if (session->used)
			{
				// We don't have connection information yet (ip / port etc)
				CryMatchMakingConnectionID id = CCryMatchMaking::AddRemoteConnection(sessionHandle, CryLobbyInvalidConnectionID, member->m_memberId, 1);

				if (id != CryMatchMakingInvalidConnectionID)
				{
					SSession::SRConnection* connection = &session->remoteConnection[id];

					connection->registered = true;
					connection->memberId = member->m_memberId;
				}
			}
		}
	}
}
					
void CCryPSNMatchMaking::ConfigureRemote(const CrySessionHandle sessionHandle, const SRoomMember* pMember)
{
	SSession* session = &m_sessions[sessionHandle];

	if (pMember && session->used)
	{
		CryMatchMakingConnectionID id = FindConnectionIDFromMemberID(session, pMember->m_memberId);

		if (id != CryMatchMakingInvalidConnectionID)
		{
			SSession::SRConnection* connection = &session->remoteConnection[id];

			if (pMember->m_conState == ePSNCS_Dead)
			{
				m_lobby->ReleaseConnection(connection->connectionID);
			}
			if (pMember->m_conState == ePSNCS_Active)
			{
				TNetAddress netAddr = TNetAddress(SIPv4Addr(pMember->m_peerAddr.s_addr, pMember->m_peerPort));

				// Add connection - if not found
				if (m_lobby->FindConnection(netAddr) == CryLobbyInvalidConnectionID)
				{
					CryLobbyConnectionID lID = m_lobby->CreateConnection(netAddr);
					m_lobby->KeepConnection(lID);
					connection->connectionID=lID;
				}
			}
		}
	}
}

void CCryPSNMatchMaking::SupportCallback(ECallbackEvent ecb, const SCallbackEventData &data, void *userArg)
{
	CCryPSNMatchMaking* _this = static_cast<CCryPSNMatchMaking*>(userArg);

	switch (ecb)
	{
	case eCE_ConnectionAdd:
		// Indicates a new member has joined the room (if its not ourself then we should add a remote connection reference)
		_this->AddRemote(data.m_sessionHandle, data.m_pMemberInfo);
		break;

	case eCE_ConnectionRemove:
		// Indicates a member has left the room (if its not ourself then we should remove the remote connection reference)
		_this->RemoveRemote(data.m_sessionHandle, data.m_pMemberInfo);
		break;

	case eCE_ConnectionOldOwner:		// Indicates old host has changed
	case eCE_ConnectionUpdate:
		// Indicates the peer-peer connection status has changed - usually indicates process started/completed/success/fail
		//We should CreateConnection or ReleaseConnection or do nothing depending on status returned here
		_this->ConfigureRemote(data.m_sessionHandle, data.m_pMemberInfo);
		break;

	case eCE_ConnectionNewOwner:
		// Indicates the new host of the session (after auto migrate) is known and this is that user.
		// We can use that flag to update our migration advertiser address and signal migration done
		_this->ConfigureRemote(data.m_sessionHandle, data.m_pMemberInfo);
#if HOST_MIGRATION
		_this->m_newHostAddressValid=true;
#endif
		//-- This is a bit of a fudge; the host migrator has no notion of multiple sessions, but it will need to know what session
		//-- is migrating later on to be able to find the new room owner.
		//-- Ideally it might be better to tie this into the matchmaker task (which has the session handle in it), and use the task to update 
		//-- the host migration. Both LAN and Live use a task (though still only with the notion of 1 session in the migration as far as I can tell).
		_this->m_hostMigrationSession = data.m_sessionHandle;
	}
}

ECryLobbyError CCryPSNMatchMaking::Initialise()
{
	SConfigurationParams	neededInfo[2] = {{'PCom',{NULL}},{'PPas',{NULL}}};

	ECryLobbyError error = CCryMatchMaking::Initialise();

	m_lobby->GetConfigurationInformation(neededInfo,2);

	if (error == eCLE_Success)
	{
		m_lobbySupport = new CCryPSNSupport((SceNpCommunicationId*)neededInfo[0].m_pData,(SceNpCommunicationPassphrase*)neededInfo[1].m_pData, SupportCallback, this, m_lobby);
		if (m_lobbySupport)
		{
			if (!m_lobbySupport->Initialise())
			{
				error = eCLE_OutOfMemory;
			}
		}
		else
		{
			error = eCLE_OutOfMemory;
		}
	
		m_registeredUserData.num = 0;
	}

	return error;
}

CCryPSNMatchMaking::CCryPSNMatchMaking(CCryLobby* lobby,CCryLobbyService* service) : CCryMatchMaking(lobby,service)
{
	m_lobbySupport = NULL;
	m_oldNatType=SCE_NP_SIGNALING_NETINFO_NAT_STATUS_UNKNOWN;
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

CCryPSNMatchMaking::~CCryPSNMatchMaking()
{
	if (m_lobbySupport) 
	{
		delete m_lobbySupport;
	}
}

ECryLobbyError CCryPSNMatchMaking::StartTask(ETask etask, bool startRunning, uint32 user, CryMatchMakingTaskID* mmTaskID, CryLobbyTaskID* lTaskID, CrySessionHandle h, void* cb, void* cbArg)
{
	CryMatchMakingTaskID tmpMMTaskID;
	CryMatchMakingTaskID* useMMTaskID = mmTaskID ? mmTaskID : &tmpMMTaskID;
	ECryLobbyError error = CCryMatchMaking::StartTask(etask, startRunning, useMMTaskID, lTaskID, h, cb, cbArg);

	if (error == eCLE_Success)
	{
		STask* task = &m_task[*useMMTaskID];

		task->returnTaskID = CryMatchMakingInvalidTaskID;
		task->user = user;
	}

	return error;
}

void CCryPSNMatchMaking::StartTaskRunning(CryMatchMakingTaskID mmTaskID)
{
	LOBBY_AUTO_LOCK;

	STask* task = &m_task[mmTaskID];

	task->running = true;

	switch (task->startedTask)
	{
	case eT_SessionSearch:
		StartSessionSearch(mmTaskID);
		break;
	case eT_SessionCreate:
		StartSessionCreate(mmTaskID);
		break;
	case eT_SessionMigrate:
		StartSessionMigrate(mmTaskID);
		break;
	case eT_SessionJoin:
		StartSessionJoin(mmTaskID);
		break;
	case eT_SessionDelete:
		StartSessionDelete(mmTaskID);
		break;

	case eT_SessionUpdate:
	case eT_SessionStart:
	case eT_SessionEnd:
	case eT_SessionRegisterUserData:
		StopTaskRunning(mmTaskID);
		break;
	}
}

void CCryPSNMatchMaking::EndTask(CryMatchMakingTaskID mmTaskID)
{
	LOBBY_AUTO_LOCK;

	STask* task = &m_task[mmTaskID];

	if (task->used)
	{
		if (m_lobbySupport->GetNetInfo().nat_status != m_oldNatType )
		{
			m_oldNatType = m_lobbySupport->GetNetInfo().nat_status;

			UCryLobbyEventData eventData;
			SCryLobbyNatTypeData natTypeData;

			eventData.pNatTypeData = &natTypeData;

			switch (m_oldNatType)
			{
			default:
			case SCE_NP_SIGNALING_NETINFO_NAT_STATUS_UNKNOWN:
				natTypeData.m_curState = eNT_Unknown;
				break;
			case SCE_NP_SIGNALING_NETINFO_NAT_STATUS_TYPE1:
				natTypeData.m_curState = eNT_Open;
				break;
			case SCE_NP_SIGNALING_NETINFO_NAT_STATUS_TYPE2:
				natTypeData.m_curState = eNT_Moderate;
				break;
			case SCE_NP_SIGNALING_NETINFO_NAT_STATUS_TYPE3:
				natTypeData.m_curState = eNT_Strict;
				break;
			}

			if (m_lobby)
				m_lobby->DispatchEvent(eCLSE_NatType, eventData);
		}
		if (task->cb)
		{
			switch (task->startedTask)
			{
			case eT_SessionRegisterUserData:
				((CryMatchmakingCallback)task->cb)(task->lTaskID, task->error, task->cbArg);
				break;

			case eT_SessionSearch:
				EndSessionSearch(mmTaskID);
				break;

			case eT_SessionMigrate:				// Migrate from the games point of view is the same as create
			case eT_SessionCreate:
				((CryMatchmakingSessionCreateCallback)task->cb)(task->lTaskID, task->error, CreateGameSessionHandle(task->session, m_sessions[task->session].localConnection.uid), task->cbArg);
				break;

			case eT_SessionJoin:
				EndSessionJoin(mmTaskID);
				break;

			case eT_SessionUpdate:
			case eT_SessionStart:
			case eT_SessionEnd:
			case eT_SessionDelete:
				((CryMatchmakingCallback)task->cb)(task->lTaskID, task->error, task->cbArg);
				break;
			}
		}

		CryLogAlways("[Lobby] EndTask %d (%d) error %d", task->startedTask, task->subTask, task->error);

		FreeTask(mmTaskID);
	}
}

void CCryPSNMatchMaking::StopTaskRunning(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];

	if (task->used)
	{
		task->running = false;
		TO_GAME(&CCryPSNMatchMaking::EndTask, this, mmTaskID);
	}
}

ECryLobbyError CCryPSNMatchMaking::CreateSessionHandle(CrySessionHandle* h, bool host, uint32* users, int numUsers)
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

void CCryPSNMatchMaking::Tick(CTimeValue tv)
{
	m_lobbySupport->Tick();

	if (m_lobby->MutexTryLock())
	{
		for (uint32 i = 0; i < MAX_MATCHMAKING_TASKS; i++)
		{
			STask* task = &m_task[i];

			if (task->used && task->running)
			{
				switch (task->subTask)
				{
				case eT_SessionRegisterUserData:
				case eT_SessionUpdate:
				case eT_SessionStart:
				case eT_SessionEnd:
					break;

				case eT_SessionSearch:
					TickSessionSearch(i);
					break;
				case eT_SessionCreate:
					TickSessionCreate(i);
					break;
				case eT_SessionMigrate:
					TickSessionMigrate(i);
					break;
				case eT_SessionJoin:
					TickSessionJoin(i);
					break;
				case eT_SessionDelete:
					TickSessionDelete(i);
					break;
				}
			}
		}

		m_lobby->MutexUnlock();
	}
}

ECryLobbyError CCryPSNMatchMaking::SessionRegisterUserData(SCrySessionUserData* data, uint32 numData, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	if (numData < MAX_MATCHMAKING_SESSION_USER_DATA)
	{
		CryMatchMakingTaskID mmTaskID;

		error = StartTask(eT_SessionRegisterUserData, false, 0, &mmTaskID, taskID, 0, (void *)cb, cbArg);

		if (error == eCLE_Success)
		{
			memcpy(m_registeredUserData.data, data, numData*sizeof(data[0]));
			m_registeredUserData.num = numData;

			uint32 searchableCount=0;
			uint32 binBucketOffset=0;
			for (uint32 a=0;a<numData;a++)
			{
				if (m_registeredUserData.data[a].m_type == eCSUDT_Int32 && searchableCount < SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_NUM)
				{
					m_registeredUserData.mapping[a].integerField=1;
					m_registeredUserData.mapping[a].fieldOffset=searchableCount;

					searchableCount++;
				}
				else
				{
					// Either we have run out of searchable integers or the data type is not 32bit
					m_registeredUserData.mapping[a].integerField=0;
					m_registeredUserData.mapping[a].fieldOffset=binBucketOffset;
					switch (m_registeredUserData.data[a].m_type)
					{
					case eCSUDT_Int64:
					case eCSUDT_Int64NoEndianSwap:
					case eCSUDT_Float64:
						binBucketOffset+=8;
						break;
					case eCSUDT_Int32:
					case eCSUDT_Float32:
						binBucketOffset+=4;
						break;
					case eCSUDT_Int16:
						binBucketOffset+=2;
						break;
					case eCSUDT_Int8:
						binBucketOffset+=1;
						break;
					default:
						error=eCLE_UserDataTypeMissMatch;
						return error;
					}
					if (binBucketOffset>SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_MAX_SIZE * SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_NUM)
					{
						error=eCLE_OutOfSessionUserData;
						return error;
					}
				}
			}

			FROM_GAME(&CCryPSNMatchMaking::StartTaskRunning, this, mmTaskID);
		}
	}
	else
	{
		error = eCLE_OutOfSessionUserData;
	}

	CryLogAlways("[Lobby] Start SessionRegisterUserData error %d", error);

	return error;
}

ECryLobbyError CCryPSNMatchMaking::SessionCreate(uint32* users, int numUsers, uint32 flags, SCrySessionData* data, CryLobbyTaskID* taskID, CryMatchmakingSessionCreateCallback cb, void* cbArg)
{
	LOBBY_AUTO_LOCK;

//	m_lobby->InternalSocketCreate(true);

	CrySessionHandle h;
	ECryLobbyError error = CreateSessionHandle(&h, true, users, numUsers);

	if (error == eCLE_Success)
	{
		CryMatchMakingTaskID mmTaskID;

		error = StartTask(eT_SessionCreate, false, users[0], &mmTaskID, taskID, h, (void*)cb, cbArg);

		if (error == eCLE_Success)
		{
			STask* task = &m_task[mmTaskID];
			SSession* session = &m_sessions[h];

			session->flags = SCE_NP_MATCHING2_ROOM_FLAG_ATTR_OWNER_AUTO_GRANT;			// Auto migrate

			if (flags&CRYSESSION_CREATE_FLAG_SEARCHABLE)
			{
				session->flags |= 0;		//TODO.. implement hidden rooms
			}

			if (data->m_ranked)
			{
				session->flags |= 0;		// TODO.. implement ranked rooms
				session->gameType = 0;
			}
			else
			{
				session->gameType = 0;
			}

			session->gameMode = 0;
			session->numPublicSlots = data->m_numPublicSlots;
			session->numPrivateSlots = data->m_numPrivateSlots;

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
					FROM_GAME(&CCryPSNMatchMaking::StartTaskRunning, this, mmTaskID);
				}
			}

			if (error != eCLE_Success)
			{
				FreeTask(mmTaskID); 
				m_sessions[h].used = false;
			}
		}
		else
		{
			m_sessions[h].used = false;
		}
	}

	CryLogAlways("[Lobby] Start SessionCreate error %d", error);

	return error;
}

// At present there are only 8 possible searchable integer values (so we need to be careful with which attributes
//use them.
// For now I have chucked all the data into the 2 binary attributes (256 bytes each). If I run out, i`ll panic later.

void CCryPSNMatchMaking::StartSessionCreate(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];
	SCrySessionData* data = (SCrySessionData*)m_lobby->MemGetPtr(task->params[0]);
	int ret;

	data->m_data = (SCrySessionUserData*)m_lobby->MemGetPtr(task->params[1]);

	CCryPSNIndividualSessionSupport* pSessionSupport = m_lobbySupport->GetSessionSupport(task->session);
	if (pSessionSupport)
	{
		CCryPSNIndividualSessionSupport::SCreateParamData& createParams = pSessionSupport->AccessCreateParamsData();

		memset(&createParams, 0, sizeof(CCryPSNIndividualSessionSupport::SCreateParamData));

		// Setup our advertiser data
		for (int a=0;a<SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_NUM;a++)
		{
			createParams.m_binAttrExternal[a].id = SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_1_ID + a;
			createParams.m_binAttrExternal[a].ptr = &createParams.m_binAttrExternalData[SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_MAX_SIZE*a];
			createParams.m_binAttrExternal[a].size = SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_MAX_SIZE;		// should really shrink these to avoid sending unused data
		}

		for (int a=0;a<SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_NUM;a++)
		{
			createParams.m_intAttrExternal[a].id = SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_1_ID + a;
		}
		// Build user data into psn format
		if (session->used)
		{
			for (uint32 i = 0; i < data->m_numData; i++)
			{
				uint32 j;

				for (j = 0; j < m_registeredUserData.num; j++)
				{
					if (data->m_data[i].m_id == m_registeredUserData.data[j].m_id)
					{
						if (data->m_data[i].m_type == m_registeredUserData.data[j].m_type)
						{
							if (m_registeredUserData.mapping[j].integerField)
							{
								createParams.m_intAttrExternal[m_registeredUserData.mapping[j].fieldOffset].num = data->m_data[i].m_int32;
							}
							else
							{
								void *dPtr;
								switch (data->m_data[i].m_type)
								{
									case eCSUDT_Int64:
									case eCSUDT_Int64NoEndianSwap:
									case eCSUDT_Float64:
										dPtr = (void*)&data->m_data[i].m_int64;
										memcpy(&createParams.m_binAttrExternalData[m_registeredUserData.mapping[j].fieldOffset],dPtr,sizeof(data->m_data[i].m_int64));
										break;
									case eCSUDT_Int32:
									case eCSUDT_Float32:
										dPtr = (void*)&data->m_data[i].m_int32;
										memcpy(&createParams.m_binAttrExternalData[m_registeredUserData.mapping[j].fieldOffset],dPtr,sizeof(data->m_data[i].m_int32));
										break;
								case eCSUDT_Int16:
										dPtr = (void*)&data->m_data[i].m_int16;
										memcpy(&createParams.m_binAttrExternalData[m_registeredUserData.mapping[j].fieldOffset],dPtr,sizeof(data->m_data[i].m_int16));
										break;
								case eCSUDT_Int8:
										dPtr = (void*)&data->m_data[i].m_int8;
										memcpy(&createParams.m_binAttrExternalData[m_registeredUserData.mapping[j].fieldOffset],dPtr,sizeof(data->m_data[i].m_int8));
										break;
								}
							}
						}
					}
				}
			}
		}
		createParams.m_createRequest.worldId = 0;		// will be filled in by PSNSupport
		createParams.m_createRequest.maxSlot = session->numPrivateSlots + session->numPublicSlots;
		createParams.m_createRequest.flagAttr = session->flags;
		createParams.m_createRequest.roomPassword = NULL;
		createParams.m_createRequest.groupConfig = NULL;
		createParams.m_createRequest.groupConfigNum = 0;
		createParams.m_createRequest.passwordSlotMask = NULL;
		createParams.m_createRequest.allowedUser = NULL;
		createParams.m_createRequest.allowedUserNum = 0;
		createParams.m_createRequest.blockedUser = NULL;
		createParams.m_createRequest.blockedUserNum = 0;
		createParams.m_createRequest.sigOptParam = NULL;
		createParams.m_createRequest.roomBinAttrInternal = NULL;
		createParams.m_createRequest.roomBinAttrInternalNum = 0;
		createParams.m_createRequest.roomBinAttrExternal = createParams.m_binAttrExternal;
		createParams.m_createRequest.roomBinAttrExternalNum = 2;
		createParams.m_createRequest.roomSearchableBinAttrExternal = NULL;
		createParams.m_createRequest.roomSearchableBinAttrExternalNum = 0;
		createParams.m_createRequest.roomSearchableIntAttrExternal = createParams.m_intAttrExternal;
		createParams.m_createRequest.roomSearchableIntAttrExternalNum = SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_NUM;
		createParams.m_createRequest.joinRoomGroupLabel = NULL;
		createParams.m_createRequest.roomMemberBinAttrInternal = NULL;
		createParams.m_createRequest.roomMemberBinAttrInternalNum = 0;
		createParams.m_createRequest.teamId = 0;
	}
	else
	{
		UpdateTaskError(mmTaskID, eCLE_InternalError);
	}

	if (task->error != eCLE_Success)
	{
		session->used = false;
		StopTaskRunning(mmTaskID);
	}
}
					
ECryLobbyError CCryPSNMatchMaking::MapSupportErrorToLobbyError(EPSNPendingError pendingError)
{
	switch (pendingError)
	{
	case ePSNPE_None:
		return eCLE_Success;								// Under normal circumstances this should not happen
	case ePSNPE_NPLibraryNotInitialised:
	case ePSNPE_NetLibraryNotInitialised:
		return eCLE_InternalError;
	case ePSNPE_InternetDisabled:
	case ePSNPE_NoPSNAccount:
	case ePSNPE_SignInCancelled:
		return eCLE_UserNotSignedIn;
	case ePSNPE_NotConnected:
	case ePSNPE_Offline:
	case ePSNPE_CableDisconnected:
		return eCLE_ConnectionFailed;
	case ePSNPE_OutOfMemory:
		return eCLE_OutOfMemory;
	case ePSNPE_ContextError:
	case ePSNPE_NoServersAvailable:
	case ePSNPE_NoWorldsAvailable:
	case ePSNPE_InternalError:
		return eCLE_InternalError;
	}

	CryLogAlways("[Lobby] Warning Unknown PSN Error - probably a new error code not in this mapping table (CCryPSNMatchMaking::MapSupportErrorToLobbyError");
	return eCLE_InternalError;
}

void CCryPSNMatchMaking::TickSessionCreate(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	CCryPSNIndividualSessionSupport* pSessionSupporter = m_lobbySupport->GetSessionSupport(task->session);

	if (pSessionSupporter)
	{
		if (pSessionSupporter->ReadyForTask())		// Are we in a state where we can kick off a task?
		{
			pSessionSupporter->StartTask(ePSNT_Create);
		}

		if (m_lobbySupport->ErrorPending())
		{
			pSessionSupporter->AbortTask(ePSNT_Create);
			
			UpdateTaskError(mmTaskID, MapSupportErrorToLobbyError(m_lobbySupport->GetPendingError()));
			m_lobbySupport->ClearPendingError();
			StopTaskRunning(mmTaskID);
		}

		if (pSessionSupporter->TaskComplete(ePSNT_Create))
		{
			SSession* session = &m_sessions[task->session];

			const SceNpMatching2CreateJoinRoomResponse* const response = pSessionSupporter->GetCreateResult();
			if (response)
			{
				session->m_roomId = response->roomDataInternal->roomId;
				session->m_lobbyId = response->roomDataInternal->lobbyId;
				session->m_worldId = response->roomDataInternal->worldId;
				session->m_serverId = response->roomDataInternal->serverId;
				session->localConnection.uid = pSessionSupporter->GetMyMemberId();
				
				CrySessionHandle h;	
				FindConnectionFromUID(pSessionSupporter->GetMyMemberId(), &h, &session->hostConnectionID);

				pSessionSupporter->ClearCreateResult();

#if USE_PSN_VOICE
				CCryPSNVoice* voice = (CCryPSNVoice*)m_pService->GetVoice();

				if (voice)
				{
					voice->StartVoice(m_lobbySupport->GetContext(),session->m_roomId);
				}
#endif//USE_PSN_VOICE

				UpdateTaskError(mmTaskID, eCLE_Success);
			}
			else
			{
				UpdateTaskError(mmTaskID, eCLE_InternalError);
			}
			StopTaskRunning(mmTaskID);
		}
	}
	else
	{
		UpdateTaskError(mmTaskID, eCLE_InternalError);
		StopTaskRunning(mmTaskID);
	}
}

ECryLobbyError CCryPSNMatchMaking::SessionMigrate(CrySessionHandle h,uint32* pUsers, int numUsers, uint32 flags, SCrySessionData* pData, CryLobbyTaskID* pTaskID, CryMatchmakingSessionCreateCallback pCB, void* pCBArg)
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
			FROM_GAME(&CCryPSNMatchMaking::StartTaskRunning, this, mmTaskID);
		}
	}
	else
	{
		error = eCLE_InvalidSession;
	}

	CryLogAlways("[Lobby] Start SessionMigrate error %d", error);

	return error;
}

void CCryPSNMatchMaking::StartSessionMigrate(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];

	if (task->error != eCLE_Success)
	{
		StopTaskRunning(mmTaskID);
	}
}

void CCryPSNMatchMaking::TickSessionMigrate(CryMatchMakingTaskID mmTaskID)
{
	UpdateTaskError(mmTaskID, eCLE_Success);
	StopTaskRunning(mmTaskID);
}

ECryLobbyError CCryPSNMatchMaking::SessionUpdate(CrySessionHandle h, SCrySessionUserData* data, uint32 numData, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	CryLogAlways("[Lobby] Session Update Is Not Implemented At Present %d", error);

	return error;
}

ECryLobbyError CCryPSNMatchMaking::SessionStart(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg)
{
	ECryLobbyError error = eCLE_Success;

	// Nothing to do on PSN

	return error;
}

ECryLobbyError CCryPSNMatchMaking::SessionEnd(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg)
{
	ECryLobbyError error = eCLE_Success;

	// Nothing to do on PSN

	return error;
}

ECryLobbyError CCryPSNMatchMaking::SessionDelete(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg)
{
	ECryLobbyError error = eCLE_Success;

	LOBBY_AUTO_LOCK;

	h = GetSessionHandleFromGameSessionHandle(h);

	if ((h < MAX_MATCHMAKING_SESSIONS) && m_sessions[h].used)
	{
		SSession* session = &m_sessions[h];
		CryMatchMakingTaskID mmTaskID;

		error = StartTask(eT_SessionDelete, false, session->localConnection.users[0], &mmTaskID, taskID, h, (void*)cb, cbArg);

		if (error == eCLE_Success)
		{
			FROM_GAME(&CCryPSNMatchMaking::StartTaskRunning, this, mmTaskID);
		}
	}
	else
	{
		error = eCLE_InvalidSession;
	}

	CryLogAlways("[Lobby] Start SessionDelete error %d", error);

	return error;
}

void CCryPSNMatchMaking::StartSessionDelete(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];
		
	CCryPSNIndividualSessionSupport* pSessionSupport = m_lobbySupport->GetSessionSupport(task->session);
	if (pSessionSupport)
	{
		SceNpMatching2LeaveRoomRequest& leaveParams = pSessionSupport->AccessLeaveParamsData();

		memset(&leaveParams,0,sizeof(SceNpMatching2LeaveRoomRequest));

		leaveParams.roomId = session->m_roomId;
		leaveParams.optData.len = 0;
				
#if USE_PSN_VOICE
		CCryPSNVoice* voice = (CCryPSNVoice*)m_pService->GetVoice();

		if (voice)
		{
			voice->StopVoice();
		}
#endif//USE_PSN_VOICE
	}
	else
	{
		UpdateTaskError(mmTaskID, eCLE_InternalError);
	}

	if (task->error != eCLE_Success)
	{
		StopTaskRunning(mmTaskID);
	}
}

void CCryPSNMatchMaking::TickSessionDelete(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];
	CCryPSNIndividualSessionSupport* pSessionSupporter = m_lobbySupport->GetSessionSupport(task->session);

	if (pSessionSupporter)
	{
		if (pSessionSupporter->ReadyForTask())		// Are we in a state where we can kick off a task?
		{
			pSessionSupporter->StartTask(ePSNT_Leave);
		}

		if (m_lobbySupport->ErrorPending())
		{
			session->used=false;

			pSessionSupporter->AbortTask(ePSNT_Leave);

			UpdateTaskError(mmTaskID, MapSupportErrorToLobbyError(m_lobbySupport->GetPendingError()));
			m_lobbySupport->ClearPendingError();
			StopTaskRunning(mmTaskID);
		}

		if (pSessionSupporter->TaskComplete(ePSNT_Leave))
		{
			session->used=false;

			pSessionSupporter->ClearJoinResult();			// Must be called to finalise the state machine
			UpdateTaskError(mmTaskID, eCLE_Success);
			StopTaskRunning(mmTaskID);
		}
	}
	else
	{
		UpdateTaskError(mmTaskID, eCLE_InternalError);
		StopTaskRunning(mmTaskID);
	}
}

ECryLobbyError CCryPSNMatchMaking::SessionSearch(uint32 user, SCrySessionSearchParam* param, CryLobbyTaskID* taskID, CryMatchmakingSessionSearchCallback cb, void* cbArg)
{
	LOBBY_AUTO_LOCK;

//	m_lobby->InternalSocketCreate(false);

	CryMatchMakingTaskID mmTaskID;

	ECryLobbyError error = StartTask(eT_SessionSearch, false, user, &mmTaskID, taskID, 0, (void*)cb, cbArg);

	if (error == eCLE_Success)
	{
		STask* task = &m_task[mmTaskID];

		error = CreateTaskParam(mmTaskID, 0, param, 1, sizeof(SCrySessionSearchParam));

		if (error == eCLE_Success)
		{
			error = CreateTaskParam(mmTaskID, 1, param->m_data, param->m_numData, param->m_numData*sizeof(param->m_data[0]));

			if (error == eCLE_Success)
			{
				FROM_GAME(&CCryPSNMatchMaking::StartTaskRunning, this, mmTaskID);
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

void CCryPSNMatchMaking::StartSessionSearch(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	CCryPSNIndividualSessionSupport* pSessionSupport = m_lobbySupport->GetSessionSupport(task->session);
	if (pSessionSupport)
	{
		CCryPSNIndividualSessionSupport::SSearchParamData& searchParams = pSessionSupport->AccessSearchParamsData();
		SCrySessionSearchParam* param = (SCrySessionSearchParam*)m_lobby->MemGetPtr(task->params[0]);

		// Build our search parameter specification up ready to give to the PSNSupport module
		//We don't actually set the call going here, thats sorted in the tick routine

		memset(&searchParams, 0, sizeof(CCryPSNIndividualSessionSupport::SSearchParamData));

		searchParams.m_searchRequest.worldId = 0;				// NB: World Id is set by PSNSupport module at present
		searchParams.m_searchRequest.lobbyId = 0;
		searchParams.m_searchRequest.flagFilter = 0;
		searchParams.m_searchRequest.flagAttr = 0;
		searchParams.m_searchRequest.intFilter = NULL;
		searchParams.m_searchRequest.intFilterNum = 0;
		searchParams.m_searchRequest.binFilter = NULL;
		searchParams.m_searchRequest.binFilterNum = 0;
		searchParams.m_searchRequest.rangeFilter.startIndex = 1;
		searchParams.m_searchRequest.rangeFilter.max = SCE_NP_MATCHING2_RANGE_FILTER_MAX;
		searchParams.m_searchRequest.option = SCE_NP_MATCHING2_SEARCH_ROOM_OPTION_WITH_NPID;

		searchParams.m_attrId[0] = SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_1_ID;
		searchParams.m_attrId[1] = SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_2_ID;
		searchParams.m_attrId[2] = SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_3_ID;
		searchParams.m_attrId[3] = SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_4_ID;
		searchParams.m_attrId[4] = SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_5_ID;
		searchParams.m_attrId[5] = SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_6_ID;
		searchParams.m_attrId[6] = SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_7_ID;
		searchParams.m_attrId[7] = SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_8_ID;
		searchParams.m_attrId[8] = SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_1_ID;
		searchParams.m_attrId[9] = SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_2_ID;

		searchParams.m_searchRequest.attrId = searchParams.m_attrId;
		searchParams.m_searchRequest.attrIdNum = 10;

		m_lobby->MemFree(task->params[0]);
		m_lobby->MemFree(task->params[1]);
		task->params[0] = TMemInvalidHdl;
		task->params[1] = TMemInvalidHdl;
	}
	else
	{
		UpdateTaskError(mmTaskID, eCLE_InternalError);
	}

	if (task->error != eCLE_Success)
	{
		StopTaskRunning(mmTaskID);
	}
}

void CCryPSNMatchMaking::TickSessionSearch(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];
	CCryPSNIndividualSessionSupport* pSessionSupporter = m_lobbySupport->GetSessionSupport(task->session);

	if (pSessionSupporter)
	{
		if (pSessionSupporter->ReadyForTask())		// Are we in a state where we can kick off a task?
		{
			pSessionSupporter->StartTask(ePSNT_Search);
		}

		if (m_lobbySupport->ErrorPending())
		{
			pSessionSupporter->AbortTask(ePSNT_Search);

			UpdateTaskError(mmTaskID,MapSupportErrorToLobbyError(m_lobbySupport->GetPendingError()));
			m_lobbySupport->ClearPendingError();
			StopTaskRunning(mmTaskID);
		}
		if (pSessionSupporter->TaskComplete(ePSNT_Search))
		{
			UpdateTaskError(mmTaskID, eCLE_Success);
			StopTaskRunning(mmTaskID);
		}
	}
	else
	{
		UpdateTaskError(mmTaskID, eCLE_InternalError);
		StopTaskRunning(mmTaskID);
	}
}

void CCryPSNMatchMaking::EndSessionSearch(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];

	if (task->error == eCLE_Success)
	{
		CCryPSNIndividualSessionSupport* pSessionSupporter = m_lobbySupport->GetSessionSupport(task->session);
		if (pSessionSupporter)
		{
			const SceNpMatching2SearchRoomResponse* const response = pSessionSupporter->GetSearchResult();
			if (response)
			{
				CryLogAlways("[Lobby] Found %d sessions", response->range.total);

				SceNpMatching2RoomDataExternal* curRoom = response->roomDataExternal;

				while (curRoom)
				{
					SCrySessionSearchResult result;
					SCrySessionUserData userData[MAX_MATCHMAKING_SESSION_USER_DATA];
					SCryPSNSessionID *roomInfo=new SCryPSNSessionID;

					roomInfo->m_roomId=curRoom->roomId;
					roomInfo->m_lobbyId=curRoom->lobbyId;
					roomInfo->m_worldId=curRoom->worldId;
					roomInfo->m_serverId=curRoom->serverId;
					roomInfo->m_fromInvite=false;
					roomInfo->m_gameType=0;

					result.m_id = roomInfo;

					result.m_numFilledSlots = curRoom->curMemberNum; //liveResult->dwFilledPrivateSlots + liveResult->dwFilledPublicSlots;

					result.m_data.m_ranked = false;
					result.m_data.m_name[0] = 0;
					result.m_data.m_data = userData;
					result.m_data.m_numData = 0;

					result.m_data.m_numPublicSlots = curRoom->maxSlot;
					result.m_data.m_numPrivateSlots = 0;

					for (uint32 k = 0; k < 16; k++)
					{
						result.m_data.m_name[k] = (char)curRoom->owner->npId.handle.data[k];
					}

					for (int a=0;a<curRoom->roomSearchableIntAttrExternalNum;a++)
					{
						for (int b=0;b<m_registeredUserData.num;b++)
						{
							if (m_registeredUserData.mapping[b].integerField)
							{
								if (curRoom->roomSearchableIntAttrExternal[a].id == m_registeredUserData.mapping[b].fieldOffset + SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_1_ID)
								{
									result.m_data.m_data[result.m_data.m_numData].m_id = m_registeredUserData.data[b].m_id;
									result.m_data.m_data[result.m_data.m_numData].m_type = m_registeredUserData.data[b].m_type;
									result.m_data.m_data[result.m_data.m_numData].m_int32 = curRoom->roomSearchableIntAttrExternal[a].num;

									result.m_data.m_numData++;
									break;
								}
							}
						}
					}

					char binAttrExternalData[SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_MAX_SIZE * SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_NUM];
					for (int a=0;a<curRoom->roomBinAttrExternalNum;a++)
					{
						if (curRoom->roomBinAttrExternal[a].ptr)
						{
							memcpy(&binAttrExternalData[SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_MAX_SIZE * a],curRoom->roomBinAttrExternal[a].ptr,curRoom->roomBinAttrExternal[a].size);
						}
					}

					for (int b=0;b<m_registeredUserData.num;b++)
					{
						if (!m_registeredUserData.mapping[b].integerField)
						{
							result.m_data.m_data[result.m_data.m_numData].m_id = m_registeredUserData.data[b].m_id;
							result.m_data.m_data[result.m_data.m_numData].m_type = m_registeredUserData.data[b].m_type;

							void *dPtr;
							switch (result.m_data.m_data[result.m_data.m_numData].m_type)
							{
							case eCSUDT_Int64:
							case eCSUDT_Int64NoEndianSwap:
							case eCSUDT_Float64:
								dPtr = (void*)&result.m_data.m_data[result.m_data.m_numData].m_int64;
								memcpy(dPtr,&binAttrExternalData[m_registeredUserData.mapping[b].fieldOffset],sizeof(result.m_data.m_data[result.m_data.m_numData].m_int64));
								break;
							case eCSUDT_Int32:
							case eCSUDT_Float32:
								dPtr = (void*)&result.m_data.m_data[result.m_data.m_numData].m_int32;
								memcpy(dPtr,&binAttrExternalData[m_registeredUserData.mapping[b].fieldOffset],sizeof(result.m_data.m_data[result.m_data.m_numData].m_int32));
								break;
							case eCSUDT_Int16:
								dPtr = (void*)&result.m_data.m_data[result.m_data.m_numData].m_int16;
								memcpy(dPtr,&binAttrExternalData[m_registeredUserData.mapping[b].fieldOffset],sizeof(result.m_data.m_data[result.m_data.m_numData].m_int16));
								break;
							case eCSUDT_Int8:
								dPtr = (void*)&result.m_data.m_data[result.m_data.m_numData].m_int8;
								memcpy(dPtr,&binAttrExternalData[m_registeredUserData.mapping[b].fieldOffset],sizeof(result.m_data.m_data[result.m_data.m_numData].m_int8));
								break;
							}
							result.m_data.m_numData++;
						}
					}

					((CryMatchmakingSessionSearchCallback)task->cb)(task->lTaskID, eCLE_SuccessContinue, &result, task->cbArg);

					curRoom=curRoom->next;
				}
				
				pSessionSupporter->ClearSearchResult();
			}
			else
			{
				UpdateTaskError(mmTaskID,eCLE_InternalError);
			}
		}
		else
		{
			UpdateTaskError(mmTaskID,eCLE_InternalError);
		}
	}

	((CryMatchmakingSessionSearchCallback)task->cb)(task->lTaskID, task->error, NULL, task->cbArg);
}

ECryLobbyError CCryPSNMatchMaking::SessionJoin(uint32* users, int numUsers, CrySessionID id, CryLobbyTaskID* taskID, CryMatchmakingSessionJoinCallback cb, void* cbArg)
{
	LOBBY_AUTO_LOCK;

	CrySessionHandle h;
	ECryLobbyError error = CreateSessionHandle(&h, false, users, numUsers);

	if (error == eCLE_Success)
	{
		CryMatchMakingTaskID mmTaskID;

		error = StartTask(eT_SessionJoin, false, users[0], &mmTaskID, taskID, h, (void*)cb, cbArg);

		if (error == eCLE_Success)
		{
			SCryPSNSessionID* psnID = (SCryPSNSessionID*)id.get();
			SSession* session = &m_sessions[h];

			session->m_roomId = psnID->m_roomId;
			session->m_lobbyId = psnID->m_lobbyId;
			session->m_worldId = psnID->m_worldId;
			session->m_serverId = psnID->m_serverId;
			session->gameType = psnID->m_gameType;
			session->flags = 0;

			for (uint32 i = 0; i < numUsers; i++)
			{
				session->localConnection.privateSlot[i] = psnID->m_fromInvite;
			}

			FROM_GAME(&CCryPSNMatchMaking::StartTaskRunning, this, mmTaskID);
		}
		else
		{
			m_sessions[h].used = false;
		}
	}

	CryLogAlways("[Lobby] Start SessionJoin error %d", error);

	return error;
}

void CCryPSNMatchMaking::StartSessionJoin(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	SSession* session = &m_sessions[task->session];
	
	CCryPSNIndividualSessionSupport* pSessionSupport = m_lobbySupport->GetSessionSupport(task->session);
	if (pSessionSupport)
	{
		SceNpMatching2JoinRoomRequest& joinParams = pSessionSupport->AccessJoinParamsData();
	
		memset(&joinParams, 0, sizeof(SceNpMatching2JoinRoomRequest));

		joinParams.roomId = session->m_roomId;
		joinParams.roomPassword = NULL;
		joinParams.joinRoomGroupLabel = NULL;
		joinParams.roomMemberBinAttrInternal = NULL;
		joinParams.roomMemberBinAttrInternalNum = 0;
		joinParams.optData.len = 0;
	}
	else
	{
		UpdateTaskError(mmTaskID, eCLE_InternalError);
	}

	if (task->error != eCLE_Success)
	{
		session->used = false;
		StopTaskRunning(mmTaskID);
	}
}

void CCryPSNMatchMaking::TickSessionJoin(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];
	CCryPSNIndividualSessionSupport* pSessionSupporter = m_lobbySupport->GetSessionSupport(task->session);

	if (pSessionSupporter)
	{
		if (pSessionSupporter->ReadyForTask())		// Are we in a state where we can kick off a task?
		{
			pSessionSupporter->StartTask(ePSNT_Join);
		}
		
		if (m_lobbySupport->ErrorPending())
		{
			pSessionSupporter->AbortTask(ePSNT_Join);

			UpdateTaskError(mmTaskID,MapSupportErrorToLobbyError(m_lobbySupport->GetPendingError()));
			m_lobbySupport->ClearPendingError();
			StopTaskRunning(mmTaskID);
		}

		if (pSessionSupporter->TaskComplete(ePSNT_Join))
		{
			UpdateTaskError(mmTaskID, eCLE_Success);
			StopTaskRunning(mmTaskID);
		}
	}
	else
	{
		UpdateTaskError(mmTaskID, eCLE_InternalError);
		StopTaskRunning(mmTaskID);
	}
}

void CCryPSNMatchMaking::EndSessionJoin(CryMatchMakingTaskID mmTaskID)
{
	STask* task = &m_task[mmTaskID];

	if (task->error == eCLE_Success)
	{
		CCryPSNIndividualSessionSupport* pSessionSupport = m_lobbySupport->GetSessionSupport(task->session);
		if (pSessionSupport)
		{
			const SceNpMatching2JoinRoomResponse* const response = pSessionSupport->GetJoinResult();
			if (response)
			{
				SSession* session = &m_sessions[task->session];
				uint32 myIP, myPort;
				const SRoomMember* const member = pSessionSupport->GetMemberFromRoomMemberId(pSessionSupport->GetOwnerMemberId());

				if (member && member->m_conState == ePSNCS_Active)
				{
					myIP = member->m_peerAddr.s_addr;
					myPort = member->m_peerPort;
				
					session->localConnection.uid = pSessionSupport->GetMyMemberId();
				CrySessionHandle h;	
				FindConnectionFromUID(pSessionSupport->GetOwnerMemberId(), &h, &session->hostConnectionID);
				
					((CryMatchmakingSessionJoinCallback)task->cb)(task->lTaskID, task->error, CreateGameSessionHandle(task->session, session->localConnection.uid), myIP, myPort, task->cbArg);

#if USE_PSN_VOICE
					CCryPSNVoice* voice = (CCryPSNVoice*)m_pService->GetVoice();

					if (voice)
					{
						voice->StartVoice(m_lobbySupport->GetContext(),session->m_roomId);
					}
#endif//USE_PSN_VOICE

					pSessionSupport->ClearJoinResult();
	
					return;
				}
			}
		}

		UpdateTaskError(mmTaskID,eCLE_InternalError);
	}

	((CryMatchmakingSessionJoinCallback)task->cb)(task->lTaskID, task->error, task->session, 0, 0, task->cbArg);
}

void CCryPSNMatchMaking::SessionDisconnectRemoteConnection(CrySessionHandle gh, const TNetAddress& addr)
{
	// At present i don't need to worry about this
}

void CCryPSNMatchMaking::OnPacket(const TNetAddress& addr, const uint8* data, uint32 length)
{
	// At present i don't need to send data prior to game session 
}

#if NETWORK_HOST_MIGRATION
void CCryPSNMatchMaking::HostMigrationInitiate()
{
// Lets try a safety sleep : I have a feeling that client could recieve updated host before host has finished transition
//Which would mean the client tries to connect to his port before the server has time to open one.
	m_hostMigrationFinished = true;
	m_hostMigrationSafetySleep = 0;
	m_hostMigrationSession = CrySessionInvalidHandle;
}

bool CCryPSNMatchMaking::GetNewHostAddress(char* address)
{
	bool success = false;
	if (m_newHostAddressValid)
	{
		uint32 myIP=0;
		uint16 myPort=0;

		//-- This is not very multi-session friendly
		CCryPSNIndividualSessionSupport* pSessionSupport = m_lobbySupport->GetSessionSupport(m_hostMigrationSession);
		if (pSessionSupport)
		{
			const SRoomMember* const member = pSessionSupport->GetMemberFromRoomMemberId(pSessionSupport->GetOwnerMemberId());

			CryLogAlways("CCryPSNMatchMaking::GetNewHostAddress: Received %s as connection address", address);
			if (member)
			{
				if (member->m_conState==ePSNCS_Active)
				{
					myIP = member->m_peerAddr.s_addr;
					myPort = member->m_peerPort;
				}
				if ((member->m_valid & ePSNMI_Me)==0)			// Make sure we are not the new host before updating the connection information
				{
					// We are trying to join a new host - We should wait a bit before actually doing so to give host time to 
					//redo his connections.

					if (m_hostMigrationSafetySleep < HOST_MIGRATION_SAFETY_SLEEP)
					{
						CryLogAlways("Safety Migration Sleep");
						m_hostMigrationSafetySleep++;
						return false;
					}
					CNetwork::DecodeAddress(myIP,myPort,address,false);
				}
				else
				{
					// We are the new host, we should return quickly - bodge temporary
					strcat(address,":"SERVER_DEFAULT_PORT_STRING);
				}
			}
			CryLogAlways("CCryPSNMatchMaking::GetNewHostAddress: Using %s as connection address", address);

			m_newHostAddressValid = false;

			success = true;
		}
	}
	return success;
}
#endif

#endif // USE_PSN
