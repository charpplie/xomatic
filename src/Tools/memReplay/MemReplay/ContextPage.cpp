// AssetPage.cpp : implementation file
//

#include "stdafx.h"
#include "MemReplay.h"
#include "ContextPage.h"
#include "MemReplayDoc.h"
#include "MainFrm.h"
#include "MemReplayView.h"

#include "ReplaySDK/Utility.h"

namespace
{
	struct ContextChildNodeSorter
	{
		bool operator () (const ContextTreeNode* a, const ContextTreeNode* b) const
		{
			return a->GetSize().GetTotal().consumed < b->GetSize().GetTotal().consumed;
		}
	};

	int CALLBACK sortTree_CompareReverseFunc(LPARAM a, LPARAM b, LPARAM col)
	{
		ContextTreeNode* an = reinterpret_cast<ContextTreeNode*>(a);
		ContextTreeNode* bn = reinterpret_cast<ContextTreeNode*>(b);

		switch (col)
		{
		default:
		case 0: return bn->GetSize().GetTotal().consumed - an->GetSize().GetTotal().consumed;
		case 1: return bn->GetSize().GetGroup(MemGroups::Main).consumed - an->GetSize().GetGroup(MemGroups::Main).consumed;
		case 2: return bn->GetSize().GetGroup(MemGroups::RSX).consumed - an->GetSize().GetGroup(MemGroups::RSX).consumed;
		}
	}

	void sortTree(CTreeCtrl& tree, HTREEITEM root, int column)
	{
		tree.LockWindowUpdate();

		std::vector<HTREEITEM> stck;
		stck.reserve(256);

		stck.push_back(root);

		while (stck.empty() == false)
		{
			HTREEITEM node = stck.back();
			stck.pop_back();

			TVSORTCB sorter = {0};
			sorter.hParent = node;
			sorter.lpfnCompare = sortTree_CompareReverseFunc;
			sorter.lParam = column;
			tree.SortChildrenCB(&sorter);

			for (HTREEITEM child = tree.GetChildItem(node); child != NULL; child = tree.GetNextSiblingItem(child))
				stck.push_back(child);
		}

		tree.UnlockWindowUpdate();
	}
}

// CContextPage dialog

IMPLEMENT_DYNAMIC(CContextPage, CDialog)

CContextPage::CContextPage(const SharedPtr<ContextTree>& contextTree, const ICSVExporter* exporter, bool showCount)
	: CDialog(CContextPage::IDD)
	, m_cgfTree(contextTree)
	, m_exporter(exporter)
	, m_showCount(showCount)
	, m_splitter(2, SSP_HORZ)
	, m_applied(false)
	, m_memType(MEMTYPE_COMBINED)
{
	m_treeMapStyle.ShowCount(showCount);
}

CContextPage::~CContextPage()
{
}

void CContextPage::OnTabActivate()
{
	theApp.GetMainFrame()->SetActiveToolBar(CMainFrame::TB_Context);
}

void CContextPage::OnInspectClicked()
{
	CMemReplayView* view = static_cast<CMemReplayView*>(((CMainFrame*)theApp.GetMainWnd())->GetActiveView());

	if (view)
	{
		CTreeCtrl& tree = m_tree.GetTreeCtrl();
		HTREEITEM selectedItem = tree.GetSelectedItem();
		if (selectedItem)
		{
			const ContextTreeNode* node = reinterpret_cast<const ContextTreeNode*>(tree.GetItemData(selectedItem));
			assert (node);

			view->OpenCodeViewFor(*node);
		}
	}
}

void CContextPage::SetMemoryType(int type)
{
	m_memType = type;
	switch(type)
	{
	case MEMTYPE_MAIN:
		m_treeMapStyle.SetMemoryGroup(MemGroups::Main);
		break;
	case MEMTYPE_RSX:
		m_treeMapStyle.SetMemoryGroup(MemGroups::RSX);
		break;
	default:
		m_treeMapStyle.SetMemoryGroup(MemGroups::Count);
		break;
	}
	m_treeMapWnd.RefreshNodeSizes();
}

int CContextPage::GetMemoryType() const
{
	return m_memType;
}

