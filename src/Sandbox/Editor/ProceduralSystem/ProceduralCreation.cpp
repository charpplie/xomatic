#include "stdafx.h"
#include "..\Objects\BaseObject.h"
#include "..\Objects\Group.h"
#include "..\Prefabs/PrefabManager.h"
#include "..\ProceduralCreationDialog.h"
#include "..\ProceduralSystem/ProceduralCreation.h"
#include "..\Settings.h"
#include "..\ViewManager.h"
#include "..\Grid.h"

#include "..\BaseLibraryManager.h"
#include "..\BaseLibrary.h"
#include "..\BaseLibraryItem.h"
#include "..\BaseLibraryManager.h"

CProcSettings procSettings;

CProcSettings::CProcSettings()
{
	buildingSelected=false;
	lastBuildingGUID=GUID_NULL;
	currentFloorNumber=0;
	currentFloorZ=0;
	lastMouseDownPos=CPoint(0,0);
	highlightFloor=false;
	wasVisibleGridActivated=false;
}

CProceduralCreationDialog* CProceduralCreationWorks::GetProcDialog()
{
	CProceduralCreationFrame *procWin = (CProceduralCreationFrame*)GetIEditor()->FindView(PROC_WINDOW_NAME);
	if (procWin)
		return procWin->GetProcDialog();
	else
		return NULL;
}

bool CProceduralCreationWorks::IsProceduralBuildingPrefabSelected(CBaseObject* selectedObject)
{
	if (!selectedObject) 
		return false;

	if (selectedObject->IsKindOf(RUNTIME_CLASS(CPrefabBuildingObject)))	
		return true;

	return false;
}

Vec3 CProceduralCreationWorks::GetSnappedPosition(Vec3 &currentPosition)
{
	CGrid *pGrid = GetIEditor()->GetViewManager()->GetGrid();
	if (!pGrid) return currentPosition;
	if (pGrid->size==0) return currentPosition;

	if (!IsProceduralCreationActivated() || (!CProcSettings::GetProcSettings().buildingSelected ) || (!pGrid->IsEnabled()) )  return currentPosition;

	CBaseObject *buildingObject = GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!buildingObject) return currentPosition;

	float floorHeight = 0;
	CBaseObject *pObject = GetIEditor()->GetSelectedObject();
	if (pObject)
		if (pObject->GetParent())
			if (IsProceduralBuildingPrefabSelected(pObject->GetParent()))
				floorHeight=GetFloorZ(pObject->GetParent()->GetId(),pObject->GetFloorNumber());

	Quat currRot=buildingObject->GetRotation();

	Vec3 vCenterPos=buildingObject->GetLocalTM().GetTranslation();
	Vec3 objPosInv=vCenterPos+(currentPosition-vCenterPos)*currRot;

	float ObjDistX = objPosInv.x - vCenterPos.x;
	float ObjDistY = objPosInv.y - vCenterPos.y;

	float ObjCorrectXOffset = floor(ObjDistX/pGrid->size)*pGrid->size-pGrid->size/2+pGrid->size;
	float ObjCorrectYOffset = floor(ObjDistY/pGrid->size)*pGrid->size-pGrid->size/2+pGrid->size;

	Vec3 rotPos(ObjCorrectXOffset,ObjCorrectYOffset,floorHeight);
	currRot.v.z=currRot.v.z*-1;
	rotPos=rotPos*currRot;

	return rotPos;
}

void CProceduralCreationWorks::SelectUnselectProceduralObjects(CString keyword, bool selectObject, bool wholeBuildingAction)
{
	IObjectManager *objMan = GetIEditor()->GetObjectManager();

	GUID buildingID = CProcSettings::GetProcSettings().lastBuildingGUID;
	CBaseObject *building = objMan->FindObject(buildingID);
	if (!building) return;

	objMan->UnselectObject(building);

	for ( int i=0;i<building->GetChildCount();i++)
	{
		CBaseObject *procObject = objMan->FindObject(building->GetChild(i)->GetId());
		if (!procObject) continue;

		if (!wholeBuildingAction)
			if (procObject->GetFloorNumber()!=CProcSettings::GetProcSettings().currentFloorNumber) continue;

		CString objName = procObject->GetName();
		if ( objName.MakeLower().Find(keyword.MakeLower())!=-1)
		{
			if (selectObject)
				objMan->SelectObject(procObject,true);
			else
				objMan->UnselectObject(procObject);
		}
	}	
}

void CProceduralCreationWorks::AssignObjectToBuilding(GUID buildingID, GUID objectID, int floorID)
{
	if (buildingID==GUID_NULL || objectID==GUID_NULL || floorID<0) return;

	CPrefabBuildingObject *building = (CPrefabBuildingObject *) GetIEditor()->GetObjectManager()->FindObject(buildingID);
	if (!building) return;

	CBaseObject *procObject = GetIEditor()->GetObjectManager()->FindObject(objectID);
	if (procObject)
	{
		// Do not add building to a building
		if (procObject->IsKindOf(RUNTIME_CLASS(CPrefabBuildingObject))) return;

		// In case this method is called from a different place than CPrefabBuildingObject::AddObjectToPrefab, we need to add this object to the current building prefab first
		if (!procObject->GetParent() || procObject->GetParent()->GetId()!=building->GetId())
		{
			building->AddObjectToPrefab(procObject);
			RefreshReplaceBrowser();
			UpdateFloorSelectionList();
			return;
		}

		procObject->SetFloorNumber(floorID);
		PositionObjectToProceduralGrid(procObject);	
		UpdateFloorHeight(floorID,GetHighestPointOnFloor(floorID));
		RefreshReplaceBrowser();
		UpdateFloorSelectionList();
	}

}

