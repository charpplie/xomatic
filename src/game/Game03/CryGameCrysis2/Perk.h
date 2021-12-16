/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description: Global Perk data
	- Uses flags and enums for cheap and efficient checking of perks
	- Perks will inevitably be spread all over the code base so keeping management of perks as simple as possible
-------------------------------------------------------------------------
History:
- 13:3:2009: Created by Ben Parbury

*************************************************************************/

#ifndef __PERK_H__
#define __PERK_H__

#include "NanoSuitDefs.h"
#include "AutoEnum.h"
#include "FrontEnd/ILoadingMessageProvider.h"

class CPlayer;

#define PERKBIT(n) (1ll<<(n))

#define PERK_STRIKE_POINTER_DEPLOYMENT (1)
#define PERK_USE_SLOT_SYSTEM           (0)

//STRINGS AND BOOLS ARE NOT SAFE TYPES TO USE BELOW.
//DONT.

#define PERK_CVAR_LIST(f) \
	f(int	,		  perk_lockedPerksA) \
	f(int,			perk_lockedPerksB) \
	f(int,			perk_disable) \
	/* Icons */ \
	f(float,    perk_icon_pulse_time) \
	/* Sonar vision / threat trails / cloak awareness */ \
	f(int,			perk_particleTypesGenerateOnWho) \
	f(int,			perk_particleTypesToHide) \
	f(float,		perk_particleBoneChoiceScale) \
	f(float,		perk_particleBoneEmitterSpawnRadius) \
	/* Sonar vision */ \
	f(float,		perk_sonarVision_BoostFadeSpeed) \
	f(float,		perk_sonarVision_IncreaseParticleSizeWhenShoot) \
	f(float,		perk_sonarVision_IncreaseParticleSizeWhenLandOnGround) \
	f(float,		perk_sonarVision_RangeGunshot) \
	f(float,		perk_sonarVision_RangeBody) \
	f(float,		perk_sonarVision_VolumeControlRecoverSpeed) \
	f(float,		perk_sonarVision_VolumeControlLowerSpeed) \
	f(float,		perk_sonarVision_VolumeControlReduceBodyEffects) \
	f(float,		perk_sonarVision_VolumeAffectedByGunshot) \
	f(float,		perk_sonarVision_VolumeAffectedByMovement) \
	f(int,			perk_sonarVision_skelAlignedParticles_numToCreate_footstep) \
	f(int,			perk_sonarVision_skelAlignedParticles_numToCreate_shoot) \
	f(int,			perk_sonarVision_skelAlignedParticles_numToCreate_land) \
	f(int,			perk_sonarVision_skelAlignedParticles_numToCreate_random) \
	f(float,		perk_sonarVision_skelAlignedParticles_awayVelocityMin) \
	f(float,		perk_sonarVision_skelAlignedParticles_awayVelocityMax) \
	f(float,		perk_sonarVision_skelAlignedParticles_upVelocityMin) \
	f(float,		perk_sonarVision_skelAlignedParticles_upVelocityMax) \
	f(float,		perk_sonarVision_skelAlignedParticles_boneEmitterRadius) \
	f(float,		perk_sonarVision_skelAlignedParticles_randomOffsetScale) \
	f(int,			perk_sonarVision_emitterSpawnRatio) \
	f(float,		perk_sonarVision_maxScaleForAllBoneEmitter) \
	f(float,		perk_sonarVision_passiveMaxViewDist) \
	f(float,		perk_sonarVision_activeMaxViewDist) \
	f(float,		perk_sonarVision_perm_interval) \
	f(int,			perk_sonarVision_perm_numCreate) \
	f(float,		perk_sonarVision_perm_maxDist) \
	f(float,		perk_sonarVision_perm_baseScale) \
	f(float,		perk_sonarVision_perm_randScale) \
	/* Cloak awareness */ \
	f(int,			perk_cloakAwareness_skelAlignedParticles_numToCreateWhenMoving) \
	f(int,			perk_cloakAwareness_skelAlignedParticles_numToCreate) \
	f(float,		perk_cloakAwareness_skelAlignedParticles_awayVelocityMin) \
	f(float,		perk_cloakAwareness_skelAlignedParticles_awayVelocityMax) \
	f(float,		perk_cloakAwareness_skelAlignedParticles_upVelocityMin) \
	f(float,		perk_cloakAwareness_skelAlignedParticles_upVelocityMax) \
	f(float,		perk_cloakAwareness_Range) \
	f(float,		perk_cloakAwareness_TimeTillFlickerTierOne) \
	f(float,		perk_cloakAwareness_TimeTillFlickerTierTwo) \
	f(float,		perk_cloakAwareness_TimeTillFlickerTierThree) \
	f(float,		perk_cloakAwareness_FlickerTime) \
	/* Threat trails */ \
	f(float,		perk_threatTrails_BulletRenderRange) \
	f(float,		perk_threatTrails_BulletRenderTime) \
	f(float,		perk_threatTrails_BulletThickness) \
	f(float,		perk_threatTrails_BulletOpacity) \
	f(float,		perk_threatTrails_GrenadeRenderRange) \
	/* Dolphin */ \
	f(float,		perk_dolphinSpeedMult) \
	/* Sniper countermeasures */ \
	f(int	,		  perk_antiSniperEffect) \
	f(float	,	  perk_antiSniperEffectDecreaseRate) \
	f(float,		perk_antiSniperEffectIncreaseRate) \
	f(float,		perk_antiSniperEnergyUseRate) \
	f(float,		perk_antiSniperEnergyRechargeRate) \
	f(float,		perk_antiSniperEnergyMinimum) \
	f(float,		perk_antiSniperSwayMult) \
	f(float,		perk_antiSniperScreenEffectAmount) \
	/* Weapons training*/ \
	f(float,		perk_weaponsTrainingReloadSpeedScale) \
	f(float,		perk_weaponsTrainingSelectSpeedScale) \
	/* Weapons Spread */ \
	f(float,		perk_weaponsSpreadSpreadScale) \
	/* Solar boost */ \
	f(float,		perk_solarBoostScaleEnergyUse) \
	/* Critical energy */ \
	f(float,		perk_criticalEnergyDamagePerSecond) \
	/* Melee defense */ \
	f(float,		perk_meleeDefense_damageMultiplier) \
	/* ArmourPiercing */ \
	f(float,    perk_armourPiercing_damageMultiplier) \
	/* Deflection */ \
	f(float,    perk_deflection_damageMultiplier) \
	/* Rear Deflection */ \
	f(float,    perk_rearDeflection_damageMultiplierWhenBehind) \
	f(float,    perk_rearDeflection_triggerWhenDotIsAbove) \
	/* Superstrength */ \
	f(float,		perk_superStrength_scaleRecoil) \
	f(float,		perk_superStrength_scaleMeleeDamage) \
	/* EMP strike / satellite strike */ \
	f(float,		perk_empSatelliteStrikeHeight) \
	f(float,		perk_empSatelliteStrikeRange) \
	f(float,		perk_empStrikeImpactTime) \
	f(float,		perk_satStrikeImpactTime) \
	/* Critical EMP */ \
	f(int	,		  perk_empHealthTriggerAmount) \
	f(float,		perk_criticalEMPRechargeTime) \
	/* Smoke blast */ \
	f(float,		perk_smokeBlast_disableTagNamesSeconds) \
	/* Chaff */ \
	f(float,		perk_chaffRechargeTime) \
	f(float,		perk_chaffSpread) \
	f(float,		perk_chaffFOV) \
	/* Flashbang protection */ \
	f(float,		perk_countermeasuresFlashBangScale) \
	/* Self destruct */ \
	f(int,			perk_selfDestruct_StartHealth) \
	f(float,		perk_SelfDestruct_minPower) \
	f(float,		perk_SelfDestruct_barSpeed) \
	f(float,		perk_SelfDestruct_damage) \
	f(float,		perk_SelfDestruct_minTime) \
	f(float,		perk_SelfDestruct_maxTime) \
	/* Stamp */ \
	f(float,		perk_Stamp_minDamage) \
	f(float,		perk_Stamp_maxDamage) \
	f(float,		perk_Stamp_minDamageSpeed) \
	f(float,		perk_Stamp_maxDamageSpeed) \
	f(float,		perk_Stamp_minRadius) \
	f(float,		perk_Stamp_maxRadius) \
	f(float,		perk_Stamp_fallspeed) \
	f(float,		perk_Stamp_rechargeTime) \
	f(int,			perk_Stamp_displayPromptUntilStampedThisManyTimes) \
	/* TEAM PERK: suit boost */ \
	f(float,    perk_teamSuitBoost_scaleEnergyUse) \
	f(float,    perk_teamSuitBoost_timeUntilDeactivate) \
  /* TEAM PERK: super radar */ \
	f(float,    perk_teamRadar_timeUntilDeactivate) \
	/* Concentrated Fire */ \
	f(float,		perk_ConcentratedFire_rateScale) \
	/* Resistant */ \
	f(float,		perk_FragResistant_damageMultiplier) \
	/* Proximity Alarm */	\
	f(float,		perk_proximityAlarm_range) \
	f(float,		perk_proximityAlarm_scanSpeedTierOne) \
	f(float,		perk_proximityAlarm_scanSpeedTierTwo) \
	f(float,		perk_proximityAlarm_scanSpeedTierThree) \
	/* Auto Target */ \
	f(float,    perk_autoTarget_assistanceConeSize) \
	f(float,    perk_autoTarget_amountOfConeWhichGivesPerfectAim) \
	f(float,		perk_autoTarget_coneShrinkDistance) \
	/* Silent Feet */ \
	f(float,		perk_silentFeet_scaleFootstepSounds) \
	/* Tracker */ \
	f(float,		perk_tracker_maxDistanceToSpawn) \
	f(float,		perk_tracker_secondsPerFreePlayerSlot) \
	f(float,		perk_tracker_minSeconds) \
	f(float,		perk_tracker_maxSeconds) \
	/* ManeuverMKII */ \
	f(float,		perk_ManeuverMKII_scaleEnergyConsumption) \
	/* Phantom */ \
	f(float,		perk_Phantom_scaleEnergyConsumptionTierOne) \
	f(float,		perk_Phantom_scaleEnergyConsumptionTierTwo) \
	f(float,		perk_Phantom_scaleEnergyConsumptionTierThree) \
	/* FatalityBonus */ \
	f(int,			perk_FatalityBonus_deathStreakRequired) \
	f(int,			perk_FatalityBonus_killStreakToStop) \


