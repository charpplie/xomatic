//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __FILESYSTEMUTIL_H__
#define __FILESYSTEMUTIL_H__

#include "PathUtil.h"
#include <Windows.h>
#include <set>
#include <fstream>
#include <algorithm>
#include <iterator>

namespace FileSystemUtil
{
	enum FileFlag
	{
		FileFlagExists = 1,
		FileFlagDirectory = 2
	};
	inline unsigned int GetFileFlags(const std::string& sPath)
	{
		unsigned int uFlags = 0;

		std::string sFilePath = PathUtil::RemoveSlash(sPath);

		WIN32_FIND_DATA findData;
		HANDLE hSearch = FindFirstFile(sFilePath.c_str(), &findData);
		if (hSearch != INVALID_HANDLE_VALUE)
		{
			uFlags |= FileFlagExists;
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				uFlags |= FileFlagDirectory;
			FindClose(hSearch);
		}

		return uFlags;
	}

	inline bool IsDirectory(const std::string& path)
	{
		return (GetFileFlags(path) & FileFlagDirectory) != 0;
	}

	template <typename F> inline void FindFiles(const std::string& sDirectory, const std::set<std::string>& extensions, F& f, bool bRecurse = true, bool bIncludeDirectories = false, bool bAbsolutePaths = true)
	{
		char path[1024];
		strcpy(path, sDirectory.c_str());
		strcat(path, "\\*.*");

		WIN32_FIND_DATA fd;
		HANDLE hff = FindFirstFile(path, &fd);
		if (hff != INVALID_HANDLE_VALUE)
		{
			BOOL bIsFind = TRUE;
			while(hff && bIsFind)
			{
				std::string sFileName;
				if (bAbsolutePaths)
					sFileName = PathUtil::Make(sDirectory, fd.cFileName);
				else
					sFileName = fd.cFileName;

				if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if(strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, ".."))
					{
						if (bIncludeDirectories)
						{
							f(sFileName);
						}
						if (bRecurse)
							FindFiles(PathUtil::Make(sDirectory, fd.cFileName), extensions, f);
					}
				}
				else
				{
					std::string sExtension = PathUtil::GetExt(fd.cFileName);
					if (extensions.empty() || extensions.find(sExtension) != extensions.end())
					{
						f(sFileName);
					}
				}
				bIsFind = FindNextFile(hff, &fd);
			}
			FindClose(hff);
		}
	}

	class DirectoryLister
	{
	public:
		DirectoryLister(std::vector<std::string>& files)
			: files(files)
		{
		}

		// Because we are using STLport 4.6, we cannot pass this argument by reference and use bind1st.
		void FoundFile(std::string sFile)
		{
			this->files.push_back(sFile);
		}

		std::vector<std::string>& files;
	};
	inline void GetDirectoryList(const std::string& sDirectory, std::vector<std::string>& files, bool bRecursive=false, bool bAbsolutePaths=false, std::set<std::string> extensions= std::set<std::string>())
	{
		DirectoryLister lister(files);
		FindFiles(sDirectory, extensions, std::bind1st(std::mem_fun(&DirectoryLister::FoundFile), &lister), bRecursive, extensions.empty(), bAbsolutePaths);
	}

	inline void DeleteFile(std::string sFile)
	{
		::DeleteFile(sFile.c_str());
	}
	inline void DeleteFiles(const std::string& sDirectory, const std::set<std::string>& extensions)
	{
		FileSystemUtil::FindFiles(sDirectory, extensions, DeleteFile);
	}

	inline size_t GetFileLength(std::ifstream& file)
	{
		size_t uOldPosition = file.tellg();
		file.seekg(0, std::ios_base::end);
		size_t uLength = file.tellg();
		file.seekg(uOldPosition, std::ios_base::beg);
		return uLength;
	}

	inline bool FilesIdentical(const std::string& sPath1, const std::string& sPath2)
	{
		bool bFilesIdentical = true;

		// Try to open both files.
		std::ifstream file1(sPath1.c_str(), std::ios_base::in | std::ios_base::binary);
		std::ifstream file2(sPath2.c_str(), std::ios_base::in | std::ios_base::binary);

		// Check whether only one file is open.
		if (file1.is_open() ^ file2.is_open())
			bFilesIdentical = false;

		// Check whether the files are the same length.
		if (file1.is_open() && file2.is_open())
		{
			if (GetFileLength(file1) != GetFileLength(file2))
				bFilesIdentical = false;
		}

		// Check whether the contents of the files are identical.
		if (bFilesIdentical)
		{
			// Compare the files in 1k blocks.
			while (!file1.eof())
			{
				enum {BlockSize = 1024};
				char pBuffer1[BlockSize];
				char pBuffer2[BlockSize];
				file1.read(pBuffer1, BlockSize);
				file2.read(pBuffer2, BlockSize);
				if (!std::equal(pBuffer1, pBuffer1 + file1.gcount(), pBuffer2))
					bFilesIdentical = false;
			}
		}

		return bFilesIdentical;
	}

	class DirectoryDifference
	{
	public:
		DirectoryDifference(const std::string& sRelativePath, unsigned int uExistenceFlags)
			:	sRelativePath(sRelativePath), uExistenceFlags(uExistenceFlags)
		{
		}
		std::string sRelativePath;
		unsigned int uExistenceFlags;
	};
	template <typename F> inline bool ComparePaths(const std::string& sDirectory1, const std::string& sDirectory2, const std::string& sRelativePath, F& f)
	{
		bool bShouldRecurse = false;

		// Combine the directories and the relative path to find the full path.
		std::string sPath1 = PathUtil::Make(sDirectory1, sRelativePath);
		std::string sPath2 = PathUtil::Make(sDirectory2, sRelativePath);

		// Query the file system about the paths.
		unsigned int uFlags1 = GetFileFlags(sPath1);
		unsigned int uFlags2 = GetFileFlags(sPath2);

		// Handle the path based on whether both files exist or not.
		unsigned int uExistenceFlags = ((uFlags1 & FileFlagExists) ? 1 : 0) | ((uFlags2 & FileFlagExists) ? 2 : 0);
		switch (uExistenceFlags)
		{
			// The file exists in only one of the directories - already we have a difference.
		case 1:
		case 2:
			f(DirectoryDifference(sRelativePath, uExistenceFlags));
			break;

			// The file exists in both directories.
		case 3:
			{

				// Handle the path based on whether both files are directories.
				unsigned int uDirectoryFlags = ((uFlags1 & FileFlagDirectory) ? 1 : 0) | ((uFlags2 & FileFlagDirectory) ? 2 : 0);
				switch (uDirectoryFlags)
				{
					// Neither file is a directory.
				case 0:

					// Check whether the files are identical.
					if (!FilesIdentical(sPath1, sPath2))
						f(DirectoryDifference(sRelativePath, uExistenceFlags));

					break;

					// Only one of the files is a directory - we have a difference.
				case 1:
				case 2:
					f(DirectoryDifference(sRelativePath, uExistenceFlags));
					break;

					// Both files are directories.
				case 3:

					// We should recurse to the children of these directories.
					bShouldRecurse = true;
					break;
				}
			}

			break;
		}

		return bShouldRecurse;
	}
	template <typename F> inline void CompareDirectoriesRecurse(const std::string& sDirectory1, const std::string& sDirectory2, const std::string& sRelativePath, F& f)
	{
		// Combine the directories and the relative path to find the full path.
		std::string sPath1 = PathUtil::Make(sDirectory1, sRelativePath);
		std::string sPath2 = PathUtil::Make(sDirectory2, sRelativePath);

		// Get a listing of both directories and then combine them.
		std::vector<std::string> directoryList1;
		std::vector<std::string> directoryList2;
		GetDirectoryList(sPath1, directoryList1);
		GetDirectoryList(sPath2, directoryList2);
		std::set<std::string> directoryList;
		std::copy(directoryList1.begin(), directoryList1.end(), std::inserter(directoryList, directoryList.begin()));
		std::copy(directoryList2.begin(), directoryList2.end(), std::inserter(directoryList, directoryList.begin()));

		// Loop through all the files, checking for differences, and possibly recursing to sub-directories.
		for (std::set<std::string>::iterator itFile = directoryList.begin(); itFile != directoryList.end(); ++itFile)
		{
			std::string sNewPath = PathUtil::Make(sRelativePath, *itFile);

			// Compare the files in each directory.
			if (ComparePaths(sDirectory1, sDirectory2, sNewPath, f))
			{
				// ComparePaths() determined that we should recurse.
				CompareDirectoriesRecurse(sDirectory1, sDirectory2, sNewPath, f);
			}
		}
	}
	template <typename F> inline void CompareDirectories(const std::string& sDirectory1, const std::string& sDirectory2, F& f)
	{
		// Check whether the root directories are identical.
		if (ComparePaths(sDirectory1, sDirectory2, "", f))
			CompareDirectoriesRecurse(sDirectory1, sDirectory2, "", f);
	}
}

#endif //__FILESYSTEMUTIL_H__
