////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2013.
// -------------------------------------------------------------------------
//  File name:   SegmentSelectionPanel.h
//  Version:     v1.00
//  Created:     8/10/2013 by Allen Chen
//  Compilers:   Visual Studio.NET
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SEGMENT_SELECTION_PANEL_H__
#define __SEGMENT_SELECTION_PANEL_H__

#include "Controls/SliderCtrlEx.h"

class CMapWnd;
class CSegmentSelectTool;

class CSegmentSelectionPanel : public CXTResizeDialog
{
public:
	CSegmentSelectionPanel(CWnd *pParent = NULL, CSegmentSelectTool *pTool = NULL);

	enum { IDD = IDD_PANEL_SEGMENT_SELECTION };

protected:
	virtual void OnOK() {}
	virtual void OnCancel() {}
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	CSegmentSelectTool *m_pTool;
	CCustomButton m_cSelectionMode;
	CCustomButton m_cSelectionClear;
	CNumberCtrl m_cSelectionSize;
	CSliderCtrlCustomDraw m_cSelectionSizeSlider;
	
	void OnUpdateNumbers();
	void OnSetSelectionSize();
	void OnChangeSelectionMode();
	void OnClearSelection();

	//{{AFX_MSG
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	BOOL m_bSelectionMode;
};

#endif // __SEGMENT_SELECTION_PANEL_H__