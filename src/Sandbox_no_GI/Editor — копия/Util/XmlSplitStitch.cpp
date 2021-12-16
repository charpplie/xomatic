////////////////////////////////////////////////////////////////////////////
//
//  Crytek Source File.
//  Copyright (C), Crytek Studios, 1999-2010.
// -------------------------------------------------------------------------
//  File Name        : XmlSplitStitch.cpp
//  Version          : v1.00
//  Created          : 8/4/2010 by Pau Novau
//  Description      : Helper class to split and stitch together xml files based on
//                     an index file.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "XmlSplitStitch.h"

CXmlSplitStitchEntry::CXmlSplitStitchEntry()
: m_xmlPathDepth( 0 )
{
}

void CXmlSplitStitchEntry::SetFilename( const string& filename )
{
	m_filename = filename;
}

const string& CXmlSplitStitchEntry::GetFilename() const
{
	return m_filename;
}

XmlNodeRef CXmlSplitStitchEntry::GetPath() const
{
	return m_xmlPath;
}

void CXmlSplitStitchEntry::SetPath( XmlNodeRef xmlPath )
{
	m_xmlPath = xmlPath;

	m_xmlPathDepth = 0;
	if ( ! m_xmlPath )
	{
		return;
	}

	m_xmlPathDepth = 1;
	XmlNodeRef currentXmlNode = m_xmlPath;
	while ( currentXmlNode->getChildCount() != 0 )
	{
		if ( currentXmlNode->getChildCount() != 1 )
		{
			// TODO: Log error and continue
		}
		currentXmlNode = currentXmlNode->getChild( 0 );
		m_xmlPathDepth++;
	}
}


int CXmlSplitStitchEntry::GetPathDepth() const
{
	return m_xmlPathDepth;
}




//////////////////////////////////////////////////////////////////////////
namespace
{
	bool NodesMatch( XmlNodeRef node, XmlNodeRef referenceNode )
	{
		if ( ! node )
		{
			return false;
		}

		if ( ! referenceNode )
		{
			return false;
		}

		if ( stricmp( node->getTag(), referenceNode->getTag() ) != 0 )
		{
			return false;
		}

		for ( int i = 0; i < referenceNode->getNumAttributes(); ++i )
		{
			const char* referenceAttrKey;
			const char* referenceAttrValue;
			referenceNode->getAttributeByIndex( i, &referenceAttrKey, &referenceAttrValue );

			const char* attrValue;

			bool attributeMatches = true;
			attributeMatches &= node->getAttr( referenceAttrKey, &attrValue );
			attributeMatches &= ( stricmp( referenceAttrValue, attrValue ) == 0 );

			if ( ! attributeMatches )
			{
				return false;
			}
		}

		return true;
	}
}



CXmlSplitStitch::CXmlSplitStitch()
{

}

void CXmlSplitStitch::SetBasePath( const char* path )
{
	m_path = path;
}

int CXmlSplitStitch::GetIndexVersion( const char* indexFilename )
{
	XmlNodeRef index = GetISystem()->LoadXmlFromFile( indexFilename );
	if ( ! index )
	{
		return 0;
	}

	int version = 0;
	bool hasVersion = index->getAttr( "version", version );
	if ( ! hasVersion )
	{
		return 0;
	}

	return version;
}

void CXmlSplitStitch::Save( XmlNodeRef data )
{
	XmlNodeRef clonedData = data->clone();

	SaveRec( clonedData );
}


struct XmlStructureIndexerEntryFindPredicate
{
	XmlNodeRef m_node;
	XmlStructureIndexerEntryFindPredicate( XmlNodeRef node )
	{
		m_node = node;
	}

