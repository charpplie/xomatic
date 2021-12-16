#pragma once

#include "ReplayQuery.h"
#include "SizeInfo.h"

template <typename> class ReplayVisitor;

class BucketUsageQuery : public ReplayQuery<SharedPtr<BucketUsageResult> >, public IReplayListener
{
	friend class ReplayVisitor<BucketUsageQuery>;

public:
	BucketUsageQuery();

public: // IReplayListener Members
	void ReplayBegin();
	void Replay(ReplayRange range);
	void ReplayEnd(u64 position);

private:
	typedef std::pair<u16, u16> AllocSize;
	typedef std::map<TAddress, std::pair<AllocSize, int>, std::less<TAddress>, STLPoolAllocator<std::pair<TAddress, std::pair<size_t, int> > > > AllocMap;
	enum
	{
		AllocEvSplit = 256
	};

private:
	void RunImpl(ReplayLogReader& reader);

	void ReplayEvent(const ReplayAlloc3Event& ev);
	void ReplayEvent(const ReplayFree3Event& ev);
	void ReplayEvent(const ReplayFree4Event& ev);
	void ReplayEvent(const ReplayBucketMark2Event& ev);
	template <typename T> void ReplayEvent(const T&){}

	void ExtendBuckets();
	void PadStream(BucketUsageResult::UsageStream& stream);

private:
	AllocMap m_allocs;
	u64 m_allocEv;

	std::vector<BucketRange> m_bucketRanges;

	SharedPtr<BucketUsageResult> m_result;
};
