
#include "StdAfx.h"

#include "CryPSNLobby.h"

#if USE_PSN

#include <netex/net.h>
#include <netex/libnetctl.h>

CCryPSNLobbyService::CCryPSNLobbyService()
{
#if USE_PSN_VOICE
	m_voice = NULL;
#endif//USE_PSN_VOICE
	m_matchmaking = NULL;
	m_NPInited=false;
	m_telemetry = NULL;
}

ECryLobbyError CCryPSNLobbyService::Initialise(CryLobbyServiceCallback cb, CCryLobby* cbArg, ECryLobbyService service)
{
	ECryLobbyError error;

	m_lobby = cbArg;
	m_service = service;

	if (!m_NPInited)
	{
		int ret = cellNetCtlInit();
		if ((ret < 0) && (ret!=CELL_NET_CTL_ERROR_NOT_TERMINATED))
		{
			NetLog("cellNetCtlInit() failed. ret = 0x%x\n", ret);
			return eCLE_InternalError;
		}

		ret = sceNp2Init(SCE_NP_MIN_POOL_SIZE, m_NPPool);
		if (ret < 0) 
		{
			NetLog("sceNpInit() failed. ret = 0x%x\n", ret);
			return eCLE_InternalError;
		}

		ret = sceNpMatching2Init2(0, 0, NULL);
		if (ret < 0) 
		{
			NetLog("sceNpMatching2Init() failed. ret = 0x%x\n", ret);
			return eCLE_InternalError;
		}

		m_NPInited=true;
	}

	m_telemetry = new CCryTelemetry(cbArg);

	if (m_telemetry)
	{
		error = m_telemetry->Initialise();
	}
	else
	{
		return eCLE_OutOfMemory;
	}

#if USE_PSN_VOICE
	if (error == eCLE_Success)
	{
		m_voice = new CCryPSNVoice(cbArg,this);

		if (m_voice)
		{
			error = m_voice->Initialise();
		}
		else
		{
			return eCLE_OutOfMemory;
		}
	}
#endif

	if (error == eCLE_Success)
	{
		m_matchmaking = new CCryPSNMatchMaking(cbArg,this);

		if (m_matchmaking)
		{
			error = m_matchmaking->Initialise();
		}
		else
		{
			return eCLE_OutOfMemory;
		}
	}

	if ((error == eCLE_Success))
	{
		if (cb)
		{
			cb(error, cbArg, service);
		}
	}
	else
	{
		Terminate(NULL,cbArg,service);
	}

	return error;
}

ECryLobbyError CCryPSNLobbyService::Terminate(CryLobbyServiceCallback cb, CCryLobby* cbArg, ECryLobbyService service)
{
	if (m_matchmaking || m_telemetry
#if USE_PSN_VOICE
		|| m_voice
#endif
		 )
	{
		ECryLobbyError ret = eCLE_Success;

		if (m_telemetry)
		{
			ECryLobbyError error = m_telemetry->Terminate();

			if (ret == eCLE_Success)
			{
				ret = error;
			}
		}

#if USE_PSN_VOICE
		if (m_voice)
		{
			ECryLobbyError error = m_voice->Terminate();

			m_voice = NULL;

			if (ret == eCLE_Success)
			{
				ret = error;
			}
		}
#endif

		if (m_matchmaking)
		{
			ECryLobbyError error = m_matchmaking->Terminate();

			m_matchmaking = NULL;

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

void CCryPSNLobbyService::InviteAccepted(uint32 user, CrySessionID id)
{
	m_lobby->InviteAccepted(m_service, user, id);
}

void CCryPSNLobbyService::Tick(CTimeValue tv)
{
	if (m_matchmaking)
	{
		m_matchmaking->Tick(tv);
	}
#if USE_PSN_VOICE
	if (m_voice)
	{
		m_voice->Tick(tv);
	}
#endif
	if (m_telemetry)
	{
		m_telemetry->Tick(tv);
	}
}

void CCryPSNLobbyService::OnPacket(const TNetAddress& addr, const uint8* data, uint32 length)
{
	if (m_matchmaking)
	{
		m_matchmaking->OnPacket(addr, data, length);
	}
}

void CCryPSNLobbyService::OnError(const TNetAddress& addr, ESocketError error, CryLobbySendID sendID)
{
	if (m_matchmaking)
	{
		m_matchmaking->OnError(addr, error, sendID);
	}
}

void CCryPSNLobbyService::OnSendComplete(const TNetAddress& addr, CryLobbySendID sendID)
{
	if (m_matchmaking)
	{
		m_matchmaking->OnSendComplete(addr, sendID);
	}
}
	
void CCryPSNLobbyService::GetConfigurationInformation(SConfigurationParams* infos,uint32 cnt)
{
	m_lobby->GetConfigurationInformation(infos,cnt);
}

#endif // USE_PSN