#ifndef ASSETTREE_H
#define ASSETTREE_H

#include "SizeInfo.h"
#include "ReplayLogReader.h"
#include "StringTable.h"

#define POOL_CONTEXTTREENODES 1

class ContextTreeNode
{
	friend class ContextTree;
	friend class ContextTreeReplayBuilder;

public:
	static const char* InternString(const char* str);

#if POOL_CONTEXTTREENODES 
public:
	void* operator new (size_t sz);
	void operator delete (void* p, size_t sz);
#endif

public:
	ContextTreeNode(const char* name, MemStatContextTypes::Type type);
	ContextTreeNode(const ContextTreeNode& other)
		: m_name(other.m_name)
		, m_type(other.m_type)
		, m_allocStreamOffsets(other.m_allocStreamOffsets)
		, m_count(other.m_count)
		, m_size(other.m_size)
		, m_parent(NULL)
		, m_nextSibling(NULL)
		, m_children(NULL)
	{
	}

	void Rename(const char* name) { m_name = InternString(name); }

	const char* GetName() const { return m_name; }

	MemStatContextTypes::Type GetType() const { return m_type; }

	SizeInfoGroups& GetSize() { return m_size; }
	const SizeInfoGroups& GetSize() const { return m_size; }

	const ContextTreeNode* GetParent() const { return m_parent; }
	const ContextTreeNode* GetNextSibling() const { return m_nextSibling; }
	const ContextTreeNode* GetChildren() const { return m_children; }

	ContextTreeNode* GetParent() { return m_parent; }
	ContextTreeNode* GetChildren() { return m_children; }
	ContextTreeNode* GetNextSibling() { return m_nextSibling; }

	void AddChild(ContextTreeNode* node)
	{
		assert(node->m_parent == NULL);
		assert(node->m_nextSibling == NULL);

		node->m_parent = this;
		node->m_nextSibling = m_children;
		m_children = node;
	}

	void RemoveChild(ContextTreeNode* node)
	{
		ContextTreeNode** child = &m_children;
		for (; *child; child = &(*child)->m_nextSibling)
		{
			if ((*child) == node)
			{
				(*child) = node->m_nextSibling;
				node->m_parent = NULL;
				node->m_nextSibling = NULL;
				return;
			}
		}
	}

	void AddAllocStreamOffset(u64 offset, TThreadId threadId)
	{
		m_allocStreamOffsets.push_back(ContextStreamOffsetSpan(offset, threadId));
	}

	void AddAllocStreamOffset(const ContextStreamOffsetSpan& span)
	{
		m_allocStreamOffsets.push_back(span);
	}

	void IncrementInstanceCount() { ++ m_count; }
	void SetInstanceCount(ptrdiff_t count) { m_count = count; }

	const std::vector<ContextStreamOffsetSpan>& GetAllocStreamOffsets() const { return m_allocStreamOffsets; }
	std::vector<ContextStreamOffsetSpan>& GetAllocStreamOffsets() { return m_allocStreamOffsets; }

	ptrdiff_t GetInstanceCount() const { return m_count; }

	size_t CountChildren() const
	{
		size_t c = 0;
		for (const ContextTreeNode* ch = m_children; ch; ch = ch->m_nextSibling, ++ c)
			;
		return c;
	}

private:
	static void InitStringPool();
	static void ReleaseStringPool();

private:
	ContextTreeNode& operator = (const ContextTreeNode&);
	~ContextTreeNode() {}

private:
	static StringTable ms_stringTable;

private:
	const char* m_name;
	MemStatContextTypes::Type m_type;

	std::vector<ContextStreamOffsetSpan> m_allocStreamOffsets;

	ptrdiff_t m_count;
	SizeInfoGroups m_size;

	ContextTreeNode* m_parent;
	ContextTreeNode* m_nextSibling;
	ContextTreeNode* m_children;
};

class ContextTreeReplayBuilder : public IReplayListener
{
public:
	ContextTreeReplayBuilder(const SharedPtr<ContextTree>& tree, ContextTreeNode* root, u64 allocEvBegin = 0, u64 allocEvEnd = ~0ULL);

	void ReplayBegin();
	void Replay(ReplayRange range);
	void ReplayEnd(u64 position);

	const SharedPtr<ContextTree>& GetBuiltTree() const { return m_tree; }

private:
	struct ChildGroupKey
	{
		ChildGroupKey(
			ContextTreeNode* parent,
			const char* name,
			MemStatContextTypes::Type type)
			: parent(parent)
			, name(name)
			, type(type)
		{
		}

		ContextTreeNode* parent;
		const char* name;
		MemStatContextTypes::Type type;

		friend bool operator < (const ChildGroupKey& a, const ChildGroupKey& b)
		{
			if (a.parent != b.parent) return a.parent < b.parent;
			if (a.name != b.name) return a.name < b.name;
			return a.type < b.type;
		}
	};
	
