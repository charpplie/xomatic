////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 2010.
// -------------------------------------------------------------------------
//  File name:   IResourceManager.h
//  Version:     v1.00
//  Created:     8/02/2010 by Timur.
//  Description: Interface to the Resource Manager
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef _CRY_IRESOURCE_MANAGER_H_
#define _CRY_IRESOURCE_MANAGER_H_

#pragma once

//////////////////////////////////////////////////////////////////////////
// IResource manager interface
//////////////////////////////////////////////////////////////////////////
struct IResourceManager
{
	// Called by level system to set the level folder
	virtual void PrepareLevel( const char *sLevelFolder,const char *sLevelName ) = 0;
	// Call to get current level resource list.
	virtual IResourceList* GetLevelResourceList() = 0;
	// Load pak file from level cache to memory.
	virtual bool LoadLevelCachePak( const char *sPakName ) = 0;
	// Unloads level cache pak file from memory.
	virtual void UnloadLevelCachePak( const char *sPakName ) = 0;
	// Unload all level cache paks from memory.
	virtual void UnloadAllLevelCachePaks() = 0;
};

#endif //_CRY_IRESOURCE_MANAGER_H_