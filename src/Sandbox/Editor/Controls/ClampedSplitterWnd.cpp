////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   ClampedSplitterWnd.cpp
//  Version:     v1.00
//  Created:     2014-04-17 by Timothy Brookes.
//  Description: A splitter window which clamps at minimum size rather than
//    culling panes smaller than minimum (MFC default)
// 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ClampedSplitterWnd.h"

IMPLEMENT_DYNAMIC(CClampedSplitterWnd, CXTSplitterWnd)

BEGIN_MESSAGE_MAP(CClampedSplitterWnd, CXTSplitterWnd)
END_MESSAGE_MAP()

void CClampedSplitterWnd::TrackRowSize(int y, int row)
{
	// Overriding base behaviour from CSplitterWnd
	// where if an entry is too small it disappears
	// we want it to instead not get that small

	ClampMovement(y, row, m_nRows, m_pRowInfo);
}

void CClampedSplitterWnd::TrackColumnSize(int x, int col)
{	
	// Overriding base behaviour from CSplitterWnd
	// where if an entry is too small it disappears
	// we want it to instead not get that small

	ClampMovement(x, col, m_nCols, m_pColInfo);
}

void CClampedSplitterWnd::ClampMovement(const int offset, const int index, const int numEntries, CRowColInfo* dataSet)
{
	// Asserts to mimic behaviour from CSplitterWnd
	ASSERT_VALID(this);
	ASSERT(numEntries > 1);

	// Determine change in splitter position
	static const int SPLITTER_WIDTH = 6;
	static const int HALF_SPLITTER_WIDTH = 3;
	int prevPos = -HALF_SPLITTER_WIDTH; // First panel only has one half a splitter in it, so negate half
	for (int i = 0; i <= index; i++)
	{
		prevPos += dataSet[i].nCurSize + SPLITTER_WIDTH; // Add the splitter to each
	}
	prevPos -= HALF_SPLITTER_WIDTH; // Last panel only has only half a splitter in it, so negate half
	int splitterPosChange = offset - prevPos;

	if (splitterPosChange > 0)
	{
		// Splitter moving right/down
		int indexID = numEntries-2;
		int requestedShrink = splitterPosChange;
		int accumulatedMovement = 0;

		while (indexID >= index && requestedShrink > 0)
		{
			RequestPanelShrink(indexID+1, requestedShrink, accumulatedMovement, dataSet);
			indexID--;
		}

		// Add the accumulated movement to the panel to the left of or above our selected splitter
		dataSet[index].nIdealSize = dataSet[index].nCurSize + accumulatedMovement;
	}
	else if (splitterPosChange < 0)
	{
		// Splitter moving left/up
		int indexID = 0;
		int requestedShrink = splitterPosChange * -1;
		int accumulatedMovement = 0;

		while (indexID <= index && requestedShrink > 0)
		{
			RequestPanelShrink(indexID, requestedShrink, accumulatedMovement, dataSet);
			indexID++;
		}

		// Add the accumulated movement to the panel to the right of or below our selected splitter
		dataSet[index+1].nIdealSize = dataSet[index+1].nCurSize + accumulatedMovement;
	}
}

// Attempts to shrink a panel, will not shrink smaller than minimum size
// requestedShrink: The amount we want to reduce from the panel
// accumulatedMovement: We add the amount we reduced from this panel to accumulatedMovement
void CClampedSplitterWnd::RequestPanelShrink(const int index, int& requestedShrink, int& accumulatedMovement, CRowColInfo* dataSet)
{
	// Determine how much we can shrink by, but no more than requested
	int availableChange = MIN(dataSet[index].nCurSize - dataSet[index].nMinSize, requestedShrink);

	// Deduct from amount we are going to shrink, and add to accumulated movement
	requestedShrink -= availableChange;
	accumulatedMovement += availableChange;

	// Apply the change to this panel
	dataSet[index].nIdealSize = dataSet[index].nCurSize - availableChange;
}