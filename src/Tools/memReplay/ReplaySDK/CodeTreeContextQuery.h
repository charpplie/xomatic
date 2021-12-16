#pragma once

#include "ReplayQuery.h"

class CodeTreeContextQuery : public ReplayQuery<SharedPtr<GenericTree> >
{
public:
	CodeTreeContextQuery(const std::vector<ContextStreamOffsetSpan>& spans, bool includeAllocs, bool includeFrees, SharedPtr<SymbolHelper> symbols)
		: m_spans(spans)
		, m_includeAllocs(includeAllocs)
		, m_includeFrees(includeFrees)
		, m_symbols(symbols)
	{
	}

	void RunImpl(ReplayLogReader& reader);

private:
	std::vector<ContextStreamOffsetSpan> m_spans;
	bool m_includeAllocs;
	bool m_includeFrees;
	SharedPtr<SymbolHelper> m_symbols;
};
