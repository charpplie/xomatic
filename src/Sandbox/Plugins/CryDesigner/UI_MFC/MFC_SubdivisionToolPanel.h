#pragma once

#include "IBaseToolPanel.h"

class MFC_SubdivisionToolPanel : public CXTResizeDialog, public IBaseToolPanel
{
	DECLARE_DYNAMIC(MFC_SubdivisionToolPanel)
	DECLARE_MESSAGE_MAP()

public:
	MFC_SubdivisionToolPanel( CBrushDesignerSubdivisionTool* pTool, CWnd* pParent = NULL );
	~MFC_SubdivisionToolPanel(){}

	enum { IDD =  IDD_PANEL_BRUSH_DESIGNERSUBDIVISIONTOOL };

	void DestroyPanel() override;

protected:
	void OnOK() {};
	void OnCancel() {};
	void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog();

	void UpdateEdgeGroupList();

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	void PostNcDestroy(){ delete this; }	

protected:
	CSliderCtrl m_SubdivisionLevelSliderCtrl;
	CListCtrl m_EdgeSharpnessListCtrl;
	CBrushDesignerSubdivisionTool* m_pEditTool;
	int m_nEditedSubItem;

	afx_msg void OnBnClickedAddSharpnessButton();
	afx_msg void OnBnClickedDeleteSharpnessButton();
	afx_msg void OnNMDblclkEdgesharpnessListctrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnEndlabeleditEdgesharpnessListctrl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeSubdivisionTessfactor();
	afx_msg void OnLvnItemchangedEdgesharpnessListctrl(NMHDR *pNMHDR, LRESULT *pResult);
};