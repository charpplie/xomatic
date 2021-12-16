/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description:	Implementation of the ILevelInfo interface. 
					MiniLevelInfo provides a basic implementation
  
 -------------------------------------------------------------------------
  History:
  - 24:2:2010	15:30 : Created by Christian Helmich

*************************************************************************/

#include "StdAfx.h"
#include "MiniLevelInfo.h"


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// ctor & dtor

simpleframework::CMiniLevelInfo::CMiniLevelInfo() :
	m_heightmapSize(0)
{
	LOG_CODE_COVERAGE();
}

//-----------------------------------

simpleframework::CMiniLevelInfo::~CMiniLevelInfo()
{
	LOG_CODE_COVERAGE();
}

// ctor & dtor/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//ILevelInfo implementation

//- level data

const char* simpleframework::CMiniLevelInfo::GetName() const
{
	LOG_CODE_COVERAGE();
	return m_levelName.c_str();
}

//-----------------------------------

const char* simpleframework::CMiniLevelInfo::GetPath() const
{
	LOG_CODE_COVERAGE();
	return m_levelPath.c_str();
}

//-----------------------------------

const char* simpleframework::CMiniLevelInfo::GetPaks() const
{
	LOG_CODE_COVERAGE();
	return m_levelPaks.c_str();
}

//-----------------------------------

const char* simpleframework::CMiniLevelInfo::GetDisplayName() const
{
	LOG_CODE_COVERAGE();
	return m_levelDisplayName.c_str();
}

//-----------------------------------

const ILevelInfo::TStringVec& simpleframework::CMiniLevelInfo::GetMusicLibs() const
{
	LOG_CODE_COVERAGE();
	return m_musicLibs;
}

//-----------------------------------

int simpleframework::CMiniLevelInfo::GetHeightmapSize() const
{
	LOG_CODE_COVERAGE();
	return m_heightmapSize;
}

//- level data/
//////////////////////////////////////////////////////////////////////////
//- game type

int simpleframework::CMiniLevelInfo::GetGameTypeCount() const
{
	LOG_CODE_COVERAGE();
	return m_gameTypes.size();
}

//-----------------------------------

const ILevelInfo::TGameTypeInfo* simpleframework::CMiniLevelInfo::GetGameType( int gameType ) const
{
	LOG_CODE_COVERAGE();
	return &m_gameTypes[gameType];
}

//-----------------------------------

const ILevelInfo::TGameTypeInfo* simpleframework::CMiniLevelInfo::GetDefaultGameType() const
{
	LOG_CODE_COVERAGE();
	return (!m_gameTypes.empty()) ? 
		&m_gameTypes[0] : 
		0;
}

//-----------------------------------

bool simpleframework::CMiniLevelInfo::SupportsGameType(const char* gameTypeName) const
{
	LOG_CODE_COVERAGE();
	
	CRY_ASSERT(gameTypeName);
	CRY_SAFE_RETURN(!gameTypeName, false);

	//read level meta data
	int nGamerules = m_gamerules.size();
	for(int i = 0; i < nGamerules; ++i)
	{
		if(!stricmp(m_gamerules.at(i).c_str(), gameTypeName))
			return true;
	}
	return false;
}

//- game type/
//////////////////////////////////////////////////////////////////////////
//- memory

void simpleframework::CMiniLevelInfo::GetMemoryStatistics(ICrySizer* s)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(s);
	CRY_SAFE_RETURN(!s, CRY_NO_RETURN_VALUE);

	s->Add(m_levelName);
	s->Add(m_levelPath);
	s->Add(m_levelPaks);

	s->AddContainer(m_musicLibs);
	int nMusicLibs = m_musicLibs.size();	
	for (int m = 0; m < nMusicLibs; ++m)
	{	
		s->Add( m_musicLibs.at(m) );
	}

	s->AddContainer(m_gameTypes);
	int nGameTypes = m_gameTypes.size();	
	for (int g = 0; g < nGameTypes; ++g)
	{
		s->Add(m_gameTypes.at(g).name);
		s->Add(m_gameTypes.at(g).xmlFile);
	}
}

//- memory/
//////////////////////////////////////////////////////////////////////////
//- utility

