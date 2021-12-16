#include "stdafx.h"
#include "CGFPhysicsExporter.h"

#include "ContextTree.h"

void CGFPhysicsCSVExporter::Export(ICSVCollator& collator, const ContextTreeNode* node) const
{
	const char* headers[] = {
		"MAX", "CGF", "Size (Main)", "Size (RSX)", "Size (Total)"
	};

	collator.AddRow(headers, 5);

	std::vector<const ContextTreeNode*> nextList;
	nextList.reserve(64);

	nextList.push_back(node);

	while (!nextList.empty())
	{
		const ContextTreeNode* top = nextList.back();
		nextList.pop_back();

		if (top->GetType() == MemStatContextTypes::MSC_CGF)
		{
			char sizeMain[32], sizeRSX[32], sizeTotal[32];
			sprintf_s(sizeMain, 32, "%i", top->GetSize().GetGroup(MemGroups::Main).requested);
			sprintf_s(sizeRSX, 32, "%i", top->GetSize().GetGroup(MemGroups::RSX).requested);
			sprintf_s(sizeTotal, 32, "%i", top->GetSize().GetTotal().requested);

			if (top->GetParent() && top->GetParent()->GetType() == MemStatContextTypes::MSC_MAX)
			{
				const char* col[] = 
				{
					top->GetParent()->GetName(),
					top->GetName(),
					sizeMain,
					sizeRSX,
					sizeTotal
				};

				collator.AddRow(col, 5);
			}
			else
			{
				const char* col[] = 
				{
					"",
					top->GetName(),
					sizeMain,
					sizeRSX,
					sizeTotal
				};

				collator.AddRow(col, 5);
			}
		}

		for (const ContextTreeNode* child = top->GetChildren(); child; child = child->GetNextSibling())
			nextList.push_back(child);
	}
}
