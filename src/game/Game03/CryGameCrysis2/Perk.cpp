/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description: Global Perk data
	- Uses flags and enums for cheap and efficient checking of perks
	- SPerkData will inevitably be spread all over the code base so keeping management of perks as simple as possible
-------------------------------------------------------------------------
History:
- 28:4:2009: Created by Tim Furnish

*************************************************************************/

#include "StdAfx.h"
#include "Perk.h"
#include "GameCVars.h"

#include "IPerk.h"
#include "CriticalEMPPerk.h"
#include "SelfDestructPerk.h"
#include "SmokeBlastPerk.h"
#include "StampPerk.h"
#include "EMPStrikePerk.h"
#include "SatStrikePerk.h"
#include "PlayerPlugin_Perk_TeamPerkWithTimeout.h"
#include "PlayerPlugin_Perk_SniperCountermeasures.h"
#include "PlayerPlugin_Perk_ModifyDamageBasedOnAngle.h"
#include "PlayerPlugin_Perk_AutoTarget.h"
#include "PlayerPlugin_Perk_ModifyValues.h"
#include "ResistantPerk.h"
#include "ProximityPerk.h"
#include "TrackerPerk.h"
#include "EagleVisionPerk.h"
#include "MissileCountermeasuresPerk.h"
#include "CloakAwarenessPerk.h"
#include "PhantomPerk.h"
#include "SilentFeetPerk.h"
#include "ThreatDetectorPerk.h"
#include "FatalityBonus.h"

#include "HUD/HUD.h"
#include "Player.h"
#include "Game.h"
#include "IUIDraw.h"
#include "PerkIconData.h"
#include "Utility/StringUtils.h"

#if !defined(PROFILE) && !defined(_RELEASE)
#define MAKE_SURE_ALL_PERK_VALUES_LOADED_FROM_XML		1
#else
#define MAKE_SURE_ALL_PERK_VALUES_LOADED_FROM_XML		0
#endif

#if defined(USER_timf) || defined(USER_benp)
#define MAKE_SURE_ALL_PERKS_HAVE_NAMES_AND_DESCRIPTIONS		1
#else
#define MAKE_SURE_ALL_PERKS_HAVE_NAMES_AND_DESCRIPTIONS		0
#endif

//------------------------------------------------------------
// Error reporting
//
static void ReportPerkError_NormalFunc(const char * message, const char * func, int lineNumber)
{
	CryLogAlways ("PERK ERROR! [" __FILE__ " line %d, %s] %s", lineNumber, func, message);
	CRY_ASSERT_MESSAGE(false, message);
}

#define ReportPerkError(...)            ReportPerkError_Func(string().Format(__VA_ARGS__), __FUNC__, __LINE__)
#define ReportPerkErrorAnywhere(...)    CPerk::GetInstance()->ReportPerkError_Func(string().Format(__VA_ARGS__), __FUNC__, __LINE__)

//------------------------------------------------------------
// Pointer to only CPerk instance (it's a singleton class)
//
CPerk * CPerk::s_instance = NULL;

static CPerkIconData s_localPlayerPerkIconData;

AUTOENUM_BUILDNAMEARRAY(SPerkData::s_perkDataFlagNames, PerkDataFlagList);

SPerkData::SPerkData()
{
	m_slot = ePerkSlot_NotInitialised;
	m_validity = ePerkValid_Invalid;
	m_iconTexture = 0;
	m_flag = 0;
}

SPerkData::~SPerkData()
{
#if 0
	// Currently, the only time we destroy a SPerkData is when the game exits - and calling DeleteTexture at that point crashes the engine

	if (m_iconTexture)
	{
		IGameFramework * fw = g_pGame->GetIGameFramework();
		IUIDraw * uiDraw = fw ? fw->GetIUIDraw() : NULL;
		if (uiDraw)
		{
			uiDraw->DeleteTexture(m_iconTexture);
		}
	}
#endif
}

void SPerkData::Init(const char * name)
{
	cry_strncpy(m_nameBuffer, strchr(name, '_') + 1, sizeof(m_nameBuffer));
}

