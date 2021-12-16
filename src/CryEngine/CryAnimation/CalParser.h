/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
Description: 
-------------------------------------------------------------------------
History:
- 03:06:2009   : Created by Dmythro Tsakhilov

*************************************************************************/
#ifndef CALPARSER_H
#define CALPARSER_H

// information about an animation to load
struct SAnimFile
{
	string m_fileName;
	string m_animName;
	uint32 m_animFlags; // combination of GlobalAnimation internal flags
  DynArray< string > arrAimPoses;
	SAnimFile(const string &fileName, const string &szAnimName, uint32 animflags):
	  m_fileName(fileName), m_animName(szAnimName), m_animFlags(animflags)
	{
		CryStringUtils::UnifyFilePath(m_fileName);
	}

		SAnimFile():m_animFlags(0){}
};

// the animation file array
typedef std::vector<SAnimFile*> TAnimFilesVec;

class CCalParser
{
public:
	CCalParser();	
	bool Parse(const char* calFileName, string animDirName, bool& parseSubFolders);
	~CCalParser(void);
private:
	bool AddIfNewModelTracksDatabase(const char* dataBase);
	int AddIfNewAnimationAlias(const char* animName, const char* animFile, const unsigned animFlags);
	int AddIfNewFacialAnimationAlias(const char* animName, const char* animFile);
public:
	string m_faceLibFile;
	string m_faceLibDir;
	string m_animEventDatabase;
	std::vector<string> m_modelTracksDatabases;
	TAnimFilesVec m_arrAnimFiles;
	TAnimFilesVec m_arrWildcardAnimFiles;
	CAnimationSet::FacialAnimationSet::container_type m_facialAnimations;
private:
	std::vector<string> m_parsedCalFiles;		
};

#endif
