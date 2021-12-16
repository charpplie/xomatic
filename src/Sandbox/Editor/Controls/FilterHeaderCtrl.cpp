//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : FilterHeaderCtrl.cpp
//  Author           : Jaewon Jung
//  Time of creation : 4/12/2011   15:11
//  Compilers        : VS2008
//  Description      : Header control with filtering support per column
//  Notice           : http://www.codeproject.com/KB/list/filterheaderctrl.aspx
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
// FilterHeaderCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "FilterListCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CFilterHeaderCtrl

CFilterHeaderCtrl::CFilterHeaderCtrl()
{
	m_pEdit=NULL;
	m_nEditColumn=-1;
	m_bEndEditSent=TRUE;	
	m_nSelectedColumn=-1;
	
	m_spacing = 6;
	m_cr3DHighLight = ::GetSysColor(COLOR_3DHIGHLIGHT);
	m_cr3DShadow = ::GetSysColor(COLOR_3DSHADOW);
	m_cr3DFace = ::GetSysColor(COLOR_3DFACE);
	m_crText = ::GetSysColor(COLOR_BTNTEXT);	
}

CFilterHeaderCtrl::~CFilterHeaderCtrl()
{
	for (int i=0;i<m_arFilters.GetSize();i++)
		delete (CFilterInfo*)m_arFilters[i];
}


BEGIN_MESSAGE_MAP(CFilterHeaderCtrl, CHeaderCtrl)
	//{{AFX_MSG_MAP(CFilterHeaderCtrl)
	ON_WM_PAINT()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_MESSAGE(FLM_EDITTEXTCHANGED,OnEditTextChanged)
	ON_MESSAGE(FLM_FILTERTEXTCHANGED,OnFilterTextChanged)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilterHeaderCtrl message handlers

void CFilterHeaderCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	HDITEM hdi;
	char szBuffer[1024];

	hdi.mask = HDI_TEXT | HDI_FORMAT | HDI_IMAGE;
	hdi.pszText = szBuffer;
	hdi.cchTextMax = 1024;

	GetItem(lpDrawItemStruct->itemID, &hdi);

	// now we have all the infos to draw our item

	CRect rcHeader=lpDrawItemStruct->rcItem; // the header
	CRect rcFilter=lpDrawItemStruct->rcItem; // the filter area

	if (GetFilterStatus(lpDrawItemStruct->itemID)!=HEADER_FILTER_NONE)
		rcHeader.bottom=rcHeader.top+2+m_nFont1Height;

	// if the item is selected, we have to shift the rect
	if( lpDrawItemStruct->itemState == ODS_SELECTED )
		rcHeader.bottom+=2;

	rcFilter.top=rcHeader.bottom;

	// need a dc!
	CDC dc;
	dc.Attach(lpDrawItemStruct->hDC);

	int nOldDC=dc.SaveDC();

	// clipping region 
	CRgn rgHeader;
	rgHeader.CreateRectRgnIndirect(&rcHeader);
	dc.SelectObject(&rgHeader);
	rgHeader.DeleteObject();

	// Draw the background
	CBrush brGray(::GetSysColor(COLOR_3DFACE));
	dc.FillRect(rcHeader, &brGray);
	
	// this is a generic 'offset' used to put space between labels and borders
	int nSpaceOffset=dc.GetTextExtent(" ").cx*2;

	int nTextWidth= dc.GetTextExtent(szBuffer,strlen(szBuffer)).cx;

	// some useful booleans
	BOOL bImage=((hdi.fmt & HDF_IMAGE) && GetImageList() && (hdi.iImage!=-1));
	BOOL bImageOnRight=(hdi.fmt & HDF_BITMAP_ON_RIGHT);

	/////////////////////////////////
	// draw the header area
	/////////////////////////////////

	int cxImage=0,cyImage=0;
	int xImage=0,yImage=0;

	rcHeader.DeflateRect(nSpaceOffset,0,nSpaceOffset,0);

	CRect rcText=rcHeader;

	if (bImage) 
	{
		ImageList_GetIconSize(GetImageList()->m_hImageList,&cxImage,&cyImage);

		// image is always centered vertically
		yImage=rcHeader.top+(rcHeader.Height()-cyImage)/2;

		// here all the cases (left, right, center, imageonright, on left, ecc...)
		if (bImageOnRight)
		{
			rcText.right-=(cxImage+nSpaceOffset*2);

			if( hdi.fmt & HDF_CENTER)
			{
				if (rcText.Width()>nTextWidth)
				{
					int nGap=(rcText.Width()-nTextWidth)/2;
					rcText.left+=nGap;
					rcText.right-=nGap;
				}

				xImage=rcText.right+nSpaceOffset*2;
			}
			else if( hdi.fmt & HDF_RIGHT)
			{
				xImage=rcText.right+nSpaceOffset*2;
			}
			else
			{
				if (rcText.Width()>nTextWidth)
				{
					int nGap=(rcText.Width()-nTextWidth);
					rcText.right-=nGap;
				}

				xImage=rcText.right+nSpaceOffset*2;
			}
		}
		else
		{
			rcText.left+=(cxImage+nSpaceOffset*2);

			if( hdi.fmt & HDF_CENTER)
			{
				if (rcText.Width()>nTextWidth)
				{
					int nGap=(rcText.Width()-nTextWidth)/2;
					rcText.left+=nGap;
					rcText.right-=nGap;
				}

				xImage=rcText.left-nSpaceOffset*2-cxImage;
			}
			else if( hdi.fmt & HDF_RIGHT)
			{
				if (rcText.Width()>nTextWidth)
				{
					int nGap=(rcText.Width()-nTextWidth);
					rcText.left+=nGap;
				}

				xImage=rcText.left-nSpaceOffset*2-cxImage;
			}
			else
			{
				xImage=rcText.left-nSpaceOffset*2-cxImage;
			}
		}
	}

	UINT uTextFormat=DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER | DT_END_ELLIPSIS ;

	if (nTextWidth>rcText.Width())
		uTextFormat|=DT_LEFT;
	else
		if (hdi.fmt & HDF_CENTER)
			uTextFormat|=DT_CENTER;
		else if (hdi.fmt & HDF_RIGHT)
			uTextFormat|=DT_RIGHT;
		else
			uTextFormat|=DT_LEFT;

	// check if the item is selected
	if (lpDrawItemStruct->itemState==ODS_SELECTED)
	{
		rcText.left++;
		rcText.top +=2;
		rcText.right++;

		xImage++;
		yImage+=2;
	}

	// draw label if the rect is valid
	if (rcText.left<rcText.right)
		dc.DrawText(szBuffer,-1,rcText,uTextFormat);

	// Draw the icon (if there is an icon to draw!)
	if (bImage)
		GetImageList()->Draw(&dc,hdi.iImage,CPoint(xImage,yImage),ILD_TRANSPARENT);

	if (GetFilterStatus(lpDrawItemStruct->itemID)==HEADER_FILTER_NONE)
	{
		dc.RestoreDC(nOldDC);
		dc.Detach();
		return;
	}

	/////////////////////////////////
	// draw the filter area
	/////////////////////////////////

	CFont* pOldFont=NULL;

	if (m_FilterFont.m_hObject)
		pOldFont=dc.SelectObject(&m_FilterFont);

	// clipping region 
	CRgn rgFilter;
	rgFilter.CreateRectRgnIndirect(&rcFilter);
	dc.SelectObject(&rgFilter);
	rgFilter.DeleteObject();

	if (GetFilterStatus(lpDrawItemStruct->itemID)==HEADER_FILTER_ENABLED)
		dc.FillSolidRect(rcFilter,RGB(245,250,255));
	else
		dc.FillSolidRect(rcFilter,RGB(192,192,192));

	dc.Draw3dRect(rcFilter,GetSysColor(COLOR_3DSHADOW),GetSysColor(COLOR_3DHILIGHT));

	rcFilter.DeflateRect(2,1);

	char szFilter[1024];
	sprintf(szFilter,GetFilterText(lpDrawItemStruct->itemID));

	dc.SetTextColor(RGB(0,0,0));

	// Adjust the rect if the mouse button is pressed on it
	if( lpDrawItemStruct->itemState == ODS_SELECTED )
	{
		rcFilter.left++;
		rcFilter.top +=2;
		rcFilter.right++;
	}

	if (GetFilterStatus(lpDrawItemStruct->itemID)==HEADER_FILTER_DISABLED)
	{
		if (m_strFilterDisabled.GetLength())
			sprintf(szFilter,m_strFilterDisabled);

		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(::GetSysColor(COLOR_3DHILIGHT));

		CRect rc1=rcFilter;

		rc1.OffsetRect(1,1);

		::DrawText(lpDrawItemStruct->hDC, szFilter, strlen(szFilter), 
		  &rc1, DT_SINGLELINE|DT_VCENTER|DT_LEFT| DT_END_ELLIPSIS);

		dc.SetTextColor(::GetSysColor(COLOR_3DSHADOW));
	}

	::DrawText(lpDrawItemStruct->hDC, szFilter, strlen(szFilter), 
	  &rcFilter, DT_SINGLELINE|DT_VCENTER|DT_LEFT| DT_END_ELLIPSIS);

	if (pOldFont)
		dc.SelectObject(pOldFont);

	dc.RestoreDC(nOldDC);
	dc.Detach();
}

