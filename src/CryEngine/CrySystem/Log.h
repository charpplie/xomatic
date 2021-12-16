
//////////////////////////////////////////////////////////////////////
//
//	Crytek CryENGINE Source code
//	
//	File:Log.h
//
//	History:
//	-Feb 2,2001:Created by Marco Corbetta
//
//////////////////////////////////////////////////////////////////////

#ifndef LOG_H
#define LOG_H

#if _MSC_VER > 1000
# pragma once
#endif

#include <ILog.h>
#include <CryThread.h>
#include <MultiThread.h>
#include <MultiThread_Containers.h>

//////////////////////////////////////////////////////////////////////

#if defined(WIN32) || defined(WIN64) || defined(LINUX)
#define MAX_TEMP_LENGTH_SIZE	2048
#else
#define MAX_TEMP_LENGTH_SIZE	512
// reduce max warning length for consoles (e.g. to prevent overflows on PS3 where thread stack are rather small)
#endif

#define MAX_FILENAME_SIZE			256


//////////////////////////////////////////////////////////////////////
class CLog :public ILog
{
public:
	typedef std::list<ILogCallback*> Callbacks;	
	typedef CryFixedStringT<MAX_TEMP_LENGTH_SIZE> LogStringType;

	// constructor
	CLog( ISystem *pSystem );
	// destructor
	~CLog();


	// interface ILog, IMiniLog -------------------------------------------------

	virtual void Release() { delete this; };
	VIRTUAL bool SetFileName(const char *filename);		
	VIRTUAL const char*	GetFileName();
	VIRTUAL void Log(const char *command,...) PRINTF_PARAMS(2, 3);
	VIRTUAL void LogWarning(const char *command,...) PRINTF_PARAMS(2, 3);
	VIRTUAL void LogError(const char *command,...) PRINTF_PARAMS(2, 3);
	VIRTUAL void LogPlus(const char *command,...) PRINTF_PARAMS(2, 3);
	VIRTUAL void LogToFile	(const char *command,...) PRINTF_PARAMS(2, 3);
  VIRTUAL void LogToFilePlus(const char *command,...) PRINTF_PARAMS(2, 3);
	VIRTUAL void LogToConsole(const char *command,...) PRINTF_PARAMS(2, 3);
	VIRTUAL void LogToConsolePlus(const char *command,...) PRINTF_PARAMS(2, 3);
	VIRTUAL void UpdateLoadingScreen(const char *command,...) PRINTF_PARAMS(2, 3);
	VIRTUAL void SetVerbosity( int verbosity );
	VIRTUAL int	 GetVerbosityLevel();
	VIRTUAL void RegisterConsoleVariables();
	VIRTUAL void UnregisterConsoleVariables();
	VIRTUAL void AddCallback( ILogCallback *pCallback );
	VIRTUAL void RemoveCallback( ILogCallback *pCallback );
	virtual void LogV( const ELogType ineType, const char* szFormat, va_list args );
	VIRTUAL void Update();

	// Used when application crash in other thread to force logging of error.
	void AllowDirectLoggingFromAnyThread( bool bEnable );

private: // -------------------------------------------------------------------

	void LogStringToFile( const char* szString,bool bAdd,bool bError=false );
	void LogStringToConsole( const char* szString,bool bAdd=false );

	FILE* OpenLogFile( const char *filename,const char *mode );
	void CloseLogFile( bool force=false );

	// will format the message into m_szTemp
	void FormatMessage(const char *szCommand, ... ) PRINTF_PARAMS(2, 3);


	ISystem	*				m_pSystem;														//
	float						m_fLastLoadingUpdateTime;							// for non-frequent streamingEngine update
	//char						m_szTemp[MAX_TEMP_LENGTH_SIZE];				//
	char						m_szFilename[MAX_FILENAME_SIZE];			// can be with path
	FILE *					m_pLogFile;
	string					m_LogMode;														//mode m_pLogFile has been opened with
	FILE *					m_pErrFile;
	int							m_nErrCount;
		
	ICVar *					m_pLogIncludeTime;										//
	
	IConsole *			m_pConsole;														//

	CryCriticalSection m_logCriticalSection;

	struct SLogHistoryItem
	{
		char str[MAX_WARNING_LENGTH];
		const char *ptr;
		ELogType type;
		float time;
	};
	SLogHistoryItem m_history[16];
	int m_iLastHistoryItem;

public: // -------------------------------------------------------------------

	// checks the verbosity of the message and returns NULL if the message must NOT be
	// logged, or the pointer to the part of the message that should be logged
	const char* CheckAgainstVerbosity(const char * pText, bool &logtofile, bool &logtoconsole, const uint8 DefaultVerbosity=2 );

	// create backup of log file, useful behavior - only on development platform
	void CreateBackupFile() const;

	ICVar *					m_pLogVerbosity;											//
	ICVar *					m_pLogWriteToFile;										//
	ICVar *					m_pLogWriteToFileVerbosity;						//
	ICVar *					m_pLogVerbosityOverridesWriteToFile;	//
	ICVar *					m_pLogSpamDelay;                      //
	Callbacks				m_callbacks;													//

	uint32 m_nMainThreadId;

	struct SLogMsg
	{
		char msg[512];
		bool bError;
		bool bAdd;
		bool bConsole;
	};
	CryMT::queue<SLogMsg> m_threadSafeMsgQueue;

	bool m_bAllowDirectLoggingFromAnyThread;
};


#endif

