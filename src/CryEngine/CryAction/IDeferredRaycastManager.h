/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Manager for deferred raycast wrappers.
The managers creates an interface between CryAction and other DLLs using the wrappers.
Use CDeferredRaycastHelper to create and access the wrapper ...

-------------------------------------------------------------------------
History:
- 02:2009 : Created By Jan Müller

*************************************************************************/
#include DEVIRTUALIZE_HEADER_FIX(IDeferredRaycastManager.h)

#ifndef _IDeferredRaycastManager
#define _IDeferredRaycastManager

#include "IGameFramework.h"

class CDeferredRaycastWrapper;
struct EventPhysRWIResult;
struct EventPhysPWIResult;
struct intersection_params;
struct ray_hit;
namespace primitives { struct primitive; }

//data received from a primitive cast
struct SPrimitiveCastHit
{
	float			dist;
	Vec3			hitPos;
	int				materialIndex;
	IPhysicalEntity*	pPhysEntity;

	SPrimitiveCastHit() : dist(-1.0f), hitPos(ZERO), materialIndex(0), pPhysEntity(NULL)
	{}
};

//it's optional to (not) use this, when there is only one result for the wrapper
//data receiver for raycasts
struct IDeferredRaycastReceiver
{
	//async raycasts results callback
	virtual void OnDataReceived(const EventPhysRWIResult *pRWIResult) = 0;
	//async primitive casts results callback
	virtual void OnDataReceived(const EventPhysPWIResult *pPWIResult) = 0;
	//reset data callback
	virtual void OnDRWReset() = 0;
};

//wrapper interface - use the CDeferredRaycastHelper to access this 
UNIQUE_IFACE struct IDeferredRaycastWrapper
{
	//targetData is not written/used, but returned as pForeignData in OnDataReceived
	//the purpose of targetData is for the receiver to know where to save the result
	//example for settings : "ent_all|ent_water, geom_colltype0|geom_colltype_player|rwi_stop_at_pierceable"
	//run a raycast -> result will arrive deferred
	virtual void CastRay(const Vec3 &rayPos, const Vec3 &rayDir, int objTypes, int flags, IPhysicalEntity **pSkipEnts = NULL, int nSkipEnts = 0, void *targetData = NULL) = 0;

	//targetData is not written automatically (returned as pForeignData in OnDataReceived)
	//the purpose of targetData is for the receiver to know where to save the result
	//example for settings : "ent_all & ~(ent_living | ent_independent), geom_colltype0"
	//run a primitive cast -> result will arrive deferred
	virtual void CastPrimitive(int itype, primitives::primitive *pprim, const Vec3 &sweepDir, int objTypes, int geomFlags, intersection_params *pip = NULL, IPhysicalEntity **pSkipEnts = NULL, int nSkipEnts = 0, void *targetData = NULL) = 0;

	//get the first hit from the last successful raycast if no target receiver was specified
	virtual const ray_hit* GetRayHit() const = 0;

	//get the hit data from the last successful primitive cast if no target receiver was specified
	virtual const SPrimitiveCastHit* GetPrimHit() const = 0;

	//set a target receiver (if not done on construction)
	//if you use CDeferredRaycastHelper, don't call this directly!
	virtual void SetReceiver(IDeferredRaycastReceiver *pReceiver) = 0;

	//get the the wrapper's unique ID
	virtual int	GetId() const = 0;

	//call this to reset the wrapper and it's receivers
	virtual void Reset() = 0;
};

//manager interface
UNIQUE_IFACE struct IDeferredRaycastManager
{
	//Find an existing wrapper
	virtual IDeferredRaycastWrapper *FindWrapper(const int iID) = 0;

	//Get/Create a raycast wrapper
	virtual IDeferredRaycastWrapper *GetWrapper(const int iID = -1) = 0;

	//remove an existing wrapper
	virtual bool RemoveWrapper(const int iID) = 0;

	//reset all wrappers
	virtual void Reset() = 0;
};

//helper dealing with deferred raycast manager
//use this to access the wrappers from other DLLs
class CDeferredRaycastHelper
{
public:
	CDeferredRaycastHelper() : m_pImpl(NULL), m_pReceiver(NULL)
	{
	}

	CDeferredRaycastHelper(const CDeferredRaycastHelper &other)
	{
		m_pImpl = NULL;
		m_pReceiver = NULL;
	}

	CDeferredRaycastHelper &operator=(const CDeferredRaycastHelper &other)
	{
		if (this != &other)
		{
			CancelPendingRays();
			m_pImpl = NULL;
			m_pReceiver = NULL;
		}
		return *this;
	}

	~CDeferredRaycastHelper()
	{
		CancelPendingRays();
	}

	void CancelPendingRays()
	{
		CRY_ASSERT(gEnv->pGame);
		IGameFramework *pGF = gEnv->pGame->GetIGameFramework();
		if(m_pImpl && pGF)
			pGF->GetIDeferredRaycastManager()->RemoveWrapper(m_pImpl->GetId());
		m_pImpl = NULL;
	}

	void SetReceiver(IDeferredRaycastReceiver *pReceiver)
	{ 
		m_pReceiver = pReceiver;
		if(m_pImpl)
			m_pImpl->SetReceiver(pReceiver);
	}

	IDeferredRaycastWrapper* operator->(void) { return GetImpl(); }

private:

	IDeferredRaycastWrapper *GetImpl()
	{
		if(!m_pImpl && gEnv->pGame)
		{
			IGameFramework *pGF = gEnv->pGame->GetIGameFramework();
			if(pGF)
			{
				m_pImpl = pGF->GetIDeferredRaycastManager()->GetWrapper();
				m_pImpl->SetReceiver(m_pReceiver);
			}
		}

		return m_pImpl;
	}

	IDeferredRaycastWrapper *m_pImpl;
	IDeferredRaycastReceiver *m_pReceiver;
};

#endif