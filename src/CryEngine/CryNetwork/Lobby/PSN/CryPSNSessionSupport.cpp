
#include "StdAfx.h"

#include "CryPSNLobby.h"
#include "CryPSNSessionSupport.h"

#if USE_PSN


#define SIGNALING_TIMEOUT (10 * 1000)			// 10 seconds using our own timeout code


CCryPSNIndividualSessionSupport::CCryPSNIndividualSessionSupport(CrySessionHandle handle, CCryPSNSupport *pMainSupport, CryLobbySupportCallback cb, void *cbArg)
{
	m_sessionHandle = handle;
	m_pPSNSupport = pMainSupport;

	m_pSupportCb = cb;
	m_pSupportArg = cbArg;

	m_searchResponseAllocHandle = TMemInvalidHdl;
	m_createResponseAllocHandle = TMemInvalidHdl;
	m_joinResponseAllocHandle = TMemInvalidHdl;

	Clean();
}

CCryPSNIndividualSessionSupport::~CCryPSNIndividualSessionSupport()
{
	Clean();
}

void CCryPSNIndividualSessionSupport::Clean()
{
	ClearMemberInfo();
	ClearCreateResult();
	ClearJoinResult();
	ClearLeaveResult();
	ClearSearchResult();

	m_curTask = ePSNT_None;
	m_curState = ePSNIIS_NothingInProgress;
	m_wantedState = ePSNIIS_NothingInProgress;
}

void CCryPSNIndividualSessionSupport::Tick()
{
	UpdateSignaling();

	switch (m_curState)
	{
		case ePSNIIS_NothingInProgress:
		case ePSNIIS_WaitingForSearchRoomCallback:
		case ePSNIIS_RoomSearchResultsAvailable:
		case ePSNIIS_WaitingForCreateRoomCallback:
		case ePSNIIS_RoomCreated:
		case ePSNIIS_WaitingForJoinRoomCallback:
		case ePSNIIS_RoomJoined:
		case ePSNIIS_WaitingForLeaveRoomCallback:
		case ePSNIIS_RoomLeft:
		default:
		break;

		case ePSNIIS_StartSearchRoom:
		{
			TransitionToSearching();
		}
		break;

		case ePSNIIS_StartCreateRoom:
		{
			TransitionToCreating();
		}
		break;

		case ePSNIIS_WaitingForCreateRoomSignaling:
		{
			switch (CheckSignaling())
			{
				case ePSNCS_None:
				{
					//-- This is bad, means there is no room owner!
					m_pPSNSupport->FlagPendingError(ePSNPE_InternalError, ePSNOS_ReadyToEnumerateServers);
				}
				break;

				case ePSNCS_Active:
				{
					//-- Ok to continue state machine
					m_curState = ePSNIIS_RoomCreated;
				}
				break;

				case ePSNCS_Dead:
				{
					//-- signaling failed to connect p2p to server, cannot join!
					m_pPSNSupport->FlagPendingError(ePSNPE_InternalError, ePSNOS_ReadyToEnumerateServers);
				}
				break;

				case ePSNCS_Pending:
				default:
				break;
			}
		}
		break;

		case ePSNIIS_StartJoinRoom:
		{
			TransitionToJoining();
		}
		break;

		case ePSNIIS_WaitingForJoinRoomSignaling:
		{
			switch (CheckSignaling())
			{
				case ePSNCS_None:
				{
					//-- This is bad, means there is no room owner!
					m_pPSNSupport->FlagPendingError(ePSNPE_InternalError, ePSNOS_ReadyToEnumerateServers);
				}
				break;

				case ePSNCS_Active:
				{
					//-- Ok to continue state machine
					m_curState = ePSNIIS_RoomJoined;
				}
				break;

				case ePSNCS_Dead:
				{
					//-- signaling failed to connect p2p to server, cannot join!
					m_pPSNSupport->FlagPendingError(ePSNPE_InternalError, ePSNOS_ReadyToEnumerateServers);
				}
				break;

				case ePSNCS_Pending:
				default:
				break;
			}
		}
		break;

		case ePSNIIS_StartLeaveRoom:
		{
			TransitionToLeaving();
		}
		break;
	}
}

