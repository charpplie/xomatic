#include "StdAfx.h"
#include "SocketIOManager360.h"

#ifdef HAS_SOCKETIOMANAGER_360

#include "Network.h"

CSocketIOManager360::CSocketIOManager360() : ISocketIOManager(eSIOMC_SupportsBackoff)
{
	m_userEvent = WSACreateEvent();
	for (int i=0; i<MAX_SOCKETS; i++)
	{
		SSocket& sk = m_sockets[i];
		sk.inUse = false;
		sk.salt = 100;
		sk.evtSend = WSACreateEvent();
		sk.evtRecvFrom = WSACreateEvent();
	}
}

bool CSocketIOManager360::Init()
{
	return true;
}

const char * CSocketIOManager360::GetName()
{
	return "X360";
}

int CSocketIOManager360::Poll( float waitTime, bool& performedWork )
{
	WSAEVENT events[WSA_MAXIMUM_WAIT_EVENTS];
	SAction actions[WSA_MAXIMUM_WAIT_EVENTS];

	m_watchdog.ClearStalls();
	
	while (true)
	{
		DWORD nEvents = FillWaitActions(events, actions);
		if (!nEvents)
			return 1;

		NET_ASSERT(nEvents < WSA_MAXIMUM_WAIT_EVENTS);

		DWORD rwait = WaitForMultipleObjectsEx( nEvents, events, false, DWORD(waitTime+0.5f), FALSE );
		if (rwait == WSA_WAIT_TIMEOUT)
		{
			return 1;
		}
		else if (rwait == WAIT_IO_COMPLETION)
		{
			NET_ASSERT(false);
			return 1;
		}
		else if (WSA_WAIT_EVENT_0 <= rwait && rwait < WSA_WAIT_EVENT_0+nEvents)
		{
			const SAction& a = actions[rwait - WSA_WAIT_EVENT_0];
			int ar = (this->*(a.action))(a.socket);
			if (ar != 0)
				return ar;
		}
		else
		{
			NET_ASSERT(false);
		}
	}
}

void CSocketIOManager360::AddWaitAction( WSAEVENT * pEvents, SAction * pActions, DWORD * pCount, WSAEVENT evt, SSocketID socket, FinishActionFunc func )
{
	assert(0 <= socket.id && socket.id < MAX_SOCKETS);
	pEvents[*pCount] = evt;
	pActions[*pCount].socket = socket;
	pActions[*pCount].action = func;
	++*pCount;
}

DWORD CSocketIOManager360::FillWaitActions( WSAEVENT * pEvents, SAction * pActions )
{
	SCOPED_GLOBAL_LOCK;
	DWORD nEvents = 0;
	AddWaitAction(pEvents, pActions, &nEvents, m_userEvent, SSocketID(0, 0), &CSocketIOManager360::FinishUser);
	for (int i=0; i<MAX_SOCKETS; i++)
	{
		SSocket& sk = m_sockets[i];
		if (!sk.inUse)
			continue;
		if (sk.outstandingSend)
			AddWaitAction(pEvents, pActions, &nEvents, sk.evtSend, SSocketID(i, m_sockets[i].salt), &CSocketIOManager360::FinishSend);
		AddWaitAction(pEvents, pActions, &nEvents, sk.evtRecvFrom, SSocketID(i, m_sockets[i].salt), &CSocketIOManager360::FinishRecv);
	}
	return nEvents;
}

SSocketID CSocketIOManager360::RegisterSocket( SOCKET sock, int protocol )
{
	SCOPED_GLOBAL_LOCK;
	for (int i=0; i<MAX_SOCKETS; i++)
	{
		SSocket& sk = m_sockets[i];

		if (sk.inUse || !sk.queuedSends.empty())
			continue;

		SSocketID id(i, ++sk.salt);
		sk.inUse = true;
		sk.sock = sock;
		sk.protocol = protocol;
		sk.outstandingSend = false;
		sk.pSendToTarget = 0;
		return id;
	}

	return SSocketID();
}