LRESULT CFilterHeaderCtrl::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if (message==HDM_INSERTITEMA || 
		message==HDM_INSERTITEMW)
	{
		CFilterInfo* pNewFilter=new CFilterInfo();
		m_arFilters.InsertAt((int)wParam,pNewFilter);
	}

	if (message==HDM_DELETEITEM)
	{
		delete (CFilterInfo*)m_arFilters[(int)wParam];
		m_arFilters.RemoveAt((int)wParam);
	}

	if (message==HDM_LAYOUT)
	{
		HD_LAYOUT *layout	=(HD_LAYOUT*)lParam;
		RECT *rect			=layout->prc;
		WINDOWPOS *pos		=layout->pwpos;
		LRESULT lRes		=CWnd::DefWindowProc(message,wParam,lParam);

		int nHeight=0;

		CalcFontHeight();

		nHeight=m_nFont1Height+m_nFont2Height+4;

		pos->cy				=nHeight;
		rect->top			=nHeight;

		return lRes;
	}
	
	return CHeaderCtrl::DefWindowProc(message, wParam, lParam);
}

void CFilterHeaderCtrl::OnSize(UINT nType, int cx, int cy) 
{
	CHeaderCtrl::OnSize(nType, cx, cy);
	
	CalcFontHeight();
}

void CFilterHeaderCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if (m_pEdit)
		HideEdit(FALSE);

	int nColumn=-1;

	// HitTest
	for (int i=0;i<GetItemCount();++i)
	{
		if (GetFilterStatus(i)!=HEADER_FILTER_NONE && GetFilterStatus(i) != HEADER_FILTER_DISABLED)
		{
			CRect rc;
			
			GetItemRect(i,rc);

			rc.top+=m_nFont1Height+2;

			rc.DeflateRect(3,2);

			if (rc.PtInRect(point))
				nColumn=i;
		}
	}

	if (nColumn!=-1)
		ShowEdit(nColumn);
	else
	{
		for (int i=0;i<GetItemCount();++i)
		{
			CRect rc;
			GetItemRect(i,rc);
			if (rc.PtInRect(point))
				m_nSelectedColumn=i;
		}
		CHeaderCtrl::OnLButtonDown(nFlags, point);
	}
}

void CFilterHeaderCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	m_nSelectedColumn=-1;
	__super::OnLButtonUp(nFlags, point);
}

