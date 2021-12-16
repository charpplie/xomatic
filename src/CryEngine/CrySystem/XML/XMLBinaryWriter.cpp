////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek
// -------------------------------------------------------------------------
//  File name:   XMLBinaryWriter.cpp
//  Created:     21/04/2006 by Michael Smith.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//     8/1/2008 - Modified by Timur
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "XMLBinaryWriter.h"
#include "Endian.h"


const char* XMLBinary::BinaryFileHeader::sk_szCorrectSignature = "CryXmlB";

//////////////////////////////////////////////////////////////////////////
namespace XMLBinary
{
	void SwapEndianness_Node( Node &t )
	{
		SwapEndian(t.nTagStringOffset, true);
		SwapEndian(t.nContentStringOffset, true);
		SwapEndian(t.nAttributeCount, true);
		SwapEndian(t.nChildCount, true);
		SwapEndian(t.nParentIndex, true);
		SwapEndian(t.nFirstAttributeIndex, true);
		SwapEndian(t.nFirstChildIndex, true);
	}

	void SwapEndianness_Attribute( Attribute &t )
	{
		SwapEndian(t.nKeyStringOffset, true);
		SwapEndian(t.nValueStringOffset, true);
	}

	void SwapEndianness_Header( BinaryFileHeader &t )
	{
		SwapEndian(t.nXMLSize, true);
		SwapEndian(t.nNodeTablePosition, true);
		SwapEndian(t.nNodeCount, true);
		SwapEndian(t.nAttributeTablePosition, true);
		SwapEndian(t.nAttributeCount, true);
		SwapEndian(t.nChildTablePosition, true);
		SwapEndian(t.nChildCount, true);
		SwapEndian(t.nStringDataPosition, true);
		SwapEndian(t.nStringDataSize, true);
	}
}

//////////////////////////////////////////////////////////////////////////
XMLBinary::CXMLBinaryWriter::CXMLBinaryWriter()
{
	m_nStringDataSize = 0;
}

static void align(size_t& nPosition, const size_t nAlignment)
{
	const size_t nPadSize = ((nPosition + (nAlignment - 1)) & ~(nAlignment - 1)) - nPosition;
	nPosition += nPadSize;
}

static void alignWrite(XMLBinary::IDataWriter* const pFile, size_t& nPosition, const size_t nAlignment)
{
	size_t nPadSize = ((nPosition + (nAlignment - 1)) & ~(nAlignment - 1)) - nPosition;

	if (nPadSize > 0)
	{
		nPosition += nPadSize;

		static const char zeroes[32] = { 0 };

		while (nPadSize > 0)
		{
			const size_t n = (nPadSize <= sizeof(zeroes)) ? nPadSize : sizeof(zeroes);
			nPadSize -= n;
			pFile->Write(zeroes, n);
		}
	}
}

static void write(XMLBinary::IDataWriter* const pFile, size_t& nPosition, const void* const pData, const size_t nDataSize)
{
	pFile->Write(pData, nDataSize);
	nPosition += nDataSize;
}

//////////////////////////////////////////////////////////////////////////
bool XMLBinary::CXMLBinaryWriter::WriteNode(IDataWriter* pFile, XmlNodeRef node, bool bNeedSwapEndian, string& error)
{
	error = "";

	// Scan the node tree, building a flat node list, attribute list and string table.
	m_nStringDataSize = 0;
	
	if (!CompileTables(node, error))
	{
		return false;
	}

	static const uint nMaxNodeCount = (NodeIndex)~0;
	if (m_nodes.size() > nMaxNodeCount)
	{
		error.Format("XMLBinary: Too many nodes: %d (max is %i)", m_nodes.size(), nMaxNodeCount);
		return false;
	}

	// Initialize the file header.
	size_t nTheoreticalPosition = 0;
	static const size_t nAlignment = sizeof(uint32);

	BinaryFileHeader header;
	std::strncpy(header.szSignature, BinaryFileHeader::sk_szCorrectSignature, sizeof(header.szSignature));
	nTheoreticalPosition += sizeof(header);
	align(nTheoreticalPosition, nAlignment);

	header.nNodeTablePosition = nTheoreticalPosition;
	header.nNodeCount = int(m_nodes.size());
	nTheoreticalPosition += header.nNodeCount * sizeof(Node);
	align(nTheoreticalPosition, nAlignment);
	
	header.nChildTablePosition = nTheoreticalPosition;
	header.nChildCount = int(m_childs.size());
	nTheoreticalPosition += header.nChildCount * sizeof(NodeIndex);
	align(nTheoreticalPosition, nAlignment);

	header.nAttributeTablePosition = nTheoreticalPosition;
	header.nAttributeCount = int(m_attributes.size());
	nTheoreticalPosition += header.nAttributeCount * sizeof(Attribute);
	align(nTheoreticalPosition, nAlignment);

	header.nStringDataPosition = nTheoreticalPosition;
	header.nStringDataSize = m_nStringDataSize;
	nTheoreticalPosition += header.nStringDataSize;

	header.nXMLSize = nTheoreticalPosition;

	// Swap endianness of the data structures
	if (bNeedSwapEndian)
	{
		SwapEndianness_Header(header);
		for (size_t i = 0, iCount = m_nodes.size(); i < iCount; ++i)
		{
			SwapEndianness_Node(m_nodes[i]);
		}
		for (size_t i = 0, iCount = m_attributes.size(); i < iCount; ++i)
		{
			SwapEndianness_Attribute(m_attributes[i]);
		}
		for (size_t i = 0, iCount = m_childs.size(); i < iCount; ++i)
		{
			SwapEndian(m_childs[i], true);
		}
	}

	// Write file
	{
		nTheoreticalPosition = 0;

		// Write out the file header.
		write(pFile, nTheoreticalPosition, &header, sizeof(header));
		alignWrite(pFile, nTheoreticalPosition, nAlignment);

		// Write out the node table.
		if (!m_nodes.empty())
		{
			write(pFile, nTheoreticalPosition, &m_nodes[0], sizeof(m_nodes[0]) * m_nodes.size());
			alignWrite(pFile, nTheoreticalPosition, nAlignment);
		}

		// Write out the children table.
		if (!m_childs.empty())
		{
			write(pFile, nTheoreticalPosition, &m_childs[0], sizeof(m_childs[0]) * m_childs.size());
			alignWrite(pFile, nTheoreticalPosition, nAlignment);
		}

		// Write out the attribute table.
		if (!m_attributes.empty())
		{
			write(pFile, nTheoreticalPosition, &m_attributes[0], sizeof(m_attributes[0]) * m_attributes.size());
			alignWrite(pFile, nTheoreticalPosition, nAlignment);
		}

		// Write out the data of all the m_strings.
		for (size_t nString = 0; nString < m_strings.size(); ++nString)
		{
			pFile->Write(m_strings[nString].c_str(), m_strings[nString].size() + 1);
		}
	}

	return true;
}

