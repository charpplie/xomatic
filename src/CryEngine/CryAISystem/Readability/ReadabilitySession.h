/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   ReadabilitySession.h
Description: 
---------------------------------------------------------------------
History:
- 08:07:2008 : Created by mieszko

*********************************************************************/
#ifndef _READABILITYSESSION_H_
	#define _READABILITYSESSION_H_

#include "ReadabilitySequence.h"
#include <ISound.h>

class CCoopReadabilitiesSystem;

struct ISound;

namespace Readabilities
{

class CReadabilitySequence;
struct SActorResource;

class CReadabilitySession : public ISoundEventListener
{

public:
	typedef std::vector< SActorResource* > tActorResources;

	/*$1- Basics -------------------------------------------------------------*/
	CReadabilitySession();
	virtual ~CReadabilitySession();

	/*$1- Util ---------------------------------------------------------------*/
	void	SetStep( bool bStep );	
	bool	IsStep() const;	

	bool	Play();
	bool	PlayNextStep();	
	void	Stop( bool bForced );

	uint32	GetId() const;

	tActorResources*				GetActorResources();
	const tActorResources*	GetActorResources() const;

	void												SetSequence( CReadabilitySequence* pSequence );
	const CReadabilitySequence* GetSequence() const;

	// helpers
	const SActorResource*	GetCurrentlySpeaking() const;
	const SSoundResource*	GetCurrentSoundRes() const;
	bool									operator==( const CReadabilitySession& other ) const;

protected:

	bool					PlayNext();	
	virtual void	OnSoundEvent( ESoundCallbackEvent event, ISound* pSound );
	bool					PlaySoundRes( const SSoundResource* pSoundRes );
	void					Finish( bool bForced = false );
	
private:

	/*$1- Members ------------------------------------------------------------*/
	CReadabilitySequence::tSequenceNode*	m_pCurrentSequenceNode;
	uint32						m_nCurrentActor;

	// determines if this session plays sounds one after the other or waist after 
	// every sound played for direct request to continue
	bool						m_bStep;					

	tSoundID				m_soundId;
	ISound*					m_pCurrentSound;
	const SSoundResource*	m_pCurrentSoundRes;
	const SSoundResource*	m_pNextStepSoundRes;
	uint32									m_nSessionId;
	CReadabilitySequence*	m_pSequence; 
	tActorResources				m_actors;

	static uint32	s_nLastId;
	/*$1- Static debug data --------------------------------------------------*/
};

// ----------------------------------------------------------------
// Inline method implementations
// ----------------------------------------------------------------

inline uint32	CReadabilitySession::GetId() const 
{ 
	return m_nSessionId; 
}

inline CReadabilitySession::tActorResources* CReadabilitySession::GetActorResources()
{
	return &m_actors;
}

inline const CReadabilitySession::tActorResources* CReadabilitySession::GetActorResources() const
{
	return &m_actors;
}

inline void CReadabilitySession::SetSequence( CReadabilitySequence* pSequence )
{
	assert( m_pSequence == NULL && "Setting sequence for readability session while sequence already set!" );
	m_pSequence = pSequence;
}

inline const CReadabilitySequence* CReadabilitySession::GetSequence() const
{
	return m_pSequence;
}

inline const SActorResource* CReadabilitySession::GetCurrentlySpeaking() const
{
	return m_actors[ m_nCurrentActor ];
}

inline const SSoundResource* CReadabilitySession::GetCurrentSoundRes() const
{
	return m_pCurrentSoundRes;
}

inline bool	CReadabilitySession::operator==( const CReadabilitySession& other ) const
{
	return m_nSessionId == other.m_nSessionId;
}

inline void CReadabilitySession::SetStep( bool bStep )
{
	m_bStep = bStep;
}

inline bool	CReadabilitySession::IsStep() const
{
	return m_bStep;
}

} // namespace Readabilities

#endif // _READABILITYSESSION_H_


