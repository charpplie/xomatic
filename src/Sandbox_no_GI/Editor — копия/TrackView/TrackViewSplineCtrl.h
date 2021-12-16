////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   TrackViewSplineCtrl.h
//  Version:     v1.00
//  Created:     30/7/2009 by Jaewon.
//  Compilers:   Visual Studio 2008
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __TrackViewSplineCtrl_h__
#define __TrackViewSplineCtrl_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include <IMovieSystem.h>
#include "Controls/SplineCtrlEx.h"

/**	A customized spline control for CTrackViewGraph.
*/
class CTrackViewSplineCtrl : public CSplineCtrlEx
{
public:
	void AddSpline( ISplineInterpolator* pSpline, IAnimTrack* pTrack, COLORREF color );
	void AddSpline( ISplineInterpolator* pSpline, IAnimTrack* pTrack, COLORREF anColorArray[4]);

	void RemoveAllSplines();

	void OnUserCommand( UINT cmd );
	bool IsUnifiedKeyCurrentlySelected() const;
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	
	std::vector<IAnimTrack*> m_tracks;

	virtual bool GetTangentHandlePts(CPoint& inTangentPt, CPoint& pt, CPoint& outTangentPt, 
																		int nSpline, int nKey, int nDimension);
	void ComputeIncomingTangentAndEaseTo(float& ds, float& easeTo, CPoint inTangentPt, 
																				int nSpline, int nKey, int nDimension);
	void ComputeOutgoingTangentAndEaseFrom(float& dd, float& easeFrom, CPoint outTangentPt, 
																					int nSpline, int nKey, int nDimension);
	void AdjustTCB(float d_tension, float d_continuity, float d_bias);
	void MoveSelectedTangentHandleTo(CPoint point);
};

#endif // __TrackViewSplineCtrl_h__