#define INSTANCE_PERKS(myType,myName) myType myName;

class IPerk;

enum EPerkValidFor
{
	ePerkValid_Invalid    = 0,
	ePerkValid_Disabled   = PERKBIT(0),
	ePerkValid_Players    = PERKBIT(1),
	ePerkValid_Rewards    = PERKBIT(2),
	ePerkValid_AllEnabled = ePerkValid_Disabled | ePerkValid_Players | ePerkValid_Rewards,
	ePerkValid_AllValid   = ePerkValid_Players | ePerkValid_Rewards,
};

enum EPerkCategory
{
	ePerkSlot_Agility,
	ePerkSlot_Combat,
	ePerkSlot_Infiltration,
	ePerkSlot_Tactical,
	ePerkSlot_Any,
	ePerkSlot_Reward,
	ePerkSlot_Last,
	ePerkSlot_NotInitialised = ePerkSlot_Last
};

#define PerkList(f)                         \
	f(ePerk_SelfDestruct)                     \
	f(ePerk_Stamp)                            \
	f(ePerk_ComboLink)                        \
	f(ePerk_ArmourPiercing)                   \
	f(ePerk_CriticalEMP)                      \
	f(ePerk_ManeuverMKII)                     \
	f(ePerk_WeaponsTraining)                  \
	f(ePerk_SmokeBlast)                       \
	f(ePerk_SilentFeet)                       \
	f(ePerk_Dolphin)                          \
	f(ePerk_AirFriction)                      \
	f(ePerk_ConcentratedFire)                 \
	f(ePerk_ProximityAlarm)                   \
	f(ePerk_Phantom)                          \
	f(ePerk_EagleVision)                      \
	f(ePerk_Tracker)                          \
	f(ePerk_SonarVision)											\
	f(ePerk_ThreatDetector)                   \
	f(ePerk_EnemyStatus)                      \
	f(ePerk_CloakAwareness)                   \
	f(ePerk_FragResistant)                    \
	f(ePerk_Deflection)                       \
	f(ePerk_Countermeasures)                  \
	f(ePerk_MissileCountermeasures)           \
	f(ePerk_RadarJammer)                      \
	f(ePerk_HeavyLoadout)                     \
	f(ePerk_MeleeDefense)                     \
	f(ePerk_SolarCellSkin)                    \
	f(ePerk_SuperStrength)                    \
	f(ePerk_WeaponsSpread)                    \
	f(ePerk_RearDeflection)                   \
	f(ePerk_AutoTarget)                       \
	/* Rewards */                             \
	f(ePerk_SatelliteStrike)                  \
	f(ePerk_EMPStrike)                        \
	f(ePerk_TeamSuperRadar)                   \
	f(ePerk_TeamSuitBoost)                    \
	f(ePerk_FatalityBonus)										\

