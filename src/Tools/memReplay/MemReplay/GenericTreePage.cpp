#include "stdafx.h"
#include "MemReplay.h"
#include "CodeTreePage.h"

#include "MainFrm.h"
#include "MemReplayView.h"

namespace
{
	struct GenericLess
	{
		explicit GenericLess(const IGenericColumnSorter* sorter)
			: m_sorter(sorter)
		{
		}

		bool operator () (const GenericTreeNode* a, const GenericTreeNode* b) const
		{
			return m_sorter->Compare(*a, *b) < 0;
		}

		const IGenericColumnSorter* m_sorter;
	};

	struct SortParams
	{
		const std::map<const GenericTreeNode*, size_t>* orderIndex;
	};

	int CALLBACK sortTree_CompareReverseFunc(LPARAM a, LPARAM b, LPARAM col)
	{
		GenericTreeNode* an = reinterpret_cast<GenericTreeNode*>(a);
		GenericTreeNode* bn = reinterpret_cast<GenericTreeNode*>(b);
		SortParams& params = *reinterpret_cast<SortParams*>(col);

		return params.orderIndex->find(an)->second - params.orderIndex->find(bn)->second;
	}

	void sortTree(CTreeCtrl& tree, HTREEITEM root, const SortParams& params)
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
			sorter.lParam = reinterpret_cast<LPARAM>(&params);
			tree.SortChildrenCB(&sorter);

			for (HTREEITEM child = tree.GetChildItem(node); child != NULL; child = tree.GetNextSiblingItem(child))
				stck.push_back(child);
		}

		tree.UnlockWindowUpdate();
	}

	float GetEfficiency(const SizeInfoGroups& sz)
	{
		return (sz.GetTotal().consumed == 0)
			? 100.0f
			: (static_cast<float>(sz.GetTotal().requested) / static_cast<float>(sz.GetTotal().consumed)) * 100.0f;
	}
}

// CGenericTreePage dialog

IMPLEMENT_DYNAMIC(CGenericTreePage, CDialog)

CGenericTreePage::CGenericTreePage(const SharedPtr<GenericTree>& memHier, const std::string& title)
	: CDialog(CGenericTreePage::IDD)
	, m_title(title)
	, m_splitter(2, SSP_HORZ)
	, m_memHier(memHier)
	, m_applied(false)
	, m_currentColumn(-1)
	, m_currentMemType(MEMTYPE_COMBINED)
{
}

CGenericTreePage::~CGenericTreePage()
{
}

GenericTreeNode* CGenericTreePage::GetSelectedNode()
{
	CTreeCtrl& treeWnd = m_tree.GetTreeCtrl();

	HTREEITEM selectedItem = treeWnd.GetSelectedItem();
	if (selectedItem)
	{
		return reinterpret_cast<GenericTreeNode*>(treeWnd.GetItemData(selectedItem));
	}

	return NULL;
}

void CGenericTreePage::SetMemoryType(int type)
{
	m_currentMemType = type;
}

void CGenericTreePage::OnFindNext(const char* searchString, u32 flags)
{
	CTreeCtrl& treeWnd = m_tree.GetTreeCtrl();
	HTREEITEM selectedItem = treeWnd.GetSelectedItem();
	const GenericTreeNode* selectedNode = selectedItem ? reinterpret_cast<const GenericTreeNode*>(treeWnd.GetItemData(selectedItem)) : NULL;

	StrStrHandler strstrFunc = strstr;
	StrStrHandler searchFunc = (flags & FF_CaseInsensitive) ? strstri : strstrFunc;

	if (flags & FF_Forward)
	{
		const GenericTreeNode** start = (selectedNode && (m_nodeFlatListIndex[selectedNode] < m_flatList.size() - 1))
			? &m_flatList[m_nodeFlatListIndex[selectedNode]] 
			: &m_flatList[0] - 1;

		for (++ start; start != &*m_flatList.begin() + m_flatList.size(); ++ start)
		{
			if (FindMatches(searchFunc, searchString, *start))
			{
				std::map<const GenericTreeNode*, HTREEITEM>::iterator it = m_nodeTreeItemIndex.find(*start);
				if (it == m_nodeTreeItemIndex.end())
				{
					Expand(*start);
					it = m_nodeTreeItemIndex.find(*start);
				}

				treeWnd.SelectItem(it->second);
				treeWnd.EnsureVisible(it->second);
				return;
			}
		}
	}
	else
	{
		const GenericTreeNode** start = (selectedNode && (m_nodeFlatListIndex[selectedNode] > 0))
			? &m_flatList[m_nodeFlatListIndex[selectedNode]]
			: &m_flatList[0] + m_flatList.size();

		for (-- start; start != &*m_flatList.begin(); -- start)
		{
			if (FindMatches(searchFunc, searchString, *start))
			{
				std::map<const GenericTreeNode*, HTREEITEM>::iterator it = m_nodeTreeItemIndex.find(*start);
				if (it == m_nodeTreeItemIndex.end())
				{
					Expand(*start);
					it = m_nodeTreeItemIndex.find(*start);
				}

				treeWnd.SelectItem(it->second);
				treeWnd.EnsureVisible(it->second);
				return;
			}
		}
	}

	treeWnd.SelectItem(NULL);
}

