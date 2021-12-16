#pragma once

#include "ReplayQuery.h"
#include "GenericTree.h"

class FreedSpaceQuery : public ReplayQuery<SharedPtr<GenericTree> >
{
public:
	FreedSpaceQuery(u64 allocEvEnd, const std::vector<int>& limitToBuckets);

private:
	void RunImpl(ReplayLogReader& reader);

private:
	u64 m_allocEvEnd;
	std::vector<int> m_limitToBuckets;
};
