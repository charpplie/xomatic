/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: Logs data to a centralised server for later analysis
				 various game subsystems, forwarding necessary events to stats
				 recording system.

	-------------------------------------------------------------------------
	History:
	- 18:11:2009  : Created by Mark Tully

*************************************************************************/

#include "StdAfx.h"
#include "TelemetryCollector.h"
#include "Game.h"

#include "ICryTelemetry.h"
#include "ILevelSystem.h"

enum ETelemetryTransactionRecording			// if you change these values, update the cvar help string
{
	k_recording_never=0,
	k_recording_ifServiceUnavailable=1,
	k_recording_always=2
};

enum
{
	TLOG_CRITICAL,
	TLOG_STANDARD,
	TLOG_VERBOSE
};

#define k_defaultTelemetryServerPath	"/~cryuser/telemetry"
#define k_defaultTelemetryServerPathSP "/~steven/telemetry"
#define k_defaultTelemetryPort			80
#define k_defaultTelemetryServer		"nottelemetry01.intern.crytek.de"			// 360 needs fully qualified domain name
#define k_telemetry_submitLogCommand	"telemetry_submitlog"
#define k_telemetry_getSessionIdCommand	"telemetry_getsessionid"
#define k_defaultTelemetryEnabled		1
#define k_defaultTelemetryLogging		0
#define k_defaultTelemetryTransactionRecording	k_recording_ifServiceUnavailable

#define STRINGIZE(x)		#x

void CTelemetryCollector::Log(int level, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	if (level == TLOG_CRITICAL)
	{
		gEnv->pLog->LogV( ILog::eAlways,format,args );
	}
	else if (level <= m_telemetryServerLogging->GetIVal())
	{
		gEnv->pLog->LogV( ILog::eMessage,format,args );
	}

	va_end(args);
}

CTelemetryCollector::CTelemetryCollector() :
	m_pTelemetry(NULL)
{
	m_telemetryTransactionRecordings=REGISTER_INT("g_telemetry_transaction_recording",k_defaultTelemetryTransactionRecording,NULL,
		"Usage: g_telemetry_transaction_recording <2/1/0 - record condition>\n"
		"Telemetry can be logged to a file in a format that can be uploaded to the server later\n"
		"0 - never log, 1 - only log if server is unreachable and telemetry is enabled, 2 - always log\n"
		"Default is " STRINGIZE(k_defaultTelemetryTransactionRecording));
	m_telemetryEnabled=REGISTER_INT("g_telemetry_enabled",k_defaultTelemetryEnabled,NULL,
		"Usage: g_telemetry_enabled <1/0 - upload telemetry>\n"
		"When set telemetry will be uploaded to the server configured in g_telemetry_server\n");
	m_serverTelemetryPath=REGISTER_STRING("g_telemetry_serverPath",k_defaultTelemetryServerPath, NULL,
		"Usage: g_telemetry_serverPath <path prefix on server>\n"
		"Default is " k_defaultTelemetryServerPath);
	m_serverTelemetryPathSP=REGISTER_STRING("g_telemetry_serverPathSP",k_defaultTelemetryServerPathSP, NULL,
		"Usage: g_telemetry_serverPathSP <path prefix on server for SP telemetry>\n"
		"Default is " k_defaultTelemetryServerPathSP);
	m_serverNameCVar=REGISTER_STRING("g_telemetry_server",k_defaultTelemetryServer, NULL,
		"Usage: g_telemetry_server <ip or hostname>\n"
		"Default is " k_defaultTelemetryServer);
	m_serverPortCVar=REGISTER_INT("g_telemetry_port",k_defaultTelemetryPort,NULL,
		"Usage: g_telemetry_port <port>\n"
		"Default is " STRINGIZE(k_defaultTelemetryPort));
	m_telemetryServerLogging = REGISTER_INT("g_telemetry_logging", k_defaultTelemetryLogging, NULL, "Usage: g_telemetry_logging <level>\nWhere level is 0, 1, or 2 - 0 is critical log messages only, 2 being verbose\nDefault is " STRINGIZE(k_defaultTelemetryLogging));

	REGISTER_COMMAND(k_telemetry_submitLogCommand, (ConsoleCommandFunc)SubmitGameLog, 0, "Saves the Game.log to the gamelogger server");
	REGISTER_COMMAND(k_telemetry_getSessionIdCommand, (ConsoleCommandFunc)OutputSessionId, 0, "Outputs the current telemetry session id to the console");

	m_lastLevelRotationIndex = 0;

	g_pGame->GetIGameFramework()->GetILevelSystem()->AddListener(this);

	char fileName[] = "%USER%/TelemetryTransactions/memory_stats.txt";
	m_telemetryMemoryLogPath=fileName;
	gEnv->pCryPak->RemoveFile(m_telemetryMemoryLogPath.c_str());
	FILE *file = gEnv->pCryPak->FOpen(m_telemetryMemoryLogPath.c_str(), "at");
	if (file)
	{
		gEnv->pCryPak->FPrintf(file, "<sheet>\n");
		gEnv->pCryPak->FPrintf(file, "</sheet>\n");
		gEnv->pCryPak->FClose(file);
	}

	SetNewSessionId();

	m_websafeClientName = GetWebSafeClientName();
	m_fileUploadCounter = 0;

	string	tmpStr = "%USER%/TelemetryTransactions";
	char path[ICryPak::g_nMaxPath];
	path[sizeof(path) - 1] = 0;
	gEnv->pCryPak->AdjustFileName(tmpStr, path, ICryPak::FLAGS_PATH_REAL | ICryPak::FLAGS_FOR_WRITING);

	m_telemetryRecordingPath=path;

	gEnv->pCryPak->MakeDir(m_telemetryRecordingPath.c_str());
}

