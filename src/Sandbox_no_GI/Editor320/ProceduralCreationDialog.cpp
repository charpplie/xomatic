// ProceduralCreationDialog.cpp : implementation file
//

#include "stdafx.h"
#include "DimensionsDialog.h"
#include "CryEditDoc.h"

#include "NumberDlg.h"
#include "SizeDialog.h"
#include "ViewManager.h"
#include "ViewPort.h"

#include "GridSettingsDialog.h"
#include "ProceduralCreationDialog.h"
#include "ProceduralSystem/ProceduralCreation.h"
#include "Objects/PrefabBuildingObject.h"
#include "Objects/ObjectManager.h"
#include "Prefabs/PrefabManager.h"
#include "Prefabs/PrefabItem.h"
#include "ObjectCreateTool.h"
#include <math.h>

#include "ProceduralSystem/ProceduralGeneration.h"

//CMEComboBox
BEGIN_MESSAGE_MAP(CMEComboBox, CComboBox)
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
END_MESSAGE_MAP()

void CProceduralCreationDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_PROCEDURAL_CREATION_BUILDING_SEL, m_buildingSel);
	DDX_Control(pDX, IDC_PROCEDURAL_CREATION_HIGHLIGHT_OBJECTS, m_highlightObjectsChk);	
	DDX_Control(pDX, IDC_PROCEDURAL_CREATION_CUR_FLOOR_TXT,m_CurFloorTxt);
}

BEGIN_MESSAGE_MAP(CProceduralCreationDialog, CDialog)
	ON_BN_CLICKED(IDC_PROCEDURAL_CREATION_PREFAB_BTN, AddBuilding)
	ON_BN_CLICKED(IDC_PROCEDURAL_CREATION_EXPORT_BUILDING_BTN, ExportBuildings)
	ON_BN_CLICKED(IDC_PROCEDURAL_CREATION_ADD_TO_BUILDING, AddObjectsToBuilding)
	ON_BN_CLICKED(IDC_PROCEDURAL_CREATION_DELETE_BUILDING_BTN, DeleteBuilding)
	ON_BN_CLICKED(IDC_PROCEDURAL_CREATION_HIGHLIGHT_OBJECTS, HightLightObjects)
	ON_CBN_CLOSEUP(IDC_PROCEDURAL_CREATION_BUILDING_SEL, BuildingSelComboCloseUP)
	ON_CBN_DROPDOWN(IDC_PROCEDURAL_CREATION_BUILDING_SEL, UpdateBuildingSelectionCombo)
	ON_BN_CLICKED(IDC_PROCEDURAL_CREATION_DETACH_FROM_BUILDING, DetachSelectedfObjects)
	ON_BN_CLICKED(IDC_PROCEDURAL_CREATION_GENERATE_BTN, GenerateBuilding)
END_MESSAGE_MAP()

CProceduralCreationDialog::CProceduralCreationDialog(CWnd* pParent /*=NULL*/)
: CDialog(CProceduralCreationDialog::IDD, pParent)
{
	m_ReplaceObjectsTreePanelPtr = NULL;
	m_SelectObjectsTreePanelPtr = NULL;
}

void CProceduralCreationDialog::OnOK(void)
{
}

void CProceduralCreationDialog::OnCancel(void)
{
}

BOOL CProceduralCreationDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_highlightObjectsChk.SetCheck(false);

	CRect rectFloorView(18,124,227,696);
	m_floorView.Create( WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE, rectFloorView, this, 0 );

	CObjectClassDesc *clsDesc = GetIEditor()->GetObjectManager()->FindClass( BRUSH_CLASS_NAME );
	int flags = CPanelTreeBrowser::NO_PREVIEW;

	m_ReplacePageNo=0;
	m_SelectPageNo=0;

	// Object Selection Dialog
	m_SelectObjectsTreePanelPtr = new CPanelTreeBrowser();
	m_SelectObjectsTreePanelPtr->SetDialogType(CPanelTreeBrowser::ME_SELECTION_DIALOG);

	if (!CProcSettings::GetProcSettings().buildingSelected)
		m_SelectObjectsTreePanelPtr->Create(functor(*this,&CProceduralCreationDialog::OnSelectionBrowserDblClick),clsDesc->GetFileSpec(),this,flags );
	else
		m_SelectObjectsTreePanelPtr->Create(functor(*this,&CProceduralCreationDialog::OnSelectionBrowserDblClick),CProceduralCreationWorks::GetBuildingPrefix(),this,flags );

	m_SelectObjectsTreePanelPtr->SetSelectCallback( functor(*this,&CProceduralCreationDialog::OnSelectionBrowserDblClick) );

	m_SelectBrowserRollupCtrl.Create( WS_CHILD|WS_VISIBLE,CRect(360,23,648,714),this,200);
	m_SelectObjectsTreePanelPtr->AddModularEditorPreviewPanel(this,&m_SelectBrowserRollupCtrl);
	m_SelectPageNo=m_SelectBrowserRollupCtrl.InsertPage("Object Selection Browser",m_SelectObjectsTreePanelPtr);

	// Object Replace Dialog
	m_ReplaceObjectsTreePanelPtr = new CPanelTreeBrowser();
	m_ReplaceObjectsTreePanelPtr->SetDialogType(CPanelTreeBrowser::ME_REPLACE_DIALOG);
	
	if (!CProcSettings::GetProcSettings().buildingSelected)
		m_ReplaceObjectsTreePanelPtr->Create(functor(*this,&CProceduralCreationDialog::OnFileChange),clsDesc->GetFileSpec(),this,flags );
	else
		m_ReplaceObjectsTreePanelPtr->Create(functor(*this,&CProceduralCreationDialog::OnFileChange),CProceduralCreationWorks::GetBuildingPrefix(),this,flags );

	m_ReplaceObjectsTreePanelPtr->SetSelectCallback( functor(*this,&CProceduralCreationDialog::OnFileChange) );
	m_ReplaceBrowserRollupCtrl.Create( WS_CHILD|WS_VISIBLE,CRect(653,23,933,714),this,100);
	m_ReplaceObjectsTreePanelPtr->AddModularEditorPreviewPanel(this,&m_ReplaceBrowserRollupCtrl);
	m_ReplacePageNo=m_ReplaceBrowserRollupCtrl.InsertPage("Object Replace Browser",m_ReplaceObjectsTreePanelPtr);

	return TRUE;
}

