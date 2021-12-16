////////////////////////////////////////////////////////////////////////////
//
//  Crytek Source File.
//  Copyright (C), Crytek Studios, 1999-2010.
// -------------------------------------------------------------------------
//  File Name        : XmlSplitStitch.h
//  Version          : v1.00
//  Created          : 8/4/2010 by Pau Novau
//  Description      : Helper class to split and stitch together xml files based on
//                     an index file.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////
#ifndef __Xml_Split_Stitch__h__
#define __Xml_Split_Stitch__h__


class CXmlSplitStitchEntry
{
public:
	CXmlSplitStitchEntry();

	void SetFilename( const string& filename );
	const string& GetFilename() const;

	XmlNodeRef GetPath() const;
	void SetPath( XmlNodeRef xmlPath );

	int GetPathDepth() const;

private:
	string m_filename;
	XmlNodeRef m_xmlPath;
	int m_xmlPathDepth;
};



class CXmlSplitStitch
{
public:
	CXmlSplitStitch();

	void SetBasePath( const char* path );

	void LoadIndex( const char* indexFilename );
	void SaveIndex( const char* indexFilename );

	void Save( XmlNodeRef mergedData );
	XmlNodeRef Load();

	static int GetIndexVersion( const char* indexFilename );

private:
	void LoadIndexEntry( XmlNodeRef entryXml );
	void LoadEntry( XmlNodeRef& mergedResult, const CXmlSplitStitchEntry& entry );
	XmlNodeRef FindOrCreateNode( XmlNodeRef node, XmlNodeRef path );
	XmlNodeRef FindOrCreateNodeRec( XmlNodeRef node, XmlNodeRef path );
	XmlNodeRef FindChildMatchingNode( XmlNodeRef node, XmlNodeRef referneceNode );

	void SaveRec( XmlNodeRef node );

	void AddEntry( const CXmlSplitStitchEntry& entry );
private:
	std::vector< CXmlSplitStitchEntry > m_entries;

	string m_path;
};

#endif