#pragma once

#include "ITabPage.h"
#include "ColumnTreeWnd.h"
#include "GLTreemapWnd.h"
#include "SimpleSplitter.h"
#include "resource.h"

#include "ReplaySDK/GenericTree.h"
#include "ReplaySDK/ISymbolTable.h"

class IGenericColumnSorter
{
public:
	virtual ~IGenericColumnSorter() {}

	virtual int Compare(const GenericTreeNode& a, const GenericTreeNode& b) const = 0;
};

class GenericSizeColumnSorter : public IGenericColumnSorter
{
public:
	GenericSizeColumnSorter(const GenericTreeStream<SizeInfoGroups>& stream, MemGroups::Group group)
		: m_stream(&stream)
		, m_group(group)
	{
	}

	int Compare(const GenericTreeNode& a, const GenericTreeNode& b) const
	{
		if ((int) m_group < 0)
			return b[*m_stream].GetTotal().consumed - a[*m_stream].GetTotal().consumed;
		else
			return b[*m_stream].GetGroup(m_group).consumed - a[*m_stream].GetGroup(m_group).consumed;
	}

private:
	const GenericTreeStream<SizeInfoGroups>* m_stream;
	MemGroups::Group m_group;
};

class GenericCountColumnSorter : public IGenericColumnSorter
{
public:
	enum Field
	{
		Total,
		Allocs,
		Frees,
	};

public:
	GenericCountColumnSorter(const GenericTreeStream<SizeInfoGroups>& stream, MemGroups::Group group, Field field)
		: m_stream(&stream)
		, m_group(group)
		, m_field(field)
	{
	}

	int Compare(const GenericTreeNode& a, const GenericTreeNode& b) const
	{
		const SizeInfo* szA;
		const SizeInfo* szB;

		if ((int) m_group < 0)
		{
			szA = &a[*m_stream].GetTotal(); 
			szB = &b[*m_stream].GetTotal(); 
		}
		else
		{
			szA = &a[*m_stream].GetGroup(m_group);
			szB = &b[*m_stream].GetGroup(m_group);
		}

		switch (m_field)
		{
		case Total: return (szB->allocCount - szB->freeCount) - (szA->allocCount - szA->freeCount);
		case Allocs: return szB->allocCount - szA->allocCount;
		case Frees: return szB->freeCount - szA->freeCount;
		}

		return 0;
	}

private:
	const GenericTreeStream<SizeInfoGroups>* m_stream;
	MemGroups::Group m_group;
	Field m_field;
};

// CGenericTreePage dialog

class CGenericTreePage : public CDialog, public ITabPage
{
	DECLARE_DYNAMIC(CGenericTreePage)

public:
	CGenericTreePage(const SharedPtr<GenericTree>& memHier, const std::string& name);
	virtual ~CGenericTreePage();

	GenericTreeNode* GetSelectedNode();
	GenericTree* GetTree() { return &*m_memHier; }

	const char* GetTitle() const { return m_title.c_str(); }

public: // ITabPage Members
	void SetMemoryType(int type);
	int GetMemoryType() const { return m_currentMemType; }

	void OnFindNext(const char* searchString, u32 flags);
	void Export(ExcelExport& xls);

// Dialog Data
	enum { IDD = IDD_CODETREEPAGE };

protected:
	typedef const char* (*StrStrHandler)(const char*, const char*);

	struct ColumnDef
	{
		HDITEM hdr;
		const IGenericColumnSorter* sorter;
	};

protected:
	static CMemReplayView* GetView();

protected:
	void SetTreeMapStyle(ITreeMapStyle* style);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual void OnSize(UINT nType, int cx, int cy);
	virtual void OnPaint();
	virtual BOOL OnEraseBkgnd(CDC* pDC);
	virtual void OnCancel() {}

	void OnTreeSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult);
	void OnTreeHeaderClicked(NMHDR* pNMHDR, LRESULT* pResult);
	void OnTreeMapActiveItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	void OnTreeItemExpanding(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()

private:
	enum
	{
		ID_TREE = 1,
		ID_TREEMAP,
	};

private:
	virtual bool FindMatches(StrStrHandler searchFunc, const char* searchString, const GenericTreeNode* searchNode) = 0;
	virtual void FormatTableItem(char* nameBuffer, const GenericTreeNode* item) = 0;
	virtual void GetColumnDefs(std::vector<ColumnDef>& columns) = 0;

private:
	void Apply();
	void Apply(CTreeCtrl& tree, HTREEITEM treeRoot, const GenericTreeNode* root, int depth);
	HTREEITEM AddTreeItem(CTreeCtrl& tree, HTREEITEM parent, const GenericTreeNode* item);

	void ApplyToFindList(const GenericTreeNode* node, const IGenericColumnSorter* sorter);
	void Expand(const GenericTreeNode* node);

	void SetTree(const GenericTree& hier, const GenericTreeNode* root);
	treemapItem* CopyTree(const GenericTree& hier, const GenericTreeStream<SizeInfoGroups>& inclusiveSizeStream, const GenericTreeNode* hierNode);
	
protected:
	CGLTreemapWnd m_treeMapWnd;
	int m_currentMemType;

private:
	std::string m_title;

	CSimpleSplitter m_splitter;
	CColumnTreeWnd m_tree;
	std::vector<ColumnDef> m_columns;

	std::map<const GenericTreeNode*, HTREEITEM> m_nodeTreeItemIndex;

	SharedPtr<GenericTree> m_memHier;

	bool m_applied;
	int m_currentColumn;
	std::map<const GenericTreeNode*, size_t> m_nodeFlatListIndex;
	std::vector<const GenericTreeNode*> m_flatList;
};