void CProceduralCreationDialog::OnFileChange(CString file)
{
	CProceduralCreationWorks::ReplaceBrowserClick(file);
}


void CProceduralCreationDialog::OnSelectionBrowserDblClick(CString file)
{
	CProceduralCreationWorks::SelectBrowserClick(file);
}

void CProceduralCreationDialog::DetachSelectedfObjects()
{
	CProceduralCreationWorks::DetachObjects(RELEASE_SELECTED);
}

void CProceduralCreationDialog::RefreshReplaceBrowser()
{
	if (m_ReplaceObjectsTreePanelPtr)
	{
		// In case the same building we skip reloading the browser
		if ( m_ReplaceObjectsTreePanelPtr->GetSearchSpec()==CProceduralCreationWorks::GetBuildingPrefix() ) return;

		int flags = CPanelTreeBrowser::NO_PREVIEW;

		CObjectClassDesc *clsDesc = GetIEditor()->GetObjectManager()->FindClass( BRUSH_CLASS_NAME );

		if (!CProcSettings::GetProcSettings().buildingSelected)
		{
			if (clsDesc)
			{
				m_ReplaceObjectsTreePanelPtr->SetSearchSpec(clsDesc->GetFileSpec() );
				m_SelectObjectsTreePanelPtr->SetSearchSpec(clsDesc->GetFileSpec() );
			}
		}
		else
		{
			m_ReplaceObjectsTreePanelPtr->SetSearchSpec( CProceduralCreationWorks::GetBuildingPrefix() );
			m_SelectObjectsTreePanelPtr->SetSearchSpec( CProceduralCreationWorks::GetBuildingPrefix() );
		}

		m_ReplaceObjectsTreePanelPtr->Refresh(false);
		m_SelectObjectsTreePanelPtr->Refresh(false);
	}
}

void CProceduralCreationDialog::AddFloor()
{
	if (!CProcSettings::GetProcSettings().buildingSelected) return;
	if (CProcSettings::GetProcSettings().lastBuildingGUID==GUID_NULL) return;

	CPrefabBuildingObject *building =(CPrefabBuildingObject *)GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) 
		return;
	if (!building->GetPrefab()) 
		return;

	int floorCount = building->GetFloorCount();

	floorCount++;
	for (int i = 0; i < floorCount; i++)
		building->SetFloorHeight(building->GetFloorHeight(i),i);

	UpdateFloorSelectionList();

	if (building->GetPrefab())
		building->SetPrefab(building->GetPrefab(),false);
}

void CProceduralCreationDialog::ExportBuildings()
{
	if (GetIEditor()->GetSelection()->GetCount()==0) return;
	
	CString outFileName("");

	if (GetIEditor()->GetSelection()->GetCount()>1)
		outFileName="buildings.xml";
	else
	{
		CBaseObject *pObject = GetIEditor()->GetSelectedObject();
		if (!pObject) return;

		if (pObject->IsKindOf(RUNTIME_CLASS(CPrefabBuildingObject)))
			outFileName=GetIEditor()->GetSelectedObject()->GetName()+".xml";
		else
			if (pObject->GetParent())
				if (pObject->GetParent()->IsKindOf(RUNTIME_CLASS(CPrefabBuildingObject)))
				{
					outFileName=pObject->GetParent()->GetName()+".xml";
				}
				else
				{
					AfxMessageBox("Please select a building to export");
					return;
				}
	}

	char szFilters[] = "Building Files (*.xml)|*.xml |All files (*.*)|*.*||";
	CAutoDirectoryRestoreFileDialog dlg(FALSE, "xml", outFileName , OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR, szFilters);

	if (dlg.DoModal() == IDOK)
	{
		CProceduralCreationWorks::ExportBuildings(dlg.GetPathName().GetBuffer(1));
	}
}
void CProceduralCreationDialog::DeleteFloor()
{
	int focusedFloor = CProcSettings::GetProcSettings().currentFloorNumber;
	if (focusedFloor>0)
	{
		CPrefabBuildingObject *building = (CPrefabBuildingObject*) GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
		if (!building) return;

		CProceduralCreationWorks::DeleteFloor(focusedFloor);
		
		int newfloorNo=focusedFloor;
		if ( focusedFloor == (building->GetFloorCount()) ) 
			newfloorNo -= 1;

		CProceduralCreationWorks::ChangeToFloor(newfloorNo);
		UpdateFloorSelectionList();
	}
	else
		AfxMessageBox("You cannot delete the base floor");

	CProceduralCreationWorks::HighlightFloor(CProcSettings::GetProcSettings().highlightFloor);
}