bool XMLBinary::CXMLBinaryWriter::CompileTables(XmlNodeRef node, string& error)
{
	bool ok = CompileTablesForNode(node, -1, error);
	ok = ok && CompileChildTable(node, error);
	return ok;
}

//////////////////////////////////////////////////////////////////////////
bool XMLBinary::CXMLBinaryWriter::CompileTablesForNode(XmlNodeRef node, int nParentIndex, string& error)
{
	// Add the tag to the string table.
	int nTagStringIndex = AddString(node->getTag());

	// Add the content string to the string table.
	int nContentStringIndex = AddString(node->getContent());

	// Add all the attributes to the attributes table.
	const char* szKey;
	const char* szValue;
	const int nFirstAttributeIndex = int(m_attributes.size());
	for (int i = 0,attrCount = node->getNumAttributes(); i < attrCount; i++)
	{
		if (node->getAttributeByIndex( i,&szKey,&szValue ))
		{
			// Add the key and the value to the string table.
			Attribute attribute;
			attribute.nKeyStringOffset = AddString(szKey);
			attribute.nValueStringOffset = AddString(szValue);

			// Add the attribute to the attribute table.
			m_attributes.push_back(attribute);
		}
	}
	const int nAttributeCount = int(m_attributes.size()) - nFirstAttributeIndex;

	static const int nMaxAttributeCount = (uint16)~0;
	if (nAttributeCount > nMaxAttributeCount)
	{
		error.Format("XMLBinary: Too many attributes in a node: %d (max is %i)", nAttributeCount, nMaxAttributeCount);
		return false;
	}

	// Add ourselves to the node list.
	const int nIndex = int(m_nodes.size());

	Node nd;
	memset(&nd,0,sizeof(nd));

	nd.nTagStringOffset = nTagStringIndex;
	nd.nContentStringOffset = nContentStringIndex;
	nd.nParentIndex = nParentIndex;
	nd.nFirstAttributeIndex = nFirstAttributeIndex;
	nd.nAttributeCount = nAttributeCount;
	m_nodes.push_back(nd);

	m_nodesMap.insert( NodesMap::value_type(node,nIndex) );

	// Recurse to the child nodes.
	for (int nChild = 0,numChilds = node->getChildCount(); nChild < numChilds; ++nChild)
	{
		if (!CompileTablesForNode(node->getChild(nChild), nIndex, error))
		{
			return false;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool XMLBinary::CXMLBinaryWriter::CompileChildTable(XmlNodeRef node, string& error)
{
	const int nChildCount = node->getChildCount();

	static const int nMaxChildCount = (uint16)~0;
	if (nChildCount > nMaxChildCount)
	{
		error.Format("XMLBinary: Too many children in a node: %d (max is %i)", nChildCount, nMaxChildCount);
		return false;
	}

	const int nIndex = m_nodesMap.find(node)->second; // Assume node always exist in map.
	const int nFirstChildIndex = (int)m_childs.size();

	Node &nd = m_nodes[nIndex];
	nd.nFirstChildIndex = nFirstChildIndex;
	nd.nChildCount = nChildCount;

	for (int i = 0; i < nChildCount; ++i)
	{
		XmlNodeRef childNode = node->getChild(i);
		const int nChildIndex = m_nodesMap.find(childNode)->second; // Assume node always exist in map.
		m_childs.push_back(nChildIndex);
	}

	// Recurse to the child nodes.
	for (int i = 0; i < nChildCount; ++i)
	{
		if (!CompileChildTable(node->getChild(i), error))
		{
			return false;
		}
	}

	return true;
}

int XMLBinary::CXMLBinaryWriter::AddString( const XmlString& sString )
{
	// Look for the string in the string map.
	StringMap::const_iterator itStringEntry = m_stringMap.find(sString);

	// Check whether we found the string.
	if (itStringEntry == m_stringMap.end())
	{
		// The string wasn't in the string map, so we should add it to the table.
		m_strings.push_back(sString);
		itStringEntry = m_stringMap.insert(StringMap::value_type(sString, m_nStringDataSize)).first;
		m_nStringDataSize += sString.length() + 1;
	}

	// Return the index of the string in the string table.
	return (*itStringEntry).second;
}
