#pragma once

#include "ReplayQuery.h"
#include "GenericTree.h"
#include "MemoryMap.h"

template <typename> class ReplayVisitor;

struct SizerRange
{
	SizerRange()
		: callstackId(0)
		, size(0)
		, mg(0)
	{
	}

	SizerRange(size_t callstackId, size_t size, MemGroups::Group mg)
		: callstackId(callstackId)
		, size(size)
		, mg(mg)
	{
	}

	size_t callstackId;
	u32 size : 31;
	u32 mg : 1;
};

class SizerTreeQuery : public ReplayQuery<SharedPtr<GenericTree> >, public IReplayListener
{
	friend class ReplayVisitor<SizerTreeQuery>;

public:
	explicit SizerTreeQuery(bool overrun);
	
	void ReplayBegin();
	void Replay(ReplayRange range);
	void ReplayEnd(u64 length);

private:
	void RunImpl(ReplayLogReader& reader);

	void ReplayEvent(const ReplayAddressProfile2Event& ev);
	void ReplayEvent(const ReplayAlloc3Event& ev);
	void ReplayEvent(const ReplayFree3Event& ev);
	void ReplayEvent(const ReplayFree4Event& ev);
	void ReplayEvent(const ReplaySizerPushEvent& ev);
	void ReplayEvent(const ReplaySizerPopEvent& ev);
	void ReplayEvent(const ReplaySizerAddRangeEvent& ev);
	template <typename T> void ReplayEvent(const T&) {}

private:
	bool m_overrun;

	MemAddressProfile m_addressProfile;
	MemoryMap m_map;

	std::vector<GenericTreeNode*> m_stack;
	GenericTree* m_currentTree;
	GenericTreeStream<SizeInfoGroups>* m_currentExclusiveStream;
	GenericTreeStream<std::vector<SizerRange> >* m_currentRangesStream;

	std::vector<SharedPtr<GenericTree> > m_trees;
};
