/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: 
  
 -------------------------------------------------------------------------
  History:
  - 30:9:2004   14:21 : Created by Márcio Martins

*************************************************************************/
#ifndef __SCRIPTBIND_ITEMSYSTEM_H__
#define __SCRIPTBIND_ITEMSYSTEM_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IScriptSystem.h>
#include <ScriptHelpers.h>

// <title ItemSystem>
// Syntax: ItemSystem
class CItemSystem;
class CEquipmentManager;

class CScriptBind_ItemSystem :
	public CScriptableBase
{
public:
	CScriptBind_ItemSystem(ISystem *pSystem, CItemSystem *pItemSystem, IGameFramework *pGameFramework);
	virtual ~CScriptBind_ItemSystem();

	void Release() { delete this; };

	// <title Reset>
	// Syntax: ItemSystem.Reset()
	// Description:
	//		Resets the item system.
	int Reset(IFunctionHandler *pH);

	// <title GiveItem>
	// Syntax: ItemSystem.GiveItem( const char *itemName )
	// Arguments:
	//		itemName - Item name.
	// Description:
	//		Gives the specified item.
	int GiveItem(IFunctionHandler *pH, const char *itemName);
	// <title GiveItemPack>
	// Syntax: ItemSystem.GiveItemPack( ScriptHandle actorId, const char *packName )
	// Arguments:
	//		actorId  - Actor identifier.
	//		packName - Pack name.
	// Description:
	//		Gives the item pack to the specified actor.
	int GiveItemPack(IFunctionHandler *pH, ScriptHandle actorId, const char *packName);
	// <title GetPackPrimaryItem>
	// Syntax: ItemSystem.GetPackPrimaryItem( const char *packName )
	// Arguments:
	//		packName - Pack name.
	// Description:
	//		Gets the primary item of the specified pack.
	int GetPackPrimaryItem(IFunctionHandler *pH, const char *packName);
	// <title Reset>
	// Syntax: ItemSystem.Reset()
	int GetPackNumItems(IFunctionHandler *pH, const char* packName);
	// <title GetPackItemByIndex>
	// Syntax: ItemSystem.GetPackItemByIndex( const char *packName, int index)
	// Arguments:
	//		packName - Pack name.
	//		index	 - Pack index.
	// Description:
	//		Gets a pack item from its index.
	int GetPackItemByIndex(IFunctionHandler *pH, const char *packName, int index);
	// <title SetActorItem>
	// Syntax: ItemSystem.SetActorItem( ScriptHandle actorId, ScriptHandle itemId, bool keepHistory )
	// Arguments:
	//		actorId - Actor identifier.
	//		itemId	- Item identifier.
	//		keepHistory - True to keep history, false otherwise.
	// Description:
	//		Sets an actor item.
	int SetActorItem(IFunctionHandler *pH, ScriptHandle actorId, ScriptHandle itemId, bool keepHistory);
	// <title SetActorItemByName>
	// Syntax: ItemSystem.SetActorItemByName( ScriptHandle actorId, const char *name, bool keepHistory)
	// Arguments:
	//		actorId - Actor identifier.
	//		name	- Actor item name.
	//		keepHistory - True to keep history, false otherwise.
	// Description:
	//		Sets an actor item by name.
	int SetActorItemByName(IFunctionHandler *pH, ScriptHandle actorId, const char *name, bool keepHistory);
	// <title SerializePlayerLTLInfo>
	// Syntax: ItemSystem.SerializePlayerLTLInfo( bool reading )
	// Arguments:
	//		reading - Boolean value.
	// Description:
	//		Serializes player LTL info.
	int SerializePlayerLTLInfo(IFunctionHandler *pH, bool reading);

private:
	void RegisterGlobals();
	void RegisterMethods();

	CItemSystem			  *m_pItemSystem;
	IGameFramework	  *m_pGameFramework;
  CEquipmentManager *m_pEquipmentManager;
};


#endif //__SCRIPTBIND_ITEMSYSTEM_H__
