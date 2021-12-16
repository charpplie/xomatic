////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   TelemetryViewDialog.h
//  Version:     v1.00
//  Created:     13/05/11 by Steve Humphreys
//  Description: Dialog class for the whole telemetry viewer
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __TelemetryViewDialog_h__
#define __TelemetryViewDialog_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "ToolbarDialog.h"
#include "TelemetryViewSplitter.h"
#include "TelemetryViewTimelines.h"
#include "TelemetryEventsView.h"

#include "Telemetry\TelemetryRepository.h"

#include "Dialogs\BaseFrameWnd.h"

namespace Telemetry
{
	struct IDataSource;
}

// CTelemetryViewDialog dialog

class CTelemetryViewDialog : public CBaseFrameWnd, public IEditorNotifyListener
{
	DECLARE_DYNCREATE(CTelemetryViewDialog)

public:

	CTelemetryViewDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTelemetryViewDialog();

// Dialog Data
	enum { IDD = IDD_TRACKVIEWDIALOG };

	static void RegisterViewClass();

	static CTelemetryViewDialog * GetCurrentInstance()	{ return s_pTelemetryViewDialog; }

	void InvalidateTrackList();

	void Update();

	void SelectNode(const char * sName);

	Telemetry::CTelemetryRepository* GetRepository() { return &m_repository; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();

	DECLARE_MESSAGE_MAP()

	CTelemetryViewSplitter m_wndSplitter;
	CTelemetryViewTimelinesCtrl	m_timelinesCtrl;
	CTelemetryEventsView	m_eventsCtrl;

	CDlgToolBar m_cDlgToolBar;

	static CTelemetryViewDialog *s_pTelemetryViewDialog;

	bool m_bPlay;
	bool m_bPause;

	float m_fLastTime;

	bool m_lazyInitDone;

	// todo: proper place for these?
	Telemetry::IDataSource* m_pDataSource;
	Telemetry::CTelemetryRepository m_repository;

	void InitToolbar();

protected:
	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );

	virtual void OnOK() {};
	virtual void OnCancel() {};

	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void SaveLayouts();
	void ReadLayouts();
	void SaveMiscSettings() const;
	void ReadMiscSettings();
	
	void SetCursorPosText( float fTime );

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnDestroy();

	afx_msg void OnOpenLegacyXmlFile();
	afx_msg void OnOpenDataFile();

	afx_msg void OnGoToStart();
	afx_msg void OnGoToEnd();
	afx_msg void OnPlay();
	afx_msg void OnStop();
	afx_msg void OnPause();
	afx_msg void OnLoop();

	afx_msg void OnCustomize();
	afx_msg void OnExportShortcuts();
	afx_msg void OnImportShortcuts();

	afx_msg void OnToolbarDropDown(NMHDR *pnhdr, LRESULT *plr);

	afx_msg void OnUpdatePlay( CCmdUI* pCmdUI );
	afx_msg void OnUpdatePause( CCmdUI* pCmdUI );
	afx_msg void OnUpdateLoop( CCmdUI* pCmdUI );
	afx_msg void OnUpdateViewTickInSeconds( CCmdUI* pCmdUI );
	afx_msg void OnUpdateViewTickInFrames( CCmdUI* pCmdUI );

	afx_msg void OnModeDopeSheet();

	afx_msg void OnViewTickInSeconds();
	afx_msg void OnViewTickInFrames();
};

#endif // __TelemetryViewDialog_h__