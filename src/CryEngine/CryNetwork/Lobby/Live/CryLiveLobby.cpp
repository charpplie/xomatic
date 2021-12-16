
#include "StdAfx.h"
#include "CryLiveLobby.h"

ECryLobbyError CryLiveLobbyGetErrorFromLive(DWORD error)
{
	switch (error)
	{
	case ERROR_SUCCESS:
	case ERROR_IO_PENDING:
		return eCLE_Success;

	case SPA_E_NOT_LOADED:
		return eCLE_SPAFileOutOfDate;

	case XONLINE_E_SESSION_FULL:
		return eCLE_SessionFull;

	case XONLINE_E_SESSION_WRONG_STATE:
		return eCLE_SessionWrongState;

	case 0x80070490: // For some reason Microsoft don't define all their errors. This one comes up when a non signed in user index is used.
	case XONLINE_E_SESSION_NOT_LOGGED_ON:
		return eCLE_UserNotSignedIn;

	case XONLINE_E_MATCH_INVALID_PARAM:
		return eCLE_InvalidParam;

	case XONLINE_E_SESSION_INSUFFICIENT_PRIVILEGES:
		return eCLE_InsufficientPrivileges;

	case XONLINE_E_STAT_USER_NOT_FOUND:
		return eCLE_UserNotInSession;

	case XONLINE_E_STAT_INVALID_TITLE_OR_LEADERBOARD:
		return eCLE_LeaderBoardNotRegistered;

	default:
		CryLogAlways("[Lobby] eCLE_InternalError: Live Error %d %x", error, error);
		return eCLE_InternalError;
	}
}

ECryLobbyError CryUserDataToLiveUserData(SCryLobbyUserData* pCryData, SLiveUserData* pLiveData)
{
	uint32 liveType = XPROPERTYTYPEFROMID(pCryData->m_id);

	switch (liveType)
	{
	case XUSER_DATA_TYPE_CONTEXT:
		{
			pLiveData->typeContext = true;
			pLiveData->context.dwContextId = pCryData->m_id;

			switch (pCryData->m_type)
			{
			case eCLUDT_Int32:
				pLiveData->context.dwValue = pCryData->m_int32;
				break;

			case eCLUDT_Int16:
				pLiveData->context.dwValue = pCryData->m_int16;
				break;

			case eCLUDT_Int8:
				pLiveData->context.dwValue = pCryData->m_int8;
				break;

			default:
				return eCLE_UserDataTypeMissMatch;
			}
		}
		break;

	case XUSER_DATA_TYPE_INT32:
		{
			pLiveData->typeContext = false;
			pLiveData->property.dwPropertyId = pCryData->m_id;
			pLiveData->property.value.type = XUSER_DATA_TYPE_INT32;

			switch (pCryData->m_type)
			{
			case eCLUDT_Int32:
				pLiveData->property.value.nData = pCryData->m_int32;
				break;

			case eCLUDT_Int16:
				pLiveData->property.value.nData = pCryData->m_int16;
				break;

			case eCLUDT_Int8:
				pLiveData->property.value.nData = pCryData->m_int8;
				break;

			default:
				return eCLE_UserDataTypeMissMatch;
			}
		}
		break;

	case XUSER_DATA_TYPE_INT64:
		{
			pLiveData->typeContext = false;
			pLiveData->property.dwPropertyId = pCryData->m_id;
			pLiveData->property.value.type = XUSER_DATA_TYPE_INT64;

			switch (pCryData->m_type)
			{
			case eCLUDT_Int64:
				pLiveData->property.value.i64Data = pCryData->m_int64;
				break;

			case eCLUDT_Int32:
				pLiveData->property.value.i64Data = pCryData->m_int32;
				break;

			case eCLUDT_Int16:
				pLiveData->property.value.i64Data = pCryData->m_int16;
				break;

			case eCLUDT_Int8:
				pLiveData->property.value.i64Data = pCryData->m_int8;
				break;

			case eCLUDT_Int64NoEndianSwap:
				// Assuming Live endian swaps it's data going between xbox and pc we need to endian swap our data here so it isn't swapped at the other end.
				pLiveData->property.value.i64Data = pCryData->m_int64;
				SwapEndian(pLiveData->property.value.i64Data);
				break;

			default:
				return eCLE_UserDataTypeMissMatch;
			}
		}
		break;

	case XUSER_DATA_TYPE_DOUBLE:
		{
			pLiveData->typeContext = false;
			pLiveData->property.dwPropertyId = pCryData->m_id;
			pLiveData->property.value.type = XUSER_DATA_TYPE_DOUBLE;

			switch (pCryData->m_type)
			{
			case eCLUDT_Float64:
				pLiveData->property.value.dblData = pCryData->m_f64;
				break;

			case eCLUDT_Float32:
				pLiveData->property.value.dblData = pCryData->m_f32;
				break;

			default:
				return eCLE_UserDataTypeMissMatch;
			}
		}
		break;

	case XUSER_DATA_TYPE_FLOAT:
		{
			pLiveData->typeContext = false;
			pLiveData->property.dwPropertyId = pCryData->m_id;
			pLiveData->property.value.type = XUSER_DATA_TYPE_FLOAT;

			switch (pCryData->m_type)
			{
			case eCLUDT_Float32:
				pLiveData->property.value.fData = pCryData->m_f32;
				break;

			default:
				return eCLE_UserDataTypeMissMatch;
			}
		}
		break;

	default:
		return eCLE_UserDataTypeMissMatch;
	}

	return eCLE_Success;
};

