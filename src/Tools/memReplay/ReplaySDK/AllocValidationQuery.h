#pragma once

#include "ReplayQuery.h"
#include "ValidateError.h"
#include "CallstackTable.h"

class AllocValidationQuery : public ReplayQuery<SharedPtr<ValidateResult> >, public IReplayListener
{
public:
	void ReplayBegin();
	void Replay(ReplayRange range);
	void ReplayEnd(u64 position);

private:
	struct AllocInfo
	{
		AllocInfo() : callstackId(0), length(0), ev(0), thread(0) {}
		AllocInfo(size_t callstackId, size_t length, u64 ev, TThreadId thread) : callstackId(callstackId), length(length), ev(ev), thread(thread) {}
		size_t callstackId;
		size_t length;
		u64 ev;
		TThreadId thread;
	};

	enum
	{
		Ser_Version = 1,
		FreedLength = (size_t) -1,

		MaxErrors = 10000
	};

	typedef std::map<TAddress, AllocInfo, std::less<TAddress>, STLPoolAllocator<std::pair<const TAddress, AllocInfo> > > AllocMap;

private:
	void RunImpl(ReplayLogReader& reader);

private:
	void ApplyFree(TAddress ptr, TThreadId threadId);
	ptrdiff_t ResolvePointer(TAddress addr, AllocMap::iterator& itOut);

private:
	ValidateResult* m_result;

	AllocMap m_allocs;
	u64 m_allocEv;
};