	typedef std::map<TAddress, std::pair<ContextTreeNode*, SizeInfo> > ActivePointersMap;
	typedef std::map<ChildGroupKey, ContextTreeNode*> ChildGroupMap;

	struct ThreadContext
	{
		explicit ThreadContext(ContextTreeNode* top)
			: top(top)
			, topNeedsAllocOffset(true)
		{}
		ThreadContext()
			: top(NULL)
			, topNeedsAllocOffset(true)
		{}
		ContextTreeNode* top;
		bool topNeedsAllocOffset;
	};

private:
	void PushContext(u32 threadId, u32 type, const char* name, u32 flags, u64 streamPosition);
	void PopContext(u32 threadId);
	void ApplyFree(TAddress ptr, TThreadId threadId, u64 streamPosition, ptrdiff_t sizeGlobal);

private:
	SharedPtr<ContextTree> m_tree;
	ContextTreeNode* m_root;

	u64 m_begin;
	u64 m_end;

	u64 m_current;

	std::map<u32, ThreadContext> threads;
	ActivePointersMap activePointers;
	ChildGroupMap childGroups;
	MemAddressProfile m_addressProfile;
};

class IContextTreeFilter
{
public:
	virtual ~IContextTreeFilter() {}
	virtual bool operator () (const ContextTreeNode& node) const = 0;
};

class ContextTree
{
public:
	static SharedPtr<ContextTreeReplayBuilder> CreateBuilder();

	static SharedPtr<ContextTree> Deserialise(IDeserialiser& ser);

	static SharedPtr<ContextTree> FilterLeaves(const ContextTree& sourceTree, const IContextTreeFilter& filter);
	static SharedPtr<ContextTree> GatherSubTrees(const ContextTree& sourceTree, const char* rootName, const IContextTreeFilter& filter);
	static SharedPtr<ContextTree> GatherSubTreesBottomUp(const ContextTree& sourceTree, const char* rootName, const IContextTreeFilter& filter);

	template <typename FolderT>
	static SharedPtr<ContextTree> FoldLeaves(const ContextTree& sourceTree, FolderT folder);

	template <typename GroupT>
	static SharedPtr<ContextTree> GroupChildren(const ContextTree& sourceTree, GroupT grouper);
	static SharedPtr<ContextTree> Filter(const ContextTree& sourceTree, const IContextTreeFilter& filter);

	static SharedPtr<ContextTree> MergeChildren(const ContextTree& a, const ContextTree& b);

public:
	ContextTree();
	~ContextTree();

	void Serialise(ISerialiser& ser);

	const ContextTreeNode* GetRoot() const { return m_root; }

	ContextTreeNode* BeginEdit();
	void EndEdit();

private:
	enum
	{
		Ser_Version = 3
	};

private:
	static void RefreshTreeSums(ContextTreeNode* node);

	static void GatherSubTreesBottomUpImpl(ContextTreeNode* root, const ContextTreeNode* sr, std::set<const ContextTreeNode*>& ignoreSet, const IContextTreeFilter& filter);
	static ContextTreeNode* FilterLeavesImpl(const ContextTreeNode* node, const IContextTreeFilter& filter);
	static void FilterImpl(const ContextTreeNode& source, ContextTreeNode& dest, const IContextTreeFilter& filter);

	template <typename FolderT>
	static ContextTreeNode* FoldLeavesImpl(const ContextTreeNode& source, FolderT folder);

	template <typename GroupT>
	static void GroupChildrenImpl(
		const ContextTreeNode& sourceNode, ContextTreeNode& destNode, GroupT grouper);

private:
	bool DeserialiseImpl(IDeserialiser& ser);

private:
	ContextTreeNode* m_root;
};

static ContextTreeNode* CloneTree(const ContextTreeNode* source, const std::set<const ContextTreeNode*>& ignoreSet = std::set<const ContextTreeNode*>())
{
	ContextTreeNode* clone = new ContextTreeNode(*source);

	for (const ContextTreeNode* child = source->GetChildren(); child; child = child->GetNextSibling())
	{
		if (ignoreSet.find(child) == ignoreSet.end())
			clone->AddChild(CloneTree(child, ignoreSet));
	}

	return clone;
}
typedef SharedPtr<ContextTree> ContextTreePtr;

template <typename FolderT>
SharedPtr<ContextTree> ContextTree::FoldLeaves(const ContextTree& sourceTree, FolderT folder)
{
	SharedPtr<ContextTree> newTree = new ContextTree();

	if (sourceTree.m_root)
	{
		newTree->m_root = FoldLeavesImpl(*sourceTree.m_root, folder);

		if (newTree->m_root)
			RefreshTreeSums(newTree->m_root);
	}

	return newTree;
}

