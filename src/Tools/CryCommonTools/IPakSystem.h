////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   IPakSystem.h
//  Version:     v1.00
//  Created:     8/4/2008 by MichaelS.
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __IPAKSYSTEM_H__
#define __IPAKSYSTEM_H__

struct PakSystemFile;
struct PakSystemArchive;
class IPakSystem
{
public:
	virtual PakSystemFile* Open(const char* filename, const char* mode) = 0;
	virtual bool ExtractNoOverwrite(const char* filename, const char* extractToFile = 0) = 0;
	virtual void Close(PakSystemFile* file) = 0;
	virtual int Read(PakSystemFile* file, void* buffer, int size) = 0;
	virtual bool EoF(PakSystemFile* file) = 0;

	virtual PakSystemArchive* OpenArchive(const char* path) = 0;
	virtual void CloseArchive(PakSystemArchive* archive) = 0;
	virtual void AddToArchive(PakSystemArchive* archive, const char* path, void* data, int size) = 0;
};

#endif //__IPAKSYSTEM_H__