CTelemetryCollector::~CTelemetryCollector()
{
	IConsole		*ic=gEnv->pConsole;
	if (ic)
	{
		ic->UnregisterVariable(m_telemetryTransactionRecordings->GetName());
		ic->UnregisterVariable(m_telemetryEnabled->GetName());
		ic->UnregisterVariable(m_serverTelemetryPath->GetName());
		ic->UnregisterVariable(m_serverTelemetryPathSP->GetName());
		ic->UnregisterVariable(m_serverNameCVar->GetName());
		ic->UnregisterVariable(m_serverPortCVar->GetName());
		ic->UnregisterVariable(m_telemetryServerLogging->GetName());
		ic->RemoveCommand(k_telemetry_submitLogCommand);
		ic->RemoveCommand(k_telemetry_getSessionIdCommand);
	}
}

// static
// outputs the session id to the console
void CTelemetryCollector::OutputSessionId(IConsoleCmdArgs *inArgs)
{
	CTelemetryCollector		*tc=static_cast<CTelemetryCollector*>(static_cast<CGame*>(gEnv->pGame)->GetITelemetryCollector());
	CryLogAlways("Telemetry: Session id = '%s'",tc->m_curSessionId.c_str());
}

// static
// test function which uploads the game.log
void CTelemetryCollector::SubmitGameLog(IConsoleCmdArgs *inArgs)
{
#if defined(PS3)
	// all file ops are redirected by cryengine to the local drive. the log file has a load of ps3 hacks to go to app home.
	// if we want to be able to open the log file and read from it, we need to replicate those hacks. it's not that important right now.
	CryLogAlways("SubmitGameLog not implemented on PS3");
#else
	ITelemetryCollector		*tc=static_cast<CGame*>(gEnv->pGame)->GetITelemetryCollector();
	const char				*logFile=gEnv->pSystem->GetILog()->GetFileName();

	if (logFile && logFile[0]!='.' && logFile[0]!='%')		// if the log file isn't set to write into an alias or the current directory, it will default to writing into the current directory. we need to prepend our path to access it from here
	{
		CryFixedStringT<512>					modLogFile;
		modLogFile.Format(".\\%s",logFile);
		tc->SubmitFile(modLogFile);
	}
	else
	{
		tc->SubmitFile(logFile);
	}
#endif
}