bool CCryPSNIndividualSessionSupport::HandleRequestEvent(SceNpMatching2ContextId id, SceNpMatching2RequestId reqId, SceNpMatching2Event myEvent, SceNpMatching2EventKey eventKey, size_t dataSize)
{
	if (reqId == m_reqId)
	{
		switch (myEvent)
		{
			default:
			{
				//-- Ignore unknown events - might be worth dumping warning about them though

				//-- If datasize > 0 we must always get the data, otherwise the internal stack will overflow (apparently)
				//-- Get data and ditch it immediately.
				if (dataSize > 0)
				{
					TMemHdl h = m_pPSNSupport->GetLobby()->MemAlloc(dataSize);
					if (h == TMemInvalidHdl)
					{
						CryLogAlways("Failed to allocate memory for buffer");
						//-- Kick the state machine to offline state
						m_pPSNSupport->FlagPendingError(ePSNPE_OutOfMemory, ePSNOS_Offline);
						return true;
					}

					void* pEventData = m_pPSNSupport->GetLobby()->MemGetPtr(h);
					memset(pEventData, 0, dataSize);

					int ret = sceNpMatching2GetEventData(id, eventKey, pEventData, dataSize);
					if (ret < 0) 
					{
						CryLogAlways("sceNpMatching2GetEventData : Error %08X", ret);
						m_pPSNSupport->GetLobby()->MemFree(h);
						m_pPSNSupport->HandlePSNError(ret);
						return true;
					}

					m_pPSNSupport->GetLobby()->MemFree(h);
				}
			}
			break;

			case SCE_NP_MATCHING2_REQUEST_EVENT_SearchRoom:
			{
				if (dataSize > 0)
				{
					assert(m_searchResponseAllocHandle == TMemInvalidHdl);

					m_searchResponseAllocHandle = m_pPSNSupport->GetLobby()->MemAlloc(dataSize);
					if (m_searchResponseAllocHandle == TMemInvalidHdl)
					{
						CryLogAlways("Failed to allocate memory for buffer");
						m_pPSNSupport->FlagPendingError(ePSNPE_OutOfMemory, ePSNOS_Offline);
						return true;
					}

					SceNpMatching2SearchRoomResponse* pSearchResponse = (SceNpMatching2SearchRoomResponse*)m_pPSNSupport->GetLobby()->MemGetPtr(m_searchResponseAllocHandle);
					memset(pSearchResponse, 0, dataSize);

					int ret = sceNpMatching2GetEventData(m_pPSNSupport->GetContext(), eventKey, pSearchResponse, dataSize);
					if (ret < 0) 
					{
						CryLogAlways("sceNpMatching2GetEventData : Error %08X", ret);
						m_pPSNSupport->HandlePSNError(ret);
						return true;
					}

					m_curState = ePSNIIS_RoomSearchResultsAvailable;
				}
			}
			break;

			case SCE_NP_MATCHING2_REQUEST_EVENT_CreateJoinRoom:
			{
				if (dataSize > 0)
				{
					assert(m_createResponseAllocHandle == TMemInvalidHdl);

					m_createResponseAllocHandle = m_pPSNSupport->GetLobby()->MemAlloc(dataSize);
					if (m_createResponseAllocHandle == TMemInvalidHdl)
					{
						CryLogAlways("Failed to allocate memory for buffer");
						m_pPSNSupport->FlagPendingError(ePSNPE_OutOfMemory, ePSNOS_Offline);
						return true;
					}

					SceNpMatching2CreateJoinRoomResponse* pCreateResponse = (SceNpMatching2CreateJoinRoomResponse*)m_pPSNSupport->GetLobby()->MemGetPtr(m_createResponseAllocHandle);
					memset(pCreateResponse, 0, dataSize);

					int ret = sceNpMatching2GetEventData(m_pPSNSupport->GetContext(), eventKey, pCreateResponse, dataSize);
					if (ret < 0) 
					{
						CryLogAlways("sceNpMatching2GetEventData : Error %08X",ret);
						m_pPSNSupport->HandlePSNError(ret);
						return true;
					}

					m_roomId = pCreateResponse->roomDataInternal->roomId;
					AddInitialRoomMembers(&pCreateResponse->roomDataInternal->memberList);

					m_curState = ePSNIIS_WaitingForCreateRoomSignaling;
				}
			}
			break;

			case SCE_NP_MATCHING2_REQUEST_EVENT_JoinRoom:
			{
				if (dataSize > 0)
				{
					assert(m_joinResponseAllocHandle == TMemInvalidHdl);

					m_joinResponseAllocHandle = m_pPSNSupport->GetLobby()->MemAlloc(dataSize);
					if (m_joinResponseAllocHandle == TMemInvalidHdl)
					{
						CryLogAlways("Failed to allocate memory for buffer");
						m_pPSNSupport->FlagPendingError(ePSNPE_OutOfMemory, ePSNOS_Offline);
						return true;
					}

					SceNpMatching2JoinRoomResponse* pJoinResponse = (SceNpMatching2JoinRoomResponse*)m_pPSNSupport->GetLobby()->MemGetPtr(m_joinResponseAllocHandle);
					memset(pJoinResponse, 0, dataSize);

					int ret = sceNpMatching2GetEventData(m_pPSNSupport->GetContext(), eventKey, pJoinResponse, dataSize);
					if (ret < 0) 
					{
						CryLogAlways("sceNpMatching2GetEventData : Error %08X",ret);
						m_pPSNSupport->HandlePSNError(ret);
						return true;
					}

					m_roomId = pJoinResponse->roomDataInternal->roomId;
					AddInitialRoomMembers(&pJoinResponse->roomDataInternal->memberList);

					m_curState = ePSNIIS_WaitingForJoinRoomSignaling;
				}
			}
			break;

			case SCE_NP_MATCHING2_REQUEST_EVENT_LeaveRoom:
			{
				RemoveAllRoomMembers();

				m_roomId = 0;
				m_curState = ePSNIIS_RoomLeft;
			}
			break;		
		}

		return true;
	}

	return false;
}
		