float CProceduralCreationWorks::GetHighestPointOnFloor(int floorNumber)
{
	float highestPoint=0;
	float curObjZHeight=0;

	if (CProcSettings::GetProcSettings().lastBuildingGUID==GUID_NULL) return 0.0;
	if (!CProcSettings::GetProcSettings().buildingSelected) return 0.0;

	CPrefabBuildingObject *building = (CPrefabBuildingObject *) GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return 0.0;

	// In case floor height is defined by user
	if (building->IsFloorHeightDefinedByUser(floorNumber))
		return building->GetFloorHeight(floorNumber);

	IObjectManager *objMan = GetIEditor()->GetObjectManager();

	for ( int i=0;i<building->GetChildCount();i++)
	{
		CBaseObject *procObject = objMan->FindObject(building->GetChild(i)->GetId());
		if (!procObject) continue;

		if ( procObject->GetFloorNumber()!=floorNumber) continue;

		AABB objBoundBbox;
		procObject->GetBoundBox(objBoundBbox);
		Vec3 objBoxSize = objBoundBbox.GetSize();

		float curFloorZ = GetFloorZ(building->GetId(),floorNumber);

		curObjZHeight=procObject->GetPos().z+objBoxSize.z;

		if (procObject->GetPos().z<curFloorZ)
			curObjZHeight = objBoxSize.z-curFloorZ+procObject->GetPos().z;
		else
			curObjZHeight=curObjZHeight-GetFloorZ(building->GetId(),floorNumber);

		if (curObjZHeight>highestPoint) highestPoint=curObjZHeight;
	}

	return highestPoint;
}

bool CProceduralCreationWorks::IsProceduralCreationActivated()
{
	return CProcSettings::GetProcSettings().buildingSelected;
}

void CProceduralCreationWorks::PositionObjectToProceduralGrid(CBaseObject* pObject)
{
	if (!pObject) return;
	if (!CProcSettings::GetProcSettings().buildingSelected) return;
	if (!IsProceduralObjectSelected(pObject)) return;
	if (CProcSettings::GetProcSettings().lastBuildingGUID==GUID_NULL) return;
	if (pObject->IsKindOf(RUNTIME_CLASS(CPrefabBuildingObject))) return; // if building, then do not position it.

	CGrid *pGrid = GetIEditor()->GetViewManager()->GetGrid();
	if (!pGrid) return;
	if (pGrid->size==0) return;

	if ( (IsProceduralCreationActivated()) && (pObject))
	{
		Vec3 objPos = pObject->GetPos();
		GUID selID = pObject->GetId();

		if (pObject->GetParent())
			if (pObject->GetParent()->GetId()!=CProcSettings::GetProcSettings().lastBuildingGUID) return;

		if (pObject->GetFloorNumber()!=CProcSettings::GetProcSettings().currentFloorNumber) return;

		if (pGrid->IsEnabled())
		{
			CBaseObject *buildingObject = GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
			if (!buildingObject) return;

			Quat currRot=buildingObject->GetRotation();

			float ObjCorrectXOffset= floor(objPos.x/pGrid->size)*pGrid->size-pGrid->size/2+pGrid->size;
			float ObjCorrectYOffset= floor(objPos.y/pGrid->size)*pGrid->size-pGrid->size/2+pGrid->size;
	
			CViewport *vp = GetIEditor()->GetViewManager()->GetView(0);
			vp->BeginUndo();

			float floorHeight = 0;
			
			if (pObject->GetParent())
			{
				if (IsProceduralBuildingPrefabSelected(pObject->GetParent()))
					floorHeight=GetFloorZ(pObject->GetParent()->GetId(),pObject->GetFloorNumber());
			}

			Vec3 rotPos(0,0,0);
			rotPos(ObjCorrectXOffset,ObjCorrectYOffset,floorHeight);
			pObject->SetPos(rotPos);

			vp->AcceptUndo( "Move Selection" );
		}
	}
}


void CProceduralCreationWorks::UpdatateBuildingPrefab(CSelectionGroup *selection, int nFlags)
{
	bool bCtrlClick = (nFlags & MK_CONTROL);
	bool bShiftClick = (nFlags & MK_SHIFT);
	if ( (bCtrlClick) || (bShiftClick) ) return;

	if (!selection) return;
	if (selection->GetCount()==0) return;

	std::map<CString,CString> buildingNames;
	std::map<CString,CString> objNames;

	for (int i=0;i<selection->GetCount();i++)
	{
		if (selection->GetObject(i)->GetParent())
			if (!IsProceduralBuildingPrefabSelected(selection->GetObject(i)->GetParent())) continue;
		CPrefabBuildingObject *curBuilding = (CPrefabBuildingObject *)selection->GetObject(i)->GetParent();
		if (!curBuilding) continue;
		if (!curBuilding->IsBuildingModified()) continue;
		if (curBuilding->GetId()!=CProcSettings::GetProcSettings().lastBuildingGUID) continue;

		objNames[selection->GetObject(i)->GetName()]=selection->GetObject(i)->GetParent()->GetName();
		buildingNames[curBuilding->GetName()]=curBuilding->GetName();
	}

	std::map<CString,CString>::iterator buildingNamesIt = buildingNames.begin();
	for (;buildingNamesIt!=buildingNames.end();buildingNamesIt++)
	{
		CPrefabBuildingObject *updatedBuilding = (CPrefabBuildingObject*)GetIEditor()->GetObjectManager()->FindObject(buildingNamesIt->first);
		if (!updatedBuilding) continue;
		if (!updatedBuilding->IsBuildingModified()) continue;

		if (!updatedBuilding->IsFloorHeightDefinedByUser(CProcSettings::GetProcSettings().currentFloorNumber))
			CProceduralCreationWorks::UpdateFloorHeight(CProcSettings::GetProcSettings().currentFloorNumber,CProceduralCreationWorks::GetHighestPointOnFloor(CProcSettings::GetProcSettings().currentFloorNumber));

		updatedBuilding->UpdatePrefab();
	}

	std::map<CString,CString>::iterator objNamesIt = objNames.begin();

	for (;objNamesIt!=objNames.end();objNamesIt++)
	{				
		CPrefabBuildingObject *curBuilding = (CPrefabBuildingObject*)GetIEditor()->GetObjectManager()->FindObject(objNamesIt->second);
		if (!curBuilding) continue;

		CBaseObject *selObj = GetIEditor()->GetObjectManager()->FindObject(objNamesIt->first);
		if (!selObj) continue;

		int selCount=objNames.size();
		for ( int childNo=0;childNo<curBuilding->GetChildCount();childNo++)
			if (curBuilding->GetChild(childNo)->GetName()==objNamesIt->first)
				GetIEditor()->SelectObject(curBuilding->GetChild(childNo));
	}

	CProceduralCreationWorks::HighlightFloor(CProcSettings::GetProcSettings().highlightFloor);
}