void CTelemetryCollector::EscapeString(
	string				*ioString)
{
	/*	from RFC2396
		escape the following by replacing them with %<hex of their ascii code>
	 
			delims      = "<" | ">" | "#" | "%" | <">
			unwise      = "{" | "}" | "|" | "\" | "^" | "[" | "]" | "`"
			reserved    = ";" | "/" | "?" | ":" | "@" | "&" | "=" | "+" |
								"$" | ","
		also replace spaces with +
			space->+
	*/
	const char				k_escapeCharacters[]="%<>#\"{}|\\^[]`;/?:@&=+$,";		// IMPORTANT: replace % first!
	const int				k_numEscapedCharacters=sizeof(k_escapeCharacters)-1;
	int						numToReplace=0;
	int						origLen=ioString->length();

	{
		const char		*rawStr=ioString->c_str();

		for (int i=0; i<k_numEscapedCharacters; i++)
		{
			char	escChar=k_escapeCharacters[i];

			for (int j=0; j<origLen; j++)
			{
				if (rawStr[j]==escChar)
				{
					numToReplace++;
				}
			}
		}
	}

	ioString->reserve(origLen+numToReplace*2);		// replace with 3 chars, as escaped chars are replaced with a % then two hex numbers. replace len = 3, old len = 1, need numReplace*2 extra bytes

	{
		CryFixedStringT<8>		find,replace;

		for (int i=0; i<k_numEscapedCharacters; i++)
		{
			char	escChar=k_escapeCharacters[i];

			find.Format("%c",escChar);
			replace.Format("%%%x",int(escChar));
			ioString->replace(find,replace);
		}
		ioString->replace(' ','+');
	}
}

const char *CTelemetryCollector::GetWebSafePlatformName()
{
#if defined(WIN32) || defined(WIN64)
	return "pc";
#elif defined(XENON)
	return "360";
#elif defined(PS3)
	return "ps3";
#else
#error unknown platform
	return "unknown";
#endif
}

string CTelemetryCollector::GetWebSafeSessionId()
{
	string	webSafe(m_curSessionId);
	EscapeString(&webSafe);
	return webSafe;
}

string CTelemetryCollector::GetWebSafeClientName()
{
	const char	*hostName=gEnv->pNetwork->GetHostName();
	if (!hostName || !*hostName)
	{
		hostName="unknown host";
	}
	
	string		webSafe;

	const char *userAlias = gEnv->pCryPak->GetAlias("%USER%");
	if (userAlias)
	{
		const char *userAliasOpenBracket = strchr(userAlias, '(');
		const char *userAliasCloseBracket = strchr(userAlias, ')');

		if (userAliasOpenBracket && userAliasCloseBracket && ((userAliasCloseBracket - userAliasOpenBracket) <= 3))
		{
			char extractUserNumber[25] = {0};
			strncpy(extractUserNumber, userAliasOpenBracket + 1, (userAliasCloseBracket - userAliasOpenBracket) - 1);

			webSafe.Format("%s_%s", extractUserNumber, hostName);
		}
		else
		{
			webSafe = hostName;
		}
	}
	else
	{
		webSafe = hostName;
	}

	EscapeString(&webSafe);

	return webSafe;
}

bool CTelemetryCollector::InitService()
{
	if (m_pTelemetry)		// Its most likely we are initialised
		return true;

	// Set server parameters in the lobby side telemetry poster (will crash if no lobby service)
	ICryTelemetry *telemService = gEnv->pNetwork->GetLobby()->GetLobbyService()->GetTelemetry();

	if (telemService)
	{
		telemService->ConfigureRemoteServer(m_serverNameCVar->GetString(),m_serverPortCVar->GetIVal());

		m_pTelemetry=telemService;

		return true;
	}

	return false;
}

static void submitFileTelemetryCallback(ECryTelemetryResult res, void* arg)
{
	int intarg = (int) arg;

	if (res == eCTR_Ok)
	{
		ICVar	*telemetryServerLogging = gEnv->pConsole->GetCVar("g_telemetry_logging");

		if (!telemetryServerLogging || telemetryServerLogging->GetIVal() == 2)
		{
			CryLog("Telemetry: Successfully sent job '%d'", intarg);
		}
	}
	else
	{
		CryLogAlways("Telemetry: Failed to send job '%d'", intarg);
	}
}

