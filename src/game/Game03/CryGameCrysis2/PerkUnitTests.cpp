/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description: Perk unit tests
*************************************************************************/

#include "StdAfx.h"
#include "Perk.h"
#include "GameCVars.h"
#include "Player.h"
#include "Game.h"

#ifndef CRY_UNIT_NO_TESTING
static string s_errorsReported = "";

static void ResetErrorsReported()
{
	s_errorsReported = "";
}

static void ReportPerkError_UnitTests(const char * message, const char * func, int lineNumber)
{
	if (strstr (message, "Didn't find value for") == NULL)
	{
		if (s_errorsReported.empty())
		{
			s_errorsReported = message;
		}
		else
		{
			s_errorsReported = string (s_errorsReported + "\n" + message);
		}
	}
}

#endif
