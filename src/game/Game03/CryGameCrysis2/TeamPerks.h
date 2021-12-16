#ifndef __TEAMPERKS_H__
#define __TEAMPERKS_H__

class CPlayer;

enum ETeamPerks
{
	eTeamPerk_Null = -1,
	eTeamPerk_Radar = 0,
	eTeamPerk_SuitBoost,
	eTeamPerk_Last
};

struct STeamPerkData
{
	const char *		m_name;
	const char *		m_announcement;

	STeamPerkData(const char * name, const char * announcement);
};

struct STeamPerkActivationInfo
{
	int m_numReasonsToBeActive;
};

class CTeamPerks
{
public:
	CTeamPerks(void);
	void Update(float fFrameTime);

	ILINE bool IsTeamPerkActive(ETeamPerks perk) const { return m_teamPerkActivationInfo[perk].m_numReasonsToBeActive > 0; }
	void UpdateTeamPerkCount(int teamId, ETeamPerks teamPerkID, EntityId activatorId, int change);

	void TeamPerkStateHasChanged(ETeamPerks teamPerk, int teamNum, EntityId activatorId, bool isNowOn);

	static const STeamPerkData	s_teamPerkList[];

	void GetMemoryUsage( ICrySizer *pSizer ) const { /*nothing*/ }
private:
	STeamPerkActivationInfo m_teamPerkActivationInfo[eTeamPerk_Last];
};

#endif //__TEAMPERKS_H__