void CProceduralCreationWorks::UpdateProceduralCreation(CBaseObject* selectedObject)
{
	if (!selectedObject) return;

	bool procWin = CProceduralCreationWorks::IsProceduralCreationActivated();

	if ((procWin) && (IsProceduralBuildingPrefabSelected(selectedObject)) ) // if Procedural Building Entity
	{	
		Vec3 procBuildingCenter = selectedObject->GetPos();
		CProcSettings::GetProcSettings().buildingSelected=true;
		GUID selID= selectedObject->GetId();

		TProcBuildingType::const_iterator bIter = CProcSettings::GetProcSettings().Buildings.find(selID);
		if ( bIter != CProcSettings::GetProcSettings().Buildings.end())
		{
			CProcSettings::GetProcSettings().lastBuildingGUID=selID;
			UpdateFloorSelectionList();
			CPrefabBuildingObject *curBuilding = (CPrefabBuildingObject *)selectedObject;
			curBuilding->GetPrefab()->UpdateFromPrefabBuildingObject(curBuilding);
		}
		else
			CProcSettings::GetProcSettings().Buildings[selID]=selID;
	}

	CProceduralCreationWorks::HighlightFloor(CProcSettings::GetProcSettings().highlightFloor);
}

Vec3 CProceduralCreationWorks::GetGridPosition(Vec3 currentGridPosition)
{
	if (!CProceduralCreationWorks::IsProceduralCreationActivated()) return currentGridPosition;
	if (!CProcSettings::GetProcSettings().buildingSelected) return currentGridPosition;

	CPrefabBuildingObject *building = (CPrefabBuildingObject *) GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (building)
		currentGridPosition=building->GetPos();

	currentGridPosition.z +=CProcSettings::GetProcSettings().currentFloorZ;

	return currentGridPosition;
}

void CProceduralCreationWorks::DeleteObject(CBaseObject *selectedObject)
{
	bool procWin = IsProceduralCreationActivated();

	CViewport *vp = GetIEditor()->GetViewManager()->GetView(0);
	IObjectManager *objMan = GetIEditor()->GetObjectManager();

	if ((procWin) && (IsProceduralBuildingPrefabSelected(selectedObject))) // if ProceduralBuilding Entity
	{	
		GUID selID= selectedObject->GetId();
		bool objectFound=false;

		vp->BeginUndo();

		if (objMan->FindObject(selectedObject->GetId()))
			objMan->DeleteObject(objMan->FindObject(selectedObject->GetId()));

		vp->AcceptUndo( "Delete Building" );

		CProceduralCreationDialog *dlg = GetProcDialog();
		if (dlg) 
			if (dlg->IsWindowVisible())
				dlg->UpdateBuildingSelectionCombo();


	}else
		if ((procWin) && (CProcSettings::GetProcSettings().buildingSelected)) // if not Procedural Building Prefab
		{
			GUID selID= selectedObject->GetId();
			bool objectFound=false;

			vp->BeginUndo();

			if (objMan->FindObject(selectedObject->GetId()))
				objMan->DeleteObject(objMan->FindObject(selectedObject->GetId()));

			vp->AcceptUndo( "Delete Procedural Building Object" );
		}
}

bool CProceduralCreationWorks::IsObjectMovementAllowed(CBaseObject* selectedObject)
{
	if (!selectedObject) 
		return true;

	if (selectedObject->IsKindOf(RUNTIME_CLASS(CPrefabBuildingObject))) 
		return true;

	if (!IsProceduralObjectSelected(selectedObject)) 
		return true;

	bool procWin = IsProceduralCreationActivated();
	if ((!CProcSettings::GetProcSettings().buildingSelected) || (!procWin) ) 
		return false; // modular object but no building selected yet = movement not possible.

	if ((selectedObject->GetFloorNumber()!=CProcSettings::GetProcSettings().currentFloorNumber) || (selectedObject->GetParent()->GetId()!=CProcSettings::GetProcSettings().lastBuildingGUID)) 
		return false;

	return true;
}

bool CProceduralCreationWorks::IsProceduralObjectSelected(CBaseObject* selectedObject)
{
	if (!selectedObject) 
		return false;

	if (selectedObject->GetParent())
		if (IsProceduralBuildingPrefabSelected(selectedObject->GetParent())) 
			return true;

	return false;
}

CString CProceduralCreationWorks::GetProceduralBuildingName(CBaseObject* selectedObject)
{
	int currentFloor = CProcSettings::GetProcSettings().currentFloorNumber;

	CBaseObject *building = selectedObject->GetParent();
	if (building)
		if (building->IsKindOf(RUNTIME_CLASS(CPrefabBuildingObject))) return building->GetName();

	return "";
}

int CProceduralCreationWorks::GetProceduralObjectFloorNumber(CBaseObject* selectedObject)
{
	int currentFloor = CProcSettings::GetProcSettings().currentFloorNumber;
	if (selectedObject)
		return selectedObject->GetFloorNumber();	
	return -1;
}


float CProceduralCreationWorks::GetFloorZ(GUID buildingID, int floorNumber)
{
	float curfloorHeight=0;

	CPrefabBuildingObject *building = (CPrefabBuildingObject *) GetIEditor()->GetObjectManager()->FindObject(buildingID);
	if (!building) return 0;

	if (floorNumber > building->GetFloorCount() ) return curfloorHeight;

		for (int i=0;i<floorNumber;i++)
			curfloorHeight += building->GetFloorHeight(i);

	return curfloorHeight;
}

void CProceduralCreationWorks::DeleteFloor(int floorNumber)
{
	CPrefabBuildingObject *building = (CPrefabBuildingObject*) GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return;

	float deletedFloorHeight = CProceduralCreationWorks::GetHighestPointOnFloor(floorNumber);

	bool deleteLastFloor = false;
	int newfloorNo=floorNumber;

	int numberOfFloors = building->GetFloorCount();
	int lastFloorNumber = numberOfFloors -1;

	int i=0;

	// Delete objects from the deleted floor
	for (i=0 ; i< building->GetChildCount();)
	{
		if (building->GetChild(i)->GetFloorNumber()==floorNumber)
			GetIEditor()->GetObjectManager()->DeleteObject(building->GetChild(i));
		else i++;
	}

	// Set deleted floor height to 0 and move all upper floors down
	UpdateFloorHeight(floorNumber,0);

	// Change object floor numbers from upper floors by one less then their actual
	for (i=0 ; i< building->GetChildCount();i++)
		if (building->GetChild(i)->GetFloorNumber()>floorNumber)
			building->GetChild(i)->SetFloorNumber(building->GetChild(i)->GetFloorNumber()-1);

	// Cascade down floor height mode - auto height or custom height assigned by user
	for (i=floorNumber ; i< building->GetFloorCount(); i++)
	{
		bool IsFloorHeightDefinedByUser = building->IsFloorHeightDefinedByUser(i+1);
		building->SetFloorHeightUserDefined(i, IsFloorHeightDefinedByUser );

		if (IsFloorHeightDefinedByUser)
			// Assign user floor height
			building->SetFloorHeight(building->GetFloorHeight(i+1),i);
		else
			// Calculate the highest point on floor and update this in the building object directly
			building->SetFloorHeight(GetHighestPointOnFloor(i), i);
	}

	if ( floorNumber <= lastFloorNumber )
		building->DeleteFloor(lastFloorNumber);

	building->UpdatePrefab();

	CProceduralCreationWorks::HighlightFloor(CProcSettings::GetProcSettings().highlightFloor);
};

