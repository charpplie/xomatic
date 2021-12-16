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

#include <StdAfx.h>
#include <IXml.h>
#include "xml.h"
#include "XmlUtils.h"
#include "ReadWriteXMLSink.h"
#include "../DataProbe.h"

#include "../SimpleStringPool.h"
#include "SerializeXMLReader.h"
#include "SerializeXMLWriter.h"

#include "XMLBinaryWriter.h"
#include "XMLBinaryReader.h"

//////////////////////////////////////////////////////////////////////////
CXmlNode_PoolAlloc *g_pCXmlNode_PoolAlloc = 0;
#ifdef CRY_COLLECT_XML_NODE_STATS
SXmlNodeStats* g_pCXmlNode_Stats = 0;
#endif

extern bool g_bEnableBinaryXmlLoading;

//////////////////////////////////////////////////////////////////////////
CXmlUtils::CXmlUtils( ISystem *pSystem )
{
	m_pSystem = pSystem;
	m_pSystem->GetISystemEventDispatcher()->RegisterListener(this);

	// create IReadWriteXMLSink object
	m_pReadWriteXMLSink = new CReadWriteXMLSink();
	g_pCXmlNode_PoolAlloc = new CXmlNode_PoolAlloc;
#ifdef CRY_COLLECT_XML_NODE_STATS
	g_pCXmlNode_Stats = new SXmlNodeStats();
#endif
	m_pStatsXmlNodePool = 0;
}

//////////////////////////////////////////////////////////////////////////
CXmlUtils::~CXmlUtils()
{
	m_pSystem->GetISystemEventDispatcher()->RemoveListener(this);
	delete g_pCXmlNode_PoolAlloc;
#ifdef CRY_COLLECT_XML_NODE_STATS
	delete g_pCXmlNode_Stats;
#endif
	SAFE_DELETE(m_pStatsXmlNodePool);
}

