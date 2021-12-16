#pragma once

#include "IBaseToolPanel.h"

class MFC_CreateRectangleToolPanel : public BUtil::CBrushDesignerBasicPanel, public ICreateRectangleToolPanel
{
public:
	DECLARE_DYNAMIC(MFC_CreateRectangleToolPanel)
	DECLARE_MESSAGE_MAP()

public:
	MFC_CreateRectangleToolPanel( CBrushDesignerDrawRectangleTool* pRectangleTool, CWnd* pParent = NULL );

	void OnOK() override{};
	void OnCancel() override{};

	void DestroyPanel() override;
	BOOL OnInitDialog() override;
	void OnDestroy();
	void Update( float fWidth, float fDepth ) override;
	void OnInternalVariableChange( IVariable* pVar );

protected:

	CBrushDesignerDrawRectangleTool* m_pRectangleTool;

	_smart_ptr<IVariable> m_Width;
	_smart_ptr<IVariable> m_Depth;
};