ECryLobbyError LiveUserDataToCryUserData(SLiveUserData* pLiveData, SCryLobbyUserData* pCryData, ECryLobbyUserDataType type)
{
	uint32 liveType = pLiveData->typeContext ? XPROPERTYTYPEFROMID(pLiveData->context.dwContextId) : XPROPERTYTYPEFROMID(pLiveData->property.dwPropertyId);

	switch (liveType)
	{
	case XUSER_DATA_TYPE_CONTEXT:
		{
			pCryData->m_id = pLiveData->context.dwContextId;
			pCryData->m_type = type;

			switch (pCryData->m_type)
			{
			case eCLUDT_Int32:
				pCryData->m_int32	= pLiveData->context.dwValue;
				break;

			case eCLUDT_Int16:
				pCryData->m_int16	= (int16)pLiveData->context.dwValue;
				break;

			case eCLUDT_Int8:
				pCryData->m_int8	= (int8)pLiveData->context.dwValue;
				break;

			default:
				return eCLE_UserDataTypeMissMatch;
			}
		}
		break;

	case XUSER_DATA_TYPE_INT32:
		{
			pCryData->m_id = pLiveData->property.dwPropertyId;
			pCryData->m_type = type;

			switch (pCryData->m_type)
			{
			case eCLUDT_Int32:
				pCryData->m_int32	= pLiveData->property.value.nData;
				break;

			case eCLUDT_Int16:
				pCryData->m_int16	= (int16)pLiveData->property.value.nData;
				break;

			case eCLUDT_Int8:
				pCryData->m_int8	= (int8)pLiveData->property.value.nData;
				break;

			default:
				return eCLE_UserDataTypeMissMatch;
			}
		}
		break;

	case XUSER_DATA_TYPE_INT64:
		{
			pCryData->m_id = pLiveData->property.dwPropertyId;
			pCryData->m_type = type;

			switch (pCryData->m_type)
			{
			case eCLUDT_Int64:
				pCryData->m_int64	= pLiveData->property.value.i64Data;
				break;

			case eCLUDT_Int32:
				pCryData->m_int32	= (int32)pLiveData->property.value.i64Data;
				break;

			case eCLUDT_Int16:
				pCryData->m_int16	= (int16)pLiveData->property.value.i64Data;
				break;

			case eCLUDT_Int8:
				pCryData->m_int8	= (int8)pLiveData->property.value.i64Data;
				break;

			case eCLUDT_Int64NoEndianSwap:
				pCryData->m_int64 = pLiveData->property.value.i64Data;
				SwapEndian(pCryData->m_int64);
				break;

			default:
				return eCLE_UserDataTypeMissMatch;
			}
		}
		break;

	case XUSER_DATA_TYPE_DOUBLE:
		{
			pCryData->m_id = pLiveData->property.dwPropertyId;
			pCryData->m_type = type;

			switch (pCryData->m_type)
			{
			case eCLUDT_Float64:
				pCryData->m_f64	= pLiveData->property.value.dblData;
				break;

			case eCLUDT_Float32:
				pCryData->m_f32	= (f32)pLiveData->property.value.dblData;
				break;

			default:
				return eCLE_UserDataTypeMissMatch;
			}
		}
		break;

	case XUSER_DATA_TYPE_FLOAT:
		{
			pCryData->m_id = pLiveData->property.dwPropertyId;
			pCryData->m_type = type;

			switch (pCryData->m_type)
			{
			case eCLUDT_Float32:
				pCryData->m_f32	= pLiveData->property.value.fData;
				break;

			default:
				return eCLE_UserDataTypeMissMatch;
			}
		}
		break;

	default:
		return eCLE_UserDataTypeMissMatch;
	}

	return eCLE_Success;
}

