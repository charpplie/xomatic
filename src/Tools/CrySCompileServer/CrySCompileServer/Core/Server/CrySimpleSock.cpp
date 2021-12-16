#include "../StdTypes.hpp"
#include "CrySimpleSock.hpp"
#include "../Error.hpp"
#include "../STLHelper.hpp"

CCrySimpleSock* CCrySimpleSock::m_pInstance=0;


CCrySimpleSock::CCrySimpleSock():
m_Type(ECrySimpleST_ROOT)
{
}

CCrySimpleSock::CCrySimpleSock(SOCKET Sock):
m_Type(ECrySimpleST_SERVER),
m_Socket(Sock)
{
	InitClient();
}


CCrySimpleSock::CCrySimpleSock(const std::string& rServerName,uint16_t Port):
m_Type(ECrySimpleST_CLIENT),
m_Socket(INVALID_SOCKET)
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(Port);
	const char* pHostName	=	rServerName.c_str();
	bool IP=true;
	for(size_t a=0,Size=strlen(pHostName);a<Size;a++)
		IP&=(pHostName[a]>='0' && pHostName[a]<='9') || pHostName[a]=='.' ;
	if(IP)
		addr.sin_addr.s_addr = inet_addr(pHostName);
	else
	{
		hostent* pHost	= gethostbyname( pHostName );
		if (!pHost)
			return;
		addr.sin_addr.s_addr = ((struct in_addr *)(pHost->h_addr))->s_addr;
	}

	m_Socket = socket(AF_INET, SOCK_STREAM, 0);
	int Err = connect(m_Socket, (struct sockaddr *)&addr, sizeof addr);
	if(Err<0)
		m_Socket=INVALID_SOCKET;
}

CCrySimpleSock::~CCrySimpleSock()
{
	Release();
}

void CCrySimpleSock::InitRoot(uint16_t Port)
{
#ifdef _MSC_VER
	WSADATA Data;
	m_Socket	=	INVALID_SOCKET;
	if(WSAStartup(MAKEWORD(2,0),&Data))
	{
		CrySimple_ERROR("Could not init root socket");
		return;
	}
#endif

	m_Socket	=	socket(AF_INET,SOCK_STREAM,0);
	if(INVALID_SOCKET==m_Socket)
	{
		CrySimple_ERROR("Could not initialize basic server due to invalid socket");
		return;
	}
#ifdef UNIX
	int arg = 1;
	setsockopt(m_Socket, SOL_SOCKET, SO_KEEPALIVE, &arg, sizeof arg);
	arg = 6;
	setsockopt(m_Socket, SOL_SOCKET, SO_PRIORITY, &arg, sizeof arg);
	arg = 1;
	setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof arg);
#endif

	sockaddr_in SockAddr;
	memset(&SockAddr,0,sizeof(sockaddr_in));
	SockAddr.sin_family	=	PF_INET;
	SockAddr.sin_port		=	htons(Port);
	if(bind(m_Socket,(sockaddr*)&SockAddr,sizeof(sockaddr_in))==SOCKET_ERROR)
	{
		closesocket(m_Socket);
		CrySimple_ERROR("Could not bind server socket");
		return;
	}
}

void CCrySimpleSock::Listen()
{
	listen(m_Socket,5);
}

void CCrySimpleSock::InitClient()
{
}

void CCrySimpleSock::Release()
{
	if(m_Socket!=INVALID_SOCKET)
		closesocket(m_Socket);

#ifdef _MSC_VER
	switch(m_Type)
	{
		case	ECrySimpleST_ROOT:WSACleanup();break;
		case	ECrySimpleST_SERVER:
		case	ECrySimpleST_CLIENT:;break;
		default:
			CrySimple_ERROR("unknown SocketType Released");
	}
#endif
}

CCrySimpleSock& CCrySimpleSock::Instance()
{
	if(!m_pInstance)
		m_pInstance	=	new	CCrySimpleSock();
	return *m_pInstance;
}

void CCrySimpleSock::ReleaseInstance()
{
	delete m_pInstance;
	m_pInstance=0;
}

CCrySimpleSock* CCrySimpleSock::Accept()
{
	if(m_Type!=ECrySimpleST_ROOT)
	{
		CrySimple_ERROR("called Accept on non root socket");
		return 0;
	}

	while (true)
	{
		SOCKET Sock=accept(m_Socket,0,0);
		if(Sock==INVALID_SOCKET)
		{
#if defined _MSC_VER
			int Error=WSAGetLastError();
			CrySimple_ERROR("Accept recived invalid socket");
#elif defined UNIX
			if (errno == EINTR
  #ifdef LINUX
					|| errno == 512 /* ERESTARTSYS */
	#endif
	#ifdef ETIMEDOUT
					|| errno == ETIMEDOUT
	#endif
					)
				continue;
			char errbuf[256];
			snprintf(errbuf, sizeof errbuf, "accept(): %s", strerror(errno));
			errbuf[sizeof errbuf - 1] = 0;
			CrySimple_ERROR(errbuf);
#endif
			return 0;
		}
		return new CCrySimpleSock(Sock);
	}
}

