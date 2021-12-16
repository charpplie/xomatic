

#include "StdAfx.h"

#include "CryMacros.h"
#include "PlayerVisTable.h"

#include "Game.h"
#include "GameCVars.h"
#include "Player.h"

CPlayerVisTable::CPlayerVisTable()
{
	m_numUsedVisTableEntries	= 0;
	m_numLinetestsThisFrame		= 0;
	m_currentBufferTarget = 0;
	m_currentBufferProcessing = 1;

#if ALLOW_VISTABLE_DEBUGGING
	m_numQueriesThisFrame = 0;
#endif
}

CPlayerVisTable::~CPlayerVisTable()
{
}

bool CPlayerVisTable::CanLocalPlayerSee(EntityId entityId, uint8 acceptableFrameLatency, const float heightOffset)
{
	assert(acceptableFrameLatency < 128); //If this is too high there is the potential for the framesSinceLastCheck to overflow

	bool localPlayerCanSee = false;

	CryPrefetch(m_visTableEntries);

	VisEntryIndex visIndex = GetEntityIndexFromID(entityId);

#if ALLOW_VISTABLE_DEBUGGING
	m_numQueriesThisFrame++;
#endif

	Vec3 localPlayerPosn;

	GetLocalPlayerPosn(localPlayerPosn);

	if(visIndex != -1)
	{
		SVisTableEntry& visInfo = m_visTableEntries[visIndex];

		visInfo.lastRequestedLatency = ((visInfo.lastRequestedLatency + acceptableFrameLatency + 1) / 2);
		visInfo.flags |= eVF_Requested;
		visInfo.heightOffset = heightOffset;

		localPlayerCanSee = (visInfo.flags & eVF_Visible);
	}
	else
	{
		SVisTableEntry& visInfo = m_visTableEntries[m_numUsedVisTableEntries];

		visInfo.flags = eVF_Requested;
		visInfo.entityId = entityId;
		visInfo.lastRequestedLatency = acceptableFrameLatency;
		visInfo.framesSinceLastCheck = acceptableFrameLatency;
		visInfo.heightOffset = heightOffset;

		localPlayerCanSee = false;

		m_numUsedVisTableEntries++;

		assert(m_numUsedVisTableEntries <= kMaxVisTableEntries);
	}

	return localPlayerCanSee;
}

bool CPlayerVisTable::CanLocalPlayerSee(EntityId entityId)
{
	CRY_TODO(30,09,2009, "Change default number of frames according to vis table entry type");
	return CanLocalPlayerSee(entityId, kDefaultAcceptableLatency, kVisTableDefaultZAxisOffset);
}

void CPlayerVisTable::RemovePendingDeferredLinetest(int index)
{
	for(int i = 0; i < kNumVisTableBuffers; i++)
	{
		SDeferredLinetestReceiver * pendingEntry = GetDeferredLinetestReceiverFromVisTableIndex(index, i);

		if(pendingEntry)
		{
			pendingEntry->SetInvalid();
		}		
	}	
}

void CPlayerVisTable::UpdatePendingDeferredLinetest(int source, int dest)
{
	for(int i = 0; i < kNumVisTableBuffers; i++)
	{
		SDeferredLinetestReceiver * pendingEntry = GetDeferredLinetestReceiverFromVisTableIndex(source, i);

		if(pendingEntry)
		{
			pendingEntry->visTableIndex = dest;
		}		
	}
}

SDeferredLinetestReceiver * CPlayerVisTable::GetDeferredLinetestReceiverFromVisTableIndex(VisEntryIndex index, int bufferIndex)
{
	SDeferredLinetestBuffer& visBuffer = m_linetestBuffers[bufferIndex];

	SDeferredLinetestReceiver * visTableProcessingEntries = visBuffer.m_deferredLinetestReceivers;

	VisEntryIndex processingIndex = -1;

	for(int i = 0; i < kMaxVisTableLinetestsPerFrame; i++)
	{
		if(visTableProcessingEntries[i].visTableIndex == index)
		{
			return &visBuffer.m_deferredLinetestReceivers[i];
		}
	}

	return NULL;
}

