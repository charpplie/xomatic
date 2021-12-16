#ifndef __CRYTELEMETRY_H__
#define __CRYTELEMETRY_H__

#include "ICryTelemetry.h"
#include "Lobby/CryLobby.h"
#include "Socket/NetAddress.h"

class CCryTelemetry : public CMultiThreadRefCount, public ICryTelemetry
{
public:
																	CCryTelemetry(CCryLobby* lobby);
					virtual									~CCryTelemetry();

					virtual ECryLobbyError	Initialise();
					virtual ECryLobbyError	Terminate();
					virtual void						Tick(CTimeValue tv);
	
					virtual bool						ConfigureRemoteServer(const char *url, uint16 port);
					virtual bool						UploadData(STelemetryDataPtr pData);

private:
					void										DiscardQueueElement();

protected:

					virtual ESocketError		GetAddress(sockaddr_in& addr);

					virtual ESocketError		Connect();
					virtual ESocketError		Send(STelemetryDataPtr pData);
					virtual ESocketError		CloseConnection();

					TNetAddress*						m_pResolvedAddr;
					SOCKET									m_socket;
					CCryLobby*							m_pLobby;
					std::queue<STelemetryDataPtr>	m_dataQueue;
					CryMutex								m_queueMutex;
};

#endif // __CRYTELEMETRY_H__

