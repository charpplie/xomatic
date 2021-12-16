////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   SequencerDopeSheet.h
//  Version:     v1.00
//  Created:     23/8/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __sequencerkeylist_h__
#define __sequencerkeylist_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "SequencerDopeSheetBase.h"

/** List of tracks.
*/
class CSequencerDopeSheet : public CSequencerDopeSheetBase
{
	DECLARE_DYNAMIC(CSequencerDopeSheet)
public:
	// public stuff.

	CSequencerDopeSheet();
	~CSequencerDopeSheet();


protected:
	DECLARE_MESSAGE_MAP()

	void DrawTrack( int item,CDC *dc,CRect &rcItem );
	void DrawKeys( CSequencerTrack *track,CDC *dc,CRect &rc,Range &timeRange,EDSRenderFlags renderFlags );
	void DrawNodeItem( CSequencerNode *pAnimNode,CDC *dc,CRect &rcItem );

	// Overrides from CSequencerKeys.
	int FirstKeyFromPoint( CPoint point, bool exact = false ) const;
	int LastKeyFromPoint( CPoint point, bool exact = false ) const;
	void SelectKeys( const CRect &rc );

	int NumKeysFromPoint( CPoint point ) const;
};


#endif // __sequencerkeylist_h__