const SRoomMember* const CCryPSNIndividualSessionSupport::GetMemberFromRoomMemberId(SceNpMatching2RoomMemberId memberId)
{
	uint32 a;

	for (a = 0; a < MAX_PSN_ROOM_MEMBERS; a++)
	{
		if ((m_members[a].m_valid != ePSNMI_None) && (m_members[a].m_memberId == memberId))
		{
			return &m_members[a];
		}
	}

	return NULL;
}

SRoomMember* const CCryPSNIndividualSessionSupport::GetMemberFromConnectionId(uint32 conID)
{
	uint32 a;

	for (a = 0; a < MAX_PSN_ROOM_MEMBERS; a++)
	{
		if ((m_members[a].m_valid != ePSNMI_None) && (m_members[a].m_conId == conID))
		{
			return &m_members[a];
		}
	}

	return NULL;
}

void CCryPSNIndividualSessionSupport::AddInitialRoomMembers(const SceNpMatching2RoomMemberDataInternalList* pMemberList)
{
	if (pMemberList == NULL)
	{
		return;
	}

	uint32 a;
	SceNpMatching2RoomMemberDataInternal *cMember = pMemberList->members;

	for (a = 0; a < pMemberList->membersNum; a++)
	{
		EPSNMemberInfo info = ePSNMI_None;

		if (cMember == pMemberList->me)
		{
			if (cMember == pMemberList->owner)
			{
				info = ePSNMI_MeOwner;
			}
			else
			{
				info = ePSNMI_Me;
			}
		}
		else
		{
			if (cMember == pMemberList->owner)
			{
				info = ePSNMI_OtherOwner;
			}
			else
			{
				info = ePSNMI_Other;
			}
		}

		AddRoomMember(cMember, info);

		cMember = cMember->next;
	}
}