void CSocketIOManager360::UnregisterSocket( SSocketID sockid )
{
	SCOPED_GLOBAL_LOCK;
	if (SSocket * pSock = GetSocket(sockid))
		pSock->inUse = false;
}

CSocketIOManager360::SSocket * CSocketIOManager360::GetSocket( SSocketID sock )
{
	ASSERT_GLOBAL_LOCK;
	if (sock.id >= MAX_SOCKETS)
		return NULL;
	SSocket& sk = m_sockets[sock.id];
	if (!sk.inUse)
		return NULL;
	if (sk.salt != sock.salt)
		return NULL;
	return &sk;
}

bool CSocketIOManager360::RequestSendTo( SSocketID sockid, const TNetAddress& addr, const uint8 * pData, size_t len )
{
	SCOPED_GLOBAL_LOCK;
	SSocket * pSock = GetSocket(sockid);

	if (pSock)
	{
		SQueuedSend * pST = new SQueuedSend;

		if (pST)
		{
			pSock->queuedSends.push(pST);

			if (pSock->protocol == IPPROTO_VDP)
			{
				// With VDP the first 2 bytes contain the size of the encrypted data section then the encrypted game data then the unencrypted voice data.
				assert((len > 0) && ((len + 2) <= MAX_UDP_PACKET_SIZE));

				// This send is for encrypted game data
				pST->data[0] = (len>>8)&0xff;
				pST->data[1] = len&0xff;
				memcpy(pST->data + 2, pData, len);
				pST->len = len + 2;
			}
			else
			{
				assert((len > 0) && (len <= MAX_UDP_PACKET_SIZE));
				memcpy(pST->data, pData, len);
				pST->len = len;
			}

#if NET_MINI_PROFILE
	g_socketBandwidth.totalBandwidthSent += (len + UDP_HEADER_SIZE) * 8;
	g_socketBandwidth.totalNumPackets++;
#endif

			pST->addr = addr;

			if (pSock->outstandingSend)
			{
				return true;
			}
			else
			{
				return DoSend(*pSock) != eSR_Fail;
			}
		}
	}

	return false;
}

bool CSocketIOManager360::RequestSendVoiceTo( SSocketID sockid, const TNetAddress& addr, const uint8 * pData, size_t len )
{
	SCOPED_GLOBAL_LOCK;
	SSocket * pSock = GetSocket(sockid);

	if (pSock)
	{
		SQueuedSend * pST = new SQueuedSend;

		if (pST)
		{
			pSock->queuedSends.push(pST);

			if (pSock->protocol == IPPROTO_VDP)
			{
				// With VDP the first 2 bytes contain the size of the encrypted data section then the encrypted game data then the unencrypted voice data.
				assert((len > 0) && ((len + 2) <= MAX_UDP_PACKET_SIZE));

				// This send is for unencrypted voice data
				pST->data[0] = 0;
				pST->data[1] = 0;
				memcpy(pST->data + 2, pData, len);
				pST->len = len + 2;
			}
			else
			{
				assert((len > 0) && (len <= MAX_UDP_PACKET_SIZE));
				memcpy(pST->data, pData, len);
				pST->len = len;
			}

			pST->addr = addr;

			if (pSock->outstandingSend)
			{
				return true;
			}
			else
			{
				return DoSend(*pSock) != eSR_Fail;
			}
		}
	}

	return false;
}

int CSocketIOManager360::FinishSend( SSocketID sockID )
{
	SCOPED_GLOBAL_LOCK;
	SSocket& sock = m_sockets[sockID.id];

	if (sock.salt == sockID.salt)
	{
		if (!sock.inUse)
		{
			// Socket is no longer in use and no sends are in progress so safe to delete send queue
			while (!sock.queuedSends.empty())
			{
				delete sock.queuedSends.front();
				sock.queuedSends.pop();
			}
			return 0;
		}

		assert(sock.outstandingSend);
		sock.outstandingSend = false;

		// An asynchronous send has completed remove and delete the data
		delete sock.queuedSends.front();
		sock.queuedSends.pop();

		while (!sock.queuedSends.empty())
		{
			ESendResult sr = DoSend(sock);
			switch (sr)
			{
			case eSR_Fail:
				// An error occurred and error handling has been done so try next send
				break;
			case eSR_Ok_Continue:
				// Send was synchronous so try next send
				break;
			case eSR_Ok_Return:
				// Send was asynchronous so return and wait for send to finish
				return 0;
			}
		}
	}

	return 0;
}

