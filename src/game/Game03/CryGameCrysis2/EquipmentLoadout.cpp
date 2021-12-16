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

#include "StdAfx.h"
#include "EquipmentLoadout.h"
#include "Actor.h"
#include "Player.h"
#include "Item.h"
#include "IItemSystem.h"
#include "Menus/OptionsManager.h"
#include "IFlashPlayer.h"
#include "StringUtils.h"
#include "HUD/HUD.h"

//------------------------------------------------------------------------
#define EQUIPMENT_LOADOUT_ITEMS_XML_PATH		"Scripts/GameRules/EquipmentLoadoutItems.xml"
#define EQUIPMENT_LOADOUT_PACKAGES_XML_PATH		"Scripts/GameRules/EquipmentLoadoutPackages.xml"
#define EQUIPMENT_LOADOUT_PROFILE_PATH		"Multiplayer.EquipmentLoadout"

#define INVALID_INDEX -1

static TKeyValuePair<CEquipmentLoadout::EEquipmentLoadoutCategory,const char*>
gEquipmentLoadoutCategories[] = {  
	{CEquipmentLoadout::eELC_NONE,""},
	{CEquipmentLoadout::eELC_PRIMARY_WEAPON,"Primary-Weapon"},
	{CEquipmentLoadout::eELC_SECONDARY_WEAPON,"Secondary-Weapon"},
	{CEquipmentLoadout::eELC_EXPLOSIVE,"Explosive"},
	{CEquipmentLoadout::eELC_ATTACHMENT,"Attachment"},
	{CEquipmentLoadout::eELC_PERK,"Perk"},
};

//------------------------------------------------------------------------
CEquipmentLoadout::CEquipmentLoadout()
{
	m_selectedPackage = 0;
	m_lastSentPackage = 0;

	m_allowLoadingFromProfile = true;
	m_allowSavingToProfile = true;
	m_allowCustomizing = true;

	m_usingServerOverride = false;
	m_serverOverridesRandomType = eELR_RandPackagesEvenlySpread;
	m_randomEachSpawn = false;

#if ALLOW_USE_SERVER_OVERRIDE
	m_serverOverrideCount = 0;
#endif

	// Item 0 used for invalid/null item.
	SEquipmentItem item("", "");
	item.m_uniqueIndex = 0;
	m_allItems.push_back(item);
	
	CRY_TODO(22, 10, 2009, "Load the slot types from XML and pass them to the gui.");
	memset(&m_slotCategories,0,sizeof(m_slotCategories));
	m_slotCategories[0] = eELC_PRIMARY_WEAPON;
	m_slotCategories[1] = eELC_ATTACHMENT;
	m_slotCategories[2] = eELC_ATTACHMENT;
	m_slotCategories[3] = eELC_ATTACHMENT;
	m_slotCategories[4] = eELC_SECONDARY_WEAPON;
	m_slotCategories[5] = eELC_ATTACHMENT;
	m_slotCategories[6] = eELC_ATTACHMENT;
	m_slotCategories[7] = eELC_ATTACHMENT;
	m_slotCategories[8] = eELC_EXPLOSIVE;
	m_slotCategories[9] = eELC_EXPLOSIVE;
	m_slotCategories[10] = eELC_PERK;
	m_slotCategories[11] = eELC_PERK;
	m_slotCategories[12] = eELC_PERK;
	m_slotCategories[13] = eELC_PERK;

	g_pGame->GetGameRules()->RegisterClientConnectionListener(this);
}

CEquipmentLoadout::~CEquipmentLoadout()
{
	if (g_pGame->GetGameRules())
		g_pGame->GetGameRules()->UnRegisterClientConnectionListener(this);
}

//------------------------------------------------------------------------
void CEquipmentLoadout::Init()
{
	LoadItemDefinitions();					// Load item definitions from xml.

	LoadDefaultEquipmentPackages(); // Load default from xml.

	if (gEnv->bClient)
	{
		if (m_allowLoadingFromProfile)
		{
			LoadSavedEquipmentPackages();		// Load saved changes from profile.
		}

		// Load default choice from profile.
		COptionsManager *optionsMgr = g_pGame->GetOptions();
		if (optionsMgr)
		{
			string defaultEquipmentPackName;
			string pathName;
			pathName.Format("%s.DefaultSelection", EQUIPMENT_LOADOUT_PROFILE_PATH);
			if(optionsMgr->GetProfileValue(pathName.c_str(),defaultEquipmentPackName))
			{
				m_selectedPackage = GetPackageIndexFromName(defaultEquipmentPackName.c_str());

				if (m_selectedPackage == INVALID_INDEX)
					m_selectedPackage = 0;
			}
		}
	}
}

//------------------------------------------------------------------------
void CEquipmentLoadout::SvReset()
{
	TClientEquipmentPackages::iterator it = m_clientLoadouts.begin();
	for(; it != m_clientLoadouts.end(); ++it)
	{
		SClientEquipmentPackage &package = it->second;
		package.m_flags |= ELOF_HAS_CHANGED;
	}
}

//------------------------------------------------------------------------
void CEquipmentLoadout::LoadItemDefinitions()
{
	XmlNodeRef root = gEnv->pSystem->LoadXmlFile( EQUIPMENT_LOADOUT_ITEMS_XML_PATH );
	if (root && !stricmp(root->getTag(), "loadouts"))
	{
		int numCategories = root->getChildCount();
		for (int i = 0; i < numCategories; ++ i)
		{
			XmlNodeRef categoryXml = root->getChild(i);
			const char *categoryName = categoryXml->getTag();
			EEquipmentLoadoutCategory category = KEY_BY_VALUE(string(categoryName), gEquipmentLoadoutCategories);

			int numItems = categoryXml->getChildCount();
			for (int j = 0; j < numItems; ++ j)
			{
				XmlNodeRef itemXml = categoryXml->getChild(j);

				if (!stricmp(itemXml->getTag(), "item"))
				{
					const char *strName = NULL;
					const char *strClassName = NULL;
					itemXml->getAttr("name", &strName);
					itemXml->getAttr("class", &strClassName);

					SEquipmentItem item(strName, strClassName);
					LoadItemDefinitionParams(itemXml, item);

					item.m_category = category;
					item.m_uniqueIndex = m_allItems.size(); // unique index must match index in array.

					m_allItems.push_back(item);
				}
			} // ~for categoryXml
		} // ~for root
	}
	else
	{
		CryLogAlways("CEquipmentLoadout Failed to get root node for xml %s", EQUIPMENT_LOADOUT_ITEMS_XML_PATH);
	}
}

