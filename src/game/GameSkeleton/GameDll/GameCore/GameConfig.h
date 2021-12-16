/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: game specific #defines go here
  
 -------------------------------------------------------------------------
  History:
  - 11:2:2010	14:00 : Created by Christian Helmich

*************************************************************************/

#ifndef __GAME_CONFIG_H__
#define	__GAME_CONFIG_H__
#pragma once

//////////////////////////////////////////////////////////////////////////

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// get rid of (really) annoying MS defines
#undef min
#undef max
#endif //WIN32

#include <CryLibrary.h>

//////////////////////////////////////////////////////////////////////////

//#define	USE_EXTERNAL_FRAMEWORK_DLL	1

#define GAME_NAME			"CryENGINE3"
#define GAME_LONGNAME		"CryENGINE3_Sample"
#define GAME_SAVE_EXT		".ce3sav"

#define LEVEL_NAME_LENGTH	512
#define FOLDER_NAME_LENGTH	512
#define GAMEGUID_LENGTH		128

#define GAME_GUID					"{CDC82B4A-7540-45A5-B92E-9A7C7033DBF2}"
#define GAME_WINDOW_CLASSNAME		"CryENGINE"

#define DEFAULT_CURSOR_RESOURCE_ID	105

static const int GLOBAL_SERVER_IP_KEY			= 1000;
static const int GLOBAL_SERVER_PUBLIC_PORT_KEY	= 1001;
static const int GLOBAL_SERVER_NAME_KEY			= 1002;


#ifdef _DEBUG
#define GAME_DEBUG_MEM  // debug memory usage
#else
#undef  GAME_DEBUG_MEM
#endif



#ifdef USE_EXTERNAL_FRAMEWORK_DLL
#define GAME_FRAMEWORK_DLL	"CryAction.dll"
#endif //USE_EXTERNAL_FRAMEWORK_DLL


#define SAFE_HARDWARE_MOUSE_FUNC(func)\
	if(gEnv->pHardwareMouse)\
	gEnv->pHardwareMouse->func

#define GAME_LOG_CODE_COVERAGE	1

#ifdef GAME_LOG_CODE_COVERAGE
#define	LOG_CODE_COVERAGE()	if(SGameCVars::gs_LogCodeCoverage) CryLogAlways("[code %i] %s %s@%i" , __COUNTER__, __FUNCTION__, __FILE__, __LINE__);
#else
#define	LOG_CODE_COVERAGE()	
#endif //GAME_LOG_CODE_COVERAGE


//functionality that should be moved somewhere into system
#define CHILD_MEM_STATISTICS(child, sizer)	{	if(child)	child->GetMemoryStatistics(sizer);	}

#define CREATE_ITER_VARS(iterT, iterN, listT)	\
	iterT	iterN##Start	= listT.begin();	\
	iterT	iterN##End		= listT.end();		\
	iterT	iterN;								\

#define FOR_EACH_LOOP(iterT, iterN, listT)	\
	CREATE_ITER_VARS(iterT, iterN, listT);	\
	for(iterN = iterN##Start; iterN != iterN##End; ++iterN)	\


#define CRY_SAFE_RETURN(cond, rv)			{if(cond) {CryLogAlways("safely returned from %s in %s @%i",  __FUNCTION__, __FILE__, __LINE__); return rv;}}
#define	CRY_SAFE_RETURN_(cond, rv)			{if(cond) return rv;}
#define	CRY_NO_RETURN_VALUE

#define CRY_SAFE_CREATE(ptr, ctor)			{if(!ptr) ptr = ctor;}

#define CRY_ASSERT_FUNCTION_CALLED(times)	{	\
	static int callCounter = 0;					\
	++callCounter;								\
	CRY_ASSERT(callCounter <= times);			\
	CryLogAlways("%s called %i times", __FUNCTION__, callCounter);	}\


//////////////////////////////////////////////////////////////////////////

#endif	//__GAME_CONFIG_H__