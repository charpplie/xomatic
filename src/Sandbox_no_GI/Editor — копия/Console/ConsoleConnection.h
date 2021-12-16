#ifndef __ConsoleConnection_h__
#define __ConsoleConnection_h__
#pragma once

#include <INotificationNetwork.h>
#include "CryThread.h"

class CConsoleConnection:
	public CryThread<CConsoleConnection>,
	public INotificationNetworkConnectionCallback,
	public IEditorNotifyListener
{
private:
	struct SConnection
	{
		SConnection():pClient(NULL),
			bFailedToConnect(false),
			bIsConnecting(false),
			boPlatformStatusUpdated(false)
		{};

		INotificationNetworkClient *pClient;
		bool												bFailedToConnect;
		bool												bIsConnecting;
		int													nStatusItemId;

		CString											strPlatformName;
		size_t											nStatusIndex;
		bool												boPlatformStatusUpdated;

		CryMutex										oConnectionMutex;
		CryMutex										oConnectionStatusMutex;
	};
public:
	static CConsoleConnection &Instance()
	{
		static CConsoleConnection instance;
		return instance;
	}

private:
	CConsoleConnection();
	~CConsoleConnection();

public:
	static INotificationNetwork *GetNotificationNetwork();

	// Returns true if the given platform is connected to the network.
	// Remarks:	may cause change the Failed to Connect status.
	bool IsConnected(CHotUpdateSystem::eConsolePlatforms ePlatform);

	//// Returns true if the given platform has an instance of CryEngine running.
	//bool IsRunning(CHotUpdateSystem::eConsolePlatforms ePlatform);

	//////////////////////////////////////////////////////////////////////////

	//
	void BeginStatusCheck(CHotUpdateSystem::eConsolePlatforms ePlatform);

	//
	void EndStatusCheck(CHotUpdateSystem::eConsolePlatforms ePlatform);


	//
	bool IsFailedToConnect(CHotUpdateSystem::eConsolePlatforms ePlatform);

	//
	bool IsConnecting(CHotUpdateSystem::eConsolePlatforms ePlatform);
	//////////////////////////////////////////////////////////////////////////



	// Copies a file to the given destination platform.
	bool FileCopyTo(CHotUpdateSystem::eConsolePlatforms ePlatform, const char *localSource, const char *remoteDestination);

	// Copies a file from the given source platform.
	bool FileCopyFrom(CHotUpdateSystem::eConsolePlatforms ePlatform, const char *remoteSource, const char *localDestination);

	// Launched the game on the console.
	bool Launch(CHotUpdateSystem::eConsolePlatforms ePlatform, const char * szProgramFilename,const char* szWorkingDirectory,const char* szParameters, const char * targetname);

	// Begin using the Notification Network Client for the given platform.
	// If NULL is returned, it was impossible to establish a connection to the
	// given platform.
	// IMPORTANT: If the method was successful it is important to call
	// NotificationEnd() when done using the Notification Network Client.
	INotificationNetworkClient *NotificationBegin(CHotUpdateSystem::eConsolePlatforms ePlatform);

	// End using the Notification Network Client for the given platform.
	void NotificationEnd(CHotUpdateSystem::eConsolePlatforms ePlatform);

	void NotificationReset(CHotUpdateSystem::eConsolePlatforms ePlatform);

	// Warning: this method is not thread safe. 
	bool Connect(CHotUpdateSystem::eConsolePlatforms ePlatform);

	// Thread run method.
	void Run();

	//////////////////////////////////////////////////////////////////////////
	// IEditorNotifyListener
	//////////////////////////////////////////////////////////////////////////
	// We only need to deal with the document events to prevent crashing.
	void OnEditorNotifyEvent( EEditorNotifyEvent event );
	//////////////////////////////////////////////////////////////////////////
private:
	bool RetrieveConnectionAddress(CHotUpdateSystem::eConsolePlatforms ePlatform, string &address);

	bool ConCom(CHotUpdateSystem::eConsolePlatforms ePlatform, const char *name, const char *parameters, string &result);

	void OnConnect(INotificationNetworkClient* pClient,bool bSucceeded);
	void OnDisconnected(INotificationNetworkClient* pClient);

	// Returns the input parameter.
	CString& GetPlatformStatusString(CHotUpdateSystem::eConsolePlatforms ePlatform,CString& strStatus);
private:
	SConnection m_connections[CHotUpdateSystem::eNumberOfPlatforms];

	// Thread parameters...
	CHotUpdateSystem::eConsolePlatforms			m_ePlatform;
	string																	m_strParameters;
	CryMutex																m_oConComMutex;
};

#endif //__ConsoleConnection_h__
