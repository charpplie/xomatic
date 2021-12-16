#include "stdafx.h"

#include "ContextTree.h"
#include "ReplayLogReader.h"

StringTable ContextTreeNode::ms_stringTable;

namespace
{

	void SumTree(ContextTreeNode* node)
	{
		SizeInfoGroups& size = node->GetSize();

		if (node->GetChildren())
		{
			ContextTreeNode* miscNode = new ContextTreeNode("Misc", MemStatContextTypes::MSC_Other);
			miscNode->GetSize() = size;
			miscNode->SetInstanceCount(node->GetInstanceCount());
			miscNode->GetAllocStreamOffsets().swap(node->GetAllocStreamOffsets());
			node->AddChild(miscNode);

			for (ContextTreeNode* child = node->GetChildren()->GetNextSibling(); child; child = child->GetNextSibling())
			{
				SumTree(child);
				size += child->GetSize();
			}
		}
	}

	void SerialiseNode(ISerialiser& ser, const ContextTreeNode* n)
	{
		int nameLen = strlen(n->GetName());
		ser.Write(nameLen);
		ser.WriteRaw(n->GetName(), nameLen);

		ser.Write((int) n->GetType());
		ser.Write((int) n->GetInstanceCount());
		n->GetSize().Serialise(ser);
		ser.Write((int) n->GetAllocStreamOffsets().size());
		ser.Write(n->GetAllocStreamOffsets().begin(), n->GetAllocStreamOffsets().end());

		ser.Write((int) n->CountChildren());

		for (const ContextTreeNode* c = n->GetChildren(); c; c = c->GetNextSibling())
			SerialiseNode(ser, c);
	}

	ContextTreeNode* DeserialiseNode(IDeserialiser& ser)
	{
		int nameLen = ser.Read<int>();
		char* name = (char*) alloca(nameLen + 1);
		memset(name, 0, nameLen + 1);
		ser.ReadRaw(name, nameLen);

		MemStatContextTypes::Type type = (MemStatContextTypes::Type) ser.Read<int>();
		int instanceCount = ser.Read<int>();

		SizeInfoGroups sz;
		sz.Deserialise(ser);

		std::vector<ContextStreamOffsetSpan> offsets;
		int offsetCount = ser.Read<int>();
		offsets.reserve(offsetCount);
		ser.Read(std::back_inserter(offsets), offsetCount);

		int childrenCount = ser.Read<int>();

		ContextTreeNode* n = new ContextTreeNode(ContextTreeNode::InternString(name), type);
		n->SetInstanceCount(instanceCount);
		n->GetSize() = sz;
		n->GetAllocStreamOffsets() = offsets;

		for (size_t ci = 0; ci != childrenCount; ++ ci)
			n->AddChild(DeserialiseNode(ser));

		return n;
	}

}

SharedPtr<ContextTree> ContextTree::MergeChildren(const ContextTree& a, const ContextTree& b)
{
	SharedPtr<ContextTree> newTree = new ContextTree();

	const ContextTreeNode* aRoot = a.GetRoot();
	if (aRoot)
	{
		newTree->m_root = CloneTree(aRoot);

		const ContextTreeNode* bRoot = b.GetRoot();
		if (bRoot)
		{
			newTree->m_root->AddChild(CloneTree(bRoot));
		}

		RefreshTreeSums(newTree->m_root);
	}

	return newTree;
}

void ContextTreeNode::InitStringPool()
{
	atexit(&ContextTreeNode::ReleaseStringPool);
}

void ContextTreeNode::ReleaseStringPool()
{
	ms_stringTable.Clear();
}

const char* ContextTreeNode::InternString(const char* str)
{
	static bool firstCall = (InitStringPool(), true);
	return ms_stringTable.InternString(str);
}

#if POOL_CONTEXTTREENODES 
static PoolAllocator<ContextTreeNode, 4096> s_NodePool;

void* ContextTreeNode::operator new (size_t sz)
{
	return s_NodePool.Allocate();
}

void ContextTreeNode::operator delete (void* p, size_t sz)
{
	s_NodePool.Free(p);
}
#endif

