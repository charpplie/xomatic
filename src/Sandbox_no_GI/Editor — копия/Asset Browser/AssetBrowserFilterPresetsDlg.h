////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetBrowserFilterPresetsDlg.h
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description:	Dialog for editing the asset field filter presets
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#ifndef __AssetBrowserFilterPresetsDlg_h__
#define __AssetBrowserFilterPresetsDlg_h__
#pragma once

#include "afxwin.h"
#include <vector>
#include "Include/IAssetDisplay.h"
#include "Include/IAssetDisplayDatabase.h"

class CAssetBrowserFilterEditDlg;
class CAssetBrowserDialog;

class CFieldFiltersPreset
{
	public:

		CString										m_presetName;
		std::vector<SAssetField>	m_fields;
};

class CFilterFieldCheckNameItem : public CXTPReportRecordItem
{
	DECLARE_SERIAL(CFilterFieldCheckNameItem)
public:

	CFilterFieldCheckNameItem();
	CFilterFieldCheckNameItem( SAssetField* pField );

	BOOL OnRequestEdit(XTP_REPORTRECORDITEM_ARGS* pItemArgs);
	void OnInplaceButtonDown(CXTPReportInplaceButton* pButton);

	SAssetField* m_pField;
};

class CFilterFieldConditionItem : public CXTPReportRecordItem
{
	DECLARE_SERIAL(CFilterFieldConditionItem)
public:

	CFilterFieldConditionItem();
	CFilterFieldConditionItem( SAssetField* pField );
	
	BOOL OnRequestEdit(XTP_REPORTRECORDITEM_ARGS* pItemArgs);
	void OnInplaceButtonDown(CXTPReportInplaceButton* pButton);
	void OnEditChanged(XTP_REPORTRECORDITEM_ARGS* pItemArgs, LPCTSTR szText);

	SAssetField* m_pField;
};

class CFilterFieldValueItem : public CXTPReportRecordItem
{
	DECLARE_SERIAL(CFilterFieldValueItem)
public:
	
	CFilterFieldValueItem();
	CFilterFieldValueItem( SAssetField* pField );

	void OnEditChanged(XTP_REPORTRECORDITEM_ARGS* pItemArgs, LPCTSTR szText);
	void OnInplaceButtonDown(CXTPReportInplaceButton* pButton);

	SAssetField* m_pField;
};

class CFilterFieldMaxValueItem : public CXTPReportRecordItem
{
	DECLARE_SERIAL(CFilterFieldMaxValueItem)
public:

	CFilterFieldMaxValueItem();
	CFilterFieldMaxValueItem( SAssetField* pField );

	BOOL OnRequestEdit(XTP_REPORTRECORDITEM_ARGS* pItemArgs);
	void OnEditChanged(XTP_REPORTRECORDITEM_ARGS* pItemArgs, LPCTSTR szText);
	void OnInplaceButtonDown(CXTPReportInplaceButton* pButton);

	SAssetField* m_pField;
};

class CFilterFieldRecord : public CXTPReportRecord
{
	DECLARE_SERIAL(CFilterFieldRecord)
public:

	// Construct record object using empty values on each field
	CFilterFieldRecord();
	// Construct record object from field
	CFilterFieldRecord( SAssetField* pField );
	// Construct record object from object
	CFilterFieldRecord(CFilterFieldRecord* pRecord);
	CFilterFieldRecord& operator= (const CFilterFieldRecord& rOther);

	// Clean up internal objects
	virtual ~CFilterFieldRecord();
	// Create record fields using empty values
	virtual void CreateItems();
	// Overridden callback method, where we can customize any drawing item metrics.
	virtual void GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);

	SAssetField* m_pField;
};


// CAssetBrowserFilterPresetsDlg dialog

class CAssetBrowserFilterPresetsDlg : public CDialog
{
public:

	DECLARE_DYNAMIC(CAssetBrowserFilterPresetsDlg)

	typedef std::map<CString/*preset name*/,CFieldFiltersPreset> TPresetNameAndPresetMap;

	CAssetBrowserFilterPresetsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAssetBrowserFilterPresetsDlg();

	void	FillPresets();
	void	FillFieldsList();
	void	FillDatabaseFieldsTree();
	void	GatherCommonFieldsFromDatabases();
	void	AddFieldsFromTreeItem( HTREEITEM hItem );
	void	SaveCurrentPreset();
	void	RemoveFieldFromList( const char* pFieldDisplayName );
	static void AutoSaveCurrentFilterPreset();

	//
	// Field filtering presets
	//

	//! load the filter presets from the XML file, bClearMap set to true so the rOutFilters is cleared first
	static bool																LoadFieldFiltersPresets( TPresetNameAndPresetMap& rOutFilters, bool bClearMap = true );
	static bool																SaveFieldFiltersPresets( TPresetNameAndPresetMap& rFilters );
	bool																			AddFieldFilterPreset( const char* pPresetName );
	bool																			UpdateFieldFilterPreset( const char* pPresetName );
	bool																			DeleteFieldFilterPreset( const char* pPresetName );
	CFieldFiltersPreset*											GetFieldFilterPresetByName( const char* pPresetName );

	CAssetBrowserDialog*												m_pAssetBrowserDlg;
	std::vector<CAssetBrowserFilterEditDlg*>		m_filterDlgs;
	TPresetNameAndPresetMap											m_fieldFiltersPresets;
	IAssetDisplayDatabase::TAssetFields					m_fields;
	CComboBox																		m_cbPresets;
	CXTPReportControl														m_rptFields;
	CXTTreeCtrl																	m_treeFields;
	CString																			m_currentPresetName;
	std::vector<CString>													m_commonFieldNames;
	std::vector<SAssetField>										m_commonFields;
	bool																				m_bDraggingField, m_bDraggedInsideList;
	HTREEITEM																		m_hDraggedTreeItem, m_hCommonFieldsTreeItem;
	// map of DB name and specific fields
	std::map<CString,IAssetDisplayDatabase::TAssetFields>	m_specificFieldsForEachDatabase;

// Dialog Data
	enum { IDD = IDD_ASSET_BROWSER_EDIT_FILTERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonPresetNew();
	afx_msg void OnBnClickedButtonPresetUpdate();
	afx_msg void OnBnClickedButtonPresetDelete();
	afx_msg void OnBnClickedButtonApplyPresetNow();
	afx_msg void OnBnClickedButtonAutoSavePreset();
	afx_msg void OnCbnSelchangeComboPresets();
	afx_msg void OnBnClickedButtonRenamePreset();
	afx_msg void OnBnClickedButtonAddFields();
	afx_msg void OnBnClickedButtonRemoveFields();
	afx_msg void OnBnClickedButtonRemoveAllFields();
	afx_msg void OnTvnBegindragTreeFields(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonAddAllFields();
	afx_msg void OnBnClickedButtonExpandAll();
	afx_msg void OnBnClickedButtonCollapseAll();
	afx_msg void OnNMDblclkTreeFields(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};

#endif //__AssetBrowserFilterPresetsDlg_h__