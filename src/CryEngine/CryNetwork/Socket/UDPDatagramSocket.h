#ifndef __UDPDATAGRAMSOCKET_H__
#define __UDPDATAGRAMSOCKET_H__

#pragma once

#include "IDatagramSocket.h"

bool MakeSocketNonBlocking( SOCKET sock );

#if SHOW_FRAGMENTATION_USAGE || ENABLE_UDP_PACKET_FRAGMENTATION
#define FRAG_MAX_MTU_SIZE				(1200)
#endif//SHOW_FRAGMENTATION_USAGE || ENABLE_UDP_PACKET_FRAGMENTATION

#if ENABLE_UDP_PACKET_FRAGMENTATION

#define FRAG_NUM_PACKET_BUFFERS		(8)									// Must be power of 2
#define FRAG_PACKET_BUFFERS_MASK	(FRAG_NUM_PACKET_BUFFERS-1)
#define FRAG_SEQ_BIT_SIZE					(4)

enum EFragHeaderOffsets
{
	fho_HeaderId=0,
	fho_SeqSize=1,
	fho_Buffer=2,
	fho_FragHeaderSize=3
};

struct SFragmentedPacket 
{
	int16	m_Length;
	uint8	m_Reconstitution:FRAG_SEQ_BIT_SIZE;								// bit mask
	uint8	m_Expected:FRAG_SEQ_BIT_SIZE;
	uint8	m_LastIndex;
	uint8	m_FragPackets[MAX_UDP_PACKET_SIZE];
};

#endif//ENABLE_UDP_PACKET_FRAGMENTATION

class CUDPDatagramSocket : public IDatagramSocket, private IRecvFromTarget, ISendToTarget
{
public:
	CUDPDatagramSocket();
	~CUDPDatagramSocket();

	bool Init( SIPv4Addr addr, uint32 flags );

	// IDatagramSocket
	virtual void GetSocketAddresses( TNetAddressVec& addrs );
	virtual ESocketError Send( const uint8 * pBuffer, size_t nLength, const TNetAddress& to );
	virtual ESocketError SendVoice( const uint8 * pBuffer, size_t nLength, const TNetAddress& to );
	virtual void SetListener( IDatagramListener * pListener );
	virtual void Die();
	virtual bool IsDead();
	virtual SOCKET GetSysSocket();
	virtual void RegisterBackoffAddress( TNetAddress addr );
	virtual void UnregisterBackoffAddress( TNetAddress addr );
	// ~IDatagramSocket

private:
	SOCKET m_socket;
	int32 m_protocol;
	SSocketID m_sockid;
	bool m_bIsIP4;
	TNetAddress m_lastError;
	IDatagramListener * m_pListener;
	ISocketIOManager * m_pSockIO;

#if SHOW_FRAGMENTATION_USAGE
	int								m_fragged;
	int								m_unfragged;
#endif//SHOW_FRAGMENTATION_USAGE
#if ENABLE_UDP_PACKET_FRAGMENTATION
	uint8							*m_pUDPFragBuffer;
	uint8							m_RollingIndex;
	SFragmentedPacket	*m_pFragmentedPackets;
#endif//ENABLE_UDP_PACKET_FRAGMENTATION

	bool Init( int af, uint32 flags, void * pSockAddr, size_t sizeSockAddr );
	bool InitWinError();
	void LogWinError( int err );
	void LogWinError();

	void Cleanup();

	void CloseSocket();

	// IRecvFromTarget
	virtual void OnRecvFromComplete( const TNetAddress& from, const uint8 * pData, uint32 len );
	virtual void OnRecvFromException( const TNetAddress& from, ESocketError err );
	// ~IRecvFromTarget

	// ISendToTarget
	virtual void OnSendToException( const TNetAddress& from, ESocketError err );
	// ~ISendToTarget

#if ENABLE_UDP_PACKET_FRAGMENTATION
	void ClearFragmentationEntry(uint32 entry,uint8 buffer,uint8 seq);
	void SendFragmented(const uint8 * pBuffer, size_t nLength, const TNetAddress& to);
	const uint8 *ReceiveFragmented(const TNetAddress& from, const uint8 * pData, uint32 &len);
#endif//ENABLE_UDP_PACKET_FRAGMENTATION
};

#endif
