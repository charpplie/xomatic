/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   SoundResourceMgr.h
Description: 
---------------------------------------------------------------------
History:
- 08:07:2008 : Created by mieszko

*********************************************************************/
#ifndef SOUNDRESOURCEMGR_H
#define SOUNDRESOURCEMGR_H

#include <vector>
#include <map>

#include "SoundResourceSet.h"

/* -------------------------------------------------
* - Named resources
*	- One file can be used multiple times as a resource, but only one resource of a given name can 
*		be registered with given resource set
*
* ---------------------------------------------------*/

class CCoopReadabilitiesSystem;

namespace Readabilities
{

class CSoundResourceMgr
{
	friend class CCoopReadabilitiesSystem;
	typedef std::map< string, CSoundResourceSet > tResourceSets;


public:
	CSoundResourceMgr( CCoopReadabilitiesSystem* pSystem );
	
	void Reset();

	bool	LoadFromFile( const string& sFile );
	CSoundResourceSet*	LoadFromXmlNode( const string& sSetName, XmlNodeRef& xmlNode );

	CSoundResourceSet*				CreateSet( const string& sSetName );
	CSoundResourceSet*				GetSet( const string& sSetName );
	const CSoundResourceSet*	GetSet( const string& sSetName ) const;
		
private:

	void RemoveSet( CSoundResourceSet* pSet );
	
	// ---------------------------------------------------
	// member variables
	// ---------------------------------------------------
	CCoopReadabilitiesSystem*			m_pSystem;	
	tResourceSets								m_mapResourceSets;

};

// ----------------------------------------------------------------
// Inline methods implementation
// ----------------------------------------------------------------
inline CSoundResourceSet* CSoundResourceMgr::GetSet( const string& sSetName )
{
	tResourceSets::iterator itSet = m_mapResourceSets.find( sSetName );
	return itSet != m_mapResourceSets.end() ? &(itSet->second) : NULL;
}

inline const CSoundResourceSet*	CSoundResourceMgr::GetSet( const string& sSetName ) const
{
	tResourceSets::const_iterator itSet = m_mapResourceSets.find( sSetName );
	return itSet != m_mapResourceSets.end() ? &(itSet->second) : NULL;
}

} // namespace Readabilities

#endif // SOUNDRESOURCEMGR_H