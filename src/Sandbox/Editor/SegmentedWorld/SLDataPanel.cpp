#include "StdAfx.h"
#include "SLDataPanel.h"
#include "SourceControlDescDlg.h"
#include <ISourceControl.h>

#define IDC_SLDATA_TREECTRL 1

#define ITEM_IMAGE_OVERLAY_CGF          8
#define ITEM_IMAGE_OVERLAY_INPAK        9
#define ITEM_IMAGE_OVERLAY_READONLY     10
#define ITEM_IMAGE_OVERLAY_ONDISK       11
#define ITEM_IMAGE_OVERLAY_LOCKED       12
#define ITEM_IMAGE_OVERLAY_CHECKEDOUT   13
#define ITEM_IMAGE_OVERLAY_NO_CHECKOUT  14

//////////////////////////////////////////////////////////////////////////
// CSLBaseItemRecord

void CSLBaseItemRecord::UpdateStatus()
{
	if(!GetIEditor()->IsSourceControlAvailable())
		return;

	uint32 nFileAttr = GetFileSCMAttributes();

	int nIcon = -1;
	if(!(nFileAttr & SCC_FILE_ATTRIBUTE_MANAGED))
	{
		nIcon = ITEM_IMAGE_OVERLAY_ONDISK;
	}
	else
	{
		if(nFileAttr&SCC_FILE_ATTRIBUTE_READONLY)
			nIcon = ITEM_IMAGE_OVERLAY_READONLY;
		else if(nFileAttr&SCC_FILE_ATTRIBUTE_CHECKEDOUT)
		{
			if(nFileAttr&SCC_FILE_ATTRIBUTE_BYANOTHER)
			{
				//string desc;
				//char username[64];
				//if(!GetIEditor()->GetSourceControl()->GetOtherUser(m_filename, username, 64))
				//	strcpy(username, "another user");
				//desc.Format("Locked by %s", username);
				nIcon = ITEM_IMAGE_OVERLAY_LOCKED;
			}
			else
				nIcon = ITEM_IMAGE_OVERLAY_CHECKEDOUT;
		}
	}
	SetIcon2(nIcon);
}

//////////////////////////////////////////////////////////////////////////
// CSLDataTreeCtrl

BEGIN_MESSAGE_MAP(CSLDataTreeCtrl, CTreeCtrlReport)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnNMRclick)
END_MESSAGE_MAP()

CSLDataTreeCtrl::CSLDataTreeCtrl()
{
	CMFCUtils::LoadTrueColorImageList(m_imageList,IDB_MATERIAL_TREE,20,RGB(255,0,255));
	CMFCUtils::LoadTrueColorImageList(m_imageList,IDB_FILE_STATUS,20,RGB(255,0,255));

	SetMultipleSelection(TRUE);
	SetImageList(&m_imageList);
	
	CXTPReportColumn *pCol1 = AddTreeColumn("");
	pCol1->SetSortable(TRUE);
	GetColumns()->SetSortColumn(pCol1,TRUE);
	SetExpandOnDblClick(true);
}

void CSLDataTreeCtrl::OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	CPoint point;
	GetCursorPos( &point );
	ScreenToClient( &point );

	CXTPReportRow* pRow = HitTest(point);
	if(pRow)
	{
		GetCursorPos( &point );
		CSLBaseItemRecord *pRecord = (CSLBaseItemRecord *)pRow->GetRecord();
		CSLDataPanel *pPanel = (CSLDataPanel *)GetParent();
		assert(pPanel);
		pPanel->ShowContextMenu(pRecord, point);
		Populate();
	}
}

//////////////////////////////////////////////////////////////////////////
// CSLDataPanel

BOOL CSLDataPanel::OnInitDialog()
{
	CRect rc;  
	GetClientRect( rc );
	m_tree.Create(WS_CHILD|WS_VISIBLE, rc, this, IDC_SLDATA_TREECTRL);

	return TRUE;
}

