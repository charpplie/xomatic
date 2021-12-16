#include "StdAfx.h"
#include "UDPDatagramSocket.h"
#include "NetCVars.h"

#include "Network.h"
#include "Lobby/CryLobby.h"
#if NET_PROFILE_ENABLE
#include "Protocol/PacketRateCalculator.h"
#endif






uint64 g_bytesIn = 0, g_bytesOut = 0;

bool MakeSocketNonBlocking( SOCKET sock )
{
#if defined(WIN32) || defined(XENON)
	unsigned long nTrue = 1;
	if (ioctlsocket(sock, FIONBIO, &nTrue) == SOCKET_ERROR)
		return false;




#else
	int nFlags = fcntl(sock, F_GETFL);
	if (nFlags == -1)
		return false;
	nFlags |= O_NONBLOCK;
	if (fcntl(sock, F_SETFL, nFlags) == -1)
		return false;
#endif
	return true;
}

template <class T>
static bool SetSockOpt( SOCKET s, int level, int optname, const T& value )
{
	return 0 == setsockopt( s, level, optname, (const char *)&value, sizeof(T) );
}

union USockAddr
{
	sockaddr_in ip4;
};

CUDPDatagramSocket::CUDPDatagramSocket() : m_socket(INVALID_SOCKET), m_pListener(0)
#if ENABLE_UDP_PACKET_FRAGMENTATION
	,m_pUDPFragBuffer(NULL),
	m_pFragmentedPackets(NULL)
#endif
{
}

CUDPDatagramSocket::~CUDPDatagramSocket()
{
	Cleanup();
	NET_ASSERT( m_socket == INVALID_SOCKET );
	NET_ASSERT( m_sockid == 0 );
#if ENABLE_UDP_PACKET_FRAGMENTATION
	if (m_pUDPFragBuffer)
		delete [] m_pUDPFragBuffer;
	if (m_pFragmentedPackets)
		delete [] m_pFragmentedPackets;
#endif
}

bool CUDPDatagramSocket::Init( SIPv4Addr addr, uint32 flags )
{
	ASSERT_GLOBAL_LOCK;

#if SHOW_FRAGMENTATION_USAGE
	m_fragged=0;
	m_unfragged=0;
#endif//SHOW_FRAGMENTATION_USAGE

#if ENABLE_UDP_PACKET_FRAGMENTATION
	m_RollingIndex=0;
	if (m_pUDPFragBuffer)
		delete [] m_pUDPFragBuffer;
	m_pUDPFragBuffer=new uint8 [FRAG_MAX_MTU_SIZE];
	if (m_pFragmentedPackets)
		delete [] m_pFragmentedPackets;
	m_pFragmentedPackets=new SFragmentedPacket [FRAG_NUM_PACKET_BUFFERS];
	for (uint32 a=0;a<FRAG_NUM_PACKET_BUFFERS;a++)
	{
		ClearFragmentationEntry(a,0xFF,0);
	}
#endif//ENABLE_UDP_PACKET_FRAGMENTATION

	m_socket = -1;
	m_bIsIP4 = true;
	m_pSockIO = &CNetwork::Get()->GetSocketIOManager();

	sockaddr_in saddr;
	memset( &saddr, 0, sizeof(saddr) );

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(addr.port);
	S_ADDR_IP4(saddr) = htonl(addr.addr);

	if (Init( AF_INET, flags, &saddr, sizeof(saddr) ))
	{
#if 0 && defined(WIN32) && !CHECK_ENCODING
		if (!SetSockOpt(m_socket, IPPROTO_IP, IP_DONTFRAGMENT, TRUE))
			return InitWinError();
#endif // WIN32

		m_sockid = m_pSockIO->RegisterSocket( m_socket, m_protocol );
		if (!m_sockid)
		{
			CloseSocket();
			return false;
		}
		m_pSockIO->SetRecvFromTarget( m_sockid, this );
		m_pSockIO->SetSendToTarget( m_sockid, this );
		for (int i=0; i<640; i++)
			m_pSockIO->RequestRecvFrom(m_sockid);

		return true;
	}

	m_socket = -1;
	return false;
}

void CUDPDatagramSocket::Die()
{
	Cleanup();
}

