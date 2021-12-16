////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   StatsAgent.cpp
//  Version:     v1.00
//  Created:     06/10/2009 by Steve Barnett.
//  Description: This the declaration for CStatsAgent
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ProjectDefines.h"

#if defined(ENABLE_STATS_AGENT)

#include "StatsAgent.h"

#	include <cstdlib>
#	include <cstring>

#if defined(PS3)
#	include <cell/cell_fs.h>
#else if defined(XENON)
# include <Xbdm.h>
#endif

bool CStatsAgent::s_pipeOpen = false;

#if defined(PS3)
int CStatsAgent::s_pipe = -1;
#else if defined(XENON)
char CStatsAgent::s_pCommand[128];
char CStatsAgent::s_pResponse[128];
volatile bool CStatsAgent::s_commandWaiting = false;
#endif

void CStatsAgent::CreatePipe( void )
{
#if defined(PS3)
	// Open pipe to communicate with the client
	CellFsErrno err = cellFsOpen( "/app_home/\\\\.\\pipe\\CrysisTargetComms.pipe", CELL_FS_O_RDWR, &s_pipe, NULL, 0);

	if (err == CELL_FS_SUCCEEDED)
	{
		char pMsg[] = "Connected.\n";
		uint64_t tx;
		cellFsWrite( s_pipe, pMsg, strlen( pMsg ) + 1, &tx );
		s_pipeOpen = true;
	}
	else
	{
		CryLogAlways( "Unable to connect to host." );
	}
#else if defined(XENON)
	DmRegisterCommandProcessor( "crysis_statsagent", CommandProcessor );
	s_pipeOpen = true;
	s_commandWaiting = false;
#endif
}

void CStatsAgent::ClosePipe( void )
{
	if ( s_pipeOpen = true  )
	{
#if defined(PS3)
		cellFsClose( s_pipe );
#else if defined(XENON)
	// Nothing to shutdown on the XBox
#endif
		s_pipeOpen = false;
	}
}

void CStatsAgent::Update( void )
{
	if ( s_pipeOpen )
	{
#if defined(PS3)
		// Read from the pipe
		char pFileBuf[1024];
		uint64 size;
		const CellFsErrno cfse = cellFsRead( s_pipe, pFileBuf, 1024, &size );
		if ( cfse == CELL_FS_SUCCEEDED && size > 0 )
		{
			pFileBuf[size] = '\0';
			string command( pFileBuf );
			command.TrimRight( '\n' );
	
			// Just return the dump filename
			if ( !strncmp( command.c_str(), "getdumpfilename", 15 ) )
			{
				CryReplayInfo info;
				CryGetIMemReplay()->ReplayGetInfo(info);
				const char* pFilename = info.filename;
				if ( !pFilename ) { pFilename = "<unknown>"; }
				string filename = string( "dumpfile " ) + string( pFilename );
				uint64_t tx;
				cellFsWrite( s_pipe, filename.c_str(), filename.length() + 1, &tx );
			}
			// Execute the command
			else if ( !strncmp( command.c_str(), "exec ", 5 ) )
			{
				// Execute the rest of the string
				if (gEnv && gEnv->pConsole )
				{
					const char* pCommand = command.c_str() + 5;
					gEnv->pConsole->ExecuteString( pCommand );
					char pMsg[] = "Finished.\n";
					uint64_t tx;
					cellFsWrite( s_pipe, pMsg, strlen( pMsg ) + 1, &tx );
				}
			}
		}
#else if defined(XENON)
		if ( s_commandWaiting )
		{
			CryLogAlways( "Processed: %s", s_pCommand );
			s_commandWaiting = false;
			// Just return the dump filename
			if ( !strncmp( s_pCommand, "getdumpfilename", 15 ) )
			{
				CryReplayInfo info;
				CryGetIMemReplay()->ReplayGetInfo(info);
				const char* pFilename = info.filename;
				if ( !pFilename ) { pFilename = "<unknown>"; }
				string filename = string( "crysis_statsagent!dumpfile " ) + string( pFilename );
				strncpy_s( s_pResponse, 128, filename.c_str(), filename.length() + 1 );
				CryLogAlways( "Sent notification '%s'", s_pResponse );
				DmSendNotificationString( s_pResponse );
			}
			// Execute the command
			else if ( !strncmp( s_pCommand, "exec ", 5 ) )
			{
				// Execute the rest of the string
				if (gEnv && gEnv->pConsole )
				{
					const char* pCommand = s_pCommand + 5;
					gEnv->pConsole->ExecuteString( pCommand );
					// Send response
					strncpy_s( s_pResponse, 128, "crysis_statsagent!Finished.\n", 28 );
					DmSendNotificationString( s_pResponse );
					CryLogAlways( "Sent notification '%s'", s_pResponse );
				}
			}
		}
#endif
	}
}

