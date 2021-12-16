#include "stdafx.h"
#include "ProceduralSystem/ProceduralCreation.h"
#include "ProceduralSystem/ProceduralGeneration.h"
#include "ProceduralSystem/ProceduralProductionRuleBaseSimple.h"

CProceduralModule::CProceduralModule()
{
	m_pObject =0;
}
CProceduralModuleLink::CProceduralModuleLink()
{
	m_pLink=0;
}

ProcObjectTypes CProceduralProductionUtils::GetObjectType(CString CGFPath)
{
	std::vector<CString> moduleTypeKeywords;
	moduleTypeKeywords.resize(eExtLast);
	moduleTypeKeywords[eExt_CornerIn]=("_cornerin_");
	moduleTypeKeywords[eExt_CornerOut] = ("_cornerout_");
	moduleTypeKeywords[eExt_Roof]=("_roof_");
	moduleTypeKeywords[eExt_RoofCornerIn]=("_roofcornerin_");
	moduleTypeKeywords[eExt_RoofCornerOut] = ("_roofcornerout_");
	moduleTypeKeywords[eExt_ShopWall] = ("_shopwall_");
	moduleTypeKeywords[eExt_Wall] = ("_wall_");
	moduleTypeKeywords[eExt_WallDoor] = ("_door_");
	moduleTypeKeywords[eExt_WallWindow] = ("_wallwin_");

	for (int i=0;i<eExtLast;++i)
		if (CGFPath.Find(moduleTypeKeywords[i])!=-1)
			return (ProcObjectTypes)i;

	return eExtLast;
}

//////////////////////////////////////////////////////////////////////////
void CProceduralProductionUtils::GenerateBuilding()
{
	if (!CProcSettings::GetProcSettings().buildingSelected)
		return;

	CPrefabBuildingObject *building = (CPrefabBuildingObject*) GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building)
		return;

	int lastFloor = CProcSettings::GetProcSettings().currentFloorNumber;
	CProceduralCreationWorks::ChangeToFloor(0);

	GetIEditor()->ClearSelection();

	TModulesList floorObjects;
	CString selGeomFile("");
	CString outLogText("");

	// random building placement
	CProceduralProductionUtils procUtils;

	// decide the rule
	CProceduralProductionRuleBaseSimple pRule;


	// Iterate through floors
	for (int floorID=0; floorID<building->GetFloorCount(); ++floorID)
	{
		floorObjects.clear();

		CProceduralCreationWorks::ChangeToFloor(floorID);

		// Iterate through children on floor and collect modules
		for (int i=0 ; i< building->GetChildCount(); ++i)
		{
			CBrushObject *childObject = (CBrushObject*)building->GetChild(i);

			// ignore object if it is not a CBrushObject
			if (!childObject->IsKindOf(RUNTIME_CLASS(CBrushObject)))
				continue;

			// ignore object if not from the current floor;
			if (childObject->GetFloorNumber()!=floorID)
				continue;
			
			CString objGeomName = GetGeometryString(childObject);

			// Create a new module and add it to the list of modules on the current floor
			CProceduralModule newModule;
			newModule.m_pObject=childObject;

			AABB objBbox;
			childObject->GetBoundBox(objBbox);

			// Possibly extend the bounding box by pivot point
			objBbox.Add(childObject->GetWorldPos());

			// Ignore object if it does not represent the defined geometry types
			ProcObjectTypes objType = GetObjectType(objGeomName);
			if (objType==-1)
				continue;

			newModule.eType = objType;
			newModule.m_bbox = objBbox;
			floorObjects.push_back(newModule);
		}


		// Iterate through modules
		// Pick first module and start finding modules around it.
		for (TModulesListIt i1=floorObjects.begin(); i1!=floorObjects.end();++i1)		
		{	
			CProceduralModule &pModule1=(*i1);

			const Vec3 vModuleCenter1 = pModule1.m_bbox.GetCenter();

			Vec3 vModuleMin1 = pModule1.m_bbox.min;			
			Vec3 vModuleMax1 = pModule1.m_bbox.max;			

			vModuleMin1.z=vModuleMax1.z=0;
			float fMaxExtentSquare=vModuleMin1.GetSquaredDistance(vModuleMax1);

			// Iterate through modules
			// Pick second module and add as a link of first module if in defined range
			for (TModulesListIt i2=floorObjects.begin(); i2!= floorObjects.end();++i2)		
			{
				// skip the same object
				if (i1==i2)
					continue;
				
				CProceduralModule &pModule2=(*i2);

				float fDist2=GetBBoxSquareDistance(pModule2.m_bbox,vModuleCenter1);

				if (fDist2<fMaxExtentSquare)
				{
					CProceduralModuleLink newLink;
					Vec3 vDisp = pModule2.m_pObject->GetPos()-pModule1.m_pObject->GetPos();
					newLink.m_pLink=&pModule2;
					newLink.m_vDisplacement = vDisp;
					pModule1.m_Links.push_back(newLink);
				}				
			}
		}

		// If floor is not empty, then generate it
		if (!floorObjects.empty())
			GenerateFloor(floorObjects,&pRule);

		// Remove old objects from this floor
		for (int i=0;i<floorObjects.size();++i)
			GetIEditor()->DeleteObject(floorObjects[i].m_pObject);
	} // end of module links collection


	building->UpdatePrefab();

	// Bring back the last selected floor as current floor
	CProceduralCreationWorks::ChangeToFloor(lastFloor);
}


//////////////////////////////////////////////////////////////////////////
void CProceduralProductionUtils::GenerateFloor(TModulesList &modulesList, CProceduralProductionRuleBase *pRule)
{
	pRule->GenerateFloor(modulesList);

}

//////////////////////////////////////////////////////////////////////////
float CProceduralProductionUtils::GetBBoxSquareDistance(const AABB &bbox,const Vec3 &vPos)
{

	float fDist2=0;

	const Vec3 &vMins=bbox.min;
	const Vec3 &vMaxs=bbox.max;

	if (vPos.x < vMins.x)
		fDist2 += sqr(vPos.x - vMins.x); 
	else if (vPos.x > vMaxs.x)    
		fDist2 += sqr(vPos.x - vMaxs.x); 

	if (vPos.y < vMins.y)
		fDist2 += sqr(vPos.y - vMins.y);
	else if (vPos.y > vMaxs.y)    
		fDist2 += sqr(vPos.y - vMaxs.y); 

	return fDist2;
}

CString CProceduralProductionUtils::GetGeometryString(CBrushObject *brushObject)
{
	if (!brushObject)
		return "";
	
	CString selGeomFile = brushObject->GetGeometryFile();
	Path::ConvertSlashToBackSlash(selGeomFile);
	selGeomFile=selGeomFile.MakeLower();

	return Path::GetFileName(selGeomFile); // get CGF filename
}


void CProceduralProductionUtils::CopyModule(CProceduralModule &sourceModule,CProceduralModule &destinationModule, bool copyObject)
{
	for (lstLinkIt i1=sourceModule.m_Links.begin(); i1!=sourceModule.m_Links.end(); ++i1)		
	{
		if (!i1->m_pLink) 
			continue;

		destinationModule.m_Links.push_back((*i1));
	}

	destinationModule.m_bbox = sourceModule.m_bbox;

	if (copyObject)
		destinationModule.m_pObject = sourceModule.m_pObject;
}