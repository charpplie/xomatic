#include "StdAfx.h"
#include "PlayerPlugin_ScoreRewards.h"
#include "Player.h"
#include "PerkDbgDisplay.h"
#include "Utility/DesignerWarning.h"

CScoreRewardTable * CScoreRewardTable::s_instance = NULL;

//--------------------------------------------------------------------------------------------
// CScoreRewardTable: singleton class containing list of scores and rewards
//--------------------------------------------------------------------------------------------

CScoreRewardTable::CScoreRewardTable() :
	ILoadingMessageProvider(& m_messageProviderListNode)
{
	assert(s_instance == NULL);
	s_instance = this;

	m_numEntries = 0;
	memset (m_entries, 0, sizeof(m_entries));
}

CScoreRewardTable::~CScoreRewardTable()
{
	assert(s_instance == this);
	s_instance = NULL;
}

void CScoreRewardTable::Init(const char* xmlFilename)
{
	XmlNodeRef  theXML = GetISystem()->LoadXmlFile(xmlFilename);

	int  numXmlChildren = theXML->getChildCount();

	CRY_ASSERT_MESSAGE (m_numEntries == 0, "Score reward table being initialised more than once! Contents will be combined; this probably isn't what you want!");

	for (int i = 0; i < numXmlChildren; ++ i)
	{
		XmlNodeRef xmlChild = theXML->getChild(i);

		if (m_numEntries < k_maxEntries && 0 == stricmp(xmlChild->getTag(), "Reward"))
		{	
			xmlChild->getAttr("pointsWithoutDying", m_entries[m_numEntries].m_numberOfPointsWithoutDying);
			const char * perkName = xmlChild->getAttr("givePerk");

#ifndef _RELEASE
			int lastScoreRead = m_numEntries ? m_entries[m_numEntries - 1].m_numberOfPointsWithoutDying : 0;
			DesignerWarning (m_entries[m_numEntries].m_numberOfPointsWithoutDying > lastScoreRead, string().Format("Error in XML file! While reading score rewards entry number %d (\"%s\"), pointsWithoutDying should have been greater than %d but it was %d. Please list the rewards in ascending order!", i, perkName, lastScoreRead, m_entries[m_numEntries].m_numberOfPointsWithoutDying));
#endif

			m_entries[m_numEntries].m_givePerk = CPerk::GetInstance()->FindPerkNumberByName(perkName);
			DesignerWarning (m_entries[m_numEntries].m_givePerk != ePerk_Null, string().Format("Error in XML file! While reading score rewards entry number %d, givePerk=\"%s\" is not the name of a perk.", i, perkName));

			CryLog ("Added score reward table entry #%d: %d points without dying gives perk '%s' %d", m_numEntries, m_entries[m_numEntries].m_numberOfPointsWithoutDying, perkName, m_entries[m_numEntries].m_givePerk);

			++ m_numEntries;
		}
	}
}

void CScoreRewardTable::CheckPassedThreshold(CPlayer * player, int & nextThreshold, int pointsWithoutDying) const
{
	while (nextThreshold < m_numEntries && pointsWithoutDying >= m_entries[nextThreshold].m_numberOfPointsWithoutDying)
	{
		CryLog ("Local player %s has got %d points without dying, passed threshold of %d (table entry %d), getting perk %d", player->GetEntity()->GetName(), pointsWithoutDying, m_entries[nextThreshold].m_numberOfPointsWithoutDying, nextThreshold, m_entries[nextThreshold].m_givePerk);
		player->SetPerkActive(m_entries[nextThreshold].m_givePerk);
		++ nextThreshold;
	}
}

int CScoreRewardTable::GetNumMessagesProvided() const
{
	return m_numEntries;
}

string CScoreRewardTable::GetMessageNum(int n) const
{
	assert (n >= 0 && n < m_numEntries);

	const SPerkData * data = CPerk::GetInstance()->GetPerkData(m_entries[n].m_givePerk);
	return string().Format("Score %d points without dying to unlock the $3%s$o reward!", m_entries[n].m_numberOfPointsWithoutDying, data->GetShowOnScreenName().c_str());
}

//--------------------------------------------------------------------------------------------
// CPlayerPlugin_ScoreReward: class keeping track of the current score of a single player
//--------------------------------------------------------------------------------------------

CPlayerPlugin_ScoreRewards::CPlayerPlugin_ScoreRewards()
{
	m_scoreWhenLastDied = 0;
	m_currentScore = 0;
}

void CPlayerPlugin_ScoreRewards::Update(float dt)
{
	PlayerPluginWatch ("Score = %d, score when last died = %d", m_currentScore, m_scoreWhenLastDied);
}

void CPlayerPlugin_ScoreRewards::HandleEvent(EPlayerPlugInEvent theEvent, void * data)
{
	CPlayerPlugin::HandleEvent(theEvent, data);

	switch (theEvent)
	{
		case EPE_Reset:
		m_currentScore = m_scoreWhenLastDied = m_nextScoreThresholdToHit = 0;
		break;

		case EPE_Die:
		m_scoreWhenLastDied = m_currentScore;
		m_nextScoreThresholdToHit = 0;
		break;

		case EPE_ClientGainedXP:
		{
			int scoreAmount = *(int*)(data);
			m_currentScore += scoreAmount;

			if (m_currentScore < m_scoreWhenLastDied)
			{
				// Special case... if you get your score lower than its starting point, also lower where you have to _start_ from to get past the reward
				// thresholds. Otherwise if you get -2 points you'll need to get those points BACK before heading for a threshold of, for example, 3 points,
				// and in such a situation you'd actually benefit from dying because it would reset your score difference. Not what we want! [TF]
				m_scoreWhenLastDied = m_currentScore;
			}
			else
			{
				CScoreRewardTable::GetInstance()->CheckPassedThreshold(m_ownerPlayer, m_nextScoreThresholdToHit, m_currentScore - m_scoreWhenLastDied);
			}
		}
		break;
	}
}
