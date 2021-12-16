////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   platform_implRC.h
//  Version:     v1.00
//  Created:     23/12/2009 by Sergey Sokov.
//  Compilers:   Visual Studio.NET 2003
//  Description: This file should only be included Once in DLL module.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __PLATFORM_IMPLRC_H__
#define __PLATFORM_IMPLRC_H__
#pragma once

#include <platform.h>

#if defined(_LIB)
#	error It is not allowed to have _LIB defined
#endif
	
#include <CryCommon.cpp>

#include <IRCLog.h>


struct SSystemGlobalEnvironment;
SSystemGlobalEnvironment* gEnv = 0;
IRCLog* g_pRCLog = 0;

void SetRCLog(IRCLog* pRCLog)
{
	g_pRCLog = pRCLog;
}

void RCLog(const char* szFormat, ...)
{
	va_list args;
	va_start(args, szFormat);
	if (g_pRCLog)
		g_pRCLog->LogV(IMiniLog::eMessage, szFormat, args);
	else
	{
		vprintf(szFormat, args);
		printf("\n");
	}
	va_end(args);
}

void RCLogWarning(const char* szFormat, ...)
{
	va_list args;
	va_start(args, szFormat);
	if (g_pRCLog)
		g_pRCLog->LogV(IMiniLog::eWarning, szFormat, args);
	else
	{
		vprintf(szFormat, args);
		printf("\n");
	}
	va_end(args);
}

void RCLogError(const char* szFormat, ...)
{
	va_list args;
	va_start (args, szFormat);
	if (g_pRCLog)
		g_pRCLog->LogV(IMiniLog::eError, szFormat, args);
	else
	{
		vprintf(szFormat, args);
		printf("\n");
	}
	va_end(args);
}



//////////////////////////////////////////////////////////////////////////
// Logs important data that must be printed regardless verbosity.
void PlatformLogOutput(const char *, ...) PRINTF_PARAMS(1, 2);
inline void PlatformLogOutput( const char *format,... )
{
	assert(g_pRCLog);
	if(g_pRCLog)		
	{
		va_list args;
		va_start(args,format);
		g_pRCLog->LogV( IMiniLog::eError,  format, args );
		va_end(args);
	}
}


// when using STL Port _STLP_DEBUG and _STLP_DEBUG_TERMINATE - avoid actually 
// crashing (default terminator seems to kill the thread, which isn't nice).
#ifdef _STLP_DEBUG_TERMINATE
void __stl_debug_terminate(void)
{
	assert( 0 && "STL Debug Error" );
}
#endif
#ifdef _STLP_DEBUG_MESSAGE
void __stl_debug_message(const char * format_str, ...)
{	
	va_list __args;
	va_start( __args, format_str );
	char __buffer [4096];
	vsnprintf(__buffer, sizeof(__buffer) / sizeof(char), format_str, __args);
	PlatformLogOutput(__buffer); // Log error.
	va_end(__args);
}
#endif //_STLP_DEBUG_MESSAGE

#endif