//////////////////////////////////////////////////////////////////////////
IXmlParser* CXmlUtils::CreateXmlParser()
{
	return new XmlParser;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CXmlUtils::LoadXmlFile( const char *sFilename )
{
	XmlParser parser;
	XmlNodeRef node = parser.parse( sFilename );

	const char* pErrorString = parser.getErrorString();
	if (strcmp("", pErrorString) != 0)
	{
		CryLog("Error parsing <%s>: %s", sFilename, parser.getErrorString());
	}

	return node;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CXmlUtils::LoadXmlFromString( const char *sXmlString )
{
	XmlParser parser;
	XmlNodeRef node = parser.parseBuffer( sXmlString );
	return node;
}

//////////////////////////////////////////////////////////////////////////
const char* CXmlUtils::HashXml( XmlNodeRef node )
{
	static char signature[16*2 + 1];
	static char temp[16];
	static const char * hex = "0123456789abcdef";
	XmlString str = node->getXML();
	CDataProbe dp;
	dp.GetMD5( str.data(), str.length(), temp );
	for (int i=0; i<16; i++)
	{
		signature[2*i+0] = hex[((uint8)temp[i])>>4];
		signature[2*i+1] = hex[((uint8)temp[i])&0xf];
	}
	signature[16*2] = 0;
	return signature;
}

//////////////////////////////////////////////////////////////////////////
IReadWriteXMLSink* CXmlUtils::GetIReadWriteXMLSink()
{
	return m_pReadWriteXMLSink;
}

//////////////////////////////////////////////////////////////////////////
class CXmlSerializer : public IXmlSerializer
{
public:
	CXmlSerializer()
	{ 
		m_nRefCount = 0;
		m_pReaderImpl = 0;
		m_pReaderSer = 0;
		m_pWriterSer = 0;
		m_pWriterImpl = 0;
	}
	~CXmlSerializer()
	{
		ClearAll();
	}
	void ClearAll()
	{
		SAFE_DELETE(m_pReaderSer);
		SAFE_DELETE(m_pReaderImpl);
		SAFE_DELETE(m_pWriterSer);
		SAFE_DELETE(m_pWriterImpl);
	}
	
	//////////////////////////////////////////////////////////////////////////
	virtual void AddRef() { ++m_nRefCount; }
	virtual void Release() { if (--m_nRefCount <= 0) delete this; }

	VIRTUAL ISerialize* GetWriter( XmlNodeRef &node )
	{
		ClearAll();
		m_pWriterImpl = new CSerializeXMLWriterImpl(node);
		m_pWriterSer = new CSimpleSerializeWithDefaults<CSerializeXMLWriterImpl>( *m_pWriterImpl );
		return m_pWriterSer;
	}
	VIRTUAL ISerialize* GetReader( XmlNodeRef &node )
	{
		ClearAll();
		m_pReaderImpl = new CSerializeXMLReaderImpl(node);
		m_pReaderSer = new CSimpleSerializeWithDefaults<CSerializeXMLReaderImpl>( *m_pReaderImpl );
		return m_pReaderSer;
	}
	//////////////////////////////////////////////////////////////////////////
private:
	int m_nRefCount;
	CSerializeXMLReaderImpl *m_pReaderImpl;
	CSimpleSerializeWithDefaults<CSerializeXMLReaderImpl> *m_pReaderSer;

	CSerializeXMLWriterImpl *m_pWriterImpl;
	CSimpleSerializeWithDefaults<CSerializeXMLWriterImpl> *m_pWriterSer;
};

//////////////////////////////////////////////////////////////////////////
IXmlSerializer* CXmlUtils::CreateXmlSerializer()
{
	return new CXmlSerializer;
}

//////////////////////////////////////////////////////////////////////////
void CXmlUtils::GetMemoryUsage( ICrySizer *pSizer )
{
	{
		SIZER_COMPONENT_NAME(pSizer, "Nodes");
		pSizer->AddObject( g_pCXmlNode_PoolAlloc,g_pCXmlNode_PoolAlloc->GetTotalAllocatedMemory() );
	}
	{
		SIZER_COMPONENT_NAME(pSizer, "StringPool");
		pSizer->AddObject( &CSimpleStringPool::g_nTotalAllocInXmlStringPools,CSimpleStringPool::g_nTotalAllocInXmlStringPools );
	}

#ifdef CRY_COLLECT_XML_NODE_STATS
	// yes, slow
	std::vector<const CXmlNode*> rootNodes;
	{
		TXmlNodeSet::const_iterator iter = g_pCXmlNode_Stats->nodeSet.begin();
		TXmlNodeSet::const_iterator iterEnd = g_pCXmlNode_Stats->nodeSet.end();
		while (iter != iterEnd)
		{
			const CXmlNode* pNode = *iter;
			if (pNode->getParent() == 0)
			{
				rootNodes.push_back(pNode);
			}
			++iter;
		}
	}

	// use the following to log to console







	// use the following to debug the nodes in the system 













	// only for debugging, add it as pseudo numbers to the CrySizer.
	// shift it by 10, so we get the actual number
	{
		SIZER_COMPONENT_NAME(pSizer, "#NumTotalNodes");
		pSizer->Add("#NumTotalNodes", g_pCXmlNode_Stats->nodeSet.size()<<10);
	}

	{
		SIZER_COMPONENT_NAME(pSizer, "#NumRootNodes");
		pSizer->Add("#NumRootNodes", rootNodes.size()<<10);
	}
#endif

}

//////////////////////////////////////////////////////////////////////////
void CXmlUtils::OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
{
	switch (event)
	{
	case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
	case ESYSTEM_EVENT_LEVEL_LOAD_END:
		//
		if (g_pCXmlNode_PoolAlloc->GetTotalAllocatedNodeSize() == 0)
			g_pCXmlNode_PoolAlloc->FreeMemory();
		STLALLOCATOR_CLEANUP;
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
class CXmlBinaryDataWriterFile : public XMLBinary::IDataWriter
{
public:
	CXmlBinaryDataWriterFile( const char *file ) { m_file = gEnv->pCryPak->FOpen( file,"wb" ); }
	~CXmlBinaryDataWriterFile() { if (m_file) gEnv->pCryPak->FClose( m_file ); };
	virtual bool IsOk() { return m_file != 0; };
	virtual void Write(const void* pData, size_t size) { if (m_file) gEnv->pCryPak->FWrite( pData,size,1,m_file ); }
private:
	FILE *m_file;
};

//////////////////////////////////////////////////////////////////////////
bool CXmlUtils::SaveBinaryXmlFile( const char *filename,XmlNodeRef root )
{
	CXmlBinaryDataWriterFile fileSink(filename);
	if (!fileSink.IsOk())
		return false;
	XMLBinary::CXMLBinaryWriter writer;
	string error;
	return writer.WriteNode( &fileSink,root,false,error );
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CXmlUtils::LoadBinaryXmlFile( const char *filename )
{
	XMLBinary::XMLBinaryReader reader;
	XmlNodeRef root = reader.Parse( filename );

	return root;
}

//////////////////////////////////////////////////////////////////////////
bool CXmlUtils::EnableBinaryXmlLoading( bool bEnable )
{
	bool bPrev = g_bEnableBinaryXmlLoading;
	g_bEnableBinaryXmlLoading = bEnable;
	return bPrev;
}

//////////////////////////////////////////////////////////////////////////
// Init xml stats nodes pool
void CXmlUtils::InitStatsXmlNodePool( uint32 nPoolSize )
{
	if (0 == m_pStatsXmlNodePool)
	{
		// create special xml node pools for game statistics
		m_pStatsXmlNodePool = new CXmlNodePool(nPoolSize);
		assert(m_pStatsXmlNodePool);
	}
	else
	{
		CryLog("[CXmlNodePool]: Xml stats nodes pool already initialized");
	}
}

//////////////////////////////////////////////////////////////////////////
// Creates new xml node for statistics.
XmlNodeRef CXmlUtils::CreateStatsXmlNode( const char *sNodeName )
{
	if (0 == m_pStatsXmlNodePool)
	{
		CryLog("[CXmlNodePool]: Xml stats nodes pool isn't initialized. Perform default initialization.");
		InitStatsXmlNodePool();
	}
	return m_pStatsXmlNodePool->GetXmlNode(sNodeName);
}

#include UNIQUE_VIRTUAL_WRAPPER(IXmlSerializer)
#include UNIQUE_VIRTUAL_WRAPPER(IXmlUtils)
