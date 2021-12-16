#pragma once

#include "IBaseToolPanel.h"

class MFC_CreateStairProfileToolPanel : public BUtil::CBrushDesignerBasicPanel, public ICreateStairProfileToolPanel
{
public:
	DECLARE_DYNAMIC(MFC_CreateStairProfileToolPanel)
	DECLARE_MESSAGE_MAP()

public:
	MFC_CreateStairProfileToolPanel( CBrushDesignerStairProfileTool* pStairTool, CWnd* pParent = NULL );
	
	void DestroyPanel() override;

	void OnOK() override{};
	void OnCancel() override{};

	BOOL OnInitDialog() override;
	void OnDestroy();
	BrushFloat GetStepRise() const;
	void OnInternalVariableChange( IVariable* pVar );

protected:

	CBrushDesignerStairProfileTool* m_pStairTool;
	_smart_ptr<IVariable> m_StepRise;
};