#include "stdafx.h"
#include "CodeUsageQuery.h"

#include "ReplayVisitor.h"

CodeUsageQuery::CodeUsageQuery(const std::vector<TAddress>& addresses)
	: m_addresses(addresses)
{
}

void CodeUsageQuery::ReplayBegin()
{
	m_allocEv = 0;

	m_result = new SizeInfoUsageResult();
	m_result->allocEvSplit = AllocEvSplit;

	m_result->byFrame.push_back(SizeInfoGroups());
	m_result->byFrame.push_back(SizeInfoGroups());
	m_result->byAllocEv.push_back(SizeInfoGroups());
	m_result->byAllocEv.push_back(SizeInfoGroups());
}

void CodeUsageQuery::Replay(ReplayRange range)
{
	ReplayVisitor<CodeUsageQuery> visitor(*this);
	visitor.Replay(range);
}

void CodeUsageQuery::ReplayEnd(u64 position)
{
}

void CodeUsageQuery::RunImpl(ReplayLogReader& reader)
{
	reader.Replay(*this);
	Complete(m_result);
}

void CodeUsageQuery::ReplayEvent(const ReplayAlloc3Event& ev)
{
	for (size_t i = 0; i < ev.callstackLength; ++ i)
	{
		if (std::binary_search(m_addresses.begin(), m_addresses.end(), ev.callstack[i]))
		{
			if (m_allocs.find(ev.ptr) == m_allocs.end())
			{
				SizeInfo si(1, 0, ev.sizeRequested, ev.sizeConsumed, ev.sizeGlobal);
				MemGroups::Group memGroup = MemGroups::SelectGroupForAddress(ev.ptr, m_addressProfile);

				m_result->byFrame.back().AddToGroup(memGroup, si);
				m_result->byAllocEv.back().AddToGroup(memGroup, si);

				m_allocs.insert(std::make_pair(ev.ptr, si));
			}

			break;
		}
	}

	++ m_allocEv;
	if (!(m_allocEv & AllocEvSplit - 1))
	{
		SizeInfoGroups back = m_result->byAllocEv.back();
		m_result->byAllocEv.push_back(back);
	}
}

void CodeUsageQuery::ReplayEvent(const ReplayFree3Event& ev)
{
	AllocMap::iterator it = m_allocs.find(ev.ptr);
	if (it != m_allocs.end())
	{
		MemGroups::Group memGroup = MemGroups::SelectGroupForAddress(ev.ptr, m_addressProfile);
		SizeInfo szInfo(0, 1, -it->second.requested, -it->second.consumed, ev.sizeGlobal);
		m_result->byFrame.back().AddToGroup(memGroup, szInfo);
		m_result->byAllocEv.back().AddToGroup(memGroup, szInfo);

		m_allocs.erase(it);
	}

	++ m_allocEv;
	if (!(m_allocEv & (AllocEvSplit - 1)))
	{
		SizeInfoGroups back = m_result->byAllocEv.back();
		m_result->byAllocEv.push_back(back);
	}
}

void CodeUsageQuery::ReplayEvent(const ReplayFree4Event& ev)
{
	AllocMap::iterator it = m_allocs.find(ev.ptr);
	if (it != m_allocs.end())
	{
		MemGroups::Group memGroup = MemGroups::SelectGroupForAddress(ev.ptr, m_addressProfile);
		SizeInfo szInfo(0, 1, -it->second.requested, -it->second.consumed, ev.sizeGlobal);
		m_result->byFrame.back().AddToGroup(memGroup, szInfo);
		m_result->byAllocEv.back().AddToGroup(memGroup, szInfo);

		m_allocs.erase(it);
	}

	++ m_allocEv;
	if (!(m_allocEv & (AllocEvSplit - 1)))
	{
		SizeInfoGroups back = m_result->byAllocEv.back();
		m_result->byAllocEv.push_back(back);
	}
}

void CodeUsageQuery::ReplayEvent(const ReplayFrameStartEvent& ev)
{
	SizeInfoGroups back = m_result->byFrame.back();
	m_result->byFrame.push_back(back);
}

void CodeUsageQuery::ReplayEvent(const ReplayAddressProfileEvent& ev)
{
	m_addressProfile = MemAddressProfile(ev.rsxStart, 0xffffffff);
}

void CodeUsageQuery::ReplayEvent(const ReplayAddressProfile2Event& ev)
{
	m_addressProfile = MemAddressProfile(ev.rsxStart, ev.rsxStart + ev.rsxLength);
}
