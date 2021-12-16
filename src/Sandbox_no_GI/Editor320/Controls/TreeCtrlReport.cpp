////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   TreeCtrlReport.cpp
//  Version:     v1.00
//  Created:     02/04/2009 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TreeCtrlReport.h"

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP( CTreeCtrlReport, CXTPReportControl )
	ON_WM_LBUTTONDOWN( )
	ON_WM_LBUTTONUP( )
	ON_WM_MOUSEMOVE( )
	ON_WM_CAPTURECHANGED( )
	ON_WM_DESTROY ( )
	ON_NOTIFY_REFLECT(XTP_NM_REPORT_HEADER_RCLICK, OnReportColumnRClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnReportItemDblClick)
	ON_NOTIFY_REFLECT(XTP_NM_REPORT_ROWEXPANDED, OnReportRowExpandChanged)
	ON_NOTIFY_REFLECT(XTP_NM_REPORT_SELCHANGED, OnReportSelChanged)
	ON_WM_RBUTTONDOWN()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()
//////////////////////////////////////////////////////////////////////////

class CTreeCtrlReportPaintManager : public CXTPReportPaintManager
{
public:
	CTreeCtrlReportPaintManager( CTreeCtrlReport *pCtrl ) : m_bHeaderVisible(true),m_pCtrl(pCtrl) {}

	virtual void DrawColumn(
		CDC* pDC, 
		CXTPReportColumn* pColumn, 
		CXTPReportHeader* pHeader, 
		CRect rcColumn, 
		BOOL bDrawExternal
		)
	{
		if (m_bHeaderVisible)
			__super::DrawColumn(pDC,pColumn,pHeader,rcColumn,bDrawExternal);
	}
	virtual void DrawItemCaption(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pMetrics)
	{
		if (pDrawArgs->pRow)
		{
			CTreeCtrlReport::CTreeItemRecord *pRecord = (CTreeCtrlReport::CTreeItemRecord*)pDrawArgs->pRow->GetRecord();
			if (pRecord)
			{
				pRecord->SetRect( pDrawArgs->pRow->GetRect() );

				//if (pRecord->IsDropTarget())
					//pMetrics->clrForeground = RGB(255,0,0);
			}
		}


		//XTPPaintManager()->GradientFill( pDrawArgs->pDC,pDrawArgs->rcItem,RGB(200, 200, 250),RGB(220, 220, 250),FALSE );
		//pDrawArgs->pDC->FillSolidRect(pDrawArgs->rcItem, pMetrics->clrBackground);

		__super::DrawItemCaption( pDrawArgs,pMetrics );
	}

	virtual int GetRowHeight(CDC* pDC, CXTPReportRow* pRow)
	{
		CTreeCtrlReport::CTreeItemRecord *pRecord = (CTreeCtrlReport::CTreeItemRecord*)pRow->GetRecord();
		if (pRecord)
		{
			int nHeight = pRecord->GetItemHeight();
			if (nHeight > 0)
				return nHeight;
		}
		return __super::GetRowHeight(pDC,pRow) - 2;
	}

	virtual int GetHeaderHeight()
	{
		if (m_bHeaderVisible) 
			return __super::GetHeaderHeight();
		else
			return 0;
	}

	bool m_bHeaderVisible;
	CTreeCtrlReport *m_pCtrl;
};

//////////////////////////////////////////////////////////////////////////
CTreeCtrlReport::CTreeCtrlReport()
{
	//CXTPReportColumn *pNodeCol   = AddColumn(new CXTPReportColumn(0, _T("NodeClass"), 150, TRUE, XTP_REPORT_NOICON, TRUE, TRUE));
	//CXTPReportColumn *pCatCol    = AddColumn(new CXTPReportColumn(1, _T("Category"), 50, TRUE, XTP_REPORT_NOICON, TRUE, TRUE));
	//CXTPReportColumn *pNodeCol    = AddColumn(new CXTPReportColumn(1, _T("Node"), 50, TRUE, XTP_REPORT_NOICON, TRUE, TRUE));
	//GetColumns()->GetGroupsOrder()->Add(pGraphCol);
	//pNodeCol->SetTreeColumn(true);
	//pNodeCol->SetSortable(FALSE);
	//pCatCol->SetSortable(FALSE);
	//GetColumns()->SetSortColumn(pNodeCol, true);

	GetReportHeader()->AllowColumnRemove(FALSE);
	ShadeGroupHeadings(FALSE);
	SkipGroupsFocus(TRUE);
	SetMultipleSelection(FALSE);
	AllowEdit(FALSE);
	EditOnClick(FALSE);

	m_bDragging = false;
	m_bDragEx = false;
	m_ptDrag.SetPoint(0,0);
	m_pDragImage = 0;
	m_pDragRow = 0;
	m_pDropTargetRow = 0;

	CXTPReportPaintManager* pPMgr = new CTreeCtrlReportPaintManager(this);
	pPMgr->m_nTreeIndent = 0x0f;
	pPMgr->m_bShadeSortColumn = false;
	pPMgr->m_treeStructureStyle = xtpReportTreeStructureDots;
	//pPMgr->m_strNoItems = _T("Empty");
	pPMgr->SetGridStyle(FALSE, xtpGridNoLines);
	pPMgr->SetGridStyle(TRUE, xtpGridNoLines);
	SetPaintManager( pPMgr );

	//CXTRegistryManager regMgr;
	//int showHeader = regMgr.GetProfileInt(_T("FlowGraph"), _T("ComponentShowHeader"), 1);
	//SetHeaderVisible(showHeader != 0);
	//SetHeaderVisible(true);
}

