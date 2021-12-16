/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios
-------------------------------------------------------------------------
History:
- 10:12:2009		Created by Ben Parbury
*************************************************************************/

#include "StdAfx.h"
#include "SuitModeProgression.h"

#include "Player.h"
#include "Utility/CryWatch.h"
#include "Utility/DesignerWarning.h"

int CSuitModeProgression::s_xpRequired[k_maxPossibleLevels];
int CSuitModeProgression::s_maxLevel = 0;

CSuitModeProgression::CSuitModeProgression()
{
	memset(s_xpRequired, 0, sizeof(s_xpRequired));

	m_xp = 0;
	m_gameStartXp = 0;
	m_level = 0;
	m_gameStartLevel = 0;
}

CSuitModeProgression::~CSuitModeProgression()
{
	
}

void CSuitModeProgression::Init(int xp)
{
	CRY_ASSERT_MESSAGE(xp >= 0, "Less than minimum xp on Init");
	m_xp = xp;
	m_gameStartXp = m_xp;
	m_level = CalculateLevelFromXp(m_xp);
	m_gameStartLevel = m_level;
}

//static
void CSuitModeProgression::InitSharedSuitLevels(const char* filename)
{
	XmlNodeRef xml = GetISystem()->LoadXmlFile( filename );

	if(xml)
	{
		const int childCount = xml->getChildCount();

		DesignerWarning(childCount < k_maxPossibleLevels, "There should be '%d' levels not '%d'", k_maxPossibleLevels, childCount);
		s_maxLevel = childCount;

		for (int iChild = 0; iChild < childCount; ++iChild)
		{
			XmlNodeRef childXML = xml->getChild(iChild);

			CRY_ASSERT_MESSAGE(childXML->haveAttr("xpRequired"), "All levels should have required xp");

			childXML->getAttr("xpRequired", s_xpRequired[iChild]);
		}

		SanityCheckSuitLevels();

	}
}

//static
void CSuitModeProgression::SanityCheckSuitLevels()
{
	for(int i = 1; i < s_maxLevel; i++)
	{
		DesignerWarning(s_xpRequired[i - 1] < s_xpRequired[i], "Level %d needs more xp than Level %d", i - 1, i);
	}
}

void CSuitModeProgression::OnGameStart()
{
	m_gameStartXp = m_xp;
	m_gameStartLevel = m_level;
}

void CSuitModeProgression::OnGameEnd(int amount)
{
	CRY_ASSERT_MESSAGE(amount >= 0, "invalid SuitModeProgression amount");
	m_xp += amount;

	if(m_level != s_maxLevel - 1 && m_xp >= s_xpRequired[m_level + 1])
	{
		m_level = CalculateLevelFromXp(m_xp);
	}
}

//static
int CSuitModeProgression::CalculateLevelFromXp(const int xp)
{
	for(int i = 0; i < s_maxLevel - 1; i++)
	{
		if(xp >= s_xpRequired[i] && xp < s_xpRequired[i + 1])
		{
			return i;
		}
	}

	if(xp >= s_xpRequired[s_maxLevel - 1])
	{
		return s_maxLevel - 1;
	}

	CRY_ASSERT_MESSAGE(false, "Failed to CalculateLevelFromXp");
	return 0;
}

const int CSuitModeProgression::GetData(EPPSuitData dataType)
{
	switch(dataType)
	{
		//Ala Player Progression values go from 0 to k_maxLevel - 1
		//However outside world should see 1 to k_maxLevel
	case ePPS_Level:
		return m_level + 1;

	case ePPS_MaxLevel:
		return s_maxLevel + 1;

	case ePPS_XP:
		return m_xp;

	case ePPS_XPToNextLevel:
		return (m_level != s_maxLevel - 1) ? s_xpRequired[m_level + 1] - m_xp : 0;

	case ePPS_XPLastMatch:
		return m_xp - m_gameStartXp;

	case ePPS_XPInCurrentLevel:
		return m_xp - s_xpRequired[m_level];

	case ePPS_MatchStartLevel:
		return m_gameStartLevel + 1;

	case ePPS_MatchStartXPInCurrentLevel:
		return m_gameStartXp - s_xpRequired[m_gameStartLevel];

	case ePPS_MatchStartXPToNextLevel:
		return (m_gameStartLevel != s_maxLevel - 1) ? s_xpRequired[m_gameStartLevel + 1] - m_gameStartXp : 0;

	default:
		CRY_ASSERT_MESSAGE(false, "Unable to find data type in CSuitModeProgression::GetData");
		return -1;
	}
}

void CSuitModeProgression::ResetXP()
{
	m_xp = 0;
	m_gameStartXp = 0;
	m_level = 0;
	m_gameStartLevel = 0;
}

void CSuitModeProgression::DebugWatch()
{
	int xpGained = m_xp - m_gameStartXp;
	CryWatch("	XP %d", m_xp);
	CryWatch("	XP Gained this match %d", xpGained);
	CryWatch("	Level %d", m_level);
	if(m_level != s_maxLevel - 1)
	{
		CryWatch("	Next Level %d xp", s_xpRequired[m_level+1] - m_xp);
	}
	else
	{
		CryWatch("	MaxLevel!");
	}
}
