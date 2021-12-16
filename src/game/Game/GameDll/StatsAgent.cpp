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




# include <Xbdm.h>


bool CStatsAgent::s_pipeOpen = false;




char CStatsAgent::s_pCommand[128];
char CStatsAgent::s_pResponse[128];
volatile bool CStatsAgent::s_commandWaiting = false;


void CStatsAgent::CreatePipe( void )
{
















	DmRegisterCommandProcessor( "crysis_statsagent", CommandProcessor );
	s_pipeOpen = true;
	s_commandWaiting = false;

}

void CStatsAgent::ClosePipe( void )
{
	if ( s_pipeOpen = true  )
	{



	// Nothing to shutdown on the XBox

		s_pipeOpen = false;
	}
}

void CStatsAgent::Update( void )
{
	if ( s_pipeOpen )
	{





































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

	}
}







































































































#endif	// defined(USE_STATS_AGENT)
