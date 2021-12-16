
#include "StdAfx.h"

#include "CryPSNLobby.h"

#if USE_PSN

#include "CryPSNSessionSupport.h"
#include "CryPSNSupport.h"

#define SYSUTIL_CALLBACK_SLOT			(3)				// Use last slot to avoid clashing with other users of the callbacks

#define CONTEXT_TIMEOUT	(10*1000*1000)			// 10 seconds as recommended by sony
#define REQUEST_TIMEOUT	(20*1000*1000)			// 20 seconds for default request timeout



CCryPSNSupport::CCryPSNSupport(SceNpCommunicationId* pCommId, SceNpCommunicationPassphrase* pPassPhrase, CryLobbySupportCallback cb, void *cbArg, CCryLobby* pLobby)
{
	m_curState = ePSNOS_Uninitialised;

	m_pSupportCb = cb;
	m_pSupportArg = cbArg;
	m_pLobby = pLobby;

	m_worldInfoAllocHandle = TMemInvalidHdl;

	m_pCommId = pCommId;
	m_pPassPhrase = pPassPhrase;

	m_ctxId = 0;
	m_sigCtxId = 0;
	
	for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
	{
		m_pSessionSupporters[i] = NULL;
	}

	ClearPendingError();
	
	memset(&m_netInfo, 0, sizeof(m_netInfo));
}

CCryPSNSupport::~CCryPSNSupport()
{
	CleanTo(ePSNOS_Uninitialised);

	ClearPendingError();

	for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
	{
		if (m_pSessionSupporters[i])
		{
			delete m_pSessionSupporters[i];
			m_pSessionSupporters[i] = NULL;
		}
	}
}

bool CCryPSNSupport::Initialise()
{
	for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
	{
		if (m_pSessionSupporters[i] == NULL)
		{
			m_pSessionSupporters[i] = new CCryPSNIndividualSessionSupport((CrySessionHandle)i, this, m_pSupportCb, m_pSupportArg);
			if (!m_pSessionSupporters[i])
			{
				CryLogAlways("CCryPSNIndividualSessionSupport alloc failed.");
				return false;
			}
		}
	}

	return true;
}

void CCryPSNSupport::SystemCallback(uint64 status, uint64 param, void *userData)
{
	CCryPSNSupport *_this = static_cast<CCryPSNSupport*>(userData);

	switch (status)
	{
		case CELL_SYSUTIL_REQUEST_EXITGAME:
		case CELL_SYSUTIL_DRAWING_BEGIN:
		case CELL_SYSUTIL_DRAWING_END:
		case CELL_SYSUTIL_BGMPLAYBACK_PLAY:
		case CELL_SYSUTIL_BGMPLAYBACK_STOP:
		case CELL_SYSUTIL_SYSTEM_MENU_CLOSE:
		case CELL_SYSUTIL_SYSTEM_MENU_OPEN:
		default:
			// Unknown or ignored value (possibly future SDK)
		break;				

		case CELL_SYSUTIL_NP_INVITATION_SELECTED:
			// Accepted game invitiation via XMB (will need to handle event for join here)
		break;

		case CELL_SYSUTIL_NET_CTL_NETSTART_LOADED:
			// Notifies the network start utility has appeared (we don't really care about that though)
		break;

		case CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED:
			// Notifies us its safe to call unload
			_this->m_curState = ePSNOS_DialogFinished;
		break;

		case CELL_SYSUTIL_NET_CTL_NETSTART_UNLOADED:
			// Network dialog has now closed
			_this->m_curState = ePSNOS_DiscoverNpId;
		break;
	}
}

// Context callback function
void CCryPSNSupport::ContextCallback(SceNpMatching2ContextId id, SceNpMatching2Event event, SceNpMatching2EventCause eventCause, int errorCode, void *arg)
{
	CCryPSNSupport *_this = static_cast<CCryPSNSupport*>(arg);

	if (errorCode < 0)
	{
		//-- Some error occurred.
		_this->HandlePSNError(errorCode);
	}

	//-- context id should match stored version - if they don't match, the context has probably been invalidated 
	//-- (which is why the callback is being called!).
	if (id != _this->m_ctxId)
	{
		//-- This should never happen!
		_this->FlagPendingError(ePSNPE_ContextError, ePSNOS_SignedIn);
		return;
	}

	switch (event)
	{
		case SCE_NP_MATCHING2_CONTEXT_EVENT_Start:
			_this->m_curState = ePSNOS_ContextReady;
		break;
		
		case SCE_NP_MATCHING2_CONTEXT_EVENT_Stop:
			_this->m_curState = ePSNOS_SignedIn;
		break;
		
		case SCE_NP_MATCHING2_CONTEXT_EVENT_StartOver:
			_this->m_curState = ePSNOS_SignedIn;
		break;
		
		default:
			// ignore unknown states (that may be introduced in future SDK)
		break;
	}
}

