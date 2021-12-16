////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   PakSystem.cpp
//  Version:     v1.00
//  Created:     8/4/2008 by MichaelS.
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PakSystem.h"
#include "PathHelpers.h"
#include "StringHelpers.h"

#include "zlib/zlib.h"

PakSystem::PakSystem()
{
}

PakSystemFile* PakSystem::Open(const char* a_path, const char* a_mode)
{
	string normalPath = a_path;

	string const zipExt = ".zip";
	bool bZip = StringHelpers::EndsWithIgnoreCase(normalPath, zipExt);

	if (bZip)
	{
		// If it's a .zip file, then we'll try to look for a file without .zip extension inside of the .zip file
		normalPath.erase(normalPath.length() - zipExt.length(), zipExt.length());
	}

	string zipPath = normalPath + zipExt;
	string filename = PathHelpers::GetFilename(normalPath);

	if (!normalPath.empty() && normalPath[0] == '@')
	{
		// File is inside pak file.
		int splitter = normalPath.find_first_of( "|:;," );
		if (splitter >= 0)
		{
			zipPath = normalPath.substr(1,splitter-1);
			filename = normalPath.substr(splitter+1);
			std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
			bZip = true;
		}
		else
		{
			return 0;
		}
	}

	if (!bZip)
	{
		// Try to open the file.
		FILE* const f = fopen(normalPath.c_str(), a_mode);
		if (f)
		{
			PakSystemFile* file = new PakSystemFile();
			file->type = PakSystemFileType_File;
			file->file = f;
			return file;
		}
	}

	// if it's simple and read-only, it's assumed it's read-only
	unsigned const nFactoryFlags = ZipDir::CacheFactory::FLAGS_DONT_COMPACT | ZipDir::CacheFactory::FLAGS_READ_ONLY;

	bool bFileExists = false;

	if (bZip)
	{
		// a caller asked to open a .zip file. check if the .zip file on disk exist
		FILE* const f = fopen(zipPath.c_str(), "rb");
		if (f)
		{
			fclose(f);
			bFileExists = true;
		}
	}
	else
	{
		// a caller specified normal file. we already failed to find it on disk,
		// so the file could be within a .pak file. let's find all 'potential'
		// pak files and look within these for a matching file

		std::vector<string> foundFileCountainer; // pak files found

		string dirToSearch = normalPath;
		while (!dirToSearch.empty()) 
		{
			dirToSearch = PathHelpers::RemoveSeparator( dirToSearch );
			dirToSearch = PathHelpers::GetParentDirectory( dirToSearch );	//descend path
			if( dirToSearch != "" )
			{
				dirToSearch += "\\";
			}

			WIN32_FIND_DATAA FindFileData;
			HANDLE hFind;
			string search = dirToSearch + "*.pak";

			hFind = FindFirstFileA( search.c_str(),&FindFileData );
			if ( hFind != INVALID_HANDLE_VALUE )
			{
				foundFileCountainer.push_back( dirToSearch + string( FindFileData.cFileName ) );
				while ( FindNextFileA( hFind,&FindFileData ) != 0 )
				{
					foundFileCountainer.push_back( dirToSearch + string( FindFileData.cFileName ) );
				}
			}

			FindClose(hFind);
		}

		// iterate through found containers and look for relevant files within them
		for (int iFile = 0; iFile < foundFileCountainer.size(); ++iFile)
		{
			zipPath = foundFileCountainer[ iFile ];
			string pathToZip = PathHelpers::GetParentDirectory( zipPath );

			// construct filename by removing path to zip from path to filename
			string pathToFile = PathHelpers::GetParentDirectory( string( normalPath ) );
			string pureFileName = PathHelpers::GetFilename( string( normalPath ) );
			pathToFile = pathToFile.substr( pathToZip.length() + 1 );
			filename = pathToFile.empty() 
				? pureFileName 
				: pathToFile + "\\" + pureFileName;

			ZipDir::CacheFactory factory(ZipDir::ZD_INIT_FAST, nFactoryFlags);
			ZipDir::CachePtr testZip = factory.New( zipPath.c_str() );
			ZipDir::FileEntry* testFileEntry = (testZip ? testZip->FindFile(filename.c_str()) : 0);

			// break out if we have a testFileEntry, as we've found our first (and best) candidate.
			if (testFileEntry)
			{
				bFileExists = true;
				break;
			}
		}
	}

	{
		ZipDir::CacheFactory factory(ZipDir::ZD_INIT_FAST, nFactoryFlags);
		ZipDir::CachePtr zip = (bFileExists ? factory.New(zipPath.c_str()) : 0);
		ZipDir::FileEntry* fileEntry = (zip ? zip->FindFile(filename.c_str()) : 0);

		if (fileEntry)
		{
			PakSystemFile* file = new PakSystemFile();
			file->type = PakSystemFileType_PakFile;
			file->zip = zip;
			file->fileEntry = fileEntry;
			file->data = zip->AllocAndReadFile(file->fileEntry);
			file->dataPosition = 0;
			return file;
		}
	}

	return 0;
}


