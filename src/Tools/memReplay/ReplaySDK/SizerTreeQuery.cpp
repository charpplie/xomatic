#include "stdafx.h"
#include "SizerTreeQuery.h"

#include "ContextTree.h"
#include "ReplayVisitor.h"

SizerTreeQuery::SizerTreeQuery(bool overrun)
	: m_overrun(overrun)
{
}

void SizerTreeQuery::ReplayBegin()
{
	m_map.Clear();
}

void SizerTreeQuery::Replay(ReplayRange range)
{
	ReplayVisitor<SizerTreeQuery> visitor(*this);
	visitor.Replay(range);
}

void SizerTreeQuery::ReplayEvent(const ReplayAddressProfile2Event& ev)
{
	m_addressProfile = MemAddressProfile(ev.rsxStart, ev.rsxStart + ev.rsxLength);
}

void SizerTreeQuery::ReplayEvent(const ReplayAlloc3Event &ev)
{
	m_map.BlitAlloc(ev.ptr, ev.sizeConsumed, ev.callstack, ev.callstackLength);
}

void SizerTreeQuery::ReplayEvent(const ReplayFree3Event& ev)
{
	m_map.BlitFree(ev.ptr);
}

void SizerTreeQuery::ReplayEvent(const ReplayFree4Event& ev)
{
	m_map.BlitFree(ev.ptr);
}

void SizerTreeQuery::ReplayEvent(const ReplaySizerPushEvent& ev)
{
	const char* name = ContextTreeNode::InternString(ev.name);
	GenericTreeNode* top;

	if (m_stack.empty())
	{
		SharedPtr<GenericTree> gt = new GenericTree();
		gt->AddStream("Inclusive", new GenericTreeStream<SizeInfoGroups>());
		gt->AddStream("Exclusive", new GenericTreeStream<SizeInfoGroups>());
		gt->AddStream("Ranges", new GenericTreeStream<std::vector<SizerRange> >());

		m_trees.push_back(gt);

		m_currentTree = &*gt;
		m_currentExclusiveStream = gt->GetStream<SizeInfoGroups>("Exclusive");
		m_currentRangesStream = gt->GetStream<std::vector<SizerRange> >("Ranges");
		top = m_currentTree->GetRoot();
	}
	else
	{
		top = m_stack.back();
	}

	GenericTreeNode* nw;
	for (nw = top->children; nw && nw->GetKey<const char*>() != name; nw = nw->sibling)
		;

	if (!nw)
	{
		nw = new GenericTreeNode(name);
		m_currentTree->AddChild(top, nw);
	}
	m_stack.push_back(nw);
}

void SizerTreeQuery::ReplayEvent(const ReplaySizerPopEvent& ev)
{
	m_stack.pop_back();
}

void SizerTreeQuery::ReplayEvent(const ReplaySizerAddRangeEvent& ev)
{
	if (!m_stack.empty())
	{
		GenericTreeNode* top = m_stack.back();
		MemGroups::Group mg = MemGroups::SelectGroupForAddress(ev.ptr, m_addressProfile);

		char objName[32];
		sprintf_s(objName, 32, "%08x", ev.ptr);

		std::vector<size_t> ids;
		ids.reserve(32);
		m_map.FindUniqueAllocIdsInRange(std::back_inserter(ids), ev.ptr, ev.size);

		if (m_overrun)
		{
			if (ids.size() > 1)
			{
				GenericTreeNode* obj = top;//new GenericTreeNode(ContextTreeNode::InternString(objName));
				//m_currentTree->AddChild(top, obj);

				size_t validLen = m_map.FindLengthToEnd(ev.ptr);
				(*obj)[*m_currentExclusiveStream].AddToGroup(mg, SizeInfo(ev.count, 0, ev.size - validLen, ev.size - validLen, 0));

				const MemoryMap::AllocEvInfo& evInfo = m_map.GetAllocEvInfo(ids.front());
				(*obj)[*m_currentRangesStream].push_back(SizerRange(evInfo.callstackId, ev.size, mg));
			}
		}
		else
		{
			GenericTreeNode* obj = top;// new GenericTreeNode(ContextTreeNode::InternString(objName));
			//m_currentTree->AddChild(top, obj);

			(*obj)[*m_currentExclusiveStream].AddToGroup(mg, SizeInfo(ev.count, 0, ev.size, ev.size, 0));

			if (!ids.empty())
			{
				if (ids.size() > 1)
				{
					// wtf?
				}

				const MemoryMap::AllocEvInfo& evInfo = m_map.GetAllocEvInfo(ids.front());
				(*obj)[*m_currentRangesStream].push_back(SizerRange(evInfo.callstackId, ev.size, mg));
			}
			else
			{
				// also wtf
			}
		}
	}
}

void SizerTreeQuery::ReplayEnd(u64 position)
{
	for (std::vector<SharedPtr<GenericTree> >::iterator it = m_trees.begin(), itEnd = m_trees.end(); it != itEnd; ++ it)
	{
		GenericTree& tree = **it;
		SumSizes(tree.GetRoot(), *tree.GetStream<SizeInfoGroups>("Exclusive"), *tree.GetStream<SizeInfoGroups>("Inclusive"));
	}
}

void SizerTreeQuery::RunImpl(ReplayLogReader& reader)
{
	reader.Replay(*this);

	SharedPtr<GenericTree> result;

	if (!m_trees.empty())
		result = m_trees.front();

	Complete(result);
}
