/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------

Description: This file contains a game parameters registry/repository class, where all kind of
             game data, that might be shared between instances of the same class, is stored.
						 Things like weapon firemode / zoom parameters, item actions, ammo parameters...

						 Also it provides an interface to be implemented by data structures that might be
						 shared between instances. The idea is to force an implementation that uses smart pointers
						 to handle this data, and to have a common way to retrieve such data.

-------------------------------------------------------------------------
History:
- 16:09:2008   Benito G.R.

*************************************************************************/
#ifndef __GAME_PARAMETERS_H__
#define __GAME_PARAMETERS_H__

#include "ActorParametersRegistry.h"

//Interface that should be implemented by any data structure that could
//be potentially shared between instances of the same class.
struct IGameSharedParameters
{
	//To be compatible with smart pointers
	virtual void AddRef() const = 0;
	virtual uint32 GetRefCount() const = 0;
	virtual void Release() const = 0;

	//Aux functions to validate/invalidate pointed data
	virtual void SetValid(bool valid) = 0;
	virtual bool Valid() const = 0;

	virtual void GetMemoryUsage(ICrySizer* s) const= 0;

	virtual const char* GetDataType() const = 0;
	
};

class CItemSharedParams;
class CWeaponSharedParams;
class CActorSharedParams;

//All shared game data/parameters go here
//Data is stored in different maps per game object types (items, ammo,...)
//and the classified per class.
class CGameSharedParametersStorage
{
	typedef std::map<string, _smart_ptr<IGameSharedParameters> > TSharedParamsMap;

public:
	CGameSharedParametersStorage() {};
	virtual ~CGameSharedParametersStorage();

	//Item parameters
	CItemSharedParams* GetItemSharedParameters(const char *className, bool create);
	void ResetItemParameters(){ m_itemParametersMap.clear(); };

	//Weapon parameters
	CWeaponSharedParams *GetWeaponSharedParameters(const char *className, bool create);
	void ResetWeaponParameters() { m_weaponParametersMap.clear(); };

	//Actor parameters
	IGameSharedParameters *GetActorSharedParameters(const char *className, bool create);
	void ResetActorParameters() { m_actorParametersMap.clear(); };

	void GetMemoryStatistics(ICrySizer *s);

	void GetDetailedMemoryStatics(TSharedParamsMap* paramsMap, const char* typeName);
	void GetDetailedItemParamMemoryStatistics() { GetDetailedMemoryStatics(&m_itemParametersMap, "Item"); };
	void GetDetailedWeaponParamMemoryStatistics() { GetDetailedMemoryStatics(&m_weaponParametersMap, "Weapon"); };

private:

	TSharedParamsMap m_itemParametersMap;
	TSharedParamsMap m_weaponParametersMap;
	TSharedParamsMap m_actorParametersMap;

	CActorParametersRegistry m_actorParamsRegistry;

};

#endif