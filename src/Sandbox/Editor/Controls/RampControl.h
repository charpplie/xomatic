#pragma once

#include <vector>

// RampControl

static UINT NEAR WM_HANDLECHANGED = RegisterWindowMessage("HANDLECHANGED");
static UINT NEAR WM_HANDLEDELETED = RegisterWindowMessage("HANDLEDELETED");
static UINT NEAR WM_HANDLEADDED = RegisterWindowMessage("HANDLEADDED");
static UINT NEAR WM_HANDLESELECTED = RegisterWindowMessage("HANDLESELECTED");
static UINT NEAR WM_HANDLESELECTIONCLEARED = RegisterWindowMessage("HANDLESELECTIONCLEAR");

class CRampControl : public CSliderCtrl
{
	DECLARE_DYNAMIC(CRampControl)

public:
	enum HandleFlags
	{
		hfSelected = 1<<0,
		hfHotitem = 1<<1,
	};

	struct HandleData
	{
		float percentage;
		int flags;
	};
	
private:
	#define ID_MENU_ADD				33000
	#define ID_MENU_SELECT			33001
	#define ID_MENU_DELETE			33002
	#define ID_MENU_SELECTALL		33003
	#define ID_MENU_SELECTNONE		33004
	#define ID_MENU_CUSTOM_BEGIN	33005
	#define ID_MENU_CUSTOM_END		33015
	
public:
	CRampControl();
	virtual ~CRampControl();
	BOOL RegisterControlClass();

protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnPaint();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnMenuAdd();
	afx_msg void OnMenuSelect();
	afx_msg void OnMenuDelete();
	afx_msg void OnMenuSelectAll();
	afx_msg void OnMenuSelectNone();
	afx_msg virtual void OnMenuCustom(UINT nID);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	virtual void DrawRamp(CDC &dc);
	virtual void DrawHandle(CDC &dc,const HandleData& data);
	virtual void DrawBackground(CDC &dc);
	virtual void DrawForeground(CDC &dc);

	virtual void AddCustomMenuOptions(CMenu * menu);

	virtual float DeterminePercentage(const CPoint& point);
	virtual bool AddHandle(const float percent);
	virtual void SortHandles();
	static bool SortHandle(const CRampControl::HandleData& dataA, const CRampControl::HandleData& dataB);
	virtual void ClearSelectedHandles();
	virtual void Reset();
	virtual bool SelectHandle(const float percent);
	virtual bool DeSelectHandle(const float percent);
	virtual bool IsSelectedHandle(const float percent);
	virtual void SelectAllHandles();
	virtual float MoveHandle(HandleData& data, const float percent);
	virtual bool HitAnyHandles(const float percent);
	virtual bool HitTestHandle(const HandleData& data, const float percent);
	static bool CheckSelected(const HandleData& data);
	virtual void RemoveSelectedHandles();
	virtual void SelectNextHandle();
	virtual void SelectPreviousHandle();
	
	virtual const bool GetSelectedData(HandleData & data);
	virtual const int GetHandleCount();
	virtual const HandleData GetHandleData(const int idx);
	virtual const int GetHotHandleIndex();

	virtual void SetHotItem(const float percentage);

	virtual void SetMaxHandles(int max) { m_max = max; }
	virtual const int GetMaxHandles() { return m_max; }
	virtual void SetDrawPercentage(bool draw) { m_drawPercentage = draw; }

protected:
	std::vector<HandleData> m_handles;
	CPoint * m_ptLeftDown;
	CPoint m_menuPoint;
	UINT m_max;
	bool m_movedHandles;
	bool m_canMoveSelected;
	bool m_mouseMoving;
	bool m_drawPercentage;
};
