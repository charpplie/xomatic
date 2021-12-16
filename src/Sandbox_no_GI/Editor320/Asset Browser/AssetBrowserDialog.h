/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: AssetBrowserDialog.h,v 1.0 2008/07/30 17:24:35 PauloZaffari Exp wwwrun $
$DateTime$
Description:  MFC class implementing the Asset browser dialog.
-------------------------------------------------------------------------
History:
- 30:7:2008   17:24 : Created by Paulo Zaffari
- 12/03/2010  12:48 : Nicusor Nedelcu - refactored

*************************************************************************/
#ifndef __AssetBrowser_H__
#define __AssetBrowser_H__
#pragma once

#include "IEditor.h"
#include "Dialogs\BaseFrameWnd.h"
#include "../Controls/AssetViewer.h"
#include "Visual Budget System/VisualBudgetSystemSettings.h"
#include <map>
#include <vector>
#include "Asset Browser/AssetBrowserSearchDlg.h"
#include "Controls/DynamicRibbonBar.h"

struct	IAssetDisplayDatabase;

//! This class handles asset browser list view clicks/sort on list header
class CAssetListReportHeader : public CXTPReportHeader
{
	public:

		CAssetListReportHeader(CAssetBrowserDialog* pAssetBrowserDlg, CXTPReportControl* pCtrl, CXTPReportColumns* pCols );
		void OnLButtonUp(UINT nFlags, CPoint ptClick);

		CAssetBrowserDialog* m_pAssetBrowserDlg;
};

//! This class is the actual asset browser list view
class CAssetBrowserReportControl : public CXTPReportControl
{
	public:

		CAssetBrowserReportControl();
		void OnSelectionChanged();

		void ClearVisibleItems();
		void AddVisibleItem(IAssetDisplay *pAsset);

		CAssetBrowserDialog* m_pAssetBrowserDlg;

	protected:

		DECLARE_MESSAGE_MAP()
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void OnTimer(UINT_PTR nIDEvent);

		bool CacheFieldsInfoForVisibleAsset();

		IAssetViewer::TAssetItems m_cVisibleItems;
};

// The comment: "/*public CDialog*/" is a helper in case we want to add more stuff using 
// resource editor and MFC.
class CAssetBrowserDialog : /*public CDialog*/ public CBaseFrameWnd ,/*public IAssetViewerStatusDisplay,*/public IEditorNotifyListener, public IAssetViewerStatusDisplay
{
	private:

		//! This class is used as a virtual record handler, this will be set into the list view and it will provide data for the list
		class CAssetListVirtualRecord : public CXTPReportRecord
		{
			public:
				
				CAssetListVirtualRecord( CAssetBrowserDialog* pDlg, std::vector<IAssetDisplay*>* pcAssets = NULL );
				void GetItemMetrics( XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics );

				//! current assets in the viewer
				std::vector<IAssetDisplay*>*	m_pAssets;
				//! recent assets which will ba cached/uncached
				std::deque<IAssetDisplay*>		m_recentAssetPool;
				//! the asset browser dialog
				CAssetBrowserDialog*					m_pAssetBrowserDlg;
				//! A saved top row index to find out if the list view has been scrolled
				int m_prevTopRowIndex;
		};	

		//! This is the class which will expose the asset browser as a view tool
		class CAssetBrowserViewClass : public TRefCountBase<IViewPaneClass>
		{
			//////////////////////////////////////////////////////////////////////////
			// IClassDesc
			//////////////////////////////////////////////////////////////////////////
			virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
			virtual REFGUID ClassID()
			{
				// {48903B8B-7269-4ffa-80EF-46E9E531CE85}
				static const GUID guid = { 0x48903b8b, 0x7269, 0x4ffa, { 0x80, 0xef, 0x46, 0xe9, 0xe5, 0x31, 0xce, 0x85 } };
				return guid;
			}