void CCryPSNSupport::SignalingCallback(uint32 id, uint32 subject, int myEvent, int error_code, void *arg)
{
	CCryPSNSupport *_this = static_cast<CCryPSNSupport*>(arg);
	SRoomMember* pMember;

	//-- signaling id should match stored version
	if (id != _this->m_sigCtxId)
	{
		//-- This should never happen!
		_this->FlagPendingError(ePSNPE_InternalError, ePSNOS_SignedIn);
		return;
	}

	switch (myEvent)
	{
		case SCE_NP_SIGNALING_EVENT_EXT_PEER_DEACTIVATED:
		case SCE_NP_SIGNALING_EVENT_DEAD:
		{
			for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
			{
				CCryPSNIndividualSessionSupport* pSessionSupport = _this->GetSessionSupport(i);
				if (pSessionSupport)
				{
					pMember = pSessionSupport->GetMemberFromConnectionId(subject);
					if (pMember)
					{
						pMember->m_conState = ePSNCS_Dead;

						if (_this->m_pSupportCb)
						{
							SCallbackEventData evd;
							evd.m_sessionHandle = (CrySessionHandle)i;
							evd.m_pMemberInfo = pMember;
							_this->m_pSupportCb(eCE_ConnectionUpdate, evd, _this->m_pSupportArg);
						}
					}
				}
			}
		}
		break;

		case SCE_NP_SIGNALING_EVENT_ESTABLISHED:
			// Don't care because we are only interested in a mutual activate
		break;

		case SCE_NP_SIGNALING_EVENT_NETINFO_ERROR:
		case SCE_NP_SIGNALING_EVENT_NETINFO_RESULT:
			CryLogAlways("[SIGNALING] Event : %08X",myEvent);
		break;

		case SCE_NP_SIGNALING_EVENT_EXT_PEER_ACTIVATED:
			// Don't care because we are only interested in a mutual activate
		break;

		case SCE_NP_SIGNALING_EVENT_EXT_MUTUAL_ACTIVATED:
		{
			int conStatus;

			for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
			{
				CCryPSNIndividualSessionSupport* pSessionSupport = _this->GetSessionSupport(i);
				if (pSessionSupport)
				{
					pMember = pSessionSupport->GetMemberFromConnectionId(subject);
					if (pMember)
					{
						int ret = sceNpSignalingGetConnectionStatus(id, subject, &conStatus, &pMember->m_peerAddr, &pMember->m_peerPort);
						if (ret < 0)
						{
							CryLogAlways("sceNpSignalingGetConnectionStatus : %08X",ret);
							pMember->m_conState = ePSNCS_Dead;
						}
						else
						{
							pMember->m_conState = ePSNCS_Active;
						}

						if (_this->m_pSupportCb)
						{
							SCallbackEventData evd;
							evd.m_sessionHandle = (CrySessionHandle)i;
							evd.m_pMemberInfo = pMember;
							_this->m_pSupportCb(eCE_ConnectionUpdate, evd, _this->m_pSupportArg);
						}
					}
				}
			}
		}
		break;

		default:
			// handle unknown responses - future sdks
		return;
	}
}

TMemHdl CCryPSNSupport::GetTemporaryEventDataBufferHandle(SceNpMatching2ContextId id, SceNpMatching2EventKey eventKey, size_t dataSize)
{
	TMemHdl h = TMemInvalidHdl;
	
	if (dataSize > 0)
	{
		h = GetLobby()->MemAlloc(dataSize);
		if (h != TMemInvalidHdl)
		{
			void* pEventData = GetLobby()->MemGetPtr(h);
			memset(pEventData, 0, dataSize);

			int ret = sceNpMatching2GetEventData(id, eventKey, pEventData, dataSize);
			if (ret < 0) 
			{
				CryLogAlways("sceNpMatching2GetEventData : Error %08X", ret);
				GetLobby()->MemFree(h);
				HandlePSNError(ret);
				return NULL;
			}
		}
		else
		{
			CryLogAlways("Failed to allocate memory for buffer");
			//-- Kick the state machine to offline state
			FlagPendingError(ePSNPE_OutOfMemory, ePSNOS_Offline);
			return NULL;
		}
	}

	return h;
}

