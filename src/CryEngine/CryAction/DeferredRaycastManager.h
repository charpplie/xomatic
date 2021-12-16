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

#ifndef DEFERRED_RAYCAST_MANAGER
#define DEFERRED_RAYCAST_MANAGER

#include "IDeferredRaycastManager.h"

//fwd decl
class CDeferredRaycastWrapper;

class CDeferredRaycastManager : public IDeferredRaycastManager
{
public:
	//get singleton instance
	static CDeferredRaycastManager *ref();

	//IDeferredRaycastManager
	//Find an existing wrapper
	virtual IDeferredRaycastWrapper *FindWrapper(const int iID);

	//Get/Create a raycast wrapper
	//if the iID is not found, a new one is created
	virtual IDeferredRaycastWrapper *GetWrapper(const int iID);

	//remove an existing wrapper
	virtual bool RemoveWrapper(const int iID = -1);

	//reset all wrappers
	virtual void Reset();
	//~IDeferredRaycastManager

private:
	//singleton
	CDeferredRaycastManager();
	~CDeferredRaycastManager();

	typedef std::map<int, IDeferredRaycastWrapper *> TWrapperMap;
	TWrapperMap m_wrapperMap;
};

#endif