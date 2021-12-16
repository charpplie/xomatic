/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios
-------------------------------------------------------------------------
History:
- 10:12:2009		Created by Ben Parbury
*************************************************************************/

#ifndef __SUITMODEPROGRESSION_H__
#define __SUITMODEPROGRESSION_H__

enum EPPSuitData
{
	ePPS_Level = 0,
	ePPS_MaxLevel,
	ePPS_XP,
	ePPS_XPToNextLevel,
	ePPS_XPLastMatch,
	ePPS_XPInCurrentLevel,
	ePPS_MatchStartLevel,
	ePPS_MatchStartXPInCurrentLevel,
	ePPS_MatchStartXPToNextLevel,
};

class CSuitModeProgression
{
public:
	CSuitModeProgression();
	virtual ~CSuitModeProgression();

	void Init(int xp);
	static void InitSharedSuitLevels(const char* filename);

	void OnGameStart();
	void OnGameEnd(int amount);

	const int GetData(EPPSuitData dataType);
	
	void ResetXP();

	void DebugWatch();
protected:
	static int CalculateLevelFromXp(const int xp);
	static void SanityCheckSuitLevels();

	const static int k_maxPossibleLevels = 20;
	static int s_xpRequired[k_maxPossibleLevels];
	static int s_maxLevel;

	int m_xp;
	int m_level;
	int m_gameStartLevel;
	int m_gameStartXp;
};

#endif // __SUITMODEPROGRESSION_H__