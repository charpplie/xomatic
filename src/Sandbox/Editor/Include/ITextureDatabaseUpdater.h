/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: ITextureDatabaseCreator.h  ,v 1.1 2008/08/28 14:35:22 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This file declares the interface used by the texture viewer
and (implemented first implemented by the Texture Database Creator) to
syncronize their threads. A thread interace could be useful there.
-------------------------------------------------------------------------
History:
- 28:08:2008   14:35 : Created by Paulo Zaffari

*************************************************************************/
#ifndef ITextureDatabaseUpdater_h__
#define ITextureDatabaseUpdater_h__

#pragma once

class CTextureDatabaseItem;

struct ITextureDatabaseUpdater
{
public:
	//////////////////////////////////////////////////////////////////////////
	// Thread control
	virtual void NotifyShutDown()=0;
	virtual void Lock()=0;
	virtual void Unlock()=0;
	virtual void WaitForThread()=0;
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Data access
	virtual CTextureDatabaseItem*	GetItem(const char* szAddItem)=0;
	//////////////////////////////////////////////////////////////////////////
};

#endif // ITextureDatabaseUpdater_h__
