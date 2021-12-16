////////////////////////////////////////////////////////////////////////////
//
//  Crytek Source File.
//  Copyright (C), Crytek Studios, 1999-2010.
// -------------------------------------------------------------------------
//  File Name        : VehiclePaintsPanel.h
//  Version          : v1.00
//  Created          : 5/3/2010 by Pau Novau
//  Description      : 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Vehicle_Paints_Panel__h__
#define __Vehicle_Paints_Panel__h__

#include "IDataBaseManager.h"

class CVehiclePaintsPanel
	: public CXTResizeDialog
	, public IDataBaseManagerListener
{
DECLARE_DYNCREATE( CVehiclePaintsPanel )

public:
	CVehiclePaintsPanel();
	virtual ~CVehiclePaintsPanel();

	enum { IDD = IDD_VEHICLE_PAINTS };

	void InitPaints( IVariable* pPaints );
	void Clear();

	// IDataBaseManagerListener
	virtual void OnDataBaseItemEvent( IDataBaseItem* pItem, EDataBaseItemEvent event );
	// ~IDataBaseManagerListener

protected:
	afx_msg void OnAddNewPaint();
	afx_msg void OnRemoveSelectedPaint();
	afx_msg void OnAssignMaterialToSelectedPaint();
	afx_msg void OnApplyPaintToVehicle();
	afx_msg void OnPaintNameSelectionChanged();
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual void OnOK() {}
	virtual void OnCancel() {}

	void CreateNewPaint( const CString& paintName );

private:
	void RebuildPaintNames();

	bool IsPaintNameSelected() const;
	CString GetSelectedPaintName() const;
	int GetSelectedPaintNameVarIndex() const;
	void RenameSelectedPaintName( const CString& name );
	void AddPaintName( const CString& paintName, int paintVarIndex );
	void ClearPaintNames();

	void HidePaintProperties();
	void ShowPaintPropertiesByName( const CString& paintName );
	void ShowPaintPropertiesByVarIndex( int paintVarIndex );
	void ShowPaintProperties( IVariable* pPaint );

	IVariable* GetPaintVarByName( const CString& paintName );
	IVariable* GetPaintVarByIndex( int index );

	void OnSelectedPaintNameChanged( IVariable* pVar );
	void OnSelectedPaintMaterialChanged( IVariable* pVar );

	void UpdateAssignMaterialButtonState();

private:
	CListBox m_paintNames;
	CPropertyCtrl m_paintProperties;
	CButton m_applyMaterialButton;
	CButton m_applyToVehicleButton;

	IVariable* m_pPaints;
	IVariable* m_pSelectedPaint;
};

#endif