void CProceduralCreationWorks::ChangeToFloor(int floorNumber)
{
	CPrefabBuildingObject *building = (CPrefabBuildingObject *) GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return;
	
	if (floorNumber>=building->GetFloorCount()) return; // kontest

	float curfloorHeight=0.0;
	for (int i=0;i<floorNumber;i++)
		curfloorHeight += building->GetFloorHeight(i);

	CProcSettings::GetProcSettings().currentFloorZ=curfloorHeight;
	CProcSettings::GetProcSettings().currentFloorNumber=floorNumber;

	CSelectionGroup *selObjects = GetIEditor()->GetObjectManager()->GetSelection();
	if (selObjects)
	{
		for (int i=0;i<selObjects->GetCount();i++)
		{
			selObjects->GetObject(i)->SetSelected(false);
		}
	}

	CProceduralCreationWorks::HighlightFloor(CProcSettings::GetProcSettings().highlightFloor);
}

void CProceduralCreationWorks::UpdateFloorHeight(int floorNumber,float floorHeight)
{
	bool heightChanged=false;

	if (CProcSettings::GetProcSettings().lastBuildingGUID==GUID_NULL) return;
	if (!CProcSettings::GetProcSettings().buildingSelected) return;

	CPrefabBuildingObject *building = (CPrefabBuildingObject*) GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return;

	std::vector<float> oldFloorZ;
	for (int i=0;i<building->GetFloorCount();i++)
		oldFloorZ.push_back(GetFloorZ(building->GetId(),i));

	TProcBuildingType::const_iterator bIter = CProcSettings::GetProcSettings().Buildings.find(CProcSettings::GetProcSettings().lastBuildingGUID);
	if ( bIter != CProcSettings::GetProcSettings().Buildings.end())
	{
		if (floorNumber > building->GetFloorCount()) return;

		building->SetFloorHeight(floorHeight,floorNumber);
		CProcSettings::GetProcSettings().currentFloorZ=CProceduralCreationWorks::GetFloorZ(CProcSettings::GetProcSettings().lastBuildingGUID,CProcSettings::GetProcSettings().currentFloorNumber);

		for ( int c=0; c<building->GetChildCount() ; c++)
		{
			CBaseObject *posObj=building->GetChild(c);
			if (posObj)
			{
				if (posObj->GetFloorNumber()==-1) continue;

				float newZ=GetFloorZ(building->GetId(),posObj->GetFloorNumber());
				float oldZ = oldFloorZ[posObj->GetFloorNumber()];
				float heightDifference = newZ-oldZ; // position of object above it's floor					
				Vec3 objVec = posObj->GetPos();
				float newObjectZ= objVec.z+heightDifference;
				objVec.z = newObjectZ;
				if (posObj->GetPos()!=objVec)
				{
					heightChanged=true;
					posObj->SetPos(objVec);
				}
			}					
		}				
	}

	CProceduralCreationWorks::HighlightFloor(CProcSettings::GetProcSettings().highlightFloor);

	UpdateFloorSelectionList();
}

bool CProceduralCreationWorks::ProceduralBuildingObjectHitTest(CBaseObject* hitObject)
{
	bool procWin = CProceduralCreationWorks::IsProceduralCreationActivated();

	if ((hitObject) && (IsProceduralBuildingPrefabSelected(hitObject)))
	{	
		CPrefabBuildingObject *building = (CPrefabBuildingObject*)hitObject;

		bool wasSelected = false;
		if (building->CheckFlags(OBJFLAG_SELECTED)) 
			wasSelected=true;

		CProceduralCreationWorks::CloseAllBuildings();
		building->Open();

		if (wasSelected) 
			GetIEditor()->SelectObject(building);

		if (CProcSettings::GetProcSettings().currentFloorNumber>=building->GetFloorCount())
		{
			CProcSettings::GetProcSettings().currentFloorNumber=0;
			CProcSettings::GetProcSettings().currentFloorZ=0;
		}

		CProcSettings::GetProcSettings().lastBuildingGUID=hitObject->GetId();
		CProcSettings::GetProcSettings().buildingSelected=true;

		CWnd* procWin = (CProceduralCreationFrame*)GetIEditor()->FindView(PROC_WINDOW_NAME);
		if (procWin)
		{
			UpdateFloorSelectionList();

			CProceduralCreationDialog *dlg = GetProcDialog();
			if (dlg)
				if (dlg->IsWindowVisible())
				{
					dlg->UpdateBuildingSelectionCombo();
					CProceduralCreationWorks::HighlightFloor(CProcSettings::GetProcSettings().highlightFloor);
					dlg->RefreshReplaceBrowser();
				}
		}

		if (!procWin)
		{
			GetIEditor()->OpenView(PROC_WINDOW_NAME);
			ProceduralBuildingObjectHitTest(hitObject);
		}

		return true; // procedural building
	}

	return false; // not a procedural building
}

void CProceduralCreationWorks::CopySelectedObjects(GUID buildingID, int floorNumber,bool entireFloor)
{
	CProcSettings::GetProcSettings().GUIDClipboard.RemoveAll();

	CPrefabBuildingObject *building = (CPrefabBuildingObject *) GetIEditor()->GetObjectManager()->FindObject(buildingID);
	if (!building) return;

	if (entireFloor)
	{
		GetIEditor()->ClearSelection();

		for ( int i=0; i<building->GetChildCount() ; i++)
		{
			if (building->GetChild(i)->GetFloorNumber()==floorNumber)
					CProcSettings::GetProcSettings().GUIDClipboard.AddObject(building->GetChild(i));
		}
		return;
	}

	CSelectionGroup *selGroup = GetIEditor()->GetObjectManager()->GetSelection();
	if (selGroup)
	{
		GetIEditor()->LockSelection( false );

		for (int i = 0; i < selGroup->GetCount(); i++)
			if (GetProceduralObjectFloorNumber(selGroup->GetObject(i))==floorNumber)
				CProcSettings::GetProcSettings().GUIDClipboard.AddObject(selGroup->GetObject(i));

		GetIEditor()->GetObjectManager()->RemoveSelection( selGroup->GetName() );
	}
}