void CProceduralCreationDialog::ChangeToFloor(int floorNumber)
{
	if (!CProcSettings::GetProcSettings().buildingSelected) return;
	if (CProcSettings::GetProcSettings().lastBuildingGUID==GUID_NULL) return;

	CProceduralCreationWorks::ChangeToFloor(floorNumber);
	SetCurFloorText(floorNumber);

	CPrefabBuildingObject *building =(CPrefabBuildingObject *)GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return;
	BuildingSelComboSelect(building->GetId());
}
CProceduralCreationDialog::~CProceduralCreationDialog()
{
	CProceduralCreationWorks::CloseAllBuildings();
	m_ReplaceBrowserRollupCtrl.RemovePage(m_ReplacePageNo);
	m_SelectBrowserRollupCtrl.RemovePage(m_SelectPageNo);
	CProcSettings::GetProcSettings().highlightFloor=false;
	CProcSettings::GetProcSettings().buildingSelected=false;
}

void CProceduralCreationDialog::SetCurFloorText(int floorNumber)
{
	CString CurFloorVal;
	CurFloorVal.Format("%d",floorNumber);
	m_CurFloorTxt.SetWindowText(CurFloorVal);
}

void CProceduralCreationDialog::UpdateBuildingSelectionCombo()
{
	if (!IsWindowVisible()) return;

	m_buildingSel.ResetContent();
	m_buildingSel.AddString(SELECT_NONE);
	selComboBuildings.clear();

	IObjectManager *objMan = GetIEditor()->GetObjectManager();

	TProcBuildingType::iterator bIter = CProcSettings::GetProcSettings().Buildings.begin();

	for (;bIter!=CProcSettings::GetProcSettings().Buildings.end();bIter++)
	{
		GUID buildingID=bIter->first;
		CBaseObject *curObj =objMan->FindObject(buildingID);
		if ( (curObj) && (m_buildingSel.FindString(-1,curObj->GetName())==-1))
		{
			m_buildingSel.AddString(curObj->GetName());
			selComboBuildings[buildingID]= curObj->GetName();
		}
	}

	if (!CProcSettings::GetProcSettings().buildingSelected) return;
	if (CProcSettings::GetProcSettings().lastBuildingGUID==GUID_NULL) return;

	CPrefabBuildingObject *building =(CPrefabBuildingObject *)GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return;
	
	BuildingSelComboSelect(building->GetId());
}

void CProceduralCreationDialog::BuildingSelComboCloseUP()
{
	if (!IsWindowVisible()) return;

	int curSelNo = m_buildingSel.GetCurSel();
	if (curSelNo!=-1)
	{
		CString selText("");
		m_buildingSel.GetLBText(curSelNo,selText);
		if (selText=="") return;

		if (selText==SELECT_NONE)
		{
			CProcSettings::GetProcSettings().buildingSelected=false;
			CProcSettings::GetProcSettings().lastBuildingGUID=GUID_NULL;

			CPrefabBuildingObject *unSelectBuildingObj = (CPrefabBuildingObject*) GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
			if (unSelectBuildingObj)
				GetIEditor()->GetObjectManager()->UnselectObject(unSelectBuildingObj);
			m_buildingSel.SetCurSel(-1);

			CProceduralCreationWorks::CloseAllBuildings();
			m_floorView.ResetContent(true);
			return;
		}

		TComboSelMap::iterator it = selComboBuildings.begin();
		for (;it!=selComboBuildings.end();++it)
			if (it->second==selText)
			{
				GetIEditor()->GetObjectManager()->ClearSelection();
				CPrefabBuildingObject *buildingObj = (CPrefabBuildingObject*) GetIEditor()->GetObjectManager()->FindObject(it->first);
				if (buildingObj)
				{
					CProceduralCreationWorks::CloseAllBuildings();
					buildingObj->Open();

					if (CProcSettings::GetProcSettings().currentFloorNumber>=buildingObj->GetFloorCount())
					{
						CProcSettings::GetProcSettings().currentFloorNumber=0;
						CProcSettings::GetProcSettings().currentFloorZ=0;
					}

					GetIEditor()->SelectObject(buildingObj);
					CProcSettings::GetProcSettings().lastBuildingGUID=buildingObj->GetId();
					CProcSettings::GetProcSettings().buildingSelected=true;
					CProceduralCreationWorks::UpdateFloorSelectionList();
					CProceduralCreationWorks::HighlightFloor(m_highlightObjectsChk.GetCheck());
					RefreshReplaceBrowser();

					CProcSettings::GetProcSettings().currentFloorZ=CProceduralCreationWorks::GetFloorZ(buildingObj->GetId(), CProcSettings::GetProcSettings().currentFloorNumber);
					SetCurFloorText(CProcSettings::GetProcSettings().currentFloorNumber);
				}
			}
	}
}

