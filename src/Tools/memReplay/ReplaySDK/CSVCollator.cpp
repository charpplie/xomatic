#include "stdafx.h"

#include "CSVCollator.h"

#include <xutility>

#include "ContextTree.h"
#include "CleanString.h"

void FindMaxDepth(const ContextTreeNode* a, int& depth)
{
	int thisDepth = depth;

	for (const ContextTreeNode* child = a->GetChildren(); child; child = child->GetNextSibling())
	{
		int childDepth = thisDepth + 1;
		FindMaxDepth(child, childDepth);
		depth = std::max(depth, childDepth);
	}
}

struct ContextChildNodeSorter
{
	bool operator () (const ContextTreeNode* a, const ContextTreeNode* b) const
	{
		return a->GetSize().GetTotal().requested < b->GetSize().GetTotal().requested;
	}
};

void ExportNodeToCSV(ICSVCollator& collator, const ContextTreeNode* a, const char** heading, int depth, int maxDepth)
{
	std::vector<const char*> cols(maxDepth + 3);

	char size[32];
	sprintf_s(size, 32, "%i", a->GetSize().GetTotal().consumed);

	char count[32];
	sprintf_s(count, 32, "%i", a->GetInstanceCount());

	if (depth > 0)
		std::copy(heading, heading + depth, cols.begin());

	if (maxDepth - depth > 0)
		std::fill(cols.begin() + depth, cols.begin() + maxDepth, "");

	cols[depth] = a->GetName();
	cols[maxDepth] = MemStatContextTypes::ToString(a->GetType());
	cols[maxDepth + 1] = size;
	cols[maxDepth + 2] = count;

	collator.AddRow(&cols[0], cols.size());

	std::vector<const ContextTreeNode*> children;
	for (const ContextTreeNode* child = a->GetChildren(); child; child = child->GetNextSibling())
		children.push_back(child);

	std::sort(children.rbegin(), children.rend(), ContextChildNodeSorter());

	for (std::vector<const ContextTreeNode*>::const_iterator it = children.begin(), itEnd = children.end();
		it != itEnd;
		++ it)
	{
		ExportNodeToCSV(collator, *it, &cols[0], depth + 1, maxDepth);
	}
}

void ContextTreeCSVExporter::Export(ICSVCollator& collator, const ContextTreeNode* node) const
{
	const ContextTreeNode* a = node;

	int maxDepth = 1;
	FindMaxDepth(a, maxDepth);

	int numColumns = maxDepth + 3;

	std::vector<const char*> cols(numColumns);

	std::fill(cols.begin(), cols.begin() + maxDepth, "");
	cols[maxDepth] = "Type";
	cols[maxDepth + 1] = "Size";
	cols[maxDepth + 2] = "Count";

	collator.AddRow(&cols[0], cols.size());

	ExportNodeToCSV(collator, a, NULL, 0, maxDepth);
}

WorksheetCSVCollator::WorksheetCSVCollator(ExcelExportWorksheet& ws)
	: m_ws(&ws)
	, m_firstRow(true)
{
}

void WorksheetCSVCollator::AddRow(const char** cols, size_t numCols)
{
	if (m_firstRow)
	{
		for (size_t i = 0; i != numCols; ++ i)
			m_ws->AddColumn(100);
		m_firstRow = false;
	}

	ExcelExportRow row(*m_ws);

	for (size_t i = 0; i != numCols; ++ i)
	{
		int num;
		char* numend;
		num = strtol(cols[i], &numend, 10);

		if (*numend || !cols[i][0])
		{
			const int len = strlen(cols[i]);
			char* const pClean = new char[len + 1];
			CleanString(pClean, cols[i]);
			row.AddCell(pClean);
			delete [] pClean;
		}
		else
		{
			row.AddCell(num);
		}
	}
}