void CProceduralCreationWorks::PasteSelectedObjects(GUID buildingID, int floorNumber)
{
	if (CProcSettings::GetProcSettings().GUIDClipboard.GetCount()==0)
	{
		AfxMessageBox("You have to select objects before you can clone them");
		return;
	}

	CPrefabBuildingObject *pBuilding = (CPrefabBuildingObject*) GetIEditor()->GetObjectManager()->FindObject(buildingID);
	if (!pBuilding) return;

	CSelectionGroup *selGroup = &CProcSettings::GetProcSettings().GUIDClipboard;
	if (selGroup->GetCount()>0)
	{
		CSelectionGroup selObjects;
		CSelectionGroup currSelection;

		for (int i=0;i<CProcSettings::GetProcSettings().GUIDClipboard.GetCount();i++)
		{
			CBaseObject *selObj=GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().GUIDClipboard.GetObject(i)->GetId());
			if (selObj)
				currSelection.AddObject(selObj);
		}

		currSelection.Clone(selObjects);

		for (int i=0;i<selObjects.GetCount();i++)
		{
			CBaseObject *newObj = selObjects.GetObject(i);
			CBaseObject *oldObj = currSelection.GetObject(i);

			Vec3 objPos= newObj->GetPos();
			Vec3 newPos=oldObj->GetPos();

			// Copied objects get also local height above the new floor
			newPos.z= GetFloorZ(buildingID,floorNumber)+oldObj->GetPos().z-GetFloorZ(buildingID,oldObj->GetFloorNumber());
			newObj->SetPos(newPos);
			newObj->SetFloorNumber(floorNumber);
		}

		int count = selObjects.GetCount();

		for (int i=0;i<selObjects.GetCount();i++)
			pBuilding->AddObjectToPrefab(selObjects.GetObject(i),true);

		if (!pBuilding->IsFloorHeightDefinedByUser(CProcSettings::GetProcSettings().currentFloorNumber))
			CProceduralCreationWorks::UpdateFloorHeight(CProcSettings::GetProcSettings().currentFloorNumber,CProceduralCreationWorks::GetHighestPointOnFloor(CProcSettings::GetProcSettings().currentFloorNumber));

		pBuilding->UpdatePrefab();
		UpdateFloorSelectionList();
		GetIEditor()->ClearSelection();
		CProceduralCreationWorks::HighlightFloor(CProcSettings::GetProcSettings().highlightFloor);
	}
}

void CProceduralCreationWorks::ShowFloor(int floorNumber, bool visible)
{
	CPrefabBuildingObject *building = (CPrefabBuildingObject*) GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return;

	for ( int i=0; i< building->GetChildCount(); i++)
		if (building->GetChild(i)->GetFloorNumber()==floorNumber)
		{
			if (visible)
				building->GetChild(i)->SetHidden(false);
			else
				building->GetChild(i)->SetHidden(true);
		}
}

void CProceduralCreationWorks::UpdateFloorSelectionList()
{
	CWnd* procWin = GetIEditor()->FindView(PROC_WINDOW_NAME);
	if ( ( procWin ) && ( GetProcDialog() ) ) 
			GetProcDialog()->UpdateFloorSelectionList();
}

void CProceduralCreationWorks::ResetProceduralCreationSettings()
{
	CProcSettings::GetProcSettings().Buildings.clear();
	CProcSettings::GetProcSettings().buildingSelected=false;
	CProcSettings::GetProcSettings().GUIDClipboard.RemoveAll();
	UpdateFloorSelectionList();
}

bool CProceduralCreationWorks::IsProceduralBuildingInSelection(CSelectionGroup *selectionGroup)
{
	CString objName;

	for (int i = 0; i < selectionGroup->GetCount(); i++)
	{
		CBaseObject *pObj = selectionGroup->GetObject(i);
		if ((pObj) && (IsProceduralBuildingPrefabSelected(pObj)))
			return true;
	}

	return false;
}

bool CProceduralCreationWorks::IsProceduralBuildingInSelection(CViewport *view,const CRect &rect, bool bSelect )
{
	CViewManager *objMan = GetIEditor()->GetViewManager();
	CBaseObjectsCache *pDisplayedViewObjects = view->GetVisibleObjectsCache();

	HitContext hc;
	hc.view = view;
	hc.b2DViewport = view->GetType() != ET_ViewportCamera;
	hc.rect = rect;
	AABB box;

	int numVis = pDisplayedViewObjects->GetObjectCount();

	for (int i = 0; i < numVis; i++)
	{
		CBaseObject *pObj = pDisplayedViewObjects->GetObject(i);
		if (!pObj->IsSelectable())
			continue;
		// Retrieve world space bound box.
		pObj->GetBoundBox( box );
		if (!view->IsBoundsVisible(box))
			continue;
		if (pObj->HitTestRect(hc))
		{
			if ((pObj) &&  (IsProceduralBuildingPrefabSelected(pObj)) )
				return true;
		}
	}

	return false;
}

CSelectionGroup CProceduralCreationWorks::GetProceduralBuildingSelection(GUID getBuildingID)
{
	CSelectionGroup procObjects;
	CSelectionGroup *selGroup = GetIEditor()->GetObjectManager()->GetSelection();

	for (int i=0; i<selGroup->GetCount();i++)
	{
		CBaseObject *pObject = selGroup->GetObject(i);

		if ((pObject) && (IsProceduralBuildingPrefabSelected(pObject)))
		{
			for ( int a=0 ; a<pObject->GetChildCount(); a++)
				procObjects.AddObject(pObject->GetChild(a));
		}
	}

	return procObjects;
}