void CCryPSNSupport::RequestCallback(SceNpMatching2ContextId id, SceNpMatching2RequestId reqId, SceNpMatching2Event myEvent, SceNpMatching2EventKey eventKey, int errorCode, size_t dataSize, void *arg)
{
	CCryPSNSupport *_this = static_cast<CCryPSNSupport*>(arg);

	if (errorCode < 0) 
	{
		switch (myEvent)
		{
			case SCE_NP_MATCHING2_REQUEST_EVENT_GetServerInfo:
			{
				CryLogAlways("SCE_NP_MATCHING2_REQUEST_EVENT_GetServerInfo : error %08X", errorCode);
			}
			break;
			case SCE_NP_MATCHING2_REQUEST_EVENT_CreateServerContext: 
			{
				CryLogAlways("SCE_NP_MATCHING2_REQUEST_EVENT_CreateServerContext : error %08x", errorCode);
			}
			break;
			case SCE_NP_MATCHING2_REQUEST_EVENT_GetWorldInfoList: 
			{ 
				CryLogAlways("SCE_NP_MATCHING2_REQUEST_EVENT_GetWorldInfoList : Error %08X", errorCode); 
			}
			break;
			case SCE_NP_MATCHING2_REQUEST_EVENT_SearchRoom: 
			{
				CryLogAlways("SCE_NP_MATCHING2_REQUEST_EVENT_SearchRoom : Error %08X", errorCode); 
			}
			break;
			case SCE_NP_MATCHING2_REQUEST_EVENT_CreateJoinRoom: 
			{
				CryLogAlways("SCE_NP_MATCHING2_REQUEST_EVENT_CreateJoinRoom : Error %08X", errorCode); 
			}
			break;
			case SCE_NP_MATCHING2_REQUEST_EVENT_JoinRoom: 
			{
				CryLogAlways("SCE_NP_MATCHING2_REQUEST_EVENT_JoinRoom : Error %08X", errorCode); 
			}
			break;
			case SCE_NP_MATCHING2_REQUEST_EVENT_LeaveRoom: 
			{
				CryLogAlways("SCE_NP_MATCHING2_REQUEST_EVENT_LeaveRoom : Error %08X", errorCode); 
			}
			break;
			default: 
			break;
		}
		
		if (dataSize > 0)
		{
			TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
			//-- if h is invalid, GetTemporaryEventDataBufferHandle() will have generated its own internal error, so we only have to 
			//-- handle the original errorCode if handle is _not_ invalid.
			if (h != TMemInvalidHdl)
			{
				_this->GetLobby()->MemFree(h);
				_this->HandlePSNError(errorCode);
			}
		}
		else
		{
			_this->HandlePSNError(errorCode);
		}
		return;
	}

	//-- context id should match stored version - if they don't match, the context has probably been invalidated
	//-- Ignore the event
	if (id != _this->m_ctxId)
	{
		TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
		if (h != TMemInvalidHdl)
		{
			_this->GetLobby()->MemFree(h);
		}
		return;
	}

	//-- Find which part of the PSN support the request relates to; main support, or one of the session supporters
	if (reqId == _this->m_reqId)
	{
		//-- This is for main support request events
		switch (myEvent)
		{
			default:
			{	
				//-- Ignore unknown events - might be worth dumping warning about them though

				//-- If datasize > 0 we must always get the data, otherwise the internal stack will overflow (apparently)
				TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
				if (h != TMemInvalidHdl)
				{
					_this->GetLobby()->MemFree(h);
				}
			}
			break;

			case SCE_NP_MATCHING2_REQUEST_EVENT_GetServerInfo:
			{
				if (dataSize > 0)
				{
					memset(&_this->m_serverList[_this->m_serverIdCur], 0, sizeof(_this->m_serverList[_this->m_serverIdCur]));
					int ret = sceNpMatching2GetEventData(id, eventKey, &_this->m_serverList[_this->m_serverIdCur], dataSize);
					if (ret < 0) 
					{
						CryLogAlways("sceNpMatching2GetEventData : error %08X",ret);
						_this->HandlePSNError(ret);
						return;
					}

					_this->m_curState = ePSNOS_ServerInfoRetrieved;
				}
			}
			break;

			case SCE_NP_MATCHING2_REQUEST_EVENT_CreateServerContext:
			{
				_this->m_curState = ePSNOS_ServerJoined;
			}
			break;

			case SCE_NP_MATCHING2_REQUEST_EVENT_GetWorldInfoList:
			{
				if (dataSize > 0)
				{
					assert(_this->m_worldInfoAllocHandle == TMemInvalidHdl);

					_this->m_worldInfoAllocHandle = _this->GetLobby()->MemAlloc(dataSize);
					if (_this->m_worldInfoAllocHandle == TMemInvalidHdl)
					{
						CryLogAlways("Failed to allocate memory for buffer");
						_this->FlagPendingError(ePSNPE_OutOfMemory, ePSNOS_Offline);
						return;
					}
						
					SceNpMatching2GetWorldInfoListResponse* pWorldInfo = (SceNpMatching2GetWorldInfoListResponse*)_this->GetLobby()->MemGetPtr(_this->m_worldInfoAllocHandle);
					memset(pWorldInfo, 0, dataSize);

					int ret = sceNpMatching2GetEventData(_this->m_ctxId, eventKey, pWorldInfo, dataSize);
					if (ret < 0) 
					{
						CryLogAlways("sceNpMatching2GetEventData : Error %08X",ret);
						_this->HandlePSNError(ret);
						return;
					}

					_this->m_curState = ePSNOS_WorldInfoRetrieved;
				}
			}
			break;
		}

		return;
	}
	else
	{
		//-- is it a session support request?
		for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
		{
			CCryPSNIndividualSessionSupport* pSessionSupport = _this->GetSessionSupport(i);
			if (pSessionSupport)
			{
				if (pSessionSupport->HandleRequestEvent(id, reqId, myEvent, eventKey, dataSize))
				{
					return;
				}
			}
		}
	}

	//-- If we reach here, we've somehow got a request event for something that is not the primary support or one of the valid session supports.
	//-- We still have to remove the event data from the stack if it exists, and then ditch it immediately.
	TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
	if (h != TMemInvalidHdl)
	{
		_this->GetLobby()->MemFree(h);
	}
}

