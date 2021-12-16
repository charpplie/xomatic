/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   ScriptBind_AI2.cpp
Description: Script Bind for AI
---------------------------------------------------------------------
History:
- 08:02:2008 : Created by ricardo pillosu

*********************************************************************/
#include "StdAfx.h"
#include "ScriptBind_AI2.h"
#include "GroupSystem/GroupSystem.h"
#include "GroupSystem/GroupMember.h"
#include "GroupSystem/FormationDataManager.h"
#include "EmotionalSystem/EmotionalSystem.h"
#include "TacticalPointSystem/TacticalPointSystem.h"

// Variation on the usual macro that just fetches id
#undef GET_ENTITY_ID
#define GET_ENTITY_ID( i )		\
	ScriptHandle	hdl;			\
	pH->GetParam( i, hdl ); \
	EntityId nID = static_cast<EntityId>(hdl.n);

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CScriptBind_AI2::CScriptBind_AI2()
{
	Init( gEnv->pSystem->GetIScriptSystem(), gEnv->pSystem );
	SetGlobalName( "AI2" );

	RegisterGlobals();
	RegisterMethods();
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CScriptBind_AI2::~CScriptBind_AI2()
{
}

// Description:
//
// Arguments:
//
// Return:
//
void CScriptBind_AI2::RegisterGlobals()
{
}

// Description:
//
// Arguments:
//
// Return:
//
void CScriptBind_AI2::RegisterMethods()
{
#undef SCRIPT_REG_CLASSNAME
#define SCRIPT_REG_CLASSNAME	& CScriptBind_AI2::
	SCRIPT_REG_FUNC( CreateGroup );
	SCRIPT_REG_FUNC( DestroyGroup );
	SCRIPT_REG_FUNC( SetGroupLeader );
	SCRIPT_REG_FUNC( GetGroupLeader );
	SCRIPT_REG_FUNC( GetGroup );
	SCRIPT_REG_FUNC( SetGroup );
	SCRIPT_REG_FUNC( AddGroupMember );
	SCRIPT_REG_FUNC( SetGroupMemberSpeed );
	SCRIPT_REG_FUNC( GetGroupMemberCount );
	SCRIPT_REG_FUNC( SetGroupFormationSpec );
	SCRIPT_REG_FUNC( SetGroupMatchLeaderSpeed );
	SCRIPT_REG_FUNC( ClearGroupSystem );

	SCRIPT_REG_FUNC( DestroyAllTPSQueries );

	//SCRIPT_REG_FUNC( MergeGroups ); // will be implemented later on.
	SCRIPT_REG_FUNC( ClearFormationDefinitions );
	SCRIPT_REG_FUNC( RegisterFormation );

	SCRIPT_REG_TEMPLFUNC( ActivateEmotionalSystem, "Entity, sProfile" );
	SCRIPT_REG_TEMPLFUNC( DeactivateEmotionalSystem, "Entity" );
	SCRIPT_REG_TEMPLFUNC( SetEmotionalPersonality, "Entity, fExtraversion, fStability, fCalm" );
}

// Description:
//
// Arguments:
//
// Return:
//
IEntity* CScriptBind_AI2::GetEntity( IScriptTable* pEntityTable )
{
	assert( pEntityTable != NULL );

	IEntity*	pRet = NULL;

	if( pEntityTable != NULL )
	{
		ScriptHandle	handle;
		if( pEntityTable->GetValue("id", handle) == true )
		{
			EntityId id = static_cast<EntityId>(handle.n);
			pRet = gEnv->pEntitySystem->GetEntity( id );
		}
	}

	return( pRet );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::CreateGroup( IFunctionHandler* pH )
{
	GET_ENTITY_ID(1);

	CEntityGroup*		pGroup = gAIEnv.pGroupSystem->CreateGroup( nID );
	EntityGroupId		nGroupId = ( pGroup != NULL ? pGroup->GetGroupId() : 0 );

	return( pH->EndFunction(nGroupId) );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::DestroyGroup( IFunctionHandler* pH )
{
	int nGroupId = 0;
	pH->GetParam( 1, nGroupId );

	gAIEnv.pGroupSystem->DestroyGroup( nGroupId );

	return( pH->EndFunction() );
}

// Description:
//
// Arguments: (groupId, leaderId)
//
// Return:
//
int CScriptBind_AI2::SetGroupLeader( IFunctionHandler* pH )
{
	int nGroupId = 0;
	pH->GetParam( 1, nGroupId );

	GET_ENTITY_ID(2);

	CGroupSystem*		pGroupSystem = gAIEnv.pGroupSystem;
	CEntityGroup*		pGroup = pGroupSystem->GetGroup( nGroupId );
	if( pGroup != NULL )
	{
		pGroup->SetLeader( pGroupSystem->CreateGroupMember(nID) );
	}
	else
	{
		GetAISystem()->Error( "<CScriptBind_AI2> ", "SetGroupLeader() could not find group with ID %d", nGroupId );
	}

	return( pH->EndFunction() );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::GetGroupLeader( IFunctionHandler* pH )
{
	return( pH->EndFunction() );
}

// Description:
//
// Arguments: (entityId)
//
// Return:
//
int CScriptBind_AI2::GetGroup( IFunctionHandler* pH )
{
	GET_ENTITY_ID(1);
	
	CGroupMember*		pMember = gAIEnv.pGroupSystem->GetGroupMember( nID );

	CEntityGroup*		pGroup = ( pMember != NULL ? pMember->GetGroup() : NULL );

	return( pH->EndFunction(pGroup != NULL ? pGroup->GetGroupId() : 0) );
}

// Description:
//	Allows to set group for a given entity. Group is specified by a number, so
//	if a group with a given number doesn't exist the entity doesn't change its
//	group, unless given groupId is 0, in which case group information is removed
//	from entity (CGroupMember's group pointer is NULLified
// Arguments:
//	(entityId, groupId)
// Return:
//	true if a group given by Id could be found or Id == 0.
//	false otherwise
int CScriptBind_AI2::SetGroup( IFunctionHandler* pH )
{
	bool					bResult = false;

	GET_ENTITY_ID(1);

	int nGroupId = 0;
	pH->GetParam( 2, nGroupId );

	CGroupSystem*		pGroupSystem = gAIEnv.pGroupSystem;
	CGroupMember*		pMember = pGroupSystem->GetGroupMember( nID );
	CEntityGroup*		pGroup = ( pMember != NULL ? pMember->GetGroup() : NULL );
	CEntityGroup*		pDestGroup = pGroupSystem->GetGroup( nGroupId );

	if( pGroup != pDestGroup )
	{
		if( pGroup != NULL )
		{
			pGroup->RemoveMember( pMember );
		}

		if( pDestGroup != NULL )
		{
			pDestGroup->AddMember( pMember );
			bResult = true;
		}
	}

	return( pH->EndFunction(bResult) );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::AddGroupMember( IFunctionHandler* pH )
{
	int nGroupId = 0;
	pH->GetParam( 1, nGroupId );

	GET_ENTITY_ID(2);

	CGroupSystem*		pGroupSystem = gAIEnv.pGroupSystem;
	if( pGroupSystem != NULL )
	{
		CGroupMember*		pMember = pGroupSystem->CreateGroupMember( nID );
		if( pGroupSystem->AddMemberToGroup(nGroupId, pMember) == false )
		{
			GetAISystem()->Warning( "<CScriptBind_AI2> ", "AddGroupMember() failed to add group member to the group" );
		}
	}
	else
	{
		GetAISystem()->Error( "<CScriptBind_AI2> ", "AddGroupMember() GroupSystem doesn't exist" );
	}

	return( pH->EndFunction() );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::SetGroupMemberSpeed( IFunctionHandler* pH )
{
	GET_ENTITY_ID(1);

	CGroupMember*		pMember = gAIEnv.pGroupSystem->GetGroupMember( nID );

	if ( pMember )
	{
		float fSpeed = 0.0f;
		pH->GetParam( 2, fSpeed );
		pMember->SetFormationSpeed( fSpeed );
	}

	return( pH->EndFunction() );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::GetGroupMemberCount( IFunctionHandler* pH )
{
	GET_ENTITY_ID(1);

	CGroupMember*		pMember = gAIEnv.pGroupSystem->GetGroupMember( nID );

	CEntityGroup*		pGroup = ( pMember != NULL ? pMember->GetGroup() : NULL );

	return( pH->EndFunction(pGroup != NULL ? pGroup->GetMemberCount() : 0) );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::SetGroupFormationSpec( IFunctionHandler* pH )
{
	int nGroupId = 0;
	pH->GetParam( 1, nGroupId );

	CEntityGroup*		pGroup = gAIEnv.pGroupSystem->GetGroup( nGroupId );

	if( pGroup == NULL )
	{
		GetAISystem()->Error( "<CScriptBind_AI2> ", "SetGroupFormationSpec() could not fing group with ID %d", nGroupId );
	}
	else
	{
		SFormationSpec		formationSpec;

		SmartScriptTable	pTable;
		pH->GetParam( 2, pTable );

		// if some value isn't present in given table original values will remain
		pTable->GetValue( "positionQuery", formationSpec.m_psComputePosQuery );
		pTable->GetValue( "testSelectedQuery", formationSpec.m_psTestComputedPosQuery );
		pTable->GetValue( "testCurMemberPosQuery", formationSpec.m_psTestCurMemberPosQuery );
		pTable->GetValue( "testPerFrameQuery", formationSpec.m_psTestPerFrameQuery );
		pTable->GetValue( "positionTimeout", formationSpec.m_fPositionTimeout );
		pTable->GetValue( "perFrameTestFailureDelay", formationSpec.m_fPerFrameTestFailureDelay );
		pTable->GetValue( "floatingFormationPos", formationSpec.m_bFloatingFormationPos );
		pTable->GetValue( "matchLeaderSpeed", formationSpec.m_bMatchLeaderSpeed );
		pTable->GetValue( "useBestPoint", formationSpec.m_bUseBestPoint );
		pTable->GetValue( "formationSet", formationSpec.m_psFormationSet );

		formationSpec.CalculateQueryIds();

		pGroup->SetFormationSpec( formationSpec );
	}

	return( pH->EndFunction() );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::SetGroupMatchLeaderSpeed( IFunctionHandler* pH )
{
	int nGroupId = 0;
	pH->GetParam( 1, nGroupId );

	CGroupSystem*		pGroupSystem = gAIEnv.pGroupSystem;
	CEntityGroup*		pGroup = pGroupSystem->GetGroup( nGroupId );

	if( pGroup == NULL )
	{
		GetAISystem()->Error( "<CScriptBind_AI2> ", "SetGroupFormationSpec() could not fing group with ID %d", nGroupId );
	}
	else
	{
		bool bMatch = false;
		pH->GetParam(2, bMatch);
		pGroup->SetMatchLeaderSpeed(bMatch);
	}

	return( pH->EndFunction() );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::ClearGroupSystem( IFunctionHandler* pH )
{
	gAIEnv.pGroupSystem->Clear();

	return( pH->EndFunction() );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::MergeGroups( IFunctionHandler* pH )
{
	return( pH->EndFunction() );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::DestroyAllTPSQueries( IFunctionHandler* pH )
{
	gAIEnv.pTacticalPointSystem->DestroyAllQueries();

	return( pH->EndFunction() );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::ClearFormationDefinitions( IFunctionHandler* pH )
{
	CFormationDataManager*	pFDMgr = CFormationDataManager::GetInstance();
	pFDMgr->ClearFormationData();

	return( pH->EndFunction() );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::RegisterFormation( IFunctionHandler* pH )
{
	CFormationDataManager*	pFDMgr = CFormationDataManager::GetInstance();

	// Check and grab specification table
	SmartScriptTable				specTable;
	if( !pH->GetParam(1, specTable) )
	{
		GetAISystem()->Warning( "<CScriptBind_AI2> ", "RegisterFormation(): no table given parameter 1." );
		return( pH->EndFunction() );
	}

	char*		pszSetName = NULL;
	int			nMaxSize = 0;
	if( !specTable->GetValue("SetName", pszSetName) || !specTable->GetValue("MaxSize", nMaxSize) )
	{
		GetAISystem()->Error(
				"<CScriptBind_AI2> ",
				"RegisterFormation() Formation set specification missing name (SetName)." );
		return( pH->EndFunction() );
	}

	CFormationSet						formationSet( nMaxSize, pszSetName );
	int											formationSize = 2;

	IScriptTable::Iterator	itO;
	for( itO = specTable->BeginIteration(); specTable->MoveNext(itO); ++formationSize )
	{
		if( itO.value.type == ANY_TSTRING )
		{
			continue;												// Skip the name field
		}

		SmartScriptTable				subTable; // for formation points
		IScriptTable::Iterator	itS;
		std::list<float>				lstFloats;

		// parameter table is optional
		if( itO.value.type == ANY_TTABLE && itO.value.CopyTo(subTable) )
		{
			for( itS = subTable->BeginIteration(); subTable->MoveNext(itS); )
			{
//				const char*		querySpec = itS.sKey;
				if( itS.value.type == ANY_TNUMBER )
				{
					lstFloats.push_back( itS.value.number );
				}
				else
				{
					GetAISystem()->Warning(
							"<ScriptBind_AI2> ",
							"RegisterFormation() Formation specification takes only numbers since it defines points." );
				}
			}
			subTable->EndIteration( itS );

			formationSet.CreateFormationSpec( formationSize, lstFloats );
		}
	}
	specTable->EndIteration(itO);

	pFDMgr->PushFormationSet( formationSet );

	return( pH->EndFunction() );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::ActivateEmotionalSystem( IFunctionHandler* pH, SmartScriptTable Entity, const char* sProfile )
{
	IEntity*	pEntity = GetEntity( Entity );

	if( pEntity == NULL || sProfile == NULL )
	{
		GetAISystem()->Warning( "<CScriptBind_AI2> ", "ActivateEmotionalSystem(): invalid arguments" );
		return( pH->EndFunction() );
	}

	if( GetAISystem()->GetEmotionalSystem() != NULL )
	{
		GetAISystem()->GetEmotionalSystem()->CreatePersonalEmotional( pEntity->GetId(), sProfile );
	}

	return( pH->EndFunction() );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::DeactivateEmotionalSystem( IFunctionHandler* pH, SmartScriptTable Entity )
{
	IEntity*	pEntity = GetEntity( Entity );

	if( pEntity == NULL )
	{
		GetAISystem()->Warning( "<CScriptBind_AI2> ", "DeactivateEmotionalSystem(): invalid arguments" );
		return( pH->EndFunction() );
	}

	if( GetAISystem()->GetEmotionalSystem() != NULL )
	{
		GetAISystem()->GetEmotionalSystem()->DeletePersonalEmotional( pEntity->GetId() );
	}

	return( pH->EndFunction() );
}

// Description:
//
// Arguments:
//
// Return:
//
int CScriptBind_AI2::SetEmotionalPersonality( IFunctionHandler*		pH,
																							SmartScriptTable Entity,
																							float fExtraversion,
																							float fStability,
																							float fCalm )
{
	IEntity*	pEntity = GetEntity( Entity );

	if( pEntity == NULL )
	{
		GetAISystem()->Warning( "<CScriptBind_AI2> ", "SetEmotionalPersonality(): invalid arguments" );
		return( pH->EndFunction() );
	}

	if( GetAISystem()->GetEmotionalSystem() != NULL )
	{
		GetAISystem()->GetEmotionalSystem()->SetPersonality( pEntity->GetId(), fExtraversion, fStability, fCalm );
	}

	return( pH->EndFunction() );
}
