#ifndef LiveCreateActor_h
#define LiveCreateActor_h

#include "../../SDKs/LiveCreate/LiveCreate.h"

#include "../Animation/SkeletonMapperGraph.h"

//

struct IEntity;
namespace Skeleton { class CMapperGraph; }

//

class CLiveCreateActor :
	public ILCActor
{
public:
	static CLiveCreateActor* Create(const char* name, const LCSet* pSet);

private:
	static std::map<IEntity*, CLiveCreateActor*> s_entityToActor;

public:
	struct SNode
	{
		string name;
		QuatT pose;
	};

private:
	CLiveCreateActor();
	~CLiveCreateActor();

public:
	void Release() { delete this; }

	const char* GetName() const { return m_name; }

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

	ISkeletonPose* HaveSkeletonPose();

	void CreateLocations(Skeleton::CMapperGraph& mapperGraph);
	void CreateNodes(Skeleton::CMapperGraph& mapperGraph);

	void UpdateLocations(Skeleton::CMapper& mapper);
	bool UpdateSkeletonPose(ISkeletonPose& skeletonPose, const QuatT& origin, Skeleton::CMapper& mapper);
	bool UpdateEntity(IEntity& entity, const QuatT& origin, Skeleton::CMapper& mapper);

	void DrawNodes(const QuatT& origin);
	void DrawMapper(const QuatT& origin, QuatT* pLocations);

	// ILCActor
public:
	LC_VIRTUAL void SetPosition(LCString name, LCu32 time, const LCVec3f32& position);
	LC_VIRTUAL void SetOrientation(LCString name, LCu32 time, const LCQuat32& orientation);
	LC_VIRTUAL void SetScale(LCString name, LCu32 time, const LCVec3f32& scale);

public:
	IVariablePtr m_entityUpdate;
	IVariablePtr m_entityShowHierarchy;
	IVariablePtr m_entityScale;

	IVariablePtr m_locationsShow;
	IVariablePtr m_locationsShowName;
	IVariablePtr m_locationsShowHierarchy;

private:
	string m_name;
	const LCSet* m_pSet;

	std::vector<SNode> m_nodes;

	IEntity* m_pEntity;

	_smart_ptr<Skeleton::CMapperGraph> m_skeletonRemapperGraph;
};

#endif // LiveCreateActor_h
