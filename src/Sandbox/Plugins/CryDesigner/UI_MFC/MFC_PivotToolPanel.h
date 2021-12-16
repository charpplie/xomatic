#pragma once

#include "IBaseToolPanel.h"

class MFC_PivotToolPanel : public BUtil::CBrushDesignerBasicPanel, public IBaseToolPanel
{
public:
	DECLARE_DYNAMIC(MFC_PivotToolPanel)
	DECLARE_MESSAGE_MAP()

public:
	MFC_PivotToolPanel( CBrushDesignerPivotTool* pTool, CWnd* pParent = NULL );

	enum { IDD = IDD_PANEL_DESIGNER_PIVOT };

	void OnOK() override{};
	void OnCancel() override{};
	BOOL OnInitDialog() override;
	void OnDestroy();

	void DestroyPanel() override;

	afx_msg void OnBnClickedDesignerPivotBoundbox();
	afx_msg void OnBnClickedDesignerPivotMesh();

private:
	CBrushDesignerPivotTool* m_pTool;
};