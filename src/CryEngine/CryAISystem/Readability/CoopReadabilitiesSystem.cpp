/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   AIReadabilitiesSystem.cpp
Description: 
---------------------------------------------------------------------
History:
- 08:07:2008 : Created by mieszko
- 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext
*********************************************************************/
#include "StdAfx.h"

#include "CoopReadabilitiesSystem.h"
#include "ActorResourceMgr.h"
#include "ReadabilitySequenceMgr.h"
#include "ReadabilitySequenceSet.h"
#include "SoundResourceMgr.h"
#include "ReadabilitySession.h"
#include "DebugDrawContext.h"

#include "IVisualLog.h"

#define COOPREADABILITIES_PATH "Libs/Readability/Cooperative"

using namespace Readabilities;

CCoopReadabilitiesSystem* CCoopReadabilitiesSystem::s_pInstance = NULL;

bool SortByDistance( SActorResource* pActor0, SActorResource* pActor1);



CCoopReadabilitiesSystem::CCoopReadabilitiesSystem()
:	m_pActorResourceMgr( NULL )
, m_pSoundResourceMgr( NULL )
, m_pSequenceMgr( NULL )
, m_bInit( false )
, m_pDebuggedSession( NULL )
{

}

CCoopReadabilitiesSystem::~CCoopReadabilitiesSystem()
{
	Shutdown();
}


// Description:
//   
// Arguments:
//
// Return:
//
CCoopReadabilitiesSystem* CCoopReadabilitiesSystem::GetInstance() 
{ 
	if (!s_pInstance) 
		s_pInstance = new CCoopReadabilitiesSystem();

	return s_pInstance;
}

// Description:	Initializes CCoopReadabilitiesSystem instance
//   
void CCoopReadabilitiesSystem::Init()
{
	if( m_bInit == false )
	{
		m_pActorResourceMgr = new CActorResourceMgr( this );
		m_pSoundResourceMgr = new CSoundResourceMgr( this );
		m_pSequenceMgr = new CReadabilitySequenceMgr( this );

		m_bInit = true;
	}
}

// ------------------------------------------------------------
// Description:
//	Reset the whole system to state just after loading static data,
//	that is, clearing 'busy' and 'used' states on all sound resources 
//	and readability sequence sets, clearing out Actor resources (let them 
//	register again).
// ------------------------------------------------------------
void CCoopReadabilitiesSystem::Reset()
{
	if( m_bInit )
	{
		m_pActorResourceMgr->Reset();
		m_pSoundResourceMgr->Reset();
		m_pSequenceMgr->Reset();

		m_activeSessions.clear();
		m_stepSessions.clear();
	}
}