//------------------------------------------------------------------------
void CEquipmentLoadout::LoadItemDefinitionParams(XmlNodeRef itemXML, SEquipmentItem &item)
{
	int numParams = itemXML->getChildCount();
	for (int i = 0; i < numParams; ++ i)
	{
		XmlNodeRef paramXml = itemXML->getChild(i);

		const char *name = paramXml->getTag();

		int value = 0;
		const char* strValue = 0;
		if (!stricmp(name, "UnlockRank"))
		{
			if (paramXml->getAttr("value", value))
				item.m_unlockRank = value;
		}
		else if (!stricmp(name, "Name"))
		{
			if (paramXml->getAttr("value", &strValue))
				item.m_displayName = strValue;
		}
		else if (!stricmp(name, "TypeName"))
		{
			if (paramXml->getAttr("value", &strValue))
				item.m_displayTypeName = strValue;
		}
		else if (!stricmp(name, "Description"))
		{
			if (paramXml->getAttr("value", &strValue))
				item.m_description = strValue;
		}
		else if (!stricmp(name, "Accuracy"))
		{
			if (paramXml->getAttr("value", value))
				item.m_accuracy = value;
		}
		else if (!stricmp(name, "RateOfFire"))
		{
			if (paramXml->getAttr("value", value))
				item.m_rateOfFire = value;
		}
		else if (!stricmp(name, "Recoil"))
		{
			if (paramXml->getAttr("value", value))
				item.m_recoil = value;
		}
		else if (!stricmp(name, "Damage"))
		{
			if (paramXml->getAttr("value", value))
				item.m_damage = value;
		}
		else if (!stricmp(name, "Subcategory"))
		{
			if (paramXml->getAttr("value", value))
				item.m_subcategory = value;
		}
		else if (!stricmp(name, "Attach0"))
		{
			if (paramXml->getAttr("value", value))
				item.m_attachmentLevel[0] = value;
		}
		else if (!stricmp(name, "Attach1"))
		{
			if (paramXml->getAttr("value", value))
				item.m_attachmentLevel[1] = value;
		}
		else if (!stricmp(name, "Attach2"))
		{
			if (paramXml->getAttr("value", value))
				item.m_attachmentLevel[2] = value;
		}
	} // ~for
}

//------------------------------------------------------------------------
void CEquipmentLoadout::LoadDefaultEquipmentPackages()
{
	XmlNodeRef root = gEnv->pSystem->LoadXmlFile( EQUIPMENT_LOADOUT_PACKAGES_XML_PATH );
	if (root)
	{
		if (!stricmp(root->getTag(), "default"))
		{
			const char *strName = NULL;
			int numCategories = root->getChildCount();
			for (int i = 0; i < numCategories; ++i)
			{
				XmlNodeRef categoryXml = root->getChild(i);
				if (!stricmp(categoryXml->getTag(), "loadout"))
				{
					categoryXml->getAttr("name", &strName);
					SEquipmentPackage package(strName);

					int slotCounts[eELC_SIZE];
					memset(&slotCounts,0,sizeof(slotCounts));

					int numItems = categoryXml->getChildCount();
					for (int j = 0; j < numItems; ++ j)
					{
						XmlNodeRef itemXml = categoryXml->getChild(j);
						const char* slotTag = itemXml->getTag();

						if (!stricmp(slotTag,"option"))
						{
							itemXml->getAttr("name", &strName);

							if (!stricmp(strName,"Customizable"))
							{
								package.m_flags |= ELOF_CAN_BE_CUSTOMIZED;
							}
						}
						else
						{
							itemXml->getAttr("name", &strName);

							EEquipmentLoadoutCategory category = KEY_BY_VALUE(string(slotTag), gEquipmentLoadoutCategories);

							// TODO: Check the category matches the slot

							int slotIndex = GetSlotIndex(category, slotCounts[category]);

							if ((category == eELC_PRIMARY_WEAPON) || (category == eELC_SECONDARY_WEAPON))
							{
								slotCounts[eELC_ATTACHMENT] += CountOfCategoryUptoIndex(eELC_ATTACHMENT, slotCounts[eELC_ATTACHMENT], slotIndex);
							}

							if (slotIndex >= 0)
							{
								int index = GetItemIndexFromName(strName);
								package.m_contents[slotIndex] = index;
								package.m_defaultContents[slotIndex] = index;
								++slotCounts[category];
							}
						}
					}

					if (gEnv->bClient)
					{
						m_packages.push_back(package);
					}

#if ALLOW_USE_SERVER_OVERRIDE
					if (gEnv->bServer)
					{
						if (m_serverOverrideCount < MAX_SERVER_OVERRIDE_PACKAGES)
						{
							SServerOverrideEquipmentPackage *overridePackage = &m_serverOverridePackages[m_serverOverrideCount];
							overridePackage->m_active = true;
							memcpy( (void*)overridePackage->m_contents, (void*)package.m_contents, sizeof(package.m_contents));
							m_serverOverrideCount++;
						}
						else
						{
							CryLog("Max number of SServerOverrideEquipmentPackages reached");
						}
					}
#endif
				}
#if ALLOW_USE_SERVER_OVERRIDE
				else if (gEnv->bServer && !stricmp(categoryXml->getTag(), "UseServerOverrides")) // Server sends out chosen loadouts to clients
				{
					CRY_TODO(04, 11, 2009, "This option is only for testing with nettests, it will unlikely needed for release.");
					int value = 0;
					if (categoryXml->getAttr("value", value))
						m_usingServerOverride = (value != 0);
				}
				else if (!stricmp(categoryXml->getTag(), "RandomEachSpawn"))	// Server sends a new loadout to clients each spawn
				{
					int value = 0;
					if (categoryXml->getAttr("value", value))
						m_randomEachSpawn = (value != 0);
				}
				else if (!stricmp(categoryXml->getTag(), "ServerOverrideRandomType"))	// Type of loadout override to use
				{
					const char *strName = NULL;
					if (categoryXml->getAttr("type", &strName))
					{
						if (!stricmp(strName,"RandomGenerated"))
							m_serverOverridesRandomType = eELR_RandGenerated;
						else if (!stricmp(strName,"RandomPackageSpread"))
							m_serverOverridesRandomType = eELR_RandPackagesEvenlySpread;
						else if (!stricmp(strName,"RandomPackage"))
							m_serverOverridesRandomType = eELR_RandPackages;
					}
				}
#endif
				else if (!stricmp(categoryXml->getTag(), "AllowLoadingFromProfile"))	// Whether any customization changes are loaded from profile
				{
					int value = 0;
					if (categoryXml->getAttr("value", value))
						m_allowLoadingFromProfile = (value != 0);
				}
				else if (!stricmp(categoryXml->getTag(), "AllowSavingToProfile"))	// Whether any customization changes are saved to profile
				{
					int value = 0;
					if (categoryXml->getAttr("value", value))
						m_allowSavingToProfile = (value != 0);
				}
				else if (!stricmp(categoryXml->getTag(), "AllowCustomizing"))	// Whether any customization can take place
				{
					int value = 0;
					if (categoryXml->getAttr("value", value))
						m_allowCustomizing = (value != 0);
				}
			}
		} // ~for
	}
	else
	{
		CryLogAlways("CEquipmentLoadout Failed to get root node for xml %s", EQUIPMENT_LOADOUT_PACKAGES_XML_PATH);
	}
}