void CProceduralCreationWorks::DetachObjects(const CString &releaseCommandStr)
{
	if (releaseCommandStr==DO_NOTHING) return;

	if (releaseCommandStr==RELEASE_CURRENT_FLOOR_OBJECTS)
		SelectUnselectProceduralObjects("",true,false);

	CBaseObject *building = GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return;

	CSelectionGroup *selGroup = GetIEditor()->GetSelection();
	std::vector<CBaseObject*> selObjects;

	for (int i=0;i<selGroup->GetCount();i++)
		selObjects.push_back(selGroup->GetObject(i));

	if (!building->IsKindOf(RUNTIME_CLASS(CPrefabBuildingObject))) return;

	CPrefabBuildingObject *curBuilding= (CPrefabBuildingObject *)building;
	curBuilding->Open();

	int obno=selObjects.size();
	if (selObjects.size()>0)
	{
		for (int i=0;i<selObjects.size();i++)
		{
			CBaseObject *selObj = selObjects[i];
			curBuilding->RemoveObjectFromPrefab(selObj);
		}
	}

	// The code bellow solves problem in Sandbox of frozen objects if all objects are released from a prefab.
	if (building->GetChildCount()==0)
	{
		CBaseObject *tmpObject = GetIEditor()->CloneObject(curBuilding);
		GetIEditor()->GetObjectManager()->DeleteObject(tmpObject);
	}

	curBuilding->UpdatePrefab();
	GetIEditor()->ClearSelection();
	ProceduralBuildingObjectHitTest(curBuilding);
	CProceduralCreationWorks::HighlightFloor(CProcSettings::GetProcSettings().highlightFloor);
}

Matrix34 CProceduralCreationWorks::GetProceduralGridTM(const Matrix34 &defaultTM)
{
	Matrix34 proceduralTM;

	CSelectionGroup *selGroup = GetIEditor()->GetSelection();
	if (selGroup->GetCount()>0)
	{
		CBaseObject *selObject = selGroup->GetObject(0);
		if ((!CProceduralCreationWorks::IsProceduralBuildingPrefabSelected(selObject)) && (CProceduralCreationWorks::IsProceduralObjectSelected(selObject)))
		{
			CBaseObject *lastBuilding = GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
			if (lastBuilding)
			{
				proceduralTM = lastBuilding->GetWorldTM();
				Vec3 zVec = proceduralTM.GetTranslation();
				zVec=CProceduralCreationWorks::GetSnappedPosition(zVec);
				proceduralTM.SetTranslation(zVec);
				return proceduralTM;
			}
		}
	}
	return defaultTM;
}

Vec3 CProceduralCreationWorks::GetProceduralGridVec(const Vec3 &defaultVec, const Matrix34 &tm)
{
	Vec3 procVec(defaultVec.x,defaultVec.y,defaultVec.z);

	CSelectionGroup *selGroup = GetIEditor()->GetSelection();
	if (selGroup->GetCount()>0)
	{
		CBaseObject *selObject = selGroup->GetObject(0);
		if ((!CProceduralCreationWorks::IsProceduralBuildingPrefabSelected(selObject)) && (CProceduralCreationWorks::IsProceduralObjectSelected(selObject)))
		{
			procVec = CProceduralCreationWorks::GetSnappedPosition(procVec);
			return procVec;
		}
	}

	return defaultVec;
}

void CProceduralCreationWorks::PositionObjectsToProceduralGrid()
{
	CSelectionGroup *selGroup = GetIEditor()->GetSelection();

	if (GetIEditor()->GetViewManager()->GetGrid()->bEnabled)
		if (selGroup->GetCount()>0)
			for (int i=0;i<selGroup->GetCount();i++)
				CProceduralCreationWorks::PositionObjectToProceduralGrid(selGroup->GetObject(i));
};

bool CProceduralCreationWorks::GetFloorVisibilityState(int FloorNumber)
{
	if (!CProcSettings::GetProcSettings().buildingSelected) 
		return false;

	CPrefabBuildingObject *building = (CPrefabBuildingObject*)GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) 
		return false;

	if (building->GetChildCount()==0) 
		return true;

	bool hasFloorObjects=false;

	for (int i=0;i< building->GetChildCount(); i++)
		if (building->GetChild(i)->GetFloorNumber()==FloorNumber)
		{
			if (!building->GetChild(i)->IsHidden()) 
				return true;
			hasFloorObjects=true;
		};

	if (!hasFloorObjects) 
		return true;

	return false;
}

void CProceduralCreationWorks::CenterOnCurrentBuilding()
{
	if (!GetIEditor()->GetSelection()->IsEmpty())
	{
		CSelectionGroup *sel = GetIEditor()->GetSelection();
		AABB bounds = sel->GetBounds();
		Vec3 selPos = sel->GetCenter();
		float size = (bounds.max-bounds.min).GetLength();
		if (size == 0)
			return;
		if (size > 10.0f)
			size = 10.0f;
		Vec3 pos = selPos;
		pos += Vec3(0,size*3,size*8);
		Vec3 dir = (selPos-pos).GetNormalized();

		pos=pos+dir*size;

		Matrix34 tm = Matrix34( Matrix33::CreateRotationVDir(dir), pos);
		if (GetIEditor()->GetViewManager()->GetGameViewport())
			GetIEditor()->GetViewManager()->GetGameViewport()->SetViewTM(tm);
	}
}

void CProceduralCreationWorks::MoveCameraViewToFloor(int floorNumber)
{
	if (!IsProceduralCreationActivated()) return;
	if (!CProcSettings::GetProcSettings().buildingSelected) return;

	CPrefabBuildingObject *building = (CPrefabBuildingObject*)GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return;

	Vec3 mousePos =  GetIEditor()->GetViewManager()->GetView(0)->ViewToWorld(CProcSettings::GetProcSettings().lastMouseDownPos);

	mousePos.z= building->GetPos().z + GetFloorZ(CProcSettings::GetProcSettings().lastBuildingGUID,floorNumber);

	if (GetFloorObjectsCount(floorNumber)>0)
		mousePos.z += GetHighestPointOnFloor(floorNumber)*1.4;
	else
		if ( (floorNumber>0) && (GetFloorObjectsCount(floorNumber-1)>0) )
			mousePos.z += GetHighestPointOnFloor(floorNumber-1)*1.4;
		else
			mousePos.z += 10;  // if lower floor has 0 elements, move by default 10 meters up above floor
			
	Matrix34 tm = GetIEditor()->GetViewManager()->GetGameViewport()->GetViewTM();
	Vec3 curPos = tm.GetTranslation();
	curPos.z=mousePos.z;
	tm.SetTranslation(curPos);

	if (GetIEditor()->GetViewManager()->GetGameViewport())
		GetIEditor()->GetViewManager()->GetGameViewport()->SetViewTM(tm);
}