int CSocketIOManager360::FinishRecv( SSocketID sockID )
{
	SCOPED_GLOBAL_LOCK;
	SSocket* sock = GetSocket(sockID);

	if (sock)
	{
		DWORD transferred;
		DWORD flags;
		BOOL r = WSAGetOverlappedResult(sock->sock, &sock->recvOverlapped, &transferred, FALSE, &flags);
		InformRecvFrom(!r, *sock, transferred);
		QueueRecvFrom(*sock);
	}

	return 0;
}

int CSocketIOManager360::FinishUser( SSocketID sockID )
{
	SCOPED_GLOBAL_LOCK;
	if (!m_userMessages.empty())
	{
		int out = m_userMessages.front();
		m_userMessages.pop();
		if (m_userMessages.empty())
			ResetEvent(m_userEvent);
		return out;
	}
	return 0;
}

CSocketIOManager360::ESendResult CSocketIOManager360::DoSend( SSocket& sock )
{
	SCOPED_GLOBAL_LOCK;
	WSABUF buf;
	SQueuedSend * pQS = sock.queuedSends.front();

	buf.buf = (char*)pQS->data;
	buf.len = pQS->len;

	DWORD bytesSent = 0;
	uint8 addrBuf[_SS_MAXSIZE];
	int addrLen = sizeof(addrBuf);

	if (!ConvertAddr(pQS->addr, (sockaddr*)addrBuf, &addrLen))
	{
		// Can't send remove and delete data
		sock.queuedSends.pop();
		delete pQS;
		return eSR_Fail;
	}

	ZeroMemory(&sock.sendOverlapped, sizeof(sock.sendOverlapped));
	sock.sendOverlapped.hEvent = sock.evtSend;
	int r = WSASendTo( sock.sock, &buf, 1, &bytesSent, 0, (sockaddr*)addrBuf, addrLen, &sock.sendOverlapped, NULL );
	if (r == 0)
	{
		// Send completed synchronously remove and delete data
		sock.queuedSends.pop();
		delete pQS;
		return eSR_Ok_Continue;
	}

	int err = WSAGetLastError();
	switch (err)
	{
	case WSA_IO_PENDING:
		// Send has started asynchronously remove and delete data after send is complete
		sock.outstandingSend = true;
		return eSR_Ok_Return;
	default:
		sock.pSendToTarget->OnSendToException(pQS->addr, OSErrorToSocketError(err));
		// Error occurred remove and delete data
		sock.queuedSends.pop();
		delete pQS;
		return eSR_Fail;
	}
}

bool CSocketIOManager360::RequestRecvFrom( SSocketID sockid )
{
	return true;
}

bool CSocketIOManager360::RequestConnect( SSocketID sockid, const TNetAddress& addr )
{
	NET_ASSERT(!"implemented");
	return false;
}

bool CSocketIOManager360::RequestAccept( SSocketID sock )
{
	NET_ASSERT(!"implemented");
	return false;
}

bool CSocketIOManager360::RequestSend( SSocketID sockid, const uint8 * pData, size_t len )
{
	NET_ASSERT(!"implemented");
	return false;
}

bool CSocketIOManager360::RequestRecv( SSocketID sockid )
{
	NET_ASSERT(!"implemented");
	return false;
}

void CSocketIOManager360::RegisterBackoffAddressForSocket( TNetAddress addr, SSocketID sockid )
{
	if (SSocket * pSock = GetSocket(sockid))
		m_watchdog.RegisterTarget(pSock->sock, addr);
}

