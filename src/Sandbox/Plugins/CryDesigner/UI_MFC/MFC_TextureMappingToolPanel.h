#pragma once

#include "IBaseToolPanel.h"

class MFC_TextureMappingToolPanel : public CXTResizeDialog, public ITextureMappingToolPanel
{
	DECLARE_DYNAMIC(MFC_TextureMappingToolPanel)

public:

	MFC_TextureMappingToolPanel( CBrushDesignerTextureMappingTool* pTool, CWnd* pParent = NULL );
	virtual ~MFC_TextureMappingToolPanel();

	enum { IDD = IDD_PANEL_DESIGNER_TEXTUREMAPPINGTOOL };

	void SetTexInfo( const BUtil::STexInfo& texInfo ) override;
	void SetMatID( int nMatID ) override;
	bool IsPicking() const override { return m_pickSelectedBtn.GetCheck() == BST_CHECKED; }
	void DestroyPanel() override;

	void PostNcDestroy(){ delete this; }

protected:
	void OnOK() {};
	void OnCancel() {};
	void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnDestroy();

private:

	bool IsRelative() const { return m_relativeBtn.GetCheck() == BST_CHECKED; }

	void ApplyChanges();
	void ResetControls();
	BUtil::STexInfo GetTexInfoFromControls() const;

	CBrushDesignerTextureMappingTool* m_pTextureMappingTool;

	CNumberCtrl m_offset[2];
	CNumberCtrl m_scale[2];
	CNumberCtrl m_rotate;
	CNumberCtrl m_MatId;
	CNumberCtrl m_fitTiling[2];

	CButton m_pickSelectedBtn;
	CButton m_absoluteBtn;
	CButton m_relativeBtn;

	BUtil::STexInfo m_LastTexInfo;

	afx_msg void OnValueChange();
	afx_msg void OnBnClickedTextureFit();
	afx_msg void OnBnClickedTextureReset();
	afx_msg void OnBnClickedSelectMatid();
	afx_msg void OnBnClickedAssignMatid();
	afx_msg void OnBnClickedTexturePickselected();
	afx_msg void OnRelative();
	afx_msg void OnAbsolute();

};