void CCryPSNIndividualSessionSupport::AddRoomMember(const SceNpMatching2RoomMemberDataInternal* pMemberInfo, EPSNMemberInfo info)
{
	if (pMemberInfo == NULL)
	{
		return;
	}

	SRoomMember* pMember = NULL;

	for (uint32 a = 0; a < MAX_PSN_ROOM_MEMBERS; a++)
	{
		if (m_members[a].m_valid != ePSNMI_None)
		{
			if (m_members[a].m_memberId == pMemberInfo->memberId)
			{
				//-- already exists!
				CRY_ASSERT_MESSAGE(m_members[a].m_memberId != pMemberInfo->memberId, "Adding a room member who already exists!");
				return;
			}
		}
		else
		{
			if (pMember == NULL)
			{
				pMember = &m_members[a];
			}
		}
	}

	if (pMember)
	{
		pMember->m_npId = pMemberInfo->userInfo.npId;
		pMember->m_memberId = pMemberInfo->memberId;
		pMember->m_valid = info;
		pMember->m_conId = 0;
		pMember->m_conState = ePSNCS_None;

		if (pMember->m_valid & ePSNMI_Me)
		{
			m_myMemberId = pMember->m_memberId;
		}
		if (pMember->m_valid & ePSNMI_Owner)
		{
			m_ownerMemberId = pMember->m_memberId;
		}

		if (m_pSupportCb)			// On add call after we modify data
		{
			SCallbackEventData evd;
			evd.m_sessionHandle = m_sessionHandle;
			evd.m_pMemberInfo = pMember;
			m_pSupportCb(eCE_ConnectionAdd, evd, m_pSupportArg);
		}

		if (!(pMember->m_valid & ePSNMI_Me))
		{
			//-- Not me, and no peer state defined yet, so lets kick off a peer connection test for this member
			int ret = sceNpSignalingActivateConnection(m_pPSNSupport->GetSignalingContext(), &pMember->m_npId, &pMember->m_conId);
			if (ret < 0)
			{
				CryLogAlways ("sceNpSignalingActivateConnection() : Error %08X", ret);
				//-- Cannot communicate with this member
				pMember->m_conState = ePSNCS_Dead;

				if (m_pSupportCb)
				{
					SCallbackEventData evd;
					evd.m_sessionHandle = m_sessionHandle;
					evd.m_pMemberInfo = pMember;
					m_pSupportCb(eCE_ConnectionUpdate, evd, m_pSupportArg);
				}

				m_pPSNSupport->HandlePSNError(ret);
			}
			else
			{
				//-- wait for peer test result to come back
				pMember->m_conState = ePSNCS_Pending;
				pMember->m_conTimer = g_time;

				if (m_pSupportCb)
				{
					SCallbackEventData evd;
					evd.m_sessionHandle = m_sessionHandle;
					evd.m_pMemberInfo = pMember;
					m_pSupportCb(eCE_ConnectionUpdate, evd, m_pSupportArg);
				}
			}
		}
	}
}

void CCryPSNIndividualSessionSupport::RemoveAllRoomMembers()
{
	for (uint32 i = 0; i < MAX_PSN_ROOM_MEMBERS; i++)
	{
		if ((m_members[i].m_conState != ePSNCS_None) && (m_members[i].m_conId != 0))
		{
			int ret = sceNpSignalingTerminateConnection(m_pPSNSupport->GetSignalingContext(), m_members[i].m_conId);
			if (ret < 0)
			{
				CryLogAlways("sceNpSignalingTerminateConnection() : Error %08X", ret);
				//-- Expected to be able to clean a signal p2p connection, but couldn't, so error
				m_pPSNSupport->HandlePSNError(ret);
			}

			m_members[i].m_conId = 0;
			m_members[i].m_conState = ePSNCS_None;
		}

		m_members[i].m_valid = ePSNMI_None;
	}
}

void CCryPSNIndividualSessionSupport::RemoveRoomMember(const SceNpMatching2RoomMemberDataInternal* pMemberInfo)
{
	if (pMemberInfo == NULL)
	{
		return;
	}

	for (uint32 a = 0; a < MAX_PSN_ROOM_MEMBERS; a++)
	{
		if (m_members[a].m_valid && (sceNpUtilCmpNpId(&m_members[a].m_npId, &pMemberInfo->userInfo.npId) == 0))
		{
			if (m_pSupportCb)									// On remove, call CB before we modify data
			{
				SCallbackEventData evd;
				evd.m_sessionHandle = m_sessionHandle;
				evd.m_pMemberInfo = &m_members[a];
				m_pSupportCb(eCE_ConnectionRemove, evd, m_pSupportArg);
			}

			if ((m_members[a].m_conState != ePSNCS_None) && (m_members[a].m_conId != 0))
			{
				int ret = sceNpSignalingTerminateConnection(m_pPSNSupport->GetSignalingContext(), m_members[a].m_conId);
				if (ret < 0)
				{
					CryLogAlways("sceNpSignalingTerminateConnection() : Error %08X", ret);
					//-- Expected to be able to clean a signal p2p connection, but couldn't, so error
					m_pPSNSupport->HandlePSNError(ret);
				}

				m_members[a].m_conId = 0;
				m_members[a].m_conState = ePSNCS_None;
			}

			m_members[a].m_valid = ePSNMI_None;

			return;
		}
	}
}

