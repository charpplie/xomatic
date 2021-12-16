////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   CustomActionsEditorManager.h
//  Version:     v1.00
//  Created:     31/8/2011 by Dean Claassen.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CustomActionsEditorManager_h__
#define __CustomActionsEditorManager_h__

#if _MSC_VER > 1000
#pragma once
#endif

// Forward declarations.

//////////////////////////////////////////////////////////////////////////
// Custom Actions Editor Manager
//////////////////////////////////////////////////////////////////////////
class CCustomActionsEditorManager
{
public:
	CCustomActionsEditorManager();
	~CCustomActionsEditorManager();
	void Init( ISystem *system );

	void ReloadCustomActionGraphs();
	void SaveCustomActionGraphs();
	void SaveAndReloadCustomActionGraphs();
	bool NewCustomAction( CString& filename );
	void GetCustomActions( std::vector<CString> &values ) const;

private:
	void LoadCustomActionGraphs();
	void FreeCustomActionGraphs();
};

#endif // __CustomActionsEditorManager_h__