void CProceduralCreationDialog::DeleteBuilding()
{
	int curSelNo = m_buildingSel.GetCurSel();
	if (curSelNo!=-1)
	{
		CString selText("");
		m_buildingSel.GetLBText(curSelNo,selText);
		if (selText=="") return;

		TComboSelMap::iterator it = selComboBuildings.begin();
		for (it=selComboBuildings.begin();it!=selComboBuildings.end();++it)
			if (it->second==selText)
			{
				GetIEditor()->GetObjectManager()->ClearSelection();
				CBaseObject *buildingObj = GetIEditor()->GetObjectManager()->FindObject(it->first);
				if (buildingObj)
				{
					GetIEditor()->SelectObject(buildingObj);
					CProceduralCreationWorks::DeleteObject(buildingObj);
					break;
				}
			}
	}

	CProcSettings::GetProcSettings().buildingSelected=false;
	UpdateFloorSelectionList();
}

void CProceduralCreationDialog::BuildingSelComboSelect(GUID buildingID)
{
	int selID=m_buildingSel.FindString(-1,selComboBuildings[buildingID]);
	if (selID!=CB_ERR)
	{
		m_buildingSel.SetCurSel(selID);
	}
}

void CProceduralCreationDialog::DeleteSelectedObjects()
{
	float posa = CProceduralCreationWorks::GetHighestPointOnFloor(CProcSettings::GetProcSettings().currentFloorNumber);

	GetIEditor()->GetObjectManager()->DeleteSelection();
	CPrefabBuildingObject *building = (CPrefabBuildingObject*) GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return;
	float pos = CProceduralCreationWorks::GetHighestPointOnFloor(CProcSettings::GetProcSettings().currentFloorNumber);

	if (!building->IsFloorHeightDefinedByUser(CProcSettings::GetProcSettings().currentFloorNumber))
		CProceduralCreationWorks::UpdateFloorHeight(CProcSettings::GetProcSettings().currentFloorNumber,CProceduralCreationWorks::GetHighestPointOnFloor(CProcSettings::GetProcSettings().currentFloorNumber));
	UpdateFloorSelectionList();
	building->UpdatePrefab();
	CProceduralCreationWorks::HighlightFloor(CProcSettings::GetProcSettings().highlightFloor);
}

void CProceduralCreationDialog::ClearObjectsSelection()
{
	GetIEditor()->ClearSelection();
}

void CProceduralCreationDialog::PasteObjects()
{
	CProceduralCreationWorks::PasteSelectedObjects(CProcSettings::GetProcSettings().lastBuildingGUID, CProcSettings::GetProcSettings().currentFloorNumber);
}

void CProceduralCreationDialog::CopyObjects(int copyMode)
{
	bool entireFloor=false;
	if (copyMode==eAllObjects) entireFloor=true;
	CProceduralCreationWorks::CopySelectedObjects(CProcSettings::GetProcSettings().lastBuildingGUID,CProcSettings::GetProcSettings().currentFloorNumber,entireFloor);
}

void CProceduralCreationDialog::AddObjectsToBuilding()
{
	if (!CProcSettings::GetProcSettings().buildingSelected) return;
	
	CString curBuildingPrefix = CProceduralCreationWorks::GetBuildingPrefix();
	curBuildingPrefix=curBuildingPrefix;

	CBaseObject *pObject = GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!pObject) return;

	CPrefabBuildingObject *pPrefBuilding = 0;

	if (pObject->IsKindOf(RUNTIME_CLASS(CPrefabBuildingObject)))
		pPrefBuilding = (CPrefabBuildingObject*)pObject;
	else return;

	CSelectionGroup *pSel = GetIEditor()->GetSelection();
	std::vector<CBaseObject*>objects;

	for (int i = 0; i < pSel->GetCount(); i++)
	{
		CBaseObject *pObj = pSel->GetObject(i);
		if (CProceduralCreationWorks::IsProceduralObjectSelected(pObj)) continue;

		if ( (pObj!=pPrefBuilding) && (!pObj->IsKindOf(RUNTIME_CLASS(CPrefabBuildingObject))) )
		{
			pObj->SetFloorNumber(CProcSettings::GetProcSettings().currentFloorNumber);
			objects.push_back(pObj);
		}
	}

	CUndo undo("Add Objects To Prefab");
	pPrefBuilding->StoreUndo( "Add Objects To Prefab" );

	if (objects.size()==0) return;

	for (int i = 0; i < objects.size(); i++)
		pPrefBuilding->AddObjectToPrefab(objects[i], true);

	pPrefBuilding->UpdatePrefab();
	CProceduralCreationWorks::HighlightFloor(CProcSettings::GetProcSettings().highlightFloor);
}