void CCryPSNIndividualSessionSupport::ChangeRoomOwner(SceNpMatching2RoomMemberId prevOwner, SceNpMatching2RoomMemberId newOwner)
{
	m_ownerMemberId = newOwner;

	for (uint32 a = 0; a < MAX_PSN_ROOM_MEMBERS; a++)
	{
		if (m_members[a].m_valid && (m_members[a].m_memberId == prevOwner))
		{
			if (m_members[a].m_valid & ePSNMI_Me)
			{
				m_members[a].m_valid = ePSNMI_Me;
			}
			else
			{
				m_members[a].m_valid = ePSNMI_Other;
			}

			if (m_pSupportCb)
			{
				SCallbackEventData evd;
				evd.m_sessionHandle = m_sessionHandle;
				evd.m_pMemberInfo = &m_members[a];
				m_pSupportCb(eCE_ConnectionOldOwner, evd, m_pSupportArg);
			}
		}

		if (m_members[a].m_valid && (m_members[a].m_memberId == newOwner))
		{
			if (m_members[a].m_valid & ePSNMI_Me)
			{
				m_members[a].m_valid = ePSNMI_MeOwner;
			}
			else
			{
				m_members[a].m_valid = ePSNMI_OtherOwner;
			}

			if (m_pSupportCb)
			{
				SCallbackEventData evd;
				evd.m_sessionHandle = m_sessionHandle;
				evd.m_pMemberInfo = &m_members[a];
				m_pSupportCb(eCE_ConnectionNewOwner, evd, m_pSupportArg);
			}
		}
	}
}

void CCryPSNIndividualSessionSupport::ClearMemberInfo()
{
	uint32 a;

	m_myMemberId = 0;
	m_ownerMemberId = 0;

	for (a = 0; a < MAX_PSN_ROOM_MEMBERS; a++)
	{
		if ((m_members[a].m_conState != ePSNCS_None) && (m_members[a].m_conId != 0))
		{
			sceNpSignalingTerminateConnection(m_pPSNSupport->GetSignalingContext(), m_members[a].m_conId);
			//-- No error handling for clean. It just does it if it can.

			m_members[a].m_conState = ePSNCS_None;
			m_members[a].m_conId = 0;
		}

		m_members[a].m_valid = ePSNMI_None;
	}
}

void CCryPSNIndividualSessionSupport::UpdateSignaling()
{
	for (uint32 a = 0; a < MAX_PSN_ROOM_MEMBERS; a++)
	{
		if (m_members[a].m_valid != ePSNMI_None)
		{
			//-- Timed out?
			if (m_members[a].m_conState == ePSNCS_Pending)
			{
				if ((g_time.GetMilliSecondsAsInt64() - m_members[a].m_conTimer.GetMilliSecondsAsInt64()) > SIGNALING_TIMEOUT)
				{
					m_members[a].m_conState = ePSNCS_Dead;

					if (m_pSupportCb)
					{
						SCallbackEventData evd;
						evd.m_sessionHandle = m_sessionHandle;
						evd.m_pMemberInfo = &m_members[a];
						m_pSupportCb(eCE_ConnectionUpdate, evd, m_pSupportArg);
					}
				}
			}
		}
	}
}

