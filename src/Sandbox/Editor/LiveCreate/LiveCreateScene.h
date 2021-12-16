#ifndef LiveCreateScene_h
#define LiveCreateScene_h

#include "../../SDKs/LiveCreate/LiveCreate.h"

#include "LiveCreateActor.h"

class CLiveCreateScene;

class ILiveCreateSceneListener
{
public:
	virtual void OnCreateActor(const CLiveCreateScene& scene, const CLiveCreateActor& actor) = 0;
};

class CLiveCreateScene :
	public ILCScene
{
public:
	CLiveCreateScene();
	~CLiveCreateScene();

public:
	uint32 GetActorCount() const { return (uint32)m_actors.size(); }
	CLiveCreateActor* GetActor(uint32 index) { return m_actors[index]; }
	CLiveCreateActor* GetActor(const char* name);
	const CLiveCreateActor* GetActor(uint32 index) const { return m_actors[index]; }
	void DeleteActors();

	void AddListener(ILiveCreateSceneListener* pListener);
	void RemoveListener(ILiveCreateSceneListener* pListener);

	void SetEntity(IEntity* pEntity) { m_pEntity = pEntity; }
	IEntity* GetEntity() { return m_pEntity; }

	void Reset();

	void Update();

	// ILCScene
public:
	ILCActor* CreateActor(LCString name, const LCSet* pSet);

private:
	std::vector<CLiveCreateActor*> m_actors;
	std::vector<ILiveCreateSceneListener*> m_listeners;

	IEntity* m_pEntity;
};

#endif // LiveCreateScene_h
