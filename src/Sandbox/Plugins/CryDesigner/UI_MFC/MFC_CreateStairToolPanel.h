#pragma once

#include "IBaseToolPanel.h"

class MFC_CreateStairToolPanel : public BUtil::CBrushDesignerBasicPanel, public ICreateStairToolPanel
{
public:
	DECLARE_DYNAMIC(MFC_CreateStairToolPanel)
	DECLARE_MESSAGE_MAP()

public:
	MFC_CreateStairToolPanel( CBrushDesignerStairTool* pStairTool, CWnd* pParent = NULL );

	void OnOK() override{};
	void OnCancel() override{};

	void DestroyPanel() override;
	BOOL OnInitDialog() override;
	void OnDestroy();
	BrushFloat GetStepRise() const;
	bool IsMirrored() const;
	void SetMirrored( bool bMirrored );
	bool IsRotateBy90Degree() const;
	void SetRotateBy90Degree( bool bRotateBy90Degree );
	void Update( BrushFloat fWidth, BrushFloat fHeight, BrushFloat fDepth );
	void OnInternalVariableChange( IVariable* pVar );

protected:

	CBrushDesignerStairTool* m_pStairTool;

	_smart_ptr<IVariable> m_StepRise;
	_smart_ptr<IVariable> m_bMirror;
	_smart_ptr<IVariable> m_bRotation90Degree;
	_smart_ptr<IVariable> m_Width;
	_smart_ptr<IVariable> m_Height;
	_smart_ptr<IVariable> m_Depth;

};