void SPerkData::SetAvailability(bool enable, EPerkCategory s)
{
	m_slot = s;
	m_validity = enable ? ((s == ePerkSlot_Reward) ? ePerkValid_Rewards : ePerkValid_Players) : ePerkValid_Disabled;
}

void SPerkData::SetDefault(bool enable)
{
	if(enable)
	{
		m_flag |= ePerkDataFlag_permanent;
	}
	else
	{
		m_flag &= ~ePerkDataFlag_permanent;
	}
	
}

bool SPerkData::IsDefault() const
{
	return (m_flag & ePerkDataFlag_permanent) == ePerkDataFlag_permanent;
}

void SPerkData::LoadIconTexture(const char * textureName)
{
	if (textureName)
	{
		string fullFilename;
		fullFilename.Format("Textures/UI2/Perks/%s.tif", textureName);
		IGameFramework * fw = g_pGame->GetIGameFramework();
		IUIDraw * uiDraw = fw->GetIUIDraw();

		m_iconTexture = uiDraw->CreateTexture(fullFilename.c_str(), false);
		if (m_iconTexture == 0)
		{
			ReportPerkErrorAnywhere("Perk '%s' failed to load perk icon texture '%s' (full filename '%s')", m_nameBuffer, textureName, fullFilename.c_str());
		}
	}
}

string SPerkData::GetDescription() const
{
	CryFixedStringT<64> strHandle, colourTheName;
	strHandle.Format("@perk_%s_desc", m_nameBuffer);
	colourTheName.Format("$8%s$o", GetShowOnScreenName().c_str());

	return g_pGame->GetHUD()->LocalizeString(strHandle.c_str(), colourTheName.c_str(), NULL);
}

string SPerkData::GetShowOnScreenName() const
{
	CryFixedStringT<64> strHandle;
	strHandle.Format("@perk_%s_name", m_nameBuffer);

	return g_pGame->GetHUD()->LocalizeString(strHandle.c_str(), NULL, NULL);
}

const SPerkCategoryData CPerk::s_categoryList[] =
{
	{"Agility",       "$6", "Yellow",   eNanoSuitMode_Power},
	{"Combat",        "$4", "Red",      eNanoSuitMode_Armor},
	{"Infiltration",  "$5", "Blue",     eNanoSuitMode_Stealth},
	{"Tactical",      "$3", "Green",    eNanoSuitMode_Tactical},
	{"Any",           "$8", "Orange",   eNanoSuitMode_Last},
	{"Reward",        "$8", "Orange",   eNanoSuitMode_Last}
};

#define DEFINE_PERKS(n) m_perkList[n].Init(#n);

CPerk::CPerk(CPerk::TReportPerkErrorMethod errorMethod) :
	ILoadingMessageProvider(& m_loadingMessageProviderListNode)
{
	ReportPerkError_Func = errorMethod ? errorMethod : ReportPerkError_NormalFunc;
	CRY_ASSERT(s_instance == NULL);
	s_instance = this;

	PerkList(DEFINE_PERKS);
	RegisterPerkCommands();

	m_defaultPerksCount = 0;
}

#undef DEFINE_PERKS

#define PERK_VAR_REGISTER(myType, myName)    gEnv->pConsole->Register(# myName, & m_vars.myName, m_vars.myName, 0, "");
#define PERK_VAR_UNREGISTER(myType, myName)  gEnv->pConsole->UnregisterVariable(# myName, false);

CPerk::~CPerk()
{
	CRY_ASSERT(s_instance == this);
	s_instance = NULL;

	if (gEnv->pConsole)
	{
		PERK_CVAR_LIST(PERK_VAR_UNREGISTER)
	}
}

void CPerk::Read(const char * filename)
{
/* //Diesel cut
	XmlNodeRef xmlData = GetISystem()->LoadXmlFile(filename);
	IItemParamsNode *paramNode = g_pGame->GetIGameFramework()->GetIItemSystem()->CreateParams();
	paramNode->ConvertFromXML(xmlData);
	Read(paramNode, filename);
	paramNode->Release();
*/ //Diesel cut
}

