/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
not really a "weapon": is just the hands grabbing objects (barrels etc) and throwing them
*************************************************************************/

#pragma once

#ifndef _PICKANDTHROWWEAPON_H_
#define _PICKANDTHROWWEAPON_H_

#include "Weapon.h"
#include "Player.h"
#include "WeaponSharedParams.h"
#include "MeleeCollisionHelper.h"

class CPickAndThrowWeapon :	public CWeapon, public IMeleeCollisionHelperListener
{
	struct FinishMelee;
	struct DoMelee;
	struct DelayedImpulse;
	
public:

	CPickAndThrowWeapon();
	virtual ~CPickAndThrowWeapon();

	virtual void OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	virtual void UpdateFPView( float frameTime );
	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(this, sizeof(*this));
		CWeapon::GetInternalMemoryUsage(s); // collect memory of parent class
	}
	virtual bool CanModify() const;
	virtual void OnSelected( bool selected );
	virtual void Update(SEntityUpdateContext& ctx, int val );
	virtual bool Init(IGameObject * pGameObject);	
	virtual bool UpdateAimAnims( SParams_WeaponFPAiming &aimAnimParams, bool &releaseCameraBone );
	virtual void Reset();
	
	//IMeleeCollisionHelperListener
	virtual void OnSuccesfulHit(ray_hit& hitResult);
	virtual void OnFailedHit(); 
	//~IMeleeCollisionHelperListener

	bool HasObjectGrabHelper(EntityId entityId) const;

	#ifndef _RELEASE
	void DebugDraw();
	#endif

	void OnDoMelee();
	void OnMeleeFinished();

private:

	enum EAction
	{
		eAC_Grab,
		eAC_Throw,
		eAC_Drop
	};
	
	typedef CWeapon inherited;
 
	void SetUpCollisionPropertiesAndMassInfo( bool pickedUp ); 
	void DrawNearObject( bool drawNear );
	void ThrowObject();
	void DropObject();
	bool CheckObjectIsStillThere();	
	IEntity* GetEntityObject();
	CPlayer* GetOwnerPlayer() { assert( GetOwnerActor() && GetOwnerActor()->IsPlayer() );  return static_cast<CPlayer*>(GetOwnerActor()); }

	bool OnActionAttack(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionUse(EntityId actorId, const ActionId& actionId, int activationMode, float value);
	bool OnActionMelee(EntityId actorId, const ActionId& actionId, int activationMode, float value);

	void GetNanosuitHelperMatrix( Matrix34& helperMat );
	void CalcObjectHelperInverseMat( Matrix34& objectHelperInvMat );
	IStatObj::SSubObject* FindHelperObject( const char* pHelperName, EntityId objectId ) const;
	IStatObj::SSubObject* FindHelperObject_RecursivePart( IStatObj* pObj, const char* pHelperName ) const;
	void UnAttachObject();
	void AttachObject();
	IAttachment* GetOwnerAttachment();
	void DecideGrabType();
	const SPickAndThrowParams::SGrabTypeParams& GetGrabTypeParams();
	bool PrepareNPCForBeingGrabbed();
	void PrepareNPCToBeThrown();
	void PrepareNPCToBeDropped();
	void UpdateHeldNPC();
	bool IsNPC() { return m_grabType==ePTGrabType_NPCFront || m_grabType==ePTGrabType_NPCBack; }
	void DoMeleeHit(ray_hit& hitResult, EntityId collidedEntityId);
	void DoMeleeImpulse( ray_hit& hitResult, EntityId collidedEntityId );


	void SendMortalHitTo( CActor* pActor );

		
	enum EState 
	{
		eST_UNKNOWN,
		eST_GOINGTOPICK,				// moving hands towards the object, but it is not grabbed yet
		eST_PICKING,						// object grabbed, hands bringing it to us
		eST_IDLE,
		eST_PUSHINGAWAY,				// hands moving ahead to throw the object, but it is still stick to them
		eST_THROWING,						// object is start moving freely away while the hands finish the throwing animation
		eST_STARTDROPPING,			// hands starts moving to drop the object, but it still stick to them
		eST_DROPPING,						// object starts dropping freely to the floor, while the hands finish the dropping animation
		eST_MELEE,							// use object to melee
		eST_KILLING_GRABBEDNPC,  // killing the NPC that is being grabbed
		eST_KILLED_GRABBEDNPC   // the NPC is killed, but we are in this state until we are ready to drop the corpse
	};
	
	static TActionHandler<CPickAndThrowWeapon> s_actionHandler;
	
	CMeleeCollisionHelper			m_collisionHelper;
	uint											m_constraintId;
	float											m_timePassed; // used for different timmings, depending on state
	float                     m_animDuration;  // used for different animations, depending on state
	float											m_objectMass;	
	EState										m_state;
	QuatT											m_attachmentOldRelativeLoc;
	uint											m_framesToCheckConstraint;  // need to give the physics some time before can really start checking for the constraint
	Vec3											m_objectScale;
	EntityId									m_objectId;  // the object being pickedup
	EntityId									m_currentMeleeAutoTargetId;	
	EPickAndThrowGrabTypes		m_grabType;
	bool											m_objectCollisionsLimited;  // need a separate flag because at some points the object is not attached to the player anymore, but the collisions are still not allowed
	bool											m_objectGrabbed;
	static int								m_objectSlot;
	static const char* const	m_pHelperName;
};

#endif // _PICKANDTHROWWEAPON_H_