void CContextPage::OnFindNext(const char* searchString, u32 flags)
{
	CTreeCtrl& treeWnd = m_tree.GetTreeCtrl();

	HTREEITEM selectedItem = treeWnd.GetSelectedItem();

	const char* (*strstrFunc)(const char*, const char*) = strstr;
	const char* (*searchFunc)(const char*, const char*) = (flags & FF_CaseInsensitive) ? strstri : strstrFunc;

	if (flags & FF_Forward)
	{
		const std::pair<const ContextTreeNode*, HTREEITEM>* start = selectedItem
			? &m_flatList[m_flatListIndex[selectedItem]] 
		: &m_flatList[0] - 1;

		for (++ start; start != &*m_flatList.begin() + m_flatList.size(); ++ start)
		{
			if ((*searchFunc)(start->first->GetName(), searchString))
			{
				treeWnd.SelectItem(start->second);
				treeWnd.EnsureVisible(start->second);
				return;
			}
		}
	}
	else
	{
		const std::pair<const ContextTreeNode*, HTREEITEM>* start = selectedItem
			? &m_flatList[m_flatListIndex[selectedItem]]
			: &m_flatList[0] + m_flatList.size();

		for (-- start; start != &*m_flatList.begin(); -- start)
		{
			if ((*searchFunc)(start->first->GetName(), searchString))
			{
				treeWnd.SelectItem(start->second);
				treeWnd.EnsureVisible(start->second);
				return;
			}
		}
	}

	treeWnd.SelectItem(NULL);
}

void CContextPage::Export(ExcelExport& xls)
{
	ExcelExportWorksheet ws(xls, m_cgfTree->GetRoot()->GetName());

	if (m_exporter)
	{
		WorksheetCSVCollator collator(ws);
		m_exporter->Export(collator, m_cgfTree->GetRoot());
	}
}

void CContextPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CContextPage, CDialog)
	ON_WM_SIZE()
	ON_NOTIFY(CGLTreemapWnd::OGC_ACTIVEITEMSELECTED, CContextPage::ID_TREEMAP, OnTreeMapActiveItemChanged)
	ON_NOTIFY(TVN_SELCHANGED, CContextPage::ID_TREE, OnTreeSelectionChanged)
	ON_NOTIFY(HDN_ITEMCLICK, CContextPage::ID_TREE, OnTreeHeaderClicked)
	ON_NOTIFY(TVN_ITEMEXPANDING, CContextPage::ID_TREE, OnTreeItemExpanding)
END_MESSAGE_MAP()


// CContextPage message handlers

BOOL CContextPage::OnInitDialog()
{
	BOOL result = CDialog::OnInitDialog();

	m_splitter.Create(this);

	m_splitter.CreatePane(0, &m_tree, WS_CHILD | WS_VISIBLE, 0, ID_TREE);

	m_splitter.CreatePane(1, &m_treeMapWnd, 0, 0, ID_TREEMAP);

	DWORD dwStyle = GetWindowLong(m_tree.GetTreeCtrl().m_hWnd, GWL_STYLE);
	dwStyle |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_FULLROWSELECT | TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS;
	SetWindowLong(m_tree.GetTreeCtrl().m_hWnd, GWL_STYLE, dwStyle);

	CHeaderCtrl& header = m_tree.GetHeaderCtrl();

	while (header.DeleteItem(0));

	HDITEM hditem;
	hditem.mask = HDI_TEXT | HDI_WIDTH | HDI_FORMAT;
	hditem.fmt = HDF_LEFT | HDF_STRING;

	hditem.cxy = 320;
	hditem.pszText = "Name";
	header.InsertItem(0, &hditem);

	hditem.cxy = 100;
	hditem.pszText = "Type";
	header.InsertItem(1, &hditem);

	hditem.cxy = 50;

	hditem.pszText = "Total Size";
	header.InsertItem(2, &hditem);

	hditem.pszText = "Main Size";
	header.InsertItem(3, &hditem);

	hditem.pszText = "RSX Size";
	header.InsertItem(4, &hditem);

	if (m_showCount)
	{
		hditem.cxy = 50;
		hditem.pszText = "Count";
		header.InsertItem(5, &hditem);
	}

	m_tree.UpdateColumns();

	if (!m_applied && m_cgfTree.IsValid())
	{
		Apply();
		m_treeMapWnd.SetStyle(&m_treeMapStyle);
		SetTree(*m_cgfTree, m_cgfTree->GetRoot());

		m_applied = true;
	}

	if (m_cgfTree.IsValid())
	{
		const ContextTreeNode* root = m_cgfTree->GetRoot();

		if (root)
		{
			SetWindowText(root->GetName());
		}
	}

	return result;
}

void CContextPage::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (m_tree.GetSafeHwnd())
	{
		CRect rcClient;
		GetClientRect(&rcClient);

		m_splitter.MoveWindow(rcClient);
	}
}