// writes a http post header for the telemetry request into the buffer provided
// returns the length of the header so other data can be packed behind it
// inDataLength should be the length of the data that will follow
int CTelemetryCollector::MakePostHeader(
	const char			*inRemoteFileName,
	int					inDataLength,
	char				*outBuffer,
	int					inMaxBufferSize,
	TTelemetrySubmitFlags	inFlags)
{
#define MAX_HEADER_SIZE			(1024)
	CryFixedStringT<MAX_HEADER_SIZE>	httpPostHeader;
	const char	*destFile=inRemoteFileName;
	const char	*pathPrefix= gEnv->bMultiplayer ? m_serverTelemetryPath->GetString() : m_serverTelemetryPathSP->GetString();
	if (!pathPrefix)
	{
		pathPrefix=k_defaultTelemetryServerPath;
	}

	// strip .\ off the beginning of file names
	// .\ is used to refer to a file in the games root directory but putting .\ at the beginning of file names on the server is
	// not the intention
	if (destFile[0]=='.' && destFile[1]=='\\')
	{
		destFile+=2;
	}

	if (destFile[0] && destFile[1]==':')		// start with a drive letter? we don't want those on the server
	{
		destFile+=2;
		if (destFile[0]=='/' || destFile[0]=='\\')
		{
			destFile++;
		}
	}

	char szFullPathBuf[ICryPak::g_nMaxPath];

	// Would be nice to also specify ICryPak::FLAGS_NO_LOWCASE here, but that also stops the expansion of %USER% into the user directory [TF]
	ICryPak			*pak=gEnv->pCryPak;
	pak->AdjustFileName(destFile, szFullPathBuf, ICryPak::FLAGS_NO_FULL_PATH);

	string webSafeFileName(szFullPathBuf);
	EscapeString(&webSafeFileName);

	// No longer needed - this is done by AdjustFileName
#if 0
	// replace windows slashes with unix slashes (proper slashes :) )
	webSafeFileName.replace('\\','/');
#endif

	httpPostHeader.Format("POST %s/filestore.php?filename=%s&session2=%s&client=%s&platform=%s&isserver=%d&append=%d HTTP/1.0\n"
		"Content-Type: text/plain\n"
		"Content-Length: %d\n"
		"\n",
		pathPrefix,
		webSafeFileName.c_str(),GetWebSafeSessionId().c_str(),GetWebSafeClientName().c_str(),GetWebSafePlatformName(),gEnv->bServer!=0,(inFlags&k_tf_appendToRemoteFile)!=0,inDataLength);

	int len=httpPostHeader.length();

	if (len>=inMaxBufferSize)
	{
		CRY_ASSERT_MESSAGE(len<inMaxBufferSize,"Http header too long for provided buffer");
		len=inMaxBufferSize;
	}

	memcpy(outBuffer,httpPostHeader.c_str(),len);

	return len;
}

// submits a file to the game logger server
// returns true if successful, false if not
bool CTelemetryCollector::SubmitFile(
	const char			*inLocalFilePath)
{
	bool				success=false;
	int					shouldSubmit=ShouldSubmitTelemetry();

	if (shouldSubmit)
	{
		ICryPak			*pak=gEnv->pCryPak;
		FILE			*file=pak->FOpen(inLocalFilePath,"rb",ICryPak::FLAGS_PATH_REAL);

		if (!file)
		{
			Log(TLOG_CRITICAL, "Telemetry: Failed to open file '%s' for submit",inLocalFilePath);
		}
		else
		{
			pak->FSeek(file,0, SEEK_END);
			int nSize=pak->FTell(file);
			pak->FSeek(file, 0, SEEK_SET);
			
			STelemetryDataPtr pUploadData = new STelemetryData(nSize + k_maxHttpHeaderSize);
			if (!pUploadData->pData)
			{
				Log(TLOG_CRITICAL, "Telemetry: Failed to allocate %d bytes to submit file '%s'",nSize+k_maxHttpHeaderSize,inLocalFilePath);
			}
			else
			{
				int		headerLen=MakePostHeader(inLocalFilePath,nSize,pUploadData->pData,k_maxHttpHeaderSize,k_tf_none);

				pak->FReadRaw(pUploadData->pData+headerLen,nSize,1,file);

				success=UploadData(pUploadData,inLocalFilePath);
			}

			pak->FClose(file);
		}
	}

	return success;
}

// returns whether telemetry will be submitted or logged. if false, there is no point generating the telemetry and submitting it as it will
// go nowhere
bool CTelemetryCollector::ShouldSubmitTelemetry()
{
	int					enabled=m_telemetryEnabled->GetIVal();
	int					recording=m_telemetryTransactionRecordings->GetIVal();
	int					serviceAvailable=enabled && InitService();

	return (enabled && (serviceAvailable || recording==k_recording_ifServiceUnavailable)) || (recording==k_recording_always);
}

