// TrackViewDopeSheet.cpp : implementation file
//

#include "stdafx.h"
#include "TrackViewDopeSheet.h"
#include "Controls\MemDC.h"

#include "TrackViewDialog.h"
#include "AnimationContext.h"

#include "TrackViewUndo.h"

//#include "IMovieSystem.h"

#define KEY_TEXT_COLOR RGB(0,0,50)
#define INACTIVE_TEXT_COLOR RGB(128,128,128)

// CTrackViewKeyList

IMPLEMENT_DYNAMIC(CTrackViewDopeSheet, CTrackViewDopeSheetBase)
CTrackViewDopeSheet::CTrackViewDopeSheet()
{
	m_leftOffset = 30;

	m_itemWidth = 1000;
	m_itemHeight = 16;
}

CTrackViewDopeSheet::~CTrackViewDopeSheet()
{
}

BEGIN_MESSAGE_MAP(CTrackViewDopeSheet, CTrackViewDopeSheetBase)
END_MESSAGE_MAP()

// CTrackViewKeyList message handlers

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheet::DrawTrack( int item,CDC *dc,CRect &rcItem )
{
	CPen pen(PS_SOLID,1,RGB(120,120,120));
	CPen *prevPen = dc->SelectObject( &pen );
	dc->MoveTo( rcItem.left,rcItem.bottom );
	dc->LineTo( rcItem.right,rcItem.bottom );
	dc->SelectObject( prevPen );

	IAnimTrack *track = GetTrack(item);
	if (!track)
	{
		IAnimNode *pAnimNode = GetNode(item);
		if (pAnimNode && GetItem(item).paramId == 0)
		{
			// Draw Anim node track.
			DrawNodeItem( pAnimNode,dc,rcItem );
		}
		return;
	}

	//dc->Draw3dRect( rcItem,GetSysColor(COLOR_3DHILIGHT),GetSysColor(COLOR_3DDKSHADOW) );
	//int minx = m_leftOffset;
	//int maxx = min( TimeToClient(

	CRect rcInner = rcItem;
	//rcInner.DeflateRect(m_leftOffset,0,m_leftOffset,0);
	rcInner.left = max( rcItem.left,m_leftOffset - m_scrollOffset.x );
	rcInner.right = min( rcItem.right,(m_scrollMax + m_scrollMin) - m_scrollOffset.x + m_leftOffset*2 );

	bool bLightAnimationSetActive = m_pSequence && (m_pSequence->GetFlags() & IAnimSequence::LIGHT_ANIMATION_SET);
	if (bLightAnimationSetActive && track->GetNumKeys() > 0)
	{
		// In the case of the light animation set, the time of of the last key
		// determines the end of the track.
		float lastKeyTime = track->GetKeyTime(track->GetNumKeys()-1);
		rcInner.right = min( rcInner.right, static_cast<LONG>(TimeToClient(lastKeyTime)) );
	}

	CRect rcInnerDraw( rcInner.left-6,rcInner.top,rcInner.right+6,rcInner.bottom );
	ColorB trackColor = track->GetColor();
	// For the case of tracks belonging to an inactive director node,
	// changes the track color to a redish one.
	ColorB colorForInactiveDirectorTrack(255, 224, 224);
	IAnimNode *pAnimNode = GetNode(item);
	IAnimNode *pDirectorNode = NULL;
	if (pAnimNode->GetType() == ANODE_SCENE)
		pDirectorNode = pAnimNode;
	else
		pDirectorNode = pAnimNode->HasDirectorAsParent();
	if (pDirectorNode && pDirectorNode != pAnimNode->GetSequence()->GetActiveDirector())
		trackColor = colorForInactiveDirectorTrack;
	CRect rc = rcInnerDraw;
	rc.DeflateRect(0,1,0,0);


	const EAnimTrackType trackType = track->GetType();
	if (trackType == ATRACK_TCB_FLOAT || trackType == ATRACK_TCB_QUAT || trackType == ATRACK_TCB_VECTOR)
	{
		trackColor = ColorB(245, 80, 70);
	}

	if (IsSelectedItem(item))
	{
		XTPPaintManager()->GradientFill( dc,rc,RGB(trackColor.r,trackColor.g,trackColor.b),
																		RGB(trackColor.r/2,trackColor.g/2,trackColor.b/2),FALSE );
	}
	else
	{
		dc->FillSolidRect(rc.left, rc.top, rc.Width(), rc.Height(), 
											RGB(trackColor.r,trackColor.g,trackColor.b));
	}

	// Left outside
	CRect rcOutside = rcItem;
	rcOutside.right = rcInnerDraw.left-1;
	rcOutside.DeflateRect(1,1,1,0);
	dc->SelectObject( m_bkgrBrushEmpty );
	//dc->Rectangle( rcOutside );

	//rcOutside.DeflateRect(1,1,1,1);
	XTPPaintManager()->GradientFill( dc,rcOutside,RGB(210,210,210),RGB(180,180,180),FALSE );

	// Right outside.
	rcOutside = rcItem;
	rcOutside.left = rcInnerDraw.right+1;
	rcOutside.DeflateRect(1,1,1,0);
	//dc->Rectangle( rcOutside );

	//rcOutside.DeflateRect(1,1,1,1);
	XTPPaintManager()->GradientFill( dc,rcOutside,RGB(210,210,210),RGB(180,180,180),FALSE );

	// Get time range of update rectangle.
	Range timeRange = GetTimeRange(rcItem);
	
	// Draw keys in time range.
	DrawKeys( track,dc,rcInner,timeRange );

	// Draw tick marks in time range.
	DrawTicks( dc,rcInner,timeRange );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheet::DrawKeys( IAnimTrack *track,CDC *dc,CRect &rc,Range &timeRange )
{
	char keydesc[1024];
	int numKeys = track->GetNumKeys();

	EAnimValue trackValueType = track->GetValueType();

	//////////////////////////////////////////////////////////////////////////
	// If this track is boolean draw bars to show true value.
	if (trackValueType == AVALUE_BOOL)
	{
		int x0 = TimeToClient(timeRange.start);
		float t0 = timeRange.start;
		CRect itemrc;

		CBrush *prevBrush = dc->SelectObject( &m_visibilityBrush );
		for (int i = 0; i < numKeys; i++)
		{
			float time = track->GetKeyTime(i);
			if (time < timeRange.start)
				continue;
			if (time > timeRange.end)
				break;

			int x = TimeToClient(time);
			bool val = false;
			track->GetValue( time-0.001f,val );
			if (val)
			{
				XTPPaintManager()->GradientFill( dc,CRect(x0,rc.top+4,x,rc.bottom-4),RGB(250,250,250),RGB(00,80,255),FALSE );
				//dc->Rectangle( x0,rc.top+5,x,rc.bottom-5 );
			}

			t0 = time;
			x0 = x;
		}
		int x = TimeToClient(timeRange.end);
		bool val = false;
		track->GetValue( timeRange.end-0.001f,val );
		if (val)
		{
			XTPPaintManager()->GradientFill( dc,CRect(x0,rc.top+4,x,rc.bottom-4),RGB(250,250,250),RGB(00,80,255),FALSE );
			//dc->Rectangle(  );
		}
		dc->SelectObject( &prevBrush );
	}
	//////////////////////////////////////////////////////////////////////////
	else if (trackValueType == AVALUE_SELECT)  // If this track is Select Track draw bars to show where selection is active.
	{
		int x0 = TimeToClient(timeRange.start);
		float t0 = timeRange.start;
		CRect itemrc;

		CBrush *prevBrush = dc->SelectObject( &m_selectTrackBrush );
		for (int i = 0; i < numKeys; i++)
		{
			ISelectKey key;
			track->GetKey( i,&key );
			if (*key.szSelection != 0)
			{
				float time = track->GetKeyTime(i);
				float nextTime = timeRange.end;
				if (i < numKeys-1)
					nextTime = track->GetKeyTime(i+1);
				time = __clamp(time,timeRange.start,timeRange.end);
				nextTime = __clamp(nextTime,timeRange.start,timeRange.end);

				int x0 = TimeToClient(time);
				int x = TimeToClient(nextTime);
				
				if (x != x0)
				{
					XTPPaintManager()->GradientFill( dc,CRect(x0,rc.top+1,x,rc.bottom),RGB(255,255,255),RGB(100,190,255),FALSE );

					//dc->Rectangle( x0,rc.top+3,x,rc.bottom-2 );
				}
			}
		}
		dc->SelectObject( &prevBrush );
	}
	//////////////////////////////////////////////////////////////////////////
	else if (trackValueType == AVALUE_SEQUENCE)  // If this track is Sequence Track draw bars to show where sequence is active.
	{
		int x0 = TimeToClient(timeRange.start);
		float t0 = timeRange.start;
		CRect itemrc;

		CBrush *prevBrush = dc->SelectObject( &m_selectTrackBrush );
		for (int i = 0; i < numKeys-1; i++)
		{
			ISequenceKey key;
			track->GetKey( i,&key );
			if (*key.szSelection != 0)
			{
				float time = track->GetKeyTime(i);
				float nextTime = timeRange.end;
				if (i < numKeys-1)
					nextTime = track->GetKeyTime(i+1);
				time = __clamp(time,timeRange.start,timeRange.end);
				nextTime = __clamp(nextTime,timeRange.start,timeRange.end);

				int x0 = TimeToClient(time);
				int x = TimeToClient(nextTime);
				
				if (x != x0)
				{
					COLORREF startColour = RGB(100,190,255);
					COLORREF endColour = RGB(250, 250, 250);
					XTPPaintManager()->GradientFill( dc,CRect(x0,rc.top+1,x,rc.bottom),startColour,endColour,FALSE );
				}
			}
		}
		dc->SelectObject( &prevBrush );
	}
	//////////////////////////////////////////////////////////////////////////
	else if(trackValueType == AVALUE_DISCRETE_FLOAT && track->GetType() == ATRACK_GOTO) // if this track is GoTo Track, draw an arrow to indicate jump position.
	{
		DrawGoToTrackArrow(track,dc,rc);
	}
	//////////////////////////////////////////////////////////////////////////

	CFont *prevFont = dc->SelectObject( m_descriptionFont );

	dc->SetTextColor(KEY_TEXT_COLOR);
	dc->SetBkMode(TRANSPARENT);

	int prevKeyPixel = -10000;
	float time0 = FLT_MIN;
	const int kDefaultWidthForDescription = 200;
	const int kSmallMargin = 10;
	// Draw keys.
	for (int i = 0; i < numKeys; i++)
	{
		float time = track->GetKeyTime(i);
		int x = TimeToClient(time);
		if (x-kSmallMargin > rc.right)
			continue;
		
		int x1 = x + kDefaultWidthForDescription;
		int nextKey = track->NextKeyByTime(i);
		if (nextKey > 0)
		{
			x1 = TimeToClient( track->GetKeyTime(nextKey) ) - kSmallMargin;
		}

		if (x1 > x + kSmallMargin)	// Enough space for description text or duration bar
		{
			// Get info about that key.
			const char *description = NULL;
			float duration = 0;
			track->GetKeyInfo(i,description,duration);

			int xlast = x;
			if (duration > 0)
				xlast = TimeToClient(time+duration);
			if (xlast + kSmallMargin < rc.left)
				continue;
			
			if (duration > 0)
			{
				DrawKeyDuration(track, dc, rc, i);
			}

			if (description)
			{
				strcpy_s(keydesc,"{");
				strcat_s(keydesc,description);
				strcat_s(keydesc,"}");
				// Draw key description text.
				// Find next key.
				CRect textRect(x+10,rc.top,x1,rc.bottom );
				//textRect &= rc;
				dc->DrawText( keydesc,strlen(keydesc),textRect,DT_LEFT|DT_END_ELLIPSIS|DT_VCENTER|DT_SINGLELINE  );
			}
		}

		if (x < 0)
			continue;

		if (track->GetSubTrackCount() == 0 // At compound tracks, keys are all green.
		&& abs(x-prevKeyPixel) < 2)
		{
			// If multiple keys on the same time.
			m_imageList.Draw( dc,2,CPoint(x-6,rc.top+2),ILD_TRANSPARENT );
		}
		else
		{
			if (track->IsKeySelected(i))
			{
				m_imageList.Draw( dc,1,CPoint(x-6,rc.top+2),ILD_TRANSPARENT );
			}
			else
			{
				m_imageList.Draw( dc,0,CPoint(x-6,rc.top+2),ILD_TRANSPARENT );
			}
		}

		prevKeyPixel = x;
		time0 = time;
	}
	dc->SelectObject( prevFont );
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewDopeSheet::FirstKeyFromPoint( CPoint point ) const
{
	int item = ItemFromPoint(point);
	if (item < 0)
		return -1;

	float t1 = TimeFromPointUnsnapped( CPoint(point.x-4,point.y) );
	float t2 = TimeFromPointUnsnapped( CPoint(point.x+4,point.y) );

	IAnimTrack *track = GetTrack(item);
	if (!track)
		return -1;

	int numKeys = track->GetNumKeys();
	for (int i = 0; i < numKeys; i++)
	{
		float time = track->GetKeyTime(i);
		if (time >= t1 && time <= t2)
		{
			return i;
		}
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewDopeSheet::DurationKeyFromPoint( CPoint point ) const
{
	int item = ItemFromPoint(point);
	if (item < 0)
		return -1;

	float t = TimeFromPointUnsnapped( CPoint(point.x,point.y) );

	IAnimTrack *track = GetTrack(item);
	if (!track)
		return -1;

	int numKeys = track->GetNumKeys();
	// Iterate in a reverse order to prioritize later nodes.
	for (int i = numKeys-1; i >= 0; --i)
	{
		float time = track->GetKeyTime(i);
		const char *description = NULL;
		float duration = 0;
		track->GetKeyInfo(i, description, duration);
		if (t >= time && t <= time+duration)
		{
			return i;
		}
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewDopeSheet::CheckCursorOnStartEndTimeAdjustBar( CPoint point, bool& bStart ) const
{
	int item = ItemFromPoint(point);
	if (item < 0)
		return -1;

	IAnimTrack *track = GetTrack(item);
	if (!track)
		return -1;

	if (track->GetType() != ATRACK_CHARACTER)
		return -1;

	int numKeys = track->GetNumKeys();
	for (int i = 0; i < numKeys; i++)
	{
		if (track->IsKeySelected(i) == false)
			continue;
		float time = track->GetKeyTime(i);
		const char *description = NULL;
		float duration = 0;
		track->GetKeyInfo(i, description, duration);
		if (duration == 0)
			continue;
		int stime = TimeToClient(time);
		int etime = TimeToClient(time+duration);
		if (point.x >= stime-3 && point.x <= stime)
		{
			bStart = true;
			return i;
		}
		else if (point.x >= etime && point.x <= etime+3)
		{
			bStart = false;
			return i;
		}
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewDopeSheet::NumKeysFromPoint( CPoint point ) const
{
	int item = ItemFromPoint(point);
	if (item < 0)
		return -1;

	float t1 = TimeFromPointUnsnapped( CPoint(point.x-4,point.y) );
	float t2 = TimeFromPointUnsnapped( CPoint(point.x+4,point.y) );

	IAnimTrack *track = GetTrack(item);
	if (!track)
		return -1;

	int count = 0;
	int numKeys = track->GetNumKeys();
	for (int i = 0; i < numKeys; i++)
	{
		float time = track->GetKeyTime(i);
		if (time >= t1 && time <= t2)
		{
			++count;
		}
	}
	return count;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheet::SelectKeys( const CRect &rc )
{
	// put selection rectangle from client to item space.
	CRect rci = rc;
	rci.OffsetRect( m_scrollOffset );

	Range selTime = GetTimeRange( rci );

	CRect rcItem;
	for (int i = 0; i < GetCount(); i++)
	{
		GetItemRect(i,rcItem);
		// Decrease item rectangle a bit.
		rcItem.DeflateRect(4,4,4,4);
		// Check if item rectanle intersects with selection rectangle in y axis.
		if ((rcItem.top >= rc.top && rcItem.top <= rc.bottom) || 
				(rcItem.bottom >= rc.top && rcItem.bottom <= rc.bottom) ||
				(rc.top >= rcItem.top && rc.top <= rcItem.bottom) ||
				(rc.bottom >= rcItem.top && rc.bottom <= rcItem.bottom))
		{
			IAnimTrack *track = GetTrack(i);
			if (!track)
				continue;

			// Check which keys we intersect.
			for (int j = 0; j < track->GetNumKeys(); j++)
			{
				float time = track->GetKeyTime(j);
				if (selTime.IsInside(time))
				{
					track->SelectKey(j, true);
					m_bAnySelected = true;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheet::DrawNodeItem( IAnimNode *pAnimNode,CDC *dc,CRect &rcItem )
{
	CFont *prevFont = dc->SelectObject( m_descriptionFont );

	IAnimNode *pDirectorNode = NULL;
	if (pAnimNode->GetType() == ANODE_SCENE)
		pDirectorNode = pAnimNode;
	else
		pDirectorNode = pAnimNode->HasDirectorAsParent();
	if (pDirectorNode && pDirectorNode != pAnimNode->GetSequence()->GetActiveDirector())
		dc->SetTextColor(INACTIVE_TEXT_COLOR);
	else
		dc->SetTextColor(KEY_TEXT_COLOR);
	dc->SetBkMode(TRANSPARENT);

	CRect textRect = rcItem;
	textRect.left += 4;
	textRect.right -= 4;

	string sAnimNodeName = pAnimNode->GetName();
	int trackCount = pAnimNode->GetTrackCount();
	bool hasObsoleteTrack = false;
	for(int i=0; i<trackCount; ++i)
	{
		if(hasObsoleteTrack)
			break;

		IAnimTrack* track = pAnimNode->GetTrackByIndex(i);
		if(!track)
			continue;

		int subTrackCount = track->GetSubTrackCount();
		for(int j=0; j<subTrackCount; ++j)
		{
			IAnimTrack* subTrack = track->GetSubTrack(j);
			if(!subTrack)
				continue;

			EAnimTrackType trackType = subTrack->GetType();
			if (trackType == ATRACK_TCB_FLOAT || trackType == ATRACK_TCB_QUAT || trackType == ATRACK_TCB_VECTOR)
			{
				hasObsoleteTrack = true;
				break;
			}
		}		
	}
	
	if(hasObsoleteTrack)
	{
		dc->SetTextColor( RGB(245, 80, 70));
		sAnimNodeName += ": Some of the sub-tracks contains obsoleted TCB splines (marked in red), thus cannot be copied or pasted.";
	}

	dc->DrawText( sAnimNodeName.c_str(), sAnimNodeName.length(), textRect, DT_LEFT|DT_END_ELLIPSIS|DT_VCENTER|DT_SINGLELINE  );

	dc->SelectObject( prevFont );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheet::DrawGoToTrackArrow( IAnimTrack *track,CDC *dc,CRect &rc)
{
	int numKeys = track->GetNumKeys();
	const COLORREF colorLine = RGB(150,150,150);
	const COLORREF colorHeader = RGB(50,50,50);
	const int tickness = 2;
	const int halfMargin = (rc.Height() - tickness)/2;

	for(int i=0; i<numKeys; ++i)
	{
		IDiscreteFloatKey key;
		track->GetKey(i,&key);
		int arrowStart = TimeToClient(key.time);
		int arrowEnd = TimeToClient(key.m_fValue);

		if(key.m_fValue < 0.f)
			continue;

		// draw arrow body line
		if(arrowStart < arrowEnd)
		{
			XTPPaintManager()->GradientFill
				(dc, CRect(arrowStart, rc.top+halfMargin, arrowEnd, rc.bottom-halfMargin), colorLine, colorLine, FALSE);
		}
		else if(arrowStart > arrowEnd)
		{
			XTPPaintManager()->GradientFill
				(dc, CRect(arrowEnd, rc.top+halfMargin, arrowStart, rc.bottom-halfMargin), colorLine, colorLine, FALSE);
		}

		// draw arrow head
		if(arrowStart != arrowEnd)
		{
			XTPPaintManager()->GradientFill
				(dc, CRect(arrowEnd, rc.top+2, arrowEnd+1, rc.bottom-2), colorHeader,colorHeader,FALSE);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewDopeSheet::DrawKeyDuration( IAnimTrack *track,CDC *dc,CRect &rc, int keyIndex )
{
	float time = track->GetKeyTime(keyIndex);
	const char *description = NULL;
	float duration = 0;
	track->GetKeyInfo(keyIndex,description,duration);
	int x = TimeToClient(time);

	// Draw key duration.
	float endt = min(time+duration,m_timeRange.end);
	int x1 = TimeToClient(endt);
	if (x1 < 0)
	{
		if (x > 0)
			x1 = rc.right;
	}
	CBrush *prevBrush = dc->SelectObject( &m_visibilityBrush );
	XTPPaintManager()->GradientFill( dc,CRect(x,rc.top+3,x1+1,rc.bottom-3),RGB(120,120,255),RGB(250,250,250),FALSE );

	dc->SelectObject( &prevBrush );
	dc->MoveTo(x1,rc.top);
	dc->LineTo(x1,rc.bottom);

	// If it is a selected animation track, draw the whole animation box (in green)
	// and two adjust bars (in red) for start/end time each, too.
	if (track->IsKeySelected(keyIndex) && track->GetType() == ATRACK_CHARACTER)
	{
		// Draw the whole animation box.
		ICharacterKey key;
		track->GetKey(keyIndex, &key);
		int startX = TimeToClient(time - key.m_startTime / key.m_speed);
		int endX = TimeToClient(time + (key.m_duration - key.m_startTime) / key.m_speed);
		CPen greenPen(PS_SOLID,1,RGB(0,255,0));
		CPen *prevPen = dc->SelectObject( &greenPen );
		dc->MoveTo( startX,rc.top );
		dc->LineTo( endX,rc.top );
		dc->LineTo( endX,rc.bottom );
		dc->LineTo( startX,rc.bottom );
		dc->LineTo( startX,rc.top );
		dc->SelectObject( prevPen );

		// Draw two adjust bars.
		int durationX = TimeToClient(time+duration);
		CPen redPen(PS_SOLID,3,RGB(255,0,0));
		prevPen = dc->SelectObject( &redPen );
		dc->MoveTo( x-2,rc.top );
		dc->LineTo( x-2,rc.bottom );
		dc->MoveTo( durationX+2,rc.top );
		dc->LineTo( durationX+2,rc.bottom );
		dc->SelectObject( prevPen );
	}
}