void CCryPSNSupport::RoomEventCallback(SceNpMatching2ContextId id, SceNpMatching2RoomId roomId, SceNpMatching2Event myEvent, SceNpMatching2EventKey eventKey, int errorCode, size_t dataSize, void *arg)
{
	CCryPSNSupport *_this = static_cast<CCryPSNSupport*>(arg);

	if (errorCode < 0) 
	{
		switch (myEvent)
		{
			case SCE_NP_MATCHING2_ROOM_EVENT_MemberJoined: 
			{
				CryLogAlways("SCE_NP_MATCHING2_ROOM_EVENT_MemberJoined : error %08X", errorCode); 
			}
			break;
			case SCE_NP_MATCHING2_ROOM_EVENT_MemberLeft: 
			{
				CryLogAlways("SCE_NP_MATCHING2_ROOM_EVENT_MemberLeft : error %08x", errorCode); 
			}
			break;
			case SCE_NP_MATCHING2_ROOM_EVENT_Kickedout:
			{
				CryLogAlways("SCE_NP_MATCHING2_ROOM_EVENT_Kickedout : Error %08X", errorCode); 
			}
			break;
			case SCE_NP_MATCHING2_ROOM_EVENT_RoomDestroyed: 
			{
				CryLogAlways("SCE_NP_MATCHING2_ROOM_EVENT_RoomDestroyed : Error %08X", errorCode); 
			}
			break;
			case SCE_NP_MATCHING2_ROOM_EVENT_RoomOwnerChanged: 
			{
				CryLogAlways("SCE_NP_MATCHING2_ROOM_EVENT_RoomOwnerChanged : Error %08X", errorCode); 
			}
			break;
			case SCE_NP_MATCHING2_ROOM_EVENT_UpdatedRoomDataInternal: 
			{
				CryLogAlways("SCE_NP_MATCHING2_ROOM_EVENT_UpdatedRoomDataInternal : Error %08X", errorCode); 
			}
			break;
			case SCE_NP_MATCHING2_ROOM_EVENT_UpdatedRoomMemberDataInternal: 
			{
				CryLogAlways("SCE_NP_MATCHING2_ROOM_EVENT_UpdatedRoomMemberDataInternal : Error %08X", errorCode); 
			}
			break;
			case SCE_NP_MATCHING2_ROOM_EVENT_UpdatedSignalingOptParam: 
			{
				CryLogAlways("SCE_NP_MATCHING2_ROOM_EVENT_UpdatedSignalingOptParam : Error %08X", errorCode); 
			}
			break;
			default: 
			break;
		}

		if (dataSize > 0)
		{
			TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
			//-- if h is invalid, GetTemporaryEventDataBufferHandle() will have generated its own internal error, so we only have to 
			//-- handle the original errorCode if handle is _not_ invalid.
			if (h != TMemInvalidHdl)
			{
				_this->GetLobby()->MemFree(h);
				_this->HandlePSNError(errorCode);
			}
		}
		else
		{
			_this->HandlePSNError(errorCode);
		}
		return;
	}
			
	//-- Now we can check this is an event for our correct context id.
	//-- This should always be in sync; if not, the context has probably invalidated so we should ignore the event.
	if (id != _this->m_ctxId)
	{
		TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
		if (h != TMemInvalidHdl)
		{
			_this->GetLobby()->MemFree(h);
		}
		return;
	}

	//-- Find the appropriate session for this room.
	CCryPSNIndividualSessionSupport* pSessionSupport = NULL;
	for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
	{
		CCryPSNIndividualSessionSupport* pSS = _this->GetSessionSupport(i);
		if (pSS && (pSS->GetRoomId() == roomId))
		{
			pSessionSupport = pSS;
			break;
		}
	}
	if (pSessionSupport == NULL)
	{
		//-- Could not find a session that matches the room id, so we'll ignore the event.
		TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
		if (h != TMemInvalidHdl)
		{
			_this->GetLobby()->MemFree(h);
		}
		return;
	}

	//-- Now we can process the event.

	switch (myEvent)
	{
		default:
		{
			// Ignore unknown events
			TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
			if (h != TMemInvalidHdl)
			{
				_this->GetLobby()->MemFree(h);
			}
		}
		break;

		case SCE_NP_MATCHING2_ROOM_EVENT_MemberJoined:
		{
			TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
			if (h != TMemInvalidHdl)
			{
				SceNpMatching2RoomMemberUpdateInfo* pMemberInfo = (SceNpMatching2RoomMemberUpdateInfo*)_this->GetLobby()->MemGetPtr(h);

				if (pMemberInfo->roomMemberDataInternal->flagAttr & SCE_NP_MATCHING2_ROOMMEMBER_FLAG_ATTR_OWNER)
				{
					pSessionSupport->AddRoomMember(pMemberInfo->roomMemberDataInternal, ePSNMI_OtherOwner);
				}
				else
				{
					pSessionSupport->AddRoomMember(pMemberInfo->roomMemberDataInternal, ePSNMI_Other);
				}

				_this->GetLobby()->MemFree(h);
			}
		}
		break;
		
		case SCE_NP_MATCHING2_ROOM_EVENT_MemberLeft:
		{
			TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
			if (h != TMemInvalidHdl)
			{
				SceNpMatching2RoomMemberUpdateInfo* pMemberInfo = (SceNpMatching2RoomMemberUpdateInfo*)_this->GetLobby()->MemGetPtr(h);

				pSessionSupport->RemoveRoomMember(pMemberInfo->roomMemberDataInternal);

				_this->GetLobby()->MemFree(h);
			}
		}
		break;

		case SCE_NP_MATCHING2_ROOM_EVENT_Kickedout:
		{
			CryLogAlways("KickedOut");

			TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
			if (h != TMemInvalidHdl)
			{
				SceNpMatching2RoomUpdateInfo* pInfo = (SceNpMatching2RoomUpdateInfo*)_this->GetLobby()->MemGetPtr(h);
				// TODO
				_this->GetLobby()->MemFree(h);
			}
		}
		break;

		case SCE_NP_MATCHING2_ROOM_EVENT_RoomOwnerChanged:
		{
			TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
			if (h != TMemInvalidHdl)
			{
				SceNpMatching2RoomOwnerUpdateInfo* pInfo = (SceNpMatching2RoomOwnerUpdateInfo*)_this->GetLobby()->MemGetPtr(h);

				pSessionSupport->ChangeRoomOwner(pInfo->prevOwner, pInfo->newOwner);

				_this->GetLobby()->MemFree(h);
			}			
		}
		break;

		case SCE_NP_MATCHING2_ROOM_EVENT_RoomDestroyed:
		{
	//		0x1104
	//		Room was deleted
			CryLogAlways("RoomDestroyed");

			TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
			if (h != TMemInvalidHdl)
			{
				SceNpMatching2RoomUpdateInfo* pInfo = (SceNpMatching2RoomUpdateInfo*)_this->GetLobby()->MemGetPtr(h);
				// TODO
				_this->GetLobby()->MemFree(h);
			}
		}
		break;


		case SCE_NP_MATCHING2_ROOM_EVENT_UpdatedRoomDataInternal:
		{
	//		0x1106
	//		Internal attributes and basic data of room were updated
	//		(Data in SceNpMatching2RoomDataInternal structure was updated)
			CryLogAlways("UpdateRoomDataInternal");

			TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
			if (h != TMemInvalidHdl)
			{
				SceNpMatching2RoomDataInternalUpdateInfo* pInfo = (SceNpMatching2RoomDataInternalUpdateInfo*)_this->GetLobby()->MemGetPtr(h);
				// TODO
				_this->GetLobby()->MemFree(h);
			}
		}
		break;

		case SCE_NP_MATCHING2_ROOM_EVENT_UpdatedRoomMemberDataInternal:
		{
	//		0x1107
	//		Internal attributes and basic data of room member were updated
	//		(Data in SceNpMatching2RoomMemberDataInternalstructure was updated)
			CryLogAlways("UpdateRoomMemberDataInternal");

			TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
			if (h != TMemInvalidHdl)
			{
				SceNpMatching2RoomMemberDataInternalUpdateInfo* pInfo = (SceNpMatching2RoomMemberDataInternalUpdateInfo*)_this->GetLobby()->MemGetPtr(h);
				// TODO
				_this->GetLobby()->MemFree(h);
			}
		}
		break;

		case SCE_NP_MATCHING2_ROOM_EVENT_UpdatedSignalingOptParam:
		{
			CryLogAlways("UpdatedSignalingOptParam");

			TMemHdl h = _this->GetTemporaryEventDataBufferHandle(id, eventKey, dataSize);
			if (h != TMemInvalidHdl)
			{
				SceNpMatching2SignalingOptParamUpdateInfo* pInfo = (SceNpMatching2SignalingOptParamUpdateInfo*)_this->GetLobby()->MemGetPtr(h);
				// TODO
				_this->GetLobby()->MemFree(h);
			}
		}
		break;
	}
}
	
