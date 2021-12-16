/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description:	Implementation of the ILevelRotation interface. 
					MiniLevelRotation provides a basic implementation
  
 -------------------------------------------------------------------------
  History:
  - 24:2:2010	15:30 : Created by Christian Helmich

*************************************************************************/

#include "StdAfx.h"
#include "MiniLevelRotation.h"

#include <IGameFramework.h>
#include <IPlayerProfiles.h>
#include <IGameRulesSystem.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// ctor & dtor

simpleframework::CMiniLevelRotation::CMiniLevelRotation()
{
	LOG_CODE_COVERAGE();
}

//-----------------------------------

simpleframework::CMiniLevelRotation::~CMiniLevelRotation()
{
	LOG_CODE_COVERAGE();
}

// ctor & dtor/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// ILevelRotation implementation

//- load/save

bool simpleframework::CMiniLevelRotation::Load( ILevelRotationFile* file )
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(file);
	CRY_SAFE_RETURN(!file, false);

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, false);

	CRY_ASSERT(gEnv->pGame);
	CRY_SAFE_RETURN(!gEnv->pGame, false);

	IGameFramework*	pGameFramework = gEnv->pGame->GetIGameFramework();
	CRY_ASSERT(pGameFramework);
	CRY_SAFE_RETURN(!pGameFramework, false);

	ILevelSystem*	pLevelSystem = pGameFramework->GetILevelSystem();
	CRY_ASSERT(pLevelSystem);
	CRY_SAFE_RETURN(!pLevelSystem, false);	

	XmlNodeRef rootNode = file->Load();
	CRY_SAFE_RETURN(!rootNode,false);
	
	bool	bHasLevelRotationTag = !stricmp(rootNode->getTag(), "levelrotation");
	CRY_SAFE_RETURN(!bHasLevelRotationTag, false);
	
	Reset();

	bool	bRandomize;
	if(rootNode->getAttr("randomize", bRandomize))
		SetRandom(bRandomize);

	int rootNodeChildCount = rootNode->getChildCount();
	for (int i = 0; i < rootNodeChildCount; ++i)
	{
		XmlNodeRef child = rootNode->getChild(i);
		if (child && !stricmp(child->getTag(), "level"))
		{
			const char* levelName = child->getAttr("name");
			if (!levelName || !levelName[0])
				continue;

			if(!pLevelSystem->GetLevelInfo(levelName)) //skipping non-present levels
				continue;

			const char* gameRulesName = child->getAttr("gamerules");
			if (!gameRulesName || !gameRulesName[0])
				gameRulesName=0;

			int lvl = AddLevel(levelName, gameRulesName);
			int subChildCount = child->getChildCount();
			for(int j = 0; j < subChildCount; ++j)
			{
				XmlNodeRef s = child->getChild(j);
				if(s && !stricmp(s->getTag(), "setting"))
				{
					const char *setting = s->getAttr("setting");
					AddSetting(lvl, setting);
				}
			}
		}
	}

	if(IsRandom())
		Shuffle();
	return true;	
}

//-----------------------------------

bool simpleframework::CMiniLevelRotation::Save( ILevelRotationFile* file )
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(file);
	CRY_SAFE_RETURN(!file, false);

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, false);

	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, false);	

	XmlNodeRef rootNode = gEnv->pSystem->CreateXmlNode("levelRotation");
	CRY_ASSERT(rootNode);
	CRY_SAFE_RETURN(!rootNode, false);

	rootNode->setAttr("randomize", m_random);
	
	int nLevelRotations = m_rotation.size();
	for (int i = 0; i < nLevelRotations; ++i)
	{
		SLevelRotationEntry &entry = m_rotation[i];

		XmlNodeRef level = gEnv->pSystem->CreateXmlNode("level");
		CRY_ASSERT(level);
		CRY_SAFE_RETURN(!level, false);

		level->setAttr("name", entry.levelName.c_str());
		if (!entry.gameRulesName.empty())
			level->setAttr("gameRules", entry.gameRulesName.c_str());

		int nEntrySettings = entry.settings.size();
		for(int j = 0; j < nEntrySettings; ++j)
		{
			XmlNodeRef s = gEnv->pSystem->CreateXmlNode("setting");
			CRY_ASSERT(s);
			CRY_SAFE_RETURN(!s, false);

			s->setAttr("setting", entry.settings[j].c_str());
			level->addChild(s);
		}

		rootNode->addChild(level);
	}

	return file->Save(rootNode);
}

