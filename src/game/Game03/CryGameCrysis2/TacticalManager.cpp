#include "StdAfx.h"
#include "TacticalManager.h"

#include "GameCVars.h"

static const int MAX_TACTICAL_ENTITIES = 256;

//////////////////////////////////////////////////////////////////////////


CTacticalManager::CTacticalManager()
{
	m_story.reserve(MAX_TACTICAL_ENTITIES);
	m_items.reserve(MAX_TACTICAL_ENTITIES);
	m_units.reserve(MAX_TACTICAL_ENTITIES);
}



CTacticalManager::~CTacticalManager()
{
}



void CTacticalManager::AddEntity(const EntityId id, ETacticalEntityType type)
{
	TInterestPoints* pPoints = GetTacticalPointsInternal(type);

	TInterestPoints::const_iterator it = pPoints->begin();
	TInterestPoints::const_iterator end = pPoints->end();
	for(; it!=end; ++it)
	{
		if(it->m_entityId == id)
		{
			return;
		}
	}
	pPoints->push_back(STacticalInterestPoint(id));
}



void CTacticalManager::RemoveEntity(const EntityId id, ETacticalEntityType type)
{
	TInterestPoints* pPoints = GetTacticalPointsInternal(type);
	TInterestPoints::iterator it = pPoints->begin();
	TInterestPoints::const_iterator end = pPoints->end();
	for(; it!=end; ++it)
	{
		if(it->m_entityId == id)
		{
			pPoints->erase(it);
			return;
		}
	}
}



void CTacticalManager::SetEntityScanned(const EntityId id, ETacticalEntityType type)
{
	TInterestPoints* pPoints = GetTacticalPointsInternal(type);
	TInterestPoints::iterator it = pPoints->begin();
	TInterestPoints::const_iterator end = pPoints->end();
	for(; it!=end; ++it)
	{
		if(it->m_entityId == id)
		{
			it->m_scanned = min(eScanned_Max, it->m_scanned + 1);
		}
	}
}



void CTacticalManager::SetEntityScanned(const EntityId id)
{
	SetEntityScanned(id, eTacticalEntity_Story);
	SetEntityScanned(id, eTacticalEntity_Item);
	SetEntityScanned(id, eTacticalEntity_Unit);
}



const CTacticalManager::TInterestPoints& CTacticalManager::GetTacticalPoints(const ETacticalEntityType type) const
{
	switch (type)
	{
	case eTacticalEntity_Story:
		return m_story;
		break;
	case eTacticalEntity_Item:
		return m_items;
		break;
	case eTacticalEntity_Unit:
		return m_units;
		break;
	default:
		return m_story;
		break;
	}
}


CTacticalManager::TInterestPoints* CTacticalManager::GetTacticalPointsInternal(const ETacticalEntityType type)
{
	switch (type)
	{
	case eTacticalEntity_Story:
		return &m_story;
		break;
	case eTacticalEntity_Item:
		return &m_items;
		break;
	case eTacticalEntity_Unit:
		return &m_units;
		break;
	default:
		return &m_story;
		break;
	}
}


//////////////////////////////////////////////////////////////////////////
