#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   LensFlareManager.h
//  Created:     7/Dec/2012 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BaseLibraryManager.h"

class IOpticsElementBase;
class CLensFlareEditor;

class CRYEDIT_API CLensFlareManager : public CBaseLibraryManager
{
public:
	CLensFlareManager();
	virtual ~CLensFlareManager();

	void ClearAll();

	virtual bool LoadFlareItemByName( const CString &fullItemName, IOpticsElementBasePtr pDestOptics );
	void Modified();
	//! Path to libraries in this manager.	
	CString GetLibsPath();
	IDataBaseLibrary* LoadLibrary( const CString &filename, bool bReload = false );	

private:
	CBaseLibraryItem* MakeNewItem();
	CBaseLibrary* MakeNewLibrary();

	//! Root node where this library will be saved.
	CString GetRootNodeName();
	CString m_libsPath;	
};