void CSocketIOManager360::UnregisterBackoffAddressForSocket( TNetAddress addr, SSocketID sockid )
{
	if (SSocket * pSock = GetSocket(sockid))
		m_watchdog.UnregisterTarget(pSock->sock, addr);
}

bool CSocketIOManager360::HasPendingData()
{
	return m_watchdog.HasStalled();
}

void CSocketIOManager360::SetRecvFromTarget( SSocketID sockid, IRecvFromTarget * pTarget )
{
	SSocket * pSock = GetSocket(sockid);
	pSock->pRecvFromTarget = pTarget;
	QueueRecvFrom(*pSock);
}

void CSocketIOManager360::QueueRecvFrom( SSocket& sock )
{
	while (true)
	{
		WSABUF buf;
		buf.buf = (char*) sock.recvBuf;
		DWORD recvLen = sizeof(sock.recvBuf);
		DWORD flags = 0;
		buf.len = recvLen;
		sock.fromLen = sizeof(sock.fromBuf);
		ResetEvent(sock.evtRecvFrom);

		ZeroMemory(&sock.recvOverlapped, sizeof(sock.recvOverlapped));
		sock.recvOverlapped.hEvent = sock.evtRecvFrom;

		int r = WSARecvFrom( sock.sock, &buf, 1, &recvLen, &flags, (sockaddr*) sock.fromBuf, &sock.fromLen, &sock.recvOverlapped, NULL );
		if (InformRecvFrom(r, sock, recvLen))
			return;
	}
}

bool CSocketIOManager360::InformRecvFrom( int r, SSocket& sock, DWORD transferred )
{
	if (0 == r)
	{
		if (sock.protocol == IPPROTO_VDP)
		{
			// With VDP the first 2 bytes contain the size of the encrypted data section then the encrypted game data then the unencrypted voice data.
			uint32 dataLen = (sock.recvBuf[0]<<8) | sock.recvBuf[1];
			uint32 voiceLen = transferred - (dataLen + 2);

			if (dataLen > 0)
			{
				// Process encrypted game data.
				sock.pRecvFromTarget->OnRecvFromComplete( ConvertAddr((sockaddr*)sock.fromBuf, sock.fromLen), sock.recvBuf + 2, dataLen );
			}

			if (voiceLen > 0)
			{
				// Process unencrypted voice data.
				sock.pRecvFromTarget->OnRecvFromComplete( ConvertAddr((sockaddr*)sock.fromBuf, sock.fromLen), sock.recvBuf + 2 + dataLen, voiceLen );
			}
		}
		else
		{
			sock.pRecvFromTarget->OnRecvFromComplete( ConvertAddr((sockaddr*)sock.fromBuf, sock.fromLen), sock.recvBuf, transferred );
		}

		return false;
	}
	else
	{
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING && err != WSAEWOULDBLOCK)
			sock.pRecvFromTarget->OnRecvFromException( ConvertAddr((sockaddr*)sock.fromBuf, sock.fromLen), OSErrorToSocketError(err) );
		return true;
	}
}

void CSocketIOManager360::SetConnectTarget( SSocketID sockid, IConnectTarget * pTarget )
{
	NET_ASSERT(false);
}

void CSocketIOManager360::SetSendToTarget( SSocketID sockid, ISendToTarget * pTarget )
{
	GetSocket(sockid)->pSendToTarget = pTarget;
}

void CSocketIOManager360::SetAcceptTarget( SSocketID sockid, IAcceptTarget * pTarget )
{
	NET_ASSERT(false);
}

void CSocketIOManager360::SetRecvTarget( SSocketID sockid, IRecvTarget * pTarget )
{
	NET_ASSERT(false);
}

void CSocketIOManager360::SetSendTarget( SSocketID sockid, ISendTarget * pTarget )
{
	NET_ASSERT(false);
}

void CSocketIOManager360::PushUserMessage( int msg )
{
	assert(msg);
	m_userMessages.push(msg);
	SetEvent(m_userEvent);
}

#endif