void CContextPage::Apply()
{
	const ContextTree& hierarchy = *m_cgfTree;
	CTreeCtrl& tree = m_tree.GetTreeCtrl();

	const ContextTreeNode* root = hierarchy.GetRoot();
	if (root == NULL)
		return;

	HTREEITEM rootCat = AddTreeItem(tree, TVI_ROOT, root);
}

void CContextPage::Apply(CTreeCtrl& tree, HTREEITEM treeRoot, const ContextTreeNode* root, int depth)
{
	struct recurseItem
	{
		const ContextTreeNode* branch;
		HTREEITEM hCat;
		int depth;
		recurseItem(const ContextTreeNode* branch, HTREEITEM hCat, int depth)
			: branch(branch)
			, hCat(hCat)
			, depth(depth)
		{}
	};

	std::vector<recurseItem> recStack;
	std::vector<const ContextTreeNode*> children;

	{
		for (const ContextTreeNode* child = root->GetChildren(); child; child = child->GetNextSibling())
			children.push_back(child);

		std::sort(children.begin(), children.end(), ContextChildNodeSorter());

		for (std::vector<const ContextTreeNode*>::iterator childIt = children.begin(), childEnd = children.end(); childIt != childEnd; ++ childIt)
			recStack.push_back(recurseItem(*childIt, treeRoot, 0));
	}

	while (recStack.empty() == false)
	{
		recurseItem item = recStack.back();
		recStack.pop_back();

		HTREEITEM hNewCat = AddTreeItem(tree, item.hCat, item.branch);

		if ((item.depth + 1) < depth)
		{
			children.clear();

			for (const ContextTreeNode* child = item.branch->GetChildren(); child; child = child->GetNextSibling())
				children.push_back(child);

			std::sort(children.begin(), children.end(), ContextChildNodeSorter());

			for (std::vector<const ContextTreeNode*>::iterator childIt = children.begin(), childEnd = children.end(); childIt != childEnd; ++ childIt)
				recStack.push_back(recurseItem(*childIt, hNewCat, item.depth + 1));
		}
	}
}

HTREEITEM CContextPage::AddTreeItem(CTreeCtrl& tree, HTREEITEM parent, const ContextTreeNode* item)
{
	char nameBuffer[1024];
	char totalSize[32], mainSize[32], rsxSize[32];
	FormatSize(totalSize, 32, item->GetSize().GetTotal().consumed);
	FormatSize(mainSize, 32, item->GetSize().GetGroup(MemGroups::Main).consumed);
	FormatSize(rsxSize, 32, item->GetSize().GetGroup(MemGroups::RSX).consumed);

	if (m_showCount)
	{
		sprintf(nameBuffer, "%s\t%s\t%s\t%s\t%s\t%i",
			item->GetName(), MemStatContextTypes::ToString(item->GetType()), totalSize, mainSize, rsxSize, item->GetInstanceCount());
	}
	else
	{
		sprintf(nameBuffer, "%s\t%s\t%s\t%s\t%s",
			item->GetName(), MemStatContextTypes::ToString(item->GetType()), totalSize, mainSize, rsxSize);
	}

	TVINSERTSTRUCT tvis = {0};
	tvis.hInsertAfter = TVI_LAST;
	tvis.hParent = parent;
	tvis.item.cChildren = item->GetChildren() ? 1 : 0;
	//  Attach the hier node to the tree, so it can be looked up later.
	tvis.item.lParam = reinterpret_cast<DWORD_PTR>(item);
	tvis.item.pszText = nameBuffer;
	tvis.item.cchTextMax = strlen(nameBuffer) + 1;
	tvis.item.mask = TVIF_CHILDREN | TVIF_PARAM | TVIF_TEXT;

	HTREEITEM hNewCat = tree.InsertItem(&tvis);

	m_nodeTreeItemIndex.insert(std::make_pair(item, hNewCat));
	m_flatList.push_back(std::make_pair(item, hNewCat));
	m_flatListIndex.insert(std::make_pair(hNewCat, m_flatList.size() - 1));

	return hNewCat;
}

