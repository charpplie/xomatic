#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   EventPresetDlg.h
//  Created:     June/19/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Controls/PropertyCtrl.h"
#include "Controls/ButtonSetCtrl.h"

class CEventPresetDlg : public CDialog
{
	DECLARE_DYNAMIC(CEventPresetDlg)

public:
	CEventPresetDlg( CAnimEventData* pAnimEventData, SButtonItem* pButtonItem, const std::vector<string>& boneNameList, CWnd* pParent = NULL );
	virtual ~CEventPresetDlg();

	enum { IDD = IDD_EDITEVENTPRESET };

protected:

	void OnOK();
	BOOL OnInitDialog();	

	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()

private:

	CPropertyCtrl m_PropertyCtrl;
	CAnimEventData* m_pEventData;
	SButtonItem* m_pButtonItem;

	std::vector<CString> m_BoneNameList;

	_smart_ptr<IVariable> m_pPresetNameVar;
	_smart_ptr<IVariable> m_pButtonColorVar;
	_smart_ptr<IVariable> m_pNameVar;
	_smart_ptr<IVariable> m_pParameterVar;	
	CSmartVariableEnum<CString> m_BoneListVar;
	_smart_ptr<IVariable> m_pOffsetVar;
	_smart_ptr<IVariable> m_pDirVar;
	_smart_ptr<IVariable> m_pModelVar;	
};