//Extracts archived file to disk without overwriting any files
//returns true on success, false on failure (due to potential overwrite or no file
//in archive
bool PakSystem::ExtractNoOverwrite(const char* fileToExtract, const char* extractToFile)
{
	if( 0 == extractToFile )
	{
		extractToFile = fileToExtract;
	}

	//open file using pak system
	PakSystemFile* fileZip = Open(fileToExtract, "r");
	if( !fileZip  )
	{
		return false;
	}

	// Try to open a writable file
	FILE* fFileOnDisk = fopen(extractToFile, "wb");
	if(!fFileOnDisk)
	{
		Close( fileZip );
		return false;
	}

	fwrite( fileZip->data, fileZip->fileEntry->desc.lSizeUncompressed, 1, fFileOnDisk );
	fclose( fFileOnDisk );
	return true;
}


void PakSystem::Close(PakSystemFile* file)
{
	if (file)
	{
		switch (file->type)
		{
		case PakSystemFileType_File:
			fclose(file->file);
			break;

		case PakSystemFileType_PakFile:
			file->zip->Free(file->data);
			break;
		}
		delete file;
	}
}

int PakSystem::Read(PakSystemFile* file, void* buffer, int size)
{
	int readBytes = 0;

	if (file)
	{
		switch (file->type)
		{
		case PakSystemFileType_File:
			{
				readBytes = fread(buffer, 1, size, file->file);
			}
			break;

		case PakSystemFileType_PakFile:
			{
				int fileSize = file->fileEntry->desc.lSizeUncompressed;
				readBytes = (fileSize - file->dataPosition > size ? size : fileSize - file->dataPosition);
				memcpy(buffer, static_cast<char*>(file->data) + file->dataPosition, readBytes);
				file->dataPosition += readBytes;
			}
			break;
		}
	}

	return readBytes;
}

bool PakSystem::EoF(PakSystemFile* file)
{
	bool EoF = true;
	if (file)
	{
		switch (file->type)
		{
		case PakSystemFileType_File:
			{
				EoF = (0 != feof(file->file));
			}
			break;

		case PakSystemFileType_PakFile:
			{
				int fileSize = file->fileEntry->desc.lSizeUncompressed;
				EoF = (file->dataPosition >= fileSize);
			}
			break;
		}
	}

	return EoF;
}

PakSystemArchive* PakSystem::OpenArchive(const char* path)
{
	//unsigned nFactoryFlags = ZipDir::CacheFactory::FLAGS_DONT_COMPACT | ZipDir::CacheFactory::FLAGS_CREATE_NEW;
	unsigned nFactoryFlags = 0;
	ZipDir::CacheFactory factory(ZipDir::ZD_INIT_FAST, nFactoryFlags);
	ZipDir::CacheRWPtr cache = factory.NewRW(path);
	PakSystemArchive* archive = (cache ? new PakSystemArchive() : 0);
	if (archive)
		archive->zip = cache;
	return archive;
}

void PakSystem::CloseArchive(PakSystemArchive* archive)
{
	if (archive)
	{
		archive->zip->Close();
		delete archive;
	}
}

void PakSystem::AddToArchive(PakSystemArchive* archive, const char* path, void* data, int size)
{
	archive->zip->UpdateFile(path, data, size, ZipFile::METHOD_DEFLATE);
}
