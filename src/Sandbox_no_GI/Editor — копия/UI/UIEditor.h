////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIEditor.h
//  Version:     v1.00
//  Created:     06/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#ifndef __UIEditor_H__
#define __UIEditor_H__

#include "Dialogs/BaseFrameWnd.h"
#include "UIRollupView.h"
#include <IFlashUI.h>

class CUIViewport;
class CUITool;
class CUIToolButtons;
class CUISettingsPropertiesPanel;
class CUIEmulatePanel;
class CUIEventLogView;

class CUIEditor
	: public CBaseFrameWnd
	, public IEditorNotifyListener
	, public CFileChangeMonitorListener
{
	DECLARE_DYNCREATE( CUIEditor )

public:
	CUIEditor();
	virtual ~CUIEditor();

	CUIViewport* GetViewport() { return m_pViewport; }
	CUIRollupControl* GetElementRollup();

	CUITool* GetMainTool() { return m_pMainTool; }
	void SetMainTool(CUITool* pTool) { m_pMainTool = pTool; }

	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );

	// add direct invoke panel
	void AddEventEmuPanelInvoke( bool bRemoveable = true );
	// add IUIElement function panel
	void AddEventEmuPanelElement( IUIElement* pElement, const SUIEventDesc& evt );
	// add IUIAction event panel
	void AddEventEmuPanelAction( IUIAction* pAction );
	// add IUIEventSystem event panel
	void AddEventEmuPanelEventSystem( IUIEventSystem* pEventSystem, const SUIEventDesc& evt );

	// add IUIElement variable panel
	void AddEventEmuPanelElementVar( IUIElement* pElement, const SUIParameterDesc& desc );
	// add IUIElement array panel
	void AddEventEmuPanelElementArray( IUIElement* pElement, const SUIParameterDesc& desc );
	// add IUIElement movieclip panel
	void AddEventEmuPanelElementMovieClip( IUIElement* pElement, const SUIParameterDesc& desc );

	// adds emualtion panels for all functions for an IUIElement
	void AddEventEmuPanelsForElement( IUIElement* pElement );
	// adds emualtion panels for all events for an IUIEventSystem
	void AddEventEmuPanelsForEventSystem( IUIEventSystem* pEventSystem );

	// remove an emulation panel
	void RemoveEventEmuPanel( CUIEmulatePanel* pPanel );
	// remove all event panels for the UIElement or IUIEventSystem
	void RemoveEventEmuPanelsForGroup( void* pGroup );
	// remove all emulation panels
	void ClearEventEmuPanels( bool force = false );
	
	virtual void OnFileMonitorChange();

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg int OnCreateControl(LPCREATECONTROLSTRUCT lpCreateControl);
	afx_msg void OnToggleFGUpdate();
	afx_msg void OnReloadAll();
	afx_msg void OnResetState();
	afx_msg void OnUpdateResolutions(CCmdUI* pCmdUI);
	afx_msg void OnSelectionResolutionsChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdatePlatform(CCmdUI* pCmdUI);
	afx_msg void OnSelectionPlatformChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClearLog();

	DECLARE_MESSAGE_MAP()

private:
	void CreateToolBars();
	void CreateViewPort();
	void CreateEventLog();
	void CreateRollupBarObjects();
	void CreateRollupBarEmulate();

	void AttachDockingWnd( UINT wndId, CWnd* pWnd, const CString& dockingPaneTitle, XTPDockingPaneDirection direction = xtpPaneDockLeft, CXTPDockingPaneBase* pNeighbour = NULL );

	void OnUILogEvent(const IFlashUI::SUILogEvent& evt);
	IFlashUI::EPlatformUI OnGetCurrentPlattform() { return m_CurrPlattform; }

	void AddEventEmuPanel( CUIEmulatePanel* pPanel, const char* name );

private:
	static CryCriticalSection m_sLock;
	CFileChangeMonitor *m_pFileChangeMonitor;

	CUIViewport* m_pViewport;

	CUIToolButtons* m_pMainToolButtons;
	CUITool* m_pMainTool;

	CUIEventLogView* m_pEventLog;

	CUIRollupView m_rollupObjects;
	CUIRollupView m_rollupEmulate;

	typedef std::vector< CUISettingsPropertiesPanel* > TSettingPanels;
	TSettingPanels m_SettingPanels;

	typedef std::map< CUIEmulatePanel*, int > TEventEmulatePanels;
	TEventEmulatePanels m_EventPanels;

	float m_LastChange;

	IFlashUI::EPlatformUI m_CurrPlattform;
};

#endif //__UIEditor_H__
