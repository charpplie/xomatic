#if !defined(AFX_ProceduralCreationDialog_H__7DCC65C5_79C7_4B64_BDAA_6D8F3A43F7B8__INCLUDED_)
#define AFX_ProceduralCreationDialog_H__7DCC65C5_79C7_4B64_BDAA_6D8F3A43F7B8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Dialogs\BaseFrameWnd.h"
#include "Controls\ListCtrlEx.h"
#include "PanelTreeBrowser.h"
#include "PanelPreview.h"

#define SELECT_OBJECTS "Select"
#define UNSELECT_OBJECTS "Unselect"
#define RELEASE_OBJECTS "Release"

#define RELEASE_SELECTED "Release Selected"
#define RELEASE_CURRENT_FLOOR_OBJECTS "Release Current Floor Objects"
#define DO_NOTHING "Do Nothing"
#define SELECT_NONE "Select None"

class CFloorListView : public CXTPReportControl
{
	DECLARE_DYNCREATE( CFloorListView )

public:
	CFloorListView();

public:
	CXTPReportColumn* m_pFloorNumber;
	CXTPReportColumn* m_pFloorHeight;
	CXTPReportColumn* m_pFloorAutoHeight;
	CXTPReportColumn* m_pShowFloor;

protected:
	class CPaintManager;


	CXTPReportRecordItem* AddItem(CXTPReportRecordItem* pItem);

	afx_msg void OnLButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnLButtonDblClk( UINT nFlags, CPoint point );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint pos );
	afx_msg void OnValueChanged(NMHDR*  pNotifyStruct, LRESULT* result);
	afx_msg void OnRequestEdit(NMHDR*  pNotifyStruct, LRESULT* result);
	afx_msg void OnUpdateCmdUI( CCmdUI* target );

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	
	DECLARE_MESSAGE_MAP()

protected:
	HCURSOR m_hCursorNoDrop;
	HCURSOR m_hCursorNormal;
	CXTPReportRow* m_pSourceRow;
	CXTPReportRow* m_pTargetRow;
	CPoint m_ptDrag;
	bool m_bDragging;
	bool m_bModified;
};

class CFloorViewRecordItemText : public CXTPReportRecordItemText
{
//	DECLARE_SERIAL(CFloorViewRecordItemText)
	public:
		CFloorViewRecordItemText(LPCTSTR szText = _T(""));
		void OnEditChanged(XTP_REPORTRECORDITEM_ARGS* pItemArgs, LPCTSTR szText);
};

class CMEComboBox : public CComboBox
{
protected:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags){ ShowDropDown(TRUE); };
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags){ ShowDropDown(TRUE); };
	DECLARE_MESSAGE_MAP()
};

class CProceduralCreationDialog : public CDialog, public IEditorNotifyListener
{
public:
	CProceduralCreationDialog(CWnd* pParent = NULL);   // standard constructor
	~CProceduralCreationDialog();

	afx_msg void ChangeToFloor(int floorNumber);
	afx_msg void BuildingSelComboSelect(GUID buildingID);
	afx_msg void SetGridEditValue(float val);
	afx_msg void UpdateBuildingSelectionCombo();
	afx_msg void UpdateFloorSelectionList();

	// Copy mode: eAllObjects,eSelectedObjects
	afx_msg void CopyObjects(int copyMode);

	afx_msg void PasteObjects();
	afx_msg void AddFloor();
	afx_msg void DeleteFloor();
	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );
	virtual void OnOK(void);
	virtual void OnCancel(void);
	afx_msg void RefreshReplaceBrowser();
	afx_msg void ResetControls();
	afx_msg void GenerateBuilding();

	// Dialog Data
	enum { IDD = IDD_PROCEDURAL_CREATION_DIALOG };

	enum
	{
		eAllObjects      = 0,
		eSelectedObjects,
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

private:
	afx_msg void AssignObjects();
	afx_msg void BuildingSelComboCloseUP();
	afx_msg void ChangeFloorHeight();
	afx_msg void AddBuilding();
	afx_msg void DeleteBuilding();
	afx_msg void ReleaseObjectsSelComboCloseUP();
	afx_msg void DeleteSelectedObjects();
	afx_msg void ClearObjectsSelection();
	afx_msg void AddObjectsToBuilding();
	afx_msg void SetCurFloorText(int floorNumber);
	afx_msg void ExportBuildings();
	afx_msg void HightLightObjects();
	afx_msg void OnReportItemRClick(NMHDR * pNotifyStruct, LRESULT* /*result*/);
	afx_msg void OnFileChange(CString file);
	afx_msg void OnSelectionBrowserDblClick(CString file);
	afx_msg void DetachSelectedfObjects();

	CEdit m_keywordSelEdit;
	CMEComboBox m_buildingSel;
	CButton m_highlightObjectsChk;
	CButton m_make_Prefab_btn;
	CButton m_detach_btn;
	CStatic m_CurFloorTxt;

	typedef std::map<GUID,CString,guid_less_predicate> TComboSelMap;
	TComboSelMap selComboBuildings;
	CFloorListView m_floorView;

	CRollupCtrl m_ReplaceBrowserRollupCtrl;
	CRollupCtrl m_SelectBrowserRollupCtrl;
	int m_ReplacePageNo;
	int m_SelectPageNo;

	CPanelTreeBrowser *m_ReplaceObjectsTreePanelPtr;
	CPanelTreeBrowser *m_SelectObjectsTreePanelPtr;
};

class CProceduralCreationFrame : public CBaseFrameWnd
{
	DECLARE_DYNCREATE(CProceduralCreationFrame)

public:
	CProceduralCreationFrame();
	enum { IDD = IDD_PROCEDURAL_CREATION_VIEW };
	afx_msg CProceduralCreationDialog* GetProcDialog();

protected:
	afx_msg LRESULT OnKickIdle(WPARAM wParam, LPARAM);
	virtual BOOL OnInitDialog();
	afx_msg void OnSetMaxHeight();
	afx_msg void OnSetUnitSize();
	afx_msg void OnSelectObjectFile();

	CProceduralCreationDialog dlg;
	DECLARE_MESSAGE_MAP()
};
#endif
