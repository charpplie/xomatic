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

#ifndef __TelemetryViewSplitter_h__
#define __TelemetryViewSplitter_h__

#if _MSC_VER > 1000
#pragma once
#endif


// CTelemetryViewSplitter

class CTelemetryViewSplitter : public CSplitterWnd
{
	DECLARE_DYNAMIC(CTelemetryViewSplitter)

	virtual CWnd* GetActivePane(int* pRow = NULL, int* pCol = NULL)
	{
		return GetFocus();
	}

	void SetPane( int row,int col,CWnd *pWnd,SIZE sizeInit );
	// Ovveride this for flat look.
	void OnDrawSplitter(CDC* pDC, ESplitType nType, const CRect& rectArg);
	
public:
	CTelemetryViewSplitter();
	virtual ~CTelemetryViewSplitter();

protected:
	DECLARE_MESSAGE_MAP()
};


#endif // __TelemetryViewSplitter_h__