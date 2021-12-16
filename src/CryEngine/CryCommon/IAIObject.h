#ifndef IAIObject_h
#define IAIObject_h

#include "IAgent.h"

#ifndef INVALID_AIOBJECTID
typedef uint32	tAIObjectID;	//TODO make a handle or something instead, description needs to be accessible from many places
#define INVALID_AIOBJECTID ((tAIObjectID)(0))
#endif



struct IAIObject:
	public IAIRecordable
{
	enum ESubTypes{
		STP_NONE,
		STP_FORMATION,
		STP_REFPOINT,
		STP_LOOKAT,
		STP_CAR,
		STP_BOAT,
		STP_HELI,
		STP_2D_FLY,
		STP_SOUND,
		STP_MEMORY,
		STP_BEACON,
		STP_SPECIAL,
		STP_ANIM_TARGET,
		STP_MAXVALUE
	};

	enum EFieldOfViewResult
	{
		eFOV_Outside = 0,
		eFOV_Primary,
		eFOV_Secondary,
	};

protected:
	union
	{
		struct 
		{
			uint32 _fastcast_CAIActor : 1;
			uint32 _fastcast_CAIAttribute : 1;	// (MATT) Unused {2009/03/31}
			uint32 _fastcast_CAIPlayer : 1;
			uint32 _fastcast_CLeader : 1;
			uint32 _fastcast_CPipeUser : 1;
			uint32 _fastcast_CPuppet : 1;
			uint32 _fastcast_CAIVehicle : 1;
		};
		int _fastcast_any;
	};
public:

	////////////////////////////////////////////////////////////////////////////////////////
	//Startup/shutdown//////////////////////////////////////////////////////////////////////
	IAIObject() : _fastcast_any(0) {}
	virtual ~IAIObject() {}
	virtual void Reset(EObjectResetType type) = 0;
	virtual void Release()=0;

	virtual bool IsUpdatedOnce() const = 0;//??

	virtual bool IsEnabled() const = 0;
	virtual void Event(unsigned short, SAIEVENT* pEvent) = 0;
	//Startup/shutdown//////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////

	
	
	////////////////////////////////////////////////////////////////////////////////////////
	//Basic properties//////////////////////////////////////////////////////////////////////
	virtual tAIObjectID GetAIObjectID() const = 0;

	virtual void SetName(const char* pName)= 0;
	virtual const char* GetName() const= 0;

	virtual unsigned short GetAIType() const = 0;	
	virtual ESubTypes GetSubType() const = 0;
	virtual void SetType(unsigned short type) = 0;

	virtual void SetPos(const Vec3& pos, const Vec3& dirForw = Vec3(1, 0, 0)) = 0;
	virtual const Vec3& GetPos() const = 0;

	virtual void SetRadius(float fRadius) = 0;
	virtual float GetRadius() const = 0;


	virtual const Vec3& GetViewDir() const = 0;
	virtual void SetViewDir(const Vec3& dir) = 0;
	virtual EFieldOfViewResult IsPointInFOV(const Vec3& pos, float distanceScale = 1.0f) const = 0;

	virtual const Vec3& GetBodyDir() const = 0;
	virtual void SetBodyDir(const Vec3& dir) = 0;

	virtual const Vec3& GetMoveDir() const = 0;
	virtual void SetMoveDir(const Vec3& dir) = 0;
	virtual Vec3 GetVelocity() const = 0;
	//Basic properties//////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////
	


	////////////////////////////////////////////////////////////////////////////////////////
	//Serialize/////////////////////////////////////////////////////////////////////////////
	//Tell AI what entity owns this so can sort out linking during save/load
	virtual void SetEntityID(unsigned ID) = 0;
	virtual unsigned GetEntityID() const = 0;
	virtual IEntity* GetEntity() const = 0;

	//true if needing PostSerialize after all AIObjects created and serialized for pointer fix up
	virtual bool Serialize( TSerialize ser, class CObjectTracker& objectTracker ) = 0;
	//Called iff Serialize() returns true after all objects serialized
	virtual void PostSerialize( bool bReading ) {}
	//Serialize/////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////
	







	////////////////////////////////////////////////////////////////////////////////////////
	//Starting to assume WAY to many conflicting things about possible derived classes//////

	//(AI Objects of type AIANCHOR_COMBAT_HIDESPOT or AIANCHOR_COMBAT_HIDESPOT_SECONDARY):
	// get navigation node the anchor is attached to.
	virtual const GraphNode* GetAnchorNavNode() = 0;

	// Perception modifiers
	virtual bool AddPerceptionHandlerModifier(IPerceptionHandlerModifier* pModifier) { return false; }
	virtual bool RemovePerceptionHandlerModifier(IPerceptionHandlerModifier* pModifier) { return false; }
	//Returns position weapon fires from
	virtual const Vec3& GetFirePos( ) const = 0;
	virtual void SetWeaponDescriptor(const AIWeaponDescriptor& descriptor) = 0;
	virtual IBlackBoard* GetBlackBoard() { return NULL; }

	virtual void SetGroupId(int id) = 0;	
	virtual int GetGroupId() const = 0;	
	virtual void SetSpecies(int id) = 0;	
	virtual int GetSpecies() const = 0;	
	virtual bool IsHostile(const IAIObject* pOther, bool bUsingAIIgnorePlayer=true) const = 0;

	//Returns true if the AIObject AIPlayer or Pipe User
	virtual bool IsAgent() const { return false; }

	virtual IAIActorProxy* GetProxy() const = 0;
	virtual IAIObject* GetSpecialAIObject(const char* objName, float range = 0.0f) =0;//??wtf??

	//Last finished AIAction sets status as succeed or failed
	virtual void SetLastActionStatus( bool bSucceed ) = 0;
	
	//Starting to assume WAY to many conflicting things about possible derived classes//////
	////////////////////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////////////////////////////////////////////
	//Formations////////////////////////////////////////////////////////////////////////////

	virtual bool CreateFormation(const char* szName, Vec3 vTargetPos=ZERO)=0;
	virtual bool HasFormation() = 0;
	virtual bool ReleaseFormation()=0;

	virtual void         CreateGroupFormation        (IAIObject* pLeader) {;}
	virtual void         SetFormationPos             (const Vec2& v2RelPos) {;}
	virtual void         SetFormationLookingPoint    (const Vec3& v3RelPos) {;}
	virtual void         SetFormationAngleThreshold  (float fThresholdDegrees) {;}
	virtual const Vec3&  GetFormationPos             () { return Vec3Constants<float>::fVec3_Zero; }
	virtual const Vec3&  GetFormationLookingPoint    () { return Vec3Constants<float>::fVec3_Zero; }
	//Sets a randomly rotating range for the AIObject's formation sight directions.
	virtual void SetFormationUpdateSight(float range,float minTime,float maxTime) = 0;

	//Formations////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////







	////////////////////////////////////////////////////////////////////////////////////////
	//Fast Casting//////////////////////////////////////////////////////////////////////////

	virtual const IAIActor* CastToIAIActor() const { return NULL; }
	virtual IAIActor* CastToIAIActor() { return NULL; }
	const CAIActor* CastToCAIActor() const { return _fastcast_CAIActor ? (const CAIActor*) this : NULL; }
	CAIActor* CastToCAIActor() { return _fastcast_CAIActor ? (CAIActor*) this : NULL; }

	const CAIPlayer* CastToCAIPlayer() const { return _fastcast_CAIPlayer ? (const CAIPlayer*) this : NULL; }
	CAIPlayer* CastToCAIPlayer() { return _fastcast_CAIPlayer ? (CAIPlayer*) this : NULL; }

	const CLeader* CastToCLeader() const { return _fastcast_CLeader ? (const CLeader*) this : NULL; }
	CLeader* CastToCLeader() { return _fastcast_CLeader ? (CLeader*) this : NULL; }

	virtual const IPipeUser* CastToIPipeUser() const { return NULL; }
	virtual IPipeUser* CastToIPipeUser() { return NULL; }
	const CPipeUser* CastToCPipeUser() const { return _fastcast_CPipeUser ? (const CPipeUser*) this : NULL; }
	CPipeUser* CastToCPipeUser() { return _fastcast_CPipeUser ? (CPipeUser*) this : NULL; }

	virtual const IPuppet* CastToIPuppet() const { return NULL; }
	virtual IPuppet* CastToIPuppet() { return NULL; }
	const CPuppet* CastToCPuppet() const { return _fastcast_CPuppet ? (const CPuppet*) this : NULL; }
	CPuppet* CastToCPuppet() { return _fastcast_CPuppet ? (CPuppet*) this : NULL; }

	virtual const IAIVehicle* CastToIAIVehicle() const { return NULL; }
	virtual IAIVehicle* CastToIAIVehicle() { return NULL; }
	const CAIVehicle* CastToCAIVehicle() const { return _fastcast_CAIVehicle ? (const CAIVehicle*) this : NULL; }
	CAIVehicle* CastToCAIVehicle() { return _fastcast_CAIVehicle ? (CAIVehicle*) this : NULL; }

	//Fast Casting//////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////
};









////////////////////////////////////////////////////////////////////////////////////////
//Needed inside and outside of class?///////////////////////////////////////////////////

inline const IAIActor* CastToIAIActorSafe(const IAIObject* pAI) { return pAI ? pAI->CastToIAIActor() : 0; }
inline IAIActor* CastToIAIActorSafe(IAIObject* pAI) { return pAI ? pAI->CastToIAIActor() : 0; }

inline const IPuppet* CastToIPuppetSafe(const IAIObject* pAI) { return pAI ? pAI->CastToIPuppet() : 0; }
inline IPuppet* CastToIPuppetSafe(IAIObject* pAI) { return pAI ? pAI->CastToIPuppet() : 0; }

inline const IPipeUser* CastToIPipeUserSafe(const IAIObject* pAI) { return pAI ? pAI->CastToIPipeUser() : 0; }
inline IPipeUser* CastToIPipeUserSafe(IAIObject* pAI) { return pAI ? pAI->CastToIPipeUser() : 0; }

//Needed inside and outside of class?///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////



#endif