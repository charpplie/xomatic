#include <StdAfx.h>
#include "ConsoleConnection.h"

// TODO: This will become a console variable!
#define DEFAULT_PORT 9432

#undef LockDebug
//#define LockDebug(str1,n)	{string strMessage;strMessage.Format(str1,n);OutputDebugString(strMessage.c_str());}
#define LockDebug(str1,n)

INotificationNetwork *CConsoleConnection::GetNotificationNetwork()
{
	IEditor *pEditor = ::GetIEditor();
	if (!pEditor)
		return NULL;

	ISystem *pSystem = pEditor->GetSystem();
	if (!pSystem)
		return NULL;

	return pSystem->GetINotificationNetwork();
}

bool CConsoleConnection::ConCom(CHotUpdateSystem::eConsolePlatforms ePlatform, const char *name, const char *parameters, string &result)
{
	CryAutoLock<CryMutex>	oAutoLock(m_oConComMutex);

	string command = ".\\Tools\\ConCom.exe";
	switch (ePlatform)
	{
	case CHotUpdateSystem::eXBOX360:			command += " XB360 ";	break;
	case CHotUpdateSystem::ePLAYSTATION3:	command +=  " PS3 ";	break;
	}
	if (name)
	{
		command += name;
		command += " ";
	}
	else
	{
		command += "* ";
	}

	command += parameters;

	SECURITY_ATTRIBUTES securityAttributes;
	::ZeroMemory(&securityAttributes, sizeof(securityAttributes));
	securityAttributes.bInheritHandle = true;
	securityAttributes.lpSecurityDescriptor = NULL;

	HANDLE hOutRead, hOutWrite;
	::CreatePipe(&hOutRead, &hOutWrite, &securityAttributes, 0);
	::SetHandleInformation(hOutRead, HANDLE_FLAG_INHERIT, 0);

	HANDLE hInRead, hInWrite;
	::CreatePipe(&hInRead, &hInWrite, &securityAttributes, 0);
	::SetHandleInformation(hInWrite, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFO startUpInfo;
	::ZeroMemory(&startUpInfo, sizeof(startUpInfo));
	startUpInfo.cb = sizeof(startUpInfo);
	startUpInfo.dwX = 100;
	startUpInfo.dwY = 100;
	startUpInfo.hStdError = hOutWrite;
	startUpInfo.hStdOutput = hOutWrite;
	startUpInfo.hStdInput = hInRead;
	startUpInfo.dwFlags = STARTF_USEPOSITION | STARTF_USESTDHANDLES;

	PROCESS_INFORMATION processInformation;
	::ZeroMemory(&processInformation, sizeof(processInformation));
	if (!CreateProcess(
		NULL,
		(LPSTR)command.c_str(),
		NULL,
		NULL,
		TRUE,
		CREATE_NO_WINDOW,
		NULL,
		NULL,
		&startUpInfo,
		&processInformation))
	{
		return false;
	}

	::CloseHandle(hInWrite);
	::CloseHandle(hOutWrite);

	time_t timeStart;
	time(&timeStart);
	time_t timeCurrent;
	result.clear();
	for (;;)
	{
		char buffer[2048];
		DWORD bytesRead;
		if (!ReadFile(hOutRead, buffer, sizeof(buffer), &bytesRead, NULL))
			break;
		if (!bytesRead)
			break;

		result.append(buffer, bytesRead);

		time(&timeCurrent);
		if ((timeCurrent - timeStart) > 30)
		{
			GetIEditor()->GetSystem()->GetILog()->Log("ConCom timed out (30s).");
			break;
		}
	}

	bool bReturn = true;
	if (::_strnicmp(result.c_str(), "failed",6)==0)
		bReturn = false;

	::WaitForSingleObject(processInformation.hProcess, INFINITE);
	::CloseHandle(processInformation.hProcess);
	::CloseHandle(processInformation.hThread);
	return bReturn;
}

/*

  CConsoleConnection

*/

CConsoleConnection::CConsoleConnection()
{
	for (uint32 i=0; i<CHotUpdateSystem::eNumberOfPlatforms; ++i)
	{
		m_connections[i].pClient = NULL;
		if (i==CHotUpdateSystem::eXBOX360)
		{
			m_connections[i].nStatusItemId=ID_XBOX360_CONNECTION;
			m_connections[i].strPlatformName="XBOX 360: ";
		}
		else if (i==CHotUpdateSystem::ePLAYSTATION3)
		{
			m_connections[i].nStatusItemId=ID_PS3_CONNECTION;
			m_connections[i].strPlatformName="PS3: ";
		}
		else
		{
			assert("CConsoleConnection::CConsoleConnection() Unknown platform"&&false);
		}		
	}

	m_ePlatform=(CHotUpdateSystem::eConsolePlatforms)0;
	m_strParameters=""; //Not needed, but not bad.

	GetIEditor()->RegisterNotifyListener(this);
}

CConsoleConnection::~CConsoleConnection()
{
	GetIEditor()->UnregisterNotifyListener(this);

	for (uint32 i=0; i<CHotUpdateSystem::eNumberOfPlatforms; ++i)
	{
		NotificationReset((CHotUpdateSystem::eConsolePlatforms)i);
	}
}

//

bool CConsoleConnection::RetrieveConnectionAddress(CHotUpdateSystem::eConsolePlatforms ePlatform, string &address)
{
	if (!gSettings.oHotUpdateSystemSettings.boPlatformAutodetectTargetMachine[ePlatform])
	{
		// TODO: If no string return false;.
		CString strAddress;
		gSettings.oHotUpdateSystemSettings.GetPlatformTargetMachine(ePlatform,strAddress);
		address=strAddress;
		return true;
	}

	if (!ConCom(ePlatform, NULL, "GetIP", address))
		return false;
	return true;
}

//

bool CConsoleConnection::FileCopyTo(CHotUpdateSystem::eConsolePlatforms ePlatform, const char *localSource, const char *remoteDestination)
{
	if (!localSource || !remoteDestination)
		return false;
	if (!*localSource || !*remoteDestination)
		return false;

	string parameters = "FileCopyTo \"";
	parameters += localSource;
	parameters += "\" \"";
	parameters += remoteDestination;
	parameters += "\"";

	string result;
	if (!ConCom(ePlatform, NULL, parameters.c_str(), result))
		return false;

	GetIEditor()->GetSystem()->GetILog()->Log(result);
	return true;
}

bool CConsoleConnection::Launch(CHotUpdateSystem::eConsolePlatforms ePlatform, const char * szProgramFilename,const char* szWorkingDirectory,const char* szParameters, const char* targetname)
{
	CryAutoLock<CryMutex>	oAutoLock(m_oConComMutex);

	assert(szProgramFilename);
	assert(szWorkingDirectory);
	assert(szParameters);

	if (!szProgramFilename || !szWorkingDirectory || !szParameters)
	{
		return false;
	}

	if (!*szProgramFilename)
	{
		return false;
	}

	string strWorkingDirectory(szWorkingDirectory);
	if (!strWorkingDirectory.empty())
	{
		// We must have double \ before ", or the command interpreter will
		// think \" means we want to add a " in the middle of a "" parameter 
		// block.
		if (strWorkingDirectory[strWorkingDirectory.size()-1]=='\\')
		{
			strWorkingDirectory.push_back('\\');
		}
		else if (strWorkingDirectory[strWorkingDirectory.size()-1]!='/')
		{
			// Here we are adding a backslash in case we don't have one already.
			strWorkingDirectory+="//";
		}
	}

	m_ePlatform=ePlatform;

	m_strParameters= "Launch \"";
	m_strParameters+=szProgramFilename;
	m_strParameters+= "\" \"";
	m_strParameters+=strWorkingDirectory;
	m_strParameters+="\" ";
	m_strParameters+=szParameters;

	string result;
	return ConCom(ePlatform, targetname, m_strParameters, result);
}

INotificationNetworkClient *CConsoleConnection::NotificationBegin(CHotUpdateSystem::eConsolePlatforms ePlatform)
{
	if (ePlatform == CHotUpdateSystem::eNumberOfPlatforms)
		return NULL;
	if (!gSettings.oHotUpdateSystemSettings.boPlatformEnabled[ePlatform])
		return NULL;

	LockDebug("Lock CConsoleConnection::NotificationBegin m_connections[%d]\n",(int)ePlatform);
	m_connections[ePlatform].oConnectionMutex.Lock();

	if (m_connections[ePlatform].pClient)
		return m_connections[ePlatform].pClient;

	string address;
	if (!RetrieveConnectionAddress(ePlatform, address))
	{
		m_connections[ePlatform].oConnectionMutex.Unlock();
		LockDebug("Unlock 1 CConsoleConnection::NotificationBegin m_connections[%d]\n",(int)ePlatform);
		m_connections[ePlatform].nStatusIndex=1;
		m_connections[ePlatform].boPlatformStatusUpdated=true;

		return NULL;
	}

	INotificationNetwork *pNotificationNetwork = GetNotificationNetwork();
	if (!pNotificationNetwork)
	{
		m_connections[ePlatform].oConnectionMutex.Unlock();
		m_connections[ePlatform].nStatusIndex=2;
		m_connections[ePlatform].boPlatformStatusUpdated=true;

		return NULL;
	}

	INotificationNetworkClient*	piClient(pNotificationNetwork->CreateClient());
	m_connections[ePlatform].pClient = piClient;

	piClient->RegisterCallbackListener(this);

	if (!piClient->Connect(address, DEFAULT_PORT))
	{
		m_connections[ePlatform].oConnectionMutex.Unlock();
		LockDebug("Unlock 3 CConsoleConnection::NotificationBegin m_connections[%d]\n",(int)ePlatform);

		CryAutoLock<CryMutex>	oAutoLock(m_connections[ePlatform].oConnectionStatusMutex);

		m_connections[ePlatform].bFailedToConnect=true;
		m_connections[ePlatform].bIsConnecting=false;

		m_connections[ePlatform].nStatusIndex=3;
		m_connections[ePlatform].boPlatformStatusUpdated=true;
	}
	else
	{
		CryAutoLock<CryMutex>	oAutoLock(m_connections[ePlatform].oConnectionStatusMutex);
		m_connections[ePlatform].bFailedToConnect=false;
		m_connections[ePlatform].bIsConnecting=true;

		m_connections[ePlatform].nStatusIndex=4;
		m_connections[ePlatform].boPlatformStatusUpdated=true;
	}
	
	return m_connections[ePlatform].pClient;
}

void CConsoleConnection::NotificationEnd(CHotUpdateSystem::eConsolePlatforms ePlatform)
{
	LockDebug("Lock CConsoleConnection::NotificationEnd m_connections[%d]\n",(int)ePlatform);
	m_connections[ePlatform].oConnectionMutex.Unlock();
}

void CConsoleConnection::NotificationReset(CHotUpdateSystem::eConsolePlatforms ePlatform)
{
	LockDebug("Lock CConsoleConnection::NotificationReset m_connections[%d]\n",(int)ePlatform);
	m_connections[ePlatform].oConnectionMutex.Lock();
	SAFE_RELEASE(m_connections[ePlatform].pClient);
	m_connections[ePlatform].oConnectionMutex.Unlock();

	CryAutoLock<CryMutex>	oAutoLock(m_connections[ePlatform].oConnectionStatusMutex);
	m_connections[ePlatform].bIsConnecting=false;

	if (!gSettings.oHotUpdateSystemSettings.boPlatformEnabled[ePlatform])
	{
		m_connections[ePlatform].nStatusIndex=0;
		m_connections[ePlatform].boPlatformStatusUpdated=true;
	}

	LockDebug("Unlock CConsoleConnection::NotificationReset m_connections[%d]\n",(int)ePlatform);
}

bool CConsoleConnection::Connect(CHotUpdateSystem::eConsolePlatforms ePlatform)
{
	if (!m_connections[ePlatform].pClient)
	{
		return false;
	}

	if (m_connections[ePlatform].pClient->IsConnected())
	{
		return true;
	}

	string address;
	if (!RetrieveConnectionAddress(ePlatform, address))
	{
		CryAutoLock<CryMutex>	oAutoLock(m_connections[ePlatform].oConnectionStatusMutex);
		m_connections[ePlatform].bFailedToConnect=true;

		m_connections[ePlatform].nStatusIndex=1;
		m_connections[ePlatform].boPlatformStatusUpdated=true;

		return false;
	}

	CryAutoLock<CryMutex>	oAutoLock(m_connections[ePlatform].oConnectionStatusMutex);
	m_connections[ePlatform].bFailedToConnect=false;
	m_connections[ePlatform].bIsConnecting=true;
	m_connections[ePlatform].pClient->RegisterCallbackListener(this);

	m_connections[ePlatform].nStatusIndex=4;
	m_connections[ePlatform].boPlatformStatusUpdated=true;

	return m_connections[ePlatform].pClient->Connect(address, DEFAULT_PORT);
}

void CConsoleConnection::Run()
{
	string result;
	if (!ConCom(m_ePlatform, NULL, m_strParameters.c_str(), result))
		return;

	GetIEditor()->GetSystem()->GetILog()->Log(result);

	m_ePlatform=(CHotUpdateSystem::eConsolePlatforms)0;
	m_strParameters=""; //Not needed, but not bad.
}

void CConsoleConnection::OnConnect(INotificationNetworkClient* pClient,bool bSucceeded)
{
	bool				bFailed(!bSucceeded);

	for (size_t ePlatform=0;ePlatform<CHotUpdateSystem::eNumberOfPlatforms;++ePlatform)
	{
		if (m_connections[ePlatform].pClient==pClient)
		{	
			CryAutoLock<CryMutex>	oAutoLock(m_connections[ePlatform].oConnectionStatusMutex);
			m_connections[ePlatform].bFailedToConnect=bFailed;
			m_connections[ePlatform].bIsConnecting=false;

			if (bFailed)
			{
				m_connections[ePlatform].nStatusIndex=3;
			}
			else
			{
				m_connections[ePlatform].nStatusIndex=5;
			}
			m_connections[ePlatform].boPlatformStatusUpdated=true;

			return;
		}
	}
}

void CConsoleConnection::OnDisconnected(INotificationNetworkClient* pClient)
{
	for (size_t ePlatform=0;ePlatform<CHotUpdateSystem::eNumberOfPlatforms;++ePlatform)
	{
		if (m_connections[ePlatform].pClient==pClient)
		{
			CryAutoLock<CryMutex>	oAutoLock(m_connections[ePlatform].oConnectionStatusMutex);
			m_connections[ePlatform].bIsConnecting=false;

			m_connections[ePlatform].nStatusIndex=6;
			m_connections[ePlatform].boPlatformStatusUpdated=true;

			return;
		}
	}
}


void CConsoleConnection::BeginStatusCheck(CHotUpdateSystem::eConsolePlatforms ePlatform)
{
	m_connections[ePlatform].oConnectionStatusMutex.Lock();
}


void CConsoleConnection::EndStatusCheck(CHotUpdateSystem::eConsolePlatforms ePlatform)
{
	m_connections[ePlatform].oConnectionStatusMutex.Unlock();
}

bool CConsoleConnection::IsFailedToConnect(CHotUpdateSystem::eConsolePlatforms ePlatform)
{
	return m_connections[ePlatform].bFailedToConnect;
}

bool CConsoleConnection::IsConnected(CHotUpdateSystem::eConsolePlatforms ePlatform)
{
	if (!m_connections[ePlatform].pClient)
	{
		return false;
	}

	return m_connections[ePlatform].pClient->IsConnected();
}

bool CConsoleConnection::IsConnecting(CHotUpdateSystem::eConsolePlatforms ePlatform)
{
	if (!m_connections[ePlatform].pClient)
	{
		return false;
	}

	return m_connections[ePlatform].bIsConnecting;
}

void CConsoleConnection::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
		case eNotify_OnIdleUpdate:
		{
			CMainFrame*	poMainFrame=((CMainFrame*)AfxGetMainWnd());
			CString			strPlatformStatusString;

			if (!poMainFrame)
			{
				return;
			}

			for (size_t nCount=CHotUpdateSystem::eFirstPlatform;
									nCount<CHotUpdateSystem::eNumberOfPlatforms;
									++nCount)
			{
				if (!m_connections[nCount].boPlatformStatusUpdated)
				{
					continue;
				}

				poMainFrame->SetItemStatusText(m_connections[nCount].nStatusItemId,GetPlatformStatusString((CHotUpdateSystem::eConsolePlatforms)nCount,strPlatformStatusString));
				m_connections[nCount].boPlatformStatusUpdated=false;
			}
		}
		break;
	}
}

CString& CConsoleConnection::GetPlatformStatusString(CHotUpdateSystem::eConsolePlatforms ePlatform,CString& strStatus)
{
	static const char* aMessages[]=
	{
		"Not Connected",
		"No Target Address",
		"No Notification Network",
		"Connection Failed",
		"Connecting",
		"Connected",
		"Disconnected",
	};

	assert("CConsoleConnection::GetPlatformStatusString"&&(m_connections[ePlatform].nStatusIndex<=6));
	if (m_connections[ePlatform].nStatusIndex>6)
	{
		strStatus="Editor Error";
		return strStatus;
	}

	strStatus=m_connections[ePlatform].strPlatformName;
	strStatus+=aMessages[m_connections[ePlatform].nStatusIndex];

	return strStatus;
}
