#pragma once

#include "IBaseToolPanel.h"

class MFC_CreateSphereToolPanel : public BUtil::CBrushDesignerBasicPanel, public ICreateSphereDiscCurveToolPanel
{
public:
	DECLARE_DYNAMIC(MFC_CreateSphereToolPanel)
	DECLARE_MESSAGE_MAP()

public:
	MFC_CreateSphereToolPanel( CBrushDesignerCreateSphereTool* pSphereTool, CWnd* pParent = NULL);

	void OnOK() override{};
	void OnCancel() override{};
	void DestroyPanel() override;
	BOOL OnInitDialog() override;
	void OnDestroy();
	void Update( float fRadius ) override;
	void OnInternalVariableChange( IVariable* pVar );
	int GetSubdivisionNum() const override;

protected:

	CBrushDesignerCreateSphereTool* m_pSphereTool;

	_smart_ptr<IVariable> m_pNumOfSubdivision;
	_smart_ptr<IVariable> m_pRadius;
};