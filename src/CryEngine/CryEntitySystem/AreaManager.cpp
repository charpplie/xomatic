//////////////////////////////////////////////////////////////////////
//
//  Game Source Code
//
//  File: XArea.cpp
//  Description: 2D area class. Area is in XY plane.
//	2D areas manager
//
//  History:
//  - Feb 24, 2002: Created by Kirill Bulatsev
//
//////////////////////////////////////////////////////////////////////
 
#include "stdafx.h"
#include "Area.h"
#include "AreaManager.h"
#include "ISound.h"

//////////////////////////////////////////////////////////////////////////
//*************************************************************************************************************
//
//			AREAs MANEGER
//
//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
CAreaManager::CAreaManager( CEntitySystem *pEntitySystem )// : CSoundAreaManager(pEntitySystem)
:	m_pEntitySystem(pEntitySystem),
	m_pCurrArea(NULL),
	m_pPrevArea(NULL),
	m_nCurStep(1),
	m_nCurSoundStep(11),
	m_nCurTailStep(22),
	m_bAreasDirty(true)
{
}

//////////////////////////////////////////////////////////////////////////
CAreaManager::~CAreaManager()
{
	assert( m_vpAreas.size() == 0 );
}

//////////////////////////////////////////////////////////////////////////
CArea* CAreaManager::CreateArea()
{
	CArea *pArea = new CArea(this);

	m_vpAreas.push_back( pArea );
	return pArea;
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::Unregister( CArea *pArea )
{
	//invalidating cache and grid
	m_areaCache.clear();
	m_bAreasDirty = true;

	// removing the area reference
	unsigned int const nCount = m_vpAreas.size();
	for (unsigned int i = 0; i < nCount; i++)
	{
		if (m_vpAreas[i] == pArea)
		{
			m_vpAreas.erase( m_vpAreas.begin() + i );
			break;
		}
	}
}

//	gets area by point
//////////////////////////////////////////////////////////////////////////
CArea*	CAreaManager::GetArea( const Vec3& point )
{
	float		dist;
	float		closeDist	= -1.0f;
	CArea*	closeArea	= NULL;

	unsigned int const nCount = m_vpAreas.size();
	for (unsigned int aIdx = 0; aIdx < nCount; ++aIdx)
	{
		if (m_vpAreas[aIdx]->GetAreaType() == ENTITY_AREA_TYPE_SHAPE)
		{
			dist = m_vpAreas[aIdx]->CalcPointWithinDist(point, false);
			if (dist>0)
			{
				if ( !closeArea || closeDist>dist )
				{
					closeDist = dist;
					closeArea = m_vpAreas[aIdx];
				}
			}
		}
	}
	return closeArea;
}

//	gets area by index
//////////////////////////////////////////////////////////////////////////
const IArea* CAreaManager::GetArea(int areaIndex) const
{
	CArea *pArea = m_vpAreas[areaIndex];
	return (IArea*)pArea;
}

void CAreaManager::DrawLinkedAreas(EntityId linkedId) const
{
	std::vector<CArea *> areas;
	const unsigned int iNumAreas = GetLinkedAreas(linkedId, -1, areas);
	for (unsigned int iIdx = 0; iIdx < iNumAreas; ++iIdx)
	{
		areas[iIdx]->Draw(iIdx);
	}
}

int CAreaManager::GetLinkedAreas(EntityId linkedId, int areaId, std::vector<CArea *> &areas) const
{
	static std::vector<EntityId> ids;
	unsigned int const nCount = m_vpAreas.size();
	for (unsigned int aIdx = 0; aIdx < nCount; aIdx++)
	{
		if (CArea *pArea = m_vpAreas[aIdx])
		{
			ids.resize(0);
			pArea->GetEntites(ids);

			if (!ids.empty())
			{
				unsigned int const nidCount = ids.size();
				for (unsigned int eIdx = 0; eIdx < nidCount; eIdx++)
				{
					if (ids[eIdx] == linkedId)
					{
						if (areaId == -1 || areaId == pArea->GetID())
							areas.push_back(pArea);
					}
				}
			}
		}
	}

	return areas.size();
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::MarkPlayerForUpdate( EntityId id )
{
	// we could get the same entity for update in one frame -> push_back_unique
	stl::push_back_unique(m_playerEntitiesToUpdate, id);
}

//////////////////////////////////////////////////////////////////////////
void  CAreaManager::Update()
{
	unsigned int const nCount = m_playerEntitiesToUpdate.size();
	for (uint32 i = 0; i < nCount; ++i)
	{
		IEntity *pPlayer = g_pIEntitySystem->GetEntity(m_playerEntitiesToUpdate[i]);
		if (pPlayer)
		{
			Vec3 playerPos = pPlayer->GetWorldPos() + Vec3(0,0,0.01f);
			UpdatePlayer( playerPos ,pPlayer );
		}
	}
	m_playerEntitiesToUpdate.clear();
}

//	check pPlayer's position for all the areas. Do onEnter, onLeave, updateFade
//////////////////////////////////////////////////////////////////////////
void	CAreaManager::UpdatePlayer( const Vec3 &vPos,IEntity *pPlayer )
{
	FUNCTION_PROFILER( GetISystem(),PROFILE_ENTITY );

	SAreasCache*  pAreaCache = GetAreaCache(pPlayer->GetId());
	if (pAreaCache)
	{
		if (vPos == pAreaCache->vLastUpdatePos)
			return;
		pAreaCache->vLastUpdatePos = vPos;
	}

	// increase the step
	m_nCurStep++;

	unsigned int nAreaIndex;

	if (pAreaCache)
	{
		int const nCacheCount = pAreaCache->Areas.size();

		// Invalidate all areas' data cache
		for (int nCacheIndex = 0; nCacheIndex < nCacheCount; ++nCacheIndex)
		{
			nAreaIndex = pAreaCache->Areas[nCacheIndex].nAreaIndex;

			// Safe check for editor
			if (nAreaIndex >= m_vpAreas.size())
				return;

			CArea* const pArea = m_vpAreas[nAreaIndex];

			// Skip those areas being processed already in the cache
			if (pArea->GetStepId() != m_nCurStep)
			{
				pArea->InvalidateCachedAreaData();

				// Now pre-calculate position data
				pArea->CalcCachedPointPosType(vPos);
			}
		}

		// check all the areas pPlayer is in already
		for (int nCacheIndex = 0; nCacheIndex < static_cast<int>(pAreaCache->Areas.size()); ++nCacheIndex)
		{
			nAreaIndex = pAreaCache->Areas[nCacheIndex].nAreaIndex;

			// safe check for editor
			if (nAreaIndex >= m_vpAreas.size())
				return;

			CArea* const pArea = m_vpAreas[nAreaIndex];

			// set flag that this area was processed
			if (pArea->GetStepId() == m_nCurStep)
				continue;

			//bool bWasUpdatedAlready = (pArea->GetStepId() == 0);

			// check if Area is hidden
			IEntity const* const pAreaEntity = m_pEntitySystem->GetEntity(pArea->GetEntityID());
			if (pAreaEntity && pAreaEntity->IsHidden())
			{
				// Is Hidden, so this Area doesn't belong here
				ProceedExclusiveLeave( pPlayer, nAreaIndex );
				pAreaCache->Areas.erase(  pAreaCache->Areas.begin() + nCacheIndex );
				nCacheIndex--;

				SEntityEvent event(pPlayer->GetId(), 0, 0, 0, 0.0f, 0.0f);
				event.event = ENTITY_EVENT_LEAVENEARAREA;
				pArea->SetCachedEvent(&event);
				pArea->SetStepId( m_nCurStep );
				continue;
			}

			//Vec3 Closest3d;
//			float fEffectRadius = pArea->GetMaximumEffectRadius();

			ProcessArea(pArea, nAreaIndex, nCacheIndex, pAreaCache, vPos, pPlayer);
			pArea->SetStepId(m_nCurStep);
		}
	}

	// Update the area grid data structure.
	UpdateDirtyAreas();

	CAreaGrid::iterator itArea		= m_areaGrid.BeginAreas(vPos);
	CAreaGrid::iterator itAreaEnd	= m_areaGrid.EndAreas(vPos);

	// Invalidate all areas' data cache
	for (; itArea != itAreaEnd; ++itArea)
	{
		nAreaIndex = *itArea;
		CArea* const pArea = m_vpAreas[nAreaIndex];

		// skip those areas being processed already in the cache
		if (pArea->GetStepId() != m_nCurStep)
		{
			pArea->InvalidateCachedAreaData();

			// Now pre-calculate position data
			pArea->CalcCachedPointPosType(vPos);
		}
	}

	itArea = m_areaGrid.BeginAreas(vPos);

	// check all the rest areas (pPlayer was outside/far of them)
	for (; itArea != itAreaEnd; ++itArea)
	{
		nAreaIndex = *itArea;
		CArea* const pArea = m_vpAreas[nAreaIndex];

		// skip those areas being processed already in the cache
		if (pArea->GetStepId() != m_nCurStep)
		{
			// check if Area is hidden
			IEntity const* const pAreaEntity = m_pEntitySystem->GetEntity(pArea->GetEntityID());
			if (pAreaEntity && pAreaEntity->IsHidden())
			{
				// Is Hidden, so this Area is marked as "left"
				SEntityEvent event(pPlayer->GetId(), 0, 0, 0, 0.0f, 0.0f);
				event.event = ENTITY_EVENT_LEAVENEARAREA;
				pArea->SetCachedEvent(&event);

				if (pAreaCache)
				{
					// if that area is in the cache, remove it from there
					unsigned int const nAreaCount = pAreaCache->Areas.size();
					for (unsigned int inIdx = 0; inIdx < nAreaCount; ++inIdx)
					{
						unsigned int const TempaIdx = pAreaCache->Areas[inIdx].nAreaIndex;
						if (TempaIdx == nAreaIndex)
						{
							pAreaCache->Areas.erase(pAreaCache->Areas.begin() + inIdx);
							break;
						}
					}
				}
				continue;
			}

			float const fMaxRadius	= pArea->GetMaximumEffectRadius();
			
			float fDistanceSq = 0.0f;
			if (pArea->GetFlags() & eCachedAreaData_DistNearSqValid)
				fDistanceSq = pArea->GetCachedPointNearDistSq();
			else
				fDistanceSq = pArea->CalcPointNearDistSq(vPos, false, false); // Take sound obstruction into account?

			bool bIsPointWithin = false;
			if (pArea->GetFlags() & eCachedAreaData_PosTypeValid)
				bIsPointWithin = pArea->GetCachedPointWithin();
			else
				bIsPointWithin = pArea->CalcPointWithin(vPos);

			if (fDistanceSq < fMaxRadius*fMaxRadius || bIsPointWithin)
			{
				// was far away, now enter near or even inside
				if (!pAreaCache)
					pAreaCache = MakeAreaCache(pPlayer->GetId());

				int nCacheIndex = pAreaCache->GetCacheIndex(nAreaIndex);

				if (nCacheIndex == -1)
				{
					// EnterArea is send if bInside is false, that means area was not in cache already
					pAreaCache->Areas.push_back(SAreaCacheEntry(nAreaIndex, false));
					nCacheIndex = pAreaCache->GetCacheIndex(nAreaIndex);

					// send EnterNearArea event if entity did not jump into the area
					if (!bIsPointWithin)
					{
						SEntityEvent event(pPlayer->GetId(), 0, 0, 0, 0.0f, fDistanceSq);
						event.event = ENTITY_EVENT_ENTERNEARAREA;
						pArea->SetCachedEvent(&event);	
						pArea->SetStepId(m_nCurStep);
					}
				}

				ProcessArea(pArea, nAreaIndex, nCacheIndex, pAreaCache, vPos, pPlayer);
			}
			else
			{
				// for some reason jumped from inside or near to far away

				if (pAreaCache)
				{
					unsigned int const nAreaCount = pAreaCache->Areas.size();
					for (unsigned int inIdx = 0; inIdx < nAreaCount; ++inIdx)
					{
						unsigned int TempaIdx = pAreaCache->Areas[inIdx].nAreaIndex;
						if (TempaIdx == nAreaIndex)
						{
							pAreaCache->Areas.erase(  pAreaCache->Areas.begin() + inIdx );
							break;
						}
					}
				}
			}
		}
	}

	if (pAreaCache)
	{
		//
		//update fade. For all hosted areas
		unsigned int const nCacheAreaCount = pAreaCache->Areas.size();
		for (unsigned int nCacheIndex = 0; nCacheIndex < nCacheAreaCount; ++nCacheIndex)
		{
			nAreaIndex = pAreaCache->Areas[nCacheIndex].nAreaIndex;
			
			// Safe check for editor
			if (nAreaIndex >= m_vpAreas.size())
				return;

			CArea const* const pArea = m_vpAreas[nAreaIndex];

			// this one already updated
			if ( pArea->GetStepId() == 0 )
				continue;
			
			// this area is not active - overwritten by same groupId area with higher id (exclusive areas)
			if ( !pArea->IsActive() )
				continue;

			ProceedExclusiveUpdate( pPlayer, nAreaIndex);
			//		m_vpAreas[aIdx]->UpdateArea(pPlayer);
		}

		if (pAreaCache->Areas.empty())
		{
			DeleteAreaCache( pPlayer->GetId() );
		}
	}

	// Go through all Areas again and send the events now

	unsigned int const nAreaCount = m_vpAreas.size();
	for (nAreaIndex = 0; nAreaIndex < nAreaCount; ++nAreaIndex)
	{
		CArea* const pArea = m_vpAreas[nAreaIndex];
		pArea->SendCachedEvent();
		pArea->SetStepId( m_nCurStep );
	}

	//SoundUpdatePlayer(vPos, pPlayer);
}

bool CAreaManager::QueryAreas( Vec3 vPos, SAreaManagerResult *pResults, int nMaxResults)
{
	if (pResults)
	{
		int nResultCount = 0;

		// search all areas the point falls into
		for (CAreaGrid::iterator itArea = m_areaGrid.BeginAreas(vPos), itAreaEnd = m_areaGrid.EndAreas(vPos); itArea != itAreaEnd; ++itArea)
		{
			unsigned int nAreaIndex = *itArea;
			CArea* pArea = m_vpAreas[nAreaIndex];

			// check if Area is hidden
			IEntity *pAreaEntity = m_pEntitySystem->GetEntity(pArea->GetEntityID());

			if (pAreaEntity && pAreaEntity->IsHidden())
				continue;

			Vec3 Closest3d(0);
			float fMaxRadius = pArea->GetMaximumEffectRadius();

			float fDistanceSq = 0.0f;
			if (pArea->GetFlags() & eCachedAreaData_DistNearSqValid)
				fDistanceSq = pArea->GetCachedPointNearDistSq();
			else
				fDistanceSq = pArea->CalcPointNearDistSq(vPos, Closest3d, false);

			bool bIsPointWithin = false;
			if (pArea->GetFlags() & eCachedAreaData_PosTypeValid)
				bIsPointWithin = pArea->GetCachedPointWithin();
			else
				bIsPointWithin = pArea->CalcPointWithin(vPos);

			if (fDistanceSq < fMaxRadius*fMaxRadius || bIsPointWithin)
			{
				// still room to put it in?
				if (nResultCount >= nMaxResults)
					return false;

				// found area that should go into the results
				pResults[nResultCount].pArea = pArea;
				pResults[nResultCount].fDistanceSq = fDistanceSq;
				pResults[nResultCount].vPosOnHull = Closest3d;
				pResults[nResultCount].bInside = bIsPointWithin;
				pResults[nResultCount].bNear = (fDistanceSq < fMaxRadius*fMaxRadius);

				++nResultCount;
			}
		}

		return true;
	}
		
	return false;
}


void CAreaManager::ProcessArea(CArea* const __restrict pArea, unsigned int const nAreaIndex, int& nCacheIndex, SAreasCache* const pAreaCache, const Vec3& vPos, IEntity const* const pPlayer)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_SOUND);

	Vec3 Closest3d;
	bool const bWasUpdatedAlready	= (pArea->GetStepId() == m_nCurStep);
	float const fEffectRadius			= pArea->GetMaximumEffectRadius();

	bool bIsPointWithin = false;
	if (pArea->GetFlags() & eCachedAreaData_PosTypeValid)
		bIsPointWithin = pArea->GetCachedPointWithin();
	else
		bIsPointWithin = pArea->CalcPointWithin(vPos);

	if (bIsPointWithin)
	{
		// was inside/near, now is is inside

		float fDistance = 0.0f;
		if (pArea->GetFlags() & eCachedAreaData_DistWithinSqValid)
			fDistance = cry_sqrtf(pArea->GetCachedPointWithinDistSq());
		else
			fDistance = pArea->CalcPointWithinDist(vPos, false);

		float fFade = 1.0f;

		// if there is no cached event waiting, Fade can be overwritten
		if (!bWasUpdatedAlready)
			pArea->SetFade(fFade);

		// fade for nested areas
		fFade = fEffectRadius>0.f ? max(0.f, (fEffectRadius-fDistance)/fEffectRadius) : 0.f; 
		bool bPosInLowerArea = false;
		bool bPosInHigherArea = false;

		int nLowerArea = FindLowestHostedArea(pAreaCache->Areas, nAreaIndex);
		int nHigherArea = FindHighestHostedArea( pAreaCache->Areas, nAreaIndex );

		if (nLowerArea != -1)
		{
			CArea const* const __restrict pLowerArea = m_vpAreas[nLowerArea];
			if (pLowerArea->GetFlags() & eCachedAreaData_PosTypeValid)
				bPosInLowerArea = pLowerArea->GetCachedPointWithin();
			else
				bPosInLowerArea = pLowerArea->CalcPointWithin(vPos);

			if (bPosInLowerArea)
				ProceedExclusiveSoundUpdate(pPlayer, nAreaIndex, fFade, pArea);
		}

		if (nHigherArea != -1)
		{
			CArea const* const __restrict pHigherArea = m_vpAreas[nHigherArea];
			if (pHigherArea->GetFlags() & eCachedAreaData_PosTypeValid)
				bPosInHigherArea = pHigherArea->GetCachedPointWithin();
			else
				bPosInHigherArea = pHigherArea->CalcPointWithin(vPos);

			//if (bPosInHigherArea)
			//ProceedExclusiveSoundUpdate(pPlayer, nAreaIndex, fFade, pArea);
		}

		if (!bPosInHigherArea)
		{
			SEntityEvent event(pPlayer->GetId(), 0, 0, 0, 0.0f, 0.0f);
			if (pAreaCache->Areas[nCacheIndex].bInside)
				event.event = ENTITY_EVENT_MOVEINSIDEAREA;
			else
				event.event = ENTITY_EVENT_ENTERAREA;

			pArea->SetCachedEvent(&event);
			pArea->SendCachedEvent(); // send it out immediately so higher areas overwrite these settings.
		}

		pAreaCache->Areas[nCacheIndex].bInside = true;

	}
	else
	{
		// was inside/near, now is not inside anymore
		float fDistanceSq = 0.0f;
		if (pArea->GetFlags() & eCachedAreaData_DistNearSqValid)
			fDistanceSq = pArea->GetCachedPointNearDistSq();
		else
			fDistanceSq = pArea->CalcPointNearDistSq(vPos, Closest3d, false);

		if (fDistanceSq < fEffectRadius*fEffectRadius)
		{
			// is near now

			// if there is no cached event waiting, Fade can be overwritten
			if (!bWasUpdatedAlready)
			{
				pArea->SetFade(1.0f);

				SEntityEvent event(pPlayer->GetId(), 0, 0, 0, 0.0f, fDistanceSq);
				if (!pAreaCache->Areas[nCacheIndex].bInside)
					event.event = ENTITY_EVENT_MOVENEARAREA;
				else
					event.event = ENTITY_EVENT_LEAVEAREA;

				pArea->SetCachedEvent(&event);					
			}
			
			pAreaCache->Areas[nCacheIndex].bInside = false;

			//m_vpAreas[aIdx]->UpdateAreaNear(pPlayer, fDistanceSq);
		}
		else
		{
			// now is far - do LeaveNearArea
			ProceedExclusiveLeave( pPlayer, nAreaIndex );
			pAreaCache->Areas.erase(  pAreaCache->Areas.begin() + nCacheIndex );
			nCacheIndex--;

			SEntityEvent event(pPlayer->GetId(), 0, 0, 0, 0.0f, 0.0f);
			event.event = ENTITY_EVENT_LEAVENEARAREA;
			pArea->SetCachedEvent(&event);
			//m_vpAreas[aIdx]->LeaveNearArea(pPlayer);
		}
	}

}