CCryLiveLobbyService::CCryLiveLobbyService()
{
	m_listener = NULL;
	m_matchmaking = NULL;
	m_telemetry = NULL;
	m_voice = NULL;
	m_pStats = NULL;
}

ECryLobbyError CCryLiveLobbyService::Initialise(CryLobbyServiceCallback cb, CCryLobby* cbArg, ECryLobbyService service)
{
	ECryLobbyError ret;

	m_lobby = cbArg;
	m_service = service;

	ret = CryLiveLobbyGetErrorFromLive(XOnlineStartup());

	if (ret == eCLE_Success)
	{
		m_listener = XNotifyCreateListener(XNOTIFY_SYSTEM | XNOTIFY_LIVE);

		if (!m_listener)
		{
			ret = eCLE_OutOfMemory;
		}
	}

	if (ret == eCLE_Success)
	{
		m_telemetry = new CCryXLSPTelemetry(cbArg);

		if (m_telemetry)
		{
			ret = m_telemetry->Initialise();
		}
		else
		{
			return eCLE_OutOfMemory;
		}
	}

	if (ret == eCLE_Success)
	{
		m_matchmaking = new CCryLiveMatchMaking(cbArg, this);

		if (m_matchmaking)
		{
			ret = m_matchmaking->Initialise();
		}
		else
		{
			ret = eCLE_OutOfMemory;
		}
	}

	if (ret == eCLE_Success)
	{
		m_voice = new CCryLiveVoice(cbArg, this);

		if (m_voice)
		{
			ret = m_voice->Initialise();
		}
		else
		{
			ret = eCLE_OutOfMemory;
		}
	}

	if (ret == eCLE_Success)
	{
		m_pStats = new CCryLiveStats(cbArg, this);

		if (m_pStats)
		{
			ret = m_pStats->Initialise();
		}
		else
		{
			ret = eCLE_OutOfMemory;
		}
	}

	if (ret == eCLE_Success)
	{
		if (cb)
		{
			cb(ret, cbArg, service);
		}
	}
	else
	{
		Terminate(NULL, cbArg, service);
	}

	CryLogAlways("[Lobby] Live Initialise error %d", ret);

	return ret;
}