template <typename GroupT>
SharedPtr<ContextTree> ContextTree::GroupChildren(const ContextTree& sourceTree, GroupT grouper)
{
	SharedPtr<ContextTree> newTree = new ContextTree();

	const ContextTreeNode* sourceRoot = sourceTree.GetRoot();
	if (sourceRoot)
	{
		newTree->m_root = new ContextTreeNode(*sourceRoot);
		GroupChildrenImpl(*sourceTree.m_root, *newTree->m_root, grouper);
		RefreshTreeSums(newTree->m_root);
	}

	return newTree;
}

template <typename FolderT>
ContextTreeNode* ContextTree::FoldLeavesImpl(const ContextTreeNode& source, FolderT folder)
{
	if (folder(source))
	{
		ContextTreeNode* sourceClone = new ContextTreeNode(source.GetName(), source.GetType());
		sourceClone->SetInstanceCount(source.GetInstanceCount());

		std::vector<const ContextTreeNode*> nextList;
		nextList.reserve(64);

		nextList.push_back(&source);

		while (nextList.empty() == false)
		{
			const ContextTreeNode* n = nextList.back();
			nextList.pop_back();

			for (const ContextTreeNode* child = n->GetChildren(); child; child = child->GetNextSibling())
				nextList.push_back(child);

			folder(*sourceClone, *n);
		}

		return sourceClone;
	}
	else
	{
		ContextTreeNode* parent = new ContextTreeNode(source);

		for (const ContextTreeNode* child = source.GetChildren(); child; child = child->GetNextSibling())
		{
			parent->AddChild(FoldLeavesImpl(*child, folder));
		}

		return parent;
	}
}

template <typename GroupT>
void ContextTree::GroupChildrenImpl(
	const ContextTreeNode& sourceNode, ContextTreeNode& destNode, GroupT grouper)
{
	typedef typename GroupT::KeyType KeyT;

	for (const ContextTreeNode* srcChild = sourceNode.GetChildren(); srcChild; srcChild = srcChild->GetNextSibling())
	{
		const KeyT& key = grouper(*srcChild);

		ContextTreeNode* dstChild = destNode.GetChildren();
		for (; dstChild; dstChild = dstChild->GetNextSibling())
		{
			if (grouper(*dstChild) == key)
				break;
		}

		if (!dstChild)
		{
			dstChild = new ContextTreeNode(srcChild->GetName(), srcChild->GetType());
			destNode.AddChild(dstChild);
		}

		if (!dstChild->GetChildren() && !srcChild->GetChildren())
		{
			dstChild->GetSize() += srcChild->GetSize();
			dstChild->GetAllocStreamOffsets().insert(
				dstChild->GetAllocStreamOffsets().end(),
				srcChild->GetAllocStreamOffsets().begin(), srcChild->GetAllocStreamOffsets().end());
			dstChild->SetInstanceCount(dstChild->GetInstanceCount() + srcChild->GetInstanceCount());
		}
		else if (dstChild->GetChildren() && !srcChild->GetChildren())
		{
			ContextTreeNode* dstChildMisc = dstChild->GetChildren();
			for (; dstChildMisc; dstChildMisc = dstChildMisc->GetNextSibling())
			{
				if (strcmp(dstChildMisc->GetName(), "Misc") == 0)
					break;
			}

			if (!dstChildMisc)
			{
				dstChildMisc = new ContextTreeNode(*srcChild);
				dstChildMisc->Rename("Misc");
				dstChild->AddChild(dstChildMisc);
			}
			else
			{
				dstChildMisc->GetSize() += srcChild->GetSize();
				dstChildMisc->GetAllocStreamOffsets().insert(
					dstChildMisc->GetAllocStreamOffsets().end(),
					srcChild->GetAllocStreamOffsets().begin(), srcChild->GetAllocStreamOffsets().end());
				dstChildMisc->SetInstanceCount(dstChildMisc->GetInstanceCount() + srcChild->GetInstanceCount());
			}
		}
		else if (!dstChild->GetChildren() && srcChild->GetChildren())
		{
			if (dstChild->GetSize().GetTotal().consumed > 0)
			{
				ContextTreeNode* dstChildMisc = new ContextTreeNode(*dstChild);
				dstChildMisc->Rename("Misc");
				dstChild->AddChild(dstChildMisc);
				dstChild->GetAllocStreamOffsets().empty();
				dstChild->GetSize() = SizeInfoGroups();
				dstChild->SetInstanceCount(0);
			}

			GroupChildrenImpl(*srcChild, *dstChild, grouper);
		}
		else // if (dstChild->GetChildren() && srcChild->GetChildren())
		{
			GroupChildrenImpl(*srcChild, *dstChild, grouper);
		}
	}
}

#endif