ContextTreeNode::ContextTreeNode(const char* name, MemStatContextTypes::Type type)
	: m_name(name)
	, m_type(type)
	, m_count(0)
	, m_parent(NULL)
	, m_nextSibling(NULL)
	, m_children(NULL)
{
}

SharedPtr<ContextTreeReplayBuilder> ContextTree::CreateBuilder()
{
	SharedPtr<ContextTree> tree = new ContextTree();
	tree->m_root = new ContextTreeNode("(root)", MemStatContextTypes::MSC_Other);

	SharedPtr<ContextTreeReplayBuilder> builder = new ContextTreeReplayBuilder(tree, tree->m_root);//, 13582336);

	return builder;
}

SharedPtr<ContextTree> ContextTree::Deserialise(IDeserialiser& ser)
{
	SharedPtr<ContextTree> tree = new ContextTree();
	if (tree->DeserialiseImpl(ser))
		return tree;

	return SharedPtr<ContextTree>();
}

void ContextTree::RefreshTreeSums(ContextTreeNode* node)
{
	SizeInfoGroups& size = node->GetSize();

	if (node->GetChildren())
	{
		size = SizeInfoGroups();

		for (ContextTreeNode* child = node->GetChildren(); child; child = child->GetNextSibling())
		{
			RefreshTreeSums(child);
			size += child->GetSize();
		}
	}
}

ContextTreeReplayBuilder::ContextTreeReplayBuilder(
	const SharedPtr<ContextTree>& tree, ContextTreeNode* root,
	u64 allocEvBegin, u64 allocEvEnd)
	: m_tree(tree)
	, m_root(root)
	, m_begin(allocEvBegin)
	, m_end(allocEvEnd)
{
}

void ContextTreeReplayBuilder::ReplayBegin()
{
	m_current = 0;
}

