//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "XMLSerializer.h"
#include "xml/xml.h"
#include "IXMLSerializer.h"

XmlNodeRef XMLSerializer::CreateNode(const char *tag)
{
	return new CXmlNode(tag);
}

bool XMLSerializer::Write(XmlNodeRef root, const char* szFileName)
{
	return root->saveToFile(szFileName);
}

XmlNodeRef XMLSerializer::Read(const IXmlBufferSource& source, bool bRemoveNonessentialSpacesFromContent, int nErrorBufferSize, char* szErrorBuffer)
{
	XmlParser parser(bRemoveNonessentialSpacesFromContent);
	XmlNodeRef root = parser.parseSource(&source);
	if (nErrorBufferSize > 0 && szErrorBuffer && parser.getErrorString())
	{
		strncpy(szErrorBuffer, parser.getErrorString(), nErrorBufferSize);
		szErrorBuffer[nErrorBufferSize - 1] = 0;
	}
	return root;
}