bool CUDPDatagramSocket::IsDead()
{
	return m_socket == INVALID_SOCKET;
}

SOCKET CUDPDatagramSocket::GetSysSocket()
{
    return m_socket;
}

bool CUDPDatagramSocket::Init( int af, uint32 flags, void * pSockAddr, size_t sizeSockAddr )
{
	m_protocol = IPPROTO_UDP;
	int dgram = SOCK_DGRAM;

#if USE_LIVE
	if (gEnv->pNetwork->GetLobby()->GetLobbyServiceType() == eCLS_Online)
	{
		m_protocol = IPPROTO_VDP;
	}
#endif
#if USE_PSN
	if (gEnv->pNetwork->GetLobby()->GetLobbyServiceType() == eCLS_Online)
	{
		m_protocol = IPROTO_UDPP2P_SAFE;
		dgram = SOCK_DGRAM_P2P;
	}
#endif

	m_socket = socket( af, dgram, m_protocol );
	if (m_socket == INVALID_SOCKET)
		return InitWinError();

	if (!MakeSocketNonBlocking(m_socket))
		return false;

	enum EFatality
	{
		eF_Fail,
		eF_Log,
		eF_Ignore
	};

	struct SFlag
	{
		int so_level;
		int so_opt;
		ESocketFlags flag;
		int trueVal;
		int falseVal;
		EFatality fatality;
	};

	SFlag allflagsudp[] = 
	{
		{ SOL_SOCKET, SO_BROADCAST, eSF_BroadcastSend, 1, 0, eF_Fail },
		{ SOL_SOCKET, SO_RCVBUF, eSF_BigBuffer, ((CNetwork::Get()->GetSocketIOManager().caps & eSIOMC_NoBuffering)==0) * 1024*1024, 4096, eF_Ignore },
		{ SOL_SOCKET, SO_SNDBUF, eSF_BigBuffer, ((CNetwork::Get()->GetSocketIOManager().caps & eSIOMC_NoBuffering)==0) * 1024*1024, 4096, eF_Ignore },
#if defined(WIN32)
		{ IPPROTO_IP, IP_RECEIVE_BROADCAST, eSF_BroadcastReceive, 1, 0, eF_Ignore },
#endif
	};

#if USE_LIVE
	SFlag allflagsvdp[] = 
	{
		{ SOL_SOCKET, SO_RCVBUF, eSF_BigBuffer, ((CNetwork::Get()->GetSocketIOManager().caps & eSIOMC_NoBuffering)==0) * 1024*1024, 4096, eF_Ignore },
		{ SOL_SOCKET, SO_SNDBUF, eSF_BigBuffer, ((CNetwork::Get()->GetSocketIOManager().caps & eSIOMC_NoBuffering)==0) * 1024*1024, 4096, eF_Ignore },
	};
#endif

	SFlag* allflags = NULL;
	int numflags = 0;

	switch (m_protocol)
	{
	case IPPROTO_UDP:
		allflags = allflagsudp;
		numflags = sizeof(allflagsudp)/sizeof(allflagsudp[0]);
		break;

#if USE_LIVE
	case IPPROTO_VDP:
		allflags = allflagsvdp;
		numflags = sizeof(allflagsvdp)/sizeof(allflagsvdp[0]);
		break;
#endif
	}

	for (size_t i=0; i<numflags; i++)
	{
		if (!SetSockOpt(m_socket, allflags[i].so_level, allflags[i].so_opt, ((flags&allflags[i].flag)==allflags[i].flag)? allflags[i].trueVal : allflags[i].falseVal))
		{
			switch (allflags[i].fatality)
			{
			case eF_Fail:
				return InitWinError();
			case eF_Log:
				LogWinError();
			case eF_Ignore:
				break;
			}
		}
	}

#if USE_PSN
	sockaddr_in_p2p inP2PSock;

	if (m_protocol == IPROTO_UDPP2P_SAFE)
	{
		// Change bind information to be udp2p2 compatible - duplicated to avoid trashing address passed in
		memset(&inP2PSock,0,sizeof(inP2PSock));
		inP2PSock.sin_family = AF_INET;
		inP2PSock.sin_port = htons(SCE_NP_PORT);
		inP2PSock.sin_vport = htons(UDPP2P_VPORT);

		pSockAddr = &inP2PSock;
		sizeSockAddr = sizeof(inP2PSock);
	}
#endif
	if (bind(m_socket, static_cast<sockaddr*>(pSockAddr), sizeSockAddr))
		return InitWinError();

	return true;
}

