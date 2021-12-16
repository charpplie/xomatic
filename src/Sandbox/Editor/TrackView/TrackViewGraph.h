////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   trackviewgraph.h
//  Version:     v1.00
//  Created:     23/8/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __trackviewgraph_h__
#define __trackviewgraph_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "TrackViewKeys.h"
#include "TrackViewSplineCtrl.h"
#include "Controls\TimelineCtrl.h"
#include "ToolbarDialog.h"

/** CTrackViewGraph dialog.
		Placed at the same position as tracks dialog, and display spline graphs of track.
*/
class CTrackViewGraph : public CToolbarDialog, public IEditorNotifyListener
{

public:
	CTrackViewGraph();
	virtual ~CTrackViewGraph();
	
	enum { IDD = IDD_DB_ENTITY };

	void SetSequence( IAnimSequence *pSequence );
	void SetCurrTime( float time );

protected:
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
	virtual void OnOK() {};
	virtual void OnCancel() {};

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSplineChange( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnSplineCmd( UINT cmd );
	afx_msg void OnSplineCmdUpdateUI( CCmdUI *pCmdUI );
	afx_msg void OnTimelineChange( NMHDR *pNMHDR, LRESULT *pResult );

	//////////////////////////////////////////////////////////////////////////
	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );
	//////////////////////////////////////////////////////////////////////////

	void AddSpline();

private:
	_smart_ptr<IAnimSequence> m_pSequence;

	CXTPToolBar m_wndToolBar;
	CTrackViewSplineCtrl m_wndSpline;
	CTimelineCtrl m_timelineCtrl;

	bool m_bIgnoreSelfEvents;
};

#endif // __trackviewgraph_h__