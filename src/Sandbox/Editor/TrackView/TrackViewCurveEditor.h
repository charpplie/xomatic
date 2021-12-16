////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   trackviewcurveeditor.h
//  Version:     v1.00
//  Created:     23/8/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "TrackViewDopeSheetBase.h"
#include "TrackViewSplineCtrl.h"
#include "Controls\TimelineCtrl.h"
#include "ToolbarDialog.h"

/** CTrackViewGraph dialog.
		Placed at the same position as tracks dialog, and display spline graphs of track.
*/
class CTrackViewCurveEditor : public CToolbarDialog, public IAnimationContextListener, 
	public IEditorNotifyListener, public ITrackViewSequenceListener
{

public:
	CTrackViewCurveEditor();
	virtual ~CTrackViewCurveEditor();
	
	enum { IDD = IDD_DB_ENTITY };

	void SetEditLock(bool bLock);
	
	void SetFPS(float fps);
	float GetFPS() const;
	void SetTickDisplayMode(ETVTickMode mode);

	CXTPToolBar* GetToolBarCtrl(){ return &m_wndToolBar;};

	CTrackViewSplineCtrl& GetSplineCtrl(){ return m_wndSpline; }
	void ResetSplineCtrlZoomLevel();

	// IAnimationContextListener
	virtual void OnSequenceChanged(CTrackViewSequence *pNewSequence);
	virtual void OnTimeChanged(float newTime);

protected:
	DECLARE_MESSAGE_MAP()

private:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	virtual void OnOK() {};
	virtual void OnCancel() {};

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSplineChange( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnSplineCmd( UINT cmd );
	afx_msg void OnSplineCmdUpdateUI( CCmdUI *pCmdUI );
	afx_msg void OnTimelineChange( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnHorizonSliderChange( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnVerticalSliderChange( NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void OnSplineScrollZoom( NMHDR *pNMHDR, LRESULT *pResult );
	
	// IEditorNotifyListener
	virtual void OnEditorNotifyEvent(EEditorNotifyEvent event) override;	

	//ITrackViewSequenceListener
	virtual void OnKeysChanged(CTrackViewSequence *pSequence) override;
	virtual void OnKeySelectionChanged(CTrackViewSequence *pSequence) override;
	virtual void OnNodeChanged(CTrackViewNode *pNode, ENodeChangeType type) override;
	virtual void OnNodeSelectionChanged(CTrackViewSequence *pSequence) override;
	virtual void OnSequenceSettingsChanged(CTrackViewSequence *pSequence) override;

	void UpdateSplines();
	void UpdateTimeRange(CTrackViewSequence *pSequence);

	void AddSpline(CTrackViewTrack *pTrack);

	void ResetSliderRange();

	CXTPToolBar m_wndToolBar;
	CTrackViewSplineCtrl m_wndSpline;
	CTimelineCtrl m_timelineCtrl;

	CSliderCtrl m_horizonSlider;
	CSliderCtrl	m_verticlSlider;

	bool m_bIgnoreSelfEvents;

	bool m_bLevelClosing;
};
