#include "stdafx.h"
#include "AllocValidationQuery.h"

void AllocValidationQuery::ReplayBegin()
{
	m_result->errors.clear();
	m_result->callstacks = CallstackTable(NULL);
	m_allocEv = 0;
}

void AllocValidationQuery::Replay(ReplayRange range)
{
	using namespace ReplayEventIds;

	if (m_result->errors.size() >= MaxErrors)
		return;

	Ids id;
	while (range.ReadNext(id))
	{
		switch (id)
		{
		case RE_Alloc3:
			{
				const ReplayAlloc3Event& ev = range.Get<ReplayAlloc3Event>();

				AllocMap::iterator it = m_allocs.find(ev.ptr);
				if (it == m_allocs.end())
				{
					m_allocs.insert(std::make_pair(ev.ptr, AllocInfo(m_result->callstacks.AddCallstack(ev), ev.sizeConsumed, m_allocEv, ev.threadId)));
				}
				else
				{
					if (it->second.length != FreedLength)
					{
						// Allocating the same pointer twice?
						m_result->errors.push_back(ValidateError(
							ValidateError::ERR_DoubleAlloc,
							ValidateErrorItem(it->second.ev, it->first, it->second.callstackId, it->second.length, it->second.thread),
							ValidateErrorItem(m_allocEv, ev.ptr, m_result->callstacks.AddCallstack(ev), ev.sizeConsumed, ev.threadId)));
					}

					it->second.length = ev.sizeConsumed;
					it->second.callstackId = m_result->callstacks.AddCallstack(ev);
				}

				++ m_allocEv;
			}
			break;

		case RE_Free3:
			{
				const ReplayFree3Event& ev = range.Get<ReplayFree3Event>();
				ApplyFree(ev.ptr, ev.threadId);
			}
			break;

		case RE_Free4:
			{
				const ReplayFree4Event& ev = range.Get<ReplayFree4Event>();
				ApplyFree(ev.ptr, ev.threadId);
			}
			break;
		}

		if (m_result->errors.size() >= MaxErrors)
			return;
	}
}

void AllocValidationQuery::ReplayEnd(u64 position)
{
	using std::swap;

	AllocMap tmp;
	swap(tmp, m_allocs);
}

void AllocValidationQuery::RunImpl(ReplayLogReader& reader)
{
	SharedPtr<ValidateResult> result = new ValidateResult();

	m_result = &*result;
	reader.Replay(*this);
	m_result = NULL;

	Complete(result);
}

void AllocValidationQuery::ApplyFree(TAddress ptr, TThreadId threadId)
{
	AllocMap::iterator it = m_allocs.find(ptr);
	if (it == m_allocs.end())
	{
		AllocMap::iterator resolveIt;
		ptrdiff_t dist = ResolvePointer(ptr, resolveIt);

		if (resolveIt != m_allocs.end())
		{
			// Offset delete.
			m_result->errors.push_back(ValidateError(
				ValidateError::ERR_OffsetFree, 
				ValidateErrorItem(m_allocEv, ptr, 0, 0, threadId),
				ValidateErrorItem(resolveIt->second.ev, resolveIt->first, resolveIt->second.callstackId, resolveIt->second.length, resolveIt->second.thread)));
		}
	}
	else
	{
		if (it->second.length == FreedLength)
		{
			// Double deletion of pointer.
			m_result->errors.push_back(ValidateError(
				ValidateError::ERR_DoubleFree, 
				ValidateErrorItem(it->second.ev, ptr, it->second.callstackId, 0, it->second.thread),
				ValidateErrorItem(m_allocEv, ptr, it->second.callstackId, 0, threadId)));
		}
		else
		{
			it->second.length = FreedLength;
			it->second.ev = m_allocEv;
			it->second.thread = threadId;
		}
	}

	++ m_allocEv;
}

ptrdiff_t AllocValidationQuery::ResolvePointer(TAddress addr, AllocMap::iterator& itOut)
{
	if (m_allocs.empty())
	{
		itOut = m_allocs.end();
		return 0;
	}

	AllocMap::iterator closest = m_allocs.end();
	ptrdiff_t closestDist = 0x7fffffff;

	AllocMap::iterator gteIt = m_allocs.lower_bound(addr);

	if (gteIt == m_allocs.begin())
	{
		itOut = gteIt;
		return gteIt->first - addr;
	}

	AllocMap::iterator ltIt = gteIt; -- ltIt;

	// Check for straddling allocs that contain the search address
	if (ltIt->first <= addr && (ltIt->first + ltIt->second.length) > addr)
	{
		itOut = ltIt;
		return 0;
	}

	ptrdiff_t distToLt = addr - (ltIt->first + ltIt->second.length);
	ptrdiff_t distToGt = gteIt->first - addr;

	if (distToLt < distToGt)
	{
		itOut = ltIt;
		return distToLt;
	}
	else
	{
		itOut = gteIt;
		return distToGt;
	}
}