void CFilterHeaderCtrl::ShowEdit(int nColumn)
{
	TRACE("Show\r\n");
	if (m_pEdit==NULL)
	{
		CRect rc,rcHeader;

		GetItemRect(nColumn,rc);
		GetWindowRect(rcHeader);

		rc.top+=m_nFont1Height+2;
		rc.bottom++;

		rc.right--;

		LRESULT lRes=0;

		NMHDR nmh;
		nmh.code=FLCN_SHOWINGEDIT;
		nmh.hwndFrom=m_hWnd;
		nmh.idFrom=nColumn;

		if (GetParent())
			lRes=GetParent()->SendMessage(WM_NOTIFY,(WPARAM)m_hWnd,(LPARAM)&nmh);

		if (lRes==0)
		{
			if (m_bEndEditSent)
			{
				NMHDR nmh;
				nmh.code=FLCN_BEGINFILTEREDIT;
				nmh.hwndFrom=m_hWnd;
				nmh.idFrom=nColumn;

				if (GetParent())
					GetParent()->SendMessage(WM_NOTIFY,(WPARAM)m_hWnd,(LPARAM)&nmh);

				m_bEndEditSent=FALSE;
			}

			m_nEditColumn=nColumn;

			m_pEdit=new CFilterEdit();

			m_pEdit->CreateEx(WS_EX_CLIENTEDGE | WS_EX_NOPARENTNOTIFY ,"EDIT",NULL,ES_WANTRETURN | WS_VISIBLE | WS_CHILD | WS_GROUP | ES_AUTOHSCROLL,rc,this,IDC_ED_EDIT,NULL);

			if (m_FilterFont.m_hObject)
				m_pEdit->SetFont(&m_FilterFont);
			else
				m_pEdit->SetFont(GetFont());

			m_pEdit->SetWindowText(GetFilter(m_nEditColumn)->m_strFilter);

			m_pEdit->SetSel(0,m_pEdit->GetWindowTextLength());

			m_pEdit->SetFocus();
		}
	}
}

void CFilterHeaderCtrl::HideEdit(BOOL bValidate)
{
	if (m_pEdit)
	{
		CString strVal;
		
		m_pEdit->GetWindowText(strVal);

		m_pEdit->DestroyWindow();

		m_pEdit=NULL;

		if (bValidate)
		{
			if (GetFilterText(m_nEditColumn)!=strVal)
			{
				NMFILTERHDR nmh;
				nmh.code=FLCN_FILTERCHANGING;
				nmh.hwndFrom=m_hWnd;
				nmh.idFrom=m_nEditColumn;
				nmh.szText=strVal.GetBuffer(strVal.GetLength()+1);

				LRESULT lRes=0;

				if (GetParent())
					lRes=GetParent()->SendMessage(WM_NOTIFY,(WPARAM)m_hWnd,(LPARAM)&nmh);

				if (lRes==0)
				{
					GetFilter(m_nEditColumn)->m_strFilter=strVal;

					Invalidate();

					UpdateWindow();

					NMFILTERHDR nmh;
					nmh.code=FLCN_FILTERCHANGED;
					nmh.hwndFrom=m_hWnd;
					nmh.idFrom=m_nEditColumn;
					nmh.szText=strVal.GetBuffer(strVal.GetLength()+1);

					if (GetParent())
						GetParent()->SendMessage(WM_NOTIFY,(WPARAM)m_hWnd,(LPARAM)&nmh);
				}
			}
		}
	}
}

LRESULT CFilterHeaderCtrl::OnEditTextChanged(WPARAM wParam, LPARAM lParam)
{
	TRACE("OnEditTextChanged\r\n");
	HideEdit(wParam);

	if (lParam)
	{
		int nCount=0;

		int nOrd=IndexToOrder(m_nEditColumn);

		do
		{
			nOrd++;
			nOrd=nOrd % GetItemCount();

			nCount++;
		}
		while (nCount!=GetItemCount() && GetFilterStatus(OrderToIndex(nOrd))!=HEADER_FILTER_ENABLED);

		m_nEditColumn=OrderToIndex(nOrd);

		ShowEdit(m_nEditColumn);
	}
	else
	{
		NMHDR nmh;
		nmh.code=FLCN_ENDFILTEREDIT;
		nmh.hwndFrom=m_hWnd;
		nmh.idFrom=0;

		if (GetParent())
			GetParent()->SendMessage(WM_NOTIFY,(WPARAM)m_hWnd,(LPARAM)&nmh);

		m_bEndEditSent=TRUE;

		if (GetParent())
			GetParent()->SetFocus();
	}

	return 0;
}