void CSLDataPanel::ShowContextMenu(CSLBaseItemRecord *pRecord, const CPoint &pos)
{
	if(!pRecord)
		return;

	if(!GetIEditor()->IsSourceControlAvailable())
		return;

	uint32 nFileAttr = pRecord->GetFileSCMAttributes();

	if(nFileAttr == SCC_FILE_ATTRIBUTE_INVALID)
		return;

	CMenu menu;
	VERIFY(menu.CreatePopupMenu());
	if(!(nFileAttr & SCC_FILE_ATTRIBUTE_MANAGED))
	{
		menu.AppendMenu(MF_STRING, MENU_SCM_ADD, "Add To Source Control");
	}
	else
	{
		menu.AppendMenu(MF_STRING | ((nFileAttr&SCC_FILE_ATTRIBUTE_READONLY || nFileAttr & SCC_FILE_ATTRIBUTE_INPAK) ? 0 : MF_GRAYED), MENU_SCM_CHECK_OUT, "Check Out");
		menu.AppendMenu(MF_STRING | (nFileAttr&SCC_FILE_ATTRIBUTE_CHECKEDOUT ? 0 : MF_GRAYED), MENU_SCM_UNDO_CHECK_OUT, "Undo Check Out");
		menu.AppendMenu(MF_STRING | (nFileAttr&SCC_FILE_ATTRIBUTE_CHECKEDOUT ? 0 : MF_GRAYED), MENU_SCM_CHECK_IN, "Check In");
		menu.AppendMenu(MF_STRING, MENU_SCM_GET_LATEST, "Get Latest Version");
		menu.AppendMenu(MF_STRING, MENU_SCM_HISTORY, "Show History");
	}

	int cmd = CXTPCommandBars::TrackPopupMenu(&menu, TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_RIGHTBUTTON, pos.x, pos.y, this, NULL);
	DoSourceControlOp(pRecord, cmd);

	if(cmd != 0)
		pRecord->UpdateStatus();
}

void CSLDataPanel::DoSourceControlOp(CSLBaseItemRecord *pRecord, int scmOp)
{
	if(!pRecord)
		return;

	if(!GetIEditor()->IsSourceControlAvailable())
		return;

	std::vector<CString> filenames;
	pRecord->GetFilename(filenames);

	if(!filenames.size())
		return;

	bool bRes = true;
	CString errmsg("Check if Source Control Provider correctly setup and working directory is correct.");
	ISourceControl *pSourceControl = GetIEditor()->GetSourceControl();
	switch(scmOp)
	{
	case MENU_SCM_ADD:
		{
			CSourceControlDescDlg dlg;
			if(dlg.DoModal()==IDOK)
			{
				char changeid[16];
				bRes = pSourceControl->CreateChangeList(dlg.m_sDesc, changeid, sizeof(changeid));
				if(!bRes)
				{
					errmsg.Format("Failed to create change list with description: %s.", dlg.m_sDesc);
					break;
				}
				else
				{
					for(int i = 0; i < filenames.size(); i++)
						pSourceControl->Add(filenames[i], 0, ADD_WITHOUT_SUBMIT|ADD_CHANGELIST, changeid);
					bRes &= pSourceControl->SubmitChangeList(changeid);
				}
			}
		}
		break;

	case MENU_SCM_CHECK_IN:
		{
			CSourceControlDescDlg dlg;
			if(dlg.DoModal()==IDOK)
			{
				char changeid[16];
				bRes = pSourceControl->CreateChangeList(dlg.m_sDesc, changeid, sizeof(changeid));
				if(!bRes)
				{
					errmsg.Format("Failed to create change list with description: %s.", dlg.m_sDesc);
					break;
				}
				else
				{
					for(int i = 0; i < filenames.size(); i++)
						bRes &= pSourceControl->Reopen(filenames[i], changeid);
					bRes &= pSourceControl->SubmitChangeList(changeid);
				}
			}
		}
		break;

	case MENU_SCM_CHECK_OUT:
		{
			if(pRecord->GetFileSCMAttributes() & SCC_FILE_ATTRIBUTE_BYANOTHER)
			{
				char username[64];
				if(!pSourceControl->GetOtherUser(filenames[0], username, 64))
					strcpy(username, "another user");
				CString str;
				str.Format( _T("This file is checked out by %s. Try to continue?"), username);
				if( AfxMessageBox( str, MB_YESNO|MB_ICONQUESTION ) != IDYES)
					return;
			}
			for(int i = 0; i < filenames.size(); i++)
			{
				bRes &= pSourceControl->GetLatestVersion(filenames[i]);
				bRes &= pSourceControl->CheckOut(filenames[i]);
			}
		}
		break;

	case MENU_SCM_UNDO_CHECK_OUT:
		for(int i = 0; i < filenames.size(); i++)
			bRes &= pSourceControl->UndoCheckOut(filenames[i]);
		break;

	case MENU_SCM_GET_LATEST:
		for(int i = 0; i < filenames.size(); i++)
			bRes &= GetIEditor()->GetSourceControl()->GetLatestVersion(filenames[i]);
		break;

	case MENU_SCM_HISTORY:
		bRes = GetIEditor()->GetSourceControl()->History(filenames[0]);
		if (bRes)
			return;
		break;
	}

	if(!bRes)
	{
		errmsg = "Source Control Operation Failed.\r\n" + errmsg;
		MessageBox(errmsg, "Error", MB_OK | MB_ICONERROR);
		return;
	}

	pRecord->UpdateStatus();
}
