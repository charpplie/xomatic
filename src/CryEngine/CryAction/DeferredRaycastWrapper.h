/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: wrapper for async/deferred raycasting

-------------------------------------------------------------------------
History:
- 02:09:2008 : Created By Jan Müller

*************************************************************************/

#ifndef DEFERRED_RAYCAST_WRAPPER
#define DEFERRED_RAYCAST_WRAPPER

#include "IDeferredRaycastManager.h"

//wrapper for deferred raycasting
class CDeferredRaycastWrapper : public IDeferredRaycastWrapper
{
	//only the manager should create/delete wrappers
	friend class CDeferredRaycastManager;

public:
	
	const static int DEFERRED_RAYCAST_FOREIGN_PHYS_FIRST_ID = PHYS_FOREIGN_ID_USER + 999999;
	static int DEFERRED_RAYCAST_FOREIGN_PHYS_ID;

	//targetData is not written/used, but returned as pForeignData in OnDataReceived
	//the purpose of targetData is for the receiver to know where to save the result
	//example for settings : "ent_all|ent_water, geom_colltype0|geom_colltype_player|rwi_stop_at_pierceable"
	//run a raycast -> result will arrive deferred
	void CastRay(const Vec3 &rayPos, const Vec3 &rayDir, int objTypes, int flags, IPhysicalEntity **pSkipEnts = NULL, int nSkipEnts = 0, void *targetData = NULL);

	//targetData is not written automatically (returned as pForeignData in OnDataReceived)
	//the purpose of targetData is for the receiver to know where to save the result
	//example for settings : "ent_all & ~(ent_living | ent_independent), geom_colltype0"
	//run a primitive cast -> result will arrive deferred
	void CastPrimitive(int itype, primitives::primitive *pprim, const Vec3 &sweepDir, int objTypes, int geomFlags, intersection_params *pip = NULL, IPhysicalEntity **pSkipEnts = NULL, int nSkipEnts = 0, void *targetData = NULL);

	//get the first hit from the last successful raycast if no target receiver was specified
	const ray_hit* GetRayHit() const { return &m_lastRayHit; }

	//get the entity id from the last successful raycast (== GetRayHit)
	EntityId GetRayHitEntityId() const { return m_lastRayHitEntityId; }

	//get the hit data from the last successful primitive cast if no target receiver was specified
	const SPrimitiveCastHit* GetPrimHit() const { return &m_lastPrimHit; }

	//set a target receiver (if not done on construction)
	void SetReceiver(IDeferredRaycastReceiver *pReceiver) { m_pReceiver = pReceiver; }

	//get the the wrapper's unique ID
	VIRTUAL int	GetId() const { return m_iPhysForeignId; }

	//call this to reset the wrapper and it's receivers
	void Reset();

protected:
	//wrapper takes optional receiver target
	CDeferredRaycastWrapper(IDeferredRaycastReceiver *pReceiver = NULL);
	~CDeferredRaycastWrapper();

	//get wrapper phys foreign id
	int GetPhysId() const { return m_iPhysForeignId; }

	//set the last hit from a raycast
	void SetRayHit(const ray_hit &hit);

	//set the last hit from a primitive cast
	void SetPrimitiveHit(const float dist, const Vec3 &hitPos, const IPhysicalEntity *pEntity, const int matIndex);

	//clear last ray hit
	void ClearRayHit();

	//clear last primitive hit
	void ClearPrimitiveHit();

	//get receiver for this wrapper
	IDeferredRaycastReceiver *GetReceiver() const { return m_pReceiver; }

private:

	//retrieve wrapper from id
	static CDeferredRaycastWrapper* FindWrapper( const int id);

	//register rwi/pwi callbacks
	static void RegisterResultCallbacks();

	//async raycasts results callback - dispatch to receiver
	static int OnRwiResult(const EventPhys *pEvent);

	//async primitive casts results callback - dispatch to receiver
	static int OnPwiResult(const EventPhys *pEvent);

	//the result is sent to this receiver if specified
	IDeferredRaycastReceiver	*m_pReceiver;

	//this is the ray_hit from the last successful raycast result
	ray_hit										m_lastRayHit;
	EntityId									m_lastRayHitEntityId;

	//this is data receiver from the last successful primitive cast result
	SPrimitiveCastHit					m_lastPrimHit;

	//this is the unique id used to identify incoming ray-data as ours
	int			m_iPhysForeignId;
};

//INLINES*****************************************

inline void CDeferredRaycastWrapper::ClearRayHit()
{
	m_lastRayHit.dist = -1.0f;
	m_lastRayHit.pCollider = NULL;
}

inline void CDeferredRaycastWrapper::ClearPrimitiveHit()
{
	m_lastPrimHit.dist = -1.0f;
	m_lastPrimHit.pPhysEntity = NULL;
}

inline void CDeferredRaycastWrapper::SetPrimitiveHit(const float dist, const Vec3 &hitPos, const IPhysicalEntity *pEntity, const int matIndex)
{
	m_lastPrimHit.dist = dist;
	m_lastPrimHit.hitPos = hitPos;
	m_lastPrimHit.materialIndex = matIndex;
	m_lastPrimHit.pPhysEntity = const_cast<IPhysicalEntity*>(pEntity);
}

inline void CDeferredRaycastWrapper::SetRayHit(const ray_hit &hit)
{ 
	m_lastRayHit = hit;
	m_lastRayHitEntityId = 0;
	if(hit.pCollider)
	{
		IEntity *pEnt = gEnv->pEntitySystem->GetEntityFromPhysics(hit.pCollider);
		if(pEnt)
			m_lastRayHitEntityId = pEnt->GetId();
	}
}

inline void CDeferredRaycastWrapper::Reset()
{
	ClearPrimitiveHit();
	ClearRayHit();
	if(m_pReceiver)
		m_pReceiver->OnDRWReset();
}

#endif