#if MAKE_SURE_ALL_PERK_VALUES_LOADED_FROM_XML
	static ILINE void DoLog(const char * name, float val) { /* CryLog("READING PERK VARIABLES: %s = %f", name, val); */ }
	static ILINE void DoLog(const char * name, int val) { /* CryLog("READING PERK VARIABLES: %s = %d", name, val); */ }
	#define PERK_VARIABLE_ENUMIFY(myType, myName) kVarNum_ ## myName,
	#define CHECK_VARIABLE_WAS_READ(myType, myName) if (haveReadVariable[kVarNum_ ## myName] == false) { ReportPerkError("Didn't find value for '%s' in Vars section of file '%s'", # myName, filename); } DoLog(# myName, m_vars.myName);

	enum { PERK_CVAR_LIST(PERK_VARIABLE_ENUMIFY) kVarNum_numVarsTotal };

	#define ADDITIONAL_READ_CODE(myName) \
		if(haveReadVariable[kVarNum_ ## myName]) \
		{ \
			ReportPerkError("Vars section in '%s' contains multiple '%s' tags!", filename, # myName); \
		} \
		haveReadVariable[kVarNum_ ## myName] = true;
#else
	#define ADDITIONAL_READ_CODE(myName)
#endif

#define READ_FROM_XML(myType, myName) \
	if (notDoneYet && 0 == stricmp(# myName, sectionChildName)) \
	{ \
		if (sectionChild->GetAttribute("value", m_vars.myName) == false) \
		{ \
			ReportPerkError("Vars section in '%s' contains a '%s' tag which doesn't specify a value!", filename, # myName); \
		} \
		ADDITIONAL_READ_CODE(myName) \
		notDoneYet = false; \
	}

void CPerk::Read(const IItemParamsNode * xml, const char * filename)
{
#if MAKE_SURE_ALL_PERK_VALUES_LOADED_FROM_XML
	bool haveReadVariable[kVarNum_numVarsTotal];
	memset (& haveReadVariable, 0, sizeof(haveReadVariable));

	// TODO: PerkLock console command currently not functioning... when (if? ) it returns, it might not even use these variables.
	// However, let's keep 'em for the time being; they should be happy to default to 0 and don't need to be read in from the XML file.
	haveReadVariable[kVarNum_perk_lockedPerksA] = true;
	haveReadVariable[kVarNum_perk_lockedPerksB] = true;
#endif

	// Clear memory outside of all conditional bits!
	// Better to have it zero'd if the load fails than to have it full of random values...
	memset (& m_vars, 0, sizeof(m_vars));

	// Remove old CVars
	if (gEnv->pConsole)
	{
		PERK_CVAR_LIST(PERK_VAR_UNREGISTER)
	}

	if (xml)
	{
		const int childCount = xml->GetChildCount();
		int countDefaultPerks = 0;

		for (int iChild = 0; iChild < childCount; ++iChild)
		{
			const IItemParamsNode * section = xml->GetChild(iChild);
			const char * sectionName = section->GetName();
			const int sectionChildCount = section->GetChildCount();

			CryLog ("Reading perks file \"%s\" section #%d: \"%s\" (with %d children)\n", filename, iChild, sectionName, sectionChildCount);
			if (0 == stricmp("Vars", sectionName))
			{
				for (int j = 0; j < sectionChildCount; ++ j)
				{
					const IItemParamsNode * sectionChild = section->GetChild(j);
					const char * sectionChildName = sectionChild->GetName();
					bool notDoneYet = true;

					PERK_CVAR_LIST(READ_FROM_XML) // This macro sets notDoneYet to false if it's found...
					
					if (notDoneYet)
					{
						ReportPerkError("Unexpected tag '%s' found in Vars section of '%s'", sectionChildName, filename);
					}
				}
			}
			else if(0 == stricmp("PreLoadedEffects", sectionName))
			{
				for(int j=0; j<sectionChildCount; j++)
				{
					const IItemParamsNode * effectToPreload = section->GetChild(j);
					const char* effectName = effectToPreload->GetAttribute("name");
					gEnv->pParticleManager->FindEffect(effectName);
				}
			}
			else if (0 == stricmp("PerkList", sectionName))
			{
				for(int j=0; j<sectionChildCount; j++)
				{
					const IItemParamsNode * perkData = section->GetChild(j);
					if (0 == stricmp("DefinePerk", perkData->GetName()))
					{
						int enable = 0;
						perkData->GetAttribute("enable", enable);
						const char * perkName = perkData->GetAttribute("ID");
						const char * slotName = perkData->GetAttribute("category");
						EPerks perkID = FindPerkNumberByName(perkName);
						EPerkCategory category = FindPerkCategoryByName(slotName);

						if (perkID == ePerk_Null)
						{
							ReportPerkError("There's no perk by the name of '%s' - skipping tag %d of PerkList in '%s'", perkName, j, filename);
						}
						else if (category == ePerkSlot_NotInitialised)
						{
							ReportPerkError("There's no category by the name of '%s' for perk '%s' - skipping tag %d of PerkList in '%s'", slotName, perkName, j, filename);
						}
						else
						{
							if(!gEnv->pSystem->IsDedicated())
							{
								m_perkList[perkID].LoadIconTexture(perkData->GetAttribute("iconTexture"));
							}
							m_perkList[perkID].SetAvailability(enable ? true : false, category);

							int defaultPerk = 0;

							if(perkData->GetAttribute("default", defaultPerk))
							{
								countDefaultPerks += defaultPerk ? 1 : 0;
							}

							if(defaultPerk)
							{
								if (category == ePerkSlot_Reward)
								{
									ReportPerkError("Shouldn't set perk %s as being both a reward AND on by default!", perkName);
								}
								else
								{
									m_perkList[perkID].SetDefault(defaultPerk ? true : false);
									AddDefaultPerk(perkID);
								}
							}
						}
					}
					else
					{
						ReportPerkError("Only expected 'DefinePerk' tags within PerkList in '%s', but found a '%s' tag...", filename, perkData->GetName());
					}
				}
			}
			else
			{
				ReportPerkError("Unexpected section '%s' (with %d children) found in file '%s'", sectionName, sectionChildCount, filename);
			}
		}

		CryLog ("[PERK] %d perks are marked as defaults (can cope with up to %d)", countDefaultPerks, CPerkChoice::k_maxPermaPerks);
		CRY_ASSERT_MESSAGE(countDefaultPerks <= CPerkChoice::k_maxPermaPerks, string().Format("%d perks marked as permanent/default, but can only cope with up to %d", countDefaultPerks, CPerkChoice::k_maxPermaPerks));
	}
	else
	{
		ReportPerkError("Failed to open or parse '%s'", filename);
	}

#if MAKE_SURE_ALL_PERK_VALUES_LOADED_FROM_XML
	PERK_CVAR_LIST(CHECK_VARIABLE_WAS_READ)
#endif

#if MAKE_SURE_ALL_PERKS_HAVE_NAMES_AND_DESCRIPTIONS
	for (int i = 0; i < ePerk_Last; ++ i)
	{
		const SPerkData * data = GetPerkData(i);
		if (data->m_validity != ePerkValid_Disabled)
		{
			const string nameString = data->GetShowOnScreenName();

			CryLog ("Perk %d ID '%s' name '%s'", i, data->GetIDName(), nameString.c_str());
			if (nameString[0] == '@' || nameString[0] == '\0')
			{
				ReportPerkError ("%s perk has no name specified in string look-up table: \"%s\"", data->GetIDName(), nameString.c_str());
			}

			// We currently don't need to provide a description for perks given as rewards, although this may change... [TF]
			if (data->m_validity != ePerkValid_Rewards)
			{
				const string descString = data->GetDescription();
				if (descString[0] == '@' || descString[0] == '\0')
				{
					ReportPerkError ("%s perk has no description specified in string look-up table: \"%s\"", data->GetIDName(), descString.c_str());
				}
			}
		}
		else
		{
			CryLog ("Perk %d ID '%s' disabled", i, data->GetIDName());
		}
	}
#endif

	// Add CVars
	if (gEnv->pConsole)
	{
		PERK_CVAR_LIST(PERK_VAR_REGISTER)
	}
}

unsigned int CPerk::Count( const EPerkValidFor valid_for )
{
	unsigned int count = 0;

	// count the valid perks returned by GetNext.
	
	for( EPerks pid = Begin( valid_for ); 
			 pid != End( );
			 pid = GetNextId( pid, valid_for ) )
	{
		count++;
	}
	return count;
}

const SPerkData* CPerk::GetPerkData( const EPerks id )
{
	if( id >= 0 && id < End() )
	{
		return &(m_perkList[id]);
	}

	ReportPerkError("Bad value %d passed into GetPerkData", id);
	return NULL;
}

ENanoSuitMode CPerk::GetPerkRequiredSuitMode(EPerks perk)
{
	EPerkCategory category = GetPerkData(perk)->m_slot;
	CRY_ASSERT_MESSAGE(category >= 0 && category < ePerkSlot_Last, string().Format("Perk %d has invalid perk category %d! Valid range is 0 to %d", perk ,category, ePerkSlot_Last - 1));
	return s_categoryList[category].m_nanosuitMode;
}

EPerks CPerk::FindPerkNumberByName(const char * name)
{
	for (int i = 0; i < ePerk_Last; ++ i)
	{
		if (strcmpi(m_perkList[i].GetIDName(), name) == 0)
		{
			return (EPerks) i;
		}
	}

	return ePerk_Null;
}

EPerkCategory CPerk::FindPerkCategoryByName(const char * name)
{
	for (int i = 0; i < ePerkSlot_Last; ++ i)
	{
		if (strcmpi(s_categoryList[i].m_name, name) == 0)
		{
			return (EPerkCategory) i;
		}
	}

	return ePerkSlot_NotInitialised;
}

EPerkValidFor CPerk::IsPerkAllowed( const EPerks id_flag )
{
	CRY_FIXME(14,8,2009, "This function shouldn't support bad EPerks values being passed in! This should be an assert rather than a check which goes through to the final build!");

	if(id_flag >= 0 && id_flag < ePerk_Last)
	{
		return GetPerkData(id_flag)->m_validity;
	}

	return ePerkValid_Invalid;
}

EPerks CPerk::Begin( const EPerkValidFor valid_for )
{
	return GetNextId( static_cast<EPerks>(-1), valid_for );
}

EPerks CPerk::GetNextId( const EPerks id_flag, const EPerkValidFor valid_for )
{
	int retid = (int) id_flag;
	EPerks nextPerk = (EPerks) retid;
	bool invalid = true;
	while( invalid && nextPerk < ePerk_Last )
	{
		nextPerk = (EPerks) ++retid;
		invalid = !(valid_for & IsPerkAllowed( nextPerk ));
	}

	return nextPerk < ePerk_Last ? nextPerk : ePerk_Last;
}

int CPerk::LookUpLoadingMessage(int getNum, string * toHere) const
{
	int total = 0;

	for (int i = 0; i < ePerk_Last; ++ i)
	{
		const SPerkData * data = s_instance->GetPerkData(i);

		// TODO: When have support for perks being locked and then earned through play, skip locked perks!
		if (data->m_validity != ePerkValid_Disabled && data->m_validity != ePerkValid_Rewards)
		{
			if (getNum == 0)
			{
				*toHere = data->GetDescription();
				return -1;
			}

			++ total;
			getNum --;
		}
	}

	assert (toHere == NULL);
	return total;
}

int CPerk::GetNumMessagesProvided() const
{
	return LookUpLoadingMessage();
}

string CPerk::GetMessageNum(int num) const
{
	string response;
	LookUpLoadingMessage(num, & response);
	return response;
}

void CPerk::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if(aspect == eEA_GameServerC)
	{
		ser.Value("lockedPerksA", m_vars.perk_lockedPerksA, 'i32');
		ser.Value("lockedPerksB", m_vars.perk_lockedPerksB, 'i32');
	}
}

CPerkChoice::CPerkChoice(uint64 initialValue)
{
	m_perkFlags = initialValue;
	m_allowed = true;
}

IPerk* CPerkChoice::AllocateInstance(CPlayer * player, EPerks perk)
{
#define GENERATE_INSTANCE(ePerk, instance) if(perk == ePerk) { IPerk* pInstance = new instance; pInstance->SetOwnerPlayer(player); pInstance->SetPerkID(perk); return pInstance; }

	const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();

	GENERATE_INSTANCE(ePerk_CriticalEMP, CriticalEMPPerk);
	GENERATE_INSTANCE(ePerk_SelfDestruct, SelfDestructPerk);
	GENERATE_INSTANCE(ePerk_SmokeBlast, SmokeBlastPerk);
	GENERATE_INSTANCE(ePerk_Stamp, StampPerk);
	GENERATE_INSTANCE(ePerk_EMPStrike, EMPStrikePerk);
	GENERATE_INSTANCE(ePerk_SatelliteStrike, SatStrikePerk);
	GENERATE_INSTANCE(ePerk_Countermeasures, CPlayerPlugin_Perk_SniperCountermeasures);
	GENERATE_INSTANCE(ePerk_TeamSuitBoost, CPlayerPlugin_Perk_TeamPerkWithTimeout(eTeamPerk_SuitBoost, perkVars->perk_teamSuitBoost_timeUntilDeactivate, "SuitBoostStart", "SuitBoostStop", "SuitBoostLoop"));
	GENERATE_INSTANCE(ePerk_TeamSuperRadar, CPlayerPlugin_Perk_TeamPerkWithTimeout(eTeamPerk_Radar, perkVars->perk_teamRadar_timeUntilDeactivate, "UnlockRadar"));
	GENERATE_INSTANCE(ePerk_FragResistant, ResistantPerk(EPE_DamageHandlingTarget, "frag", &perkVars->perk_FragResistant_damageMultiplier, "Perk_FragResistant", NULL));
	GENERATE_INSTANCE(ePerk_MeleeDefense, ResistantPerk(EPE_DamageHandlingTarget, "melee", &perkVars->perk_meleeDefense_damageMultiplier, "Perk_MeleeDefence", "perk_fx.melee_deflect.melee_deflect"));
	GENERATE_INSTANCE(ePerk_ArmourPiercing, ResistantPerk(EPE_DamageHandlingShooter, "bullet", &perkVars->perk_armourPiercing_damageMultiplier, "Perk_ArmourPiercing", NULL));
	GENERATE_INSTANCE(ePerk_Deflection, ResistantPerk(EPE_DamageHandlingTarget, "bullet", &perkVars->perk_deflection_damageMultiplier, "Perk_Deflection", "perk_fx.deflection.deflect"));
	GENERATE_INSTANCE(ePerk_ProximityAlarm, ProximityPerk(&perkVars->perk_proximityAlarm_range, &perkVars->perk_proximityAlarm_scanSpeedTierOne, &perkVars->perk_proximityAlarm_scanSpeedTierTwo, &perkVars->perk_proximityAlarm_scanSpeedTierThree));
	GENERATE_INSTANCE(ePerk_RearDeflection, CPlayerPlugin_Perk_ModifyDamageBasedOnAngle(EPE_DamageHandlingTarget, "bullet", &perkVars->perk_rearDeflection_triggerWhenDotIsAbove, &perkVars->perk_rearDeflection_damageMultiplierWhenBehind, "Perk_RearDeflection", "perk_fx.deflection.deflect"));
	GENERATE_INSTANCE(ePerk_Tracker, TrackerPerk("perk_fx.tracker.tracker"));
	GENERATE_INSTANCE(ePerk_AutoTarget, CPlayerPlugin_Perk_AutoTarget);
	GENERATE_INSTANCE(ePerk_WeaponsSpread, CPlayerPlugin_Perk_WeaponsSpread);
	GENERATE_INSTANCE(ePerk_WeaponsTraining, CPlayerPlugin_Perk_WeaponsTraining);
	GENERATE_INSTANCE(ePerk_SuperStrength, CPlayerPlugin_Perk_SuperStrength);
	GENERATE_INSTANCE(ePerk_EagleVision, EagleVisionPerk);
	GENERATE_INSTANCE(ePerk_MissileCountermeasures, MissileCountermeasuresPerk);
	GENERATE_INSTANCE(ePerk_CloakAwareness, CloakAwarenessPerk(&perkVars->perk_cloakAwareness_Range, &perkVars->perk_cloakAwareness_TimeTillFlickerTierOne, &perkVars->perk_cloakAwareness_TimeTillFlickerTierTwo, &perkVars->perk_cloakAwareness_TimeTillFlickerTierThree, &perkVars->perk_cloakAwareness_FlickerTime));
	GENERATE_INSTANCE(ePerk_Phantom, PhantomPerk(&perkVars->perk_Phantom_scaleEnergyConsumptionTierOne, &perkVars->perk_Phantom_scaleEnergyConsumptionTierTwo, &perkVars->perk_Phantom_scaleEnergyConsumptionTierThree));
	GENERATE_INSTANCE(ePerk_SilentFeet, SilentFeetPerk);
	GENERATE_INSTANCE(ePerk_ThreatDetector, ThreatDetectorPerk);
	GENERATE_INSTANCE(ePerk_FatalityBonus, FatalityBonus);

	return NULL;
#undef GENERATE_INSTANCE
}

bool CPerkChoice::SetPerkActiveInternal(EPerks perkIndex, const bool enable)
{
	assert (perkIndex > ePerk_Null && perkIndex < ePerk_Last);
	CPerk * perkInstance = CPerk::GetInstance();
	const SPerkData * perkData = perkInstance->GetPerkData(perkIndex);
	const SPerkCategoryData * slotData = & CPerk::s_categoryList[perkData->m_slot];

	if (!m_allowed && enable)
	{
		CryLog("Perks have been disallowed for this local player.");
		return false;
	}

	if (enable == IsPerkActive(perkIndex))
	{
		CryLog("    %s[%s]$1 %s is already %s\n", slotData->m_colour, slotData->m_name, perkData->GetIDName(), enable ? "activated" : "deactivated");
		return false;
	}

	if(enable)
	{
#if PERK_USE_SLOT_SYSTEM
		uint64 bit = 1;
		for(int i = 0; i < ePerk_Last; i++)
		{
			if(perkInstance->GetPerkData(i)->m_slot == perkData->m_slot && IsPerkBitActive(bit))
			{
				m_perkFlags &= ~PERKBIT(i);
				CryLog("    %s[%s]$1 Deactivated %s for %s\n", slotData->m_colour, slotData->m_name, perkInstance->GetPerkData(i)->m_name.c_str(), perkData->m_name.c_str());
			}
			bit <<= 1;
		}
#else
		if(perkData->m_validity != ePerkValid_Rewards && !perkData->IsDefault())
		{
			// Only count non team perks and non default perks!

			int numNonTeamPerks = CountPerksActive() - CountPerksOfTypeActive(ePerkSlot_Reward) - CountDefaultPerks();
			uint64 bit = 1;
			int i = 0;

			// Only disable non team perks and non default perks!
			while (numNonTeamPerks >= k_maxPickedPerks)
			{
				CRY_ASSERT(i < ePerk_Last);
				const SPerkData * nthPerkData = perkInstance->GetPerkData(i);
				if (IsPerkBitActive(bit) && (nthPerkData->m_slot != ePerkSlot_Reward) && (!nthPerkData->IsDefault()) )
				{
					SetPerkActiveInternal((EPerks)i, false);
					-- numNonTeamPerks;
				}
				bit <<= 1;
				++ i;
			}
		}
		else if(perkData->m_validity == ePerkValid_Rewards)
		{
			uint64 bit = 1;

			for(int i = 0; i < ePerk_Last; i++)
			{
				if(IsPerkBitActive(bit) && perkInstance->GetPerkData(i)->m_validity == ePerkValid_Rewards)
				{
					SetPerkActiveInternal((EPerks)i, false);
					break;
				}
				bit <<= 1;
			}
		}

#endif
		m_perkFlags |= PERKBIT(perkIndex);
		CryLog("    %s[%s]$1 Activated %s\n", slotData->m_colour, slotData->m_name, perkData->GetIDName());
	}
	else
	{
		m_perkFlags &= ~PERKBIT(perkIndex);
		CryLog("    %s[%s]$1 Deactivated %s\n", slotData->m_colour, slotData->m_name, perkData->GetIDName());
	}

	CRY_ASSERT_MESSAGE(CountPerksActive() <= k_numberOfPerkIcons, string().Format("%d is too many perks to have active (%d are rewards, %d are defaults): %s", CountPerksActive(), CountPerksOfTypeActive(ePerkSlot_Reward), CountDefaultPerks(), GetListAsString().c_str()));

	return true;
}

bool CPerkChoice::SetPerkActive(EPerks perkIndex, bool enable)
{
	if (perkIndex <= ePerk_Null || perkIndex >= ePerk_Last)
	{
		GameWarning("Perk index %d is outside valid range!", perkIndex);
		return false;
	}

	if (enable)
	{
		CPerk * perkInstance = CPerk::GetInstance();

		if ((g_pGameCVars && ((perkInstance->GetLockedPerksBitfield() & PERKBIT(perkIndex)) || perkInstance->GetVars()->perk_disable)))
		{
			CryLog ("Can't enable perk '%s' because that perk is locked by the server!", perkInstance->GetPerkData(perkIndex)->GetIDName());
			enable = false;
		}
	}

	return SetPerkActiveInternal(perkIndex, enable);
}

void CPerk::AddDefaultPerksToBitfield(uint64 &bitfield)
{
#if !defined(_RELEASE)
	int defaultCount = 0;
#endif
	for(int i = 0; i < ePerk_Last; i++)
	{
		EPerks perk = (EPerks) i;
		if(GetPerkData(perk)->IsDefault())
		{
			bitfield |= PERKBIT(i);
#if !defined(_RELEASE)
			defaultCount++;
#endif
		}
	}
#if !defined(_RELEASE)
	CRY_ASSERT_MESSAGE(defaultCount == m_defaultPerksCount, "Something's gone wrong initializing default perks");
#endif

}

int CPerkChoice::CountPerksActive() const
{
	int total = 0;
	uint64 bit = 1;

	for(int i = 0; i < ePerk_Last; i++)
	{
		total += (IsPerkBitActive(bit)) ? 1 : 0;
		bit <<= 1;
	}

	return total;
}

int CPerkChoice::CountDefaultPerks() const
{
	int total = 0;
	uint64 bit = 1;

	CPerk * perkInstance = CPerk::GetInstance();

	for(int i = 0; i < ePerk_Last; i++)
	{
		total += (IsPerkBitActive(bit) && perkInstance->GetPerkData(i)->IsDefault()) ? 1 : 0;
		bit <<= 1;
	}

	return total;
}

int CPerkChoice::CountPerksOfTypeActive(EPerkCategory category) const
{
	int total = 0;
	uint64 bit = 1;

	CPerk * instance = CPerk::GetInstance();

	for(int i = 0; i < ePerk_Last; i++)
	{
		total += (IsPerkBitActive(bit) && instance->GetPerkData(i)->m_slot == category) ? 1 : 0;
		bit <<= 1;
	}

	return total;
}

string CPerkChoice::GetListAsString() const
{
	string output = "";
	bool got = false;
	uint64 bit = 1;

	for(int i = 0; i < ePerk_Last; i++)
	{
		if (IsPerkBitActive(bit))
		{
			if (got)
			{
				output += "+";
			}
			else
			{
				got = true;
			}
			output = string(output + CPerk::GetInstance()->GetPerkData(i)->GetIDName());
		}
		bit <<= 1;
	}

	return got ? output : string("none");
}
