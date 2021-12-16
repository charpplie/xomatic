/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   ActorResourceMgr.h
Description: 
---------------------------------------------------------------------
History:
- 08:07:2008 : Created by mieszko

*********************************************************************/
#ifndef ACTORRESURCEMGR_H
#define ACTORRESURCEMGR_H

#include <map>
#include <list>
#include <set>

class CCoopReadabilitiesSystem;

namespace Readabilities
{

struct SActorResource
{
	EntityId	nEntityId;
	bool			bBusy;

	SActorResource( EntityId nID = ~0 )
		: nEntityId( nID ), bBusy( false )
	{}
};

class CActorResourceMgr
{
	friend class CCoopReadabilitiesSystem;

	typedef std::map< EntityId, SActorResource >		tActorResources;
	typedef std::set< SActorResource* >							tActorResourceRefs;
	typedef	std::map< string, tActorResourceRefs >	tSetToActorRefs;

public:

	CActorResourceMgr( CCoopReadabilitiesSystem* pSystem );
	~CActorResourceMgr();

	typedef	bool(*tSortFuncPtr)(SActorResource*,SActorResource*);

	void	Reset();

	void	RegisterForReadabilitySet( const string& sSetName, EntityId nEntityId );
	void	UnregisterFromReadabilitySet( const string& sSetName, EntityId nEntityId );
	bool	IsRegisteredFor( const string& sSetName, EntityId nEntityId ) const;
	bool	IsRegisteredFor( const string& sSetName, SActorResource* pActor ) const;

	int		GetRegisteredForSetCount( const string& sSetName ) const;
	int		GetNotBusyForSetCount( const string& sSetName ) const;

	SActorResource*				GetActorResource( EntityId nEntityId );
	const SActorResource*	GetActorResource( EntityId nEntityId ) const;

	void	GetCandidates( const string& sSetName, std::vector< SActorResource* >& outCandidates, tSortFuncPtr pSortFun = NULL );

private:

	CCoopReadabilitiesSystem* m_pSystem;

	tActorResources			m_allActors;	
	tSetToActorRefs			m_mapSetToActorRefs;
};

// ----------------------------------------------------------------
// Inline implementations
// ----------------------------------------------------------------

} // namespace Readabilities

#endif // ACTORRESURCEMGR_H