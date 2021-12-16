/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   ReadabilitySession.cpp
Description: 
---------------------------------------------------------------------
History:
- 08:07:2008 : Created by mieszko

*********************************************************************/
#include "StdAfx.h"

#include <algorithm>

#include "CoopReadabilitiesSystem.h"
#include "ReadabilitySession.h"
#include "ReadabilitySequenceSet.h"
#include "ReadabilitySequence.h"
#include "ActorResourceMgr.h"
#include "SoundResourceSet.h"

#include <IEntityProxy.h>

using namespace Readabilities;

uint32 CReadabilitySession::s_nLastId = 0;

namespace 
{

// class for marking/unmarking actor resources as busy
class CMarkBusy
{
	bool m_bBusy;
public:
	CMarkBusy( bool bBusy ) : m_bBusy( bBusy ) 
	{}

	void operator ( ) ( SActorResource*& pRes ) const
	{
		pRes->bBusy = m_bBusy;
	}
};

} // anynomous namespace 

// ------------------------------------------------------------
// Description:
//   Constructor
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
CReadabilitySession::CReadabilitySession()
: m_nSessionId( ++s_nLastId )
, m_pSequence( NULL )
, m_pCurrentSequenceNode( NULL )
, m_soundId( INVALID_SOUNDID )
, m_nCurrentActor( 0 )
, m_pCurrentSoundRes( NULL )
, m_pNextStepSoundRes( NULL )
{

}

