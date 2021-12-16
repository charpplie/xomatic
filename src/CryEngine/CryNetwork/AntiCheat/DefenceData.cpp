/*************************************************************************
 Crytek Source File.
 Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
 $Id$
 $DateTime$
 Description:  common anti-cheat defence data
 -------------------------------------------------------------------------
 History:
 - 10/08/2004   : Created by Craig Tiller, based on CDefenceWall by Timur
*************************************************************************/

#include "StdAfx.h"
#include "DefenceData.h"
#include "Network.h"
#include "Config.h"

#if USE_DEFENCE

CDefenceData::CDefenceData()
{
	version = 0;
	level = -1;
}

void CDefenceData::UnifyFilename( string &sFilename )
{
	string file = sFilename;
	file.replace( '\\','/' );
	file.replace("%GAME_FOLDER%", PathUtil::GetGameFolder().c_str());
	strlwr( const_cast<char*>(file.c_str()) );
	sFilename = file;
}

const string* CDefenceData::AddProtectedFile( const char * filename )
{
	if (!GetISystem()->GetIDataProbe())
	{
		NetWarning("No data probe: not adding files");
		return NULL;
	}

#if !defined(LINUX) || 1
	string file = filename;
	UnifyFilename( file );

	TProtectedFiles::iterator lower = m_vProtectedFiles.lower_bound(file);
	if (lower != m_vProtectedFiles.end() && lower->compareNoCase(file) == 0)
		return NULL;
	TProtectedFiles::iterator it = m_vProtectedFiles.insert(lower, file);
	m_listProt.push_back(*it);
	version ++;
	return &*it;
#endif
}

void CDefenceData::ClearProtectedFiles()
{
	m_vProtectedFiles.clear();
	version ++;
}

void CDefenceData::WalkFolderAndAddToVector( const string& toppath, bool recurse, std::vector<string>& where )
{
	ICryPak * pSys = gEnv->pCryPak;

	std::queue<string> searchPaths;
	searchPaths.push( toppath );

	_finddata_t fd;
	while (!searchPaths.empty())
	{
		string path = searchPaths.front();
		searchPaths.pop();

		string search = path + "/*.*";
		intptr_t handle = pSys->FindFirst( search.c_str(), &fd );
		if (handle != -1)
		{
			int res = 0;

			do 
			{
				string filename = path + "/" + fd.name;
				UnifyFilename(filename);

				if (fd.attrib & _A_SUBDIR)
				{
					if (recurse)
						if (0 != strcmp(fd.name, ".") && 0 != strcmp(fd.name, ".."))
							searchPaths.push(filename);
				}
				else
				{
					where.push_back(filename);
				}

				res = pSys->FindNext( handle, &fd );
			}
			while (res >= 0);

			pSys->FindClose( handle );
		}
	}
}

void CDefenceData::FoldersToFiles( const std::map<string, bool>& mapFolders, std::vector<string>& vecFiles )
{
	for (std::map<string, bool>::const_iterator it = mapFolders.begin(); it != mapFolders.end(); ++it)
	{
		WalkFolderAndAddToVector(it->first, it->second, vecFiles);
	}
}

