/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Equipment Loadouts for C2MP

-------------------------------------------------------------------------
History:
- 18:09:2009  : Created by Ben Johnson

*************************************************************************/

#ifndef __EQUIPMENT_LOADOUT_H__
#define __EQUIPMENT_LOADOUT_H__

# pragma once

#include "GameRules.h"
#include "GameRulesModules/IGameRulesClientConnectionListener.h"
#include "PlayerProgression.h"

#define ATTACHMENT_SLOTS_NUM 3

#define ALLOW_USE_SERVER_OVERRIDE 1
#define MAX_SERVER_OVERRIDE_PACKAGES 10

struct IFlashPlayer;

class CEquipmentLoadout : public IGameRulesClientConnectionListener
{
public:

	enum EEquipmentLoadoutFlags
	{
		ELOF_NONE											= 0,
		ELOF_HAS_CHANGED							= (1<<0),		// Var to indicate a loadout has changed since last being sent.
		ELOF_HAS_BEEN_SET_ON_SERVER		= (1<<1),		// On the server. Has the loadout ever been set for a client - for use in m_clientLoadouts 
		ELOF_CAN_BE_CUSTOMIZED				= (1<<2),		// Whether the loadout can be customized in the menus
	};

	enum EEquipmentLoadoutCategory
	{
		eELC_NONE             = 0,
		eELC_PRIMARY_WEAPON   = 1,
		eELC_SECONDARY_WEAPON = 2,
		eELC_EXPLOSIVE        = 3,
		eELC_ATTACHMENT       = 4,
		eELC_PERK             = 5,
		eELC_SIZE             = 6
	};

	CEquipmentLoadout();
	~CEquipmentLoadout();
	
	// IGameRulesClientConnectionListener
	virtual void OnClientConnect(int channelId, bool isReset, EntityId playerId);
	virtual void OnClientDisconnect(int channelId, EntityId playerId);
	virtual void OnClientEnteredGame(int channelId, bool isReset, EntityId playerId) {}
	virtual void OnOwnClientEnteredGame() {};
	// ~IGameRulesClientConnectionListener

	void Init();
	void SvReset();

	void ClSendCurrentEquipmentLoadout(int channelId);
	
	bool SvCanSetClientLoadout();

	void SvSetClientLoadout(int channelId, const CGameRules::EquipmentLoadoutParams &params);
	void SvAssignClientEquipmentLoadout(int channelId, int playerId);

	bool SvHasClientEquipmentLoadout(int channelId);

	// Server override functions, for NetTests
	void SvSendServerOverrideLoadout(int channelId);
	void ClSetServerOverrideLoadout(const CGameRules::EquipmentLoadoutParams &params);
#if ALLOW_USE_SERVER_OVERRIDE
	void GenerateRandomOverrideLoadout(CGameRules::EquipmentLoadoutParams &params);
#endif

	// HUD related
	void InitializeLoadoutPage(IFlashPlayer *flashPlayer);
	void HandleFSCommand(IFlashPlayer *flashPlayer, const char *strCommand,const char *strArgs);
	void LoadoutMenuClosed();

	// HostMigration
	void ClGetCurrentLoadoutParams(CGameRules::EquipmentLoadoutParams &params);
private:

	typedef uint8 TEquipmentPackageContents[EQUIPMENT_LOADOUT_NUM_SLOTS];
	typedef CryFixedStringT<32> TFixedString;
	typedef CryFixedStringT<64> TFixedString64;

	// A local equipment package representation.
	struct SEquipmentPackage
	{
		SEquipmentPackage(const char*name)
			:	m_name(name), m_flags(ELOF_NONE)
		{
			memset(&m_contents,0,sizeof(m_contents));
			memset(&m_defaultContents,0,sizeof(m_defaultContents));
		}

		string m_name;
		uint32 m_flags;
		TEquipmentPackageContents m_contents;
		TEquipmentPackageContents m_defaultContents;
	};

	// A clients equipment package representation on the server.
	struct SClientEquipmentPackage
	{
		SClientEquipmentPackage()
			:	m_flags(ELOF_NONE)
		{
			memset(&m_contents,0,sizeof(m_contents));
		}

		uint32 m_flags;
		TEquipmentPackageContents m_contents;
	};

#if ALLOW_USE_SERVER_OVERRIDE
	// Server override package.
	struct SServerOverrideEquipmentPackage
	{
		SServerOverrideEquipmentPackage()
			:	m_active(false), m_count(0)
		{ }

