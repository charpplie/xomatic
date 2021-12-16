/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: TextureBrowserDialog.h,v 1.0 2008/07/30 17:24:35 PauloZaffari Exp wwwrun $
$DateTime$
Description:  MFC class implementing the texture browser dialog.
-------------------------------------------------------------------------
History:
- 30:7:2008   17:24 : Created by Paulo Zaffari

*************************************************************************/
#ifndef TextureBrowser_h__
#define TextureBrowser_h__

#pragma once

#include "IEditor.h"
#include "Dialogs\BaseFrameWnd.h"
#include "../Include/ITextureViewerStatusDisplay.h"
#include "../Controls/TextureViewer.h"

#include "TextureDatabaseCreator.h"

#include <map>
#include <vector>

class CTextureBrowserPanel;
class CTextureViewer;

// The comment: "/*public CDialog*/" is a helper in case we want to add more stuff using 
// resource editor and MFC.
class CTextureBrowserDialog : /*public CDialog*/ public CBaseFrameWnd ,public ITextureViewerStatusDisplay,public IEditorNotifyListener
{
private:
	//////////////////////////////////////////////////////////////////////////
	class CTextureBrowserViewClass : public TRefCountBase<IViewPaneClass>
	{
		//////////////////////////////////////////////////////////////////////////
		// IClassDesc
		//////////////////////////////////////////////////////////////////////////
		virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
		virtual REFGUID ClassID()
		{
			// {F6DB7988-A8B4-4cb7-ADAB-23B0E32EC2C5}
			static const GUID guid = { 0xf6db7988, 0xa8b4, 0x4cb7, { 0xad, 0xab, 0x23, 0xb0, 0xe3, 0x2e, 0xc2, 0xc5 } };
			return guid;
		}
		virtual const char* ClassName() { return "Texture Browser"; };
		virtual const char* Category() { return "Texture Browser"; };
		//////////////////////////////////////////////////////////////////////////
		virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CTextureBrowserDialog); };
		virtual const char* GetPaneTitle() { return _T("Texture Browser"); };
		virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
		virtual CRect GetPaneRect() { return CRect(100,100,1000,800); };
		virtual bool SinglePane() { return false; };
		virtual bool WantIdleUpdate() { return true; };
	};

	DECLARE_DYNCREATE(CTextureBrowserDialog)
public:
	CTextureBrowserDialog(/*CWnd* pParent = NULL*/);   // standard constructor
	virtual ~CTextureBrowserDialog();

	static void RegisterViewClass();

	// Dialog Data
	enum { IDD = IDD_TEXTURE_BROWSER_DIALOG };

	//////////////////////////////////////////////////////////////////////////
	// From ITextureViewerStatusDisplay.
	//////////////////////////////////////////////////////////////////////////
	void SetTotalFileSize(unsigned long long nTotalFilzeSize);
	void SetCurrentFilename(const char* cszDirectory,const char* cszCurrentFilename);

	void SetTextureType(const char* cszTextureType);
	void SetNumberofMips(const char* szNumberOfMips);
	void SetResolution(const char* szResolution);
	void SetFileSize(const char* szTotalFilzeSize);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEditorNotifyListener
	//////////////////////////////////////////////////////////////////////////
	// We only need to deal with the document events to prevent crashing.
	void OnEditorNotifyEvent( EEditorNotifyEvent event );
	//////////////////////////////////////////////////////////////////////////

	static CTextureBrowserDialog*& GetCurrentInstance();

	void GetSelectedItems(CTextureViewer::TDTextureDatabase& rcpoSelectedItemArray);
	bool SetSelectedItem(const char* szItemFilename);

	void SetTextureBrowserDoubleClickCallback(CTextureViewer::TDDoubleClickCallback	pfnDoubleClickCallback);


	void SetAssetInfo(const XmlNodeRef& xmlAssetInfo){};
	void SetAssetFilterInfo(const XmlNodeRef& xmlAssetFilterInfo){};
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
protected:
	BOOL OnInitDialog();

	void CreateTextureViewer();

	afx_msg	void OnDestroy();
	afx_msg void OnClose();
	
	afx_msg LRESULT OnDockingPaneNotify(WPARAM wParam, LPARAM lParam);	

	BOOL PreTranslateMessage(MSG* pMsg);

	void StandardDoubleClickCallback();

	CTextureBrowserPanel		*m_poTextureBrowserPanel;
	CXTPDockingPane					*m_poDockingPane;

	CXTCaption							m_oWindowCaption;
	CFont										m_fontCaption;
	CXTSplitterWnd					m_oViewportContainer;

	CRollupCtrl							m_oRollupControl;
	CXTPStatusBar						m_oWindowStatusBar;

	// As this is a view object, as is required for it to work with CXTSplitterWnd, the framework
	// supposes it will be dynamically allocated and it will try to deallocate it by force...
	// ... for this reason we must allocate it dynamically or we will have a crash.
	CTextureViewer						m_oTextureViewer;

	// Added this variable as a pointer because it was needed as a mean to ensure the correct
	// destruction order. For instance, the correct order is:
	// 1 - Notify the viewer it will be destroyed.
	// 2 - Notify the database updater.
	// 3 - Wait for the updater to finish.
	// 4 - You SHOULD tell detach from the viewer the updater (set it to NULL).
	// 5 - Delete the updater.
	// 6 - Delete the viewer.
	// Also, this has to be done while the window is still visible or the control will freak out...
	// so it has to be called on OnDestroy.
	CTextureDatabaseCreator		*m_poTextureDatabaseCreator;
};

#endif // TextureBrowser_h__
