#pragma once

#include "ColumnTreeWnd.h"
#include "ITabPage.h"

#include "SimpleSplitter.h"
#include "GLTreemapWnd.h"

#include "ReplaySDK/ContextTree.h"
#include "ReplaySDK/ICSVExporter.h"
#include "ReplaySDK/CSVCollator.h"

class CMemReplayDoc;

class ContextTreeMapStyle : public ITreeMapStyle
{
public:
	ContextTreeMapStyle();

	void ShowCount(bool show = true);

public: // ITreeMapStyle Members
	double MeasureNode(const treemapItem* item) const;
	void GetLeafNodeText(const treemapItem* item, char* textOut, size_t textOutCapacity) const;
	void GetNodeColour(const treemapItem* item, int depth, float* colourOut) const;
	void GetPopupText(const treemapItem* item, std::string& popupTextOut) const;
	const char* GetNodeTitle(const treemapItem* item) const;

	void SetMainRootNode(const treemapItem *item) { m_mainNode = item; }
	void SetRSXRootNode(const treemapItem *item) { m_rsxNode = item; }
	void SetContextRoot(const ContextTreeNode *root) { m_root = root; }
	void SetMemoryGroup(MemGroups::Group group) { m_group = group; }

private:
	bool m_showCount;
	MemGroups::Group m_group;
	const treemapItem *m_mainNode;
	const treemapItem *m_rsxNode;
	const ContextTreeNode *m_root;
};

// CContextPage dialog

class CContextPage : public CDialog, public ITabPage
{
	DECLARE_DYNAMIC(CContextPage)

public:
	CContextPage(const SharedPtr<ContextTree>& contextTree, const ICSVExporter* exporter, bool showCount);
	virtual ~CContextPage();

// Dialog Data
	enum { IDD = IDD_ASSETPAGE };

public: // ITabPage Members
	void OnTabActivate();

	void OnInspectClicked();
	void SetMemoryType(int type);
	int GetMemoryType() const;

	void OnFindNext(const char* searchString, u32 flags);
	void Export(ExcelExport& xls);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual void OnSize(UINT nType, int cx, int cy);
	virtual void OnCancel() {}

	DECLARE_MESSAGE_MAP()

private:
	enum
	{
		ID_TREE = 1,
		ID_TREEMAP,
	};

private:
	void Apply();
	void Apply(CTreeCtrl& tree, HTREEITEM treeRoot, const ContextTreeNode* root, int depth);
	HTREEITEM AddTreeItem(CTreeCtrl& tree, HTREEITEM parent, const ContextTreeNode* item);

	void SetTree(const ContextTree& hier, const ContextTreeNode* root);
	treemapItem* CopyTree(const ContextTree& hier, const ContextTreeNode* hierNode, MemGroups::Group memGroup);

	void OnTreeSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult);
	void OnTreeHeaderClicked(NMHDR* pNMHDR, LRESULT* pResult);
	void OnTreeMapActiveItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	void OnTreeItemExpanding(NMHDR* pNMHDR, LRESULT* pResult);

private:
	SharedPtr<ContextTree> m_cgfTree;
	const ICSVExporter* m_exporter;
	bool m_showCount;

	CSimpleSplitter m_splitter;
	CColumnTreeWnd m_tree;
	CGLTreemapWnd m_treeMapWnd;

	ContextTreeMapStyle m_treeMapStyle;

	bool m_applied;
	std::map<HTREEITEM, size_t> m_flatListIndex;
	std::vector<std::pair<const ContextTreeNode*, HTREEITEM> > m_flatList;

	std::map<const ContextTreeNode*, HTREEITEM> m_nodeTreeItemIndex;
	int m_memType;
};

