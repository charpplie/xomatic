#ifndef __SOCKETIOMANAGER360_H__
#define __SOCKETIOMANAGER360_H__

#pragma once





#ifdef HAS_SOCKETIOMANAGER_360
#include "ISocketIOManager.h"
#include "WatchdogTimer.h"

class CSocketIOManager360 : public ISocketIOManager
{
public:
	CSocketIOManager360();
	bool Init();

	virtual const char * GetName();

	virtual int Poll( float waitTime, bool& performedWork );

	virtual SSocketID RegisterSocket( SOCKET sock, int protocol );
	virtual void SetRecvFromTarget( SSocketID sockid, IRecvFromTarget * pTarget );
	virtual void SetConnectTarget( SSocketID sockid, IConnectTarget * pTarget );
	virtual void SetSendToTarget( SSocketID sockid, ISendToTarget * pTarget );
	virtual void SetAcceptTarget( SSocketID sockid, IAcceptTarget * pTarget );
	virtual void SetRecvTarget( SSocketID sockid, IRecvTarget * pTarget );
	virtual void SetSendTarget( SSocketID sockid, ISendTarget * pTarget );
	virtual void RegisterBackoffAddressForSocket( TNetAddress addr, SSocketID sockid );
	virtual void UnregisterBackoffAddressForSocket( TNetAddress addr, SSocketID sockid );
	virtual void UnregisterSocket( SSocketID sockid );

	virtual bool RequestRecvFrom( SSocketID sockid );
	virtual bool RequestSendTo( SSocketID sockid, const TNetAddress& addr, const uint8 * pData, size_t len );
	virtual bool RequestSendVoiceTo( SSocketID sockid, const TNetAddress& addr, const uint8 * pData, size_t len );

	virtual bool RequestConnect( SSocketID sockid, const TNetAddress& addr );
	virtual bool RequestAccept( SSocketID sock );
	virtual bool RequestSend( SSocketID sockid, const uint8 * pData, size_t len );
	virtual bool RequestRecv( SSocketID sockid );

	virtual void PushUserMessage( int msg );

	virtual bool HasPendingData();

private:
	CWatchdogTimer m_watchdog;

	struct SQueuedSend
	{
		size_t len;
		TNetAddress addr;
		uint8 data[MAX_UDP_PACKET_SIZE];
	};

	struct SSocket
	{
		SOCKET sock;
		int32 protocol;
		std::queue<SQueuedSend*> queuedSends;

		WSAEVENT evtSend;
		WSAEVENT evtRecvFrom;

		ISendToTarget * pSendToTarget;
		IRecvFromTarget * pRecvFromTarget;

		WSAOVERLAPPED sendOverlapped;
		WSAOVERLAPPED recvOverlapped;

		INT fromLen;
		uint16 salt;
		bool inUse;
		bool outstandingSend;

		uint8 recvBuf[MAX_UDP_PACKET_SIZE];
		uint8 fromBuf[_SS_MAXSIZE];
	};

	SSocket * GetSocket( SSocketID sock );

	static const int MAX_SOCKETS = 4;
	SSocket m_sockets[MAX_SOCKETS];

	typedef int (CSocketIOManager360::*FinishActionFunc)(SSocketID sock);

	struct SAction
	{
		SSocketID socket;
		FinishActionFunc action;
	};

	static void AddWaitAction( WSAEVENT * pEvents, SAction * pActions, DWORD * pCount, WSAEVENT evt, SSocketID socket, FinishActionFunc func );
	DWORD FillWaitActions( WSAEVENT * pEvents, SAction * pActions );

	int FinishSend( SSocketID sock );
	int FinishRecv( SSocketID sock );
	int FinishUser( SSocketID sock );

	enum ESendResult
	{
		eSR_Ok_Return,
		eSR_Ok_Continue,
		eSR_Fail,
	};
	ESendResult DoSend( SSocket& sock );
	void QueueRecvFrom( SSocket& sock );
	bool InformRecvFrom( int r, SSocket& sock, DWORD transferred );

	WSAEVENT m_userEvent;
	std::queue<int> m_userMessages;
};
#endif

#endif