//------------------------------------------------------------------------
// 
int CEquipmentLoadout::CountOfCategoryUptoIndex(EEquipmentLoadoutCategory category, int Nth, int upToIndex)
{
	if (upToIndex<0)
		return 0;

	int result = 0;

	int currNth = 0;
	for (int i=0; i<upToIndex; i++)
	{
		if (m_slotCategories[i] == category)
		{
			if (currNth >= Nth)
			{
				result++;
			}

			currNth++;
		}
	}

	return result;
}

//------------------------------------------------------------------------
// Gets the slot index for the Nth in that category (Nth 0 is first)
int CEquipmentLoadout::GetSlotIndex(EEquipmentLoadoutCategory category, int Nth)
{
	int result = INVALID_INDEX;

	int currNth = 0;
	for (int i=0; i<EQUIPMENT_LOADOUT_NUM_SLOTS; i++)
	{
		if (m_slotCategories[i] == category)
		{
			if (currNth == Nth)
			{
				result = i;
				break;
			}
			else
			{
				currNth++;
			}
		}
	}

	return result;
}

//------------------------------------------------------------------------
// Load saved loadouts from profile.
void CEquipmentLoadout::LoadSavedEquipmentPackages()
{
	COptionsManager *optionsMgr = g_pGame->GetOptions();
	if (!optionsMgr)
		return;

	CryFixedStringT<256> strPathName;
	strPathName.Format("%s.NumPacks", EQUIPMENT_LOADOUT_PROFILE_PATH);
	int numEntries = 0;
	if(optionsMgr->GetProfileValue(strPathName,numEntries))
	{
		string loadoutName;
		for(int i(0); i<numEntries; ++i)
		{
			strPathName.Format("%s.%d.Name",EQUIPMENT_LOADOUT_PROFILE_PATH,i);
			if (optionsMgr->GetProfileValue(strPathName, loadoutName))
			{
				SEquipmentPackage *package = GetPackageFromName(loadoutName.c_str());
				if (package)
				{
					CryLog("CEquipmentLoadout::LoadSavedEquipmentPackage Name: %s", loadoutName.c_str());

					strPathName.Format("%s.%d.Num",EQUIPMENT_LOADOUT_PROFILE_PATH,i);
					int numItemEntries = 0;
					if(optionsMgr->GetProfileValue(strPathName,numItemEntries))
					{
						string strValue;
						int slotIndex = INVALID_INDEX;
						for(int j(0); j<numItemEntries; ++j)
						{
							strPathName.Format("%s.%d.%d.SlotIndex",EQUIPMENT_LOADOUT_PROFILE_PATH,i,j);
							if(optionsMgr->GetProfileValue(strPathName, slotIndex))
							{
								if (slotIndex >= 0 && slotIndex < EQUIPMENT_LOADOUT_NUM_SLOTS)
								{
									strPathName.Format("%s.%d.%d.Name",EQUIPMENT_LOADOUT_PROFILE_PATH,i,j);
									if(optionsMgr->GetProfileValue(strPathName, strValue))
									{
										int itemIndex = GetItemIndexFromName(strValue);
										package->m_contents[slotIndex] = itemIndex;
									}
								}
							}
						} // ~for
					}
				}
				else
				{
					CryLogAlways("CEquipmentLoadout Tried to load an unknown equipment pack, Name: %s", loadoutName.c_str());
				}
			}
		}
	}
}

//------------------------------------------------------------------------
// Save any changes in loadout from the default packages to profile.
void CEquipmentLoadout::SaveEquipmentPackages()
{
	COptionsManager *optionsMgr = g_pGame->GetOptions();
	if (!optionsMgr)
		return;

	int count = 0;
	int numEntries = m_packages.size();
	for (int i=0; i<numEntries; ++i)
	{
		if (SaveEquipmentPackage(i, count))
		{
			count++;
		}
	}

	if (m_allowSavingToProfile)
	{
		CryFixedStringT<256> strPathName;
		strPathName.Format("%s.NumPacks", EQUIPMENT_LOADOUT_PROFILE_PATH);
		optionsMgr->SaveValueToProfile(strPathName,count);

		optionsMgr->SaveProfile();
	}
}

//------------------------------------------------------------------------
bool CEquipmentLoadout::SaveEquipmentPackage(int packIndex, int count)
{
	COptionsManager *optionsMgr = g_pGame->GetOptions();
	if (!optionsMgr)
		return false;

	SEquipmentPackage &package = m_packages[packIndex];

	// Collate changed slots
	std::vector<const char *> itemNames;
	std::vector<uint8> itemSlotIndexes;

	int i=0;
	for (i=0; i<EQUIPMENT_LOADOUT_NUM_SLOTS; ++i)
	{
		if (package.m_contents[i] != package.m_defaultContents[i])
		{
			itemNames.push_back( GetItemNameFromIndex(package.m_contents[i]) );
			itemSlotIndexes.push_back( i );
		}
	}

	int size = itemNames.size();
	if (size > 0)
	{
		package.m_flags |= ELOF_HAS_CHANGED;

		if (m_allowSavingToProfile)
		{
			CryFixedStringT<256> strPathName;
			strPathName.Format("%s.%d.Name",EQUIPMENT_LOADOUT_PROFILE_PATH,count);
			optionsMgr->SaveValueToProfile(strPathName, package.m_name);

			strPathName.Format("%s.%d.Num",EQUIPMENT_LOADOUT_PROFILE_PATH,count);
			optionsMgr->SaveValueToProfile(strPathName, size);

			for (i=0; i<size; ++i)
			{
				strPathName.Format("%s.%d.%d.Name",EQUIPMENT_LOADOUT_PROFILE_PATH,count,i);
				optionsMgr->SaveValueToProfile(strPathName, string(itemNames[i]));

				strPathName.Format("%s.%d.%d.SlotIndex",EQUIPMENT_LOADOUT_PROFILE_PATH,count,i);
				optionsMgr->SaveValueToProfile(strPathName, itemSlotIndexes[i]);
			}
		}
	}

	return (size > 0);
}

