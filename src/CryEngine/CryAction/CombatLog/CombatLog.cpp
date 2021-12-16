/********************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006-2007.
---------------------------------------------------------------------
File name:   CombatLog.h
$Id$
$DateTime$
Description: Logs Combat numbers to help designer balancing the game
---------------------------------------------------------------------
History:
- 12:04:2007 : Created by Ricardo Pillosu

*********************************************************************/
#include "StdAfx.h"
#include "CombatLog.h"
#include "CryActionCVars.h"

CCombatLog* CCombatLog::  m_pInstance = NULL;

CCombatLog::CCombatLog()
{
  m_bInit = false;
}

CCombatLog::~CCombatLog()
{
}

CCombatLog& CCombatLog::ref()
{
  CRY_ASSERT( NULL != m_pInstance );
  return( *m_pInstance );
}

bool CCombatLog::Create()
{
  if( NULL == m_pInstance )
  {
    m_pInstance = new CCombatLog();
  }
  else
  {
    CRY_ASSERT( "Trying to Create() the singleton more than once" );
  }

  return( m_pInstance != NULL );
}

void CCombatLog::Init()
{
  if( m_bInit == false )
  {
    m_bInit = true;
  }
}

void CCombatLog::Shutdown()
{
  if( m_bInit == true )
  {
    SAFE_DELETE( m_pInstance );
  }
}

void CCombatLog::Log( EntityId IdShooter, EntityId IdVictim, const char* format, ... )
{
	if (CCryActionCVars::Get().cl_DisableCombatLog == 0)
	{
		string  sShooter;
		string  sVictim;
		va_list ArgList;
		char    szBuffer[MAX_WARNING_LENGTH];

		va_start( ArgList, format );
		vsprintf_s( szBuffer, format, ArgList );
		va_end( ArgList );

		IActor*   pShooter = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor( IdShooter );
		IActor*   pVictim = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor( IdVictim );

		if( pShooter != NULL )
		{
			sShooter = pShooter->GetEntity()->GetName();
		}
		else
		{
			sShooter = "Unknown";
		}

		if( pVictim != NULL )
		{
			sVictim = pVictim->GetEntity()->GetName();
		}
		else
		{
			sVictim = "Unknown";
		}

		gEnv->pLog->Log( "<CL>[%s]->[%s] %s", sShooter.c_str(), sVictim.c_str(), szBuffer );
	}
}
