//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : FilterListCtrl.cpp
//  Author           : Jaewon Jung
//  Time of creation : 4/12/2011   15:11
//  Compilers        : VS2008
//  Description      : List control with filtering support per column
//  Notice           : http://www.codeproject.com/KB/list/filterheaderctrl.aspx
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
// FilterListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "FilterListCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CFilterListCtrl

CFilterListCtrl::CFilterListCtrl()
{
}

CFilterListCtrl::~CFilterListCtrl()
{
	if(m_ctlHeader.m_hWnd)
		m_ctlHeader.UnsubclassWindow();
}


BEGIN_MESSAGE_MAP(CFilterListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CFilterListCtrl)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilterListCtrl message handlers

void CFilterListCtrl::PreSubclassWindow() 
{
	m_ctlHeader.SubclassWindow(GetHeaderCtrl()->m_hWnd);

	CListCtrl::PreSubclassWindow();
}

int CFilterListCtrl::InsertColumn(int nCol, const LVCOLUMN* pColumn, int nFilter)
{
	int nRetVal=CListCtrl::InsertColumn(nCol,pColumn);

	if (nRetVal!=-1)
	{
		if (nFilter!=HEADER_FILTER_NONE)
		{
			HDITEM hdr;
			memset(&hdr,0,sizeof(HDITEM));

			hdr.mask=HDI_FORMAT;
			m_ctlHeader.GetItem(nRetVal,&hdr);
			hdr.fmt|= HDF_OWNERDRAW;

			m_ctlHeader.SetItem(nRetVal,&hdr);

			m_ctlHeader.SetFilterStatus(nRetVal,nFilter);
		}
		else
			m_ctlHeader.SetFilterStatus(nRetVal,HEADER_FILTER_NONE);
	}

	return nRetVal;
}

int CFilterListCtrl::InsertColumn(int nCol, LPCTSTR lpszColumnHeading,
	int nFormat , int nWidth , int nSubItem , int nFilter)
{
	int nRetVal=CListCtrl::InsertColumn(nCol,lpszColumnHeading,nFormat,nWidth,nSubItem);

	if (nRetVal!=-1)
	{
		if (nFilter!=HEADER_FILTER_NONE)
		{
			HDITEM hdr;
			memset(&hdr,0,sizeof(HDITEM));

			hdr.mask=HDI_FORMAT;
			m_ctlHeader.GetItem(nRetVal,&hdr);
			hdr.fmt|= HDF_OWNERDRAW;

			m_ctlHeader.SetItem(nRetVal,&hdr);

			m_ctlHeader.SetFilterStatus(nRetVal,nFilter);
		}
		else
			m_ctlHeader.SetFilterStatus(nRetVal,HEADER_FILTER_NONE);
	}

	return nRetVal;
}

BOOL CFilterListCtrl::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	BOOL bDefault=TRUE;

	if (GetHeaderCtrl())
	{
		if ((DWORD)wParam==(DWORD)GetHeaderCtrl()->m_hWnd)
		{
			NMHDR* pNM=(NMHDR*)lParam;

			if (pNM->code==FLCN_FILTERCHANGING ||
				pNM->code==FLCN_FILTERCHANGED ||
				pNM->code==FLCN_BEGINFILTEREDIT ||
				pNM->code==FLCN_ENDFILTEREDIT ||
				pNM->code==FLCN_FILTERTEXTCHANGED)
			{
				NMFILTERHDR* pNMF=(NMFILTERHDR*)lParam;
				NMFILTERHDR nmh;

				nmh.code=pNMF->code;
				nmh.hwndFrom=m_hWnd;
				nmh.idFrom=pNMF->idFrom;
				nmh.szText=pNMF->szText;

				if (GetParent())
				{
					*pResult=GetParent()->SendMessage(WM_NOTIFY,(WPARAM)m_hWnd,(LPARAM)&nmh);
					bDefault=FALSE;
				}
			}

			if (pNM->code==FLCN_SHOWINGEDIT)
			{
				CRect rc,rcWin;
				GetHeaderCtrl()->GetItemRect(pNM->idFrom,&rc);
				GetClientRect(rcWin);

				// scroll the window

				int nScrollBarGap=GetSystemMetrics(SM_CXVSCROLL);

				int nHScroll=GetScrollPos(SB_HORZ);

				if (rc.left<=nHScroll)
					Scroll(CSize(rc.left-nHScroll,0));
				else
				{
					if (rc.right>nHScroll+rcWin.Width()-nScrollBarGap)
					{
						if (rc.Width()>rcWin.Width()-nScrollBarGap)
							Scroll(CSize(rc.left-nHScroll,0));
						else
							Scroll(CSize(rc.right-(nHScroll+rcWin.Width())+nScrollBarGap*0,0));
					}
				}

				*pResult=0; // 0 continue editing, 1 stop editing

				bDefault=FALSE;
			}
		}
	}
	
	return (bDefault ? CListCtrl::OnNotify(wParam, lParam, pResult) : TRUE);
}