//------------------------------------------------------------------------
// SERVER: Create a client's loadout representation
void CEquipmentLoadout::OnClientConnect(int channelId, bool isReset, EntityId playerId)
{
	//CryLog("CEquipmentLoadout::CreateClientLoadout() %d", channelId);
	CRY_ASSERT_MESSAGE(gEnv->bServer, "CEquipmentLoadout::CreateClientLoadout() but NOT the server");

	if (!isReset)
	{
		m_clientLoadouts.insert(TClientEquipmentPackages::value_type(channelId, SClientEquipmentPackage()));

#if ALLOW_USE_SERVER_OVERRIDE
		m_serverOverrideChannels.insert(TServerOverrideClientEquipmentPackages::value_type(channelId, -1));

		if (m_usingServerOverride || g_pGameCVars->g_loadoutServerControlled)
		{
			SvSendServerOverrideLoadout(channelId);
		}
#endif
	}
}

//------------------------------------------------------------------------
// SERVER: Remove a client's loadout representation
void CEquipmentLoadout::OnClientDisconnect(int channelId, EntityId playerId)
{
	//CryLog("CEquipmentLoadout::RemoveClientLoadout() %d", channelId);
	CRY_ASSERT_MESSAGE(gEnv->bServer, "CEquipmentLoadout::RemoveClientLoadout() but NOT the server");

	stl::member_find_and_erase(m_clientLoadouts, channelId);

#if ALLOW_USE_SERVER_OVERRIDE
	TServerOverrideClientEquipmentPackages::iterator it= m_serverOverrideChannels.find(channelId);
	if (it != m_serverOverrideChannels.end())
	{
		if ((it->second >= 0) && (it->second < m_serverOverrideCount))
		{
			if (m_serverOverridePackages[it->second].m_count > 0)
			{
				m_serverOverridePackages[it->second].m_count--;
			}
		}
		m_serverOverrideChannels.erase(it);
	}
#endif
}

//------------------------------------------------------------------------
// SERVER: Set a client's current loadout.
void CEquipmentLoadout::SvSetClientLoadout(int channelId, const CGameRules::EquipmentLoadoutParams &params)
{
	TClientEquipmentPackages::iterator it= m_clientLoadouts.find(channelId);
	if (it != m_clientLoadouts.end())
	{
		SClientEquipmentPackage &package = it->second;

		package.m_flags |= (ELOF_HAS_CHANGED);
		
		memcpy( (void*)package.m_contents, (void*)params.m_contents, sizeof(package.m_contents));

		/*
		CryLog("SERVER SvSetClientLoadout %d", channelId);
		for (int i=0; i<EQUIPMENT_LOADOUT_NUM_SLOTS; ++i)
		{
			CryLog("     %d - %d", i, package.m_contents[i]);
		}
		*/
	}
}

//------------------------------------------------------------------------
// CLIENT: Send the current loadout to the server
void CEquipmentLoadout::ClSendCurrentEquipmentLoadout(int channelId)
{
	CGameRules::EquipmentLoadoutParams params;

	int numPackage = m_packages.size();
	if ((numPackage > 0) && (numPackage > m_selectedPackage))
	{
		TEquipmentPackageContents &contents = m_packages[m_selectedPackage].m_contents;

		memcpy( (void*)params.m_contents, (void*)contents, sizeof(contents));

		m_lastSentPackage = m_selectedPackage;

		/*
		CryLog("CLIENT ClSendCurrentEquipmentLoadout %d", channelId);
		for (int i=0; i<EQUIPMENT_LOADOUT_NUM_SLOTS; ++i)
		{
			CryLog("     %d - %d", i, contents[i]);
		}*/

		if (gEnv->bServer && SvCanSetClientLoadout())
		{
			SvSetClientLoadout(channelId, params);
		}
		else
		{
			g_pGame->GetGameRules()->GetGameObject()->InvokeRMI(CGameRules::SvSetEquipmentLoadout(), params, eRMI_ToServer);
		}
	}
}

