////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
// -------------------------------------------------------------------------
//  File name:   AssetResolverDialog.cpp
//  Version:     v1.00
//  Created:     11/12/2012 by Paul Reindell.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AssetResolverDialog.h"
#include "AssetResolver.h"
#include "IAssetSearcher.h"

#include "Util/CryMemFile.h"					// CCryMemFile

#define ASSET_RESOLVE_DIAG_CLASSNAME "Missing Asset Resolver"

// CErrorReportDialog dialog
#define BITMAP_NOT_RESOLVED 0
#define BITMAP_PENDING 2
#define BITMAP_AUTO_RESOLVED 1

#define COLUMN_ICON        0
#define COLUMN_TYPE        1
#define COLUMN_ORGNAME     2
#define COLUMN_NEWNAME     3
#define COLUMN_STATE       4

#define ID_REPORT_CONTROL 100


//////////////////////////////////////////////////////////////////////////
// CMissingAssetMessage
//////////////////////////////////////////////////////////////////////////
class CMissingAssetMessage : public CXTPReportRecord  
{
	DECLARE_DYNAMIC(CMissingAssetMessage)
public:
	CXTPReportRecordItem *m_pIconItem;
	CXTPReportRecordItemText* m_pType;
	CXTPReportRecordItemText* m_pOrgFile;
	CXTPReportRecordItemText* m_pSubstitution;
	CXTPReportRecordItemText* m_pState;

	CMissingAssetMessage( CMissingAssetRecord *pRecord )
	{
		CString type, oldfile, newfile, state;
		int nIcon;
		GetData(pRecord, nIcon, type, oldfile, newfile, state);

		m_pIconItem = AddItem(new CXTPReportRecordItem());
		m_pType = (CXTPReportRecordItemText*) AddItem(new CXTPReportRecordItemText(type));
		m_pOrgFile = (CXTPReportRecordItemText*) AddItem(new CXTPReportRecordItemText(oldfile));
		m_pSubstitution = (CXTPReportRecordItemText*) AddItem(new CXTPReportRecordItemText(newfile));
		m_pState = (CXTPReportRecordItemText*) AddItem(new CXTPReportRecordItemText(state));

		m_pIconItem->SetIconIndex(nIcon);
		m_pIconItem->SetGroupPriority(nIcon);
		m_pIconItem->SetSortPriority(nIcon);

		pRecord->pRecordMessage = this;
	}

	void Update(CMissingAssetRecord *pRecord)
	{
		CString type, oldfile, newfile, state;
		int nIcon;
		GetData(pRecord, nIcon, type, oldfile, newfile, state);

		m_pSubstitution->SetValue(newfile);
		m_pState->SetValue(state);

		m_pIconItem->SetIconIndex(nIcon);
		m_pIconItem->SetGroupPriority(nIcon);
		m_pIconItem->SetSortPriority(nIcon);

		pRecord->SetUpdated();
	}

	virtual void GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics)
	{
		if (m_pIconItem == pDrawArgs->pItem)
		{
			switch(pDrawArgs->pItem->GetIconIndex())
			{
			case BITMAP_PENDING:       pItemMetrics->clrForeground = RGB(255, 255, 0); break;
			case BITMAP_AUTO_RESOLVED: pItemMetrics->clrForeground = RGB(0,   255, 0); break;
			case BITMAP_NOT_RESOLVED:  pItemMetrics->clrForeground = RGB(255, 0,   0); break;
			}
		}
	}

