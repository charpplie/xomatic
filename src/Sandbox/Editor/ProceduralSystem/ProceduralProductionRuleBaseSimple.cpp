#include "stdafx.h"
#include <stdlib.h>
#include <time.h>
#include "..\ProceduralSystem/ProceduralProductionRuleBaseSimple.h"

void CProceduralProductionRuleBaseSimple::CalcSize()
{
}

CProceduralModule* CProceduralProductionRuleBaseSimple::GenerateModule(int nX,int nY)
{
	CProceduralModule *module=0;
	// get the surrounding modules
	// if nothing, starts placing a corner

	// example: 1,0 -> corner
	// 1) you look at the links from corner module
	// 2) see the connected one
	// 3) displace a new one

	return module;

}
//////////////////////////////////////////////////////////////////////////

void CProceduralProductionRuleBaseSimple::GenerateFloor(TModulesList &modulesList)
{
	if (modulesList.empty()) 
		return;

	CProceduralProductionUtils procUtils;

	// Get random value
	int startModuleNo = rand() % (modulesList.size()); // random module number for the start

	TModulesList generatedModulesList; // list of modules to generate

	// add first element to start from some random place on this floor
	generatedModulesList.push_back(modulesList[startModuleNo]);

	// final list of created modules
	TModulesList currentFloorList;

	for (int generatedModulesNo=0; generatedModulesNo < generatedModulesList.size(); ++generatedModulesNo)
	{
		if (!generatedModulesList[generatedModulesNo].m_pObject)
			continue;

		// Add module if not in the list yet or it is the first module
		if ( (!IsModuleInModulesList(generatedModulesList,generatedModulesList[generatedModulesNo])) || generatedModulesNo == 0 )
		{
			CProceduralModule addModule;
			procUtils.CopyModule(generatedModulesList[generatedModulesNo],addModule,false);
			addModule.m_pObject = (CBrushObject*)GetIEditor()->NewObject(BRUSH_CLASS_NAME,generatedModulesList[generatedModulesNo].m_pObject->GetGeometryFile());
			Vec3 orgPos = generatedModulesList[generatedModulesNo].m_pObject->GetWorldPos();
			addModule.m_pObject->SetPos(orgPos);						
			addModule.m_pObject->SetUniqName(generatedModulesList[generatedModulesNo].m_pObject->GetName()+"-M");
			addModule.m_pObject->SetRotation(generatedModulesList[generatedModulesNo].m_pObject->GetRotation());
			currentFloorList.push_back(addModule);
		}

		// Iterate through available directions
		for (int i2=0; i2<generatedModulesList[generatedModulesNo].m_Links.size(); ++i2)
		{
			CProceduralModuleLink linkedModule = generatedModulesList[generatedModulesNo].m_Links[i2];
				if (!IsModuleInModulesList(generatedModulesList,*linkedModule.m_pLink)) // if does not exist in the created objects list
				{
					Vec3 orgPos = generatedModulesList[generatedModulesNo].m_pObject->GetWorldPos();
					Vec3 linkPos = linkedModule.m_vDisplacement;
					Vec3 newModulePos = orgPos + linkPos;					
					
					CProceduralModule createdModule;
					procUtils.CopyModule(*linkedModule.m_pLink,createdModule,false);

					CBrushObject *newLinkedObj = (CBrushObject*)GetIEditor()->NewObject(BRUSH_CLASS_NAME,linkedModule.m_pLink->m_pObject->GetGeometryFile());
					createdModule.m_pObject = newLinkedObj;
					createdModule.m_pObject->SetUniqName(linkedModule.m_pLink->m_pObject->GetName()+"-L");
					createdModule.m_pObject->SetPos(newModulePos);						
					createdModule.m_pObject->SetRotation(linkedModule.m_pLink->m_pObject->GetRotation());

					currentFloorList.push_back(createdModule);
					generatedModulesList.push_back(*linkedModule.m_pLink);
				};			
		}

	} // End of building creation

	// Get the currently selected building
	if (!CProcSettings::GetProcSettings().buildingSelected)
		return;
	CPrefabBuildingObject *building = (CPrefabBuildingObject*) GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building)
		return;

	//Add new objects without re-creating the prefab each time.
	for ( TModulesListIt sIter = currentFloorList.begin() ; sIter!= currentFloorList.end(); ++sIter)
	{
		CProceduralModule *currentProcModule = &(*sIter);
		CBaseObject *currentObject =  currentProcModule->m_pObject;
		building->AddObjectToPrefab(currentObject,true);
	}
}

bool CProceduralProductionRuleBaseSimple::IsModuleInModulesList(TModulesList &searchList, CProceduralModule &module)
{
	TModulesListIt sIter = searchList.begin();
	for ( ; sIter!= searchList.end(); ++sIter)
	{
		CProceduralModule *searchModule = &(*sIter);
		if (!searchModule->m_pObject) 
			continue;
		if (!module.m_pObject) 
			continue;

		AABB bbox1=searchModule->m_bbox;
		AABB bbox2=module.m_bbox;

		float fDist2=bbox1.GetCenter().GetSquaredDistance(bbox2.GetCenter());
		if (fDist2<MAX_BBOX_SPACE)
			return true;
	}

	return false;
}
