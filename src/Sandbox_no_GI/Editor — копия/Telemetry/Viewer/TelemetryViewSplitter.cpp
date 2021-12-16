////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   TelemetryViewSplitter.h
//  Version:     v1.00
//  Created:     13/05/11 by Steve Humphreys
//  Description: Splitter bar between timeline and events controls
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "TelemetryViewSplitter.h"


// CTelemetryViewSplitter

IMPLEMENT_DYNAMIC(CTelemetryViewSplitter, CSplitterWnd)
CTelemetryViewSplitter::CTelemetryViewSplitter()
{
	m_cxSplitter = m_cySplitter = 3 + 1 + 1 -1 ;
	m_cxBorderShare = m_cyBorderShare = 0;
	m_cxSplitterGap = m_cySplitterGap = 3 + 1 + 1 - 1;
	m_cxBorder = m_cyBorder = 0;
}

CTelemetryViewSplitter::~CTelemetryViewSplitter()
{
}


BEGIN_MESSAGE_MAP(CTelemetryViewSplitter, CSplitterWnd)
END_MESSAGE_MAP()



// CTelemetryViewSplitter message handlers

void CTelemetryViewSplitter::SetPane( int row,int col,CWnd *pWnd,SIZE sizeInit )
{
	assert( pWnd != NULL );
	
	// set the initial size for that pane
	m_pColInfo[col].nIdealSize = sizeInit.cx;
	m_pRowInfo[row].nIdealSize = sizeInit.cy;

	pWnd->ModifyStyle( 0,WS_BORDER,WS_CHILD|WS_VISIBLE );
	pWnd->SetParent(this);
	
	CRect rect(CPoint(0,0), sizeInit);
	pWnd->MoveWindow( 0,0,sizeInit.cx,sizeInit.cy,FALSE );
	pWnd->SetDlgCtrlID( IdFromRowCol(row,col) );

	ASSERT((int)::GetDlgCtrlID(pWnd->m_hWnd) == IdFromRowCol(row, col));
}

void CTelemetryViewSplitter::OnDrawSplitter(CDC* pDC, ESplitType nType, const CRect& rectArg)
{
	// Let CSplitterWnd handle everything but the border-drawing
	//if((nType != splitBorder) || (pDC == NULL))
	{
		CSplitterWnd::OnDrawSplitter(pDC, nType, rectArg);
		return;
	}

	ASSERT_VALID(pDC);

	// Draw border
	pDC->Draw3dRect(rectArg, GetSysColor(COLOR_BTNSHADOW), GetSysColor(COLOR_BTNHIGHLIGHT));
}