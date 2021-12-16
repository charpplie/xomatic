/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: TextureViewer.h  ,v 1.1 2008/08/19 12:54:41 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This file declares a control which objective is to display 
multiple assets allowing selection and preview of such things.
It also handles scrolling and changes in the thumbnail display size.
-------------------------------------------------------------------------
History:
- 19:08:2008   12:54 : Created by Paulo Zaffari
- 11:03:2010   16:10 : Nicusor Nedelcu - refactored

*************************************************************************/
#ifndef __IAssetViewer_H__
#define __IAssetViewer_H__
#pragma once

struct IAssetDisplay;
struct IAssetDisplayDatabase;

struct IAssetViewerStatusDisplay
{
	virtual void OnChangeStatusBarInfo( UINT nSelectedItems, UINT nVisibleItems, UINT nTotalItems ) = 0;
	virtual void OnSelectionChanged() = 0;
};

struct __declspec( uuid("{157DCA62-4D7E-44b1-9782-0B7B4B6F2BB6}") ) IAssetViewer : public IUnknown
{
	typedef std::vector<IAssetDisplay*> 				TAssetItems;
	typedef Functor0														TDoubleClickCallback;
	typedef std::vector<IAssetDisplayDatabase*>	TAssetDatabases;

	virtual HWND	GetRenderWindow() = 0;
	virtual void	SetStatusDisplay( IAssetViewerStatusDisplay* pObserver ) = 0;
	virtual IAssetViewerStatusDisplay*	GetStatusDisplay() = 0;
	//! this will push the asset into the cache for thumbnail, and if it will not fit, then it will discard/uncache the oldest asset thumbnail
	virtual void	PushToThumbsCacheQueue( IAssetDisplay* pAsset ) = 0;
};

#endif // __IAssetViewer_H__