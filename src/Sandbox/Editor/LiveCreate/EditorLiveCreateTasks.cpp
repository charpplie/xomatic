////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2011.
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <IServiceNetwork.h>
#include <IRemoteCommand.h>
#include "LiveCreate/EditorLiveCreateHostInfo.h"
#include "LiveCreate/EditorLiveCreateManager.h"
#include "EditorLiveCreateTasks.h"
#include "CrySocks.h"

#ifndef NO_LIVECREATE

namespace LiveCreate
{

//-----------------------------------------------------------------------------

CBGTask_ResolveAddress::CBGTask_ResolveAddress(IPlatformHandlerFactory* pFactory, const CString& targetName)
	: m_pFactory(pFactory)
	, m_targetName(targetName)
{
}

ETaskResult CBGTask_ResolveAddress::Work()
{
	const uint32 kAddressSize = 32;

	// use the platform factory to resolve the IP address (slow)
	char tempIpAddres[kAddressSize];
	if (!m_pFactory->ResolveAddress(m_targetName, tempIpAddres, kAddressSize))
	{
		return eTaskResult_Failed;
	}

	GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Normal, "[LC] Target '%s' on platform '%s' resolved to '%s'",
		(const char*)m_targetName,
		m_pFactory->GetPlatformName(),
		tempIpAddres );

	// save the address
	m_resolvedAddress = tempIpAddres;

	// we have finished the work
	return eTaskResult_Completed;
}

//-----------------------------------------------------------------------------

CBGTask_LaunchExecutable::CBGTask_LaunchExecutable(IPlatformHandler* pPlatform, const CString& directory, const CString& executable, const CString& args)
	: m_pPlatform(pPlatform)
	, m_directory(directory)
	, m_executable(executable)
	, m_args(args)
{
	m_pPlatform->AddRef();
}

CBGTask_LaunchExecutable::~CBGTask_LaunchExecutable()
{
	m_pPlatform->Release();
}

ETaskResult CBGTask_LaunchExecutable::Work()
{
	// Launch 
	GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Normal, "Launching '%s' on '%s'...", 
		(const char*)m_executable, m_pPlatform->GetTargetName());
	if (!m_pPlatform->Launch(m_executable, m_directory, m_args))
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Error, "Failed to launch '%s' on '%s'", 
			(const char*)m_executable, m_pPlatform->GetTargetName());

		return eTaskResult_Failed;
	}

	// Assume it's launched
	return eTaskResult_Completed;
}

//-----------------------------------------------------------------------------

CBGTask_Connect::CBGTask_Connect(IHostInfo* pHost)
	: m_pHost(pHost)
{
	m_pHost->AddRef();
}

CBGTask_Connect::~CBGTask_Connect()
{
	m_pHost->Release();
}

ETaskResult CBGTask_Connect::Work()
{
	GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Normal, "Trying to connect to target '%s', platform '%s'...",
		m_pHost->GetTargetName(),
		m_pHost->GetPlatformName());

	if (m_pHost->Connect())
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Normal, "Connected to target '%s', platform %s",
			m_pHost->GetTargetName(),
			m_pHost->GetPlatformName());

		// we have connected
		return eTaskResult_Completed;
	}

	// we didn't connect
	return eTaskResult_Failed;
}

//-----------------------------------------------------------------------------

CBGTask_SearchForHosts::CBGTask_SearchForHosts(CEditorManager* pManager)
	: m_numTargets(0)
{
	// Collect the registered platforms
	const uint32 numPlatforms = pManager->GetNumPlatforms();
	for (uint32 i=0; i<numPlatforms; ++i)
	{
		IPlatformHandlerFactory* pFactory = pManager->GetPlatform(i);
		m_pFactories.push_back(pFactory);
	}
}

ETaskResult CBGTask_SearchForHosts::Work()
{
	// process all available platform factories
	for (uint32 i=0; i<m_pFactories.size(); ++i)
	{
		IPlatformHandlerFactory* pFactory = m_pFactories[i];

		const uint32 maxTargets = kMaxTargets - m_numTargets;
		const uint32 numPlatformTargets = pFactory->ScanForTargets(&m_targets[m_numTargets], maxTargets);

		// make sure the factory data is set up
		for (uint32 j=0; j<numPlatformTargets; ++j)
		{
			m_targets[m_numTargets+j].pFactory = pFactory;
		}

		m_numTargets += numPlatformTargets;
	}

	// finished
	return eTaskResult_Completed;
}

//-----------------------------------------------------------------------------

