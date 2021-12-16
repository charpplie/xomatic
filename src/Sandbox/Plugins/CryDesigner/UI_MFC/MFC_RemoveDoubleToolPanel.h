#pragma once

#include "IBaseToolPanel.h"

class MFC_RemoveDoubleToolPanel : public BUtil::CBrushDesignerBasicPanel, public IRemoveDoubleToolPanel
{
public:
	DECLARE_DYNAMIC(MFC_RemoveDoubleToolPanel)
	DECLARE_MESSAGE_MAP()

public:
	MFC_RemoveDoubleToolPanel( CBrushDesignerRemoveDoublesTool* pStairTool, CWnd* pParent = NULL );

	void OnOK() override{};
	void OnCancel() override{};

	void DestroyPanel() override;

	BOOL OnInitDialog() override;
	void OnDestroy();
	BrushFloat GetDistance() const override;
	void OnInternalVariableChange( IVariable* pVar );

protected:

	CBrushDesignerRemoveDoublesTool* m_pRemoveDoubleTool;
	_smart_ptr<IVariable> m_Distance;

};