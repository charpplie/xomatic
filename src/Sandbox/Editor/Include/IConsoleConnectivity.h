////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   IConsole.h
//  Version:     v1.00
//  Created:     4/8/2008 by Paulo Zaffari.
//  Compilers:   Visual Studio.NET 2005
//  Description: Standard interface for console connectivity plugins.
// -------------------------------------------------------------------------
//  History: 04/08/2008 - Created by Paulo Zaffari
//
////////////////////////////////////////////////////////////////////////////
#ifndef IConsoleConnectivity_h__
#define IConsoleConnectivity_h__

#pragma once

//////////////////////////////////////////////////////////////////////////
// Description
//    This interface provide access to the console connectivity 
// functionality.
//////////////////////////////////////////////////////////////////////////
struct __declspec( uuid("{4DAA85E1-8498-402f-9B85-7F629D76798A}") ) IConsoleConnectivity : public IUnknown
{
	//////////////////////////////////////////////////////////////////////////
	//TODO: Must add the useful interface here.
	//////////////////////////////////////////////////////////////////////////

	// Description:
	//   Checks if a development console is connected to the development PC.
	// See Also: 
	// Arguments:
	//   Nothing
	// Return:
	//   bool - true if it is connected, false otherwise.
	virtual bool IsConnectedToConsole()=0;

	// Description:
	//   Send a file from the specified local filename to the console platform creating the full path
	// as required so it can copy to the remote filename.
	// See Also: 
	//   Nothing
	// Arguments:
	//   szLocalFileName - is the local filename from which you want to copy the file.
	//	 szRemoteFilename - is the full path and filename to where you want to copy the file.
	// Return:
	//   bool - true if the copy succeeded, false otherwise.
	virtual bool SendFile(const char* szLocalFileName,const char* szRemoteFilename)=0;

	// Description:
	//   Notifies to the console that a file has been changed, typically uploaded.
	// This will be usually called after a SendFile (see above) call, so that the
	// system running on the console may decide what to do with this new file.
	//   Typically the system will have to load or reloads this new file.
	// See Also: 
	//   SendFile
	// Arguments:
	//	 szRemoteFilename - is the full path and filename in the console of the changed
	// file.
	// Return:
	//   bool - true if succeeded sending the notification, false otherwise.
	virtual bool NotifyFileChange(const char* szRemoteFilename)=0;


	// Description:
	// Gets the the title IP for the connected XBOX 360 console .
	// Arguments:
	//	dwConsoleAddressPlaceholder - is the pointer to the placeholder of the variable
	// which will contain the title IP of the console.
	// Return:
	//   bool - true if dwConsoleAddressPlaceholder now contains the IP address, else false.
	virtual bool GetConsoleAddress(DWORD* dwConsoleAddressPlaceholder)=0;
	//////////////////////////////////////////////////////////////////////////
	// IUnknown
	//////////////////////////////////////////////////////////////////////////
	virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid,void** ppvObject ) { return E_NOINTERFACE; };
	virtual ULONG		STDMETHODCALLTYPE AddRef()	{ return 0; };
	virtual ULONG		STDMETHODCALLTYPE Release() { return 0; };
	//////////////////////////////////////////////////////////////////////////
};

#endif // IConsoleConnectivity_h__
