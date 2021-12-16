////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeErrorDialog.cpp
//  Version:     v1.00
//  Created:     14/9/2011 by Paul Reindell
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SelectionTreeErrorDialog.h"

#include "Util/CryMemFile.h"					// CCryMemFile

#define BST_ERROR_DIAG_CLASSNAME "BSTErrorDialog"

// CErrorReportDialog dialog
#define BITMAP_ERROR 0
#define BITMAP_WARNING 1
#define BITMAP_COMMENT 2

#define COLUMN_SEVERITY    0
#define COLUMN_ERROR       1
#define COLUMN_MODULE      2
#define COLUMN_TYPE        3  
#define COLUMN_FILE        4
#define COLUMN_DESCRIPTION 5

#define ID_REPORT_CONTROL 100


//////////////////////////////////////////////////////////////////////////
// CSelectionTreeErrorMessageRecord
//////////////////////////////////////////////////////////////////////////
class CSelectionTreeErrorMessageRecord : public CXTPReportRecord  
{
	DECLARE_DYNAMIC(CSelectionTreeErrorMessageRecord)
public:
	CXTPReportRecordItem *m_pIconItem;

	CSelectionTreeErrorMessageRecord( const CSelectionTreeErrorRecord &err )
	{
		CString error,file,group,module,desc;
		int count = 0;//err.count;

		error = err.error;
		desc = err.description;
		file = err.file;
		group = err.group;
		if (err.type == CSelectionTreeErrorRecord::ETYPE_REFERENCE)
			group += " (Ref)";



		switch (err.module)
		{
		case CSelectionTreeErrorRecord::EMODULE_TREE: module = "SelectionTree"; break;
		case CSelectionTreeErrorRecord::EMODULE_SIGNALS: module = "Signals"; break;
		case CSelectionTreeErrorRecord::EMODULE_VARIABLES: module = "Variables"; break;
		case CSelectionTreeErrorRecord::EMODULE_LEAFTRANSLATIONS: module = "LeafMapping"; break;
		default:
			module = "Unknown";
		}

		m_pIconItem = AddItem(new CXTPReportRecordItem());
		AddItem(new CXTPReportRecordItemText(error));
		AddItem(new CXTPReportRecordItemText(module));
		AddItem(new CXTPReportRecordItemText(group));
		AddItem(new CXTPReportRecordItemText(file));
		AddItem(new CXTPReportRecordItemText(desc));


		int nIcon = 0;
 		if (err.severity == CSelectionTreeErrorRecord::ESEVERITY_ERROR)
 			nIcon = BITMAP_ERROR;
 		else if (err.severity == CSelectionTreeErrorRecord::ESEVERITY_WARNING)
 			nIcon = BITMAP_WARNING;
 		else if (err.severity == CSelectionTreeErrorRecord::ESEVERITY_COMMENT)
 			nIcon = BITMAP_COMMENT;
		m_pIconItem->SetIconIndex(nIcon);
		m_pIconItem->SetGroupPriority(nIcon);
		m_pIconItem->SetSortPriority(nIcon);
	}

	virtual void GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics)
	{
		if (m_pIconItem == pDrawArgs->pItem && pDrawArgs->pItem->GetIconIndex() == 0)
		{
			// Red error text.
			pItemMetrics->clrForeground = RGB(0xFF, 0, 0);
		}
	}
};
IMPLEMENT_DYNAMIC(CSelectionTreeErrorMessageRecord,CXTPReportRecord)