void CCryPSNSupport::TransitionToInitialisedOffline()
{
	int ret = cellSysutilRegisterCallback(SYSUTIL_CALLBACK_SLOT, SystemCallback, this);
	if (ret < 0)
	{
		CryLogAlways("cellSysutilRegisterCallback() : Error %08X", ret);
		return;
	}

	m_curState = ePSNOS_Offline;
}

void CCryPSNSupport::TransitionToWaitingForDialogOpen()
{
	CellNetCtlNetStartDialogParam dialogParam;
	dialogParam.size = sizeof(dialogParam);
	dialogParam.type = CELL_NET_CTL_NETSTART_TYPE_NP;
	dialogParam.cid = 0;

	// Launch signin dialog (in case user has not signed in yet)
	int ret = cellNetCtlNetStartDialogLoadAsync(&dialogParam);
	if (ret < 0)
	{
		CryLogAlways("cellNetCtlNetStartDialogLoadAsync() : Error %08X",ret);
		HandlePSNError(ret);
		return;
	}

	m_curState = ePSNOS_WaitCallback;
}

void CCryPSNSupport::TransitionToWaitingForDialogClose()
{
	m_dialogResult.size = sizeof(m_dialogResult);

	int ret = cellNetCtlNetStartDialogUnloadAsync(&m_dialogResult);
	if (ret < 0)
	{
		CryLogAlways("cellNetCtlNetStartDialogUnloadAsync() : Error %08X",ret);
		HandlePSNError(ret);
		return;
	}

	m_curState = ePSNOS_WaitCallback;
}

void CCryPSNSupport::TransitionToSignedIn()
{
	if (m_dialogResult.result == 0)
	{
		int ret = sceNpManagerGetNpId(&m_myNpId);
		if (ret < 0)
		{
			CryLogAlways("sceNpManagerGetNpId() : Error %08X",ret);
			HandlePSNError(ret);
			return;
		}

		m_curState = ePSNOS_SignedIn;
	}
	else
	{
		HandlePSNError(m_dialogResult.result);
	}
}

