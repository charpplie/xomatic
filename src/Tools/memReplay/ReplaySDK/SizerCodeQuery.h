#pragma once

#include "ReplayQuery.h"
#include "GenericTree.h"
#include "ReplayLogReader.h"

#include "SizeInfo.h"
#include "MemoryMap.h"
#include "memHier2.h"

template <typename> class ReplayVisitor;

class SizerCodeQuery : public ReplayQuery<SharedPtr<GenericTree> >, public IReplayListener
{
	friend class ReplayVisitor<SizerCodeQuery>;

public:
	SizerCodeQuery(size_t treeIdx, const std::vector<const char*>& match);
	
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
	size_t m_treeIdx;
	std::vector<const char*> m_match;

	MemAddressProfile m_addressProfile;
	MemoryMap m_map;
	std::vector<const char*> m_stack;
	std::vector<size_t> m_ids;
	CallstackNodeMap m_csnm;
	size_t m_currentTreeIdx;
	bool m_matching;

	SharedPtr<GenericTree> m_result;
	GenericTreeStream<SizeInfoGroups>* m_exclusive;
	GenericTreeStream<SizeInfoGroups>* m_inclusive;
};
