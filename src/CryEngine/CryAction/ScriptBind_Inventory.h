/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Script Binding for Inventory

-------------------------------------------------------------------------
History:
- 4:9:2005   15:29 : Created by Márcio Martins

*************************************************************************/
#ifndef __SCRIPTBIND_INVENTORY_H__
#define __SCRIPTBIND_INVENTORY_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include <IScriptSystem.h>
#include <ScriptHelpers.h>


struct IItemSystem;
struct IGameFramework;
class CInventory;


class CScriptBind_Inventory :
	public CScriptableBase
{
public:
	CScriptBind_Inventory(ISystem *pSystem, IGameFramework *pGameFramework);
	virtual ~CScriptBind_Inventory();
	void Release() { delete this; };
	void AttachTo(CInventory *pItem);

	// <title Release>
	// Syntax: Inventory.Release(ScriptHandle id)
	// Arguments:
	//		id - Item identifier.
	// Description:
	//		Adds an item to the inventory.
	int AddItem(IFunctionHandler *pH, ScriptHandle id);
	// <title RemoveItem>
	// Syntax: Inventory.RemoveItem(ScriptHandle id)
	// Arguments:
	//		id - Item identifier.
	// Description:
	//		Removes an item from the inventory.
	int RemoveItem(IFunctionHandler *pH, ScriptHandle id);

	// <title Validate>
	// Syntax: Inventory.Validate()
	// Description:
	//		Validates the inventory.
	int Validate(IFunctionHandler *pH);
	// <title Destroy>
	// Syntax: Inventory.Destroy()
	// Description:
	//		Destroys the inventory.
	int Destroy(IFunctionHandler *pH);
	// <title Clear>
	// Syntax: Inventory.Clear()
	// Description:
	//		Clears the inventory.
	int Clear(IFunctionHandler *pH);

	// <title Dump>
	// Syntax: Inventory.Dump()
	// Description:
	//		Dumps the inventory.
	int Dump(IFunctionHandler *pH);

	// <title SetCapacity>
	// Syntax: Inventory.SetCapacity( int size )
	// Arguments:
	//		size - Inventory capacity.
	// Description:
	//		Sets the inventory capacity.
	int SetCapacity(IFunctionHandler *pH, int size);
	// <title GetCapacity>
	// Syntax: Inventory.GetCapacity()
	// Description:
	//		Gets the inventory capacity.
	int GetCapacity(IFunctionHandler *pH);

	// <title GetCount>
	// Syntax: Inventory.GetCount()
	// Description:
	//		Gets the number of the inventory elements.
	int GetCount(IFunctionHandler *pH);
	// <title GetCountOfClass>
	// Syntax: Inventory.GetCountOfClass()
	// Arguments:
	//		className - Object class name.
	// Description:
	//		Gets the number of element of the specified class inside the inventory.
	int GetCountOfClass(IFunctionHandler *pH, const char *className);

	// <title GetItem>
	// Syntax: Inventory.GetItem( int slotId )
	// Description:
	//		Gets the item in the specified slot.
	int GetItem(IFunctionHandler *pH, int slotId);
	// <title GetItemByClass>
	// Syntax: Inventory.GetItemByClass( const char *className )
	int GetItemByClass(IFunctionHandler *pH, const char *className);

	// <title GetGrenadeWeaponByClass>
	// Syntax: Inventory.GetGrenadeWeaponByClass( const char *className )
	// Arguments:
	//		className - Class name.
	// Description:
	//		Gets grenade weapon by class name.
	int GetGrenadeWeaponByClass(IFunctionHandler *pH, const char *className);
	
	// <title HasAccessory>
	// Syntax: Inventory.HasAccessory( const char *accessoryName )
	// Arguments:
	//		accessoryName - Accessory name.
	// Description:
	//		Checks if the inventory contains the specified accessory.
	int HasAccessory(IFunctionHandler *pH, const char *accessoryName);

	// <title FindItem>
	// Syntax: Inventory.FindItem( ScriptHandle itemId )
	// Arguments:
	//		itemId - Item identifier.
	// Description:
	//		Finds an item from its identifier.
	int FindItem(IFunctionHandler *pH, ScriptHandle itemId);

	// <title FindNext>
	// Syntax: Inventory.FindNext( const char *className, const char *category,	int firstSlot, bool wrap )
	// Arguments:
	//		className	- Class name.
	//		category	- Category name.
	//		firstSlot	- Number of the first slot.
	//		wrap		- True to wrap, false otherwise.
	// Description:
	//		Search for the specified class and return the next one in the inventory with the specific info.
	int FindNext(IFunctionHandler *pH, const char *className, const char *category,	int firstSlot, bool wrap);
	// <title FindNext>
	// Syntax: Inventory.FindNext( const char *className, const char *category,	int firstSlot, bool wrap )
	// Arguments:
	//		className	- Class name.
	//		category	- Category name.
	//		firstSlot	- Number of the first slot.
	//		wrap		- True to wrap, false otherwise.
	// Description:
	//		Search for the specified class and return the previous one in the inventory with the specific info.
	int FindPrev(IFunctionHandler *pH, const char *className, const char *category, int firstSlot, bool wrap);

	// <title GetCurrentItemId>
	// Syntax: Inventory.GetCurrentItemId()
	// Description:
	//		Gets the identifier of the current item.
	int GetCurrentItemId(IFunctionHandler *pH);
	// <title GetCurrentItem>
	// Syntax: Inventory.GetCurrentItem()
	// Description:
	//		Gets the current item.
	int GetCurrentItem(IFunctionHandler *pH);
	// <title GetCurrentItem>
	// Syntax: Inventory.GetCurrentItem( ScriptHandle itemId )
	// Description:
	//		Sets the specified item as the current one.
	int SetCurrentItemId(IFunctionHandler *pH, ScriptHandle itemId);

	// <title SetLastItemId>
	// Syntax: Inventory.SetLastItemId( ScriptHandle itemId )
	// Description:
	//		Sets the specified item as the last used one.
	int SetLastItemId(IFunctionHandler *pH, ScriptHandle itemId);
	// <title GetLastItemId>
	// Syntax: Inventory.GetLastItemId()
	// Description:
	//		Gets the identifier for the last used item.
	int GetLastItemId(IFunctionHandler *pH);
	// <title GetLastItem>
	// Syntax: Inventory.GetLastItem()
	// Description:
	//		Gets the last used item.
	int GetLastItem(IFunctionHandler *pH);

	// <title SetAmmoCount>
	// Syntax: Inventory.SetAmmoCount( const char *ammoName, int count )
	// Arguments:
	//		ammoName - Ammunition name.
	//		count	 - Ammunition amount.
	// Description:
	//		Sets the amount of the specified ammunition.
	int SetAmmoCount(IFunctionHandler *pH, const char *ammoName, int count);
	// <title GetAmmoCount>
	// Syntax: Inventory.GetAmmoCount()
	// Arguments:
	//		ammoName - Ammunition name.
	// Description:
	//		Gets the amount of the specified ammunition name.
	int GetAmmoCount(IFunctionHandler *pH, const char *ammoName);
	// <title SetAmmoCapacity>
	// Syntax: Inventory.SetAmmoCapacity( const char *ammoName, int max )
	// Arguments:
	//		ammoName - Ammunition name.
	//		max		 - Ammunition capacity.
	// Description:
	//		Sets the specific ammunition capacity.
	int SetAmmoCapacity(IFunctionHandler *pH, const char *ammoName, int max);
	// <title GetAmmoCapacity>
	// Syntax: Inventory.GetAmmoCapacity( const char *ammoName )
	// Arguments:
	//		ammoName - Ammunition name.
	// Description:
	//		Gets the capacity for the specified ammunition.
	int GetAmmoCapacity(IFunctionHandler *pH, const char *ammoName);
	// <title ResetAmmo>
	// Syntax: Inventory.ResetAmmo()
	// Description:
	//		Resets ammunition.
	int ResetAmmo(IFunctionHandler *pH);
	// <title ReloadWeapons>
	// Syntax: Inventory.ReloadWeapons()
	// Description:
	//		Reloads weapons.
	int ReloadWeapons(IFunctionHandler* pH);

	// <title GetInventoryTable>
	// Syntax: Inventory.GetInventoryTable()
	// Description:
	//		Gets the inventory table.
	int GetInventoryTable(IFunctionHandler *pH);
private:
	void RegisterGlobals();
	void RegisterMethods();

	CInventory *GetInventory(IFunctionHandler *pH);

	ISystem					*m_pSystem;
	IEntitySystem		*m_pEntitySystem;
	IGameFramework	*m_pGameFramework;
};


#endif //__SCRIPTBIND_INVENTORY_H__