void ContextTreeReplayBuilder::Replay(ReplayRange reader)
{
	using namespace ReplayEventIds;

	ReplayEventIds::Ids id;

	while (reader.ReadNext(id))
	{
		switch(id)
		{
		case RE_AddressProfile:
			{
				const ReplayAddressProfileEvent& ev = reader.Get<ReplayAddressProfileEvent>();
				m_addressProfile = MemAddressProfile(ev.rsxStart, 0xffffffff);
			}
			break;

		case RE_AddressProfile2:
			{
				const ReplayAddressProfile2Event& ev = reader.Get<ReplayAddressProfile2Event>();
				m_addressProfile = MemAddressProfile(ev.rsxStart, ev.rsxStart + ev.rsxLength);
			}
			break;

		case RE_PushContext:
			{
				const ReplayPushContextEvent& ev = reader.Get<ReplayPushContextEvent>();
				PushContext(ev.threadId, ev.contextType, ev.name, MemStatContextFlags::MSF_Instance, reader.GetPosition());
			}
			break;

		case RE_PushContext2:
			{
				const ReplayPushContext2Event& ev = reader.Get<ReplayPushContext2Event>();
				PushContext(ev.threadId, ev.contextType, ev.name, MemStatContextFlags::MSF_Instance, reader.GetPosition());
			}
			break;

		case RE_PushContext3:
			{
				const ReplayPushContext3Event& ev = reader.Get<ReplayPushContext3Event>();
				PushContext(ev.threadId, ev.contextType, ev.name, ev.flags, reader.GetPosition());
			}
			break;

		case RE_PopContext:
			{
				const ReplayPopContextEvent& ev = reader.Get<ReplayPopContextEvent>();
				PopContext(ev.threadId);
			}
			break;
			
		case RE_Alloc3:
			{
				const ReplayAlloc3Event& ev = reader.Get<ReplayAlloc3Event>();

				std::map<u32, ThreadContext>::iterator itThread = threads.find(ev.threadId);
				if (itThread == threads.end())
				{
					char name[32];
					_snprintf_s(name, 32, "Thread %08x", ev.threadId);

					ContextTreeNode* threadChild = new ContextTreeNode(ContextTreeNode::InternString(name), MemStatContextTypes::MSC_Other);
					m_root->AddChild(threadChild);

					threadChild->AddAllocStreamOffset(reader.GetPosition(), ev.threadId);

					itThread = threads.insert(std::make_pair(ev.threadId, ThreadContext(threadChild))).first;
				}

				ThreadContext& current = itThread->second;

				MemGroups::Group group = MemGroups::SelectGroupForAddress(ev.ptr, m_addressProfile);
				SizeInfo size(1, 0, ev.sizeRequested, ev.sizeConsumed, ev.sizeGlobal);

				if (m_current >= m_begin && m_current < m_end)
				{
					if (current.topNeedsAllocOffset)
					{
						current.top->AddAllocStreamOffset(reader.GetPosition(), ev.threadId);
						current.topNeedsAllocOffset = false;
					}

					current.top->GetSize().AddToGroup(group, size);
				}

				activePointers.insert(std::make_pair(ev.ptr, std::make_pair(current.top, size)));

				++ m_current;
			}
			break;

		case RE_Free3:
			{
				const ReplayFree3Event& ev = reader.Get<ReplayFree3Event>();
				ApplyFree(ev.ptr, ev.threadId, reader.GetPosition(), ev.sizeGlobal);
			}
			break;

		case RE_Free4:
			{
				const ReplayFree4Event& ev = reader.Get<ReplayFree4Event>();
				ApplyFree(ev.ptr, ev.threadId, reader.GetPosition(), ev.sizeGlobal);
			}
			break;

		case RE_Info:
			{
				const ReplayInfoEvent& ev = reader.Get<ReplayInfoEvent>();
				ContextTreeNode* exeNode = new ContextTreeNode("Pre-track size (executable)", MemStatContextTypes::MSC_Other);
				m_root->AddChild(exeNode);
				exeNode->GetSize().AddToGroup(MemGroups::Main, SizeInfo(1, 0, ev.preTrackSize, ev.preTrackSize, ev.preTrackSize));
			}
			break;
		
		case RE_Info2:
			{
				const ReplayInfo2Event& ev = reader.Get<ReplayInfo2Event>();
				ContextTreeNode* exeNode = new ContextTreeNode("Pre-track size (executable)", MemStatContextTypes::MSC_Other);
				m_root->AddChild(exeNode);
				exeNode->GetSize().AddToGroup(MemGroups::Main, SizeInfo(1, 0, ev.preTrackSize - ev.bucketsFree, ev.preTrackSize - ev.bucketsFree, ev.preTrackSize));
			}
			break;
		}
	}
}

void ContextTreeReplayBuilder::PushContext(u32 threadId, u32 type, const char* name, u32 flags, u64 streamPosition)
{
	std::map<u32, ThreadContext>::iterator itThread = threads.find(threadId);
	if (itThread == threads.end())
	{
		char name[32];
		_snprintf_s(name, 32, "Thread %08x", threadId);

		ContextTreeNode* threadChild = new ContextTreeNode(ContextTreeNode::InternString(name), MemStatContextTypes::MSC_Other);
		m_root->AddChild(threadChild);

		itThread = threads.insert(std::make_pair(threadId, ThreadContext(threadChild))).first;
	}

	ContextTreeNode*& current = itThread->second.top;
	itThread->second.topNeedsAllocOffset = true;

	const char* internedEvName = ContextTreeNode::InternString(name);
	ChildGroupMap::iterator it = childGroups.find(ChildGroupKey(current, internedEvName, static_cast<MemStatContextTypes::Type>(type)));

	ContextTreeNode* child = NULL;

	if (it == childGroups.end())
	{
		child = new ContextTreeNode(internedEvName, static_cast<MemStatContextTypes::Type>(type));
		current->AddChild(child);

		childGroups.insert(std::make_pair(ChildGroupKey(current, internedEvName, static_cast<MemStatContextTypes::Type>(type)), child));
	}
	else
	{
		child = it->second;
	}

	if ((flags & MemStatContextFlags::MSF_Instance) || (child->GetInstanceCount() == 0))
		child->IncrementInstanceCount();

	current = child;
}

