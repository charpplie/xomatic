////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   PathUtil.cpp
//  Version:     v1.00
//  Created:     4/11/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "PathUtil.h"

namespace Path
{
	//////////////////////////////////////////////////////////////////////////
	CString GamePathToFullPath( const CString &path, bool bForWriting )
	{
		char szAdjustedFile[ICryPak::g_nMaxPath];
		const char *szTemp = gEnv->pCryPak->AdjustFileName( path,szAdjustedFile,bForWriting ? ICryPak::FLAGS_FOR_WRITING : 0);

		return szAdjustedFile;
	}

	//////////////////////////////////////////////////////////////////////////
	SANDBOX_API CString FullPathToGamePath( const CString &path )
	{
		char szAdjustedFile[ICryPak::g_nMaxPath];
		const char *szTemp = gEnv->pCryPak->AdjustFileName( path,szAdjustedFile,0 );
		return szAdjustedFile;
	}

	//////////////////////////////////////////////////////////////////////////
	void SplitPath(const CString& rstrFullPathFilename,CString& rstrDriveLetter,CString& rstrDirectory,CString& rstrFilename,CString& rstrExtension)
	{
		string			strFullPathString(rstrFullPathFilename);
		string			strDriveLetter;
		string			strDirectory;
		string			strFilename;
		string			strExtension;

		char*				szPath((char*)strFullPathString.c_str());
		char*				pchLastPosition(szPath);
		char*       pchCurrentPosition(szPath);
		char*				pchAuxPosition(szPath);

		// Directory names or filenames containing ":" are invalid, so we can assume if there is a :
		// it will be the drive name.
		pchCurrentPosition=strchr(pchLastPosition,':');
		if (pchCurrentPosition==NULL)
		{
			rstrDriveLetter="";
		}
		else
		{
			strDriveLetter.assign(pchLastPosition,pchCurrentPosition+1);
			pchLastPosition=pchCurrentPosition+1;
		}

		pchCurrentPosition=strrchr(pchLastPosition,'\\');
		pchAuxPosition=strrchr(pchLastPosition,'/');
		if ((pchCurrentPosition==NULL)&&(pchAuxPosition==NULL))
		{
			rstrDirectory="";
		}
		else
		{
			// Since NULL is < valid pointer, so this will work.
			if (pchAuxPosition>pchCurrentPosition)
			{
				pchCurrentPosition=pchAuxPosition;
			}
			strDirectory.assign(pchLastPosition,pchCurrentPosition+1);
			pchLastPosition=pchCurrentPosition+1;
		}

		pchCurrentPosition=strrchr(pchLastPosition,'.');
		if (pchCurrentPosition==NULL)
		{
			rstrExtension="";
			strFilename.assign(pchLastPosition);
		}
		else
		{
			strExtension.assign(pchCurrentPosition);
			strFilename.assign(pchLastPosition,pchCurrentPosition);
		}

		rstrDriveLetter=strDriveLetter;
		rstrDirectory=strDirectory;
		rstrFilename=strFilename;
		rstrExtension=strExtension;
	}
	//////////////////////////////////////////////////////////////////////////
	void GetDirectoryQueue(const CString& rstrSourceDirectory,std::vector<CString>& rcstrDirectoryTree)
	{
		string						strCurrentDirectoryName;
		string						strSourceDirectory(rstrSourceDirectory);
		const char*				szSourceDirectory(strSourceDirectory.c_str());
		const char*				pchCurrentPosition(szSourceDirectory);
		const char*				pchLastPosition(szSourceDirectory);
		const char*				pchAuxPosition(szSourceDirectory);

		rcstrDirectoryTree.clear();

		if (strSourceDirectory.empty())
		{
			return;
		}

		// It removes as many slashes the path has in its start...
		// MAYBE and just maybe we should consider paths starting with
		// more than 2 slashes invalid paths...
		while ((*pchLastPosition=='\\')||(*pchLastPosition=='/'))
		{
			++pchLastPosition;
			++pchCurrentPosition;
		}

		do 
		{
			pchCurrentPosition=strpbrk(pchLastPosition,"\\/");
			if (pchCurrentPosition==NULL)
			{
				break;
			}
			strCurrentDirectoryName.assign(pchLastPosition,pchCurrentPosition);
			pchLastPosition=pchCurrentPosition+1;
			// Again, here we are skipping as many consecutive slashes.
			while ((*pchLastPosition=='\\')||(*pchLastPosition=='/'))
			{
				++pchLastPosition;
			}
			

			rcstrDirectoryTree.push_back((CString)strCurrentDirectoryName);

		} while (true);
	}
	//////////////////////////////////////////////////////////////////////////
	void ConvertSlashToBackSlash(CString& rstrStringToConvert)
	{
		int nCount(0);
		int nTotal(0);

		string	strStringToConvert(rstrStringToConvert);

		nTotal=strStringToConvert.size();
		for (nCount=0;nCount<nTotal;++nCount)
		{
			if (strStringToConvert[nCount]=='/')
			{
				strStringToConvert.replace(nCount,1,1,'\\');
			}
		}
		rstrStringToConvert=strStringToConvert;
	}
	//////////////////////////////////////////////////////////////////////////
	void ConvertBackSlashToSlash(CString& rstrStringToConvert)
	{
		int nCount(0);
		int nTotal(0);

		string	strStringToConvert(rstrStringToConvert);

		nTotal=strStringToConvert.size();
		for (nCount=0;nCount<nTotal;++nCount)
		{
			if (strStringToConvert[nCount]=='\\')
			{
				strStringToConvert.replace(nCount,1,1,'/');
			}
		}	
		rstrStringToConvert=strStringToConvert;
	}
	//////////////////////////////////////////////////////////////////////////
	void SurroundWithQuotes(CString& rstrSurroundString)
	{
		string strSurroundString(rstrSurroundString);

		if (!strSurroundString.empty())
		{
			if (strSurroundString[0]!='\"')
			{
				strSurroundString.insert(0,"\"");
			}
			if (strSurroundString[strSurroundString.size()-1]!='\"')
			{
				strSurroundString.insert(strSurroundString.size(),"\"");
			}
		}
		else
		{
			strSurroundString.insert(0,"\"");
			strSurroundString.insert(strSurroundString.size(),"\"");
		}
		rstrSurroundString=strSurroundString;
	}
	//////////////////////////////////////////////////////////////////////////
	void GetParentDirectoryString(CString& strInputParentDirectory)
	{
		size_t	nLastFoundSlash(string::npos);
		size_t	nFirstFoundNotSlash(string::npos);

		string strTempInputParentDirectory(strInputParentDirectory);

		nFirstFoundNotSlash=strTempInputParentDirectory.find_last_not_of("\\/",nLastFoundSlash);
		// If we can't find a non-slash caracter, this is likely to be a mal formed path...
		// ...so we won't be able to determine a parent directory, if any.
		if (nFirstFoundNotSlash==string::npos)
		{
			return;
		}

		nLastFoundSlash=strTempInputParentDirectory.find_last_of("\\/",nFirstFoundNotSlash);
		// If we couldn't find any slash, this might be the root folder...and the root folder
		// has no parent at all.
		if (nLastFoundSlash==string::npos)
		{
			return;
		}
		
		strTempInputParentDirectory.erase(nLastFoundSlash,string::npos);

		strInputParentDirectory=strTempInputParentDirectory;

		return;
	}
	//////////////////////////////////////////////////////////////////////////
	CString GetExecutableFullPath()
	{
		#if defined (_WINDOWS_)
			return (CString)_pgmptr;
		#else
			assert("Not implemented"&&false);
		#endif
			return (CString)"";
	}
	//////////////////////////////////////////////////////////////////////////
	CString GetWindowsTempDirectory()
	{
		DWORD									nCurrentTempPathSize(128);
		DWORD									nReturnedPathSize(0);
		std::vector<TCHAR>		cTempStringBuffer;

		do 
		{
			nCurrentTempPathSize=nCurrentTempPathSize<<2;
			cTempStringBuffer.resize(nCurrentTempPathSize,0);
			nReturnedPathSize=GetTempPath(nCurrentTempPathSize-1,&cTempStringBuffer.front());
		} while (nCurrentTempPathSize<=nReturnedPathSize);
		return &cTempStringBuffer.front();
	}
	//////////////////////////////////////////////////////////////////////////
	CString GetExecutableParentDirectory()
	{
		CString strExecutablePath;
		CString strDriveLetter;
		CString strDirectory;
		CString strFilename;
		CString strExtension;
		CString strReturnValue;

		strExecutablePath=GetExecutableFullPath();
		SplitPath(strExecutablePath,strDriveLetter,strDirectory,strFilename,strExtension);
		strReturnValue=strDriveLetter;
		strReturnValue+=strDirectory;	
		GetParentDirectoryString(strReturnValue);

		return strReturnValue;
	}
	//////////////////////////////////////////////////////////////////////////
	CString GetExecutableParentDirectoryUnicode()
	{
		CString strReturnValue;
		WCHAR sBufferW[MAX_PATH];
		DWORD dwDirLen = GetCurrentDirectoryW(sizeof(sBufferW), sBufferW);
		strReturnValue = sBufferW;

		return strReturnValue;
	}
	//////////////////////////////////////////////////////////////////////////
	CString& ReplaceFilename( const CString &strFilepath,const CString &strFilename,CString& strOutputFilename)
	{
		CString strDriveLetter;
		CString strDirectory;
		CString strOriginalFilename;
		CString strExtension;

		SplitPath(strFilepath,strDriveLetter,strDirectory,strOriginalFilename,strExtension);

		strOutputFilename=strDriveLetter;
		strOutputFilename+=strDirectory;
		strOutputFilename+=strFilename;
		strOutputFilename+=strExtension;
		
		
		return strOutputFilename;
	}
	//////////////////////////////////////////////////////////////////////////
}
