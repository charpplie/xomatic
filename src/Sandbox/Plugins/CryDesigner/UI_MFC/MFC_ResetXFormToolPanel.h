#pragma once

#include "IBaseToolPanel.h"

class MFC_ResetXFormToolPanel : public CXTResizeDialog, public IBaseToolPanel
{
	DECLARE_MESSAGE_MAP()
	DECLARE_DYNAMIC(MFC_ResetXFormToolPanel)

public:

	MFC_ResetXFormToolPanel(CBrushDesignerResetXFormTool* pResetXFormTool);
	virtual ~MFC_ResetXFormToolPanel(){}

	enum { IDD = IDD_PANEL_BRUSHDESIGNER_RESETXFORM };

protected:
	void OnOK() {};
	void OnCancel() {};
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL OnInitDialog() override;
	void DestroyPanel() override;
	
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedDesignerResetxformFreeze();

	void PostNcDestroy(){ delete this; }

private:

	CBrushDesignerResetXFormTool* m_pDesignerResetXFormTool;
};