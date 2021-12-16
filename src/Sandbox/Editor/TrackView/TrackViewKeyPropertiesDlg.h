////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   TrackViewKeyPropertiesDlg.h
//  Created:     7/4/2009 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __TrackViewKeyPropertiesDlg_h__
#define __TrackViewKeyPropertiesDlg_h__
#pragma once

#if _MSC_VER > 1000
#pragma once
#endif

#include "ZViewport.h"
#include "Controls\PropertyCtrl.h"
#include "TrackViewUtils.h"
#include "tcbpreviewctrl.h"
#include "Plugin.h"

class CTrackViewKeyPropertiesDlg;

//////////////////////////////////////////////////////////////////////////
class CTrackViewKeyUIControls : public CObject,public _i_reference_target_t
{
	DECLARE_DYNAMIC(CTrackViewKeyUIControls)
public:
	typedef CTrackViewUtils::SelectedKey  SelectedKey;
	typedef CTrackViewUtils::SelectedKeys SelectedKeys;

	CTrackViewKeyUIControls() { m_pVarBlock = new CVarBlock; };

	void SetKeyPropertiesDlg( CTrackViewKeyPropertiesDlg *pDlg ) { m_pKeyPropertiesDlg = pDlg; }

	// Return internal variable block.
	CVarBlock* GetVarBlock() const { return m_pVarBlock; }

	//////////////////////////////////////////////////////////////////////////
	// Callbacks that must be implemented in derived class
	//////////////////////////////////////////////////////////////////////////
	// Returns true if specified animation track type is supported by this UI.
	virtual bool SupportTrackType( EAnimTrackType type ) const = 0;

	// Called when UI variable changes.
	virtual void OnCreateVars() = 0;

	// Called when user changes selected keys.
	// Return true if control update UI values
	virtual bool OnKeySelectionChange( SelectedKeys &keys ) = 0;

	// Called when UI variable changes.
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &keys ) = 0;

protected:
	//////////////////////////////////////////////////////////////////////////
	// Helper functions.
	//////////////////////////////////////////////////////////////////////////
	template <class T>
	void SyncValue( CSmartVariable<T> &var,T& value,bool bCopyToUI,IVariable *pSrcVar=NULL )
	{
		if (bCopyToUI)
			var = value;
		else
		{
			if (!pSrcVar || pSrcVar == var.GetVar())
				value = var;
		}
	}
	void AddVariable( CVariableBase &varArray,CVariableBase &var,const char *varName,unsigned char dataType=IVariable::DT_SIMPLE )
	{
		if (varName)
			var.SetName(varName);
		var.SetDataType(dataType);
		var.AddOnSetCallback( functor(*this,&CTrackViewKeyUIControls::OnInternalVariableChange) );
		varArray.AddChildVar(&var);
		m_registeredVariables.push_back(&var);
	}
	//////////////////////////////////////////////////////////////////////////
	void AddVariable( CVariableBase &var,const char *varName,unsigned char dataType=IVariable::DT_SIMPLE )
	{
		if (varName)
			var.SetName(varName);
		var.SetDataType(dataType);
		var.AddOnSetCallback( functor(*this,&CTrackViewKeyUIControls::OnInternalVariableChange) );
		m_pVarBlock->AddVariable(&var);
		m_registeredVariables.push_back(&var);
	}
	void OnInternalVariableChange( IVariable *pVar );
	
	void RefreshTrackViewKeys();

protected:
	_smart_ptr<CVarBlock> m_pVarBlock;
	std::vector<_smart_ptr<IVariable> > m_registeredVariables;
	CTrackViewKeyPropertiesDlg *m_pKeyPropertiesDlg;
};

//////////////////////////////////////////////////////////////////////////
class CTrackViewTrackPropsDlg : public CDialog
{
public:
	CTrackViewTrackPropsDlg();   // standard constructor

	void SetSequence( IAnimSequence *pSequence );
	bool OnKeySelectionChange( CTrackViewUtils::SelectedKeys &keys );
	void ReloadKey();

	// Dialog Data
	enum { IDD = IDD_TV_TRACK_PROPERTIES };

protected:
	void SetCurrKey( int nkey );

	virtual BOOL OnInitDialog();
	virtual void OnOK() {};
	virtual void OnCancel() {};
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg void OnDeltaposPrevnext(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnUpdateTime();
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()

	CSpinButtonCtrl m_keySpinBtn;
	CStatic m_keynum;

	_smart_ptr<IAnimTrack> m_track;
	int m_key;
	CNumberCtrl m_time;
};


//////////////////////////////////////////////////////////////////////////
class TrackViewKeys;
class CTrackViewKeyPropertiesDlg : public CDialog, public ITrackViewEventsListener
{
public:
	CTrackViewKeyPropertiesDlg();
	enum { IDD = IDD_PANEL_PROPERTIES };

	void SetKeysCtrl( CTrackViewKeys *pKeysCtrl ) 
	{ m_keysCtrl = pKeysCtrl; }

	void SetSequence( IAnimSequence *pSequence );
	IAnimSequence* GetSequence() { return m_pSequence; };
	
	void PopulateVariables();

	//////////////////////////////////////////////////////////////////////////
	// ITrackViewEventsListener interface implementation
	//////////////////////////////////////////////////////////////////////////
	virtual void OnKeySelectionChange();
	//////////////////////////////////////////////////////////////////////////


protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);

	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK() {};
	virtual void OnCancel() {};

	//////////////////////////////////////////////////////////////////////////
	void OnVarChange( IVariable *pVar );
	void CreateAllVars();
	void AddVars( CTrackViewKeyUIControls *pUI );
	void ReloadValues();

protected:
	std::vector< _smart_ptr<CTrackViewKeyUIControls> > m_keyControls;

	_smart_ptr<CVarBlock> m_pVarBlock;
	_smart_ptr<IAnimSequence> m_pSequence;
	
	CPropertyCtrl m_wndProps;
	CTrackViewTrackPropsDlg m_wndTrackProps;
	CTcbPreviewCtrl m_wndTcbPreview;

	CTrackViewKeys *m_keysCtrl;
};

#endif //__TrackViewKeyPropertiesDlg_h__