void simpleframework::CMiniLevelInfo::ReadMetaData()
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pCryPak);
	CRY_SAFE_RETURN(!gEnv->pCryPak, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, CRY_NO_RETURN_VALUE);

	string	fullPath(GetPath());
	int		slashPos = fullPath.rfind('\\');
	
	if(slashPos == -1)
		slashPos = fullPath.rfind('/');
	
	string mapName = fullPath.substr(slashPos+1, fullPath.length()-slashPos);
	fullPath.append("/");
	fullPath.append(mapName);
	fullPath.append(".xml");

	if (!gEnv->pCryPak->IsFileExist(fullPath.c_str()))
		return;

	XmlNodeRef mapInfo = gEnv->pSystem->LoadXmlFile(fullPath.c_str());
	CRY_SAFE_RETURN(!mapInfo, CRY_NO_RETURN_VALUE);

	//retrieve the coordinates of the map
	int mapInfoChildCount = mapInfo->getChildCount();
	for(int n = 0; n < mapInfoChildCount; ++n)
	{
		XmlNodeRef rulesNode = mapInfo->getChild(n);
		const char* name = rulesNode->getTag();
		if(!stricmp(name, "Gamerules"))
		{
			int numAttbs = rulesNode->getNumAttributes();
			for(int a = 0; a < numAttbs; ++a)
			{
				const char* key;
				const char* value;
				rulesNode->getAttributeByIndex(a, &key, &value);
				m_gamerules.push_back(value);
			}
		}
		if(!stricmp(name,"Display"))
		{
			XmlString v;
			if(rulesNode->getAttr("Name",v))
				m_levelDisplayName = v.c_str();
		}
	}	
}

//-----------------------------------

bool simpleframework::CMiniLevelInfo::ReadInfo()
{
	LOG_CODE_COVERAGE();
	
	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, false);

	CRY_ASSERT(gEnv->pCryPak);
	CRY_SAFE_RETURN(!gEnv->pCryPak, false);

	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, false);

	string levelPath = m_levelPath;
	string paks = levelPath + string("/*.pak");

	bool bOpenSuccess = gEnv->pCryPak->OpenPacks(paks.c_str(), (unsigned int)0);
	CRY_ASSERT(bOpenSuccess);
	CRY_SAFE_RETURN(!bOpenSuccess, false);

	string xmlFile = levelPath + string("/LevelInfo.xml");
	XmlNodeRef rootNode = gEnv->pSystem->LoadXmlFile(xmlFile.c_str());
	CRY_SAFE_RETURN(!rootNode, false);

	string name = m_levelName;
	m_heightmapSize = atoi(rootNode->getAttr("HeightmapSize"));

	string dataFile = levelPath + string("/LevelDataAction.xml");
	XmlNodeRef dataNode = gEnv->pSystem->LoadXmlFile(dataFile.c_str());
	if(!dataNode)
	{
		dataFile = levelPath + string("/LevelData.xml");
		dataNode = gEnv->pSystem->LoadXmlFile(dataFile.c_str());
	}
	CRY_SAFE_RETURN(!dataNode, false);	//quit here if still not contained

	XmlNodeRef gameTypesNode = dataNode->findChild("Missions");
	int gameTypesNodeChildCount = gameTypesNode->getChildCount();
	if ((gameTypesNode!=0) && (gameTypesNodeChildCount > 0))
	{
		for (int i = 0; i < gameTypesNodeChildCount; ++i)
		{
			XmlNodeRef gameTypeNode = gameTypesNode->getChild(i);
			if (gameTypeNode->isTag("Mission"))
			{
				const char* gameTypeName = gameTypeNode->getAttr("Name");
				if (gameTypeName)
				{
					ILevelInfo::TGameTypeInfo info;
					info.cgfCount = 0;
					gameTypeNode->getAttr("CGFCount", info.cgfCount);
					info.name = gameTypeNode->getAttr("Name");
					info.xmlFile = gameTypeNode->getAttr("File");
					m_gameTypes.push_back(info);
				}
			}
		}

		XmlNodeRef musicLibraryNode = dataNode->findChild("MusicLibrary");
		int musicLibChildCount = musicLibraryNode->getChildCount();
		if ((musicLibraryNode!=0) && (musicLibChildCount > 0))
		{
			for (int i = 0; i < musicLibChildCount; ++i)
			{
				XmlNodeRef musicLibrary = musicLibraryNode->getChild(i);
				if (musicLibrary->isTag("Library"))
				{
					const char *musicLibraryName = musicLibrary->getAttr("File");
					if (musicLibraryName)
					{
						m_musicLibs.push_back(string("music/") + musicLibraryName);
					}
				}
			}
		}
	}

	gEnv->pCryPak->ClosePacks(paks.c_str());
	return true;
}

//- utility/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include UNIQUE_VIRTUAL_WRAPPER(ILevelInfo)

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////