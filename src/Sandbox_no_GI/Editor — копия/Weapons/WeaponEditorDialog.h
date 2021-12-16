////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2008.
// -------------------------------------------------------------------------
//  File name:   WeaponEditorDialog.h
//  Version:     v1.00
//  Created:     15-02-2008 by Evgeny Strokov
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __WeeaponEditorDialog_h__
#define __WeeaponEditorDialog_h__

#include "ToolbarDialog.h"
#include "WeaponEditorPreviewDialog.h"
#include "WeaponEditorSettingsDialog.h"
#include "IEditor.h"

struct IItem;
//////////////////////////////////////////////////////////////////////////
//
// Main Dialog for the Weapon Editor.
//
//////////////////////////////////////////////////////////////////////////
class CWeaponEditorDialog : public CXTPFrameWnd, public IEditorNotifyListener
{
	DECLARE_DYNCREATE(CWeaponEditorDialog)
public:
	static void RegisterViewClass();

	CWeaponEditorDialog();
	~CWeaponEditorDialog();

	BOOL Create(DWORD dwStyle,const RECT& rect,CWnd* pParentWnd);

	CWeaponEditorPreviewDialog& GetPreviewDialog() { return m_panePreview; }
	const char* GetWeaponName() const;
	IItem* GetWeapon() const { return m_pWeapon; }

	void LoadFile(const char* filepath);

protected:
	DECLARE_MESSAGE_MAP()  
	virtual void OnOK() {};
	virtual void OnCancel() {};

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();  
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnDestroy();
	afx_msg void OnClose();

	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
	afx_msg void OnReplayOpen();
	afx_msg void OnReplayPlay();

	afx_msg void OnMoveMode();
	afx_msg void OnMoveModeUpdateUI(CCmdUI *pCmdUI);
	afx_msg void OnRotateMode();
	afx_msg void OnRotateModeUpdateUI(CCmdUI *pCmdUI);

	afx_msg LRESULT OnDockingPaneNotify(WPARAM wParam, LPARAM lParam); 

	CXTPDockingPaneManager* GetDockingPaneManager() { return &m_paneManager; }

	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );

private:
	void CleanUpEntity();

	// dialog stuff  
	CXTPDockingPaneManager m_paneManager;
	CWeaponEditorPreviewDialog m_panePreview;
	CWeaponEditorSettingsDialog m_paneSettings;

	CDlgToolBar m_toolbar;

	//CXTPTaskPanel m_taskPanel;
	//CXTPDockingPane* m_pTaskDockPane; 

	IItem *m_pWeapon;
};

#endif __WeeaponEditorDialog_h__