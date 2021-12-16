//---------------------------------------
// Actor parameters registry, etc
//
// 15-09-2008 Benito G.R.
// ----------------------------------------

#include "StdAfx.h"
#include "ActorParametersRegistry.h"

#include "BasicActorParameters.h"

template <typename T, typename R> R *CreateIt() { return new T(); };

CActorParametersRegistry::CActorParametersRegistry()
{
	Init();
}

CActorParametersRegistry::~CActorParametersRegistry()
{
	Clear(true);
}

//--------------------------------------------
void CActorParametersRegistry::Init()
{
	//Register data types
	RegisterActorParameters("BasicActorParams", &CreateIt<SBasicActorParams, IGameSharedParameters>);
	RegisterActorParameters("TrooperParams", &CreateIt<SBasicActorParams, IGameSharedParameters>);
	RegisterActorParameters("ScoutParams", &CreateIt<SBasicActorParams, IGameSharedParameters>);
	RegisterActorParameters("PingerParams", &CreateIt<SBasicActorParams, IGameSharedParameters>);
	RegisterActorParameters("HunterParams", &CreateIt<SBasicActorParams, IGameSharedParameters>);
	RegisterActorParameters("AlienParams", &CreateIt<SBasicActorParams, IGameSharedParameters>);
}

//------------------------------------------
void CActorParametersRegistry::Clear(bool destroying /* = false */)
{
	if(destroying)
		m_registry.clear();
}

//------------------------------------------------------------------------
IGameSharedParameters *CActorParametersRegistry::CreateActorParameters(const char *name)
{
	TActorParamsRegistry::iterator it = m_registry.find(CONST_TEMP_STRING(name));
	if (it != m_registry.end())
		return it->second();
	return NULL;
}

//------------------------------------------------------------------------
void CActorParametersRegistry::RegisterActorParameters(const char *name, IGameSharedParameters *(*CreateProc)())
{
	m_registry.insert(TActorParamsRegistry::value_type(name, CreateProc));
}

//=====================================================
void CActorParametersRegistry::GetMemoryStatistics(ICrySizer *s)
{

}
