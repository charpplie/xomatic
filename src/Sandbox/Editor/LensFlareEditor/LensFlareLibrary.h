#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   LensFlareLibrary.h
//  Created:     12/Dec/2012 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#include "BaseLibrary.h"

class CRYEDIT_API CLensFlareLibrary : public CBaseLibrary
{
public:
	CLensFlareLibrary( CBaseLibraryManager *pManager ) : CBaseLibrary(pManager) {};
	virtual bool Save();
	virtual bool Load( const CString &filename );	
	virtual void Serialize( XmlNodeRef &node,bool bLoading );
	
	IOpticsElementBasePtr GetOpticsOfItem( const char* szflareName );
};