//////////////////////////////////////////////////////////////////////////
bool CTreeCtrlReport::IsHeaderVisible()
{
	return m_bHeaderVisible;
}

//////////////////////////////////////////////////////////////////////////
void CTreeCtrlReport::SetHeaderVisible(bool bVisible)
{
	CTreeCtrlReportPaintManager* pPMgr = static_cast<CTreeCtrlReportPaintManager*> (GetPaintManager());
	pPMgr->m_bHeaderVisible = bVisible;
	m_bHeaderVisible = bVisible;
}


//////////////////////////////////////////////////////////////////////////
CTreeCtrlReport::~CTreeCtrlReport()
{
}

//////////////////////////////////////////////////////////////////////////
void CTreeCtrlReport::OnDestroy()
{
	//CXTRegistryManager regMgr;
	//regMgr.WriteProfileInt(_T("FlowGraph"), _T("ComponentShowHeader"), IsHeaderVisible() ? 1 : 0);
	__super::OnDestroy();
}

#define ID_SORT_ASC  1
#define ID_SORT_DESC  2
#define ID_SHOWHIDE_HEADER 3

//////////////////////////////////////////////////////////////////////////
void CTreeCtrlReport::OnReportColumnRClick( NMHDR* pNotifyStruct, LRESULT* result )
{
	XTP_NM_REPORTRECORDITEM* pItemNotify = (XTP_NM_REPORTRECORDITEM*) pNotifyStruct;
	ASSERT(pItemNotify->pColumn);
	CPoint ptClick = pItemNotify->pt;
	CXTPReportColumn* pColumn = pItemNotify->pColumn;

	CMenu menu;
	VERIFY(menu.CreatePopupMenu());

	CXTPReportColumns* pColumns = GetColumns();
	CXTPReportColumn* pNodeClassColumn = pColumns->GetAt(0);
	CXTPReportColumn* pCatColumn = pColumns->GetAt(1);

	// create main menu items
	menu.AppendMenu(MF_STRING, ID_SORT_ASC, _T("Sort ascending"));
	menu.AppendMenu(MF_STRING, ID_SORT_DESC, _T("Sort descending"));
	menu.AppendMenu(MF_STRING, ID_SHOWHIDE_HEADER, IsHeaderVisible() ?  _T("Hide Header") : _T("Show Header"));

	// track menu
	int nMenuResult = CXTPCommandBars::TrackPopupMenu(&menu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTALIGN |TPM_RIGHTBUTTON, ptClick.x, ptClick.y, this, NULL);

	// other general items
	switch (nMenuResult)
	{
	case ID_SORT_ASC:
	case ID_SORT_DESC:
		// only sort by node class
		if (pNodeClassColumn && pNodeClassColumn->IsSortable())
		{
			pColumns->SetSortColumn(pNodeClassColumn, nMenuResult == ID_SORT_ASC);
			Populate();
		}
		break;
	case ID_SHOWHIDE_HEADER:
		SetHeaderVisible(!IsHeaderVisible());
		Populate();
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CTreeCtrlReport::OnLButtonDown( UINT nFlags, CPoint point )
{
	if (!m_bDragging) {
		__super::OnLButtonDown(nFlags,point);

		CRect reportArea = GetReportRectangle();
		if (reportArea.PtInRect(point))
		{
			CXTPReportRow* pRow = HitTest( point );
			if( pRow) 
			{
				if (pRow->GetParentRow() != 0)
				{
					m_ptDrag = point;
					m_bDragEx   = true;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTreeCtrlReport::OnLButtonUp( UINT nFlags, CPoint point )
{
	m_bDragEx = false;

	m_pDropTargetRow = 0;

	if( !m_bDragging )
	{
		__super::OnLButtonUp(nFlags,point);
	}
	else
	{
		m_bDragging = false;
		ReleaseCapture();
	}

	if (m_pDragImage)
	{
		CPoint p;
		GetCursorPos( &p );

		//ReleaseCapture();
		m_pDragImage->DragLeave( this );
		m_pDragImage->EndDrag();
		m_pDragImage->DeleteImageList();
		m_pDragImage = 0;

		OnDragAndDrop( m_pDragRow,p );
	}
	m_pDragRow = 0;
}

//////////////////////////////////////////////////////////////////////////
void CTreeCtrlReport::OnMouseMove( UINT nFlags, CPoint point )
{
	if (!m_bDragging)
	{
		__super::OnMouseMove(nFlags,point);

		const int dragX = ::GetSystemMetrics(SM_CXDRAG) + 5;
		const int dragY = ::GetSystemMetrics(SM_CYDRAG) + 5;
		if ( (m_ptDrag.x || m_ptDrag.y) && (abs(point.x - m_ptDrag.x) > dragX || abs(point.y - m_ptDrag.y) > dragY) )
		{
			CXTPReportRow* pRow = HitTest( m_ptDrag );
			if (pRow)
			{
				m_bDragging = true;
				CPoint wpoint = m_ptDrag;
				ClientToScreen(&wpoint);
				if (OnBeginDragAndDrop(pRow, wpoint))
				{
					m_bDragging = true;
				}
			}
		}
	}
	else
	{
		CXTPReportRow* pRow = HitTest( point );
		if (pRow!=NULL && pRow->GetRecord())
		{
			CTreeItemRecord* pItem = static_cast<CTreeItemRecord*>( pRow->GetRecord() );
			if (pItem)
			{
				if (m_pDropTargetRow)
					m_pDropTargetRow->SetDropTarget(false);
				m_pDropTargetRow = pItem;
				if (m_pDropTargetRow)
					m_pDropTargetRow->SetDropTarget(true);
			}
		}
	}
	if (m_pDragImage)
	{
		CRect rc;
		GetWindowRect( rc );
		CPoint p = point;
		ClientToScreen( &p );
		p.x -= rc.left;
		p.y -= rc.top;
		m_pDragImage->DragMove( p );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTreeCtrlReport::OnCaptureChanged( CWnd* pWnd)
{
	if (pWnd != this)
	{
		// Stop the dragging
		m_pDropTargetRow = 0;
		m_bDragging = false;
		m_bDragEx = false;
		m_ptDrag.SetPoint(0,0);
	}
	__super::OnCaptureChanged(pWnd);
}

//////////////////////////////////////////////////////////////////////////
void CTreeCtrlReport::OnReportItemDblClick( NMHDR* pNotifyStruct, LRESULT* result )
{
	XTP_NM_REPORTRECORDITEM* pItemNotify = (XTP_NM_REPORTRECORDITEM*) pNotifyStruct;
	CXTPReportRow* pRow = pItemNotify->pRow;
	if (pRow)
	{
		OnItemDblClick(pRow);
	}
	*result = TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CTreeCtrlReport::OnReportRowExpandChanged(NMHDR*  pNotifyStruct, LRESULT* /*result*/)
{
	XTP_NM_REPORTRECORDITEM* pItemNotify = (XTP_NM_REPORTRECORDITEM*) pNotifyStruct;
  ASSERT(pItemNotify != NULL);
	CXTPReportRow* pRow = pItemNotify->pRow;
	if (pRow)
	{
		OnItemExpanded( pRow,pRow->IsExpanded() == TRUE );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTreeCtrlReport::OnReportSelChanged(NMHDR*  pNotifyStruct, LRESULT* /*result*/)
{
	OnSelectionChanged();
}

//////////////////////////////////////////////////////////////////////////
void CTreeCtrlReport::OnFillItems()
{


}

//////////////////////////////////////////////////////////////////////////
void CTreeCtrlReport::Reload()
{
	BeginUpdate();
	GetRecords()->RemoveAll();

	OnFillItems();

	EndUpdate();
	Populate();
}

//////////////////////////////////////////////////////////////////////////
void CTreeCtrlReport::DeleteAllItems()
{
	BeginUpdate();
	GetRecords()->RemoveAll();
	EndUpdate();
}

//////////////////////////////////////////////////////////////////////////
CImageList* CTreeCtrlReport::CreateDragImage(CXTPReportRow* pRow)
{
	CXTPReportRecord* pRecord = pRow->GetRecord();
	if (pRecord == 0)
		return 0;

	CXTPReportRecordItem* pItem = pRecord->GetItem(0);
	if (pItem == 0)
		return 0;

	CXTPReportColumn* pCol = GetColumns()->GetAt(0);
	assert (pCol != 0);

	CClientDC	dc (this);
	CDC memDC;	
	if(!memDC.CreateCompatibleDC(&dc))
		return NULL;

	CRect rect;
	rect.SetRectEmpty();

	XTP_REPORTRECORDITEM_DRAWARGS drawArgs;
	drawArgs.pDC = &memDC;
	drawArgs.pControl = this;
	drawArgs.pRow = pRow;
	drawArgs.pColumn = pCol;
	drawArgs.pItem = pItem;
	drawArgs.rcItem = rect;
	XTP_REPORTRECORDITEM_METRICS metrics;
	pRow->GetItemMetrics(&drawArgs, &metrics);

	CString text = pItem->GetCaption(drawArgs.pColumn);

	CFont* pOldFont = memDC.SelectObject(GetFont());
	memDC.SelectObject(metrics.pFont);
	memDC.DrawText(text, &rect, DT_CALCRECT);

	HICON hIcon = m_imageList.ExtractIcon(pItem->GetIconIndex());
	int sx,sy;
	ImageList_GetIconSize(*&m_imageList, &sx, &sy);
	rect.right += sx+6;
	rect.bottom = sy > rect.bottom ? sy : rect.bottom;
	CRect boundingRect = rect;

	CBitmap bitmap;
	if(!bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height()))
		return NULL;

	CBitmap* pOldMemDCBitmap = memDC.SelectObject( &bitmap );
	memDC.SetBkColor(	metrics.clrBackground);
	memDC.FillSolidRect(&rect, metrics.clrBackground);
	memDC.SetTextColor(GetPaintManager()->m_clrWindowText);

	::DrawIconEx (*&memDC, rect.left+2, rect.top, hIcon, sx, sy, 0, NULL, DI_NORMAL); 

	rect.left += sx+4;
	rect.top -= 1;
	memDC.DrawText(text, &rect, 0);
	memDC.SelectObject( pOldFont );
	memDC.SelectObject( pOldMemDCBitmap );

	// Create image list
	CImageList* pImageList = new CImageList;
	pImageList->Create(boundingRect.Width(), boundingRect.Height(), 
		ILC_COLOR | ILC_MASK , 0, 1);
	pImageList->Add(&bitmap, metrics.clrBackground); // Here green is used as mask color

	::DestroyIcon(hIcon);
	bitmap.DeleteObject();
	return pImageList;
}

//////////////////////////////////////////////////////////////////////////
int CTreeCtrlReport::GetSelectedCount()
{
	return GetSelectedRows()->GetCount();
}

//////////////////////////////////////////////////////////////////////////
int CTreeCtrlReport::GetSelectedRecords( Records &items )
{
	CXTPReportSelectedRows *pRows = GetSelectedRows();
	items.reserve( pRows->GetCount() );
	for (int i = 0,nNumRows = pRows->GetCount(); i < nNumRows; i++)
	{
		CXTPReportRow *pRow = pRows->GetAt(i);
		CTreeItemRecord *pRecord = (CTreeItemRecord*)pRow->GetRecord();
		if (pRecord)
		{
			items.push_back( pRecord );
		}
	}
	return items.size();
}

//////////////////////////////////////////////////////////////////////////
void CTreeCtrlReport::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	__super::OnVScroll( nSBCode,nPos,pScrollBar );
	OnVerticalScroll( nSBCode,nPos,pScrollBar );
}

//////////////////////////////////////////////////////////////////////////
bool CTreeCtrlReport::OnBeginDragAndDrop( CXTPReportRow *pRow, CPoint pt )
{
	if (!pRow->GetRecord())
		return false;

	m_pDragRow = pRow;

	m_pDragImage = CreateDragImage(pRow);
	if (m_pDragImage)
	{
		m_pDragImage->BeginDrag(0, CPoint(-10, -10));

		//m_dragNodeClass = pRec->GetName();

		ScreenToClient(&pt);

		CRect rc;
		GetWindowRect( rc );

		CPoint p = pt;
		ClientToScreen( &p );
		p.x -= rc.left;
		p.y -= rc.top;

		RedrawControl();
		m_pDragImage->DragEnter( this,p );
		SetCapture();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTreeCtrlReport::HighlightDropTarget( CTreeItemRecord *pItem )
{
}