void CCryPSNSupport::TransitionToContextFound()
{
	int option	=	0;							// Note this means online name and avatar are not requested (bandwidth saving)
												// Will need to expose the choice to users of the lobby at a later point
												// SCE_NP_MATCHING2_CONTEXT_OPTION_USE_ONLINENAME | SCE_NP_MATCHING2_CONTEXT_OPTION_USE_AVATARURL

	// Create a context (we will use the same one at all times, while still signed in)
	int ret = sceNpMatching2CreateContext(&m_myNpId, m_pCommId, m_pPassPhrase, &m_ctxId, option);
	if (ret < 0) 
	{
		CryLogAlways("sceNpMatching2CreateContext() : Error %08X",ret);
		HandlePSNError(ret);
		return;
	}
 
	// Register the context callback handler
	ret = sceNpMatching2RegisterContextCallback(m_ctxId, ContextCallback, this);
	if (ret < 0) 
	{
		CryLogAlways("sceNpMatching2RegisterContextCallback() : Error %08X",ret);
		HandlePSNError(ret);
		return;
	}

	ret = sceNpMatching2RegisterRoomEventCallback(m_ctxId, RoomEventCallback, this);
	if (ret < 0)
	{
		CryLogAlways("sceNpMatching2RegisterRoomEventCallback() : Error %08X",ret);
		HandlePSNError(ret);
		return;
	}

	// Start the context
	ret = sceNpMatching2ContextStartAsync(m_ctxId, CONTEXT_TIMEOUT);
	if (ret < 0) 
	{
		CryLogAlways("sceNpMatching2ContextStartAsync() : Error %08X",ret);
		HandlePSNError(ret);
		return;
	}

	m_curState = ePSNOS_WaitCallback;
}

void CCryPSNSupport::TransitionToPeerInfoSet()
{
	SceNpMatching2RequestOptParam optParam;
	int ret;

	// Set default option parameters for this context - means we don't need to pass optParam to any query functions
	memset(&optParam, 0, sizeof(optParam));
	optParam.cbFunc = RequestCallback;
	optParam.cbFuncArg = this;
	optParam.timeout = REQUEST_TIMEOUT;
	optParam.appReqId = 0;

	ret = sceNpMatching2SetDefaultRequestOptParam(m_ctxId, &optParam);
	if (ret < 0) 
	{
		CryLogAlways("sceNpMatching2SetDefaultRequestOptParam() : Error %08X",ret);
		HandlePSNError(ret);
		return;
	}

	ret = sceNpSignalingCreateCtx(&m_myNpId, NULL, NULL, &m_sigCtxId);
	if (ret < 0)
	{
		CryLogAlways("sceNpSignalingCreateCtx() : Error %08X", ret);
		HandlePSNError(ret);
		return;
	}

	ret = sceNpSignalingAddExtendedHandler(m_sigCtxId, SignalingCallback, this);
	if (ret < 0)
	{
		CryLogAlways("sceNpSignalingAddExtendedHandler() : Error %08X",ret);
		HandlePSNError(ret);
		return;
	}

	memset(&m_netInfo,0,sizeof(m_netInfo));
	m_netInfo.size = sizeof(m_netInfo);

	ret = sceNpSignalingGetLocalNetInfo(m_sigCtxId, &m_netInfo);
	if (ret < 0)
	{
		CryLogAlways("sceNpSignalingGetLocalNetInfo() : Error %08X",ret);
			// this isn't fatal so can just continue as normal
	}

	m_curState = ePSNOS_ReadyToEnumerateServers;
}

void CCryPSNSupport::TransitionToFirstServer()
{
	//E Get list of IDs of servers allocated to the application
	int ret = sceNpMatching2GetServerIdListLocal(m_ctxId, m_serverIdsList, MAX_PSN_SERVER_IDS);
	if (ret < 0) 
	{
		CryLogAlways("sceNpMatching2GetServerIdListLocal : error %08x",ret);
		HandlePSNError(ret);
		return;
	}

	m_serverIdCnt = ret;
	m_serverIdCur = 0;

	m_curState = ePSNOS_GettingNextServer;
}

void CCryPSNSupport::TransitionToServerInfoDiscovery()
{
	SceNpMatching2GetServerInfoRequest reqParam;
	int ret;

	if (m_serverIdCur < m_serverIdCnt)
	{
		memset(&reqParam, 0, sizeof(reqParam));
		reqParam.serverId = m_serverIdsList[m_serverIdCur];

		ret = sceNpMatching2GetServerInfo(m_ctxId, &reqParam, NULL, &m_reqId);
		if (ret < 0) 
		{
			CryLogAlways("sceNpMatching2GetServerInfo : error %08X", ret);
			HandlePSNError(ret);
			return;
		}

		m_curState = ePSNOS_WaitCallback;
	}
	else
	{
		m_curState = ePSNOS_ServerListDone;
	}
}

void CCryPSNSupport::TransitionToNextServer()
{
	m_serverIdCur++;
	m_curState = ePSNOS_GettingNextServer;
}

void CCryPSNSupport::TransitionToServerContextCreated()
{
	SceNpMatching2CreateServerContextRequest reqParam;
	int ret;

	//-- This just picks the first available server, which WILL need changing later
	
	m_serverIdCur = m_serverIdCnt;
	for (uint32 a = 0; a < m_serverIdCnt; a++)
	{
		if (m_serverList[a].server.status == SCE_NP_MATCHING2_SERVER_STATUS_AVAILABLE)
		{
			m_serverIdCur = a;
			break;
		}
	}

	if (m_serverIdCur == m_serverIdCnt)
	{
		FlagPendingError(ePSNPE_NoServersAvailable, ePSNOS_SignedIn);
		return;
	}

	memset(&reqParam, 0, sizeof(reqParam));
	reqParam.serverId = m_serverIdsList[m_serverIdCur];

	ret = sceNpMatching2CreateServerContext(m_ctxId, &reqParam, NULL, &m_reqId);
	if (ret < 0)
	{
		CryLogAlways("sceNpMatching2CreateServerContext : error %08X",ret);
		HandlePSNError(ret);
		return;
	}

	m_curState = ePSNOS_WaitCallback;
}

