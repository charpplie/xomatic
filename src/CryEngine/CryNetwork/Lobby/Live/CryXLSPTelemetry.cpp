#include "StdAfx.h"
#include "CryXLSPTelemetry.h"
#include "Network.h"

CCryXLSPTelemetry::CCryXLSPTelemetry(CCryLobby* pLobby) : CCryTelemetry(pLobby)
{
	SConfigurationParams	neededInfo[2] = {{'XSvc',{NULL}},{'XPor',{NULL}}};

	pLobby->GetConfigurationInformation(neededInfo,2);

	m_serviceId = neededInfo[0].m_32;
	m_servicePort = neededInfo[1].m_16;
}

CCryXLSPTelemetry::~CCryXLSPTelemetry()
{
	Terminate();
}

void CCryXLSPTelemetry::CloseServerHandle()
{
	CloseHandle(m_serverEnum);
	m_serverEnum=INVALID_HANDLE_VALUE;
}

ECryLobbyError CCryXLSPTelemetry::Initialise()
{
	ZeroMemory(m_pServers, sizeof(m_pServers));
  ZeroMemory(&m_secureAddr, sizeof(m_secureAddr));
	ZeroMemory(&m_overlapped, sizeof(m_overlapped));

	m_serverCount=0;
	m_bufferSize=0;
	m_serverEnum=INVALID_HANDLE_VALUE;
	m_chosenServer=MAX_XLSP_SERVERS;

	m_state = ECXLSPTS_None;

	return CCryTelemetry::Initialise();
}

ECryLobbyError CCryXLSPTelemetry::Terminate()
{
	if (m_state == ECXLSPTS_SecureConnectionValid)
	{
		XNetUnregisterInAddr(m_secureAddr);
		ZeroMemory( &m_secureAddr, sizeof( m_secureAddr ) );
	}

	if (m_serverEnum != INVALID_HANDLE_VALUE)
	{
		CloseServerHandle();
	}

	return CCryTelemetry::Terminate();
}

ESocketError CCryXLSPTelemetry::GetAddress(sockaddr_in& addr)
{
	if (IsAbleToSend())
	{
		addr.sin_family = AF_INET;
		addr.sin_addr = m_secureAddr;
		addr.sin_port = htons(m_servicePort);

		return eSE_Ok;
	}

	return eSE_MiscFatalError;
}

ESocketError CCryXLSPTelemetry::Connect()
{
	if (IsAbleToSend())
	{
		ESocketError ret = CCryTelemetry::Connect();

		if (ret!=eSE_Ok)
		{
			m_state = ECXLSPTS_ChooseServer;
		}

		return ret;
	}
	return eSE_MiscFatalError;
}

ESocketError CCryXLSPTelemetry::Send(STelemetryDataPtr pData)
{
	if (IsAbleToSend())
	{
		ESocketError ret = CCryTelemetry::Send(pData);

		if (ret!=eSE_Ok)
		{
			m_state = ECXLSPTS_ChooseServer;
		}

		return ret;
	}
	return eSE_MiscFatalError;
}

ESocketError CCryXLSPTelemetry::CloseConnection()
{
	if (IsAbleToSend())
	{
		ESocketError ret = CCryTelemetry::CloseConnection();

		if (ret!=eSE_Ok)
		{
			m_state = ECXLSPTS_ChooseServer;
		}

		return ret;
	}
	return eSE_MiscFatalError;
}
					
bool CCryXLSPTelemetry::EnumerateServers()
{
	if (m_serverEnum == INVALID_HANDLE_VALUE)
	{
		DWORD dwResult = XTitleServerCreateEnumerator( NULL, MAX_XLSP_SERVERS, &m_bufferSize, &m_serverEnum);
		if( ERROR_SUCCESS != dwResult )
		{
			CryLogAlways("XTitleServerCreateEnumerator : Error %08X",dwResult);
			return false;
		}

		dwResult = XEnumerate( m_serverEnum, m_pServers, sizeof( m_pServers ), NULL, &m_overlapped );
		if( ERROR_IO_PENDING != dwResult )
		{
			CryLogAlways("XEnumerate : Error %08X",dwResult);
			CloseServerHandle();
			return false;
		}

		return true;
	}
	CloseServerHandle();
	return false;
}