void	CAreaManager::UpdateTailEntity( const Vec3 &vPos,IEntity *pTailEntity )
{
	FUNCTION_PROFILER( GetISystem(),PROFILE_ENTITY );

	SAreasCache *pAreaCache = GetAreaCache(pTailEntity->GetId());	
	if (pAreaCache)
	{
		if (vPos == pAreaCache->vLastUpdatePos)
			return;
		pAreaCache->vLastUpdatePos = vPos;
	}

	// increase the step
	m_nCurTailStep++;

	unsigned int aIdx;

	if (pAreaCache)
	{
		// check all the areas pPlayer is in already
		unsigned int const nCacheCount = pAreaCache->Areas.size();
		for (unsigned int inIdx = 0; inIdx < nCacheCount; inIdx++)
		{
			aIdx = pAreaCache->Areas[inIdx].nAreaIndex;

			//safecheck for editor
			if ( aIdx>=m_vpAreas.size() )
				return;

			CArea* pArea = m_vpAreas[aIdx];

			pArea->SetStepId( m_nCurTailStep );
			if (!pArea->CalcPointWithin(vPos))
			{
				// was inside, now is out - do OnLeaveArea
				pAreaCache->Areas.erase(  pAreaCache->Areas.begin() + inIdx );
				inIdx--;
			}
		}
	}

	// check all the rest areas (pPlayer is outside of them)
	unsigned int const nCount = m_vpAreas.size();
	for (aIdx = 0; aIdx < nCount; aIdx++)
	{
					
		CArea* pArea = m_vpAreas[aIdx];

		if ( pArea->GetStepId() != m_nCurTailStep )
		{
			if (pArea->HasSoundAttached()) 
			{
				if (pArea->CalcPointWithin(vPos))
				{
					// was outside, now inside - do enter area
					//m_vpAreas[aIdx]->EnterArea(pTailEntity);

					if (!pAreaCache)
						pAreaCache = MakeAreaCache(pTailEntity->GetId());
					pAreaCache->Areas.push_back( SAreaCacheEntry(aIdx, true) );
				}
			}
		}
	}

	// skip proceed fade for weapons

	if (pAreaCache)
	{
		if (pAreaCache->Areas.empty())
		{
			DeleteAreaCache( pTailEntity->GetId() );
		}
	}
}