int CProceduralCreationWorks::GetFloorObjectsCount(int floorNumber)
{
	int floorObjects=0;

	if (!IsProceduralCreationActivated()) return floorObjects;
	if (!CProcSettings::GetProcSettings().buildingSelected) return floorObjects;

	CPrefabBuildingObject *building = (CPrefabBuildingObject*)GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return floorObjects;

	for (int i=0; i< building->GetChildCount();i++)
		if (building->GetChild(i)->GetFloorNumber()==floorNumber) ++floorObjects;

	return floorObjects;
}


void CProceduralCreationWorks::ExportBuildings(const CString &saveFilePath)
{
	if (!IsProceduralCreationActivated()) return;
	if (!CProcSettings::GetProcSettings().buildingSelected) return;


	CSelectionGroup *selection = GetIEditor()->GetSelection();
	XmlNodeRef rootXMLNode = CreateXmlNode("Buildings");
	IObjectManager *objMan = GetIEditor()->GetObjectManager();

	CObjectArchive ar(objMan,rootXMLNode,false);

	for (int i=0;i<selection->GetCount();i++)
	{
		if (!selection->GetObject(i)->IsKindOf(RUNTIME_CLASS(CPrefabBuildingObject))) continue;

		CPrefabBuildingObject *curBuilding = (CPrefabBuildingObject *)selection->GetObject(i);
		if (!curBuilding) continue;

		XmlNodeRef buildingNode = rootXMLNode->newChild( curBuilding->GetName() );

		ar.node = buildingNode;

		ar.bLoading=false;
		curBuilding->Serialize(ar);

		XmlNodeRef objectsNode = buildingNode->newChild( "Objects" );
		ar.node = objectsNode;

		for (int childNo=0;childNo<curBuilding->GetChildCount();childNo++)
		{
			XmlNodeRef objectNode = objectsNode->newChild( "Object" );
			ar.node = objectNode;
			curBuilding->GetChild(childNo)->Serialize(ar);
		}
	}

	CFile cFile;

	if (!cFile.Open(saveFilePath, CFile::modeCreate | CFile::modeWrite | CFile::typeText))
	{
		CLogFile::FormatLine("Warning: Saving of %s failed",saveFilePath );
		return;
	}

	CString writeStr = rootXMLNode->getXML(0);
	cFile.Write(writeStr,strlen(writeStr));
	cFile.Close();
}

void CProceduralCreationWorks::CloseAllBuildings()
{
	TProcBuildingType &buildings = CProcSettings::GetProcSettings().Buildings;
	TProcBuildingType::iterator buildingIt = buildings.begin();

	for (buildingIt = buildings.begin(); buildingIt!=buildings.end();buildingIt++ )
	{
		CPrefabBuildingObject *buildingObject = (CPrefabBuildingObject*) GetIEditor()->GetObjectManager()->FindObject(buildingIt->first);
		if (!buildingObject) continue;
		buildingObject->Close();
		GetIEditor()->GetObjectManager()->UnselectObject(buildingObject);

		for (int i=0;i<buildingObject->GetChildCount();i++)
			buildingObject->GetChild(i)->SetHighlight(false);
	}
}

void CProceduralCreationWorks::FollowFloorZ(CViewport *view, CPoint &point)
{
	CPrefabBuildingObject *building = (CPrefabBuildingObject*)GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return;

	Vec3 pos = view->ViewToWorld( point );
	pos.z = building->GetPos().z+CProcSettings::GetProcSettings().currentFloorZ;

	Matrix34 tm;
	tm.SetIdentity();
	tm = building->GetWorldTM();
	tm.SetTranslation( pos );
	Matrix34 userTM = view->GetViewTM();
	userTM.SetTranslation( pos );
	view->SetConstructionMatrix( COORDS_LOCAL,tm );
	view->SetConstructionMatrix( COORDS_PARENT,tm );
	view->SetConstructionMatrix( COORDS_USERDEFINED,userTM );
	GetIEditor()->SetAxisConstrains(AXIS_XY);
}

void CProceduralCreationWorks::HighlightFloor(bool highlight)
{
	if (!IsProceduralCreationActivated()) return;
	if (!CProcSettings::GetProcSettings().buildingSelected) return;

	CPrefabBuildingObject *building = (CPrefabBuildingObject*)GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return;

	for (int i=0; i< building->GetChildCount(); i++)
		if ( (building->GetChild(i)->GetFloorNumber()==CProcSettings::GetProcSettings().currentFloorNumber) && highlight )
			building->GetChild(i)->SetHighlight(true);
		else
			building->GetChild(i)->SetHighlight(false);

	CProcSettings::GetProcSettings().highlightFloor=highlight;
}

void CProceduralCreationWorks::ClickedGridSnap()
{
	CSelectionGroup *selGroup = GetIEditor()->GetSelection();
	if (selGroup->GetCount()>0)
	{
		if (!GetIEditor()->GetViewManager()->GetGrid()->IsEnabled())
			GetIEditor()->GetViewManager()->GetGrid()->Enable(true);

		for (int i=0;i<selGroup->GetCount();i++)
			CProceduralCreationWorks::PositionObjectToProceduralGrid(selGroup->GetObject(i));

		CPrefabBuildingObject *building = (CPrefabBuildingObject*)GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
		if (building)
			if (!building->IsFloorHeightDefinedByUser(CProcSettings::GetProcSettings().currentFloorNumber))
				CProceduralCreationWorks::UpdateFloorHeight(CProcSettings::GetProcSettings().currentFloorNumber,CProceduralCreationWorks::GetHighestPointOnFloor(CProcSettings::GetProcSettings().currentFloorNumber));

		if (GetIEditor()->GetSelection()->GetCount()>0)
			CProceduralCreationWorks::UpdatateBuildingPrefab(GetIEditor()->GetSelection(),0);
	}
}

void CProceduralCreationWorks::SetBuildingModified(CBaseObject *childObjectFromBuiding)
{
	if (!childObjectFromBuiding) return;

	if (IsProceduralObjectSelected(childObjectFromBuiding))
	{
		CPrefabBuildingObject *building = (CPrefabBuildingObject*)childObjectFromBuiding->GetParent();
		if (!building) return;
		building->SetBuildingModified(true);
	}
}

