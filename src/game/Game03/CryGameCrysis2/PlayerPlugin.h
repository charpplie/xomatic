/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
Description:
Base class for creating additional optional player functionality
**************************************************************************/

#ifndef __PLAYER_PLUGIN_H__
#define __PLAYER_PLUGIN_H__

#include "NanoSuit_v2.h"
#include "AutoEnum.h"

struct HitInfo;

class CPlayer;
class CProjectile;

// Feel free to turn this on and off locally to make sure code compiles with player plugin debugging both on and off...
#define ENABLE_PLAYER_PLUGIN_DEBUGGING			(1)

// Should only use "#if PLAYER_PLUGIN_DEBUGGING" in code, never use ENABLE_PLAYER_PLUGIN_DEBUGGING directly!
#define PLAYER_PLUGIN_DEBUGGING             !(defined(_RELEASE) || defined(PROFILE) || !ENABLE_PLAYER_PLUGIN_DEBUGGING)

#define PlayerPluginEventList(f)           \
	f(EPE_Reset)                             \
	f(EPE_OverrideHealth)                    \
	f(EPE_SelfDestructExplode)               \
	f(EPE_StampMelee)                        \
	f(EPE_OverrideLandVelocity)              \
	f(EPE_OverrideFallVelocity)              \
	f(EPE_Landed)                            \
	f(EPE_Jump)                              \
	f(EPE_ChangedSuitMode)                   \
	f(EPE_ChangedSuitState)                  \
	f(EPE_Pointer)                           \
	f(EPE_OrbitalStrike)                     \
	f(EPE_OrbitalStrikeFromMap)              \
	f(EPE_Spawn)                             \
	f(EPE_Die)                               \
	f(EPE_ClientGainedXP)                    \
	f(EPE_SetTeam)                           \
	f(EPE_ScopeActive)                       \
	f(EPE_IronSightActive)                   \
	f(EPE_OverrideSwayAmount)                \
	f(EPE_AnimationEvent)                    \
	f(EPE_DamageHandlingTarget)              \
	f(EPE_DamageHandlingShooter)             \
	f(EPE_DamageHandlingFeedback)            \
	f(EPE_Footstep)                          \
	f(EPE_OverrideMovementStateData)         \
	f(EPE_SvActivateMissileCountermeasures)  \
	f(EPE_FlashbangScale)                    \
	f(EPE_PlayerCloaking)                    \
	f(EPE_SetCloak)                          \
	f(EPE_OverrideStealthEnergyParams)       \
	f(EPE_OverrideFootstepSoundScale)        \
	f(EPE_BulletTrail)                       \
	f(EPE_GrenadeLaunched)                   \
	f(EPE_GrenadeDestroyed)                  \
	f(EPE_EnterSwimming)                     \
	f(EPE_ActivateTeamPerk)                  \

AUTOENUM_BUILDENUMWITHTYPE_WITHNUM(EPlayerPlugInEvent, PlayerPluginEventList, EPE_Num);

struct STrailInfo
{
	STrailInfo(CProjectile* _pProj, Vec3 _pos)
	{
		pProjectile = _pProj;
		pos = _pos;
	}
	CProjectile* pProjectile;
	Vec3 pos;
};

//---------------------------------------------------------
enum EPlayerPlugInData
{
	EPD_SelfDestructMinigame,
	EPD_Stamp,
	EPD_SelfDestructKiller,
	EPD_MuteFootsteps,
	EPD_MuteJumping,
	EPD_ThreatIcons,
};

//---------------------------------------------------------
// NB: The void * data param in HandleEvent points to an instance of this struct when EPlayerPlugInEvent theEvent == EPE_Pointer, EPE_OrbitalStrike, EPE_StampMelee etc.
struct SOnActionData
{
	SOnActionData(int mode)
	{
		activationMode = mode;
		handled = false;
	}

	int activationMode;
	bool handled;
};

struct SDamageHandling
{
	SDamageHandling(const HitInfo *hit, float dm)
	{
		pHitInfo = hit;
		damageMultiplier = dm;
	}
	float damageMultiplier;
	const HitInfo *pHitInfo;
};