//	checks for areas in the same group, if entered area is lower priority (areaID) - return false 
//	(don't do enreArea for it)
//////////////////////////////////////////////////////////////////////////
void	CAreaManager::ProceedExclusiveUpdate( IEntity const* const pPlayer , unsigned int const nAreaIndex )
{
	int	maxArea = -1;
	SAreasCache *pAreaCache = GetAreaCache(pPlayer->GetId());
	if (pAreaCache)
		maxArea = FindHighestHostedArea( pAreaCache->Areas, nAreaIndex );

	CArea *firstArea=m_vpAreas[nAreaIndex];

	// not grouped or the only one in a group
	if (maxArea < 0)
	{
		firstArea->UpdateArea( pPlayer->GetWorldPos(),pPlayer );
		return;
	}

	CArea *secondArea = m_vpAreas[maxArea];

	if ( firstArea->GetID()<secondArea->GetID() )
	{
		firstArea = secondArea;
		int	secondMaxArea = FindHighestHostedArea( pAreaCache->Areas, maxArea );
		if (secondMaxArea >= 0)
		{
			secondArea=m_vpAreas[secondMaxArea];
		}
	}

	Vec3 pos = pPlayer->GetWorldPos();
	float fadeBase = firstArea->CalculateFade(pos);

	if (fadeBase < 1.0f)
	{
		if ( !secondArea->IsActive() )
			secondArea->EnterArea(pPlayer);

		float fadeSecond = secondArea->CalculateFade(pos);
		fadeSecond *= (1.0f-fadeBase);
		firstArea->ProceedFade(pPlayer, fadeBase);
		secondArea->ProceedFade(pPlayer, fadeSecond);
	}
	else
	{
		if ( secondArea->IsActive() )
			secondArea->LeaveArea(pPlayer);
	}

	// mark all the areas of this group as updated
	int curGroup = m_vpAreas[nAreaIndex]->GetGroup();
	unsigned int const nCount = pAreaCache->Areas.size();
	for (unsigned int inIdx = 0; inIdx < nCount; inIdx++)
	{
		unsigned int aIdx = pAreaCache->Areas[inIdx].nAreaIndex;

		//safecheck for editor
		if ( aIdx >= m_vpAreas.size() )
			continue;

		if (m_vpAreas[nAreaIndex]->GetGroup() != m_vpAreas[aIdx]->GetGroup())
			continue;

		m_vpAreas[aIdx]->SetStepId(0);
	}
}