// uploads data to the telemetry server and logs the transaction if required
// returns whether or not the upload was queued successfully
bool CTelemetryCollector::UploadData(
	STelemetryDataPtr pData,
	const char				*inReferenceFileName)
{
	bool					success=false;
	int						recording=m_telemetryTransactionRecordings->GetIVal();
	int						enabled=m_telemetryEnabled->GetIVal();

	if (enabled && InitService())
	{
		pData->telemCb = submitFileTelemetryCallback;
		pData->userArg = (void*)m_fileUploadCounter;
		success=m_pTelemetry->UploadData(pData);
		if (success)
		{
			Log(TLOG_STANDARD, "Telemetry: Successfully queued append to file '%s' to server, job ID is %d", inReferenceFileName, m_fileUploadCounter);
		}
		else
		{
			Log(TLOG_CRITICAL, "Telemetry: Failed to queue submit of file '%s'.", inReferenceFileName);
		}
	}

	if ((!success && recording==k_recording_ifServiceUnavailable) || recording==k_recording_always)
	{
		ICryPak		*pak=gEnv->pCryPak;
		string		sessionId=GetSessionId();

		if (sessionId.empty())
		{
			sessionId="no_session";
		}

		CryFixedStringT<1024>	path;

		path.Format("%s/%s_%d.tel",m_telemetryRecordingPath.c_str(),sessionId.c_str(),m_fileUploadCounter);

		FILE		*file=pak->FOpen(path,"wb",ICryPak::FLAGS_PATH_REAL);
		int			written=0;
		if (file)
		{
			written=pak->FWrite(pData->pData,1,pData->length,file);
			pak->FClose(file);
		}

		if (!file || written!=pData->length)
		{
			Log(TLOG_CRITICAL, "Telemetry: Failed to record submit of file '%s' to transactions log.", inReferenceFileName);
		}
		else
		{
			Log(TLOG_STANDARD, "Telemetry: Successfully recorded submit of file '%s' to transactions log.",inReferenceFileName);
		}
	}
	
	m_fileUploadCounter++;

	return success;
}

bool CTelemetryCollector::AppendToFile(const char *inLocalFilePath, const char *inDataToAppend)
{
	bool				success=false;
	bool				shouldSubmit=ShouldSubmitTelemetry();

	if (shouldSubmit)
	{
		int			nSize=strlen(inDataToAppend);

		STelemetryDataPtr pUploadData = new STelemetryData(nSize + k_maxHttpHeaderSize);
		
		if (!pUploadData->pData)
		{
			Log(TLOG_CRITICAL, "Telemetry: Failed to allocate %d bytes to append to file '%s'",nSize+k_maxHttpHeaderSize,inLocalFilePath);
		}
		else
		{
			int	headerlen=MakePostHeader(inLocalFilePath,nSize,pUploadData->pData,k_maxHttpHeaderSize,k_tf_appendToRemoteFile);

			memcpy(pUploadData->pData+headerlen,inDataToAppend,nSize);

			success=UploadData(pUploadData,inLocalFilePath);
		}
	}

	return success;
}

void CTelemetryCollector::OutputTelemetryServerHintFile()
{
	char headerMem[k_maxHttpHeaderSize];

	int headerLen=MakePostHeader("_TelemetryServerDestFile_",-666666,headerMem,k_maxHttpHeaderSize,k_tf_none);
	
#if defined(XENON)
	char fileName[] = "telemetry_server_hint.txt"; // doesn't like relative paths? or long filenames?
#else
	char fileName[] = "../telemetry_server_hint.txt";
#endif

	FILE *file = gEnv->pCryPak->FOpen( fileName,"wt" );
	if (file)
	{
		gEnv->pCryPak->FWrite(headerMem, headerLen, file);
		gEnv->pCryPak->FClose(file);
	}
}