AUTOENUM_BUILDENUMWITHTYPE_WITHINVALID_WITHNUM(EPerks, PerkList, ePerk_Null, ePerk_Last);

#define PerkDataFlagList(f)                 \
	f(ePerkDataFlag_permanent)                \

struct SPerkData
{
private:
	char						m_nameBuffer[24];

public:
	int							m_flag;
	int							m_iconTexture;
	EPerkCategory		m_slot;
	EPerkValidFor		m_validity;

	SPerkData();
	~SPerkData();
	void Init(const char * name);
	void LoadIconTexture(const char * textureName);
	void SetAvailability(bool enable, EPerkCategory s);
	void SetDefault(bool enable);
	bool IsDefault() const;
	ILINE const char * GetIDName() const
	{
		return m_nameBuffer;
	}
	string GetShowOnScreenName() const;
	string GetDescription() const;

	AUTOENUM_BUILDFLAGS_WITHZERO(PerkDataFlagList, ePerkData_null);
	static const char * s_perkDataFlagNames[];
};

struct SPerkCategoryData
{
	const char * m_name;
	const char * m_colour;
	const char * m_colourName;
	ENanoSuitMode m_nanosuitMode;
};

class CPerk : public ILoadingMessageProvider
{
	public:
	static const SPerkCategoryData	s_categoryList[ePerkSlot_Last];

