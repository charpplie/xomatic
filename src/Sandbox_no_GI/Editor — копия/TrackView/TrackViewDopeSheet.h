////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   TrackViewDopeSheet.h
//  Version:     v1.00
//  Created:     23/8/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __trackviewkeylist_h__
#define __trackviewkeylist_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "TrackViewDopeSheetBase.h"

/** List of tracks.
*/
class CTrackViewDopeSheet : public CTrackViewDopeSheetBase
{
	DECLARE_DYNAMIC(CTrackViewDopeSheet)
public:
	// public stuff.

	CTrackViewDopeSheet();
	~CTrackViewDopeSheet();


protected:
	DECLARE_MESSAGE_MAP()

	void DrawTrack( int item,CDC *dc,CRect &rcItem );
	void DrawKeys( IAnimTrack *track,CDC *dc,CRect &rc,Range &timeRange );
	void DrawKeyDuration( IAnimTrack *track,CDC *dc,CRect &rc, int keyIndex );
	void DrawNodeItem( IAnimNode *pAnimNode,CDC *dc,CRect &rcItem );
	void DrawGoToTrackArrow( IAnimTrack *track,CDC *dc,CRect &rc);

	// Ovverides from CTrackViewKeys.
	int FirstKeyFromPoint( CPoint point ) const;
	int DurationKeyFromPoint( CPoint point ) const;
	int CheckCursorOnStartEndTimeAdjustBar( CPoint point, bool& bStart ) const;
	void SelectKeys( const CRect &rc );

	int NumKeysFromPoint( CPoint point ) const;
};


#endif // __trackviewkeylist_h__