void CUDPDatagramSocket::Cleanup()
{
	if (m_sockid)
	{
		SCOPED_GLOBAL_LOCK;
		m_pSockIO->UnregisterSocket(m_sockid);
		m_sockid = SSocketID();
	}
	if (m_socket != INVALID_SOCKET)
	{
		CloseSocket();
	}
}

void CUDPDatagramSocket::CloseSocket()
{
	if (m_socket != INVALID_SOCKET)
	{
#if defined(WIN32) || defined(XENON)
		closesocket( m_socket );


#else
		close( m_socket );
#endif
		m_socket = INVALID_SOCKET;
	}
}

bool CUDPDatagramSocket::InitWinError()
{
	CloseSocket();
	LogWinError();
	return false;
}

void CUDPDatagramSocket::LogWinError()
{	
#if defined(WIN32) || defined(XENON)
	int error = WSAGetLastError();


#else
	int error = errno;
#endif
	LogWinError( error );
}

void CUDPDatagramSocket::LogWinError( int error )
{
	// ugly
	const char * msg = ((CNetwork*)(gEnv->pNetwork))->EnumerateError( MAKE_NRESULT(NET_FAIL, NET_FACILITY_SOCKET, error) );
	NetWarning( "[net] socket error: %s", msg );
}

void CUDPDatagramSocket::GetSocketAddresses( TNetAddressVec& addrs )
{
	if (m_socket == INVALID_SOCKET)
		return;

#if defined(WIN32) || defined(WIN64) || defined(XENON)
	char addrBuf[_SS_MAXSIZE];
	int addrLen = _SS_MAXSIZE;
	if (0 == getsockname(m_socket, (sockaddr*)addrBuf, &addrLen))
	{
		TNetAddress addr = ConvertAddr((sockaddr*)addrBuf, addrLen);
		bool valid = true;
		if (addr.GetPtr<SNullAddr>())
			valid = false;
		else if (SIPv4Addr * pIPv4 = addr.GetPtr<SIPv4Addr>())
		{
			if (!pIPv4->addr)
				valid = false;
		}
		if (valid)
		{
			addrs.push_back(addr);
			return;
		}
	}
#endif // defined(XENON) || defined(WIN32) || defined(WIN64)

#if !defined(XENON)
	std::vector<string> hostnames;

	uint16 nPort;

	USockAddr sockAddr;
	socklen_t sockAddrSize = sizeof(sockAddr);

	if (0 != getsockname( m_socket, (sockaddr*)&sockAddr, &sockAddrSize ))
	{
		InitWinError();
		return;
	}
	if (sockAddrSize == sizeof(sockaddr_in))
	{
		if (!S_ADDR_IP4(sockAddr.ip4))
			hostnames.push_back("localhost");
		nPort = ntohs( sockAddr.ip4.sin_port );
	}
	else
	{
#ifdef _DEBUG
		CryFatalError( "Unhandled sockaddr type" );
#endif
		return;
	}

#if !defined(PS3) // FIXME ?
	char hostnameBuffer[NI_MAXHOST];
	if (!gethostname(hostnameBuffer, sizeof(hostnameBuffer)))
		hostnames.push_back( hostnameBuffer );
#endif

	if (hostnames.empty())
		return;

	for (std::vector<string>::const_iterator iter = hostnames.begin(); iter != hostnames.end(); ++iter)
	{
		hostent * hp = gethostbyname( iter->c_str() );
		if (hp)
		{
			switch (hp->h_addrtype)
			{
			case AF_INET:
				{
					SIPv4Addr addr;
					NET_ASSERT( sizeof(addr.addr) == hp->h_length );
					addr.port = nPort;
					for (size_t i=0; hp->h_addr_list[i]; i++)
					{
						addr.addr = ntohl( *(uint32*)hp->h_addr_list[i] );
						addrs.push_back( TNetAddress(addr) );
					}
				}
				break;
			default:
				NetWarning("Unhandled network address type %d length %d bytes", hp->h_addrtype, hp->h_length);
			}
		}
	}
#endif // defined(XENON)
}