	bool operator () ( const CXmlSplitStitchEntry& entry ) const
	{
		XmlNodeRef node = m_node;
		std::vector< XmlNodeRef > entryPathHierarchy;
		entryPathHierarchy.reserve( entry.GetPathDepth() );
		FillChildrenHierarchy( entryPathHierarchy, entry.GetPath() );

		// TODO: remove this special case
		if ( entryPathHierarchy.size() == 0 && ! node->getParent() )
		{
			// special case with root node and root file treated separately so that empty
			// path doesn't match all ( since we aren't searching in the most restrictive first, which would solve the issue! )
			return true;
		}

		std::vector< XmlNodeRef > nodeHierarchy;
		nodeHierarchy.reserve( entry.GetPathDepth() );
		FillParentHierarchy( nodeHierarchy, node );

		bool depthEqual = ( nodeHierarchy.size() == entryPathHierarchy.size() );
		if ( ! depthEqual )
		{
			return false;
		}

		for ( int i = 0; i < entryPathHierarchy.size(); i++ )
		{
			XmlNodeRef currentNodeReference = entryPathHierarchy[ i ];
			XmlNodeRef currentNode = nodeHierarchy[ i ];

			bool nodesMatch = NodesMatch( currentNode, currentNodeReference );
			if ( ! nodesMatch )
			{
				return false;
			}
		}
		
		return true;
	}

	void FillChildrenHierarchy( std::vector< XmlNodeRef >& hirerarchyOut, XmlNodeRef pathNode ) const
	{
		if ( ! pathNode )
		{
			return;
		}

		if ( pathNode->getChildCount() != 0 )
		{
			XmlNodeRef childPathNode = pathNode->getChild( 0 );
			FillChildrenHierarchy( hirerarchyOut, childPathNode );
		}
		
		hirerarchyOut.push_back( pathNode );
	}

	void FillParentHierarchy( std::vector< XmlNodeRef >& hirerarchyOut, XmlNodeRef node ) const
	{
		while( node )
		{
			hirerarchyOut.push_back( node );
			node = node->getParent();
		}
	}
};

void CXmlSplitStitch::SaveRec( XmlNodeRef node )
{
	if ( ! node )
	{
		return;
	}

	// Temp vector used because child structure may change if removing a child in SaveRec
	std::vector< XmlNodeRef > nodeChildren( node->getChildCount() );
	for ( int i = 0; i < node->getChildCount(); ++i )
	{
		XmlNodeRef childNode = node->getChild( i );
		nodeChildren[ i ] = childNode;
	}

	for ( int i = 0; i < nodeChildren.size(); ++i )
	{
		XmlNodeRef childNode = nodeChildren[ i ];
		SaveRec( childNode );
	}

	std::vector< CXmlSplitStitchEntry >::const_iterator cit = std::find_if( m_entries.begin(), m_entries.end(), XmlStructureIndexerEntryFindPredicate( node ) );

	bool matchEntryFound = ( cit != m_entries.end() );
	if ( matchEntryFound )
	{
		const CXmlSplitStitchEntry& matchEntry = *cit;
		XmlNodeRef parentNode = node->getParent();
		if ( parentNode )
		{
			parentNode->removeChild( node );
		}

		string filename = m_path + matchEntry.GetFilename();
		node->saveToFile( filename );
	}
}

struct XmlStructureIndexerEntrySortPredicate
{
	bool operator () ( const CXmlSplitStitchEntry& lhs, const CXmlSplitStitchEntry& rhs ) const
	{
		// TODO: restrictiveness doesn't account for attributes
		// This can cause unforseen behaviours if using attributes that aren't key ( as in database keys ) attributes.
		return ( lhs.GetPathDepth() < rhs.GetPathDepth() );
	}
};

XmlNodeRef CXmlSplitStitch::Load()
{
	std::sort( m_entries.begin(), m_entries.end(), XmlStructureIndexerEntrySortPredicate() );

	XmlNodeRef mergedResult;
	for ( int i = 0; i < m_entries.size(); ++i )
	{
		const CXmlSplitStitchEntry& entry = m_entries[ i ];
		LoadEntry( mergedResult, entry );
	}

	return mergedResult;
}

void CXmlSplitStitch::LoadEntry( XmlNodeRef& mergedResult, const CXmlSplitStitchEntry& entry )
{
	const string& entryFilename = entry.GetFilename();
	XmlNodeRef currentNode = GetISystem()->LoadXmlFromFile( m_path + entryFilename );
	if ( ! currentNode )
	{
		// TODO: Log error
		return;
	}

	XmlNodeRef entryPath = entry.GetPath();
	XmlNodeRef mergedResultInsertionPoint = FindOrCreateNode( mergedResult, entryPath );
	assert( mergedResultInsertionPoint );
	
	if ( ! mergedResult )
	{
		mergedResult = mergedResultInsertionPoint;
		while ( mergedResult->getParent() )
		{
			mergedResult = mergedResult->getParent();
		}
	}

	mergedResultInsertionPoint->copyAttributes( currentNode );
	for ( int i = 0; i < currentNode->getChildCount(); ++i )
	{
		XmlNodeRef childNode = currentNode->getChild( i );
		mergedResultInsertionPoint->addChild( childNode );
	}
}

