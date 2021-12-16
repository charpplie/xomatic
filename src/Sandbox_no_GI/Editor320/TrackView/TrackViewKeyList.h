////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   trackviewkeylist.h
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

#include "TrackViewKeys.h"

/** List of tracks.
*/
class CTrackViewKeyList : public CTrackViewKeys
{
	DECLARE_DYNAMIC(CTrackViewKeyList)
public:
	// public stuff.

	CTrackViewKeyList();
	~CTrackViewKeyList();


protected:
	DECLARE_MESSAGE_MAP()

	void DrawTrack( int item,CDC *dc,CRect &rcItem );
	void DrawKeys( IAnimTrack *track,CDC *dc,CRect &rc,Range &timeRange );
	void DrawNodeItem( IAnimNode *pAnimNode,CDC *dc,CRect &rcItem );
	void DrawGoToTrackArrow( IAnimTrack *track,CDC *dc,CRect &rc);

	// Ovverides from CTrackViewKeys.
	int FirstKeyFromPoint( CPoint point ) const;
	void SelectKeys( const CRect &rc );

	int NumKeysFromPoint( CPoint point ) const;
};


#endif // __trackviewkeylist_h__