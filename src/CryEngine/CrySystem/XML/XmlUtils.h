////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2001-2006.
// -------------------------------------------------------------------------
//  File name:   XmlUtils.h
//  Created:     21/04/2006 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __XmlUtils_h__
#define __XmlUtils_h__
#pragma once

#include "ISystem.h"

class CXmlNodePool;

//////////////////////////////////////////////////////////////////////////
// Implements IXmlUtils interface.
//////////////////////////////////////////////////////////////////////////
class CXmlUtils : public IXmlUtils, public ISystemEventListener
{
public:
	CXmlUtils( ISystem *pSystem );
	virtual ~CXmlUtils();

	//////////////////////////////////////////////////////////////////////////
	// IXmlUtils
	//////////////////////////////////////////////////////////////////////////

	VIRTUAL IXmlParser* CreateXmlParser();

	// Load xml file, return 0 if load failed.
	VIRTUAL XmlNodeRef LoadXmlFile( const char *sFilename );
	// Load xml from string, return 0 if load failed.
	VIRTUAL XmlNodeRef LoadXmlFromString( const char *sXmlString );	

	// create an MD5 hash of an XML file
	VIRTUAL const char * HashXml( XmlNodeRef node );

	// Get an object that can read a xml into a IReadXMLSink 
	// and write a xml from a IWriteXMLSource
	VIRTUAL IReadWriteXMLSink* GetIReadWriteXMLSink();

	VIRTUAL IXmlSerializer* CreateXmlSerializer();

	VIRTUAL bool SaveBinaryXmlFile( const char *sFilename,XmlNodeRef root );
	VIRTUAL XmlNodeRef LoadBinaryXmlFile( const char *sFilename );

	VIRTUAL bool EnableBinaryXmlLoading( bool bEnable );
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// ISystemEventListener
	//////////////////////////////////////////////////////////////////////////
	virtual void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam );
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	virtual void GetMemoryUsage( ICrySizer *pSizer );

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// Init xml stats nodes pool
	VIRTUAL void InitStatsXmlNodePool( uint32 nPoolSize = 1024*1024 );

	// Creates new xml node for statistics.
	VIRTUAL XmlNodeRef CreateStatsXmlNode( const char *sNodeName="" );


private:
	ISystem *m_pSystem;
	IReadWriteXMLSink*    m_pReadWriteXMLSink;
	CXmlNodePool*					m_pStatsXmlNodePool;
};

#endif //__XmlUtils_h__
