#pragma once

#include "IBaseToolPanel.h"

class MFC_CreateBoxToolPanel : public BUtil::CBrushDesignerBasicPanel, public ICreateBoxToolPanel
{
public:

	MFC_CreateBoxToolPanel( CBrushDesignerCreateBoxTool* pBoxCreateTool, CWnd* pParent = NULL );

	void DestroyPanel() override;
	void Update( const BrushVec2& p0, const BrushVec2& p1, BrushFloat fHeight ) override;

	BOOL OnInitDialog() override;
	void OnDestroy();	

	void OnOK() override{};
	void OnCancel() override{};

	void OnInternalVariableChange( IVariable* pVar );

	DECLARE_DYNAMIC(MFC_CreateBoxToolPanel)
	DECLARE_MESSAGE_MAP()

private:

	CBrushDesignerCreateBoxTool* m_pBoxCreateTool;

	_smart_ptr<IVariable> m_Width;
	_smart_ptr<IVariable> m_Height;
	_smart_ptr<IVariable> m_Depth;
};