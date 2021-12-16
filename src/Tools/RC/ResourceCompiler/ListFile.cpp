////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek GmbH, 2010.
// -------------------------------------------------------------------------
//  File name:   ListFile.cpp
//  Version:     v1.00
//  Created:     02/02/2010 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ListFile.h"
#include <StringUtils.h>
#include "TempFilePakExtraction.h"
#include "PathHelpers.h"
#include "IPakSystem.h"
#include "IResCompiler.h"
#include "IRCLog.h"

//////////////////////////////////////////////////////////////////////////
CListFile::CListFile( IResourceCompiler *pRC )
{
	m_pRC = pRC;
}

//////////////////////////////////////////////////////////////////////////
bool CListFile::Process( const string &inListFile,const string &wildcard,std::vector<string> &outFiles )
{
	if (!inListFile.empty() && inListFile[0] == '@')
	{
		int splitter = inListFile.find_first_of( "|:;," );
		if (splitter < 0)
			return false;

		string zipFilename = inListFile.substr(1,splitter-1);
		string listFilename = inListFile.substr(splitter+1);
		zipFilename.Trim();
		listFilename.Trim();

		ParseListFileInZip( zipFilename,listFilename,wildcard,outFiles );
		return true;
	}

	std::vector<string> lines;
	// Parse List File.
	if (!ParseListFile(inListFile,lines))
		return false;

	for (int i = 0; i < (int)lines.size(); i++)
	{
		string line = lines[i];

		// Line can either be normal file
		// or a z zip file + list file (ex: @Levels\AlienVessel\Level.pak|resourcelist.txt)
		if (line[0] == '@')
		{
			// If Line starts with a @ sign, this means zip file
			
			line = line.substr(1); // skip @ character

			int splitter = line.find_first_of( "|:;," );
			if (splitter < 0)
				continue;

			string zipFilename = line.substr(0,splitter);
			string listFilename = line.substr(splitter+1);
			zipFilename.Trim();
			listFilename.Trim();

			ParseListFileInZip( zipFilename,listFilename,wildcard,outFiles );
		}
		else
		{
			if (CryStringUtils::MatchWildcardIgnoreCase( line.c_str(),wildcard.c_str() ))
			{
				line.replace( '/','\\' );
				outFiles.push_back(line);
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CListFile::ParseListFile( const string &inListFile,std::vector<string> &lines )
{
	FILE *f = fopen(inListFile,"rt");
	if (!f)
		return false;

	char line[1024];
	while (fgets(line,sizeof(line),f) != NULL)
	{
		if (strlen(line) > 0)
		{
			string strLine = line;
			strLine.Trim();
			lines.push_back(strLine);
		}
	}
	fclose(f);

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CListFile::ParseListFileInZip( const string &zipFilename,const string &listFilename,const string &wildcard,std::vector<string> &outFiles )
{
	// Open zip file
	IPakSystem* pPakSystem = m_pRC->GetPakSystem();

	string sFileInPak = string("@") + zipFilename + "|" + listFilename;
	TempFilePakExtraction fileProxy( sFileInPak.c_str(), pPakSystem );

	std::vector<string> lines;
	// Parse List File.
	if (!ParseListFile(fileProxy.GetTempName(),lines))
	{
		RCLogWarning("List file %s not find in zip file %s",zipFilename.c_str(),listFilename.c_str());
		return;
	}

	for (int i = 0; i < (int)lines.size(); i++)
	{
		//CryStringUtils::MatchWildcardIgnoreCase( lines[i].c_str(),wildcard.c_str() );
		//if (PathUtil::MatchWildcard( lines[i].c_str(),wildcard.c_str() ))
		string line = lines[i].c_str();
		if (CryStringUtils::MatchWildcardIgnoreCase( line,wildcard.c_str() ))
		{
			line.replace( '/','\\' );
			outFiles.push_back(line);
		}
	}
}
