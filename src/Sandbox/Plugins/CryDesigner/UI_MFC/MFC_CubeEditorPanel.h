#pragma once

#include "IBaseToolPanel.h"

class MFC_CubeEditorPanel : public BUtil::CBrushDesignerBasicPanel, public ICubeEditorPanel
{
public:
	DECLARE_DYNAMIC(MFC_CubeEditorPanel)
	DECLARE_MESSAGE_MAP()

public:
	MFC_CubeEditorPanel( CBrushDesignerCubeEditor* pCubeEditor, CWnd* pParent = NULL );

	enum { IDD = IDD_PANEL_DESIGNER_CUBEEDITOR };

	void OnOK() override{};
	void OnCancel() override{};

	BOOL OnInitDialog() override;
	void OnDestroy();
	void DestroyPanel() override;

	BrushFloat GetCubeSize() const override;

	bool IsSidesMerged() const;
	void UncheckAllButtons();

	afx_msg void OnBnClickedDesignerCubeeditorAdd();
	afx_msg void OnBnClickedDesignerCubeeditorRemove();
	afx_msg void OnBnClickedDesignerCubeeditorPaint();

	CButton* GetAddButton() const;
	CButton* GetRemoveButton() const;
	CButton* GetPaintButton() const;

	bool IsAddButtonChecked() const override;
	bool IsRemoveButtonChecked() const override;
	bool IsPaintButtonChecked() const override;

	int GetSubMatID() const override;
	void SetSubMatID( int nID ) const override;

	bool SetMaterial( CMaterial* pMaterial ) override;
	void SelectPrevBrush() override;
	void SelectNextBrush() override;
	void UpdateSubMaterialComboBox() override;

protected:

	CBrushDesignerCubeEditor* m_pCubeEditor;
	std::vector<BrushFloat> m_CubeSizeList;	
};