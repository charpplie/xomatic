#pragma once

#include "IBaseToolPanel.h"

class MFC_CreateCylinderToolPanel : public BUtil::CBrushDesignerBasicPanel, public ICreateCylinderConeToolPanel
{
public:

	MFC_CreateCylinderToolPanel( CBrushDesignerCreateCylinderTool* pCylinderCreateTool, CWnd* pParent = NULL );

	void DestroyPanel() override;

	BOOL OnInitDialog() override;
	void OnDestroy();	

	void OnOK() override{};
	void OnCancel() override{};

	void Update( float fRadius, float fHeight ) override;
	int GetSubdivisionNum() const override;
	float GetRadius() const override;

	void OnInternalVariableChange( IVariable* pVar );

	DECLARE_DYNAMIC(MFC_CreateCylinderToolPanel)
	DECLARE_MESSAGE_MAP()

private:

	CBrushDesignerCreateCylinderTool* m_pCylinderTool;

	_smart_ptr<IVariable> m_pNumOfSubdivision;
	_smart_ptr<IVariable> m_pHeight;
	_smart_ptr<IVariable> m_pRadius;
};