void CPlayerVisTable::RemoveNthEntity(int n)
{
	//NOTE: This does not have to be synched as the access will be on this thread.

	const int swapIndex = m_numUsedVisTableEntries - 1;

	if(m_visTableEntries[n].flags & eVF_Pending)
	{
		RemovePendingDeferredLinetest(n);
	}

	if(m_visTableEntries[swapIndex].flags & eVF_Pending && n != swapIndex)
	{
		UpdatePendingDeferredLinetest(swapIndex, n);
	}

	m_visTableEntries[n] = m_visTableEntries[swapIndex];
	m_visTableEntries[swapIndex].Reset();
	m_numUsedVisTableEntries--;
}

void CPlayerVisTable::Update(float dt)
{
	const int numEntries = m_numUsedVisTableEntries;

	CryPrefetch(m_visTableEntries);

	//Flip the buffers
	m_currentBufferProcessing = 1 - m_currentBufferProcessing;
	m_currentBufferTarget			= 1 - m_currentBufferTarget;

	Vec3 localPlayerPosn;

	GetLocalPlayerPosn(localPlayerPosn);

	//Iterate over all of the vis table entries and calculate the priorities

	int numAdded = AddVisTableEntriesToPriorityList();

	//Iterate over all of the vis table entries
	for(int i = 0; i < numAdded; i++)
	{
		DoVisibilityCheck(localPlayerPosn, *m_visTablePriorities[i].visInfo, m_visTablePriorities[i].visIndex);
	}

	const int kNumVistableEntries = m_numUsedVisTableEntries;
	for(int i = 0; i < kNumVistableEntries; i++)
	{
		m_visTableEntries[i].framesSinceLastCheck++;			
	}

#if ALLOW_VISTABLE_DEBUGGING
	if(g_pGameCVars->pl_debug_vistable)
	{
		float white[] = {1.0f,1.0f,1.0f,1.0f};

		const int kNumUsedVistableEntries = m_numUsedVisTableEntries;
		VisEntryIndex worstIndex = -1;

		int worstLatency = 0;
		for(int i = 0; i < kNumUsedVistableEntries; i++)
		{
			if((m_visTableEntries[i].flags & eVF_Requested) && !(m_visTableEntries[i].flags & eVF_Remove) && (m_visTableEntries[i].framesSinceLastCheck > worstLatency))
			{
				worstLatency	= m_visTableEntries[i].framesSinceLastCheck;
				worstIndex		= i;
			}
		}

		int assertAfterThisManyFrames = g_pGameCVars->g_assertWhenVisTableNotUpdatedForNumFrames;
		if (worstLatency >= assertAfterThisManyFrames)
		{
			IEntity * entity = gEnv->pEntitySystem->GetEntity(m_visTableEntries[worstIndex].entityId);
			CRY_ASSERT_MESSAGE(false, string().Format("%u frames have passed since last check of vis-table element %d (entity %d = %s \"%s\") flags=%u", m_visTableEntries[worstIndex].framesSinceLastCheck, worstIndex, m_visTableEntries[worstIndex].entityId, entity ? entity->GetClass()->GetName() : "NULL", entity ? entity->GetName() : "NULL", m_visTableEntries[worstIndex].flags));
		}


		gEnv->pRenderer->Draw2dLabel(20.f, 500.f, 1.5f, white, false, "VistableInfo:\n  Num Linetests this frame: %d\n  Num queries this frame: %d\n  Worst latency: %d", m_numLinetestsThisFrame, m_numQueriesThisFrame, worstLatency);
	}

	m_numQueriesThisFrame = 0;
#endif

	m_numLinetestsThisFrame = 0;

	ClearRemovedEntities();
}


//---------------------------------------------
// CPlayerVisTable::AddVisTableEntriesToPriorityList()
//		Scans the current list of entities in the vis table and looks
//		for the highest priority entities to test to. Any entities
//		that have not been involved in a request are marked for
//		removal from the vistable