void CProceduralCreationDialog::AddBuilding()
{
	IDataBaseLibrary *procLib= GetIEditor()->GetPrefabManager()->FindLibrary("Level");
	if (!procLib) return;

	CProceduralCreationWorks::CloseAllBuildings();

	CPrefabItem *pPrefabItem  = (CPrefabItem*)GetIEditor()->GetPrefabManager()->CreateItem(procLib);
	if (!pPrefabItem) return;

	CString prefName = GetIEditor()->GetDBItemManager(EDB_TYPE_PREFAB)->MakeUniqItemName("Building");
	pPrefabItem->SetName(prefName);

	CPrefabBuildingObject *buildingPrefab = (CPrefabBuildingObject*)GetIEditor()->NewObject(PREFABBUILDING_OBJECT_CLASS_NAME);
	if (!buildingPrefab) return;

	buildingPrefab->Open();
	buildingPrefab->SetPrefab(pPrefabItem,true);
	buildingPrefab->SetUniqName(prefName);
	
	Vec3 buildingPos(GetIEditor()->GetViewManager()->GetView(0)->ViewToWorld(CProcSettings::GetProcSettings().lastMouseDownPos));
	buildingPrefab->SetPos(buildingPos);

	GetIEditor()->SelectObject(buildingPrefab);

	CProceduralCreationWorks::CenterOnCurrentBuilding();
	CProceduralCreationWorks::UpdateProceduralCreation(buildingPrefab);
	CProceduralCreationWorks::ProceduralBuildingObjectHitTest(buildingPrefab);

	GetIEditor()->GetPrefabManager()->SaveAllLibs();
};

void CProceduralCreationDialog::ResetControls()
{
	if (!this) return;
	if (!IsWindowVisible()) return;

	m_buildingSel.ResetContent();
	m_buildingSel.AddString(SELECT_NONE);
	m_floorView.ResetContent(TRUE);

	if (m_ReplaceObjectsTreePanelPtr)
	{
		int flags = CPanelTreeBrowser::NO_PREVIEW;
		m_ReplaceObjectsTreePanelPtr->SetSearchSpec(CProceduralCreationWorks::GetDefaultBuildingPrefix() );
		m_ReplaceObjectsTreePanelPtr->Refresh(false);
	}

	if (m_SelectObjectsTreePanelPtr)
	{
		int flags = CPanelTreeBrowser::NO_PREVIEW;
		m_SelectObjectsTreePanelPtr->SetSearchSpec(CProceduralCreationWorks::GetDefaultBuildingPrefix() );
		m_SelectObjectsTreePanelPtr->Refresh(false);
	}

}


void CProceduralCreationDialog::UpdateFloorSelectionList()
{
	if (!this) return;
	if (!IsWindowVisible()) return;

	m_floorView.BeginUpdate();
	m_floorView.ResetContent(TRUE);
	m_floorView.EndUpdate();
	m_floorView.Populate();

	if (!CProcSettings::GetProcSettings().buildingSelected) return;
	if (CProcSettings::GetProcSettings().lastBuildingGUID==GUID_NULL) return;

	CPrefabBuildingObject *building =(CPrefabBuildingObject *)GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	int floorCount = building->GetFloorCount();

	CString strItem;
	m_floorView.BeginUpdate();

	for (int i = 0; i < floorCount; i++)
	{
		float floorHeight= building->GetFloorHeight(i);

		if (floorHeight<=0)
		{ 
			floorHeight = CProceduralCreationWorks::GetHighestPointOnFloor(i);

			if (floorHeight>0)
				building->SetFloorHeight(floorHeight,i);
		}

		strItem.Format(_T("%i"), i);

		CXTPReportRecord *curRecord = new CXTPReportRecord();

		CXTPReportRecordItem *pFloorNumberItem = new CXTPReportRecordItemText(strItem);
		pFloorNumberItem->HasCheckbox(FALSE);
		curRecord->AddItem(pFloorNumberItem);

		strItem.Format(_T("%.2f"), floorHeight);
		CXTPReportRecordItem *pFloorHeightItem  = new CFloorViewRecordItemText(strItem);
		pFloorHeightItem->SetCaption(strItem);
		pFloorHeightItem->SetEditable(true);
		pFloorHeightItem->HasCheckbox(FALSE);
		curRecord->AddItem(pFloorHeightItem);

		CXTPReportRecordItem *pFloorAutoHeightItem  = new CXTPReportRecordItem();
		pFloorAutoHeightItem->HasCheckbox(TRUE);

		// if floor Height is not defined by user then check the auto height checkbox
		pFloorAutoHeightItem->SetChecked(!building->IsFloorHeightDefinedByUser(i));

		curRecord->AddItem(pFloorAutoHeightItem);

		CXTPReportRecordItem *pShowFloorItem = new CXTPReportRecordItem();
		pShowFloorItem->HasCheckbox(TRUE);
		pShowFloorItem->SetChecked(CProceduralCreationWorks::GetFloorVisibilityState(i));
		curRecord->AddItem(pShowFloorItem);
		
		m_floorView.AddRecord(curRecord);
	}

	m_floorView.EndUpdate();
	m_floorView.Populate();

	CString CurFloorVal("");
	CurFloorVal.Format("%d",CProcSettings::GetProcSettings().currentFloorNumber);

	CXTPReportRecordItem *pNewSelItem = m_floorView.GetRecords()->FindRecordItem(0,m_floorView.GetRecords()->GetCount(),0,0,0,0,CurFloorVal,0);
	if (!pNewSelItem) return;

	CXTPReportRecord *pNewSelRecord = pNewSelItem->GetRecord();
	if (pNewSelRecord)
	{
		CXTPReportRow* pRow = m_floorView.GetRows()->Find(pNewSelRecord);
		if (pRow)
			m_floorView.SetFocusedRow(pRow);
	}

	SetCurFloorText(CProcSettings::GetProcSettings().currentFloorNumber);
}