void CUDPDatagramSocket::RegisterBackoffAddress( TNetAddress addr )
{
	m_pSockIO->RegisterBackoffAddressForSocket( addr, m_sockid );
}

void CUDPDatagramSocket::UnregisterBackoffAddress( TNetAddress addr )
{
	m_pSockIO->UnregisterBackoffAddressForSocket( addr, m_sockid );
}

ESocketError CUDPDatagramSocket::Send( const uint8 * pBuffer, size_t nLength, const TNetAddress& to )
{
#if ENABLE_DEBUG_KIT
	CAutoCorruptAndRestore acr(pBuffer, nLength, CVARS.RandomPacketCorruption == 1);
#endif
	

#if SHOW_FRAGMENTATION_USAGE || ENABLE_UDP_PACKET_FRAGMENTATION
	if (nLength > FRAG_MAX_MTU_SIZE)
	{
#if SHOW_FRAGMENTATION_USAGE
		m_fragged++;
		NetQuickLog(true, 10.f, "Packet Fragmentation : Fragged Count = %d, UnFragged Count = %d, Packet Size = %d", m_fragged,m_unfragged,nLength);
#endif//SHOW_FRAGMENTATION_USAGE

#if ENABLE_UDP_PACKET_FRAGMENTATION
		SendFragmented(pBuffer,nLength,to);
#endif
	}
	else
#endif//SHOW_FRAGMENTATION_USAGE || ENABLE_UDP_PACKET_FRAGMENTATION
	{
#if SHOW_FRAGMENTATION_USAGE
		m_unfragged++;
#endif

		g_bytesOut += nLength + UDP_HEADER_SIZE;
		if (g_time > CNetCVars::Get().StallEndTime)
			m_pSockIO->RequestSendTo( m_sockid, to, pBuffer, nLength );
	}
	
	return eSE_Ok;
}

ESocketError CUDPDatagramSocket::SendVoice( const uint8 * pBuffer, size_t nLength, const TNetAddress& to )
{
#if ENABLE_DEBUG_KIT
	CAutoCorruptAndRestore acr(pBuffer, nLength, CVARS.RandomPacketCorruption == 1);
#endif

	g_bytesOut += nLength + UDP_HEADER_SIZE;
	if (g_time > CNetCVars::Get().StallEndTime)
		m_pSockIO->RequestSendVoiceTo( m_sockid, to, pBuffer, nLength );

	return eSE_Ok;
}

void CUDPDatagramSocket::OnRecvFromComplete( const TNetAddress& from, const uint8 * pData, uint32 len )
{
	g_bytesIn += len + UDP_HEADER_SIZE;

#if NET_MINI_PROFILE
	g_socketBandwidth.totalBandwidthRecvd += (len + UDP_HEADER_SIZE) * 8;
#endif
#if NET_PROFILE_ENABLE
	g_socketBandwidth.totalBandwidthRecvd += (len + UDP_HEADER_SIZE) * 8;
	g_socketBandwidth.sizeRecv += 8 * (len + UDP_HEADER_SIZE);
#endif

#if ENABLE_UDP_PACKET_FRAGMENTATION

	pData = ReceiveFragmented(from,pData,len);		// No point checking for fragmented packets if support for them is off
	if (pData == NULL)
		return;																			// early out, fragmented packet is not complete

#endif

	if (m_pListener)
	{
		m_pListener->OnPacket( from, pData, len );
	}

	m_pSockIO->RequestRecvFrom(m_sockid);
}

void CUDPDatagramSocket::OnRecvFromException( const TNetAddress& from, ESocketError err )
{
	if (err != eSE_Cancelled)
	{
		if (m_pListener)
			m_pListener->OnError( from, err );

		m_pSockIO->RequestRecvFrom(m_sockid);
	}
}

void CUDPDatagramSocket::OnSendToException( const TNetAddress& from, ESocketError err )
{
	if (err != eSE_Cancelled)
	{
		if (m_pListener)
			m_pListener->OnError( from, err );
	}
}

void CUDPDatagramSocket::SetListener( IDatagramListener * pListener )
{
	m_pListener = pListener;
}