void CGenericTreePage::Export(ExcelExport& xls)
{
}

void CGenericTreePage::SetTreeMapStyle(ITreeMapStyle* style)
{
	m_treeMapWnd.SetStyle(style);
}

void CGenericTreePage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CGenericTreePage, CDialog)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_NOTIFY(CGLTreemapWnd::OGC_ACTIVEITEMSELECTED, CGenericTreePage::ID_TREEMAP, OnTreeMapActiveItemChanged)
	ON_NOTIFY(TVN_SELCHANGED, CGenericTreePage::ID_TREE, OnTreeSelectionChanged)
	ON_NOTIFY(HDN_ITEMCLICK, CGenericTreePage::ID_TREE, OnTreeHeaderClicked)
	ON_NOTIFY(TVN_ITEMEXPANDING, CGenericTreePage::ID_TREE, OnTreeItemExpanding)
END_MESSAGE_MAP()

// CGenericTreePage message handlers

BOOL CGenericTreePage::OnInitDialog()
{
	BOOL result = CDialog::OnInitDialog();

	m_splitter.Create(this);

	m_splitter.CreatePane(0, &m_tree, WS_CHILD | WS_VISIBLE, 0, ID_TREE);
	m_splitter.CreatePane(1, &m_treeMapWnd, 0, 0, ID_TREEMAP);

	DWORD dwStyle = GetWindowLong(m_tree.GetTreeCtrl().m_hWnd, GWL_STYLE);
	dwStyle |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_FULLROWSELECT | TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS;
	SetWindowLong(m_tree.GetTreeCtrl().m_hWnd, GWL_STYLE, dwStyle);

	CHeaderCtrl& header = m_tree.GetHeaderCtrl();

	while (header.DeleteItem(0))
		;

	GetColumnDefs(m_columns);

	for (size_t idx = 0, idxCount = m_columns.size(); idx != idxCount; ++ idx)
		header.InsertItem((int) idx, &m_columns[idx].hdr);

	m_tree.UpdateColumns();
	m_currentColumn = -1;

	const IGenericColumnSorter* sorter = NULL;

	for (size_t idx = 0, idxCount = m_columns.size(); idx != idxCount; ++ idx)
	{
		if (m_columns[idx].sorter)
		{
			sorter = m_columns[idx].sorter;
			m_currentColumn = idx;
			break;
		}
	}

	if (!m_applied && m_memHier.IsValid())
	{
		m_flatList.clear();
		ApplyToFindList(m_memHier->GetRoot(), sorter);

		Apply();

		SetTree(*m_memHier, m_memHier->GetRoot());

		m_applied = true;
	}

	SetWindowText(m_title.c_str());

	return result;
}

void CGenericTreePage::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (m_splitter.GetSafeHwnd())
	{
		CRect rcClient;
		GetClientRect(&rcClient);

		m_splitter.MoveWindow(rcClient);
	}
}

void CGenericTreePage::OnPaint()
{
	CPaintDC dc(this);
}

BOOL CGenericTreePage::OnEraseBkgnd(CDC *pDC)
{
	return TRUE;
}