CBGTask_SearchForHostsForPlatform::CBGTask_SearchForHostsForPlatform(IPlatformHandlerFactory* pFactory)
	: m_pFactory(pFactory)
	, m_numTargets(0)
{
}

ETaskResult CBGTask_SearchForHostsForPlatform::Work()
{
	const uint32 maxTargets = kMaxTargets - m_numTargets;
	const uint32 numPlatformTargets = m_pFactory->ScanForTargets(&m_targets[m_numTargets], maxTargets);

	// make sure the factory data is set up
	for (uint32 j=0; j<numPlatformTargets; ++j)
	{
		m_targets[m_numTargets+j].pFactory = m_pFactory;
	}

	m_numTargets = numPlatformTargets;
	return eTaskResult_Completed;
}

//-----------------------------------------------------------------------------

CBGTask_ScanDirectory::CBGTask_ScanDirectory(IPlatformHandler* pPlatform, const CString& remotePath)
	: m_pPlatform(pPlatform)
	, m_path(remotePath)
{
	m_pPlatform->AddRef();
}

CBGTask_ScanDirectory::~CBGTask_ScanDirectory()
{
	m_pPlatform->Release();
}

ETaskResult CBGTask_ScanDirectory::Work()
{
	if (m_pPlatform->ScanFolder(m_path, *this))
	{
		return eTaskResult_Completed;
	}
	else
	{
		return eTaskResult_Failed;
	}
}

void CBGTask_ScanDirectory::OnFolder(const char* szBasePath, const char* szEntryName)
{
	m_directories.push_back(szEntryName);
}

void CBGTask_ScanDirectory::OnFile(const char* szBasePath, const char* szEntryName, bool bIsExecutable)
{
	if (bIsExecutable)
	{
		m_executables.push_back(szEntryName);
	}
	else
	{
		m_files.push_back(szEntryName);
	}
}

//-----------------------------------------------------------------------------

CBGTask_CopyFileToTarget::CBGTask_CopyFileToTarget(const CString& address, const CString& srcPath, const CString& destPath, bool bIsSourceAbsolute/*=false*/, bool bAutoDeletesource/*=false*/)
	: m_address(address)
	, m_srcPath(srcPath)
	, m_destPath(destPath)
	, m_bIsSourceAbsolute(bIsSourceAbsolute)
	, m_bAutoDeleteSource(bAutoDeletesource)
{
}

CBGTask_CopyFileToTarget::~CBGTask_CopyFileToTarget()
{
	if (m_bAutoDeleteSource && m_bIsSourceAbsolute)
	{
		::DeleteFileA(m_srcPath);
	}
}

class CFileStream 
{
private:
	FILE* m_file;

public:
	CFileStream(FILE* file)
		: m_file(file)
	{}

	~CFileStream()
	{
		gEnv->pCryPak->FClose(m_file);
	}

	void Read(const uint32 offset, const uint32 size, void* pBuffer)
	{
		gEnv->pCryPak->FSeek(m_file, offset, SEEK_SET);
		gEnv->pCryPak->FReadRaw(pBuffer, size, 1, m_file);
	}
};