private:
	void GetData(CMissingAssetRecord *pRecord, int& nIcon, CString& type, CString& oldfile, CString& newfile, CString& state)
	{
		newfile = pRecord->substitutions.empty() ? "" : pRecord->substitutions.front();
		oldfile = pRecord->orgname;

		IAssetSearcher* pSearcher = GetIEditor()->GetMissingAssetResolver()->GetAssetSearcherById(pRecord->searcherId);
		type = pSearcher ? pSearcher->GetAssetTypeName(pRecord->assetTypeId) : "UNDEFINED";

		if (pSearcher)
			state = pSearcher->GetName() + CString(": ");
		else
			state = "";

		switch (pRecord->state)
		{
		case CMissingAssetRecord::ESTATE_PENDING:       nIcon = BITMAP_PENDING;       state += "Searching"; break;
		case CMissingAssetRecord::ESTATE_AUTO_RESOLVED: nIcon = BITMAP_AUTO_RESOLVED; state += "Resolved";  break;
		case CMissingAssetRecord::ESTATE_NOT_RESOLVED:  nIcon = BITMAP_NOT_RESOLVED;  state += "Not found"; break;
		case CMissingAssetRecord::ESTATE_ACCEPTED:			nIcon = BITMAP_AUTO_RESOLVED; state += "Accepted";  break;
		case CMissingAssetRecord::ESTATE_CANCELLED:			nIcon = BITMAP_AUTO_RESOLVED; state += "Cancelled"; break;
		default: state = "UNDEFINED";
		}

		if (pRecord->substitutions.size() > 1)
			state += string().Format(" (%i Files)", pRecord->substitutions.size()).c_str();
	}
};
IMPLEMENT_DYNAMIC(CMissingAssetMessage,CXTPReportRecord)

//////////////////////////////////////////////////////////////////////////
// CAssetResolverDialogViewPaneClass
//////////////////////////////////////////////////////////////////////////
class CAssetResolverDialogViewPaneClass : public TRefCountBase<IViewPaneClass>
{
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	virtual REFGUID ClassID()
	{
		static const GUID guid =
			{ 0x52977EA0, 0x8791, 0x4840, { 0x9A, 0xF4, 0x94, 0xBC, 0x96, 0x46, 0xBD, 0xB3 } };
		return guid;
	}
	virtual const char* ClassName() { return ASSET_RESOLVE_DIAG_CLASSNAME; };
	virtual const char* Category() { return "Editor"; };

	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CMissingAssetDialog); };
	virtual const char* GetPaneTitle() { return ASSET_RESOLVE_DIAG_CLASSNAME; };
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
	virtual CRect GetPaneRect() { return CRect(0,0,600,200); };
	virtual CSize GetMinSize() { return CSize(200,100); }
	virtual bool SinglePane() { return true; };
	virtual bool WantIdleUpdate() { return true; };
};


//////////////////////////////////////////////////////////////////////////
// CMissingAssetDialog
//////////////////////////////////////////////////////////////////////////
CMissingAssetDialog* CMissingAssetDialog::m_instance = 0;

IMPLEMENT_DYNCREATE(CMissingAssetDialog,CXTResizeDialog)

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CMissingAssetDialog, CXTResizeDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_SIZE()

	ON_NOTIFY(NM_RCLICK, ID_REPORT_CONTROL, OnReportItemRClick)
	ON_NOTIFY(NM_DBLCLK, ID_REPORT_CONTROL, OnReportItemDblClick)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
CMissingAssetDialog::CMissingAssetDialog( CWnd* pParent /*=NULL*/)
	: CXTResizeDialog(CMissingAssetDialog::IDD, pParent)
{
	m_instance = this;
	Create( IDD,pParent );
}