bool CCryXLSPTelemetry::WaitForServers()
{
	if (XHasOverlappedIoCompleted(&m_overlapped))
	{
		DWORD ret = XGetOverlappedExtendedError(&m_overlapped);

		switch (ret)
		{
		case ERROR_IO_INCOMPLETE:
		case ERROR_IO_PENDING:
			return false;

		case ERROR_SUCCESS:
			m_serverCount = m_overlapped.InternalHigh;
			break;
		default:
			m_serverCount = 0;
			CryLogAlways("XOverlappedExtendedError : Error %08X",ret);
			break;
		}
		CloseServerHandle();

		return true;
	}

	return false;
}

bool CCryXLSPTelemetry::ChooseServer()
{
	if (m_serverCount == 0)
	{
		return false;
	}

	m_chosenServer = Random((uint32)m_serverCount);

	return true;
}

bool CCryXLSPTelemetry::OpenSecureConnection(CTimeValue &tv)
{
	DWORD res = XNetServerToInAddr( m_pServers[m_chosenServer].inaServer, m_serviceId, &m_secureAddr);

	if (res != 0)
	{
		CryLogAlways("XNetServerToInAddr : Error %08X",res);
		return false;
	}

	res = XNetConnect(m_secureAddr);
	if (res != 0)
	{
		CryLogAlways("XNetConnect : Error %08X",res);
		return false;
	}

	m_connectionTimer = tv;

	return true;
}

bool CCryXLSPTelemetry::WaitSecureConnection(CTimeValue &tv,DWORD &error)
{
	DWORD res = XNetGetConnectStatus( m_secureAddr );

	if (res == XNET_CONNECT_STATUS_PENDING)
	{
		if ((tv - m_connectionTimer).GetMilliSecondsAsInt64() < MAX_SECURE_CONNECT_TIME)
		{
			return false;
		}
		else
		{
			res = XNET_CONNECT_STATUS_LOST;
		}
	}

	error = res;

	return true;
}

void CCryXLSPTelemetry::RemoveCurrentServer()
{
	if ((m_chosenServer < MAX_XLSP_SERVERS) && (m_serverCount>0))
	{
		int32 remainingServers = m_serverCount - m_chosenServer - 1;

		if (remainingServers>0)
		{
			memmove(&m_pServers[m_chosenServer],&m_pServers[m_chosenServer+1],remainingServers * sizeof(m_pServers[m_chosenServer]));
		}

		m_chosenServer=MAX_XLSP_SERVERS;
		m_serverCount--;
	}
}

void CCryXLSPTelemetry::Tick(CTimeValue tv)
{
	DWORD error;

	CCryTelemetry::Tick(tv);

	switch (m_state)
	{
	case ECXLSPTS_None:
		if (EnumerateServers())
		{
			m_state = ECXLSPTS_FindServers;
		}
		break;
	case ECXLSPTS_FindServers:
		if (WaitForServers())
		{
			m_state = ECXLSPTS_ChooseServer;
		}
		break;
	case ECXLSPTS_ChooseServer:
		if (ChooseServer())
		{
			m_state = ECXLSPTS_OpenSecureConnection;
		}
		else
		{
			m_state = ECXLSPTS_None;
		}
		break;
	case ECXLSPTS_OpenSecureConnection:
		if (OpenSecureConnection(tv))
		{
			m_state = ECXLSPTS_WaitingSecureConnection;
		}
		else
		{
			RemoveCurrentServer();
			m_state = ECXLSPTS_ChooseServer;
		}
		break;
	case ECXLSPTS_WaitingSecureConnection:
		if (WaitSecureConnection(tv,error))
		{
			if (error != XNET_CONNECT_STATUS_CONNECTED)
			{
				CryLogAlways("Failed to achieve connection to secure server : Error %08X",error);
				RemoveCurrentServer();
				m_state = ECXLSPTS_ChooseServer;
			}
			else
			{
				m_state = ECXLSPTS_SecureConnectionValid;
			}
		}
		break;
	case ECXLSPTS_SecureConnectionValid:
		break;
	}
}
