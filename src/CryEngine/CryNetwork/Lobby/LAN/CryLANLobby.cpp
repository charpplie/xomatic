
#include "StdAfx.h"
#include "CryLANLobby.h"

ECryLobbyError CCryLANLobbyService::Initialise(CryLobbyServiceCallback cb, CCryLobby* cbArg, ECryLobbyService service)
{
	ECryLobbyError ret = eCLE_Success;

	m_telemetry = new CCryTelemetry(cbArg);

	if (m_telemetry)
	{
		ret = m_telemetry->Initialise();
	}
	else
	{
		return eCLE_OutOfMemory;
	}

	if (ret == eCLE_Success)
	{
		m_matchmaking = new CCryLANMatchMaking(cbArg, this);

		if (m_matchmaking)
		{
			ret = m_matchmaking->Initialise();

			if ((ret == eCLE_Success) && cb)
			{
				cb(ret, cbArg, service);
			}
		}
		else
		{
			return eCLE_OutOfMemory;
		}
	}

	return ret;
}

ECryLobbyError CCryLANLobbyService::Terminate(CryLobbyServiceCallback cb, CCryLobby* cbArg, ECryLobbyService service)
{
	if (m_telemetry || m_matchmaking)
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

		if (m_matchmaking)
		{
			ECryLobbyError error = m_matchmaking->Terminate();

			if (ret == eCLE_Success)
			{
				ret = error;
			}
		}

		if ((ret == eCLE_Success) && cb)
		{
			cb(ret, cbArg, service);
		}

	}

	return eCLE_NotInitialised;
}

void CCryLANLobbyService::Tick(CTimeValue tv)
{
	if (m_matchmaking)
	{
		m_matchmaking->Tick(tv);
	}
	if (m_telemetry)
	{
		m_telemetry->Tick(tv);
	}
}

void CCryLANLobbyService::OnPacket(const TNetAddress& addr, const uint8* data, uint32 length)
{
	if (m_matchmaking)
	{
		m_matchmaking->OnPacket(addr, data, length);
	}
}

void CCryLANLobbyService::OnError(const TNetAddress& addr, ESocketError error, CryLobbySendID sendID)
{
	if (m_matchmaking)
	{
		m_matchmaking->OnError(addr, error, sendID);
	}
}

void CCryLANLobbyService::OnSendComplete(const TNetAddress& addr, CryLobbySendID sendID)
{
	if (m_matchmaking)
	{
		m_matchmaking->OnSendComplete(addr, sendID);
	}
}
