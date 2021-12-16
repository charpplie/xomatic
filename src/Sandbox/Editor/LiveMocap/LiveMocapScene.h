#ifndef LiveMocapScene_h
#define LiveMocapScene_h

#include "../../SDKs/LiveMocap/LiveMocap.h"

#include "LiveMocapActor.h"

class CLiveMocapScene;

class ILiveMocapSceneListener
{
public:
	virtual void OnCreateActor(const CLiveMocapScene& scene, const CLiveMocapActor& actor) = 0;
};

class CLiveMocapScene :
	public ILMScene
{
public:
	CLiveMocapScene();
	~CLiveMocapScene();

public:
	uint32 GetActorCount() const { return (uint32)m_actors.size(); }
	CLiveMocapActor* GetActor(uint32 index) { return m_actors[index]; }
	CLiveMocapActor* GetActor(const char* name);
	const CLiveMocapActor* GetActor(uint32 index) const { return m_actors[index]; }
	void DeleteActors();

	void AddListener(ILiveMocapSceneListener* pListener);
	void RemoveListener(ILiveMocapSceneListener* pListener);

	void SetEntity(IEntity* pEntity) { m_pEntity = pEntity; }
	IEntity* GetEntity() { return m_pEntity; }

	void Reset();

	void Update();

	// ILCScene
public:
	ILMActor* CreateActor(LMString name, const LMSet* pSet);

private:
	std::vector<CLiveMocapActor*> m_actors;
	std::vector<ILiveMocapSceneListener*> m_listeners;

	IEntity* m_pEntity;
};

#endif // LiveMocapScene_h
