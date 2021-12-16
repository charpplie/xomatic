/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Interactive objects are registered here with required params

-------------------------------------------------------------------------
History:
- 10:12:2009: Created by Benito G.R.

*************************************************************************/

#pragma once

#ifndef __INTERACTIVEOBJECT_REGISTRY_H__
#define __INTERACTIVEOBJECT_REGISTRY_H__

#include "../ItemString.h"

struct SInteractionParams
{
	SInteractionParams()
		: objectAnimation("")
		, userAnimation("")
		, helperName("player_align")
		, speed (1.0f)
		, correctionDelay(0.4f)
	{

	}

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(objectAnimation);
		pSizer->AddObject(userAnimation);
		pSizer->AddObject(helperName);
	}
	ItemString objectAnimation;
	ItemString userAnimation; 
	ItemString helperName;
	float speed;
	float correctionDelay;
};

class CInteractiveObjectRegistry
{
private:

	typedef std::map<ItemString, SInteractionParams> TInteractionParamsMap;
	typedef std::map<EntityId, ItemString> TEntityToParamsMap;
public:

	void Init();
	void Reload();

	void RegisterInteractiveObject(EntityId entityId, const char* interactionName);
	void UnregisterInteractiveObject(EntityId entityId);

	const SInteractionParams* GetInteractionParamsForEntity(EntityId entityId) const;
	const SInteractionParams* GetInteractionParamsByName(const char* interactionName) const;

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(m_entityToParams);
		pSizer->AddObject(m_interactiveObjectParams);
	}
private:

	void ReadInteractionsFromXml(const char* fileName);

	TEntityToParamsMap	m_entityToParams;
	TInteractionParamsMap m_interactiveObjectParams;
};

#endif