void CTelemetryCollector::OutputMemoryUsage(const char *message, const char *newLevelName)
{
	IMemoryManager::SProcessMemInfo processMemInfo;
	GetISystem()->GetIMemoryManager()->GetProcessMemInfo(processMemInfo);
	const float cpuMemUsedInMB = (float)(processMemInfo.PagefileUsage)/(1024.0f*1024.0f);

	CryFixedStringT<128> timeStr;
	time_t ltime;
	time( &ltime );
	tm *today = localtime( &ltime );
	strftime(timeStr.m_str, timeStr.MAX_SIZE, "%Y-%m-%d %H:%M:%S", today);

	const char *mapName;
	if (g_pGame && g_pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel())
	{
		mapName = g_pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel()->GetLevelInfo()->GetDisplayName();
	}
	else
	{
		mapName = "unknown";
	}

	FILE *file = gEnv->pCryPak->FOpen(m_telemetryMemoryLogPath.c_str(), "r+");
	if (file)
	{
		int fileSize=gEnv->pCryPak->FGetSize(file);
		gEnv->pCryPak->FSeek(file, fileSize-10, SEEK_SET);
		gEnv->pCryPak->FPrintf(file, "  <row timestamp=\"%s\" type=\"%s\" map1=\"%s\" map2=\"%s\" memoryused=\"%f\">\n", timeStr.c_str(), message, mapName, newLevelName, cpuMemUsedInMB);
		gEnv->pCryPak->FPrintf(file, "</sheet>\n");
		//gEnv->pCryPak->FPrintf(file, "%s - %s - %s -> %s - %f\n", timeStr.c_str(), message, mapName, newLevelName, cpuMemUsedInMB);
		gEnv->pCryPak->FClose(file);
	}

	SubmitFile(m_telemetryMemoryLogPath.c_str());
}

string CTelemetryCollector::GetSessionId()
{
	return m_curSessionId;
}

void CTelemetryCollector::SetSessionId(
	string		inNewId)
{
	m_curSessionId=inNewId;

	OutputTelemetryServerHintFile();
	OutputMemoryUsage("SetSessionID", "unknown");

#if !_RELEASE
	gEnv->pConsole->ExecuteString("memReplayLabel SetSessionId");
#endif
}

// the session id is used when uploading to the telemetry server to ensure all clients in the game store their telemetry
// in the same place
void CTelemetryCollector::SetNewSessionId()
{
	CryFixedStringT<1024>		hostNameStr;

	ICVar		*serverName=gEnv->pConsole->GetCVar("sv_servername");
	if (serverName)
	{
		hostNameStr=serverName->GetString();
	}

	if (hostNameStr.empty())
	{
		const char	*hostName=gEnv->pNetwork->GetHostName();
		if (!hostName || !*hostName)
		{
			hostName="unknown host";
		}
		hostNameStr=hostName;
	}

	CryFixedStringT<64> rotationStr;
	ILevelRotation *pLevelRotation = g_pGame->GetIGameFramework()->GetILevelSystem()->GetLevelRotation();
	if (pLevelRotation && pLevelRotation->GetLength() > 1)
	{
		int next = pLevelRotation->GetNext();

		if (m_lastLevelRotationIndex > 0 && next == 0)
		{
			// next has looped around to the first level, display the last level for this session
			next = pLevelRotation->GetLength();
		}
		rotationStr.Format("round_%02d_", next);
		m_lastLevelRotationIndex = next;
	}

	CryFixedStringT<128> timeStr;
	time_t ltime;
	time( &ltime );
	tm *today = localtime( &ltime );
	strftime(timeStr.m_str, timeStr.MAX_SIZE, "%H%M%S", today);

	SetSessionId(string().Format("%s_%s%s",hostNameStr.c_str(),rotationStr.c_str(), timeStr.c_str()));
}

void CTelemetryCollector::OnLoadingStart(ILevelInfo *pLevel)
{
//	CryLog("CTelemetryCollector::OnLoadingStart()");
//	OutputMemoryUsage("OnLoadingStart", pLevel->GetDisplayName());
#if !_RELEASE
	gEnv->pConsole->ExecuteString("memReplayLabel loadStart");
#endif
}

