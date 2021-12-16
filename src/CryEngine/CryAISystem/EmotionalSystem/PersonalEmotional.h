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
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#ifndef _PERSONALEMOTIONAL_H_
	#define _PERSONALEMOTIONAL_H_

class CEmotionalSystem;
class CEmotionalInfo;
struct SEmotionalReaction;

#include "IEmotionalSystem.h"

struct SIgnoringEvent
{
	SEmotionalReaction const*		m_pEvent;
	float												m_fReceivedTime;

	SIgnoringEvent( SEmotionalReaction const *pEvent, float fIgnoringTime );
	SIgnoringEvent (const SIgnoringEvent&sEvent);
	SIgnoringEvent &operator	=( const SIgnoringEvent& sEvent );
	bool operator							==( const SIgnoringEvent& sEvent );
};

class CPersonalEmotional
{

	public:

		/*$1- Basics -------------------------------------------------------------*/
		CPersonalEmotional( CEmotionalSystem const* pSystem );
		~			CPersonalEmotional();
		bool	Init( EntityId Id, CEmotionalInfo const* pInfo );
		void	Reset();
		void	Update( float fDelta );

		/*$1- Util ---------------------------------------------------------------*/
		void							SetPersonality( float fExtraversion, float fStability, float fCalm );
		bool							ReceiveEvent( const char* sEvent, int iValue = -1 );
		EntityId					GetEntityId() const;
		void							DebugDraw() const;
		CAIActor const*		GetActor() const;
		CAIActor*					GetActor();
		EEmotionalStates	GetCurrentEmotionalState() const;

	private:
		void							UpdateDecay( float fDelta );
		bool							ApplyReaction( const SEmotionalReaction* pReaction );
		const char*				GetEmotionalState( EEmotionalStates eState ) const;
		float							FilterBasedOnPersonality( float fStimuli ) const;
		void							AddIgnoreEvent( const SEmotionalReaction* pReaction );
		bool							IsIgnored( SEmotionalReaction const* pReaction ) const;
		EEmotionalStates	DeductEmotionalState() const;

	private:

		/*$1- Members ------------------------------------------------------------*/
		bool											m_bInit;
		EEmotionalStates					m_eCurrentState;
		EEmotionalStates					m_ePreviousState;
		CEmotionalSystem const*		m_pEmotionalSystem;
		EntityId									m_EntityId;
		CEmotionalInfo const*			m_pInfo;
		float											m_fExtraversion;
		float											m_fStability;
		float											m_fCalm;
		float											m_fClarity;
		float											m_fPleasure;
		float											m_fArousal;
		std::list<SIgnoringEvent> m_lsIgnoringEvents;

		/*$1- Debug Info ---------------------------------------------------------*/
		std::list<string> m_lsDebugHistory;
};

/*$1- GetEntityId ------------------------------------------------------------*/
inline EntityId CPersonalEmotional::GetEntityId() const
{
	assert( m_bInit == true );
	return( m_EntityId );
}

/*$1- GetCurrentEmotionalState ------------------------------------------------------------*/
inline EEmotionalStates CPersonalEmotional::GetCurrentEmotionalState() const
{
	assert( m_bInit == true );
	return( m_eCurrentState );
}

#endif // _PERSONALEMOTIONAL_H_
