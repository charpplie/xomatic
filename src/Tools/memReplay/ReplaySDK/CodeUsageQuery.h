#pragma once

#include "ReplayQuery.h"
#include "SizeInfo.h"

template <typename> class ReplayVisitor;

class CodeUsageQuery : public ReplayQuery<SharedPtr<SizeInfoUsageResult> >, public IReplayListener
{
	friend class ReplayVisitor<CodeUsageQuery>;

public:
	explicit CodeUsageQuery(const std::vector<TAddress>& addresses);

public: // IReplayListener Members
	void ReplayBegin();
	void Replay(ReplayRange range);
	void ReplayEnd(u64 position);

private:
	typedef std::map<TAddress, SizeInfo, std::less<TAddress>, STLPoolAllocator<std::pair<TAddress, SizeInfo> > > AllocMap;
	enum
	{
		AllocEvSplit = 256
	};

private:
	void RunImpl(ReplayLogReader& reader);

	void ReplayEvent(const ReplayAlloc3Event& ev);
	void ReplayEvent(const ReplayFree3Event& ev);
	void ReplayEvent(const ReplayFree4Event& ev);
	void ReplayEvent(const ReplayFrameStartEvent& ev);
	void ReplayEvent(const ReplayAddressProfileEvent& ev);
	void ReplayEvent(const ReplayAddressProfile2Event& ev);
	template <typename T> void ReplayEvent(const T&){}

private:
	std::vector<TAddress> m_addresses;

	MemAddressProfile m_addressProfile;
	AllocMap m_allocs;
	u64 m_allocEv;

	SharedPtr<SizeInfoUsageResult> m_result;
};
