#ifndef SkeletonHierarchy_h
#define SkeletonHierarchy_h

#include <ICryAnimation.h>

namespace Skeleton {
	
class CHierarchy :
	public _reference_target_t
{
public:
	struct SNode
	{
		string name;
		QuatT pose;

		int32 parent;

/* TODO: Implement
		uint32 childrenIndex;
		uint32 childrenCount;
*/
	};

public:
	CHierarchy();
	~CHierarchy();

public:
	uint32 AddNode(const char* name, const QuatT& pose, int32 parent = -1);
	uint32 GetNodeCount() const { return uint32(m_nodes.size()); }
	SNode* GetNode(uint32 index) { return &m_nodes[index]; }
	const SNode* GetNode(uint32 index) const { return &m_nodes[index]; }
	int32 FindNodeIndexByName(const char* name) const;
	const SNode* FindNode(const char* name) const;
	void ClearNodes() { m_nodes.clear(); }

	void CreateFrom(ISkeletonPose& pose);
	void ValidateReferences();

	void AbsoluteToRelative(const QuatT* pSource, QuatT* pDestination);

	bool SerializeTo(XmlNodeRef& node);

private:
	std::vector<SNode> m_nodes;
};

} // namespace Skeleton

#endif // SkeletonHierarchy_h
