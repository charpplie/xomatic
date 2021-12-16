/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   PersonalBehaviorTree.cpp
$Id$
$DateTime$
Description: As Behavior Trees are stored only once in memory, they
are read-only. We use this class per puppet to track down the current
position in the tree and manages the changes. It is also able to
manage profile changes. It's also responsible of resending signals
to the activation conditions and check if the state changed. If so, 
behavior tree will be called.
---------------------------------------------------------------------
History:
- 10:06:2007 : Created by Ricardo Pillosu
- 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/

#include "StdAfx.h"
#include "ActivationConditions.h"
#include "PersonalActivationConditions.h"
#include "BehaviorTree.h"
#include "BehaviorTree_node.h"
#include "PersonalBehaviorTree.h"
#include "ProfileDictionary.h"
#include "SubTactic.h"
#include "IVisualLog.h"
#include "DebugDrawContext.h"

#define	BLACKBOARD_DATA_NAME		"LastTreeData"
#define	RECENT_NODES_COUNT			15

CPersonalBehaviorTree const* CPersonalBehaviorTree::sm_pCurrentlyDebugged = NULL;
std::list<string> CPersonalBehaviorTree::sm_lstRecentNodes;

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CPersonalBehaviorTree::CPersonalBehaviorTree()
: m_bReseted( true )
, m_bDisabled( false )
, m_bNeedUpdate( false )
, m_userId( g_uBTUserId_Invalid )
, m_pProfile( NULL )
, m_currentSignalData( NULL_BSS_SIGNAL_DATA )
, m_pPrevious( NULL )
, m_pCurrent( NULL )
, m_pNext( NULL )
{

}

