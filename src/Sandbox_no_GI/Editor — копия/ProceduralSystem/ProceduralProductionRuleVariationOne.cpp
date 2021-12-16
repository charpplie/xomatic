#include "stdafx.h"
#include "ProceduralSystem/ProceduralProductionRuleVariationOne.h"

void CProceduralProductionRuleVariationOne::CalcSize()
{
}

CProceduralModule* CProceduralProductionRuleVariationOne::GenerateModule(int nX,int nY)
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

void CProceduralProductionRuleVariationOne::GenerateFloor(TModulesList &modulesList)
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
		CProceduralModule &mainModule = generatedModulesList[generatedModulesNo];

		if (!mainModule.m_pObject)
			continue;

		// Add main module if not in the currentFloorList list yet or it is the starting main module
		if ( (!IsModuleInModulesList(generatedModulesList,generatedModulesList[generatedModulesNo])) || generatedModulesNo == 0 )
		{
			CProceduralModule newMainModule;
			currentFloorList.push_back(SpawnModule(mainModule, newMainModule, mainModule.m_pObject->GetWorldPos()));		
		}

		// Iterate through available directions and create links if found
		for (int linkNo=0; linkNo<generatedModulesList[generatedModulesNo].m_Links.size(); ++linkNo)
		{
			CProceduralModuleLink linkedModule = generatedModulesList[generatedModulesNo].m_Links[linkNo];

			if (!IsModuleInModulesList(generatedModulesList,*linkedModule.m_pLink)) // if does not exist in the created objects list
			{
				Vec3 newModulePos = generatedModulesList[generatedModulesNo].m_pObject->GetWorldPos() + linkedModule.m_vDisplacement;
				CProceduralModule newLinkModule;
				currentFloorList.push_back(SpawnModule(*linkedModule.m_pLink,newLinkModule, newModulePos));
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

void CProceduralProductionRuleVariationOne::AddModuleLinks(TModulesList &modulesList)
{
	CProceduralProductionUtils procUtils;

	TLinksMap cgfLinksMap;

	for (TModulesListIt i1=modulesList.begin(); i1!=modulesList.end();++i1)		
	{	
		CProceduralModule &pModule1=*i1;              

		const Vec3 vModuleCenter1 = pModule1.m_bbox.GetCenter();

		Vec3 vModuleMin1 = pModule1.m_bbox.min;			
		Vec3 vModuleMax1 = pModule1.m_bbox.max;			

		vModuleMin1.z=vModuleMax1.z=0;
		float fMaxExtentSquare=vModuleMin1.GetSquaredDistance(vModuleMax1);

		CString geomFile = i1->m_pObject->GetGeometryFile();
		Path::ConvertSlashToBackSlash(geomFile);
		geomFile=geomFile.MakeLower();

		// Iterate through modules
		// Pick a second module and add as a link of first module if in defined distance range
		for (TModulesListIt i2=modulesList.begin(); i2!= modulesList.end();++i2)		
		{
			// skip the same object
			if (i1==i2)
				continue;

			CProceduralModule &pModule2=*i2;

			float fDist2=procUtils.GetBBoxSquareDistance(pModule2.m_bbox,vModuleCenter1);

			if (fDist2<fMaxExtentSquare)
			{
				CProceduralModuleLink newLink;
				newLink.m_pLink=&pModule2;

				 //Skip if about to add the same link as in particular CGF file links list
				bool skipModuleLink=false;

				lstLink *lst = &cgfLinksMap[geomFile];
				if (lst->size()>0)
					for ( lstLinkIt linkIt=lst->begin() ; linkIt!=lst->end() ; ++linkIt )
					{
						CProceduralModuleLink &tmpMod = *linkIt;						

						// filter links by geometry
						//if (tmpMod.m_pLink->m_pObject->GetGeometryFile() == pModule2.m_pObject->GetGeometryFile())

						//or filter links by object id
						if (tmpMod.m_pLink->m_pObject->GetId() == pModule2.m_pObject->GetId())
						{
							skipModuleLink=true;
							break;
						}
					}

					// Do not add this module as a link,
					// because it already exists in the list of links
					if (!skipModuleLink)
						cgfLinksMap[geomFile].push_back(newLink);
			}				
		}
	}


	// Every module gets the same list of links, but linked object displacement must be updated
	for (TModulesListIt i3=modulesList.begin(); i3!=modulesList.end();++i3)		
	{
		CString geomFile2= i3->m_pObject->GetGeometryFile();
		Path::ConvertSlashToBackSlash(geomFile2);
		geomFile2=geomFile2.MakeLower();

		TLinksMap::iterator val = cgfLinksMap.find(geomFile2);
		if (val!=cgfLinksMap.end())
		{
			CProceduralModule &sourceModule = *i3;

			// Copy links from main map to the current object
			sourceModule.m_Links.resize(val->second.size());
			std::copy(val->second.begin(),val->second.end(),sourceModule.m_Links.begin());

			// Update displacement of each link from the current object;
			for (lstLinkIt linkDispItemIt=sourceModule.m_Links.begin(); linkDispItemIt!= sourceModule.m_Links.end(); ++linkDispItemIt )		
			{
				CProceduralModuleLink &displacedModule = *linkDispItemIt;
				Vec3 vDisp = displacedModule.m_pLink->m_pObject->GetPos() - sourceModule.m_pObject->GetPos();
				displacedModule.m_vDisplacement=vDisp;
			}
		}
	}
}