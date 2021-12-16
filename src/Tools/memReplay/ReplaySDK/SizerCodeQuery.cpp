#include "stdafx.h"
#include "SizerCodeQuery.h"

#include "ContextTree.h"
#include "ReplayVisitor.h"

SizerCodeQuery::SizerCodeQuery(size_t treeIdx, const std::vector<const char*>& match)
	: m_treeIdx(treeIdx)
	, m_matching(false)
	, m_currentTreeIdx(0)
	, m_exclusive(NULL)
	, m_inclusive(NULL)
{
	m_match.reserve(match.size());
	std::transform(match.begin(), match.end(), std::back_inserter(m_match), ContextTreeNode::InternString);
}

void SizerCodeQuery::ReplayBegin()
{
	m_map.Clear();
	m_stack.clear();
	m_stack.reserve(32);
	m_csnm.clear();
	m_matching = false;

	m_result = CreateCodeGenericTree();
	m_exclusive = m_result->GetStream<SizeInfoGroups>("Exclusive");
	m_inclusive = m_result->GetStream<SizeInfoGroups>("Inclusive");
}

void SizerCodeQuery::Replay(ReplayRange range)
{
	ReplayVisitor<SizerCodeQuery> visitor(*this);
	visitor.Replay(range);
}

void SizerCodeQuery::ReplayEnd(u64 length)
{
	SumSizes(m_result->GetRoot(), *m_exclusive, *m_inclusive);
	m_map.Clear();
}

void SizerCodeQuery::RunImpl(ReplayLogReader& reader)
{
	reader.Replay(*this);

	Complete(m_result);
}

void SizerCodeQuery::ReplayEvent(const ReplayAddressProfile2Event& ev)
{
	m_addressProfile = MemAddressProfile(ev.rsxStart, ev.rsxStart + ev.rsxLength);
}

void SizerCodeQuery::ReplayEvent(const ReplayAlloc3Event& ev)
{
	m_map.BlitAlloc(ev.ptr, ev.sizeConsumed, ev.callstack, ev.callstackLength);
}

void SizerCodeQuery::ReplayEvent(const ReplayFree3Event& ev)
{
	m_map.BlitFree(ev.ptr);
}

void SizerCodeQuery::ReplayEvent(const ReplayFree4Event& ev)
{
	m_map.BlitFree(ev.ptr);
}

void SizerCodeQuery::ReplayEvent(const ReplaySizerPushEvent& ev)
{
	const char* name = ContextTreeNode::InternString(ev.name);
	m_stack.push_back(name);

	// Not the most efficient match in the world...
	if (!m_matching)
	{
		if (m_currentTreeIdx == m_treeIdx && m_stack.size() >= m_match.size())
		{
			if (memcmp(&m_stack[0], &m_match[0], sizeof(const char*) * m_match.size()) == 0)
				m_matching = true;
		}
	}
}

void SizerCodeQuery::ReplayEvent(const ReplaySizerPopEvent& ev)
{
	m_stack.pop_back();

	if (m_stack.size() < m_match.size())
		m_matching = false;
	if (m_stack.empty())
		++ m_currentTreeIdx;
}

void SizerCodeQuery::ReplayEvent(const ReplaySizerAddRangeEvent& ev)
{
	if (m_matching)
	{
		m_ids.clear();
		m_map.FindUniqueAllocIdsInRange(std::back_inserter(m_ids), ev.ptr, ev.size);

		if (!m_ids.empty())
		{
			if (m_ids.size() > 1)
			{
				// wtf?
			}

			const MemoryMap::AllocEvInfo& evInfo = m_map.GetAllocEvInfo(m_ids.front());

			GenericTreeNode* dstNode = FindNodeForCallstackId(m_csnm, evInfo.callstackId, *m_map.GetCallstacks(), *m_result, m_result->GetRoot());
			MemGroups::Group group = MemGroups::SelectGroupForAddress(ev.ptr, m_addressProfile);

			(*dstNode)[*m_exclusive].AddToGroup(group, SizeInfo(1, 0, ev.size, ev.size, 0));
		}
		else
		{
			// also wtf
		}
	}
}
