#ifndef LiveMocapActor_h
#define LiveMocapActor_h

#include "../../SDKs/LiveMocap/LiveMocap.h"

#include "../Animation/SkeletonMapper.h"

//

struct IEntity;
namespace Skeleton { class CMapperGraph; }

//

class CLiveMocapActor :
	public ILMActor
{
public:
	static CLiveMocapActor* Create(const char* name, const LMSet* pSet);

private:
	static std::map<IEntity*, CLiveMocapActor*> s_entityToActor;

public:
	struct SNode
	{
		string name;
		QuatT pose;
	};

private:
	CLiveMocapActor();
	~CLiveMocapActor();

public:
	void Release() { delete this; }

	const char* GetName() const { return m_name; }
	const char* GetEntityName() const;

	uint32 GetNodeCount() const { return (uint32)m_nodes.size(); }
	const SNode* GetNode(uint32 index) const { return &m_nodes[index]; }

	void SetEntity(IEntity* pEntity);
	IEntity* GetEntity() { return m_pEntity; }

	Skeleton::CMapperGraph* CreateSkeletonMapperGraph();
	bool SetSkeletonMapperGraph(Skeleton::CMapperGraph* pMapperGraph);
	Skeleton::CMapperGraph* GetSkeletonMapperGraph() { return m_skeletonRemapperGraph; }

	void Reset();

	void Update(QuatT& origin);

private:
	SNode* GetNode(const char* name, bool bCreate = false);

	void UnsetEntity();

	IDefaultSkeleton* HaveSkeletonPose();

	void CreateLocations(Skeleton::CMapperGraph& mapperGraph);
	void CreateNodes(Skeleton::CMapperGraph& mapperGraph);

	void UpdateLocations(Skeleton::CMapper& mapper);
	bool UpdateSkeletonPose(ISkeletonPose& skeletonPose, IDefaultSkeleton& rIDefaultSkeleton, const QuatT& origin, Skeleton::CMapper& mapper);
	bool UpdateEntity(IEntity& entity, const QuatT& origin, Skeleton::CMapper& mapper);

	void DrawNodes(const QuatT& origin);
	void DrawMapper(const QuatT& origin, QuatT* pLocations);

	// ILMActor
public:
	LM_VIRTUAL void SetPosition(LMString name, LMu32 time, const LMVec3f32& position);
	LM_VIRTUAL void SetOrientation(LMString name, LMu32 time, const LMQuat32& orientation);
	LM_VIRTUAL void SetScale(LMString name, LMu32 time, const LMVec3f32& scale);

public:
	IVariablePtr m_entityUpdate;
	IVariablePtr m_entityShowHierarchy;
	IVariablePtr m_entityScale;

	IVariablePtr m_locationsShow;
	IVariablePtr m_locationsShowName;
	IVariablePtr m_locationsShowHierarchy;
	IVariablePtr m_locationsFreeze;

private:
	string m_name;
	const LMSet* m_pSet;

	std::vector<SNode> m_nodes;

	IEntity* m_pEntity;
	QuatT m_entityLocBackup;


	_smart_ptr<Skeleton::CMapperGraph> m_skeletonRemapperGraph;
};

#endif // LiveMocapActor_h
