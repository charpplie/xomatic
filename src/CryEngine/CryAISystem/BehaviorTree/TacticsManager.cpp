/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   TacticsManager.cpp
$Id$
$DateTime$
Description: Main interface to load tactics and get them by name. 
A tactic is just a filter added to certain Behavior Tree nodes that is
executed AFTER the conditions are met for a certain puppet. The filter
can:
1. Ask for a minimum or maximum of resources to mark the node as
valid.
2. Check if some OTHER tactic is running before marking this node
as valid.
3. Keep monitoring of the resources (entities) allocated in one node
and mark it invalid if less than "min-to-run" resources are running
this node.
---------------------------------------------------------------------
History:
- 12:07:2007 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#include "StdAfx.h"
#include "TacticsManager.h"
#include "TacticInfo.h"
#include "Tactic.h"
#include "SubTactic.h"
#include "BehaviorTree.h"
#include "ProfileDictionary.h"

#include "DebugDrawContext.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CTacticsManager::CTacticsManager( CProfileDictionary *pDictionary )
: m_bReloadOnGameModeExit( false )
, m_bRegistered( false )
, m_uUniqueTacticGroupId( FIRST_TACTIC_GROUP )
, m_pLastInfoLoaded( NULL )
, m_pDictionary( pDictionary )
{
	assert(m_pDictionary);
}

// Description:
//
// Arguments:
//
// Return:
//
CTacticsManager::~CTacticsManager()
{
	Shutdown();
}

// Description:
//
// Arguments:
//
// Return:
//
void CTacticsManager::Init()
{
	Register();
}

