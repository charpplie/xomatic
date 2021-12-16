////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 2010.
// -------------------------------------------------------------------------
//  File name:   ResourceManager.cpp
//  Version:     v1.00
//  Created:     8/02/2010 by Timur.
//  Description: Interface to the Resource Manager
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef _CRY_RESOURCE_MANAGER_H_
#define _CRY_RESOURCE_MANAGER_H_

#pragma once

#include <IResourceManager.h>

//////////////////////////////////////////////////////////////////////////
// IResource manager interface
//////////////////////////////////////////////////////////////////////////
class CResourceManager : public IResourceManager, public ISystemEventListener
{
public:
	void Init();
	void Shutdown();

	//////////////////////////////////////////////////////////////////////////
	// IResourceManager interface implementation.
	//////////////////////////////////////////////////////////////////////////
	void PrepareLevel( const char *sLevelFolder,const char *sLevelName );
	IResourceList* GetLevelResourceList();
	bool LoadLevelCachePak( const char *sPakName );
	void UnloadLevelCachePak( const char *sPakName );
	void UnloadAllLevelCachePaks();
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// ISystemEventListener interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam );
	//////////////////////////////////////////////////////////////////////////

	// Per frame update of the resource manager.
	void Update();

	CryPathString GetCurrentLevelCacheFolder() const { return m_currentLevelCacheFolder; };

private:
	CryPathString m_currentLevelCacheFolder;
	std::vector<string> m_openedPaks;
};

#endif //_CRY_RESOURCE_MANAGER_H_