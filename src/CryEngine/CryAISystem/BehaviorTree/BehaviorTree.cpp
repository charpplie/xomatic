/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   BehaviorTree.cpp
$Id$
$DateTime$
Description: Behavior Tree class using CDAG as base. A Behavior Tree is
a formal, tree-like graphical form that represents behavior of individual
or networks of entities which realize or change states, make decisions,
respond-to/cause events, and interact by exchanging information and/or
passing control.
---------------------------------------------------------------------
History:
- 02:06:2007 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#include "StdAfx.h"
#include "BehaviorTree.h"
#include "BehaviorTree_node.h"
#include "ProfileDictionary.h"
#include "TacticsManager.h"
#include "SubTactic.h"
#include "../CryCommon/IPersonalBehaviorTree.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CBehaviorTree::CBehaviorTree( const char* sRootName, uint32 uProfileId )
: m_bDebugTree( false )
, m_bRegistered( false )
, m_bReloadOnGameModeExit( false )
, m_uProfileId( uProfileId )
, m_pTacticsManager( NULL )
{
	SetSkipEmptyRows(true);

	assert( sRootName != NULL );

	m_pRoot = new CBehaviorTree_node( NULL, sRootName, m_uProfileId );
	AssignMethodNames();
}

// Description:
//
// Arguments:
//
// Return:
//
CBehaviorTree::~CBehaviorTree()
{
	UnRegister();
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree::Init()
{
	Register();
}

// Description:
//
// Arguments:
//
// Return:
//
bool CBehaviorTree::Reload( const string& sFile )
{
	return( TParent::ReLoad(sFile) );
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree::Reset()
{
	static_cast<CBehaviorTree_node*>( m_pRoot )->Reset();
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree::Register()
{
	if( m_bRegistered == false && !TParent::m_sConfFile.empty() )
	{
		// Register for file change alerts
		if( gEnv->pFileChangeMonitor != NULL )
		{
			gEnv->pFileChangeMonitor->RegisterListener( this, TParent::m_sConfFile );
		}

		m_bRegistered = true;
	}
}

// Description:
//   Init
// Arguments:
//
// Return:
//
void CBehaviorTree::UnRegister()
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
void CBehaviorTree::UpdateDraw()
{
	if( m_bDebugTree == true )
	{
		DebugDraw();
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree::OnFileChange( const char* sFilename )
{
	if( GetAISystem()->GetBTProfileDictionary()->IsEditing() == true )
	{
		// Not in game mode, so reload now
		bool bOk = TParent::ReLoad();
		if( bOk )
		{
			gEnv->pLog->Log( "BehaviorTree updated successfully: %s", sFilename );
		}
		else
		{
			AIError( "BehaviorTree update failed: %s", sFilename );
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
void CBehaviorTree::OnEditorSetGameMode( bool bGameMode )
{
	if( bGameMode == false )
	{
		Reset();

		if( m_bReloadOnGameModeExit == true )
		{
			m_bReloadOnGameModeExit = false;
			TParent::ReLoad();
		}
	}
	else
	{
		// Reset sub tactic
		static_cast<CBehaviorTree_node*>( m_pRoot )->GetYourSubTactic(m_pTacticsManager);
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree::SetDebugTree( bool bDebug )
{
	m_bDebugTree = bDebug;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CBehaviorTree::GetDebugTree() const
{
	return( m_bDebugTree );
}

// Description:
//
// Arguments:
//
// Return:
//
CSubTactic* CBehaviorTree::FindRunningTacticForResource( TBSSProfileUserId userId, const char* sTactic ) const
{
	assert( userId != g_uBTUserId_Invalid );
	return( m_pTacticsManager->FindResource(userId, sTactic) );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CBehaviorTree::LoadSheet( const string& sName, uint32 uRows, uint32 uColumns, XmlNodeRef& xmlTable )
{
	bool bRet = false;

	bRet = TParent::LoadSheet( sName, uRows, uColumns, xmlTable );

	bool bNodes = ( bRet == false && sName.compareNoCase("Node Properties") == 0 );

	if( bNodes == true )
	{
		bRet = LoadRows( xmlTable );

		if( bRet == true )
		{
			CheckInfiteLoops();
		}
	}

	return( bRet );
}

// Description:
//   Post assignment
// Arguments:
//
// Return:
//
void CBehaviorTree::LoadingFinished()
{
	GetAISystem()->GetBSSProfileManager()->ResetAllUsers();
}

// Description:
//    Creation of tactics pointer
// Arguments:
//
// Return:
//
bool CBehaviorTree::LoadTactics( CTacticsManager* pTacticsManager )
{
	CRY_ASSERT( pTacticsManager );
	m_pTacticsManager = pTacticsManager;
	AssignTactics();
	return( true );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CBehaviorTree::LoadRows( XmlNodeRef& xmlTable )
{
	bool bRet = true;
	bool bFirstSkipped = false;
	uint32 uRealRow = 0;

	for( int iRow = 0; iRow < xmlTable->getChildCount(); ++iRow )
	{
		XmlNodeRef xmlRow = xmlTable->getChild( iRow );

		if( xmlRow->isTag("Row") == true )
		{
			if( bFirstSkipped == true )
			{
				bRet = LoadNodeProperties_Row( xmlRow, uRealRow++ );
			}
			else
			{
				bFirstSkipped = true;
			}
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
bool CBehaviorTree::LoadNodeProperties_Row( XmlNodeRef& xmlRow, uint32 uRow )
{
	bool bRet = false;
	int iColumn = 0;
	string sParent;
	CBehaviorTree_node *pNode = NULL;

	if (m_bSkipEmptyRows && IsRowEmpty(xmlRow))
		return true;

	for( int iCell = 0; iCell < xmlRow->getChildCount(); ++iCell, ++iColumn )
	{
		XmlNodeRef xmlCell = xmlRow->getChild( iCell );
		XmlNodeRef xmlCellData = xmlCell->findChild( "Data" );

		if( xmlCell->getAttr("ss:Index", iColumn) == true )
		{
			--iColumn;
		}

		if( xmlCell->isTag("Cell") == true && xmlCellData )
		{
			switch( iColumn )
			{
			case 0:
				sParent = xmlCellData->getContent();
				break;

			case 1:
				pNode = static_cast<CBehaviorTree_node*>( GetNode(xmlCellData->getContent(), sParent, false) );
				if( pNode == NULL )
				{
					AIError(
						"CBehaviorTree::LoadNodeProperties_Row() Node %s could not be found in tree under parent %s - missing in DAG panel of tree spreadsheet? ",
						xmlCellData->getContent(),
						sParent.c_str() );
				}

				break;

			default:
				if( pNode != NULL )
				{
					bRet = LoadCell( pNode, iColumn, xmlCellData->getContent() );
				}

				break;
			}
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
bool CBehaviorTree::LoadCell( CBehaviorTree_node* pNode, uint32 uCell, const char* sData )
{
	assert( pNode != NULL );

	bool bRet = true;

	switch( uCell )
	{
	case 2:
		{
			CBehaviorTree_node *pNodeTo = NULL;
			string sNode( sData );

			if( sNode.empty() == false )
			{
				int iPos = sNode.rfind( '.' );
				if( iPos >= 0 )
				{
					string sNodeName;
					string sNodePath;
					sNodeName = sNode.substr( iPos + 1, sNode.length() - iPos );
					sNodePath = sNode.substr( 0, iPos );
					pNodeTo = static_cast<CBehaviorTree_node*>( GetNode(sNodeName, sNodePath) );
				}
				else
				{
					pNodeTo = static_cast<CBehaviorTree_node*>( GetNode(sNode) );
				}

				if( pNode != NULL )
				{
					pNode->SetImpulseTo( pNodeTo );
				}
				else
				{
					bRet = false;
				}
			}
		}

		break;

	case 3:
		pNode->SetChildSelectionMethod( GetChildSelectionMethodByName(sData) );
		break;

	case 4:
		pNode->SetSubTactic( sData );
		break;

	case 5:
		pNode->SetSignal( sData );
		break;

	default:
		{
			if( uCell == 6 )
			{
				pNode->AddActivationCondition( sData, true );
			}
			else if( uCell > 6 )
			{
				pNode->AddActivationCondition( sData );
			}
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
bool CBehaviorTree::CheckInfiteLoops()
{
	bool bRet = true;
	CBehaviorTree_node const* pBadNode = static_cast<CBehaviorTree_node*>( GetRoot() )->CheckInfiteLoops();

	if( pBadNode != NULL )
	{
		bRet = false;
		CryFatalError(
			"Infinite loop detected in Behavior Tree xml file [%s], check impulse from node [%s]",
			m_sConfFile.c_str(),
			pBadNode->GetName() );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree::AssignMethodNames()
{
	// do not change the order !
	m_vecMethodNames.push_back( "prioritized" );
	m_vecMethodNames.push_back( "random" );
	m_vecMethodNames.push_back( "random_non_repeat" );
	m_vecMethodNames.push_back( "sequential" );
	m_vecMethodNames.push_back( "sequential_loop" );
}

// Description:
//
// Arguments:
//
// Return:
//
EChildSelectionMethod CBehaviorTree::GetChildSelectionMethodByName( const char* sName ) const
{
	assert( sName != NULL );

	EChildSelectionMethod eRes = ECSM_INVALID;

	for( uint32 uIndex = 0; uIndex < m_vecMethodNames.size(); ++uIndex )
	{
		if( m_vecMethodNames[uIndex].compareNoCase(sName) == 0 )
		{
			eRes = ( EChildSelectionMethod ) uIndex;
			break;
		}
	}

	return( eRes );
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree::AssignTactics()
{
	static_cast<CBehaviorTree_node*>( m_pRoot )->GetYourSubTactic( m_pTacticsManager );
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree::AddInstancedTactic( CTactic* pTactic )
{
	assert( pTactic != NULL );

	static_cast<CBehaviorTree_node*>( m_pRoot )->InstanceTactic( pTactic );
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree::DeleteSubTactics( CTactic const* pTactic )
{
	assert( pTactic != NULL );

	static_cast<CBehaviorTree_node*>( m_pRoot )->DeleteSubTactics( pTactic );
}

// Description:
//
// Arguments:
//
// Return:
//
CBehaviorTree_node* CBehaviorTree::Run( TBSSProfileUserId userId ) const
{
	assert( userId != g_uBTUserId_Invalid );

	// Start running the tree
	CBehaviorTree_node* pRes = (static_cast<CBehaviorTree_node*>(m_pRoot))->Run( userId );

	if( m_bDebugTree == true )
	{
		gEnv->pLog->Log(
			"Behavior Tree: User [%d] reached node [%s]",
			userId,
			(pRes != NULL ? pRes->GetName() : "NULL") );
	}

	// We wipe this candidate from all tactics that did not reached by the user
	if( m_pTacticsManager != NULL )
	{
		m_pTacticsManager->DeleteResource( userId, true );
	}

	return( pRes );
}

// Description:
//
// Arguments:
//
// Return:
//
void CBehaviorTree::OnActorChangeNode( TBSSProfileUserId userId, CBehaviorTree_node* pOld, CBehaviorTree_node* pNew )
{
	if( pOld != NULL )
	{
		pOld->OnResourceLeave( userId );
	}

	if( pNew != NULL )
	{
		pNew->OnResourceEnter( userId );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CBehaviorTree::DisableResource( TBSSProfileUserId userId )
{
	bool bRet = false;

	if( m_pTacticsManager != NULL )
	{
		bRet = m_pTacticsManager->DeleteResource( userId );
	}

	return( bRet );
}
