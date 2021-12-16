#ifndef __IPERK_H__
#define __IPERK_H__

#include "Perk.h"
#include "PlayerPlugin.h"

class IPerk : public CPlayerPlugin
{
public:
	IPerk();

	void SetPerkID(EPerks perkId);

	enum EPerkTier
	{
		eTierInvalid = 0,
		eTierOne,
		eTierTwo,
		eTierThree,
		eTierMax,
	};

	void SetTier(EPerkTier tier);
	ILINE EPerkTier GetTier() const { return m_tier; }
	bool IsTier(EPerkTier tier) const;

protected:
	EPerks m_perkId;
	EPerkTier	m_tier;

#if PLAYER_PLUGIN_DEBUGGING
	const char * DbgGetPerkName();
	virtual string DbgGetClassDetails() { return string().Format("%s:%s", DbgGetPluginName(), DbgGetPerkName()); }
	virtual void Leave();
	virtual void Enter();
#endif
};

#endif // __IPERK_H__
