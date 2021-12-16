#include "../StdTypes.hpp"
#ifdef _MSC_VER
#include <process.h>
#include <direct.h>
#endif
#ifdef UNIX
#include <pthread.h>
#endif
#include "../Error.hpp"
#include "../STLHelper.hpp"
#include "../tinyxml/tinyxml.h"
#include "CrySimpleJob.hpp"
#include "CrySimpleSock.hpp"
#include "CrySimpleServer.hpp"
#include "CrySimpleCache.hpp"
#include "ShaderList.hpp"

#define MAX_COMPILER_WAIT_TIME (60*1000)

volatile long g_GlobalRequestNumber = 0;
volatile long g_GlobalCompileTasks	= 0;
volatile long g_RemoteServerID			= 0;
volatile long long g_GlobalCompileTime	=	0;

struct SCompileTaskAutoTracker
{
	SCompileTaskAutoTracker() { InterlockedIncrement( &g_GlobalCompileTasks); }
	~SCompileTaskAutoTracker() { InterlockedDecrement( &g_GlobalCompileTasks); }
};

struct STimer
{
	__int64 m_freq;
	STimer()
	{
		QueryPerformanceFrequency((LARGE_INTEGER *)&m_freq);
	}
	__int64 GetTime() const
	{
		__int64 t;
		QueryPerformanceCounter((LARGE_INTEGER *)&t);
		return t;
	}

	double TimeToSeconds( __int64 t )
	{
		return ((double)t)/m_freq;
	}
};

/*
void LogReguest(uint32_t requestIP,std::vector<uint8_t>& rVec)
{
	static CCrySimpleMutex gMutex;
	
	m_ID	=	CSTLHelper::Hash(rVec);

	std::string Request(reinterpret_cast<const char*>(&rVec[0]),rVec.size());
	TiXmlDocument ReqParsed( "Request.xml" );
	ReqParsed.Parse( Request.c_str() );

	const char* pShaderRequestLine =	"";
	const TiXmlElement* pElement = ReqParsed.FirstChildElement();
	if (pElement)
	{
		pShaderRequestLine =	pElement->Attribute( "ShaderRequest" );
	}

	unsigned int crcVal	=	CSTLHelper::Crc32(rVec);
	int nSize = rVec.size();

	std::string strHash = CSTLHelper::Hash2String(m_ID);

	std::vector<uint8_t> rTemp(rVec);
	const char *sInCache = "Not In Cache";
	if (CCrySimpleCache::Instance().Find(m_ID,rTemp))
		sInCache = "In Cache";

	gMutex.Lock();
	FILE *f = fopen( "Crc32_Log.txt","at" );
	if (f)
	{
		fprintf( f,"Sum:%x  Size:%d Hash(%s) (%s) (%s)\n",crcVal,nSize,strHash.c_str(),sInCache,pShaderRequestLine );
		fclose(f);
	}
	gMutex.Unlock();
}
}
*/

STimer g_Timer;

CCrySimpleJob::CCrySimpleJob(uint32_t requestIP,std::vector<uint8_t>& rVec)
	: m_RequestIP(requestIP)
{
	InterlockedIncrement( &g_GlobalRequestNumber );

	m_ID	=	CSTLHelper::Hash(rVec);

	if(CCrySimpleCache::Instance().Find(m_ID,rVec))
	{
		printf( "\r%d",g_GlobalRequestNumber );
	}
	else
	{
		bool LocalCompile=true;
		if(!SEnviropment::Instance().m_FallbackServer.empty() && 
			g_GlobalCompileTasks > SEnviropment::Instance().m_FallbackTreshold)
		{
			tdEntryVec ServerVec;
			CSTLHelper::Tokenize(ServerVec,SEnviropment::Instance().m_FallbackServer,";");
			uint32_t Idx=g_RemoteServerID++;
			uint32_t Count=(uint32_t)ServerVec.size();
			std::string Server	=	ServerVec[Idx%Count];
			printf("  Remote Compile on %s ...\n",Server.c_str());
			CCrySimpleSock Sock(Server,SEnviropment::Instance().m_port);
			if(Sock.Valid())
			{
				Sock.Forward(rVec);
				std::vector<uint8_t> Tmp;
				if(Sock.Backward(Tmp))
				{
					rVec					=	Tmp;
					if(Tmp.size()==0)
					{
						CrySimple_ERROR("failed to compile request");
						return;
					}
					LocalCompile	=	false;
					//printf("done\n");
				}
				else
					printf("failed fallback to local\n");
			}
			else
				printf("failed fallback to local\n");
		}
		if(LocalCompile)
		{
			SCompileTaskAutoTracker trackNumTasks;

			std::string Request(reinterpret_cast<const char*>(&rVec[0]),rVec.size());
			TiXmlDocument ReqParsed( "Request.xml" );
			ReqParsed.Parse( Request.c_str() );

			if(ReqParsed.Error())
			{
				CrySimple_ERROR("failed to parse request XML");
				return;
			}

			if(!Compile(ReqParsed,rVec) || rVec.size() == 0)
			{
				CrySimple_ERROR("failed to compile request");
				return;
			}

			tdDataVector rDataRaw;
			rDataRaw.swap(rVec);
			if (!CSTLHelper::Compress( rDataRaw,rVec ))
			{
				CrySimple_ERROR("failed to compress request");
				return;
			}
		}
		// Cache compiled data
		CCrySimpleCache::Instance().Add(m_ID,rVec);
	}
}