//////////////////////////////////////////////////////////////////////////
CMissingAssetDialog::~CMissingAssetDialog()
{
	Clear();
	m_instance = NULL;
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass( new CAssetResolverDialogViewPaneClass );
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::Open()
{
	if(!m_instance)
	{
		GetIEditor()->OpenView( ASSET_RESOLVE_DIAG_CLASSNAME );
	}
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::Close()
{
	if (m_instance)
	{
		m_instance->DestroyWindow();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::Update()
{
	if(!m_instance)
		return;

	m_instance->UpdateReport();
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
}

//////////////////////////////////////////////////////////////////////////
BOOL CMissingAssetDialog::OnInitDialog()
{
	__super::OnInitDialog();

	VERIFY( m_wndReport.Create(WS_CHILD|WS_TABSTOP|WS_VISIBLE|WM_VSCROLL, CRect(0, 0, 0, 0), this, ID_REPORT_CONTROL) );

	//m_imageList.Create(IDB_ERROR_REPORT, 16, 1, RGB (255, 255, 255));
	CMFCUtils::LoadTrueColorImageList( m_imageList,IDB_ERROR_REPORT,16,RGB(255,255,255) );

	m_wndReport.SetImageList(&m_imageList); 

	CXTPReportColumn *pSortCol = 0;

	//  Add sample columns
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_ICON, _T(""), 18, FALSE ));
	m_wndReport.AddColumn(pSortCol=new CXTPReportColumn(COLUMN_TYPE, _T("Type"), 30, TRUE));
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_ORGNAME, _T("Original File"), 100, TRUE));
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_NEWNAME, _T("Resolved File"), 100, TRUE));
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_STATE, _T("State"), 30, TRUE));

	m_wndReport.GetPaintManager()->m_clrHyper = ::GetSysColor(COLOR_HIGHLIGHT);

	m_wndReport.GetColumns()->GetGroupsOrder()->Add( pSortCol );

	AutoLoadPlacement( "Dialogs\\MissingAssetDialog" );

	UINT nSize = 0;
	LPBYTE pbtData = NULL;
	CXTRegistryManager regManager;
	if (regManager.GetProfileBinary( "Dialogs\\MissingAssetDialog", "Configuration", &pbtData, &nSize))
	{
		CCryMemFile memFile( pbtData, nSize );
		CArchive ar( &memFile, CArchive::load );
		m_wndReport.SerializeState( ar );
	}


	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::OnSize( UINT nType,int cx,int cy )
{
	__super::OnSize(nType,cx,cy);

	if (m_wndReport)
	{
		CRect rc;
		GetClientRect(rc);
		m_wndReport.MoveWindow(rc);
	}
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::OnSysCommand(UINT nID, LPARAM lParam)
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
void CMissingAssetDialog::Clear()
{
	CMissingAssetReport* pReport = GetReport();
	CMissingAssetReport::SRecordIterator iter = pReport->GetIterator();
	while (CMissingAssetRecord* pRecord = iter.Next())
		pRecord->pRecordMessage = NULL;
	m_wndReport.ResetContent();
	pReport->SetUpdated(false);
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::UpdateReport()
{
	CMissingAssetReport* pReport = GetReport();
	if(!pReport || !pReport->NeedUpdate())
		return;

	m_wndReport.RedrawControl();

	RedrawWindow();

	m_wndReport.BeginUpdate();

	CMissingAssetReport::SRecordIterator iter = pReport->GetIterator();
	while (CMissingAssetRecord* pRecord = iter.Next())
	{
		if (pRecord->pRecordMessage == NULL)
		{
			if (!pRecord->NeedRemove())
				m_wndReport.AddRecord( new CMissingAssetMessage(pRecord) );
		}
		else if (pRecord->NeedRemove())
		{
			m_wndReport.RemoveRecordEx(pRecord->pRecordMessage);
		}
		else if (pRecord->NeedUpdate())
		{
			pRecord->pRecordMessage->Update(pRecord);
			m_wndReport.UpdateRecord(pRecord->pRecordMessage, TRUE);
		}
	}
	
	m_wndReport.EndUpdate();
	m_wndReport.Populate();

	CRect rc;
	GetClientRect(rc);
	m_wndReport.MoveWindow(rc);

	pReport->SetUpdated(true);
}

//////////////////////////////////////////////////////////////////////////
CMissingAssetReport* CMissingAssetDialog::GetReport()
{
	return GetIEditor()->GetMissingAssetResolver()->GetReport();
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::GetSelectedRecords(TRecords& records)
{
	CMissingAssetReport* pReport = GetReport();
	if(!pReport) return;

	int count = m_wndReport.GetSelectedRows()->GetCount();
	for (int i = 0; i < count; ++i)
	{
		CMissingAssetRecord* pRecord = pReport->GetRecord( (CMissingAssetMessage*)m_wndReport.GetSelectedRows()->GetAt(i)->GetRecord());
		if (pRecord)
			records.push_back(pRecord);
	}
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::PostNcDestroy()
{
	__super::PostNcDestroy();
	if (m_instance)
		delete m_instance;
	m_instance = 0;
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::OnReportItemRClick(NMHDR * pNotifyStruct, LRESULT * /*result*/)
{
	TRecords records;
	GetSelectedRecords(records);

	enum EMenuItem
	{
		eMI_Nothing = 0,
		eMI_OpenBrowser,
		eMI_Accept,
		eMI_Cancel,
		eMI_AcceptAll,
		eMI_CancelAll,
		eMI_AcceptSubstitution,
	};

	CMenu menu;
	menu.CreatePopupMenu();

	const int recordCount = records.size();

	if (recordCount == 1)
	{
		menu.AppendMenu( MF_STRING, eMI_OpenBrowser, "Find manually");
		menu.AppendMenu( MF_SEPARATOR );
		const std::vector<CString>& substitutions = records.front()->substitutions;
		if (substitutions.size() > 1)
		{
			int i = 0;
			for (std::vector<CString>::const_iterator it = substitutions.begin(); it != substitutions.end(); ++it)
			{
				CString str = "Accept: ";
				str += *it;
				menu.AppendMenu( MF_STRING, eMI_AcceptSubstitution + i++, str);
			}
			menu.AppendMenu( MF_SEPARATOR );
		}
		if (!substitutions.empty())
			menu.AppendMenu( MF_STRING, eMI_Accept, "Accept new file");
		menu.AppendMenu( MF_STRING, eMI_Cancel, "Cancel file");
		menu.AppendMenu( MF_SEPARATOR );
	}
	else if (recordCount > 1)
	{
		menu.AppendMenu( MF_STRING, eMI_Accept, "Accept all selected");
		menu.AppendMenu( MF_STRING, eMI_Cancel, "Cancel all selected");
		menu.AppendMenu( MF_SEPARATOR );
	}

	menu.AppendMenu( MF_STRING, eMI_AcceptAll, "Accept all resolved files");
	menu.AppendMenu( MF_STRING, eMI_CancelAll, "Cancel all files");

	CPoint point;
	GetCursorPos( &point );

	const int commandId = menu.TrackPopupMenu( TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_NONOTIFY, point.x, point.y, this );

	switch (commandId)
	{
	case eMI_OpenBrowser:
		assert(recordCount == 1);
		ResolveRecord(records.front());
		break;
	case eMI_Accept:
		for (TRecords::iterator it = records.begin(); it != records.end(); ++it)
		{
			CMissingAssetRecord* pRecord = *it;
			if (!pRecord->substitutions.empty())
				AcceptRecort(pRecord);
		}
		break;
	case eMI_Cancel:
		for (TRecords::iterator it = records.begin(); it != records.end(); ++it)
			CancelRecort(*it);
		break;
	case eMI_AcceptAll:
		if (CMissingAssetReport* pReport = GetReport())
		{
			CMissingAssetReport::SRecordIterator iter = pReport->GetIterator();
			while (CMissingAssetRecord* pRecord = iter.Next())
			{
				if (!pRecord->substitutions.empty())
					AcceptRecort(pRecord);
			}
		}
		break;
	case eMI_CancelAll:
		if (CMissingAssetReport* pReport = GetReport())
		{
			CMissingAssetReport::SRecordIterator iter = pReport->GetIterator();
			while (CMissingAssetRecord* pRecord = iter.Next())
				CancelRecort(pRecord);
		}
		break;
	default:
		if (commandId >= eMI_AcceptSubstitution)
		{
			int substitutionIdx = commandId - eMI_AcceptSubstitution;
			assert(recordCount == 1);
			AcceptRecort(records.front(), substitutionIdx);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::OnReportItemDblClick(NMHDR * pNotifyStruct, LRESULT * result)
{
	TRecords records;
	GetSelectedRecords(records);
	if (records.size() == 1)
		ResolveRecord(records.front());
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::AcceptRecort(CMissingAssetRecord* pRecord, int idx)
{
	assert(pRecord->substitutions.size() > idx);
	GetIEditor()->GetMissingAssetResolver()->AcceptRequest(pRecord->id, pRecord->substitutions[idx].GetString());
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::CancelRecort(CMissingAssetRecord* pRecord)
{
	GetIEditor()->GetMissingAssetResolver()->CancelRequest(pRecord->id);
}

//////////////////////////////////////////////////////////////////////////
void CMissingAssetDialog::ResolveRecord(CMissingAssetRecord* pRecord)
{
	IAssetSearcher* pSearcher = GetIEditor()->GetMissingAssetResolver()->GetAssetSearcherById(pRecord->searcherId);

	CString fullFileName;
	if(pSearcher && pSearcher->GetReplacement(fullFileName, pRecord->assetTypeId))
	{
		GetIEditor()->GetMissingAssetResolver()->AcceptRequest(pRecord->id, fullFileName.GetString());
	}
}