ETaskResult CBGTask_CopyFileToTarget::Work()
{
	const CTimeValue startTime = gEnv->pTimer->GetAsyncTime();

	// General global file transfer ID
	static uint32 NextFileTransferID = 1;
	const uint32 fileTransferID = NextFileTransferID++;
		
	// Open file locally
	const uint32 fileSize = gEnv->pCryPak->GetFileSizeOnDisk(m_srcPath);
	if (fileSize == 0)
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "File '%s' has zero size or does not exist. Not sending.", (const char*)m_srcPath);
		return eTaskResult_Failed;
	}

	// Open the source file
	FILE* sourceFile = gEnv->pCryPak->FOpen(m_srcPath, "rb", ICryPak::FOPEN_ONDISK); // closed automatically in ~CFileStream
	if (NULL == sourceFile)
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "File '%s' transfer error: failed to open source file.", (const char*)m_srcPath);
		return eTaskResult_Failed;
	}

	CFileStream reader(sourceFile); // will close the file in destructor

	// Connect to the server
	const ServiceNetworkAddress address = gEnv->pServiceNetwork->GetHostAddress((const char*)m_address, LiveCreate::kDefaultFileTransferServicePort);
	IServiceNetworkConnection* pConnection = gEnv->pServiceNetwork->Connect(address);
	if (NULL == pConnection)
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "File '%s' transfer error: failed to connect to file transfer service on '%s'.", 
			(const char*)m_srcPath, (const char*)m_address);
		return eTaskResult_Failed;
	}

	// Send header
	{
		TAutoDelete<IDataWriteStream> writer(gEnv->pServiceNetwork->CreateMessageWriter());
		writer->WriteString(m_destPath);
		writer->WriteUint32(fileTransferID);
		writer->WriteUint32(fileSize);

		IServiceNetworkMessage* pMessage = writer->BuildMessage();
		pConnection->SendMsg(pMessage);
		SAFE_RELEASE(pMessage);
	}

	// Wait for response
	uint8 responseCode = 2; // timeout
	{
		const CTimeValue responseWaitStart = gEnv->pTimer->GetAsyncTime();
		const float maxResponseWaitTime = 2.0f;
		while (gEnv->pTimer->GetAsyncTime().GetDifferenceInSeconds(responseWaitStart) < maxResponseWaitTime)
		{
			IServiceNetworkMessage* pResponse = pConnection->ReceiveMsg();
			if (NULL != pResponse)
			{
				responseCode = ((const uint8*)pResponse->GetPointer())[0];
				pResponse->Release();
				break;
			}

			Sleep(100);
		}
	}

	// Error response
	if (1 != responseCode)
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "File '%s' transfer error: no initial ACK. Error code: %d", (const char*)m_srcPath, responseCode);
		pConnection->Close();
		pConnection->Release();
		return eTaskResult_Failed;
	}

	// Send file data
	const uint32 kBlockSize = 1 << 18; // 250 KB
	uint32 fileOffset = 0;
	while (fileOffset < fileSize && !IsCanceled())
	{
		const uint32 sizeToRead = min<uint32>(fileSize - fileOffset, kBlockSize);

		// connection died
		if (!pConnection->IsAlive())
		{
			GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Normal, "File '%s' transfered failed: Connection closed", (const char*)m_srcPath);
			pConnection->FlushAndClose();
			pConnection->Release();
			break;
		}

		// send file data
		IServiceNetworkMessage* pMessage = gEnv->pServiceNetwork->AllocMessageBuffer(sizeToRead);
		reader.Read(fileOffset, sizeToRead, pMessage->GetPointer());
		if (pConnection->SendMsg(pMessage))
		{
			fileOffset += sizeToRead;
		}
		else
		{
			Sleep(100); // yield some time to process the messages
		}

		pMessage->Release();
	}

	// wait for the connection to send all data
	if (!IsCanceled())
	{
		pConnection->FlushAndWait();
	}

	// wait before closing the connection
	Sleep(100);
	pConnection->Close();
	pConnection->Release();

	// File transfer completed
	const float time = gEnv->pTimer->GetAsyncTime().GetDifferenceInSeconds(startTime);
	GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Normal, "File '%s' transfered: %1.3fs (%1.2f MB/s)", 
		(const char*)m_srcPath, time, (float)(fileSize / time) / (1024.0f*1024.0f));
	return eTaskResult_Completed;
}

//-----------------------------------------------------------------------------

CBGTask_ExecuteCommand::CBGTask_ExecuteCommand(const CString& address, const CString& command, const float maxWaitTime/*=3.0f*/, const uint32 maxRetryCount/*=10*/)
	: m_address(address)
	, m_command(command)
	, m_waitTime(maxWaitTime)
	, m_retryCount(maxRetryCount)
{
}

CBGTask_ExecuteCommand::~CBGTask_ExecuteCommand()
{
}

