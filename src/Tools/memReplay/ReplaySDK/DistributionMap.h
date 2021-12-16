#pragma once

#include "ReplayLogReader.h"

class MemoryMapReplay : public IReplayListener
{
public:
	struct Alloc
	{
		Alloc() {}
		Alloc(u32 frame, u32 size) : frame(frame), size(size) {}
		u32 frame;
		u32 size;
	};

	typedef std::map<TAddress, Alloc, std::less<TAddress>, STLPoolAllocator<std::pair<const TAddress, Alloc> > >::const_iterator ConstIterator;

public:
	MemoryMapReplay();

	void ReplayBegin();
	void Replay(ReplayRange range);
	void ReplayEnd(u64 position);

	ConstIterator FindFirstFromAddress(TAddress addr) const;
	ConstIterator FindLastFromAddress(TAddress addr) const;

private:
	typedef std::map<TAddress, Alloc, std::less<TAddress>, STLPoolAllocator<std::pair<const TAddress, Alloc> > > AllocMap;

	void ApplyFree(TAddress ptr);

private:
	AllocMap m_allocs;

	u32 m_currentFrame;
};
