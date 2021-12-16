/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Manager for deferred raycast wrappers.
The managers creates an interface between CryAction and other DLLs using the wrappers.

-------------------------------------------------------------------------
History:
- 02:2009 : Created By Jan Müller

*************************************************************************/

#include "StdAfx.h"
#include "DeferredRaycastManager.h"
#include "DeferredRaycastWrapper.h"

//statics
static CDeferredRaycastManager *g_pManagerInstance = NULL;

CDeferredRaycastManager * CDeferredRaycastManager::ref()
{
	if(!g_pManagerInstance)
		g_pManagerInstance = new CDeferredRaycastManager();
	return g_pManagerInstance;
}

CDeferredRaycastManager::CDeferredRaycastManager()
{
	gEnv->pPhysicalWorld->AddEventClient(EventPhysRWIResult::id, CDeferredRaycastWrapper::OnRwiResult, 1);
	gEnv->pPhysicalWorld->AddEventClient(EventPhysPWIResult::id, CDeferredRaycastWrapper::OnPwiResult, 1);
}

CDeferredRaycastManager::~CDeferredRaycastManager()
{
	gEnv->pPhysicalWorld->RemoveEventClient(EventPhysRWIResult::id, CDeferredRaycastWrapper::OnRwiResult, 1);
	gEnv->pPhysicalWorld->RemoveEventClient(EventPhysPWIResult::id, CDeferredRaycastWrapper::OnPwiResult, 1);

	TWrapperMap::iterator it = m_wrapperMap.begin();
	TWrapperMap::iterator end = m_wrapperMap.end();
	for(; it != end; ++it)
		SAFE_DELETE(it->second);

	m_wrapperMap.clear();
}

IDeferredRaycastWrapper *CDeferredRaycastManager::FindWrapper(const int iID)
{
	return stl::find_in_map(m_wrapperMap, iID, NULL);
}

IDeferredRaycastWrapper *CDeferredRaycastManager::GetWrapper(const int iID)
{
	//try to find the wrapper
	IDeferredRaycastWrapper *pWrapper = FindWrapper(iID);
	if(pWrapper)
		return pWrapper;

	//create new wrapper, add to list and return
	pWrapper = static_cast<IDeferredRaycastWrapper*>(new CDeferredRaycastWrapper());
	m_wrapperMap[pWrapper->GetId()] = pWrapper;

	return pWrapper;
}

bool CDeferredRaycastManager::RemoveWrapper(const int iID)
{
	TWrapperMap::iterator it = m_wrapperMap.find(iID);
	if(it != m_wrapperMap.end())
	{
		delete (it->second);
		m_wrapperMap.erase(it);
		return true;
	}

	return false;
}

void CDeferredRaycastManager::Reset()
{
	TWrapperMap::iterator it = m_wrapperMap.begin();
	TWrapperMap::iterator end = m_wrapperMap.end();
	for(; it != end; ++it)
		it->second->Reset();
}
