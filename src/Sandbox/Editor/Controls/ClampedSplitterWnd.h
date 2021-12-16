////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   ClampedSplitterWnd.h
//  Version:     v1.00
//  Created:     2014-04-17 by Timothy Brookes.
//  Description: A splitter window which clamps at minimum size rather than
//    culling panes smaller than minimum (MFC default)
// 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////


#ifndef __clampedsplitterwnd_h__
#define __clampedsplitterwnd_h__

#if _MSC_VER > 1000
#pragma once
#endif

// CClampedSplitterWnd

class CClampedSplitterWnd : public CXTSplitterWnd
{
	DECLARE_DYNAMIC(CClampedSplitterWnd)

public:
	CClampedSplitterWnd() {};
	virtual ~CClampedSplitterWnd() {};

protected:
	DECLARE_MESSAGE_MAP()

	// Overrides for standard CSplitterWnd behaviour
	virtual void TrackRowSize(int y, int row);
	virtual void TrackColumnSize(int x, int col);

private:
	// Attempts to resize panels but will not allow resizing smaller than min. Will bunch up multiple panels to try and meet resize requirements
	// offset: The position of the splitter relative to the entry at index
	// index: The index of the dataset which is to the left of the splitter
	// numEntries: The number of entries in this dataset
	// dataSet: The dataset (rows / cols)
	void ClampMovement(const int offset, const int index, const int numEntries, CRowColInfo* dataSet);

	// Attempts to shrink a panel, will not shrink smaller than minimum size
	// requestedShrink: The amount we want to reduce from the panel
	// accumulatedMovement: We add the amount we reduced from this panel to accumulatedMovement
	void RequestPanelShrink(const int index, int& requestedShrink, int& accumulatedMovement, CRowColInfo* dataSet);
};

#endif // __clampedsplitterwnd_h__