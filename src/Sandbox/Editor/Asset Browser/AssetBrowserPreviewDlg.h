#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetBrowserPreviewDlg.h
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description:	The asset browser preview dialog window
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 
#include "Include/IAssetItem.h"
#include "Util/GdiUtil.h"
#include "afxwin.h"

class CAssetBrowserPreviewDlg : public CDialog
{
	DECLARE_DYNAMIC(CAssetBrowserPreviewDlg)

public:
	CAssetBrowserPreviewDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAssetBrowserPreviewDlg();

	void StartPreviewAsset(IAssetItem* pAsset);
	void EndPreviewAsset();
	void ResizeControls();
	void SetAssetPreviewHeaderDlg(CDialog* pDlg);
	void SetAssetPreviewFooterDlg(CDialog* pDlg);

// Dialog Data
	enum { IDD = IDD_ASSET_BROWSER_PREVIEW };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	bool m_bDragging;
	IAssetItem* m_pAssetItem;
	CPoint m_lastPanDragPt;
	IRenderer* m_piRenderer;
	CGdiCanvas m_canvas;
	CStatic m_wndAssetRender;
	CDialog* m_pAssetPreviewHeaderDlg;
	CDialog* m_pAssetPreviewFooterDlg;
	CFont m_noPreviewTextFont;

	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};