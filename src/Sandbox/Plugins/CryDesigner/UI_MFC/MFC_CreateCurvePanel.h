#pragma once

#include "IBaseToolPanel.h"

class CBrushDesignerDrawCurveTool;

class MFC_CreateCurvePanel : public BUtil::CBrushDesignerBasicPanel, public ICreateSphereDiscCurveToolPanel
{
public:
	DECLARE_DYNAMIC(MFC_CreateCurvePanel)
	DECLARE_MESSAGE_MAP()

public:

	MFC_CreateCurvePanel( CBrushDesignerDrawCurveTool* pDrawCurveTool, CWnd* pParent = NULL );

	void OnOK() override {};
	void OnCancel() override {};

	void DestroyPanel() override;

	void OnDestroy();
	BOOL OnInitDialog() override;

	int GetSubdivisionNum() const override;
	void Update( float fRadius ) override {}

	void OnInternalVariableChange( IVariable* pVar );

private:

	CBrushDesignerDrawCurveTool* m_pCurveTool;	
	_smart_ptr<IVariable> m_pNumOfSubdivision;
};