void CCryPSNSupport::TransitionToWorldInfoDiscovery()
{
	SceNpMatching2GetWorldInfoListRequest reqParam;
	int ret;

	//E Request parameters
	memset(&reqParam, 0, sizeof(reqParam));
	reqParam.serverId = m_serverIdsList[m_serverIdCur];

	ret = sceNpMatching2GetWorldInfoList(m_ctxId, &reqParam, NULL, &m_reqId);
	if (ret < 0) 
	{
		CryLogAlways("sceNpMatching2GetWorldInfoList : error %08X", ret);
		HandlePSNError(ret);
		return;
	}
	
	m_curState = ePSNOS_WaitCallback;
}

void CCryPSNSupport::TransitionToOnline()
{
	if (m_worldInfoAllocHandle == TMemInvalidHdl)
	{
		FlagPendingError(ePSNPE_NoWorldsAvailable, ePSNOS_ReadyToEnumerateServers);
		return;
	}

	SceNpMatching2GetWorldInfoListResponse* pWorldInfo = (SceNpMatching2GetWorldInfoListResponse*)GetLobby()->MemGetPtr(m_worldInfoAllocHandle);
	if (pWorldInfo->worldNum < 1)
	{
		FlagPendingError(ePSNPE_NoWorldsAvailable, ePSNOS_ReadyToEnumerateServers);
		return;
	}

	// For now, we will use world 1 (but we WILL need to have a world selection policy in place in the near future)
	memcpy(&m_curWorldInfo, &pWorldInfo->world[0], sizeof(m_curWorldInfo));

	GetLobby()->MemFree(m_worldInfoAllocHandle);
	m_worldInfoAllocHandle = TMemInvalidHdl;

	m_curState = ePSNOS_Online;
}

void CCryPSNSupport::CleanTo(EPSNOnlineState state)
{
	switch (state)
	{
		default:
		{
			NET_ASSERT(!"Shouldn't be cleaning to this state!");
			return;
		}

		case ePSNOS_Uninitialised:
		{
			int ret = cellSysutilUnregisterCallback(SYSUTIL_CALLBACK_SLOT);
			if (ret < 0)
			{
				CryLogAlways("cellSysutilUnregisterCallback() : Error %08X", ret);
			}			
		}

		case ePSNOS_Offline:
		case ePSNOS_SignedIn:
		{
			if (m_sigCtxId != 0)
			{
				int ret = sceNpSignalingDestroyCtx(m_sigCtxId);
				if (ret < 0)
				{
					CryLogAlways("sceNpSignalingDestroyCtx() : Error %08X", ret);
				}			
				m_sigCtxId = 0;
			}

			if (m_ctxId != 0)
			{
				int ret = sceNpMatching2DestroyContext(m_ctxId);
				if (ret < 0)
				{
					CryLogAlways("sceNpMatching2DestroyContext() : Error %08X", ret);
				}			
				m_ctxId = 0;
			}
		}

		case ePSNOS_ReadyToEnumerateServers:
		{
			for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
			{
				if (m_pSessionSupporters[i])
				{
					m_pSessionSupporters[i]->Clean();
				}
			}

			if (m_worldInfoAllocHandle != TMemInvalidHdl)
			{
				GetLobby()->MemFree(m_worldInfoAllocHandle);
				m_worldInfoAllocHandle = TMemInvalidHdl;
			}			
		}
	}
}

void CCryPSNSupport::TransitionForwards()
{
	switch (m_curState)
	{
	case ePSNOS_WaitCallback:								// Holding slot (waiting for external callback to trigger before moving on)
	default:
		break;
	case ePSNOS_Uninitialised:
		TransitionToInitialisedOffline();
		break;
	case ePSNOS_Offline:
		TransitionToWaitingForDialogOpen();
		break;
	case ePSNOS_DialogFinished:
		TransitionToWaitingForDialogClose();
		break;
	case ePSNOS_DiscoverNpId:
		TransitionToSignedIn();
		break;
	case ePSNOS_SignedIn:
		TransitionToContextFound();
		break;
	case ePSNOS_ContextReady:
		TransitionToPeerInfoSet();
		break;
	case ePSNOS_ReadyToEnumerateServers:
		TransitionToFirstServer();
		break;
	case ePSNOS_GettingNextServer:
		TransitionToServerInfoDiscovery();
		break;
	case ePSNOS_ServerInfoRetrieved:
		TransitionToNextServer();
		break;
	case ePSNOS_ServerListDone:
		TransitionToServerContextCreated();
		break;
	case ePSNOS_ServerJoined:
		TransitionToWorldInfoDiscovery();
		break;
	case ePSNOS_WorldInfoRetrieved:
		TransitionToOnline();
		break;
	case ePSNOS_Online:										// Holding state (waiting for request for a particular service)
		break;
	}
}

CCryPSNIndividualSessionSupport* CCryPSNSupport::GetSessionSupport(CrySessionHandle sessionHandle)
{
	assert(sessionHandle < MAX_MATCHMAKING_SESSIONS);
	return m_pSessionSupporters[sessionHandle];
}
	