#if PLAYER_PLUGIN_DEBUGGING
#define CVAR_IS_ON(n)                 (g_pGameCVars && g_pGameCVars->n)
#define SET_PLAYER_PLUGIN_NAME(name)  const char * DbgGetPluginName() { return # name; } ILINE name * CheckCorrectNameGiven_ ## name () { return this; }
#define PlayerPluginLog(...)          CryLogAlways                                     ("[PLAYER PLUG-IN] <%s %s %s \"%s\"> %s", DbgGetClassDetails().c_str(), m_ownerPlayer->IsClient() ? "Local" : "Remote", m_ownerPlayer->GetEntity()->GetClass()->GetName(), m_ownerPlayer->GetEntity()->GetName(), string().Format(__VA_ARGS__).c_str())
#define PlayerPluginWatch(...)        CVAR_IS_ON(g_displayDbgText_plugins) && CryWatch ("[PLAYER PLUG-IN] <%s %s %s \"%s\"> %s", DbgGetClassDetails().c_str(), m_ownerPlayer->IsClient() ? "Local" : "Remote", m_ownerPlayer->GetEntity()->GetClass()->GetName(), m_ownerPlayer->GetEntity()->GetName(), string().Format(__VA_ARGS__).c_str())
#define PlayerPluginAssert(cond, ...) CRY_ASSERT_MESSAGE(cond, string().Format         ("[PLAYER PLUG-IN] <%s %s %s \"%s\"> %s", DbgGetClassDetails().c_str(), m_ownerPlayer->IsClient() ? "Local" : "Remote", m_ownerPlayer->GetEntity()->GetClass()->GetName(), m_ownerPlayer->GetEntity()->GetName(), string().Format(__VA_ARGS__).c_str()))
#else
#define SET_PLAYER_PLUGIN_NAME(name)
#define PlayerPluginLog(...)          (void)0
#define PlayerPluginWatch(...)        (void)0
#define PlayerPluginAssert(cond, ...) (void)0
#endif

//---------------------------------------------------------
class CPlayerPlugin
{
public:
	CPlayerPlugin();
	virtual ~CPlayerPlugin();

#if PLAYER_PLUGIN_DEBUGGING
	static const char * s_playerPluginEventNames[];

	virtual const char * DbgGetPluginName() = 0;
	virtual string DbgGetClassDetails() { return DbgGetPluginName(); }
#endif

	ILINE void SetOwnerPlayer(CPlayer * player)
	{
		CRY_ASSERT_MESSAGE(!m_entered, string().Format("%s shouldn't change owner while entered!", DbgGetClassDetails().c_str()));
		CRY_ASSERT_MESSAGE((player == NULL) != (m_ownerPlayer == NULL), string().Format("%s shouldn't change owner from %p to %p!", DbgGetClassDetails().c_str(), m_ownerPlayer, player));
		m_ownerPlayer = player;
	}

	ILINE CPlayer * GetOwnerPlayer()
	{
		return m_ownerPlayer;
	}

	virtual void Enter();
	virtual void Leave();

	virtual void Update(const float dt) {}
	
	virtual const void* GetData(EPlayerPlugInData) { return NULL; } 
	virtual void HandleEvent(EPlayerPlugInEvent theEvent, void * data = NULL);

	ILINE bool IsEntered() { return m_entered; }
	ILINE bool InRequiredSuitMode() { return m_InRequiredSuitMode; }
	virtual void NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) {}

	static const void* DefaultData(EPlayerPlugInData dataType);

protected:
	ENanoSuitMode m_requiredSuitMode;
	CPlayer * m_ownerPlayer;

	void SetRequiredSuitMode(ENanoSuitMode suitMode);

	virtual void InformActiveHasChanged() {}

private:
	bool m_entered;
	bool m_InRequiredSuitMode;

	void SetIsNowActive(bool nowOn);
};

#endif // __PLAYER_PLUGIN_H__