#if ENABLE_UDP_PACKET_FRAGMENTATION
void CUDPDatagramSocket::ClearFragmentationEntry(uint32 entry,uint8 buffer,uint8 seq)
{
	assert(entry<FRAG_NUM_PACKET_BUFFERS);
	m_pFragmentedPackets[entry].m_LastIndex=buffer;
	m_pFragmentedPackets[entry].m_Expected=seq;
	m_pFragmentedPackets[entry].m_Reconstitution=0;
	m_pFragmentedPackets[entry].m_Length=0;
}

void CUDPDatagramSocket::SendFragmented(const uint8 * pBuffer, size_t nLength, const TNetAddress& to)
{
	uint32			fraggedCnt=1;
	uint8				expectedFragmentedPacketCount=(nLength + ((FRAG_MAX_MTU_SIZE-fho_FragHeaderSize)-1)) / 
																							(FRAG_MAX_MTU_SIZE-fho_FragHeaderSize);
	uint8				expectedFragmentedPacketMask=0;
	const uint8 expectationMasks[FRAG_SEQ_BIT_SIZE]={0x10,0x30,0x70,0xF0};

	assert(expectedFragmentedPacketCount<=FRAG_SEQ_BIT_SIZE);
	assert(expectedFragmentedPacketCount>1);

	m_RollingIndex++;

	expectedFragmentedPacketMask=expectationMasks[expectedFragmentedPacketCount-1];

	while (nLength)
	{
		int blkSize = MIN(nLength,FRAG_MAX_MTU_SIZE-fho_FragHeaderSize);

		m_pUDPFragBuffer[fho_HeaderId]=Frame_IDToHeader[eH_Fragmentation];
		m_pUDPFragBuffer[fho_SeqSize]=expectedFragmentedPacketMask | fraggedCnt;
		fraggedCnt<<=1;
		m_pUDPFragBuffer[fho_Buffer]=m_RollingIndex;
		memcpy(&m_pUDPFragBuffer[fho_FragHeaderSize],pBuffer,blkSize);

		nLength-=blkSize;
		pBuffer+=blkSize;

		g_bytesOut += blkSize + UDP_HEADER_SIZE;
		if (g_time > CNetCVars::Get().StallEndTime)
			m_pSockIO->RequestSendTo( m_sockid, to, m_pUDPFragBuffer, blkSize+fho_FragHeaderSize);
	}
}

const uint8 *CUDPDatagramSocket::ReceiveFragmented(const TNetAddress& from, const uint8 * pData, uint32 &len)
{
	uint8 nType = Frame_HeaderToID[pData[fho_HeaderId]];
	if (nType==eH_Fragmentation)
	{
		uint8 bufferToUse = pData[fho_Buffer];
		uint8 maskedBuf = bufferToUse&FRAG_PACKET_BUFFERS_MASK;
		uint8 seq = pData[fho_SeqSize];
		uint8 rSeq=BitIndex(uint8(seq&0x0F));

		if (m_pFragmentedPackets[maskedBuf].m_LastIndex!=bufferToUse)
		{
			// Wipe old packet out we haven't received all packets in time
			ClearFragmentationEntry(maskedBuf,bufferToUse,seq>>4);
		}

		m_pFragmentedPackets[maskedBuf].m_Reconstitution|=seq&0x0F;

		memcpy(&m_pFragmentedPackets[maskedBuf].m_FragPackets[0+rSeq*(FRAG_MAX_MTU_SIZE-fho_FragHeaderSize)],
					&pData[fho_FragHeaderSize],len-fho_FragHeaderSize);
		
		assert(len-fho_FragHeaderSize>0);
		
		m_pFragmentedPackets[maskedBuf].m_Length+=len-fho_FragHeaderSize;

		if (m_pFragmentedPackets[maskedBuf].m_Reconstitution == m_pFragmentedPackets[maskedBuf].m_Expected)
		{
			// Complete packet received... 
			pData = &m_pFragmentedPackets[maskedBuf].m_FragPackets[0];
			len = m_pFragmentedPackets[maskedBuf].m_Length;
		}
		else
		{
			return NULL;
		}
	}

	return pData;
}
#endif