			virtual const char*				ClassName() { return "Asset Browser"; };
			virtual const char*				Category() { return "Asset Browser"; };
			virtual CRuntimeClass*		GetRuntimeClass() { return RUNTIME_CLASS( CAssetBrowserDialog ); };
			virtual const char*				GetPaneTitle() { return _T("Asset Browser"); };
			virtual EDockingDirection	GetDockingDirection() { return DOCK_FLOAT; };
			virtual CRect							GetPaneRect() { return CRect( 100, 100, 1000, 800 ); };
			virtual bool							SinglePane() { return false; };
			virtual bool							WantIdleUpdate() { return true; };
		};

		DECLARE_DYNCREATE( CAssetBrowserDialog )

	public:

		friend class CAssetBrowserSearchDlg;

		CAssetBrowserDialog( /*CWnd* pParent = NULL*/ );   // standard constructor
		virtual ~CAssetBrowserDialog();
		static void RegisterViewClass();

		// Dialog Data
		enum { IDD = IDD_TEXTURE_BROWSER_DIALOG };

		//////////////////////////////////////////////////////////////////////////
		// IEditorNotifyListener
		//////////////////////////////////////////////////////////////////////////
		void OnEditorNotifyEvent( EEditorNotifyEvent event );

		static const CAssetBrowserDialog*	GetCurrentInstance();
		void 															GetSelectedItems( CAssetViewer::TAssetItems& rcpoSelectedItemArray );
		void															SetAssetBrowserDoubleClickCallback( CAssetViewer::TDoubleClickCallback pfnDoubleClickCallback );
		void															CreateRibbonBar();
		void															UpdateAssetDatabaseForVisualBudgetSystem( const std::vector<AABB>& AABBs );

	protected:

		//////////////////////////////////////////////////////////////////////////
		// IAssetViewerStatusDisplay
		//////////////////////////////////////////////////////////////////////////

		void OnChangeStatusBarInfo( UINT nSelectedItems, UINT nVisibleItems, UINT nTotalItems );
		void OnSelectionChanged();

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		DECLARE_MESSAGE_MAP()

	public:

		BOOL					OnInitDialog();
		void					CreateAssetViewer();
		void					CreateDatabases();
		void					FreeDatabases();
		void					ShowDatabase( const char* pName );
		void					SortAssets( const char* pFieldname, bool bDescending = false );
		void					ApplyFilters( IAssetDisplayDatabase::TAssetFieldFiltersMap& rFilterFields );
		void					ClearFilters();
		void					ResetAssetListView();
		void					DefaultDoubleClickCallback();

		BOOL					PreTranslateMessage(MSG* pMsg);
		afx_msg	void	OnDestroy();
		afx_msg void	OnClose();
		afx_msg void	OnTimer(UINT_PTR nIDEvent);

		CXTPDockingPane											*m_poDockingPane;
		CXTCaption													m_oThumbsCaption;
		CFont																m_fontCaption;
		CStatusBar													m_oWindowStatusBar;
		CDynamicRibbonBar										*m_poRibbonBar;
		CXTSplitterWnd											m_oThumbsSplitter;
		CAssetBrowserReportControl					m_oAssetListView;
		CAssetBrowserSearchDlg							m_searchDlg;
		CXTPDockingPane											*m_pDockPaneSearch;
		CAssetListReportHeader*							m_pReportHeader;
		CAssetViewer												m_oAssetViewer;
		CVisualBudgetSystemSettings	m_oVisualBudgetSystem;
		IAssetDisplayDatabase								*m_poCurrentAssetDatabase;
		CAssetViewer::TAssetDatabases				m_cAssetDatabases;
		static CAssetBrowserDialog*					s_poCurrentInstance;
		CAssetListVirtualRecord*						m_pAssetVirtualRecord;
		IAssetDisplayDatabase::TAssetFields m_commonFields;
};

#endif // __AssetBrowser_H__
