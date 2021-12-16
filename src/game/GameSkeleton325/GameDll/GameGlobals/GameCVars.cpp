/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:	Game-wide global variables, aka CVars	

-------------------------------------------------------------------------
History:
- 15:2:2010		17:00 : Created by Christian Helmich	

*************************************************************************/

#include "StdAfx.h"
#include "GameCVars.h"

//////////////////////////////////////////////////////////////////////////
//- static member var definitions

int		SGameCVars::gs_LogCodeCoverage;

//- static member var definitions/
//////////////////////////////////////////////////////////////////////////
//- ctor & dtor

SGameCVars::SGameCVars()
{
	LOG_CODE_COVERAGE();
}

SGameCVars::~SGameCVars()
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pConsole);
	CRY_SAFE_RETURN(!gEnv->pConsole, CRY_NO_RETURN_VALUE);

	gEnv->pConsole->UnregisterVariable("gs_LogCodeCoverage", true);
}

//- ctor & dtor/
//////////////////////////////////////////////////////////////////////////
//- registration

void	SGameCVars::Register()
{
	LOG_CODE_COVERAGE();
	
	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);
	
	CRY_ASSERT(gEnv->pConsole);
	CRY_SAFE_RETURN(!gEnv->pConsole, CRY_NO_RETURN_VALUE);

	REGISTER_CVAR(gs_LogCodeCoverage, 0, VF_NULL, "logs code coverage");
}

//- registration/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////