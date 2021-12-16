#include "stdafx.h"
#include "RangedContextExporter.h"

#include "ContextTree.h"

RangedContextExporter::RangedContextExporter(int depthBegin, int depthEnd, const char** columns, bool inclCount)
	: m_depthBegin(depthBegin)
	, m_depthEnd(depthEnd)
	, m_columns(columns)
	, m_inclCount(inclCount)
{
}

void RangedContextExporter::Export(ICSVCollator& collator, const ContextTreeNode* node) const
{
	int range = (m_depthEnd - m_depthBegin);
	std::vector<const char*> row(range + 3 + (m_inclCount ? 1 : 0));
	
	for (int i = 0; i < range; ++ i)
		row[i] = m_columns[i];

	row[range] = "Size (Main)";
	row[range + 1] = "Size (RSX)";
	row[range + 2] = "Size (Total)";
	if (m_inclCount)
		row[range + 3] = "Count";

	collator.AddRow(&row[0], row.size());

	struct item
	{
		item(const ContextTreeNode* node, int depth)
			: node(node), depth(depth) {}
		const ContextTreeNode* node;
		int depth;
	};

	std::vector<item> nextList;
	nextList.reserve(64);

	nextList.push_back(item(node, 0));

	while (!nextList.empty())
	{
		item top = nextList.back();
		nextList.pop_back();

		if (top.depth == m_depthEnd - 1)
		{
			if (top.node->GetSize().GetTotal().requested > 0)
			{
				const ContextTreeNode* wn = top.node;

				for (int i = m_depthEnd - 1; i >= m_depthBegin; -- i, wn = wn->GetParent())
				{
					row[i - m_depthBegin] = wn->GetName();
				}

				const SizeInfoGroups& sz = top.node->GetSize();

				char szMain[32], szRSX[32], szTotal[32];
				sprintf_s(szMain, sizeof(szMain), "%i", sz.GetGroup(MemGroups::Main).requested);
				sprintf_s(szRSX, sizeof(szRSX), "%i", sz.GetGroup(MemGroups::RSX).requested);
				sprintf_s(szTotal, sizeof(szTotal), "%i", sz.GetTotal().requested);

				row[range] = szMain;
				row[range + 1] = szRSX;
				row[range + 2] = szTotal;

				if (m_inclCount)
				{
					char cnt[32];
					sprintf_s(cnt, sizeof(cnt), "%i", top.node->GetInstanceCount());
					row[range + 3] = cnt;
				}

				collator.AddRow(&row[0], row.size());
			}
		}
		else
		{
			for (const ContextTreeNode* child = top.node->GetChildren(); child; child = child->GetNextSibling())
				nextList.push_back(item(child, top.depth + 1));
		}
	}
}