// Description:
//
// Arguments:
//
// Return:
//
CPersonalBehaviorTree::~CPersonalBehaviorTree()
{
	if( m_pProfile != NULL )
	{
		m_pProfile->DelPersonalBT( this );
	}

	DeletePersonalActivationConditions();
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
SBTProfileUser* CPersonalBehaviorTree::GetUser() const
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
const char* CPersonalBehaviorTree::GetUserName() const
{
	SBTProfileUser *pUser = GetUser();
	string sName;
	if ( pUser && pUser->m_pUser && pUser->m_pUser->GetBTUserName(sName) )
	{
		return sName.c_str();
	}

	return "NONAME";
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalBehaviorTree::Update()
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	bool ret = m_bNeedUpdate;

	if( m_bNeedUpdate )
	{
		RunTreeNow();
		m_bNeedUpdate = false;
	}

	return ret;
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalBehaviorTree::Reset()
{
	m_pPrevious = NULL;
	m_pCurrent = NULL;
	m_bReseted = true;
	m_bDisabled = true;
	m_currentSignalData.Clear();
	Enable();
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalBehaviorTree::InitPersonalActivationConditions()
{
	// Temp store for our new set
	TPersonalACVec vecNewPersonalACs;

	if (m_pProfile)
	{
		// Reserve to potential max size
		vecNewPersonalACs.reserve(m_pProfile->m_vecActivationConditions.size());

		CProfileDictionary *pDictionary = GetAISystem()->GetBTProfileDictionary();
		CRY_ASSERT(pDictionary);

		SBTProfile::TActivationConditionsVec::const_iterator itCondition = m_pProfile->m_vecActivationConditions.begin();
		SBTProfile::TActivationConditionsVec::const_iterator itConditionEnd = m_pProfile->m_vecActivationConditions.end();
		for (; itCondition != itConditionEnd; ++itCondition)
		{
			const SBTProfile::SActivationConditionDef &conditionDef = *itCondition;
			CActivationConditions* pConditions = pDictionary->GetActivationConditions(conditionDef.m_sName);
			if (!pConditions)
				continue;

			CPersonalActivationConditions *pPersonal = NULL;

			// Check if we have this one already. If so, just keep it around.
			TPersonalACVec::iterator itPersonal = m_vecPersonalACs.begin();
			TPersonalACVec::iterator itPersonalEnd = m_vecPersonalACs.end();
			for (; itPersonal != itPersonalEnd; ++itPersonal)
			{
				CPersonalActivationConditions *pThisPersonal = *itPersonal;
				CRY_ASSERT(pThisPersonal);

				if (pThisPersonal && conditionDef.m_sName.compareNoCase(pThisPersonal->GetName()) == 0)
				{
					// Reset user id on it
					pThisPersonal->SetProfileUserId(m_userId);

					// Reset the knowledge if needed
					if (conditionDef.m_bMustReset)
					{
						pThisPersonal->Reset();
					}

					// Remove entry from vector. At end, only the ones needed to be deleted should be left in it.
					pPersonal = pThisPersonal;
					m_vecPersonalACs.erase(itPersonal);
					break;
				}
			}

			if (!pPersonal)
			{
				// Create a new one for this
				pPersonal = pConditions->CreatePersonalActivationConditions(m_userId);
			}

			CRY_ASSERT(pPersonal);
			if (pPersonal)
			{
				// Set table name on it
				pPersonal->SetTableName(conditionDef.m_sTable);

				vecNewPersonalACs.push_back(pPersonal);
			}
		}
	}

	// Delete any that are left in our in-use list
	TPersonalACVec::iterator itPersonal = m_vecPersonalACs.begin();
	TPersonalACVec::iterator itPersonalEnd = m_vecPersonalACs.end();
	for (; itPersonal != itPersonalEnd; ++itPersonal)
	{
		CPersonalActivationConditions *pPersonal = *itPersonal;
		CRY_ASSERT(pPersonal);

		CActivationConditions* pConditions = pPersonal->GetActivationConditions();
		if (pConditions)
		{
			pConditions->DeletePersonalActivationConditions(m_userId);
		}
	}

	// Swap out
	m_vecPersonalACs.swap(vecNewPersonalACs);
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalBehaviorTree::DeletePersonalActivationConditions()
{
	TPersonalACVec::iterator itPersonal = m_vecPersonalACs.begin();
	TPersonalACVec::iterator itPersonalEnd = m_vecPersonalACs.end();
	for (; itPersonal != itPersonalEnd; ++itPersonal)
	{
		CPersonalActivationConditions *pPersonal = *itPersonal;
		CRY_ASSERT(pPersonal);

		CActivationConditions* pConditions = pPersonal->GetActivationConditions();
		if (pConditions)
		{
			pConditions->DeletePersonalActivationConditions(m_userId);
		}
	}

	m_vecPersonalACs.clear();
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalBehaviorTree::SetProfile( SBTProfile* pProfile, TBSSProfileUserId userId )
{
	assert( pProfile != NULL );

	bool bRet = false;

	if( pProfile != NULL )
	{
		bRet = true;

		TBSSProfileUserId oldUserId = m_userId;
		m_userId = userId;

		if( pProfile != m_pProfile )
		{
			SBTProfile *pOldProfile = m_pProfile;
			m_pProfile = pProfile;

			// Swap out BTs
			if( pOldProfile != NULL )
			{
				pOldProfile->DelPersonalBT( this );
			}
			m_pProfile->AddPersonalBT( this );

			InitPersonalActivationConditions();
			Reset();
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
bool CPersonalBehaviorTree::SetProfile( const char* sProfile, TBSSProfileUserId userId )
{
	assert( sProfile != NULL );

	bool bRet = false;
	SBTProfile*		pProfile = GetAISystem()->GetBTProfileDictionary()->GetProfile( sProfile );

	if( pProfile != NULL )
	{
		bRet = SetProfile( pProfile, userId );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalBehaviorTree::ReloadProfile()
{
	bool bRet = false;

	if (m_pProfile)
	{
		DeletePersonalActivationConditions();
		InitPersonalActivationConditions();
		Reset();
		bRet = true;
	}

	return bRet;
}

// Description:
//   Marks this tree for execution at the end of the AI tick
// Arguments:
//
// Return:
//
bool CPersonalBehaviorTree::RunTree()
{
	bool bRet = false;

	if( m_bDisabled == false && m_pProfile != NULL )
	{
		bRet = true;
		m_bNeedUpdate = true;
	}

	return( bRet );
}

// Description:
//   Forces execution of the tree, use with care as it is expensive
// Arguments:
//
// Return:
//
bool CPersonalBehaviorTree::RunTreeNow()
{
	bool bRet = false;

	if( m_bDisabled == false && m_pProfile != NULL )
	{
		bRet = true;

		CBehaviorTree_node*		pNode = m_pProfile->m_pBehaviorTree->Run( m_userId );

		if( pNode != NULL )
		{
			bRet = SetNextChildNode( pNode );
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
void CPersonalBehaviorTree::Enable()
{
	if( m_bDisabled == true && m_pProfile != NULL )
	{
		m_bDisabled = false;
		RunTree();
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalBehaviorTree::Disable()
{
	if( m_bDisabled == false && m_pProfile != NULL )
	{
		m_bDisabled = true;
		m_pProfile->m_pBehaviorTree->DisableResource( m_userId );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
SBTProfile* CPersonalBehaviorTree::GetProfile()
{
	return( m_pProfile );
}

// Description:
//
// Arguments:
//
// Return:
//
SBTProfile const* CPersonalBehaviorTree::GetProfile() const
{
	return( m_pProfile );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalBehaviorTree::IsReseted() const
{
	return( m_bReseted );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalBehaviorTree::ReceiveSignal( const char* sSignal, const ScriptAnyValue &argValue )
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	bool bRet = false;

	SBTProfileUser *pUser = GetUser();
	if (pUser)
	{
		const bool bMustRun = pUser->CanTreeRun();
		if (m_bDisabled && bMustRun)
		{
			Enable();
		}
		else if (!m_bDisabled && !bMustRun)
		{
			Disable();
		}
	}

	if( m_bDisabled == false && m_pProfile != NULL )
	{
		if( IsReseted() == true )
		{
			bRet = RunTree();
		}

		/*else
		{*/
		if( sSignal && strlen(sSignal) > 0 )
		{
			if( HandleReceiveSignalCondition( sSignal, argValue ) == true )
			{
				// Fill entity's blackboard with signal data for additional use later
				if (argValue != NULL_BSS_SIGNAL_DATA)
				{
					m_currentSignalData = argValue;
					SetBlackBoardSignalData(argValue);
				}

				m_pProfile->RunAllTrees();
				bRet = true;
			}
		}

		//}
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalBehaviorTree::CreateConditionsTable( SmartScriptTable &pTable ) const
{
	bool bResult = false;

	IScriptSystem *pScriptSystem = gEnv->pScriptSystem;
	CRY_ASSERT(pScriptSystem);

	// Refresh the global table
	if (!pTable.GetPtr())
	{
		pTable.Create(pScriptSystem);
	}
	else
	{
		pTable->Clear();
	}

	// Build personals into it
	TPersonalACVec::const_iterator itPersonal = m_vecPersonalACs.begin();
	TPersonalACVec::const_iterator itPersonalEnd = m_vecPersonalACs.end();
	for (; itPersonal != itPersonalEnd; ++itPersonal)
	{
		const CPersonalActivationConditions *pPersonal = *itPersonal;
		CRY_ASSERT(pPersonal);

		bResult |= pPersonal->AddValues(pTable);
	}

	return bResult;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalBehaviorTree::HandleReceiveSignalCondition( const char* sSignal, const ScriptAnyValue &argValue )
{
	bool bResult = false;

	// Construct the conditions table
	SmartScriptTable pConditionsTable;
	if (CreateConditionsTable(pConditionsTable))
	{
		TPersonalACVec::iterator itPersonal = m_vecPersonalACs.begin();
		TPersonalACVec::iterator itPersonalEnd = m_vecPersonalACs.end();
		for (; itPersonal != itPersonalEnd; ++itPersonal)
		{
			CPersonalActivationConditions *pPersonal = *itPersonal;
			CRY_ASSERT(pPersonal);

			bResult |= pPersonal->ReceiveSignal(pConditionsTable, sSignal, argValue);
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
void CPersonalBehaviorTree::SetBlackBoardSignalData(const ScriptAnyValue &argValue) const
{
	SBTProfileUser *pUser = GetUser();
	IBlackBoard* pBlackBoard = (pUser && pUser->m_pUser ? pUser->m_pUser->GetBTUserBlackBoard() : NULL);
	if (pBlackBoard)
	{
		SmartScriptTable pBlackBoardScript = pBlackBoard->GetForScript();
		pBlackBoardScript->SetValueAny(BLACKBOARD_DATA_NAME, argValue);
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalBehaviorTree::GetBlackBoardSignalData(ScriptAnyValue &argValue) const
{
	bool bResult = false;
	argValue.Clear();

	SBTProfileUser *pUser = GetUser();
	IBlackBoard* pBlackBoard = (pUser && pUser->m_pUser ? pUser->m_pUser->GetBTUserBlackBoard() : NULL);
	if (pBlackBoard)
	{
		SmartScriptTable pBlackBoardScript = pBlackBoard->GetForScript();
		bResult = pBlackBoardScript->GetValueAny(BLACKBOARD_DATA_NAME, argValue);
	}

	return( bResult );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalBehaviorTree::GetBlackBoardSignalData(IAISignalExtraData* &pData) const
{
	CRY_ASSERT(pData);

	bool bResult = false;

	ScriptAnyValue blackBoardData;
	if (pData && GetBlackBoardSignalData(blackBoardData))
	{
		switch (blackBoardData.GetVarType())
		{
			case svtBool:
				pData->iValue = (blackBoardData.b ? 1 : 0);
				break;

			case svtNumber:
				pData->iValue = (int)blackBoardData.number;
				pData->fValue = blackBoardData.number;
				break;

			case svtString:
				pData->string1 = blackBoardData.str;
				break;

			case svtObject:
				pData->FromScriptTable(blackBoardData.table);
				break;

			case svtNull:
				// Do nothing
				break;

			default:
				CRY_ASSERT_MESSAGE(false, "CPersonalBehaviorTree::GetBlackBoardSignalData Blackboard contains data that cannot be placed into a signal extra data object");
				break;
		}

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
void CPersonalBehaviorTree::DebugDraw()
{
	IVisualLog *pVLog = gEnv->pVisualLog;
	assert( pVLog != NULL );

	SVisualLogParams visLogParams( ColorF(1, 1, 1, 1), 1.2f, 3, false );

	if( this != sm_pCurrentlyDebugged )
	{
		sm_lstRecentNodes.clear();
		sm_pCurrentlyDebugged = this;
	}

	CDebugDrawContext dc;

	// Debug my personal ACs
	float fColumnX = 1.0f;
	float fColumnY = 11.0f;
	TPersonalACVec::iterator itPersonal = m_vecPersonalACs.begin();
	TPersonalACVec::iterator itPersonalEnd = m_vecPersonalACs.end();
	for (; itPersonal != itPersonalEnd; ++itPersonal)
	{
		CPersonalActivationConditions *pPersonal = *itPersonal;
		CRY_ASSERT(pPersonal);

		pPersonal->DebugDraw(dc, fColumnX, fColumnY);
		fColumnX += 35.0f;
	}

	dc->TextToScreen( fColumnX, fColumnY+0.0f, "----------------------------" );
	dc->TextToScreen( fColumnX, fColumnY+1.0f, "Recent Behavior tree nodes:" );
	dc->TextToScreen( fColumnX, fColumnY+2.0f, "----------------------------" );
	pVLog->Log( visLogParams, "----------- Recent Behavior tree nodes: -----------" );
	fColumnY += 3.0f;

	if( sm_lstRecentNodes.size() > 0 )
	{
		std::list<string>::iterator itEnd = sm_lstRecentNodes.end();
		const int iX = (int)fColumnX;
		float fColor = 1.0f;
		for( std::list<string>::iterator itVal = sm_lstRecentNodes.begin(); itVal != itEnd; ++itVal )
		{
			dc->TextToScreenColor( iX, (int) fColumnY, fColor, fColor, fColor, 1.0f, "%s", (*itVal).c_str() );
			pVLog->Log( visLogParams, "%s", (*itVal).c_str() );
			fColumnY += 2.0f;
			fColor -= 1.0f / RECENT_NODES_COUNT;
		}
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalBehaviorTree::SetNextChildNode( CBehaviorTree_node* pNewNode )
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	assert( pNewNode != NULL );

	bool bRet = false;

	if( pNewNode != m_pCurrent )
	{
		if( this == sm_pCurrentlyDebugged )
		{
			string sPath;
			GetNodePath( pNewNode, sPath );
			sm_lstRecentNodes.push_front( sPath );

			if( sm_lstRecentNodes.size() > RECENT_NODES_COUNT )
			{
				sm_lstRecentNodes.pop_back();
			}
		}

		m_bReseted = false;
		m_pNext = pNewNode;

		if( m_pCurrent == NULL || m_pCurrent->GetSignal() == m_pNext->GetSignal() )
		{
			AllowNodeChange();
		}
		else
		{
			if( m_pProfile->m_pBehaviorTree->GetDebugTree() == true )
			{
				gEnv->pLog->Log(
						"CBehaviorTree: Asking \'%s\' to get ready for switching from \'%s\', to \'%s\' <-",
						GetUserName(),
						m_pCurrent->GetName(),
						m_pNext->GetName() );
			}

			SBTProfileUser *pUser = GetUser();
			if (pUser)
			{
				pUser->SendEvent(eBTUE_OnNewLeafNode, m_pCurrent->GetSignal(), &m_currentSignalData);
			}
		}

		bRet = true;
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalBehaviorTree::AllowNodeChange()
{
	if( m_pNext != NULL && m_bDisabled == false && m_pProfile != NULL )
	{
		bool bSignalSent = false;
		m_pProfile->m_pBehaviorTree->OnActorChangeNode( m_userId, m_pCurrent, m_pNext );

		if( m_pCurrent == NULL || m_pCurrent->GetSignal() != m_pNext->GetSignal() ) // even if we switch nodes, we won't signal again the same signal string
		{
			SBTProfileUser *pUser = GetUser();
			if (pUser)
			{
				pUser->SendEvent(eBTUE_OnDecisionMade, m_pNext->GetSignal(), &m_currentSignalData);
				bSignalSent = true;
			}
		}

		m_pPrevious = m_pCurrent;
		m_pCurrent = m_pNext;
		m_pNext = NULL;

		if( m_pProfile->m_pBehaviorTree->GetDebugTree() == true )
		{
			if( bSignalSent == true )
			{
				gEnv->pLog->Log(
						"CBehaviorTree: New behavior tree node selected for \'%s\' called \'%s\', sending \'%s\' <-",
						GetUserName(),
						m_pCurrent->GetName(),
						m_pCurrent->GetSignal().c_str() );
			}
			else
			{
				gEnv->pLog->Log(
						"CBehaviorTree: New behavior tree node selected for \'%s\' called \'%s\', but NOT SENDING ANY SIGNAL since it's the same as previous node (%s)<-",
						GetUserName(),
						m_pCurrent->GetName(),
						m_pCurrent->GetSignal().c_str() );
			}
		}
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalBehaviorTree::CheckCondition( int iPreCompiledCode, bool bDebug ) const
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	bool bResult = false;

	TPersonalACVec::const_iterator itPersonal = m_vecPersonalACs.begin();
	TPersonalACVec::const_iterator itPersonalEnd = m_vecPersonalACs.end();
	for (; itPersonal != itPersonalEnd; ++itPersonal)
	{
		const CPersonalActivationConditions *pPersonal = *itPersonal;
		CRY_ASSERT(pPersonal);

		bResult |= pPersonal->CheckCondition(iPreCompiledCode, bDebug);
	}

	return bResult;
}

// Description:
//
// Arguments:
//
// Return:
//
CSubTactic* CPersonalBehaviorTree::GetAssignedTactic( const char* sTactic ) const
{
	// sTactic can be NULL
	CSubTactic *pRet = NULL;

	if( m_pProfile != NULL )
	{
		pRet = m_pProfile->m_pBehaviorTree->FindRunningTacticForResource( m_userId, sTactic );
	}

	return( pRet );
}

// Description:
//
// Arguments:
//
// Return:
//
uint32 CPersonalBehaviorTree::GetUsersInMyTactic( std::vector<TBSSProfileUserId>* pListToFill, const char* sTactic ) const
{
	assert( pListToFill != NULL );
	assert( pListToFill->size() == 0 );

	// sTactic can be NULL
	CSubTactic *pSubTactic = GetAssignedTactic( sTactic );
	if( pSubTactic != NULL )
	{
		pSubTactic->GetAssignedUsers( pListToFill );
	}

	return( pListToFill->size() );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalBehaviorTree::IsWaitingForNodeChange() const
{
	return( m_pNext != NULL );
}

// Description:
//
// Arguments:
//
// Return:
//
const char* CPersonalBehaviorTree::GetNextNode() const
{
	const char*		pRet = NULL;

	if( m_pNext != NULL )
	{
		pRet = m_pNext->GetName();
	}

	return( pRet );
}

// Description:
//
// Arguments:
//
// Return:
//
const char* CPersonalBehaviorTree::GetCurrentNode() const
{
	const char*		pRet = NULL;

	if( m_pCurrent != NULL )
	{
		pRet = m_pCurrent->GetName();
	}

	return( pRet );
}

// Description:
//
// Arguments:
//
// Return:
//
const char* CPersonalBehaviorTree::GetPreviousNode() const
{
	const char*		pRet = NULL;

	if( m_pPrevious != NULL )
	{
		pRet = m_pPrevious->GetName();
	}

	return( pRet );
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalBehaviorTree::GetNextNodePath( string& sPath ) const
{
	GetNodePath( m_pNext, sPath );
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalBehaviorTree::GetCurrentNodePath( string& sPath ) const
{
	GetNodePath( m_pCurrent, sPath );
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalBehaviorTree::GetPreviousNodePath( string& sPath ) const
{
	GetNodePath( m_pPrevious, sPath );
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalBehaviorTree::GetNodePath( CBehaviorTree_node const* pNode, string& sPath ) const
{
	if( pNode != NULL )
	{
		CDagNode const*		pIt = pNode;
		while( pIt != NULL && pIt != m_pProfile->m_pBehaviorTree->GetRoot() )
		{
			if( pIt != pNode )
			{
				sPath.insert( 0, ">" );
			}

			sPath.insert( 0, pIt->GetName() );
			pIt = pIt->GetParent();
		}
	}
}

#include UNIQUE_VIRTUAL_WRAPPER(IPersonalBehaviorTree)