EPSNConnectionState CCryPSNIndividualSessionSupport::CheckSignaling()
{
	//-- If we can talk to the room owner, we'll allow the session state machine to run
	for (uint32 a = 0; a < MAX_PSN_ROOM_MEMBERS; a++)
	{
		if (m_members[a].m_valid & ePSNMI_Owner)
		{
			//-- Are we the owner?
			if (m_members[a].m_valid & ePSNMI_Me)
				return ePSNCS_Active;

			//-- Is peer connection to remote owner active?
			return m_members[a].m_conState;
		}
	}

	//-- No owner at all! This is wrong!
	return ePSNCS_None;
}

const SceNpMatching2SearchRoomResponse* const CCryPSNIndividualSessionSupport::GetSearchResult() const
{
	if (m_searchResponseAllocHandle != TMemInvalidHdl)
	{
		return (SceNpMatching2SearchRoomResponse*)m_pPSNSupport->GetLobby()->MemGetPtr(m_searchResponseAllocHandle);
	}

	return NULL;
}

void CCryPSNIndividualSessionSupport::ClearSearchResult()
{
	if (m_searchResponseAllocHandle != TMemInvalidHdl)
	{
		m_pPSNSupport->GetLobby()->MemFree(m_searchResponseAllocHandle);
		m_searchResponseAllocHandle = TMemInvalidHdl;
	}

	m_curTask = ePSNT_None;
}

const SceNpMatching2CreateJoinRoomResponse* const CCryPSNIndividualSessionSupport::GetCreateResult() const
{
	if (m_createResponseAllocHandle != TMemInvalidHdl)
	{
		return (SceNpMatching2CreateJoinRoomResponse*)m_pPSNSupport->GetLobby()->MemGetPtr(m_createResponseAllocHandle);
	}

	return NULL;
}

void CCryPSNIndividualSessionSupport::ClearCreateResult()
{
	if (m_createResponseAllocHandle != TMemInvalidHdl)
	{
		m_pPSNSupport->GetLobby()->MemFree(m_createResponseAllocHandle);
		m_createResponseAllocHandle = TMemInvalidHdl;
	}

	m_curTask = ePSNT_None;
}

const	SceNpMatching2JoinRoomResponse* const CCryPSNIndividualSessionSupport::GetJoinResult() const
{
	if (m_joinResponseAllocHandle != TMemInvalidHdl)
	{
		return (SceNpMatching2JoinRoomResponse*)m_pPSNSupport->GetLobby()->MemGetPtr(m_joinResponseAllocHandle);
	}

	return NULL;
}

void CCryPSNIndividualSessionSupport::ClearJoinResult()
{
	if (m_joinResponseAllocHandle != TMemInvalidHdl)
	{
		m_pPSNSupport->GetLobby()->MemFree(m_joinResponseAllocHandle);
		m_joinResponseAllocHandle = TMemInvalidHdl;
	}

	m_curTask = ePSNT_None;
}

void CCryPSNIndividualSessionSupport::ClearLeaveResult()
{
	m_curTask = ePSNT_None;
}

bool CCryPSNIndividualSessionSupport::StartTask(EPSNTask taskType)
{
	if (m_curTask != ePSNT_None)
	{	
		//-- task already running!
		return false;
	}

	assert(m_curTask == ePSNT_None);

	switch (taskType)
	{
		case ePSNT_None:
		default:
			return false;
		break;
		
		case ePSNT_Search:
		{
			m_curTask = ePSNT_Search;
			m_curState = ePSNIIS_StartSearchRoom;
			m_wantedState = ePSNIIS_RoomSearchResultsAvailable;
		}
		break;

		case ePSNT_Create:
		{
			m_curTask = ePSNT_Create;
			m_curState = ePSNIIS_StartCreateRoom;
			m_wantedState = ePSNIIS_RoomCreated;
		}
		break;

		case ePSNT_Join:
		{
			m_curTask = ePSNT_Join;
			m_curState = ePSNIIS_StartJoinRoom;
			m_wantedState = ePSNIIS_RoomJoined;
		}
		break;

		case ePSNT_Leave:
		{
			m_curTask = ePSNT_Leave;
			m_curState = ePSNIIS_StartLeaveRoom;
			m_wantedState = ePSNIIS_RoomLeft;
		}
		break;
	}

	return true;
}

