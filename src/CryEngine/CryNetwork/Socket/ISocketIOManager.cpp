#include "StdAfx.h"
#include "ISocketIOManager.h"
#include "SocketIOManagerIOCP.h"
#include "SocketIOManager360.h"
#include "SocketIOManagerNull.h"
#include "SocketIOManagerSelect.h"

ISocketIOManager * CreateSocketIOManager( int ncpus )
{
#if defined(HAS_SOCKETIOMANAGER_360)
	CSocketIOManager360 * pMgr360 = new CSocketIOManager360();
	if (pMgr360->Init())
		return pMgr360;
	delete pMgr360;
#endif
#if defined(HAS_SOCKETIOMANAGER_IOCP)
	if (ncpus >= 2 || gEnv->pSystem->IsDedicated())
	{
		CSocketIOManagerIOCP * pMgr = new CSocketIOManagerIOCP();
		if (pMgr->Init())
			return pMgr;
		delete pMgr;
	}
#endif
#if defined(HAS_SOCKETIOMANAGER_SELECT)
	CSocketIOManagerSelect * pSelectMgr = new CSocketIOManagerSelect();
	if (pSelectMgr->Init())
		return pSelectMgr;
	delete pSelectMgr;
#endif
	return new CSocketIOManagerNull();
}
