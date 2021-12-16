//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File Name        : TooltipListCtrl.h
//  Author           : Jaewon Jung
//  Time of creation : 7/7/2010   11:30
//  Compilers        : VS2008
//  Description      : A list control with a tooltip support
//  Notice           : Also see CTootipComboBox.
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
#ifndef __TOOLTIPLISTCTRL_H__
#define __TOOLTIPLISTCTRL_H__
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CTooltipListCtrl window

class CTooltipListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CTooltipListCtrl)
public:
	CTooltipListCtrl();
	CString GetItemTip( int nRow ) const;
	int SetItemTip( int nRow, CString sTip );
	void Display( CRect rc );
	void Init( CComboBox *pComboParent );
	virtual ~CTooltipListCtrl();

	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
	virtual void PreSubclassWindow();
	virtual INT_PTR OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;

	BOOL OnToolTipText( UINT id, NMHDR * pNMHDR, LRESULT * pResult );

	int m_nLastItem;
	CComboBox *m_pComboParent;
	CMap< int, int &, CString, CString & > m_mpItemToTip;

	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
};

#endif // __TOOLTIPLISTCTRL_H__
