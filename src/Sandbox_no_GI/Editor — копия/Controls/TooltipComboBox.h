//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File Name        : TooltipComboBox.h
//  Author           : Jaewon Jung
//  Time of creation : 7/7/2010   11:34
//  Compilers        : VS2008
//  Description      : A combo box with a tooltip support
//  Notice           : See CTooltipListCtrl also.
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
#ifndef __TOOLTIPCOMBOBOX_H__
#define __TOOLTIPCOMBOBOX_H__
#pragma once

/////////////////////////////////////////////////////////////////////////////
// CTooltipComboBox window
#include "TooltipListCtrl.h"

class CTooltipComboBox : public CComboBox
{
	DECLARE_DYNAMIC(CTooltipComboBox)
public:
	CTooltipComboBox();
	CString GetComboTip( ) const;
	void SetComboTip( CString sTip );
	int SetItemTip( int nRow, CString sTip );
	BOOL GetDroppedState( ) const;
	int GetDroppedHeight( ) const;
	int GetDroppedWidth( ) const;
	int SetDroppedHeight( UINT nHeight );
	int SetDroppedWidth( UINT nWidth );
	void DisplayList( BOOL bDisplay = TRUE );
	virtual ~CTooltipComboBox();

protected:
	virtual void PreSubclassWindow();
	virtual INT_PTR OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;
	BOOL OnToolTipText( UINT id, NMHDR * pNMHDR, LRESULT * pResult );

	int m_nDroppedHeight;
	int m_nDroppedWidth;
	CTooltipListCtrl m_lstCombo;

	CString m_sTip;
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()
};

#endif // __TOOLTIPCOMBOBOX_H__