void CProceduralCreationDialog::HightLightObjects()
{
	if (CProceduralCreationWorks::IsProceduralCreationActivated())
		CProceduralCreationWorks::HighlightFloor(m_highlightObjectsChk.GetCheck());
}

//////////////////////////////////////////////////////////////////////////
void CProceduralCreationDialog::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
	case eNotify_OnEndNewScene:
	case eNotify_OnEndSceneOpen:
		break;
	}
}

// Dockable view frame definition
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CProceduralCreationFrame,CBaseFrameWnd)
//////////////////////////////////////////////////////////////////////////
class CProceduralCreationEditorViewClass : public TRefCountBase<IViewPaneClass>
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	virtual REFGUID ClassID()
	{
		static const GUID guid = { 0xcb15c291, 0x6423, 0x45ac, { 0xd3, 0xa6, 0xb5, 0xd0, 0xaf, 0xa0, 0xc8, 0xa5 } };
		return guid;
	}
	virtual const char* ClassName() { return _T(PROC_WINDOW_NAME); };
	virtual const char* Category() { return _T(PROC_WINDOW_NAME); };
	//////////////////////////////////////////////////////////////////////////
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CProceduralCreationFrame); };
	virtual const char* GetPaneTitle() { return _T(PROC_WINDOW_NAME); };
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
	virtual CRect GetPaneRect() { return CRect(5,110,956,862); };
	virtual bool SinglePane() { return false; };
	virtual bool WantIdleUpdate() { return true; };
};
REGISTER_CLASS_DESC(CProceduralCreationEditorViewClass)

/////////////////////////////////////////////////////////////////////////////
// CProceduralCreationFrame dialog
CProceduralCreationFrame::CProceduralCreationFrame()
{
	CRect rcClient(0,0,1,1);
	Create(WS_CHILD|WS_VISIBLE|WM_VSCROLL,rcClient, AfxGetMainWnd() );
}

/////////////////////////////////////////////////////////////////////////////
// CProceduralCreationFrame message handlers
BEGIN_MESSAGE_MAP(CProceduralCreationFrame, CBaseFrameWnd)
END_MESSAGE_MAP()

BOOL CProceduralCreationFrame::OnInitDialog() 
{
	CRect rcClient(0,0,1,1);
	GetClientRect(rcClient);
	dlg.Create(IDD_PROCEDURAL_CREATION_DIALOG,this);
	dlg.SetWindowPos(NULL, rcClient.left-5, rcClient.top-5,935,835,SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
	dlg.ShowWindow(1);

	return TRUE;  // return TRUE unless you set the focus to a control
}

CProceduralCreationDialog* CProceduralCreationFrame::GetProcDialog()
{
	return &dlg;
}

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE( CFloorListView, CXTPReportControl )

BEGIN_MESSAGE_MAP( CFloorListView, CXTPReportControl )
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(XTP_NM_REPORT_CHECKED, OnValueChanged)
	ON_NOTIFY_REFLECT(XTP_NM_REPORT_VALUECHANGED, OnValueChanged)
	ON_NOTIFY_REFLECT(XTP_NM_REPORT_REQUESTEDIT, OnRequestEdit)
END_MESSAGE_MAP()

// FloorListView columns
enum
{
	eFloorNumber      = 0,
	eFloorHeight,
	eFloorAutoHeight,
	eShowFloor,
};

// Context Menu Items
enum
{
	eAddFloor      = 1,
	eDeleteFloor,
	eCopyAllObjects,
	eCopySelectedObjects,
	ePasteObjects,
};

//////////////////////////////////////////////////////////////////////////

CFloorListView::CFloorListView()
{
	m_bDragging = false;
	m_bModified = false;
	m_ptDrag.SetPoint(0,0);
	m_hCursorNoDrop = (HCURSOR) LoadImage( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDC_POINTER_SO_SELECT ), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE );
	m_hCursorNormal = (HCURSOR) LoadImage( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDC_POINTER ), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE );

	m_pSourceRow = 0;
	m_pTargetRow = 0;

	m_pFloorNumber  = (CXTPReportColumn*)AddColumn( new CXTPReportColumn(eFloorNumber, "Floor", 30, TRUE, XTP_REPORT_NOICON, FALSE));
	m_pFloorNumber->SetEditable(FALSE);

	m_pFloorHeight  = (CXTPReportColumn*)AddColumn( new CXTPReportColumn(eFloorHeight, "Height", 40, TRUE, XTP_REPORT_NOICON, FALSE));
	m_pFloorHeight->SetEditable(TRUE);

	m_pFloorAutoHeight = (CXTPReportColumn*)AddColumn( new CXTPReportColumn(eFloorAutoHeight, "Auto Height", 60, TRUE, XTP_REPORT_NOICON, FALSE));
	m_pFloorAutoHeight->SetEditable(TRUE);

	m_pShowFloor  = (CXTPReportColumn*)AddColumn( new CXTPReportColumn(eShowFloor, "Show", 30, TRUE, XTP_REPORT_NOICON, FALSE));
	m_pShowFloor->SetEditable(TRUE);

	AllowEdit(TRUE);
	EditOnClick(FALSE);
	FocusSubItems(TRUE);
}

