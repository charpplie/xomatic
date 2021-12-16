/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   ReadabilitySequence.cpp
Description: 
---------------------------------------------------------------------
History:
- 09:07:2008 : Created by mieszko

*********************************************************************/
#include "StdAfx.h"

#include "ReadabilitySequence.h"
#include "SoundResourceSet.h"

#include "BehaviorTree/dag_node.h"

using namespace Readabilities;

typedef CReadabilitySequence::tSequenceNode tSequenceNode;

// ------------------------------------------------------------
// Description:
//   Constructor
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
CReadabilitySequence::CReadabilitySequence( CSoundResourceSet* pResourceSet )
: m_pNodePool( NULL )
, m_pRoot( NULL )
, m_bPlayable( true )
, m_nPoolSize( 0 )
, m_nFirstFreeInPool( 0 )
, m_pResourceSet( pResourceSet )
, m_nRequiredActors( 2 ) // set only in constructor
, m_pParentSet( NULL )
{
}

CReadabilitySequence::CReadabilitySequence( const CReadabilitySequence& src )
: m_pNodePool( NULL )
, m_pRoot( NULL )
, m_bPlayable( true )
, m_nPoolSize( 0 )
, m_nFirstFreeInPool( 0 )
, m_pResourceSet( src.m_pResourceSet )
, m_nRequiredActors( 2 ) // set only in constructor
, m_pParentSet( NULL )
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
CReadabilitySequence::~CReadabilitySequence()
{
	SAFE_DELETE_ARRAY( m_pNodePool );
}

