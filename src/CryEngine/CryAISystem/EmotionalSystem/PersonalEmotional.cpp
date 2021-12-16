/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   PersonalEmotional.h
Description: This class stores per-actor values related to emotional
state. It can only be created from EmotionalSystem and uses one (and
only one) EmotionalInfo to know how to behave when an event is
received.
---------------------------------------------------------------------
History:
- 08:05:2008 : Created by Ricardo Pillosu
- 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/
#include "StdAfx.h"
#include "PersonalEmotional.h"
#include "EmotionalInfo.h"
#include "EmotionalSystem.h"
#include "DebugDrawContext.h"

#define MAX_DEBUG_HISTORY_ENTRIES 35

// Description:
//   Constructor
// Arguments:
//
// Return:
//
SIgnoringEvent::SIgnoringEvent( SEmotionalReaction const* pEvent, float fIgnoringTime ) : m_pEvent( pEvent ), m_fReceivedTime( fIgnoringTime )
{
	assert( pEvent != NULL );
	assert( fIgnoringTime > 0.0f );
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
SIgnoringEvent::SIgnoringEvent( const SIgnoringEvent& sEvent ) : m_pEvent( sEvent.m_pEvent ), m_fReceivedTime( sEvent.m_fReceivedTime )
{
	assert( sEvent.m_pEvent != NULL );
	assert( sEvent.m_fReceivedTime > 0.0f );
}

// Description:
//
// Arguments:
//
// Return:
//
SIgnoringEvent &SIgnoringEvent::operator=( const SIgnoringEvent& sEvent )
{
	assert( sEvent.m_pEvent != NULL );
	assert( sEvent.m_fReceivedTime > 0.0f );

	m_pEvent = sEvent.m_pEvent;
	m_fReceivedTime = sEvent.m_fReceivedTime;
	return( *this );
}

// Description:
//
// Arguments:
//
// Return:
//
bool SIgnoringEvent::operator==( const SIgnoringEvent& sEvent )
{
	return( m_pEvent == sEvent.m_pEvent );
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CPersonalEmotional::CPersonalEmotional( CEmotionalSystem const* pSystem ) : m_bInit( false ), m_pEmotionalSystem( pSystem ), m_EntityId( 0 ), m_pInfo( NULL ), m_fExtraversion( 0.0f ), m_fStability( 0.0f ), m_fClarity( 0.0f ), m_fPleasure( 0.0f ), m_fArousal( 0.0f ), m_fCalm( 0.0f ), m_eCurrentState( EMO_NULL ), m_ePreviousState( EMO_NULL )
{
	assert( pSystem != NULL );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CPersonalEmotional::~CPersonalEmotional()
{
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalEmotional::Init( EntityId Id, CEmotionalInfo const* pInfo )
{
	assert( m_bInit == false );
	assert( Id > 0 );
	assert( pInfo != NULL );

	m_bInit = true;
	m_EntityId = Id;
	m_pInfo = pInfo;

	return( m_bInit );
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalEmotional::Reset()
{
	m_fClarity = 0.0f;
	m_fPleasure = 0.0f;
	m_fArousal = 0.0f;

	m_eCurrentState = EMO_NULL;
	m_ePreviousState = EMO_NULL;

	m_lsIgnoringEvents.clear();
	m_lsDebugHistory.clear();
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalEmotional::Update( float fDelta )
{
	assert( m_bInit == true );

	UpdateDecay( fDelta );

	EEmotionalStates	eState = DeductEmotionalState();

	if( eState != m_eCurrentState )
	{
		m_ePreviousState = m_eCurrentState;
		m_eCurrentState = eState;

		// Signal actor
		IAISignalExtraData*		pExtraData = GetAISystem()->CreateSignalExtraData();
		pExtraData->iValue = (int) m_eCurrentState;
		pExtraData->string1 = GetEmotionalState( m_eCurrentState );
		GetAISystem()->SendSignal(
				SIGNALFILTER_SENDER,
				AISIGNAL_ALLOW_DUPLICATES,
				"OnNewEmoState",
				GetActor(),
				pExtraData );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalEmotional::UpdateDecay( float fDelta )
{
	assert( m_bInit == true );

	float fMod = fDelta * ( m_fCalm * 100.0f );
	m_fArousal = clamp( m_fArousal - fMod, -1.0f, 1.0f );
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalEmotional::SetPersonality( float fExtraversion, float fStability, float fCalm )
{
	assert( fExtraversion <= 1.0f && fExtraversion >= 0.0f );
	assert( fStability <= 1.0f && fStability >= 0.0f );

	m_fExtraversion = clamp( fExtraversion, 0.0f, 1.0f );
	m_fStability = clamp( fStability, 0.0f, 1.0f );
	m_fCalm = clamp( fCalm, 0.0f, 1.0f );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalEmotional::ReceiveEvent( const char* sEvent, int iValue )
{
	assert( sEvent != NULL );

	bool												bRet = false;
	SEmotionalReaction const*		pReaction = m_pInfo->FindReaction( sEvent, iValue );

	if( pReaction != NULL && IsIgnored(pReaction) == false )
	{
		bRet = ApplyReaction( pReaction );
	}

	/*$1- Debug Stuff ----------------------------------------------------------*/
	string	sDebugHistory( sEvent );

	if( iValue >= 0 )
	{
		sDebugHistory.Format( "%s:%d", sEvent, iValue );
	}

	if( bRet == true )
	{
		//sDebugHistory.append( " + EXECUTED" );
		m_lsDebugHistory.push_front( sDebugHistory );
	}
	else
	{
		if( pReaction == NULL )
		{
			//sDebugHistory.append( " - NO REACTION" );
		}
		else
		{
			sDebugHistory.append( " - IGNORED" );
			m_lsDebugHistory.push_front( sDebugHistory );
		}
	}

	if( m_lsDebugHistory.size() > MAX_DEBUG_HISTORY_ENTRIES )
	{
		m_lsDebugHistory.pop_back();
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalEmotional::IsIgnored( SEmotionalReaction const* pReaction ) const
{
	assert( pReaction != NULL );

	bool																			bRet = false;
	std::list<SIgnoringEvent>::const_iterator itC = m_lsIgnoringEvents.begin();
	std::list<SIgnoringEvent>::const_iterator itEnd = m_lsIgnoringEvents.end();
	for( ; itC != itEnd; ++itC )
	{
		if( (*itC).m_pEvent == pReaction )
		{
			if( m_pEmotionalSystem->GetTimePassed() - (*itC).m_fReceivedTime < (*itC).m_pEvent->m_fOverTime )
			{
				bRet = true;
			}

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
CAIActor* CPersonalEmotional::GetActor()
{
	assert( m_bInit == true );

	IEntity* pEntity = gEnv->pEntitySystem->GetEntity( m_EntityId );
	IAIObject *pAI = NULL;

	if( pEntity != NULL )
		pAI = pEntity->GetAI();

	return( CastToCAIActorSafe(pAI) );
}

// Description:
//
// Arguments:
//
// Return:
//
CAIActor const* CPersonalEmotional::GetActor() const
{
	assert( m_bInit == true );

	IEntity* pEntity = gEnv->pEntitySystem->GetEntity( m_EntityId );
	IAIObject *pAI = NULL;
	
	if( pEntity != NULL )
		pAI = pEntity->GetAI();

	return( CastToCAIActorSafe(pAI) );
}

// Description:
//
// Arguments:
//
// Return:
//
EEmotionalStates CPersonalEmotional::DeductEmotionalState() const
{
	if( m_fClarity >= 0.0f )
	{
		if( m_fPleasure >= 0.0f )
		{
			if( m_fArousal >= 0.0f )
			{
				return( EMO_JOY );
			}
			else
			{
				return( EMO_CONTENTMENT );
			}
		}
		else
		{
			if( m_fArousal >= 0.0f )
			{
				return( EMO_ANGER );
			}
			else
			{
				return( EMO_INDIGNATION );
			}
		}
	}
	else
	{
		if( m_fPleasure >= 0.0f )
		{
			if( m_fArousal >= 0.0f )
			{
				return( EMO_ANXIETY );
			}
			else
			{
				return( EMO_DISORIENTED );
			}
		}
		else
		{
			if( m_fArousal >= 0.0f )
			{
				return( EMO_FEAR );
			}
			else
			{
				return( EMO_SADNESS );
			}
		}
	}

	return( EMO_NULL );
}

// Description:
//
// Arguments:
//
// Return:
//
const char* CPersonalEmotional::GetEmotionalState( EEmotionalStates eState ) const
{
	switch( eState )
	{
		case EMO_JOY:
			return( "joy" );

		case EMO_CONTENTMENT:
			return( "contentment" );

		case EMO_ANGER:
			return( "anger" );

		case EMO_INDIGNATION:
			return( "indignation" );

		case EMO_ANXIETY:
			return( "anxiety" );

		case EMO_DISORIENTED:
			return( "disoriented" );

		case EMO_FEAR:
			return( "fear" );

		case EMO_SADNESS:
			return( "sadness" );

		default:
			return( "ERROR" );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CPersonalEmotional::ApplyReaction( const SEmotionalReaction* pReaction )
{
	assert( pReaction != NULL );

	bool bRet = true;

	/*$1- Clarity --------------------------------------------------------------*/
	if( pReaction->m_bForceClarity == true )
	{
		m_fClarity = clamp( FilterBasedOnPersonality(pReaction->m_fClarity), -1.0f, 1.0f );
	}
	else
	{
		m_fClarity = clamp( m_fClarity + FilterBasedOnPersonality(pReaction->m_fClarity), -1.0f, 1.0f );
	}

	/*$1- Pleasure -------------------------------------------------------------*/
	if( pReaction->m_bForcePleasure == true )
	{
		m_fPleasure = clamp( FilterBasedOnPersonality(pReaction->m_fPleasure), -1.0f, 1.0f );
	}
	else
	{
		m_fPleasure = clamp( m_fPleasure + FilterBasedOnPersonality(pReaction->m_fPleasure), -1.0f, 1.0f );
	}

	/*$1- Arousal --------------------------------------------------------------*/
	if( pReaction->m_bForceArousal == true )
	{
		m_fArousal = clamp( FilterBasedOnPersonality(pReaction->m_fArousal), -1.0f, 1.0f );
	}
	else
	{
		m_fArousal = clamp( m_fArousal + FilterBasedOnPersonality(pReaction->m_fArousal), -1.0f, 1.0f );
	}

	AddIgnoreEvent( pReaction );

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalEmotional::AddIgnoreEvent( const SEmotionalReaction* pReaction )
{
	assert( pReaction != NULL );

	if( pReaction->m_fOverTime > 0.0f )
	{
		bool bFound = false;
		std::list<SIgnoringEvent>::iterator itC = m_lsIgnoringEvents.begin();
		std::list<SIgnoringEvent>::iterator itEnd = m_lsIgnoringEvents.end();
		for( ; itC != itEnd; ++itC )
		{
			if( (*itC).m_pEvent == pReaction )
			{
				( *itC ).m_fReceivedTime = m_pEmotionalSystem->GetTimePassed();
				bFound = true;
				break;
			}
		}

		if( bFound == false )
		{
			m_lsIgnoringEvents.push_back( SIgnoringEvent(pReaction, m_pEmotionalSystem->GetTimePassed()) );
		}
	}
}

// Description:
//
// Arguments:
//
// Return:
//
float CPersonalEmotional::FilterBasedOnPersonality( float fStimuli ) const
{
	assert( fStimuli <= 2.0f && fStimuli >= -2.0f );

	float fRet = clamp( fStimuli, -2.0f, 2.0f );

	if( fStimuli >= 0.0f )
	{
		fRet *= m_fExtraversion;
	}
	else
	{
		fRet *= m_fStability;
	}

	return( fRet );
}

// Description:
//
// Arguments:
//
// Return:
//
void CPersonalEmotional::DebugDraw() const
{
	CDebugDrawContext dc;

	ColorB fColor(255, 255, 255);
	float x = 20.0f;
	float y = 50.0f;
	float fSize = 1.8f;
	
	/*
	int iTexId = gEnv->pRenderer->EF_LoadTexture(
			"Textures/Defaults/White.dds",
			 FT_DONT_STREAM|FT_DONT_RELEASE,
			eTF_Unknown )->GetTextureID();
	*/

	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "--------------------------------------------" );
	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "Emotional state of %s", this->GetActor()->GetName() );
	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "--------------------------------------------" );

	dc->Draw2dLabel(
			x,
			y += 20.0f,
			fSize,
			fColor,
			false,
			"Current State: %s",
			GetEmotionalState(m_eCurrentState) );
	dc->Draw2dLabel(
			x,
			y += 20.0f,
			fSize,
			fColor,
			false,
			"Previous State: %s",
			GetEmotionalState(m_ePreviousState) );
	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "Clarity: %0.2f", m_fClarity );
	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "Pleasure: %0.2f", m_fPleasure );
	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "Arousal: %0.2f", m_fArousal );

	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "--------------------------------------------" );
	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "Personality" );
	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "--------------------------------------------" );

	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "Extraversion: %0.2f", m_fExtraversion );
	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "Stability: %0.2f", m_fStability );
	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "Calm: %0.2f", m_fCalm );

	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "--------------------------------------------" );
	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "Events being tracked" );
	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "--------------------------------------------" );
	{
		std::list<SIgnoringEvent>::const_iterator itC = m_lsIgnoringEvents.begin();
		std::list<SIgnoringEvent>::const_iterator itEnd = m_lsIgnoringEvents.end();
		for( ; itC != itEnd; ++itC )
		{
			y += 20.0f;
			dc->Draw2dLabel( x, y, fSize, fColor, false, "%s", (*itC).m_pEvent->m_sEvent.c_str() );
		}
	}

	x += 400.0f;
	y = 50.0f;
	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "--------------------------------------------" );
	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "Last Events received" );
	dc->Draw2dLabel( x, y += 20.0f, fSize, fColor, false, "--------------------------------------------" );

	fSize = 1.5f;
	{
		std::list<string>::const_iterator itC = m_lsDebugHistory.begin();
		std::list<string>::const_iterator itEnd = m_lsDebugHistory.end();
		for( ; itC != itEnd; ++itC )
		{
			dc->Draw2dLabel( x, y += 18.0f, fSize, fColor, false, "%s", (*itC).c_str() );
		}
	}

	/*
	dc->Set2DMode( true, dc->GetWidth(), dc->GetHeight() );
	dc->SetState( GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA );
	dc->Draw2dImage( x, 130, 100, 20, iTexId, 0.f, 1.f, 1.f, 0.f, 0.0f, fRed[0], fRed[1], fRed[2], fRed[3] );
	dc->Set2DMode( false, 0, 0 );
	*/
}
