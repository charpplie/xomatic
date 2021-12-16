/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   dag.cpp
$Id$
$DateTime$
Description: Base class for a generic DAG (Directed Acyclic Graph)
In computer science and mathematics, a directed acyclic graph, also 
called a DAG, is a directed graph with no directed cycles; that is,
for any vertex v, there is no nonempty directed path that starts and
ends on v. DAGs appear in models where it doesn't make sense for a
vertex to have a path to itself; for example, if an edge u->v
indicates that v is a part of u, such a path would indicate that u
is a part of itself, which is impossible. Informally speaking, a DAG
"flows" in a single direction.
---------------------------------------------------------------------
History:
- 02:06:2007 : Created by Ricardo Pillosu
- 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/
#include "StdAfx.h"
#include "dag.h"
#include "dag_node.h"
#include "DebugDrawContext.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CDag::CDag( const char* sRootName ) : m_bInfoLoaded( false ), m_bSkipEmptyRows( false )
{
	assert( sRootName != NULL );

	m_pRoot = new CDagNode( NULL, sRootName );
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CDag::CDag() : m_bInfoLoaded( false ), m_pRoot( NULL ), m_bSkipEmptyRows( false )
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CDag::~CDag()
{
	SAFE_DELETE( m_pRoot );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void CDag::Flush()
{
	GetRoot()->DestroyAllChilds();
	m_bInfoLoaded = false;
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CDag::ReLoad()
{
	return( ReLoad(m_sConfFile) );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CDag::ReLoad( const string& sFile )
{
	bool				bRet = false;
	XmlNodeRef	xmlFile = GetISystem()->LoadXmlFile( sFile );

	if( xmlFile )
	{
		Flush();
		m_sConfFile = sFile;

		for( int iSheet = 0; iSheet < xmlFile->getChildCount(); ++iSheet )
		{
			XmlNodeRef	xmlSheet = xmlFile->getChild( iSheet );

			if( xmlSheet->isTag("Worksheet") == false )
			{
				continue;
			}

			if( xmlSheet )
			{
				XmlNodeRef	xmlTable = xmlSheet->findChild( "Table" );

				if( xmlTable )
				{
					uint32		uRows = 0;
					uint32		uColumns = 0;
					string	sSheetName;

					sSheetName = xmlSheet->getAttr( "ss:Name" );
					xmlTable->getAttr( "ss:ExpandedRowCount", uRows );
					xmlTable->getAttr( "ss:ExpandedColumnCount", uColumns );

					// BUG: This is just checking if the last one is true. The value is overwritten by the last one, and the first results are discarded. /Jonas & Mario
					bRet = LoadSheet( sSheetName, uRows, uColumns, xmlTable );
				}
			}
		}

		LoadingFinished();
	}

	return( bRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void CDag::LoadingFinished()
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CDag::LoadSheet( const string& sName, uint32 uRows, uint32 uColumns, XmlNodeRef& xmlTable )
{
	bool	bRet = false;

	if( sName.compareNoCase("dag") == 0 )
	{
		m_Grid.Allocate( uRows, uColumns );

		int iRealRow = 0;
		for( int iRow = 0; iRow < xmlTable->getChildCount(); ++iRow )
		{
			XmlNodeRef	xmlRow = xmlTable->getChild( iRow );

			if( xmlRow->isTag("Row") == true )
			{
				bRet = LoadRow( xmlRow, iRealRow++ );
			}
		}

		m_bInfoLoaded = true;
	}

	return( bRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CDag::LoadRow( XmlNodeRef& xmlTable, int iRow )
{
	bool	bRet = false;

	if( m_bSkipEmptyRows == true && IsRowEmpty( xmlTable ) == true )
	{
		bRet = true;
	}
	else
	{
		int		iColumn = 0;
		for( int iCell = 0; iCell < xmlTable->getChildCount(); ++iCell )
		{
			XmlNodeRef	xmlCell = xmlTable->getChild( iCell );
			XmlNodeRef	xmlCellData = xmlCell->findChild( "Data" );

			if( xmlCell->isTag("Cell") == true && xmlCellData )
			{
				if( xmlCell->getAttr("ss:Index", iColumn) )
				{
					--iColumn;	// excel starts counting from 1, we from 0
				}

				bRet = LoadNode( iRow, iColumn, xmlCellData->getContent() );
				++iColumn;
			}
		}
	}

	return( bRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CDag::LoadNode( int iRow, int iColumn, const char* sName )
{
	bool				bRet = false;

	CDagNode*		pParent = FindParent( iRow, iColumn );

	if( pParent == NULL )
	{
		GetRoot()->SetName( sName );
		m_Grid.Set( iRow, iColumn, GetRoot() );
		bRet = true;
	}
	else
	{
		m_Grid.Set( iRow, iColumn, pParent->AddChild(sName) );
		bRet = true;
	}

	return( bRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CDagNode* CDag::FindParent( int iRow, int iColumn )
{
	CDagNode*		pRet = NULL;

	if( iColumn == 1 )
	{
		pRet = GetRoot();
	}
	else if( iColumn > 1 )
	{
		int offset = iRow;
		while( pRet == NULL )
		{
			pRet = m_Grid.Get( offset--, iColumn - 1 );
		}
	}

	return( pRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CDag::AddNode( CDagNode* pParent, const char* sName )
{
	assert( pParent != NULL );
	assert( sName != NULL );

	bool	bRet = false;

	if( pParent != NULL )
	{
		pParent->AddChild( sName );
		bRet = true;
	}

	return( bRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CDag::DestroyNode( CDagNode* pNode )
{
	assert( pNode != NULL );

	bool	bRet = false;

	if( pNode != NULL )
	{
		if( pNode == GetRoot() )
		{
			Flush();
		}
		else
		{
			SAFE_DELETE( pNode );
		}

		bRet = true;
	}

	return( bRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CDagNode* CDag::GetNode( const char* sName, const char* sParent, bool bRecursively )
{
	CDagNode*		pRet = GetRoot();

	if( sParent != NULL && strlen(sParent) > 0 )
	{
		int			iPos = 0;
		string	sBranch( sParent );
		string	sCurrentBranch = sBranch.Tokenize( ".", iPos );

		while( sCurrentBranch.empty() == false && pRet != NULL )
		{
			pRet = pRet->SearchNode( sCurrentBranch, bRecursively );
			sCurrentBranch = sBranch.Tokenize( ".", iPos );
		}
	}

	pRet = pRet->SearchNode( sName, bRecursively );
	assert( pRet != NULL );

	return( pRet );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CDagNode* CDag::GetRoot() const
{
	return( m_pRoot );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void CDag::DebugDraw() const
{
	for( uint32 uRow = 0; uRow < m_Grid.GetRows(); ++uRow )
	{
		for( uint32 uCol = 0; uCol < m_Grid.GetColumns(); ++uCol )
		{
			if( m_Grid.Get(uRow, uCol) != NULL )
			{
				float xPos = (float)(( uCol + 1 ) * 130);
				float yPos = (float)(15 + ( (uRow + 1) * 15 ));
				DebugDrawNode( xPos, yPos, m_Grid.Get(uRow, uCol) );
			}
		}
	}
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool CDag::IsInfoLoaded() const
{
	return( m_bInfoLoaded );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void CDag::DebugDrawNode( float xPos, float yPos, const CDagNode* pNode ) const
{
	CDebugDrawContext dc;
	dc->Draw2dLabel( xPos, yPos, 1.2f, ColorB(255, 255, 255), false, "%s", pNode->GetDebugText() );
}


// Description:
//	Tells instance to skip (or not) empty rows while reading in data from xmlTables. 
//	Default if 'false' (i.e. not skipping) which may result in 'failing' reading in data 
//	just because an empty row was encountered (typically at the end of the xml table).
// Arguments:
//	bSkip	- determines this instance's new skipping empty rows property value
// Return:
//	void
void CDag::SetSkipEmptyRows( bool bSkip )
{
	m_bSkipEmptyRows = bSkip;
}

// Description:
//	
// Arguments:
//
// Return:
//	If given table is empty
bool CDag::IsRowEmpty( XmlNodeRef& xmlRow ) const
{
	bool bEmpty = true;

	int nChildCount = xmlRow->getChildCount();

	for( int iCell = 0; iCell < nChildCount && bEmpty == true; ++iCell )
	{
		XmlNodeRef	xmlCell = xmlRow->getChild( iCell );
		if( xmlCell->isTag("Cell") == true )
		{	
			// if it's a cell check if there's any data
			bEmpty &= !(xmlCell->findChild( "Data" ));
		}		
	}

	return bEmpty;
}