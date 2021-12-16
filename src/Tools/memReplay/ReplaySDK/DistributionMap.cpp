#include "stdafx.h"
#include "DistributionMap.h"

MemoryMapReplay::MemoryMapReplay()
{
	m_currentFrame = 0;
}

void MemoryMapReplay::ReplayBegin()
{
	m_allocs.clear();
	m_currentFrame = 0;
}

void MemoryMapReplay::Replay(ReplayRange range)
{
	using namespace ReplayEventIds;

	ReplayEventIds::Ids id;
	while (range.ReadNext(id))
	{
		switch (id)
		{
		case RE_Alloc3:
			{
				const ReplayAlloc3Event& ev = range.Get<ReplayAlloc3Event>();

				m_allocs.insert(std::make_pair(ev.ptr, Alloc(m_currentFrame, ev.sizeRequested)));
			}
			break;

		case RE_Free3:
			ApplyFree(range.Get<ReplayFree3Event>().ptr);
			break;

		case RE_Free4:
			ApplyFree(range.Get<ReplayFree4Event>().ptr);
			break;

		case RE_FrameStart:
			++ m_currentFrame;
			break;
		}
	}
}

void MemoryMapReplay::ApplyFree(TAddress ptr)
{
	AllocMap::iterator it = m_allocs.find(ptr);
	if (it != m_allocs.end())
	{
		m_allocs.erase(it);
	}
}

void MemoryMapReplay::ReplayEnd(u64 position)
{
}

MemoryMapReplay::ConstIterator MemoryMapReplay::FindFirstFromAddress(TAddress addr) const
{
	ConstIterator it = m_allocs.lower_bound(addr);

	if (it->first != addr)
	{
		// The previous element possibly spans the address, so check that.
		if (it != m_allocs.begin())
		{
			ConstIterator it2 = it;
			-- it2;

			if (it2->first + it2->second.size > addr)
				return it2;
		}
	}

	return it;
}

MemoryMapReplay::ConstIterator MemoryMapReplay::FindLastFromAddress(TAddress addr) const
{
	return m_allocs.lower_bound(addr);
}
