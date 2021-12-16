#pragma once

#include "IBaseToolPanel.h"

class MFC_MirrorToolPanel : public CXTResizeDialog, public IMirrorToolPanel
{
	DECLARE_MESSAGE_MAP()
	DECLARE_DYNAMIC(MFC_MirrorToolPanel)

public:

	MFC_MirrorToolPanel( CBrushDesignerMirrorTool* pMirrorTool, CWnd* pParent = NULL );
	~MFC_MirrorToolPanel(){}

	void PostNcDestroy(){ delete this; }
	void ToggleWndEnableDisable() override;

	void DestroyPanel() override;

	enum { IDD = IDD_PANEL_DESIGNER_EDIT_MIRRORTOOL };

	void OnOK() {};
	void OnCancel() {};

	void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog();
	afx_msg void OnBnClickedAlignX();
	afx_msg void OnBnClickedAlignY();
	afx_msg void OnBnClickedAlignZ();
	void OnBnClickedMirrorApply();
	void OnBnClickedFreezeDesigner();
	void OnBnClickedMirrorInvert();
	void OnBnClickedCenterPivot();

private:

	CBrushDesignerMirrorTool* m_pMirrorTool;
};