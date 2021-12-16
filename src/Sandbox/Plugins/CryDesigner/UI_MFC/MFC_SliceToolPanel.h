#pragma once

#include "IBaseToolPanel.h"

class MFC_SliceToolPanel : public CXTResizeDialog, public IBaseToolPanel
{
	DECLARE_DYNAMIC(MFC_SliceToolPanel)

public:

	MFC_SliceToolPanel( CBrushDesignerSliceTool* pSliceTool, CWnd* pParent = NULL );
	virtual ~MFC_SliceToolPanel();

	void PostNcDestroy(){ delete this; }
	void DestroyPanel() override;

	enum { IDD = IDD_PANEL_DESIGNER_EDIT_SLICETOOL };

protected:
	void OnOK() {};
	void OnCancel() {};

	void DoDataExchange(CDataExchange* pDX);

	BOOL OnInitDialog();
	afx_msg void OnBnClickedSliceFront();
	afx_msg void OnBnClickedSliceBack();
	afx_msg void OnBnClickedSliceClip();
	afx_msg void OnBnClickedSliceDivide();
	afx_msg void OnBnClickedMirrorInvert();
	afx_msg void OnBnClickedAlignX();
	afx_msg void OnBnClickedAlignY();
	afx_msg void OnBnClickedAlignZ();

	void OnEnChangeNumberSliceplanes();

	DECLARE_MESSAGE_MAP()

private:

	CBrushDesignerSliceTool* m_pSliceTool;
	int m_nCutRadioButtons;
};