// Description:
//
// Arguments:
//
// Return:
//
void CTacticsManager::Register()
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
void CTacticsManager::UnRegister()
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
void CTacticsManager::OnFileChange( const char* sFilename )
{
	if( GetAISystem()->GetBTProfileDictionary()->IsEditing() == true )
	{
		// Not in game mode, so reload now
		bool bOk = ReLoad( m_sFile );
		if( bOk )
		{
			gEnv->pLog->Log( "Tactics updated successfully: %s", sFilename );
		}
		else
		{
			AIError( "Tactics update failed: %s", sFilename );
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
void CTacticsManager::OnEditorSetGameMode( bool bGameMode )
{
	if( bGameMode == false )
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
bool CTacticsManager::ReLoad( const string& sFile )
{
	bool bRet = false;

	XmlNodeRef xmlFile = GetISystem()->LoadXmlFile( sFile );
	if( xmlFile )
	{
		const int iSheetCount = xmlFile->getChildCount();
		for( int iSheet = 0; iSheet < iSheetCount; ++iSheet )
		{
			XmlNodeRef	xmlSheet = xmlFile->getChild( iSheet );
			if( xmlSheet->isTag("Worksheet") == false )
			{
				continue;
			}

			string sSheetName = xmlSheet->getAttr( "ss:Name" );
			XmlNodeRef xmlTable = xmlSheet->findChild( "Table" );
			bRet = LoadSheet( sSheetName, xmlTable );
		}
	}

	if( bRet )
	{
		m_sFile = sFile;
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
void CTacticsManager::Shutdown()
{
	m_pLastInfoLoaded = NULL;

	TTacticsVec::iterator it;
	for( it = m_vecTactics.begin(); it != m_vecTactics.end(); ++it )
	{
		SAFE_DELETE( *it );
	}

	m_vecTactics.clear();

	for( uint32 uIndex = 0; uIndex < m_vecTacticsInfo.size(); ++uIndex )
	{
		SAFE_DELETE( m_vecTacticsInfo[uIndex] );
	}

	m_vecTacticsInfo.clear();

	UnRegister();
}

// Description:
//
// Arguments:
//
// Return:
//
void CTacticsManager::Reset()
{
	TTacticsVec::iterator	it = m_vecTactics.begin();

	while( it != m_vecTactics.end() )
	{
		CTactic *pTactic = *it;

		if( pTactic->IsInstance() == true )
		{
			SAFE_DELETE( pTactic );
			it = m_vecTactics.erase( it );
		}
		else
		{
			pTactic->Reset();
			++it;
		}
	}

	m_uUniqueTacticGroupId = FIRST_TACTIC_GROUP;
}

// Description:
//
// Arguments:
//
// Return:
//
void CTacticsManager::Update()
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	TTacticsVec::iterator itTactic = m_vecTactics.begin();
	while( itTactic != m_vecTactics.end() )
	{
		CTactic *pTactic = *itTactic;
		const CTacticInfo *pInfo = pTactic->GetInfo();
		assert(pInfo);

		bool bCanDelete = false;
		if ( pTactic->IsInstance() == true )
		{
			bCanDelete = ( pTactic->IsObsolete() == true && NeedToCreateInstances(pInfo, pTactic) == false );
			if ( bCanDelete == false && pInfo->UsesGroup() == true )
			{
				TTacticGroupId groupId = pTactic->GetCurrentGroup();
				if ( groupId != NULL_TACTIC_GROUP )
				{
					const uint32 uCount = GetRunningGroupInstancesCount( pInfo, groupId );
					bCanDelete = ( pTactic->ExceedsMaxGroupInstances(uCount) == true );
				}
			}
		}

		if( bCanDelete == true )
		{
			DestroyTacticInstance(pTactic);
			itTactic = m_vecTactics.erase(itTactic);
		}
		else
		{
			if ( pTactic->NeedNewInstance() == true )
			{
				// The tactic will be pushed into m_vecNewTactics, to be parsed at the end of updating
				CreateTacticInstance(pInfo, true);
			}
			else
			{
				pTactic->Update();
			}

			++itTactic;
		}
	}

	// Handle newly created tactic instances
	TTacticsVec::iterator itCreateInstancesEnd = m_vecNewTactics.end();
	for (TTacticsVec::iterator itInstance = m_vecNewTactics.begin(); itInstance != itCreateInstancesEnd; ++itInstance)
	{
		CTactic *pTactic = *itInstance;
		CRY_ASSERT(pTactic);

		pTactic->SetRequestNewInstance(false);

		m_vecTactics.push_back(pTactic);
	}
	m_vecNewTactics.clear();
}

// Description:
//
// Arguments:
//
// Return:
//
void CTacticsManager::DebugDraw() const
{
	uint32 uRows = 1;
	TTacticsVec::const_iterator it;

	float xPos = 30.f;
	float yPos = 100.f;

	CDebugDrawContext dc;
	const ColorB color(255, 25, 25, 255);
	dc->Draw2dLabel(xPos, yPos - 30.0f, 1.5f, color, false, "Count: %u", m_vecTactics.size() );

	for( it = m_vecTactics.begin(); it != m_vecTactics.end(); ++it )
	{
		float yPosCurr = yPos + ( (float) uRows * 30.f );

		uRows += ( *it )->DebugDraw( xPos, yPosCurr );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CTacticsManager::LoadSheet( const string& sName, XmlNodeRef& xml )
{
	bool bRet = false;
	bool bFirstSkipped = false;
	int nRowsCount = xml->getChildCount();

	m_pLastInfoLoaded = NULL;

	const bool bTactics = ( sName.compareNoCase("Tactics") == 0 );
	const bool bRequirements = ( sName.compareNoCase("Requirements") == 0 );

	for( int iRow = 0; iRow < nRowsCount; ++iRow )
	{
		XmlNodeRef xmlRow = xml->getChild( iRow );

		if( xmlRow->isTag("Row") == false )
		{
			continue;
		}

		if( bFirstSkipped == false )
		{
			bFirstSkipped = true;
			continue;
		}

		if( bTactics || bRequirements )
		{
			bRet = LoadRow( xmlRow, bRequirements );
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
bool CTacticsManager::LoadRow( XmlNodeRef& xmlRow, bool bIsRequirements )
{
	// MTJ: This makes quiet a few assumptions when using data from one row to complete another.
	// Duplicated information will currently silently override other bits. Needs checks.
	bool bRet = true;
	CTacticInfo*	pTacticInfo = NULL;
	uint32 uColumn = 0;

	for( int iCell = 0; iCell < xmlRow->getChildCount(); ++iCell )
	{
		XmlNodeRef xmlCell = xmlRow->getChild( iCell );
		XmlNodeRef xmlCellData = xmlCell->findChild( "Data" );

		if( xmlCell->getAttr("ss:Index", uColumn) == false )
		{
			++uColumn;
		}

		if( xmlCell->isTag("Cell") == true && xmlCellData )
		{
			if( uColumn == 1 )
			{
				FindOrCreateTacticsInfo( xmlCellData->getContent(), pTacticInfo );
				assert( pTacticInfo );
			}

			if( pTacticInfo == NULL )
			{
				bRet = m_pLastInfoLoaded->LoadCell( uColumn, xmlCellData->getContent(), bIsRequirements );
			}
			else
			{
				bRet = pTacticInfo->LoadCell( uColumn, xmlCellData->getContent(), bIsRequirements );
			}
		}
	}

	if( pTacticInfo != NULL )
	{
		m_pLastInfoLoaded = pTacticInfo;
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
void CTacticsManager::FindOrCreateTacticsInfo( const string& sName, CTacticInfo *& pOutTacticInfo )
{
	pOutTacticInfo = NULL;

	TTacticInfoVec::iterator itC = m_vecTacticsInfo.begin();
	TTacticInfoVec::iterator itEnd = m_vecTacticsInfo.end();

	for( ; itC != itEnd; ++itC )
	{
		if( (*itC)->GetName().compareNoCase(sName) == 0 )
		{
			pOutTacticInfo = *itC;
			return;
		}
	}

	// Make a new one
	CTacticInfo *pInfo = new CTacticInfo( sName );
	m_vecTacticsInfo.push_back( pInfo );
	pOutTacticInfo = m_vecTacticsInfo.back();
}

// Description:
//
// Arguments:
//
// Return:
//
CTactic* CTacticsManager::GetTactic( const string& sName )
{
	CTactic *pRes = NULL;
	CTacticInfo const* pInfo = GetTacticInfo( sName );

	if( pInfo != NULL )
	{
		pRes = CheckIfTacticExists( pInfo );

		if( pRes == NULL )
		{
			pRes = CreateTactic( pInfo );
		}
	}

	return( pRes );
}

// Description:
//
// Arguments:
//
// Return:
//
CSubTactic* CTacticsManager::GetSubTactic( const string& sName )
{
	CSubTactic *pRes = NULL;
	SSubTacticInfo const* pSubInfo = GetSubTacticInfo( sName );

	if( pSubInfo != NULL )
	{
		CTactic *pTactic = CheckIfTacticExists( pSubInfo->pInfo );

		if( pTactic == NULL )
		{
			pTactic = CreateTactic( pSubInfo->pInfo );
		}

		pRes = pTactic->GetSubTactic( pSubInfo );

		assert( pRes != NULL );
	}

	return( pRes );
}

// Description:
//
// Arguments:
//
// Return:
//
CTactic* CTacticsManager::CreateTactic( CTacticInfo const* pInfo, bool bInstance )
{
	assert( pInfo != NULL );

	// Create the tactic and add to the new container. These will be handled at the end of the frame update.
	CTactic *pRes = new CTactic( pInfo, this, bInstance );
	m_vecNewTactics.push_back( pRes );

	return( pRes );
}

// Description:
//
// Arguments:
//
// Return:
//
void CTacticsManager::DeleteTactic( CTactic* pTactic )
{
	for (TTacticsVec::iterator itTactic = m_vecTactics.begin(); itTactic != m_vecTactics.end(); ++itTactic)
	{
		if ((*itTactic) == pTactic)
		{
			DestroyTacticInstance(pTactic);
			
			SAFE_DELETE(pTactic);
			m_vecTactics.erase(itTactic);

			break;
		}
	}
}

// Description:
//
// Arguments:
//
// Return:
//
CTactic* CTacticsManager::CheckIfTacticExists( CTacticInfo const* pInfo )
{
	assert( pInfo != NULL );

	CTactic *pRes = NULL;
	TTacticsVec::iterator	it;

	for( it = m_vecTactics.begin(); it != m_vecTactics.end(); ++it )
	{
		if( (*it)->GetInfo() == pInfo )
		{
			pRes = ( *it );
			break;
		}
	}

	return( pRes );
}

// Description:
//
// Arguments:
//
// Return:
//
CTacticInfo const* CTacticsManager::GetTacticInfo( const string& sName ) const
{
	assert( sName.empty() == false );

	CTacticInfo const*	pRes = NULL;

	for( uint32 uIndex = 0; uIndex < m_vecTacticsInfo.size(); ++uIndex )
	{
		if( m_vecTacticsInfo[uIndex]->GetName().compareNoCase(sName) == 0 )
		{
			pRes = m_vecTacticsInfo[uIndex];
			break;
		}
	}

	return( pRes );
}

// Description:
//
// Arguments:
//
// Return:
//
SSubTacticInfo const* CTacticsManager::GetSubTacticInfo( const string& sName ) const
{
	assert( sName.empty() == false );

	SSubTacticInfo const* pRes = NULL;
	string sTacticName;
	string sSubTacticName( sName );
	int iPos = sName.find( '.' );

	if( iPos >= 0 )
	{
		sTacticName = sName.substr( 0, iPos );
		sSubTacticName = sName.substr( iPos + 1, sName.length() - iPos );
	}

	if( sTacticName.empty() == false )
	{
		CTacticInfo const*	pTacticInfo = GetTacticInfo( sTacticName );

		if( pTacticInfo != NULL )
		{
			pRes = pTacticInfo->GetSubTacticInfo( sSubTacticName );
		}
	}
	else
	{
		for( uint32 uIndex = 0; uIndex < m_vecTacticsInfo.size(); ++uIndex )
		{
			pRes = m_vecTacticsInfo[uIndex]->GetSubTacticInfo( sName );
			if( pRes != NULL )
			{
				break;
			}
		}
	}

	return( pRes );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CTacticsManager::DeleteResource( TBSSProfileUserId userId, bool bOnlyIfNotInLastRun )
{
	bool bRet = false;
	TTacticsVec::iterator it;

	for( it = m_vecTactics.begin(); it != m_vecTactics.end(); ++it )
	{
		bRet |= ( *it )->DeleteResource( userId, bOnlyIfNotInLastRun );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
CSubTactic* CTacticsManager::FindResource( TBSSProfileUserId userId, const char* sTactic ) const
{
	assert( userId != g_uBTUserId_Invalid );

	// sTactic can be NULL
	CSubTactic *pRet = NULL;
	TTacticsVec::const_iterator it = m_vecTactics.begin();
	TTacticsVec::const_iterator itEnd = m_vecTactics.end();

	for( ; it != itEnd && pRet == NULL; ++it )
	{
		if( sTactic != NULL && (*it)->GetName().compareNoCase(sTactic) != 0 )
		{
			continue;
		}

		pRet = ( *it )->FindResource( userId );
	}

	return( pRet );
}

// Description:
//
// Arguments:
//
// Return:
//
uint32 CTacticsManager::GetNumOfRunningInstances( CTacticInfo const* pInfo )
{
	uint32 uRet = false;
	TTacticsVec::iterator it;

	for( it = m_vecTactics.begin(); it != m_vecTactics.end(); ++it )
	{
		if( (*it)->GetInfo() == pInfo )
		{
			++uRet;
		}
	}

	return( uRet );
}

// Description:
//
// Arguments:
//
// Return:
//   True if we created an instance
bool CTacticsManager::NotifyTacticRunning( CTacticInfo const* pInfo )
{
	bool bRes = true;

	if ( pInfo->UsesGroup() == false )
	{
		// Create a new instance for non-group tactics only once one runs
		bRes = CreateTacticInstance( pInfo );
	}

	return ( bRes );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CTacticsManager::CreateTacticInstance( CTacticInfo const* pInfo, bool bForce )
{
	assert( pInfo != NULL );

	bool bRet = false;
	bool bCreateNewInstance = true;

	switch( pInfo->m_uMaxInstances )
	{
	case 0:
		// 0 means no limit on instancing
		break;

	case 1:
		bCreateNewInstance = false;
		break;

	default:
		{
			if( GetNumOfRunningInstances(pInfo) >= pInfo->m_uMaxInstances )
			{
				bCreateNewInstance = false;
			}
		}

		break;
	}

	if( bCreateNewInstance == true && (bForce == true || NeedToCreateInstances(pInfo) == true) )
	{
		CTactic *pTactic = CreateTactic( pInfo, true );

		// Notify trees of instanced tactic
		CTacticInfo::TBTProfileReferences vecTreeReferences = pInfo->GetBTProfileReferences();
		CTacticInfo::TBTProfileReferences::iterator itReference = vecTreeReferences.begin();
		CTacticInfo::TBTProfileReferences::iterator itReferenceEnd = vecTreeReferences.end();
		for (; itReference != itReferenceEnd; ++itReference)
		{
			SBTProfile *pProfile = m_pDictionary->GetProfile(*itReference);
			assert(pProfile && pProfile->m_pBehaviorTree);
			pProfile->m_pBehaviorTree->AddInstancedTactic(pTactic);
			pProfile->RunAllTrees();
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
void CTacticsManager::DestroyTacticInstance( CTactic const* pTactic )
{
	assert(pTactic);

	const CTacticInfo *pInfo = pTactic->GetInfo();
	assert(pInfo);

	// Notify trees to delete this instance
	CTacticInfo::TBTProfileReferences vecTreeReferences = pInfo->GetBTProfileReferences();
	CTacticInfo::TBTProfileReferences::iterator itReference = vecTreeReferences.begin();
	CTacticInfo::TBTProfileReferences::iterator itReferenceEnd = vecTreeReferences.end();
	for (; itReference != itReferenceEnd; ++itReference)
	{
		SBTProfile *pProfile = m_pDictionary->GetProfile(*itReference);
		assert(pProfile && pProfile->m_pBehaviorTree);
		pProfile->m_pBehaviorTree->DeleteSubTactics(pTactic);
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CTacticsManager::NeedToCreateInstances( CTacticInfo const* pInfo, CTactic const* pSkip )
{
	bool bRet = true;
	TTacticsVec::iterator	it;

	for( it = m_vecTactics.begin(); it != m_vecTactics.end(); ++it )
	{
		CTactic*	pTactic = *it;

		if( pSkip != pTactic && pTactic->GetInfo() == pInfo && pTactic->GetState() != ETS_RUNNING )
		{
			bRet = false;
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
uint32 CTacticsManager::GetRunningGroupInstancesCount( CTacticInfo const* pInfo, TTacticGroupId groupId ) const
{
	uint32 uCount = 0;
	TTacticsVec::const_iterator it;

	for( it = m_vecTactics.begin(); it != m_vecTactics.end(); ++it )
	{
		CTactic *pTactic = *it;
		if( pTactic->GetInfo() == pInfo && pTactic->GetCurrentGroup() == groupId )
		{
			++uCount;
		}
	}

	return( uCount );
}

// Description:
//
// Arguments:
//
// Return:
//
TTacticGroupId CTacticsManager::GenerateUniqueGroupId()
{
	return m_uUniqueTacticGroupId++;
}

// Description:
//
// Arguments:
//
// Return:
//
TTacticGroupId CTacticsManager::GetUserGroupId( TBSSProfileUserId userId ) const
{
	return stl::find_in_map(m_mapUserGroupId, userId, NULL_TACTIC_GROUP);
}

// Description:
//
// Arguments:
//
// Return:
//
bool CTacticsManager::SetUserGroupid( TBSSProfileUserId userId, TTacticGroupId GroupId )
{
	m_mapUserGroupId[userId] = GroupId;
	return true;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CTacticsManager::RemoveUserGroupid( TBSSProfileUserId userId )
{
	return stl::member_find_and_erase(m_mapUserGroupId, userId);
}
