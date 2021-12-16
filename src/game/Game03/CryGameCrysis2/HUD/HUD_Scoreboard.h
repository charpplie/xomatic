#ifndef __HUD_SCOREBOARD_H__
#define __HUD_SCOREBOARD_H__

#include "HUDObject.h"

//-----------------------------------------------------------------------------------------------------

class CHUD_Scoreboard : public CHUDObject
{
public:

	CHUD_Scoreboard();
	virtual ~CHUD_Scoreboard();

	void Init();
	void Update(float frameTime);
	void Draw();
	void Reload();

	void OnHUDEvent(const SHUDEvent& event);

private:

	enum EHUDScoreboardGamemode
	{
		eHSG_InstantAction = 0,
		eHSG_TeamInstantAction,
		eHSG_CaptureTheFlag,
		eHSG_CrashSite,
		eHSG_PowerStruggleLite,
		eHSG_Extraction,
		eHSG_AllOrNothing,
		eHSG_Assault,
		eHSG_Countdown,
	};

	struct SPlayerScoreEntry
	{
		SPlayerScoreEntry()
		{
			playerId = 0;
			playerName = 0;
			rank = 0;
			score = 0;
			kills = 0;
			deaths = 0;
			assists = 0;
			gamemodeData1 = 0;
			isDead = false;
		};

		EntityId playerId;
		int rank;
		int score;
		int kills;
		int deaths;
		int assists;
		int gamemodeData1;
		//int gamemodeData2; etc..
		const char *playerName;
		bool isDead;
	};

	void UpdateTeamGame(IHUDAsset *scoreboard, EntityId localActorId);
	void UpdateNonTeamGame(IHUDAsset *scoreboard, EntityId localActorId);
	void GetGamemodeSpecificPlayerData(SPlayerScoreEntry *scoreEntry);
	void GetGamemodeSpecificTeamScores(CGameRules *pGameRules, int &team1Score, int &team2Score);

	int m_teamScore[2];
	int m_teamRoundScore[2];
	
	int m_roundNumber;
	int m_scoreLimit;
	int m_roundLimit;
	
	EHUDScoreboardGamemode m_gamemode;

	float m_timeLimit;
	float m_lastUpdateTime;
	
	bool m_isSuddenDeath;
	bool m_bVisible;

	static bool CompareScores(const SPlayerScoreEntry &elem1, const SPlayerScoreEntry &elem2);
};


#endif // ~__HUD_SCOREBOARD_H__

