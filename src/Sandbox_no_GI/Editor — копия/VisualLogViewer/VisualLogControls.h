// VisualLogControls.cpp : header file
//
#pragma once

#define UWM_BUTTON_CLICKED	(WM_USER + 10)

#include "VisualLogCommon.h"
#include "ToolbarDialog.h"
#include "afxcmn.h"



enum EVLogPlayerButtons
{
	eVLPB_None		= 0x0000,
	eVLPB_First		= 0x0001,
	eVLPB_Prev		= 0x0002,
	eVLPB_Play		= 0x0004,
	eVLPB_Stop		= 0x0008,
	eVLPB_Next		= 0x0010,
	eVLPB_Last		= 0x0020,
	eVLPB_All			= 0x003F,
};



// VisualLogDialog
class CVisualLogDialog : public CXTResizeDialog
{
	DECLARE_DYNAMIC(CVisualLogDialog)

public:				// Construction & destruction
	CVisualLogDialog( CWnd* pParent = NULL );
	virtual ~CVisualLogDialog();

public:				// Dialog Data
	enum { IDD = IDD_VL_CONTROLS };
	CString			m_strFolder;
	CSliderCtrl	m_sldSpeed;
	CSliderCtrl	m_sldFrame;
	BOOL				m_bUpdateText;
	BOOL				m_bUpdateImages;
	BOOL				m_bKeepAspectR;

public:			// Operations
	void SetCommonData(SVLogCommonData *pCD) { m_pCD = pCD; }
	void EnablePlayerButtons();
private:
	void DrawPlayerButtons(EVLogPlayerButtons eb);
	void OnPlayerFirst();
	void OnPlayerPrev();
	void OnPlayerPlay();
	void OnPlayerStop();
	void OnPlayerNext();
	void OnPlayerLast();

protected:	// Virtual overrides
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

protected:	// Message handlers
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnBnFolderClicked();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedVLchkUpdatetext();
	afx_msg void OnBnClickedVLchkUpdateimages();
	afx_msg void OnBnClickedVLchkKeepaspectr();
	afx_msg void OnBnClickedColorPicker();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	DECLARE_MESSAGE_MAP()

private:		// Member data
	CBitmap							m_bmControlBoard, m_bmPlayerBtns, *m_pBmCB, *m_pBmPB;
	CDC									m_dcControlBoard, m_dcPlayerBtns;
	CRect								m_rcPlayer;
	int									m_nEnabled;
	EVLogPlayerButtons	m_eDown;
	int									m_nPlayerClick;
	CRect								m_arrButtons[6];
	CBrush							m_brSlider;

	CXTColorPicker			m_colorPickBtn;		// Color picker button
	CFixedToolTip				m_ctlTT;					// Tool tips
	int									m_nTimerVar;

	SVLogCommonData *m_pCD;
};



// VisualLogControls docking frame
class CVisualLogControls : public CToolbarDialog
{
	DECLARE_DYNAMIC(CVisualLogControls)

public:			// Construction & destruction
	CVisualLogControls();   // standard constructor
	virtual ~CVisualLogControls();

public:			// Dialog Data
	enum { IDD = IDD_DATABASE };

protected:	// Message handlers
	DECLARE_MESSAGE_MAP()
};