// ------------------------------------------------------------
// Description:
//	Clears 'nBlocker' and 'bUsed' values for every tree node in
//	sequence.
// ------------------------------------------------------------
void CReadabilitySequence::Reset()
{
	if( m_pNodePool == NULL )
	{
		return;
	}

	// stop this sequence if it's being played at this moment
	if( IsBlocked() == true )
	{
		//Stop();
		assert( false && "Need to stop playing current sounds!" );
	}

	// reset data marks
	tSequenceNode*	pNode = m_pNodePool;
	for( uint32 i = 0; i < m_nPoolSize; ++i, ++pNode )
	{
		pNode->GetData().Reset();
	}
	m_bPlayable = true;
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
bool CReadabilitySequence::LoadFromDagNode( CDagNode* pNode )
{
	uint32 uTreeSize = pNode->GetChildCount( false ) + 1;
	bool bRet = false;

	if( uTreeSize > 0 )
	{
		SetPoolSize( uTreeSize );

		CopyDagData( NULL, pNode ); // 

		bRet = true;
	}

	return bRet;
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
const SSoundResource*	CReadabilitySequence::GetNext( tSequenceNode*& pCurrentNode ) const
{
	const SSoundResource* pSoundRes = NULL; 

	if( pCurrentNode == NULL )
	{
		pCurrentNode = m_pRoot;
		pSoundRes = m_pResourceSet->GetResource( pCurrentNode->GetData().sResourceName );
	}
	else
	{
		// NOTE: naive implementation!
		int nChildCount = pCurrentNode->GetChildrenCount();

		if( nChildCount == 1 )
		{	
			// check playability of the only child
			bool bPlayable = CheckNodePlayability( pCurrentNode->GetFirstChild() );

			// if it's playable no more checks need to be done
			if( bPlayable == true )
			{
				// get the only child node using the shortcut
				pCurrentNode = pCurrentNode->GetFirstChild();

				// get resource associated with it
				pSoundRes = m_pResourceSet->GetResource( pCurrentNode->GetData().sResourceName );
			}
		}
		else if( nChildCount > 1 ) //i.e. not 0
		{
			int* aAvailableIndexes = new int[ nChildCount ];
			int* pIndex = aAvailableIndexes;
			int	nAvailableCount = 0;

			// put indexes of all free children (i.e. with not used resources) into array
			tSequenceNode::tChildrenConstIterator itChild = pCurrentNode->GetChildrenIterator();
			tSequenceNode::tChildrenConstIterator itEnd = pCurrentNode->GetChildrenEndIterator();
			for( int i = 0; itChild != itEnd; ++itChild, ++i )
			{		
				// check playability of every child
				if( CheckNodePlayability( *itChild )  == true )
				{
					*pIndex = i;
					++pIndex;
					++nAvailableCount;
				}
			}

			// if there ar any available nodes
			if( nAvailableCount > 0 )
			{
				// pick a random from availables
				pCurrentNode = pCurrentNode->GetChild( aAvailableIndexes[ cry_rand() % nAvailableCount ] );
				pSoundRes = m_pResourceSet->GetResource( pCurrentNode->GetData().sResourceName );
			}

			// don't forget to release temporary index array
			delete[] aAvailableIndexes;
		}
		// else, for 0, it's still NULL
	}

	return pSoundRes;
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
void CReadabilitySequence::Block( uint32 nSessionId, bool bBlock )
{
	BlockNodeAndChildren( m_pRoot, nSessionId, bBlock );
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
void CReadabilitySequence::SetPoolSize( uint32 nNewSize )
{
	assert( nNewSize > 0 );

	SAFE_DELETE_ARRAY( m_pNodePool );

	m_pNodePool = new tSequenceNode[ nNewSize ];
	m_nPoolSize = nNewSize;
	m_pRoot = m_pNodePool;
	m_nFirstFreeInPool = 1;	// index 0 is for root pointer
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
tSequenceNode* CReadabilitySequence::AddOption( tSequenceNode* pNode, const string& sOptionName )
{
	assert( m_pNodePool != NULL );

	tSequenceNode* pRetNode = NULL;

	if( pNode != NULL ) // otherwise it's the root
	{
		pRetNode = &m_pNodePool[ m_nFirstFreeInPool ];
		++m_nFirstFreeInPool;
	
		pNode->AddChild( pRetNode );
	}
	else
	{
		pRetNode = m_pRoot;
	}

	if( pRetNode != NULL )
	{
		pRetNode->GetData().sResourceName = sOptionName;
	}

	return pRetNode;
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
void CReadabilitySequence::BlockNodeAndChildren( tSequenceNode* pNode, uint32 nSessionId, bool bBlock )
{	
	// sanity check
	if( pNode == NULL )
	{
		GetAISystem()->Warning("<CReadabilitySequence::BlockNodeAndChildren> ","Trying to block NULL node!" );
		return;
	}
	// do nothing if trying to block and node already blocked
	else if( bBlock && pNode->GetData().IsBlocked() == true )
	{
		GetAISystem()->Warning("<CReadabilitySequence::BlockNodeAndChildren> ","Trying to block already blocked sequence node" );
		return;
	}	

	// block the node itself
	pNode->GetData().nBlocker = bBlock ? nSessionId : 0;
	
	if( m_pResourceSet != NULL )
	{
		// block the resource in resource set
		m_pResourceSet->SetBlockResource( nSessionId, pNode->GetData().sResourceName, bBlock);

		// and if there are any children
		if( pNode->GetChildrenCount() > 0 )
		{
			// do the same for them
			tSequenceNode::tChildrenContainer::iterator itChild = pNode->GetChildrenIterator();
			tSequenceNode::tChildrenContainer::const_iterator itEnd = pNode->GetChildrenEndIterator();

			for( ; itChild != itEnd; ++itChild )
			{
				BlockNodeAndChildren( *itChild, nSessionId, bBlock );
			}
		}
	}
}

// ------------------------------------------------------------
// Description:
//   
// Arguments:
//
// Return:
//
// ------------------------------------------------------------
void CReadabilitySequence::CopyDagData( tSequenceNode* pTreeNode, CDagNode* pDagNode )
{
	if( pDagNode != NULL && strlen(pDagNode->GetName()) > 0)
	{	
		tSequenceNode* pNewTreeNode = AddOption( pTreeNode, pDagNode->GetName() );

		uint32 nChildCount = pDagNode->GetChildCount();
		pNewTreeNode->PresetChildrenCount( nChildCount );
		for( uint32 i = 0; i < nChildCount; ++i )
		{
			CopyDagData( pNewTreeNode, pDagNode->GetChild( i ) );
		}
	}
}

// ------------------------------------------------------------
// Description:
//		Checks if there's at least one path of not used (i.e. available) sound 
//		resources in this sequence. If this test is failed its result is cashed
//		and kept until sequence is 'reseted' (see CReadabilitySequence::Reset)
// Return:
//		result of the check
// ------------------------------------------------------------
bool CReadabilitySequence::CheckPlayability()
{
	bool bRet = false;

	// if this sequence haven't failed this test yet
	if( m_bPlayable == true )
	{
		// do the test
		bRet = CheckNodePlayability( m_pRoot );
		// and remember the result
		m_bPlayable = bRet;
	}

	return bRet;
}

// ------------------------------------------------------------
// Description:
//		CheckPlayability helper method. Recursively checks given node
//		for its resources availability (i.e. if they are not marked as 'used')
// Arguments:
//		pNode	- node to check
// Return:
//		result of the check
// ------------------------------------------------------------
bool CReadabilitySequence::CheckNodePlayability( const tSequenceNode* pCurrentNode) const
{	
	assert( pCurrentNode != NULL && "Checking playability of NULL sequence node!" );
	// sanity check
	if( m_pResourceSet == NULL || pCurrentNode == NULL )
	{
		return false;
	}

	// get resource associated with given node
	const SSoundResource* pTmpSoundRes = m_pResourceSet->GetResource( pCurrentNode->GetData().sResourceName );
	// check if it's available
	bool bThisNode = ( pTmpSoundRes->IsUsed() == false );
	uint32 nChildCount = pCurrentNode->GetChildrenCount();
	bool bChildNodes = ( nChildCount == 0 );	// if no children then this node's playability doesn't depend on it

	// if this node is available, check the children
	if( bThisNode == true && nChildCount > 0 )
	{
		if( nChildCount == 1 )
		{
			// fast way for one child
			bChildNodes = CheckNodePlayability( pCurrentNode->GetFirstChild() );
		}
		else
		{
			tSequenceNode::tChildrenConstIterator itChild = pCurrentNode->GetChildrenIterator();
			tSequenceNode::tChildrenConstIterator itEnd = pCurrentNode->GetChildrenEndIterator();

			// do it until you reach the end or you find the first playable child
			for( ; itChild != itEnd && bChildNodes == false; ++ itChild )
			{
				bChildNodes |= CheckNodePlayability( *itChild );	// could be assignment as well. |= left for readability
			}
		}
	}

	return bThisNode && bChildNodes;
}