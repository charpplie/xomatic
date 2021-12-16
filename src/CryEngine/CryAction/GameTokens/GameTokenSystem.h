////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2005.
// -------------------------------------------------------------------------
//  File name:   GameTokenSystem.h
//  Version:     v1.00
//  Created:     20/10/2005 by Craig,Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef _GameTokenSystem_h_
#define _GameTokenSystem_h_
#pragma once

#include "IGameTokens.h"
#include "StlUtils.h"

class CGameToken;

//////////////////////////////////////////////////////////////////////////
// Game Token Manager implementation class.
//////////////////////////////////////////////////////////////////////////
class CGameTokenSystem : public IGameTokenSystem
{
	friend class CGameTokenIterator;
public:
	CGameTokenSystem();
	~CGameTokenSystem();

	//////////////////////////////////////////////////////////////////////////
	// IGameTokenSystem
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL IGameToken* SetOrCreateToken( const char *sTokenName,const TFlowInputData &defaultValue );
	VIRTUAL void DeleteToken( IGameToken* pToken );
	VIRTUAL IGameToken* FindToken( const char *sTokenName );
	VIRTUAL void RenameToken( IGameToken *pToken,const char *sNewName );

	VIRTUAL IGameTokenIt *GetGameTokenIterator();

	VIRTUAL void RegisterListener( const char *sGameToken,IGameTokenEventListener *pListener,bool bForceCreate,bool bImmediateCallback );
	VIRTUAL void UnregisterListener( const char *sGameToken,IGameTokenEventListener *pListener );

	VIRTUAL void RegisterListener( IGameTokenEventListener *pListener );
	VIRTUAL void UnregisterListener( IGameTokenEventListener *pListener );

	VIRTUAL void Reset();
	VIRTUAL void Serialize( TSerialize ser );

	VIRTUAL void DebugDraw();

 	VIRTUAL void LoadLibs( const char *sFileSpec );
	VIRTUAL void RemoveLibrary( const char *sPrefix); 

	VIRTUAL void GetMemoryStatistics(ICrySizer * s);
	//////////////////////////////////////////////////////////////////////////

	CGameToken* GetToken( const char *sTokenName );
	void Notify( EGameTokenEvent event,CGameToken *pToken );
	void DumpAllTokens();

	static int sDebug;

private:
	bool _InternalLoadLibrary( const char *filename, const char* tag );

	// Use Hash map for speed.
	typedef stl::hash_map<const char*,CGameToken*,stl::hash_stricmp<const char*> > GameTokensMap;

	typedef std::vector<IGameTokenEventListener*> Listeners;
	Listeners m_listeners;

	// Loaded libraries.
	std::vector<string> m_libraries;

	GameTokensMap m_gameTokensMap;
	class CGameTokenScriptBind* m_pScriptBind;

	static int sShow;
};

#endif // _GameTokenSystem_h_