// ------------------------------------------------------------
// Description:
//   Destructor
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
CReadabilitySession::~CReadabilitySession()
{
	// stop sound
	_smart_ptr<ISound> pSound = gEnv->pSoundSystem->GetSound( m_soundId );
	if (pSound)
	{
		pSound->RemoveEventListener( this );
		pSound->Stop();
	}
	m_soundId = INVALID_SOUNDID;
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
bool CReadabilitySession::Play()
{
	bool bSuccess = false;
	assert( m_pSequence != NULL && "Trying to play readability session while not having sequence set for it!" );

	// it's possible that this sequence has been blocked since we picked it. We need to check that.
	if( m_pSequence != NULL && m_pSequence->IsBlocked() == false )
	{
		ISoundSystem *pSoundSystem = gEnv->pSoundSystem;

		// first block resources:		
		// block sequence and let readability sequence set know that this sequence is no longer available
		CReadabilitySequenceSet* pReadabilitySequenceSet = m_pSequence->GetParentSet();
		pReadabilitySequenceSet->StartUsingSequence( this, m_pSequence );
	
		// block actors
		// first size vector down to number of actors really needed for this sequence
		int nActorsNeeded = m_pSequence->GetRequiredActorsCount();
		assert( nActorsNeeded > 0 && nActorsNeeded <= (int)m_actors.size() && "Invalid number of actors needed for readability sequence!" );
		m_actors.resize( nActorsNeeded );
		std::for_each( m_actors.begin(), m_actors.end(), CMarkBusy(true) );

		// Play first readability. m_pCurrentSequenceNode is passed by reference and is filled by m_pSequence rootNode
		m_pCurrentSequenceNode = NULL;
		const SSoundResource* pSoundRes = m_pSequence->GetNext( m_pCurrentSequenceNode );

		if( pSoundRes != NULL )
		{
			bSuccess = PlaySoundRes( pSoundRes );
		}
		else
		{
			Finish();
		}
	}

	return bSuccess;
}


void CReadabilitySession::Stop( bool bForced )
{
	if( bForced == true )
	{
		// this means plying was rudely interrupted so sound currently being 
		// played needs to be stopped
		
		// m_pSequence->Stop();		
		assert( false && "Need to stop playing current sounds!" );
	}

	// unblock sequence	
	CReadabilitySequenceSet* pReadabilitySequenceSet = m_pSequence->GetParentSet();
	pReadabilitySequenceSet->StopUsingSequence( this, m_pSequence );
	
	// unblock actors		
	std::for_each( m_actors.begin(), m_actors.end(), CMarkBusy(false) );
}

bool CReadabilitySession::PlayNext()
{
	bool bSuccess = false;

	const SSoundResource* pSoundRes = m_pSequence->GetNext( m_pCurrentSequenceNode );

	if( pSoundRes != NULL )
	{
		m_nCurrentActor = ( m_nCurrentActor + 1 ) % m_actors.size();
		bSuccess = PlaySoundRes( pSoundRes );
	}
	else
	{
		Finish();
	}

	return bSuccess;
}

// ------------------------------------------------------------
// Description:
//   PlayNext when m_nStep == true
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
bool CReadabilitySession::PlayNextStep()
{
	bool bSuccess = false;

	assert( m_pNextStepSoundRes != NULL && "Invalid next readability step!" );

	if( m_pNextStepSoundRes != NULL )
	{		
		bSuccess = PlaySoundRes( m_pNextStepSoundRes );
	}

	return bSuccess;
}

void CReadabilitySession::OnSoundEvent( ESoundCallbackEvent event, ISound *pSound )
{
	if( event == SOUND_EVENT_ON_STOP )
	{
		pSound->RemoveEventListener(this);
		m_soundId = INVALID_SOUNDID;

		// if no-wait is required proceed to next sound
		if( m_bStep == false )
		{
			PlayNext();
		}
		else // otherwise send interested actors a signal, that this sound has finished
		{
			// first check if it's not the end
			const SSoundResource* pSoundRes = m_pSequence->GetNext( m_pCurrentSequenceNode );

			if( pSoundRes != NULL )
			{
				// save data for next step
				m_nCurrentActor = ( m_nCurrentActor + 1 ) % m_actors.size();
				m_pNextStepSoundRes = pSoundRes;


				tActorResources::iterator itActor = m_actors.begin();
				tActorResources::iterator itEnd = m_actors.end();

				for( int i = 0; itActor != itEnd; ++itActor, ++i )
				{
					IEntity*	pEntity = gEnv->pEntitySystem->GetEntity( (*itActor)->nEntityId );
					CAIActor* pActor = CastToCAIActorSafe( pEntity->GetAI() );

					IAISignalExtraData *pEData = gEnv->pAISystem->CreateSignalExtraData();	// no leak - this will be deleted inside SetSignal
					// in iValue we put information if actor receiving this signal should go next.
					pEData->iValue = (i == m_nCurrentActor ? 1 : 0);
					
					pEData->iValue2 = m_nSessionId;

					// and send to itself
					pActor->SetSignal(0,"OnFinishedCoopReadabilityStep", pEntity, pEData);
				}
			}
			else	
			{
				// no more resources to play - finish
				Finish();
			}
		}
	}
}

bool CReadabilitySession::PlaySoundRes( const SSoundResource* pSoundRes )
{
	bool bSuccess = false;

	IEntity* pEntity = gEnv->pEntitySystem->GetEntity( m_actors[ m_nCurrentActor ]->nEntityId );
	IAIObject* pAI = pEntity->GetAI();
	if (!pAI)
	{
		gEnv->pAISystem->Warning("<CScriptBind_AI> ", "PlayReadabilitySound(): Entity '%s' does not have AI.", pEntity->GetName());
	}
	else
	{
		IAIActorProxy *pAIActorProxy = pAI->GetProxy();
		if (!pAIActorProxy)
		{
			gEnv->pAISystem->Warning("<CScriptBind_AI> ", "PlayReadabilitySound(): Entity '%s' does not have puppet AI proxy.", pEntity->GetName());					
		}
		else
		{
			IEntitySoundProxy* pSoundProxy = (IEntitySoundProxy*) pEntity->GetProxy( ENTITY_PROXY_SOUND );
			if( pSoundProxy == NULL )
			{
				if( pEntity->CreateProxy(ENTITY_PROXY_SOUND ) )
				{
					pSoundProxy = (IEntitySoundProxy*)pEntity->GetProxy(ENTITY_PROXY_SOUND);
				}
			}

			if( pSoundProxy != NULL )
			{						
				// sound proxy uses head pos on dialog sounds
				ISound* pSound = gEnv->pSoundSystem->CreateSound( pSoundRes->sFileName.c_str(), FLAG_SOUND_DEFAULT_3D | FLAG_SOUND_VOICE );

				if (pSound)
				{
					//			pActor->m_ReadibilitySoundID = pSound->GetId();
					//			pSound->AddEventListener( pActor, "AIReadibilityManager" );
					pSound->SetSemantic(eSoundSemantic_AI_Readability);
					//			pActor->m_bSoundFinished = false;
					
					pSoundProxy->PlaySound(pSound, Vec3(ZERO), FORWARD_DIRECTION);
					pSound->AddEventListener( this, "CReadabilitySession" );
					tSoundID soundId = pSound->GetId();
					
					pAIActorProxy->SetReadabilitySound( soundId, true );
					m_soundId = soundId;

					// mark this resource as used
					m_pSequence->GetSoundResourceSet()->MarkResourceUsed( pSoundRes->nResourceId );

					m_pCurrentSound = pSound;
					m_pCurrentSoundRes = pSoundRes;
					bSuccess = true;
				}

				//int soundId = pPuppetProxy->PlayReadabilitySound( pSoundRes->sFileName.c_str(), true);
			}
		}
	}

	return bSuccess;
}

void CReadabilitySession::Finish( bool bForced )
{
	if( bForced == true )
	{
		// stop the sound 
		m_pCurrentSound->RemoveEventListener(this);
		m_soundId = INVALID_SOUNDID;
	//	pPuppetProxy->SetReadabilitySound( INVALID_SOUNDID, true );		
	}

	// free actors
	std::for_each( m_actors.begin(), m_actors.end(), CMarkBusy(false) );

	// free sequence
	CReadabilitySequenceSet* pReadabilitySequenceSet = m_pSequence->GetParentSet();
	pReadabilitySequenceSet->StopUsingSequence( this, m_pSequence );

	// let participants know
	tActorResources::iterator itActor = m_actors.begin();
	tActorResources::iterator itEnd = m_actors.end();
	
	for( int i = 0; itActor != itEnd; ++itActor, ++i )
	{
		IEntity*	pEntity = gEnv->pEntitySystem->GetEntity( (*itActor)->nEntityId );
		CAIActor* pActor = CastToCAIActorSafe( pEntity->GetAI() );

		IAISignalExtraData *pEData = gEnv->pAISystem->CreateSignalExtraData();	// no leak - this will be deleted inside SetSignal
		// in iValue we put information if this was readability starter
		pEData->iValue = (i == 0 ? 1 : 0);
		
		// and send to itself
		pActor->SetSignal(0,"OnFinishedCoopReadability", pEntity, pEData);
	}

	// WARNING: this has to be the last thing that is done in this method. 
	// It can result in destruction of this instance
	CCoopReadabilitiesSystem::GetInstance()->ReadabilitySessionFinished( this );
}