LRESULT CFilterHeaderCtrl::OnFilterTextChanged(WPARAM wParam, LPARAM lParam)
{
	TRACE("OnFilterTextChanged\r\n");

	CString strVal;
	m_pEdit->GetWindowText(strVal);
	GetFilter(m_nEditColumn)->m_strFilter=strVal;

	NMHDR nmh;
	nmh.code=FLCN_FILTERTEXTCHANGED;
	nmh.hwndFrom=m_hWnd;
	nmh.idFrom=0;

	if (GetParent())
			GetParent()->SendMessage(WM_NOTIFY,(WPARAM)m_hWnd,(LPARAM)&nmh);

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CFilterEdit

CFilterHeaderCtrl::CFilterEdit::CFilterEdit()
{
	m_bNotifySent=FALSE;
}

CFilterHeaderCtrl::CFilterEdit::~CFilterEdit()
{
}


BEGIN_MESSAGE_MAP(CFilterHeaderCtrl::CFilterEdit, CEdit)
	//{{AFX_MSG_MAP(CFilterHeaderCtrl::CFilterEdit)
	{ WM_KILLFOCUS, 0, 0, 0, AfxSig_vW, (AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(CWnd*))&CFilterHeaderCtrl::CFilterEdit::OnKillFocus },
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilterEdit message handlers

void CFilterHeaderCtrl::CFilterEdit::OnChange()
{
	CString filterText;
	GetWindowText(filterText);
	if(m_lastFilterText != filterText)
	{
		((CFilterHeaderCtrl*)GetParent())->SendMessage(FLM_FILTERTEXTCHANGED,0,0);
		m_lastFilterText = filterText;
	}
}

void CFilterHeaderCtrl::CFilterEdit::OnKillFocus(CWnd* pNewWnd) 
{
	CEdit::OnKillFocus(pNewWnd);

	if (!m_bNotifySent)
	{
		m_bNotifySent=TRUE;

		((CFilterHeaderCtrl*)GetParent())->SendMessage(FLM_EDITTEXTCHANGED,0,0);
	}
}

void CFilterHeaderCtrl::CFilterEdit::PostNcDestroy()
{
	delete this;
}

BOOL CFilterHeaderCtrl::CFilterEdit::PreTranslateMessage(MSG* pMsg) 
{
	BOOL bRetVal=FALSE;

	if (pMsg->message==WM_KEYDOWN)
	{
		if (pMsg->wParam==VK_RETURN) // CR - input is ok
		{
			m_bNotifySent=TRUE;

			((CFilterHeaderCtrl*)GetParent())->SendMessage(FLM_EDITTEXTCHANGED,TRUE,0);

			bRetVal=TRUE;
		}
		if (pMsg->wParam==VK_ESCAPE) // ESCAPE - ignore input
		{
			m_bNotifySent=TRUE;

			((CFilterHeaderCtrl*)GetParent())->SendMessage(FLM_EDITTEXTCHANGED,FALSE,0);

			bRetVal=TRUE;
		}
		if (pMsg->wParam==VK_TAB) // TAB - input is ok, go to the next filter
		{
			m_bNotifySent=TRUE;

			((CFilterHeaderCtrl*)GetParent())->SendMessage(FLM_EDITTEXTCHANGED,TRUE,TRUE);

			bRetVal=TRUE;
		}
	}

	if (!bRetVal)
		bRetVal=CEdit::PreTranslateMessage(pMsg);

	return bRetVal;
}

void CFilterHeaderCtrl::SetFilterStatus(int nIndex, UINT uStatus)
{
	GetFilter(nIndex)->m_nStatus=uStatus;

	if (IsWindow(m_hWnd))
	{
		Invalidate();
		UpdateWindow();
	}
}

void CFilterHeaderCtrl::SetFilterDisabledString(CString strText)
{
	m_strFilterDisabled=strText;
}

void CFilterHeaderCtrl::CalcFontHeight()
{
	if (IsWindow(m_hWnd))
	{
		CDC* pDC=GetDC();
		int nOldDC=pDC->SaveDC();

		m_nFont1Height=pDC->GetTextExtent(" ").cy;
		m_nFont2Height=m_nFont1Height;

		if (m_FilterFont.m_hObject)
		{
			CFont* pOldFont=pDC->SelectObject(&m_FilterFont);

			m_nFont2Height=pDC->GetTextExtent(" ").cy;

			pDC->SelectObject(pOldFont);
		}

		pDC->RestoreDC(nOldDC);
		ReleaseDC(pDC);
	}
}


void CFilterHeaderCtrl::SetFilterFont(CFont *pFont)
{
	if (m_FilterFont.m_hObject)
		m_FilterFont.DeleteObject();

	if (pFont)
	{
		LOGFONT lf;

		pFont->GetLogFont(&lf);

		m_FilterFont.CreateFontIndirect(&lf);
	}

	CalcFontHeight();
}

BOOL CFilterHeaderCtrl::FilterEditing()
{
	return (m_pEdit ? TRUE : FALSE);
}

int CFilterHeaderCtrl::IndexToOrder(int nIndex)
{
	int nOrd=-1;

	int	nCount=GetItemCount();
	LPINT pnOrder=(LPINT)malloc(nCount*sizeof(int));

	GetOrderArray(pnOrder,nCount);

	for (int i=0;i<nCount && nOrd==-1;i++)
	{
		if (pnOrder[i]==nIndex)
			nOrd=i;
	}

	free(pnOrder);

	return nOrd;
}

void CFilterHeaderCtrl::OnPaint()
{
	CPaintDC dc(this);

	DrawCtrl(&dc);
}

void CFilterHeaderCtrl::DrawCtrl(CDC* pDC)
{
	RECT rectClip;
	if (pDC->GetClipBox(&rectClip) == ERROR)
		return;

	CRect rectClient, rectItem;
	GetClientRect(&rectClient);

	pDC->FillSolidRect(&rectClip, m_cr3DFace);

	INT iItems = GetItemCount();
	assert(iItems >= 0);

	CPen penHighLight(PS_SOLID, 1, m_cr3DHighLight);
	CPen penShadow(PS_SOLID, 1, m_cr3DShadow);
	CPen* pPen = pDC->GetCurrentPen();

	CFont* pFont = pDC->SelectObject(GetFont());

	pDC->SetBkColor(m_cr3DFace);
	pDC->SetTextColor(m_crText);

	INT iWidth = 0;

	for(INT i=0;i<iItems;i++)
	{
		INT iItem = OrderToIndex(i);

		TCHAR szText[1024];

		HDITEM hditem;
		hditem.mask = HDI_WIDTH|HDI_FORMAT|HDI_TEXT|HDI_IMAGE|HDI_BITMAP;
		hditem.pszText = szText;
		hditem.cchTextMax = sizeof(szText);
		VERIFY(GetItem(iItem, &hditem));

		VERIFY(GetItemRect(iItem, rectItem));

		if (rectItem.right >= rectClip.left || rectItem.left <= rectClip.right)
		{
			bool bSelected = false;
			if (m_nSelectedColumn == i)
				bSelected = true;

			DRAWITEMSTRUCT disItem;
			disItem.CtlType = ODT_BUTTON;
			disItem.CtlID = GetDlgCtrlID();
			disItem.itemID = iItem;
			disItem.itemAction = ODA_DRAWENTIRE;
			disItem.itemState = bSelected?ODS_SELECTED:0;
			disItem.hwndItem = m_hWnd;
			disItem.hDC = pDC->m_hDC;
			disItem.rcItem = rectItem;
			disItem.itemData = 0;

			DrawItem(&disItem);

			if(i < iItems-1)
			{
				pDC->SelectObject(&penShadow);
				pDC->MoveTo(rectItem.right-1, rectItem.top+2);
				pDC->LineTo(rectItem.right-1, rectItem.bottom-2);

				pDC->SelectObject(&penHighLight);
				pDC->MoveTo(rectItem.right, rectItem.top+2);
				pDC->LineTo(rectItem.right, rectItem.bottom-2);
			}
		}

		iWidth += hditem.cxy;
	}

	if(iWidth > 0)
	{
		rectClient.right = rectClient.left + iWidth;
		pDC->Draw3dRect(rectClient, m_cr3DHighLight, m_cr3DShadow);
	}

	pDC->SelectObject(pFont);
	pDC->SelectObject(pPen);

	penHighLight.DeleteObject();
	penShadow.DeleteObject();
}

void CFilterHeaderCtrl::OnSysColorChange() 
{
	CHeaderCtrl::OnSysColorChange();
	
	m_cr3DHighLight = ::GetSysColor(COLOR_3DHIGHLIGHT);
	m_cr3DShadow = ::GetSysColor(COLOR_3DSHADOW);
	m_cr3DFace = ::GetSysColor(COLOR_3DFACE);
	m_crText = ::GetSysColor(COLOR_BTNTEXT);
}