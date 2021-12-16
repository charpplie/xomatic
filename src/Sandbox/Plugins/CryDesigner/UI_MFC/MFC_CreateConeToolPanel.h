#pragma once

#include "IBaseToolPanel.h"

class MFC_CreateConeToolPanel : public BUtil::CBrushDesignerBasicPanel, public ICreateCylinderConeToolPanel
{
public:

	MFC_CreateConeToolPanel( CBrushDesignerCreateConeTool* pConeCreateTool, CWnd* pParent = NULL );

	void DestroyPanel() override;

	BOOL OnInitDialog() override;
	void OnDestroy();	

	void OnOK() override{};
	void OnCancel() override{};

	void Update( float fRadius, float fHeight ) override;
	int GetSubdivisionNum() const override;
	float GetRadius() const override;

	void OnInternalVariableChange( IVariable* pVar );

	DECLARE_DYNAMIC(MFC_CreateConeToolPanel)
	DECLARE_MESSAGE_MAP()

private:

	CBrushDesignerCreateConeTool* m_pConeTool;

	_smart_ptr<IVariable> m_pNumOfSubdivision;
	_smart_ptr<IVariable> m_pHeight;
	_smart_ptr<IVariable> m_pRadius;
};