	typedef void (* TReportPerkErrorMethod) (const char * message, const char * func, int lineNumber);

	CPerk(TReportPerkErrorMethod errorMethod = NULL);
	~CPerk();

	void										Read(const char * filename = "Scripts/PerkInfo/PerkInfo.xml");
	void										Read(const IItemParamsNode * xml, const char * filename = "N/A");

	struct SPerkVars
	{
		PERK_CVAR_LIST(INSTANCE_PERKS)
	};
	
	ILINE uint64 GetLockedPerksBitfield()
	{
		return ((uint32) m_vars.perk_lockedPerksA) + (((uint64)(uint32)m_vars.perk_lockedPerksB) << 32);
	}

	// iteration
	unsigned int     Count( const EPerkValidFor valid_for );
	EPerks           Begin( const EPerkValidFor valid_for );
	ILINE EPerks     End(void) const	{		return ePerk_Last;	}
	EPerks           GetNextId( const EPerks id_flag, const EPerkValidFor valid_for );

	// Data access
	EPerks                  FindPerkNumberByName(const char * name);
	EPerkCategory						FindPerkCategoryByName(const char * name);
	const SPerkData*				GetPerkData( const EPerks id_flag );
	ILINE const SPerkData * GetPerkData(int num) { return GetPerkData((EPerks)num); }
	ENanoSuitMode		        GetPerkRequiredSuitMode(EPerks perk);

