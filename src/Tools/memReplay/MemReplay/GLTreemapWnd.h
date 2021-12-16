#pragma once
#include "afxwin.h"

#include "treemap.h"

#include <gl/gl.h>
#include <gl/glu.h>
#include "GLFont.h"

#include "IIdleHandler.h"

struct treemapItem
{
	void* operator new (size_t sz);
	void operator delete (void* p, size_t sz);

	treemapItem()
		: hasBeenBuilt(FALSE)
		, clientNode(NULL)
	{
	}

	double oldSize;
	double size;
	double defaultSize;
	float builtW;
	float builtH;
	float	childrenCollapse;
	float	collapseDirection;
	int   hasBeenBuilt;
	treemapnode node;
	std::vector<treemapItem*> children;
	const void* clientNode;
};

struct TreeMapChildReverseSizeSorter
{
	bool operator () (const treemapItem* a, const treemapItem* b) const
	{
		return b->size < a->size;
	}
};

inline void freeTree(treemapItem *item)
{
	for (size_t i=0; i<item->children.size(); i++)
		freeTree(item->children[i]);

	delete item;
}

class CMemReplayView;

class ITreeMapStyle
{
public:
	virtual ~ITreeMapStyle() {}

	virtual double MeasureNode(const treemapItem* item) const = 0;
	virtual void GetLeafNodeText(const treemapItem* item, char* textOut, size_t textOutCapacity) const = 0;
	virtual void GetNodeColour(const treemapItem* item, int depth, float* colourOut) const = 0;
	virtual void GetPopupText(const treemapItem* item, std::string& popupTextOut) const = 0;
	virtual const char* GetNodeTitle(const treemapItem* item) const = 0;
};

class CGLTreemapWnd : public CWnd, public IIdleHandler
{
public:
	enum
	{
		OGC_ACTIVEITEMCHANGED = WM_USER + 1,
		OGC_ACTIVEITEMSELECTED,
	};
	
public:
	static void DefaultColour(const treemapItem* item, int depth, float* col);

public:
	CGLTreemapWnd();
	virtual ~CGLTreemapWnd();

	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam = NULL);

	void oglCreate(CRect rect, CWnd *parent, UINT id);
	void oglInitialize(void);
	void oglDrawScene(void);

	void SetStyle(const ITreeMapStyle* style);

	void SetTree(treemapItem* tree);

	void FindAddress(TAddress address);
	void FindAddressNext();

	void Zoom(short zDelta, const CPoint& pt);

	const void* GetActiveItem() const { return m_liveNode ? m_liveNode->clientNode : NULL; }
	void SetActiveItem(const void* clientNode);
	void RefreshNodeSizes(void);

public:
	void OnIdle();

protected:
	// Added message classes:
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();

	DECLARE_MESSAGE_MAP()

private:
	typedef std::map<const void*, treemapItem*> ClientNodeMap;

private:
	void drawTree(float x, float y, float w, float h, int depth);
	void drawTreeFromControl(treemapItem *item, float x, float y, float w, float h, int depth, int parentHadChildren, float collapse);
	CMemReplayView* GetView(void);
	void freeTree(treemapItem *item);

	void GetPopupText(const treemapItem* item, std::string& popupTextOut);
	void RaiseActiveItemChanged();
	void RaiseActiveItemSelected();
	float GetSize(const treemapItem * item);
	void RecurseRefreshNodeSizes(treemapItem *r);

private:
	HDC   hdc;			
	HGLRC hrc;			

	float m_x, m_y;
	float m_zoom;
	float m_width;
	float m_height;
	LONG  m_mouseX;
	LONG  m_mouseY;
	int		m_findZoomPoint;
	int		m_expand;
	float m_zoomTargetX;
	float m_zoomTargetY;
	float m_zoomTarget;
	int		m_zooming;
	treemapItem* m_zoomTargetItem;
	int		m_firstRender;

	treemapItem *m_treeRoot;
	treemapItem *m_liveNode;
	GLFont m_font;

	float m_popupScale;
	int m_rebuildNeeded;
	int m_collapsing;
	float m_popupFade;
	int m_moved;
	int m_growing;
	float m_collapsedAmount;
	float m_3dOffset;

	const ITreeMapStyle* m_style;

	treemapItem* m_lastPopupNode;
	std::string m_popupText;

	float m_groupFilter;
	float m_groupRatio;

	ClientNodeMap m_clientNodeIndex;
};