XmlNodeRef CXmlSplitStitch::FindOrCreateNodeRec( XmlNodeRef node, XmlNodeRef childPath )
{
	assert( node );
	assert( childPath );

	XmlNodeRef matchingChildNode = FindChildMatchingNode( node, childPath );
	if ( ! matchingChildNode )
	{
		string tag = childPath->getTag();
		matchingChildNode = GetISystem()->CreateXmlNode( tag );
		matchingChildNode->copyAttributes( childPath );
		node->addChild( matchingChildNode );
	}

	if ( childPath->getChildCount() == 0 )
	{
		return matchingChildNode;
	}

	return FindOrCreateNodeRec( matchingChildNode, childPath->getChild( 0 ) );
}

XmlNodeRef CXmlSplitStitch::FindOrCreateNode( XmlNodeRef node, XmlNodeRef path )
{
	assert( path );

	if ( ! NodesMatch( node, path ) )
	{
		string tag = path->getTag();
		node = GetISystem()->CreateXmlNode( tag );
		node->copyAttributes( path );
	}

	if ( path->getChildCount() == 0 )
	{
		return node;
	}

	XmlNodeRef matchingChild = FindOrCreateNodeRec( node, path->getChild( 0 ) );
	return matchingChild;	
}

XmlNodeRef CXmlSplitStitch::FindChildMatchingNode( XmlNodeRef node, XmlNodeRef referenceNode )
{
	if ( ! node )
	{
		return XmlNodeRef();
	}

	if ( ! referenceNode )
	{
		return node;
	}

	string tagToFind = referenceNode->getTag();

	for ( int i = 0; i < node->getChildCount(); ++i )
	{
		XmlNodeRef childNode = node->getChild( i );
		bool nodesMatch = NodesMatch( childNode, referenceNode );
		if ( nodesMatch )
		{
			return childNode;
		}
	}

	return XmlNodeRef();
}


void CXmlSplitStitch::SaveIndex( const char* indexFilename )
{
	// TODO: Implement!
	assert( false );
}

void CXmlSplitStitch::LoadIndex( const char* indexFilename )
{
	XmlNodeRef indexXml = GetISystem()->LoadXmlFromFile( indexFilename );
	if ( ! indexXml )
	{
		Log( "Index file '%s' not found.", indexFilename );
		return;
	}

	m_entries.clear();

	if ( indexXml->getChildCount() == 0 )
	{
		Log( "Index file '%s' is empty.", indexFilename );
		return;
	}
	
	m_entries.reserve( indexXml->getChildCount() );
	for ( int i = 0; i < indexXml->getChildCount(); ++i )
	{
		XmlNodeRef entryXml = indexXml->getChild( i );
		LoadIndexEntry( entryXml );
	}
}



void CXmlSplitStitch::LoadIndexEntry( XmlNodeRef entryXml )
{
	assert( entryXml );
	
	string entryFilename = entryXml->getAttr( "filename" );
	if ( entryFilename.empty() )
	{
		// TODO: log error
		return;
	}

	XmlNodeRef pathXml = entryXml->findChild( "path" );
	if ( ! pathXml )
	{
		// TODO: log error
		return;
	}

	if ( pathXml->getChildCount() == 0 )
	{
		pathXml = XmlNodeRef();
	}
	else
	{
		pathXml = pathXml->getChild( 0 );
	}

	CXmlSplitStitchEntry entry;
	entry.SetFilename( entryFilename );
	entry.SetPath( pathXml );

	AddEntry( entry );
}

void CXmlSplitStitch::AddEntry( const CXmlSplitStitchEntry& entry )
{
	// TODO: insert unique ( based on path )
	// TODO: check for unique filenames?
	// TODO: check that path is not empty?
	m_entries.push_back( entry );
}

