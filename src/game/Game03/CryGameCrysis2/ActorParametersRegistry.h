//---------------------------------------
// Actor parameters registry, etc
//
// 15-09-2008 Benito G.R.
// ----------------------------------------

#ifndef _ACTOR_PARAMETERS_REGISTRY_H_
#define _ACTOR_PARAMETERS_REGISTRY_H_

struct IItemParamsNode;
struct IGameSharedParameters;
	
//Contains also a registry with data types supported
class CActorParametersRegistry
{

public:
	CActorParametersRegistry();
	virtual ~CActorParametersRegistry();

	IGameSharedParameters *CreateActorParameters(const char *name);
	virtual void GetMemoryStatistics(ICrySizer *s);

private:
	typedef std::map<string, IGameSharedParameters*>			TActorParamsMap;
	typedef std::map<string, IGameSharedParameters		*(*)()>		TActorParamsRegistry;

	void Init();
	void Clear(bool destroying = false);

	void RegisterActorParameters(const char *name, IGameSharedParameters *(*)());

	TActorParamsRegistry m_registry;
};

#endif