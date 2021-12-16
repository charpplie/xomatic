/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
Description: 
A multiplatform base class for handling errors and collecting call stacks

-------------------------------------------------------------------------
History:
- 12:10:2009	: Created by Alex McCarthy
*************************************************************************/

#include "StdAfx.h"
#include "IDebugCallStack.h"

#if !defined(PS3) && !defined(LINUX)

#include <ISystem.h>

//////////////////////////////////////////////////////////////////////////
void IDebugCallStack::LogCallstack()
{
	CollectCurrentCallStack();		// is updating m_functions

	CryLogAlways( "=============================================================================" );
	int len = (int)m_functions.size();
	for (int i = 0; i < len; i++) {
		const char* str = m_functions[i].c_str();
		CryLogAlways( "%2d) %s",len-i,str );
	}
	CryLogAlways( "=============================================================================" );
}

const char* IDebugCallStack::TranslateExceptionCode( DWORD dwExcept )
{
	switch (dwExcept)
	{
	case EXCEPTION_ACCESS_VIOLATION :	return "EXCEPTION_ACCESS_VIOLATION"; break ;
	case EXCEPTION_DATATYPE_MISALIGNMENT : return "EXCEPTION_DATATYPE_MISALIGNMENT"; break ;
	case EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";	break ;
	case EXCEPTION_SINGLE_STEP:	return "EXCEPTION_SINGLE_STEP";	break ;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED"; break ;
	case EXCEPTION_FLT_DENORMAL_OPERAND :	return "EXCEPTION_FLT_DENORMAL_OPERAND"; break ;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "EXCEPTION_FLT_DIVIDE_BY_ZERO"; break ;
	case EXCEPTION_FLT_INEXACT_RESULT: return "EXCEPTION_FLT_INEXACT_RESULT";	break ;
	case EXCEPTION_FLT_INVALID_OPERATION: return "EXCEPTION_FLT_INVALID_OPERATION"; break ;
	case EXCEPTION_FLT_OVERFLOW: return "EXCEPTION_FLT_OVERFLOW"; break ;
	case EXCEPTION_FLT_STACK_CHECK: 	return "EXCEPTION_FLT_STACK_CHECK";	break ;
	case EXCEPTION_FLT_UNDERFLOW:	return "EXCEPTION_FLT_UNDERFLOW";	break ;
	//case EXCEPTION_INT_DIVIDE_BY_ZERO: return "EXCEPTION_INT_DIVIDE_BY_ZERO";break ; //Diesel cut
	case EXCEPTION_INT_OVERFLOW:return "EXCEPTION_INT_OVERFLOW";break ;
	case EXCEPTION_PRIV_INSTRUCTION:	return "EXCEPTION_PRIV_INSTRUCTION";	break ;
	case EXCEPTION_IN_PAGE_ERROR:	return "EXCEPTION_IN_PAGE_ERROR";	break ;
	case EXCEPTION_ILLEGAL_INSTRUCTION:	return "EXCEPTION_ILLEGAL_INSTRUCTION";	break ;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:	return "EXCEPTION_NONCONTINUABLE_EXCEPTION";	break ;
	case EXCEPTION_STACK_OVERFLOW:	return "EXCEPTION_STACK_OVERFLOW";	break ;
	case EXCEPTION_INVALID_DISPOSITION:	return "EXCEPTION_INVALID_DISPOSITION";	break ;
	case EXCEPTION_GUARD_PAGE:	return "EXCEPTION_GUARD_PAGE";	break ;
	case EXCEPTION_INVALID_HANDLE:	return "EXCEPTION_INVALID_HANDLE";	break ;
	//case EXCEPTION_POSSIBLE_DEADLOCK:	return "EXCEPTION_POSSIBLE_DEADLOCK";	break ;

	case STATUS_FLOAT_MULTIPLE_FAULTS:	return "STATUS_FLOAT_MULTIPLE_FAULTS";	break ;
	case STATUS_FLOAT_MULTIPLE_TRAPS:	return "STATUS_FLOAT_MULTIPLE_TRAPS";	break ;

		
		
	default:
		return "Unknown";
		break;
	}
}

void IDebugCallStack::PutVersion( char *str )
{
	if(!gEnv || !gEnv->pSystem)
		return;

	char sFileVersion[128];
	gEnv->pSystem->GetFileVersion().ToString(sFileVersion);

	char sProductVersion[128];
	gEnv->pSystem->GetProductVersion().ToString(sProductVersion);


	//! Get time.
	time_t ltime;
	time( &ltime );
	tm *today = localtime( &ltime );

	char s[1024];
	//! Use strftime to build a customized time string.
	strftime( s,128,"Logged at %#c\n", today );
	strcat( str,s );
	sprintf( s,"FileVersion: %s\n",sFileVersion );
	strcat( str,s );
	sprintf( s,"ProductVersion: %s\n",sProductVersion );
	strcat( str,s );
}

void IDebugCallStack::WriteLineToLog( const char* format,... )
{
	if (gEnv && gEnv->pLog)
	{
		const char *logFilename = gEnv->pLog->GetFileName();

		va_list	ArgList;
		char		szBuffer[MAX_WARNING_LENGTH];
		va_start(ArgList, format);
		int count = vsnprintf_s(szBuffer,sizeof(szBuffer),sizeof(szBuffer)-1,format, ArgList);
		strcat_s(szBuffer,"\n");
		szBuffer[sizeof(szBuffer)-1] = '\0';
		va_end(ArgList);

		FILE *f = fxopen( logFilename, "a+t" );
		if (f)
		{
			fputs( szBuffer,f );
			fflush(f);
			fclose(f);
		}
	}
}

#endif //PS3
