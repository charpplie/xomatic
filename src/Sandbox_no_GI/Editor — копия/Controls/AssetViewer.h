/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id: AssetViewer.h  ,v 1.1 2008/08/19 12:54:41 PauloZaffari Exp wwwrun $
	$DateTime$
	Description:
	This file declares a control which objective is to display 
	multiple Assets allowing selection and preview of such things.
	It also must handle scrolling and changes in the Asset 
	cell display size.
	-------------------------------------------------------------------------
	History:
	- 19:08:2008   12:54 : Created by Paulo Zaffari
	- 11:03:2010   17:34 : Nicusor Nedelcu - refactored

*************************************************************************/
#ifndef __AssetViewer_H__
#define __AssetViewer_H__
#pragma once

#include "ScrollableWindow.h"
#include "CryThread.h"
#include "MultiThread_Containers.h"
#include "Include/IAssetViewer.h"
#include "Include/IAssetDisplayDatabase.h"
#include "Util/GdiUtil.h"

struct IAssetDisplay;

class CAssetViewer : public CScrollableWindow, public CryThread<CAssetViewer>, public IAssetViewer, public IEditorNotifyListener
{
	public:

		CAssetViewer();
		~CAssetViewer();
		void FreeData();
		bool Create( int nLeft, int nTop, UINT nWidth, UINT nHeight, CWnd* poParentWindow, UINT nId = 0, DWORD dwStyle = WS_CHILD | WS_VISIBLE );

		void Run();
		void RestartThread();
		void WaitThreadAndFreeData();

		// thread data protection
		void LockAll();
		void UnlockAll();
		void LockAssetItemsArray();
		void UnlockAssetItemsArray();
		void LockAssetDatabasesArray();
		void UnlockAssetDatabasesArray();

		void SetDatabases( TAssetDatabases& rcDatabases );
		TAssetDatabases& GetDatabases();
		void RefreshDatabases();
		void ClearDatabases();
		void SortAssets( const char* pFieldname, bool bDescending = false );
		void ApplyFilters( const IAssetDisplayDatabase::TAssetFieldFiltersMap& rFieldFilters );
		void ClearFilters();
		void EnsureAssetVisible( UINT aIndex );
		void EnsureAssetVisible( IAssetDisplay* pAsset );
		void SetAssetThumbSize( const unsigned int nThumbSize );
		UINT GetAssetThumbSize();
		void GetSelectedItems( TAssetItems& rSelectedItems );
		int	GetFirstSelectedItemIndex();
		void DeselectAll();
		void SetDoubleClickCallback( TDoubleClickCallback fctDoubleClickCallback );
		HWND GetRenderWindow();
		void PushToThumbsCacheQueue( IAssetDisplay* pAsset );
		TAssetItems& GetAssetItems();
		bool IsAssetVisibleInView( IAssetDisplay* pAsset );
		void UpdateSmoothPanning();
		void CachePendingNonThreadedAssets();
		static string GetAssetFieldDisplayValue( IAssetDisplay* pAsset, const char* pFieldname );
		bool	IsAnyOperationInProgress();
		void	GetCurrentOperationTextAndProgress( CString& rText, int& rProgress /*0-100*/ );
		void	SetStatusDisplay( IAssetViewerStatusDisplay* pObserver );
		IAssetViewerStatusDisplay*	GetStatusDisplay();

		// from IEditorNotifyListener
		// Here we receive the idle updates, from which we call the rendering method
		void OnEditorNotifyEvent( EEditorNotifyEvent event );

		//! from IUnknown
		HRESULT STDMETHODCALLTYPE	QueryInterface( const IID &riid, void **ppvObj );
		ULONG STDMETHODCALLTYPE		AddRef();
		ULONG STDMETHODCALLTYPE		Release();

	protected:

