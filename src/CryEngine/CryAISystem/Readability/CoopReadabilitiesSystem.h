/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   CoopReadabilitiesSystem.h
Description: 

Things missing:
* debugging whole sets (ie not only one entity's sessions)
* variable number of actors participating in a sequence (not hardcoded 2)
* add reusibility of soud resources (as a param to resource, in xml file,
	how many times given resource can be used)

---------------------------------------------------------------------
History:
- 08:07:2008 : Created by mieszko
- 2 Mar 2009	: Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#ifndef AIREADABILITIESSYSTEM_H
#define AIREADABILITIESSYSTEM_H

#include <list>
#include "ReadabilitySession.h"
#include "ReadabilitySequence.h"	// only for debugging

namespace Readabilities
{
	class CActorResourceMgr;
	class CSoundResourceMgr;
	class CReadabilitySequenceMgr;	
}

class CCoopReadabilitiesSystem 
{

	CCoopReadabilitiesSystem();
	~CCoopReadabilitiesSystem();

	typedef std::list< Readabilities::CReadabilitySession >	tSessionList;
	typedef	std::map< uint32, Readabilities::CReadabilitySession* >					tStepSessionMap;

public:

	// Get the singleton
	static CCoopReadabilitiesSystem*		GetInstance();

	void	Init();
	void	Reset();
	void	Shutdown();

	bool	LoadData();

	bool	PlayFromSet( const string& sSetName );
	uint32	PlayFromSetStartingWith( const string& sSetName, EntityId nEntityId, bool bStep );
	bool	PlayNextStep( uint32 nSessionId );

	Readabilities::CActorResourceMgr*				GetActorResourceMgr()				{ return m_pActorResourceMgr; }
	const Readabilities::CActorResourceMgr*	GetActorResourceMgr() const	{ return m_pActorResourceMgr; }
	Readabilities::CSoundResourceMgr*				GetSoundResourceMgr()				{ return m_pSoundResourceMgr; }
	const Readabilities::CSoundResourceMgr*	GetSoundResourceMgr() const { return m_pSoundResourceMgr; }
	Readabilities::CReadabilitySequenceMgr*	GetReadabilitySequenceMgr()	{ return m_pSequenceMgr; }
	const Readabilities::CReadabilitySequenceMgr*	GetReadabilitySequenceMgr() const { return m_pSequenceMgr; }

	bool	IsInitialized() const;

	Readabilities::CReadabilitySession* GetNewReadabilitySession();
	void																ReadabilitySessionFinished( Readabilities::CReadabilitySession* pSession );

	// debug drawing
	void	UpdateDraw(EntityId nEntityId ) const;

protected:
	uint32	DebugDrawSequenceNodes( uint32 nColumn, uint32 nLine, const Readabilities::CReadabilitySequence::tSequenceNode* pSequence ) const;

private:
	static CCoopReadabilitiesSystem*		s_pInstance;	// The singleton
	
	Readabilities::CActorResourceMgr*				m_pActorResourceMgr;
	Readabilities::CSoundResourceMgr*				m_pSoundResourceMgr;
	Readabilities::CReadabilitySequenceMgr*	m_pSequenceMgr;

	bool	m_bInit;

	tSessionList		m_activeSessions;
	tStepSessionMap	m_stepSessions;

	// debug helpers 
	const mutable Readabilities::CReadabilitySession*	m_pDebuggedSession;

};

// ----------------------------------------------------------------
// Inline methods implementation
// ----------------------------------------------------------------

inline bool CCoopReadabilitiesSystem::IsInitialized() const
{
	return m_bInit;
}

#endif // AIREADABILITIESSYSTEM_H