void CContextPage::SetTree(const ContextTree& hier, const ContextTreeNode* root)
{
	if (root)
	{
		treemapItem* treeRoot = new treemapItem();
		treeRoot->size = std::abs(root->GetSize().GetTotal().consumed);
		treeRoot->defaultSize = root->GetSize().GetTotal().consumed;
		treeRoot->childrenCollapse = 1.0f;
		treeRoot->collapseDirection = 0.0f;

		treemapItem* main = new treemapItem();
		main->size = std::abs(root->GetSize().GetGroup(MemGroups::Main).consumed);
		main->defaultSize = root->GetSize().GetGroup(MemGroups::Main).consumed;
		main->children.push_back(CopyTree(hier, root, MemGroups::Main));
		main->childrenCollapse = 1.0f;
		main->collapseDirection = 0.0f;
		m_treeMapStyle.SetMainRootNode(main);

		treeRoot->children.push_back(main);

		if (root->GetSize().GetGroup(MemGroups::RSX).consumed > 0)
		{
			treemapItem* rsx = new treemapItem();
			rsx->size = std::abs(root->GetSize().GetGroup(MemGroups::RSX).consumed);
			rsx->defaultSize = root->GetSize().GetGroup(MemGroups::RSX).consumed;
			rsx->childrenCollapse = 1.0f;
			rsx->collapseDirection = 0.0f;

			rsx->children.push_back(CopyTree(hier, root, MemGroups::RSX));
			m_treeMapStyle.SetRSXRootNode(rsx);
			
			treeRoot->children.push_back(rsx);
		}

		m_treeMapWnd.SetTree(treeRoot);
		m_treeMapStyle.SetContextRoot(root);
	}

	Invalidate(FALSE);
}

treemapItem* CContextPage::CopyTree(const ContextTree& hier, const ContextTreeNode* hierNode, MemGroups::Group memGroup)
{
	treemapItem *parent=new treemapItem();

	parent->size = hierNode->GetSize().GetGroup(memGroup).consumed;
	parent->hasBeenBuilt=FALSE;
	parent->defaultSize = (ptrdiff_t)parent->size;
	if (memGroup==MemGroups::RSX)
		parent->defaultSize=-parent->defaultSize;
	parent->children.clear();
	parent->clientNode = hierNode;
	parent->childrenCollapse=1.0f;
	parent->collapseDirection=0.0f;

	parent->children.reserve(hierNode->CountChildren());

	for (const ContextTreeNode* child = hierNode->GetChildren(); child; child = child->GetNextSibling())
	{
		parent->children.push_back(CopyTree(hier, child, memGroup));
	}

	std::sort(parent->children.begin(), parent->children.end(), TreeMapChildReverseSizeSorter());

	return parent;
}

void CContextPage::OnTreeMapActiveItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	const ContextTreeNode* node = reinterpret_cast<const ContextTreeNode*>(m_treeMapWnd.GetActiveItem());

	if (!node)
		return;

	std::vector<const ContextTreeNode*> stck;

	std::map<const ContextTreeNode*, HTREEITEM>::const_iterator it;
	for (const ContextTreeNode* search = node; search->GetParent(); search = search->GetParent())
	{
		it = m_nodeTreeItemIndex.find(search);
		if (it == m_nodeTreeItemIndex.end())
			stck.push_back(search->GetParent());
	}

	for (std::vector<const ContextTreeNode*>::reverse_iterator stckIt = stck.rbegin(), stckItEnd = stck.rend(); stckIt != stckItEnd; ++ stckIt)
	{
		it = m_nodeTreeItemIndex.find(*stckIt);
		Apply(m_tree.GetTreeCtrl(), it->second, *stckIt, 1);
	}

	it = m_nodeTreeItemIndex.find(node);
	if (it == m_nodeTreeItemIndex.end())
		return;

	CTreeCtrl& tree = m_tree.GetTreeCtrl();

	HTREEITEM curItem = it->second;
	curItem = tree.GetParentItem(curItem);

	while (curItem)
	{
		tree.Expand(curItem, TVE_EXPAND);
		curItem = tree.GetParentItem(curItem);
	}

	tree.EnsureVisible(it->second);
	tree.SelectItem(it->second);
}

void CContextPage::OnTreeSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	CTreeCtrl& tree = m_tree.GetTreeCtrl();

	HTREEITEM item = tree.GetSelectedItem();
	if (item != NULL)
	{
		const ContextTreeNode* node = reinterpret_cast<const ContextTreeNode*>(tree.GetItemData(item));
		m_treeMapWnd.SetActiveItem(node);
	}
}

void CContextPage::OnTreeHeaderClicked(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMHEADER* hdr = reinterpret_cast<NMHEADER*>(pNMHDR);

	if (hdr->iItem >= 2 && hdr->iItem <= 4)
	{
		CTreeCtrl& tree = m_tree.GetTreeCtrl();
		sortTree(tree, tree.GetRootItem(), hdr->iItem - 2);
	}
}