bool CCrySimpleJob::Execute(const std::string& rCmd,std::string &outError)
{
#ifdef _MSC_VER
	bool	Ret	= false;
	DWORD	ExitCode	=	0;

	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcessInfo;
	memset(&StartupInfo, 0, sizeof(StartupInfo));
	memset(&ProcessInfo, 0, sizeof(ProcessInfo));
	StartupInfo.cb = sizeof(StartupInfo);

	
	std::string Path="";
	std::string::size_type Pt = rCmd.find_first_of(' ');
	if(Pt!=std::string::npos)
	{
		std::string First	=	std::string(rCmd.c_str(),Pt);
		std::string::size_type Pt2 = First.find_last_of('/');
		if(Pt2!=std::string::npos)
			Path	=	std::string(First.c_str(),Pt2);
		else
			Pt	=	std::string::npos;
	}

	HANDLE hReadErr, hWriteErr;

	{
		CreatePipe(&hReadErr, &hWriteErr, NULL, 0);
		SetHandleInformation(hWriteErr, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

		StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		StartupInfo.hStdError = hWriteErr;
		StartupInfo.dwFlags |= STARTF_USESTDHANDLES;

		if(CreateProcess(NULL,(char*)rCmd.c_str(),0,0, TRUE,CREATE_DEFAULT_ERROR_MODE,0,Pt!=std::string::npos?Path.c_str():0,&StartupInfo,&ProcessInfo) != false)
		{
			std::string error;

			DWORD waitResult = 0;
			HANDLE waitHandles[] = { ProcessInfo.hProcess, hReadErr };
			while(true)
			{
				//waitResult = WaitForMultipleObjects(sizeof(waitHandles) / sizeof(waitHandles[0]), waitHandles, FALSE, 1000 );
				waitResult = WaitForSingleObject(ProcessInfo.hProcess, 1000 );
				if (waitResult == WAIT_FAILED)
					break;

				DWORD bytesRead, bytesAvailable;
				while(PeekNamedPipe(hReadErr, NULL, 0, NULL, &bytesAvailable, NULL) && bytesAvailable)
				{
					char buff[4096];
					ReadFile(hReadErr, buff, sizeof(buff)-1, &bytesRead, 0);
					buff[bytesRead] = '\0';
					error += buff;
				}

				//if (waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT)
					//break;

				if (waitResult == WAIT_OBJECT_0)
					break;
			}

			//if (waitResult != WAIT_TIMEOUT)
			{
				GetExitCodeProcess(ProcessInfo.hProcess,&ExitCode);
				if (ExitCode)
				{
					Ret = false;
					outError = error;
				}
				else
				{
					Ret = true;
				}
			}
			/*
			else
			{
				Ret = false;
				outError = std::string("Timed out executing compiler: ") + rCmd;
				TerminateProcess(ProcessInfo.hProcess, 1);
			}
			*/

			CloseHandle(ProcessInfo.hProcess);
			CloseHandle(ProcessInfo.hThread);
		}

		CloseHandle(hReadErr);
		if (hWriteErr)
			CloseHandle(hWriteErr);
	}

	return Ret;
#endif

#ifdef UNIX
	return system((rFileName + " " + rParams).c_str()) == 0;
#endif
}

bool CCrySimpleJob::Compile(const TiXmlDocument& rReqParsed,std::vector<uint8_t>& rVec)
{
	const TiXmlElement* pElement = rReqParsed.FirstChildElement();
	if(!pElement)
	{
		CrySimple_ERROR("failed to extract First Element of the request");
		return false;
	}

	const char* pProfile			=	pElement->Attribute( "Profile" );
	const char* pProgram			=	pElement->Attribute( "Program" );
	const char* pEntry				=	pElement->Attribute( "Entry" );
	const char* pCompileFlags	=	pElement->Attribute( "CompileFlags" );

	const char* pShaderRequestLine =	pElement->Attribute( "ShaderRequest" );
	
	const char* platform = "";
	if (strstr(pCompileFlags,"PC") != 0)
	{
		CShaderList::Instance().m_PC.InsertLine( pShaderRequestLine );
		platform = "PC";
	}		
	if (strstr(pCompileFlags,"X360") != 0)
	{
		CShaderList::Instance().m_X360.InsertLine( pShaderRequestLine );
		platform = "Xbox360";
	}		
	if (strstr(pCompileFlags,"PS3") != 0)
	{
		CShaderList::Instance().m_PS3.InsertLine( pShaderRequestLine );
		platform = "PS3";
	}		

	if(!pProfile)
	{
		CrySimple_ERROR("failed to extract Profile of the request");
		return false;
	}
	if(!pProgram)
	{
		CrySimple_ERROR("failed to extract Profile of the request");
		return false;
	}
	if(!pEntry)
	{
		CrySimple_ERROR("failed to extract Profile of the request");
		return false;
	}
	if(!pCompileFlags)
	{
		CrySimple_ERROR("failed to extract Compile+Flags of the request");
		return false;
	}

	const std::string Hash	=	CSTLHelper::Hash2String(m_ID);

	static long volatile nTmpCounter = 0;

	InterlockedIncrement( &nTmpCounter );

	char tmpstr[64];
	sprintf( tmpstr,"%d",nTmpCounter );

	const std::string TmpIn	=	SEnviropment::Instance().m_Temp+tmpstr+".In";
	const std::string TmpOut=	SEnviropment::Instance().m_Temp+tmpstr+".Out";
	CSTLHelper::ToFile(TmpIn,std::vector<uint8_t>(pProgram,&pProgram[strlen(pProgram)]));

	char BuildCmd[1024];
	sprintf(BuildCmd,pCompileFlags,pEntry,pProfile,TmpOut.c_str(),TmpIn.c_str());

	const std::string Cmd=SEnviropment::Instance().m_Compiler+BuildCmd;

/*	const uint32_t Ret	=	WinExec(Cmd.c_str(),SW_HIDE);

	if(Ret>31)//http://msdn.microsoft.com/en-us/library/ms687393(VS.85).aspx
	{
		if(Ret==0)
		{
			CrySimple_ERROR("CCrySimpleJob::Compile Error:The system is out of memory or resources.");
			return false;
		}
		if(Ret==ERROR_BAD_FORMAT)
		{
			CrySimple_ERROR("CCrySimpleJob::Compile Error:The .exe file is invalid.");
			return false;
		}
		if(Ret==ERROR_FILE_NOT_FOUND)
		{
			CrySimple_ERROR("CCrySimpleJob::Compile Error:The specified file was not found.");
			return false;
		}
		if(Ret==ERROR_PATH_NOT_FOUND)
		{
			CrySimple_ERROR("CCrySimpleJob::Compile Error:The specified path was not found..");
			return false;
		}
	}
*/
	__int64 t0 = g_Timer.GetTime();

	std::string outError;
	if(!Execute(Cmd,outError))
	{
		unsigned char* nIP = (unsigned char*) &m_RequestIP;
		char sIP[128];
		sprintf(sIP, "%d.%d.%d.%d", nIP[0], nIP[1], nIP[2], nIP[3]);

		std::string errorString;
		errorString += std::string("Request IP:          ") + sIP + "\n";
		errorString += std::string("Platform:            ") + platform + "\n";
		errorString += std::string("Target profile:      ") + pProfile + "\n";
		errorString += std::string("Entry function:      ") + pEntry + "\n";
		errorString += std::string("Shader request line: ") + pShaderRequestLine + "\n";
		errorString += std::string("Reported error(s):   ") + "\n";
		errorString += outError + "\n";

		remove(TmpIn.c_str());
		remove(TmpOut.c_str());

		char sErrText[512];
		sprintf_s( sErrText,"  Error Compiling (%s)(%s) %s\n",platform,pProfile,pEntry );

		CryCompiler_ERROR("Shader compile error",errorString);
		return false;
	}

	if (!CSTLHelper::FromFile(TmpOut,rVec))
	{
		remove(TmpIn.c_str());
		remove(TmpOut.c_str());
		CrySimple_ERROR(std::string("Could not read: ")+TmpOut);
	}
	remove(TmpIn.c_str());
	remove(TmpOut.c_str());

	__int64 t1 = g_Timer.GetTime();
	__int64 dt = t1-t0;
	InterlockedAdd64( &g_GlobalCompileTime,dt );

	int millis = (int)(g_Timer.TimeToSeconds(dt) * 1000.0);
	int secondsTotal = (int)g_Timer.TimeToSeconds(g_GlobalCompileTime);
	printf( "  Compiled [%4dms|%dms] (%s)(%s) %s\n",millis,secondsTotal,platform,pProfile,pEntry );

	return true;
}

