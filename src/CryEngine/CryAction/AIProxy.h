/********************************************************************
  CryGame Source File.
  Copyright (C), Crytek Studios, 2001-2009.
 -------------------------------------------------------------------------
  File name:   AIProxy.h
  Version:     v1.00
  Description: 
  
 -------------------------------------------------------------------------
  History:
  - 7:10:2004   18:54 : Created by Kirill Bulatsev
	- 2 Mar 2009				: Evgeny Adamenkov: Removed parameter of type IRenderer in DebugDraw

*********************************************************************/
#include DEVIRTUALIZE_HEADER_FIX(AIProxy.h)

#ifndef __AIProxy_H__
#define __AIProxy_H__

#pragma once

#include <IAgent.h>
#include "IWeapon.h"
#include "IGameObject.h"

struct IAISignalExtraData;
struct IEntity;
class  CAIHandler;
class CommunicationHandler;

//! AIProxy listener
struct IAIProxyListener
{
	virtual ~IAIProxyListener() {}
	virtual void OnAIProxyEnabled(bool bEnabled) = 0;
};


UNIQUE_IFACE struct IFireController
{
	virtual bool RequestFire(bool bFire) = 0;
};


class CAIProxy
	: public IAIActorProxy
	, public IWeaponEventListener
{
// (MATT) Needed to restrict creation to the CryAction AI Proxy management methods {2009/04/06}
friend class CCryAction;

public:
	//------------------  IWeaponEventListener
	virtual void OnShoot(IWeapon *pWeapon, EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType,
		const Vec3 &pos, const Vec3 &dir, const Vec3 &vel);
	virtual void OnStartFire(IWeapon *pWeapon, EntityId shooterId) {};
	virtual void OnFireModeChanged(IWeapon *pWeapon, int currentFireMode) {};
	virtual void OnStopFire(IWeapon *pWeapon, EntityId shooterId) {};
	virtual void OnStartReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType) {};
	virtual void OnEndReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType) {};
	virtual void OnSetAmmoCount(IWeapon *pWeapon, EntityId shooterId) {};
	virtual void OnOutOfAmmo(IWeapon *pWeapon, IEntityClass* pAmmoType) {};
	virtual void OnReadyToFire(IWeapon *pWeapon) {};
	virtual void OnPickedUp(IWeapon *pWeapon, EntityId actorId, bool destroyed){};
	virtual void OnDropped(IWeapon *pWeapon, EntityId actorId);
	virtual void OnMelee(IWeapon* pWeapon, EntityId shooterId) {}
	virtual void OnStartTargetting(IWeapon *pWeapon) {}
	virtual void OnStopTargetting(IWeapon *pWeapon) {}
	virtual void OnSelected(IWeapon *pWeapon, bool selected);
	//------------------  ~IWeaponEventListener


	//------------------  IAIActorProxy
	VIRTUAL int  Update(SOBJECTSTATE *pStates, bool bFullUpdate);
	VIRTUAL bool CheckUpdateStatus();
	VIRTUAL void EnableUpdate(bool enable);
	VIRTUAL bool IsEnabled() const;
	VIRTUAL int  GetAlertnessState() const;
	VIRTUAL bool IsCurrentBehaviorExclusive() const;
	VIRTUAL bool SetCharacter( const char* character, const char* behaviour=NULL );
	VIRTUAL const char* GetCharacter();
	VIRTUAL void QueryBodyInfo( SAIBodyInfo& bodyInfo ) ;
	VIRTUAL bool QueryBodyInfo( const SAIBodyInfoQuery& query, SAIBodyInfo& bodyInfo );
	VIRTUAL void QueryWeaponInfo( SAIWeaponInfo& weaponInfo );
	VIRTUAL EntityId GetLinkedDriverEntityId();
	VIRTUAL bool IsDriver();
	VIRTUAL EntityId GetLinkedVehicleEntityId();
	virtual void Release();
	VIRTUAL void Reset(EObjectResetType type);

	VIRTUAL IAICommunicationHandler* GetCommunicationHandler();

	// This will internally keep a counter to allow stacking of such commands
	VIRTUAL void SetForcedExecute(bool forced);
	VIRTUAL bool IsForcedExecute() const;

	VIRTUAL void Serialize( TSerialize ser );
	VIRTUAL IPhysicalEntity* GetPhysics(bool wantCharacterPhysics=false);
	VIRTUAL void DebugDraw(int iParam = 0);
	VIRTUAL const char* GetCurrentReadibilityName() const;
	VIRTUAL void GetWorldBoundingRect(Vec3& FL, Vec3& FR, Vec3& BL, Vec3& BR, float extra=0) const;
	VIRTUAL bool SetAGInput(EAIAGInput input, const char* value);
	VIRTUAL bool ResetAGInput(EAIAGInput input);
	VIRTUAL bool IsSignalAnimationPlayed( const char* value );
	VIRTUAL bool IsActionAnimationStarted( const char* value );
	VIRTUAL bool IsAnimationBlockingMovement() const;
	VIRTUAL EActorTargetPhase GetActorTargetPhase() const;
	virtual void PlayAnimationAction( const struct IAIAction* pAction, int goalPipeId );
	VIRTUAL void AnimationActionDone( bool succeeded );
	VIRTUAL int  GetAndResetShotBulletCount() { int ret = m_shotBulletCount; m_shotBulletCount = 0; return ret; }  // Only used in firecommand
	VIRTUAL void EnableWeaponListener(bool needsSignal);
	VIRTUAL void UpdateMind(SOBJECTSTATE *state);
	VIRTUAL bool IsDead() const;
	VIRTUAL int  GetActorHealth();
	VIRTUAL int  GetActorMaxHealth();
	VIRTUAL int  GetActorArmor();
	VIRTUAL int  GetActorMaxArmor();
	VIRTUAL bool GetActorIsFallen() const;
	VIRTUAL IWeapon* GetCurrentWeapon() const;
	VIRTUAL IWeapon* GetSecWeapon( ERequestedGrenadeType prefGrenadeType=eRGT_ANY ) const;
	VIRTUAL AIWeaponDescriptor GetSecWeaponDescriptor(ERequestedGrenadeType prefGrenadeType) const;
	VIRTUAL IEntity* GetGrabbedEntity() const;
	VIRTUAL bool IsUpdateAlways() const {return m_UpdateAlways;}
	VIRTUAL bool IfShouldUpdate() {return m_pGameObject->ShouldUpdate();}
	

	VIRTUAL	const char* GetVoiceLibraryName() const;
	VIRTUAL	const char* GetCommunicationConfigName() const;
	VIRTUAL const char* GetBehaviorSelectionTreeName() const;

	VIRTUAL bool PredictProjectileHit(const Vec3& throwDir, float vel, Vec3& posOut, float& speedOut,
		ERequestedGrenadeType prefGrenadeType = eRGT_ANY, Vec3* pTrajectory = 0, unsigned int* trajectorySizeInOut = 0);
	VIRTUAL int PlayReadabilitySound(const char* szReadability, bool stopPreviousSound, float responseDelay = 0.0f);
	VIRTUAL void TestReadabilityPack(bool start, const char* szReadability);
	VIRTUAL void GetReadabilityBlockingParams(const char* text, float& time, int& id);
	VIRTUAL void SetReadabilitySound(tSoundID nSoundId, bool bStopPrevious);
	VIRTUAL bool QueryCurrentAnimationSpeedRange(float& smin, float& smax);
	VIRTUAL const char* GetCurrentBehaviorName() const;
	VIRTUAL const char* GetPreviousBehaviorName() const;
	VIRTUAL void UpdateMeAlways(bool doUpdateMeAlways);
	//------------------  ~IAIActorProxy


	void SetMinFireTime(float fTime) {m_fMinFireTime = fTime;}
	CAIHandler*	GetAIHandler() { return m_pAIHandler; }
	void AddListener(IAIProxyListener *pListener);
	void RemoveListener(IAIProxyListener *pListener);
	CTimeValue GetEstimatedAGAnimationLength(EAIAGInput input, const char* value);

	void SetBehaviour(const char* szBehavior, const IAISignalExtraData* pData = 0);

