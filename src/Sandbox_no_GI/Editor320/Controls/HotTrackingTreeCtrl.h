#ifndef _HOTTRACKINGTREECTRL_H_
#define _HOTTRACKINGTREECTRL_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CHotTrackingTreeCtrl : public CTreeCtrl
{
public:
	CHotTrackingTreeCtrl();
	virtual ~CHotTrackingTreeCtrl(){};

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

private:
	HTREEITEM m_hHoverItem;
};

#endif // _HOTTRACKINGTREECTRL_H_