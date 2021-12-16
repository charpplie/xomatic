#pragma once

#include "IBaseToolPanel.h"

class MFC_BooleanToolPanel : public CXTResizeDialog, public IBaseToolPanel
{
	DECLARE_MESSAGE_MAP()
	DECLARE_DYNAMIC(MFC_BooleanToolPanel)

public:

	MFC_BooleanToolPanel(CBrushDesignerBooleanTool* pBooleanTool);
	virtual ~MFC_BooleanToolPanel(){}

	void DestroyPanel() override;

	enum { IDD = IDD_PANEL_DESIGNER_BOOLEAN };

protected:
	void OnOK() {};
	void OnCancel() {};
	void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog();
	void PostNcDestroy(){ delete this; }	
	
	afx_msg void OnBnClickedDesignerBooleanUnion();
	afx_msg void OnBnClickedDesignerBooleanDifference();
	afx_msg void OnBnClickedDesignerBooleanIntersection();

private:
	CBrushDesignerBooleanTool* m_pDesignerBooleanTool;
};