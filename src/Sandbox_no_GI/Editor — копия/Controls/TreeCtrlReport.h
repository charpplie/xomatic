////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   TreeCtrlReport.h
//  Version:     v1.00
//  Created:     02/04/2009 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __TreeCtrlReport_h__
#define __TreeCtrlReport_h__
#pragma once

//////////////////////////////////////////////////////////////////////////
class CTreeCtrlReport : public CXTPReportControl
{
	DECLARE_MESSAGE_MAP()
public:
	class CTreeItemRecord : public CXTPReportRecord
	{
	public:
		CTreeItemRecord() : m_bIsGroup(false), m_nHeight(-1), m_bDropTarget(false) {}
		CTreeItemRecord( bool bIsGroup, const CString& name )
			: m_name(name), m_bIsGroup(bIsGroup), m_nHeight(-1), m_bDropTarget(false)
		{
			CXTPReportRecordItem *pItem = new CXTPReportRecordItemText(name);
			AddItem(pItem);
			if (bIsGroup)
			{
				pItem->SetBold(TRUE);
				pItem->SetIconIndex(0);
			}
			else
				pItem->SetIconIndex(1);
		}

		bool IsGroup() const { return m_bIsGroup; }
		const CString& GetName() const { return m_name; }

		void SetRect( const CRect & rc ) { m_clientRect = rc; }
		CRect GetRect() const { return m_clientRect; }

		void  SetItemHeight( int nHeight ) { m_nHeight = nHeight; };
		int   GetItemHeight() const { return m_nHeight; }

		void  SetDropTarget( bool bDropTarget ) { m_bDropTarget = bDropTarget; }
		bool  IsDropTarget() const { return m_bDropTarget; }

	protected:
		int m_nHeight;
		CString m_name;
		bool m_bIsGroup;
		bool m_bDropTarget;
		CRect m_clientRect;
	};
	typedef std::vector<CTreeItemRecord*> Records;

	CTreeCtrlReport();
	~CTreeCtrlReport();

public:
	//////////////////////////////////////////////////////////////////////////
	virtual void Reload();
	virtual int GetSelectedCount();
	virtual int GetSelectedRecords( Records &items );

protected:
	//////////////////////////////////////////////////////////////////////////
	// Can be overriden by defrived classes.
	//////////////////////////////////////////////////////////////////////////
	virtual void OnFillItems();
	virtual void OnItemExpanded( CXTPReportRow *pRow,bool bExpanded ) {};
	virtual void OnSelectionChanged() {};
	virtual bool OnBeginDragAndDrop( CXTPReportRow *pRow, CPoint point );
	virtual void OnDragAndDrop( CXTPReportRow *pRow, CPoint absoluteCursorPos ) {};
	virtual void OnVerticalScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) {};
	virtual void OnItemDblClick( CXTPReportRow *pRow ) {};

	virtual CImageList* CreateDragImage(CXTPReportRow* pRow);
	//////////////////////////////////////////////////////////////////////////

protected:
	//////////////////////////////////////////////////////////////////////////
	afx_msg void OnLButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnLButtonUp( UINT nFlags, CPoint point );
	afx_msg void OnMouseMove( UINT nFlags, CPoint point );
	afx_msg void OnCaptureChanged( CWnd* );
	afx_msg void OnReportColumnRClick( NMHDR* pNotifyStruct, LRESULT* result );
	afx_msg void OnReportItemDblClick( NMHDR* pNotifyStruct, LRESULT* result );
	afx_msg void OnReportRowExpandChanged( NMHDR* pNotifyStruct, LRESULT* result );
	afx_msg void OnReportSelChanged( NMHDR* pNotifyStruct, LRESULT* result );
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	afx_msg void OnDestroy();

	bool IsHeaderVisible();
	void SetHeaderVisible(bool bVisible);
	void DeleteAllItems();
	void HighlightDropTarget( CTreeItemRecord *pItem );

protected:
	CImageList m_imageList;
	CImageList *m_pDragImage;
	CXTPReportRow *m_pDragRow;
	
	CTreeItemRecord *m_pDropTargetRow;

	uint32 m_mask;
	CPoint m_ptDrag;
	bool m_bDragging;
	bool m_bDragEx;
	bool m_bHeaderVisible;
};

#endif //__TreeCtrlReport_h__