void CGenericTreePage::OnTreeSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	CTreeCtrl& tree = m_tree.GetTreeCtrl();

	HTREEITEM item = tree.GetSelectedItem();
	if (item != NULL)
	{
		const GenericTreeNode* node = reinterpret_cast<const GenericTreeNode*>(tree.GetItemData(item));
		m_treeMapWnd.SetActiveItem(node);
	}
}

void CGenericTreePage::OnTreeHeaderClicked(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMHEADER* hdr = reinterpret_cast<NMHEADER*>(pNMHDR);
	CTreeCtrl& tree = m_tree.GetTreeCtrl();

	SortParams params;
	params.orderIndex = &m_nodeFlatListIndex;

	const IGenericColumnSorter* sorter = m_columns[hdr->iItem].sorter;

	if (sorter)
	{
		m_currentColumn = hdr->iItem;

		m_flatList.clear();
		ApplyToFindList(m_memHier->GetRoot(), sorter);

		sortTree(tree, tree.GetRootItem(), params);
	}
}

void CGenericTreePage::OnTreeMapActiveItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	const GenericTreeNode* node = reinterpret_cast<const GenericTreeNode*>(m_treeMapWnd.GetActiveItem());

	if (!node)
		return;

	Expand(node);

	std::map<const GenericTreeNode*, HTREEITEM>::iterator it = m_nodeTreeItemIndex.find(node);
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

void CGenericTreePage::OnTreeItemExpanding(NMHDR* pNMHDR, LRESULT* pResult)
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
				const GenericTreeNode* node = reinterpret_cast<GenericTreeNode*>(tree.GetItemData(expanding));

				if (node)
					Apply(tree, expanding, node, 1);
			}
		}
		break;
	}
}

void CGenericTreePage::Apply()
{
	CTreeCtrl& tree = m_tree.GetTreeCtrl();

	const GenericTreeNode* root = m_memHier->GetRoot();
	if (root == NULL)
		return;

	AddTreeItem(tree, TVI_ROOT, root);
}

void CGenericTreePage::Apply(CTreeCtrl& tree, HTREEITEM treeRoot, const GenericTreeNode* root, int depth)
{
	struct recurseItem
	{
		const GenericTreeNode* branch;
		HTREEITEM hCat;
		int depth;
		recurseItem(const GenericTreeNode* branch, HTREEITEM hCat, int depth)
			: branch(branch)
			, hCat(hCat)
			, depth(depth)
		{}
	};

	std::vector<recurseItem> recStack;
	std::vector<const GenericTreeNode*> children;

	const GenericTreeStream<SizeInfoGroups>* szStream = m_memHier->GetStream<SizeInfoGroups>("Inclusive");

	tree.LockWindowUpdate();

	{
		for (const GenericTreeNode* child = root->children; child; child = child->sibling)
			children.push_back(child);

		if (m_currentColumn >= 0 && m_columns[m_currentColumn].sorter)
			std::sort(children.begin(), children.end(), GenericLess(m_columns[m_currentColumn].sorter));

		for (std::vector<const GenericTreeNode*>::reverse_iterator childIt = children.rbegin(), childEnd = children.rend(); childIt != childEnd; ++ childIt)
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

			for (const GenericTreeNode* child = item.branch->children; child; child = child->sibling)
				children.push_back(child);

			if (m_currentColumn >= 0 && m_columns[m_currentColumn].sorter)
				std::sort(children.begin(), children.end(), GenericLess(m_columns[m_currentColumn].sorter));

			for (std::vector<const GenericTreeNode*>::iterator childIt = children.begin(), childEnd = children.end(); childIt != childEnd; ++ childIt)
				recStack.push_back(recurseItem(*childIt, hNewCat, item.depth + 1));
		}
	}

	tree.UnlockWindowUpdate();
}

HTREEITEM CGenericTreePage::AddTreeItem(CTreeCtrl& tree, HTREEITEM parent, const GenericTreeNode* item)
{
	char nameBuffer[10240];
	FormatTableItem(nameBuffer, item);

	TVINSERTSTRUCT tvis = {0};
	tvis.hInsertAfter = TVI_LAST;
	tvis.hParent = parent;
	tvis.item.cChildren = item->children ? 1 : 0;
	//  Attach the hier node to the tree, so it can be looked up later.
	tvis.item.lParam = reinterpret_cast<DWORD_PTR>(item);
	tvis.item.pszText = nameBuffer;
	tvis.item.cchTextMax = strlen(nameBuffer) + 1;
	tvis.item.mask = TVIF_CHILDREN | TVIF_PARAM | TVIF_TEXT;

	HTREEITEM hNewCat = tree.InsertItem(&tvis);

	m_nodeTreeItemIndex.insert(std::make_pair(item, hNewCat));

	return hNewCat;
}

