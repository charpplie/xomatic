/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   dag_node.cpp
$Id$
$DateTime$
Description: Small class to manage a single DAG tree node. It is
expected that you hinerit from this class to use your specific
implementation, as Behavior Tree code does.
---------------------------------------------------------------------
History:
- 02:06:2007 : Created by Ricardo Pillosu

*********************************************************************/
#include "StdAfx.h"
#include "dag_node.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CDagNode::CDagNode( CDagNode* pParent, const char* sName )
{
	assert( sName != NULL );

	m_pParent = pParent;
	m_sName = sName;
}

// Description:
//
// Arguments:
//
// Return:
//
CDagNode::~CDagNode()
{
	DestroyAllChilds();
}

// Description:
//
// Arguments:
//
// Return:
//
CDagNode* CDagNode::AddChild( const char* sName )
{
	assert( sName != NULL );

	CDagNode*		pNew = new CDagNode( this, sName );
	m_vecChilds.push_back( pNew );

	return( pNew );
}

// Description:
//
// Arguments:
//
// Return:
//
void CDagNode::DestroyAllChilds()
{
	for( uint32 uIndex = 0; uIndex < m_vecChilds.size(); ++uIndex )
	{
		SAFE_DELETE( m_vecChilds[uIndex] );
	}

	m_vecChilds.clear();
}

// Description:
//
// Arguments:
//
// Return:
//
CDagNode* CDagNode::SearchNode( const char* sName, bool bRecursively )
{
	CDagNode*		pRet = NULL;

	if( m_sName.compareNoCase(sName) == 0 )
	{
		pRet = this;
	}
	else
	{
		std::vector<CDagNode *>::iterator itC = m_vecChilds.begin();
		std::vector<CDagNode *>::iterator itEnd = m_vecChilds.end();

		for( ; itC != itEnd && pRet == NULL; ++itC )
		{
			if( bRecursively == true )
			{
				pRet = ( *itC )->SearchNode( sName );
			}
			else if( (*itC)->m_sName.compareNoCase(sName) == 0 )
			{
				pRet = ( *itC );
			}
		}
	}

	return( pRet );
}

// Description:
//
// Arguments:
//
// Return:
//
const char* CDagNode::GetName() const
{
	return( m_sName );
}

// Description:
//
// Arguments:
//
// Return:
//
void CDagNode::SetName( const char* sName )
{
	assert( sName != NULL );

	m_sName = sName;
	m_sName.Trim();
}

// Description:
//
// Arguments:
//
// Return:
//
const char* CDagNode::GetDebugText() const
{
	return( GetName() );
}

// Description:
//
// Arguments:
//
// Return:
//
CDagNode* CDagNode::GetParent()
{
	return( m_pParent );
}

// Description:
//
// Arguments:
//
// Return:
//
CDagNode const* CDagNode::GetParent() const
{
	return( m_pParent );
}


// Description:
//	Returns number of children. See 'Return' label.
// Arguments:
//	bDepthOfOne	- controls output of the method. See 'Return' label.
// Return:
//	depending on bDepthOfOne returns number of direct children (ie the size of children 
//	container) if it's true, or the size of the whole tree starting form this node
//	as a root
uint32 CDagNode::GetChildCount( bool bDepthOfOne )const
{
	uint32 uChildCount = m_vecChilds.size();

	// if we want childrens' children count 
	if( bDepthOfOne == false )
	{
		// iterate through children and add their children count
		std::vector<CDagNode*>::const_iterator itChild = m_vecChilds.begin();
		std::vector<CDagNode*>::const_iterator itEnd = m_vecChilds.end();
		for( ; itChild != itEnd; ++itChild )
		{
			uChildCount += (*itChild)->GetChildCount( false );
		}
	}
	
	return uChildCount;
}