//	checks for areas in the same group, if entered area is lower priority (areaID) 
//	change fade value the deeper player is inside
//////////////////////////////////////////////////////////////////////////
void	CAreaManager::ProceedExclusiveSoundUpdate(IEntity const* const pPlayer , unsigned int const nAreaIndex, float const fFade, CArea const* const __restrict pArea)
{
	SAreasCache * const pAreaCache = GetAreaCache(pPlayer->GetId());

	if (pAreaCache)
	{
		int nLowerArea = nAreaIndex;
		do 
		{
			nLowerArea = FindLowestHostedArea(pAreaCache->Areas, nLowerArea);
			if (nLowerArea != -1)
			{
				// there is a area with lower priority, so this one controls it
				CArea *lowerArea = m_vpAreas[nLowerArea];
				lowerArea->SetFade( fFade*lowerArea->GetFade() );			
				lowerArea->ExclusiveUpdateAreaInside(pPlayer, pArea->GetEntityID());
				lowerArea->SetStepId(m_nCurStep);

			} 
		} while (nLowerArea != -1);

		int nHigherArea = nAreaIndex;
		do 
		{
			nHigherArea = FindHighestHostedArea(pAreaCache->Areas, nHigherArea);
			if (nHigherArea != -1)
			{
				// there is a area with higher priority, so this one is later processed by it
				//CArea *higherArea = m_vpAreas[nHigherArea];
				//higherArea->SetFade(fFade);
				//higherArea->SetStepId(m_nCurStep);
			} 
		} while (nHigherArea != -1);


		//Vec3 pos = pPlayer->GetWorldPos();
		//float fadeBase = firstArea->CalcDistToPoint()

		//if ( fadeBase<1.0f )
		//{
		//	if ( !secondArea->IsActive() )
		//		secondArea->EnterArea(pPlayer);
		//	float fadeSecond = secondArea->CalculateFade(pos);
		//	fadeSecond *= (1.0f-fadeBase);
		//	firstArea->ProceedFade(pPlayer, fadeBase);
		//	secondArea->ProceedFade(pPlayer, fadeSecond);
		//}
		//else
		//{
		//	if ( secondArea->IsActive() )
		//		secondArea->LeaveArea(pPlayer);
		//}

		// mark all the areas of this group as updated
		int curGroup = m_vpAreas[nAreaIndex]->GetGroup();
		unsigned int const nCount = pAreaCache->Areas.size();
		for (unsigned int inIdx = 0; inIdx < nCount; inIdx++)
		{
			unsigned int aIdx = pAreaCache->Areas[inIdx].nAreaIndex;

			//safecheck for editor
			if ( aIdx >= m_vpAreas.size() )
				continue;

			if (m_vpAreas[nAreaIndex]->GetGroup() != m_vpAreas[aIdx]->GetGroup())
				continue;

			//m_vpAreas[aIdx]->SetStepId(0);
		}

	}
}



