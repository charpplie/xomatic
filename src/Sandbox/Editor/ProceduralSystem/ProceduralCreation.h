#ifndef __proceduralcreation_h__
#define __proceduralcreation_h__
#pragma once

#include "..\ProceduralCreationDialog.h"
#include "..\Objects/PrefabBuildingObject.h"
#include "..\Objects/ObjectManager.h"
#include "..\Prefabs/PrefabManager.h"
#include "..\Prefabs/PrefabItem.h"

#define PROC_WINDOW_NAME "Modular Editor"

// Predefined object names used in the "Select Objects" combo.
#define OBJECT_TYPE_INTERIOR "Interior"
#define OBJECT_TYPE_FLOOR "Floor"
#define OBJECT_TYPE_WALL "Wall"

// CGF file prefix of all available objects from the same building set as the last selected building.
// Used for filtering of objects displayed in the Modular Editor multiple objects replacement browser.
#define BUILDING_SET_CGF_FILENAME_PREFIX "var_"
#define BUILDING_SET_CGF_FILENAME_EXT_PREFIX "*.cgf"
#define DEFAULT_FILENAME_EXT_PREFIX "*.*"
#define FILENAME_CUT_TOKEN "_"
#define COUNT_OF_TOKENS_TO_CUT_PLACE 2

#define BRUSH_CLASS_NAME "Brush"

class CBaseLibrary;
class CBaseLibraryItem;
class CBaseLibraryManager;

typedef std::map<GUID,GUID,guid_less_predicate> TProcBuildingType;

class CProcSettings
{
public:
	CProcSettings();
	TProcBuildingType Buildings;
	float currentFloorZ; // Units above ground/object.
	int currentFloorNumber; // Currently selected floor number.
	bool buildingSelected; // True, if any building was selected after level was loaded and that building still exists.
	bool highlightFloor; // Highlight floor state.
	CPoint lastMouseDownPos;
	GUID lastBuildingGUID;
	bool wasVisibleGridActivated;
	CSelectionGroup GUIDClipboard; // Stores GUIDs of objects to be pasted.
	static CProcSettings& GetProcSettings(){static CProcSettings oProcSettings; return oProcSettings;}
};

class CProceduralCreationWorks
{
public:
	static CProceduralCreationDialog* GetProcDialog();
	static CSelectionGroup GetProceduralBuildingSelection(GUID getBuildingID);
	static CString GetProceduralBuildingName(CBaseObject* selectedObject);
	static Vec3 GetGridPosition(Vec3 currentGridPosition);
	static Vec3 GetSnappedPosition(Vec3 &currentPosition);
	static bool IsObjectMovementAllowed(CBaseObject* selectedObject);
	static bool IsProceduralBuildingPrefabSelected(CBaseObject* hitObject);
	static bool IsProceduralBuildingInSelection(CSelectionGroup* selectionGroup);
	static bool IsProceduralBuildingInSelection(CViewport *view,const CRect &rect, bool bSelect);
	static bool IsProceduralCreationActivated();
	static bool IsProceduralObjectSelected(CBaseObject* selectedObject);
	static bool ProceduralBuildingObjectHitTest(CBaseObject* hitObject);
	static float GetFloorZ(GUID buildingID, int floorNumber);
	static int GetProceduralObjectFloorNumber(CBaseObject* selectedObject);
	static void AssignNewObjectsToBuildings(GUID prefabGUID, XmlNodeRef prefabItemNode, CSelectionGroup oldSel, CSelectionGroup newSel);
	static void AssignObjectToBuilding(GUID buildingID, GUID objectID, int floorID);
	static void ChangeToFloor(int floorNumber);
	static void CopySelectedObjects(GUID buildingID, int floorNumber,bool entireFloor);
	static void PasteSelectedObjects(GUID buildingID, int floorNumber);
	static void DeleteObject(CBaseObject *selectedObject);
	static void PositionObjectToProceduralGrid(CBaseObject* pObject);
	static void DetachObjects(const CString &detachCommandStr);
	static void ResetProceduralCreationSettings();
	static void SelectUnselectProceduralObjects(CString keyword, bool Select, bool wholeBuildingAction);
	static void ShowFloor(int floorNumber, bool visible);
	static void UpdateFloorHeight(int floorNumber,float floorHeight);
	static void UpdateFloorSelectionList();
	static void UpdateProceduralCreation(CBaseObject* selectedObject);
	static Matrix34 GetProceduralGridTM(const Matrix34 &defaultTM);
	static Vec3 GetProceduralGridVec(const Vec3 &defaultVec, const Matrix34 &tm);
	static void PositionObjectsToProceduralGrid();
	static bool GetFloorVisibilityState(int FloorNumber);
	static void UpdatateBuildingPrefab(CSelectionGroup *selection,int nFlags);
	static void CenterOnCurrentBuilding();
	static float GetHighestPointOnFloor(int floorNumber);
	static void MoveCameraViewToFloor(int floorNumber);
	static int GetFloorObjectsCount(int floorNumber);
    static void DeleteFloor(int floorNumber);
	static void ExportBuildings(const CString &saveFilePath);
	static void CloseAllBuildings();
	static void FollowFloorZ(CViewport *view, CPoint &point);
	static void HighlightFloor(bool highlight);
	static void ClickedGridSnap();
	static void SetBuildingModified(CBaseObject *childObjectFromBuiding);
	static void UpdateBuilding(REFGUID buildingID);
	static bool IsObjectFromCurrentBuilding(CBaseObject *object);
	static bool MustCreatePrefabBuilding(const CString &file);
	static CString GetBuildingPrefix();
	static CString GetDefaultBuildingPrefix();
	static void RefreshReplaceBrowser();
	static void ResetControls();
	static void ReplaceBrowserClick(CString file);
	static void SelectBrowserClick(CString file);
};
///////////////////////////////////////////////
#endif // __proceduralcreation_h__
