/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:  This is the header file for the module LNMEditorDialog. 
The LNMEditorDialog is a framewindow containing all the other windows 
related to useful shortcuts about LNM. It also serves as a hub and manager
for this windows.
-------------------------------------------------------------------------
History:
- 18:01:2010   10:51 : Created by Ricardo Pillosu

*************************************************************************/

#ifndef LNMEditorDialog_h__
#define LNMEditorDialog_h__

#pragma once

#include "..\Dialogs\BaseFrameWnd.h"

class CLNMEditorDialog:public CBaseFrameWnd 
{
	//////////////////////////////////////////////////////////////////////////
	// Typed & typedefs
	public:
		enum EControlIDs
		{
				IDC_OPERATIONS_DOCKING_PANE	= 1024,
				IDC_PROPERTIES_DOCKING_PANE	= 2048,
				IDC_BROWSER_DOCKING_PANE		= 4096,
		};

	protected:
	private:
	//////////////////////////////////////////////////////////////////////////
	class CLNMEditorClass : public TRefCountBase<IViewPaneClass>
	{
		//////////////////////////////////////////////////////////////////////////
		// IClassDesc
		//////////////////////////////////////////////////////////////////////////
		virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
		virtual REFGUID ClassID()
		{
			// {9A69393D-4973-4ca9-A4D3-9FF49F2F2D37}
			static const GUID guid = { 0x9a69393d, 0x4973, 0x4ca9, { 0xa4, 0xd3, 0x9f, 0xf4, 0x9f, 0x2f, 0x2d, 0x37 } };
			return guid;
		}
		
		virtual const char* ClassName() { return "LNM Editor"; };
		virtual const char* Category() { return "LNM Editor"; };
		//////////////////////////////////////////////////////////////////////////
		virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CLNMEditorDialog); };
		virtual const char* GetPaneTitle() { return _T("LNM Editor"); };
		virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
		virtual CRect GetPaneRect() { return CRect(100,100,1000,800); };
		virtual bool SinglePane() { return false; };
		virtual bool WantIdleUpdate() { return true; };
	};

	DECLARE_DYNCREATE(CLNMEditorDialog)

	//////////////////////////////////////////////////////////////////////////
	// Methods
	public:
		CLNMEditorDialog();
		virtual ~CLNMEditorDialog();

		static void RegisterViewClass();

		//void OnNotifySelectionChange(TDAnimActionPrototypes& rcAnimActionPrototypes);

		// Dialog Data
		enum { IDD = IDD_LNM_EDITOR };
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		// Without this method and its implementation, the docking system simply doesn't work.
		// We should consider making this method a MACRO of creating a base class or component
		// to have this done in a more automatic way.
		// Note that this function makes the specific attachment of the elements to the 
		// Docking Panel, so there is specific code linked to attaching the window we are
		// dealing with to the panel.
		// SPECIFIC: for this particular case, we are attaching to the panel the rollup control.
		// We are using a rollup control due to its automatic scrolling feature. Apart from that
		// we could be placing the panel directly into the GetDockingSite() of the docking panel.
		afx_msg LRESULT OnDockingPaneNotify(WPARAM wParam, LPARAM lParam);	

		DECLARE_MESSAGE_MAP()

	protected:
		BOOL OnInitDialog();

		afx_msg	void OnDestroy();
		afx_msg void OnClose();
	private:
		void SetStatusBarText(const char* pText);

	//////////////////////////////////////////////////////////////////////////
	// Data
	public:
	protected:

	CXTPDockingPane												*m_poDockingPane;
	CStatusBar														m_oWindowStatusBar;
	private:
};


#endif // LNMEditorDialog_h__