//	checks for areas in the same group, if entered area is lower priority (areaID) - return false 
//	(don't do enreArea for it)
//////////////////////////////////////////////////////////////////////////
bool	CAreaManager::ProceedExclusiveEnter( IEntity const* const pPlayer , unsigned int const nAreaIndex )
{
	int	maxArea = -1;
	SAreasCache *pAreaCache = GetAreaCache(pPlayer->GetId());
	if (pAreaCache)
		maxArea = FindHighestHostedArea( pAreaCache->Areas, nAreaIndex );

	if (maxArea >= 0)
	{
		int maxID = m_vpAreas[maxArea]->GetID();

		if (m_vpAreas[nAreaIndex]->GetID()<maxID)
			return false;
	}

	//	m_vpAreas[aIdx]->LeaveArea( pPlayer );			// new area will override this one
	return true;
}

//	checks for areas in the same group, if entered area is lower priority (areaID) - return false 
//	(don't do enreArea for it)
//////////////////////////////////////////////////////////////////////////
bool	CAreaManager::ProceedExclusiveLeave( IEntity const* const pPlayer , unsigned int const nAreaIndex )
{
	int	maxArea = -1;
	SAreasCache *pAreaCache = GetAreaCache(pPlayer->GetId());

	if (pAreaCache)
		maxArea = FindHighestHostedArea( pAreaCache->Areas, nAreaIndex );

	if ( maxArea >= 0 )
	{
		int maxID = m_vpAreas[maxArea]->GetID();
		if ( m_vpAreas[nAreaIndex]->GetID()<maxID )
			return false;

		m_vpAreas[nAreaIndex]->LeaveArea( pPlayer );

		if (!m_vpAreas[maxArea]->IsActive())
		{
			SEntityEvent event(pPlayer->GetId(), 0, 0, 0, 0.0f, 0.0f);
			event.event = ENTITY_EVENT_ENTERAREA;
			m_vpAreas[maxArea]->SetCachedEvent(&event);
			//m_vpAreas[maxArea]->EnterArea( pPlayer );			// new area will owerride this one
		}
	}
	else
	{
		m_vpAreas[nAreaIndex]->LeaveArea( pPlayer );
	}

	return true;
}

