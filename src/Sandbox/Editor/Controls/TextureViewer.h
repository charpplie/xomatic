/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: TextureViewer.h  ,v 1.1 2008/08/19 12:54:41 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This file declares a control which objective is to display 
							multiple textures allowing selection and preview of such
							things.
							It also must handle scrolling and changes in the texture 
							cell display size.
-------------------------------------------------------------------------
History:
- 19:08:2008   12:54 : Created by Paulo Zaffari
- 03:09:2009   17:50 : Interface improvements by Francesco Roccucci	

*************************************************************************/
#ifndef TextureViewer_h__
#define TextureViewer_h__

#pragma once

#include "ScrollableWindow.h"
#include "CryThread.h"

class		CTextureDatabaseItem;
class		Gdiplus::Color;

struct  ITextureDatabaseUpdater;
struct  ITextureViewerStatusDisplay;

#define MINIMUNCELLSIZE								32
#define MAXIMUMCELLSIZEMULTIPLIER		 992

class CTextureViewer:public CScrollableWindow,public CryThread<CTextureViewer>
{
	public:
		typedef std::vector<CTextureDatabaseItem*> TDTextureDatabase;
		typedef Functor0													 TDDoubleClickCallback;
	private:
		
		typedef std::vector<int>									 TDTIndexChangeList;
	public:
		CTextureViewer();
		~CTextureViewer();


		bool Create(int nLeft,int nTop,unsigned int nWidth,unsigned int nHeight,CWnd* poParentWindow,int nId=-1,DWORD dwStyle=WS_CHILD|WS_VISIBLE);

		bool AddTextureDatabaseItem(CTextureDatabaseItem* poNewItem);

		// Parameter nCellSize should vary between 0 an 100, as it's a percentage (currently, feel free
		// to change it if you need).
		void SetCellSize(const unsigned int nCellSize);

		void FinishedDatabaseUpdate();

		void Run();

		bool SetDatabaseUpdater(ITextureDatabaseUpdater* piDatabaseUpdater);
		void SetStatusDisplay(ITextureViewerStatusDisplay*	piStatusDisplay);

		void NotifyShutDown();

		void FilterWithMinimum(int nTextureWidthMinimum);
		void FilterWithMaximum(int nTextureWidthMaximum);

		void FilterHeightMinimum(int nTextureHeightMinimum);
		void FilterHeightMaximum(int nTextureHeightMaximum);

		void FilterShowDDSOnly(bool boShowOnlyDDS);
		void FilterShowTIFOnly(bool boShowOnlyTIF);
		void FilterShowUsedInLevelOnly(bool boShowOnlyUsedInLevel);
		void FilterShowIfHasAlphaChannel(bool boShowIfHasAlphaChannel);

		void FilterShowSemanticsDiffuseOnly(bool boShowOnlyDiffuse);
		void FilterShowSemanticsSpecularOnly(bool boShowOnlySpecular);
		void FilterShowSemanticsBumpOnly(bool boShowOnlyUsedInBump);
		void FilterShowSemanticsCubemapOnly(bool boShowOnlyCubemap);

		void FilterByName(const string& strNameFilter);

		void GetSelectedItems(TDTextureDatabase& rcpoSelectedItemArray);
		bool SetSelectedItem(const char* szFilename,bool& rboIsVisible);

		void ResumeThread();

		void RestartThread();

		//
		void ShowAlphaChannel(bool boShowAlphaChannel);

		// It's here where all dynamically allocated memory is freed.
		void FinishDatabase();

		void SetDoubleClickCallback(TDDoubleClickCallback rfnDoubleClickCallback);
  protected:		
		DECLARE_MESSAGE_MAP()		

		afx_msg void OnSize(UINT nType,int cx,int cy);

		afx_msg void OnPaint( );

		afx_msg BOOL OnEraseBkgnd(CDC* pDC);

		afx_msg void OnLButtonDown(UINT nFlags,CPoint point);
		afx_msg void OnMouseMove(UINT nFlags,CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags,CPoint point);

		afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);

		afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
		afx_msg BOOL OnMouseWheel(UINT nFlags,short zDelta,CPoint pt);

		// For future use in possible optimizations... not used now.
		void AddItemToLayout(CTextureDatabaseItem*	poAddedItem);

		// Calculates the layout of all textures in the database for later drawing.
		// This method must be called after changing the cell size of whenever
		// adding some new texture.
		void CalculateLayout();

		// Loads into memory all necessary bitmaps.
		void CacheVisibleBitmaps();

		//
		void CalculateBitmapDrawingRemoveList(TDTIndexChangeList& rcnRemoveList);

		//
		bool RemoveFromDrawingListInvisibleBitmaps();

		//
		void CalculateBitmapDrawingAddList(TDTextureDatabase& rcnAddList);

		//
		bool AddToDrawingListNewlyVisibleBitmaps();


		// Draw all items to the specificed DC in the specified area.
		void DrawItems(CDC* poDC,CRect& roUpdateRect,bool boEraseBackground=true);

		void DrawAllItems(bool boEraseBackground=true);

		// Draw an specific item.
		void DrawItem(CTextureDatabaseItem* poItem,CRect& roUpdateRect,CDC* poDC);

		// Draw a tooltip box
		void DrawTooltip(CDC* poDC);

		// Filters the visibility parameters based on the current filter settings.
		void FilterDatabse();

		// The return value indicates if there was a change to it's visibility due to
		// filtering.
		bool FilterDatabseItem(CTextureDatabaseItem*	poDatabseItem);

		//
		bool CheckVirtualKey( int virtualKey );		

		//
		COLORREF ReturnColorForTextureType(const CTextureDatabaseItem* poItem);


		CPoint											m_oStarDraggingPoint;
		bool												m_boMouseLeftButtonDown;
		bool												m_boIsDragging;
		CRect												m_oLastDragRect;

		CSize												m_oSelectionRectangleSize;

		int													m_nItemHorizontalMargin;
		int													m_nItemVerticalMargin;

		int													m_nItemBorderSize;

		int													m_nTextureCellSize;

		int													m_nYOffset;

		// m_nMaxHeight and m_nMaxWidth are necessary for differential
		// layout calculation.
		int													m_nMaxHeight;
		int													m_nMaxWidth;

		TDTextureDatabase						m_cTextureDatabase;
		TDTextureDatabase						m_cTextureDrawingCache;

		// Indicates that changes have been made to the layout and have not yet been
		// updated to the display.
		bool                        m_boLayoutIsDirty;

		CRect												m_oClientRect;

		// Indicates that the thread must die as oon as possible, so it should
		// be ready for any sort of cleanup and finish procedure.
		volatile bool								m_boMustEndThread;

		// Filters
		int													m_nFilterTextureWidthMinimum;
		int													m_nFilterTextureWidthMaximum;

		int													m_nFilterTextureHeightMinimum;
		int													m_nFilterTextureHeightMaximum;

		bool												m_boFilterShowDDSOnly;
		bool												m_boFilterShowTIFOnly;
		bool												m_boFilterShowUsedInLevelOnly;
		bool												m_boShowIfHasAlphaChannel;
		bool												m_boShowAlphaChannel;

		bool                        m_boFilterShowDiffuseOnly;
		bool                        m_boFilterShowSpecularOnly;
		bool                        m_boFilterShowBumpOnly;
		bool												m_boFilterShowCubemapOnly;

		string                      m_strNameFilter;

		unsigned long long          m_nTotalDatabaseSize;

		// Database updater.
		ITextureDatabaseUpdater*		m_piDatabaseUpdater;

		// Status display object.
		ITextureViewerStatusDisplay*	m_piDisplayStatus;

		CBitmap												m_oBackBuffer;
		CDC														m_oBackBufferDC;
		CDC														m_oSourceBackBufferDC;

		volatile int                  m_nLastUpdateCount,m_nUpdateCount;
		CryConditionVariable					m_oRenderConditionVariable;
		CryMutex											m_oRenderLock;

		int                           m_nIdealClientWidth;
		int													  m_nIdealClientHeight;

		CTextureDatabaseItem					*m_poEnsureVisible;


		TDDoubleClickCallback					m_pfnDoubleClickCallback;

		int									m_hoverItemId;
		CPoint								m_tooltipPosition;
		bool								m_isCtrlDown;
};

#endif // TextureViewer_h__
