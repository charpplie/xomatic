// TrackViewNodes.cpp : implementation file
//

#include "stdafx.h"
#include "TrackViewSequences.h"
#include "TrackViewDialog.h"
#include "StringDlg.h"

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CTrackViewSequencesCtrl, CTreeCtrlReport)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnNMRclick)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
CTrackViewSequencesCtrl::CTrackViewSequencesCtrl()
{
	CMFCUtils::LoadTrueColorImageList( m_imageList,IDB_TRACKVIEW_NODES,16,RGB(255,0,255) );
	SetImageList( &m_imageList );

	CXTPReportColumn *pNodeCol   
		= AddColumn(new CXTPReportColumn(0, _T("Node"), 150, TRUE, XTP_REPORT_NOICON, TRUE, TRUE));
	pNodeCol->SetTreeColumn(true);
	pNodeCol->SetSortable(FALSE);

	ShowHeader(FALSE);

	SetMultipleSelection(FALSE);

	SetGroupRowsBold(TRUE);
	//SetTreeIndent(30);

	GetReportHeader()->AllowColumnRemove(FALSE);
	GetReportHeader()->AllowColumnSort(FALSE);

	AllowEdit(TRUE);
	EditOnClick(FALSE);
	m_pTrackViewDialog = 0;
};


//////////////////////////////////////////////////////////////////////////
void CTrackViewSequencesCtrl::OnFillItems()
{
	CRecord *pRootGroupRec = new CRecord(true,"Sequences");
	pRootGroupRec->SetExpanded(TRUE);
	AddRecord( pRootGroupRec );

	std::map<CString,CRecord*> mapGroups;

	ISequenceIt *pSequenceIt = gEnv->pMovieSystem->GetSequences();

	//////////////////////////////////////////////////////////////////////////
	// Make Node Group Items.
	//////////////////////////////////////////////////////////////////////////
	for (IAnimSequence *pSequence = pSequenceIt->first(); pSequence != 0; pSequence = pSequenceIt->next())
	{
		CString sGroupName = pSequence->GetGroupName();
		if (sGroupName.IsEmpty())
			continue;
		CRecord *pGroupRecord = stl::find_in_map(mapGroups,sGroupName,0);
		if (!pGroupRecord)
		{
			pGroupRecord = new CRecord(true,pSequence->GetGroupName());
			pGroupRecord->SetExpanded(TRUE);

			pRootGroupRec->GetChilds()->Add( pGroupRecord );
			mapGroups[sGroupName] = pGroupRecord;
		}
	}
	

	//////////////////////////////////////////////////////////////////////////
	for (IAnimSequence *pSequence = pSequenceIt->first(); pSequence != 0; pSequence = pSequenceIt->next())
	{	
		int nNodeImage = 1;

		CRecord *pNodeRecord = new CRecord( false,pSequence->GetName() );
		pNodeRecord->sequenceName = pSequence->GetName();
		//if (pSequence->GetFlags()&ANODE_FLAG_EXPANDED)
		{
			pNodeRecord->SetExpanded(TRUE);
		}

		CString sGroupName = pSequence->GetGroupName();
		CRecord *pGroupRecord = stl::find_in_map(mapGroups,sGroupName,0);
		if (pGroupRecord)
		{
			pGroupRecord->GetChilds()->Add( pNodeRecord );
		}
		else
		{
			//AddRecord( pNodeRecord );
			pRootGroupRec->GetChilds()->Add( pNodeRecord );
		}
	}

	pSequenceIt->Release();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewSequencesCtrl::OnSelectionChanged()
{
	if (!m_pTrackViewDialog)
		return;

	int nCount = GetSelectedRows()->GetCount();
	if (nCount == 1)
	{
		CRecord *pRecord = (CRecord*)GetSelectedRows()->GetAt(0)->GetRecord();
		if (pRecord)
		{
			IAnimSequence *pSequence = gEnv->pMovieSystem->FindSequence( pRecord->sequenceName );
			if (pSequence)
				m_pTrackViewDialog->SetCurrentSequence( pSequence );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewSequencesCtrl::OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	CPoint point;

	if (!m_pTrackViewDialog)
		return;

	CRecord *pItemInfo = 0;

	// Find node under mouse.
	GetCursorPos( &point );
	ScreenToClient( &point );
	// Select the item that is at the point myPoint.

	CXTPReportRow* pRow = HitTest(point);
	if (pRow)
	{
		pItemInfo = (CRecord*)pRow->GetRecord();
	}

	// Create pop up menu.
	CMenu menu;
	CMenu menuAddTrack;
	
	CMenu menuExpand;
	CMenu menuCollapse;

	menu.CreatePopupMenu();

	Records records;
	IAnimSequence *pSelected = 0;
	if (GetSelectedRecords( records ))
	{
		CRecord *pRec = (CRecord*)records[0];
		pSelected = gEnv->pMovieSystem->FindSequence(	pRec->sequenceName );
	}

	menu.AppendMenu( MF_STRING,10,"New Sequence" );
	menu.AppendMenu( MF_SEPARATOR );
	if (pSelected)
	{
		menu.AppendMenu( MF_STRING,11,"Change Group" );
		menu.AppendMenu( MF_STRING,12,"Delete Sequence" );
		menu.AppendMenu( MF_SEPARATOR );
		menu.AppendMenu( MF_STRING,20,"Properties" );
	}

	GetCursorPos( &point );
	//int cmd = menu.TrackPopupMenu( TPM_RETURNCMD|TPM_LEFTALIGN|TPM_LEFTBUTTON,point.x,point.y,this->GetParent()->GetParent() );
	// track menu
	int cmd = CXTPCommandBars::TrackPopupMenu(&menu, TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_RIGHTBUTTON, 
																						point.x,point.y, this, NULL);

	if (cmd == 10)
	{
		// Create new sequence.
		m_pTrackViewDialog->OnAddSequence();
	}
	if (cmd == 12)
	{
		if (AfxMessageBox( "Delete sequence?",MB_YESNO|MB_ICONQUESTION ) == IDYES)
		{
			if (pSelected)
				m_pTrackViewDialog->DeleteSequence( pSelected->GetName() );
		}
	}
	if (cmd == 11) // Group
	{
		CStringDlg dlg( _T("Enter Group Name") );
		dlg.SetString( pSelected->GetGroupName());
		if (dlg.DoModal() == IDOK)
		{
			pSelected->SetGroupName( dlg.GetString() );
			Reload();
		}
	}
	if (cmd == 20)
	{
		m_pTrackViewDialog->OnEditSequence();
	}


	// processed
	*pResult = 1;
}