//	do onexit for all areas pPlayer is in - do it before kill pPlayer
//////////////////////////////////////////////////////////////////////////
void	CAreaManager::ExitAllAreas( IEntity const* const pPlayer )
{
	SAreasCache *pAreaCache = GetAreaCache(pPlayer->GetId());

	if (pAreaCache)
	{
		// check all the areas pPlayer is in already
		unsigned int const nCount = pAreaCache->Areas.size();
		for (unsigned int inIdx = 0; inIdx < nCount; inIdx++)
		{
			unsigned int aIdx;
			aIdx = pAreaCache->Areas[inIdx].nAreaIndex;
			// was inside, now is out - do OnLeaveArea
			if (aIdx<m_vpAreas.size())
			{
				m_vpAreas[aIdx]->LeaveArea( pPlayer );
				pAreaCache->Areas.erase( pAreaCache->Areas.begin() + inIdx );
				inIdx--;
			}
		}
		DeleteAreaCache( pPlayer->GetId() );
	}
}


/*
// [marco] functions common for game and editor mode
// to be able to retrigger areas etc.
//////////////////////////////////////////////////////////////////////////
void CAreaManager::CheckSoundVisAreas()
{
	m_pPrevArea = gEnv->pSoundSystem->GetListenerArea();		
	if ((!m_pCurrArea && m_pPrevArea) || (m_pCurrArea && !m_pPrevArea))
	{
		// if we switched between outdoor and indoor, let's
		// retrigger the area to account for random ambient sounds
		// indoor / outdoor only
		//		ReTriggerArea(gEnv->pSoundSystem->GetListenerPos(),false);

		//
		//if ( GetMyPlayer() )
		//ReTriggerArea(GetMyPlayer(), gEnv->pSoundSystem->GetListenerPos(),false);
	}
	m_pPrevArea = m_pCurrArea;
	m_pCurrArea = gEnv->pSoundSystem->GetListenerArea();
}
*/

