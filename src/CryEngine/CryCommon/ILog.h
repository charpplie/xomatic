#include DEVIRTUALIZE_HEADER_FIX(ILog.h)

#ifndef _ILOG_H_
#define _ILOG_H_

#include <platform.h>



#include "IMiniLog.h"

// Summary:
//	 Callback interface to the ILog.
struct ILogCallback
{
	virtual void OnWriteToConsole( const char *sText,bool bNewLine ) = 0;
	virtual void OnWriteToFile( const char *sText,bool bNewLine ) = 0;
};

// Summary:
//	 Interface for logging operations based on IMiniLog.
// Notes:
//	 Logging in CryEngine should be done using the following global functions:
//		CryLog (eMessage)
//		CryLogAlways (eAlways)
//		CryError (eError)
//		CryWarning (eWarning)
//		CryComment (eComment) 
//	 ILog gives you more control on logging operations.
// See also:
//	 IMiniLog, CryLog, CryLogAlways, CryError, CryWarning
UNIQUE_IFACE struct ILog: public IMiniLog
{
	virtual void Release() = 0;

	// Summary:
	//	 Sets the file used to log to disk.
	virtual bool	SetFileName(const char *command = NULL) = 0;

	// Summary:
	//	 Gets the filename used to log to disk.
	virtual const char*	GetFileName() = 0;

	//all the following functions will be removed are here just to be able to compile the project ---------------------------

	// Summary:
	//	 Logs the text both to file and console.
	virtual void	Log(const char *szCommand,...) PRINTF_PARAMS(2, 3) = 0;

	virtual void	LogWarning(const char *szCommand,...) PRINTF_PARAMS(2, 3) = 0;

	virtual void	LogError(const char *szCommand,...) PRINTF_PARAMS(2, 3) = 0;

	// Summary:
	//	 Logs the text both to the end of file and console.
	virtual void	LogPlus(const char *command,...) PRINTF_PARAMS(2, 3) = 0;	

	// Summary:
	//	 Logs to the file specified in SetFileName.
	// See also:
	//	 SetFileName
	virtual void	LogToFile(const char *command,...) PRINTF_PARAMS(2, 3) = 0;	

	//
	virtual void	LogToFilePlus(const char *command,...) PRINTF_PARAMS(2, 3) = 0;

	// Summary:
	//	 Logs to console only.
	virtual void	LogToConsole(const char *command,...) PRINTF_PARAMS(2, 3) = 0;

	//
	virtual void	LogToConsolePlus(const char *command,...) PRINTF_PARAMS(2, 3) = 0;

	//
	virtual void	UpdateLoadingScreen(const char *command,...) PRINTF_PARAMS(2, 3) = 0;	

	//
	virtual void RegisterConsoleVariables() {}

	//
	virtual void UnregisterConsoleVariables() {}

	// Notes:
	//	 Full logging (to console and file) can be enabled with verbosity 4.
	//	 In the console 'log_Verbosity 4' command can be used.
	virtual void	SetVerbosity( int verbosity ) = 0;

	virtual int		GetVerbosityLevel()=0;

	virtual void  AddCallback( ILogCallback *pCallback ) = 0;
	virtual void  RemoveCallback( ILogCallback *pCallback ) = 0;

	// Notes:
	//	 The function called every frame by system.
	virtual void Update() = 0;
	
	DEVIRTUALIZATION_VTABLE_FIX
};

#endif //_ILOG_H_



