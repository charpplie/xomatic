/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   ReadabilitySequenceSet.h
Description: 
---------------------------------------------------------------------
History:
- 08:07:2008 : Created by mieszko

*********************************************************************/
#ifndef READABILITYSEQUENCESSET_H
#define READABILITYSEQUENCESSET_H

#include "ReadabilitySequence.h"

class CCoopReadabilitiesSystem;

namespace Readabilities
{

class CReadabilitySession;
class CSoundResourceSet;

class CReadabilitySequenceSet
{

public:
	
	typedef std::vector< CReadabilitySequence > tSequences;
	typedef std::set< CReadabilitySequence* >	tSequenceSet;

	CReadabilitySequenceSet( const string& sSetName );
	CReadabilitySequenceSet( const string& sSetName, int nSize, CSoundResourceSet* pSoundResourceSet );
	
	bool	Init( int nSize, CSoundResourceSet* pSoundResourceSet );
	void	Reset();
	void	Clear();	

	bool	LoadFromXmlNode( XmlNodeRef& xmlNode );

	void	StartUsingSequence( CReadabilitySession* pSession, CReadabilitySequence* pSequence );
	void	StopUsingSequence( CReadabilitySession* pSession, CReadabilitySequence* pSequence );

	CReadabilitySequence*				GetNewSequence();
	const string&								GetName() const;

	CReadabilitySequence*				GetSequence( int nIndex );
	const CReadabilitySequence*	GetSequence( int nIndex ) const;

	CReadabilitySequence*	GetAvailableSequence( int nMaxActors );
	int										GetAvailableSequenceCount() const;

	void											SetSoundResourceSet( CSoundResourceSet* m_pSoundResourceSet );
	CSoundResourceSet*				GetSoundResourceSet();
	const CSoundResourceSet*	GetSoundResourceSet() const;
	
	bool	IsBlank() const;	// instance is Blank, when it was created without no parameters and needs to be initialized
	bool	IsEmpty() const;	// instance is Empty when it has all members set but no sequences registered

	bool	CheckSequencePlayability( CReadabilitySequence* pSequence );

private:
	
	void	ResizeSequenceContainer( int nSize );

	bool		m_bBlank;
	int			m_nSize;
	int			m_nAvialableLeft;
	string	m_sSetName;
	CSoundResourceSet* m_pSoundResourceSet;
	tSequences::iterator	m_itFirstFree;

	// list of sequences currently being played
	tSequenceSet	m_currentlyPlaying;
	// list of unused sequences left
	tSequenceSet	m_available;
	// list of all sequences
	tSequences		m_sequences;
};



// ----------------------------------------------------------------
// Inline functions implementation
// ----------------------------------------------------------------

inline const string& CReadabilitySequenceSet::GetName() const
{
	return m_sSetName;
}

inline int CReadabilitySequenceSet::GetAvailableSequenceCount() const
{
	return m_nAvialableLeft;
}

inline CReadabilitySequence* CReadabilitySequenceSet::GetSequence( int nIndex )
{
	assert( nIndex < m_nSize && "Invalid index while retrieving CReadabilitySequence from CReadabilitySequenceSet!" );
	return &(m_sequences[nIndex]);
}

inline const CReadabilitySequence*	CReadabilitySequenceSet::GetSequence( int nIndex ) const
{
	assert( nIndex < m_nSize && "Invalid index while retrieving CReadabilitySequence from CReadabilitySequenceSet!" );
	return &(m_sequences[nIndex]);
}

inline CSoundResourceSet* CReadabilitySequenceSet::GetSoundResourceSet()
{
	return m_pSoundResourceSet;
}

inline const CSoundResourceSet* CReadabilitySequenceSet::GetSoundResourceSet() const
{
	return m_pSoundResourceSet;
}

inline bool CReadabilitySequenceSet::IsBlank() const
{
	return m_bBlank;
}

inline bool CReadabilitySequenceSet::IsEmpty() const
{
	return m_sequences.size() == 0;
}

} // namespace Readabilities

#endif // READABILITYSEQUENCESSET_H