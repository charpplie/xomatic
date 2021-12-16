#pragma once

#include "IBaseToolPanel.h"

class MFC_CloneToolPanel : public BUtil::CBrushDesignerBasicPanel, public ICloneToolPanel
{
	DECLARE_DYNAMIC(MFC_CloneToolPanel)

public:

	MFC_CloneToolPanel( CBrushDesignerCloneTool* pCloneTool, CWnd* pParent = NULL );
	virtual ~MFC_CloneToolPanel(){}

	void DestroyPanel() override;
	int GetNumOfClone() override;
	BUtil::EPlacementType GetPlacementType() const override;

protected:
	void OnOK() {};
	void OnCancel() {};
	BOOL OnInitDialog();
	void OnDestroy();
	void OnInternalVariableChange( IVariable* pVar );

	DECLARE_MESSAGE_MAP()

private:

	CString GetNumberString( int number ) const;

	_smart_ptr<CBrushDesignerCloneTool> m_pDesignerCloneTool;
	_smart_ptr<IVariable> m_NumberOfClones;
	CSmartVariableEnum<CString> m_PlacementWayVar;
};