//	find hosted area with highest priority within the group of nAreaIndex area
//////////////////////////////////////////////////////////////////////////
int	CAreaManager::FindHighestHostedArea( TAreaCacheVector const& hostedAreas, unsigned int const nAreaIndex )const
{
	unsigned int aIdx;
	int	minPriority = m_vpAreas[nAreaIndex]->GetPriority();
	int	higherArea = -1;
	int curGroup = m_vpAreas[nAreaIndex]->GetGroup();

	if (curGroup <= 0)
	{
		return higherArea;
	}

	unsigned int const nHostedAreaCount = hostedAreas.size();
	for (unsigned int inIdx = 0; inIdx < nHostedAreaCount; ++inIdx)
	{
		aIdx = hostedAreas[inIdx].nAreaIndex;
		if ( aIdx == nAreaIndex )
			continue;

		//safecheck for editor
		if ( aIdx >= m_vpAreas.size() )
			continue;

		CArea const* const pArea = m_vpAreas[aIdx];

		if (curGroup != pArea->GetGroup())	// different groups
			continue;

		if (minPriority < pArea->GetPriority())
		{
			minPriority = pArea->GetPriority();
			higherArea	= aIdx;
		}
	}
	return higherArea;
}


//	find hosted area with lowest priority within the group of nAreaIndex area
//////////////////////////////////////////////////////////////////////////
int	CAreaManager::FindLowestHostedArea( TAreaCacheVector const& hostedAreas, unsigned int const nAreaIndex )const
{
	unsigned int aIdx;
	int	maxPriority = -1;
	int	lowerArea = -1;
	int curGroup = m_vpAreas[nAreaIndex]->GetGroup();

	if (curGroup > 0)
	{
		unsigned int const nHostedAreaCount = hostedAreas.size();
		for (unsigned int inIdx = 0; inIdx < nHostedAreaCount; ++inIdx)
		{
			aIdx = hostedAreas[inIdx].nAreaIndex;
			if ( aIdx == nAreaIndex )
				continue;

			//safecheck for editor
			if ( aIdx >= m_vpAreas.size() )
				continue;

			CArea const* const pArea = m_vpAreas[aIdx];

			if (curGroup != pArea->GetGroup())	// different groups
				continue;

			if ( (pArea->GetPriority() > maxPriority) && (pArea->GetPriority() < m_vpAreas[nAreaIndex]->GetPriority() ) )
			{
				maxPriority = pArea->GetPriority();
				lowerArea	= aIdx;
			}
		}
	}
		
	return lowerArea;
}

