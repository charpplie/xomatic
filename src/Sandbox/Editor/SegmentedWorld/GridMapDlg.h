////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   GridMapDlg.h
//  Version:     v1.00
//  Created:     19/10/2009 by Oleg.
//  Compilers:   Visual Studio.NET
//  Description: Utility to work with world map
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __GRIDMAPDLG_H__
#define __GRIDMAPDLG_H__

#pragma once

#include "SegmentedWorldManager.h"
#include "SegmentedWorldMiniMapUpdater.h"
#include "Dialogs\BaseFrameWnd.h"

class CGridMapDlg
	: public CBaseFrameWnd
	, public IEditorNotifyListener
	, public ISWMiniMapUpdaterCallback
	, public ISWMiniMapLockStatusUpdaterCallback
	, public ISWGDLockStatusUpdaterCallback
{
	DECLARE_DYNCREATE(CGridMapDlg)

public:
	enum { IDD = IDD_GRID_MAP };

	static CryCriticalSection s_cs;

	void OnEditorNotifyEvent(EEditorNotifyEvent event);

protected:
	class CGridMapRenderWnd *m_pViewport;
	class CSegmentSelectTool *m_pSegmentTool;

	CButton m_btnSWMode;

	CXTPStatusBar m_statusBar;
	CRollupCtrl m_rollupCtrl;

	CString m_strWorldName;
	CString m_strMergeWorldName;

	class CWorldDataStatusPanel *m_pWDStatusPanel;
	class CSegmentDataStatusPanel *m_pSDStatusPanel;
	class CSegmentSelectionPanel *m_pSelectionPanel;

	Vec2 m_tagLocations[12];

public:
	static void RegisterViewClass();

	CGridMapDlg();
	virtual ~CGridMapDlg();

	virtual void OnCancel() {}
	virtual void OnOK() {}

	void OnLevelChanged();
	bool IsSelectMode() const;
	bool IsOpenForEditMode() const;
	bool IsSegmentSelected(CPoint const& pt ) const;
	void SendImage(int nSegmentID, CImageEx* pImage);
	void SetStatus(std::map<int, DWORD> &stateMap, std::map<int,TSegmentState> &mapSegStates);
	void SetSegInfoUpdateSuspended(bool bSuspend = true);
	void SetGDStatus(int nWDBType, int nWDBState, int nLockedBy, const char* pchLockedBy);
	const char* GetWorldName() { return m_strWorldName; }
	void SetWorld(const char* pchWorldName);
	void GetSelectedGDs(std::vector<int> &arrWDBTypes);
	void UnSelectedGDs();
	void OnSWModeChange(int nMode);

protected:
	BOOL OnInitDialog();
	BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual LRESULT OnDockingPaneNotify(WPARAM wParam, LPARAM lParam);

	void OnOpenWorld();
	void OnMoveToSelected();
	void OnMergeLevels();
	void OnDeleteSelected();

	void TagLocation(int index);
	void GotoTagLocation(int index);

	void LoadLocations();
	void SaveLocations();

	afx_msg void OnSize(UINT nType, int cx, int cy);

	afx_msg void OnBnClickedLockSegments();
	afx_msg void OnBnClickedUnlockSegments();
	afx_msg void OnBnClickedResolveSegments();
	afx_msg void OnBnClickedRevertSegments();
	afx_msg void OnBnClickedRollbackSegments();
	afx_msg void OnBnClickedGenerateSegMap();

	afx_msg void OnBnClickedLockGData();
	afx_msg void OnBnClickedCommitGData();
	afx_msg void OnBnClickedReloadGData();
	afx_msg void OnBnClickedRevertGData();
	afx_msg void OnBnClickedSaveGData();

	afx_msg void OnBnClickedAutoLevel();
	afx_msg void OnBnClickedAutoLevelReset();

	afx_msg void OnTagLocation1();
	afx_msg void OnTagLocation2();
	afx_msg void OnTagLocation3();
	afx_msg void OnTagLocation4();
	afx_msg void OnTagLocation5();
	afx_msg void OnTagLocation6();
	afx_msg void OnTagLocation7();
	afx_msg void OnTagLocation8();
	afx_msg void OnTagLocation9();
	afx_msg void OnTagLocation10();
	afx_msg void OnTagLocation11();
	afx_msg void OnTagLocation12();

	afx_msg void OnGotoLocation1();
	afx_msg void OnGotoLocation2();
	afx_msg void OnGotoLocation3();
	afx_msg void OnGotoLocation4();
	afx_msg void OnGotoLocation5();
	afx_msg void OnGotoLocation6();
	afx_msg void OnGotoLocation7();
	afx_msg void OnGotoLocation8();
	afx_msg void OnGotoLocation9();
	afx_msg void OnGotoLocation10();
	afx_msg void OnGotoLocation11();
	afx_msg void OnGotoLocation12();

	DECLARE_MESSAGE_MAP()
};

#endif // __GRIDMAPDLG_H__
