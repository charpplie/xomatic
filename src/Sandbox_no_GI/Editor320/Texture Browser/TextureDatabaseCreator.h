/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: TextureDatabaseCreator.h  ,v 1.1 2008/08/28 11:27:08 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This file declares the database creator for the texture viewer
which implements the ITextureDatabaseUpdater interface.
Objects of this class are responsible for updating the texture viewer with
textures in a thread based way.
-------------------------------------------------------------------------
History:
- 28:08:2008   11:27 : Created by Paulo Zaffari

*************************************************************************/

#ifndef TextureDatabaseCreator_h__
#define TextureDatabaseCreator_h__

#pragma once

#include "CryThread.h"
#include "..\Include\ITextureDatabaseUpdater.h"

class CTextureViewer;

class CTextureDatabaseCreator:public CryThread<CTextureDatabaseCreator>,virtual public ITextureDatabaseUpdater//,public IDataBaseManagerListener
{
private:
	typedef std::map<string,CTextureDatabaseItem*>									 TDFilenameToItemMap;	
	typedef std::pair<string,CTextureDatabaseItem*>									 TDInsertionElement;
public:
	CTextureDatabaseCreator();
	~CTextureDatabaseCreator();

	//////////////////////////////////////////////////////////////////////////
	// From ITextureDatabaseUpdater.
	// Unfortunately had to redefine this methods here as there is no 
	// CryThreadInteface
	void NotifyShutDown(); 
	void Lock();
	void Unlock();
	void WaitForThread();

	CTextureDatabaseItem*	GetItem(const char* szAddItem);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// From CryThread<CTextureDatabaseCreator>
	void Run();
	//////////////////////////////////////////////////////////////////////////

	bool SetAssociatedViewer(CTextureViewer* poAssociatedViewer);

	void GatherPreloadedTextures();
	void AddPreLoadedTextures(TDFilenameToItemMap& rcFilenameSet);
	void AddDiskTexture( TDFilenameToItemMap &rcFilenameSet );

	// For future use, in case we wish any sort of hot update of items... as it will
	// be not so hard to implement.
	//void OnDataBaseItemEvent( IDataBaseItem *pItem,EDataBaseItemEvent event );
protected:
	CTextureViewer*							m_poAssociatedViewer;

	// Used to avoid adding a same texture multiple times.
	// Tip: change this into a map from filename to index to hot update textures.
	TDFilenameToItemMap								m_cKnwonTextures;
	volatile bool											m_boMustEndThread;
	CryMutex													m_oAddItemMutex;
	std::set<string>									m_cPreloadedTexturesFilenames;
};

#endif // TextureDatabaseCreator_h__