void CContextPage::OnTreeItemExpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMHEADER* hdr = reinterpret_cast<NMHEADER*>(pNMHDR);

	CTreeCtrl& tree = m_tree.GetTreeCtrl();

	NMTREEVIEW* nm = reinterpret_cast<NMTREEVIEW*>(hdr);
	HTREEITEM expanding = nm->itemNew.hItem;

	switch (nm->action)
	{
	case TVE_EXPAND:
		{
			if (tree.GetChildItem(expanding) == NULL)
			{
				std::map<HTREEITEM, size_t>::iterator it = m_flatListIndex.find(expanding);
				if (it != m_flatListIndex.end())
				{
					const ContextTreeNode* node = m_flatList[it->second].first;
					Apply(m_tree.GetTreeCtrl(), expanding, node, 1);
				}
			}
		}
		break;
	}
}

ContextTreeMapStyle::ContextTreeMapStyle()
	: m_showCount(true)
	, m_group(MemGroups::Count)
{
	m_rsxNode = NULL;
	m_mainNode = NULL;
}

void ContextTreeMapStyle::ShowCount(bool show /* = true */)
{
	m_showCount = show;
}

double ContextTreeMapStyle::MeasureNode(const treemapItem* item) const
{
	const ContextTreeNode* node = reinterpret_cast<const ContextTreeNode*>(item->clientNode);
	if (node)
	{
		if (item->defaultSize<0)
			return (m_group==MemGroups::Main)?0:abs(item->defaultSize);
		else
			return (m_group==MemGroups::RSX)?0:item->defaultSize;
	}
	if (item==m_mainNode)
	{
		if (m_group==MemGroups::RSX)
			return 0;
		return m_root->GetSize().GetGroup(MemGroups::Main).consumed;
	}
	else if (item==m_rsxNode)
	{
		if (m_group==MemGroups::Main)
			return 0;
		return m_root->GetSize().GetGroup(MemGroups::RSX).consumed;
	}
	return item->size;
}

void ContextTreeMapStyle::GetLeafNodeText(const treemapItem* item, char* textOut, size_t textOutCapacity) const
{
	const ContextTreeNode* node = reinterpret_cast<const ContextTreeNode*>(item->clientNode);
	if (node)
	{
		FormatSize(textOut, textOutCapacity, abs((ptrdiff_t)item->defaultSize));
	}
}

void ContextTreeMapStyle::GetNodeColour(const treemapItem* item, int depth, float* colourOut) const
{
	CGLTreemapWnd::DefaultColour(item, depth, colourOut);
}

void ContextTreeMapStyle::GetPopupText(const treemapItem* item, std::string& popupTextOut) const
{
	char temp[2028];

	const ContextTreeNode* node = reinterpret_cast<const ContextTreeNode*>(item->clientNode);

	char sizeFmt[64];
	const char* name = "";
	unsigned int count = 1;
	unsigned int size = 0;

	if (node)
	{
		char totalSize[32], mainSize[32], rsxSize[32];
		size=node->GetSize().GetTotal().consumed;
		FormatSize(totalSize, 32, size);
		FormatSize(mainSize, 32, node->GetSize().GetGroup(MemGroups::Main).consumed);
		FormatSize(rsxSize, 32, node->GetSize().GetGroup(MemGroups::RSX).consumed);
		count = node->GetSize().GetTotal().allocCount-node->GetSize().GetTotal().freeCount;
		name = node->GetName();

		sprintf_s(sizeFmt, 64, "%s (%s/%s)", totalSize, mainSize, rsxSize);
	}
	else
	{
		size=abs((ptrdiff_t)item->defaultSize);
		FormatSize(sizeFmt, 64, size);
	}

	if (m_showCount)
	{
		char meanSizeFmt[64];
		FormatSize(meanSizeFmt, 64, (count != 0) ? (size / count) : 0);

		_snprintf_s(temp, 2048, "%s\nCount: %d\nSize: %s\nMean alloc size: %s",
			name,
			count,
			sizeFmt,
			meanSizeFmt);

		popupTextOut = temp;
	}
	else
	{
		_snprintf_s(temp, 2048, "%s\nSize: %s", name, sizeFmt);
	}

	popupTextOut = temp;
}

const char* ContextTreeMapStyle::GetNodeTitle(const treemapItem* item) const
{
	const ContextTreeNode* node = reinterpret_cast<const ContextTreeNode*>(item->clientNode);
	if (node)
	{
		return node->GetName();
	}
	else
	{
		if (!item->children.empty() && !item->children[0]->clientNode)
			return "All";
		if (item==m_mainNode)
			return "Main";
		else if (item=m_rsxNode)
			return "RSX";
		return "";
	}
}
