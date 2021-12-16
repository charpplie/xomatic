// TrackViewFindDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TrackViewFindDlg.h"
#include "TrackViewDialog.h"

#include <IMovieSystem.h>
#include <StringUtils.h>

/////////////////////////////////////////////////////////////////////////////
// CTrackViewFindDlg dialog


CTrackViewFindDlg::CTrackViewFindDlg( const char *title,CWnd* pParent /*=NULL*/)
	: CToolbarDialog(CTrackViewFindDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTrackViewFindDlg)
	//}}AFX_DATA_INIT

	m_tvDlg = 0;
	m_numSeqs = 0;
}


BEGIN_MESSAGE_MAP(CTrackViewFindDlg, CToolbarDialog)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_EN_CHANGE( IDC_FILTER, OnFilterChange )
	ON_LBN_DBLCLK(IDC_LIST, OnLbnDblClk)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrackViewFindDlg message handlers

void CTrackViewFindDlg::DoDataExchange(CDataExchange* pDX)
{
	CToolbarDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILTER, m_filter);
	DDX_Control(pDX, IDC_LIST, m_list);
}


void CTrackViewFindDlg::FillData()
{
	m_numSeqs = 0;
	m_objs.resize(0);
	ISequenceIt *It= GetIEditor()->GetMovieSystem()->GetSequences();
	IAnimSequence *seq=It->first();
	while (seq)
	{
		for(int i=0; i<seq->GetNodeCount(); i++)
		{
			IAnimNode * pNode = seq->GetNode(i);
			ObjName obj;
			obj.m_objName = pNode->GetName();
			obj.m_seqName = seq->GetName();
			m_objs.push_back(obj);
		}
		seq=It->next();
		m_numSeqs++;
	}
	FillList();
}


void CTrackViewFindDlg::Init(CTrackViewDialog * tvDlg)
{
	m_tvDlg = tvDlg;
}


void CTrackViewFindDlg::FillList()
{
	CString filter;
	m_filter.GetWindowText(filter);

	m_list.ResetContent();
	int len = strlen(filter);
	for(int i=0; i<m_objs.size(); i++)
	{
		ObjName pObj = m_objs[i];
		if(!len || CryStringUtils::stristr(pObj.m_objName, filter))
		{
			int ind;
			if(m_numSeqs==1)
				ind = m_list.AddString( pObj.m_objName);
			else
				ind = m_list.AddString( pObj.m_objName + " / " +  pObj.m_seqName);
			m_list.SetItemData(ind, i);
		}
	}
	m_list.SetCurSel(0);
}


BOOL CTrackViewFindDlg::OnInitDialog() 
{
	CToolbarDialog::OnInitDialog();

	if (XTPPaintManager() == NULL)
	{		
		CXTPPaintManager::SetTheme(xtpThemeOffice2000);
	}

	//FillData();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTrackViewFindDlg::OnDestroy()
{
	CToolbarDialog::OnDestroy();
}

void CTrackViewFindDlg::OnSize(UINT nType, int cx, int cy)
{
	CToolbarDialog::OnSize(nType, cx, cy);
}

void CTrackViewFindDlg::OnFilterChange()
{
	FillList();
}


void CTrackViewFindDlg::ProcessSel()
{
	int sel = m_list.GetCurSel();
	if(sel>=0 && m_tvDlg)
	{
		int i = m_list.GetItemData(sel);
		ObjName pObj = m_objs[i];
		IAnimSequence * seq = GetIEditor()->GetMovieSystem()->FindSequence(pObj.m_seqName);
		if(seq)
		{
			if(seq != m_tvDlg->GetCurrentSequence())
				m_tvDlg->SetCurrentSequence(seq);
			m_tvDlg->SelectNode(pObj.m_objName);
		}
	}
}


void CTrackViewFindDlg::OnLbnDblClk()
{
	ProcessSel();
}


void CTrackViewFindDlg::OnOK()
{
	ProcessSel();
	ShowWindow(SW_HIDE);
	//CToolbarDialog::OnOK();
}


void CTrackViewFindDlg::OnCancel()
{
	ShowWindow(SW_HIDE);
}


void CTrackViewFindDlg::OnClose()
{
	ShowWindow(SW_HIDE);
}