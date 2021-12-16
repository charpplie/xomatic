/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Exposes actor functionality to LUA
  
 -------------------------------------------------------------------------
  History:
  - 7:10:2004   14:19 : Created by Márcio Martins

*************************************************************************/
#ifndef __SCRIPTBIND_ACTOR_H__
#define __SCRIPTBIND_ACTOR_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IScriptSystem.h>
#include <ScriptHelpers.h>


struct IGameFramework;
struct IActor;

enum ECloakMode
{
	eCloakMode_Disabled = 0,
	eCloakMode_Enabled
};

// <title Actor>
// Syntax: Actor
class CScriptBind_Actor :
	public CScriptableBase
{
public:
	CScriptBind_Actor(ISystem *pSystem);
	virtual ~CScriptBind_Actor();

	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

	void AttachTo(IActor *pActor);

	//------------------------------------------------------------------------
	virtual int DumpActorInfo(IFunctionHandler *pH);

	virtual int Revive(IFunctionHandler *pH);
	virtual int Kill(IFunctionHandler *pH);
	virtual int RagDollize(IFunctionHandler *pH);
	virtual int SetStats(IFunctionHandler *pH);
	virtual int SetParams(IFunctionHandler *pH);
	virtual int GetParams(IFunctionHandler *pH);
	virtual int GetHeadDir(IFunctionHandler *pH);

	virtual int PostPhysicalize(IFunctionHandler *pH);
	virtual int GetChannel(IFunctionHandler *pH);
	virtual int IsPlayer(IFunctionHandler *pH);
	virtual int IsLocalClient(IFunctionHandler *pH);
	virtual int GetLinkedVehicleId(IFunctionHandler *pH);

	virtual int LinkToEntity(IFunctionHandler *pH);
	virtual int IsGhostPit(IFunctionHandler *pH);
	virtual int IsFlying(IFunctionHandler *pH);
	virtual int SetAngles(IFunctionHandler *pH,Ang3 vAngles );
	virtual int PlayerSetViewAngles( IFunctionHandler *pH,Ang3 vAngles );
	virtual int GetAngles(IFunctionHandler *pH);
	virtual int AddAngularImpulse(IFunctionHandler *pH,Ang3 vAngular,float deceleration,float duration);

	virtual int SimulateOnAction(IFunctionHandler *pH,const char *action,int mode,float value);
	virtual int SetMovementTarget(IFunctionHandler *pH, Vec3 pos, Vec3 target, Vec3 up, float speed);
	virtual int CameraShake(IFunctionHandler *pH,float amount,float duration,float frequency,Vec3 pos);
	virtual int SetViewShake(IFunctionHandler *pH, Ang3 shakeAngle, Vec3 shakeShift, float duration, float frequency, float randomness);
	virtual int VectorToLocal(IFunctionHandler *pH);

	virtual int SetExtensionActivation(IFunctionHandler *pH, const char *extension, bool activation);
	virtual int SetExtensionParams(IFunctionHandler* pH, const char *extension, SmartScriptTable params);
	virtual int GetExtensionParams(IFunctionHandler* pH, const char *extension, SmartScriptTable params);

	// these functions are multiplayer safe
	// these should be called by the server to set ammo on the clients
	virtual int SetInventoryAmmo(IFunctionHandler *pH, const char *ammo, int amount);
	virtual int AddInventoryAmmo(IFunctionHandler *pH, const char *ammo, int amount);
	virtual int GetInventoryAmmo(IFunctionHandler *pH, const char *ammo);

	// these should be called for the local client, and sets ammo through weapon, which is net-synch
	virtual int RefillAllAmmo(IFunctionHandler* pH);

	virtual int SetHealth(IFunctionHandler *pH, float health);
	virtual int SetMaxHealth(IFunctionHandler *pH, float health);
	virtual int GetHealth(IFunctionHandler *pH);
	virtual int GetMaxHealth(IFunctionHandler *pH);
	virtual int GetArmor(IFunctionHandler *pH);
	virtual int GetMaxArmor(IFunctionHandler *pH);
	virtual int DamageInfo(IFunctionHandler *pH, ScriptHandle shooter, ScriptHandle target, ScriptHandle weapon, ScriptHandle projectile, float damage, const char *damageType, Vec3 hitDirection);
	virtual int GetLowHealthThreshold(IFunctionHandler *pH);

	virtual int IsPerkActive(IFunctionHandler *pH, int perkIndex);
	virtual int ActivateNanoSuit(IFunctionHandler *pH, int on);
	virtual int SetNanoSuitMode(IFunctionHandler *pH, int mode);
	virtual int GetNanoSuitMode(IFunctionHandler *pH);
	virtual int GetNanoSuitEnergy(IFunctionHandler *pH);
	virtual int SetNanoSuitEnergy(IFunctionHandler *pH, int energy);
	virtual int NanoSuitHit(IFunctionHandler *pH, int damage, const char *hitType);
	virtual int SendPlayerSuitEvent(IFunctionHandler *pH, unsigned int eventIndex);

	virtual int SetCloakMode(IFunctionHandler *pH, int cloakMode);

	virtual int SetPhysicalizationProfile(IFunctionHandler *pH, const char *profile);
	virtual int GetPhysicalizationProfile(IFunctionHandler *pH);

	virtual int QueueAnimationState(IFunctionHandler *pH, const char *animationState);

	virtual int CreateCodeEvent(IFunctionHandler *pH,SmartScriptTable params);

	virtual int GetCurrentAnimationState(IFunctionHandler *pH);
	virtual int SetAnimationInput( IFunctionHandler *pH, const char * inputID, const char * value );
	int PauseAnimationGraph(IFunctionHandler* pH);
	int ResumeAnimationGraph(IFunctionHandler* pH);
	virtual int TrackViewControlled( IFunctionHandler *pH, int characterSlot );

	virtual int SetSpectatorMode(IFunctionHandler *pH, int mode, ScriptHandle targetId);
	virtual int GetSpectatorMode(IFunctionHandler *pH);
	virtual int GetSpectatorState(IFunctionHandler *pH);
	virtual int GetSpectatorTarget(IFunctionHandler* pH);

	virtual int Fall(IFunctionHandler *pH, Vec3 hitPos);
	virtual int LooseHelmet(IFunctionHandler *pH, Vec3 hitDir, Vec3 hitPos);

	virtual int StandUp(IFunctionHandler *pH);

	virtual int GetExtraHitLocationInfo(IFunctionHandler *pH, int slot, int partId);

	virtual int SetForcedLookDir(IFunctionHandler *pH, CScriptVector dir);
	virtual int ClearForcedLookDir(IFunctionHandler *pH);

	//------------------------------------------------------------------------
	// ITEM STUFF
	//------------------------------------------------------------------------
	virtual int CheckInventoryRestrictions(IFunctionHandler *pH, const char *itemClassName);
	virtual int HolsterItem(IFunctionHandler *pH, bool holster);
	virtual int DropItem(IFunctionHandler *pH, ScriptHandle itemId);
	virtual int PickUpItem(IFunctionHandler *pH, ScriptHandle itemId);

	virtual int SelectItemByName(IFunctionHandler *pH, const char *name);
	virtual int SelectItem(IFunctionHandler *pH, ScriptHandle itemId);
	virtual int SelectLastItem(IFunctionHandler *pH);
	int SelectNextItem(IFunctionHandler *pH, int direction, bool keepHistory, const char *category);

	virtual int GetClosestAttachment(IFunctionHandler *pH, int characterSlot, Vec3 testPos, float maxDistance, const char* suffix);
	virtual int AttachVulnerabilityEffect(IFunctionHandler *pH, int characterSlot, int partid, Vec3 hitPos, float radius, const char* effect, const char* attachmentIdentifier);
	virtual int ResetVulnerabilityEffects(IFunctionHandler *pH, int characterSlot);
	virtual int GetCloseColliderParts(IFunctionHandler *pH, int characterSlot, Vec3 hitPos, float radius);
	virtual int CreateIKLimb( IFunctionHandler *pH, int slot, const char *limbName, const char *rootBone, const char *midBone, const char *endBone, int flags);

	int CreateFXBoneAttachment(IFunctionHandler* pH, int iCharacterSlot, const char* szBoneName, const char *szAttachmentName);

	virtual int ResetScores(IFunctionHandler *pH);
	virtual int RenderScore(IFunctionHandler *pH, ScriptHandle player, int kills, int deaths, int ping);

	virtual int SetSearchBeam(IFunctionHandler *pH, Vec3 dir);

	virtual int PlayAction(IFunctionHandler *pH, const char* action);
	virtual int RequestMechanic(IFunctionHandler *pH, const char* mechanic);

	virtual int EnableSwitchingItems(IFunctionHandler *pH, bool enable);
	virtual int EnableIronSights(IFunctionHandler *pH, bool enable);
	virtual int EnablePickingUpItems(IFunctionHandler *pH, bool enable);

	int ForceItemSuffix( IFunctionHandler *pH, const char* suffix);
	int ResetItemSuffix( IFunctionHandler *pH);
		
	//misc
	//virtual int MeleeEffect(IFunctionHandler *pH);

protected:
	class CActor *GetActor(IFunctionHandler *pH);

	SmartScriptTable m_pParams;

	ISystem					*m_pSystem;
	IGameFramework	*m_pGameFW;

	void CloakRenderProxy(IEntityRenderProxy* pRenderProxy, ECloakMode cloakMode);
	void CloakEntity(IEntity* pEntity, ECloakMode cloakMode);
	void CloakCurrentItemForActor(CActor* pActor, ECloakMode cloakMode);

};

#endif //__SCRIPTBIND_ACTOR_H__