void CGenericTreePage::ApplyToFindList(const GenericTreeNode* node, const IGenericColumnSorter* sorter)
{
	m_flatList.push_back(node);
	m_nodeFlatListIndex[node] = m_flatList.size() - 1;

	if (sorter)
	{
		size_t numChildren = node->CountChildren();
		if (numChildren)
		{
			std::vector<const GenericTreeNode*> children;
			children.reserve(numChildren);

			for (const GenericTreeNode* child = node->children; child; child = child->sibling)
				children.push_back(child);

			std::sort(children.begin(), children.end(), GenericLess(sorter));

			for (std::vector<const GenericTreeNode*>::iterator childIt = children.begin(), childEnd = children.end(); childIt != childEnd; ++ childIt)
				ApplyToFindList(*childIt, sorter);
		}
	}
	else
	{
		for (const GenericTreeNode* child = node->children; child; child = child->sibling)
			ApplyToFindList(child, sorter);
	}
}

void CGenericTreePage::Expand(const GenericTreeNode* node)
{
	std::vector<const GenericTreeNode*> stck;

	std::map<const GenericTreeNode*, HTREEITEM>::const_iterator it;
	for (const GenericTreeNode* search = node; search->parent; search = search->parent)
	{
		it = m_nodeTreeItemIndex.find(search);
		if (it == m_nodeTreeItemIndex.end())
			stck.push_back(search->parent);
	}

	for (std::vector<const GenericTreeNode*>::reverse_iterator stckIt = stck.rbegin(), stckItEnd = stck.rend(); stckIt != stckItEnd; ++ stckIt)
	{
		it = m_nodeTreeItemIndex.find(*stckIt);
		Apply(m_tree.GetTreeCtrl(), it->second, *stckIt, 1);
	}
}

CMemReplayView* CGenericTreePage::GetView()
{
	return static_cast<CMemReplayView*>(((CMainFrame*)theApp.GetMainWnd())->GetActiveView());
}

void CGenericTreePage::SetTree(const GenericTree& hier, const GenericTreeNode* root)
{
	if (root)
	{
		const GenericTreeStream<SizeInfoGroups>* szStream = reinterpret_cast<const GenericTreeStream<SizeInfoGroups>*>(hier.GetStream("Inclusive"));
		if (szStream)
		{
			m_treeMapWnd.SetTree(CopyTree(hier, *szStream, root));
		}
	}
}

treemapItem* CGenericTreePage::CopyTree(const GenericTree& hier, const GenericTreeStream<SizeInfoGroups>& inclusiveSizeStream, const GenericTreeNode* hierNode)
{
	int numChildren=0;
	treemapItem *parent=new treemapItem();

	const SizeInfoGroups& sz = (*hierNode)[inclusiveSizeStream];

	parent->size = std::abs(sz.GetTotal().consumed);
	parent->hasBeenBuilt=FALSE;
	parent->defaultSize = sz.GetTotal().consumed;
	parent->children.clear();
	parent->clientNode = hierNode;

	//if (std::abs(hierNode->GetAmountConsumed()) >= (500 * 1024))
	{
		parent->children.reserve(hierNode->CountChildren());

		for (const GenericTreeNode* child = hierNode->children; child; child = child->sibling)
		{
			parent->children.push_back(CopyTree(hier, inclusiveSizeStream, child));
			++ numChildren;
		}
	}

	if (numChildren==1 && parent->children[0]->children.size()==1)
	{
		parent->childrenCollapse=0.0f;
		parent->collapseDirection=-1.0f;
		parent->children[0]->childrenCollapse=1.0f;
		if (parent->children[0]->collapseDirection!=0.0f)
			parent->children[0]->collapseDirection=1.0f;
	}
	else
	{
		parent->childrenCollapse=1.0f;
		parent->collapseDirection=0.0f;
	}

	std::sort(parent->children.begin(), parent->children.end(), TreeMapChildReverseSizeSorter());

	return parent;
}
