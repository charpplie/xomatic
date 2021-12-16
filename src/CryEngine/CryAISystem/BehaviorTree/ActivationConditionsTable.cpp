/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   ActivationConditionsTable.cpp
$Id$
$DateTime$
Description: Manages the LUA table that stores the state of each puppet
that is used to select the proper node to use in the Behavior Tree.
---------------------------------------------------------------------
History:
- 06:07:2007 : Created by Ricardo Pillosu
- 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/

#include "StdAfx.h"
#include "ActivationConditionsTable.h"
#include "IVisualLog.h"
#include "DebugDrawContext.h"

// Description:
//
// Arguments:
//
// Return:
//
CACTDump::CACTDump( IScriptTable* obj, const string& sTableName, const string& sMostParentName )
: m_pScriptObject( obj )
, m_sTableName( sTableName )
, m_sMostParentName( sMostParentName )
{
	gEnv->pLog->LogToConsole(
		">>>>>>>>>>>>>>>>>>>>>>> Start Dumping Script Table %s[\"%s\"]\n",
		m_sMostParentName.c_str(),
		m_sTableName.c_str() );
}

// Description:
//
// Arguments:
//
// Return:
//
CACTDump::~CACTDump()
{
	gEnv->pLog->LogToConsole(
		"<<<<<<<<<<<<<<<<<<<<<<< End Dumping Script Table %s[\"%s\"]\n",
		m_sMostParentName.c_str(),
		m_sTableName.c_str() );
}

// Description:
//
// Arguments:
//
// Return:
//
void CACTDump::OnElementFound( const char* sName, ScriptVarType type )
{
	ScriptAnyValue Value;
	string sValue;

	m_pScriptObject->GetValueAny( sName, Value );

	switch( Value.type )
	{
	case ANY_ANY:
		sValue = string( "" );
		break;

	case ANY_TBOOLEAN:
		sValue = string( Value.b ? "true" : "false" );
		break;

	case ANY_TNIL:
		sValue = string( "nil" );
		break;

	case ANY_TNUMBER:
		sValue.Format( "%f", Value.number );
		break;

	case ANY_TSTRING:
		sValue = string( Value.str );
		break;

	case ANY_TTABLE:
		sValue = string( "[Table]" );
		break;

	case ANY_TUSERDATA:
		sValue = string( "[UserData]" );
		break;

	case ANY_TVECTOR:
		sValue = string( "[Vector]" );
		break;

	default:
		sValue = string( "Unrecognised type" );
	}

	gEnv->pLog->LogToConsole( "     %s = %s\n", sName, sValue.c_str() );
}

// Description:
//
// Arguments:
//
// Return:
//
void CACTDump::OnElementFound( int nIdx, ScriptVarType type )
{
}

// ------------------------------------------------------------------------------------- //
// CACTDebugDrawDump implementation
// ------------------------------------------------------------------------------------- //
// Description:
//
// Arguments:
//
// Return:
//
CACTDebugDrawDump::CACTDebugDrawDump(IScriptTable* obj, float x, float y )
: m_fX( x )
, m_fY( y )
, m_pScriptObject( obj )
, m_pVisualLog( NULL )
, m_visLogParams( ColorF(1, 1, 1, 1), 1.2f, 1, false )
{
	m_pVisualLog = gEnv->pVisualLog;
	assert( m_pVisualLog != NULL );
}

