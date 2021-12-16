#ifndef __CRYSIMPLESOCK__
#define __CRYSIMPLESOCK__

#ifdef UNIX
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <errno.h>
#define closesocket close
#endif

#ifdef _MSC_VER
#include <WinSock2.h>
#endif
#include <vector>

#include "../STLHelper.hpp"

enum ECrySimpleS_TYPE
{
	ECrySimpleST_ROOT,
	ECrySimpleST_SERVER,
	ECrySimpleST_CLIENT,
	ECrySimpleST_INVALID,
};

class CCrySimpleSock
{
	static CCrySimpleSock*		m_pInstance;

	const ECrySimpleS_TYPE		m_Type;
	SOCKET										m_Socket;
	bool											m_SwapEndian;

	tdDataVector              m_tempSendBuffer;


														CCrySimpleSock();
														CCrySimpleSock(SOCKET Sock);
														CCrySimpleSock(const CCrySimpleSock&):m_Type(ECrySimpleST_INVALID){};

	void											InitClient();
	void											Release();
	void											ReleaseInstance();
public:
														CCrySimpleSock(const std::string& rServerName,uint16_t Port);

														~CCrySimpleSock();
	void											InitRoot(uint16_t Port);
	void											Listen();

	static CCrySimpleSock&		Instance();

	CCrySimpleSock*						Accept();

	bool											Recv(std::vector<uint8_t>& rVec);
	bool											Backward(std::vector<uint8_t>& rVec);
	void											Send(const std::vector<uint8_t>& rVec);
	void											Forward(const std::vector<uint8_t>& rVec);

	uint32_t									PeerIP();

	bool											Valid()const{return m_Socket!=INVALID_SOCKET;}
};

#endif