void CCryPSNSupport::Tick()
{
	if (m_curState > ePSNOS_Uninitialised)
	{
		cellSysutilCheckCallback();
	}

	//-- We should only transition forwards if there is no pending error, and at least one of the sessions has a task to process
	if (!ErrorPending())
	{
		bool bNeedsUpdates = false;
		for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
		{
			if (m_pSessionSupporters[i])
			{
				if (!m_pSessionSupporters[i]->InTheDesiredState())
				{
					bNeedsUpdates = true;
					break;
				}
			}
		}

		if (bNeedsUpdates)
		{
			TransitionForwards();

			if (m_curState == ePSNOS_Online)
			{
				for (uint32 i = 0; i < MAX_MATCHMAKING_SESSIONS; i++)
				{
					if (m_pSessionSupporters[i])
					{
						m_pSessionSupporters[i]->Tick();
					}
				}
			}
		}
	}
}

void CCryPSNSupport::FlagPendingError(EPSNPendingError errorCode, EPSNOnlineState newState)
{
	m_pendingErrorCondition = errorCode;

	switch (newState)
	{
		default:
		{
			NET_ASSERT(!"Shouldn't be switching to this state after an error!");
		}
		break;

		case ePSNOS_Offline:
		case ePSNOS_SignedIn:
		case ePSNOS_ReadyToEnumerateServers:
		{
			CleanTo(newState);
			m_curState = newState;
		}
		break;
	}
}

void CCryPSNSupport::ClearPendingError()
{
	m_pendingErrorCondition = ePSNPE_None;
}

void CCryPSNSupport::HandlePSNError(int retCode)
{
	int ret;

	switch (retCode)
	{
	case CELL_NET_CTL_ERROR_NOT_INITIALIZED:
		FlagPendingError(ePSNPE_NetLibraryNotInitialised, ePSNOS_Offline);
		break;
	case CELL_NET_CTL_ERROR_NP_RESERVED2: 
	case CELL_NET_CTL_ERROR_NP_RESERVED1:
	case CELL_NET_CTL_ERROR_INVALID_SIZE:
	case CELL_NET_CTL_ERROR_INVALID_TYPE:
	case CELL_NET_CTL_ERROR_NOT_AVAIL:
	case CELL_NET_CTL_ERROR_INVALID_ADDR:
	case CELL_NET_CTL_ERROR_INVALID_CODE:
	case CELL_NET_CTL_ERROR_INVALID_ID:
	case CELL_NET_CTL_ERROR_ID_NOT_FOUND:
	case CELL_NET_CTL_ERROR_HANDLER_MAX:
	case CELL_NET_CTL_ERROR_NOT_TERMINATED:
		FlagPendingError(ePSNPE_InternalError, ePSNOS_Offline);
		break;
	case CELL_NET_CTL_ERROR_NOT_CONNECTED:
	case CELL_NET_CTL_ERROR_NET_NOT_CONNECTED:
		FlagPendingError(ePSNPE_NotConnected, ePSNOS_Offline);
		break;
	case CELL_NET_CTL_ERROR_NET_DISABLED:
		FlagPendingError(ePSNPE_InternetDisabled, ePSNOS_Offline);
		break;
	case CELL_NET_CTL_ERROR_NP_NO_ACCOUNT:
		FlagPendingError(ePSNPE_NoPSNAccount, ePSNOS_Offline);
		break;
	case CELL_NET_CTL_ERROR_NET_CABLE_NOT_CONNECTED:
		FlagPendingError(ePSNPE_CableDisconnected, ePSNOS_Offline);
		break;
	case CELL_NET_CTL_ERROR_DIALOG_CANCELED:
	case CELL_NET_CTL_ERROR_DIALOG_ABORTED:
		FlagPendingError(ePSNPE_SignInCancelled, ePSNOS_Offline);
		break;
	case SCE_NP_ERROR_OFFLINE:
	case SCE_NP_ERROR_INVALID_STATE:
		FlagPendingError(ePSNPE_Offline, ePSNOS_Offline);
		break;
	case SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED:
		FlagPendingError(ePSNPE_NPLibraryNotInitialised, ePSNOS_Offline);
		break;
	case SCE_NP_MATCHING2_ERROR_CONTEXT_UNAVAILABLE:
		FlagPendingError(ePSNPE_ContextError, ePSNOS_SignedIn);
		break;
	case SCE_NP_MATCHING2_ERROR_CONTEXT_NOT_FOUND:
	case SCE_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID:
		FlagPendingError(ePSNPE_ContextError, ePSNOS_SignedIn);
		break;
	case SCE_NP_MATCHING2_ERROR_CONTEXT_ALREADY_STARTED:		
		FlagPendingError(ePSNPE_InternalError, ePSNOS_Offline);
		break;
	case SCE_NP_MATCHING2_ERROR_SERVER_NOT_AVAILABLE:
		// Don't flag the error, but record the server as unavailable for this slot and move to info retrieved state
		m_serverList[m_serverIdCur].server.status = SCE_NP_MATCHING2_SERVER_STATUS_UNAVAILABLE;
		m_curState = ePSNOS_ServerInfoRetrieved;
		break;
	case SCE_NP_MATCHING2_ERROR_INVALID_SERVER_ID:
	case SCE_NP_MATCHING2_ERROR_INVALID_ALIGNMENT:
	case SCE_NP_MATCHING2_ERROR_INVALID_ARGUMENT:
	default:
		FlagPendingError(ePSNPE_InternalError, ePSNOS_Offline);
		break;
	}
}


#endif//USE_PSN
