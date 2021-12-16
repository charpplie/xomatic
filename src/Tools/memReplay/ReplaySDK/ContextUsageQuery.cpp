#include "stdafx.h"
#include "ContextUsageQuery.h"

#include "ReplayVisitor.h"

ContextUsageQuery::ContextUsageQuery()
{
}

void ContextUsageQuery::ReplayBegin()
{
}

void ContextUsageQuery::Replay(ReplayRange range)
{
	ReplayVisitor<ContextUsageQuery> visitor(*this);
	visitor.Replay(range);
}

void ContextUsageQuery::ReplayEnd(u64 position)
{
}

void ContextUsageQuery::RunImpl(ReplayLogReader& reader)
{
	reader.Replay(*this);
	Complete(ResultType());
}

void ContextUsageQuery::ReplayEvent(const ReplayAlloc3Event& ev)
{
}

void ContextUsageQuery::ReplayEvent(const ReplayFree3Event& ev)
{
}

void ContextUsageQuery::ReplayEvent(const ReplayPushContext3Event& ev)
{
}

void ContextUsageQuery::ReplayEvent(const ReplayPopContextEvent& ev)
{
}