void CFloorListView::OnValueChanged(NMHDR*  pNotifyStruct, LRESULT* /*result*/)
{
	CXTPReportColumn *focusedColumn = GetFocusedColumn();
	if (!focusedColumn) return;

	int floorNumber = CProcSettings::GetProcSettings().currentFloorNumber;

	if (focusedColumn->GetIndex()== eShowFloor)
	{		
		CProceduralCreationWorks::ShowFloor(floorNumber,GetFocusedRow()->GetRecord()->GetItem(eShowFloor)->IsChecked());
	}
	else
		if (focusedColumn->GetIndex()== eFloorHeight)
			GetFocusedRow()->GetRecord()->GetItem(eFloorAutoHeight)->SetChecked(false);
		else
			if (focusedColumn->GetIndex()== eFloorAutoHeight)
			{
				CPrefabBuildingObject *building =(CPrefabBuildingObject *)GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
				if (!building) return;

				int floorNumber = atol(GetFocusedRow()->GetRecord()->GetItem(eFloorNumber)->GetCaption(GetFocusedColumn()));
				bool isAutoHeight = GetFocusedRow()->GetRecord()->GetItem(eFloorAutoHeight)->IsChecked();
				building->SetFloorHeightUserDefined(floorNumber,!isAutoHeight);

				if (isAutoHeight)
					CProceduralCreationWorks::UpdateFloorHeight(floorNumber,CProceduralCreationWorks::GetHighestPointOnFloor(floorNumber));
				else
				{
					float heightValue = atof(GetFocusedRow()->GetRecord()->GetItem(eFloorHeight)->GetCaption(GetColumns()->GetAt(eFloorHeight)));
					CProceduralCreationWorks::UpdateFloorHeight(floorNumber,heightValue);
				}

				CProceduralCreationDialog *pDlg = CProceduralCreationWorks::GetProcDialog();
				if(!pDlg) return;

				building->UpdatePrefab();
				pDlg->UpdateFloorSelectionList();
				CProceduralCreationWorks::HighlightFloor(CProcSettings::GetProcSettings().highlightFloor);
			};
}

void CFloorViewRecordItemText::OnEditChanged(XTP_REPORTRECORDITEM_ARGS* pItemArgs, LPCTSTR szText)
{
	SetValue(szText);
	m_strText=m_strText;
	CXTPReportRecordItem::OnEditChanged(pItemArgs, szText);
	CString floorNumber = GetRecord()->GetItem(eFloorNumber)->GetCaption(GetColumn());

	CPrefabBuildingObject *building =(CPrefabBuildingObject *)GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return;
	CProceduralCreationWorks::UpdateFloorHeight(atol(floorNumber),atof(szText));
	building->SetFloorHeight(atof(szText),atol(floorNumber));
	building->SetFloorHeightUserDefined(atol(floorNumber),true);
	building->UpdatePrefab();
	CProceduralCreationDialog *pDlg = CProceduralCreationWorks::GetProcDialog();

	if(!pDlg) return;
	pDlg->UpdateFloorSelectionList();

	CProceduralCreationWorks::HighlightFloor(CProcSettings::GetProcSettings().highlightFloor);
}

void CFloorListView::OnRequestEdit(NMHDR* pNotifyStruct, LRESULT* result)
{
	XTP_NM_REPORTREQUESTEDIT* pItemNotify = (XTP_NM_REPORTREQUESTEDIT*) pNotifyStruct;
	pItemNotify->bCancel = false;
	*result=TRUE;
}

BOOL CFloorListView::PreTranslateMessage(MSG* msg)
{
	if (msg->message == WM_LBUTTONDBLCLK)
	{
		CPoint point (msg->lParam);
		CRect reportArea = GetReportRectangle();
		if (reportArea.PtInRect(point))
		{
			CXTPReportRow* pRow = HitTest( point );
			if (pRow == 0)
				return TRUE;
		}
	}
	return __super::PreTranslateMessage(msg);
}

