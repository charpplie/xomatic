////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   ScrollHelper.h
//  Version:     v1.00
//  Created:     06/08/2009 by Pau Novau
//  Description: Helper class to aid with scrolling CWnd controls.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ScrollHelper__h__
#define __ScrollHelper__h__

class CScrollHelper
{
public:
	enum BarId
	{
		HORIZONTAL,
		VERTICAL
	};

	CScrollHelper();
	~CScrollHelper();

	void SetWnd( CWnd* pWnd );

	void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	void OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	void OnSize( UINT nType, int cx, int cy );

	void SetDesiredClientSize( const CRect& desiredClientSize );
	void SetAutoScrollWindow( bool autoScrollWindow );
	void ResetScroll();
	void SetAllowed( BarId barId, bool allowed );

	int GetScrollAmount( BarId barId ) const;

private:
	void OnClientSizeUpdated();
	void UpdateScrollBars();

	void UpdateScrollBar( BarId barId, int desiredSize, int viewSize );

	void OnScroll( BarId barId, UINT nSBCode, UINT nPos );

	void Update();

private:
	CRect m_desiredClientSize;

	bool m_autoScrollWindow;

	CWnd* m_pWnd;

	bool m_visible[ 2 ];
	int m_totalScroll[ 2 ];
	int m_winBarId[ 2 ];
	bool m_allowed[ 2 ];
};

#endif