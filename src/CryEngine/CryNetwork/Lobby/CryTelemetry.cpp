#include "StdAfx.h"
#include "CryTelemetry.h"
#include "Network.h"





CCryTelemetry::CCryTelemetry(CCryLobby* lobby)
{
	assert(lobby);

	m_socket = INVALID_SOCKET;
	m_pLobby = lobby;
	m_pResolvedAddr=NULL;
}

CCryTelemetry::~CCryTelemetry()
{
	Terminate();
	SAFE_DELETE(m_pResolvedAddr);
}

ECryLobbyError CCryTelemetry::Initialise()
{
	return eCLE_Success;
}

void CCryTelemetry::DiscardQueueElement()
{
	m_dataQueue.pop();
}

ECryLobbyError CCryTelemetry::Terminate()
{
	CloseConnection();

	m_queueMutex.Lock();

	while (!m_dataQueue.empty())
	{
		DiscardQueueElement();
	}

	m_queueMutex.Unlock();

	return eCLE_Success;
}

ESocketError CCryTelemetry::GetAddress(sockaddr_in& addr)
{
	if (m_pResolvedAddr)
	{
		addr.sin_family = AF_INET;

		if (ConvertAddr(*m_pResolvedAddr,&addr))
		{
			return eSE_Ok;
		}
	}

	return eSE_MiscFatalError;
}

ESocketError CCryTelemetry::Connect()
{
	sockaddr_in addr;
	ESocketError ret = GetAddress(addr);

	if (ret!=eSE_Ok)
	{
		return ret;
	}

	m_socket = ::socket(AF_INET, SOCK_STREAM, NULL);
	if (m_socket < 0 || m_socket == INVALID_SOCKET)
	{
		return eSE_MiscFatalError;
	}

	const int yes = 1;
	if (::setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR,(const char *)&yes, sizeof(int)) < 0)
	{
		::closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return eSE_MiscFatalError;
	}

	// TCP_NODELAY required for ps3 because of high latency connection otherwise
#if defined(PS3) || defined(WIN32)
	if (::setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY,	(const char *)&yes, sizeof(int)) < 0)
	{
		::closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return eSE_MiscFatalError;
	}
#endif 

	if (::connect(m_socket, (sockaddr *)&addr, sizeof(addr)) < 0)
	{
		::closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return eSE_MiscFatalError;
	}

	return eSE_Ok;
}

ESocketError CCryTelemetry::Send(STelemetryDataPtr pData)
{
	if (m_socket!=INVALID_SOCKET)
	{
		int		sent=0;

		while (sent<pData->length)
		{
			int r = ::send(m_socket,(const char*)(pData->pData+sent),pData->length-sent,NULL);
			if (r<0)
			{
				return eSE_MiscFatalError;
			}
			sent+=r;
		}
		return eSE_Ok;
	}

	return eSE_MiscFatalError;
}

ESocketError CCryTelemetry::CloseConnection()
{
	if (m_socket != INVALID_SOCKET)
	{
		::closesocket(m_socket);
		m_socket=INVALID_SOCKET;
	}
	return eSE_Ok;
}
					
void CCryTelemetry::Tick(CTimeValue tv)
{
	if (m_queueMutex.TryLock())
	{
		while(!m_dataQueue.empty())
		{
			ESocketError result = Connect();			// don't care about result for now

			STelemetryDataPtr element = m_dataQueue.front();

			result = Send(element);

			if (element->telemCb)
			{
				element->telemCb(result == eSE_Ok ? eCTR_Ok : eCTR_Failed, element->userArg);
			}

			DiscardQueueElement();

			CloseConnection();
		}

		m_queueMutex.Unlock();
	}
}

bool CCryTelemetry::ConfigureRemoteServer(const char *url, uint16 port)
{
	// resolve URL and store
	SAFE_DELETE(m_pResolvedAddr);

	CNameRequestPtr pNR = RESOLVER.RequestNameLookup(url);
	pNR->Wait();
	TNetAddressVec nav;
	if ( eNRR_Succeeded == pNR->GetResult(nav) )
	{
		SIPv4Addr* pIpv4Addr = NULL;
		size_t naddrs = nav.size();
		for (size_t i = 0; i < naddrs; ++i)
		{
			TNetAddress addr = nav[i];
			pIpv4Addr = addr.GetPtr<SIPv4Addr>();
			if (pIpv4Addr)
			{
				pIpv4Addr->port = port;
				break;
			}
		}
		if (pIpv4Addr)
		{
			m_pResolvedAddr = new TNetAddress(*pIpv4Addr);

			if (m_pResolvedAddr)
				return true;
		}
	}
	return false;
}

bool CCryTelemetry::UploadData(STelemetryDataPtr pData)
{
	if (pData->pData)
	{
		m_queueMutex.Lock();
		{
			m_dataQueue.push(pData);
		}
		m_queueMutex.Unlock();

		return true;
	}

	return false;
}