ETaskResult CBGTask_ExecuteCommand::Work()
{
	// allocate unique console ID
	CryGUID commandId;
	HRESULT hRet = CoCreateGuid((GUID*)&commandId);
	if (FAILED(hRet))
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "Command error: failed to allocate command GUID."); 
		return eTaskResult_Failed;
	}

	// Connect to the server
	const ServiceNetworkAddress address = gEnv->pServiceNetwork->GetHostAddress((const char*)m_address, LiveCreate::kDefaultHostListenPort);
	IServiceNetworkConnection* pConnection = gEnv->pServiceNetwork->Connect(address);
	if (NULL == pConnection)
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "Command error: failed to connect to '%s'.", 
			(const char*)m_address);

		return eTaskResult_Failed;	
	}

	// retry loop
	bool bSuccess = false;
	bool bGotResponse = false;
	for (uint32 i=0; i<m_retryCount && !bGotResponse; ++i)
	{
		if (IsCanceled())
		{
			break;
		}

		const CTimeValue startTime = gEnv->pTimer->GetAsyncTime();


		// Send command
		{
			TAutoDelete<IDataWriteStream> writer(gEnv->pServiceNetwork->CreateMessageWriter());
			writer->WriteString("ConsoleCommand");
			writer->WriteUint64(commandId.lopart);
			writer->WriteUint64(commandId.hipart);
			writer->WriteString(m_command);

			IServiceNetworkMessage* pMessage = writer->BuildMessage();
			pConnection->SendMsg(pMessage);
			SAFE_RELEASE(pMessage);
		}

		// Wait for response
		while (gEnv->pTimer->GetAsyncTime().GetDifferenceInSeconds(startTime) < m_waitTime && !bGotResponse)
		{
			IServiceNetworkMessage* pResponseMessage = pConnection->ReceiveMsg();
			while (NULL != pResponseMessage && !bGotResponse)
			{
				TAutoDelete<IDataReadStream> reader(pResponseMessage->CreateReader());
				const string reponseString = reader->ReadString();
				if (reponseString == "CmdResponse")
				{
					const uint64 responseLo = reader->ReadUint64();
					const uint64 responseHi = reader->ReadUint64();
					if (responseLo == commandId.lopart && responseHi == commandId.hipart)
					{
						const string status = reader->ReadString();
						if (status == "OK")
						{
							GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Normal, "RemoteCommand '%s' for '%s': SUCCESS", (const char*)m_command, (const char*)m_address);
							bSuccess = true;
						}
						else
						{
							GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "RemoteCommand '%s' for '%s': FAILED: %s", (const char*)m_command, (const char*)m_address, status.c_str());
							bSuccess = false;
						}

						bGotResponse = true;
					}
					else
					{
						GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "RemoteCommand '%s' for '%s': INVALID ID", (const char*)m_command, (const char*)m_address);
					}
				}
				else
				{
					GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "RemoteCommand '%s' for '%s': INVALID RESPONSE '%s'", (const char*)m_command, (const char*)m_address, reponseString.c_str());
				}

				// get next
				SAFE_RELEASE(pResponseMessage);
				pResponseMessage = pConnection->ReceiveMsg();
			}

			// yield some thread time
			SAFE_RELEASE(pResponseMessage);
			Sleep(100);
		}
	}

	// Timed out
	if (!bGotResponse)
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "RemoteCommand '%s' for '%s': TIMED OUT", (const char*)m_command, (const char*)m_address);
		bSuccess = false;
	}

	// close connection
	pConnection->Close();
	pConnection->Release();

	// File transfer completed
	return bSuccess ? eTaskResult_Completed : eTaskResult_Failed;
}

//-----------------------------------------------------------------------------

CBGTask_GetHostInfoPacket::CBGTask_GetHostInfoPacket(const CString& address)
	: m_address(address)
{
}

CBGTask_GetHostInfoPacket::~CBGTask_GetHostInfoPacket()
{
}

ETaskResult CBGTask_GetHostInfoPacket::Work()
{
	// Create socket
	CRYSOCKET s = CrySock::socket(AF_INET,SOCK_DGRAM,0);
	if (s <= 0)
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "socket() failed: %d", s);
		return eTaskResult_Failed;
	}

	// Setup some default timeout
	CrySock::SetRecvTimeout(s, 2, 0);
	CrySock::SetSendTimeout(s, 2, 0);

	// Prepare address
	sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(m_address);
	addr.sin_port = htons(LiveCreate::kDefaultDiscoverySerivceListenPost);

	// Prepare packet data
	char buffer[1024];
	int bufferSize = 0;
	{
		TAutoDelete<IDataWriteStream> writer(gEnv->pServiceNetwork->CreateMessageWriter());
		writer->WriteString("GetInfo");
		writer->CopyToBuffer(buffer);
		bufferSize = writer->GetSize();
	}

	// Send data
	int ret = CrySock::sendto(s, buffer, bufferSize, 0, (sockaddr *)&addr, sizeof(addr));
	if (bufferSize != ret)
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "sendto() to '%s' failed: %d (%d)", (const char*)m_address, ret, bufferSize);
		CrySock::closesocket(s);
		return eTaskResult_Failed;
	}

	// Wait for response data
	int addrSize = sizeof(addr);
	ret = CrySock::recvfrom(s, buffer, sizeof(buffer), 0, (sockaddr* )&addr, &addrSize);
	if (ret < 0)
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "recvfrom() from '%s' failed: %d", (const char*)m_address, ret);
		CrySock::closesocket(s);
		return eTaskResult_Failed;
	}

	// Parse data
	TAutoDelete<IDataReadStream> reader(gEnv->pServiceNetwork->CreateMessageReader(buffer, ret));
	const string responseType = reader->ReadString();
	if (responseType != "Info")
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Error, "Invalid response type '%s' for 'GetInfo' request", responseType.c_str());
		CrySock::closesocket(s);
		return eTaskResult_Failed;
	}

	m_packet.Serialize((IDataReadStream&)reader);

	CrySock::closesocket(s);
	return eTaskResult_Completed;
}

