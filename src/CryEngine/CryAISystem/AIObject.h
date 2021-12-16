// AIObject.h: interface for the CAIObject class.
//
//////////////////////////////////////////////////////////////////////

#ifndef AIOBJECT_H
#define AIOBJECT_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#include "IAgent.h"
#include "AStarSolver.h"
#include "IPhysics.h"
//#include "Adapters.h"
#include "IAIObject.h"

#ifdef CRYAISYSTEM_DEBUG
	#include "AIRecorder.h"
#endif //CRYAISYSTEM_DEBUG


struct GraphNode;
class CFormation;
class COPWaitSignal;


/*! Basic ai object class. Defines a framework that all puppets and points of interest
later follow.
*/
class CAIObject :
	public IAIObject
#ifdef CRYAISYSTEM_DEBUG
	, public CRecordable
#endif //CRYAISYSTEM_DEBUG
{
public:

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//IAIObject interfaces//////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////
	//Startup/shutdown//////////////////////////////////////////////////////////////////////
	CAIObject();
	virtual ~CAIObject();
	virtual void Reset(EObjectResetType type);
	virtual void Release();

	virtual bool IsUpdatedOnce() const;//??

	virtual bool IsEnabled() const;
	virtual void Event(unsigned short, SAIEVENT* pEvent);
	//Startup/shutdown//////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////////////////////////////////////////////
	//Basic properties//////////////////////////////////////////////////////////////////////
	virtual tAIObjectID GetAIObjectID() const;

	virtual void SetName(const char* pName);
	virtual const char* GetName() const;

	virtual unsigned short GetAIType() const;	
	virtual ESubTypes GetSubType() const;
	virtual void SetType(unsigned short type);

	virtual void SetPos(const Vec3& pos, const Vec3& dirForw = Vec3(1, 0, 0));
	virtual const Vec3& GetPos() const;

	virtual void SetRadius(float fRadius);
	virtual float GetRadius() const;


	virtual const Vec3& GetViewDir() const;
	virtual void SetViewDir(const Vec3& dir);
	virtual EFieldOfViewResult IsPointInFOV(const Vec3& pos,float distanceScale = 1.0f) const;

	virtual const Vec3& GetBodyDir() const;
	virtual void SetBodyDir(const Vec3& dir);

	virtual const Vec3& GetMoveDir() const;
	virtual void SetMoveDir(const Vec3& dir);
	virtual Vec3 GetVelocity() const;
	//Basic properties//////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////////////////////////////////////////////
	//Serialize/////////////////////////////////////////////////////////////////////////////
	//Tell AI what entity owns this so can sort out linking during save/load
	virtual void SetEntityID(unsigned ID);
	virtual unsigned GetEntityID() const;
	virtual IEntity* GetEntity() const;

	//true if needing PostSerialize after all AIObjects created and serialized for pointer fix up
	virtual bool Serialize( TSerialize ser, class CObjectTracker& objectTracker );
	//Called iff Serialize() returns true after all objects serialized
	virtual void PostSerialize( bool bReading ) {}
	//Serialize/////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////////////////////////////////////////////
	//Starting to assume WAY to many conflicting things about possible derived classes//////

	//(AI Objects of type AIANCHOR_COMBAT_HIDESPOT or AIANCHOR_COMBAT_HIDESPOT_SECONDARY):
	// get navigation node the anchor is attached to.
	virtual const GraphNode* GetAnchorNavNode();

	// Perception modifiers
	virtual bool AddPerceptionHandlerModifier(IPerceptionHandlerModifier* pModifier) { return false; }
	virtual bool RemovePerceptionHandlerModifier(IPerceptionHandlerModifier* pModifier) { return false; }
	//Returns position weapon fires from
	virtual const Vec3& GetFirePos( ) const;
	virtual void SetWeaponDescriptor(const AIWeaponDescriptor& descriptor);
	virtual IBlackBoard* GetBlackBoard() { return NULL; }

	virtual void SetGroupId(int id);	
	virtual int GetGroupId() const;	
	virtual void SetSpecies(int id);	
	virtual int GetSpecies() const;	
	virtual bool IsHostile(const IAIObject* pOther, bool bUsingAIIgnorePlayer=true) const;

	//Returns true if the AIObject AIPlayer or Pipe User
	virtual bool IsAgent() const;

	virtual IAIActorProxy* GetProxy() const;
	virtual IAIObject* GetSpecialAIObject(const char* objName, float range = 0.0f);//??wtf??

	//Last finished AIAction sets status as succeed or failed
	virtual void SetLastActionStatus( bool bSucceed );
	//Starting to assume WAY to many conflicting things about possible derived classes//////
	////////////////////////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////////////////////////////////////////////
	//Formations////////////////////////////////////////////////////////////////////////////
	virtual bool CreateFormation(const char* szName, Vec3 vTargetPos=ZERO);
	virtual bool HasFormation();
	virtual bool ReleaseFormation();

	virtual void         CreateGroupFormation        (IAIObject* pLeader) {;}
	virtual void         SetFormationPos             (const Vec2& v2RelPos) {;}
	virtual void         SetFormationLookingPoint    (const Vec3& v3RelPos) {;}
	virtual void         SetFormationAngleThreshold  (float fThresholdDegrees) {;}
	virtual const Vec3&  GetFormationPos             () { return Vec3Constants<float>::fVec3_Zero; }
	virtual const Vec3&  GetFormationLookingPoint    () { return Vec3Constants<float>::fVec3_Zero; }
	//Sets a randomly rotating range for the AIObject's formation sight directions.
	virtual void SetFormationUpdateSight(float range,float minTime,float maxTime);
	//Formations////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////





	//IAIObject interfaces//////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	












	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//IAIRecordable interfaces////////////////////////////////////////////////////////////////////////////////////////
	virtual void RecordEvent(IAIRecordable::e_AIDbgEvent event, const IAIRecordable::RecorderEventData* pEventData=NULL) {};
	virtual void RecordSnapshot() {};
	virtual IAIDebugRecord* GetAIDebugRecord();
	//IAIRecordable interfaces////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////












	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//CAIObject interfaces////////////////////////////////////////////////////////////////////////////////////////////
	
	////////////////////////////////////////////////////////////////////////////////////////
	//Original Virtuals/////////////////////////////////////////////////////////////////////
	virtual IPhysicalEntity* GetPhysics(bool wantCharacterPhysics=false) const;

	virtual const Vec3 &GetFireDir( ) const;
	virtual const Vec3 &GetShootAtPos( ) const {return m_vPosition;} ///< Returns the position to shoot (bbox middle for tank, etc)

	virtual CWeakRef<CAIObject> GetAssociation() const  {return m_refAssociation;}
	virtual void SetAssociation(CWeakRef<CAIObject> refAssociation);
	
	virtual void OnObjectRemoved(CAIObject *pObject ) {}
	//Original Virtuals/////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////////////
	//local level///////////////////////////////////////////////////////////////////////////
	const Vec3& GetLastPosition() {return m_vLastPosition;}
	Vec3 GetPhysicsPos() const;
	
	const char* GetEventName(unsigned short eType) const;

	void SetSubType(ESubTypes type);
	inline unsigned short GetType() const { return m_nObjectType; } // used internally to inline the function calls
	//local level///////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////


	
	////////////////////////////////////////////////////////////////////////////////////////
	//ObjectContainer/WeakRef stuff/////////////////////////////////////////////////////////
	void SetSelfReference(CWeakRef<CAIObject> ref)
	{
		// Should only ever have to call once
		// Should never be invalid
		// Is serialized like any other reference
		assert(m_refThis.IsNil());
		m_refThis = ref;
	}

	CWeakRef<CAIObject> GetSelfReference() const
	{
		assert(!m_refThis.IsNil());
		return m_refThis;
	}

	bool HasSelfReference() const
	{
		return m_refThis.IsSet();
	}
	//ObjectContainer/WeakRef stuff/////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////

	//CAIObject interfaces////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	






protected:

	bool				m_bEnabled;
	unsigned short		m_nObjectType;		//	type would be Dummy
	ESubTypes			m_objectSubType;	//	subType would be soundDummy, formationPoint, etc	
	int					m_groupId;
	int					m_species;
	float				m_fRadius;

	EntityId		m_entityID;

private:
	CWeakRef<CAIObject> m_refThis;

	Vec3				m_vPosition;
	Vec3				m_vBodyDir;		// body forward direction of the entity
	Vec3				m_vMoveDir;		// last move direction of the entity
	Vec3				m_vView;		// view direction (where my head is looking at, tank turret turned, etc)

public:

	bool				m_bUpdatedOnce;
	mutable bool		m_bTouched;			// this gets frequent write access. 
	unsigned int m_lastNavNodeIndex;

	// please add a new variable below, the section above is optimized for memory caches.

	bool				m_bUncheckedBody;
	bool				m_bCanReceiveSignals;
	bool				m_bLastNearForbiddenEdge;
	bool				m_bLastActionSucceed;

	Vec3				m_vLastPosition;
	CFormation			*m_pFormation;

private: 

	Vec3				m_vFirePosition;
	Vec3				m_vFireDir;

	string			m_name;

protected:
	CWeakRef<CAIObject>  m_refAssociation;
};

#endif 
