#pragma once

#include "ReplayQuery.h"
#include "SizeInfo.h"

template <typename> class ReplayVisitor;

class ContextUsageQuery : public ReplayQuery<SharedPtr<SizeInfoGroupVector> >, public IReplayListener
{
	friend class ReplayVisitor<ContextUsageQuery>;

public:
	ContextUsageQuery();

public: // IReplayListener Members
	void ReplayBegin();
	void Replay(ReplayRange range);
	void ReplayEnd(u64 position);

private:
	void RunImpl(ReplayLogReader& reader);

	void ReplayEvent(const ReplayAlloc3Event& ev);
	void ReplayEvent(const ReplayFree3Event& ev);
	void ReplayEvent(const ReplayPushContext3Event& ev);
	void ReplayEvent(const ReplayPopContextEvent& ev);
	template <typename T> void ReplayEvent(const T&){}

private:

};
