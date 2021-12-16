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

#ifndef __ITELEMETRYCOLLECTOR_H__
#define __ITELEMETRYCOLLECTOR_H__

struct ITelemetryCollector
{
	virtual bool				SubmitFile(
									const char		*inLocalFileName)=0;
	virtual bool				AppendToFile(
									const char		*inLocalFilePath,
									const char		*inDataToAppend)=0;

	virtual void				SetNewSessionId()=0;
	virtual string				GetSessionId()=0;
	virtual void				SetSessionId(
									string			inNewSessionId)=0;

	virtual bool				ShouldSubmitTelemetry()=0;
};

#endif // __ITELEMETRYCOLLECTOR_H__