ECryLobbyError CCryLiveLobbyService::Terminate(CryLobbyServiceCallback cb, CCryLobby* cbArg, ECryLobbyService service)
{
	if (m_listener || m_matchmaking || m_voice || m_telemetry || m_pStats)
	{
		ECryLobbyError ret = eCLE_Success;

		if (m_listener)
		{
			XCloseHandle(m_listener);
			m_listener = NULL;
		}

		if (m_telemetry)
		{
			ECryLobbyError error = m_telemetry->Terminate();

			if (ret == eCLE_Success)
			{
				ret = error;
			}
		}

		if (m_matchmaking)
		{
			ECryLobbyError error = m_matchmaking->Terminate();

			m_matchmaking = NULL;

			if (ret == eCLE_Success)
			{
				ret = error;
			}
		}

		if (m_voice)
		{
			ECryLobbyError error = m_voice->Terminate();

			m_voice = NULL;

			if (ret == eCLE_Success)
			{
				ret = error;
			}
		}

		if (m_pStats)
		{
			ECryLobbyError error = m_pStats->Terminate();

			m_pStats = NULL;

			if (ret == eCLE_Success)
			{
				ret = error;
			}
		}

		if ((ret == eCLE_Success) && cb)
		{
			cb(ret, cbArg, service);
		}

		return ret;
	}
	else
	{
		return eCLE_NotInitialised;
	}
}

void CCryLiveLobbyService::InviteAccepted(uint32 user, CrySessionID id)
{
	m_lobby->InviteAccepted(m_service, user, id);
}

void CCryLiveLobbyService::Tick(CTimeValue tv)
{
	if (m_listener)
	{
		DWORD id;
		ULONG_PTR param;

		if (XNotifyGetNext(m_listener, 0, &id, &param))
		{
			switch (id)
			{
			case XN_LIVE_CONNECTIONCHANGED:
				{
					switch (param)
					{
					case XONLINE_S_LOGON_CONNECTION_ESTABLISHED:
						{
							UCryLobbyEventData eventData;
							SCryLobbyNatTypeData natTypeData;

							eventData.pNatTypeData = &natTypeData;

							switch (XOnlineGetNatType())
							{
							case XONLINE_NAT_OPEN:			natTypeData.m_curState = eNT_Open;			break;
							case XONLINE_NAT_MODERATE:	natTypeData.m_curState = eNT_Moderate;	break;
							case XONLINE_NAT_STRICT:		natTypeData.m_curState = eNT_Strict;		break;
							default:										natTypeData.m_curState = eNT_Unknown;		break;
							}

							if (m_lobby)
								m_lobby->DispatchEvent(eCLSE_NatType, eventData);
						}
						break;
					}
				}
				break;
			case XN_LIVE_LINK_STATE_CHANGED:
				{
					UCryLobbyEventData				eventData;
					SCryLobbyEthernetStateData ether;

					eventData.pEthernetStateData=&ether;
					if (param)
						ether.m_curState=eCS_Connected;
					else
						ether.m_curState=eCS_Unplugged;

					if (m_lobby)
						m_lobby->DispatchEvent(eCLSE_EthernetState,eventData);
				}
				break;
			case XN_SYS_SIGNINCHANGED:
				{
					UCryLobbyEventData				eventData;
					SCryLobbyOnlineStateData		onlineState;

					eventData.pOnlineStateData=&onlineState;
					for (onlineState.m_user=0;onlineState.m_user<MAX_LIVE_LOCAL_USERS;onlineState.m_user++)
					{
						switch(XUserGetSigninState(onlineState.m_user))
						{
						case eXUserSigninState_NotSignedIn:
						case eXUserSigninState_SignedInLocally:
							onlineState.m_curState=eOS_SignedOut;
							break;
						case eXUserSigninState_SignedInToLive:
							onlineState.m_curState=eOS_SignedIn;
							break;
						default:
							onlineState.m_curState=eOS_Unknown;
							break;
						}
						if (m_lobby)
							m_lobby->DispatchEvent(eCLSE_OnlineState,eventData);
					}
				}
				break;
			case XN_LIVE_INVITE_ACCEPTED:
				{
					XINVITE_INFO InviteInfo;

					if (XInviteGetAcceptedInfo(param, &InviteInfo) == ERROR_SUCCESS)
					{
						SCryLiveSessionID* sessionID = new SCryLiveSessionID;

						sessionID->m_info = InviteInfo.hostInfo;
						// Game type will always be standard as can't invite to ranked sessions.
						sessionID->m_gameType = X_CONTEXT_GAME_TYPE_STANDARD;
						sessionID->m_fromInvite = InviteInfo.fFromGameInvite;

						TO_GAME(&CCryLiveLobbyService::InviteAccepted, this, (uint32)param, CrySessionID(sessionID));
					}

					break;
				}

			case XN_SYS_MUTELISTCHANGED:
				if (m_voice)
				{
					m_voice->UpdateMuteList();
				}

				break;
			}
		}
	}

	if (m_matchmaking)
	{
		m_matchmaking->Tick(tv);
	}
	
	if (m_telemetry)
	{
		m_telemetry->Tick(tv);
	}

	if (m_voice)
	{
		m_voice->Tick(tv);
	}

	if (m_pStats)
	{
		m_pStats->Tick(tv);
	}
}