#if defined(XENON)
//--------------------------------------------------------------------------------------
// Temporary replacement for CRT string funcs, since
// we can't call CRT functions on the debug monitor
// thread right now.
// From MS DebugChannel sample project
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Name: dbgstrlen
// Desc: Critical section safe strlen() function
//--------------------------------------------------------------------------------------
static int dbgstrlen( const CHAR* str )
{
    const CHAR* strEnd = str;

    while( *strEnd )
        strEnd++;

    return strEnd - str;
}


//--------------------------------------------------------------------------------------
// Name: dbgtolower
// Desc: Returns lowercase of char
//--------------------------------------------------------------------------------------
inline CHAR dbgtolower( CHAR ch )
{
    if( ch >= 'A' && ch <= 'Z' )
        return ch - ( 'A' - 'a' );
    else
        return ch;
}


//--------------------------------------------------------------------------------------
// Name: dbgstrnicmp
// Desc: Critical section safe string compare.
//       Returns zero if the strings match.
//--------------------------------------------------------------------------------------
static INT dbgstrnicmp( const CHAR* str1, const CHAR* str2, int n )
{
    while( n > 0 )
    {
        if( dbgtolower( *str1 ) != dbgtolower( *str2 ) )
           return *str1 - *str2;
        --n;
        ++str1;
        ++str2;
    }

    return 0;
}


static VOID dbgstrcpy( CHAR* strDest, const CHAR* strSrc )
{
    while( ( *strDest++ = *strSrc++ ) != 0 );
}

//--------------------------------------------------------------------------------------
// End of Temporary replacement for CRT string funcs from MS DebugChannel sample
//--------------------------------------------------------------------------------------

HRESULT __stdcall CStatsAgent::CommandProcessor( LPCSTR pCommand, LPSTR pResponse, DWORD response, PDM_CMDCONT pdmcc )
{
	CryLogAlways( "Received: %s", pCommand );
	// Check for and strip off the handler name - write specifies a command
	
	if ( !dbgstrnicmp( pCommand, "crysis_statsagent!write ", 24 ) )
	{
		// Acknowledge the command
		dbgstrcpy( pResponse, "Processed." );
		// Store the command to be processed
		dbgstrcpy( s_pCommand, pCommand + 24 );
		// Trim it to a single line
		char* pCur = s_pCommand;
		while ( (*pCur++) != '\0' )
		{
			if ( *pCur == '\n' ) *pCur = '\0';
		}
		s_commandWaiting = true;
		return XBDM_NOERR;
	}
	// Check for and strip off the handler name - read specifies a request for a response
	else if ( !dbgstrnicmp( pCommand, "crysis_statsagent!read", 22 ) )
	{
		// Always respond with "Connected."
		dbgstrcpy( pResponse, "Connected." );
		return XBDM_NOERR;
	}
	else
	{
		// Acknowledge that the command failed
		dbgstrcpy( pResponse, "Failed." );
		return XBDM_UNDEFINED;
	}
}
#endif

#endif	// defined(USE_STATS_AGENT)
