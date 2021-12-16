#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   DesignerRegionDebuggerDlg.h
//  Created:     12/11/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Core/BrushRegion.h"
#include "afxwin.h"
#include "IBaseToolPanel.h"

class MFC_DesignerRegionDebuggerDlg : public CDialog, public IDesignerRegionDebuggerDlg
{
	DECLARE_DYNAMIC(MFC_DesignerRegionDebuggerDlg)

public:
	MFC_DesignerRegionDebuggerDlg(CWnd* pParent = NULL);
	~MFC_DesignerRegionDebuggerDlg(){}

	void AddRegion( CBrushRegion* pRegion, const char* name ) override;
	int GetRegionCount() const override { return m_Regions.size(); }
	void Open() override
	{
		DoModal(); 
	}
	void DestroyPanel() override {}
	void PostNcDestroy(){ delete this; }

	enum { IDD = IDD_DESIGNERREGION_DEBUGGERDLG };
	BOOL OnInitDialog();

protected:

	void OnOK()	{	DeleteAllTreeItems();	__super::OnOK();	}
	void OnCancel()	{	DeleteAllTreeItems();	__super::OnCancel();	}

	CRect GetCanvasRect() const;
	CRect GetCanvasFillRect() const;
	CPoint Dlg2CanvasPos( const CPoint& point ) const;
	void MoveZoomRect( const CPoint& point );
	void UpdateRegionsBoundary();
	CPoint World2Screen( const BrushVec2& v, const CRect& rCanvas ) const;
	void DrawEdge( CDC* pDC, int x0, int y0, int x1, int y1 );
	void DrawNumber( CDC* pDC, int x, int y, int number );

	struct SRegionDrawInfo
	{
		SRegionDrawInfo() : edgeColor(RGB(0,0,0)),vertexNumberColor(RGB(50,150,50)),edgeNumberColor(RGB(50,50,200))
		{	
		}

		SRegionDrawInfo( COLORREF _edgeColor, COLORREF _vertexNumberColor, COLORREF _edgeNumberColor ) : 
		edgeColor(_edgeColor),
		vertexNumberColor(_vertexNumberColor),
		edgeNumberColor(_edgeNumberColor)
		{
		}

		COLORREF edgeColor;
		COLORREF vertexNumberColor;
		COLORREF edgeNumberColor;
	};

	void DrawRegion( CDC* pDC, const SRegionDrawInfo& colorInfo, const CRect& rCanvas, CBrushRegion::RegionPtr pRegion, bool bSelectedInfo );
	void DrawRect( CDC* pDC, COLORREF color, const CRect& rect );

	void UpdateElementTree( CBrushRegion::RegionPtr pRegion );
	void DoDataExchange(CDataExchange* pDX);
	void DeleteAllTreeItems();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnPaint();
	afx_msg void OnBnClickedResetpivotview();
	afx_msg void OnLbnSelchangeRegionlist();
	afx_msg void OnTvnSelchangedRegionElementinfoTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedDisplaytransparenttext();
	afx_msg void OnBnClickedDisplayzoomrect();

	std::vector<CBrushRegion::RegionPtr> m_Regions;
	std::vector<CString> m_RegionNames;

	AABB m_RegionsBoundary;
	AABB m_ZoomedBoundary;
	CRect m_ZoomRect;
	CListBox m_RegionListBox;

	CBitmap m_MemoryBitmap;
	CDC m_MemoryDC;

	CPoint m_prevMousePos;
	CTreeCtrl m_ElementInfoTree;

	XmlNodeRef m_SelectedItemNode;
	CButton m_TransparentCheckBox;
	CButton m_DisplayZoomRectCheckBox;
};