union CrySimpleRecvSize
{
	uint8_t		m_Data8[8];
	uint64_t	m_Data64;
};

bool CCrySimpleSock::Recv(std::vector<uint8_t>& rVec)
{
	CrySimpleRecvSize Size;
	if(recv(m_Socket,reinterpret_cast<char*>(&Size.m_Data8[0]),8,0)!=8)
	{
		CrySimple_ERROR("Error while reciving size of data");
		return false;
	}

	if(Size.m_Data64==0)
		return false;

	m_SwapEndian	=	(Size.m_Data64>>32)!=0;
	if(m_SwapEndian)
		CSTLHelper::EndianSwizzleU64(Size.m_Data64);

	rVec.clear();
	rVec.resize(static_cast<size_t>(Size.m_Data64));

	for(uint32_t a=0;a<Size.m_Data64;)
	{
		int Read=recv(m_Socket,reinterpret_cast<char*>(&rVec[a]),static_cast<int>(Size.m_Data64)-a,0);
		if(Read<=0)
		{
			CrySimple_ERROR("Error while reciving tcp-data");
			return false;
		}
		a+=Read;
	}
	return true;
}


void CCrySimpleSock::Forward(const std::vector<uint8_t>& rVecIn)
{
	tdDataVector &rVec = m_tempSendBuffer;
	rVec.resize( rVecIn.size()+8 );
	CrySimpleRecvSize& rHeader	=	*(CrySimpleRecvSize*)(&rVec[0]);
	rHeader.m_Data64	= (uint32_t)rVecIn.size();
	memcpy( &rVec[8],&rVecIn[0],rVecIn.size() );

	const size_t	BLOCKSIZE	=	4*1024;
	CrySimpleRecvSize Size;
	Size.m_Data64=static_cast<int>(rVec.size());
	for(uint64_t a=0;a<Size.m_Data64;a+=BLOCKSIZE)
	{
		char* pData		=	reinterpret_cast<char*>(&rVec[a]);
		int nSendRes	= send(m_Socket,pData,std::min<int>(static_cast<int>(Size.m_Data64-a),BLOCKSIZE),0);
		if (nSendRes == SOCKET_ERROR)
		{
			int nLastSendError = WSAGetLastError();
			printf( "Socket send(forward) error: %d",nLastSendError );
		}
	}
}


bool CCrySimpleSock::Backward(std::vector<uint8_t>& rVec)
{
	uint32_t Size;
	if(recv(m_Socket,reinterpret_cast<char*>(&Size),4,0)!=4)
	{
		CrySimple_ERROR("Error while reciving size of data");
		return false;
	}

	rVec.clear();
	rVec.resize(static_cast<size_t>(Size));

	for(uint32_t a=0;a<Size;)
	{
		int Read=recv(m_Socket,reinterpret_cast<char*>(&rVec[a]),Size-a,0);
		if(Read<=0)
		{
			CrySimple_ERROR("Error while reciving tcp-data");
			return false;
		}
		a+=Read;
	}
	return true;
}

void CCrySimpleSock::Send(const std::vector<uint8_t>& rVecIn)
{
	tdDataVector &rVec = m_tempSendBuffer;
	rVec.resize( rVecIn.size()+4 );
	*(uint32_t*)(&rVec[0]) = (uint32_t)rVecIn.size();
	memcpy( &rVec[4],&rVecIn[0],rVecIn.size() );

	if(m_SwapEndian)
		CSTLHelper::EndianSwizzleU32( *(uint32_t*)&rVec[0] );

	const size_t	BLOCKSIZE	=	4*1024;
	CrySimpleRecvSize Size;
	Size.m_Data64=static_cast<int>(rVec.size());
	for(uint64_t a=0;a<Size.m_Data64;a+=BLOCKSIZE)
	{
//		if(m_SwapEndian)
//			CSTLHelper::EndianSwizzleU64(Size.m_Data64);
//		send(m_Socket,reinterpret_cast<char*>(&Size.m_Data8[0]),8,0);
		char* pData	=	reinterpret_cast<char*>(&rVec[a]);
		int nSendRes = send(m_Socket,pData,std::min<int>(static_cast<int>(Size.m_Data64-a),BLOCKSIZE),0);
		if (nSendRes == SOCKET_ERROR)
		{
			int nLastSendError = WSAGetLastError();
			printf( "Socket send error: %d",nLastSendError );
		}
	}
}

uint32_t CCrySimpleSock::PeerIP()
{
	struct sockaddr_in addr;
	int addr_size = sizeof(sockaddr_in);
	int nRes = getpeername(m_Socket, (sockaddr*) &addr, &addr_size);
	if (nRes == SOCKET_ERROR)
	{
		int nError = WSAGetLastError();
		printf("Socket getpeername error: %d", nError);
		return 0;
	}
	return addr.sin_addr.S_un.S_addr;
}
