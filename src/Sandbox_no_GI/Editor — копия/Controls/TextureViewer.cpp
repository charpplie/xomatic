/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: TextureViewer.cpp,v 1.1 2008/08/19 12:54:41 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This file implements a control which objective is to display 
							multiple textures allowing selection and preview of such
							things.
							It also must handle scrolling and changes in the texture 
							cell display size.
-------------------------------------------------------------------------
History:
- 19:08:2008   12:54 : Created by Paulo Zaffari

*************************************************************************/
#include "StdAfx.h"

#include "TextureViewer.h"

#include "ImageExtensionHelper.h"

#include "../Include/ITextureViewerStatusDisplay.h"

#include "..\Texture Browser\TextureDatabaseItem.h"
#include "..\Texture Browser\TextureDatabaseCreator.h"

#include <Gdiplus.h>

//#define LOCKINDEBUG
#ifdef LOCKINDEBUG
#		define LOCKDEBUGMESSAGE		OutputDebugString
#else  //LOCKINDEBUG
#    define LOCKDEBUGMESSAGE(x) 
#endif //LOCKINDEBUG

BEGIN_MESSAGE_MAP(CTextureViewer, CScrollableWindow)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
CTextureViewer::CTextureViewer()
{
	m_nItemHorizontalMargin=25*2;
	m_nItemVerticalMargin=30*2;

	m_nItemBorderSize = 2;

	m_nTextureCellSize=gSettings.sTextureBrowserSettings.nCellSize-MINIMUNCELLSIZE;

	m_boMouseLeftButtonDown=false;
	m_boIsDragging=false;

	m_oStarDraggingPoint.SetPoint(0,0);
	m_oLastDragRect.SetRect(0,0,0,0);
	m_oSelectionRectangleSize.SetSize(2,2);

	m_oClientRect.SetRect(0,0,0,0);

	m_nYOffset=0;

	m_boLayoutIsDirty=false;

	m_boMustEndThread=false;

	m_piDatabaseUpdater=NULL;
	m_piDisplayStatus=NULL;

	SetAutoScrollWindowFlag(false);

	m_nTotalDatabaseSize=0;

	// Filters

	// Currently -1 means the dimensions are not being filtered.
	// We must replace this with an enum or something better after
	// we have all the filters active.
	m_nFilterTextureWidthMinimum=-1;
	m_nFilterTextureWidthMaximum=-1;

	m_nFilterTextureHeightMinimum=-1;
	m_nFilterTextureHeightMaximum=-1;

	m_boFilterShowDDSOnly=false;
	m_boFilterShowTIFOnly=false;
	m_boFilterShowUsedInLevelOnly=false;
	m_boShowIfHasAlphaChannel=false;
	m_boShowAlphaChannel=false;

	m_boFilterShowDiffuseOnly=false;
	m_boFilterShowSpecularOnly=false;
	m_boFilterShowBumpOnly=false;
	m_boFilterShowCubemapOnly=false;


	m_nLastUpdateCount=0;m_nUpdateCount=0;

	m_nIdealClientWidth=0;
	m_nIdealClientHeight=0;

	m_poEnsureVisible=NULL;

	m_hoverItemId = -1;
	m_tooltipPosition = CPoint(0,0);

	m_isCtrlDown = false;

}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CTextureViewer::~CTextureViewer()
{
	// Now we finish our 
	m_boMustEndThread=true;
	ResumeThread();
	WaitForThread();
	Stop(); // This is here only to avoid log warnings, as this is not necessary.

	m_oBackBuffer.DeleteObject();
	m_oBackBufferDC.DeleteDC();
	m_oSourceBackBufferDC.DeleteDC();

	//Detach();
	FinishDatabase();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
bool CTextureViewer::Create(int nLeft,int nTop,unsigned int nWidth,unsigned int nHeight,CWnd* poParentWindow,int nId,DWORD dwStyle)
{
	bool boReturn = (CScrollableWindow::Create(NULL,"TextureViewer",dwStyle,CRect(nLeft,nTop,nWidth,nHeight),poParentWindow,nId)!=FALSE);

	CDC *poDC=GetDC();
	m_oBackBufferDC.CreateCompatibleDC(poDC);
	m_oSourceBackBufferDC.CreateCompatibleDC(poDC);
	ReleaseDC(poDC);

	Start();

	return boReturn;
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
bool CTextureViewer::AddTextureDatabaseItem(CTextureDatabaseItem* poNewItem)
{
	// If we must end this thread, we will discard the information deleting
	// the given pointer and returning as soon as possible.
	if (m_boMustEndThread)
	{
		return false;
	}

	// BUG: Instead of making 2 locks, we would need a multi-m_oRenderLock primitive or there
	// can be deadlocks..
	if (m_piDatabaseUpdater)
	{
		LOCKDEBUGMESSAGE("TextureViewer.cpp, 139 - Database Lock\n");
		m_piDatabaseUpdater->Lock();
	}

	// If we must end this thread, we will discard the information deleting
	// the given pointer and returning as soon as possible.
	if (m_boMustEndThread)
	{
		LOCKDEBUGMESSAGE("TextureViewer.cpp, 148 - Database Unlock\n");
		m_piDatabaseUpdater->Unlock();
		return false;
	}

	LOCKDEBUGMESSAGE("TextureViewer.cpp, 153 - Lock\n");
	Lock();

	// If we must end this thread, we will discard the information deleting
	// the given pointer and returning as soon as possible.
	if (m_boMustEndThread)
	{
		LOCKDEBUGMESSAGE("TextureViewer.cpp, 160 - Database Unlock\n");
		m_piDatabaseUpdater->Unlock();
		LOCKDEBUGMESSAGE("TextureViewer.cpp, 163 - Unlock\n");
		Unlock();
		return false;
	}

	AddItemToLayout(poNewItem);
	m_cTextureDatabase.push_back(poNewItem);
	m_boLayoutIsDirty=true;	

	LOCKDEBUGMESSAGE("TextureViewer.cpp, 187 - Unlock\n");
	Unlock();

	if (m_piDatabaseUpdater)
	{
		LOCKDEBUGMESSAGE("TextureViewer.cpp, 189 - Database Unlock\n");
		m_piDatabaseUpdater->Unlock();
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::SetCellSize(const unsigned int nCellSize)
{
	// 992 + 32 =  1024. 32 is the minimum cell size. 1024 the maximum, currently.
	m_nTextureCellSize=(int)((MAXIMUMCELLSIZEMULTIPLIER*nCellSize)/100.0f);
	m_nTextureCellSize+=MINIMUNCELLSIZE;

	gSettings.sTextureBrowserSettings.nCellSize=m_nTextureCellSize;

	// reset thumb cache for visible ones
	Lock();
	for( size_t i = 0, iCount = m_cTextureDrawingCache.size(); i < iCount; ++i)
	{
		m_cTextureDrawingCache[i]->UnCacheBitmap();
	}
	m_boLayoutIsDirty=true;
	Unlock();

	CalculateLayout();
	SetClientSize(m_nIdealClientWidth,m_nIdealClientHeight);

	if (m_nYOffset>GetScrollLimit(SB_VERT))
	{
		m_nYOffset=GetScrollLimit(SB_VERT);
	}
	SetScrollPos(SB_VERT,m_nYOffset);
	++m_nLastUpdateCount;

	//m_nYOffset=0;
	//SetScrollPos(SB_VERT,0);

	Invalidate(TRUE);
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FinishedDatabaseUpdate()
{
	if ((m_boLayoutIsDirty)&&(!m_boMustEndThread))
	{
		CalculateLayout();
		SetClientSize(m_nIdealClientWidth,m_nIdealClientHeight);
	}
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// This function is actually the rendering thread. It starts as soon as
// the object is created and than it sleeps. When we need to render things
// again for any reason, this thread will be awaken again and when it finishes
// rendering it will go back to sleep.
// Design decision: in order to achieve faster user feedback, we separated
// the rendering thread from the caching thread as seeking the files on the disk
// can be quite a lengthy operation and it would stall the render for a while in
// the beginning of the execution. We also did this in order to allow the users
// of this control to fully personalize their data input mechanism so it would
// not be a texture browser bound to its first usage.
// Finally, we needed this render thread as otherwise used input could stall as well.
// In simple words: we don't want to stall either used input and user feedback
// and thus we created a thread for feedback as it would stall input, this render thread, 
// and a thread for updating the texture database we created another thread as it would stall 
// the feedback.
void CTextureViewer::Run()
{	
	while (true)
	{
		while (!m_boMustEndThread && (m_nLastUpdateCount == m_nUpdateCount)) 
		{
			m_oRenderConditionVariable.Wait(m_oRenderLock);
		}
		
		if (m_boMustEndThread)
		{
			break;
		}

		if (m_boLayoutIsDirty)
		{
			CalculateLayout();
		}

		if (m_piDatabaseUpdater)
		{
			LOCKDEBUGMESSAGE("TextureViewer.cpp, 759 - Database Lock\n");
			m_piDatabaseUpdater->Lock();
		}	

		LOCKDEBUGMESSAGE("TextureViewer.cpp, 766 - Lock\n");
		Lock();

		RemoveFromDrawingListInvisibleBitmaps(); 
		AddToDrawingListNewlyVisibleBitmaps();	

		LOCKDEBUGMESSAGE("TextureViewer.cpp, 777 - Unlock\n");
		Unlock();

		if (m_piDatabaseUpdater)
		{
			LOCKDEBUGMESSAGE("TextureViewer.cpp, 777 - Database Unlock\n");
			m_piDatabaseUpdater->Unlock();
		}	

		if (m_boMustEndThread)
		{
			break;
		}

		// Better than just erasing the background, here we erase the background, draw
		// all cached items and item placeholders. The cached items and the placeholders
		// will be redrawn again later.
		// Optimization: Don't redraw the items which were already cached ad they are
		// already correct in the backbuffer.
		DrawItems(NULL,m_oClientRect,true);

		int           nCurrentItem(0);
		int           nTotalItems(0);

		string				strFilename;
		CRect					stElementRect;

		LOCKDEBUGMESSAGE("TextureViewer.cpp, 232 - Lock\n");
		Lock();

		CDC								*poDC=GetDC();
		CBrush						oBrush(RGB(32,32,32));

		m_oBackBufferDC.SelectObject(m_oBackBuffer);

		// First we check all bitmaps which were previously visible and which are not anymore
		// and then we uncache them. The ones still visible will keep cached.
		for (nCurrentItem=0;nCurrentItem<m_cTextureDrawingCache.size();++nCurrentItem)
		{
			if (m_boMustEndThread)
			{
				break;
			}

			if (m_nUpdateCount!=(m_nLastUpdateCount-1))
			{
				break;
			}

			CTextureDatabaseItem*&	rpoDatabaseItem=m_cTextureDrawingCache[nCurrentItem];

			// No need to cache an item that is already cached.
			if (rpoDatabaseItem->GetCachedFlag())
			{
				continue;
			}

			// If we got to here, the item we are checking has some intersection with the client
			// area of the window, and thus it must be drawn.
			rpoDatabaseItem->CacheBitmap(m_boShowAlphaChannel);

			stElementRect=rpoDatabaseItem->GetDrawingRectangle();
			stElementRect.top-=m_nYOffset;
			stElementRect.bottom-=m_nYOffset;
			DrawItem(rpoDatabaseItem,stElementRect,&m_oBackBufferDC);

			// Apparently, drawing things directly to the window DC cause some bugs...
			// so we will be avoiding this for now.
			//DrawItem(rpoDatabaseItem,stElementRect,poDC);

			m_oBackBufferDC.SelectObject(m_oBackBuffer);
			poDC->SetStretchBltMode( HALFTONE );
			poDC->BitBlt(0,0,m_oClientRect.Width(),m_oClientRect.Height(),&m_oBackBufferDC,0,0,SRCCOPY);
			m_oBackBufferDC.SelectObject((HGDIOBJ)NULL);
		}

		ReleaseDC(poDC);

		LOCKDEBUGMESSAGE("TextureViewer.cpp, 264 - Unlock\n");
		Unlock();

		DrawItems(NULL,m_oClientRect,false);

		++m_nUpdateCount;
	}
}
//////////////////////////////////////////////////////////////////////////
bool CTextureViewer::SetDatabaseUpdater(ITextureDatabaseUpdater* piDatabaseUpdater)
{
	//// For now you can't hot change database updaters. (not true anymore)
	//if (m_piDatabaseUpdater)
	//{
	//	return false;
	//}
	m_piDatabaseUpdater=piDatabaseUpdater;

	return true;
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::SetStatusDisplay(ITextureViewerStatusDisplay*	piStatusDisplay)
{
	m_piDisplayStatus=piStatusDisplay;
}

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::NotifyShutDown()
{
	Lock();
	m_boMustEndThread=true;
	Unlock();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FilterWithMinimum(int nTextureWidthMinimum)
{
	m_nFilterTextureWidthMinimum=nTextureWidthMinimum;
	FilterDatabse();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FilterHeightMinimum(int nTextureHeightMinimum)
{
	m_nFilterTextureHeightMinimum=nTextureHeightMinimum;
	FilterDatabse();
}
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FilterWithMaximum(int nTextureWidthMaximum)
{
	m_nFilterTextureWidthMaximum=nTextureWidthMaximum;
	FilterDatabse();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FilterHeightMaximum(int nTextureHeightMaximum)
{
	m_nFilterTextureHeightMaximum=nTextureHeightMaximum;
	FilterDatabse();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FilterShowDDSOnly(bool boShowOnlyDDS)
{
	m_boFilterShowDDSOnly=boShowOnlyDDS;
	FilterDatabse();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FilterShowTIFOnly(bool boShowOnlyTIF)
{
	m_boFilterShowTIFOnly=boShowOnlyTIF;
	FilterDatabse();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FilterShowUsedInLevelOnly(bool boShowOnlyUsedInLevel)
{
	m_boFilterShowUsedInLevelOnly=boShowOnlyUsedInLevel;
	FilterDatabse();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FilterShowIfHasAlphaChannel(bool boShowIfHasAlphaChannel)
{
	m_boShowIfHasAlphaChannel=boShowIfHasAlphaChannel;
	FilterDatabse();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FilterShowSemanticsDiffuseOnly(bool boShowOnlyDiffuse)
{
	m_boFilterShowDiffuseOnly=boShowOnlyDiffuse;
	FilterDatabse();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FilterShowSemanticsSpecularOnly(bool boShowOnlySpecular)
{
	m_boFilterShowSpecularOnly=boShowOnlySpecular;
	FilterDatabse();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FilterShowSemanticsBumpOnly(bool boShowOnlyBump)
{
	m_boFilterShowBumpOnly=boShowOnlyBump;
	FilterDatabse();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FilterShowSemanticsCubemapOnly(bool boShowOnlyCubemap)
{
	m_boFilterShowCubemapOnly=boShowOnlyCubemap;
	FilterDatabse();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FilterByName(const string& strNameFilter )
{
	m_strNameFilter=strNameFilter;
	FilterDatabse();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::GetSelectedItems(TDTextureDatabase& rcpoSelectedItemArray)
{
	if (m_piDatabaseUpdater)
	{
		m_piDatabaseUpdater->Lock();
	}

	int nTotalTextures(m_cTextureDatabase.size());
	int nCurrentTexture(0);

	for (nCurrentTexture=0;nCurrentTexture<nTotalTextures;++nCurrentTexture)
	{
		CTextureDatabaseItem*& rpoCurrentItem=m_cTextureDatabase[nCurrentTexture];
		if (rpoCurrentItem->GetSelectedFlag())
		{
			rcpoSelectedItemArray.push_back(rpoCurrentItem);
		}
	}

	if (m_piDatabaseUpdater)
	{
		m_piDatabaseUpdater->Unlock();
	}
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// This function will first try to check if there is an item with the selected
// filename. In case there isn't it will try to add the item.
// If there was an item or the item was added successfully, this the scroll
// position will be changed so that the currently selected item will be visible.
// If the item didn't exist and could not be added, we will just return.
bool CTextureViewer::SetSelectedItem( const char* szFilename,bool& rboIsVisible)
{
	CTextureDatabaseItem*	poNewSelectedItem(NULL);
	// We will ask the database updater to add the item for us, so we won't have
	// duplicates. Also, this element does keep track, in it's original implementation
	// of all added filenames, so a search if it has been already added will be faster
	// than a local database search.
	rboIsVisible=false;

	if (m_piDatabaseUpdater)
	{
		poNewSelectedItem=m_piDatabaseUpdater->GetItem(szFilename);
		if (!poNewSelectedItem)
		{
			m_poEnsureVisible=NULL;
			return false;
		}
	}
	else
	{
		//int nTotalItems(0);
		//int nCurrentItem(0);

		// With no database updater, we will have to do things ourselves.
		// Or in other words, for now we will do nothing :-P, as there is
		// currently no need for this implementation to do this.
		// Also it's not it's implementation responsibility.
		return false;
	}

	// We unselect all previously selected items...

	if (m_piDatabaseUpdater)
	{
		m_piDatabaseUpdater->Lock();
	}

	Lock();
	int nTotalItems(0);
	int nCurrentItem(0);

	nTotalItems=m_cTextureDatabase.size();
	for (nCurrentItem=0;nCurrentItem<nTotalItems;++nCurrentItem)
	{
		CTextureDatabaseItem*& rpoCurrentItem=m_cTextureDatabase[nCurrentItem];
		rpoCurrentItem->SetSelectedFlag(false);
	}
	Unlock();

	if (m_piDatabaseUpdater)
	{
		m_piDatabaseUpdater->Unlock();
	}

	// And here we select our new item.
	poNewSelectedItem->SetSelectedFlag(true);
	rboIsVisible=poNewSelectedItem->GetVisibleFlag();

	if (m_piDisplayStatus)
	{
		string		strTempString;

		poNewSelectedItem->GetFilename(strTempString);
		m_piDisplayStatus->SetCurrentFilename("",strTempString.c_str());

		strTempString.Format("%d",poNewSelectedItem->GetFileSize());
		m_piDisplayStatus->SetFileSize(strTempString);
		m_piDisplayStatus->SetTotalFileSize(poNewSelectedItem->GetFileSize());

		strTempString.Format("%d",poNewSelectedItem->GetMips());
		m_piDisplayStatus->SetNumberofMips(strTempString);

		strTempString.Format("%d x %d",poNewSelectedItem->GetTextureWidth(),poNewSelectedItem->GetTextureHeight());
		m_piDisplayStatus->SetResolution(strTempString);

		poNewSelectedItem->GetSurfaceType(strTempString);		
		m_piDisplayStatus->SetTextureType(strTempString);
	}

	// We had to create this ensure visible because while opening
	// the editor the size will change many times and what was visible
	// may stop being visible after some time.
  m_poEnsureVisible=poNewSelectedItem;
	m_boLayoutIsDirty=true;

	Invalidate(TRUE);

	return true;
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::ResumeThread()
{
	++m_nLastUpdateCount;
	m_oRenderConditionVariable.Notify();
	m_oRenderLock.Unlock();
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::RestartThread()
{
	Lock();
	m_boMustEndThread=false;
	Unlock();

	Start();
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::ShowAlphaChannel(bool boShowAlphaChannel)
{
	if (m_boShowAlphaChannel!=boShowAlphaChannel)
	{
		// We must set the flag, uncache everything and invalidate the display.
		int									nCurrentItem(0);
		for (nCurrentItem=0;nCurrentItem<m_cTextureDrawingCache.size();++nCurrentItem)
		{
			CTextureDatabaseItem*&	rpoDatabaseItem=m_cTextureDrawingCache[nCurrentItem];
			rpoDatabaseItem->UnCacheBitmap();
		}
		m_cTextureDrawingCache.resize(0);

		m_boShowAlphaChannel=boShowAlphaChannel;
		Invalidate();
	}
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FinishDatabase()
{
	int nTotal(0);
	int nCurrent(0);

	nTotal=m_cTextureDatabase.size();
	for (nCurrent=0;nCurrent<nTotal;++nCurrent)
	{
		delete(m_cTextureDatabase[nCurrent]);
	}
	m_cTextureDatabase.resize(0);
	m_cTextureDrawingCache.resize(0);	
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::SetDoubleClickCallback(TDDoubleClickCallback rfnDoubleClickCallback)
{
	m_pfnDoubleClickCallback=rfnDoubleClickCallback;
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::OnSize(UINT nType,int cx,int cy)
{
	__super::OnSize(nType,cx,cy);

	GetClientRect(&m_oClientRect);

	// We must call those after __super::OnSize as we need to know the client
	// area before we can calculate the layout.

	// Instead of updating things right now, we gently ask the thread to do it for
	// us later. And it will happen when we scroll OR filter the database.
	m_boLayoutIsDirty=true;
	//m_nYOffset=0;
	if (m_nYOffset>GetScrollLimit(SB_VERT))
	{
		m_nYOffset=GetScrollLimit(SB_VERT);
	}
	//SetScrollPos(SB_VERT,0);

	// As CalculateLayout may change the desired client size...
	// we must call OnSize so it will have the correct size this time.
	__super::OnSize(nType,cx,cy);

	GetClientRect(&m_oClientRect);

	m_oBackBuffer.DeleteObject();
	CDC* poDc=GetDC();
	m_oBackBuffer.CreateCompatibleBitmap(poDc,m_oClientRect.Width(),m_oClientRect.Height());
	ReleaseDC(poDc);
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::OnPaint()
{
	CRect					stRect;
	BOOL					bnMustUpdateRegion(FALSE);

	// We are not using the real paint method as this is not the most
	// appropriate place for us to draw anything, but we set up the 
	// render request for the rendering thread here.

	bnMustUpdateRegion=GetUpdateRect(&stRect,FALSE);

	// We are not using the real paint method as this is not the most
	// appropriate place for us to draw anything, but we set up the 
	// render request for the rendering thread here.

	// As we are using a backbuffer, it doesn't matter to us if we needed to update
	// just a region of the client area. We will have to update it completly anyay.
	CacheVisibleBitmaps();
	
	// We validate the 
	ValidateRect(&stRect);
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// We are not erasing the background for now, as all the painting, including
// background erasing is done in the painting routine. (DrawItems)
BOOL CTextureViewer::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::OnLButtonDown(UINT nFlags,CPoint point)
{	
	// Negative coordinates are actually invalid data.
	if ((point.x<0)||(point.y<0))
	{
		return;
	}

	//SetFocus();

	m_oStarDraggingPoint=point;
	m_boMouseLeftButtonDown=true;
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::OnMouseMove(UINT nFlags,CPoint point)
{
	// Negative coordinates are actually invalid data.
	if ((point.x<0)||(point.y<0))
	{
		return;
	}

	// If the button was down AND is still down...
	// we must start drawing a drag rectangle from  the starting point stored
	// to the current point.
	// We will also have to check which textures are inside that rectangle.
	if (m_boMouseLeftButtonDown&&(nFlags&MK_LBUTTON))
	{
		CDC*	poDC=GetDC();

		CRect	stCurrentRect(
												MIN(m_oStarDraggingPoint.x,point.x),
												MIN(m_oStarDraggingPoint.y,point.y),
												MAX(m_oStarDraggingPoint.x,point.x),
												MAX(m_oStarDraggingPoint.y,point.y)
												);

		if (m_boIsDragging)
		{
			poDC->DrawDragRect(
				stCurrentRect,
				m_oSelectionRectangleSize,
				&m_oLastDragRect,
				m_oSelectionRectangleSize,
				NULL,
				NULL 
				);
		}
		else
		{
			poDC->DrawDragRect(
				stCurrentRect,
				m_oSelectionRectangleSize,
				NULL,
				m_oSelectionRectangleSize,
				NULL,
				NULL 
				);
		}
		m_oLastDragRect=stCurrentRect;

		m_boIsDragging=true;
	}

	POINT stHitTestPoint(point);
	stHitTestPoint.y+=m_nYOffset;
	int nTotalTextures(0);
	int nCurrentTexture(0);
	bool boHitItem = false;

	nTotalTextures=m_cTextureDatabase.size();

	m_hoverItemId = -1;

	for (nCurrentTexture=0;nCurrentTexture<nTotalTextures;++nCurrentTexture)
	{
		CTextureDatabaseItem*& rpoCurrentItem=m_cTextureDatabase[nCurrentTexture];

		boHitItem=rpoCurrentItem->HitTest(stHitTestPoint.x,stHitTestPoint.y);

		rpoCurrentItem->SetHoverFlag(boHitItem);

		if(boHitItem){

			m_hoverItemId = nCurrentTexture;
			m_tooltipPosition = point;
		}
	}

	DrawItems(NULL,m_oClientRect);
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::OnLButtonUp(UINT nFlags,CPoint point)
{
	// Negative coordinates are actually invalid data.
	if ((point.x<0)||(point.y<0))
	{
		return;
	}

	// If this we were not with the mouse down, the mouse up event actually makes no sense.
	if (!m_boMouseLeftButtonDown)
	{
		return;
	}

	// Used in case of dragging the selection.
	CRect	stCurrentRect(
		MIN(m_oStarDraggingPoint.x,point.x),
		MIN(m_oStarDraggingPoint.y,point.y),
		MAX(m_oStarDraggingPoint.x,point.x),
		MAX(m_oStarDraggingPoint.y,point.y)
		);

	// For adjust the scrolling position to the hit test.
	// For drawing things to the client area we should
	// subtract the scroll position instead.
	stCurrentRect.top+=m_nYOffset;
	stCurrentRect.bottom+=m_nYOffset;

	if (m_boIsDragging)
	{
		CDC*	poDC=GetDC();
		CSize	oFinishSize(0,0);

		poDC->DrawDragRect(
			&m_oLastDragRect,
			oFinishSize,
			&m_oLastDragRect,
			m_oSelectionRectangleSize,
			NULL,
			NULL 
			);


		m_boIsDragging=false;
	}

	m_boMouseLeftButtonDown=false;	

	// For adjust the scrolling position to the hit test.
	// For drawing things to the client area we should
	// subtract the scroll position instead.
	POINT stHitTestPoint(point);
	stHitTestPoint.y+=m_nYOffset;

	int       nNumberofSelectedFiles(0);
	string		strSelectedFilename;
	string    strDirectory;

	string    strSurfaceType;
	string    strNewSurfaceType;
	bool      boMultipleSurfaceTypes(false);

	bool								boSelect(false);
	unsigned long long	nTotalSelectionSize(0);
	bool								boAddSelection = CheckVirtualKey(VK_LCONTROL);
	bool								boUnselect =			CheckVirtualKey(VK_LMENU);
	bool								boStartSelection = ((!boAddSelection) && (!boUnselect));
	bool								boHitItem(false);
	bool								boClickSelection=((stCurrentRect.Width()==0)&&(stCurrentRect.Height()==0));

	int       nWidth(-1),nHeight(-1);
	int       nNewWidth(-1),nNewHeight(-1);
	bool      boMultipleResolutions(false);

	int       nNumberOfMips(-1);
	int       nNewNumberOfMips(-1);
	bool			boMultipleMipDifferentLevels(false);

	string		strOutputHelper;

	if (m_piDatabaseUpdater)
	{
		m_piDatabaseUpdater->Lock();
	}

		// If we are starting a new selection, we must first clear all previously selected items...
		// as they may not be currently visible this may take a while as we will have to traverse
		// then ENTIRE texture database...at least while traversing it we will also be able to
		// check for which items we have to select.
		int nTotalTextures(0);
		int nCurrentTexture(0);

		nTotalTextures=m_cTextureDatabase.size();
		for (nCurrentTexture=0;nCurrentTexture<nTotalTextures;++nCurrentTexture)
		{
			CTextureDatabaseItem*& rpoCurrentItem=m_cTextureDatabase[nCurrentTexture];

			if (rpoCurrentItem->GetVisibleFlag())
			{
				if (boClickSelection)
				{
					boHitItem=rpoCurrentItem->HitTest(stHitTestPoint.x,stHitTestPoint.y);
				}
				else
				{
					boHitItem=rpoCurrentItem->HitTest(stCurrentRect);
				}
			}
			else
			{
				boHitItem=false;
			}

			if (boHitItem)
			{
				// When you're clicking over an unselected item, even in unselect mode you will
				// want to add it to selection.
				if (boClickSelection)
				{
					//When you're clicking over an unselected item, even in unselect mode you will
					//want to add it to selection. (actually this inverses selection behavior).
					boSelect=!rpoCurrentItem->GetSelectedFlag();
					rpoCurrentItem->SetSelectedFlag(boSelect);
					if (boSelect)
					{
						++nNumberofSelectedFiles;
						rpoCurrentItem->GetRelativePath(strDirectory);
						rpoCurrentItem->GetFilename(strSelectedFilename);
						rpoCurrentItem->GetSurfaceType(strNewSurfaceType);
						nTotalSelectionSize+=rpoCurrentItem->GetFileSize();
						nNewNumberOfMips=rpoCurrentItem->GetMips();
						nNewWidth=rpoCurrentItem->GetTextureWidth();
						nNewHeight=rpoCurrentItem->GetTextureHeight();

						if ((!strSurfaceType.empty())&&(strNewSurfaceType!=strSurfaceType))
						{
							boMultipleSurfaceTypes=true;
						}
						strSurfaceType=strNewSurfaceType;

						if ((nNumberOfMips!=-1)&&(nNewNumberOfMips!=nNumberOfMips))
						{
							boMultipleMipDifferentLevels=true;
						}
						nNumberOfMips=nNewNumberOfMips;

						if ((nWidth!=-1)&&((nNewWidth!=nWidth)||(nNewHeight!=nHeight)))
						{
							boMultipleResolutions=true;
						}
						nWidth=nNewWidth;
						nHeight=nNewHeight;
					}					
				}
				else
				{
					 //We have to add those which hit the selection unless we are unselecting items
					 //so if unselect is false, it will select items, otherwise it will unselect them
					boSelect=!boUnselect;
					rpoCurrentItem->SetSelectedFlag(boSelect);
					if (boSelect)
					{
						++nNumberofSelectedFiles;
						rpoCurrentItem->GetRelativePath(strDirectory);
						rpoCurrentItem->GetFilename(strSelectedFilename);
						rpoCurrentItem->GetSurfaceType(strNewSurfaceType);
						nTotalSelectionSize+=rpoCurrentItem->GetFileSize();
						nNewNumberOfMips=rpoCurrentItem->GetMips();
						nNewWidth=rpoCurrentItem->GetTextureWidth();
						nNewHeight=rpoCurrentItem->GetTextureHeight();

						if ((!strSurfaceType.empty())&&(strNewSurfaceType!=strSurfaceType))
						{
							boMultipleSurfaceTypes=true;
						}
						strSurfaceType=strNewSurfaceType;

						if ((nNumberOfMips!=-1)&&(nNewNumberOfMips!=nNumberOfMips))
						{
							boMultipleMipDifferentLevels=true;
						}
						nNumberOfMips=nNewNumberOfMips;

						if ((nWidth!=-1)&&((nNewWidth!=nWidth)||(nNewHeight!=nHeight)))
						{
							boMultipleResolutions=true;
						}
						nWidth=nNewWidth;
						nHeight=nNewHeight;
					}					
				}
			}
			else
			{
				// If we are starting a new selection, the item wasn't hit in the selection, 
				// we must unselect it.
				if (boStartSelection)
				{
					rpoCurrentItem->SetSelectedFlag(false);
				}
				else
				{
					if (rpoCurrentItem->GetSelectedFlag())
					{
						++nNumberofSelectedFiles;
						rpoCurrentItem->GetRelativePath(strDirectory);
						rpoCurrentItem->GetFilename(strSelectedFilename);
						rpoCurrentItem->GetSurfaceType(strNewSurfaceType);
						nTotalSelectionSize+=rpoCurrentItem->GetFileSize();
						nNewNumberOfMips=rpoCurrentItem->GetMips();
						nNewWidth=rpoCurrentItem->GetTextureWidth();
						nNewHeight=rpoCurrentItem->GetTextureHeight();

						if ((!strSurfaceType.empty())&&(strNewSurfaceType!=strSurfaceType))
						{
							boMultipleSurfaceTypes=true;
						}
						strSurfaceType=strNewSurfaceType;

						if ((nNumberOfMips!=-1)&&(nNewNumberOfMips!=nNumberOfMips))
						{
							boMultipleMipDifferentLevels=true;
						}
						nNumberOfMips=nNewNumberOfMips;

						if ((nWidth!=-1)&&((nNewWidth!=nWidth)||(nNewHeight!=nHeight)))
						{
							boMultipleResolutions=true;
						}
						nWidth=nNewWidth;
						nHeight=nNewHeight;
					}
				}
			}			
		}

	if (m_piDatabaseUpdater)
	{
		m_piDatabaseUpdater->Unlock();
	}

	if (m_piDisplayStatus)
	{
		// TODO: Review are them any different one from another?
		m_piDisplayStatus->SetTotalFileSize(nTotalSelectionSize);

		if (nNumberofSelectedFiles>1)
		{
			m_piDisplayStatus->SetCurrentFilename("","Multiple textures selected.");
		}
		else if (nNumberofSelectedFiles==1)
		{
			m_piDisplayStatus->SetCurrentFilename(strDirectory.c_str(),strSelectedFilename.c_str());
		}
		else if (nNumberofSelectedFiles==0)
		{
			m_piDisplayStatus->SetCurrentFilename("","No selection");
		}

		if (boMultipleSurfaceTypes)
		{
			m_piDisplayStatus->SetTextureType("Multiple");
		}
		else
		{
			m_piDisplayStatus->SetTextureType(strSurfaceType.c_str());
		}

		if (boMultipleMipDifferentLevels)
		{
			m_piDisplayStatus->SetNumberofMips("Multiple Selected");
		}
		else if (nNewNumberOfMips==-1) // No items selected
		{
			m_piDisplayStatus->SetNumberofMips("");
		}
		else
		{
			strOutputHelper.Format("%d",nNumberOfMips);
			m_piDisplayStatus->SetNumberofMips(strOutputHelper.c_str());
		}

		if (nTotalSelectionSize==0)
		{
			m_piDisplayStatus->SetFileSize("");
		}
		else
		{
			strOutputHelper.Format("%I64u bytes",nTotalSelectionSize);
			m_piDisplayStatus->SetFileSize(strOutputHelper.c_str());
		}		

		if (boMultipleResolutions)
		{
			m_piDisplayStatus->SetResolution("Multiple selected");
		}
		else if (nWidth==-1)// No items selected
		{
			m_piDisplayStatus->SetResolution("");
		}
		else
		{
			strOutputHelper.Format("%dx%d",nWidth,nHeight);
			m_piDisplayStatus->SetResolution(strOutputHelper.c_str());
		}
	}	

	DrawItems(NULL,m_oClientRect);
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_pfnDoubleClickCallback)
	{
		if (m_piDatabaseUpdater)
		{
			m_piDatabaseUpdater->Lock();
		}

		// We only must call the callback if we have one texture under the mouse...
		// and if we did, at least one was selected.
		int											nTotalTextures(0);
		int											nCurrentTexture(0);

		POINT										stHitTestPoint(point);

		bool										boAnySelected(false);

		string									strDirectory;
		string									strSelectedFilename;
		string									strSurfaceType;
		string									strOutputHelper;
		
		int											nWidth(0);
		int											nHeight(0);
		int											nNumberOfMips(0);		
		unsigned long long			nTotalSelectionSize(0);

		stHitTestPoint.y+=m_nYOffset;

		nTotalTextures=m_cTextureDatabase.size();
		for (nCurrentTexture=0;nCurrentTexture<nTotalTextures;++nCurrentTexture)
		{
			CTextureDatabaseItem*& rpoCurrentItem=m_cTextureDatabase[nCurrentTexture];
			if (rpoCurrentItem->HitTest(stHitTestPoint.x,stHitTestPoint.y))
			{
				// We can't simply break here because we must also make sure all other items are unselected.
				rpoCurrentItem->SetSelectedFlag(true);

				// As we don't know if we will have to close the window or not, we must update all the information
				// as we would in other circumstances.
				rpoCurrentItem->GetRelativePath(strDirectory);
				rpoCurrentItem->GetFilename(strSelectedFilename);
				rpoCurrentItem->GetSurfaceType(strSurfaceType);
				nTotalSelectionSize+=rpoCurrentItem->GetFileSize();
				nNumberOfMips=rpoCurrentItem->GetMips();
				nWidth=rpoCurrentItem->GetTextureWidth();
				nHeight=rpoCurrentItem->GetTextureHeight();

				boAnySelected=true;				
			}
			else
			{
				rpoCurrentItem->SetSelectedFlag(false);
			}
		}	

		if (m_piDatabaseUpdater)
		{
			m_piDatabaseUpdater->Unlock();
		}

		// As we don't know if we will have to close the window or not, we must update all the information
		// as we would in other circumstances.
		if (m_piDisplayStatus)
		{
			// TODO: Review are them any different one from another?
			m_piDisplayStatus->SetTotalFileSize(nTotalSelectionSize);

			if (boAnySelected)
			{
				m_piDisplayStatus->SetCurrentFilename(strDirectory.c_str(),strSelectedFilename.c_str());
				m_piDisplayStatus->SetTextureType(strSurfaceType.c_str());
				strOutputHelper.Format("%d",nNumberOfMips);
				m_piDisplayStatus->SetNumberofMips(strOutputHelper.c_str());
				strOutputHelper.Format("%I64u bytes",nTotalSelectionSize);
				m_piDisplayStatus->SetFileSize(strOutputHelper.c_str());
				strOutputHelper.Format("%dx%d",nWidth,nHeight);
				m_piDisplayStatus->SetResolution(strOutputHelper.c_str());
			}
			else
			{
				m_piDisplayStatus->SetCurrentFilename("","No selection");
				m_piDisplayStatus->SetTextureType("");
				m_piDisplayStatus->SetNumberofMips("");
				m_piDisplayStatus->SetFileSize("");
				m_piDisplayStatus->SetResolution("");
			}
		}	

		DrawItems(NULL,m_oClientRect);

		if (boAnySelected)
		{
			m_pfnDoubleClickCallback();
		}
	}
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	__super::OnVScroll(nSBCode,nPos,pScrollBar);

	m_poEnsureVisible=NULL;

	m_nYOffset=GetScrollPos(SB_VERT);

	// Here we update the layout based on the data calculated by the render thread.
	SetClientSize(m_nIdealClientWidth,m_nIdealClientHeight);

	Invalidate(TRUE);
}
//////////////////////////////////////////////////////////////////////////
BOOL CTextureViewer::OnMouseWheel(UINT nFlags,short zDelta,CPoint pt)
{

	int nNewOffset(m_nYOffset);
	int nMaxScroll(m_nIdealClientHeight-m_oClientRect.Height()-1);

	nNewOffset-=zDelta;
	if (nNewOffset<0)
	{
		nNewOffset=0;
	}

	if (nNewOffset>nMaxScroll)
	{
		nNewOffset=nMaxScroll;
	}

	m_poEnsureVisible=NULL;

	m_nYOffset=nNewOffset;

	SetScrollPos(SB_VERT, m_nYOffset);

	// Here we update the layout based on the data calculated by the render thread.
	SetClientSize(m_nIdealClientWidth,m_nIdealClientHeight);

	++m_nLastUpdateCount;

	Invalidate(TRUE);

	return TRUE;
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Adds to an existing layout database a new item at the end so it might
// not need to check all items already in the database.
// For future use in possible optimizations... not used now.
void CTextureViewer::AddItemToLayout(CTextureDatabaseItem*	poAddedItem)
{
	int								nTextureWidth(0);
	int								nTextureHeight(0);

	float							fScaleHorizontal(1.0f);
	float							fScaleVertical(1.0f);
	float             fCurrentScale(1.0f);

	int								nCurrentTop(0);
	int								nCurrentLeft(0);

	// This will set the visibility based on the current filtering flags.
	FilterDatabseItem(poAddedItem);

	m_nTotalDatabaseSize+=poAddedItem->GetFileSize();

	// Invisible items, due to filtering or any other reason
	// are not considered in the layout calculation...
	if (!poAddedItem->GetVisibleFlag())
	{
		return;
	}

	// If there is an element already added, we start from this m_nLastUpdateCount added element...
	if (!m_cTextureDatabase.empty())
	{
		CTextureDatabaseItem*& rpoLastItem=m_cTextureDatabase.back();
		CRect roLastDrawingRectangle=rpoLastItem->GetDrawingRectangle();

		nCurrentLeft=roLastDrawingRectangle.right;
		nCurrentTop=roLastDrawingRectangle.top;
	}	

	// We must first find which is the first item of the m_nLastUpdateCount added line
	// so we can calculate current line height.
	CRect&											rstElementRect=poAddedItem->GetDrawingRectangle();


	nTextureWidth=poAddedItem->GetTextureWidth();
	nTextureHeight=poAddedItem->GetTextureHeight();

	fScaleHorizontal=m_nTextureCellSize/(float)nTextureWidth;
	fScaleVertical=m_nTextureCellSize/(float)nTextureHeight;

	fCurrentScale=MIN(fScaleHorizontal,fScaleVertical);

	// If the right of the element would go out of the client rectangle, and thus it would not be visible...
	if ((nCurrentLeft+m_nItemHorizontalMargin+(int)(nTextureWidth*fCurrentScale))>m_oClientRect.Width())
	{
		// This is the case we have a single texture bigger than the whole client width...
		if (nCurrentLeft==0)
		{
			m_nMaxWidth=MAX(m_nMaxWidth,m_nItemHorizontalMargin+(int)(nTextureWidth*fCurrentScale));
		}
		// This is the case we just need to go to the next line...
		else
		{
			nCurrentLeft=0;
			nCurrentTop+=m_nMaxHeight+m_nItemVerticalMargin;
			m_nMaxHeight=0;
		}
	}
	m_nMaxHeight=MAX(m_nMaxHeight,nTextureHeight*fCurrentScale);

	rstElementRect.left=nCurrentLeft				+(int)(m_nItemHorizontalMargin);
	rstElementRect.right=rstElementRect.left+(int)(nTextureWidth*fCurrentScale);

	rstElementRect.top=nCurrentTop+(int)(m_nItemVerticalMargin);
	rstElementRect.bottom=rstElementRect.top+(int)(nTextureHeight*fCurrentScale);

	// No need when adding a single item.
	//nCurrentLeft=rstElementRect.right;

	// If we updated the layout for every new item we added, it would blink constantly
	// and that would render the control unusable... unless we customize it further...(which I didn't do)
	// In order to avoid it, we created this dirty flag that will be checked in key
	// operations (such as when the user scrolls the screen) and in such operations
	// we will m_nUpdateCount the scroll size.
	// Also, updating it from a thread which is not the owner thread of the control would cause input problems.
	m_boLayoutIsDirty=true;
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::CalculateLayout()
{
	int								nCurrentItem(0);
	int								nTotalItems(0);

	int								nCurrentTop(0);
	int								nCurrentLeft(0);

	int								nTextureWidth(0);
	int								nTextureHeight(0);

	float							fScaleHorizontal(1.0f);
	float							fScaleVertical(1.0f);
	float             fCurrentScale(1.0f);

	m_nMaxHeight=0;
	m_nMaxWidth=0;

	// BUG: Instead of making 2 locks, we would need a multi-lock primitive or there
	// can be deadlocks..
	if (m_piDatabaseUpdater)
	{
		LOCKDEBUGMESSAGE("TextureViewer.cpp, 680 - Database Lock\n");
		m_piDatabaseUpdater->Lock();
	}
	LOCKDEBUGMESSAGE("TextureViewer.cpp, 683 - Lock\n");
	Lock();

	nTotalItems=m_cTextureDatabase.size();
	for (nCurrentItem=0;nCurrentItem<nTotalItems;++nCurrentItem)
	{
		CTextureDatabaseItem*&			rpoCurrentTextureItem=m_cTextureDatabase[nCurrentItem];
		CRect&											rstElementRect=rpoCurrentTextureItem->GetDrawingRectangle();

		// Invisible items, due to filtering or any other reason
		// are not considered in the layout calculation...
		if (!rpoCurrentTextureItem->GetVisibleFlag())
		{
			continue;
		}

		nTextureWidth=rpoCurrentTextureItem->GetTextureWidth();
		nTextureHeight=rpoCurrentTextureItem->GetTextureHeight();

		fScaleHorizontal=m_nTextureCellSize/(float)nTextureWidth;
		fScaleVertical=m_nTextureCellSize/(float)nTextureHeight;

		fCurrentScale=MIN(fScaleHorizontal,fScaleVertical);

		// If the right of the element would go out of the client rectangle, and thus it would not be visible...
		if ((nCurrentLeft+m_nItemHorizontalMargin+(int)(nTextureWidth*fScaleHorizontal))>m_oClientRect.Width())
		{
			// This is the case we have a single texture bigger than the whole client width...
			if (nCurrentLeft==0)
			{
				m_nMaxWidth=MAX(m_nMaxWidth,m_nItemHorizontalMargin+(int)(nTextureWidth*fScaleHorizontal));
			}
			// This is the case we just need to go to the next line...
			else
			{
				nCurrentLeft=0;
				nCurrentTop+=m_nMaxHeight+m_nItemVerticalMargin;
				m_nMaxHeight=0;
			}
		}
		m_nMaxHeight=MAX(m_nMaxHeight,nTextureHeight*fCurrentScale);

		rstElementRect.left=nCurrentLeft				+(int)(m_nItemHorizontalMargin);
		rstElementRect.right=rstElementRect.left+(int)(nTextureWidth*fScaleHorizontal);

		rstElementRect.top=nCurrentTop+(int)(m_nItemVerticalMargin);
		rstElementRect.bottom=rstElementRect.top+(int)(nTextureHeight*fScaleVertical);

		nCurrentLeft=rstElementRect.right;
	}


	m_nIdealClientWidth=m_nMaxWidth;
	m_nIdealClientHeight=nCurrentTop+m_nMaxHeight+2*m_nItemVerticalMargin;

	// If called from a thread different of the thread that owns the control it will
	// cause the window owning the scroll bar to freeze until you click in another window
	// and than click again in the window you were using (who owns the control).
	//SetClientSize(m_nMaxWidth,nCurrentTop+m_nMaxHeight+2*m_nItemVerticalMargin);

	m_boLayoutIsDirty=false;

	if (m_poEnsureVisible)
	{
		if (m_poEnsureVisible->GetVisibleFlag())
		{
			CRect	oElementRect=m_poEnsureVisible->GetDrawingRectangle();
			int nOldYOffset(m_nYOffset);
			m_nYOffset=oElementRect.top-m_nItemVerticalMargin;
			if (m_nYOffset!=nOldYOffset)
			{
				// This works for this particular case though.
				SetClientSize(m_nIdealClientWidth,m_nIdealClientHeight);
				SetScrollPos(SB_VERT,m_nYOffset);
			}
		}
	}

	LOCKDEBUGMESSAGE("TextureViewer.cpp, 753 - Database Unlock\n");
	Unlock();
	if (m_piDatabaseUpdater)
	{
		LOCKDEBUGMESSAGE("TextureViewer.cpp, 757 - Database Unlock\n");
		m_piDatabaseUpdater->Unlock();
	}
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::CacheVisibleBitmaps()
{
	ResumeThread();
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::CalculateBitmapDrawingRemoveList(TDTIndexChangeList& rcnRemoveList)
{
	int		nCurrentItem(0);
	CRect oCurrentRect;
	CRect oIntersection;

	// First we check all bitmaps which were previously visible and which are not anymore
	// and then we uncache them. The ones still visible will keep cached.
	for (nCurrentItem=0;nCurrentItem<m_cTextureDrawingCache.size();++nCurrentItem)
	{
		CTextureDatabaseItem*&	rpoDatabaseItem=m_cTextureDrawingCache[nCurrentItem];

		// No need to cache bitmaps that should not be displayed for one reason or another...
		// specially because we have no guarantee that their positions will be valid...
		// so it is a cached item, we must remove it from the cache.
		if (!rpoDatabaseItem->GetVisibleFlag())
		{
			rcnRemoveList.push_back(nCurrentItem);
			continue;
		}

		oCurrentRect=rpoDatabaseItem->GetDrawingRectangle();
		// We may need to have 2 copies of the rectangles, one the drawing and one the
		// layout rectangle...
		oCurrentRect.top-=m_nYOffset;
		oCurrentRect.bottom-=m_nYOffset;

		// We don't need to uncache textures which are still visible...
		if (oIntersection.IntersectRect(&oCurrentRect,&m_oClientRect))
		{
			continue;
		}
		rcnRemoveList.push_back(nCurrentItem);
	}
}
//////////////////////////////////////////////////////////////////////////
bool CTextureViewer::RemoveFromDrawingListInvisibleBitmaps()
{
	int									nCurrentItem(0);
	CRect								oCurrentRect;
	CRect								oIntersection;
	bool								boRemovedAny(false);
	TDTIndexChangeList	cnRemoveList;
	//int                 nIndexOffset(0);

	CalculateBitmapDrawingRemoveList(cnRemoveList);

	boRemovedAny=!cnRemoveList.empty();

	// First we check all bitmaps which were previously visible and which are not anymore
	// and then we uncache them. The ones still visible will keep cached.
	for (nCurrentItem=0;nCurrentItem<cnRemoveList.size();++nCurrentItem)
	{
		int& rnItemIndex=cnRemoveList[nCurrentItem];
		CTextureDatabaseItem*&	rpoDatabaseItem=m_cTextureDrawingCache[rnItemIndex];

		rpoDatabaseItem->UnCacheBitmap();
		//m_cTextureDrawingCache.erase(m_cTextureDrawingCache.begin()+rnItemIndex/*-nIndexOffset*/);
		//++nIndexOffset
	}
	m_cTextureDrawingCache.resize(0);

	return boRemovedAny;
}
//////////////////////////////////////////////////////////////////////////
//
void CTextureViewer::CalculateBitmapDrawingAddList(TDTextureDatabase& rcoAddList)
{
	int		nTotalItems(0);
	int		nCurrentItem(0);
	CRect	oCurrentRect;
	CRect oIntersection;

	nTotalItems=m_cTextureDatabase.size();
	for (nCurrentItem=0;nCurrentItem<nTotalItems;++nCurrentItem)
	{
		CTextureDatabaseItem*&	rpoDatabaseItem=m_cTextureDatabase[nCurrentItem];

		// No need to cache bitmaps that should not be displayed for one reason or another...
		// specially because we have no guarantee that their positions will be valid...
		if (!rpoDatabaseItem->GetVisibleFlag())
		{
			continue;
		}

		oCurrentRect=rpoDatabaseItem->GetDrawingRectangle();
		// We may need to have 2 copies of the rectangles, one the drawing and one the
		// layout rectangle...
		oCurrentRect.top-=m_nYOffset;
		oCurrentRect.bottom-=m_nYOffset;

		// If the item is entirely outside of the client rect, no need to cache a bitmap for it.
		if (oCurrentRect.bottom<0)
		{
			continue;
		}

		// If the item is entirely below the client rect, as Y coordinates grow monotonically,
		// no need to cache anything else.
		if (oCurrentRect.top>m_oClientRect.bottom)
		{
			break;
		}

		// Added to use the same criteria as in the remove list.
		if (!oIntersection.IntersectRect(m_oClientRect,oCurrentRect))
		{
			continue;
		}

		rcoAddList.push_back(rpoDatabaseItem);
	}
}
//////////////////////////////////////////////////////////////////////////
bool CTextureViewer::AddToDrawingListNewlyVisibleBitmaps()
{
	int									nTotalItems(0);
	int									nCurrentItem(0);
	CRect								oCurrentRect;
	bool								boAddedAnItem(false);
	TDTextureDatabase	 coAddList;

	CalculateBitmapDrawingAddList(coAddList);
	
	boAddedAnItem=!coAddList.empty();

	nTotalItems=coAddList.size();
	for (nCurrentItem=0;nCurrentItem<nTotalItems;++nCurrentItem)
	{
		// If we got to here, the item we are checking has some intersection with the client
		// area of the window, and thus it must be drawn.
		m_cTextureDrawingCache.push_back(coAddList[nCurrentItem]);
	}
	return boAddedAnItem;
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::DrawItems(CDC* poDC,CRect& roUpdateRect,bool boEraseBackground)
{
	//CacheVisibleBitmaps();

	// BUG: Instead of making 2 locks, we would need a multi-m_oRenderLock primitive or there
	// can be deadlocks..
	if (m_piDatabaseUpdater)
	{
		LOCKDEBUGMESSAGE("TextureViewer.cpp, 988 - Database Lock\n");
		m_piDatabaseUpdater->Lock();
	}

	LOCKDEBUGMESSAGE("TextureViewer.cpp, 996 - Lock\n");
	Lock();

	CDC *poWindowDeviceContext(GetDC());
	poDC=poWindowDeviceContext;


	DrawAllItems(boEraseBackground);
	m_oBackBufferDC.SelectObject(m_oBackBuffer);
	poWindowDeviceContext->SetStretchBltMode( HALFTONE );
	poWindowDeviceContext->BitBlt(0,0,m_oClientRect.Width(),m_oClientRect.Height(),&m_oBackBufferDC,0,0,SRCCOPY);
	m_oBackBufferDC.SelectObject((HGDIOBJ)NULL);

	ReleaseDC(poWindowDeviceContext);

	LOCKDEBUGMESSAGE("TextureViewer.cpp, 1030 - Unlock\n");
	Unlock();

	if (m_piDatabaseUpdater)
	{
		LOCKDEBUGMESSAGE("TextureViewer.cpp, 1026 - Database Unlock\n");
		m_piDatabaseUpdater->Unlock();
	}
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::DrawAllItems(bool boMustEraseBackgroung)
{
	string						strFilename;

	CRect							stElementRect;
	CRect							stIntersection;
	int								nTotalTextures(0);
	int								nCurrentTexture(0);
	CBrush						oBrush(::GetSysColor(COLOR_WINDOW));


	m_oBackBufferDC.SelectObject(m_oBackBuffer);
	if (boMustEraseBackgroung)
	{
		m_oBackBufferDC.FillRect(&m_oClientRect,&oBrush);
	}


	nTotalTextures=m_cTextureDrawingCache.size();
	for (nCurrentTexture=0;nCurrentTexture<nTotalTextures;++nCurrentTexture)
	{
		CTextureDatabaseItem*& rpoTextureDatabseItem=m_cTextureDrawingCache[nCurrentTexture];
		stElementRect=rpoTextureDatabseItem->GetDrawingRectangle();
		stElementRect.top-=m_nYOffset;
		stElementRect.bottom-=m_nYOffset;

		// If the rectangle is outside the m_nUpdateCount area...(possible when region m_nUpdateCount is needed).
		if (!stIntersection.IntersectRect(&stElementRect,&m_oClientRect))
		{
			continue;
		}

		// we must set HALFTONE blt mode each time, the thumb bitmap hosted in m_oBackBufferDC is changing every time
		DrawItem(rpoTextureDatabseItem,stElementRect,&m_oBackBufferDC);
	}
	DrawTooltip(&m_oBackBufferDC);
	m_oBackBufferDC.SelectObject((HGDIOBJ)NULL);
}
//////////////////////////////////////////////////////////////////////////
COLORREF CTextureViewer::ReturnColorForTextureType(const CTextureDatabaseItem* poItem)
{
	string textureExtension;
	poItem->GetExtension(textureExtension);
	if (textureExtension == string(".dds"))
		return RGB(128,16,42);
	else if (textureExtension == string(".tif"))
		return RGB(99,172,255);
	else
		return RGB(132,255,220);
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::DrawTooltip(CDC* poDC)
{
	if(m_hoverItemId == -1)
		return;

	enum {MAX_STRING = 256};
	WCHAR wszString[MAX_STRING];

	CTextureDatabaseItem *poItem = m_cTextureDatabase[m_hoverItemId];

	string	strDirectory;
	string	strSelectedFilename;
	string	strSurfaceType;
	string	strOutputHelper;
	string  strGlobalString;
	int		nNumberOfMips;
	int		nWidth;
	int		nHeight;
	int		nSize;
	int		titleTextSize = 12;
	int		titleTopMargin = 3;

	int tooltipWidth = 240;
	int tooltipHeight = 100;
	int tooltipTopHeight = 20;

	Gdiplus::PointF origin(m_tooltipPosition.x,m_tooltipPosition.y-tooltipHeight);
	Gdiplus::Graphics graphics(poDC->GetSafeHdc());
	Gdiplus::RectF tooltipRect(origin.X,origin.Y, tooltipWidth,tooltipHeight);

	// If the tooltip is out on the right side of the client screen
	if( m_oClientRect.Width() < tooltipRect.X + tooltipRect.Width)
	{
		origin.X = m_tooltipPosition.x-tooltipWidth;
		tooltipRect.X = origin.X;
	}
	// If the tooltip is out from the top of the client screen
	if( m_oClientRect.top > tooltipRect.Y - tooltipRect.Height)
	{
		origin.Y = m_tooltipPosition.y + 24; // The tolltip position plus the size of the pointer icon
		tooltipRect.Y = origin.Y;
	}

	Gdiplus::SolidBrush tooltipBrush(Gdiplus::Color(240, 100,100,100));
	graphics.FillRectangle(&tooltipBrush,tooltipRect);
	Gdiplus::SolidBrush topTooltipBrush(Gdiplus::Color(255, 64,64,64));
	Gdiplus::RectF topTooltipRect(origin.X,origin.Y, tooltipWidth,tooltipTopHeight);
	graphics.FillRectangle(&topTooltipBrush,topTooltipRect);
	Gdiplus::Pen tooltipPen(Gdiplus::Color(255,0,0,0), 1.0f);
	graphics.DrawRectangle(&tooltipPen,tooltipRect);

	poItem->GetRelativePath(strDirectory);
	poItem->GetFilename(strSelectedFilename);
	poItem->GetSurfaceType(strSurfaceType);
	nNumberOfMips=poItem->GetMips();
	nSize=poItem->GetFileSize();
	nWidth=poItem->GetTextureWidth();
	nHeight=poItem->GetTextureHeight();

	strGlobalString.Format("Path: %s\nSurface Type: %s\nMIPS: %d\nSize: %d bytes\nResolution:%dx%d",strDirectory,strSurfaceType,nNumberOfMips,nSize,nWidth,nHeight);

	Gdiplus::FontFamily fontFamily(L"Arial");
	Gdiplus::Font titleFont(&fontFamily,titleTextSize,Gdiplus::FontStyleRegular|Gdiplus::FontStyleBold,Gdiplus::UnitPixel);
	Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255,255,255));
	Gdiplus::RectF textRect(origin.X+1,origin.Y+titleTopMargin,tooltipWidth,tooltipHeight-titleTopMargin);
	size_t numCharsConverted = mbstowcs(wszString, (strSelectedFilename.c_str() ? strSelectedFilename.c_str() : ""), MAX_STRING - 1);
	if (numCharsConverted == MAX_STRING - 1)
		wszString[numCharsConverted] = 0;
	if (numCharsConverted == -1)
		wszString[0] = 0;

	Gdiplus::StringFormat format;
	format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
	format.SetAlignment(Gdiplus::StringAlignmentNear);

	if (wcslen(wszString) == 0)
		wcscpy(wszString, L"{}");
	graphics.DrawString(wszString,-1,&titleFont,textRect,&format,&textBrush);

	// Filename
	Gdiplus::RectF filenameRect(origin.X+1,origin.Y+titleTopMargin+tooltipTopHeight,tooltipWidth,tooltipHeight-titleTopMargin);
	numCharsConverted = mbstowcs(wszString, (strGlobalString.c_str() ? strGlobalString.c_str() : ""), MAX_STRING - 1);
	if (numCharsConverted == MAX_STRING - 1)
		wszString[numCharsConverted] = 0;
	if (numCharsConverted == -1)
		wszString[0] = 0;
	if (wcslen(wszString) == 0)
		wcscpy(wszString, L"{}");
	Gdiplus::Font textFont(&fontFamily,10,Gdiplus::FontStyleRegular|Gdiplus::FontStyleBold,Gdiplus::UnitPixel);
	graphics.DrawString(wszString,-1,&textFont,filenameRect,&format,&textBrush);
}
//////////////////////////////////////////////////////////////////////////
void CTextureViewer::DrawItem(CTextureDatabaseItem* poItem,CRect& roUpdateRect,CDC* poDC)
{
	int				nTextureWidth(0);
	int				nTextureHeight(0);

	float     fTextureScale(1.0f);
	float     fHorizontalTextureScale(1.0f);
	float     fVerticalTextureScale(1.0f);

	DWORD     dwRasterOperation(SRCCOPY);

	int				nOriginalTextLeft(0);
	int				nOriginalTextRight(0);

	string		strOutputText;

	int				nPreviousBottom(0);

	CRect			stTextRectangle;
	int				oldTextureCellSize = m_nTextureCellSize;

	if(poItem->GetHoverFlag())
	{
		roUpdateRect.InflateRect(5,5,5,5);
		m_nTextureCellSize += 10;
	}

	if (poItem->GetSelectedFlag())
	{
		CBrush oSelcetionBrush(::GetSysColor(COLOR_HIGHLIGHT));
		CRect stSelectionRect(roUpdateRect);
		dwRasterOperation=SRCCOPY;
		CBrush blackBrush(RGB(0,0,0));
		poDC->FillRect(stSelectionRect,&blackBrush);
		for(int i=0; i<(m_nItemBorderSize+2);i++)
		{
			stSelectionRect.InflateRect(1,1,1,1);
			poDC->FrameRect(stSelectionRect,&oSelcetionBrush);
		}
	}
	else
	{
		CBrush blackBrush(RGB(0,0,0));
		CBrush coloredBrush(ReturnColorForTextureType(poItem));
		CRect borderRect(roUpdateRect);
		int borderSize = m_nItemBorderSize+2;
		borderRect.InflateRect(borderSize,borderSize,borderSize,borderSize);
		poDC->FillRect(borderRect,&blackBrush);
		borderRect.InflateRect(-1,-1,-1,-1);
		poDC->FillRect(borderRect,&coloredBrush);
		borderRect.InflateRect(-m_nItemBorderSize,-m_nItemBorderSize,-m_nItemBorderSize,-m_nItemBorderSize);
		poDC->FillRect(borderRect,&blackBrush);
	}

	// If the bitmap is cached, draw it...
	if (poItem->GetCachedFlag())
	{
		m_oSourceBackBufferDC.SelectObject(poItem->GetBitmap());
		poDC->SetStretchBltMode(HALFTONE);
		poDC->BitBlt(
			roUpdateRect.left,
			roUpdateRect.top,
			roUpdateRect.Width(),
			roUpdateRect.Height(),
			&m_oSourceBackBufferDC,
			0,
			0,
			dwRasterOperation
			);
	}
	else // If the bitmap is not cached, draw the placeholder.
	{
		CBrush oBrush(::GetSysColor(COLOR_HIGHLIGHT));
		m_oBackBufferDC.FillRect(roUpdateRect,&oBrush);
		m_oBackBufferDC.DrawText("CACHING BITMAP",roUpdateRect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
	}


	// Due to a change of the algorithm , we are not anymore using the vertical center flag...
	// ... this change was made in order to accommodate texture name AND resolution text below it.
	// The change of the algorithm removed any space between the filename and the texture itself...
	// and to make it possible for us to fit everything with minimum space, we had to use the calculated...
	// ...text height (and just the height) to know the placement of the below text (resolution).
	// Right and left positions are of no use to us.

	//////////////////////////////////////////////////////////////////////////
	// Calculate the texture filename rectangle and draws it to the screen.
	stTextRectangle=roUpdateRect;

	nOriginalTextLeft=roUpdateRect.left;
	nOriginalTextRight=roUpdateRect.right;

	poItem->GetFilename(strOutputText);

	stTextRectangle.top=stTextRectangle.bottom+m_nItemBorderSize;
	stTextRectangle.bottom+=m_nItemVerticalMargin/2;

	COLORREF oldTextColor = m_oBackBufferDC.GetTextColor();
	int oldBkMode = m_oBackBufferDC.GetBkMode();

	if (poItem->GetSelectedFlag())
		m_oBackBufferDC.SetTextColor(::GetSysColor(COLOR_HIGHLIGHT));
	else
		m_oBackBufferDC.SetTextColor(::GetSysColor(COLOR_MENUTEXT));
	m_oBackBufferDC.SetBkMode(TRANSPARENT);

	m_oBackBufferDC.DrawText(strOutputText.c_str(),stTextRectangle,DT_CALCRECT|DT_CENTER|/*DT_VCENTER|*/DT_SINGLELINE|DT_END_ELLIPSIS);
	stTextRectangle.left=nOriginalTextLeft;
	stTextRectangle.right=nOriginalTextRight;
	m_oBackBufferDC.DrawText(strOutputText.c_str(),stTextRectangle,DT_CENTER/*|DT_VCENTER*/|DT_SINGLELINE|DT_END_ELLIPSIS);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Draws the texure resolution based on the calculated filename text position.
	//strOutputText.Format("[%dX%d]",poItem->GetTextureWidth(),poItem->GetTextureHeight());
	//nPreviousBottom=stTextRectangle.bottom;
	//stTextRectangle.bottom+=stTextRectangle.bottom-stTextRectangle.top;
	//stTextRectangle.top=nPreviousBottom;
	//m_oBackBufferDC.DrawText(strOutputText.c_str(),stTextRectangle,DT_CENTER/*|DT_VCENTER*/|DT_SINGLELINE|DT_END_ELLIPSIS);
	//////////////////////////////////////////////////////////////////////////

	m_oBackBufferDC.SetTextColor(oldTextColor);
	m_oBackBufferDC.SetBkMode(oldBkMode);

	m_oSourceBackBufferDC.SelectObject((HGDIOBJ)NULL);	

	m_nTextureCellSize = oldTextureCellSize;
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CTextureViewer::FilterDatabse()
{
	// BUG: Instead of making 2 locks, we would need a multi-m_oRenderLock primitive or there
	// can be deadlocks..
	if (m_piDatabaseUpdater)
	{
		LOCKDEBUGMESSAGE("TextureViewer.cpp, 1130 - Database Lock\n");
		m_piDatabaseUpdater->Lock();
	}

	int		nTotalTextures(0);
	int		nCurrentTexture(0);
	bool	boVisibilityChanged(false);

	nTotalTextures=m_cTextureDatabase.size();
	for (nCurrentTexture=0;nCurrentTexture<nTotalTextures;++nCurrentTexture)
	{
		boVisibilityChanged|=FilterDatabseItem(m_cTextureDatabase[nCurrentTexture]);
	}

	// If the visibility changed, we need to recalculate the layout AND we must
	// recalculate what to display.
	// In order to do it, we must:
	// 1. Pause the loading thread
	// 2. Calculate the new layout
	// 3. Resume the rendering thread 
	// OBS: steps 2 and 3 might be already done if we just calculate the new layout as long
	// as we do this with the right locking. We must check for deadlocks.
	// Hint: Try RecalcLayout inside the m_oRenderLock and CacheVisibleBitmaps outside all locks.
	if (m_piDatabaseUpdater)
	{
		LOCKDEBUGMESSAGE("TextureViewer.cpp, 1173 - Database Unlock\n");
		m_piDatabaseUpdater->Unlock();
	}

	// If the visibility has changed, rebuild it.
	if (boVisibilityChanged)
	{
		CalculateLayout();
		SetClientSize(m_nIdealClientWidth,m_nIdealClientHeight);
		if (m_nYOffset>GetScrollLimit(SB_VERT))
		{
			m_nYOffset=GetScrollLimit(SB_VERT);
		}
		//m_nYOffset=0;
		SetScrollPos(SB_VERT,m_nYOffset);
		Invalidate(TRUE);
	}
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
bool CTextureViewer::FilterDatabseItem(CTextureDatabaseItem*	poDatabseItem)
{
	bool				boNextVisibleFlag(true);
	bool				boCurrentVisibleFlag(poDatabseItem->GetVisibleFlag());
	string			strExtension;
	string			strFilename;
	string			strRelativePath;

	// Algorithm: a texture should always be visible, unless it is filtered out.
	if (m_boFilterShowDDSOnly)
	{
		poDatabseItem->GetExtension(strExtension);
		if (strExtension.compareNoCase("dds")!=0)
		{
			boNextVisibleFlag=false;
		}
	}

	if (m_boFilterShowTIFOnly)
	{
		poDatabseItem->GetExtension(strExtension);
		if (strExtension.compareNoCase("tif")!=0)
		{
			boNextVisibleFlag=false;
		}
	}

	if (m_boFilterShowUsedInLevelOnly)
	{
		if (!poDatabseItem->GetUsedInLevel())
		{
			boNextVisibleFlag=false;
		}
	}

	if (m_boShowIfHasAlphaChannel)
	{
		if (!poDatabseItem->GetHasAlphaChannelFlag())
		{
			boNextVisibleFlag=false;
		}
	}

	// If the width filtering is active...
	if (m_nFilterTextureWidthMinimum!=-1)
	{
		if (poDatabseItem->GetTextureWidth()<m_nFilterTextureWidthMinimum)
		{
			boNextVisibleFlag=false;
		}
	}

	// If the height filter is active...
	if (m_nFilterTextureHeightMinimum!=-1)
	{
		if (poDatabseItem->GetTextureHeight()<m_nFilterTextureHeightMinimum)
		{
			boNextVisibleFlag=false;
		}
	}

	// If the width filtering is active...
	if (m_nFilterTextureWidthMaximum!=-1)
	{
		if (poDatabseItem->GetTextureWidth()>m_nFilterTextureWidthMaximum)
		{
			boNextVisibleFlag=false;
		}
	}

	// If the height filter is active...
	if (m_nFilterTextureHeightMaximum!=-1)
	{
		if (poDatabseItem->GetTextureHeight()>m_nFilterTextureHeightMaximum)
		{
			boNextVisibleFlag=false;
		}
	}

	// TODO: Martin recommended us not to count on naming conventions...
	// ... so we should consider discarding this...
	// If we should display specular textures only...
	if (m_boFilterShowSpecularOnly)
	{
		poDatabseItem->GetFilename(strFilename);
		strFilename.MakeLower();

		// If it didn't find "spec" in the name, we assume it's not a specular texture.
		if (strFilename.find("spec")==strFilename.npos)
		{
			boNextVisibleFlag=false;
		}
	}

	// If we should display specular textures only...
	if (m_boFilterShowBumpOnly)
	{
		poDatabseItem->GetFilename(strFilename);
		strFilename.MakeLower();

		// If it didn't find "spec" in the name, we assume it's not a specular texture.
		if (strFilename.find("ddn")==strFilename.npos)
		{
			boNextVisibleFlag=false;
		}
	}

	if (m_boFilterShowCubemapOnly)
	{
		if (!poDatabseItem->GetIsCubemap())
		{
			boNextVisibleFlag=false;
		}
	}

	// TODO: Martin recommended us not to count on naming conventions...
	// ... so we should consider discarding this...
	// A texture will be considered diffuse if it is not of any other semantics.
	if (m_boFilterShowDiffuseOnly)
	{
		poDatabseItem->GetFilename(strFilename);
		strFilename.MakeLower();

		// If it didn't find "spec" in the name, we assume it's not a specular texture.
		if (strFilename.find("spec")!=strFilename.npos)
		{
			boNextVisibleFlag=false;
		}
		else if (strFilename.find("ddn")!=strFilename.npos)
		{
			boNextVisibleFlag=false;
		}
		else if (false) // TODO: implement the bump map testing here.
		{
			boNextVisibleFlag=false;
		}
	}

	// If we have something in the filename filter (and thus we must filter the filenames)...
	if (!m_strNameFilter.empty())
	{
		poDatabseItem->GetFilename(strFilename);
		poDatabseItem->GetRelativePath(strRelativePath);
		strRelativePath+=strFilename;
		strRelativePath.MakeLower();

		if (!PathUtil::MatchWildcard(strRelativePath.c_str(),m_strNameFilter.c_str()))
		{
			boNextVisibleFlag=false;
		}
	}

	// If the current visible flag is different of the next, we must
	// set it to the next flag.
	if (boCurrentVisibleFlag!=boNextVisibleFlag)
	{
		poDatabseItem->SetVisibleFlag(boNextVisibleFlag);
		// In case it's not visible, we will make it's position invalid.
		if (!boNextVisibleFlag)
		{
			CRect& roFilteredRect=poDatabseItem->GetDrawingRectangle();
			// An invalid rectangle outsite of the client area.... ths won't bother us...
			roFilteredRect.SetRect(-65535,-65535,-65535,-65535);
		}

		// Yes, the visible flag changed.
		return true;
	}	

	// No, the visible flag didn't change.
  return false;
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
bool CTextureViewer::CheckVirtualKey( int virtualKey )
{
	GetAsyncKeyState(virtualKey);
	if (GetAsyncKeyState(virtualKey) & (1<<15))
		return true;
	return false;
}
//////////////////////////////////////////////////////////////////////////
