#pragma once

#include "IBaseToolPanel.h"

class MFC_SmoothingGroupToolPanel : public CXTResizeDialog, public ISmoothingGroupToolPanel
{
	DECLARE_DYNAMIC(MFC_SmoothingGroupToolPanel)

public:

	MFC_SmoothingGroupToolPanel( CBrushDesignerSmoothingGroupTool* pSmoothingGroupTool, CWnd* pParent = NULL );
	virtual ~MFC_SmoothingGroupToolPanel();

	enum { IDD = IDD_PANEL_BRUSHDESIGNER_SMOOTHINGGROUP };

	void ClearAllSelectionsOfNumbers( int nExcludedID = -1 ) override;
	void ShowAllNumbers() override;
	void HideNumber( int nNumber ) override;
	void DestroyPanel() override;
	void PostNcDestroy(){ delete this; }

protected:
	void OnOK() {};
	void OnCancel() {};

	void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog();
	void OnDestroy();

	afx_msg void OnBnClickedSmoothingGroupNumber(UINT nCtrlID);
	afx_msg void OnBnClickedSmoothinggroupSelectbysg();
	afx_msg void OnBnClickedSmoothinggroupRemoveSmoothingGroup();
	afx_msg void OnBnClickedSmoothinggroupAutosmooth();

	DECLARE_MESSAGE_MAP()

private:

	CBrushDesignerSmoothingGroupTool* m_pSmoothingGroupTool;
	std::map<int,int> m_nSmoothingGroupButtonIDs;
};	