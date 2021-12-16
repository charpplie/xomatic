/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   ActivationConditions.cpp
$Id$
$DateTime$
Description: Main interface for receiving signals that will affect the
state of each puppet. This state then will be read by the Behavior Tree
to select the proper tree node to run by this puppet.
---------------------------------------------------------------------
History:
- 04:07:2007 : Created by Ricardo Pillosu

*********************************************************************/

#include "StdAfx.h"
#include "ActivationConditions.h"
#include "PersonalActivationConditions.h"
#include "DefaultActivationConditions.h"
#include "SignalConditions.h"
#include "ProfileDictionary.h"
#include "GlobalActivationConditions.h"
#include "IPersonalBehaviorTree.h"

#define AC_SCRIPT_TABLE "ActCond_"
#define AC_TICK_SIGNAL	"__TICK_ONCE_PER_SEC__"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CActivationConditions::CActivationConditions( const string& sName )
: m_bInit( true )
, m_bDebug( false )
, m_bReloadOnGameModeExit( false )
, m_bRegistered( false )
, m_fUpdateOncePerSec( 1.0f )
, m_pDefaults( NULL )
, m_pSignals( NULL )
{
	assert( !sName.empty() );

	m_sName = sName;
	m_sTable.Format( "%s%s", AC_SCRIPT_TABLE, sName.c_str() );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CActivationConditions::~CActivationConditions()
{
	Shutdown();
	UnRegister();
}

// Description:
//
// Arguments:
//
// Return:
//
void CActivationConditions::Init()
{
	Register();
}

// Description:
//
// Arguments:
//
// Return:
//
void CActivationConditions::Register()
{
	if( m_bRegistered == false && !m_sConfigFile.empty() )
	{
		// Register for file change alerts
		if( gEnv->pFileChangeMonitor != NULL )
		{
			gEnv->pFileChangeMonitor->RegisterListener( this, m_sConfigFile );
		}

		m_bRegistered = true;
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CActivationConditions::UnRegister()
{
	if( m_bRegistered == true )
	{
		if( gEnv->pFileChangeMonitor != NULL )
		{
			gEnv->pFileChangeMonitor->UnregisterListener( this );
		}

		m_bRegistered = false;
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CActivationConditions::Shutdown()
{
	assert( m_bInit == true );

	for( uint32 uIndex = 0; uIndex < m_vecPersonals.size(); ++uIndex )
	{
		SAFE_DELETE( m_vecPersonals[uIndex] );
	}

	m_vecPersonals.clear();

	SAFE_DELETE( m_pDefaults );
	SAFE_DELETE( m_pSignals );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CActivationConditions::Reload( const string& sFile )
{
	assert( m_bInit == true );

	bool bRet = true;

	XmlNodeRef xmlFile = GetISystem()->LoadXmlFile( sFile );

	if( xmlFile )
	{
		// Wipe all current data
		Shutdown();
		for( int iSheet = 0; iSheet < xmlFile->getChildCount(); ++iSheet )
		{
			XmlNodeRef xmlSheet = xmlFile->getChild( iSheet );

			if( xmlSheet->isTag("Worksheet") == false )
			{
				continue;
			}

			string sSheetName = xmlSheet->getAttr( "ss:Name" );
			XmlNodeRef xmlTable = xmlSheet->findChild( "Table" );

			if( xmlTable )
			{
				if( sSheetName.compareNoCase("values") == 0 && m_pDefaults == NULL )
				{
					m_pDefaults = new CDefaultActivationConditions( xmlTable, m_sTable, eACS_Actor );
				}
				else if( sSheetName.compareNoCase("signals") == 0 && m_pSignals == NULL )
				{
					m_pSignals = new CSignalConditions( xmlTable );
				}
			}
		}
	}

	if( bRet == true )
	{
		m_sConfigFile = sFile;
	}

	if( m_vecPersonals.size() > 1 )
	{
		Reset();
	}

	// Force a reset on all personalbehaviortree classes
	GetAISystem()->GetBSSProfileManager()->ResetAllUsers();

	return( bRet );
}

// Description:
//   configuration
// Arguments:
//
// Return:
//
void CActivationConditions::Reset()
{
	assert( m_bInit == true );

	for( uint32 uIndex = 0; uIndex < m_vecPersonals.size(); ++uIndex )
	{
		m_vecPersonals[uIndex]->Reset();
	}

	m_fUpdateOncePerSec = 1.0f;
}

// Description:
//
// Arguments:
//
// Return:
//
void CActivationConditions::Update( float fDeltaTime )
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	m_fUpdateOncePerSec -= fDeltaTime;

	if( m_fUpdateOncePerSec <= 0.0f )
	{
		ReceiveAnonymousSignal( AC_TICK_SIGNAL );
		m_fUpdateOncePerSec = 1.0f;
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CActivationConditions::OnFileChange( const char* sFilename )
{
	if( GetAISystem()->GetBTProfileDictionary()->IsEditing() == true )
	{
		// Not in game mode, so reload now
		bool bOk = Reload( m_sConfigFile );
		if( bOk )
		{
			gEnv->pLog->Log( "Global Activation Conditions updated successfully: %s", sFilename );
		}
		else
		{
			AIError( "Global Activation Conditions update failed: %s", sFilename );
		}
	}
	else
	{
		// In game mode, so reload when we leave it
		m_bReloadOnGameModeExit = true;
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CActivationConditions::OnEditorSetGameMode( bool bGameMode )
{
	if( bGameMode == false )
	{
		if( m_bReloadOnGameModeExit == true )
		{
			m_bReloadOnGameModeExit = false;
			Reload( m_sConfigFile );
		}
	}
}

// Description:
//
// Arguments:
//
// Return:
//
CPersonalActivationConditions* CActivationConditions::CreatePersonalActivationConditions( TBSSProfileUserId userId )
{
	assert( m_bInit == true );
	assert( userId != g_uBTUserId_Invalid );

	CPersonalActivationConditions *pPersonal = GetPersonalActivationConditions( userId );

	if( pPersonal == NULL && m_pDefaults != NULL )
	{
		pPersonal = new CPersonalActivationConditions( m_sName, this );

		bool bRet = pPersonal->Init( userId, m_pDefaults->GetValuesFromScope(eACS_Actor), m_sTable, m_sName );

		if( bRet == true )
		{
			m_vecPersonals.push_back( pPersonal );
		}
		else
		{
			SAFE_DELETE( pPersonal );
		}
	}

	return( pPersonal );
}

// Description:
//
// Arguments:
//
// Return:
//
CPersonalActivationConditions* CActivationConditions::GetPersonalActivationConditions( TBSSProfileUserId userId )
{
	CPersonalActivationConditions*	pPersonal = NULL;

	for( uint32 uIndex = 0; uIndex < m_vecPersonals.size(); ++uIndex )
	{
		if( m_vecPersonals[uIndex]->GetUserId() == userId )
		{
			pPersonal = m_vecPersonals[uIndex];
			break;
		}
	}

	return( pPersonal );
}

// Description:
//
// Arguments:
//
// Return:
//
CPersonalActivationConditions const* CActivationConditions::GetPersonalActivationConditions( TBSSProfileUserId userId ) const
{
	assert( m_bInit == true );

	CPersonalActivationConditions const*	pPersonal = NULL;

	for( uint32 uIndex = 0; uIndex < m_vecPersonals.size(); ++uIndex )
	{
		if( m_vecPersonals[uIndex]->GetUserId() == userId )
		{
			pPersonal = m_vecPersonals[uIndex];
			break;
		}
	}

	return( pPersonal );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CActivationConditions::DeletePersonalActivationConditions( TBSSProfileUserId userId )
{
	assert( m_bInit == true );

	bool bRet = false;

	std::vector<CPersonalActivationConditions *>::iterator	iter;
	for( iter = m_vecPersonals.begin(); iter != m_vecPersonals.end(); ++iter )
	{
		CPersonalActivationConditions*	pPersonal = *iter;
		if( pPersonal->GetUserId() == userId )
		{
			SAFE_DELETE( pPersonal );
			m_vecPersonals.erase( iter );
			bRet = true;
			break;
		}
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
SSignalReaction const* CActivationConditions::GetReaction( const char* sSignal ) const
{
	SSignalReaction const* pResult = NULL;

	if (m_pSignals)
	{
		pResult = m_pSignals->GetSignalReaction(sSignal);
	}

	return pResult;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CActivationConditions::ReceiveAnonymousSignal( const char* sSignal, const ScriptAnyValue &argValue )
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	assert( sSignal != NULL );

	bool bRet = false;

	for( uint32 uIndex = 0; uIndex < m_vecPersonals.size(); ++uIndex )
	{
		CPersonalActivationConditions*	pPersonal = m_vecPersonals[uIndex];

		if( pPersonal->GetUserId() != g_uBTUserId_Invalid )
		{
			SmartScriptTable pConditionsTable;
			if (pPersonal->GetConditionsTable(pConditionsTable))
			{
				bRet |= pPersonal->ReceiveSignal( pConditionsTable, sSignal, argValue );
			}
		}
	}

	return bRet;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CActivationConditions::CheckCondition( int iPreCompiledCode, TBSSProfileUserId userId ) const
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	assert( iPreCompiledCode >= 0 );

	bool bRet = false;

	if( userId != 0 )
	{
		CPersonalActivationConditions const* pPersonal = GetPersonalActivationConditions( userId );
		assert( pPersonal != NULL );

		if( pPersonal != NULL )
		{
			bRet = pPersonal->CheckCondition( iPreCompiledCode, m_bDebug );
		}
	}
	else
	{
		bRet = GetAISystem()->GetBTProfileDictionary()->GetGlobalActivationConditions()->CheckCondition( iPreCompiledCode );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CActivationConditions::IsDebug() const
{
	return( m_bDebug );
}

// Description:
//
// Arguments:
//
// Return:
//
void CActivationConditions::SetDebug( bool bDebug )
{
	m_bDebug = bDebug;
}

// Description:
//
// Arguments:
//
// Return:
//
const char* CActivationConditions::GetName() const
{
	return m_sName.c_str();
}
