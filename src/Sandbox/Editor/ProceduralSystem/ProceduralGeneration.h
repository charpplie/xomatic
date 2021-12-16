///////////////////////////////////////////////
// Procedural Buildings Generation Class
#ifndef __proceduralgeneration_h__
#define __proceduralgeneration_h__
#pragma once

#include "..\ProceduralCreationDialog.h"
#include "..\ProceduralSystem/ProceduralCreation.h"
#include "..\Objects/PrefabBuildingObject.h"

// Maximum acceptable space between bounding boxes of neighbor modules
#define MAX_BBOX_SPACE 1.0f

enum ProcObjectTypes
{
	eExt_CornerIn=0,
	eExt_CornerOut,
	eExt_Roof,
	eExt_RoofCornerIn,
	eExt_RoofCornerOut,
	eExt_ShopWall,
	eExt_Wall,
	eExt_WallDoor,
	eExt_WallWindow,
	eExtLast
};

enum AxisDirs
{
	eDirUp=0,
	eDirDown,
	eDirLeft,
	eDirRight,
	eDirLast
};

class CProceduralModule;
class CProceduralProductionUtils;
class CProceduralProductionRuleBase;
class CProceduralProductionRuleBaseSimple;
class CPrefabBuildingObject;

typedef std::vector<CProceduralModule>	TModulesList;
typedef	TModulesList::iterator			TModulesListIt;

/////////////////////////////////////////////////////////////////////////
class CProceduralModuleLink
{
public:
	CProceduralModuleLink();

	CProceduralModule *m_pLink;
	Vec3 m_vDisplacement;
};

typedef std::vector<CProceduralModuleLink>	lstLink;
typedef lstLink::iterator					lstLinkIt;

//////////////////////////////////////////////////////////////////////////
class CProceduralModule
{
public:
	CProceduralModule();

	CBrushObject *m_pObject;
	ProcObjectTypes eType;	
	lstLink m_Links;
	AABB m_bbox;
};

//////////////////////////////////////////////////////////////////////////
class CProceduralProductionUtils
{
public:
	float	GetBBoxSquareDistance(const AABB &bbox,const Vec3 &vPos);
	void	GenerateBuilding();
	ProcObjectTypes	GetObjectType(CString CGFPath);
	CString GetGeometryString(CBrushObject *brushObject);
	void CopyModule(CProceduralModule &sourceModule,CProceduralModule &destinationModule, bool copyObject);
	
	Vec3 GetRandomBuildingDisplacement() { return m_vBuildingDisplacement;};
	void SetRandomBuildingDisplacement(){ m_vBuildingDisplacement = Vec3(-10 + rand()%20,-10 + rand()%20,0);};

protected: 
	void GenerateFloor(TModulesList &modulesList, CProceduralProductionRuleBase *pRule);
	Vec3 m_vBuildingDisplacement;

};
//////////////////////////////////////////////////////////////////////////
#endif