//-----------------------------------------------------------------------------

CUDPConnection::CUDPConnection(const CString& addr)
	: m_addrStr(addr)
{// Create socket
	m_socket = CrySock::socket(AF_INET,SOCK_DGRAM,0);

	// Setup some default timeout
	{
		struct timeval timeout;      
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		CrySock::setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
		CrySock::setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
	}

	// Prepare address
	memset(&m_addr,0,sizeof(m_addr));
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = inet_addr(addr);
	m_addr.sin_port = htons(LiveCreate::kDefaultDiscoverySerivceListenPost);
}

CUDPConnection::~CUDPConnection()
{
	CrySock::closesocket(m_socket);
}

IDataReadStream* CUDPConnection::SendWithResponse(IDataWriteStream& writer)
{
	// Prepare packet data
	char buffer[kMaxPacketSize];
	const uint32 bufferSize = writer.GetSize();
	if (bufferSize > sizeof(buffer))
	{
		CryFatalError("Trying to send to large packet via UDP. Packet size = %d.", bufferSize);
		return NULL;
	}
	writer.CopyToBuffer(buffer);

	// Send data
	int ret = sendto(m_socket, buffer, bufferSize, 0, (sockaddr *)&m_addr, sizeof(m_addr));
	if (bufferSize != ret)
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "sendto() to '%s' failed: %d (%d)", (const char*)m_addrStr, ret, bufferSize);
		return NULL;
	}

	// Wait for result data
	int addrSize = sizeof(m_addr);
	ret = recvfrom(m_socket, buffer, sizeof(buffer), 0, (sockaddr* )&m_addr, &addrSize);
	if (ret < 0)
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "recvfrom() from '%s' failed: %d", (const char*)m_addrStr, ret);
		return NULL;
	}

	// Parse data
	return gEnv->pServiceNetwork->CreateMessageReader(buffer, ret);
}

uint8 CUDPConnection::SendWithShortResponse(IDataWriteStream& writer)
{
	// Prepare packet data
	char buffer[kMaxPacketSize];
	const uint32 bufferSize = writer.GetSize();
	if (bufferSize > sizeof(buffer))
	{
		CryFatalError("Trying to send to large packet via UDP. Packet size = %d.", bufferSize);
		return 0;
	}
	writer.CopyToBuffer(buffer);

	// Send data
	int ret = sendto(m_socket, buffer, bufferSize, 0, (sockaddr *)&m_addr, sizeof(m_addr));
	if (bufferSize != ret)
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "sendto() to '%s' failed: %d (%d)", (const char*)m_addrStr, ret, bufferSize);
		return 0;
	}

	// Wait for result data
	int addrSize = sizeof(m_addr);
	ret = recvfrom(m_socket, buffer, sizeof(buffer), 0, (sockaddr* )&m_addr, &addrSize);
	if (ret < 0)
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "recvfrom() from '%s' failed: %d", (const char*)m_addrStr, ret);
		return 0;
	}

	// return response number
	return buffer[0];
}

IDataReadStream* CUDPConnection::Receive()
{
	// Wait for result data
	char buffer[kMaxPacketSize];
	int addrSize = sizeof(m_addr);
	int ret = recvfrom(m_socket, buffer, sizeof(buffer), 0, (sockaddr* )&m_addr, &addrSize);
	if (ret < 0)
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "recvfrom() from '%s' failed: %d", (const char*)m_addrStr, ret);
		return NULL;
	}

	// Parse data
	return gEnv->pServiceNetwork->CreateMessageReader(buffer, ret);
}

void CUDPConnection::SendNoResponse(IDataWriteStream& writer)
{
	// Prepare packet data
	char buffer[kMaxPacketSize];
	const uint32 bufferSize = writer.GetSize();
	if (bufferSize > sizeof(buffer))
	{
		CryFatalError("Trying to send to large packet via UDP. Packet size = %d.", bufferSize);
		return;
	}
	writer.CopyToBuffer(buffer);

	// Send data
	int ret = sendto(m_socket, buffer, bufferSize, 0, (sockaddr *)&m_addr, sizeof(m_addr));
	if (bufferSize != ret)
	{
		GetIEditor()->GetLiveCreate()->LogMessagef(eLogType_Warning, "sendto() to '%s' failed: %d (%d)", (const char*)m_addrStr, ret, bufferSize);
		return;
	}
}

//-----------------------------------------------------------------------------

}

#endif
