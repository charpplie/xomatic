/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   GlobalActivationConditions.cpp
$Id$
$DateTime$
Description: Handles the global list of activation conditions which
are shared by all trees
---------------------------------------------------------------------
History:
- 08:02:2009 : Created by Ricardo Pillosu

*********************************************************************/

#include "StdAfx.h"
#include "GlobalActivationConditions.h"
#include "PersonalActivationConditions.h"
#include "DefaultActivationConditions.h"
#include "SignalConditions.h"
#include "ProfileDictionary.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CGlobalActivationConditions::CGlobalActivationConditions( const char* sName )
: CPersonalActivationConditions( sName, NULL )
, m_bReloadOnGameModeExit( false )
, m_bRegistered( false )
, m_pGlobalDefaults( NULL )
, m_pSignals( NULL )
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CGlobalActivationConditions::~CGlobalActivationConditions()
{
	SAFE_DELETE( m_pGlobalDefaults );
	SAFE_DELETE( m_pSignals );

	UnRegister();
}

// Description:
//
// Arguments:
//
// Return:
//
void CGlobalActivationConditions::InitGlobals()
{
	Register();
}

// Description:
//
// Arguments:
//
// Return:
//
void CGlobalActivationConditions::Register()
{
	if( m_bRegistered == false && !m_sFile.empty() )
	{
		// Register for file change alerts
		if( gEnv->pFileChangeMonitor != NULL )
		{
			gEnv->pFileChangeMonitor->RegisterListener( this, m_sFile );
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
void CGlobalActivationConditions::UnRegister()
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
//   Destructor
// Arguments:
//
// Return:
//
bool CGlobalActivationConditions::ReLoad( const string& sFile )
{
	bool bRet = false;

	SAFE_DELETE( m_pGlobalDefaults );

	XmlNodeRef	xmlFile = GetISystem()->LoadXmlFile( sFile );
	if( xmlFile )
	{
		// Wipe all current data
		for( int iSheet = 0; iSheet < xmlFile->getChildCount(); ++iSheet )
		{
			XmlNodeRef	xmlSheet = xmlFile->getChild( iSheet );

			if( xmlSheet->isTag("Worksheet") == false )
			{
				continue;
			}

			string sSheetName = xmlSheet->getAttr( "ss:Name" );
			XmlNodeRef xmlTable = xmlSheet->findChild( "Table" );

			if( xmlTable )
			{
				if( sSheetName.compareNoCase("values") == 0 && m_pGlobalDefaults == NULL )
				{
					m_pGlobalDefaults = new CDefaultActivationConditions( xmlTable, "Global", eACS_Global );
					bRet = ( m_pGlobalDefaults && m_pGlobalDefaults->IsInit() );
					if( !bRet )
					{
						break;
					}
				}
				else if( sSheetName.compareNoCase("signals") == 0 && m_pSignals == NULL )
				{
					m_pSignals = new CSignalConditions( xmlTable );
					bRet = ( m_pSignals && m_pSignals->IsInit() );
					if( !bRet )
					{
						break;
					}
				}
			}
		}
	}

	if( bRet )
	{
		// Create the conditions
		bRet = Init( 0, m_pGlobalDefaults->GetValuesFromScope(eACS_Global), "Global", "Global" );
	}

	if( bRet )
	{
		m_sFile = sFile;
	}

	return( bRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void CGlobalActivationConditions::OnFileChange( const char* sFilename )
{
	if( GetAISystem()->GetBTProfileDictionary()->IsEditing() )
	{
		// Not in game mode, so reload now
		bool bOk = ReLoad( m_sFile );
		if( bOk )
		{
			gEnv->pLog->Log( "Activation Conditions updated successfully: %s", sFilename );
		}
		else
		{
			AIError( "Activation Conditions update failed: %s", sFilename );
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
void CGlobalActivationConditions::OnEditorSetGameMode( bool bMode )
{
	if( bMode == false )
	{
		if( m_bReloadOnGameModeExit == true )
		{
			m_bReloadOnGameModeExit = false;
			ReLoad( m_sFile );
		}
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CGlobalActivationConditions::AssembleFinalCode( string const& sCode, string& sFinalCode ) const
{
	assert( sCode.empty() == false );
	sFinalCode.Format( "return %s;", sCode.c_str() );
	return( true );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CGlobalActivationConditions::SetGlobalSignal( const char* sSignal, const ScriptAnyValue &argValue )
{
	CRY_ASSERT( sSignal != NULL );
	CRY_ASSERT( m_pSignals != NULL );

	bool bRet = false;

	SSignalReaction const* pReaction = m_pSignals->GetSignalReaction( sSignal );
	if (pReaction)
	{
		bRet = ReceiveGlobalSignal( pReaction, sSignal, argValue );
	}

	return( bRet );
}
