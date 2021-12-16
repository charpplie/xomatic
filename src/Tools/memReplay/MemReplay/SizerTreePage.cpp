#include "stdafx.h"
#include "SizerTreePage.h"

#include "MainFrm.h"
#include "MemReplay.h"
#include "MemReplayView.h"

SizerTreeMapStyle::SizerTreeMapStyle(const GenericTreeStream<SizeInfoGroups>& szInclusiveStream)
	: m_szInclusiveStream(&szInclusiveStream)
{
}

double SizerTreeMapStyle::MeasureNode(const treemapItem* item) const
{
	const GenericTreeNode* node = reinterpret_cast<const GenericTreeNode*>(item->clientNode);
	if (node)
	{
		return abs((*node)[*m_szInclusiveStream].GetTotal().consumed);
	}
	else
	{
		return item->size;
	}
}

void SizerTreeMapStyle::GetLeafNodeText(const treemapItem* item, char* textOut, size_t textOutCapacity) const
{
	const GenericTreeNode* node = reinterpret_cast<const GenericTreeNode*>(item->clientNode);
	if (node)
	{
		const SizeInfoGroups& sz = (*m_szInclusiveStream)[node->id];

		char totalSize[32], mainSize[32], rsxSize[32];
		FormatSize(totalSize, sizeof(totalSize), sz.GetTotal().consumed);
		FormatSize(mainSize, 32, sz.GetGroup(MemGroups::Main).consumed);
		FormatSize(rsxSize, 32, sz.GetGroup(MemGroups::RSX).consumed);

		sprintf_s(textOut, textOutCapacity, "%s (%s/%s)", totalSize, mainSize, rsxSize);
	}
	else
	{
		FormatSize(textOut, textOutCapacity, (ptrdiff_t)item->size);
	}
}

void SizerTreeMapStyle::GetNodeColour(const treemapItem* node, int depth, float* col) const
{
	if (node->children.empty())
	{
		float eff = 1.0f;
		col[0] = 1.0f - eff;
		col[1] = eff;
		col[2] = 0.0f;
		col[3] = 1.0f;
	}
	else
	{
		CGLTreemapWnd::DefaultColour(node, depth, col);
	}
}

void SizerTreeMapStyle::GetPopupText(const treemapItem* item, std::string& popupTextOut) const
{
	char temp[2028];

	const GenericTreeNode* node = reinterpret_cast<const GenericTreeNode*>(item->clientNode);

	char sizeFmt[64];
	unsigned int count=1;
	unsigned int size=0;
	
	if (node)
	{
		const SizeInfoGroups& sz = (*node)[*m_szInclusiveStream];

		char totalSize[32], mainSize[32], rsxSize[32];
		size = sz.GetTotal().consumed;
		count = sz.GetTotal().allocCount-sz.GetTotal().freeCount;
		FormatSize(totalSize, sizeof(totalSize), size);
		FormatSize(mainSize, sizeof(mainSize), sz.GetGroup(MemGroups::Main).consumed);
		FormatSize(rsxSize, sizeof(rsxSize), sz.GetGroup(MemGroups::RSX).consumed);

		const GenericTreeNode* root = node;
		while (root->parent)
			root = root->parent;

		float percent = (sz.GetTotal().consumed * 100.0f) / (*root)[*m_szInclusiveStream].GetTotal().consumed;
		sprintf_s(sizeFmt, sizeof(sizeFmt), "%s (%s/%s) %f%%", totalSize, mainSize, rsxSize, percent);
	}
	else
	{
		FormatSize(sizeFmt, 64, (ptrdiff_t)item->defaultSize);
	}

	char meanSizeFmt[64];
	FormatSize(meanSizeFmt, 64, (count != 0) ? (size / count) : 0);

	const char* name = node->GetKey<const char*>();

	_snprintf_s(temp, 2048, "%s\n\nCount: %d\nSize: %s\nMean alloc size: %s\n",
		name, 
		count,
		sizeFmt,
		meanSizeFmt);

	popupTextOut = temp;
}

const char* SizerTreeMapStyle::GetNodeTitle(const treemapItem* item) const
{
	const GenericTreeNode* node = reinterpret_cast<const GenericTreeNode*>(item->clientNode);
	const char* title = node->GetKey<const char*>();
	return title ? title : "";
}

CSizerTreePage::CSizerTreePage(const SharedPtr<GenericTree>& tree, const std::string& name)
	: CGenericTreePage(tree, name)
	, m_style(*tree->GetStream<SizeInfoGroups>("Inclusive"))
{
	SetTreeMapStyle(&m_style);
}

void CSizerTreePage::OnTabActivate()
{
	theApp.GetMainFrame()->SetActiveToolBar(CMainFrame::TB_None);
}

void CSizerTreePage::OnInspectClicked()
{
	const GenericTreeNode* selected = GetSelectedNode();
	CMemReplayView* view = GetView();

	if (view && selected)
	{
		view->OpenCodeQuery_Sizer(*GetTree(), selected);
	}
}

void CSizerTreePage::OnReverseClicked()
{
}

void CSizerTreePage::OnGatherTDClicked(GatherType gt)
{
}

void CSizerTreePage::OnExcludeClicked(GatherType gt)
{
}

bool CSizerTreePage::FindMatches(StrStrHandler searchFunc, const char* searchString, const GenericTreeNode* searchNode)
{
	const char* name = searchNode->GetKey<const char*>();

	if (name && (*searchFunc)(name, searchString))
		return true;

	return false;
}

void CSizerTreePage::FormatTableItem(char* nameBuffer, const GenericTreeNode* item)
{
	GenericTree& tree = *GetTree();
	GenericTreeStream<SizeInfoGroups>& szInclusiveStream = *tree.GetStream<SizeInfoGroups>("Inclusive");

	const char* name = item->GetKey<const char*>();
	const SizeInfoGroups& szInclusive = (*item)[szInclusiveStream];

	char sizeTotalStr[32], sizeMainStr[32], sizeRSXStr[32];
	FormatSize(sizeTotalStr, 32, szInclusive.GetTotal().consumed);
	FormatSize(sizeMainStr, 32, szInclusive.GetGroup(MemGroups::Main).consumed);
	FormatSize(sizeRSXStr, 32, szInclusive.GetGroup(MemGroups::RSX).consumed);

	int count = szInclusive.GetTotal().allocCount - szInclusive.GetTotal().freeCount;
	sprintf(nameBuffer, "%s\t%i\t%s\t%s\t%s", name, count, sizeTotalStr, sizeMainStr, sizeRSXStr);
}

void CSizerTreePage::GetColumnDefs(std::vector<ColumnDef>& headers)
{
	ColumnDef col = {0};
	col.hdr.mask = HDI_TEXT | HDI_WIDTH | HDI_FORMAT;
	col.hdr.fmt = HDF_LEFT | HDF_STRING;

	headers.clear();
	headers.resize(5, col);

	headers[0].hdr.cxy = 300;
	headers[0].hdr.pszText = "Name";

	headers[1].hdr.cxy = 50;
	headers[1].hdr.pszText = "Allocs";

	headers[2].hdr.cxy = 50;
	headers[2].hdr.pszText = "Total Size";

	headers[3].hdr.cxy = 50;
	headers[3].hdr.pszText = "Main Size";

	headers[4].hdr.cxy = 50;
	headers[4].hdr.pszText = "RSX Size";
}