// Description:
//
// Arguments:
//
// Return:
//
void CACTDebugDrawDump::OnElementFound( const char* sName, ScriptVarType type )
{
	ScriptAnyValue Value;
	string sValue;

	m_pScriptObject->GetValueAny( sName, Value );

	CDebugDrawContext dc;

	switch( Value.type )
	{
	case ANY_ANY:
		sValue = string( "" );
		break;

	case ANY_TBOOLEAN:
		sValue = string( Value.b ? "true" : "false" );

		if( Value.b )
		{
			dc->TextToScreenColor( (int) m_fX, (int) m_fY, 0.2f, 0.9f, 0.2f, 1.0f, sName );
			m_visLogParams.color.Set( 0.0f, 1.0f, 0.0f );
			m_pVisualLog->Log( m_visLogParams, sName );
		}
		else
		{
			dc->TextToScreenColor( (int) m_fX, (int) m_fY, 1.0f, 0.3f, 0.2f, 1.0f, sName );
			m_visLogParams.color.Set( 1.0f, 0.2f, 0.1f );
			m_pVisualLog->Log( m_visLogParams, sName );
		}

		m_fY += 2.0f;
		break;

	case ANY_TNIL:
		sValue = string( "nil" );
		break;

	case ANY_TNUMBER:
		sValue.Format( "%s = %f", sName, Value.number );
		dc->TextToScreen( m_fX, m_fY, sValue.c_str() );
		m_visLogParams.color.Set( 1.0f, 1.0f, 1.0f );
		m_pVisualLog->Log( m_visLogParams, sValue.c_str() );
		m_fY += 2.0f;
		break;

	case ANY_TSTRING:
		sValue = string( Value.str );
		dc->TextToScreen( m_fX, m_fY, "%s = %s", sName, sValue.c_str() );
		m_visLogParams.color.Set( 1.0f, 1.0f, 1.0f );
		m_pVisualLog->Log( m_visLogParams, "%s = %s", sName, sValue.c_str() );
		m_fY += 2.0f;
		break;

	case ANY_TTABLE:
		sValue = string( "[Table]" );
		break;

	case ANY_TUSERDATA:
		sValue = string( "[UserData]" );
		break;

	case ANY_TVECTOR:
		sValue = string( "[Vector]" );
		break;

	default:
		sValue = string( "Unrecognised type" );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CACTDebugDrawDump::OnElementFound( int nIdx, ScriptVarType type )
{
}

// ------------------------------------------------------------------------------------- //
// CActivationConditionTable implementation
// ------------------------------------------------------------------------------------- //
// Description:
//
// Arguments:
//
// Return:
//
CActivationConditionTable::CActivationConditionTable(const char* sName, const char* sDefaultsName, const string& sProfile )
: m_sName( sName )
, m_sProfile( sProfile )
{
	assert( sName != NULL );
	assert( sProfile.empty() == false );

	if( sDefaultsName != NULL )
	{
		m_sDefaults = sDefaultsName;
	}

	IScriptSystem *pScript = gEnv->pScriptSystem;

	// Create our own table
	m_pTable = pScript->CreateTable();
	m_pTable->AddRef();

	// Get global table, create if not exists
	if( pScript->GetGlobalValue(m_sProfile.c_str(), m_pParent) == false )
	{
		m_pParent = pScript->CreateTable();
		m_pParent->AddRef();
		pScript->SetGlobalValue( m_sProfile.c_str(), m_pParent );
	}

	// Set our table inside our parent table
	m_pParent->SetValue( m_sName, m_pTable );

	// Get defaults table
	if( sDefaultsName != NULL )
	{
		m_pParent->GetValue( m_sDefaults, m_pDefaults );
	}

	// Copy all values from default table
	Reset();
}

// Description:
//
// Arguments:
//
// Return:
//
void CActivationConditionTable::Reset()
{
	if( m_sDefaults.empty() == false )
	{
		m_pTable->Clear();
		m_pTable->Clone( m_pDefaults );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
SmartScriptTable& CActivationConditionTable::GetTable()
{
	return( m_pTable );
}

// Description:
//
// Arguments:
//
// Return:
//
const SmartScriptTable& CActivationConditionTable::GetTable() const
{
	return( m_pTable );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CActivationConditionTable::GetValue( const char* sName, ScriptAnyValue& Value ) const
{
	assert( sName != NULL );

	return( m_pTable->GetValueAny(sName, Value) );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CActivationConditionTable::GetValue( int iIndex, ScriptAnyValue& Value ) const
{
	assert( iIndex >= 0 );

	return( m_pTable->GetAtAny(iIndex, Value) );
}

// Description:
//
// Arguments:
//
// Return:
//
void CActivationConditionTable::SetValue( const char* sName, ScriptAnyValue& Value ) const
{
	assert( sName != NULL );

	m_pTable->SetValueAny( sName, Value );
}

// Description:
//
// Arguments:
//
// Return:
//
void CActivationConditionTable::SetValue( int iIndex, ScriptAnyValue& Value ) const
{
	assert( iIndex >= 0 );

	m_pTable->SetAtAny( iIndex, Value );
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
void CActivationConditionTable::CreateValue( const string& sName, const string& sValue )
{
	assert( m_sDefaults.empty() == true );

	if( sValue.compareNoCase("true") == 0 )
	{
		m_pTable->SetValueAny( sName, ScriptAnyValue(true) );
	}
	else if( sValue.compareNoCase("false") == 0 )
	{
		m_pTable->SetValueAny( sName, ScriptAnyValue(false) );
	}
	else if( atoi(sValue) != 0 || sValue.compare("0") == 0 )
	{
		m_pTable->SetValueAny( sName, ScriptAnyValue(atoi(sValue)) );
	}
	else if( atof(sValue) != 0.0f || sValue.compare("0.0") == 0 )
	{
		m_pTable->SetValueAny( sName, ScriptAnyValue(float(atof(sValue))) );
	}
	else
	{
		m_pTable->SetValueAny( sName, ScriptAnyValue(sValue) );
	}
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
void CActivationConditionTable::Dump( IScriptTableDumpSink* pSink ) const
{
	if( pSink == NULL )
	{
		CACTDump Sink( m_pTable, m_sName, GetMostParentName() );
		m_pTable->Dump( &Sink );
	}
	else
	{
		m_pTable->Dump( pSink );
	}
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
const string& CActivationConditionTable::GetName() const
{
	return( m_sName );
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
const string& CActivationConditionTable::GetParentName() const
{
	return( m_sDefaults );
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
const string& CActivationConditionTable::GetMostParentName() const
{
	return( m_sProfile );
}
