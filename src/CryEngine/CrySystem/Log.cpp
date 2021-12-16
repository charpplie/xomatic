	
//////////////////////////////////////////////////////////////////////
//
//	Crytek CryENGINE Source code
//	
//	File:Log.cpp
//  Description:Log related functions
//
//	History:
//	-Feb 2,2001:Created by Marco Corbetta
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Log.h"

//this should not be included here
#include <IConsole.h>
#include <ISystem.h>
#include <IStreamEngine.h>
#include <INetwork.h>  // EvenBalance - M. Quinn
#include "System.h"
#include "CryPath.h"					// PathUtil::ReplaceExtension()

#ifdef WIN32
#include <time.h>
#endif

#ifdef LINUX
#include <syslog.h>
#endif





















//#define RETURN return
//#define RETURN_NULL (return NULL);
#define RETURN_NULL
#define RETURN


// Only accept logging from the main thread.
#ifdef WIN32

#define THREAD_SAFE_LOG
//#define THREAD_SAFE_LOG  CryAutoCriticalSection scope_lock(m_logCriticalSection);

#else
#define THREAD_SAFE_LOG
#endif //WIN32


//////////////////////////////////////////////////////////////////////
namespace LogCVars
{
	float s_log_tick = 0;
};


//////////////////////////////////////////////////////////////////////
CLog::CLog( ISystem *pSystem )
{
	memset(m_szFilename,0,MAX_FILENAME_SIZE);	
	//memset(m_szTemp,0,MAX_TEMP_LENGTH_SIZE);
	m_pSystem=pSystem;
	m_pLogVerbosity = 0;
	m_pLogWriteToFile = 0;
	m_pLogWriteToFileVerbosity = 0;
	m_pLogVerbosityOverridesWriteToFile = 0;
	m_pLogIncludeTime = 0;
	m_pLogSpamDelay = 0;
	m_fLastLoadingUpdateTime = -1.f;	// for streaming engine update

	m_nMainThreadId = CryGetCurrentThreadId();

	m_bAllowDirectLoggingFromAnyThread = false;

	m_pLogFile = NULL;

	m_iLastHistoryItem = 0;
	memset(m_history, 0, sizeof(m_history));
}

void CLog::RegisterConsoleVariables()
{
	IConsole *console = m_pSystem->GetIConsole();
//Diesel cut
#ifdef	_RELEASE
	#define DEFAULT_VERBOSITY 0
#else
	#define DEFAULT_VERBOSITY 3
#endif	
//Diesel cut
	if(console)
	{

		m_pLogVerbosity = REGISTER_INT("log_Verbosity",DEFAULT_VERBOSITY,VF_DUMPTODISK,
			"defines the verbosity level for log messages written to console\n"
			"-1=suppress all logs (including eAlways)\n"
			"0=suppress all logs(except eAlways)\n"
			"1=additional errors\n"
			"2=additional warnings\n"
			"3=additional messages\n"
			"4=additional comments");

		//writing to game.log during game play causes stalls on consoles
		m_pLogWriteToFile = REGISTER_INT("log_WriteToFile",1,VF_DUMPTODISK,"toggle whether to write log to file (game.log)");

		m_pLogWriteToFileVerbosity = REGISTER_INT("log_WriteToFileVerbosity", DEFAULT_VERBOSITY, VF_DUMPTODISK, 
			"defines the verbosity level for log messages written to files\n"
			"-1=suppress all logs (including eAlways)\n"
			"0=suppress all logs(except eAlways)\n"
			"1=additional errors\n"
			"2=additional warnings\n"
			"3=additional messages\n"
			"4=additional comments");
		m_pLogVerbosityOverridesWriteToFile = REGISTER_INT("log_VerbosityOverridesWriteToFile", 1, VF_DUMPTODISK, "when enabled, setting log_verbosity to 0 will stop all logging including writing to file");

		// put time into begin of the string if requested by cvar
		m_pLogIncludeTime = REGISTER_INT("log_IncludeTime",0,0,
			"Toggles time stamping of log entries.\n"
			"Usage: log_IncludeTime [0/1/2/3/4]\n"
			"  0=off (default)\n"
			"  1=current time\n"
			"  2=relative time\n"
			"  3=current+relative time\n"
			"  4=absolute time in seconds since this mode was started");

		m_pLogSpamDelay = REGISTER_FLOAT("log_SpamDelay",0.0f,0, "Sets the minimum time interval between messages classified as spam");
		
		REGISTER_CVAR2("log_tick",&LogCVars::s_log_tick,LogCVars::s_log_tick,0, "When not 0, writes tick log entry into the log file, every N seconds");
	}
/*
	//testbed
	{
		int iSave0 = m_pLogVerbosity->GetIVal();
		int iSave1 = m_pLogFileVerbosity->GetIVal();

		for(int i=0;i<=4;++i)
		{
			m_pLogVerbosity->Set(i);
			m_pLogFileVerbosity->Set(i);

			LogWithType(eAlways,"CLog selftest: Verbosity=%d FileVerbosity=%d",m_pLogVerbosity->GetIVal(),m_pLogFileVerbosity->GetIVal());
			LogWithType(eAlways,"--------------");

			LogWithType(eError,"eError");
			LogWithType(eWarning,"eWarning");
			LogWithType(eMessage,"eMessage");
			LogWithType(eInput,"eInput");
			LogWithType(eInputResponse,"eInputResponse");

			LogWarning("LogWarning()");
			LogError("LogError()");
			LogWithType(eAlways,"--------------");
		}

		m_pLogVerbosity->Set(iSave0);
		m_pLogFileVerbosity->Set(iSave1);
	}
*/
	#undef DEFAULT_VERBOSITY
}