void CCryPSNIndividualSessionSupport::AbortTask(EPSNTask taskType)
{
	if (m_curTask == taskType)
	{
		switch (m_curTask)
		{
			case ePSNT_None:
			default:
			break;

			case ePSNT_Leave:
				ClearLeaveResult();
			break;

			case ePSNT_Search:
				ClearSearchResult();
			break;

			case ePSNT_Create:
				ClearCreateResult();
			break;

			case ePSNT_Join:
				ClearJoinResult();
			break;
		}

		m_curTask = ePSNT_None;
		m_curState = ePSNIIS_NothingInProgress;
		m_wantedState = ePSNIIS_NothingInProgress;
	}
}

bool CCryPSNIndividualSessionSupport::TaskComplete(EPSNTask taskType)
{
	if (m_curTask == taskType)
	{
		switch (m_curTask)
		{
			default:
			case ePSNT_None:
				return true;
			break;

			case ePSNT_Search:
			case ePSNT_Create:
			case ePSNT_Join:
			case ePSNT_Leave:
			{
				if (InTheDesiredState())
				{
					return true;
				}
			}
			break;
		}
	}

	return false;
}

bool CCryPSNIndividualSessionSupport::InTheDesiredState()
{
	return (m_wantedState == m_curState);
}

void CCryPSNIndividualSessionSupport::TransitionToSearching()
{
	// Ensure world id is valid and pointing at our current world
	m_searchParam.m_searchRequest.worldId = m_pPSNSupport->GetWorldInfo()->worldId;
	if (m_searchParam.m_searchRequest.worldId == 0)
	{
		//-- Something funny.
		m_pPSNSupport->FlagPendingError(ePSNPE_NoWorldsAvailable, ePSNOS_ReadyToEnumerateServers);
	}

	int ret = sceNpMatching2SearchRoom(m_pPSNSupport->GetContext(), &m_searchParam.m_searchRequest, NULL, &m_reqId);
	if (ret < 0) 
	{
		CryLogAlways("sceNpMatching2SearchRoom : error %08X", ret);

		AbortTask(ePSNT_Search);

		m_pPSNSupport->HandlePSNError(ret);
		return;
	}

	m_curState = ePSNIIS_WaitingForSearchRoomCallback;
}

void CCryPSNIndividualSessionSupport::TransitionToCreating()
{
	// Ensure world id is valid and pointing at our current world
	m_createParam.m_createRequest.worldId = m_pPSNSupport->GetWorldInfo()->worldId;
	if (m_createParam.m_createRequest.worldId == 0)
	{
		//-- Something funny.
		m_pPSNSupport->FlagPendingError(ePSNPE_NoWorldsAvailable, ePSNOS_ReadyToEnumerateServers);
	}

	int ret = sceNpMatching2CreateJoinRoom(m_pPSNSupport->GetContext(), &m_createParam.m_createRequest, NULL, &m_reqId);
	if (ret < 0) 
	{
		CryLogAlways("sceNpMatching2CreateJoinRoom : error %08X", ret);

		AbortTask(ePSNT_Create);

		m_pPSNSupport->HandlePSNError(ret);
		return;
	}

	m_curState = ePSNIIS_WaitingForCreateRoomCallback;
}

void CCryPSNIndividualSessionSupport::TransitionToJoining()
{
	int ret = sceNpMatching2JoinRoom(m_pPSNSupport->GetContext(), &m_joinParam, NULL, &m_reqId);
	if (ret < 0) 
	{
		CryLogAlways("sceNpMatching2JoinRoom : error %08X",ret);

		AbortTask(ePSNT_Join);

		m_pPSNSupport->HandlePSNError(ret);
		return;
	}

	m_curState = ePSNIIS_WaitingForJoinRoomCallback;
}

void CCryPSNIndividualSessionSupport::TransitionToLeaving()
{
	int ret = sceNpMatching2LeaveRoom(m_pPSNSupport->GetContext(), &m_leaveParam, NULL, &m_reqId);
	if (ret < 0)
	{
		CryLogAlways("sceNpMatching2LeaveRoom : error %08X",ret);
			
		AbortTask(ePSNT_Leave);

		m_pPSNSupport->HandlePSNError(ret);
		return;
	}

	m_curState = ePSNIIS_WaitingForLeaveRoomCallback;
}



#endif//USE_PSN
