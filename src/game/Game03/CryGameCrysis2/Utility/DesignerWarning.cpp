/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
DesignerWarning.h
	- basic message box in windows to allow designers to be told something is wrong with their setup

-	[10/11/2009] : Created by James Bamford

*************************************************************************/

#include "StdAfx.h"
#include "DesignerWarning.h"
#include "GameCVars.h"

#if DESIGNER_WARNING_ENABLED

static int s_numDesignerWarningsHit=0;

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// get rid of (really) annoying MS defines
#undef min
#undef max
#endif

int DesignerWarningFunc(const char * message)
{
#if defined(WIN32) || defined(WIN64)
		if (g_pGameCVars->designer_warning_enabled)
		{
			MessageBox(GetDesktopWindow(), message, "DESIGNER WARNING", MB_OK);
		}
#endif
	CryLogAlways("---DESIGNER_WARNING: %s", message);
	CryLogAlways("----------------------------------------");
	s_numDesignerWarningsHit++;
	return 0;
}

int GetNumDesignerWarningsHit()
{
	return s_numDesignerWarningsHit;
}

#endif // DESIGNER_WARNING_ENABLED