void CProceduralCreationWorks::UpdateBuilding(REFGUID buildingID)
{
	CPrefabBuildingObject *buildingObject = (CPrefabBuildingObject*)GetIEditor()->GetObjectManager()->FindObject(buildingID);
	if (!buildingObject)
	{
		ResetControls();
		return;
	}

	buildingObject->GetPrefab()->UpdateFromPrefabBuildingObject(buildingObject);
	CProceduralCreationWorks::HighlightFloor(CProcSettings::GetProcSettings().highlightFloor);
	RefreshReplaceBrowser();
	UpdateFloorHeight(CProcSettings::GetProcSettings().currentFloorNumber,GetHighestPointOnFloor(CProcSettings::GetProcSettings().currentFloorNumber));
	UpdateFloorSelectionList();
}


bool CProceduralCreationWorks::IsObjectFromCurrentBuilding(CBaseObject *object)
{
	if (!IsProceduralCreationActivated()) 
		return false;

	if (!CProcSettings::GetProcSettings().buildingSelected) 
		return false;

	if (!object) 
		return false;

	if (!object->GetParent()) 
		return false;

	CPrefabBuildingObject *building = (CPrefabBuildingObject*)GetIEditor()->GetObjectManager()->FindObject(object->GetParent()->GetId());
	if (!building) 
		return false;
	
	if (building->GetId()!=CProcSettings::GetProcSettings().lastBuildingGUID) 
		return false;

	return true;
}

bool CProceduralCreationWorks::MustCreatePrefabBuilding(const CString &file)
{
	if (file=="") 
		return false;

	GUID fileGUID = GuidUtil::FromString(file);
	if (fileGUID==GUID_NULL) 
		return false;

	CPrefabItem *pPrefabItem  = (CPrefabItem*)GetIEditor()->GetPrefabManager()->FindItem( fileGUID );
	if (!pPrefabItem) 
		return false;

	if ( (pPrefabItem->GetPrefabObjectClassName()==PREFABBUILDING_OBJECT_CLASS_NAME) && (pPrefabItem->GetFloorCount()>0) )
		return true;

	return false;
}

CString CProceduralCreationWorks::GetBuildingPrefix()
{
	CString retVal=GetDefaultBuildingPrefix();

	if (!CProcSettings::GetProcSettings().buildingSelected) return retVal;

	CPrefabBuildingObject *building = (CPrefabBuildingObject*)GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return GetDefaultBuildingPrefix();

	if (building->GetChildCount()==0) return GetDefaultBuildingPrefix();

	int cutPlace=-1;

	for (int i=0 ;i< building->GetChildCount();++i)
	{
		if (!building->GetChild(i)->IsKindOf(RUNTIME_CLASS(CBrushObject))) continue;
		CBrushObject *childObject = (CBrushObject*)building->GetChild(i);

		retVal = childObject->GetGeometryFile();
		if (retVal.Find(BUILDING_SET_CGF_FILENAME_PREFIX)==-1) continue; // must be "building structure" object starting with BUILDING_SET_CGF_FILENAME_PREFIX
		retVal=Path::GetFile(retVal);

		cutPlace=0;
		for (int a=0; a< COUNT_OF_TOKENS_TO_CUT_PLACE; a++)
			cutPlace += retVal.Find(FILENAME_CUT_TOKEN,cutPlace);

		if (cutPlace>0)
		{
			retVal.Delete(cutPlace,retVal.GetLength());
			break;
		}
	}

	if (cutPlace==-1) 
		return GetDefaultBuildingPrefix();

	retVal.Append(BUILDING_SET_CGF_FILENAME_EXT_PREFIX);

	if ( retVal.Find(BUILDING_SET_CGF_FILENAME_PREFIX) >= 0 )
		return retVal; // returns file browser prefix like "var_XXX*.cgf" where XXX is the last selected building set number
	else 
		return retVal=GetDefaultBuildingPrefix();
}

CString CProceduralCreationWorks::GetDefaultBuildingPrefix()
{
	CString retVal=DEFAULT_FILENAME_EXT_PREFIX;

	CObjectClassDesc *clsDesc = GetIEditor()->GetObjectManager()->FindClass( BRUSH_CLASS_NAME );
	if (!clsDesc) return DEFAULT_FILENAME_EXT_PREFIX;

	retVal = clsDesc->GetFileSpec();
	if (retVal=="") return DEFAULT_FILENAME_EXT_PREFIX;

	return retVal;
}

void CProceduralCreationWorks::RefreshReplaceBrowser()
{
	CProceduralCreationDialog *dlg = GetProcDialog();
	if (dlg)
		if (dlg->IsWindowVisible())
			dlg->RefreshReplaceBrowser();
}

void CProceduralCreationWorks::ResetControls()
{
	CProceduralCreationDialog *dlg = GetProcDialog();
	if (dlg)
		if (dlg->IsWindowVisible())
			dlg->ResetControls();
}

void CProceduralCreationWorks::ReplaceBrowserClick(CString file)
{
	CSelectionGroup *selGroup = GetIEditor()->GetSelection();
	if (!selGroup) return;

	for (int i=0;i<selGroup->GetCount();i++)
	{
		if (!CProceduralCreationWorks::IsProceduralObjectSelected(selGroup->GetObject(i))) continue;
		if (!selGroup->GetObject(i)->IsKindOf(RUNTIME_CLASS(CBrushObject))) continue;
		CBrushObject *selObj = (CBrushObject*)selGroup->GetObject(i);
		selObj->SetGeometryFile(file);		
		CProceduralCreationWorks::SetBuildingModified(selObj);
	}

	CProceduralCreationWorks::UpdatateBuildingPrefab(selGroup,0);
}

void CProceduralCreationWorks::SelectBrowserClick(CString file)
{
	if (!CProcSettings::GetProcSettings().buildingSelected) return;

	CPrefabBuildingObject *building = (CPrefabBuildingObject*) GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return;

	CString selGeomFile("");
	Path::ConvertSlashToBackSlash(file);
	file=file.MakeLower();

	for (int i=0 ;i< building->GetChildCount();++i)
	{
		if (!building->GetChild(i)->IsKindOf(RUNTIME_CLASS(CBrushObject))) continue;
		CBrushObject *childObject = (CBrushObject*)building->GetChild(i);
		
		if (childObject->GetFloorNumber()!=CProcSettings::GetProcSettings().currentFloorNumber) continue;

		selGeomFile = childObject->GetGeometryFile();
		Path::ConvertSlashToBackSlash(selGeomFile);
		selGeomFile=selGeomFile.MakeLower();

		if (selGeomFile==file)
			GetIEditor()->SelectObject(childObject);
	}
}