//////////////////////////////////////////////////////////////////////////
void	CAreaManager::DrawAreas(const ISystem * const pSystem) 
{
	unsigned int const nCount = m_vpAreas.size();
	for (unsigned int aIdx = 0; aIdx < nCount; aIdx++)
		m_vpAreas[aIdx]->Draw( aIdx );
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::DrawGrid()
{
	m_areaGrid.Draw();
}

//////////////////////////////////////////////////////////////////////////
unsigned CAreaManager::MemStat()
{
	unsigned memSize = sizeof *this;

	unsigned int const nCount = m_vpAreas.size();
	for (unsigned int aIdx = 0; aIdx < nCount; aIdx++)
		memSize += m_vpAreas[aIdx]->MemStat();

	return memSize;
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::ReTriggerArea(IEntity* pEntity, const Vec3 &vPos,bool bIndoor)
{
	TAreaCacheVector	hostedAreasIdx;
	std::vector<int>	updatedID;
	unsigned int aIdx;


	hostedAreasIdx.clear();
	updatedID.clear();
	// check all the rest areas (pPlayer is outside of them)
	unsigned int const nCount = m_vpAreas.size();
	for (aIdx = 0; aIdx < nCount; aIdx++)
	{
		if (m_vpAreas[aIdx]->CalcPointWithin(vPos))
			hostedAreasIdx.push_back( SAreaCacheEntry(aIdx, true) );
	}

	unsigned int const nHostCount = hostedAreasIdx.size();
	for (unsigned int idxIdx = 0; idxIdx < nHostCount; idxIdx++)
	{
		if (std::find(updatedID.begin(), updatedID.end(), m_vpAreas[hostedAreasIdx[idxIdx].nAreaIndex]->GetGroup())!=updatedID.end())
			continue;
		int exclIdx = FindHighestHostedArea( hostedAreasIdx, hostedAreasIdx[idxIdx].nAreaIndex );
		if (exclIdx < 0)
		{
			m_vpAreas[hostedAreasIdx[idxIdx].nAreaIndex]->EnterArea(pEntity);
			continue;
		}
		m_vpAreas[exclIdx]->EnterArea(pEntity);
		updatedID.push_back( m_vpAreas[hostedAreasIdx[idxIdx].nAreaIndex]->GetGroup() );
	}
}

void CAreaManager::ResetAreas()
{
	m_areaCache.clear();
	m_playerEntitiesToUpdate.clear();

	// invalidate cached event
	unsigned int const nCount = m_vpAreas.size();
	for (unsigned int i = 0; i < nCount; i++)
	{
		m_vpAreas[i]->SetCachedEvent(NULL);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::SetAreasDirty()
{
	m_bAreasDirty = true;
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::UpdateDirtyAreas()
{
	if (m_bAreasDirty)
	{
		m_areaGrid.Compile(m_pEntitySystem, m_vpAreas);

		m_bAreasDirty = false;
	}
}

//------------------------------------------------------------------------
void CAreaManager::AddEventListener(IAreaManagerEventListener *pListener)
{
	stl::push_back_unique(m_EventListeners, pListener );
}

//------------------------------------------------------------------------
void CAreaManager::RemoveEventListener(IAreaManagerEventListener *pListener)
{
	stl::find_and_erase(m_EventListeners, pListener);
}

//////////////////////////////////////////////////////////////////////////
void CAreaManager::OnEvent( EEntityEvent event, EntityId TriggerEntityID, IArea *pArea )
{
	if (!m_EventListeners.empty())
	{
		TAreaManagerEventListenerVector::iterator ItEnd = m_EventListeners.end();
		for (TAreaManagerEventListenerVector::iterator It = m_EventListeners.begin(); It != ItEnd; ++It)
		{
			assert (*It);
				(*It)->OnAreaManagerEvent( event, TriggerEntityID, pArea);
		}
	}
}

#include UNIQUE_VIRTUAL_WRAPPER(IAreaManager)