//------------------------------------------------------------------------
// SERVER: On spawning, assign the current client's loadout
void CEquipmentLoadout::SvAssignClientEquipmentLoadout(int channelId, int playerId)
{
#if ALLOW_USE_SERVER_OVERRIDE
	if (m_randomEachSpawn && (m_usingServerOverride || g_pGameCVars->g_loadoutServerControlled))
	{
		SvSendServerOverrideLoadout(channelId);
	}
#endif

	TClientEquipmentPackages::iterator it= m_clientLoadouts.find(channelId);
	if (it != m_clientLoadouts.end())
	{
		SClientEquipmentPackage &package = it->second;

		/*
		CryLog("SERVER SvAssignClientEquipmentLoadout %d", channelId);
		for (int i=0; i<EQUIPMENT_LOADOUT_NUM_SLOTS; ++i)
		{
			CryLog("     %d - %d", i, package.m_contents[i]);
		}
		*/

		IItemSystem *pItemSystem = g_pGame->GetIGameFramework()->GetIItemSystem();
		CActor *pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId));

		CRY_ASSERT_MESSAGE(pActor->IsPlayer(), "Actor appling loadout to is not a player!");
		CPlayer *pPlayer = static_cast<CPlayer*>(pActor);
		if (pPlayer && pItemSystem)
		{
			// Give the client actor their equipment package. TODO: Package validity checking here, e.g. Unlock Levels.

			SEquipmentItem *item = NULL;
			EntityId lastWeaponId = 0;
			int numItems = m_allItems.size();

			CGameRules *pGameRules = g_pGame->GetGameRules();

			uint64 perkBitField = 0;
			CPerk *pPerk = CPerk::GetInstance();

			bool  useWeaponLoadouts = (!pGameRules || pGameRules->RulesUseWeaponLoadouts());
			bool  usePerkLoadouts = (!pGameRules || pGameRules->RulesUsePerkLoadouts());

			for (int i=0; i<=EQUIPMENT_LOADOUT_NUM_SLOTS-1; i++)
			{
				uint8 itemId = package.m_contents[i];
				if (itemId >= numItems)
				{
					CryLogAlways("CEquipmentLoadout::SvAssignClientEquipmentLoadout Invalid item id");
					continue;
				}

				if (itemId == 0) // nil item
				{
					continue;
				}
				
				item = &m_allItems[itemId];

				switch(m_slotCategories[i])
				{
					// Perks
					case eELC_PERK:
					{
						if (usePerkLoadouts)
						{
							EPerks perkId = pPerk->FindPerkNumberByName(item->m_className); // TODO: Look-up and store the id on item load instead of a string look-up here.
							if (perkId > ePerk_Null && perkId < ePerk_Last)
							{
								perkBitField |= PERKBIT(perkId);
							}
						}
					}
					break;

					// Primary weapon should be selected immediately, others aren't
					case eELC_PRIMARY_WEAPON:
					{
						if (useWeaponLoadouts)
						{
							lastWeaponId = pItemSystem->GiveItem(pActor, item->m_className, false, true, true); 
						}
					}
					break;

					case eELC_SECONDARY_WEAPON:
					{
						if (useWeaponLoadouts)
						{
							lastWeaponId = pItemSystem->GiveItem(pActor, item->m_className, false, false, true);
						}
					}
					break;

					case eELC_EXPLOSIVE:
					{
						if (useWeaponLoadouts)
						{
							pItemSystem->GiveItem(pActor, item->m_className, false, false, true);
						}
					}
					break;

					case eELC_ATTACHMENT:
					{
						if (useWeaponLoadouts)
						{
							EntityId attachmentId = pItemSystem->GiveItem(pActor, item->m_className, false, false, true);

							IItem * pWeaponItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(lastWeaponId);
							IItem * pAttachmentItem = g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(attachmentId);

							IEntityClass *pAttachmentClass = pAttachmentItem->GetEntity()->GetClass();

							CRY_ASSERT_MESSAGE(pWeaponItem, "No item found!");

							if (IWeapon *pWeapon = pWeaponItem->GetIWeapon())
							{
								CItem * pCWeaponItem = static_cast<CItem*>(pWeaponItem);

								if (pCWeaponItem)
								{
									pCWeaponItem->AttachAccessory(pAttachmentClass->GetName(), true, true, true);
								}
							}
						}
					}
					break;

					// All other items e.g. weapons, attachments (all given the same way)
					default:
					{
						pItemSystem->GiveItem(pActor, item->m_className, false, false, true);
					}
					break;
				}
			} // ~for

			if (usePerkLoadouts && pPerk && (package.m_flags & ELOF_HAS_CHANGED))
			{
				package.m_flags &= ~ELOF_HAS_CHANGED; // Don't set perks after first time, even if they have changed

				pPerk->AddDefaultPerksToBitfield(perkBitField);

				if (pPlayer->NetGetPerksBitfield() != perkBitField)
				{
					pPlayer->GetGameObject()->InvokeRMI(CPlayer::ClSetThesePerks(), CPlayer::SetPerksParams(perkBitField), eRMI_ToClientChannel, pPlayer->GetChannelId());
				}
			}

			//pPlayer->EquipAttachmentsOnCarriedWeapons();
			pPlayer->ResetFPWeapon();
		}
	}
}

//------------------------------------------------------------------------
bool CEquipmentLoadout::SvHasClientEquipmentLoadout(int channelId)
{
	TClientEquipmentPackages::iterator it= m_clientLoadouts.find(channelId);
	if (it != m_clientLoadouts.end())
	{
		return true;
	}

	return false;
}

//------------------------------------------------------------------------
// HUD Related
//------------------------------------------------------------------------
void CEquipmentLoadout::InitializeLoadoutPage(IFlashPlayer *flashPlayer)
{
	if (flashPlayer)
	{
		SendAllItemsToFlash(flashPlayer);
		SendAllPackagesToFlash(flashPlayer);
		SendSelectedPackageIndexToFlash(flashPlayer);
	}
}

//------------------------------------------------------------------------
void CEquipmentLoadout::SendAllPackagesToFlash(IFlashPlayer *flashPlayer)
{
	std::vector<string> equipmentPacks;

	TFixedString64 unlockString;

	std::vector<SEquipmentPackage>::const_iterator it = m_packages.begin();
	std::vector<SEquipmentPackage>::const_iterator itEnd = m_packages.end();
	for(; it != itEnd; ++it)
	{
		const SEquipmentPackage& pack = (*it);
		equipmentPacks.push_back( pack.m_name );		// 1

		if (m_usingServerOverride || !m_allowCustomizing || g_pGameCVars->g_loadoutServerControlled)
			equipmentPacks.push_back( CryStringUtils::toString( 0 ) );		// 2
		else
			equipmentPacks.push_back( CryStringUtils::toString( (uint32)((pack.m_flags&ELOF_CAN_BE_CUSTOMIZED)!=0) ) );		// 2
		
		int unlockValue = 0;
		bool locked = IsLocked(eUT_Loadout, pack.m_name.c_str(), unlockValue);

		CHUD *pHUD = g_pGame->GetHUD();
		if (pHUD && locked)
			unlockString = pHUD->LocalizeString("@ui_menu_loadout_unlock_loadout", CryStringUtils::toString(unlockValue).c_str());
		else
			unlockString = "";

		equipmentPacks.push_back( CryStringUtils::toString( locked ? 1 : 0 ) );		// 3
		equipmentPacks.push_back( locked ? unlockString.c_str() : "" );		// 4


		const TEquipmentPackageContents& contents = pack.m_contents;

		for (int i=0; i<EQUIPMENT_LOADOUT_NUM_SLOTS; i++)	// 5 -> (EQUIPMENT_LOADOUT_NUM_SLOTS+4)
		{
			equipmentPacks.push_back( CryStringUtils::toString( (uint32)contents[i] ) );
		}
	}

	int size = equipmentPacks.size();
	if(size)
	{
		std::vector<const char*> pushArray;
		pushArray.reserve(size);
		for (int i(0); i<size; ++i)
		{
			pushArray.push_back(equipmentPacks[i].c_str());
		}

		flashPlayer->SetVariableArray(FVAT_ConstStrPtr, "m_allValues", 0, &pushArray[0], pushArray.size());
	}

	flashPlayer->Invoke("UpdateAllEquipmentPackages",0,0,0);
}