void CTelemetryCollector::OnLoadingComplete(ILevel *pLevel)
{
//	CryLog("CTelemetryCollector::OnLoadingComplete()");
	OutputMemoryUsage("OnLoadingComplete", pLevel->GetLevelInfo()->GetDisplayName());
#if !_RELEASE
	gEnv->pConsole->ExecuteString("memReplayLabel loadEnd");
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Telemetry buffer utility class
////////////////////////////////////////////////////////////////////////////////

CTelemetryBuffer::CTelemetryBuffer(int buffersize, ITelemetryCollector *collector, size_t structSize) : m_collector(collector), m_structSize(structSize)
{
	m_pBuffer = new CRecordingBuffer(buffersize);
}

void CTelemetryBuffer::AddData(ITelemetryBufferData *dataStruct)
{
	m_pBuffer->AddPacket(*dataStruct);
}

void CTelemetryBuffer::SubmitToServer(const char *filename)
{
#define FIXED_BUFFER_SIZE (10 * 1024)
	CryFixedStringT<FIXED_BUFFER_SIZE> tempBuffer;

	size_t offset = 0;
	for (offset = 0; offset < m_pBuffer->size(); offset+=m_structSize)
	{
		ITelemetryBufferData *packet = (ITelemetryBufferData *) m_pBuffer->at(offset);
		CRY_ASSERT_MESSAGE(packet->size == m_structSize, "Struct size is not correct, something went wrong here");

		CryFixedStringT<1024> oneString;
		FormatBufferData(packet, oneString);

		if ((tempBuffer.length() + oneString.length()) > FIXED_BUFFER_SIZE)
		{
			m_collector->AppendToFile(filename, tempBuffer);
			tempBuffer.clear();
		}
		
		tempBuffer.append(oneString);
	}

	m_collector->AppendToFile(filename, tempBuffer);
}

void CTelemetryBuffer::DumpToFile(const char *filename)
{
#define FIXED_BUFFER_SIZE (10 * 1024)
	CryFixedStringT<FIXED_BUFFER_SIZE> tempBuffer;

	FILE *file = gEnv->pCryPak->FOpen(filename, "wt");
	if (file)
	{
		size_t offset = 0;
		for (offset = 0; offset < m_pBuffer->size(); offset+=m_structSize)
		{
			ITelemetryBufferData *packet = (ITelemetryBufferData *) m_pBuffer->at(offset);
			CRY_ASSERT_MESSAGE(packet->size == m_structSize, "Struct size is not correct, something went wrong here");
			CryFixedStringT<1024> oneString;
			FormatBufferData(packet, oneString);

			if ((tempBuffer.length() + strlen(oneString)) > FIXED_BUFFER_SIZE)
			{
				m_collector->AppendToFile(filename, tempBuffer);
				gEnv->pCryPak->FWrite(tempBuffer, 1, tempBuffer.length(), file);
				tempBuffer.clear();
			}

			tempBuffer.append(oneString);
		}

		gEnv->pCryPak->FWrite(tempBuffer, 1, tempBuffer.length(), file);
		gEnv->pCryPak->FClose(file);
	}
}

void CTelemetryBuffer::FormatBufferData(ITelemetryBufferData* packet, CryFixedStringT<1024> &outString)
{
	switch (packet->type)
	{
	case eTPT_Performance:
		{
			SPerformanceTelemetry* perfPacket = (SPerformanceTelemetry*)packet;
			outString.Format("%.2f\t%d\t%f\t%d\n", perfPacket->m_timeInSeconds, perfPacket->m_numTicks, perfPacket->m_gpuTime, perfPacket->m_gpuLimited);
		}
		break;
	case eTPT_Bandwidth:
		{
			SBandwidthTelemetry* bandPacket = (SBandwidthTelemetry*)packet;
			outString.Format("%.2f\t%lld\t%lld\t%d\n", bandPacket->m_timeInSeconds, bandPacket->m_bandwidthSent, bandPacket->m_bandwidthReceived, bandPacket->m_packetsSent);
		}
		break;
	case eTPT_Memory:
		{
			SMemoryTelemetry* memPacket = (SMemoryTelemetry*)packet;
			outString.Format("%.2f\t%.3f\n", memPacket->m_timeInSeconds, memPacket->m_cpuMemUsedInMB);
		}
		break;
	default:
		{
			outString.Format("TelemetryCollector::FormatBufferData ERROR unknown telemetry packet type '%d'", packet->type);
			CryLog( "TelemetryCollector::FormatBufferData ERROR unknown telemetry packet type '%d'", packet->type);
		}
		break;
	}
}

void CTelemetryBuffer::Reset()
{
	m_pBuffer->Reset();
}