		bool m_active;
		uint32 m_count;
		TEquipmentPackageContents m_contents;
	};
#endif

	// Loadout Item definition.
	struct SEquipmentItem
	{
		SEquipmentItem(const char*name, const char* className)
			: m_name(name), m_className(className), m_category(eELC_NONE), m_uniqueIndex(0), m_unlockRank(0), m_subcategory(0),
			  m_accuracy(0), m_rateOfFire(0), m_recoil(0), m_damage(0)
		{ memset(&m_attachmentLevel, 0, sizeof(m_attachmentLevel)); }

		TFixedString m_name;
		TFixedString m_className;
		TFixedString m_displayName;
		TFixedString m_displayTypeName;
		TFixedString m_description;

		EEquipmentLoadoutCategory m_category;
		uint32 m_uniqueIndex;
		uint32 m_unlockRank;

		uint8 m_subcategory;

		// Stats
		uint8 m_accuracy;
		uint8 m_rateOfFire;
		uint8 m_recoil;
		uint8 m_damage;

		// Attachment locks
		uint8 m_attachmentLevel[ATTACHMENT_SLOTS_NUM];
	};

	SEquipmentPackage* GetPackageFromName(const char *name);
	int         GetPackageIndexFromName(const char *name);
	const char* GetItemNameFromIndex(int idx);
	int         GetItemIndexFromName(const char *name);

	int  CountOfCategoryUptoIndex(EEquipmentLoadoutCategory category, int Nth, int upToIndex);
	int  GetSlotIndex(EEquipmentLoadoutCategory category, int Nth);

	void LoadDefaultEquipmentPackages();
	void LoadSavedEquipmentPackages();
	void LoadItemDefinitions();
	void LoadItemDefinitionParams(XmlNodeRef itemXML, SEquipmentItem &item);

	void SaveEquipmentPackages();
	bool SaveEquipmentPackage(int packIndex, int count);

	// HUD related
	void SetSelectedPackage(uint32 index);
	void UpdateAndSavePackage(IFlashPlayer *flashPlayer, uint32 index);
	void SetDefaultLoadout(IFlashPlayer *flashPlayer, uint32 index);

	void SendAllPackagesToFlash(IFlashPlayer *flashPlayer);
	void SendPackageToFlash(IFlashPlayer *flashPlayer, int index);
	void SendAllItemsToFlash(IFlashPlayer *flashPlayer);
	void SendSelectedPackageIndexToFlash(IFlashPlayer *flashPlayer);

	bool IsLocked(EUnlockType type, const char* name, int &unlockValue);

	typedef std::vector<SEquipmentItem> TEquipmentItems;
	typedef std::vector<SEquipmentPackage> TEquipmentPackages;
	typedef std::map<int, SClientEquipmentPackage> TClientEquipmentPackages;
	
	TEquipmentItems m_allItems;
	uint8 m_slotCategories[EQUIPMENT_LOADOUT_NUM_SLOTS];

	// Local Client
	TEquipmentPackages m_packages;
	uint32 m_selectedPackage;
	uint32 m_lastSentPackage;

	bool m_allowLoadingFromProfile;
	bool m_allowSavingToProfile;
	bool m_allowCustomizing;

	// Server
	bool m_usingServerOverride; // Server override will need removing if ALLOW_USE_SERVER_OVERRIDE stuff is removed.
	bool m_randomEachSpawn;
	
	enum eEquipmentLoudoutRandom
	{
		eELR_RandPackagesEvenlySpread = 0,
		eELR_RandPackages,
		eELR_RandGenerated,
	};
	
	eEquipmentLoudoutRandom m_serverOverridesRandomType;

	TClientEquipmentPackages m_clientLoadouts;

#if ALLOW_USE_SERVER_OVERRIDE
	typedef std::map<int, int> TServerOverrideClientEquipmentPackages;	// Client channel -> Server override package

	SServerOverrideEquipmentPackage m_serverOverridePackages[MAX_SERVER_OVERRIDE_PACKAGES];
	TServerOverrideClientEquipmentPackages m_serverOverrideChannels;
	int m_serverOverrideCount;
#endif
};

#endif // ~__EQUIPMENT_LOADOUT_H__
