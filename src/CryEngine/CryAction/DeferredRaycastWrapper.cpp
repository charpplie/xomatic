/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Wrapper for async/deferred Game DLL raycasting

-------------------------------------------------------------------------
History:
- 02:09:2008 : Created By Jan Müller

*************************************************************************/

#include "StdAfx.h"
#include "DeferredRaycastWrapper.h"

#include <StlUtils.h>

//this increments when a new id is registered with physics
// const static int CDeferredRaycastWrapper::DEFERRED_RAYCAST_FOREIGN_PHYS_FIRST_ID = PHYS_FOREIGN_ID_USER + 999999;

int CDeferredRaycastWrapper::DEFERRED_RAYCAST_FOREIGN_PHYS_ID = CDeferredRaycastWrapper::DEFERRED_RAYCAST_FOREIGN_PHYS_FIRST_ID;

//make sure RWI and PWI callbacks are only registered once
//static bool g_bRegisteredPhysClient = false;

//this list contains all wrappers to look up the sender for received data
//after the migrated to CryAction, this functionality moved to the manager
//static std::vector<CDeferredRaycastWrapper*>	g_wrapperMap;

CDeferredRaycastWrapper::CDeferredRaycastWrapper(IDeferredRaycastReceiver *pReceiver) :
m_pReceiver(pReceiver),
m_iPhysForeignId(DEFERRED_RAYCAST_FOREIGN_PHYS_ID)
{
	//increment phys register id
	++DEFERRED_RAYCAST_FOREIGN_PHYS_ID;
	RegisterResultCallbacks();

	//add myself to wrapperMap
	//g_wrapperMap.push_back(this);

	//clear/init hits
	ClearRayHit(); 
	ClearPrimitiveHit();
}

CDeferredRaycastWrapper::~CDeferredRaycastWrapper()
{
	//list moved to manager, unregistering not necessary in CryAction
	//disable wrapper in list - don't remove the iterator/item, since we access by id
	//g_wrapperMap[m_iPhysForeignId - DEFERRED_RAYCAST_FOREIGN_PHYS_FIRST_ID] = NULL;

	//should we unregister ?
	/*if(g_bRegisteredPhysClient)
	{
	//check for any active wrapper
	std::vector<CDeferredRaycastWrapper*>::iterator it = g_wrapperMap.begin();
	std::vector<CDeferredRaycastWrapper*>::iterator end = g_wrapperMap.end();
	for(; it != end; ++it)
	{
	if(*it != NULL)	//wrapper still available
	return;
	}

	//if there is no active wrapper, remove listener
	gEnv->pPhysicalWorld->RemoveEventClient(EventPhysRWIResult::id, OnRwiResult, 1);
	gEnv->pPhysicalWorld->RemoveEventClient(EventPhysPWIResult::id, OnPwiResult, 1);
	g_bRegisteredPhysClient = false;
	}*/
}

void CDeferredRaycastWrapper::RegisterResultCallbacks()
{
	//register only once
	/*if(!g_bRegisteredPhysClient)
	{
	gEnv->pPhysicalWorld->AddEventClient(EventPhysRWIResult::id, OnRwiResult, 1);
	gEnv->pPhysicalWorld->AddEventClient(EventPhysPWIResult::id, OnPwiResult, 1);
	g_bRegisteredPhysClient = true;
	}*/
}

void CDeferredRaycastWrapper::CastRay(const Vec3 &rayPos, const Vec3 &rayDir, int objTypes, int flags, IPhysicalEntity **pSkipEnts, int nSkipEnts, void *targetData)
{
	//shoot ray with wrapper id and "queue" flag
	gEnv->pPhysicalWorld->RayWorldIntersection(rayPos, rayDir,
		objTypes, flags | rwi_queue, 0, 1, pSkipEnts, nSkipEnts, targetData, m_iPhysForeignId);
}

void CDeferredRaycastWrapper::CastPrimitive(int itype, primitives::primitive *pprim, const Vec3 &sweepDir, int objTypes, int geomFlags, intersection_params *pip, IPhysicalEntity **pSkipEnts, int nSkipEnts, void *targetData)
{
	//cast primitive with wrapper id and "queue" flag
	gEnv->pPhysicalWorld->PrimitiveWorldIntersection(itype, pprim, sweepDir,
		objTypes | rwi_queue, 0, 0, geomFlags, pip, targetData, m_iPhysForeignId, pSkipEnts, nSkipEnts);
}

CDeferredRaycastWrapper *CDeferredRaycastWrapper::FindWrapper(const int id)
{
	CDeferredRaycastWrapper *pWrapper = NULL;
	//access the wrapper directly by it's id (minus firstIndex)
	//if(id >= DEFERRED_RAYCAST_FOREIGN_PHYS_FIRST_ID && id < DEFERRED_RAYCAST_FOREIGN_PHYS_ID)
	//wrapper = g_wrapperMap[id - DEFERRED_RAYCAST_FOREIGN_PHYS_FIRST_ID];
	pWrapper = static_cast<CDeferredRaycastWrapper*>(gEnv->pGame->GetIGameFramework()->GetIDeferredRaycastManager()->FindWrapper(id));
	return pWrapper;
}

int CDeferredRaycastWrapper::OnRwiResult(const EventPhys *pEvent)
{
	EventPhysRWIResult *pRWIResult = (EventPhysRWIResult*)pEvent;
	//try to find target wrapper
	if(pRWIResult->iForeignData >= DEFERRED_RAYCAST_FOREIGN_PHYS_FIRST_ID)
	{
		CDeferredRaycastWrapper *sender = FindWrapper(pRWIResult->iForeignData);
		CRY_ASSERT_TRACE(sender, ("Couldn't find deferred raycast sender for event ID %i", pRWIResult->iForeignData));

		if(sender)
		{
			//found target wrapper -> dispatch result
			IDeferredRaycastReceiver *pReceiver = sender->GetReceiver();
			if(pReceiver)
				pReceiver->OnDataReceived(pRWIResult);
			else
			{
				//set ray hit for this result
				if(pRWIResult->nHits > 0 && pRWIResult->pHits)
					sender->SetRayHit(pRWIResult->pHits[0]);
				else
					sender->ClearRayHit();
			}

			return 1;
		}
	}

	return 0;
}

int CDeferredRaycastWrapper::OnPwiResult(const EventPhys *pEvent)
{
	EventPhysPWIResult *pPWIResult = (EventPhysPWIResult*)pEvent;
	//try to find target wrapper
	if(pPWIResult->iForeignData >= DEFERRED_RAYCAST_FOREIGN_PHYS_FIRST_ID)
	{
		CDeferredRaycastWrapper *sender = FindWrapper(pPWIResult->iForeignData);
		CRY_ASSERT_TRACE(sender, ("Couldn't find deferred raycast sender for event ID %i", pPWIResult->iForeignData));

		if(sender)
		{
			//found target wrapper -> dispatch result
			IDeferredRaycastReceiver *pReceiver = sender->GetReceiver();
			if(pReceiver)
				pReceiver->OnDataReceived(pPWIResult);
			else
			{
				//set primitive hit for this result
				if(pPWIResult->dist > 0.0f)
					sender->SetPrimitiveHit(pPWIResult->dist, pPWIResult->pt, pPWIResult->pEntity, pPWIResult->idxMat);
				else
					sender->ClearPrimitiveHit();
			}

			return 1;
		}
	}

	return 0;
}

#include UNIQUE_VIRTUAL_WRAPPER(IDeferredRaycastWrapper)