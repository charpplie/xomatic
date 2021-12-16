////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   AreaManager.h
//  Version:     v1.00
//  Created:     24/02/2002 by Kirill Bulatsev.
//  Compilers:   Visual Studio.NET
//  Description: 2D area class. Area is in XY plane. Area has entities attached to it.
//	Area has fade width (m_Proximity) - distance from border to inside at wich fade coefficient
//	changes linearly from 0(on the border) to 1(inside distance to border more than m_Proximity). 
//	
//	Description: 2D areas manager. Checks player for entering/leaving areas. Updates fade 
//	coefficient for areas player is in

// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AreaManager_h__
#define __AreaManager_h__
#pragma once

#include "AreaGrid.h"

//#include "SoundAreaManager.h"
class CEntitySystem;
class CArea;
struct IVisArea;
struct IAreaManagerEventListener;

typedef std::vector<IAreaManagerEventListener*>	TAreaManagerEventListenerVector;

//Areas manager
class CAreaManager : public IAreaManager //: CSoundAreaManager
{

	struct SAreaCacheEntry
	{
	public:
		SAreaCacheEntry(int nAreaIdx, bool bInsd)
		{
			nAreaIndex = nAreaIdx;
			bInside = bInsd;
		};

		int nAreaIndex;
		bool bInside;
	};

	struct SAreasCache
	{
		public:
			int GetCacheIndex(const int nAreaIdx)
			{
				int nCacheIndex = 0;

				const std::vector<SAreaCacheEntry>::const_iterator cEnd = Areas.end();
				for(std::vector<SAreaCacheEntry>::const_iterator iter = Areas.begin(); iter != cEnd; ++iter)
				{
					if (nAreaIdx == (*iter).nAreaIndex)
						return nCacheIndex;

					++nCacheIndex;
				}
				return -1;
			}

		std::vector<SAreaCacheEntry> Areas;
		Vec3 vLastUpdatePos;

	};

	typedef std::vector<SAreaCacheEntry>	TAreaCacheVector;
	typedef std::map<int,SAreasCache>			TAreaCacheMap;




public:
	CAreaManager( CEntitySystem *pEntitySystem );
	~CAreaManager(void);

	//IAreaManager
	VIRTUAL int						GetAreaAmount() const { return m_vpAreas.size(); }
	VIRTUAL const IArea*	GetArea(int areaIndex) const;
	VIRTUAL void					SetAreasDirty();
	//~IAreaManager

	// Makes a new area.
	CArea* CreateArea();

	CEntitySystem* GetEntitySystem() const { return m_pEntitySystem; };

	// Mark player to be checked in next update.
	void  MarkPlayerForUpdate( EntityId id );
	
	// Called every frame.
	void  Update();

	//void	SoundUpdatePlayer( const Vec3 &vPos,IEntity *pPlayer );
	void	UpdateTailEntity( const Vec3 &vPos,IEntity *pTailEntity );

	void	ProceedExclusiveSoundUpdate( IEntity const* const pPlayer, unsigned int const nAreaIndex, float const fFade, CArea const* const pArea );
	int		FindHighestHostedArea( TAreaCacheVector const& hostedAreas, unsigned int const nAreaIndex )const;
	int		FindLowestHostedArea( TAreaCacheVector const& hostedAreas, unsigned int const nAreaIndex )const;

	// disabled for now, lets see if we need this later
	//void CheckSoundVisAreas();

	void	ReTriggerArea(IEntity* pEntity,const Vec3 &vPos,bool bIndoor);

	bool	ProceedExclusiveEnter( IEntity const* const pPlayer, unsigned int const nAreaIndex );
	bool	ProceedExclusiveLeave( IEntity const* const pPlayer, unsigned int const nAreaIndex );
	void	ProceedExclusiveUpdate( IEntity const* const pPlayer, unsigned int const nAreaIndex );
//	int		FindHighestHostedArea( IEntity *pPlayer, unsigned int nAreaIndex );

	void	ExitAllAreas( IEntity const* const pPlayer );
	CArea*	GetArea( const Vec3& point );
	VIRTUAL void DrawLinkedAreas(EntityId linkedId) const;
	int GetLinkedAreas(EntityId linkedId, int areaId, std::vector<CArea *> &areas) const;
//	CArea*	GetArea(const int nBuilding, const int nSectorId, const EntityId entityID);
	
//	void	DeleteEntity();

	void	DrawAreas(const ISystem * const pSystem);
	void DrawGrid();
	unsigned MemStat();
	void ResetAreas();

	void UpdatePlayer( const Vec3 &vPos,IEntity *pPlayer );
	VIRTUAL bool	QueryAreas( Vec3 vPos, SAreaManagerResult *pResults, int nMaxResults);

	// Register EventListener to the AreaManager.
	VIRTUAL void AddEventListener( IAreaManagerEventListener *pListener );
	VIRTUAL void RemoveEventListener( IAreaManagerEventListener *pListener );
	
	//! Fires event for all listeners to this sound.
	void  OnEvent( EEntityEvent event, EntityId TriggerEntityID, IArea *pArea );


protected:
	friend class CArea;
	void Unregister( CArea *pArea );

	// list of all registered Areas
	std::vector<CArea*>	m_vpAreas;

	std::vector<EntityId> m_playerEntitiesToUpdate;

	CEntitySystem *m_pEntitySystem;
	int m_nCurStep;
	int m_nCurSoundStep;
	int m_nCurTailStep;
	bool m_bAreasDirty;
	CAreaGrid m_areaGrid;

private:

	IVisArea *m_pPrevArea,*m_pCurrArea;

	TAreaManagerEventListenerVector m_EventListeners;

	//////////////////////////////////////////////////////////////////////////
	SAreasCache* GetAreaCache( int nEntityId )
	{
		SAreasCache *pAreaCache = NULL;
		TAreaCacheMap::iterator it = m_areaCache.find(nEntityId);
		if (it != m_areaCache.end())
		{
			pAreaCache = &(it->second);
		}
		return pAreaCache;
	}
	//////////////////////////////////////////////////////////////////////////
	SAreasCache* MakeAreaCache( int nEntityId )
	{
		SAreasCache *pAreaCache = &m_areaCache[nEntityId];
		pAreaCache->vLastUpdatePos = Vec3(0);
		return pAreaCache;
	}
	//////////////////////////////////////////////////////////////////////////
	void DeleteAreaCache( int nEntityId )
	{
		m_areaCache.erase( nEntityId );
	}

	void UpdateDirtyAreas();

	void ProcessArea(CArea* const __restrict pArea, unsigned int const nAreaIdx, int &nCacheIdx, SAreasCache* const pAreaCache, const Vec3& point, IEntity const* const pPlayer);


	// Area cache per entity id.
	TAreaCacheMap m_areaCache;
};

#endif //__AreaManager_h__