//////////////////////////////////////////////////////////////////////////
// CCSelectionTreeErrorDialogViewPaneClass
//////////////////////////////////////////////////////////////////////////
class CCSelectionTreeErrorDialogViewPaneClass : public TRefCountBase<IViewPaneClass>
{
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	virtual REFGUID ClassID()
	{
		static const GUID guid = 
			{ 0xea523b7e, 0x3f63, 0x821b, { 0x48, 0x23, 0xa1, 0x31, 0xfc, 0x5b, 0x46, 0xa3 } };
		return guid;
	}
	virtual const char* ClassName() { return BST_ERROR_DIAG_CLASSNAME; };
	virtual const char* Category() { return "Editor"; };

	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CSelectionTreeErrorDialog); };
	virtual const char* GetPaneTitle() { return _T("SelectionTree Error Report"); };
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
	virtual CRect GetPaneRect() { return CRect(0,0,600,200); };
	virtual CSize GetMinSize() { return CSize(200,100); }
	virtual bool SinglePane() { return true; };
	virtual bool WantIdleUpdate() { return true; };
};


//////////////////////////////////////////////////////////////////////////
// CSelectionTreeErrorDialog
//////////////////////////////////////////////////////////////////////////
CSelectionTreeErrorDialog* CSelectionTreeErrorDialog::m_instance = 0;

IMPLEMENT_DYNCREATE(CSelectionTreeErrorDialog,CXTResizeDialog)

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass( new CCSelectionTreeErrorDialogViewPaneClass );
}


CSelectionTreeErrorDialog::CSelectionTreeErrorDialog( CWnd* pParent /*=NULL*/)
	: CXTResizeDialog(CSelectionTreeErrorDialog::IDD, pParent)
{
	m_instance = this;
	m_pErrorReport = 0;

	Create( IDD,pParent );
}

CSelectionTreeErrorDialog::~CSelectionTreeErrorDialog()
{
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::Open( CSelectionTreeErrorReport *pReport )
{
	bool reload = false;
	if(!m_instance)
	{
		GetIEditor()->OpenView( BST_ERROR_DIAG_CLASSNAME );
		reload = true;
	}

	if(!m_instance)
		return;

	reload |= m_instance->m_pErrorReport != pReport;
	if (reload)
	{
		m_instance->SetReport( pReport );
		m_instance->UpdateErrors();
	}
}

BEGIN_MESSAGE_MAP(CSelectionTreeErrorDialog, CXTResizeDialog)
	ON_NOTIFY(NM_DBLCLK, IDC_ERRORS, OnNMDblclkErrors)
	ON_BN_CLICKED(IDC_SELECTOBJECTS, OnSelectObjects)
	ON_WM_SYSCOMMAND()
	ON_WM_SIZE()

	ON_NOTIFY(NM_CLICK, ID_REPORT_CONTROL, OnReportItemClick)
	ON_NOTIFY(NM_RCLICK, ID_REPORT_CONTROL, OnReportItemRClick)
	ON_NOTIFY(NM_DBLCLK, ID_REPORT_CONTROL, OnReportItemDblClick)
	ON_NOTIFY(XTP_NM_REPORT_HEADER_RCLICK, ID_REPORT_CONTROL, OnReportColumnRClick)
	ON_NOTIFY(NM_KEYDOWN, ID_REPORT_CONTROL, OnReportKeyDown)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::Close()
{
	if (m_instance)
	{
		m_instance->DestroyWindow();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::Clear()
{
	if (m_instance)
	{
		m_instance->SetReport( 0 );
		m_instance->UpdateErrors();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::Reload()
{
	if(!m_instance)
		return;

	m_instance->UpdateErrors();
}


//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
}



// CErrorReportDialog message handlers

BOOL CSelectionTreeErrorDialog::OnInitDialog()
{
	__super::OnInitDialog();

	VERIFY( m_wndReport.Create(WS_CHILD|WS_TABSTOP|WS_VISIBLE|WM_VSCROLL, CRect(0, 0, 0, 0), this, ID_REPORT_CONTROL) );

	//m_imageList.Create(IDB_ERROR_REPORT, 16, 1, RGB (255, 255, 255));
	CMFCUtils::LoadTrueColorImageList( m_imageList,IDB_ERROR_REPORT,16,RGB(255,255,255) );

	m_wndReport.SetImageList(&m_imageList); 

	CXTPReportColumn *pModuleCol = 0;

	//  Add sample columns
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_SEVERITY, _T(""), 18, FALSE ));
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_ERROR, _T("Error"), 40, TRUE));
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_MODULE, _T("Type"), 30, TRUE));
	m_wndReport.AddColumn(pModuleCol=new CXTPReportColumn(COLUMN_TYPE, _T("Tree/Reference"), 20, TRUE));
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_FILE, _T("File"), 30, TRUE));
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_DESCRIPTION, _T("Description"), 20, TRUE));

	m_wndReport.GetPaintManager()->m_clrHyper = ::GetSysColor(COLOR_HIGHLIGHT);

	m_wndReport.GetColumns()->GetGroupsOrder()->Add( pModuleCol );

	AutoLoadPlacement( "Dialogs\\SelectionTreeErrorReport" );

	UINT nSize = 0;
	LPBYTE pbtData = NULL;
	CXTRegistryManager regManager;
	if (regManager.GetProfileBinary( "Dialogs\\SelectionTreeErrorReport", "Configuration", &pbtData, &nSize))
	{
		CCryMemFile memFile( pbtData, nSize );
		CArchive ar( &memFile, CArchive::load );
		m_wndReport.SerializeState( ar );
	}


	return TRUE;
}