void ContextTreeReplayBuilder::PopContext(u32 threadId)
{
	std::map<u32, ThreadContext>::iterator itThread = threads.find(threadId);
	if (itThread != threads.end())
	{
		ThreadContext& current = itThread->second;

		current.top = current.top->GetParent();
		if (current.top)
			current.topNeedsAllocOffset = true;
	}
}

void ContextTreeReplayBuilder::ApplyFree(TAddress ptr, TThreadId threadId, u64 streamPosition, ptrdiff_t sizeGlobal)
{
	ActivePointersMap::iterator it = activePointers.find(ptr);
	if (it != activePointers.end())
	{
		if (m_current >= m_begin && m_current < m_end)
		{
			MemGroups::Group group = MemGroups::SelectGroupForAddress(ptr, m_addressProfile);

			std::map<u32, ThreadContext>::iterator itThread = threads.find(threadId);
			if (itThread != threads.end())
			{
				if (itThread->second.topNeedsAllocOffset)
				{
					itThread->second.top->AddAllocStreamOffset(streamPosition, threadId);
					itThread->second.topNeedsAllocOffset = false;
				}
			}

			std::pair<ContextTreeNode*, SizeInfo> info = it->second;
			info.first->GetSize().AddToGroup(group, SizeInfo(0, 1, -info.second.requested, -info.second.consumed, sizeGlobal));
		}

		activePointers.erase(it);
	}

	++ m_current;
}

void ContextTreeReplayBuilder::ReplayEnd(u64 position)
{
	for (ContextTreeNode* child = m_root->GetChildren(), *next; child; child = next)
	{
		next = child->GetNextSibling();

		for (ContextTreeNode* child2 = child->GetChildren(), *next2; child2; child2 = next2)
		{
			next2 = child2->GetNextSibling();
			child->RemoveChild(child2);
			m_root->AddChild(child2);
		}
	}
	
	SumTree(m_root);
}

SharedPtr<ContextTree> ContextTree::GatherSubTrees(const ContextTree& sourceTree, const char* rootName, const IContextTreeFilter& filter)
{
	SharedPtr<ContextTree> newTree = new ContextTree();

	ContextTreeNode* root = new ContextTreeNode(*sourceTree.m_root);
	root->Rename(rootName);
	newTree->m_root = root;

	struct StackNode
	{
		ContextTreeNode* parent;
		const ContextTreeNode* source;
		StackNode(ContextTreeNode* parent, const ContextTreeNode* source)
			: parent(parent)
			, source(source)
		{}
	};

	std::vector<StackNode> stck;

	stck.reserve(64);
	stck.push_back(StackNode(root, sourceTree.m_root));

	while (!stck.empty())
	{
		StackNode item = stck.back();
		stck.pop_back();

		if (filter(*item.source))
		{
			ContextTreeNode* clone = CloneTree(item.source);
			item.parent->AddChild(clone);
		}
		else
		{
			for (const ContextTreeNode* sourceChild = item.source->GetChildren(); sourceChild; sourceChild = sourceChild->GetNextSibling())
			{
				stck.push_back(StackNode(item.parent, sourceChild));
			}
		}
	}

	RefreshTreeSums(newTree->m_root);

	return newTree;
}

void ContextTree::GatherSubTreesBottomUpImpl(ContextTreeNode* root, const ContextTreeNode* sr, std::set<const ContextTreeNode*>& ignoreSet, const IContextTreeFilter& filter)
{
	for (const ContextTreeNode* sn = sr->GetChildren(); sn; sn = sn->GetNextSibling())
	{
		GatherSubTreesBottomUpImpl(root, sn, ignoreSet, filter);
	}

	if (filter(*sr))
	{
		ContextTreeNode* clone = CloneTree(sr, ignoreSet);
		root->AddChild(clone);

		ignoreSet.insert(sr);
	}
}

