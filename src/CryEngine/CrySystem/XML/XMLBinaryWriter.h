////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek
// -------------------------------------------------------------------------
//  File name:   XMLBinaryWriter.h
//  Created:     21/04/2006 by Michael Smith.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//     8/1/2008 - Modified by Timur
////////////////////////////////////////////////////////////////////////////

#ifndef __XMLBINARYWRITER_H__
#define __XMLBINARYWRITER_H__

#include "IXml.h"
#include "XMLBinaryHeaders.h"
#include <vector>
#include <map>

class IXMLDataSink;

namespace XMLBinary
{
	class CXMLBinaryWriter
	{
	public:
		CXMLBinaryWriter();
		bool WriteNode(IDataWriter* pFile, XmlNodeRef node, bool bNeedSwapEndian, string& error);

	private:
		bool CompileTables(XmlNodeRef node, string& error);

		bool CompileTablesForNode(XmlNodeRef node, int nParentIndex, string& error);
		bool CompileChildTable(XmlNodeRef node, string& error);
		int AddString(const XmlString& sString);

	private:
		// tables.
		typedef std::map<IXmlNode*,int> NodesMap;
		typedef std::map<string, uint> StringMap;

		std::vector<Node> m_nodes;
		NodesMap m_nodesMap;
		std::vector<Attribute> m_attributes;
		std::vector<NodeIndex> m_childs;
		std::vector<string> m_strings;
		StringMap m_stringMap;

		uint m_nStringDataSize;
	};
}

#endif //__XMLBINARYWRITER_H__
