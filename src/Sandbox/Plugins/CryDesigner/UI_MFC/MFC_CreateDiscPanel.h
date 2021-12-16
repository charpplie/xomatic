#pragma once

#include "IBaseToolPanel.h"

class MFC_CreateDiscPanel : public BUtil::CBrushDesignerBasicPanel, public ICreateSphereDiscCurveToolPanel
{
public:
	DECLARE_DYNAMIC(MFC_CreateDiscPanel)
	DECLARE_MESSAGE_MAP()

public:
	MFC_CreateDiscPanel( CBrushDesignerDrawDiscTool* pDiscTool, CWnd* pParent = NULL );
	void DestroyPanel() override;
	void OnOK() override{};
	void OnCancel() override{};
	BOOL OnInitDialog() override;
	void OnDestroy();
	void Update( float fRadius ) override;
	void OnInternalVariableChange( IVariable* pVar );
	int GetSubdivisionNum() const override;

protected:

	CBrushDesignerDrawDiscTool* m_pDiscTool;

	_smart_ptr<IVariable> m_pNumOfSubdivision;
	_smart_ptr<IVariable> m_pRadius;

};