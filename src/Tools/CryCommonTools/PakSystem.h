////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   PakSystem.h
//  Version:     v1.00
//  Created:     8/4/2008 by MichaelS.
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __PAKSYSTEM_H__
#define __PAKSYSTEM_H__

#include "IPakSystem.h"
#include "ZipDir/ZipDir.h"

enum PakSystemFileType
{
	PakSystemFileType_File,
	PakSystemFileType_PakFile
};
struct PakSystemFile
{
	PakSystemFileType type;

	// PakSystemFileType_File
	FILE* file;

	// PakSystemFileType_PakFile
	ZipDir::CachePtr zip;
	ZipDir::FileEntry* fileEntry;
	void* data;
	int dataPosition;
};

struct PakSystemArchive
{
	ZipDir::CacheRWPtr zip;
};

class PakSystem : public IPakSystem
{
public:
	PakSystem();

	// IPakSystem
	virtual PakSystemFile* Open(const char* filename, const char* mode);
	virtual bool ExtractNoOverwrite(const char* filename, const char* extractToFile = 0);
	virtual void Close(PakSystemFile* file);
	virtual int Read(PakSystemFile* file, void* buffer, int size);
	virtual bool EoF(PakSystemFile* file);

	virtual PakSystemArchive* OpenArchive(const char* path);
	virtual void CloseArchive(PakSystemArchive* archive);
	virtual void AddToArchive(PakSystemArchive* archive, const char* path, void* data, int size);

private:
	ZipDir::CachePtr OpenArchive(const char* szPath, const char* mode);
};

#endif //__PAKSYSTEM_H__
