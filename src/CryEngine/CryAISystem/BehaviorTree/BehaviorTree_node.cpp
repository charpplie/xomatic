/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   BehaviorTree_node.cpp
$Id$
$DateTime$
Description: Behavior Tree node (leaf or not) that include it's own
conditions and a callback to any tactics to add candidates to them
if needed. It uses CDagNode as base.
---------------------------------------------------------------------
History:
- 02:06:2007 : Created by Ricardo Pillosu

*********************************************************************/

#include "StdAfx.h"
#include "BehaviorTree_node.h"
#include "ActivationConditions.h"
#include "TacticsManager.h"
#include "Tactic.h"
#include "SubTactic.h"
#include "../CryCommon/IPersonalBehaviorTree.h"
#include "PersonalActivationConditions.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CBehaviorTree_node::CBehaviorTree_node( CBehaviorTree_node* pParent, const char* sName, uint32 uProfileId )
: TParent( pParent, sName )
, m_bActive( true )
, m_uPriority( 0 )
, m_uProfileId( uProfileId )
, m_eChildSelectionMethod( ECSM_INVALID )
, m_pImpulseTo( NULL )
{
	assert( sName != NULL );
}

// Description:
//
// Arguments:
//
// Return:
//
CBehaviorTree_node::~CBehaviorTree_node()
{
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree_node::Reset()
{
	// Delete all subtactics but the first
	if( m_vecSubTactics.size() > 1 )
	{
		TSubTacticsVec::iterator it = m_vecSubTactics.begin();
		m_vecSubTactics.erase( ++it, m_vecSubTactics.end() );
	}

	// Continue with my childs
	for( uint32 uIndex = 0; uIndex < m_vecChilds.size(); ++uIndex )
	{
		(static_cast<CBehaviorTree_node*>(m_vecChilds[uIndex]))->Reset();
	}
}

// Description:
//
// Arguments:
//
// Return:
//
CDagNode* CBehaviorTree_node::AddChild( const char* sName )
{
	assert( sName != NULL );

	CBehaviorTree_node*		pNew = new CBehaviorTree_node( this, sName, m_uProfileId );
	pNew->SetPriority( m_vecChilds.size() );
	m_vecChilds.push_back( pNew );

	return( pNew );
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree_node::DestroyAllChilds()
{
	for( uint32 uIndex = 0; uIndex < m_vecChilds.size(); ++uIndex )
	{
		SAFE_DELETE( m_vecChilds[uIndex] );
	}

	m_vecChilds.clear();
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree_node::SetPriority( uint32 uPriority )
{
	m_uPriority = uPriority;
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree_node::SetChildSelectionMethod( EChildSelectionMethod eMethod )
{
	m_eChildSelectionMethod = eMethod;
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree_node::SetSignal( const string& sSignal )
{
	m_sSignal = sSignal;
	m_sSignal.Trim();
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree_node::SetSubTactic( const string& sSubTactic )
{
	m_sSubTactic = sSubTactic;
	m_sSubTactic.Trim();
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree_node::SetImpulseTo( CBehaviorTree_node* pNodeTo )
{
	assert( pNodeTo != NULL );

	m_pImpulseTo = pNodeTo;
}

// Description:
//
// Arguments:
//
// Return:
//
uint32 CBehaviorTree_node::GetPriority() const
{
	return( m_uPriority );
}

// Description:
//
// Arguments:
//
// Return:
//
EChildSelectionMethod CBehaviorTree_node::GetChildSelectionMethod() const
{
	if( m_pImpulseTo != NULL && m_eChildSelectionMethod == ECSM_INVALID )
	{
		return( m_pImpulseTo->m_eChildSelectionMethod );
	}
	else
	{
		return( m_eChildSelectionMethod );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
const string& CBehaviorTree_node::GetSignal() const
{
	if( m_pImpulseTo != NULL && m_sSignal.empty() == true )
	{
		return( m_pImpulseTo->m_sSignal );
	}
	else
	{
		return( m_sSignal );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
const char* CBehaviorTree_node::GetDebugText() const
{
	static string txt;
	static string tmp;

	txt.Format( "%s", GetName() );

	if( m_sSubTactic.empty() == false )
	{
		tmp.Format( " [%s]", m_sSubTactic.c_str() );
		txt.append( tmp );
	}

	if( m_pImpulseTo != NULL )
	{
		tmp.Format( " -> %s", m_pImpulseTo->GetName() );
		txt.append( tmp );
	}

	return( txt );
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree_node::AddActivationCondition( const string& sCondition, bool bEraseBeforeAdd )
{
	if( bEraseBeforeAdd == true )
	{
		m_vecPreCompiledConditions.clear();
	}

	string sCode;
	sCode.Format( "return %s", sCondition.c_str() );
	sCode.TrimRight( ';' );

	IScriptSystem*	pScript = gEnv->pScriptSystem;
	m_vecPreCompiledConditions.push_back( pScript->PreCacheBuffer(sCode, sCode.length(), "AI Activation Condition"));
}

// Description:
//
// Arguments:
//
// Return:
//
bool CBehaviorTree_node::IsLeafNode() const
{
	if( m_pImpulseTo != NULL )
	{
		return( m_pImpulseTo->m_vecChilds.size() == 0 );
	}
	else
	{
		return( m_vecChilds.empty() );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
CBehaviorTree_node* CBehaviorTree_node::GetImpulse() const
{
	return( m_pImpulseTo );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CBehaviorTree_node::RunSubTactics( TBSSProfileUserId userId )
{
	bool bRet = true;

	TSubTacticsVec::iterator itNext = m_vecSubTactics.begin();
	bool bNeedNewInstance = !m_vecSubTactics.empty();

	while (itNext != m_vecSubTactics.end())
	{
		TSubTacticsVec::iterator itSubTactic = itNext++;
		CSubTactic *pSubTactic = ( *itSubTactic );
		CRY_ASSERT(pSubTactic);
		if (!pSubTactic)
			continue;

		bool bMatchesGroup = false;
		bRet = pSubTactic->Run( userId, bMatchesGroup );

		if( bRet == true )
		{
			bNeedNewInstance = false;
			break;
		}

		// Need a new instance if we don't match all of the tested groups
		if ( !pSubTactic->IsRunning() )
		{
			bNeedNewInstance &= (!bMatchesGroup);
		}
	}

	if (bNeedNewInstance)
	{
		TSubTacticsVec::iterator itFirst = m_vecSubTactics.begin();
		CSubTactic *pSubTactic = ( *itFirst );
		pSubTactic->RequestNewTacticInstance( userId );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree_node::GetYourSubTactic( CTacticsManager* pTacticManager )
{
	assert( pTacticManager != NULL );

	if( m_sSubTactic.empty() == false )
	{
		m_vecSubTactics.push_back( pTacticManager->GetSubTactic(m_sSubTactic) );
	}

	for( uint32 uIndex = 0; uIndex < m_vecChilds.size(); ++uIndex )
	{
		(static_cast<CBehaviorTree_node*>(m_vecChilds[uIndex]))->GetYourSubTactic( pTacticManager );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree_node::InstanceTactic( CTactic* pTactic )
{
	assert( pTactic != NULL );

	if( !m_vecSubTactics.empty() )
	{
		TSubTacticsVec::iterator it = m_vecSubTactics.begin();
		CSubTactic *pSubTactic = pTactic->GetSubTactic( (*it)->GetInfo() );

		if( pSubTactic != NULL )
		{
			m_vecSubTactics.push_back( pSubTactic );
		}
	}

	for( uint32 uIndex = 0; uIndex < m_vecChilds.size(); ++uIndex )
	{
		(static_cast<CBehaviorTree_node*>(m_vecChilds[uIndex]))->InstanceTactic( pTactic );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree_node::DeleteSubTactics( CTactic const* pTactic )
{
	assert( pTactic != NULL );

	TSubTacticsVec::iterator it;
	for( it = m_vecSubTactics.begin(); it != m_vecSubTactics.end(); ++it )
	{
		if( (*it)->GetTactic() == pTactic )
		{
			m_vecSubTactics.erase( it );
			break;
		}
	}

	for( uint32 uIndex = 0; uIndex < m_vecChilds.size(); ++uIndex )
	{
		(static_cast<CBehaviorTree_node*>(m_vecChilds[uIndex]))->DeleteSubTactics( pTactic );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CBehaviorTree_node::CheckConditions( TBSSProfileUserId userId ) const
{
	bool bRet = true;

	SBTProfileUser *pUser = GetAISystem()->GetBTProfileDictionary()->GetProfileUser(userId);

	if( pUser && !m_vecPreCompiledConditions.empty() )
	{
		for( uint32 uIndex = 0; uIndex < m_vecPreCompiledConditions.size() && bRet == true; ++uIndex )
		{
			bRet = pUser->CheckCondition(m_vecPreCompiledConditions[uIndex]);
		}
	}
	else if( m_pImpulseTo != NULL )
	{
		bRet = m_pImpulseTo->CheckConditions( userId );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree_node::OnResourceEnter( TBSSProfileUserId userId )
{
	TSubTacticsVec::iterator it;
	for( it = m_vecSubTactics.begin(); it != m_vecSubTactics.end(); ++it )
	{
		( *it )->OnResourceEnter( userId );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree_node::OnResourceLeave( TBSSProfileUserId userId )
{
	TSubTacticsVec::iterator it;
	for( it = m_vecSubTactics.begin(); it != m_vecSubTactics.end(); ++it )
	{
		( *it )->OnResourceLeave( userId );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
CBehaviorTree_node* CBehaviorTree_node::Run( TBSSProfileUserId userId )
{
	assert( userId > 0 );

	CBehaviorTree_node*		pRet = NULL;
	bool bConditionsValid = CheckConditions( userId );

	/*$1- First, dont go further is Act Conditions are not met -----------------*/
	if( bConditionsValid == false )
	{
		return( NULL );
	}

	/*$1- Second, check my tactic ----------------------------------------------*/
	if( RunSubTactics(userId) == false )
	{
		return( NULL );
	}

	/*$1- Third, if at this stage Im a leaf node stay here ---------------------*/
	if( IsLeafNode() == true )
	{
		return( this );
	}

	/*$1- If we are not a leaf node but we have a signal, send it anyway -------*/
	if( m_sSignal.empty() == false )
	{
		SendSignal( userId );
	}

	/*$1- ok, Act Cond are OK, tactic is OK and im a branch ... go on ----------*/
	pRet = RunChilds( userId );

	return( pRet );
}

// Description:
// Only to be used for non-leaf nodes. This signals can be useful.
// Arguments:
//
// Return:
//
void CBehaviorTree_node::SendSignal( TBSSProfileUserId userId ) const
{
	assert( userId > 0 );
	assert( m_sSignal.empty() == false );

	SBTProfileUser *pUser = GetAISystem()->GetBTProfileDictionary()->GetProfileUser(userId);
	if (pUser)
	{
		pUser->SendEvent(eBTUE_OnNodeSignal, m_sSignal);
	}
}

// Description:
//
// Arguments:
//
// Return:
//
CBehaviorTree_node* CBehaviorTree_node::RunChilds( TBSSProfileUserId userId ) const
{
	assert( userId > 0 );

	CBehaviorTree_node *pRet = NULL;

	switch( GetChildSelectionMethod() )
	{
	case ECSM_PRIORITIZED:
		{
			if( m_pImpulseTo != NULL )
			{
				for( uint32 uIndex = 0; uIndex < m_pImpulseTo->m_vecChilds.size() && pRet == NULL; ++uIndex )
				{
					pRet = (static_cast<CBehaviorTree_node*>(m_pImpulseTo->m_vecChilds[uIndex]))->Run( userId );
				}
			}
			else
			{
				for( uint32 uIndex = 0; uIndex < m_vecChilds.size() && pRet == NULL; ++uIndex )
				{
					pRet = (static_cast<CBehaviorTree_node*>(m_vecChilds[uIndex]))->Run( userId );
				}
			}
		}

		break;

	case ECSM_RANDOM:
	case ECSM_RANDOM_NON_REPEAT:
	case ECSM_SEQUENTIAL:
	case ECSM_SEQUENTIAL_LOOP:
		assert( 0 && "Method not implemented yet" );

	default:
		assert( 0 && "Invalid method" );
	}

	return( pRet );
}

// Description:
//
// Arguments:
//
// Return:
//
CBehaviorTree_node const* CBehaviorTree_node::CheckInfiteLoops()
{
	CBehaviorTree_node const* pBadNode = NULL;

	if( m_pImpulseTo != NULL )
	{
		if( m_pImpulseTo->ExploreTree(this) == false )
		{
			pBadNode = this;
		}
	}

	for( uint32 uIndex = 0; uIndex < m_vecChilds.size() && pBadNode == NULL; ++uIndex )
	{
		pBadNode = (static_cast<CBehaviorTree_node*>(m_vecChilds[uIndex]))->CheckInfiteLoops();
	}

	return( pBadNode );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CBehaviorTree_node::ExploreTree( CBehaviorTree_node const* pOrigin )
{
	bool bRet = true;

	if( this == pOrigin )
	{
		bRet = false;
	}
	else if( m_pImpulseTo != NULL )
	{
		m_pImpulseTo->ExploreTree( pOrigin );
	}
	else
	{
		for( uint32 uIndex = 0; uIndex < m_vecChilds.size() && bRet == true; ++uIndex )
		{
			bRet = (static_cast<CBehaviorTree_node*>(m_vecChilds[uIndex]))->ExploreTree( pOrigin );
		}
	}

	return( bRet );
}
