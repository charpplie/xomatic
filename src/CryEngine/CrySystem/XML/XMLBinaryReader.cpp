//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "XMLBinaryReader.h"
#include "XMLBinaryNode.h"

XMLBinary::XMLBinaryReader::XMLBinaryReader()
:	pError(0)
{
}

XMLBinary::XMLBinaryReader::~XMLBinaryReader()
{
	if (this->pError)
	{
		delete this->pError;
		this->pError = 0;
	}
}

bool XMLBinary::XMLBinaryReader::HasSignature(CCryFile* pFile)
{
	// Read the header from the file.
	BinaryFileHeader header;
	size_t oldPos = pFile->GetPosition();
	bool bReadSuccess = (pFile->ReadRaw(&header, sizeof(header)) == sizeof(header));
	
	pFile->Seek(oldPos, SEEK_SET);
	if (!bReadSuccess)
		return false;

	// Check the signature of the file to make sure that it is a binary xml file.
	if (memcmp(header.szSignature, BinaryFileHeader::sk_szCorrectSignature, sizeof(BinaryFileHeader::sk_szCorrectSignature)) != 0)
		return false;

	return true;
}

XmlNodeRef XMLBinary::XMLBinaryReader::Parse( const char *filename )
{
	CCryFile xmlFile;

	if (!xmlFile.Open( filename,"rb" ))
	{
		return 0;
	}

	// Read the file header.
	BinaryFileHeader layout;
	if (!this->ReadHeader(&xmlFile, layout))
	{
		return XmlNodeRef();
	}
	
	//////////////////////////////////////////////////////////////////////////
	CBinaryXmlData* pData = new CBinaryXmlData;

	// Read in the entire file - this buffer will not be deallocated immediately, since the nodes
	// will contain pointers directly into it. It will be deleted once the reference count on the
	// StringBuffer object reaches 0 again.

	char* const pFileData = new char[layout.nXMLSize];
	pData->pFileContents = pFileData;
	{
		memcpy(pFileData, &layout, sizeof(layout));

		const size_t sizeToRead = layout.nXMLSize - sizeof(layout);

		if (xmlFile.ReadRaw(pFileData + sizeof(layout), sizeToRead) != sizeToRead)
		{
			this->SetError(new CorruptError());
			return false;
		}
	}

	// Create the nodes.
	pData->pBinaryNodes = new CBinaryXmlNode[layout.nNodeCount];

	pData->pAttributes = reinterpret_cast<Attribute*>(pFileData + layout.nAttributeTablePosition);
	pData->pChildIndices = reinterpret_cast<NodeIndex*>(pFileData + layout.nChildTablePosition);
	pData->pNodes = reinterpret_cast<Node*>(pFileData + layout.nNodeTablePosition);
	pData->pStringData = pFileData + layout.nStringDataPosition;

	for (uint32 nNode = 0; nNode < layout.nNodeCount; ++nNode)
	{
		// Create the new node.
		CBinaryXmlNode *pNode = &pData->pBinaryNodes[nNode];
		pNode->m_nRefCount = 0;
		pNode->m_pData = pData;
	}

	// Return first node.
	return &pData->pBinaryNodes[0];
}

XMLBinary::IError* XMLBinary::XMLBinaryReader::GetError()
{
	return this->pError;
}

bool XMLBinary::XMLBinaryReader::ReadHeader(CCryFile* pFile, BinaryFileHeader& layout)
{
	// Read the header from the file.
	if (pFile->ReadRaw(&layout, sizeof(layout)) != sizeof(layout))
	{
		this->SetError(new CorruptError());
		return false;
	}

	// Check the signature of the file to make sure that it is a binary xml file.
	if (memcmp(layout.szSignature, BinaryFileHeader::sk_szCorrectSignature, sizeof(BinaryFileHeader::sk_szCorrectSignature)) != 0)
	{
		this->SetError(new FileTypeError());
		return false;
	}

	// Read the file layout.

	// Sanity check the file layout.
	bool bCorrupt = false;
	const uint32 nNodeTableEnd = layout.nNodeTablePosition + layout.nNodeCount * sizeof(Node);
	const uint32 nChildTableEnd = layout.nChildTablePosition + layout.nChildCount * sizeof(NodeIndex);
	const uint32 nAttributeTableEnd = layout.nAttributeTablePosition + layout.nAttributeCount * sizeof(Attribute);
	const uint32 nStringDataEnd = layout.nStringDataPosition + layout.nStringDataSize;

	bCorrupt = bCorrupt || layout.nXMLSize > pFile->GetLength();
	bCorrupt = bCorrupt || nNodeTableEnd > layout.nChildTablePosition;
	bCorrupt = bCorrupt || nChildTableEnd > layout.nAttributeTablePosition;
	bCorrupt = bCorrupt || nAttributeTableEnd > layout.nStringDataPosition;
	bCorrupt = bCorrupt || nStringDataEnd > layout.nXMLSize;
	if (bCorrupt)
	{
		this->SetError(new CorruptError());
		return false;
	}

	return true;
}

void XMLBinary::XMLBinaryReader::SetError(IError* pError)
{
	// Delete any current error.
	if (this->pError)
	{
		delete this->pError;
		this->pError = 0;
	}

	// Store the error.
	this->pError = pError;
}

void XMLBinary::XMLBinaryReader::CorruptError::GetDescription(int nBufferSize, char* szDescriptionBuffer)
{
	strncpy(szDescriptionBuffer, "Binary XML file is corrupt.", nBufferSize);
	szDescriptionBuffer[nBufferSize - 1] = 0;
}

void XMLBinary::XMLBinaryReader::FileTypeError::GetDescription(int nBufferSize, char* szDescriptionBuffer)
{
	strncpy(szDescriptionBuffer, "File is not a binary XML file.", nBufferSize);
	szDescriptionBuffer[nBufferSize - 1] = 0;
}

/*
{
XmlNodeRef node = GetXmlUtils()->LoadXmlFile( "SmartObjects.xml" );
GetXmlUtils()->SaveBinaryXmlFile("Test.binxml",node);
}
{
XmlNodeRef node = GetXmlUtils()->LoadBinaryXmlFile("Test.binxml");
int nChilds = node->getChildCount();
XmlNodeRef child = node->getChild(2);
const char *s  = child->getAttr("sName");
}
*/