void CFloorListView::OnLButtonDown( UINT nFlags, CPoint point )
{
	if ( !m_bDragging )
	{
		__super::OnLButtonDown( nFlags, point );

			CRect reportArea = GetReportRectangle();
			if (reportArea.PtInRect(point))
			{
				m_pTargetRow = 0;
				m_pSourceRow = HitTest( point );
				if ( m_pSourceRow )
				{
					 CXTPReportRecord *selectedFloorRecord = m_pSourceRow->GetRecord();
					 if (!selectedFloorRecord) return;
					 CXTPReportRecordItem *selectedFloorItem = m_pSourceRow->GetRecord()->GetItem(0);

					 CXTPReportColumns* pColumns = GetColumns();
					 int nColumnCount = pColumns->GetCount();
					 if (nColumnCount>0)
					 {
						 CXTPReportColumn* pCol = pColumns->GetAt(0);
						 CString selectedFloorNumberStr = selectedFloorItem->GetCaption(pCol);
						 if (selectedFloorNumberStr=="") return;
						 int selectedFloorNumber = atol(selectedFloorNumberStr);
						 
						 // Change to other floor and return if the selected floor number is different from the current floor
						 if (selectedFloorNumber!=CProcSettings::GetProcSettings().currentFloorNumber)
						 {
							 CProceduralCreationWorks::GetProcDialog()->ChangeToFloor(selectedFloorNumber);
							 CProceduralCreationWorks::ChangeToFloor(selectedFloorNumber);							 
							 return;
						 }
					 }
				}
			}
	}
}


void CFloorListView::OnLButtonDblClk( UINT nFlags, CPoint point )
{
	if ( !m_bDragging )
		__super::OnLButtonDblClk( nFlags, point );
}

void CFloorListView::OnContextMenu( CWnd* pWnd, CPoint pos )
{
	if (!CProcSettings::GetProcSettings().buildingSelected) return;
	if (CProcSettings::GetProcSettings().lastBuildingGUID==GUID_NULL) return;

	CPrefabBuildingObject *building =(CPrefabBuildingObject *)GetIEditor()->GetObjectManager()->FindObject(CProcSettings::GetProcSettings().lastBuildingGUID);
	if (!building) return;

	int selCount = GetSelectedRows()->GetCount();

	if ( pos.x == -1 && pos.y == -1 )
	{
		pos.x = 7;
		pos.y = GetReportRectangle().top+1;
		ClientToScreen( &pos );
	}

	CPoint ptClient( pos );
	ScreenToClient( &ptClient );
	CXTPReportRow* pRow = HitTest( ptClient );
	if ( pRow )
	{
		CMenu menu;
		VERIFY( menu.CreatePopupMenu() );

		menu.AppendMenu( MF_STRING, eAddFloor, _T("Add Floor") );
		menu.AppendMenu( MF_STRING, eDeleteFloor, _T("Delete Floor") );
		menu.AppendMenu( MF_STRING, eCopyAllObjects, _T("Copy All Objects") );
		menu.AppendMenu( MF_STRING, eCopySelectedObjects, _T("Copy Selected Objects") );
		menu.AppendMenu( MF_STRING, ePasteObjects, _T("Paste Objects") );

		CProceduralCreationDialog *pDlg = CProceduralCreationWorks::GetProcDialog();
		if (!pDlg) return;

		int nMenuResult = CXTPCommandBars::TrackPopupMenu( &menu, TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_RIGHTBUTTON, pos.x, pos.y, this, NULL );

		switch ( nMenuResult )
		{
		case eAddFloor:
			pDlg->AddFloor();
			break;
		case eDeleteFloor:
			pDlg->DeleteFloor();
			break;
		case eCopyAllObjects:
			pDlg->CopyObjects(CProceduralCreationDialog::eAllObjects);
			break;
		case eCopySelectedObjects:
			pDlg->CopyObjects(CProceduralCreationDialog::eSelectedObjects);
			break;
		case ePasteObjects:
			pDlg->PasteObjects();
			break;
		}

	}
	else
	{
		CMenu menu;
		VERIFY( menu.CreatePopupMenu() );

		menu.AppendMenu( MF_STRING, eAddFloor, _T("Add Floor") );

		CProceduralCreationDialog *pDlg = CProceduralCreationWorks::GetProcDialog();
		if(!pDlg) return;

		int nMenuResult = CXTPCommandBars::TrackPopupMenu( &menu, TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_RIGHTBUTTON, pos.x, pos.y, this, NULL );

		switch ( nMenuResult )
		{
		case eAddFloor:
			pDlg->AddFloor();
			break;
		}
	}
}

CFloorViewRecordItemText::CFloorViewRecordItemText(LPCTSTR szText)
{
	m_strText=szText;
}

void CProceduralCreationDialog::GenerateBuilding()
{
	CProceduralProductionUtils tProceduralGen;

	tProceduralGen.GenerateBuilding();
}