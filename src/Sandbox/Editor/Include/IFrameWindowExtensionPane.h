////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   IFrameWindowExtensionPane.h
//  Version:     v1.00
//  Created:     30/4/2009 by Timur.
//  Compilers:   Visual Studio.NET 2005
//  Description: Interface for extending frame windows with custom dock panes.
// -------------------------------------------------------------------------
//  History: 04/08/2008 - Created by Paulo Zaffari
//
////////////////////////////////////////////////////////////////////////////
#ifndef __IFrameWindowExtensionPane_h__
#define __IFrameWindowExtensionPane_h__

#pragma once

//////////////////////////////////////////////////////////////////////////
// Description
//    This interface is used in frame windows to add a new custom 
//    docking pane to then from the plugins.
//////////////////////////////////////////////////////////////////////////
struct __declspec( uuid("{991A2772-B5A1-440d-8D9C-B1CEA5F83A0A}") ) IFrameWindowExtensionPane : public IUnknown
{
	// Return name of the frame window this extension is designed for.
	virtual const char* SupportedFrameWindow() const = 0;

	// Creates a window for the extension docking pane.
	virtual CWnd* CreatePaneWindow( CWnd *pParentWnd ) = 0;

	//////////////////////////////////////////////////////////////////////////
	// optional callbacks that pane may want to handle.
	//////////////////////////////////////////////////////////////////////////
	virtual void OnAttachToDockingPane( CXTPDockingPane *pwndDockWindow,CWnd *pPaneWnd ) {};
	virtual void OnFileLoad( const char *filepath ) {};
	virtual void SaveFile( const char *filepath ) {};
	// Called to check if data in the pane needs to be saved.
	virtual bool IsModified() { return false; };
	// Called when view of the frame window is updating.
	virtual void OnViewUpdate() {};
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IUnknown
	//////////////////////////////////////////////////////////////////////////
	virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid,void** ppvObject ) { return E_NOINTERFACE; };
	virtual ULONG		STDMETHODCALLTYPE AddRef()	{ return 0; };
	virtual ULONG		STDMETHODCALLTYPE Release() { return 0; };
	//////////////////////////////////////////////////////////////////////////
};

#endif // __IFrameWindowExtensionPane_h__