void CDefenceData::Populate()
{
	ClearProtectedFiles();

	level = CNetCVars::Get().CheatProtectionLevel;
	if (!gEnv->bMultiplayer || !gEnv->bServer)
		level = 0;

	if (!level)
		return;

	std::vector<XmlNodeRef> roots;
	// check for a user defined file
	XmlNodeRef nodeRoot = gEnv->pSystem->LoadXmlFile( "Protect.xml" );
	if (nodeRoot)
		roots.push_back(nodeRoot);
	// if that file exists, does it override the system version?
	bool userOverride = false;
	if (nodeRoot && nodeRoot->getAttr("override", userOverride) && userOverride)
		;
	else
	{
		// if not, load the system version if it exists
		nodeRoot = gEnv->pSystem->LoadXmlFile( PathUtil::GetGameFolder() + "/Scripts/Network/Protect.xml" );
		if (nodeRoot)
			roots.push_back(nodeRoot);
	}

	// files we don't want
	std::vector<string> explicitExclusions;
	// folders we don't want
	std::map<string, bool> folderExclusions;
	// wildcards we don't want
	std::vector<string> wildcardExclusions;
	// files we want
	std::vector<string> explicitFiles;
	// folders we want
	std::map<string, bool> explicitFolders;

	// gather the initial data structures
	for (std::vector<XmlNodeRef>::iterator itRoot = roots.begin(); itRoot != roots.end(); ++itRoot)
	{
		nodeRoot = *itRoot;
		int children = nodeRoot->getChildCount();
		for (int i=0; i<children; i++)
		{
			XmlNodeRef node = nodeRoot->getChild(i);

			int nodelevel = 0;
			node->getAttr("level", nodelevel);
			if (nodelevel < 1)
				nodelevel = 1;
			if (nodelevel > CNetCVars::Get().CheatProtectionLevel)
				continue;

			const char * tag = node->getTag();
			if (0 == strcmp(tag, "add"))
			{
				XmlString s;
				if (node->getAttr("file", s) && !s.empty())
				{
					string file = s.c_str();
					UnifyFilename(file);
					explicitFiles.push_back(file);
				}
				else if (node->getAttr("folder", s) && !s.empty())
				{
					bool recurse = true;
					node->getAttr("recurse", recurse);
					string file = s.c_str();
					UnifyFilename(file);
					explicitFolders[file] |= recurse; // only ever become more recursive in the case of dupes
				}
				else
					NetWarning("Invalid add node: %s", node->getXML().c_str());
			}
			else if (0 == strcmp(tag, "exclude"))
			{
				XmlString s;
				if (node->getAttr("file", s) && !s.empty())
				{
					string file = s.c_str();
					UnifyFilename(file);
					explicitExclusions.push_back(file);
				}
				else if (node->getAttr("folder", s) && !s.empty())
				{
					bool recurse = true;
					node->getAttr("recurse", recurse);
					string file = s.c_str();
					UnifyFilename(file);
					folderExclusions[file] |= recurse; // only ever become more recursive in the case of dupes
				}
				else if (node->getAttr("wildcard", s) && !s.empty())
					wildcardExclusions.push_back(s.c_str());
				else
					NetWarning("Invalid add node: %s", node->getXML().c_str());
			}
			else
			{
				NetWarning("Invalid protection tag %s", tag);
			}
		}
	}

	// turn folders into files
	FoldersToFiles(explicitFolders, explicitFiles);
	FoldersToFiles(folderExclusions, explicitExclusions);

	// sort the explicit exclusions for faster rejecting
	std::sort(explicitExclusions.begin(), explicitExclusions.end());

	// now walk through the files we want, rejecting them using the exclusions if needed, and adding them to the main data structure
	for (std::vector<string>::const_iterator itAdd = explicitFiles.begin(); itAdd != explicitFiles.end(); ++itAdd)
	{
		std::vector<string>::const_iterator itExplicitExclude = std::lower_bound( explicitExclusions.begin(), explicitExclusions.end(), *itAdd );
		if (itExplicitExclude != explicitExclusions.end() && *itExplicitExclude == *itAdd)
			continue;
		bool matchesExclusionWildcard = false;
		for (std::vector<string>::const_iterator itWildcard = wildcardExclusions.begin(); !matchesExclusionWildcard && itWildcard != wildcardExclusions.end(); ++itWildcard)
		{
			if (PathUtil::MatchWildcard(itAdd->c_str(), itWildcard->c_str()))
				matchesExclusionWildcard = true;
		}
		if (matchesExclusionWildcard)
			continue;
		AddProtectedFile(itAdd->c_str());
	}
}

#endif //NOT_USE_DEFENCE