// ------------------------------------------------------------
// Description:	Shuts CCoopReadabilitiesSystem down
//   
// ------------------------------------------------------------
void CCoopReadabilitiesSystem::Shutdown()
{
	SAFE_DELETE( m_pActorResourceMgr );
	SAFE_DELETE( m_pSoundResourceMgr );
	SAFE_DELETE( m_pSequenceMgr );

	m_bInit = false;

	m_activeSessions.clear();
	m_stepSessions.clear();	
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
bool CCoopReadabilitiesSystem::LoadData()
{
	bool bRet = true;

	ICryPak*	pack = gEnv->pCryPak;
	_finddata_t fd;
	string sLibPath( COOPREADABILITIES_PATH );

	string sProfilePath = PathUtil::Make( sLibPath, string("*"), string("xml") );
	intptr_t handle = pack->FindFirst( sProfilePath, &fd );

	if( handle >= 0 )
	{
		do
		{
			// for every file found:
			string sFile = PathUtil::Make(sLibPath, string(fd.name), string("xml"));

			XmlNodeRef	xmlFile = GetISystem()->LoadXmlFile( sFile );

			if( xmlFile )
			{
				CSoundResourceSet* pSoundResourcesSet = NULL;

				for( int iSheet = 0; iSheet < xmlFile->getChildCount(); ++iSheet )
				{
					XmlNodeRef	xmlSheet = xmlFile->getChild( iSheet );

					if( xmlSheet->isTag("Worksheet") == false )
					{
						continue;
					}

					string			sSheetName = xmlSheet->getAttr( "ss:Name" );
					XmlNodeRef	xmlTable = xmlSheet->findChild( "Table" );

					if( sSheetName.compareNoCase("resources") == 0 )
					{
						// load sound resources from it						
						pSoundResourcesSet = m_pSoundResourceMgr->LoadFromXmlNode( PathUtil::GetFileName( fd.name ), xmlTable );
						bRet &= pSoundResourcesSet != NULL;						
					}
					else
					{					
						// load readability sequences
						CReadabilitySequenceSet* pReadabilitySeqSet = m_pSequenceMgr->LoadFromXmlNode( sSheetName, xmlTable );
						bRet &= pReadabilitySeqSet != NULL;
						if( pReadabilitySeqSet != NULL )
						{
							pReadabilitySeqSet->SetSoundResourceSet( pSoundResourcesSet );
						}
					}
				}
			}
		} while( pack->FindNext(handle, &fd) >= 0 );

		pack->FindClose( handle );
	}

	return( bRet );
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//	true	- if succeeded to start readability
//	false	- otherwise
// ------------------------------------------------------------
bool CCoopReadabilitiesSystem::PlayFromSet( const string& sSetName )
{
	bool bSuccess = false;

	// find number of actor available for this sequence
	// NOTE: distance test omitted at this moment
	int nMaxActors = m_pActorResourceMgr->GetNotBusyForSetCount( sSetName );

	if( nMaxActors > 0 )
	{
		// find sequence
		CReadabilitySequenceSet* pSet = m_pSequenceMgr->GetSet( sSetName );
		if( pSet != NULL && pSet->GetAvailableSequenceCount() > 0 )
		{
			CReadabilitySequence* pSequence = pSet->GetAvailableSequence( nMaxActors );
			
			if( pSequence != NULL )
			{
				// create new readability session
				CReadabilitySession* pSession = GetNewReadabilitySession();

				// fill it with data
				pSession->SetSequence( pSequence );			
				m_pActorResourceMgr->GetCandidates( sSetName, *(pSession->GetActorResources()) );
				
				// and let it spin!
				bSuccess = pSession->Play();

				if( bSuccess == false )
				{	
					m_activeSessions.remove( *pSession );
				}
			}
		}
		else
		{
			GetAISystem()->Warning( "<CCoopReadabilitiesSystem::PlayFromSet> ", "No readability sequence set or no available sequences for readability \"%s\".", sSetName.c_str() );
		}
	}
	else
	{
		GetAISystem()->Warning( "<CCoopReadabilitiesSystem::PlayFromSet> ", "No actors to play readability \"%s\".", sSetName.c_str() );
	}

	return bSuccess;
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//	true	- if succeeded to start readability
//	false	- otherwise
// ------------------------------------------------------------
uint32 CCoopReadabilitiesSystem::PlayFromSetStartingWith( const string& sSetName, EntityId nEntityId, bool bStep )
{
	uint32 nSessionId = 0;

	SActorResource* pStarterActor = m_pActorResourceMgr->GetActorResource( nEntityId );

	if( pStarterActor == NULL 
		|| m_pActorResourceMgr->IsRegisteredFor( sSetName, pStarterActor ) == false )
	{
		GetAISystem()->Warning( "<CCoopReadabilitiesSystem::PlayFromSetStartingWith> ", "Readability starter (id %d) is not registered for readability %s", nEntityId, sSetName.c_str() );
	}
	else if( pStarterActor->bBusy == true )
	{
		GetAISystem()->Warning( "<CCoopReadabilitiesSystem::PlayFromSetStartingWith> ", "Readability starter (id %d) is busy at the moment %s", nEntityId, sSetName.c_str() );
	}
	else
	{
		// find number of actor available for this sequence
		// NOTE: distance test omitted at this moment
		int nMaxActors = m_pActorResourceMgr->GetNotBusyForSetCount( sSetName );

		if( nMaxActors > 0 )
		{
			// find sequence
			CReadabilitySequenceSet* pSet = m_pSequenceMgr->GetSet( sSetName );
			if( pSet != NULL && pSet->GetAvailableSequenceCount() > 0 )
			{
				CReadabilitySequence* pSequence = pSet->GetAvailableSequence( nMaxActors );

				if( pSequence != NULL )
				{
					// create new readability session
					CReadabilitySession* pSession = GetNewReadabilitySession();
					pSession->SetStep( bStep );

					// fill it with data
					pSession->SetSequence( pSequence );
					CReadabilitySession::tActorResources* pCandidates = pSession->GetActorResources();
					m_pActorResourceMgr->GetCandidates( sSetName, *pCandidates );

					// make sure our main actor (the one given by parameter) is first on the list
					// NOTE: Naive implementation
					CReadabilitySession::tActorResources::iterator itFirstActor = pCandidates->begin();
					// if main actor is not the first one
					if( *itFirstActor != pStarterActor )
					{
						// make him first

						// start from the second (we DO know main actor is on this list - if not, a big FAIL)
						CReadabilitySession::tActorResources::iterator itSearch = itFirstActor + 1;
						
						// we check only the number of actors required for this sequence
						int nNeededActors = pSequence->GetRequiredActorsCount();
						int nFoundAt = nNeededActors;
						for( int i = 1; i < nNeededActors; ++i, ++itSearch )
						{
							if( *itSearch == pStarterActor )
							{
								// found! leave this loop to make him first
								nFoundAt = i;
								break;
							}
						}

						// first move all others one place back
						for( int j = nFoundAt; j > 0; --j, --itSearch )
						{
							*itSearch = *( itSearch - 1 );
						}

						assert( itSearch == itFirstActor && "Making selected actor first to play sequence - failed!" );
						*itFirstActor = pStarterActor;

					}

					// and let it spin!
					bool bSuccess = pSession->Play();					
					
					if( bSuccess )
					{						
						if( bStep == true )
						{
							// if it's a step readability then place it in the map for ease of finding
							m_stepSessions.insert( std::make_pair( pSession->GetId(), pSession ) );
						}
						nSessionId = pSession->GetId();
					}
					else
					{
						// remove from active sessions list
						m_activeSessions.remove( *pSession );

						nSessionId = 0;
					}
				}
				else
				{
					GetAISystem()->Warning( "<CCoopReadabilitiesSystem::PlayFromSetStartingWith> ", "No more available readabiliy sequences in set \"%s\".", sSetName.c_str() );
				}
			}
			else
			{
				GetAISystem()->Warning( "<CCoopReadabilitiesSystem::PlayFromSetStartingWith> ", "No readability sequence set or no available sequences for readability \"%s\".", sSetName.c_str() );
			}
		}
		else
		{
			GetAISystem()->Warning( "<CCoopReadabilitiesSystem::PlayFromSetStartingWith> ", "No actors to play readability \"%s\".", sSetName.c_str() );
		}
	}

	return nSessionId;
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
bool CCoopReadabilitiesSystem::PlayNextStep( uint32 nSessionId )
{
	bool bSuccess = false;

	// find session with given id
	tStepSessionMap::iterator itIdSession = m_stepSessions.find( nSessionId );

	if( itIdSession != m_stepSessions.end() )
	{
		CReadabilitySession* pSession = itIdSession->second;
		bSuccess = pSession->PlayNextStep();
	}

	return bSuccess;
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
CReadabilitySession* CCoopReadabilitiesSystem::GetNewReadabilitySession()
{
	m_activeSessions.push_back( CReadabilitySession() );
	CReadabilitySession* pNewSession = &(m_activeSessions.back());

	return pNewSession;
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
void CCoopReadabilitiesSystem::ReadabilitySessionFinished( CReadabilitySession* pSession )
{
	if( pSession->IsStep() == true )
	{
		m_stepSessions.erase( pSession->GetId() );
	}

	// this destroys instance represented by pSession (if it was created by CCoopReadabilitiesSystem)
	// after this line pSession pointer is no longer valid!
	m_activeSessions.remove( *pSession );
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
void CCoopReadabilitiesSystem::UpdateDraw(EntityId nEntityId ) const
{
	IVisualLog*	pVLog = gEnv->pVisualLog;
	assert(pVLog);
	
	SVisualLogParams	visLogParams( ColorF(1, 1, 1, 1), 1.2f, 3, false );

	const CReadabilitySession* pSession = NULL;
	// find requested actor
	SActorResource* pActorRes = m_pActorResourceMgr->GetActorResource( nEntityId );
	if( pActorRes != NULL )
	{
		// find a sequence with this actor being played
		tSessionList::const_iterator it = m_activeSessions.begin();
		tSessionList::const_iterator itEnd = m_activeSessions.end();
		for( ; it != itEnd && pSession == NULL; ++it )
		{
			// check all actors used by this sequence
			const CReadabilitySession::tActorResources* pUsedActors = it->GetActorResources();
			CReadabilitySession::tActorResources::const_iterator itUsedActor = pUsedActors->begin();
			CReadabilitySession::tActorResources::const_iterator itUsedActorsEnd = pUsedActors->end();
			for( ; itUsedActor != itUsedActorsEnd; ++itUsedActor )
			{
				if( *itUsedActor == pActorRes )
				{
					pSession = &(*it);
					break;
				}
			}
		}
	}

	if( pSession != NULL )
	{
		CDebugDrawContext dc;
	
		m_pDebuggedSession = pSession;
		// first, draw every actor used 
		const CReadabilitySession::tActorResources* pUsedActors = pSession->GetActorResources();
		CReadabilitySession::tActorResources::const_iterator itUsedActor = pUsedActors->begin();
		CReadabilitySession::tActorResources::const_iterator itUsedActorsEnd = pUsedActors->end();
		for( ; itUsedActor != itUsedActorsEnd; ++itUsedActor )
		{
			SActorResource* pActor = *itUsedActor;

			IEntity*	pEntity = gEnv->pEntitySystem->GetEntity( pActor->nEntityId );

			dc->DrawCylinder( pEntity->GetWorldPos() + Vec3(0, 0, 0.9f), Vec3(0, 0, 1), 0.3f, 1.8f, ColorB(255, 255, 0, 153) );
		}

		// mark the one currently speaking
		IEntity*	pEntity = gEnv->pEntitySystem->GetEntity( pSession->GetCurrentlySpeaking()->nEntityId );
		dc->DrawCone( pEntity->GetWorldPos() + Vec3(0, 0,    4), Vec3(0, 0, -1), 0.3f, 2.2f, ColorB(0, 153, 0, 153) );
		dc->DrawCone( pEntity->GetWorldPos() + Vec3(0, 0, 2.8f), Vec3(0, 0, -1), 0.3f,  1.f, ColorB(0, 153, 0, 153) );
		
		dc->TextToScreen( 1, 15, "-------------------------------" );
		dc->TextToScreen( 1, 16, "-- Cooperative Readabilities --" );
		dc->TextToScreen( 1, 17, "-------------------------------" );		
		
		//pVLog->Log( visLogParams, "----------- Groups and members: -----------" );
		
		uint32 nLine = 19;
		uint32 nColumn = 1;
		nLine = DebugDrawSequenceNodes(nColumn, nLine, pSession->GetSequence()->GetRoot() );

	} // if( pSession != NULL )
}

uint32 CCoopReadabilitiesSystem::DebugDrawSequenceNodes(uint32 nColumn, uint32 nLine, const CReadabilitySequence::tSequenceNode* pNode ) const
{
	const SSoundResource* pSoundRes = m_pDebuggedSession->GetSequence()->GetSoundResourceSet()->GetResource( pNode->GetData().sResourceName.c_str() );
	
	CDebugDrawContext dc;
	if( pSoundRes == m_pDebuggedSession->GetCurrentSoundRes() )
	{
		dc->TextToScreenColor( nColumn, nLine, 1,0,1,1, "--> %s", pNode->GetData().sResourceName.c_str() );
	}
	else if( pSoundRes->bUsed )
	{
		dc->TextToScreenColor( nColumn, nLine, 1,0,0,1, "--> %s", pNode->GetData().sResourceName.c_str() );
	}
	else
	{
		dc->TextToScreenColor( nColumn, nLine, 0,1,0,1, "--> %s", pNode->GetData().sResourceName.c_str() );
	}

	if( pNode->GetChildrenCount() > 0 )
	{
		CReadabilitySequence::tSequenceNode::tChildrenConstIterator itChild = pNode->GetChildrenIterator();
		CReadabilitySequence::tSequenceNode::tChildrenConstIterator itEnd = pNode->GetChildrenEndIterator();
		for( ; itChild != itEnd; ++itChild )
		{
			nLine = DebugDrawSequenceNodes(nColumn + 10, nLine + 1, *itChild ) + 1;
		}
	}

	return nLine;
}

// ----------------------------------------------------------------
// Helper functions
// ----------------------------------------------------------------
bool SortByDistance( SActorResource* pActor0, SActorResource* pActor1)
{
	return false;
}