		DECLARE_MESSAGE_MAP()
		afx_msg void OnSize(UINT nType,int cx,int cy);
		afx_msg void OnPaint();
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg void OnLButtonDown(UINT nFlags,CPoint point);
		afx_msg void OnMouseMove(UINT nFlags,CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags,CPoint point);
		afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg BOOL OnMouseWheel(UINT nFlags,short zDelta,CPoint pt);
		afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnTimer(UINT_PTR nIDEvent);
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnDestroy();

		//! calculates the layout of all visible assets
		void RecalculateLayout();
		//! calculates the currently visible elements.
		void UpdateVisibility();
		//! update scroll bar range and vertical offset
		void UpdateScrollBar();
		//! draw all ( asset thumbs, tooltip, etc. )
		void Draw();
		//! draw one asset
		void DrawAsset( IAssetDisplay* poItem, CRect& roDrawRect );
		//! check for a virtual key press
		bool CheckVirtualKey( int virtualKey );
		void CheckClickedThumb( UINT nFlags, CPoint point, bool bShowInteractiveRenderWnd = false );
		
		static bool AssetViewerSortAssetItems( IAssetDisplay* pA, IAssetDisplay* pB );

		static SAssetField*	m_spSortField;
		static bool					m_sbSortDescending;

		ULONG								m_ref;
		TAssetItems					m_cAssetItems;
		TAssetItems					m_cAssetDrawingCache;
		TAssetItems					m_cPendingCacheAssets;
		TAssetItems					m_cSelectedAssets;
		TAssetDatabases			m_cAssetDatabases;
		CryMutex						m_oAssetItemsLock, m_oAssetDatabasesLock, m_oOperationLock;
		UINT								m_nTotalAssets;
		std::deque<IAssetDisplay*>		m_cThumbsCache;
		static std::map<string,BYTE*> m_assetInfoPersistentCache;
		string							m_strCurrentSortFieldname;
		CPoint							m_oStartDraggingPoint;
		bool								m_bMouseLeftButtonDown;
		bool								m_bMouseRightButtonDown;
		bool								m_bMouseMiddleButtonDown;
		bool								m_bDragging;
		CRect								m_oSelectionRect;
		UINT								m_nItemHorizontalMargin;
		UINT								m_nItemVerticalMargin;
		UINT								m_nAssetThumbSize;
		int									m_nYOffset;
		float								m_smoothPanLeftAmount;
		int									m_smoothPanLastDelta;
		//! this client rect changes only on control resize
		CRect								m_oClientRect;
		UINT								m_nIdealClientWidth;
		UINT								m_nIdealClientHeight;
		IAssetDisplay				*m_poEnsureVisible;
		IAssetDisplay				*m_poClickedAsset;
		IAssetDisplay				*m_poHoveredAsset;
		TDoubleClickCallback m_pfnDoubleClickCallback;
		IRenderer						*m_piRenderer;
		CWnd								m_wndAssetThumbRender;
		CGdiCanvas					m_canvas;
		CAlphaBitmap				m_thumbShadowBmp, m_thumbFlagBmp, m_thumbLoadingBmp, m_backgroundBmp, m_thumbInvalidAssetBmp;
		CBrush							m_backBrush;
		CFont								m_fontLabel, m_fontInfo, m_fontInfoTitle;
		CPoint							m_lastPanDragPt, m_lastRightClickPt;
		UINT								m_nItemBorderSize;
		unsigned __int64		m_nTotalDatabaseSize;
		//! used in the worker thread, to advance in the asset current databases
		std::vector<IAssetDisplayDatabase::TFilenameAssetMap::iterator>		m_nextFieldsInfoCacheAssetIter;
		volatile bool				m_bOperationInProgress, m_bMustRedraw;
		CString							m_operationText;
		UINT								m_operationProgressPercent;
		UINT								m_operationTotalCount, m_operationCurrentCount;
		UINT								m_smoothPanningLastTickCount;
		IAssetViewerStatusDisplay* m_pStatusDisplayObserver;
		bool								m_bWarningBlinkToggler;
		CRect								m_thumbFocusAnimationRect;
		float								m_thumbFocusAnimationTime;
};

#endif // AssetViewer_h__