int CPlayerVisTable::AddVisTableEntriesToPriorityList()
{
	int numAdded = 0;
	int worstPriority = 0;
	int worstPriorityIndex = 0;

	const int kNumUsedVisTableEntries = m_numUsedVisTableEntries;

	for(int i = 0; i < kNumUsedVisTableEntries; i++)
	{
		CryPrefetch(&m_visTableEntries[i+4]);

		SVisTableEntry& visInfo = m_visTableEntries[i];

		const int lastRequestedLatency = visInfo.lastRequestedLatency;
		const int framesSinceLastCheck = visInfo.framesSinceLastCheck;
		int newEntryPriority = lastRequestedLatency - framesSinceLastCheck;

		if(visInfo.flags & eVF_Requested)
		{
			if(numAdded < kMaxVisTableLinetestsPerFrame)
			{	
				SVisTablePriority& visPriority = m_visTablePriorities[numAdded];
				visPriority.visInfo = &visInfo;
				visPriority.priority = newEntryPriority;
				visPriority.visIndex = i;

				if(newEntryPriority > worstPriority)
				{
					worstPriority				= newEntryPriority;
					worstPriorityIndex	= numAdded;					
				}

				numAdded++;
			}
			else if(newEntryPriority < worstPriority)
			{
				SVisTablePriority& worstVisPriority = m_visTablePriorities[worstPriorityIndex];

				worstPriority = newEntryPriority;
				worstVisPriority.priority		= newEntryPriority;
				worstVisPriority.visInfo		= &visInfo;
				worstVisPriority.visIndex		= i;

				for(int j = 0; j < numAdded; j++)
				{
					SVisTablePriority& visP = m_visTablePriorities[j];
					if(worstPriority <= visP.priority )
					{
						worstPriority				= visP.priority;
						worstPriorityIndex	= j;
					}
				}
			}
		}
		else
		{
			if(visInfo.framesSinceLastCheck > (kMinUnusedFramesBeforeEntryRemoved + visInfo.lastRequestedLatency))
			{
				visInfo.flags |= eVF_Remove;
			}
		}
	}

	return numAdded;
}

SDeferredLinetestReceiver * CPlayerVisTable::GetAvailableDeferredLinetestReceiver(SDeferredLinetestBuffer& visBuffer)
{
	for(int i = 0; i < kMaxVisTableLinetestsPerFrame; i++)
	{
		if(visBuffer.m_deferredLinetestReceivers[i].IsFree())
		{
			return &visBuffer.m_deferredLinetestReceivers[i];
		}
	}

	assert(!"Failed to find free processing entry when one was present according to m_numLinetestsCurrentlyProcessing");
	CryLogAlways("[RS] GetAvailableEntryForProcessing: Failed to find a valid index, about to memory overwrite\n[RS]  Num Entries Processing: %d\n[RS]  Num Free: %d\n", visBuffer.m_numLinetestsCurrentlyProcessing, kMaxVisTableLinetestsPerFrame - visBuffer.m_numLinetestsCurrentlyProcessing);

	return NULL;
}

//---------------------------------------------
// Check to see if the local player can see a given entity.
//		This function can flag entities as visible or not,
//		or to be removed.

void CPlayerVisTable::DoVisibilityCheck(const Vec3& localPlayerPosn, SVisTableEntry& visInfo, VisEntryIndex visIndex)
{
	Vec3 vecToTarget;

	IEntity * pEntity = gEnv->pEntitySystem->GetEntity(visInfo.entityId);

	if(pEntity)
	{
		IPhysicalEntity* pTargetPhysEnt = pEntity->GetPhysics();

		SDeferredLinetestBuffer& targetBuffer = m_linetestBuffers[GetCurrentLinetestBufferTargetIndex()];

		if(pTargetPhysEnt && targetBuffer.m_numLinetestsCurrentlyProcessing < kMaxVisTableLinetestsPerFrame)
		{
			visInfo.framesSinceLastCheck = 0;

			SDeferredLinetestReceiver * processingEntry = GetAvailableDeferredLinetestReceiver(targetBuffer);

			assert(processingEntry);

			Vec3 targetPosn = pEntity->GetWorldPos();
			targetPosn.z += visInfo.heightOffset;
			vecToTarget = targetPosn - localPlayerPosn;

			processingEntry->visTableIndex = visIndex;			

			ray_hit hit;
			const int rayFlags = geom_colltype_player<<rwi_colltype_bit | rwi_stop_at_pierceable;

			m_numLinetestsThisFrame++;
			targetBuffer.m_numLinetestsCurrentlyProcessing++;

			visInfo.flags |= eVF_Pending;

			const int NumIgnoredEntities = 1;
			IPhysicalEntity *pIgnoreEntities[NumIgnoredEntities];
			pIgnoreEntities[0] = pTargetPhysEnt;

			processingEntry->visBufferIndex = m_currentBufferTarget;

			processingEntry->raycastHelper->CastRay(localPlayerPosn, vecToTarget, ent_terrain|ent_static|ent_sleeping_rigid|ent_rigid, rayFlags, &pIgnoreEntities[0], NumIgnoredEntities);
		}
	}
	else
	{
		visInfo.flags |= eVF_Remove;
	}
}


