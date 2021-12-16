/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   ReadabilitySequenceMgr.h
Description: 
---------------------------------------------------------------------
History:
- 08:07:2008 : Created by mieszko

*********************************************************************/
#ifndef READABILITYSEQUENCESMGR_H
#define READABILITYSEQUENCESMGR_H

#include <map>
#include "ReadabilitySequenceSet.h"

class CCoopReadabilitiesSystem;

namespace Readabilities
{

class CReadabilitySequenceMgr
{
	friend class CCoopReadabilitiesSystem;

public:

	CReadabilitySequenceMgr( CCoopReadabilitiesSystem* pSystem );

	typedef std::map< string, CReadabilitySequenceSet >	tReadabilitySetsMap;
	
	CReadabilitySequenceSet*	LoadFromXmlNode( const string& sSetName, XmlNodeRef& xmlNode );

	void	Reset();

	CReadabilitySequenceSet* CreateSet( const string& sSetName );
	CReadabilitySequenceSet* GetSet( const string& sSetName );
	const CReadabilitySequenceSet* GetSet( const string& sSetName ) const;

	int		GetSetCount() const;

private:

	void RemoveSet( CReadabilitySequenceSet* pSet );

	CCoopReadabilitiesSystem* m_pSystem;
	tReadabilitySetsMap		m_sets;

};


// ----------------------------------------------------------------
// Inline methods implementation
// ----------------------------------------------------------------

inline int CReadabilitySequenceMgr::GetSetCount() const
{
	return (int)m_sets.size();
}

}

#endif // READABILITYSEQUENCESMGR_H