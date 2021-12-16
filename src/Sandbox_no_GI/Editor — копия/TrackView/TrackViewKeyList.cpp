// TrackViewKeyList.cpp : implementation file
//

#include "stdafx.h"
#include "TrackViewKeyList.h"
#include "Controls\MemDC.h"

#include "TrackViewDialog.h"
#include "AnimationContext.h"

#include "TrackViewUndo.h"

//#include "IMovieSystem.h"

enum ETVMouseMode
{
	MOUSE_MODE_NONE = 0,
	MOUSE_MODE_SELECT = 1,
	MOUSE_MODE_MOVE,
	MOUSE_MODE_CLONE,
	MOUSE_MODE_DRAGTIME
};

#define KEY_TEXT_COLOR RGB(0,0,50)

// CTrackViewKeyList

IMPLEMENT_DYNAMIC(CTrackViewKeyList, CTrackViewKeys)
CTrackViewKeyList::CTrackViewKeyList()
{
	m_leftOffset = 30;

	m_itemWidth = 1000;
	m_itemHeight = 16;
}

CTrackViewKeyList::~CTrackViewKeyList()
{
}

BEGIN_MESSAGE_MAP(CTrackViewKeyList, CTrackViewKeys)
END_MESSAGE_MAP()

// CTrackViewKeyList message handlers

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeyList::DrawTrack( int item,CDC *dc,CRect &rcItem )
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

	CRect rcInnerDraw( rcInner.left-6,rcInner.top,rcInner.right+6,rcInner.bottom );
	ColorB trackColor = track->GetColor();
	CRect rc = rcInnerDraw;
	rc.DeflateRect(0,1,0,0);
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
void CTrackViewKeyList::DrawKeys( IAnimTrack *track,CDC *dc,CRect &rc,Range &timeRange )
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
	// Draw keys.
	for (int i = 0; i < numKeys; i++)
	{
		float time = track->GetKeyTime(i);
		
		// Get info about that key.
		const char *description = NULL;
		float duration = 0;
		track->GetKeyInfo(i,description,duration);

		int x = TimeToClient(time);
		int xlast = x;
		if (duration > 0)
			xlast = TimeToClient(time+duration);
		if (xlast + 10 < rc.left)
			continue;
		if (x-10 > rc.right)
			continue;
		
		if (duration > 0)
		{
			// Draw key duration.
			float endt = min(time+duration,m_timeRange.end);
			int x1 = TimeToClient(endt);
			if (x1 < 0)
			{
				if (x > 0)
					x1 = rc.right;
			}
			CBrush *prevBrush = dc->SelectObject( &m_visibilityBrush );
			//dc->Rectangle(  );
			XTPPaintManager()->GradientFill( dc,CRect(x,rc.top+3,x1+1,rc.bottom-3),RGB(120,120,255),RGB(250,250,250),FALSE );

			dc->SelectObject( &prevBrush );
			dc->MoveTo(x1,rc.top);
			dc->LineTo(x1,rc.bottom);
		}

		if (description)
		{
			strcpy_s(keydesc,"{");
			strcat_s(keydesc,description);
			strcat_s(keydesc,"}");
			// Draw key description text.
			// Find next key.
			int x1 = x + 200;
			//if (x < 0)
				//x1 = rc.right;
			int nextKey = track->NextKeyByTime(i);
			if (nextKey > 0)
			{
				x1 = TimeToClient( track->GetKeyTime(nextKey) ) - 10;
			}
			CRect textRect(x+10,rc.top,x1,rc.bottom );
			//textRect &= rc;
			dc->DrawText( keydesc,strlen(keydesc),textRect,DT_LEFT|DT_END_ELLIPSIS|DT_VCENTER|DT_SINGLELINE  );
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

			/*
			CBrush keyBrush;
			if (track->IsKeySelected(i))
				keyBrush.CreateSolidBrush( RGB(255,255,255) );
			else
				keyBrush.CreateSolidBrush( RGB(0,250,0) );

			CBrush *prevBrush = dc->SelectObject( &keyBrush );
			CRect rcKey(x,rc.top+1,x+8,rc.bottom);
			dc->Rectangle( rcKey );
			dc->SelectObject( &prevBrush );
			*/
		}

		prevKeyPixel = x;
		time0 = time;
	}
	dc->SelectObject( prevFont );
}

//////////////////////////////////////////////////////////////////////////
int CTrackViewKeyList::FirstKeyFromPoint( CPoint point ) const
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
int CTrackViewKeyList::NumKeysFromPoint( CPoint point ) const
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
void CTrackViewKeyList::SelectKeys( const CRect &rc )
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
void CTrackViewKeyList::DrawNodeItem( IAnimNode *pAnimNode,CDC *dc,CRect &rcItem )
{
	CFont *prevFont = dc->SelectObject( m_descriptionFont );

	dc->SetTextColor(KEY_TEXT_COLOR);
	dc->SetBkMode(TRANSPARENT);

	CRect textRect = rcItem;
	textRect.left += 4;
	textRect.right -= 4;

	const char *sAnimNodeName = pAnimNode->GetName();
	dc->DrawText( sAnimNodeName,strlen(sAnimNodeName),textRect,DT_LEFT|DT_END_ELLIPSIS|DT_VCENTER|DT_SINGLELINE  );

	dc->SelectObject( prevFont );
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewKeyList::DrawGoToTrackArrow( IAnimTrack *track,CDC *dc,CRect &rc)
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