SharedPtr<ContextTree> ContextTree::GatherSubTreesBottomUp(const ContextTree& sourceTree, const char* rootName, const IContextTreeFilter& filter)
{
	SharedPtr<ContextTree> newTree = new ContextTree();

	ContextTreeNode* root = new ContextTreeNode(rootName, MemStatContextTypes::MSC_Other);
	root->Rename(rootName);
	newTree->m_root = root;

	std::set<const ContextTreeNode*> ignoreSet;

	GatherSubTreesBottomUpImpl(root, sourceTree.GetRoot(), ignoreSet, filter);

	RefreshTreeSums(newTree->m_root);

	return newTree;
}

SharedPtr<ContextTree> ContextTree::FilterLeaves(const ContextTree& sourceTree, const IContextTreeFilter& filter)
{
	SharedPtr<ContextTree> newTree = new ContextTree();

	newTree->m_root = FilterLeavesImpl(sourceTree.m_root, filter);

	if (newTree->m_root)
		RefreshTreeSums(newTree->m_root);

	return newTree;
}

SharedPtr<ContextTree> ContextTree::Filter(const ContextTree& sourceTree, const IContextTreeFilter& filter)
{
	SharedPtr<ContextTree> newTree = new ContextTree();

	const ContextTreeNode* sourceRoot = sourceTree.GetRoot();
	if (sourceRoot)
	{
		newTree->m_root = new ContextTreeNode(*sourceRoot);
		FilterImpl(*sourceRoot, *newTree->m_root, filter);
		RefreshTreeSums(newTree->m_root);
	}

	return newTree;
}

ContextTreeNode* ContextTree::FilterLeavesImpl(const ContextTreeNode* node, const IContextTreeFilter& filter)
{
	if (node->GetChildren())
	{
		ContextTreeNode* rootClone = NULL;

		for (const ContextTreeNode* child = node->GetChildren(); child; child = child->GetNextSibling())
		{
			ContextTreeNode* childClone = FilterLeavesImpl(child, filter);
			if (childClone)
			{
				if (rootClone == NULL)
				{
					rootClone = new ContextTreeNode(*node);
				}

				rootClone->AddChild(childClone);
			}
		}

		return rootClone;
	}
	else
	{
		return filter(*node) ? new ContextTreeNode(*node) : NULL;
	}
}

void ContextTree::FilterImpl(const ContextTreeNode& source, ContextTreeNode& dest, const IContextTreeFilter& filter)
{
	for (const ContextTreeNode* child = source.GetChildren(); child; child = child->GetNextSibling())
	{
		if (filter(*child))
		{
			ContextTreeNode* clone = new ContextTreeNode(*child);
			dest.AddChild(clone);

			FilterImpl(*child, *clone, filter);
		}
	}
}

ContextTree::ContextTree()
	: m_root(NULL)
{
}

bool ContextTree::DeserialiseImpl(IDeserialiser& ser)
{
	int version = ser.Read<int>();
	if (version != Ser_Version)
		return false;

	if (ser.Read<int>() != MemGroups::Count)
		return false;

	int rootCount = ser.Read<int>();

	if (rootCount)
		m_root = DeserialiseNode(ser);

	return true;
}

ContextTree::~ContextTree()
{
	struct recItem
	{
		recItem(ContextTreeNode* r) : next(r) {}
		ContextTreeNode* next;
	};

	if (m_root)
	{
		std::vector<recItem> stck;
		stck.reserve(256);

		stck.push_back(recItem(m_root));

		while (stck.empty() == false)
		{
			recItem item = stck.back();
			stck.pop_back();

			for (ContextTreeNode* child = item.next->m_children; child; child = child->GetNextSibling())
			{
				stck.push_back(child);
			}

			delete item.next;
		}
	}
}

void ContextTree::Serialise(ISerialiser& ser)
{
	ser.Write((int) Ser_Version);
	ser.Write((int) MemGroups::Count);

	if (m_root)
	{
		ser.Write((int) 1);
		SerialiseNode(ser, m_root);
	}
	else
	{
		ser.Write((int) 0);
	}
}

ContextTreeNode* ContextTree::BeginEdit()
{
	if (m_root == NULL)
	{
		m_root = new ContextTreeNode("", MemStatContextTypes::MSC_Other);
	}

	return m_root;
}

void ContextTree::EndEdit()
{
	SumTree(m_root);
}
