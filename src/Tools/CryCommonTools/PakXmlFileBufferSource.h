////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   PakXmlFileBufferSource.h
//  Version:     v1.00
//  Created:     28/4/2008 by MichaelS.
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __PAKXMLFILEBUFFERSOURCE_H__
#define __PAKXMLFILEBUFFERSOURCE_H__

#include "IXMLSerializer.h"
#include "IPakSystem.h"

class PakXmlFileBufferSource : public IXmlBufferSource
{
public:
	PakXmlFileBufferSource(IPakSystem* pakSystem, const char* path)
		: pakSystem(pakSystem)
	{
		file = pakSystem->Open(path, "r");
	}
	~PakXmlFileBufferSource()
	{
		if (file)
			pakSystem->Close(file);
	}

	virtual int Read(void* buffer, int size) const
	{
		return pakSystem->Read(file, buffer, size);
	};

	IPakSystem* pakSystem;
	PakSystemFile* file;
};

#endif //__PAKXMLFILEBUFFERSOURCE_H__
