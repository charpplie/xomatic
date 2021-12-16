/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   PersonalActivationConditions.cpp
$Id$
$DateTime$
Description: This class manages the LUA table that stores the current
state of one puppet, so we will create one instance per puppet.
Everytime this table changes due to signals received, the Behavior
Tree will be re-evaluated.
---------------------------------------------------------------------
History:
- 04:07:2007 : Created by Ricardo Pillosu
- 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/

#include "StdAfx.h"
#include "PersonalActivationConditions.h"
#include "SignalConditions.h"
#include "DefaultActivationConditions.h"
#include "ActivationConditionsTable.h"
#include "ActivationConditions.h"
#include "GlobalActivationConditions.h"
#include "ProfileDictionary.h"
#include "IVisualLog.h"
#include "DebugDrawContext.h"

/*$1-  -----------------------------------------------------------------------*/
static const char*	ACTOR_TABLE_ALIAS = "a";
static const char*	GLOBAL_TABLE_ALIAS = "g";
static const char*	USER_TABLE_ALIAS = "userTbl";
static const char*	GLOBAL_TABLE_NAME = "_global_";
static const char*	GLOBAL_TABLE_WITH_SIGNAL_DATA = "data";

/*$1- Debug info -------------------------------------------------------------*/
static uint32 RECENT_SIGNALS_COUNT = 8;
TBSSProfileUserId CPersonalActivationConditions::sm_userCurrentlyDebugged = g_uBTUserId_Invalid;

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CPersonalActivationConditions::CPersonalActivationConditions( const string& sName, CActivationConditions *pActivationConditions )
: m_bInit( false )
, m_userId( g_uBTUserId_Invalid )
, m_pActivationConditions( pActivationConditions )
, m_pValues( NULL )
, m_pDefaults( NULL )
{
	assert( !sName.empty() );

	m_sName = sName;
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CPersonalActivationConditions::~CPersonalActivationConditions()
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
SBTProfileUser* CPersonalActivationConditions::GetUser() const
{
	SBTProfileUser *pUser = GetAISystem()->GetBTProfileDictionary()->GetProfileUser(m_userId);
	return pUser;
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CPersonalActivationConditions::GetUserName(string& sUserName) const
{
	SBTProfileUser *pUser = GetUser();
	if ( pUser && pUser->m_pUser )
	{
		return pUser->m_pUser->GetBTUserName(sUserName);
	}

	return false;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalActivationConditions::GetConditionsTable( SmartScriptTable &pConditionsTable ) const
{
	bool bResult = false;

	CProfileDictionary *pDictionary = GetAISystem()->GetBTProfileDictionary();
	CRY_ASSERT(pDictionary);

	SBTProfileUser *pProfileUser = pDictionary->GetProfileUser(m_userId);
	if (pProfileUser)
	{
		bResult = pProfileUser->CreateConditionsTable(pConditionsTable);
	}

	return bResult;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalActivationConditions::Init( TBSSProfileUserId userId, CActivationConditionTable const* pDefaults, const string& sProfile, const string& sTable )
{
	m_bInit = true;
	m_sProfile = sProfile;

	SetProfileUserId(userId);
	SetTableName(sTable);

	if( pDefaults != NULL )
	{
		string sUserName;
		if( GetUserName(sUserName) )
		{
			m_pValues = new CActivationConditionTable( sUserName.c_str(), pDefaults->GetName(), sProfile );
		}
		else
		{
			m_pValues = new CActivationConditionTable( GLOBAL_TABLE_NAME, pDefaults->GetName(), sProfile );
		}

		SmartScriptTable pConditionsTable;
		if (GetConditionsTable(pConditionsTable))
		{
			assert( m_pValues != NULL );
			TransformStartingValues(pConditionsTable);
		}
	}

	return( m_bInit );
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalActivationConditions::SetProfileUserId( TBSSProfileUserId userId )
{
	m_userId = userId;
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalActivationConditions::SetTableName( const string& sTable )
{
	CRY_ASSERT(!sTable.empty());
	m_sTable = sTable;
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalActivationConditions::Reset()
{
	if( m_pValues != NULL )
	{
		m_pValues->Reset();

		SmartScriptTable pConditionsTable;
		if (GetConditionsTable(pConditionsTable))
		{
			TransformStartingValues(pConditionsTable);
		}
	}

	m_lstRecentSignals.clear();
}

// Description:
//
// Arguments:
//
// Return:
//
CActivationConditions* CPersonalActivationConditions::GetActivationConditions()
{
	return m_pActivationConditions;
}

// Description:
//
// Arguments:
//
// Return:
//
CActivationConditions const* CPersonalActivationConditions::GetActivationConditions() const
{
	return m_pActivationConditions;
}

// Description:
//
// Arguments:
//
// Return:
//
const char* CPersonalActivationConditions::GetName() const
{
	return m_sName.c_str();
}

// Description:
//
// Arguments:
//
// Return:
//
TBSSProfileUserId CPersonalActivationConditions::GetUserId() const
{
	assert( m_bInit == true );

	return( m_userId );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalActivationConditions::ReceiveSignal( SmartScriptTable &pConditionsTable, const char* sSignal, const ScriptAnyValue &argValue ) const
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	bool bResult = false;

	SSignalReaction const* pReaction = (m_pActivationConditions ? m_pActivationConditions->GetReaction(sSignal) : NULL);
	if (pReaction)
	{
		bResult = ReceiveSignal(pReaction, pConditionsTable, sSignal, argValue);
	}

	return bResult;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalActivationConditions::ReceiveGlobalSignal( SSignalReaction const* pReaction, const char* sSignal, const ScriptAnyValue &argValue ) const
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	bool bResult = false;

	if (pReaction)
	{
		SmartScriptTable emptyTable;
		bResult = ReceiveSignal(pReaction, emptyTable, sSignal, argValue);
	}

	return bResult;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalActivationConditions::ReceiveSignal( SSignalReaction const* pReaction, SmartScriptTable &pConditionsTable, const char* sSignal, const ScriptAnyValue &argValue ) const
{
	CRY_ASSERT(pReaction);

	bool bResult = false;

	if (pReaction)
	{
		const bool bDebug = (m_pActivationConditions ? m_pActivationConditions->IsDebug() : false);

		if( bDebug == true )
		{
			Dump();

			string sUserName;
			if( GetUserName(sUserName) )
			{
				gEnv->pLog->Log(
					"======================= Activation Conditions for actor \"%s\": Receiving signal \"%s\" ===========",
					sUserName.c_str(),
					pReaction->sSignal.c_str() );
			}
			else
			{
				gEnv->pLog->Log(
					"======================= Global Activation Conditions : Receiving signal \"%s\" ==================",
					pReaction->sSignal.c_str() );
			}
		}

		bResult = ApplyAllReactions( pReaction, pConditionsTable, argValue, bDebug );

		if( bDebug == true )
		{
			Dump();
		}

		if( sm_userCurrentlyDebugged == m_userId )
		{
			m_lstRecentSignals.push_front( pReaction->sSignal );

			if( m_lstRecentSignals.size() > RECENT_SIGNALS_COUNT )
			{
				m_lstRecentSignals.pop_back();
			}
		}
	}

	return bResult;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalActivationConditions::CheckCondition( int iPreCompiledCode, bool bDebug ) const
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	assert( iPreCompiledCode >= 0 );

	ScriptAnyValue anyResult;
	bool bRet = false;

	SmartScriptTable pConditionsTable;
	if( GetConditionsTable(pConditionsTable) && ExecutePreCompiledCode(iPreCompiledCode, pConditionsTable, &anyResult) == true )
	{
		bRet = anyResult.b;
	}
	else
	{
		AIWarning( "Could not execute PreCompiled code %d", iPreCompiledCode );
	}

	/*
	if( bDebug == true )
	{
	if( m_userId == 0 )
	{
	gEnv->pLog->Log(
	"======================= Global Activation Conditions : Checking condition \"%s\" ==================",
	sScriptCode );
	}
	else
	{
	gEnv->pLog->Log(
	"======================= Activation Conditions for actor \"%s\": Checking condition \"%s\" ===========",
	GetActor()->GetName(),
	sScriptCode );
	}

	Dump();
	}
	*/
	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalActivationConditions::Dump() const
{
	assert( m_pValues != NULL );

	if( m_pValues != NULL )
	{
		m_pValues->Dump();
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalActivationConditions::DebugDraw(CDebugDrawContext &dc, float fX, float fY) const
{
	IVisualLog *pVLog = gEnv->pVisualLog;
	assert( pVLog != NULL );

	SVisualLogParams visLogParams( ColorF(1, 1, 1, 1), 1.2f, 1, true );

	if( sm_userCurrentlyDebugged != m_userId )
	{
		m_lstRecentSignals.clear();
		sm_userCurrentlyDebugged = m_userId;
	}
	
	dc->TextToScreen( fX, fY+0.0f, "----------------------------" );
	dc->TextToScreen( fX, fY+1.0f, "Recent signals of %s", m_sName.c_str() );
	dc->TextToScreen( fX, fY+2.0f, "----------------------------" );
	visLogParams.column = 2;
	pVLog->Log( visLogParams, "------ Recent signals of %s -----", m_sName.c_str() );
	fY += 3.0f;

	if( m_lstRecentSignals.size() > 0 )
	{
		std::list<string>::iterator itEnd = m_lstRecentSignals.end();
		const int iX = (int)fX;
		float fColor = 1.0f;
		for( std::list<string>::iterator itVal = m_lstRecentSignals.begin(); itVal != itEnd; ++itVal )
		{
			dc->TextToScreenColor( iX, (int) fY, fColor, fColor, fColor, 1.0f, "%s", (*itVal).c_str() );
			pVLog->Log( visLogParams, "%s", (*itVal).c_str() );
			fY += 2.0f;
			fColor -= 1.0f / RECENT_SIGNALS_COUNT;
		}
	}
	fY += 2.0f;

	dc->TextToScreen( fX, fY+0.0f, "----------------------------" );
	dc->TextToScreen( fX, fY+1.0f, "Activation conditions of %s", m_sName.c_str() );
	dc->TextToScreen( fX, fY+2.0f, "----------------------------" );
	pVLog->Log( visLogParams, "----------- Activation conditions of %s -----------", m_sName.c_str() );
	visLogParams.alignColumnsToThis = false;
	fY += 3.0f;

	CACTDebugDrawDump sink( *(m_pValues->GetTable()), fX, fY );
	m_pValues->Dump( &sink );

	/*sink.m_fY = fY;
	sink.m_fX = 25.0f;
	dc->TextToScreen( 25, sink.m_fY + 2.0f, "Global conditions" );
	dc->TextToScreen( 25, sink.m_fY + 3.0f, "----------------------------" );
	pVLog->Log( visLogParams, "-------------------------------" );
	pVLog->Log( visLogParams, "--Global conditions -----------" );
	sink.m_fY += 4.0f;

	CActivationConditionTable*	pGlobalConditions = GetAISystem()->GetBTProfileDictionary()->GetGlobalActivationConditions()->m_pValues;
	if( pGlobalConditions != NULL )
	{
		sink.m_pScriptObject = *( pGlobalConditions->GetTable() );
		pGlobalConditions->Dump( &sink );
	}*/
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalActivationConditions::TransformStartingValues(SmartScriptTable &pConditionsTable)
{
	if( m_pValues != NULL )
	{
		ScriptAnyValue Value;
		IScriptTable::Iterator It = m_pValues->GetTable()->BeginIteration();

		while( m_pValues->GetTable()->MoveNext(It) )
		{
			if( It.value.type == ANY_TSTRING && It.value.str[0] == '#' )
			{
				TransformStartValue( pConditionsTable, It.value.str, Value );
				m_pValues->SetValue( It.sKey, Value );
				Value.Clear();
			}
		}

		m_pValues->GetTable()->EndIteration( It );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalActivationConditions::TransformStartValue( SmartScriptTable &pConditionsTable, const char* sValue, ScriptAnyValue& Value )
{
	assert( sValue != NULL );

	// Skip the first "#" and evaluate as script code
	return( ExecutePreCompiledCode(PreCompileCode(&sValue[1]), pConditionsTable, &Value) );
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalActivationConditions::CopyValues( CActivationConditionTable const* pValues )
{
	assert( pValues != NULL );

	IScriptTable *pValuesTable = m_pValues->GetTable();
	IScriptTable *pCopyTable = pValues->GetTable();
	IScriptTable::Iterator copyIter = pCopyTable->BeginIteration();

	while (pCopyTable->MoveNext(copyIter))
	{
		// Only apply the value if we have the same entry
		if (pValuesTable->HaveValue(copyIter.sKey))
		{
			pValuesTable->SetValue(copyIter.sKey, copyIter.value);
		}
	}

	pCopyTable->EndIteration(copyIter);
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalActivationConditions::AddValues( SmartScriptTable &pTable ) const
{
	CRY_ASSERT(pTable.GetPtr());

	bool bResult = false;

	if (pTable.GetPtr() && m_pValues)
	{
		pTable->SetValue(m_sTable.c_str(), m_pValues->GetTable());
		bResult = true;
	}

	return bResult;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalActivationConditions::ApplyReaction( int iPreCompiledCode, SmartScriptTable &pConditionsTable, const ScriptAnyValue &argValue, bool bDebug ) const
{
	assert( iPreCompiledCode >= 0 );

	ScriptAnyValue Result;
	return( ExecutePreCompiledCode(iPreCompiledCode, pConditionsTable, NULL, argValue) );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalActivationConditions::ApplyAllReactions( SSignalReaction const* pReaction, SmartScriptTable &pConditionsTable, const ScriptAnyValue &argValue, bool bDebug ) const
{
	assert( pReaction != NULL );

	bool bRet = true;

	if( pReaction->pParent != NULL )
	{
		bRet = ApplyAllReactions( pReaction->pParent, pConditionsTable, argValue, bDebug );
	}

	for( uint32 uIndex = 0; uIndex < pReaction->vReactions.size() && bRet == true; ++uIndex )
	{
		bRet = ApplyReaction( pReaction->vReactions[uIndex], pConditionsTable, argValue, bDebug );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalActivationConditions::AssembleFinalCode( string const& sCode, string& sFinalCode ) const
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
int CPersonalActivationConditions::PreCompileCode( const string& sCode ) const
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	assert( sCode.empty() == false );
	assert( m_pValues != NULL );

	int iRet = -1;

	// Assemble final code
	string sFinalCode;
	if( m_pValues->GetParentName().empty() == false && AssembleFinalCode(sCode, sFinalCode) == true )
	{
		IScriptSystem*	pScript = gEnv->pScriptSystem;

		// Pre Catch code
		iRet = pScript->PreCacheBuffer( sFinalCode.c_str(), sFinalCode.length(), "AI Personal Activation Condition" );
	}

	if( iRet < 0 )
	{
		AIWarning( "Could not pre compile LUA code: %s", sCode.c_str() );
	}

	return( iRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalActivationConditions::ExecutePreCompiledCode( int iPreCompiledCode, SmartScriptTable &pConditionsTable, ScriptAnyValue *pResult, const ScriptAnyValue &argValue ) const
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	assert( iPreCompiledCode >= 0 );
	assert( m_pValues != NULL );

	IScriptSystem *pScript = gEnv->pScriptSystem;
	CActivationConditionTable *pTable = GetAISystem()->GetBTProfileDictionary()->GetGlobalActivationConditions()->m_pValues;

	IScriptTable *pUserTable = NULL;
	SBTProfileUser *pUser = GetUser();
	if (pUser && pUser->m_pUser)
	{
		pUserTable = pUser->m_pUser->GetBTUserTable();
	}

	pScript->SetGlobalValue( USER_TABLE_ALIAS, (pUserTable != NULL) ? pUserTable : ScriptAnyValue(ANY_TNIL) );
	pScript->SetGlobalValue( GLOBAL_TABLE_ALIAS, (pTable != NULL) ? pTable->GetTable() : ScriptAnyValue(ANY_TNIL) );
	pScript->SetGlobalValue( ACTOR_TABLE_ALIAS, (pConditionsTable.GetPtr() != NULL) ? pConditionsTable : ScriptAnyValue(ANY_TNIL) );
	pScript->SetGlobalValue( GLOBAL_TABLE_WITH_SIGNAL_DATA, argValue );

	pScript->BeginPreCachedBuffer( iPreCompiledCode );

	if( pResult != NULL )
	{
		return( pScript->EndCallAny(*pResult) );
	}

	return( pScript->EndCall() );
}