//- load/save/
//////////////////////////////////////////////////////////////////////////
//- rotation modification

void simpleframework::CMiniLevelRotation::Reset()
{
	LOG_CODE_COVERAGE();
	m_random = false;
	m_next = 0;
	m_rotation.resize(0);
	m_shuffle.resize(0);
}

//-----------------------------------

int simpleframework::CMiniLevelRotation::AddLevel( const char* level, const char* gamerules )
{
	LOG_CODE_COVERAGE();
	CRY_ASSERT(level);
	CRY_ASSERT(gamerules);

	SLevelRotationEntry entry;
	entry.levelName = level;
	if (gamerules)
		entry.gameRulesName = gamerules;

	int idx = m_rotation.size();
	m_rotation.push_back(entry);
	if(m_random)
		m_shuffle.push_back(idx);
	return idx;
}

//-----------------------------------

void simpleframework::CMiniLevelRotation::AddSetting( int level, const char* setting )
{
	LOG_CODE_COVERAGE();
	CRY_ASSERT(setting);
	CRY_SAFE_RETURN(!setting, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(level >= 0 && level < m_rotation.size());
	m_rotation[level].settings.push_back(setting);
}

//- rotation modification/
//////////////////////////////////////////////////////////////////////////
//- iteration through levels

bool simpleframework::CMiniLevelRotation::First()
{
	LOG_CODE_COVERAGE();
	m_next = 0;
	if(m_random)
		Shuffle();
	return !m_rotation.empty();
}

//-----------------------------------

bool simpleframework::CMiniLevelRotation::Advance()
{
	LOG_CODE_COVERAGE();
	++m_next;
	if (m_next >= m_rotation.size())
		return false;
	return true;
}

//- iteration through levels/
//////////////////////////////////////////////////////////////////////////
//- next data

const char* simpleframework::CMiniLevelRotation::GetNextLevel() const
{
	LOG_CODE_COVERAGE();
	if (m_next >= 0 && m_next < m_rotation.size())
	{
		int next = m_random ? 
			m_shuffle[m_next] :
			m_next;
		return m_rotation[next].levelName.c_str();
	}
	return 0;
}

//-----------------------------------

const char* simpleframework::CMiniLevelRotation::GetNextGameRules() const
{
	LOG_CODE_COVERAGE();
	if (m_next >= 0 && m_next < m_rotation.size())
	{
		int next = m_random ?
			m_shuffle[m_next] :
			m_next;
		return m_rotation[next].gameRulesName.c_str();
	}
	return 0;
}

//-----------------------------------

int simpleframework::CMiniLevelRotation::GetNextSettingsNum() const
{
	LOG_CODE_COVERAGE();
	if (m_next >= 0 && m_next < m_rotation.size())
	{
		int next = m_random ?
			m_shuffle[m_next] :
			m_next;    
		return m_rotation[next].settings.size();
	}
	return 0;
}

//-----------------------------------

const char* simpleframework::CMiniLevelRotation::GetNextSetting( int idx )
{
	LOG_CODE_COVERAGE();
	if (m_next >= 0 && m_next < m_rotation.size())
	{
		int next = m_random ?
			m_shuffle[m_next] :
			m_next;    
		
		return (idx >= 0 && idx < m_rotation[next].settings.size()) ? 
			m_rotation[next].settings[idx].c_str() :
			0;
	}
	return 0;
}

//-----------------------------------
//-----------------------------------

int simpleframework::CMiniLevelRotation::GetLength() const
{
	LOG_CODE_COVERAGE();
	return (int)m_rotation.size();
}

//-----------------------------------

int simpleframework::CMiniLevelRotation::GetNext() const
{
	LOG_CODE_COVERAGE();
	return m_next;
}

//- next data/
//////////////////////////////////////////////////////////////////////////
//- randomizer

void simpleframework::CMiniLevelRotation::SetRandom( bool rnd )
{
	LOG_CODE_COVERAGE();
	m_random = rnd;
	if(m_random)
		Shuffle();
}

//-----------------------------------

bool simpleframework::CMiniLevelRotation::IsRandom() const
{
	LOG_CODE_COVERAGE();
	return m_random;
}

//- randomizer/
//////////////////////////////////////////////////////////////////////////
//- change level

void simpleframework::CMiniLevelRotation::ChangeLevel(IConsoleCmdArgs* pArgs)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pConsole);
	CRY_SAFE_RETURN(!gEnv->pConsole, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pGame);
	CRY_SAFE_RETURN(!gEnv->pGame, CRY_NO_RETURN_VALUE);

	IGameFramework*	pGameFramework = gEnv->pGame->GetIGameFramework();
	CRY_ASSERT(pGameFramework);
	CRY_SAFE_RETURN(!pGameFramework, CRY_NO_RETURN_VALUE);

	IEntity* pCurrentGameRulesEntity = NULL;
	IGameRulesSystem* pGameRulesSys = pGameFramework->GetIGameRulesSystem();
	//CRY_ASSERT(pGameRulesSys);
	if(pGameRulesSys)
		pCurrentGameRulesEntity = pGameRulesSys->GetCurrentGameRulesEntity();
	
	ILevelInfo* pNextLevelInfo = NULL;
	ILevelSystem* pLevelSystem = pGameFramework->GetILevelSystem();
	//CRY_ASSERT(pLevelSystem);
	if(pLevelSystem)
		pNextLevelInfo = pLevelSystem->GetLevelInfo(GetNextLevel());

	SGameContextParams ctx;
	const char* nextGameRules = GetNextGameRules();

	if (nextGameRules && nextGameRules[0])
		ctx.gameRules = nextGameRules;
	else if (pCurrentGameRulesEntity)
		ctx.gameRules = pCurrentGameRulesEntity->GetClass()->GetName();
	else if (pNextLevelInfo)
		ctx.gameRules = pNextLevelInfo->GetDefaultGameType()->name;

	ctx.levelName = GetNextLevel();

	int nNextSettings = GetNextSettingsNum();
	for(int i = 0; i < nNextSettings; ++i)
	{
		gEnv->pConsole->ExecuteString(GetNextSetting(i));
	}

	if (pGameFramework->StartedGameContext())
	{
		pGameFramework->ChangeGameContext(&ctx);
	}
	else
	{
		gEnv->pConsole->ExecuteString( string("sv_gamerules ") + ctx.gameRules );

		string command = string("map ") + ctx.levelName;
		if (pArgs)
		{
			for (int i = 1; i < pArgs->GetArgCount(); ++i)
			{
				command += string(" ") + pArgs->GetArg(i);
			}
		}
		command += " s";
		gEnv->pConsole->ExecuteString(command);
	}
	
	if (!Advance())
		First();
}

//- change level/
// ILevelRotation implementation
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//- utility

void simpleframework::CMiniLevelRotation::Shuffle()
{
	LOG_CODE_COVERAGE();
	m_shuffle.resize(m_rotation.size());
	if(m_rotation.empty())
		return;

	int nShuffle = m_shuffle.size();		
	for(int i = 0; i < nShuffle; ++i)
	{
		m_shuffle[i] = i;
		int idx = rand() % nShuffle;
		std::swap(m_shuffle[i], m_shuffle[idx]);
	}
}

//- utility/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include UNIQUE_VIRTUAL_WRAPPER(ILevelRotation)

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////