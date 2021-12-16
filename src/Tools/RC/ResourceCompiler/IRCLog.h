////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ILog.h
//  Version:     v1.00
//  Created:     24.1.2002 by Sergiy
//  Compilers:   Visual Studio.NET
//  Description: ILog interface.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef _RESOURCE_COMPILER_IRCLOG_HDR_
#define _RESOURCE_COMPILER_IRCLOG_HDR_

#include <stdarg.h>
//#include "..\CryCommonTools\ILog.h"		// FIXME: won't compile otherwise?
#include "ILog.h"

// This interface is used to log events inside the convertors,
// including information and warning messages
struct IRCLog: public ILog
{

	// interface ILog -----------------------------------------------

	// use only these three methods (\n is auto splitting in lines)

	// 
	virtual void Log( const char* szFormat, ...)
	{
		va_list arg;
		va_start(arg, szFormat);
		LogV(eMessage, szFormat, arg);
		va_end(arg);
	}

	// 
	virtual void LogWarning( const char* szFormat, ...)
	{
		va_list arg;
		va_start(arg, szFormat);
		LogV(eWarning, szFormat, arg);
		va_end(arg);
	}

	// 
	virtual void LogError( const char* szFormat, ...)
	{
		va_list arg;
		va_start(arg, szFormat);
		LogV(eError, szFormat, arg);
		va_end(arg);
	}
	
	// --------------------------------------------------------------

	virtual void LogPlus(const char* szFormat, ...) // print message on the prev line (maybe not needed)
	{
		va_list arg;
		va_start(arg, szFormat);
		LogV(eMessage, szFormat, arg);
		va_end(arg);
	}

	//! split in lines (empty lines if given) and print with Log function
	virtual void LogV( const ELogType ineType, const char* szFormat, va_list args ) = 0;

	virtual void Release() {}

	//set the file used to log to disk
	virtual void SetRootFolder(const char* root = NULL) {}
	virtual bool SetFileName (const char *command = NULL) {return true;}
	virtual const char*	GetFileName() {return "stdout";}
	virtual void	LogToFile(const char *szCommand,...)
	{
		va_list arg;
		va_start(arg, szCommand);
		LogV (eMessage, szCommand, arg);
		va_end(arg);
	}

	virtual void	LogToFilePlus(const char *szCommand,...)
	{
		va_list arg;
		va_start(arg, szCommand);
		LogV (eMessage, szCommand, arg);
		va_end(arg);
	}

	//log to console only
	virtual void	LogToConsole(const char *szCommand,...)
	{
		va_list arg;
		va_start(arg, szCommand);
		LogV (eMessage, szCommand, arg);
		va_end(arg);
	}

	//
	virtual void	LogToConsolePlus(const char *szCommand,...) {}

	//
	virtual void	UpdateLoadingScreen(const char *szCommand,...) {}

	//
	virtual void	EnableVerbosity( bool bEnable ) {}

	//
	virtual void	SetVerbosity( int verbosity ) {}
	virtual int GetVerbosityLevel () {return 5;}
};


void SetRCLog(IRCLog* pRCLog);
void RCLog(const char* szFormat, ...);
void RCLogWarning(const char* szFormat, ...);
void RCLogError(const char* szFormat, ...);


#endif // _RESOURCE_COMPILER_IRCLOG_HDR_