protected:
	// (MATT) We want to keep these in sync with the m_aiProxyMap structure, so keep access restricted {2009/04/06}
	CAIProxy(IGameObject *pGameObject);
	virtual ~CAIProxy(void);

	IActor*	GetActor() const;
	void SendSignal(int signalID, const char * szText, IEntity *pSender, const IAISignalExtraData* pData);
	void UpdateAuxSignal(SOBJECTSTATE *state);
	IFireController* GetFireController();
	void	UpdateShooting(SOBJECTSTATE *pStates, bool isAlive);

	static float LinearInterp(float x, float k, float A, float B, float C);
	static float QuadraticInterp( float x, float k, float A, float B, float C );
	static float GetTofPointOnParabula(const Vec3& A, const Vec3& V, const Vec3& P, float g);
	static Vec3  GetPointOnParabula(const Vec3& A, const Vec3& V, float t, float g);
	static bool  IntersectSweptSphereWrapper(Vec3 *hitPos, float& hitDist, const Lineseg& lineseg, float radius,IPhysicalEntity** pSkipEnts=0, int nSkipEnts=0, int additionalFilter = 0);

	// (MATT) We currently have no way to serialise this. It may be possible to recovery from the entity. Currently must keep between serialisations {2009/04/30}
	IGameObject	*m_pGameObject;

	// (MATT) Is deleted/recreated across serialisations {2009/04/30}
	CAIHandler	*m_pAIHandler;

	bool m_firing;
	bool m_firingSecondary;

	string	m_readibilityName;
	CTimeValue	m_readibilityDelay;
	bool		m_readibilityPlayed;
	int			m_readibilityPriority;
	int			m_readibilityId;
	int			m_readibilityIgnored;

	string m_commConfigName;
	string m_voiceLibrary;
	string m_behaviorSelectionTreeName;

	int8		m_forcedExecute;

	// weapon/shooting related
	float		m_fMinFireTime; //forces a minimum time for fire trigger to stay on
	bool		m_WeaponShotIsDone;	//if weapon did shot during last update
	bool		m_NeedsShootSignal;

	int			m_shotBulletCount;
	CTimeValue m_lastShotTime;

	int			m_PrevAlertness;
	bool		m_IsDisabled;
	bool		m_UpdateAlways;
	bool		m_IsWeaponForceAlerted;

	// (MATT) Mutable for cached lookup {2009/01/27}
	mutable IActor	*m_pIActor;

	// (MATT) Not serialised, which might be a source of bugs. {2009/04/30}
	int			m_animActionGoalPipeId;

	// Listeners
	typedef std::vector<IAIProxyListener*> TListeners;
	TListeners m_listeners;

	std::auto_ptr<CommunicationHandler> m_commHandler;
};


#endif __AIProxy_H__
