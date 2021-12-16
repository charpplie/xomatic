#ifndef __PLAYERPLUGIN_PERK_MODIFYVALUES_H__
#define __PLAYERPLUGIN_PERK_MODIFYVALUES_H__

#include "IPerk.h"

class CPlayerPlugin_Perk_WeaponsTraining : public IPerk
{
	public:
	SET_PLAYER_PLUGIN_NAME(CPlayerPlugin_Perk_WeaponsTraining);

	private:
	void InformActiveHasChanged();
};

class CPlayerPlugin_Perk_SuperStrength : public IPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(CPlayerPlugin_Perk_SuperStrength);

private:
	void InformActiveHasChanged();
};

class CPlayerPlugin_Perk_WeaponsSpread : public IPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(CPlayerPlugin_Perk_WeaponsSpread);

private:
	void InformActiveHasChanged();
};

#endif	// __PLAYERPLUGIN_PERK_MODIFYVALUES_H__