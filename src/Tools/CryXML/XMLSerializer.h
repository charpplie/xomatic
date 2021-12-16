//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __XMLSERIALIZER_H__
#define __XMLSERIALIZER_H__

#include "IXMLSerializer.h"

class XMLSerializer : public IXMLSerializer
{
public:
	virtual XmlNodeRef CreateNode(const char *tag);
	virtual bool Write(XmlNodeRef root, const char* szFileName);

	virtual XmlNodeRef Read(const IXmlBufferSource& source, bool bRemoveNonessentialSpacesFromContent, int nErrorBufferSize, char* szErrorBuffer);
};

#endif //__XMLSERIALIZER_H__
