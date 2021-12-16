#ifndef __CRYXLSPTELEMETRY_H__
#define __CRYXLSPTELEMETRY_H__

#include "Lobby/CryTelemetry.h"

#include <Xonline.h>

#define MAX_XLSP_SERVERS				(10)
#define MAX_SECURE_CONNECT_TIME	(10 * 1000)

enum ECryXLSPTelemetryStates
{
	ECXLSPTS_None,
	ECXLSPTS_FindServers,
	ECXLSPTS_ChooseServer,
	ECXLSPTS_OpenSecureConnection,
	ECXLSPTS_WaitingSecureConnection,
	ECXLSPTS_SecureConnectionValid
};

class CCryXLSPTelemetry : public CCryTelemetry
{
public:
																	CCryXLSPTelemetry(CCryLobby* pLobby);
					virtual									~CCryXLSPTelemetry();

					virtual ECryLobbyError	Initialise();
					virtual ECryLobbyError	Terminate();
					virtual void						Tick(CTimeValue tv);
	
protected:

					virtual ESocketError		GetAddress(sockaddr_in& addr);

					virtual ESocketError		Connect();
					virtual ESocketError		Send(STelemetryDataPtr pData);
					virtual ESocketError		CloseConnection();
    
					void										RemoveCurrentServer();

					bool										IsAbleToSend() {return (m_state==ECXLSPTS_SecureConnectionValid) && (m_chosenServer<MAX_XLSP_SERVERS);}

					void										CloseServerHandle();
					bool										EnumerateServers();
					bool										WaitForServers();
					bool										ChooseServer();
					bool										OpenSecureConnection(CTimeValue &tv);
					bool										WaitSecureConnection(CTimeValue &tv,DWORD &error);

					XTITLE_SERVER_INFO			m_pServers[ MAX_XLSP_SERVERS ];
					IN_ADDR									m_secureAddr;												// Secure title server address
					XOVERLAPPED							m_overlapped;
					
					CTimeValue							m_connectionTimer;

					ECryXLSPTelemetryStates	m_state;
    
					DWORD										m_chosenServer;
					DWORD										m_serverCount;
					DWORD										m_bufferSize;
					HANDLE									m_serverEnum;

					uint32									m_serviceId;
					uint16									m_servicePort;
};

#endif // __CRYXLSPTELEMETRY_H__