void CCryLiveLobbyService::OnPacket(const TNetAddress& addr, const uint8* data, uint32 length)
{
	if (m_matchmaking)
	{
		m_matchmaking->OnPacket(addr, data, length);
	}

	if (m_voice)
	{
		m_voice->OnPacket(addr, data, length);
	}
}

void CCryLiveLobbyService::OnError(const TNetAddress& addr, ESocketError error, CryLobbySendID sendID)
{
	if (m_matchmaking)
	{
		m_matchmaking->OnError(addr, error, sendID);
	}
}

void CCryLiveLobbyService::OnSendComplete(const TNetAddress& addr, CryLobbySendID sendID)
{
	if (m_matchmaking)
	{
		m_matchmaking->OnSendComplete(addr, sendID);
	}
}

ECryLobbyError CCryLiveLobbyService::GetUserXUID(uint32 user, XUID* pXUID)
{
	XUSER_SIGNIN_INFO info;
	ECryLobbyError error = CryLiveLobbyGetErrorFromLive(XUserGetSigninInfo(user, XUSER_GET_SIGNIN_INFO_ONLINE_XUID_ONLY, &info));

	assert(pXUID);

	if (error == eCLE_Success)
	{
		*pXUID = info.xuid;
	}

	return error;
}

CryUserID CCryLiveLobbyService::GetUserID(uint32 user)
{
	XUSER_SIGNIN_INFO info;

	ECryLobbyError error = CryLiveLobbyGetErrorFromLive(XUserGetSigninInfo(user, XUSER_GET_SIGNIN_INFO_ONLINE_XUID_ONLY, &info));

	if (error == eCLE_Success)
	{
		SCryLiveUserID* pID = new SCryLiveUserID;

		if (pID)
		{
			pID->xuid = info.xuid;
			return pID;
		}
	}

	return CryUserInvalidID;
}


#if USE_GFWL
CCryLobbyGFWLExtras::CCryLobbyGFWLExtras()
{
	XLIVE_INITIALIZE_INFO xii = {0};

	xii.cbSize  = sizeof(xii);
	xii.pD3D    = NULL;
	xii.pD3DPP  = NULL;
	xii.langID  = GetUserDefaultLangID();

	HRESULT res = XLiveInitialize(&xii);

	m_initialised = res == S_OK;
}

void CCryLobbyGFWLExtras::OnCreateDevice(void* pD3D, void* pD3DPP)
{
	if (m_initialised)
	{
		XLiveOnCreateDevice((IUnknown*)pD3D, pD3DPP);
	}
}

void CCryLobbyGFWLExtras::OnDeviceReset(void* pD3DPP)
{
	if (m_initialised)
	{
		XLiveOnResetDevice(pD3DPP);
	}
}

void CCryLobbyGFWLExtras::OnDeviceDestroyed()
{
	if (m_initialised)
	{
		XLiveOnDestroyDevice();
	}
}

void CCryLobbyGFWLExtras::Render()
{
	if (m_initialised)
	{
		XLiveRender();
	}
}

uint32 CCryLobbyGFWLExtras::PreTranslateMessage(void* msg)
{
	if (m_initialised)
	{
		return XLivePreTranslateMessage((MSG*)msg);
	}

	return FALSE;
}
#endif