	void AddDefaultPerksToBitfield(uint64 &bitfield);

	void NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);
	
	static ILINE CPerk *		GetInstance()
	{
		CRY_ASSERT_MESSAGE (s_instance, "No CPerk instance... GetInstance() is about to return NULL");
		return s_instance;
	}

	ILINE const SPerkVars *	GetVars() const
	{
		return & m_vars;
	}

	int GetDefaultPerksCount()
	{
		return m_defaultPerksCount;
	}

	void AddDefaultPerk(EPerks perk)
	{
		m_defaultPerksCount++;
	}

	TReportPerkErrorMethod	ReportPerkError_Func;

	private :
	// checks
	EPerkValidFor           IsPerkAllowed( const EPerks id_flag );

	static void             CmdPerk(IConsoleCmdArgs* pCmdArgs);
	static void             CmdPerks(IConsoleCmdArgs* pCmdArgs);
	static void             CmdPerkTier(IConsoleCmdArgs* pCmdArgs);
	static void             CmdPerksClear(IConsoleCmdArgs* pCmdArgs);
	static void             CmdPerkUnlock(IConsoleCmdArgs* pCmdArgs);
	static void             CmdPerkLock(IConsoleCmdArgs* pCmdArgs);
	static void             CmdPerkLockUnlock(IConsoleCmdArgs* pCmdArgs, bool lockIt);
	void										RegisterPerkCommands();

	static CPerk *					s_instance;
	SPerkVars								m_vars;
	SPerkData								m_perkList[ePerk_Last];
	int											m_defaultPerksCount;
	
	CLoadingMessageProviderListNode		m_loadingMessageProviderListNode;

	int LookUpLoadingMessage(int getNum = -1, string * toHere = NULL) const;
	int GetNumMessagesProvided() const;
	string GetMessageNum(int num) const;
};

class CPerkChoice
{
public:
	CPerkChoice(uint64 initialValue = 0);
	bool SetPerkActive(EPerks perkIndex, bool enable);
	int CountPerksActive() const;
	int CountDefaultPerks() const;
	int CountPerksOfTypeActive(EPerkCategory category) const;
	string GetListAsString() const;

	ILINE bool IsPerkBitActive(uint64 perkBit) const			{		return ((m_perkFlags & perkBit) == perkBit);			}
	ILINE bool IsPerkActive(EPerks perk) const						{		return IsPerkBitActive(PERKBIT(perk));						}
	ILINE uint64 GetBitfield() const											{		return m_perkFlags;																}
	ILINE void SetBitfield(uint64 newfield)								{		m_perkFlags = newfield;														}
	ILINE const uint64 * GetPointerToBitfield() const			{		return & m_perkFlags;															}
	ILINE uint64 * GetPointerToBitfield()									{		return & m_perkFlags;															}
	ILINE bool operator==(const CPerkChoice & perk)				{		return (m_perkFlags == perk.m_perkFlags);					}
	ILINE bool operator!=(const CPerkChoice & perk)				{		return (m_perkFlags != perk.m_perkFlags);					}

	void CopyFrom(const CPerkChoice & perk)								{		m_perkFlags = perk.m_perkFlags;	}

	ILINE void SetAllowed(bool a)													{		m_allowed = a; }
	ILINE bool IsAllowed()																{		return m_allowed; }

	static bool IsPerkActive(const uint64 perkBitfield, const EPerks perk)
	{
		uint64 perkBit = PERKBIT(perk);
		return (perkBitfield & perkBit) == perkBit;
	}

	static IPerk* AllocateInstance(CPlayer * player, EPerks perk);

	const static int k_maxPickedPerks = 4;
	const static int k_maxRewardPerks = 1;
	const static int k_maxPermaPerks = 7;
	const static int k_maxPerks = k_maxPickedPerks + k_maxPermaPerks + k_maxRewardPerks;

private:
	uint64 m_perkFlags;
	bool m_allowed;

	bool SetPerkActiveInternal(EPerks perkIndex, const bool enable);
};

#endif // __PERK_H__