//------------------------------------------------------------------------
void CEquipmentLoadout::SendPackageToFlash(IFlashPlayer *flashPlayer, int index)
{
	std::vector<string> equipmentPack;

	if (index >= 0 && index < m_packages.size())
	{
		if (m_usingServerOverride || !m_allowCustomizing || g_pGameCVars->g_loadoutServerControlled)
			equipmentPack.push_back( CryStringUtils::toString( 0 ) );		// 1
		else
			equipmentPack.push_back( CryStringUtils::toString( (uint32)((m_packages[index].m_flags&ELOF_CAN_BE_CUSTOMIZED)!=0) ) );	// 1

		int unlockValue = 0;
		bool locked = IsLocked(eUT_Loadout, m_packages[index].m_name.c_str(), unlockValue);

		TFixedString64 unlockString;
		CHUD *pHUD = g_pGame->GetHUD();
		if (pHUD && locked)
			unlockString = pHUD->LocalizeString("@ui_menu_loadout_unlock_loadout", CryStringUtils::toString(unlockValue).c_str());
		else
			unlockString = "";

		equipmentPack.push_back( CryStringUtils::toString( locked ? 1 : 0 ) );		// 2
		equipmentPack.push_back( locked ? unlockString.c_str() : "" );		// 3

		const TEquipmentPackageContents &contents = m_packages[index].m_contents;
		for (int i=0; i<EQUIPMENT_LOADOUT_NUM_SLOTS; i++)	// 4 -> (EQUIPMENT_LOADOUT_NUM_SLOTS+3)
		{
			equipmentPack.push_back( CryStringUtils::toString( (uint32)contents[i] ) );
		}

		int size = equipmentPack.size();
		if(size)
		{
			std::vector<const char*> pushArray;
			pushArray.reserve(size);
			for (int i(0); i<size; ++i)
			{
				pushArray.push_back(equipmentPack[i].c_str());
			}

			flashPlayer->SetVariableArray(FVAT_ConstStrPtr, "m_allValues", 0, &equipmentPack[0], equipmentPack.size());
			SFlashVarValue temp(index);
			flashPlayer->Invoke("UpdateEquipmentPackage",&temp,1,0);
		}
	}
}

//------------------------------------------------------------------------
void CEquipmentLoadout::SendAllItemsToFlash(IFlashPlayer *flashPlayer)
{
	std::vector<string> itemsArray;

	std::vector<SEquipmentItem>::const_iterator it = m_allItems.begin();
	for(; it != m_allItems.end(); ++it)
	{
		const SEquipmentItem& item = (*it);
		itemsArray.push_back( item.m_name );                                            // 1
		itemsArray.push_back( CryStringUtils::toString( (uint32)item.m_category ) );    // 2
		itemsArray.push_back( item.m_displayName);                                      // 3
		itemsArray.push_back( item.m_displayTypeName);                                  // 4
		itemsArray.push_back( item.m_description);                                      // 5
		
		int unlockValue = 0;
		bool locked = IsLocked(eUT_Weapon, item.m_name.c_str(), unlockValue);

		TFixedString64 unlockString;
		CHUD *pHUD = g_pGame->GetHUD();
		if (pHUD && locked)
			unlockString = pHUD->LocalizeString("@ui_menu_loadout_unlock_weapon", CryStringUtils::toString(unlockValue).c_str());
		else
			unlockString = "";

		itemsArray.push_back( CryStringUtils::toString( locked ? 1 : 0 ) );							// 6
		itemsArray.push_back( locked ? unlockString.c_str() : "" );											// 7

		itemsArray.push_back( CryStringUtils::toString( (uint32)item.m_accuracy ) );    // 8
		itemsArray.push_back( CryStringUtils::toString( (uint32)item.m_rateOfFire ) );  // 9
		itemsArray.push_back( CryStringUtils::toString( (uint32)item.m_recoil ) );      // 10
		itemsArray.push_back( CryStringUtils::toString( (uint32)item.m_damage ) );      // 11

		itemsArray.push_back( CryStringUtils::toString( (uint32)item.m_subcategory ) ); // 12
		
		itemsArray.push_back( CryStringUtils::toString( (uint32)item.m_attachmentLevel[0] ) ); // 13
		itemsArray.push_back( CryStringUtils::toString( (uint32)item.m_attachmentLevel[1] ) ); // 14
		itemsArray.push_back( CryStringUtils::toString( (uint32)item.m_attachmentLevel[2] ) ); // 15
	}

	int size = itemsArray.size();
	if(size)
	{
		std::vector<const char*> pushArray;
		pushArray.reserve(size);
		for (int i(0); i<size; ++i)
		{
			pushArray.push_back(itemsArray[i].c_str());
		}

		flashPlayer->SetVariableArray(FVAT_ConstStrPtr, "m_allValues", 0, &pushArray[0], pushArray.size());
	}

	flashPlayer->Invoke("UpdateItemDefinitions",0,0,0);
}

//------------------------------------------------------------------------
bool CEquipmentLoadout::IsLocked(EUnlockType type, const char* name, int &unlockValue)
{
	bool locked = false;
	CPlayerProgression *pPlayerProgression = CPlayerProgression::GetInstance();
	if (pPlayerProgression)
	{
		bool exists = false;
		bool unlocked = pPlayerProgression->HaveUnlocked(type, name, exists, unlockValue);

		if (exists && unlocked == false)
		{
			locked = true;
		}
	}

	return locked;
}

//------------------------------------------------------------------------
void CEquipmentLoadout::SendSelectedPackageIndexToFlash(IFlashPlayer *flashPlayer)
{
	SFlashVarValue packageIndex(m_selectedPackage);
	flashPlayer->Invoke("SetSelectedPackageIndex",&packageIndex, 1, 0);
}

//------------------------------------------------------------------------
void CEquipmentLoadout::HandleFSCommand(IFlashPlayer *flashPlayer, const char *strCommand,const char *strArgs)
{
	if (!stricmp(strCommand,"Loadout_SetCurrentLoadout"))
	{
		SetSelectedPackage(atoi(strArgs));
	}
	else if (!stricmp(strCommand,"Loadout_SaveLoadout"))
	{
		UpdateAndSavePackage(flashPlayer, atoi(strArgs));
	}
	else if (!stricmp(strCommand,"Loadout_SetDefaultLoadout"))
	{
		SetDefaultLoadout(flashPlayer, atoi(strArgs));
	}
}

