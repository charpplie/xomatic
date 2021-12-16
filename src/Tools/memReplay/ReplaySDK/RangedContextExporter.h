#pragma once

#include "ICSVExporter.h"

class RangedContextExporter : public ICSVExporter
{
public:
	RangedContextExporter(int depthBegin, int depthEnd, const char** columns, bool inclCount);

	void Export(ICSVCollator& collator, const ContextTreeNode* node) const;

private:
	int m_depthBegin;
	int m_depthEnd;
	const char** m_columns;
	bool m_inclCount;
};