void CSelectionTreeErrorDialog::UpdateErrors()
{
	ReloadErrors();
	m_wndReport.RedrawControl();

	RedrawWindow();

	if (m_pErrorReport)
		m_pErrorReport->SetReloaded();

	CRect rc;
	GetClientRect(rc);
	m_wndReport.MoveWindow(rc);
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::OnSize( UINT nType,int cx,int cy )
{
	__super::OnSize(nType,cx,cy);

	if (m_wndReport)
	{
		CRect rc;
		GetClientRect(rc);
		m_wndReport.MoveWindow(rc);
	}
}

void CSelectionTreeErrorDialog::OnSysCommand(UINT nID, LPARAM lParam)
{
	if (nID == SC_CLOSE)
	{
		Close();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}



//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::ReloadErrors()
{
	m_wndReport.ResetContent();

	if(!m_pErrorReport)
		return;
	
	m_wndReport.BeginUpdate();

	// Store localarray of error records.
	m_errorRecords.clear();
	m_errorRecords.reserve( m_pErrorReport->GetErrorCount() );
	for (int i = 0; i < m_pErrorReport->GetErrorCount(); i++)
	{
		const CSelectionTreeErrorRecord &err = m_pErrorReport->GetError(i);
		m_errorRecords.push_back( err );
	}
	
	for (int i = 0; i < m_errorRecords.size(); i++)
	{
		const CSelectionTreeErrorRecord &err = m_errorRecords[i];
		m_wndReport.AddRecord( new CSelectionTreeErrorMessageRecord(err) );
	}

	m_wndReport.EndUpdate();
	m_wndReport.Populate();
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::PostNcDestroy()
{
	__super::PostNcDestroy();
	if (m_instance)
		delete m_instance;
	m_instance = 0;
}

void CSelectionTreeErrorDialog::OnNMDblclkErrors(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::OnOK()
{
	//DestroyWindow();
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::OnCancel()
{
	DestroyWindow();
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::OnSelectObjects()
{
}

//////////////////////////////////////////////////////////////////////////

void CSelectionTreeErrorDialog::OnReportColumnRClick(NMHDR * pNotifyStruct, LRESULT * /*result*/)
{
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::CopyToClipboard()
{
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::OnReportItemRClick(NMHDR * pNotifyStruct, LRESULT * /*result*/)
{
}


//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::OnReportItemClick(NMHDR * pNotifyStruct, LRESULT * /*result*/)
{
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::OnReportKeyDown(NMHDR * pNotifyStruct, LRESULT * /*result*/)
{
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorDialog::OnReportItemDblClick(NMHDR * pNotifyStruct, LRESULT * result)
{
}