//------------------------------------------------------------------------
void CEquipmentLoadout::LoadoutMenuClosed()
{
	bool hasChanged = false;

	int numPackages = m_packages.size();
	if ((m_selectedPackage >= 0) && (numPackages > m_selectedPackage))
	{
		SEquipmentPackage &package = m_packages[m_selectedPackage];
		if ( (m_selectedPackage != m_lastSentPackage) || (package.m_flags&ELOF_HAS_CHANGED) )
		{
			package.m_flags &= ~ELOF_HAS_CHANGED;

			if (!m_usingServerOverride && !g_pGameCVars->g_loadoutServerControlled)
			{
				// Send equipment package as soon as a new one is picked.
				if (IActor *pActor = g_pGame->GetIGameFramework()->GetClientActor())
				{
					ClSendCurrentEquipmentLoadout(pActor->GetChannelId());

					if (pActor->GetHealth() > 0) // Is alive
					{
						CGameRules *pGameRules = g_pGame->GetGameRules();
						if (pGameRules)
						{
							pGameRules->OnTextMessage(eTextMessageAnnouncement,"@ui_announce_changedLoadout");
						}
					}
				}
				else
				{
					CRY_ASSERT_MESSAGE(0,"Failed to send EquipmentLoadout, Local actor not found.");
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CEquipmentLoadout::SetSelectedPackage(uint32 index)
{
	int numPackages = m_packages.size();
	if ((index >= 0) && (numPackages > index))
	{
		CryLog("CEquipmentLoadout::SetSelectedPackage new:%d (%s) from old:%d", index, m_packages.at(index).m_name.c_str(), m_selectedPackage);

		if (m_selectedPackage != index)
		{
			m_selectedPackage = index;

			COptionsManager *optionsMgr = g_pGame->GetOptions();
			if (optionsMgr)
			{
				string pathName;
				pathName.Format("%s.DefaultSelection", EQUIPMENT_LOADOUT_PROFILE_PATH);
				optionsMgr->SaveValueToProfile(pathName.c_str(),m_packages[m_selectedPackage].m_name);
				optionsMgr->SaveProfile();
			}
		}
	}
}

//------------------------------------------------------------------------
void CEquipmentLoadout::UpdateAndSavePackage(IFlashPlayer *flashPlayer, uint32 index)
{

	int iSize = flashPlayer->GetVariableArraySize("m_backArray");
	if (iSize && (index >= 0) && (m_packages.size() > index))
	{
		std::vector<const char *> getArray;
		getArray.resize(iSize);

		flashPlayer->GetVariableArray(FVAT_ConstStrPtr,"m_backArray", 0, &getArray[0], iSize);

		SEquipmentPackage &package = m_packages[index];
		bool changed = false;

		for (int i=0; i<iSize; i++)
		{
			int itemId = atoi(getArray[i]);
			if (package.m_contents[i] != itemId)
			{
				package.m_contents[i] = itemId;
				changed = true;
			}
		}

		if (changed)
		{
			SaveEquipmentPackages(); // Have to save all packages (instead of just the one that's changed) to preserve indexes
		}
	}
}

//------------------------------------------------------------------------
void CEquipmentLoadout::SetDefaultLoadout(IFlashPlayer *flashPlayer, uint32 index)
{
	int numPackage = m_packages.size();
	if ((index >= 0) && (numPackage > index))
	{
		SEquipmentPackage &package = m_packages[index];
		bool changed = false;

		for (int i=0; i<EQUIPMENT_LOADOUT_NUM_SLOTS; ++i)
		{
			if (package.m_contents[i] != package.m_defaultContents[i])
			{
				package.m_contents[i] = package.m_defaultContents[i];
				changed = true;
			}
		}

		if (changed)
		{
			package.m_flags |= ELOF_HAS_CHANGED;

			SendPackageToFlash(flashPlayer, index);
		
			SaveEquipmentPackages();
		}
	}
}

//------------------------------------------------------------------------
int CEquipmentLoadout::GetItemIndexFromName(const char *name)
{
	TEquipmentItems::iterator itCurrent = m_allItems.begin();
	TEquipmentItems::iterator itEnd = m_allItems.end();

	while (itCurrent != itEnd)
	{
		if (!stricmp(name, itCurrent->m_name.c_str()))
			return itCurrent->m_uniqueIndex;

		itCurrent++;
	}

	return 0;
}

//------------------------------------------------------------------------
const char* CEquipmentLoadout::GetItemNameFromIndex(int idx)
{
	return m_allItems.at(idx).m_name.c_str();
}

//------------------------------------------------------------------------
int CEquipmentLoadout::GetPackageIndexFromName(const char *name)
{
	if (name)
	{
		int num = m_packages.size();

		for (int i=0; i<num; ++i)
		{
			if (!stricmp(name, m_packages[i].m_name.c_str()))
			{
				return i;
			}
		}
	}

	return INVALID_INDEX;
}

//------------------------------------------------------------------------
CEquipmentLoadout::SEquipmentPackage *CEquipmentLoadout::GetPackageFromName(const char *name)
{
	if (name)
	{
		int index = GetPackageIndexFromName(name);
		if (index >= 0)
		{
			return &m_packages[index];
		}
	}

	return NULL;
}

//////////////////// NetTest Loadout Override functions ////////////////////

//------------------------------------------------------------------------
// SERVER: Send a client their fixed loadout that has been allocated to them
void CEquipmentLoadout::SvSendServerOverrideLoadout(int channelId)
{
#if ALLOW_USE_SERVER_OVERRIDE
	assert(gEnv->bServer);

	CGameRules::EquipmentLoadoutParams params;

	if ((m_serverOverrideCount > 0))
	{
		int packNum = 0;

		if (m_serverOverridesRandomType == eELR_RandGenerated)
		{
			// Generate a loadout with random weapons & perks
			GenerateRandomOverrideLoadout(params);
		}
		else
		{
			TServerOverrideClientEquipmentPackages::iterator it= m_serverOverrideChannels.find(channelId);

			if (m_serverOverrideCount > 1)
			{
				if (m_serverOverridesRandomType == eELR_RandPackagesEvenlySpread)
				{
					if (it != m_serverOverrideChannels.end())
					{
						int curPack = it->second;
						if ((curPack >= 0) && (curPack < m_serverOverrideCount))
						{
							if (m_serverOverridePackages[curPack].m_count > 0)
							{
								m_serverOverridePackages[curPack].m_count--;
							}
						}
					}

					// Distribute loadouts evenly between clients.
					int i=0;
					int minCount = 99;
					for (i=0; i<m_serverOverrideCount; ++i)
					{
						if (m_serverOverridePackages[i].m_active)
						{
							if (m_serverOverridePackages[i].m_count <= minCount)
								minCount = m_serverOverridePackages[i].m_count;
						}
					}

					int packNumCount = 0;
					int packagesToUse[MAX_SERVER_OVERRIDE_PACKAGES];
					memset( packagesToUse, -1, sizeof(packagesToUse) );

					for (i=0; i<m_serverOverrideCount; ++i)
					{
						if (m_serverOverridePackages[i].m_active && (m_serverOverridePackages[i].m_count <= minCount + 1))	// If is lowest or close to lowest
						{
							packagesToUse[packNumCount++] = i;
						}
					}

					if (packNumCount > 0)
					{
						packNum = packagesToUse[cry_rand() % packNumCount];
					}
				}
				else
				{
						packNum = cry_rand() % m_serverOverrideCount;
				}
			}

			SServerOverrideEquipmentPackage &package = m_serverOverridePackages[packNum];

			assert(package.m_active);

			if (m_serverOverridesRandomType == eELR_RandPackagesEvenlySpread)
			{
				package.m_count++;
			}

			if (it != m_serverOverrideChannels.end())
			{
				it->second = packNum;
			}

			memcpy( (void*)params.m_contents, (void*)package.m_contents, sizeof(package.m_contents));
		}

		IActor *pActor = g_pGame->GetGameRules()->GetActorByChannelId(channelId);
		IEntity *pEntity = pActor ? pActor->GetEntity() : NULL;
		if (pEntity)
		{
			CryLogAlways("SERVER assigned loadout override %d type:%d for %s (Team: %d; Channel: %d). Contents:", packNum, (int)m_serverOverridesRandomType, pEntity->GetName(), g_pGame->GetGameRules()->GetChannelTeam(channelId), channelId);
			int allItemsSize = m_allItems.size();
			for (int i=0; i<EQUIPMENT_LOADOUT_NUM_SLOTS; ++i)
			{
				int id = params.m_contents[i];
				CryLogAlways(" %d - %s [%d]", i, (id<allItemsSize) ? m_allItems[id].m_name.c_str() : "null", id);
			}
		}

		g_pGame->GetGameRules()->GetGameObject()->InvokeRMI(CGameRules::ClSetServerOverrideLoadout(), params, eRMI_ToClientChannel, channelId);
	}

	SvSetClientLoadout(channelId, params);
#endif
}

//------------------------------------------------------------------------
bool CEquipmentLoadout::SvCanSetClientLoadout()
{
	return !(m_usingServerOverride || g_pGameCVars->g_loadoutServerControlled);
}

//------------------------------------------------------------------------
void CEquipmentLoadout::ClSetServerOverrideLoadout(const CGameRules::EquipmentLoadoutParams &params)
{
#if ALLOW_USE_SERVER_OVERRIDE
	m_packages.clear();

	SEquipmentPackage package("[Server Allocated]");

	memcpy( (void*)package.m_contents, (void*)params.m_contents, sizeof(params.m_contents) );

	m_packages.push_back(package);

	m_usingServerOverride = true;

	m_selectedPackage = 0;
	m_lastSentPackage = 0;
#endif
}



#if ALLOW_USE_SERVER_OVERRIDE
//------------------------------------------------------------------------
// For NetTests generate a random loadout
void CEquipmentLoadout::GenerateRandomOverrideLoadout(CGameRules::EquipmentLoadoutParams &params)
{
	CRY_TODO(04, 11, 2009, "This function is only for testing with nettests. It will need removing or refactoring afterwards.");

	// Hardcoded nastyness - Only used for nettests.
	const int primaryWeaponSlotNum = 0;
	const int secondaryWeaponSlotNum = 1;
	const int explosivesWeaponSlotNum = 2; //+2
	const int perkWeaponSlotNum = 4;	// +4

	int primaryWeaponCount = 0;
	int secondaryWeaponCount = 0;
	int explosivesWeaponCount = 0;
	int perkCount = 0;

	std::vector<SEquipmentItem>::const_iterator it = m_allItems.begin();
	std::vector<SEquipmentItem>::const_iterator itEnd = m_allItems.end();
	for(; it != itEnd; ++it)
	{
		// TODO: Explosives & attachments?
		if (it->m_category == eELC_PRIMARY_WEAPON)
		{
			primaryWeaponCount++;
		}
		else if (it->m_category == eELC_SECONDARY_WEAPON)
		{
			secondaryWeaponCount++;
		}
		else if (it->m_category == eELC_PERK)
		{
			perkCount++;
		}
	}

	int primaryRand = cry_rand() % primaryWeaponCount;
	int secondaryRand = cry_rand() % secondaryWeaponCount;

	int perksRand[3];
	memset( perksRand, 0, sizeof(perksRand) );

	if (perkCount <= 3)
	{
		perksRand[0] = 0;
		perksRand[1] = 1;
		perksRand[2] = 2;
	}
	else
	{
		perksRand[0] = cry_rand() % perkCount;
		int perksRandNum = 1;

		int curRand = 0, i = 0, bailCount = 0;
		bool found = false;

		while((perksRandNum < 3) || (bailCount > 100))
		{
			found = false;
			curRand = cry_rand() % perkCount;

			for (i=0; i<perksRandNum; i++)
			{
				if (perksRand[i] == curRand)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				perksRand[perksRandNum] = curRand;
				perksRandNum++;
			}

			bailCount++; // Don't get stuck in this loop for too long
		}
	}

	primaryWeaponCount = 0;
	secondaryWeaponCount = 0;
	explosivesWeaponCount = 0;
	perkCount = 0;

	it = m_allItems.begin();
	itEnd = m_allItems.end();

	for(; it != itEnd; ++it)
	{
		if (it->m_category == eELC_PRIMARY_WEAPON)
		{
			if (primaryWeaponCount == primaryRand)
			{
				params.m_contents[primaryWeaponSlotNum] = it->m_uniqueIndex;
			}
			primaryWeaponCount++;
		}
		else if (it->m_category == eELC_SECONDARY_WEAPON)
		{
			if (secondaryWeaponCount == secondaryRand)
			{
				params.m_contents[secondaryWeaponSlotNum] = it->m_uniqueIndex;
			}
			secondaryWeaponCount++;
		}
		else if (it->m_category == eELC_EXPLOSIVE)
		{
			if (explosivesWeaponCount < 2)
			{
				params.m_contents[explosivesWeaponSlotNum+explosivesWeaponCount] = it->m_uniqueIndex;
			}
			explosivesWeaponCount++;
		}
		else if (it->m_category == eELC_PERK)
		{
			for (int i=0; i<3; i++)
			{
				if (perksRand[i] == perkCount)
				{
					params.m_contents[perkWeaponSlotNum+i] = it->m_uniqueIndex;
				}
			}
			perkCount++;
		}
	}
}
#endif

//------------------------------------------------------------------------
void CEquipmentLoadout::ClGetCurrentLoadoutParams( CGameRules::EquipmentLoadoutParams &params )
{
	int numPackage = m_packages.size();
	if ((numPackage > 0) && (numPackage > m_selectedPackage))
	{
		TEquipmentPackageContents &contents = m_packages[m_selectedPackage].m_contents;

		memcpy( (void*)params.m_contents, (void*)contents, sizeof(contents));
	}
}
