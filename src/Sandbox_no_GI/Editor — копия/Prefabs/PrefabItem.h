////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   PrefabItem.h
//  Version:     v1.00
//  Created:     10/11/2003 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __PrefabItem_h__
#define __PrefabItem_h__
#pragma once

#include "BaseLibraryItem.h"
#include <I3dEngine.h>

class CPrefabObject;
class CPrefabBuildingObject;

/*! CPrefabItem contain definition of particle system spawning parameters.
 *	
 */
class CRYEDIT_API CPrefabItem : public CBaseLibraryItem
{
public:
	CPrefabItem();
	~CPrefabItem();

	virtual EDataBaseItemType GetType() const { return EDB_TYPE_PREFAB; };

	void Serialize( SerializeContext &ctx );

	//////////////////////////////////////////////////////////////////////////
	// Make prefab from selection of objects.
	void MakeFromSelection( CSelectionGroup &selection );

	//////////////////////////////////////////////////////////////////////////
	void UpdateFromPrefabObject( CPrefabObject *pPrefabObject  );

	//Procedural Creation
	void UpdateFromPrefabBuildingObject( CPrefabBuildingObject *pPrefabObject  );

	//! Called after particle parameters where updated.
	void Update();
	//! Returns xml node containing prefab objects.
	XmlNodeRef GetObjectsNode() { return m_objectsNode; };
	CString GetPrefabObjectClassName() { return m_PrefabClassName;};
	void SetPrefabClassName(CString prefabClassNameString); 

	//////////////////////////////////////////////////////////////////////////
	// Procedural Floor Management.
	//////////////////////////////////////////////////////////////////////////
	int GetFloorCount()	{ return m_floors.size();};
	float GetFloorHeight(int floorNumber){	return m_floors[floorNumber]; };
	void SetFloorHeight(float floorHeight, int floorNumber) { m_floors[floorNumber]=floorHeight; };
	void SetFloorHeightUserDefined(int floorNumber, bool floorHasUserDefinedHeight) { m_floorHeightMode[floorNumber]=floorHasUserDefinedHeight;};
	bool IsFloorHeightDefinedByUser(int floorNumber) { return m_floorHeightMode[floorNumber]; };
	void DeleteFloor(int floorNumber) { if (floorNumber>0) m_floors.erase(floorNumber); };
	//////////////////////////////////////////////////////////////////////////

private:
	//Vec3 m_origin;
	XmlNodeRef m_objectsNode;
	CString m_PrefabClassName;

	// Procedural Creation code
	void LoadFloorHeights( SerializeContext &ctx);
	void SaveFloorHeights( SerializeContext &ctx);
	typedef std::map<int,float> TFloorHeightMap;
	typedef std::map<int,bool> TFloorHeightUserDefinedMap;

	TFloorHeightMap m_floors;
	TFloorHeightUserDefinedMap m_floorHeightMode;
};

#endif // __PrefabItem_h__


