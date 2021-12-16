/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   ReadabilitySequence.h
Description: 
---------------------------------------------------------------------
History:
- 09:07:2008 : Created by mieszko

*********************************************************************/
#ifndef _READABILITYSEQUENCE_H_
#define _READABILITYSEQUENCE_H_

#include "GenericTreeNode.h"


class CDagNode;


namespace Readabilities
{

class CSoundResourceSet;
class CReadabilitySequenceSet;
struct SSoundResource;

struct SSequenceNodeState
{	
	uint32	nBlocker;
	bool		bUsed;
	string	sResourceName;
	int			nResourceId;

	SSequenceNodeState()
		: bUsed( false ), sResourceName( "" ), nResourceId( -1 ), nBlocker( 0 )
	{}

	bool IsBlocked() const;
	void Reset();
};


class CReadabilitySequence
{

public:
	/*$1- Basics -------------------------------------------------------------*/
	typedef TGenericTreeNode<SSequenceNodeState> tSequenceNode;

	/*$1- Basics -------------------------------------------------------------*/
	CReadabilitySequence( CSoundResourceSet* pResourceSet = NULL );
	CReadabilitySequence( const CReadabilitySequence& );
	~CReadabilitySequence();

	void Reset();
	bool LoadFromDagNode( CDagNode* pNode );

	// Play control
	const SSoundResource*	GetNext( tSequenceNode*& pCurrentNode ) const;

	/*$1- Util ---------------------------------------------------------------*/
	
	tSequenceNode* GetRoot();
	const tSequenceNode* GetRoot() const;

	void	Block( uint32 nSessionId, bool bBlock );
	bool	IsBlocked()	const;
	bool	IsBlockedBy( uint32 nBlocker )	const;

	int		GetRequiredActorsCount() const;
	
	void	SetParentSet( CReadabilitySequenceSet* pParentSet );
	CReadabilitySequenceSet*	GetParentSet() const;

	/*$1- Managing -----------------------------------------------------------*/
	//void	Set( const CReadabilitySequence& );	// purposefully separated from copying constructor.
	void	SetPoolSize( uint32 );
	void	SetSoundResourceSet( CSoundResourceSet* pSoundResourceSet );
	CSoundResourceSet* GetSoundResourceSet() const;

	tSequenceNode* AddOption( tSequenceNode* pNode, const string& sOptionName );

	bool	CheckPlayability();
	
private:

	/*$1- Helpers ------------------------------------------------------------*/
	void	BlockNodeAndChildren( tSequenceNode* pNode, uint32 nSessionId, bool bBlock );
	void	CopyDagData( tSequenceNode* pTreeNode, CDagNode* pDagNode );
	bool	CheckNodePlayability( const tSequenceNode* pCurrentNode) const;

	/*$1- Members ------------------------------------------------------------*/
	tSequenceNode*	m_pRoot;
	bool						m_bPlayable;
	tSequenceNode*	m_pNodePool;
	CSoundResourceSet*				m_pResourceSet;
	CReadabilitySequenceSet*	m_pParentSet;
	uint32					m_nFirstFreeInPool;
	uint32					m_nPoolSize;
	int							m_nRequiredActors;

	/*$1- Static debug data --------------------------------------------------*/
};

// ----------------------------------------------------------------
// Inline CReadabilitySequence methods definition
// ----------------------------------------------------------------

inline bool SSequenceNodeState::IsBlocked() const	
{ 
	return nBlocker > 0; 
}

inline void SSequenceNodeState::Reset() 
{ 
	nBlocker = 0;	bUsed = false; 
}

// ----------------------------------------------------------------
// Inline CReadabilitySequence methods definition
// ----------------------------------------------------------------

inline CReadabilitySequence::tSequenceNode* CReadabilitySequence::GetRoot()
{
	return m_pRoot;
}

inline const CReadabilitySequence::tSequenceNode* CReadabilitySequence::GetRoot() const
{
	return m_pRoot;
}

inline bool CReadabilitySequence::IsBlocked()	const
{
	return m_pRoot->GetData().nBlocker > 0;
}

inline bool CReadabilitySequence::IsBlockedBy( uint32 nBlocker )	const
{
	return m_pRoot->GetData().nBlocker == nBlocker;
}

inline void CReadabilitySequence::SetSoundResourceSet( CSoundResourceSet* pSoundResourceSet )
{
	m_pResourceSet = pSoundResourceSet;
}

inline int CReadabilitySequence::GetRequiredActorsCount() const
{
	return m_nRequiredActors;
}

inline void CReadabilitySequence::SetParentSet( CReadabilitySequenceSet* pParentSet )
{
	m_pParentSet = pParentSet;
}

inline CReadabilitySequenceSet* CReadabilitySequence::GetParentSet() const
{
	return m_pParentSet;
}

inline CSoundResourceSet* CReadabilitySequence::GetSoundResourceSet() const
{
	return m_pResourceSet;
}

} // namespace Readabilities

#endif // _READABILITYSEQUENCE_H_