//////////////////////////////////////////////////////////////////////
CLog::~CLog()
{
	CreateBackupFile();

	UnregisterConsoleVariables();

	CloseLogFile(true);
}

void CLog::UnregisterConsoleVariables()
{
	m_pLogVerbosity = 0;
	m_pLogWriteToFile = 0;
	m_pLogWriteToFileVerbosity = 0;
	m_pLogVerbosityOverridesWriteToFile = 0;
	m_pLogIncludeTime = 0;
	m_pLogSpamDelay = 0;
}

//////////////////////////////////////////////////////////////////////////
void CLog::CloseLogFile( bool forceClose )
{
	if(m_pLogFile)
	{







		fclose(m_pLogFile);
		m_pLogFile = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
FILE* CLog::OpenLogFile( const char *filename, const char *mode )
{


















	m_pLogFile = fxopen( filename,mode );
	if (!m_pLogFile)
	{
#ifdef LINUX
		syslog(LOG_NOTICE, "Failed to open log file [%s], mode [%s]", filename, mode);
#endif
	}

	return m_pLogFile;
}


//////////////////////////////////////////////////////////////////////////
void CLog::SetVerbosity( int verbosity )
{
	RETURN;

	if(m_pLogVerbosity)
		m_pLogVerbosity->Set(verbosity);
}

//////////////////////////////////////////////////////////////////////////
void CLog::LogWarning(const char *szFormat,...)
{
	THREAD_SAFE_LOG;

	va_list	ArgList;
	char		szBuffer[MAX_WARNING_LENGTH];
	va_start(ArgList, szFormat);
	int count = vsnprintf_s(szBuffer,sizeof(szBuffer),sizeof(szBuffer)-1,szFormat, ArgList);
	szBuffer[sizeof(szBuffer)-1] = '\0';
	va_end(ArgList);

	va_start(ArgList, szFormat);
	LogV( eWarning, szFormat, ArgList);
	va_end(ArgList);

	IValidator *pValidator = m_pSystem->GetIValidator();
	if (pValidator)
	{
		CryAutoCriticalSection scope_lock(m_logCriticalSection);

		SValidatorRecord record;
		record.text = szBuffer;
		record.module = VALIDATOR_MODULE_SYSTEM;
		record.severity = VALIDATOR_WARNING;
		pValidator->Report( record );
	}
}

//////////////////////////////////////////////////////////////////////////
void CLog::LogError(const char *szFormat,...)
{
	THREAD_SAFE_LOG;

	va_list	ArgList;
	char		szBuffer[MAX_WARNING_LENGTH];
	va_start(ArgList, szFormat);
	int count = vsnprintf_s(szBuffer,sizeof(szBuffer),sizeof(szBuffer)-1, szFormat, ArgList);
	szBuffer[sizeof(szBuffer)-1] = '\0';
	va_end(ArgList);

	va_start(ArgList, szFormat);
	LogV( eError, szFormat, ArgList);
	va_end(ArgList);

	IValidator *pValidator = m_pSystem->GetIValidator();
	if (pValidator)
	{
		CryAutoCriticalSection scope_lock(m_logCriticalSection);

		SValidatorRecord record;
		record.text = szBuffer;
		record.module = VALIDATOR_MODULE_SYSTEM;
		record.severity = VALIDATOR_ERROR;
		pValidator->Report( record );
	}
}

//////////////////////////////////////////////////////////////////////////
void CLog::Log(const char *szFormat,...)
{
	RETURN;

	va_list arg;
	va_start(arg, szFormat);
	LogV (eMessage, szFormat, arg);
	va_end(arg);
}

#pragma warning(push)
#pragma warning( disable:6053 )

int MatchStrings(const char *str0, const char *str1)
{
	const char *str[] = { str0,str1 };
	int i,bSkipWord,bAlpha[2],bWS[2],bStop=0,nDiffs=0,nWordDiffs,len=0;
	do {
		for(i=0;i<2;i++) // skip the spaces, stop at 0
			while(*str[i]==' ') if (!*str[i]++)
				goto break2;
		bWS[0]=bWS[1]=nWordDiffs=bSkipWord = 0;
		do {
			for(i=bAlpha[0]=bAlpha[1]=0;i<2;i++) if (!bWS[i]) do {
				int chr = *str[i]++;
				bSkipWord |= iszero(chr-'\\')|iszero(chr-'/')|iszero(chr-'_'); // ignore different words with \,_,/
				bAlpha[i] = inrange(chr,'A'-1,'Z'+1)|inrange(chr,'a'-1,'z'+1);
				bWS[i] = iszero(chr-' ');
				bStop |= iszero(chr);
			} while (!(bAlpha[i]|bWS[i]|bStop)); // wait for a letter or a space in each input string
			len += bAlpha[0]&bAlpha[1];
			nWordDiffs += 1-iszero((int)(*str[0]-*str[1])) & - (bAlpha[0]&bAlpha[1]);	// count diffs in this word
		} while((1-bWS[0] | 1-bWS[1]) & 1-bStop); // wait for space (word end) in both strings
		nDiffs += nWordDiffs & ~-bSkipWord;
	} while (!bStop);
	break2:
	return nDiffs*10 < len;
}

//will log the text both to file and console
//////////////////////////////////////////////////////////////////////
void CLog::LogV( const ELogType type, const char* szFormat, va_list args )
{





	RETURN;
	if (!szFormat)
		return;

	if (m_pLogVerbosityOverridesWriteToFile && m_pLogVerbosityOverridesWriteToFile->GetIVal())
	{
		if (m_pLogVerbosity && !m_pLogVerbosity->GetIVal())
		{
			return;
		}
	}

	THREAD_SAFE_LOG;

	FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);

	bool bfile = false, bconsole = false;
	const char* szCommand = szFormat;

	uint8 DefaultVerbosity=0;	// 0 == Always log (except for special -1 verbosity overrides)

	switch(type)
  {
		case eAlways:						DefaultVerbosity=0; break;
		case eWarningAlways:		DefaultVerbosity=0; break;
		case eErrorAlways:			DefaultVerbosity=0; break;
		case eInput:						DefaultVerbosity=0; break;
		case eInputResponse:		DefaultVerbosity=0; break;
		case eError:						DefaultVerbosity=1; break;
		case eWarning:					DefaultVerbosity=2; break;
		case eMessage:					DefaultVerbosity=3; break;
		case eComment:					DefaultVerbosity=4; break;

		default: assert(0);
	}

	szCommand = CheckAgainstVerbosity(szFormat, bfile, bconsole,DefaultVerbosity);
	if (!bfile && !bconsole)
			return;

	bool bError = false;
	
	LogStringType tempString;

	char szBuffer[MAX_WARNING_LENGTH+32];
	char *szString = szBuffer;
	char *szAfterColour = szString;







	size_t prefixSize = 0;
	switch(type)
	{
		case eWarning:
		case eWarningAlways:
			bError = true;






			strcpy_s( szString,MAX_WARNING_LENGTH,"$6[Warning] " );
			szString += 12;	// strlen("$6[Warning] ");
			szAfterColour += 2;
			prefixSize = 12;

			break;

		case eError:
		case eErrorAlways:
			bError = true;







			strcpy_s( szString,MAX_WARNING_LENGTH,"$4[Error] " );
			szString += 10;	// strlen("$4[Error] ");
			szAfterColour += 2;
			prefixSize = 10;

			break;

		default:





			break;
	}
	
	int bufferlen = sizeof(szBuffer) - prefixSize;
	if ( bufferlen > 0 )
	{
		int count = vsnprintf_s( szString, bufferlen, bufferlen - 1, szCommand, args );
		if ( count == -1 || count >=bufferlen )
			szBuffer[sizeof(szBuffer)-1] = '\0';
	}

	float dt;
	const char *szSpamCheck = *szFormat=='%' ? szString : szFormat;
	if (m_pLogSpamDelay && (dt=m_pLogSpamDelay->GetFVal())>0.0f && type!=eAlways && type!=eInputResponse) 
	{
		const int sz = sizeof(m_history)/sizeof(m_history[0]);
		int i,j;
		float time = m_pSystem->GetITimer()->GetCurrTime();
		for(i=m_iLastHistoryItem,j=0; m_history[i].time>time-dt && j<sz; j++,i=i-1&sz-1)
		{
			if (m_history[i].type!=type)
				continue;
			if (m_history[i].ptr==szSpamCheck && *(int*)m_history[i].str==*(int*)szFormat || MatchStrings(m_history[i].str,szSpamCheck))
				return;
		}
		i=m_iLastHistoryItem = m_iLastHistoryItem+1 & sz-1;
		strcpy(m_history[i].str, m_history[i].ptr=szSpamCheck);
		m_history[i].type = type;
		m_history[i].time = time;
	}








	if (bfile)
		LogStringToFile( szAfterColour,false,bError );
	if (bconsole)
	{





		LogStringToConsole( szBuffer );
	}
}

#pragma warning(pop)

//will log the text both to the end of file and console
//////////////////////////////////////////////////////////////////////
void CLog::LogPlus(const char *szFormat,...)
{
	if (m_pLogVerbosity && !m_pLogVerbosity->GetIVal())
	{
		return;
	}

	THREAD_SAFE_LOG;

	RETURN;
	if (!szFormat)
		return;

	bool bfile = false, bconsole = false;
	const char* szCommand = CheckAgainstVerbosity(szFormat, bfile, bconsole);
	if (!bfile && !bconsole)
		return;

	va_list		arglist;	

	char szTemp[MAX_TEMP_LENGTH_SIZE];
	va_start(arglist, szFormat);
	vsnprintf_s(szTemp, sizeof(szTemp), sizeof(szTemp) - 1, szCommand, arglist);
	szTemp[sizeof(szTemp)-1]=0;
	va_end(arglist);	

	if (bfile)
		LogToFilePlus(szTemp);		
	if (bconsole)
		LogToConsolePlus(szTemp);	
}

//log to console only
//////////////////////////////////////////////////////////////////////
void CLog::LogStringToConsole( const char *szString,bool bAdd )
{
	//////////////////////////////////////////////////////////////////////////
	if (CryGetCurrentThreadId() != m_nMainThreadId)
	{
		// When logging from other thread then main, push all log strings to queue.
		SLogMsg msg;
		strncpy_s(msg.msg,szString,sizeof(msg.msg)-1); msg.msg[sizeof(msg.msg)-1] = 0;
		msg.bAdd = bAdd;
		msg.bError = false;
		msg.bConsole = true;
		m_threadSafeMsgQueue.push( msg );
		return;
	}
	//////////////////////////////////////////////////////////////////////////

	if (!szString)
		return;

	if (!m_pSystem)
		return;
	IConsole *console = m_pSystem->GetIConsole();
	if (!console)
		return;

	LogStringType tempString;
	tempString = szString;
	if (tempString.length() > MAX_TEMP_LENGTH_SIZE)
		tempString.erase( MAX_TEMP_LENGTH_SIZE );
	// add \n at end.
	if (tempString.length() > 0 && tempString[tempString.length()-1] != '\n')
	{
		tempString += '\n';
	}

	if (bAdd)
		console->PrintLinePlus(tempString.c_str());	
	else
		console->PrintLine(tempString.c_str());

	// Call callback function.
	if (!m_callbacks.empty())
	{
		for (Callbacks::iterator it = m_callbacks.begin(); it != m_callbacks.end(); ++it)
		{
			(*it)->OnWriteToConsole(tempString.c_str(),!bAdd);
		}
	}
}

//log to console only
//////////////////////////////////////////////////////////////////////
void CLog::LogToConsole(const char *szFormat,...)
{
	if (m_pLogVerbosity && !m_pLogVerbosity->GetIVal())
	{
		return;
	}

	RETURN;

	THREAD_SAFE_LOG;

	if (!szFormat)
		return;

	bool bfile = false, bconsole = false;
	const char* szCommand = CheckAgainstVerbosity(szFormat, bfile, bconsole);
	if (!bconsole)
		return;

	va_list		arglist;	

	char szBuffer[MAX_WARNING_LENGTH];
	va_start(arglist, szFormat);
	vsnprintf_s(szBuffer, sizeof(szBuffer), sizeof(szBuffer) - 1, szCommand, arglist);
	szBuffer[sizeof(szBuffer)-1]=0;
	va_end(arglist);

	LogStringToConsole( szBuffer );
}

//////////////////////////////////////////////////////////////////////
void CLog::LogToConsolePlus(const char *szFormat,...)
{
	if (m_pLogVerbosity && !m_pLogVerbosity->GetIVal())
	{
		return;
	}

	RETURN;

	THREAD_SAFE_LOG;

	if (!szFormat)
		return;

	bool bfile = false, bconsole = false;
	const char* szCommand = CheckAgainstVerbosity(szFormat, bfile, bconsole);
	if (!bconsole)
		return;

	va_list		arglist;
	
	char szTemp[MAX_TEMP_LENGTH_SIZE];	
	va_start(arglist, szFormat);
	vsnprintf_s(szTemp, sizeof(szTemp), sizeof(szTemp) - 1, szCommand, arglist);
	szTemp[sizeof(szTemp)-1]=0;
	va_end(arglist);	

	if (!m_pSystem)
		return;

	LogStringToConsole( szTemp,true );
}




//////////////////////////////////////////////////////////////////////
static void RemoveColorCodeInPlace( CLog::LogStringType &rStr )
{
	char *s=(char *)rStr.data();
	char *d=s;

	while(*s!=0)
	{
		if(*s=='$' && *(s+1)>='0' && *(s+1)<='9')
		{
			s+=2;
			continue;
		}

		*d++=*s++;
	}
	*d=0;

	rStr.resize(d-rStr.data());
}

//////////////////////////////////////////////////////////////////////
void CLog::LogStringToFile( const char *szString,bool bAdd,bool bError )
{
//Diesel cut
#if defined(_RELEASE) // no file logging in release
	return;
#endif
//Diesel cut
	if (!szString)
		return;






	//////////////////////////////////////////////////////////////////////////
	if (CryGetCurrentThreadId() != m_nMainThreadId && !m_bAllowDirectLoggingFromAnyThread)
	{
		// When logging from other thread then main, push all log strings to queue.
		SLogMsg msg;
		strncpy_s(msg.msg,szString,sizeof(msg.msg)-1); msg.msg[sizeof(msg.msg)-1] = 0;
		msg.bAdd = bAdd;
		msg.bError = bError;
		msg.bConsole = false;
		m_threadSafeMsgQueue.push( msg );
		return;
	}
	//////////////////////////////////////////////////////////////////////////

	if (!m_pSystem)
		return;
	IConsole * console = m_pSystem->GetIConsole();

	bool bCriticalSectionLocked = false;
	if (m_bAllowDirectLoggingFromAnyThread)
	{
		bCriticalSectionLocked = true;
		m_logCriticalSection.Lock();
	}

	LogStringType tempString;
	tempString = szString;

	// Skip any non character.
	if (tempString.length() > 0 && tempString.at(0) < 32)
	{
		tempString.erase(0,1);
	}

	RemoveColorCodeInPlace(tempString);

	if(m_pLogIncludeTime && gEnv->pTimer)
	{
		uint32 dwCVarState=m_pLogIncludeTime->GetIVal();
//		char szTemp[MAX_TEMP_LENGTH_SIZE];

		if(dwCVarState==1)				// Log_IncludeTime
		{
			char sTime[128];
			time_t ltime;
			time( &ltime );
			struct tm *today = localtime( &ltime );
			strftime( sTime, 20, "<%H:%M:%S> ", today );

			tempString = LogStringType(sTime) + tempString;
		}
		else if(dwCVarState==2)		// Log_IncludeTime
		{
			static CTimeValue lasttime;
			CTimeValue currenttime = gEnv->pTimer->GetAsyncTime();
			if(lasttime!=CTimeValue())
			{
				LogStringType timeStr;
				uint32 dwMs=(uint32)((currenttime-lasttime).GetMilliSeconds());
				timeStr.Format( "<%3d.%.3d>: ", dwMs/1000,dwMs%1000 );
				tempString = timeStr + tempString;
			}
			lasttime = currenttime;
		}
		else if(dwCVarState==3)		// Log_IncludeTime
		{
			char sTime[128];
			time_t ltime;
			time( &ltime );
			struct tm *today = localtime( &ltime );
			strftime( sTime, 20, "<%H:%M:%S> ", today );
			tempString = LogStringType(sTime) + tempString;

			static CTimeValue lasttime;
			CTimeValue currenttime = gEnv->pTimer->GetAsyncTime();
			if(lasttime!=CTimeValue())
			{
				LogStringType timeStr;
				uint32 dwMs=(uint32)((currenttime-lasttime).GetMilliSeconds());
				timeStr.Format( "<%3d.%.3d>: ", dwMs/1000,dwMs%1000 );
				tempString = timeStr + tempString;
			}
			lasttime = currenttime;
		}
		else if(dwCVarState==4)				// Log_IncludeTime
		{
			static bool bFirst = true;
			
			if(gEnv->pTimer)
			{
				static CTimeValue lasttime;
				CTimeValue currenttime = gEnv->pTimer->GetAsyncTime();
				if(lasttime!=CTimeValue())
				{
					LogStringType timeStr;
					uint32 dwMs=(uint32)((currenttime-lasttime).GetMilliSeconds());
					timeStr.Format( "<%3d.%.3d>: ", dwMs/1000,dwMs%1000 );
					tempString = timeStr + tempString;
				}
				if(bFirst)
				{
					lasttime = currenttime;
					bFirst=false;
				}
			}
		}
	}

	// add \n at end.
	if (tempString.empty() || tempString[tempString.length()-1] != '\n')
		tempString += '\n';

	//////////////////////////////////////////////////////////////////////////
	// Call callback function.
	if (!m_callbacks.empty())
	{
		for (Callbacks::iterator it = m_callbacks.begin(); it != m_callbacks.end(); ++it)
		{
			(*it)->OnWriteToFile(tempString.c_str(),!bAdd);
		}
	}
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Write to file.
	//////////////////////////////////////////////////////////////////////////
	int logToFile = m_pLogWriteToFile ? m_pLogWriteToFile->GetIVal() : 1; 

	if(logToFile)
	{
		CDebugAllowFileAccess dafa;
		if (bAdd)
		{
			FILE *fp=OpenLogFile(m_szFilename,"r+t");
			if (fp)
			{
				int nRes;
				int p1 = ftell(fp);
				nRes = fseek(fp,0,SEEK_END); assert(nRes==0);
				p1 = ftell(fp);
				nRes = fseek(fp,-2,SEEK_CUR); assert(nRes==0);
				p1 = ftell(fp);
				(void)nRes;
				fputs(tempString.c_str(),fp);		
				CloseLogFile();
			}
		}
		else
		{
		// comment on bug by TN: Log file misses the last lines of output
		// Temporally forcing the Log to flush before closing the file, so all lines will show up
			if(FILE * fp = OpenLogFile(m_szFilename,"at")) // change to option "atc"
			{
				fputs(tempString.c_str(),fp);
				// fflush(fp);  // enable to flush the file
				CloseLogFile();
			}
		}
	}













	if (bError)
	{
		static int errcount = 0;
#if !defined(PS3)	//for PS3 we do not want too much log traffic over network (file io is slow)
/*
		char errfile[MAX_PATH];
		strcpy( errfile,m_szFilename );
		strcat( errfile,".err" );
		if(FILE * fp = OpenLogFile(errfile,"at"))
		{
			fprintf( fp,"[%d] %s",errcount,szTemp );
			fclose(fp);
		}
		*/
#endif
		errcount++;
	}

	if (bCriticalSectionLocked)
	{
		m_logCriticalSection.Unlock();
	}
}

//same as above but to a file
//////////////////////////////////////////////////////////////////////
void CLog::LogToFilePlus(const char *szFormat,...)
{
	if (m_pLogVerbosity && !m_pLogVerbosity->GetIVal())
	{
		return;
	}

	RETURN;

	THREAD_SAFE_LOG;

	if (!m_szFilename[0] || !szFormat) 
		return;

	bool bfile = false, bconsole = false;
	const char* szCommand = CheckAgainstVerbosity(szFormat, bfile, bconsole);
	if (!bfile)
		return;

	char szTemp[MAX_TEMP_LENGTH_SIZE];
	va_list		arglist;	
	va_start(arglist, szFormat);
	vsnprintf_s(szTemp, sizeof(szTemp),sizeof(szTemp)-1, szCommand, arglist);
	szTemp[sizeof(szTemp)-1]=0;
	va_end(arglist);	

	LogStringToFile( szTemp,true );
}

//log to the file specified in setfilename
//////////////////////////////////////////////////////////////////////
void CLog::LogToFile(const char *szFormat,...)
{
	if (m_pLogVerbosity && !m_pLogVerbosity->GetIVal())
	{
		return;
	}

	RETURN;

	THREAD_SAFE_LOG;

	if (!m_szFilename[0] || !szFormat) 
		return;	 

	bool bfile = false, bconsole = false;
	const char* szCommand = CheckAgainstVerbosity(szFormat, bfile, bconsole);
	if (!bfile)
		return;

	char szTemp[MAX_TEMP_LENGTH_SIZE];
	va_list		arglist;  	
	va_start(arglist, szFormat);
	vsnprintf_s(szTemp, sizeof(szTemp),sizeof(szTemp)-1, szCommand, arglist);
	szTemp[sizeof(szTemp)-1]=0;
	va_end(arglist);	

	LogStringToFile( szTemp,false );
}


//////////////////////////////////////////////////////////////////////
void CLog::CreateBackupFile() const
{
#if defined(PS3) || defined(WIN32) || defined(XENON) || defined(LINUX)
	// simple:
	//		string bakpath = PathUtil::ReplaceExtension(m_szFilename,"bak");
	//		CopyFile(m_szFilename,bakpath.c_str(),false);

	// advanced: to backup directory
	#ifndef PS3
		char temppath[_MAX_PATH];
		char szPath[MAX_FILENAME_SIZE];
	#endif
	string sExt = PathUtil::GetExt(m_szFilename);
	string sFileWithoutExt = PathUtil::GetFileName(m_szFilename);

	{
		assert(::strstr(sFileWithoutExt,":")==0);
		assert(::strstr(sFileWithoutExt,"\\")==0);
	}
	
	PathUtil::RemoveExtension(sFileWithoutExt);

	#define LOG_BACKUP_PATH "LogBackups"

	const char *path = LOG_BACKUP_PATH ;















	string temp = m_pSystem->GetRootFolder();
	temp += path;
	if (temp.size() < MAX_FILENAME_SIZE)
		strcpy(szPath, temp.data());
	else
		strcpy(szPath, path);

	if(!gEnv->pCryPak)
	{
		return;
	}
	string szBackupPath = gEnv->pCryPak->AdjustFileName(szPath,temppath,ICryPak::FLAGS_FOR_WRITING|ICryPak::FLAGS_PATH_REAL);
	gEnv->pCryPak->MakeDir(szBackupPath);
	string sLogFilename = gEnv->pCryPak->AdjustFileName(m_szFilename,temppath,ICryPak::FLAGS_FOR_WRITING|ICryPak::FLAGS_PATH_REAL);



	FILE *in = fxopen(sLogFilename,"rb");


	string sBackupNameAttachment;

	// parse backup name attachment
	// e.g. BackupNameAttachment="attachment name"
	if(in)
	{
		bool bKeyFound=false;
		string sName;

		while(!feof(in))
		{
			uint8 c=fgetc(in);

			if(c=='\"')
			{
				if(!bKeyFound)
				{
					bKeyFound=true;

					if(sName.find("BackupNameAttachment=") == string::npos)
					{
#ifdef WIN32
						OutputDebugString("Log::CreateBackupFile ERROR '");
						OutputDebugString(sName.c_str());
						OutputDebugString("' not recognized \n");
#endif
						assert(0);		// broken log file? - first line should include this name - written by LogVersion()
						return;
					}
					sName.clear();
				}
				else
				{
					sBackupNameAttachment=sName;
					break;
				}
				continue;
			}
			if(c>=' ')
				sName += c;
			else
				break;
		}
		fclose(in);
	}




















		string bakdest = PathUtil::Make(szBackupPath,sFileWithoutExt+sBackupNameAttachment+"."+sExt);
		CopyFile(sLogFilename,bakdest,false);








#endif//WIN32 || PS3 || LINUX
}

//set the file used to log to disk
//////////////////////////////////////////////////////////////////////
bool CLog::SetFileName(const char *filename)
{
	RETURN;
	if (!filename) 
    return false;





	string temp = PathUtil::Make(m_pSystem->GetRootFolder(), PathUtil::GetFile(filename));
	if ( temp.empty() || temp.size() >= MAX_FILENAME_SIZE )
		return false;
	strcpy(m_szFilename, temp.data());

	CreateBackupFile();

	FILE *fp=OpenLogFile(m_szFilename,"wt");
	if (fp)
	{
		CloseLogFile(true);
		return true;
	}

	return false;

	/*
#if !defined(PS3)	//for PS3 we do not want too much log traffic over network (file io is slow)
	// Clear error file.
	char errfile[MAX_PATH];
	strcpy( errfile,m_szFilename );
	strcat( errfile,".err" );
	fp=OpenLogFile(errfile,"wt");
	if (fp)
		fclose(fp);
#endif
	*/
}

//////////////////////////////////////////////////////////////////////////
const char* CLog::GetFileName()
{
	return m_szFilename;
}


//////////////////////////////////////////////////////////////////////
void CLog::UpdateLoadingScreen(const char *szFormat,...)
{
	THREAD_SAFE_LOG;
	{
		va_list args;
		va_start(args, szFormat);

		LogV(ILog::eMessage,szFormat,args);

		va_end(args);
	}

	if (CryGetCurrentThreadId() == m_nMainThreadId)
	{
		((CSystem*)m_pSystem)->UpdateLoadingScreen();

#ifndef LINUX
		// Take this opportunity to update streaming engine.
		const float curTime = m_pSystem->GetITimer()->GetAsyncCurTime();
		if(curTime - m_fLastLoadingUpdateTime > .1f)	// not frequent than once in 100ms
		{
			m_fLastLoadingUpdateTime = curTime;
			IStreamEngine *pStreamEngine = m_pSystem->GetStreamEngine();
			if(pStreamEngine)
				pStreamEngine->Update(IStreamEngine::FLAGS_DISABLE_CALLBACK_TIME_QUOTA);
		}
#endif
}
}

//////////////////////////////////////////////////////////////////////////
int	CLog::GetVerbosityLevel()
{
	if (m_pLogVerbosity)
		return (m_pLogVerbosity->GetIVal());

	return (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Checks the verbosity of the message and returns NULL if the message must NOT be
// logged, or the pointer to the part of the message that should be logged
// NOTE:
//    Normally, this is either the pText pointer itself, or the pText+1, meaning
//    the first verbosity character may be cut off)
//    This is done in order to avoid modification of const char*, which may cause GPF
//    sometimes, or kill the verbosity qualifier in the text that's gonna be passed next time.
const char* CLog::CheckAgainstVerbosity(const char * pText, bool &logtofile, bool &logtoconsole, const uint8 DefaultVerbosity )
{
	RETURN_NULL;
	// the max verbosity (most detailed level)
	const unsigned char nMaxVerbosity = 8;
	
	// the current verbosity of the log
	int nLogVerbosityConsole = m_pLogVerbosity ? m_pLogVerbosity->GetIVal() : nMaxVerbosity;
	int nLogVerbosityFile = m_pLogWriteToFileVerbosity ? m_pLogWriteToFileVerbosity->GetIVal() : nMaxVerbosity;

	logtoconsole = (nLogVerbosityConsole >= DefaultVerbosity);
	
	//to preserve logging to TTY, logWriteToFile logic has been moved to inside logStringToFile
	//int logToFileCVar = m_pLogWriteToFile ? m_pLogWriteToFile->GetIVal() : 1; 

	logtofile = (nLogVerbosityFile >= DefaultVerbosity);

	return pText;
}


//////////////////////////////////////////////////////////////////////////
void CLog::AddCallback( ILogCallback *pCallback )
{
	stl::push_back_unique(m_callbacks,pCallback);
}

//////////////////////////////////////////////////////////////////////////
void CLog::RemoveCallback( ILogCallback *pCallback )
{
	m_callbacks.remove(pCallback);
}

//////////////////////////////////////////////////////////////////////////
void CLog::Update()
{
	FUNCTION_PROFILER_FAST( m_pSystem,PROFILE_SYSTEM,g_bProfilerEnabled );

	if (CryGetCurrentThreadId() == m_nMainThreadId)
	{
		// Must be called from main thread
		while (!m_threadSafeMsgQueue.empty())
		{
			SLogMsg msg;
			if (m_threadSafeMsgQueue.try_pop(msg))
			{
				if (msg.bConsole)
					LogStringToConsole( msg.msg,msg.bAdd );
				else
					LogStringToFile( msg.msg,msg.bAdd,msg.bConsole );
			}
		}

		if (LogCVars::s_log_tick != 0)
		{
			static CTimeValue t0 = GetISystem()->GetITimer()->GetAsyncTime();
			CTimeValue t1 = GetISystem()->GetITimer()->GetAsyncTime();
			if (fabs((t1-t0).GetSeconds()) > LogCVars::s_log_tick)
			{
				t0 = t1;
				
				char sTime[128];
				time_t ltime;
				time( &ltime );
				struct tm *today = localtime( &ltime );
				strftime( sTime, sizeof(sTime)-1, "<%H:%M:%S> ", today );
				Log( "<tick> %s",sTime );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CLog::AllowDirectLoggingFromAnyThread( bool bEnable )
{
	m_bAllowDirectLoggingFromAnyThread = bEnable;
}
#undef RETURN_NULL
#undef RETURN

//#include UNIQUE_VIRTUAL_WRAPPER(ILog)
DEVIRTUALIZATION_VTABLE_FIX_IMPL(ILog);
