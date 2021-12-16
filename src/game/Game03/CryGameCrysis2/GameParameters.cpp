/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------

- 16:09:2008   Benito G.R.

*************************************************************************/

#include "StdAfx.h"
#include "GameParameters.h"
#include "ItemSharedParams.h"
#include "WeaponSharedParams.h"


CGameSharedParametersStorage::~CGameSharedParametersStorage()
{
	m_itemParametersMap.clear();
	m_weaponParametersMap.clear();
}

//================================================================
void CGameSharedParametersStorage::GetMemoryStatistics(ICrySizer *s)
{	
	s->AddObject(m_itemParametersMap);	
	s->AddObject(m_weaponParametersMap);	
	s->AddObject(m_actorParametersMap);	
}

//========================================================
CItemSharedParams *CGameSharedParametersStorage::GetItemSharedParameters(const char *className, bool create)
{
	TSharedParamsMap::iterator it=m_itemParametersMap.find(CONST_TEMP_STRING(className));
	if (it!=m_itemParametersMap.end())
		return static_cast<CItemSharedParams*>(it->second.get());

	if (create)
	{
		CItemSharedParams *params=new CItemSharedParams();
		m_itemParametersMap.insert(TSharedParamsMap::value_type(className, params));

		return params;
	}

	return 0;
}

//========================================================
CWeaponSharedParams *CGameSharedParametersStorage::GetWeaponSharedParameters(const char *className, bool create)
{
	TSharedParamsMap::iterator it=m_weaponParametersMap.find(CONST_TEMP_STRING(className));
	if (it!=m_weaponParametersMap.end())
		return static_cast<CWeaponSharedParams*>(it->second.get());

	if (create)
	{
		CWeaponSharedParams *params = new CWeaponSharedParams();
		m_weaponParametersMap.insert(TSharedParamsMap::value_type(className, params));

		return params;
	}

	return 0;
}

//==========================================================
IGameSharedParameters * CGameSharedParametersStorage::GetActorSharedParameters(const char *className, bool create)
{
	TSharedParamsMap::iterator it=m_actorParametersMap.find(CONST_TEMP_STRING(className));
	if (it!=m_actorParametersMap.end())
		return it->second;

	if (create)
	{
		IGameSharedParameters *pNewParams = m_actorParamsRegistry.CreateActorParameters(className);
		if(!pNewParams)
		{
			GameWarning("Requested shared parameter type '%s' not found!", className);
			return 0;
		}
		m_actorParametersMap.insert(TSharedParamsMap::value_type(className, pNewParams));

		return pNewParams;
	}

	return 0;
}

void CGameSharedParametersStorage::GetDetailedMemoryStatics(TSharedParamsMap* paramsMap, const char* typeName)
{
	const float kbInvert = 1.0f/1024.0f;

	int totalSize = 0;

	float white[4] = {1.f, 1.f, 1.f, 1.f};
	float purple[4] = {1.f, 0.f, 1.f, 1.f};

	float posY = 50.f;
	float posX = 50.f;

	int num = 0;
	
	for (TSharedParamsMap::iterator iter = paramsMap->begin(); iter != paramsMap->end(); ++iter)
	{
		ICrySizer *pSizer = gEnv->pSystem->CreateSizer();

		iter->second->GetMemoryUsage(pSizer);

		int paramSize = pSizer->GetTotalSize();
		totalSize += paramSize;

		gEnv->pRenderer->Draw2dLabel(posX, posY, 1.5f, purple, false, "%s Params: %s. Mem: %.3f Kb", typeName, iter->first.c_str(), paramSize*kbInvert);
		posY += 15.f;

		if(posY > 600.f)
		{
			posX += 500.f;
			posY = 50.f;
		}

		num++;

		pSizer->Release();
	}

	gEnv->pRenderer->Draw2dLabel(50.0f, 30.0f, 1.5f, white, false, "%s Params Total: %d. Mem: %.2f Kb.", typeName, num, totalSize*kbInvert);
}