//---------------------------------------------
// Fills out the local player position

void CPlayerVisTable::GetLocalPlayerPosn(Vec3& localPlayerPosn)
{
	CCamera&  camera = gEnv->pSystem->GetViewCamera();
	localPlayerPosn = camera.GetPosition();
}

//---------------------------------------------
// Actually removes any entites from the list that have been flagged

void CPlayerVisTable::ClearRemovedEntities()
{
	for(int i = 0; i < m_numUsedVisTableEntries; i++)
	{
		CRY_TODO(30,09,2009, "Prefetch to avoid cache misses");

		SVisTableEntry& visInfo = m_visTableEntries[i];

		if(visInfo.flags & eVF_Remove)
		{
			RemoveNthEntity(i);
		}
	}
}

VisEntryIndex CPlayerVisTable::GetEntityIndexFromID(EntityId entityId)
{
	CRY_TODO(30,09,2009, "Make faster");
	CryPrefetch(((char*)m_visTableEntries) + 128);

	for(VisEntryIndex i = 0; i < m_numUsedVisTableEntries; i++)
	{
		SVisTableEntry& visInfo = m_visTableEntries[i];

		if(m_visTableEntries[i].entityId == entityId)
		{
			return i;
		}		
	}

	return -1;
}

void CPlayerVisTable::Reset()
{
	//Note: This isn't curently being called and is provided for completeness.
	//		  Due to the threading of deferred linetests there may (and probably will)
	//			be issues if this is called with linetests currently outstanding

	m_numUsedVisTableEntries = 0;

	for(int i = 0; i < kNumVisTableBuffers; i++)
	{
		SDeferredLinetestBuffer& visBuffer = m_linetestBuffers[i];

		visBuffer.m_numLinetestsCurrentlyProcessing = 0;		

		for(int j = 0; j < kMaxVisTableLinetestsPerFrame; j++)
		{
			visBuffer.m_deferredLinetestReceivers[j].SetInvalid();
			visBuffer.m_deferredLinetestReceivers[j].SetFree();			
		}
	}
}

void SDeferredLinetestReceiver::OnDataReceived(const EventPhysRWIResult *pRWIResult)
{
	CPlayerVisTable * visTable = g_pGame->GetPlayerVisTable();

	//This may be receiving results from the previous frame's physics, when
	//	 the game has just been reset and the vis table with it
	if(!IsFree())
	{
		SDeferredLinetestBuffer& visBuffer = visTable->GetDeferredLinetestBuffer(visBufferIndex);

		if(IsValid())
		{
			SVisTableEntry& visEntry = visTable->GetNthVisTableEntry(visTableIndex);

			if(pRWIResult->nHits)
			{
				//The linetest hit something en route to the target
				visEntry.flags &= ~(eVF_Visible|eVF_Requested|eVF_Pending);		
			}
			else
			{
				visEntry.flags |= eVF_Visible;
				visEntry.flags &= ~(eVF_Requested|eVF_Pending);
			}

			SetInvalid();
			visEntry.framesSinceLastCheck = 0;
		}

		//Mark this SVisTableEntry_Processing as available
		SetFree();
		visBuffer.m_numLinetestsCurrentlyProcessing--;
	}
}