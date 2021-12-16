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
#include "GridMapWindow.h"
#include "Dialogs\BaseFrameWnd.h"

//////////////////////////////////////////////////////////////////////////
// Global Data List Ctrl
//////////////////////////////////////////////////////////////////////////

class CGDListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CGDListCtrl)
public:
	void Init();
	void SetGDState( int nWDBType, int nWDBState, int nLockedBy, const char* pchLockedBy );
protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
	void GetGDStatusText(int nWDBType, string &strText);
protected:
	TGDState m_GDStates[sw::WDB_COUNT];
};

//////////////////////////////////////////////////////////////////////////
// CGridMapDlg dialog
//////////////////////////////////////////////////////////////////////////
class CGridMapDlg : public CXTResizeDialog, public ISWMiniMapUpdaterCallback, public ISWMiniMapLockStatusUpdaterCallback, public ISWGDLockStatusUpdaterCallback
{
	DECLARE_DYNCREATE(CGridMapDlg)

public:
	enum { IDD = IDD_GRID_MAP };

	static CryCriticalSection s_cs;

protected:
	static CGridMapDlg *m_pInstance;

  BOOL m_bSkipTextChange;
  CMapWnd m_MapWnd;
  CEdit m_EditX;
	CButton m_btnSWMode;

	CGDListCtrl m_GDListCtrl;
	CXTPStatusBar m_statusBar;

  CRect m_rcOrgRect;
  INT m_nMapWndDH;
  INT m_nMapWndDW;

  CString m_strWorldName;
  CString m_strMergeWorldName;
  XmlNodeRef m_pnodeLocations;

public:
	static CGridMapDlg *Get();

	CGridMapDlg(CWnd *pParent = NULL);
	virtual ~CGridMapDlg();

	virtual void OnCancel();
	virtual void OnOK();
	virtual void PostNcDestroy();

  // interface
  void UpdatePosition();

  //
  void OnLevelChanged();
  bool IsSelectMode() const;
  bool IsOpenForEditMode() const;
  bool IsSegmentSelected(CPoint const& pt ) const;
  void SetEditArea(int32 x0,int32 y0, int32 w, int32 h);
  void UpdateWorld();
  void SendImage(int nSegmentID, CImage* pImage);
	void SetStatus(std::map<int, DWORD> &stateMap, std::map<int,TSegmentState> &mapSegStates);
	void SetSegInfoUpdateSuspended(bool bSuspend = true);
	void SetGDStatus(int nWDBType, int nWDBState, int nLockedBy, const char* pchLockedBy);
	const char* GetWorldName() { return m_strWorldName; }
	void SetWorld(const char* pchWorldName);
	void GetSelectedGDs(std::vector<int> &arrWDBTypes);
	void UnSelectedGDs();
	void OnSWModeChange(int nMode);
	void OnOpenWorld();

	int GetEditSize();
	void SetEditSize(int size);

	void LoadLocations(const CString &worldName);
	void SaveLocations(const CString &worldName);

protected:
  virtual BOOL OnInitDialog();
  virtual void DoDataExchange(CDataExchange* pDX);
  virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
  BOOL PreTranslateMessage(MSG* pMsg);

  bool MoveToSelected();
  bool MergeToSelected();
  bool OpenWorldInteranl(CString &outWorldName);
  bool UpdateWorldBounds();

  afx_msg void OnDestroy();
  afx_msg void OnEnChangeGridX();
  afx_msg void OnBnClickedGo();
  afx_msg void OnBnClickedLoad();
  afx_msg void OnBnClickedCancel();
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnSize(UINT nType, int cx, int cy);

  afx_msg void OnLocationShow();
  afx_msg void OnLocationUpdate();
  afx_msg void OnLocationAdd();
  afx_msg void OnLocationDel();

  afx_msg void OnBnClickedCheckingMode();

  afx_msg void OnBnClickedLockSegments();
  afx_msg void OnBnClickedUnlockSegments();
  afx_msg void OnBnClickedResolveSegments();
  afx_msg void OnBnClickedUnselectAllSegments();
  afx_msg void OnBnClickedRevertSegments();
  afx_msg void OnBnClickedRollbackSegments();
  afx_msg void OnBnClickedGenerateSegMap();

  afx_msg void OnBnClickedLockGData();
  afx_msg void OnBnClickedCommitGData();
  afx_msg void OnBnClickedReloadGData();
  afx_msg void OnBnClickedRevertGData();
  afx_msg void OnBnClickedSaveGData();

  afx_msg LRESULT OnDoubleClickedInMapWnd(WPARAM wParam, LPARAM lParam);

  DECLARE_MESSAGE_MAP()
protected:
	CToolTipCtrl m_ToolTip;
	
public:
	afx_msg void OnBnClickedAutoLevel();
	afx_msg void OnBnClickedAutoLevelReset();
	afx_msg void OnBnClickedBtnSwMode();
	afx_msg void OnBnClickedDelete();
	afx_msg void OnMergeLevels();

	friend class CMapWnd;
	friend class CGridMapEditor;
};

class SANDBOX_API CGridMapEditor : public CBaseFrameWnd
{
	DECLARE_DYNCREATE(CGridMapEditor)

public:
	CGridMapEditor();
	~CGridMapEditor();

	virtual LRESULT OnDockingPaneNotify(WPARAM wParam, LPARAM lParam);

protected:
	virtual BOOL OnInitDialog();

	void OnOpenWorld();
	void OnMoveToSelected();
	void OnGenerateMap();
	void OnDelSelected();
	void OnMergeSelected();
	void OnAutoColor();
	void OnResetColor();

	void OnSetMapSize(UINT id);
	void OnUpdateMapSizeUI(CCmdUI *pCmdUI);

	DECLARE_MESSAGE_MAP()

	CGridMapDlg *m_pGridMapDlg;
};

#endif // __GRIDMAPDLG_H__
