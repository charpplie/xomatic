#include "stdafx.h"
#include "InternalCommon.h"
#include "SegmentedWorldDoc.h"

#include "SegmentedWorldManager.h"
#include "WorldData.h"
#include "SegmentData.h"
#include "LayerData.h"
#include "ObjectData.h"
#include "ISWDataPersistence.h"
#include "NullVersionControl.h"
#include "NativeSCMAdapter.h"
#include "DataUpgrade.h"

// UIs
#include "SegmentedWorldMiniMapUpdater.h"
#include "GridMapDlg.h"

#include "EngineSettingsManager.h"

#include "Terrain/Heightmap.h"
#include "Terrain/TerrainManager.h"
#include "Terrain/SurfaceType.h"
#include "Terrain/TerrainGrid.h"
#include "TerrainTexturePainter.h"
#include "VegetationTool.h"
#include "VegetationMap.h"
#include "ISourceControl.h"
#include "IRenderAuxGeom.h"
#include "ViewManager.h"
#include "Objects/Light.h"
#include "Objects/Group.h"
#include "Util/PakFile.h"					// CPakFile
#include "GameEngine.h"
#include "Util/ImageTIF.h"
#include "CryMemBlock.h"
#include "GameExporter.h"
#include "CryEditDoc.h"
#include "Mission.h"



/*
mihailp todo:
+ prepare all data to be "external storage" (database) ready
	+ CSWManager::Get/SetData(sx, sy, eType, pData)
	+ keep using files at this point
	+ general segment data (XML text part)
	+ heightmap data (heights and layer ids)
	+ vegetation instances
	+ RGB layer tiles
	+ objects (very rough support)
	+ remove the no-longer-necessary changes in heightmap.cpp and vegetationmap.cpp
+ change SerializeViewSettings() to use external sw.xml (move out of .cry file)
+ store segments data in database
+ do not save unnecessary data in .cry file
+ generate default data for missing segments
- store objects in database
	- keep small (as in "not big") static objects with segment data
	- big objects and "potential reference targets" (i.e. entities) go to separate table
	- query objects by coordinates and segment (or world portion) bounding box on load
	- additional passes in ResolveObjects() to pull unloaded referenced objects
	- use DB for unique object DWORD IDs generation
	- try to maintain plain file (non-DB) version if possible
- merge layer IDs
	- with each segment save layers set and mark unused layers (count instances)
	- after loading, build remap table for each segment and "global" layers set
	- remap layer ids in editor's bitmap for all segments where remap is not identity
	- can and should work incrementally, segment per segment
- merge vegetation map
	- similar approach to layer ids, investigate more
- object layers support
	+ minimal: reconstruct / merge layers when loading objects
	- optional: keep objects in different layers separate (e.g. different tables)
	- problem: how to "lock" and display lock status of sub-segment data (layer)
+ basic coop-related functionality
	+ segment locking / unlocking functions
	+ versions / timestamp handling
	+ status updates
	+ reload modified
- detect changes and try to lock segments, step 1
	+- post-change detection (hook after data has changed)
	+- use ConsoleSync approach (or same hooks directly) to hook on most changes
	+- try to lock affected resources (segments, central .cry, objects, whatever)
	+- handle failed locks (mark level as unsaveable, undo by reloading cached data, ?!?)
	+ prompt for saving on SW move / load operations if necessary
- change detection, step 2, optional, evaluate 1st
	- detect changes before they occured (hook on all attempts to change something)
	- classify change (which resources it will affect)
	- try to lock affected resources
	- deny the change if lock fails
+ version tracking in database mode
	+ keep separate table where each row is one version of one resource (segment / obj / global)
	+ keep and update current and official version indexes in the main table
- export to engine, when engine support is ready
	- manually and if needed with offline tool
- better RGB layer update to engine, when engine support is functional
- evaluate "hot update" functionality
	- keep additional "unpublished / unsaved" version of each resource in the DB
	- query DB for changes and reload
- reconsider (evaluate) background objects loading in editor
- UI for chosing world portion to load
	- show lock status if possible
	- "minimaps" generation with mipmaps?
- better "status" draw (DrawBoxes)
	- only draw "boxes" in view frustum
	- different visualisation options - lines are not visible underwater and underground
*/


using namespace sw;

bool s_bLevelMultiPack = true;

SW_NAMESPACE_BEGIN();

#ifdef SW_USE_DUMMY_LOCK
_DummyLock g_cs;
_DummyLock g_csBigLock;
#else
CryCriticalSection g_cs;
CryCriticalSection g_csBigLock;
#endif



//---------------------- version types, HRID constants
const char* const sw::g_pcVersionTypes[VT_COUNT] = {
	//"OV",
	"CV",
	//"HV",
};

SW_NAMESPACE_END();


int CSegmentedWorldDoc::s_nRollUpCtrlID = 0;
bool CSegmentedWorldDoc::s_bSWFindObjectEnabled = true;
bool CSegmentedWorldDoc::s_bAutomate = false;
bool CSegmentedWorldDoc::s_bMapConverting = false;
int CSegmentedWorldDoc::s_nSpecialOperationType = CSegmentedWorldDoc::SW_SO_NONE;


//---------------------- CUndoSWMove
class CUndoSWMove : public IUndoObject
{
	int m_iUndoX, m_iUndoY;
	UINT m_uiUndoW, m_uiUndoH;
	int m_iRedoX, m_iRedoY;
	UINT m_uiRedoW, m_uiRedoH;

public:
	CUndoSWMove()
	{
		m_iUndoX = m_iUndoY = 0;
		m_uiUndoH = m_uiUndoW = 0;
		m_iRedoX = m_iRedoY = 0;
		m_uiRedoH = m_uiRedoW = 0;
		ISWDoc &psw = CSWManager::Get().GetDoc();
		psw.GetOffsetInSegments(m_iUndoX, m_iUndoY);
		psw.GetSizeInSegments(m_uiUndoW, m_uiUndoH);
	}
protected:
	virtual void Release() { delete this; };
	virtual int GetSize() {	return sizeof(*this); };
	virtual const char* GetDescription() { return "SW Move"; };

	virtual void Undo( bool bUndo )
	{
		ISWDoc &psw = CSWManager::Get().GetDoc();

		if (bUndo)
		{
			psw.GetOffsetInSegments(m_iRedoX, m_iRedoY);
			psw.GetSizeInSegments(m_uiRedoW, m_uiRedoH);
		}
		if (m_uiUndoW && m_uiUndoH)
			psw.MoveTo(m_iUndoX, m_iUndoY, m_uiUndoW, m_uiUndoH);
	}

	virtual void Redo()
	{
		if (!m_uiRedoW || !m_uiRedoH)
			return;
		ISWDoc &psw = CSWManager::Get().GetDoc();
		psw.MoveTo(m_iRedoX, m_iRedoY, m_uiRedoW, m_uiRedoH);
	}
};




//typedef std::map<string, TLayerData*> TLayerMap;
//typedef std::pair<string, TLayerData*> TLayerPair;


//---------------------- CSegmentedWorld declaration



TLocalCoords::TLocalCoords(const TWorldCoords &wc)
{
	lx = wc.wx - GetInternalSWDoc()->m_wcOfs.wx;
	ly = wc.wy - GetInternalSWDoc()->m_wcOfs.wy;
}

TWorldCoords::TWorldCoords(const TLocalCoords &lc)
{
	wx = lc.lx + GetInternalSWDoc()->m_wcOfs.wx;
	wy = lc.ly + GetInternalSWDoc()->m_wcOfs.wy;
}








/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
// Updater thread
struct TSegmentedWorldUpdater : public CrySimpleThread<>
{
	static TSegmentedWorldUpdater *Get()
	{
		static TSegmentedWorldUpdater *s_pThis = 0;
		if (!s_pThis)
			s_pThis = new TSegmentedWorldUpdater();
		return s_pThis;
	}
	
#ifndef SW_NO_REALTIME_UPDATE
	TSegmentedWorldUpdater()
	{
		Start();
	}

	virtual void Run()
	{
		//gEnv->pDB->GetConnector("mysql")->InitThread();

		//{
		//	AUTO_LOCK_BIG(g_csBigLock);
		//	CSWDoc *psw = GetInternalSWDoc();
		//	if (!g_db->MakeSWUpdaterCachedConn(GetCurrentThreadId()))
		//		return;
		//}

		while (!GetISystem()->IsQuitting()) {
			CSegmentedWorldDoc *psw;
			{
				AUTO_LOCK_BIG(g_csBigLock);
				psw = GetInternalSWDoc();
				if (psw && psw->m_bSuspendThread)
					psw = 0;
				if (psw && !psw->IsActive())
					psw->Update(true);
			}
			Sleep(500);
		}

		//gEnv->pDB->GetConnector("mysql")->EndThread();
	}
#endif
};

//////////////////////////////////////////////////////////////////////////
// CCryEditDoc Adapter
IMPLEMENT_DYNCREATE(CSWDocAdapter,CDocument);
BOOL CSWDocAdapter::OnOpenDocument(LPCTSTR lpszPathName)
{
	CString strPathName;
	{
		CString strPath,strFile,strExt;
		Path::Split(CString(lpszPathName), strPath, strFile, strExt);
		if (strExt.IsEmpty())
			strPathName = lpszPathName;
		else
		{
			CString strWorldName,strTmp;
			Path::Split(Path::RemoveBackslash(strPath), strTmp,strWorldName);
			strPathName = strPath + strWorldName;
		}
	}

	TOpenDocContext context;
	
	if (!BeforeOpenDocument(strPathName, context))
		return FALSE;

	bool bForceUnsegmented = gEnv->pConsole->GetCVar("doc_enable_segmented_world")->GetIVal() && (GetAsyncKeyState(VK_SHIFT) & ~1) != 0;
	if (!CSWManager::Get().OpenDoc( bForceUnsegmented ? 0 : (const char*)strPathName, bForceUnsegmented))
		return FALSE;
	m_pSWDoc = GetInternalSWDoc();

	return DoOpenDocument(strPathName,context);
}
void CSWDocAdapter::Load(TDocMultiArchive &arrXmlAr,const CString &szFilename,bool bReloadEngineLevel )
{
	if (m_pSWDoc->NeedReloadLevel())
	{
		__super::Load(arrXmlAr, szFilename, bReloadEngineLevel);
		m_pSWDoc->OnWorldDataApplied();
	}
}

void CSWDocAdapter::StartStreamingLoad()
{
	m_pSWDoc->Load(true);	// do real load job
}

void CSWDocAdapter::SyncCurrentMissionContent( bool bRetrieve )
{
	bool bIgnoreObjects = true;
	GetCurrentMission()->SyncContent( bRetrieve, bIgnoreObjects );

	if ( bRetrieve )
	{
		m_pSWDoc->InitSegmentedWorldInfo();
		if (!GetIEditor()->GetObjectManager()->GetLayersManager()->GetCurrentLayer())
		{
			GetIEditor()->GetObjectManager()->GetLayersManager()->CreateMainLayer();
		}
	}

}

BOOL CSWDocAdapter::DoSave(LPCTSTR lpszPathName, BOOL bReplace)
{
	//if (m_pSWDoc->IsOk())
	{
		string name = GetIEditor()->GetLevelFolder() + "\\" + m_pSWDoc->GetWorldName();
		BOOL bResult = CDocument::DoSave(name,bReplace);
		// Restore current directory to root.
		SetCurrentDirectoryW( GetIEditor()->GetMasterCDFolder() );
		return bResult;
	}

}

BOOL CSWDocAdapter::OnSaveDocument( LPCTSTR lpszPathName )
{
	CCryEditDoc::TSaveDocContext context;
	if (!__super::BeforeSaveDocument(lpszPathName, context))
		return FALSE;

	//// check offline mode
	//if (psw && CSWManager::IsOfflineMode() && !CSWManager::IsMapConverting())
	//{
	//	AfxMessageBox( "Saving denied! You are working in Offline (local) mode!!!" ,MB_OK|MB_ICONSTOP );
	//	return FALSE;
	//}

	 if (!DoSaveDocument(lpszPathName, context))
		 return FALSE;

	 return __super::AfterSaveDocument(lpszPathName, context);
}

BOOL CSWDocAdapter::DoSaveDocument( LPCTSTR filename, CCryEditDoc::TSaveDocContext& context )
{
	bool& bSaved = context.bSaved;

	// Save to Pak file.

	// skip
		//// filename && "TerrainTexture.pak"
		//bSaved = CFileUtil::OverwriteFile( filename ) && CTerrainManager::GetTerrainManager().WouldHeightmapSaveSucceed();

	if (bSaved)
	{
		// Save Tag Point locations to file if auto save of tag points disabled
		if (!gSettings.bAutoSaveTagPoints)
			((CCryEditApp *)(AfxGetApp()))->SaveTagLocations();

		// Changes filename for this document.
		SetPathName( filename );

		bSaved = m_pSWDoc->Save(true);
	}

	return TRUE;
}

BOOL CSWDocAdapter::LoadXmlArchiveArray( TDocMultiArchive& arrXmlAr, const CString& relativeLevelName, const CString& levelPath )
{

	// then load archive
	BOOL bRet = m_pSWDoc->WorldData_LoadToArchiveArray(arrXmlAr);
	if (!bRet)
		return FALSE;

	CXmlArchive* parLevel = arrXmlAr[WDB_LEVELGENERAL];

	int32 SWDataStructureVersion = 0;
	XmlNodeRef xmlLevel = parLevel->root;

	return TRUE;
}


CSWDocAdapter::~CSWDocAdapter()
{
	GetIEditor()->GetSegmentedWorldManager()->DeleteDoc();
}

CSWDocAdapter::CSWDocAdapter()
	: CCryEditDoc()
	, m_pSWDoc(NULL)//(&ISegmentedWorldDoc::GetNullObject())
{
}

void CSWDocAdapter::DeleteContents()
{
	// maybe we don't need to actually delete everything, but at least set them as need to be reloaded.
	//m_pSWDoc->SetAsForceReloadLevel();

	__super::DeleteContents();
}




/////////////////////////////////////////////////////////////////////////////////////////
//---------------------- CSegmentedWorld implementation

//-------------------------------------------------------------------------------------
bool CSegmentedWorldDoc::IsObjectBig( CBaseObject* pObject)
{
	//handle big objects
	TWorldCoords wcPos, wcBoxMin, wcBoxMax;
	wcPos = WorldToSegment(pObject->GetWorldPos(), false);//m_lc;
	AABB box;
	GetBoundBoxWithChilds(pObject, box);
	//pObject->GetBoundBox(box);
	wcBoxMin = WorldToSegment(box.min, false);
	wcBoxMax = WorldToSegment(box.max, false);
	
	return (wcPos != wcBoxMin || wcPos != wcBoxMax || wcBoxMin != wcBoxMax);
}

//-------------------------------------------------------------------------------------
bool CSegmentedWorldDoc::CanUngroup( CBaseObject *pObject)
{
	if (pObject && pObject->IsKindOf(RUNTIME_CLASS(CGroup)))
	{
		if (!CanModify(pObject, true, false))
			return false;
		
		CGroup* pGroupObject = ((CGroup*)pObject);
		for (int i = 0; i < pGroupObject->GetChildCount(); i++)
		{
			if (!CanObjectPlaceTo(pGroupObject->GetChild(i)->GetWorldPos(), pGroupObject->GetChild(i)->GetLayer()))
				return false;
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------
bool CSegmentedWorldDoc::CanMakeGroup()
{
	CSelectionGroup *selection = GetIEditor()->GetSelection();
	selection->FilterParents();

	int i;
	std::vector<CBaseObjectPtr> objects;
	for (i = 0; i < selection->GetFilteredCount(); i++)
	{
		objects.push_back( selection->GetFilteredObject(i) );
	}

	if (objects.size())
	{
		Vec3 center = gSettings.pGrid->Snap( selection->GetCenter() );
		if (!CanObjectPlaceTo(center, objects[0]->GetLayer()))
			return false;
	}
	for (i = 0; i < objects.size(); i++)
	{
		if (!CanModify(objects[i], true, false))
			return false;
	}

	//TO DO
	return true;
}
//-------------------------------------------------------------------------------------
// need to call SaveObjectWithChilds instead serialize of object
void CSegmentedWorldDoc::SaveObjectWithChildren( CBaseObject* pObject, CObjectArchive &ar, TWorldCoords const& wcSeg )
{
	//assert(s_nSWMode != 0);
	pObject->Serialize(ar);

	if (!pObject->GetParent() && !pObject->GetGroup())
	{
		static const char* szPosAttr = "Pos";
		Vec3 pos;
		bool bRet = ar.node->getAttr(szPosAttr,pos);
		assert(bRet);
		pos = SegRelativeToEditorLocal(pos,wcSeg,-1);
		ar.node->setAttr(szPosAttr,pos);
	}

	if (!CanAccessChilds(pObject))
		return;

	XmlNodeRef xmlNode = ar.node;
	if (pObject->GetChildCount() > 0)
	{
		// Saving.
		XmlNodeRef root = xmlNode->newChild( "ChildObjects" );
		ar.node = root;

		// Save all child objects to XML.
		int num = pObject->GetChildCount();
		for (int i = 0; i < num; i++)
		{
			CBaseObject *obj = pObject->GetChild(i);
			ar.node = root->newChild("Object");
			SaveObjectWithChildren(obj, ar, wcSeg);
			//ar.SaveObject( obj,true );
		}
	}
	ar.node = xmlNode;
}
//-------------------------------------------------------------------------------------
void CSegmentedWorldDoc::LoadObjectChildrenRecursively( CBaseObject* pObject, CObjectArchive &ar )
{
	if (!CanAccessChilds(pObject))
		return;

	assert(ar.bLoading);
	XmlNodeRef xmlNode = ar.node;
	// Loading.
	pObject->DetachAll();

	// Loading.
	XmlNodeRef childsRoot = xmlNode->findChild( "ChildObjects" );
	if (childsRoot)
	{
		// Load all childs from XML archive.
		int numObjects = childsRoot->getChildCount();
		for (int i = 0; i < numObjects; i++)
		{
			ar.node = childsRoot->getChild(i);
			CBaseObject *obj = ar.LoadObject(ar.node, 0);
			LoadObjectChildrenRecursively(obj, ar);
		}
		//ar.LoadObjects( childsRoot );
		//InvalidateBBox();
	}
	ar.node = xmlNode;
}
//-------------------------------------------------------------------------------------
void CSegmentedWorldDoc::DeleteObjectWithChilds( CBaseObject* pObject)
{
	if (CanAccessChilds(pObject))
	{
		int num = pObject->GetChildCount();
		for (int i = num - 1; i >= 0; i--)
		{
			CBaseObject *obj = pObject->GetChild(i);
			DeleteObjectWithChilds(obj);
		}
	}
	GetIEditor()->GetObjectManager()->DeleteObject(pObject);
}
//-------------------------------------------------------------------------------------
void CSegmentedWorldDoc::InvalidateObjectWithChilds( CBaseObject* pObject)
{
	TObjectData* pObjData = m_extobjs.FindObjectData(pObject);
	if (pObjData)
		pObjData->NoLongerApplied();

	int num = pObject->GetChildCount();
	for (int i = num - 1; i >= 0; i--)
	{
		CBaseObject *obj = pObject->GetChild(i);
		InvalidateObjectWithChilds(obj);
	}
}
//-------------------------------------------------------------------------------------
bool CSegmentedWorldDoc::CanAccessChilds( CBaseObject* pObject)
{
	assert(pObject);
	if (pObject->IsKindOf(RUNTIME_CLASS(CGroup)))
		return false;
	return true;
}
//-------------------------------------------------------------------------------------
void CSegmentedWorldDoc::GetBoundBoxWithChilds( CBaseObject* pObject, AABB &box )
{
	assert(pObject);

	pObject->GetBoundBox(box);

	if (!CanAccessChilds(pObject))
		return;

	int num = pObject->GetChildCount();
	for (int i = 0; i < num; i++)
	{
		CBaseObject *obj = pObject->GetChild(i);
		AABB box2;
		GetBoundBoxWithChilds(obj, box2);
		box.Add(box2);
		box.Add(obj->GetWorldPos());
	}
}
//-------------------------------------------------------------------------------------
void CSegmentedWorldDoc::PreLoadObjects( TWorldCoordsArray &arrSegmentsToLoad, CBaseObjectsArray &arrObjects, bool bUpdateStatus)
{
	bool bWasActive = GetIEditor()->Get3DEngine()->IsSegmentedWorldActive();
	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);

	GetObjectsFiltered(arrObjects);
	if (bUpdateStatus)
	{
		m_extobjs.ResetNeedInWorld();
		m_extobjs.ObjectData_UpdateStatus();
	}

	bool bReady = false;
	uint32 w = m_uiWidth;
	uint32 h = m_uiHeight;
	TLocalCoords lc;

	while (!bReady)
	{
		AUTO_LOCK(g_cs);
		bReady = true;

		//ObjectData
		{
			bool bAnyCleaned;
			m_extobjs.CleanAllUnappliedData(&bAnyCleaned);
			bReady = !bAnyCleaned;  // if anything cleaned then it's not ready in this loop
		}

		//Layers
		{
			bool bAnyCleaned;
			CleanAllUnappliedLayerData(&bAnyCleaned);
			bReady = !bAnyCleaned;  // if anything cleaned then it's not ready in this loop
		}
	} //~while

	for (lc.ly = 0; lc.ly < h; ++lc.ly)
	{
		for (lc.lx = 0; lc.lx < w; ++lc.lx) 
		{
			TSegmentData *pd = m_segs.GetSegmentData(lc, false);
			if (pd->NeedsToBeApplied() || pd->NeedsToBeAppliedLayers())
			{
				stl::push_back_unique(arrSegmentsToLoad, pd->m_wc);
			}
		}
	}

	//Delete needed objects
	for (int i = 0; i < arrSegmentsToLoad.size(); i++)
	{
		TSegmentData *pd = m_segs.GetSegmentData(arrSegmentsToLoad[i], true);
		if (!pd)
			continue;

		if (bUpdateStatus)
			pd->UpdateLayersStatus();
		if(pd->NeedsToBeAppliedLayers())
		{
			TLocalCoords lc(arrSegmentsToLoad[i]);

			AABB bb = CalcWorldBounds(lc);
			CPoint ptPos(lc.lx * SEGMENT_SIZE_UNITS, lc.ly * SEGMENT_SIZE_UNITS);

			AUTO_LOCK(g_cs);
			for(TLayerMap::iterator it = pd->m_layersMap.begin(); it != pd->m_layersMap.end(); ++it)
			{
				TLayerData *pl = it->second;
				if(pl->NeedsToBeApplied())
				{
					for (DWORD i = 0; i < arrObjects.size(); ++i) {
						CBaseObject *po = arrObjects[i];
						if (!po)
							continue;
						if (m_extobjs.IsExternal(po))
							continue;
						if (po->GetParent())
							continue;
						if (po->GetGroup())
							continue;
						Vec3 pt = po->GetPos();
						if (!ObjInBox(bb, pt))
							continue;
						if (po->GetLayer()->GetGUID() != pl->m_LayerGUID)
							continue;

						DeleteObjectWithChilds(po);
						arrObjects[i] = 0;
					}
				}
			}
		}
	}

	AUTO_LOCK(g_cs);
	std::vector<TObjectData*> arrNeedToBeApplied;
	m_extobjs.FetchAll_NeedToBeApplied(arrNeedToBeApplied);
	for (std::vector<TObjectData*>::iterator it = arrNeedToBeApplied.begin(); it != arrNeedToBeApplied.end(); ++it)
	{
		TObjectData *pObjData = *it;
		//bool bWasFindObjectEnabled = IsSWFindObjectEnabled();
		//EnableSWFindObject(false);
		//CBaseObject *po = GetIEditor()->GetObjectManager()->FindObject(pObjData->GetGUID());
		//EnableSWFindObject(bWasFindObjectEnabled);
		CBaseObject* po = pObjData->GetBaseObject();
		if (po)
		{
			//delete if is external in DB
			//uint32 flagToCheck = 1<<((int)psw->GetVersionType());
			//if (CheckFlags(flagToCheck))
			{
				DeleteObjectWithChilds(po);
				//GetIEditor()->GetObjectManager()->DeleteObject(po);	
			}
		}
	}

	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(bWasActive);
}
//-------------------------------------------------------------------------------------

bool CSegmentedWorldDoc::CleanUp(bool bAll, int iMaxMem)
{ //!!! not implemented yet
	AUTO_LOCK_BIG(g_csBigLock);
	if (bAll) {
		m_segs.UnloadSegmentData();
		m_extobjs.UnloadAll();
		m_layerInfoMgr.ObjectLayersInfo_Unload();
		return true;
	}

	m_segs.CleanCache();

	return false;
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
CSegmentedWorldDoc::CSegmentedWorldDoc()
: ISegmentedWorldDoc(false)
,m_wcOfs(0, 0)
#pragma warning(push)
#pragma warning(disable: 4355)
, m_segs(this, this, this)
, m_extobjs(this,this,this)
#pragma warning(pop)
, m_bForceSaving(false)
, m_pVersionControl(NULL)
, m_pPersistance(NULL)
, m_bAutoUpdateLevelPak(false)
{
	AUTO_LOCK_BIG(g_csBigLock);

	m_bSuspendThread = true;
	GetIEditor()->Get3DEngine()->SetSegmentsManager(this);
//#ifdef _DEBUG
//  m_uiWidth = 4;
//  m_uiHeight = 4;
//#else 
	m_uiWidth = 8;
	m_uiHeight = 8;
//#endif
	m_bEditMode = true;
	m_bAutoMoveTo = false;
	m_iDelayedLoading = 0;
	m_fNextContentLoadTime = -1;
	m_matLastView.SetIdentity();


	m_vt = VT_CURRENT; //!!! must be VT_OFFICIAL;

	m_chName[0] = 0;
	m_pTerrainLayerPanelGroupItem = NULL;
	m_bNeedApplyObjectData = false;

	//Hot version
	m_lastHotSaveTime = clock();
	m_bDeleteHotHistoryRecord = true;
	m_bModified = false;

	//ScreenShot stuff
	m_mmgen.Reset();

	// init worlddata Types
	// this should be in WorldData constructor
	// , but C++ not support constructor's arguments in array init yet.
	for (int iWBT = 0; iWBT < WDB_COUNT; iWBT++)
	{
		m_WorldData[iWBT].m_eType = iWBT;
		m_WorldData[iWBT].m_subtype = iWBT;
	}

}

CSegmentedWorldDoc::~CSegmentedWorldDoc()
{
	AUTO_LOCK_BIG(g_csBigLock);
	//g_db->DisconnectFromDB();
	CleanUp(true);
	GetIEditor()->Get3DEngine()->SetSegmentsManager(0);
	m_pTerrainLayerPanelGroupItem = NULL;


	//ScreenShot stuff
	m_mmgen.Reset();
}

void CSegmentedWorldDoc::OnDBUserIDChanged(int nDBUserID)
{
	CSWMiniMapUpdater::s_nDBUserID = nDBUserID;
}





bool CSegmentedWorldDoc::GetWorldBounds(Vec2i* pMin, Vec2i* pMax)
{
	return SegmentAggr().GetWorldBoundsFromSegmentXML(m_sPath, pMin, pMax);
}

bool CSegmentedWorldDoc::WorldData_UpdateStatus(int eWDType)
{
	if (eWDType != -1) {
		m_WorldData[eWDType].UpdateStatus();
		return true;
	}

	for (int iWBT = 0; iWBT < WDB_COUNT; iWBT++) {
		m_WorldData[iWBT].UpdateStatus();
	}

	return true;
}

bool CSegmentedWorldDoc::GetGDState(int nWDBType, int &nWDBState, int &nLockedBy)
{
	if ( CSWManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return false;
	if (GetIEditor()->IsInGameMode() || gEnv->IsEditorGameMode())
		return false;
	if (GetISystem()->IsQuitting())
		return false;
	if (IsBusy())
		return false;

	if (m_bSuspendThread)
		return false;
	AUTO_LOCK_BIG(g_csBigLock);
	if (CSWManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return false;
	if (GetIEditor()->IsInGameMode() || gEnv->IsEditorGameMode())
		return false;
	if (GetISystem()->IsQuitting())
		return false;


	if (m_WorldData[nWDBType].IsMissingInPersist())
	{
		nLockedBy = 0;
		nWDBState = sw::WDBState_Missing;
		return true;
	} 
	else 
	{
		nLockedBy = DataInspector::m_nLockedBy(m_WorldData[nWDBType]);

		//if (m_WorldData[nWDBType].m_nLoadedHRID != m_WorldData[nWDBType].m_nHRIDs[m_vt])
		if (!m_WorldData[nWDBType].LoadedVersionIsVT(m_vt))
		{
			//nLockedBy = m_WorldData[nWDBType].m_nLockedBy;
			if (m_WorldData[nWDBType].IsLockedByMe())
			{
				nWDBState = sw::WDBState_Conflicted;
				return true;
			}

			nWDBState = sw::WDBState_OutOfDate;
			return true;
		}

		if (!m_WorldData[nWDBType].IsLocked())
		{
			//nLockedBy = 0;
			nWDBState = sw::WDBState_Default;
			return true;
		}	
		else if (m_WorldData[nWDBType].IsLockedByMe())
		{
			//nLockedBy = m_WorldData[nWDBType].m_nLockedBy;
			nWDBState = sw::WDBState_LockedByMe;
			return true;
		}	
		else
		{
			//nLockedBy = m_WorldData[nWDBType].m_nLockedBy;
			nWDBState = sw::WDBState_LockedByOther;
			return true;
		}
		
	}

	CRY_ASSERT_MESSAGE(0,"missing condition");
	return true;
}

const char *CSegmentedWorldDoc::WorldData_GetStatusText(char *pBuf, int eWDType)
{
	AUTO_LOCK(g_cs);

	TWorldData& wd = m_WorldData[eWDType];
	if (eWDType != -1) 
	{
		pBuf += sprintf(pBuf, "%s: ", GetWorldDataBlockName((EWDBType)eWDType));
		if (wd.IsMissingInPersist()) 
		{
			pBuf += sprintf(pBuf, "missing");
		}
		else 
		{
			char szLockedBy[ISWVersionControl::MAX_USERNAME_CBSIZE];
			if (!wd.IsLocked())
				pBuf += sprintf(pBuf, "not locked");
			else if (wd.IsLockedByMe())
				pBuf += sprintf(pBuf, "locked by me, %s(%d)"
					,m_pVersionControl->GetLockedBy(wd,szLockedBy,ISWVersionControl::MAX_USERNAME_CBSIZE,"<N/A>")
					,DataInspector::m_nLockedBy(wd));
			else
				pBuf += sprintf(pBuf, "LOCKED by %s(%d)"
					,m_pVersionControl->GetLockedBy(wd,szLockedBy,ISWVersionControl::MAX_USERNAME_CBSIZE,"<N/A>")
					, DataInspector::m_nLockedBy(wd));

			if (!wd.LoadedVersionIsVT(m_vt))
				pBuf += sprintf(pBuf, ", OUT OF DATE");
		}
	}
	else
	{
		
		for (int iWBT = 0; iWBT < WDB_COUNT; iWBT++) 
		{
			pBuf += WorldData_GetStatusText(pBuf,iWBT) - pBuf;
			pBuf += sprintf(pBuf, ";\r\n");
		}
	}
	return pBuf;
}

bool CSegmentedWorldDoc::WorlData_ReloadIfNeeded()
{
	// reload if needed
	bool bRes = true;
	bool bNeedReload;
	{
		AUTO_LOCK(g_cs);
		bNeedReload = !m_WorldData[WDB_TERRAIN_LAYERS].LoadedVersionIsVT(m_vt);
	}
	if (bNeedReload)
	{
		CRY_ASSERT(!m_WorldData[WDB_TERRAIN_LAYERS].IsLocked());
		bRes = WorldData_ReloadTerrainLayers();
	}

	{
		AUTO_LOCK(g_cs);
		bNeedReload = !m_WorldData[WDB_VEGETATION].LoadedVersionIsVT(m_vt);
	}
	if (bNeedReload)
	{
		CRY_ASSERT(!m_WorldData[WDB_VEGETATION].IsLockedByMe());
		bRes = WorldData_ReloadVegetationMap() && bRes;
	}
	return bRes;
}

bool CSegmentedWorldDoc::Init( bool bLoading, const char *pcPath, bool bForce, _smart_ptr<sw::ISWDataPersistence> pPersist )
{
	AUTO_LOCK_BIG(g_csBigLock);
	//AUTO_LOCK(g_cs);
	m_bSuspendThread = true;
	m_pPersistance = pPersist;
	m_pVersionControl = pPersist->GetVersionControl();

	m_sPath = NormalizeLevelPath(pcPath);


	m_mmgen.Init(this,this, m_pVersionControl);


	CString sXmlFile = m_sPath + "sw.xml";

	if (bLoading)
	{
		CleanUp(true);
		m_wcOfs.wx = 0;
		m_wcOfs.wy = 0;

		CCryFile file;
		if(!file.Open(sXmlFile, "rb"))
			return false;
		DWORD dwLen = file.GetLength();
		CMemoryBlock mem;
		if (!mem.Allocate(dwLen + 1))
			return false;
		char *pBuf = (char *) mem.GetBuffer();
		if (!file.ReadRaw(pBuf, dwLen))
			return false;
		pBuf[dwLen] = 0;
		XmlNodeRef root = XmlHelpers::LoadXmlFromBuffer(pBuf, strlen(pBuf));
		if (!root)
			return false;
		SerializeViewSettings(root, true);
	}
	else
	{
		if (bForce)
		{
			CHeightmap *phm = GetIEditor()->GetHeightmap();
			if (!phm)
				return false;
			m_uiWidth = phm->GetWidth() / SEGMENT_SIZE_UNITS;
			m_uiHeight = phm->GetHeight() / SEGMENT_SIZE_UNITS;
			if (m_uiWidth < 1)
				m_uiWidth = 1;
			if (m_uiHeight < 1)
				m_uiHeight = 1;
		} else if (!CFileUtil::FileExists(sXmlFile))
			return false;
		
		XmlNodeRef root = XmlHelpers::CreateXmlNode("SegmentedWorld");
		if (!root)
			return false;
		SerializeViewSettings(root, false);
		if (!root->saveToFile(sXmlFile))
			return false;
	}

	string sFN = PathUtil::GetFile(Path::RemoveBackslash(m_sPath));
	sFN.MakeLower();
	strcpy_s(m_chName, sizeof(m_chName), sFN);

	//if (!CFileUtil::PathExists(m_sPath + "segs"))
	//	if (int nDBUserID = g_db->ConnectToDB(m_chName))
	//		OnDBUserIDChanged(nDBUserID);

	//if (!bLoading && !g_db->GetCachedConn())
	//	CFileUtil::CreateDirectory(m_sPath + "segs");

  //WorldData_Init();
	WorldData_UpdateStatus();

	// check if parameters retrieved in previous steps are all good
	if (!CheckOpenParameters())
		return false;
	
	// Start SW updater thread
	TSegmentedWorldUpdater::Get();

	return true;
}

void CSegmentedWorldDoc::GetObjectsFiltered(CBaseObjectsArray &arrObjects)
{
	GetIEditor()->GetObjectManager()->GetObjects(arrObjects);
	for (int i = 0; i < arrObjects.size(); ++i)
	{
		CBaseObject *po = arrObjects[i];
		if (!po)
			continue;
		if (po->GetParent())
		{
			arrObjects[i] = 0;
			continue;
		}
		if (po->GetGroup())
		{
			arrObjects[i] = 0;
			continue;
		}
	}
}

void CSegmentedWorldDoc::GetSizeInUnits(uint64 &w, uint64 &h)
{
	w = m_uiWidth * SEGMENT_SIZE_UNITS;
	h = m_uiHeight * SEGMENT_SIZE_UNITS;
}

void CSegmentedWorldDoc::GetSizeInSegments(UINT &w, UINT &h)
{
	w = m_uiWidth;
	h = m_uiHeight;
}

void CSegmentedWorldDoc::GetOffsetInSegments(int &x, int &y)
{
	x = m_wcOfs.wx;
	y = m_wcOfs.wy;
}

// TODO: remove view code out side of doc code
void CSegmentedWorldDoc::SerializeViewSettings(IXmlNode *sw, bool bLoading)
{
	if (bLoading) {
		sw->getAttr("x", m_wcOfs.wx);
		sw->getAttr("y", m_wcOfs.wy);
		sw->getAttr("w", m_uiWidth);
		sw->getAttr("h", m_uiHeight);

		XmlNodeRef wm = sw->findChild("WorldMap");
		if (wm) {
			wm->getAttr("ofs", m_viewStates.m_ptWorldMapOfs);
			wm->getAttr("zoom", m_viewStates.m_nWorldMapZoom);
		}

		XmlNodeRef cam = sw->findChild("Camera");
		if (cam) {
			cam->getAttr("pos", m_viewStates.m_ptCamPos);
			cam->getAttr("ang", m_viewStates.m_vCamAngles);
		}
	} else {
		sw->setAttr("x", m_wcOfs.wx);
		sw->setAttr("y", m_wcOfs.wy);
		sw->setAttr("w", m_uiWidth);
		sw->setAttr("h", m_uiHeight);

		XmlNodeRef wm = sw->newChild("WorldMap");
		wm->setAttr("ofs", m_viewStates.m_ptWorldMapOfs);
		wm->setAttr("zoom", m_viewStates.m_nWorldMapZoom);

		CViewport *pVP = GetIEditor()->GetViewManager()->GetGameViewport();
		if (pVP) {
			m_viewStates.m_ptCamPos = pVP->GetViewTM().GetTranslation();
			m_viewStates.m_vCamAngles = Ang3::GetAnglesXYZ(Matrix33(pVP->GetViewTM()));
			XmlNodeRef cam = sw->newChild("Camera");
			cam->setAttr("pos", m_viewStates.m_ptCamPos);
			cam->setAttr("ang", m_viewStates.m_vCamAngles);
		}
	}
}


bool CSegmentedWorldDoc::WorldData_IsModified()
{
	AUTO_LOCK(g_cs);
	for (int i = 0; i < WDB_COUNT; i++)
	{
		if (m_WorldData[i].IsModified())
			return true;
	}
	return false;
}

bool CSegmentedWorldDoc::WorldData_Save( ESaveFlag sflag, sw::EVersionType vt, int64 nVersion /*= -1*/, EWDBType nWDBType /*= (int)sw::WDB_COUNT*/ )
{
	// make a set of flags for what type we want to process
	assert(WDB_COUNT<32);
	size_t nWDBFlags = 1<<nWDBType;
	if (nWDBType == WDB_COUNT)
		nWDBFlags -= 1; //< little trick

	return WorldData_Save(sflag,vt,nVersion,nWDBFlags);
}

bool CSegmentedWorldDoc::WorldData_Save( ESaveFlag sflag, sw::EVersionType vt, int64 nVersion /*= -1*/, uint32 nWDBFlags /*= (1<<WDB_COUNT)-1*/ )
{
	assert( (sflag&SF_WithSubmit && nVersion>0) || (!(sflag&SF_WithSubmit) && nVersion<=0) );
	AUTO_LOCK(g_cs);
	//assert(!(s_nSWMode == 0 && sflag&SF_WithSubmit));
	
	uint32 flagNeedSave = nWDBFlags;
	// process levelpak only when it's either committing or map-converting
	//if (!(/*vt==VT_OFFICIAL ||*/ CSWManager::IsAutomate()))
	//	flagNeedSave &= ~(1<<WDB_LEVELPAK);

	ELockResult arrSaves[WDB_COUNT] = {};
	if (!WorldData_PrepareData(flagNeedSave, sflag, vt, arrSaves)) // if nothing changed
		return true;

	bool bRes = true;
		for (int i = 0; i < WDB_COUNT; i++)
		{
			//if (nWDBType != WDB_COUNT && i != nWDBType)
			//	continue;
			if (!(flagNeedSave&(1<<i)))
				continue;

			if (arrSaves[i] > LockResult_Failed)
			{
			if (!m_WorldData[i].DoSave(sflag,vt,nVersion))
					{
						if (arrSaves[i] == LockResult_Succeeded)
							m_WorldData[i].Unlock();
					bRes = false;
				}
			}
			assert(!(sflag&SF_AsIntegrate));
			//else if (sflag&SF_AsIntegrate)
			//{
			//m_WorldData[i].DoIntegrateOnly(vt,nVersion);
			//}
	}
	//}
	return bRes;
}

//-------------------------------------------------------------------------------------------------------------------------
bool CSegmentedWorldDoc::TryLockModified(std::vector<TWorldData*> &wdJustLocked, std::vector<TSegmentData*> &sdJustLocked, std::vector<TLayerData*> &ldJustLocked)
{
	m_bSuspendThread = true;
	AUTO_LOCK(g_cs);
	m_bSuspendThread = false;
	CSWModeFakeAutoGuard swFakeMode(1);
	
	//world data
	WorldData_UpdateStatus();
	for (int i = 0; i < WDB_COUNT; i++)
	{
		if (m_WorldData[i].IsModified())
		{
			//bool bVersionConflict = m_WorldData[i].m_nHRIDBeforeModify != m_WorldData[i].m_nHRIDs[m_vt];
			bool bVersionConflict = m_WorldData[i].IsVersionConflicting(m_vt);
			if (bVersionConflict)
				return false;
			
			ELockResult nLockRes = (ELockResult)m_WorldData[i].Lock();
			if (nLockRes == LockResult_Failed)
				return false;
			if (nLockRes == LockResult_Succeeded)
				wdJustLocked.push_back(&m_WorldData[i]);
		}
	}
	WorldData_UpdateStatus();

	CBaseObjectsArray arrObjects;
	bool bObjectsFilled = false;
	CObjectArchive oa(GetIEditor()->GetObjectManager(), 0, true );
	GetIEditor()->SetEditTool(0); // Turn off any active edit tools.

	//ObjectData
	m_segs.UpdateLayersStatus();

	std::vector<TObjectData*> arrExternal;
	m_extobjs.FetchAll_External(arrExternal);
	for (std::vector<TObjectData*>::iterator it = arrExternal.begin(); it != arrExternal.end(); ++it)
	{
		TObjectData* pObjData = *it;
		assert(pObjData);

		TLayerData* pl = m_segs.GetSegmentLayer(pObjData->GetSegCoords(), pObjData->GetLayerGUID(), pObjData->GetLayerName());
		if (!pl)
			continue;

		if (pObjData->IsModified())
		{
			bool bVersionConflict = pObjData->IsVersionConflicting(m_vt); //pObjData->m_nHRIDBeforeModify != pObjData->m_nHRIDs[m_vt];
			if (bVersionConflict)
				return false;

			ELockResult nLockRes = pl->Lock();
			if (nLockRes == 0)
				return false;
			if (nLockRes == 2)
				ldJustLocked.push_back(pl);
		}
	}

	//SegmentData
	std::vector<TSegmentData*> arrSegmentData;
	m_segs.FetchAll(arrSegmentData);
	TWorldCoordsArray arrSegmentsToLoad;
	for (std::vector<TSegmentData*>::iterator it = arrSegmentData.begin(); it != arrSegmentData.end(); ++it) 
	{
		TSegmentData *pd = *it;
		bool bModified = pd->IsModified() || pd->IsLayersModified();
		if (pd->IsModified())
		{
			bool bVersionConflict = false;
			for (int i = 0; i < SDB_COUNT; i++)
			{
				if (IsBlockTypeNeeded((ESDBType)i) && pd->m_Data[i]->IsVersionConflicting(m_vt))
					bVersionConflict = true;
			}

			if (bVersionConflict)
				return false;

			ELockResult nLockRes = pd->Lock();
			if (nLockRes == 0)
				return false;
			if (nLockRes == 2)
				sdJustLocked.push_back(pd);
		}

		if (pd->IsLayersModified())
		{
			for (TLayerMap::iterator it = pd->m_layersMap.begin(); it != pd->m_layersMap.end(); ++it)
			{
				TLayerData* pl = it->second;
				if (pl->IsModified())
				{
					bool bVersionConflict = pl->IsVersionConflicting(m_vt);
					if (bVersionConflict)
						return false;

					ELockResult nLockRes = pl->Lock();
					if (nLockRes == LockResult_Failed)
						return false;
					if (nLockRes == LockResult_Succeeded)
						ldJustLocked.push_back(pl);
				}
			}
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------------------------------------------
bool CSegmentedWorldDoc::TryUnLockJustLocked(std::vector<TWorldData*> &wdJustLocked, std::vector<TSegmentData*> &sdJustLocked, std::vector<TLayerData*> &ldJustLocked)
{
	m_bSuspendThread = true;
	AUTO_LOCK(g_cs);
	m_bSuspendThread = false;
	CSWModeFakeAutoGuard swFakeMode(1);
	bool bRes = true;
	for (int i = 0; i < wdJustLocked.size(); i++)
	{
		if (!wdJustLocked[i]->Unlock())
			bRes = false;
	}

	for (int i = 0; i < sdJustLocked.size(); i++)
	{
		if (!sdJustLocked[i]->Unlock())
			bRes = false;
	}

	for (int i = 0; i < ldJustLocked.size(); i++)
	{
		if (!ldJustLocked[i]->Unlock())
			bRes = false;
	}

	return bRes;
}

#ifndef SW_NO_BUILTIN_BRANCH_MANAGING
bool CSegmentedWorldDoc::CommitToOfficial( const char* pcDescription)
{
	EVersionType vt = VT_OFFICIAL;
	ESaveFlag sflag = SF_IntegrateSave;
	assert(m_vt != vt);

	AUTO_LOCK_BIG(g_csBigLock);
	if (CSWManager::Get().IsOfflineMode() && !s_bMapConverting)
	{
		AfxMessageBox( "Committing denied! You are working in Offline (local) mode!!!" ,MB_OK|MB_ICONSTOP );
		return false;
	}

	// always save before integrate
	if (!Save(true))
		return false;

	IObjectManager *pom = GetIEditor()->GetObjectManager();
	if (!pom)
		return false;

	//AUTO_LOCK(g_cs);
	m_bSuspendThread = false;

	int64 nVersion = GetVersionControl()->CreateChangeList(vt,pcDescription);
	assert(nVersion != 0);
	bool bRes = true;

	CWaitProgress progress( _T("Committing to Official Version ...") );
	progress.Start();
	int nNumSteps = m_uiHeight*m_uiWidth+1;
	int nStep = 0;

	progress.Step( 100*nStep/nNumSteps );
	nStep++;


	// == WorldData Integration ==
	{
		ELockResult arrSaves[WDB_COUNT] = {};
		if (WorldData_PrepareData( (1<<WDB_COUNT)-1, sflag, vt, arrSaves)) // if something changed
		{
			for (int i = 0; i < WDB_COUNT; i++)
			{
				//if (nWDBType != WDB_COUNT && i != nWDBType)
				//	continue;
				//if (!(flagNeedSave&(1<<i)))
				//	continue;

				if (arrSaves[i] > LockResult_Failed)
				{
					if (!m_WorldData[i].DoSave(sflag,vt,nVersion))
					{
						if (arrSaves[i] == LockResult_Succeeded)
							m_WorldData[i].Unlock();
					}
				}
				else/* if (sflag&SF_AsIntegrate)*/
				{
					m_WorldData[i].DoIntegrateOnly(vt,nVersion);
				}
			}
		}
	}


	//if (nVersion != 0)
	{
		std::vector<CObjectLayer*> vLayers;
		GetIEditor()->GetObjectManager()->GetLayersManager()->GetLayers(vLayers);
		for (int i = 0; i < vLayers.size(); i++)
		{
			//m_layerInfoMgr.RegisterNewLayer(vLayers[i]->GetGUID(),vLayers[i]->GetName());
			assert(0 < m_layerInfoMgr.ObjectLayersInfo_GetID(vLayers[i]->GetGUID()));
		}
	}
	//bRes &= m_layerInfoMgr.SaveLayerXML(m_sPath);
	assert(CFileUtil::FileExists(m_sPath + CString("layerinfo.xml")));

	CBaseObjectsArray arrObjects;
	GetObjectsFiltered(arrObjects);
	//pom->GetObjects(arrObjects);

	// save loaded segments (to cache and then to storage)

	TLocalCoords lc;
	for (lc.ly = 0; lc.ly < m_uiHeight; ++lc.ly)
	{
		for (lc.lx = 0; lc.lx < m_uiWidth; ++lc.lx)
		{
			progress.Step( 100*nStep/nNumSteps );
			nStep++;

			if (!IsSegmentSelected(lc))
				continue;
			if (!m_segs.IntegrateSegment(lc, arrObjects, nVersion, vt))
				bRes = false;
			//ObjectData_Save(lc, arrObjects, false, nVersion, vt);
			if (!m_extobjs.ObjectData_Integrate( lc, arrObjects, nVersion, vt))
				bRes = false;
		}
	}

	// save cached segments and objects to storage
	//if (vt != VT_COUNT) {
		//save external objects
		bRes &= m_extobjs.IntegrateExternalObjects(vt,nVersion);
		//bRes &= m_objs.SaveObjectXML(m_sPath);
		assert(CFileUtil::FileExists(m_sPath + CString("objectinfo.xml")));

		//save segments
		bRes &= m_segs.IntegrateToDB(vt,nVersion);
		//bRes &= m_segs.SaveSegmentXML(m_sPath);
		assert(CFileUtil::FileExists(m_sPath+CString("segmentinfo.xml")));

		//for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
		//{
		//	TObjectData* pObjData = it->second;
		//	if (bModifiedOnly && !pObjData->IsModified() && pObjData->IsSaved(vt))
		//		continue;
		//	if (!pObjData->Save())
		//}
	//}

	progress.Stop();
	UpdateLevelPak();

	//ObjectData_Save(arrObjects, false, nVersion, vt);
	//ObjectData_Save(arrObjects, bModifiedOnly, nVersion, vt);

	if (!IsInSelectMode())
		GetVersionControl()->Commit(nVersion);

	return bRes;
}
#endif

bool CSegmentedWorldDoc::Save(ESaveFlag sflag, EVersionType vt, int64 &nVer, const char* pcDescription)
{

	if (vt == VT_COUNT)
		assert( (sflag&SF_NoSubmitSave) == SF_NoSubmitSave );
	else if (m_vt != vt)
		assert( false && (sflag&SF_IntegrateSave) == SF_IntegrateSave );
	else
		assert( (sflag&SF_RegularSave) == SF_RegularSave );

	assert(!m_bForceSaving); // make sure there are no overlapping state setting.
	_AutoResetGuard<bool> __autoResetGuard(&m_bForceSaving, (sflag&SF_ForceSaveAll)?true:false);

	AUTO_LOCK_BIG(g_csBigLock);
	//if (CSWManager::Get().IsOfflineMode() && !s_bMapConverting)
	//{
	//	AfxMessageBox( "Saving denied! You are working in Offline (local) mode!!!" ,MB_OK|MB_ICONSTOP );
	//	return false;
	//}
	IObjectManager *pom = GetIEditor()->GetObjectManager();
	if (!pom)
		return false;

	//AUTO_LOCK(g_cs);
	m_bSuspendThread = false;

	int64 nVersion = (vt == VT_COUNT) ? 0 : GetVersionControl()->CreateChangeList(vt,pcDescription)/*(pcDescription != NULL ? GetNewVersionIDWithDescr(vt, pcDescription):GetNewVersionID(vt))*/;
	nVer = nVersion;
	bool bRes = true;

	CWaitProgress progress( _T("Saving Segments ...") );
	progress.Start();
	int nNumSteps = m_uiHeight*m_uiWidth+1;
	int nStep = 0;

	progress.Step( 100*nStep/nNumSteps );
	nStep++;

	if ((vt != VT_COUNT) && !WorldData_Save(sflag, vt, nVersion) && sflag&SF_ForceSaveAll)
		bRes = false;

	if (nVersion != 0)
	{
		std::vector<CObjectLayer*> vLayers;
		GetIEditor()->GetObjectManager()->GetLayersManager()->GetLayers(vLayers);
		m_layerInfoMgr.SyncLayers(vLayers);
	}
	bRes &= m_layerInfoMgr.SaveLayerXML(m_sPath);

	CBaseObjectsArray arrObjects;
	GetObjectsFiltered(arrObjects);
	//pom->GetObjects(arrObjects);

	// save loaded segments (to cache and then to storage)
	
	TLocalCoords lc;
	for (lc.ly = 0; lc.ly < m_uiHeight; ++lc.ly)
	{
		for (lc.lx = 0; lc.lx < m_uiWidth; ++lc.lx)
		{
			progress.Step( 100*nStep/nNumSteps );
			nStep++;

			if (!IsSegmentSelected(lc))
				continue;

			// seglayers may be set as modified, during process for external objects
			if (!m_extobjs.SaveForSegment(sflag, lc, arrObjects, nVersion, vt))
				bRes = false;
			if (!m_segs.SaveSegment(sflag, lc, arrObjects, nVersion, vt))
				bRes = false;
		}
	}
	
	// save cached segments and objects to storage
	if (vt != VT_COUNT) {
		//save external objects
		bRes &= m_extobjs.SaveAll(sflag, vt,nVersion);
		bRes &= m_extobjs.SaveObjectXML(m_sPath);

		//save segments
		bRes &= m_segs.SaveToDB(sflag, vt,nVersion);
		bRes &= m_segs.SaveSegmentXML(m_sPath);

		//for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
		//{
		//	TObjectData* pObjData = it->second;
		//	if (bModifiedOnly && !pObjData->IsModified() && pObjData->IsSaved(vt))
		//		continue;
		//	if (!pObjData->Save())
		//}
	}

	progress.Stop();
	OnMayUpdateLevelPak();

	//ObjectData_Save(arrObjects, false, nVersion, vt);
	//ObjectData_Save(arrObjects, bModifiedOnly, nVersion, vt);

	return bRes;
}

bool CSegmentedWorldDoc::Save(bool bModifiedOnly)
{
	int64 nVer = 0;
	assert(/*m_vt != VT_OFFICIAL &&*/ m_vt != VT_COUNT);
	ESaveFlag sflag = SF_RegularSave;
	sflag |= bModifiedOnly? 0 : SF_ForceSaveAll;
	bool bRes = Save( sflag, m_vt, nVer, NULL);
	OnSWModified(!bRes);
	return bRes;
}

bool CSegmentedWorldDoc::Load(bool bFull)
{
	m_bSuspendThread = true;
	AUTO_LOCK_BIG(g_csBigLock);
	IObjectManager *pom = GetIEditor()->GetObjectManager();
	if (!pom)
		return false;

	//AUTO_LOCK(g_cs);
	m_bSuspendThread = false;

	//GetIEditor()->GetErrorReport()->SetImmidiateMode(false);
	GetIEditor()->GetDocument()->SetDocumentReady(false);
	GetIEditor()->SuspendUndo();

	{
		CRGBLayer &TerrainRGBLayer = CTerrainManager::GetTerrainManager().GetHeightmap().m_TerrainRGBTexture;
		TerrainRGBLayer.FreeData(); // looks like AllocateTiles() will cause memory leaks if this is not called
		int w = m_uiWidth;
		int h = m_uiHeight;
		Seg2RGB(w, h);
		TerrainRGBLayer.AllocateTiles(w, h, SEGMENT_SIZE_UNITS); // TODO: !!! tile resolution hardcoded
	}


	CXmlArchive xmlArNamedSelection;

	if (bFull)
	{
		xmlArNamedSelection.bLoading = false;
		xmlArNamedSelection.root = XmlHelpers::CreateXmlNode("NamedSelections");
		GetIEditor()->GetObjectManager()->SerializeNameSelection(xmlArNamedSelection.root, xmlArNamedSelection.bLoading);
	}

	CBaseObjectsArray arrObjects;
	pom->DeleteAllObjects();
	CObjectArchive oa(GetIEditor()->GetObjectManager(), 0, true );

	CLogFile::WriteLine("Loading Segments...");
	CWaitProgress progress( _T("Loading Segments"), true);
	int nNumSteps = m_uiHeight*m_uiWidth+2;
	DWORD dwFlags = WS_LOAD_ALL | WS_UPDATE_STATUS;
	bool bRes = true;
	TLocalCoords lc;
	int i = 0;
	for (lc.ly = 0; lc.ly < m_uiHeight; ++lc.ly)
	{
		for (lc.lx = 0; lc.lx < m_uiWidth; ++lc.lx)
		{
			progress.Step( 100*i/nNumSteps );
			i++;
			if (!m_segs.LoadSegment(lc, arrObjects, oa, dwFlags))
				bRes = false;
		}
	}
	
	progress.Stop();
	OnMayUpdateLevelPak();
	
	progress.SetText( _T("Update Terrain..."));
	progress.Start();
	progress.Step( 100*i/nNumSteps );
	i++;
	UpdateEngine();

	progress.SetText(_T("Loading Objects..."));
	progress.Start();
	progress.Step( 100*i/nNumSteps );
	i++;

	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);
	oa.ResolveObjects();
	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(false);
	
	progress.Stop();
	// Loading ObjectData
	m_extobjs.ObjectData_Load(true);

	progress.SetText( _T("Finalize..."));
	progress.Start();
	progress.Step( 100*i/nNumSteps );

	if (bFull)
	{
		CViewport *pvp = GetIEditor()->GetViewManager()->GetGameViewport();
		if (pvp) 
		{
			if (m_viewStates.m_ptCamPos.x > 1e30f) 
			{
				CPoint ptHM(m_uiWidth * SEGMENT_SIZE_UNITS / 2, m_uiHeight * SEGMENT_SIZE_UNITS / 2);
				m_viewStates.m_ptCamPos = GetIEditor()->GetHeightmap()->HmapToWorld(ptHM);
				m_viewStates.m_ptCamPos.z = GetIEditor()->Get3DEngine()->GetTerrainElevation(m_viewStates.m_ptCamPos.x, m_viewStates.m_ptCamPos.y, true) + 5.0f;
			}
			Matrix34 tm = Matrix34::CreateRotationXYZ(m_viewStates.m_vCamAngles);
			tm.SetTranslation(m_viewStates.m_ptCamPos);
			pvp->SetViewTM(tm);
		}
	}

	if (bFull)
	{
		xmlArNamedSelection.bLoading = true;
		GetIEditor()->GetObjectManager()->SerializeNameSelection(xmlArNamedSelection.root, xmlArNamedSelection.bLoading);
	}

	GetIEditor()->ResumeUndo();
	GetIEditor()->GetDocument()->SetDocumentReady(true);
	//GetIEditor()->GetErrorReport()->Display();
	CleanUp(false);
	return bRes;
}

bool CSegmentedWorldDoc::ExportToGame(const char *pszGamePath, EEndian eExportEndian, CPakFile &pakFile, bool bForce)
{
	bool bRes = true;
	std::vector<TSegmentData*> arrSegmentData;
	m_segs.FetchAll(arrSegmentData);
	sw::ESaveFlag sflag = bForce ? SF_ForceSaveAll : SF_RegularSave;
	TRectI worldBounds(m_wcOfs.wx, m_wcOfs.wy, m_wcOfs.wx + m_uiWidth - 1, m_wcOfs.wy + m_uiHeight - 1);
	for ( std::vector<TSegmentData*>::iterator iter = arrSegmentData.begin(); iter != arrSegmentData.end(); ++iter)
	{
		TSegmentData* pSeg = *iter;

		if(!worldBounds.InRect(pSeg->m_wc))
			continue;

		pSeg->SaveDataForEngine(sflag,pSeg->m_wc,-1,GetVersionType());

		for ( ESDBTypeIterator eBlockType = SDB_ENGINEDATA_BEGIN; eBlockType != SDB_ENGINEDATA_END; ++eBlockType)
		{
			if (eBlockType == SDB_EngRGBData) continue; // skip surface texture
			bRes = bRes && GenerateLevelPakFromFileBlocks(pSeg->m_wc, eBlockType, pakFile, bForce);
		}
	}

	return bRes;
}
//------------------------------------------------------------------------------------
// terrain layers
//------------------------------------------------------------------------------------
bool CSegmentedWorldDoc::WorldData_ReloadTerrainLayers()
{
	AUTO_LOCK(g_cs);
	//int nType = (int)WDB_TERRAIN_LAYERS;
	TWorldData& worlddata = m_WorldData[WDB_TERRAIN_LAYERS];


	if (!worlddata.LoadByVT(worlddata.GetWorldDataNeededVersionType()))
		return false;

	worlddata.m_xmlAr->bLoading = true;

	CEditTool *pTool = GetIEditor()->GetEditTool();
	bool bNeedToResetTool = false;
	bool bIsMultiTool = false;
	if (pTool && pTool->IsKindOf(RUNTIME_CLASS(CTerrainTexturePainter)))
	{
		bNeedToResetTool = true;
		GetIEditor()->SetEditTool(0); // Turn off any active edit tools.
	}
	
	GetIEditor()->GetDocument()->SetDocumentReady(false);

	CTerrainManager::GetTerrainManager().SerializeSurfaceTypes( (*worlddata.m_xmlAr) );
	CTerrainManager::GetTerrainManager().SerializeLayerSettings( (*worlddata.m_xmlAr) );
	
	CTerrainManager::GetTerrainManager().ReloadSurfaceTypes();

	//reload segment layer IDs
	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);
	for (int y = 0; y < m_uiHeight; y++)
	{
		for (int x = 0; x < m_uiWidth; x++) 
		{
			TLocalCoords lc(x,y);
			m_segs.ReloadSegmentLayerID(lc);
		}
	}
	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(false);
	GetIEditor()->GetDocument()->SetDocumentReady(true);

	GetIEditor()->GetHeightmap()->UpdateEngineTerrain();
	
	if (bNeedToResetTool)
	{
		pTool = new CTerrainTexturePainter;
		GetIEditor()->SetEditTool( pTool );
		GetIEditor()->SelectRollUpBar( ROLLUP_TERRAIN );
		AfxGetMainWnd()->RedrawWindow(NULL,NULL,RDW_INVALIDATE|RDW_ALLCHILDREN);
	}

	//worlddata.SetAppliedHRID(worlddata.m_nLoadedHRID);
	worlddata.ApplyLoadedVersion();

	return true;
}
//------------------------------------------------------------------------------------
// Vegetation
//------------------------------------------------------------------------------------
bool CSegmentedWorldDoc::WorldData_ReloadVegetationMap()
{
	AUTO_LOCK(g_cs);
	//int nType = WDB_VEGETATION;
	TWorldData& worlddata = m_WorldData[WDB_VEGETATION];
	if (!worlddata.LoadByVT(worlddata.GetWorldDataNeededVersionType()))
		return false;

	//worlddata.SetAppliedHRID(worlddata.m_nLoadedHRID);
	worlddata.ApplyLoadedVersion();

	CEditTool *pTool = GetIEditor()->GetEditTool();
	bool bNeedToResetTool = false;
	bool bIsMultiTool = false;
	if (pTool && pTool->IsKindOf(RUNTIME_CLASS(CVegetationTool)))
	{
		bNeedToResetTool = true;
		GetIEditor()->SetEditTool(0); // Turn off any active edit tools.
	}
	
	GetIEditor()->GetDocument()->SetDocumentReady(false);

	CVegetationMap* pVegMap = GetIEditor()->GetHeightmap()->GetVegetationMap();

	if (!pVegMap)
		return false;

	for (int i = pVegMap->GetObjectCount() - 1; i >= 0; i--)
	{
		pVegMap->RemoveObject(pVegMap->GetObject(i));
	}

	CLogFile::WriteLine("Loading Vegetation Map...");

	CWaitProgress progress( _T("Reloading Static Objects") );

	XmlNodeRef mainNode = worlddata.m_xmlAr->root->findChild( "VegetationMap" );
	if (mainNode)	
	{
		XmlNodeRef objects = mainNode->findChild( "Objects" );
		if (objects)
		{
			int numObjects = objects->getChildCount();
			for (int i = 0; i < numObjects; i++)
			{	
				if (!progress.Step( 100*i/numObjects ))
					break;
				
				pVegMap->ImportObject(objects->getChild(i));
			}
		}
	} 
	else 
	{
		return false;
	}

	//reload vegetation instances
	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);
	for (int y = 0; y < m_uiHeight; y++) 
	{
		for (int x = 0; x < m_uiWidth; x++) 
		{
			TLocalCoords lc(x,y);
			m_segs.ReloadVegetationInstances(lc);
		}
	}
	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(false);
	GetIEditor()->GetDocument()->SetDocumentReady(true);
	CTerrainManager::GetTerrainManager().ReloadSurfaceTypes(true, false);

	if (bNeedToResetTool) 
	{
		pTool = new CVegetationTool;
		GetIEditor()->SetEditTool( pTool );
		GetIEditor()->SelectRollUpBar( ROLLUP_TERRAIN );
		AfxGetMainWnd()->RedrawWindow(NULL,NULL,RDW_INVALIDATE|RDW_ALLCHILDREN);
	}

	return true;
}

//------------------------------------------------------------------------------------
bool CSegmentedWorldDoc::WorldData_LoadToArchiveArray(TDocMultiArchive &arrXmlAr)
{
	WorldData_UpdateStatus();

	AUTO_LOCK(g_cs);
	FillXmlArArray(arrXmlAr, NULL);

	for (int i = 0; i < WDB_COUNT; i++)
	{
		TWorldData& worlddata = m_WorldData[i];
		EVersionType vt = worlddata.GetWorldDataNeededVersionType();//m_vt;

		if (!worlddata.LoadByVT(vt)) 
		{
			assert(0);
			continue;
		}

		// xml blocks have more trivial work to do
		if (i < WDB_XMLCOUNT)
		{
			m_WorldData[i].m_xmlAr->bLoading = true;
			arrXmlAr[i] = m_WorldData[i].m_xmlAr;

			// will be set to applied when data are actually reloaded by Editor
			//m_WorldData[i].SetAppliedHRID(m_WorldData[i].m_nLoadedHRID);
			//m_WorldData[i].ApplyLoadedVersion();
		}
	}

	return true;
}

bool CSegmentedWorldDoc::WorldData_Revert(EVersionType vt)
{
	//assert(s_nSWMode != 0);
	int64 nVer = 0;
	Save( SF_NoSubmitSave, VT_COUNT, nVer, NULL);
	GetIEditor()->SetEditTool(0); // Turn off any active edit tools.
	
	//for (int i = 0; i < WDB_COUNT; i++)
	//{
	//	//if (vt == VT_OFFICIAL)
	//	//	m_WorldData[i].Lock();

	//	bool bLocked;
	//	{
	//		AUTO_LOCK(g_cs);
	//		bLocked = (m_WorldData[i].IsLockedByMe());
	//	}
	//	
	//	m_WorldData[i].Revert(bLocked ? vt : m_WorldData[i].GetWorldDataNeededVersionType());
	//		
	//	if (vt == VT_OFFICIAL && !CSWManager::Get().IsOfflineMode())
	//		m_WorldData[i].Unlock();
	//	
	//}
	
	//pSW->DeleteSegments(GetIEditor()->Get3DEngine()->GetITerrain());
	TDocMultiArchive arrXmlAr;
	//CXmlArchive xmlAr;
	//xmlAr.bLoading = true;
	SetAsForceReloadLevel();
	bool bRes = WorldData_LoadToArchiveArray(arrXmlAr);
	
	//GetIEditor()->GetDocument()->Load(xmlAr, pSW->m_sPath + pSW->m_chName);

	ICryPak *pIPak = GetIEditor()->GetSystem()->GetIPak();
	pIPak->GetResourceList(ICryPak::RFOM_NextLevel)->Load( Path::Make(m_sPath + m_chName,"resourcelist.txt") );
	//pIPak->Notify( ICryPak::EVENT_BEGIN_LOADLEVEL );
	GetIEditor()->Notify( eNotify_OnBeginLoad );
	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_LEVEL_LOAD_START,0,0 );

	GetIEditor()->GetDocument()->DeleteContents();
	GetIEditor()->GetDocument()->SetModifiedFlag(TRUE);  // dirty during de-serialize
	GetIEditor()->GetDocument()->SetModifiedModules(eModifiedAll);

	DeleteSegments(GetIEditor()->Get3DEngine()->GetITerrain());

	GetIEditor()->GetDocument()->Load(arrXmlAr, m_sPath + m_chName);

	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_LEVEL_LOAD_END,0,0 );
	// We don't need next level resource list anymore.
	pIPak->GetResourceList(ICryPak::RFOM_NextLevel)->Clear();
	//pIPak->Notify( ICryPak::EVENT_END_LOADLEVEL );

	GetIEditor()->GetDocument()->SetModifiedFlag(FALSE); // start off with unmodified
	GetIEditor()->GetDocument()->SetModifiedModules(eModifiedNothing);

	Load(false);
	return bRes;
}
//-------------------------------------------------------------------------
bool CSegmentedWorldDoc::WorldDataCC(EWDBType nWDBType, EWDBConsoleComands nWDBCC)
{
	AUTO_LOCK_BIG(g_csBigLock);
	if (CSWManager::Get().IsOfflineMode()) return true;
	bool bRes = false;
	int64 nVersion = -1;
	switch (nWDBCC)
	{
	case sw::WDBCC_Lock:
		{
			bRes = (m_WorldData[nWDBType].Lock() != 0);
		}
		break;
	case sw::WDBCC_Save:
		{
			m_bSuspendThread = false;
			bRes = WorldData_Save(SF_RegularSave, VT_CURRENT, nVersion, nWDBType);
		}
		break;
	case sw::WDBCC_Commit:
		{
			//m_bSuspendThread = false;
			//bRes = WorldData_Save(SF_IntegrateSave, VT_OFFICIAL, nVersion, nWDBType);
		}
		break;
	case sw::WDBCC_Revert:
		{
			//if (nWDBType == WDB_TERRAIN_LAYERS || nWDBType == WDB_VEGETATION)
			//{
			//	m_bSuspendThread = false;
			//	if (nWDBType == WDB_TERRAIN_LAYERS)
			//	{
			//		bRes = WorldData_ReloadTerrainLayers();
			//	} 
			//	else if (nWDBType == WDB_VEGETATION)
			//	{
			//		bRes = WorldData_ReloadVegetationMap();
			//	}
			//}
			//else
			//{
			//	bRes = WorldData_Revert(VT_CURRENT);
			//}
		}
		break;
	case sw::WDBCC_Rollback:
		{

			//if (nWDBType == WDB_TERRAIN_LAYERS || nWDBType == WDB_VEGETATION)
			//{

			//	m_bSuspendThread = false;
			//	m_WorldData[nWDBType].Revert(VT_OFFICIAL);
			//	if (nWDBType == WDB_TERRAIN_LAYERS)
			//	{
			//		bRes = WorldData_ReloadTerrainLayers();
			//	} 
			//	else if (nWDBType == WDB_VEGETATION)
			//	{
			//		bRes = WorldData_ReloadVegetationMap();
			//	}
			//	m_WorldData[nWDBType].Unlock();
			//}
			//else
			//{
			//	bRes = WorldData_Revert(VT_OFFICIAL);
			//}
		}
		break;
	case sw::WDBCC_Unlock:
		{
			bRes = m_WorldData[nWDBType].Unlock();
		}
		break;
	default:
		break;
	}

	if (nWDBType == WDB_VEGETATION)
		GetIEditor()->Notify( eNotify_OnSWVegetationStatusChange );
	SWLog("WDBCC Executed...", true);
	return bRes;
}
//-------------------------------------------------------------------------
bool CSegmentedWorldDoc::IsInSelectMode()
{
	CGridMapDlg *pDlg = CGridMapDlg::Get();
	if (!pDlg)
		return false;

	return pDlg->IsSelectMode();
}

bool CSegmentedWorldDoc::IsSegmentSelected(const TWorldCoords &wc)
{
	CGridMapDlg *pDlg = CGridMapDlg::Get();
	if (!pDlg)
		return true;

	if (pDlg->IsOpenForEditMode())
		return true;

	if (pDlg->IsSegmentSelected(CPoint(wc.wx,wc.wy)))
		return true;
	
	return false;
}



void CSegmentedWorldDoc::OnWorldMapUIMoved(const Vec2 &ptOfs, int nZoomLevel)
{
	m_viewStates.m_ptWorldMapOfs = ptOfs;
	m_viewStates.m_nWorldMapZoom = nZoomLevel;
}

bool CSegmentedWorldDoc::IsVegetationMapLockedByMe()
{
	//m_WorldData[WDB_VEGETATION].UpdateStatus();
	AUTO_LOCK(g_cs);
	if (m_WorldData[WDB_VEGETATION].IsLockedByMe())
		return true;
	return false;
}

bool CSegmentedWorldDoc::CanModifyWorldData(EWDBType eType, bool bLock)
{
	int nType = (int)eType;
	{
		AUTO_LOCK(g_cs);
		if (m_WorldData[nType].IsLockedByMe())
			return true;
	}
	if (bLock)
		return (m_WorldData[nType].Lock() > 0);
	return false;
}
// notification callbacks
///////////////////////////////////////////////////////////
bool CSegmentedWorldDoc::OnWorldDataModified(int nType)
{
	if (m_WorldData[nType].Lock()) {
		m_WorldData[nType].ModifiedLocally();
		return true;
	}
	AUTO_LOCK(g_cs);
	CString msg;
	char szLockedBy[ISWVersionControl::MAX_USERNAME_CBSIZE];
	msg.Format("Access denied!!!\r\n\r\n%s is Locked by %s(%d)!",
		GetWorldDataBlockName((EWDBType)nType),
		//g_db->GetUserName( DataInspector::m_nLockedBy(m_WorldData[nType])),
		m_pVersionControl->GetLockedBy(m_WorldData[nType],szLockedBy,ISWVersionControl::MAX_USERNAME_CBSIZE,"<N/A>"),
		DataInspector::m_nLockedBy(m_WorldData[nType]));
	AfxMessageBox( _T(msg), MB_ICONEXCLAMATION|MB_OK );
	return false;
}
//segment notifications
///////////////////////////////////////////////////////////
void CSegmentedWorldDoc::OnHeightmapModified( const AABB &bounds )
{
	if (CSWManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return;
	if (bounds.IsReset())
		return;
	if(CSWManager::Get().IsIgnoringChanges())
		return;
	TLocalCoords lcMin = WorldToSegment(bounds.min);
	TLocalCoords lcMax = WorldToSegment(bounds.max);
	m_segs.SetModified(lcMin, lcMax, SDB_Heights);
	//!?! SetModified(lcMin, lcMax, SDB_LayerIDs);
	m_segs.SetModified(lcMin, lcMax, SDB_EngData);
	m_segs.LockSegment(lcMin, lcMax, true);
}

void CSegmentedWorldDoc::OnVegetationModified( const AABB &bounds )
{
	if (CSWManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return;
	if (bounds.IsReset())
		return;
	if(CSWManager::Get().IsIgnoringChanges())
		return;
	TLocalCoords lcMin = WorldToSegment(bounds.min);
	TLocalCoords lcMax = WorldToSegment(bounds.max);
	m_segs.SetModified(lcMin, lcMax, SDB_Vegetation);
	m_segs.SetModified(lcMin, lcMax, SDB_EngData);
	m_segs.LockSegment(lcMin, lcMax, true);
}

void CSegmentedWorldDoc::OnObjectModified( CBaseObject *pObject,bool bDelete, CBaseObject* pLinkTarget)
{ //!!! not implemented yet
	if (CSWManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return;
	if (GetIEditor()->IsInGameMode() || gEnv->IsEditorGameMode())
		return;
	if (GetISystem()->IsQuitting())
		return;
	if(CSWManager::Get().IsIgnoringChanges())
		return;

	
	CBaseObject *pAdam = GetRootObject(pObject);
	Vec3 pos = pAdam->GetPos();
	
	TLocalCoords lc = WorldToSegment(pos, false);
	
	//SetModified(lc, SDB_EngData);
	//if (pObject->IsKindOf(RUNTIME_CLASS(CEntity)))
	//	SetModified(lc, SDB_EngEntities);
	//LockSegment(lc);

	bool createdNow = false;
	bool isExternal = m_extobjs.IsExternal(pAdam, &createdNow);
	bool needToModifyLayer = !isExternal;
	if (needToModifyLayer || createdNow)
		m_segs.SetLayerDataModified(lc, pAdam->GetLayer()->GetGUID(), pAdam->GetLayer()->GetName());
	
	if (pAdam != pObject)
	{
		m_extobjs.ObjectData_SetModified(pObject, bDelete, pLinkTarget);
		m_extobjs.ObjectData_SetModified(pAdam, false);
	}
	else
		m_extobjs.ObjectData_SetModified(pAdam, bDelete, pLinkTarget);
	
	if (!m_segs.LockSegmentLayerData(lc, pAdam->GetLayer()->GetGUID(), pAdam->GetLayer()->GetName()))
		assert(0);

	// if it was external, clear disapproved flag and recalculate
	if (!bDelete && isExternal && !createdNow )
	{
		m_extobjs.RecheckExternal(pAdam,pLinkTarget);
	}
}

void CSegmentedWorldDoc::OnObjectModified( CBaseObject *pObject, const Vec3 &newPos, CObjectLayer *pNewLayer)
{
	if (CSWManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return;
	if (GetIEditor()->IsInGameMode() || gEnv->IsEditorGameMode())
		return;
	if (GetISystem()->IsQuitting())
		return;
	if(CSWManager::Get().IsIgnoringChanges())
		return;

	Vec3 pos = newPos;
	CBaseObject *pAdam = pObject;
	if(pObject->GetParent() || pObject->GetGroup())
	{
		pAdam = GetRootObject(pObject);
		pos = pAdam->GetPos();
	}

	TLocalCoords lc = WorldToSegment(pos, false);
	assert(pNewLayer);
	
	//SetModified(lc, SDB_EngData);
	//if (pObject->IsKindOf(RUNTIME_CLASS(CEntity)))
	//	SetModified(lc, SDB_EngEntities);
	//LockSegment(lc);
	
	bool createdNow = false;
	bool needToModifyLayer = !m_extobjs.IsExternal(pAdam, &createdNow);
	if (needToModifyLayer || createdNow)
		m_segs.SetLayerDataModified(lc, pNewLayer->GetGUID(), pNewLayer->GetName());
	
	if (pAdam != pObject)
		m_extobjs.ObjectData_SetModified(pObject, false);
	m_extobjs.ObjectData_SetModified(pAdam, false);
	if(!m_segs.LockSegmentLayerData(lc, pNewLayer->GetGUID(), pNewLayer->GetName()))
		assert(0);
	
}

void CSegmentedWorldDoc::OnTerrainRGBLayerPainted(int32 posx,int32 posy,int w,int h, bool bIsInWorldCoord)
{ //!!! not implemented yet
	if (CSWManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return;
	if(CSWManager::Get().IsIgnoringChanges())
		return;
	int nTerrainSectorSize= (bIsInWorldCoord ? 1 : gEnv->p3DEngine->GetTerrainSectorSize());
	Vec3 p1((posy/*-1*/)*nTerrainSectorSize,(posx/*-1*/)*nTerrainSectorSize,-32000.0f);
	Vec3 p2((posy/*+1*/ + h)*nTerrainSectorSize,(posx/*+1*/ + w)*nTerrainSectorSize,+32000.0f);
	TLocalCoords lcMin = WorldToSegment(p1);
	TLocalCoords lcMax = WorldToSegment(p2);
	//SetModified(lcMin, lcMax, SDB_LayerIDs);
	m_segs.SetModified(lcMin, lcMax, SDB_RGBLayer);
	m_segs.SetModified(lcMin, lcMax, SDB_EngData);
	m_segs.SetModified(lcMin, lcMax, SDB_ModSectors);
	m_segs.LockSegment(lcMin, lcMax, true);
}

void CSegmentedWorldDoc::OnTerrainLayerIDPainted(int32 posx,int32 posy,int w,int h, bool bIsInWorldCoord)
{ //!!! not implemented yet
	if (CSWManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return;
	if(CSWManager::Get().IsIgnoringChanges())
		return;
	int nTerrainSectorSize= (bIsInWorldCoord ? 1 : gEnv->p3DEngine->GetTerrainSectorSize());
	Vec3 p1((posy/*-1*/)*nTerrainSectorSize,(posx/*-1*/)*nTerrainSectorSize,-32000.0f);
	Vec3 p2((posy/*+1*/ + h)*nTerrainSectorSize,(posx/*+1*/ + w)*nTerrainSectorSize,+32000.0f);
	TLocalCoords lcMin = WorldToSegment(p1);
	TLocalCoords lcMax = WorldToSegment(p2);
	m_segs.SetModified(lcMin, lcMax, SDB_LayerIDs);
	//SetModified(lcMin, lcMax, SDB_RGBLayer);
	m_segs.SetModified(lcMin, lcMax, SDB_EngData);
	//SetModified(lcMin, lcMax, SDB_ModSectors);
	m_segs.LockSegment(lcMin, lcMax, true);
}

bool CSegmentedWorldDoc::OnGenerateSurfaceTexture(int wx, int wy )
{
	TWorldCoords wc(wx, wy);
	if (!m_segs.LockSegment(wc))
		return false;

	m_segs.SetModified(wc, SDB_ModSectors);

	m_segs.SetModified(wc, SDB_EngRGBData);
	m_segs.SetModified(wc, SDB_EngData);
	m_segs.SetModified(wc, SDB_EngEntities);
	m_segs.UpdateFileBlockVersion(wc, 0, SDB_EngRGBData, true);

	return true;
}
// Objects check
///////////////////////////////////////////////////////////
bool CSegmentedWorldDoc::IsMultiSelObjectLockedByMe()
{
	CSelectionGroup *pSel = GetIEditor()->GetSelection();
	for (int i = 0; i < pSel->GetCount(); i++)
	{
		CBaseObject *pObject = pSel->GetObject(i);
		if(!IsLockedByMe(pObject))
			return false;
	}
	return true;
}

bool CSegmentedWorldDoc::IsLockedByMe(CBaseObject *pObject)
{
	if (CSWManager::Get().IsOfflineMode()) return true;
	if (!CanModify(pObject, false, false))
		return false;

	CBaseObject *pAdam = GetRootObject(pObject);
	Vec3 pos = pAdam->GetPos();
	
	TLocalCoords lc = WorldToSegment(pos, false);
	//TSegmentData *pd = GetSegmentData(lc, true);
	//if (!pd)
	//	return false;
	////if (pd->m_nLockedBy == m_pVersionControl->GetUserID())
	////	return true;

	////vm check
	//TLayerData *pl = pd->CreateLayer(pAdam->GetLayer()->GetGUID(), pAdam->GetLayer()->GetName());
	assert(pAdam);
	TLayerData *pl = m_segs.CreateSegmentLayer(lc, pAdam->GetLayer()->GetGUID(), pAdam->GetLayer()->GetName());
	if (!pl)
		return false;

	{
		AUTO_LOCK(g_cs);
		if (pl->IsLockedByMe())
			return true;
	}

	return false;
}

bool CSegmentedWorldDoc::LockMultiSelObjects(bool bModify)
{
	if (CSWManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return true;
	if (GetIEditor()->IsInGameMode() || gEnv->IsEditorGameMode())
		return true;
	if (GetISystem()->IsQuitting())
		return true;

	TWorldCoordsArray arrLocked;
	std::vector<TLayerData*> arrLayerData;
	ELockResult nRes = LockResult_Unspecified;
	CSelectionGroup *pSel = GetIEditor()->GetSelection();
	for (int i = 0; i < pSel->GetCount(); i++)
	{
		CBaseObject *pObject = pSel->GetObject(i);
		CBaseObject *pAdam = GetRootObject(pObject);
		Vec3 pos = pAdam->GetPos();
		
		TLocalCoords lc = WorldToSegment(pos, false);
		if (!IsInLoadedWorld(lc))
		{
			nRes = LockResult_Failed;
			break;
		}

		TWorldCoords wc(lc);
		//nRes = LockSegment(wc);
		//if(nRes == 2)
		//	arrLocked.push_back(wc);
		//if(nRes == 0)
		//	break;

		//vm check LayerData
		//TSegmentData* pd = GetSegmentData(wc, true);
		//TLayerData* pl = pd->CreateLayer(pAdam->GetLayer()->GetGUID(), pAdam->GetLayer()->GetName());
		assert(pAdam);
		TLayerData *pl = m_segs.CreateSegmentLayer(wc, pAdam->GetLayer()->GetGUID(), pAdam->GetLayer()->GetName());
		if(pl)
			nRes = pl->Lock();
		else
			nRes = LockResult_Failed;
		
		if(nRes == LockResult_Succeeded)
			arrLayerData.push_back(pl);
		if(nRes == LockResult_Failed)
			break;

		//~LayerData
	}
	
	if (nRes == LockResult_Failed)
	{
		//for (int i = 0; i < arrLocked.size(); ++i)
		//	UnlockSegment(arrLocked[i]);
		for (int i = 0; i < arrLayerData.size(); ++i)
			arrLayerData[i]->Unlock();
		return false;
	}

	if (bModify)
	{
		for (int i = 0; i < pSel->GetCount(); i++)
		{
			CBaseObject *pObject = pSel->GetObject(i);
			CBaseObject *pAdam = GetRootObject(pObject);
			Vec3 pos = pAdam->GetPos();
			TLocalCoords lc = WorldToSegment(pos, false);
			//SetModified(lc, SDB_EngData);
			//if (pObject->IsKindOf(RUNTIME_CLASS(CEntity)))
			//	SetModified(lc, SDB_EngEntities);
			bool createdNow = false;
			bool needToModifyLayer = !m_extobjs.IsExternal(pAdam, &createdNow);
			if (needToModifyLayer || createdNow)
				m_segs.SetLayerDataModified(lc, pAdam->GetLayer()->GetGUID(), pAdam->GetLayer()->GetName());

			if (pAdam != pObject)
				m_extobjs.ObjectData_SetModified(pObject, false);
			m_extobjs.ObjectData_SetModified(pAdam, false);
		}
	}

	return true;
}

bool CSegmentedWorldDoc::LockObject(CBaseObject *pObject, bool bModify)
{
	if (CSWManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return true;
	if (GetIEditor()->IsInGameMode() || gEnv->IsEditorGameMode())
		return true;
	if (GetISystem()->IsQuitting())
		return true;
	
	CBaseObject *pAdam = GetRootObject(pObject);
	Vec3 pos = pAdam->GetPos();
	TLocalCoords lc = WorldToSegment(pos, false);

	if (!IsInLoadedWorld(lc))
		return false;

	//if(!LockSegment(lc))
	//	return false;
	
	//SetModified(lc, SDB_EngData);
	//if (pObject->IsKindOf(RUNTIME_CLASS(CEntity)))
	//	SetModified(lc, SDB_EngEntities);

	//vm check
	if(!m_segs.LockSegmentLayerData(lc, pAdam->GetLayer()->GetGUID(), pAdam->GetLayer()->GetName()))
		return false;

	if (bModify)
	{
		bool createdNow = false;
		bool needToModifyLayer = !m_extobjs.IsExternal(pAdam, &createdNow);
		if (needToModifyLayer || createdNow)
			m_segs.SetLayerDataModified(lc, pAdam->GetLayer()->GetGUID(), pAdam->GetLayer()->GetName());
		if (pAdam != pObject)
			m_extobjs.ObjectData_SetModified(pObject, false);
		m_extobjs.ObjectData_SetModified(pAdam, false);
	}

	return true;
}

bool CSegmentedWorldDoc::CanObjectPlaceTo(const Vec3 &newPos, CObjectLayer *pLayer, bool bLock)
{
	if(CSWManager::Get().IsIgnoringChanges() || CSWManager::Get().IsOfflineMode())
		return true;
	if (CSWManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return true;
	if (GetIEditor()->IsInGameMode() || gEnv->IsEditorGameMode())
		return true;
	if (GetISystem()->IsQuitting())
		return true;

	TLocalCoords lc = WorldToSegment(newPos, false);
	
	if (!IsInLoadedWorld(lc))
		return false;

	if(bLock)
	{
		//vm check
		return m_segs.LockSegmentLayerData(lc, pLayer->GetGUID(), pLayer->GetName());
			//return LockSegment(lc);
		//return false;
	}

	//TSegmentData *pd = GetSegmentData(lc, true);
	//if (!pd)
	//	return false;

	////vm check
	//TLayerData *pl = pd->CreateLayer(pLayer->GetGUID(), pLayer->GetName());
	TLayerData *pl = m_segs.CreateSegmentLayer(lc, pLayer->GetGUID(), pLayer->GetName());
	if (!pl)
		return false;
	AUTO_LOCK(g_cs);
	if (pl->IsLockedByMe() || IsHot())
		return true;
	if (pl->IsLocked())
		return false;

	//if (pd->m_nLockedBy == m_pVersionControl->GetUserID())
	//	return true;
	//if (pd->m_nLockedBy > 0)
	//	return false;
	return true;
}

bool CSegmentedWorldDoc::CanModifyMultiSel(bool bDelete, bool bLock)
{
	if(bLock)
		return LockMultiSelObjects(false);
	
	CSelectionGroup *pSel = GetIEditor()->GetSelection();
	for (int i = 0; i < pSel->GetCount(); i++)
	{
		CBaseObject *pObject = pSel->GetObject(i);
		if(!CanModify(pObject, bDelete, bLock))
			return false;
	}
	return true;
}

bool CSegmentedWorldDoc::CanModify( const TWorldCoords &wc )
{
	//TSegmentData *pd = GetSegmentData(wc, true);
	//if (!pd)
	//	return false;
	//if (pd->m_nLockedBy == m_pVersionControl->GetUserID())
	//	return 1;
	//if (pd->m_nLockedBy > 0)
	//	return 0;
	return m_segs.LockSegment(wc);
}

bool CSegmentedWorldDoc::CanModify( const AABB &bounds )
{
	if (CSWManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return true;
	if (bounds.IsReset())
		return false;
	TLocalCoords lcMin = WorldToSegment(bounds.min);
	TLocalCoords lcMax = WorldToSegment(bounds.max);
	TWorldCoords wcMin(lcMin);
	TWorldCoords wcMax(lcMax);
	TWorldCoords wc;
	for (wc.wy = wcMin.wy; wc.wy <= wcMax.wy; ++wc.wy) {
		for (wc.wx = wcMin.wx; wc.wx <= wcMax.wx; ++wc.wx) {
			if(!CanModify(wc))
				return false;
		}
	}

	return true;
}

bool CSegmentedWorldDoc::CanModify( int x1, int y1, int x2, int y2, bool bIsInWorldCoord, bool bWithAdditionalSafeZone )
{
	AABB bounds;
	CSWManager::CoordToAABB(bounds, x1, y1, x2, y2, bIsInWorldCoord, bWithAdditionalSafeZone);
	return CanModify(bounds);
}

CBaseObject* CSegmentedWorldDoc::GetRootObject( CBaseObject *pObject )
{ 
	if (NULL == pObject)
		return NULL;

	CBaseObject *pParent,*pAdam=pObject; 
	while(pParent=pAdam->GetParent()) pAdam=pParent;
	if(pObject->GetGroup())
	{
		pAdam = pObject->GetGroup();
		while(pParent=pAdam->GetGroup()) pAdam=pParent;
	}
	return pAdam;
}

//////////////////////////////////////////////////////////////////////////////////////////////
bool CSegmentedWorldDoc::GetEngDataFromLevelPak(CMemoryBlock &mem, const TWorldCoords &wc, ESDBType eFileBlockType)
{
	mem.Free();
	CString sSegmentPakFileName;
	CString sSegmentPath; 
	CString sFileBlockPath;
	
	// name of pak
	m_pPersistance->GetPakPath(sSegmentPakFileName, sSegmentPath, sFileBlockPath, wc, s_bLevelMultiPack, false);

	// name of file block
	CString strFileBlockName;
	strFileBlockName.Format( "%s%s", sFileBlockPath, GetFileBlockName(eFileBlockType) );

	// Make sure 3D engine closes texture handle.
	GetIEditor()->Get3DEngine()->CloseTerrainTextureFile();
	if(!gEnv->pCryPak->ClosePack(sSegmentPakFileName))
	{
		assert(0);
		return false;
	}

	gEnv->pCryPak->OpenPack(sSegmentPakFileName);


	FILE *pFile = gEnv->pCryPak->FOpen(strFileBlockName, "rb");
	if (!pFile)
		return false;

	// get size of ctc file
	gEnv->pCryPak->FSeek(pFile, 0, SEEK_END);
	int iSize = gEnv->pCryPak->FTell(pFile); 
	gEnv->pCryPak->FSeek(pFile, 0, SEEK_SET);


	if(iSize > 0) {
		// read to memoryblock
		CMemoryBlock temp;
		temp.Allocate(iSize);
		int resultOfRead = gEnv->pCryPak->FReadRawAll(temp.GetBuffer(), iSize, pFile); //file.Read(temp.GetBuffer(), fileSize);
		gEnv->pCryPak->FClose(pFile);
		temp.Compress(mem);
	} else {
		gEnv->pCryPak->FClose(pFile);
	}
	//gEnv->pCryPak->ClosePack(sSegmentPakFileName);

	// reopen pak for streaming
	//gEnv->pCryPak->OpenPack(sSegmentPakFileName);
	return true;
}

bool CSegmentedWorldDoc::GenerateLevelPakFromFileBlocks( const TWorldCoords &wc, ESDBType eFileBlockType, CPakFile &pakFile, bool bForce)
{
	TSegmentData *pSeg = m_segs.GetSegmentData(wc, false);
	if (!pSeg )
		return false;

	// name of pak
	CString sSegmentPakFileName;
	CString sSegmentPath;
	CString sFileBlockPath;

	m_pPersistance->GetPakPath(sSegmentPakFileName, sSegmentPath, sFileBlockPath, wc, s_bLevelMultiPack, true);

	// name of ctc file
	CString strFileBlockName;
	strFileBlockName.Format( "%s%s", sFileBlockPath, GetFileBlockName(eFileBlockType) );
	CString sVerFilename = strFileBlockName + ".ver";

	// check if need to download
	TSegDataBlock* pBlock = 0;
	if (!bForce)
	{
		// check if pak file exist
		if (!gEnv->pCryPak->OpenPack(sSegmentPakFileName))
			goto lb_DownloadFileBlockFromDB_needdownload;

		FILE *pFile = gEnv->pCryPak->FOpen(sVerFilename, "r");
		if (!pFile)
			goto lb_DownloadFileBlockFromDB_needdownload;

		int64 iHRID = 0;
		//read version of ctc file
		int resultOfRead = gEnv->pCryPak->FRead<int64>(&iHRID,1,pFile);
		gEnv->pCryPak->FClose(pFile);

		pBlock = pSeg->GetBlock(eFileBlockType,false);
		if (!pBlock)
			goto lb_DownloadFileBlockFromDB_needdownload;

		AUTO_LOCK(g_cs);
		//if(iHRID == pBlock->m_nLoadedHRID)
		if (pBlock->LoadedVersionIs(iHRID))
			return true; // we are good
	}

lb_DownloadFileBlockFromDB_needdownload:
	// Make sure 3D engine closes texture handle.
	GetIEditor()->Get3DEngine()->CloseTerrainTextureFile();
	if(!gEnv->pCryPak->ClosePack(sSegmentPakFileName))
	{
		assert(0);
		return false;
	}

	//open segment pak for writing
	CPakFile segmentPakFile;
	CPakFile *pPakFile = s_bLevelMultiPack ? &segmentPakFile : &pakFile;
	if (s_bLevelMultiPack && !pPakFile->Open(sSegmentPakFileName)) {
		CRY_ASSERT(!"could not open segment pak for writing");
		return false;
	}

	// uncompress to memory block
	//CMemoryBlock mem;
	//pb->m_data.Uncompress(mem);
	CMemoryBlock mem;
	pSeg->GetData(eFileBlockType,mem);

	// update file
	//segmentPakFile.Open(sSegmentPakFileName);
	pPakFile->GetArchive()->UpdateFile(strFileBlockName, mem.GetBuffer(), mem.GetSize());

	// update version file
	{
		if (!pBlock) // re-fetch block ptr
			pBlock = pSeg->GetBlock(eFileBlockType, false);

		assert(pBlock);

		int64 i64HRID;
		{ AUTO_LOCK(g_cs);
			i64HRID = pBlock->GetLoadedVersion();
		}
		pPakFile->GetArchive()->UpdateFile(sVerFilename, &i64HRID,sizeof(i64HRID));
	}

	if(s_bLevelMultiPack)
	{
		pPakFile->Close();

		if(!gEnv->pCryPak->OpenPack(sSegmentPakFileName))
		{
			Warning( "Cannot Open Pak File %s, set larger value to sys_max_stdio", sSegmentPakFileName);
		}
	}

	return true;
}
//----------------------------------------------------------------------
void CSegmentedWorldDoc::DoUpdateLevelPak()
{
	AUTO_LOCK(g_cs);
	bool bWasActive = GetIEditor()->Get3DEngine()->IsSegmentedWorldActive();
	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);

	//1. ensure pack open for read
	//2. check if need from sync
	//3. get file blocks version on hdd

	CString strSegmentPakFileName;
	CString sLevelPath = m_sPath;
	CString sSegmentPath = sLevelPath;
	CString sFileBlockPath;

	if (!CFileUtil::PathExists(sLevelPath))
		CFileUtil::CreateDirectory(sLevelPath);

	sLevelPath = Path::AddBackslash( sLevelPath );

	// name of pak
	strSegmentPakFileName.Format("%slevel.pak", sLevelPath);

	bool bForceUpdateAll = false;
	if (!s_bLevelMultiPack && !gEnv->pCryPak->OpenPack(strSegmentPakFileName))
	{
		bForceUpdateAll = true;
		//GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(bWasActive);
		//return;
	}

	CLogFile::WriteLine("Synchronizing ...");
	CWaitProgress progress( _T("Synchronizing ...") );
	progress.Start();
	int nNumSteps = m_uiHeight*m_uiWidth+1;
	int nStep = 0;
	TLocalCoords lc;
	std::vector<TSegmentData*> arrSegmentDataBlocksToUpdate;
	CMemoryBlock mem;
	for (lc.ly = 0; lc.ly < m_uiHeight; ++lc.ly)
	{
		for (lc.lx = 0; lc.lx < m_uiWidth; ++lc.lx)
		{
			progress.Step( 100*nStep/nNumSteps );
			nStep++;
			TSegmentData *psd = m_segs.GetSegmentData(lc, false);
			if(!psd)
				continue;
			
			bool bNeedUpdate = false;
			if (s_bLevelMultiPack)
			{
				TWorldCoords wc(lc);
				m_pPersistance->GetPakPath(strSegmentPakFileName, sSegmentPath, sFileBlockPath, wc, s_bLevelMultiPack, true);
				if (!gEnv->pCryPak->OpenPack(strSegmentPakFileName))
					bNeedUpdate = true;
			}

			if (!bForceUpdateAll && !bNeedUpdate)
			{
				for (ESDBTypeIterator i = SDB_ENGINEDATA_BEGIN; i != SDB_ENGINEDATA_END; ++i)
				{
					TSegDataBlock* pb = NULL;
					if(psd->NeedsToBeApplied(i))
					{
						psd->GetData(i, mem);

						pb = psd->GetBlock(i, false);
						assert(pb);

						// name of version file
						CString sVerFilename;
						sVerFilename.Format( "%s%s.ver", sFileBlockPath, GetFileBlockName(i));
						FILE *pFile = gEnv->pCryPak->FOpen(sVerFilename, "r");
						if (!pFile)
						{
							//pb->SetAppliedHRID(HRID_UNKNOWN);
							pb->GoToState(Versioned::S_Unapplied);
							bNeedUpdate = true;
							continue;
						}

						int64 iHRID = 0;
						//read version file
						int resultOfRead = gEnv->pCryPak->FRead<int64>(&iHRID,1,pFile);
						gEnv->pCryPak->FClose(pFile);
						//pb->SetAppliedHRID(iHRID);
						assert(pb->LoadedVersionIs(iHRID) && pb->IsStateAtLeast(Versioned::S_Applied));
						pb->GoToStateAtLeast(Versioned::S_Applied);
						
						bNeedUpdate = true;
					}
				}
			}
			else
			{
				bNeedUpdate = true;
			}

			if (bNeedUpdate)
				arrSegmentDataBlocksToUpdate.push_back(psd);
		}
	}

	//4. check if need from sync
	
	if (arrSegmentDataBlocksToUpdate.size() == 0)
	{
		GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(bWasActive);
		return; //no file sync is needed
	}

	bool bNeedUpdate = false;
	for(int nCurrSD = 0; nCurrSD < arrSegmentDataBlocksToUpdate.size(); nCurrSD++)
	{	
		for (ESDBTypeIterator i = SDB_ENGINEDATA_BEGIN; i != SDB_ENGINEDATA_END; ++i)
		{
			if(arrSegmentDataBlocksToUpdate[nCurrSD]->NeedsToBeApplied(i))
			{
				bNeedUpdate = true;
				//break;
			}
			else
			{
				if (IsBlockTypeNeeded(i))
					arrSegmentDataBlocksToUpdate[nCurrSD]->Applied(i);
			}
		}
	}

	if (!bNeedUpdate)
	{
		GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(bWasActive);
		return; //no file sync is needed
	}

	
	//5. ensure pack open for write
	// Make sure 3D engine closes texture handle.
	GetIEditor()->Get3DEngine()->CloseTerrainTextureFile();
	CPakFile segmentPakFile;
	if (!s_bLevelMultiPack)
	{
		if(!gEnv->pCryPak->ClosePack(strSegmentPakFileName))
		{
			GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(bWasActive);
			return;
		}

		//open segment pak for writing
		if (!segmentPakFile.Open(strSegmentPakFileName)) {
			CRY_ASSERT(!"could not open segment pak for writing");
			GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(bWasActive);
			return;
		}
	}	

	//6. write file blocks on hdd
	//7. write file blocks version on hdd
	nNumSteps = arrSegmentDataBlocksToUpdate.size();
	nStep = 0;
	TSegDataBlock* pb = NULL;
	CString strFileBlockName;
	CString sVerFilename;
	
	for(int nCurrSD = 0; nCurrSD < arrSegmentDataBlocksToUpdate.size(); nCurrSD++)
	{
		progress.Step( 100*nCurrSD/nNumSteps );
		TSegmentData* psd = arrSegmentDataBlocksToUpdate[nCurrSD];
		if (s_bLevelMultiPack)
		{
			m_pPersistance->GetPakPath(strSegmentPakFileName, sSegmentPath, sFileBlockPath, psd->m_wc, s_bLevelMultiPack, true);
			if(!gEnv->pCryPak->ClosePack(strSegmentPakFileName))
			{
				GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(bWasActive);
				return;
			}

			//open segment pak for writing
			if (!segmentPakFile.Open(strSegmentPakFileName)) {
				CRY_ASSERT(!"could not open segment pak for writing");
				GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(bWasActive);
				return;
			}
		}	

		for (ESDBTypeIterator i = SDB_ENGINEDATA_BEGIN; i != SDB_ENGINEDATA_END; i++)
		{
			if(psd->NeedsToBeApplied(i))
			{
				// name of file block
				sVerFilename.Format( "%s%s.ver", sFileBlockPath, GetFileBlockName(i));
				strFileBlockName.Format( "%s%s", sFileBlockPath, GetFileBlockName(i) );
				bool bDelete = false;
				if (psd->GetData(i, mem))
				{
					pb = psd->GetBlock(i, false);
					assert(pb);
					// update files
					int64 nLoadedHRID = pb->GetLoadedVersion();
					segmentPakFile.GetArchive()->UpdateFile(strFileBlockName, mem.GetBuffer(), mem.GetSize());
					segmentPakFile.GetArchive()->UpdateFile(sVerFilename, &nLoadedHRID,sizeof(nLoadedHRID));
					/*if (g_pcSFileBlockMAP[i] == SDB_Map)
					{
						CString sMiniMapFileName;
						sMiniMapFileName.Format("%smap.bmp",sSegmentPath);
						CImage img;
						img.Allocate(128, 128);
						img.Uncompress(pb->m_data);
						CImageUtil::SaveBitmap(sMiniMapFileName, img);
					}*/
				}
				else
				{
					bDelete = true;
				}

				if (bDelete)
				{
					segmentPakFile.RemoveFile(strFileBlockName);
					segmentPakFile.RemoveFile(sVerFilename);
					//int64 nCurrVersion = HRID_MISSING;
					//segmentPakFile.GetArchive()->UpdateFile(sVerFilename, &nCurrVersion,sizeof(nCurrVersion));
				}
				
				//8. mark file blocks as sync-ed
				psd->Applied(i);
			}
		}

		if (s_bLevelMultiPack)
		{
			segmentPakFile.Close();
			gEnv->pCryPak->OpenPack(strSegmentPakFileName);
		}
	}
	
	if (!s_bLevelMultiPack)
	{
		//9. close pack
		segmentPakFile.Close();

		//10. open pak for read
		gEnv->pCryPak->OpenPack(strSegmentPakFileName);
	}

	// No need from CRC for now
	//11. computeCRC
	//12. open pak for write
	//13. UpdateCRC
	//14. close pak
	

	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(bWasActive);
}

void CSegmentedWorldDoc::WorldVecToGlobalSegVec(const Vec3 &inPos, Vec3 &outPos, Vec2 &outAbsCoords)
{
	outAbsCoords.x = (int)inPos.y / SEGMENT_SIZE_METERS;
	outAbsCoords.y = (int)inPos.x / SEGMENT_SIZE_METERS;
	outPos = inPos;
	outPos.x -= outAbsCoords.y * SEGMENT_SIZE_METERS;
	outPos.y -= outAbsCoords.x * SEGMENT_SIZE_METERS;
}

void CSegmentedWorldDoc::GlobalSegVecToLocalSegVec(const Vec3 &inPos, const Vec2 &inAbsCoords, Vec3 &outPos)
{
	assert(0);
}

void CSegmentedWorldDoc::GetTerrainSizeInMeters(int &x, int &y)
{
	x = m_uiWidth * SEGMENT_SIZE_METERS;
	y = m_uiHeight * SEGMENT_SIZE_METERS;
}

bool CSegmentedWorldDoc::CreateSegments(ITerrain *pTerrain)
{
	AUTO_LOCK(g_cs);
	CXmlArchive	ar;
	XmlNodeRef node = XmlHelpers::CreateXmlNode( "SurfaceTypes" );
	for (int i = 0; i < CTerrainManager::GetTerrainManager().GetSurfaceTypeCount(); i++)
	{
		ar.root = node->newChild( "SurfaceType" );
		CTerrainManager::GetTerrainManager().GetSurfaceTypePtr(i)->Serialize( ar );
	}

	Vec3 vSize((float) SEGMENT_SIZE_METERS, (float) SEGMENT_SIZE_METERS, (float) SEGMENT_SIZE_METERS);
	TLocalCoords lc;
	CString sSegmentPakFileName;
	CString sSegmentPath;
	CString sFileBlockPath;
	for (lc.ly = 0; lc.ly < m_uiHeight; ++lc.ly) {
		for (lc.lx = 0; lc.lx < m_uiWidth; ++lc.lx) {
			TSegmentData *psd = m_segs.GetSegmentData(lc, true);
			if (!psd)
				continue;
			AABB bb = CalcWorldBounds(lc);
			Vec3 vOfs = bb.min;
			vOfs.z = 0;
			if (!psd->IsInUse()) {

				m_pPersistance->GetPakPath(sSegmentPakFileName, sSegmentPath, sFileBlockPath, psd->m_wc, s_bLevelMultiPack, false);
				sFileBlockPath = Path::FullPathToLevelPath(sFileBlockPath);
				int nSID = pTerrain->CreateSegment(vSize, vOfs, sFileBlockPath);
				GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);
				GetIEditor()->Get3DEngine()->LoadTerrainSurfacesFromXML(node, false, nSID);
				GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(false);
				psd->OnCreatedInEngine(nSID);
			} else
				pTerrain->SetSegmentOrigin(psd->GetEngineSID(), vOfs);
		}
	}
	return true;
}

bool CSegmentedWorldDoc::DeleteSegments(ITerrain *pTerrain)
{
	AUTO_LOCK(g_cs);
	TLocalCoords lc;
	for (lc.ly = 0; lc.ly < m_uiHeight; ++lc.ly) {
		for (lc.lx = 0; lc.lx < m_uiWidth; ++lc.lx) {
			TSegmentData *psd = m_segs.GetSegmentData(lc, false);
			if (!psd || !psd->IsInUse())
				continue;
			if (pTerrain)
				pTerrain->DeleteSegment(psd->GetEngineSID(),true);
			psd->OnRemoveFromEngine();
			psd->NoLongerApplied();
			m_extobjs.ObjectData_NoLongerApplied(lc);
		}
	}
	return true;
}

bool CSegmentedWorldDoc::FindSegment(ITerrain *pTerrain, Vec3 pt, int &nSID)
{
	if (pt.x < 0 || pt.y < 0) {
		nSID = -1;
		return true;
	}
	CPoint pthm = GetIEditor()->GetHeightmap()->WorldToHmap(pt);
	if (pthm.x >= m_uiWidth * SEGMENT_SIZE_UNITS || pthm.y >= m_uiHeight * SEGMENT_SIZE_UNITS) {
		nSID = -1;
		return true;
	}
	TLocalCoords lc(pthm.x / SEGMENT_SIZE_UNITS, pthm.y / SEGMENT_SIZE_UNITS);
	TSegmentData *psd = m_segs.GetSegmentData(lc, false);
	if (!psd) {
		nSID = -1;
		return true;
	}

	nSID = psd->GetEngineSID();

#ifdef _DEBUG
	if (psd->IsInUse())	// i.e. SID >= 0
	{
		AABB aabb;
		if (!pTerrain->GetSegmentBounds(psd->GetEngineSID(), aabb))
			assert(0);
		else
			assert(Overlap::Point_AABB2D(pt, aabb));
	}
#endif

	return true;
}



bool CSegmentedWorldDoc::GetRGBLayerData(int lx, int ly, CMemoryBlock &mem)
{
	RGB2Seg(lx, ly);
	TSegmentData *pd = m_segs.GetSegmentData(TLocalCoords(lx, ly), true);
	if (!pd)
		return false;
	return pd->GetData(SDB_RGBLayer, mem);
}

void CSegmentedWorldDoc::UpdateEngine()
{
	if (!m_bEditMode) {
		//CTerrainManager::GetTerrainManager().ReloadSurfaceTypes(false, false);
		return;
	}

	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);
	
	//GetIEditor()->GetHeightmap()->InitSectorGrid();
	
	GetIEditor()->GetHeightmap()->UpdateEngineTerrain(0, 0, GetIEditor()->GetHeightmap()->GetWidth(), GetIEditor()->GetHeightmap()->GetHeight(), true, true);
	//GetIEditor()->Get3DEngine()->GetITerrain()->Compile();
	
	//CRect rc(0, 0, GetIEditor()->GetHeightmap()->GetWidth(), GetIEditor()->GetHeightmap()->GetHeight());
	//GetIEditor()->GetHeightmap()->UpdateLayerTexture(rc);

	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(false);

	//GetIEditor()->GetHeightmap()->GetVegetationMap()->PlaceObjectsOnTerrain();
}

bool CSegmentedWorldDoc::GetModifiedSegments(const CRect &worldSegRect, std::map<int,TSegmentState> &mapSegStates, int &nLayerId, bool bCheckLock, bool bCheckMissing, bool bFromThread)
{
	if (bFromThread && m_bSuspendThread)
		return false;
	AUTO_LOCK_BIG(g_csBigLock);
	if (bFromThread && m_bSuspendThread)
		return false;
	return GetModifiedSegmentsEx(worldSegRect, mapSegStates, nLayerId, bCheckLock, bCheckMissing, bFromThread);
}
//------------------------------------------------------------------------------
bool CSegmentedWorldDoc::GetModifiedSegmentsEx(const CRect &worldSegRect, std::map<int,TSegmentState> &mapSegStates, int &nLayerId, bool bCheckLock, bool bCheckMissing, bool bFromThread)
{
	if (CSWManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return false;
	if (GetIEditor()->IsInGameMode() || gEnv->IsEditorGameMode())
		return false;
	if (GetISystem()->IsQuitting())
		return false;
	if (bFromThread && IsBusy())
		return false;

	if (bFromThread && m_bSuspendThread)
		return false;

	if (CSWManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return false;
	if (GetIEditor()->IsInGameMode() || gEnv->IsEditorGameMode())
		return false;
	if (GetISystem()->IsQuitting())
		return false;

	CObjectLayer* pCurrLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->GetCurrentLayer();
	CBaseObject *pObject = GetIEditor()->GetSelectedObject();
	if (pObject)
		pCurrLayer = pObject->GetLayer();
	if (pCurrLayer != NULL)
		nLayerId = m_layerInfoMgr.ObjectLayersInfo_GetID(pCurrLayer->GetGUID());

	for (int wy = worldSegRect.top; wy <= worldSegRect.bottom; wy++)
		for (int wx = worldSegRect.left; wx <= worldSegRect.right; wx++)
		{
			TSegmentData *psd = m_segs.GetSegmentData(TWorldCoords(wx, wy), false);
			if (psd == NULL)
				continue;
			int nSegmentId = MakeSegmentID(TWorldCoords(wx, wy));
			bool bInit = true;
			TMapSegStates::iterator it = mapSegStates.find(nSegmentId);
			if (it != mapSegStates.end())
				bInit = false;
			TSegmentState &sls = mapSegStates[nSegmentId];
			sls.nSegmentID = nSegmentId;

			if (bInit)
			{
				sls.m_SegmentDataStatus.m_nStatus = 0;
				sls.m_SegmentDataStatus.m_nLockedBy = 0;
			}

			if (psd->IsModified())
			{
				sls.m_SegmentDataStatus.m_nStatus |= TSegmentState::SWSLS_MODIFIED;
			}

			for (int i = 0; i < SDB_COUNT; i++)
			{
				if (IsBlockTypeNeeded((ESDBType)i))
				{
					AUTO_LOCK(g_cs);
					//if (psd->m_Data[i]->m_nHRIDBeforeModify != psd->m_Data[i]->m_nHRIDs[m_vt] && psd->m_Data[i]->m_nAppliedHRID != HRID_UNKNOWN)
					if (psd->m_Data[i]->IsVersionConflicting(m_vt)
						//&& !psd->m_Data[i]->AppliedVersionIs(HRID_UNKNOWN)
						&& !psd->m_Data[i]->StateEqualTo(Versioned::S_Unapplied)
						)
						sls.m_SegmentDataStatus.m_nStatus |= TSegmentState::SWSLS_OUT_OF_DATE;

				}
			}


			{
				AUTO_LOCK(g_cs);
				if (bCheckLock && psd->IsLocked())
				{
					//if (psd->IsLockedByMe())
					//	sls.m_SegmentDataStatus.m_nStatus = sls.m_SegmentDataStatus.m_nStatus|TSegmentState::SWSLS_LOCKED_BY_ME;
					//else
					//	sls.m_SegmentDataStatus.m_nStatus = sls.m_SegmentDataStatus.m_nStatus|TSegmentState::SWSLS_LOCKED_BY_OTHER;
					sls.m_SegmentDataStatus.m_nStatus |= psd->IsLockedByMe()? TSegmentState::SWSLS_LOCKED_BY_ME: TSegmentState::SWSLS_LOCKED_BY_OTHER;
					sls.m_SegmentDataStatus.m_nLockedBy = DataInspector::m_nLockedBy(psd);
				}
				if (bCheckMissing && psd->IsMissingInPersist())
					sls.m_SegmentDataStatus.m_nStatus |= TSegmentState::SWSLS_MISSING;
			}

			AUTO_LOCK(g_cs);
			for (TLayerMap::iterator it = psd->m_layersMap.begin(); it != psd->m_layersMap.end(); ++it)
			{
				TLayerData* pl = it->second;
				bool bInitLayer = bInit;
				if (!bInit)
				{
					std::map<int,TSegmentState::TLayerState>::iterator it2 = sls.m_mapLayersStatus.find(pl->m_nLayerID);
					if (it2 == sls.m_mapLayersStatus.end())
						bInitLayer = true;
				}

				if (bInitLayer)
				{
					sls.m_mapLayersStatus[pl->m_nLayerID].m_nStatus=0;
					sls.m_mapLayersStatus[pl->m_nLayerID].m_nLockedBy=0;
				}
				if (pl->IsModified())
					sls.m_mapLayersStatus[pl->m_nLayerID].m_nStatus |= TSegmentState::SWSLS_MODIFIED;
				
				if (pl->IsVersionConflicting(m_vt)
					//&& !pl->AppliedVersionIs(HRID_UNKNOWN)
					&& !pl->StateEqualTo(Versioned::S_Unapplied)
					)
					sls.m_mapLayersStatus[pl->m_nLayerID].m_nStatus |= TSegmentState::SWSLS_OUT_OF_DATE;

				if (bCheckLock && pl->IsLocked())
				{
					if (pl->IsLockedByMe())
						sls.m_mapLayersStatus[pl->m_nLayerID].m_nStatus |= TSegmentState::SWSLS_LOCKED_BY_ME;
					else
						sls.m_mapLayersStatus[pl->m_nLayerID].m_nStatus |= TSegmentState::SWSLS_LOCKED_BY_OTHER;
					sls.m_mapLayersStatus[pl->m_nLayerID].SyncLockInfo(*pl);
				}
				if (bCheckMissing && pl->IsMissingInPersist())
					sls.m_mapLayersStatus[pl->m_nLayerID].m_nStatus |= TSegmentState::SWSLS_MISSING;
			}
		}

		AUTO_LOCK(g_cs);
		std::vector<TObjectData*> arrObjsInRect;
		m_extobjs.FetchAll_InRect(arrObjsInRect,worldSegRect);
		for (std::vector<TObjectData*>::iterator it = arrObjsInRect.begin(); it != arrObjsInRect.end(); ++it) 
		{
			TObjectData *pObjData = *it;

			if (!pObjData->CheckFlags(EXTOBJFLAG_NOW))
				continue;
			TSegmentData *psd = m_segs.GetSegmentData(pObjData->GetSegCoords(), false);
			if (psd == NULL)
				continue;
			int nSegmentId = MakeSegmentID(pObjData->GetSegCoords());
			bool bInit = true;
			TMapSegStates::iterator it1 = mapSegStates.find(nSegmentId);
			if (it1 != mapSegStates.end())
				bInit = false;
			TSegmentState &sls = mapSegStates[nSegmentId];
			sls.nSegmentID = nSegmentId;

			if (bInit)
			{
				sls.m_SegmentDataStatus.m_nStatus = 0;
				sls.m_SegmentDataStatus.m_nLockedBy = 0;
			}

			int32 nObjLayerId = pObjData->GetLayerID();

			bool bInitLayer = bInit;
			if (!bInit)
			{
				std::map<int,TSegmentState::TLayerState>::iterator it2 = sls.m_mapLayersStatus.find(nObjLayerId);
				if (it2 == sls.m_mapLayersStatus.end())
					bInitLayer = true;
			}

			if (bInitLayer)
			{
				sls.m_mapLayersStatus[nObjLayerId].m_nStatus=0;
				sls.m_mapLayersStatus[nObjLayerId].m_nLockedBy=0;
			}
			
			if (pObjData->IsModified())
				sls.m_mapLayersStatus[nObjLayerId].m_nStatus |= TSegmentState::SWSLS_MODIFIED;

			//if (pObjData->m_nHRIDBeforeModify != pObjData->m_nHRIDs[m_vt]  && pObjData->m_nAppliedHRID != HRID_UNKNOWN)
			if (pObjData->IsVersionConflicting(m_vt)
				//&& !pObjData->AppliedVersionIs(HRID_UNKNOWN)
				&& !pObjData->StateEqualTo(Versioned::S_Unapplied)
				)
				sls.m_mapLayersStatus[nObjLayerId].m_nStatus |= TSegmentState::SWSLS_OUT_OF_DATE;

			TLayerData* pld = psd->GetLayer(pObjData->GetLayerGUID());
			if (bCheckLock && pld && pld->IsLocked())
			{
				if (pld->IsLockedByMe())
					sls.m_mapLayersStatus[pld->m_nLayerID].m_nStatus |= TSegmentState::SWSLS_LOCKED_BY_ME;
				else
					sls.m_mapLayersStatus[pld->m_nLayerID].m_nStatus |= TSegmentState::SWSLS_LOCKED_BY_OTHER;
				sls.m_mapLayersStatus[pld->m_nLayerID].SyncLockInfo(*pld);
			}
			if (bCheckMissing && pld && pld->IsMissingInPersist())
				sls.m_mapLayersStatus[pld->m_nLayerID].m_nStatus |= TSegmentState::SWSLS_MISSING;
			
		}


		return true;
}

void CSegmentedWorldDoc::UpdateOnIdle()
{
	bool bMinimapGenerating = m_mmgen.Step();
	if (bMinimapGenerating)
		return;							// skip all other updates if we are generating minimap.

	if (m_bAutoMoveTo)
		MoveToCam(true);

	Update(false);

	DrawBoxesEx();
}

bool CSegmentedWorldDoc::Update(bool bFromThread)
{
	if (GetISystem()->IsQuitting())
		return false;

	if (bFromThread && m_bSuspendThread)
		return false;

	bool bDelayedLoad;
	if (bFromThread)
		bDelayedLoad = /*false; */ m_iDelayedLoading == 2; // TODO: won't work until multithreaded object loading is supported
	else
		bDelayedLoad = m_iDelayedLoading == 1;

	bool bUpdateStatus = bFromThread;

	bool bReloadModified = !bFromThread;

	if (!bDelayedLoad && !bUpdateStatus && !bReloadModified)
		return false;

	CHeightmap *phm = GetIEditor()->GetHeightmap();
	if (!phm)
		return false;
	CViewport *pvp = GetIEditor()->GetViewManager()->GetGameViewport();
	if (!pvp)
		return false;

	float fNow = GetISystem()->GetITimer()->GetAsyncCurTime();
	if (!bFromThread && fNow < m_fNextContentLoadTime)
		return false;
	
	uint32 w = m_uiWidth;
	uint32 h = m_uiHeight;
	TLocalCoords lcBest(-1, -1);
	int iBestEval = 0x7FFFFFFF;
	DWORD dwBestFlags = WS_LOAD_ALL;

	Matrix34 mat = pvp->GetViewTM();
	if (!bFromThread && (
			m_matLastView.GetColumn0() != mat.GetColumn0() || 
			m_matLastView.GetColumn1() != mat.GetColumn1() || 
			m_matLastView.GetColumn2() != mat.GetColumn2())) {
		m_matLastView = mat;
		m_fNextContentLoadTime = fNow + 1.0f;
		return false;
	}

	Vec3 ptWorld = mat.GetTranslation();
	CPoint pthm = phm->WorldToHmap(ptWorld);
	TLocalCoords lcSeg(pthm.x / SEGMENT_SIZE_UNITS, pthm.y / SEGMENT_SIZE_UNITS);

	if (bUpdateStatus) {
		//AUTO_LOCK(g_cs);
		if (bFromThread && m_bSuspendThread)
			return false;
		WorldData_UpdateStatus();
		if (bFromThread && m_bSuspendThread)
			return false;
		//m_objs.ObjectData_UpdateStatus();
		if (bFromThread && m_bSuspendThread)
			return false;
		//m_bNeedApplyObjectData = m_objs.ObjectData_NeedsToBeApplied();
		if (bFromThread && m_bSuspendThread)
			return false;
	}
	
	TLocalCoords lc;
	bool bNeedLoadSegments = false;
	//TWorldCoordsArray arrSegmentsToLoad;

	for (lc.ly = 0; lc.ly < h; ++lc.ly) {
		for (lc.lx = 0; lc.lx < w; ++lc.lx) {
			TSegmentData *pd = m_segs.GetSegmentData(lc, true);
			if (bUpdateStatus) {
				if (bFromThread && m_bSuspendThread)
					return false;
				{
					//AUTO_LOCK(g_cs);
					pd = m_segs.GetSegmentData(lc, true); // need to get segment data again because in main thread after ::MoveTo(..) , flushData() is called and sd can be deleted
					pd->UpdateStatus();
					if (bFromThread && m_bSuspendThread)
						return false;
					pd->UpdateLayersStatus();
					if (bFromThread && m_bSuspendThread)
						return false;
					pd->m_bNeedApplyLayers = pd->NeedsToBeAppliedLayers();
					if (bFromThread && m_bSuspendThread)
						return false;
				}
				if (bFromThread)
					Sleep(10);
			}

			//reload data blocks
			if (bFromThread && m_bSuspendThread)
				return false;

			if (bFromThread && pd->NeedsReload()) {
				if (bFromThread && m_bSuspendThread)
					return false;
				//AUTO_LOCK(g_cs);
				if (bFromThread && m_bSuspendThread)
					return false;
				char chLog[256]; sprintf(chLog, "Background reloading (%d,%d)\n", pd->m_wc.wx, pd->m_wc.wy);
				SWLog(chLog);
				pd->Reload();
				
			}

			if (bFromThread && m_bSuspendThread)
				return false;

			//reload layers
			if (bFromThread && pd->NeedsReloadLayers()) {
				if (bFromThread && m_bSuspendThread)
					return false;
				//AUTO_LOCK(g_cs);
				//if (bFromThread && m_bSuspendThread)
				//	return false;
				char chLog[256]; sprintf(chLog, "Background reloading Layers(%d,%d)\n", pd->m_wc.wx, pd->m_wc.wy);
				SWLog(chLog);
				pd->ReloadLayers();
			}

			if (bFromThread && m_bSuspendThread)
				return false;
			if (bReloadModified && (pd->NeedToBeFastApplied() || pd->m_bNeedApplyLayers)/*dwLoaded != WS_LOAD_ALL*/) 
			{
				bNeedLoadSegments = true;
			}
		}
	}

	if (bReloadModified)
		WorlData_ReloadIfNeeded();
	

	//ObjectData
	if (bFromThread && m_extobjs.ObjectData_NeedsReload())
	{
		if (m_bSuspendThread)
			return false;
		//AUTO_LOCK(g_cs);
		//if (bFromThread && m_bSuspendThread)
		//	return false;
		m_extobjs.ObjectData_Reload();
	}

	if (bReloadModified)
	{
		//bool bNeedLoadSegments = arrSegmentsToLoad.size() != 0;
		bool bNeedApplyObjectData = m_bNeedApplyObjectData;
		if (bNeedLoadSegments || bNeedApplyObjectData)
		{
			AUTO_LOCK(g_cs);
			CUndoSuspend suspendUndo;
			m_bNeedApplyObjectData = false;
			CBaseObjectsArray arrObjects;
			
			//if (bNeedLoadSegments)
			//	GetObjectsFiltered(arrObjects);
			
			//delete needed objects
			TWorldCoordsArray arrSegmentsToLoad;
			PreLoadObjects(arrSegmentsToLoad, arrObjects, true);

			if (arrSegmentsToLoad.size())
			{	
				CObjectArchive oa(GetIEditor()->GetObjectManager(), 0, true );
				//load segments
				for (int i = 0; i < arrSegmentsToLoad.size(); i++)
				{
					TSegmentData *pd = m_segs.GetSegmentData(arrSegmentsToLoad[i], true);
					if (pd)
						pd->m_bNeedApplyLayers = false;
					char chLog[256]; sprintf(chLog, "Background applying (%d,%d)\n", arrSegmentsToLoad[i].wx, arrSegmentsToLoad[i].wy);
					SWLog(chLog);
					m_segs.LoadSegment(arrSegmentsToLoad[i], arrObjects, oa, (WS_LOAD_ALL /*& ~dwBestFlags*/) | WS_FINALIZE);
				}
				OnMayUpdateLevelPak();
			}

			// load external objects
			if (bNeedApplyObjectData)
				m_extobjs.ObjectData_Load(false);
		}
	}

	//HOT Version Auto Save
	if (bReloadModified)
	{
		DoAutoSave();
	}

	//if (bReloadModified && ObjectData_NeedsToBeApplied())
	//{
	//	AUTO_LOCK(g_cs);
	//	if (bFromThread && m_bSuspendThread)
	//		return false;
	//	ObjectData_Load(true);
	//}

	//fNow = GetISystem()->GetITimer()->GetAsyncCurTime();
	//m_fNextContentLoadTime = fNow + 0.1f;

	return !!dwBestFlags;
}


inline void DrawTerrainLine(const Vec3 &pt1, const Vec3 &pt2, const ColorB &clr)
{
	IRenderAuxGeom *pag = GetIEditor()->GetRenderer()->GetIRenderAuxGeom();
	float step = 8; //GetIEditor()->GetHeightmap()->GetUnitSize()
	Vec3 v = (pt2 - pt1);
	float len = max(fabs(v.x), fabs(v.y)) / step;
	v /= len;
	float dz = 0.5f;
	Vec3 ptPrev = pt1;
	ptPrev.z = max(GetIEditor()->Get3DEngine()->GetTerrainElevation(ptPrev.x, ptPrev.y),GetIEditor()->GetHeightmap()->GetWaterLevel()) + dz;
	for (float t = 0; t < len; ++t) {
		Vec3 pt = pt1 + v * t;
		pt.z = max(GetIEditor()->Get3DEngine()->GetTerrainElevation(ptPrev.x, ptPrev.y),GetIEditor()->GetHeightmap()->GetWaterLevel()) + dz;
		pag->DrawLine(ptPrev, clr, pt, clr, 2);
		ptPrev = pt;
	}
}

void CSegmentedWorldDoc::Commit()
{
	AUTO_LOCK_BIG(g_csBigLock);
	if (CSWManager::Get().IsOfflineMode())
	{
		std::vector<TWorldData*> wdJustLocked;
		std::vector<TSegmentData*> sdJustLocked; 
		std::vector<TLayerData*> ldJustLocked;

		bool bRes = TryLockModified(wdJustLocked, sdJustLocked, ldJustLocked);
		if (!bRes)
		{
			TryUnLockJustLocked(wdJustLocked, sdJustLocked, ldJustLocked);
			return;
		}
	}
	CSWModeFakeAutoGuard swFakeMode(1);

#ifdef SW_NO_BUILTIN_BRANCH_MANAGING
	// just commit to current branch
	m_segs.SubmitAllSegments();
#else
	if (!CommitToOfficial( dlg.GetDescription()))
	{
		gEnv->pLog->LogToConsole("Commit failed!");
		return;
	}
#endif
//	AUTO_LOCK(g_cs);
	//if (!IsInSelectMode())
	//{
	//	//WorldData_Commit(nVer);

	//	//Unlock and equalize SegmentData version types HRIDs OV=CV
	//	//SegmentsCommit(nVer);
	//	

	//	////Unlock ObjectLayers which are not modified (unlock if cv=ov in both ObjectsStatus and LayersStatus)
	//	//if (g_db->GetCachedConn())
	//	//{
	//	//	IDBPreparedStatement *pStatement;
	//	//	char chStatement[64];
	//	//	pStatement = g_db->GetCachedConn()->GetPreparedStatement(SD_OBJECT_LAYERS_UNLOCK_NO_MODIFIED);
	//	//	sprintf(chStatement, "SD_OBJECT_LAYERS_UNLOCK_NO_MODIFIED");

	//	//	if (!pStatement)
	//	//	{	
	//	//		char chLog[256]; sprintf(chLog, "Cannot create %s query.\n", chStatement);
	//	//		SWLog(chLog);
	//	//		assert(0);
	//	//		//return
	//	//	}
	//	//	else if (!DBAPIHelper::Exec(pStatement, m_pVersionControl->GetUserID()))
	//	//	{
	//	//		char chLog[256]; sprintf(chLog, "%s query failed.\n", chStatement);
	//	//		SWLog(chLog);
	//	//		assert(0);
	//	//		//return;
	//	//	}
	//	//}

	//	//External Objects
	//	//ObjectData_Commit(nVer);

	//	//Object Layers
	//	//ObjectLayersCommit(nVer);

	//	GetVersionControl()->Commit(nVer);
	//}

	OnUnlock();

	return;
	//!!! not implemented for file version / source control
	m_segs.UnlockSegment(TLocalCoords(0, 0), TLocalCoords(m_uiWidth - 1, m_uiHeight - 1));
	return;
}

//refresh opened windows
void CSegmentedWorldDoc::OnUnlock()
{
	AUTO_LOCK(g_cs);

	uint32 w = m_uiWidth;
	uint32 h = m_uiHeight;
	TLocalCoords lc;
	for (lc.ly = 0; lc.ly < h; ++lc.ly)
	{
		for (lc.lx = 0; lc.lx < w; ++lc.lx)
		{
			TSegmentData *pd = m_segs.GetSegmentData(lc, true);
			pd->UpdateLayersStatus();
		}
	}

	GetIEditor()->Notify( eNotify_OnSWLockUnlock );

}
//
bool CSegmentedWorldDoc::NeedToShowLayerDataStatus()
{
	if(s_nRollUpCtrlID == ROLLUP_OBJECTS || s_nRollUpCtrlID == ROLLUP_LAYERS)
	{
		if(GetIEditor()->GetObjectManager()->GetLayersManager()->GetCurrentLayer())
			return true;
	}
	return false;
}
//--------------------------------------------------------------------
void CSegmentedWorldDoc::DrawBoxesEx()
{
	int nDebugDrawMode = CSWManager::Get().GetDebugDraw();
	if (!nDebugDrawMode)
		return;

	CHeightmap *phm = GetIEditor()->GetHeightmap();
	if (!phm)
		return;
	AUTO_LOCK(g_cs);

	IRenderAuxGeom *pag = GetIEditor()->GetRenderer()->GetIRenderAuxGeom();

	static const ColorB slsColors[] = {
		ColorB(255,255,0,255),			// 0 - SSClr_Default_Yellow - not locked
		ColorB(255,0,255,255),			// 1 - SSClr_Purple, //Conflict - modified but not locked by me
		ColorB(255,0,0,255),				// 2 - SSClr_Red, //selected is locked by other
		ColorB(255,128,64,255),			// 3 - SSClr_Orange, //something is locked by other, nothing is locked by us
		ColorB(0,255,255,255),			// 4 - SSClr_Cyan, //something locked by other, something locked by us
		ColorB(0,0,255,255),				// 5 - SSClr_Blue, //selected locked and modified by me
		ColorB(0,255,0,255),				// 6 - SSClr_Green //something locked by us
		ColorB(128,128,128,255)			// 7 - SSClr_Gray //missing
	};
	
	

	uint32 w = phm->GetWidth() / SEGMENT_SIZE_UNITS;
	uint32 h = phm->GetHeight() / SEGMENT_SIZE_UNITS;

	CCamera *pCamera = &GetISystem()->GetViewCamera();
	CObjectLayer *pCurrLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->GetCurrentLayer();
	CBaseObject *pObject = GetIEditor()->GetSelectedObject();
	if (pObject)
		pCurrLayer = pObject->GetLayer();

	bool bNeedToShowLayerDataStatus = NeedToShowLayerDataStatus();
	int nSelectedLock = bNeedToShowLayerDataStatus ? CSWMiniMapUpdater::LOCK_STATUS_TYPE_OBJECTLAYERS : CSWMiniMapUpdater::LOCK_STATUS_TYPE_SEGMENTS;
	//collect information
	TWorldCoords wcMin(TLocalCoords(0,0));
	TWorldCoords wcMax(TLocalCoords(h-1,w-1));
	CRect worldSegRect(wcMin.wx, wcMin.wy, wcMax.wx, wcMax.wy);

	std::map<int,TSegmentState> mapSegStates;
	int nLayerID;

	GetModifiedSegmentsEx(worldSegRect, mapSegStates, nLayerID, true, (nDebugDrawMode >= 3), false);

	bool bConflictFound = false;
	TLocalCoords lc;
	for (lc.ly = 0; lc.ly < h; ++lc.ly) 
	{
		for (lc.lx = 0; lc.lx < w; ++lc.lx) 
		{
			AABB bb = CalcWorldBounds(lc);

			if(!pCamera->IsAABBVisible_F(bb))
				continue;

			TSegmentData *pd = m_segs.GetSegmentData(lc, false);
			if (!pd)
				continue;

			int nClrIndex = TSegStsClr::SSClr_Gray;
			int nSegmentID = MakeSegmentID(pd->m_wc);
			std::map<int,TSegmentState>::iterator it = mapSegStates.find(nSegmentID);
			if (it != mapSegStates.end())
			{
				TSegmentState &sls = it->second;
				nClrIndex = sls.GetSegStsClr(nSelectedLock, nLayerID);
			}
			else
				nClrIndex = TSegStsClr::SSClr_Gray;
//#ifdef USE_NATIVE_VC
			if (nDebugDrawMode < 2 && (nClrIndex == TSegStsClr::SSClr_Default || nClrIndex == TSegStsClr::SSClr_Gray))
				continue;
//#else
//			nClrIndex = TSegStsClr::SSClr_Default;
//#endif
			//draw detailed lock info
			if (nDebugDrawMode >= 3) 
			{
				char chTxt[2048];
				char szLockedByBuf[ISWVersionControl::MAX_USERNAME_CBSIZE];
				char *pcTxt = chTxt;
				Vec3 vCenter = bb.GetCenter();

				CCamera camera = GetISystem()->GetIRenderer()->GetCamera();

				Vec3 vCamPos = camera.GetPosition();

				vCenter.z = GetIEditor()->Get3DEngine()->GetTerrainElevation(vCenter.x, vCenter.y);
				Vec3 vDist = vCamPos - vCenter;
				f32 fDist = vDist.GetLength();
				f32 fScaleFactor = 512/fDist;
				if(fScaleFactor > 1)
					fScaleFactor = 1;

				
				Vec3 vTxtDeltaPos = camera.GetViewdir();
				vTxtDeltaPos.z = 0;//vCenter.z;
				vTxtDeltaPos.Normalize();
				vTxtDeltaPos.SetLength(25.0f*fScaleFactor);

				int nLockedStuff = 0;
				{
					AUTO_LOCK(g_cs);
					for(TLayerMap::iterator it = pd->m_layersMap.begin(); it != pd->m_layersMap.end(); ++it)
					{
						TLayerData *pl = it->second;
						if (pl->IsLocked())
							nLockedStuff++;
					}
				}
				if(pd->IsLocked())
					nLockedStuff++;

				Vec3 vTxtCurrPos = vCenter + (nLockedStuff/2) * vTxtDeltaPos;

				if (pd->IsLocked())
				{
					sprintf(chTxt, "Segment (%d,%d) - ", pd->m_wc.wx, pd->m_wc.wy);
					sprintf(chTxt + strlen(chTxt), " %s(%d)"
						,m_pVersionControl->GetLockedBy(*pd,szLockedByBuf,ISWVersionControl::MAX_USERNAME_CBSIZE,"<N/A>")
						, DataInspector::m_nLockedBy(pd));
					//vTxtCurrPos.z = GetIEditor()->Get3DEngine()->GetTerrainElevation(vTxtCurrPos.x, vTxtCurrPos.y);
					GetIEditor()->GetRenderer()->DrawLabel(vTxtCurrPos, 1.8f*fScaleFactor, chTxt);
					vTxtCurrPos = vTxtCurrPos - vTxtDeltaPos;
				}

				AUTO_LOCK(g_cs);
				for(TLayerMap::iterator it = pd->m_layersMap.begin(); it != pd->m_layersMap.end(); ++it)
				{
					TLayerData *pl = it->second;
					if (pl->IsLocked())
					{
						sprintf(chTxt, "Layer: %d(%s) - (%d,%d)", pl->m_nLayerID, pl->m_strName.c_str(), pd->m_wc.wx, pd->m_wc.wy);
						sprintf(chTxt + strlen(chTxt), " %s(%d)\n"
							, m_pVersionControl->GetLockedBy(*pl,szLockedByBuf,ISWVersionControl::MAX_USERNAME_CBSIZE,"<N/A>")
							, DataInspector::m_nLockedBy(pl) );
						//vTxtCurrPos.z = GetIEditor()->Get3DEngine()->GetTerrainElevation(vTxtCurrPos.x, vTxtCurrPos.y);
						GetIEditor()->GetRenderer()->DrawLabel(vTxtCurrPos, 1.7f*fScaleFactor, chTxt);
						vTxtCurrPos = vTxtCurrPos - vTxtDeltaPos;
					}
				}
				
			} //~ if (m_nDrawBoxes >= 3)


			if (nClrIndex == TSegStsClr::SSClr_Purple)
			{
				bConflictFound = true;
				float fTime = GetISystem()->GetITimer()->GetAsyncCurTime();
				float fBlinkRate = 0.25f;
				if (fmod(fTime, fBlinkRate * 2) < fBlinkRate)
					nClrIndex = TSegStsClr::SSClr_Blue;
			}


			float dx = 0.1f;
			bb.Expand(Vec3(-dx, -dx, 0));

			Vec3 pt00 = bb.min;
			Vec3 pt11 = bb.max;
			Vec3 pt01(pt00.x, pt11.y, 0);
			Vec3 pt10(pt11.x, pt00.y, 0);

			DrawTerrainLine(pt00, pt01, slsColors[nClrIndex]);
			DrawTerrainLine(pt01, pt11, slsColors[nClrIndex]);
			DrawTerrainLine(pt11, pt10, slsColors[nClrIndex]);
			DrawTerrainLine(pt10, pt00, slsColors[nClrIndex]);
		}
	}

	if (nDebugDrawMode > 0)
	{
		float fTime = GetISystem()->GetITimer()->GetAsyncCurTime();
		float fBlinkRate = 0.25f;
		bool bBlink = (fmod(fTime, fBlinkRate * 2) < fBlinkRate);
		
		if (!bConflictFound && bBlink) //check all for conflict
		{	
			for (int ix = wcMin.wx; ix <= wcMax.wx; ix++)
			{
				for (int iy = wcMin.wy; iy <= wcMax.wy; iy++)
				{
					int nSegmentID = MakeSegmentID(TWorldCoords(ix, iy));
					std::map<int,TSegmentState>::iterator it = mapSegStates.find(nSegmentID);
					if (it != mapSegStates.end())
					{
						TSegmentState &sls = it->second;
						if (sls.GetSegStsClr(nSelectedLock, nLayerID) == TSegStsClr::SSClr_Purple)
							bConflictFound = true;
					}

					if (bConflictFound)
						break;
				}
				if (bConflictFound)
					break;
			}

			if (!bConflictFound)
			{
				for (int i = 0; i < WDB_COUNT; i++)
				{
					if (!IsWorldDataBlockNeeded(i))
						continue;
					//if (m_WorldData[i].m_nHRIDBeforeModify != m_WorldData[i].m_nHRIDs[m_vt])
					if (m_WorldData[i].IsVersionConflicting(m_vt))
					{
						bConflictFound = true;
						break;
					}

					if (!bConflictFound && CSWManager::Get().IsOfflineMode() && 
							m_WorldData[i].IsLocked()&& 
							!m_WorldData[i].IsLockedByMe()&& 
							m_WorldData[i].IsModified())
					{
						bConflictFound = true;
					}

					if (!bConflictFound && !CSWManager::Get().IsOfflineMode() &&
							!m_WorldData[i].IsLockedByMe()&& 
							m_WorldData[i].IsModified())
					{
						bConflictFound = true;
					}
				}
			}

		}

		float posY = 0.f;
		if (bConflictFound)
		{
			if (bBlink)
			{
				char Text[1024];
				sprintf(Text,"CONFLICTED");
				SDrawTextInfo ti;
				ti.flags = eDrawText_FixedSize | eDrawText_2D | eDrawText_Monospace;
				ti.xscale = ti.yscale = (1.5f);
				ti.color[0] = 1;
				ti.color[1] = 0;
				ti.color[2] = 1;
				ti.color[3] = 1;
				GetIEditor()->GetRenderer()->DrawTextQueued( Vec3(0.0f,posY,1.0f),ti,Text );
			}
			

			posY = 20.f;
		}
		if (CSWManager::Get().IsOfflineMode())
		{
			char Text[1024];
			sprintf(Text,"<OFFLINE MODE>");
			SDrawTextInfo ti;
			ti.flags = eDrawText_FixedSize | eDrawText_2D | eDrawText_Monospace;
			ti.xscale = ti.yscale = (1.5f);
			ti.color[0] = 1;
			ti.color[1] = 0;
			ti.color[2] = 0;
			ti.color[3] = 1;
			GetIEditor()->GetRenderer()->DrawTextQueued( Vec3(0.0f,posY,1.0f),ti,Text );

			
		}
	}
}
//--------------------------------------------------------------------
void CSegmentedWorldDoc::DrawBoxes()
{
	int nDrawBoxes = CSWManager::Get().GetDebugDraw();
	if (!nDrawBoxes)
		return;

	CHeightmap *phm = GetIEditor()->GetHeightmap();
	if (!phm)
		return;
	
	IRenderAuxGeom *pag = GetIEditor()->GetRenderer()->GetIRenderAuxGeom();
	ColorB clrGray(128, 128, 128, 255);
	ColorB clrRed(255, 0, 0, 255);
	ColorB clrGreen(0, 255, 0, 255);
	ColorB clrYellow(255, 255, 0, 255);
	ColorB clrBlue(0, 0, 255, 255);
	
	uint32 w = phm->GetWidth() / SEGMENT_SIZE_UNITS;
	uint32 h = phm->GetHeight() / SEGMENT_SIZE_UNITS;

	CCamera *pCamera = &GetISystem()->GetViewCamera();
	CObjectLayer *pCurrLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->GetCurrentLayer();
	CBaseObject *pObject = GetIEditor()->GetSelectedObject();
	if (pObject)
		pCurrLayer = pObject->GetLayer();

	bool bNeedToShowLayerDataStatus = NeedToShowLayerDataStatus();
	TLocalCoords lc;
	for (lc.ly = 0; lc.ly < h; ++lc.ly) 
	{
		for (lc.lx = 0; lc.lx < w; ++lc.lx) 
		{
			AABB bb = CalcWorldBounds(lc);

			if(!pCamera->IsAABBVisible_F(bb))
				continue;

			TSegmentData *pd = m_segs.GetSegmentData(lc, false);
			if (!pd)
				continue;
			bool bMissing;
			int nLockedBy;
			bool bModified;
			if (bNeedToShowLayerDataStatus)
			{
				TLayerData *pl = pd->GetLayer(pCurrLayer->GetGUID(), pCurrLayer->GetName());
				if (!pl)
				{
					if (nDrawBoxes >= 3)
					{
						bMissing = true;
						nLockedBy = 0;
						bModified = false;
					}
					else
						continue;
				}
				else
				{
					bMissing = pl->IsMissingInPersist();
					nLockedBy = DataInspector::m_nLockedBy(pl);
					bModified = pl->IsModified();
				}
			}
			else
			{
				bMissing = pd->IsMissingInPersist();
				nLockedBy = DataInspector::m_nLockedBy(pd);
				bModified = pd->IsModified();
			}

			//if ((ws.GetLockStatus() == WS_LOCKED_LOCAL) && (ws.GetLoadedFlags() == WS_LOAD_ALL))
			//  continue;

			ColorB clr1;
			if (bMissing /*|| (ws.GetLoadedFlags() != WS_LOAD_ALL)*/)
				clr1 = clrGray;
			else if (nLockedBy == m_pVersionControl->GetUserID())
				clr1 = clrGreen;
			else if (nLockedBy != 0)
				clr1 = clrRed;
			else 
			{
				if (nDrawBoxes < 2 && !bModified)
					continue;
				clr1 = clrYellow;
			}

			if (nDrawBoxes >= 3) 
			{
				char chTxt[2048];
				char szBufLockedBy[ISWVersionControl::MAX_USERNAME_CBSIZE];
				char *pcTxt = chTxt;
				Vec3 vCenter = bb.GetCenter();

				if (bNeedToShowLayerDataStatus)
				{
					if (pd->m_layersMap.size() == 0)
					{
						pcTxt += sprintf(pcTxt, "(%d,%d)", pd->m_wc.wx, pd->m_wc.wy);
						vCenter.z = GetIEditor()->Get3DEngine()->GetTerrainElevation(vCenter.x, vCenter.y);
						GetIEditor()->GetRenderer()->DrawLabel(vCenter, 2, chTxt);
					}
					else
					{
						CCamera camera = GetISystem()->GetIRenderer()->GetCamera();
						Vec3 vTxtDeltaPos = camera.GetViewdir();
						vTxtDeltaPos.z = 0;
						vTxtDeltaPos.Normalize();
						vTxtDeltaPos.SetLength(50.0f);
						Vec3 vTxtCurrPos = vCenter - (pd->m_layersMap.size()/2) * vTxtDeltaPos;

						for(TLayerMap::iterator it = pd->m_layersMap.begin(); it != pd->m_layersMap.end(); ++it)
						{
							TLayerData *pl = it->second;
							sprintf(chTxt, "Layer: %d - (%d,%d)", pl->m_nLayerID, pd->m_wc.wx, pd->m_wc.wy);
							if (pl->IsLocked())
								sprintf(chTxt + strlen(chTxt), " %s(%d)\n"
									, m_pVersionControl->GetLockedBy(*pl,szBufLockedBy,sizeof(szBufLockedBy),"<N/A>")
									, DataInspector::m_nLockedBy(pl));
							
							vTxtCurrPos.z = GetIEditor()->Get3DEngine()->GetTerrainElevation(vTxtCurrPos.x, vTxtCurrPos.y);
							GetIEditor()->GetRenderer()->DrawLabel(vTxtCurrPos, 1.5f, chTxt);

							vTxtCurrPos = vTxtCurrPos + vTxtDeltaPos;
						}
					}
				}
				else
				{
					sprintf(chTxt, "(%d,%d)", pd->m_wc.wx, pd->m_wc.wy);
					if (nLockedBy)
						sprintf(chTxt + strlen(chTxt), " %s(%d)"
							//, g_db->GetUserName(nLockedBy)
							, m_pVersionControl->GetLockedBy(*pd,szBufLockedBy,sizeof(szBufLockedBy),"<N/A>")
							, nLockedBy);
					vCenter.z = GetIEditor()->Get3DEngine()->GetTerrainElevation(vCenter.x, vCenter.y);
					GetIEditor()->GetRenderer()->DrawLabel(vCenter, 2, chTxt);
				}
				
			}

			if (bModified) 
			{
				if (nLockedBy == m_pVersionControl->GetUserID())
					clr1 = clrBlue;
				else 
				{
					float fTime = GetISystem()->GetITimer()->GetAsyncCurTime();
					float fBlinkRate = 0.25f;
					if (fmod(fTime, fBlinkRate * 2) < fBlinkRate)
						clr1 = clrBlue;
				}
			}

			float dx = 0.1f;
			bb.Expand(Vec3(-dx, -dx, 0));

			Vec3 pt00 = bb.min;
			Vec3 pt11 = bb.max;
			Vec3 pt01(pt00.x, pt11.y, 0);
			Vec3 pt10(pt11.x, pt00.y, 0);

			DrawTerrainLine(pt00, pt01, clr1);
			DrawTerrainLine(pt01, pt11, clr1);
			DrawTerrainLine(pt11, pt10, clr1);
			DrawTerrainLine(pt10, pt00, clr1);

			//if (ws.GetLockStatus() <= 0) {
			//  float step = 8;
			//  for (float y = pt00.y + step; y <= pt11.y - step; y += step)
			//    DrawTerrainLine(Vec3(pt00.x, y, 0), Vec3(pt11.x, y, 0), clr1);
			//  for (float x = pt00.x + step; x <= pt11.x - step; x += step)
			//    DrawTerrainLine(Vec3(x, pt00.y, 0), Vec3(x, pt11.y, 0), clr1);
			//}

		 // ColorB clr2 = clr1; clr2.a = 128;
		 // clr1.a = 32;
			////pag->DrawAABB(bb, false, clr, eBBD_Faceted);
		 // float zMin = 0, zMax = 128.0f;

		 // if (sx <= 0 || GetSegment(sx - 1, sy).GetLockStatus() != ws.GetLockStatus())
		 // {
		 //   Vec3 v0(bb.min.x, bb.min.y, zMin);
		 //   Vec3 v1(bb.min.x, bb.min.y, zMax);
		 //   Vec3 v2(bb.max.x, bb.min.y, zMax);
		 //   Vec3 v3(bb.max.x, bb.min.y, zMin);
		 //   pag->DrawTriangle(v0, clr1, v1, clr2, v2, clr2);
		 //   //pag->DrawTriangle(v0, clr, v2, clr2, v1, clr2);
		 //   pag->DrawTriangle(v2, clr2, v3, clr1, v0, clr1);
		 //   //pag->DrawTriangle(v2, clr2, v0, clr1, v3, clr1);
		 // }

		 // if (sy <= 0 || GetSegment(sx, sy - 1).GetLockStatus() != ws.GetLockStatus())
		 // {
		 //   Vec3 v0(bb.min.x, bb.min.y, zMin);
		 //   Vec3 v1(bb.min.x, bb.min.y, zMax);
		 //   Vec3 v2(bb.min.x, bb.max.y, zMax);
		 //   Vec3 v3(bb.min.x, bb.max.y, zMin);
		 //   //pag->DrawTriangle(v0, clr1, v1, clr2, v2, clr2);
		 //   pag->DrawTriangle(v0, clr1, v2, clr2, v1, clr2);
		 //   //pag->DrawTriangle(v2, clr2, v3, clr1, v0, clr1);
		 //   pag->DrawTriangle(v2, clr2, v0, clr1, v3, clr1);
		 // }

		 // if (sx + 1 >= w || GetSegment(sx + 1, sy).GetLockStatus() != ws.GetLockStatus())
		 // {
		 //   Vec3 v0(bb.min.x, bb.max.y, zMin);
		 //   Vec3 v1(bb.min.x, bb.max.y, zMax);
		 //   Vec3 v2(bb.max.x, bb.max.y, zMax);
		 //   Vec3 v3(bb.max.x, bb.max.y, zMin);
		 //   //pag->DrawTriangle(v0, clr1, v1, clr2, v2, clr2);
		 //   pag->DrawTriangle(v0, clr1, v2, clr2, v1, clr2);
		 //   //pag->DrawTriangle(v2, clr2, v3, clr1, v0, clr1);
		 //   pag->DrawTriangle(v2, clr2, v0, clr1, v3, clr1);
		 // }

		 // if (sy + 1 >= h || GetSegment(sx, sy + 1).GetLockStatus() != ws.GetLockStatus())
		 // {
		 //   Vec3 v0(bb.max.x, bb.min.y, zMin);
		 //   Vec3 v1(bb.max.x, bb.min.y, zMax);
		 //   Vec3 v2(bb.max.x, bb.max.y, zMax);
		 //   Vec3 v3(bb.max.x, bb.max.y, zMin);
		 //   pag->DrawTriangle(v0, clr1, v1, clr2, v2, clr2);
		 //   //pag->DrawTriangle(v0, clr1, v2, clr2, v1, clr2);
		 //   pag->DrawTriangle(v2, clr2, v3, clr1, v0, clr1);
		 //   //pag->DrawTriangle(v2, clr2, v0, clr1, v3, clr1);
		 // }

		}
	}
}

inline bool CSegmentedWorldDoc::ShouldMove(const Vec3 &ptWorld, UINT w, UINT h)
{
	float ss = SEGMENT_SIZE_UNITS * GetIEditor()->GetHeightmap()->GetUnitSize();
	int GUARD = m_uiWidth / 4;
	if (ptWorld.x < ss * GUARD)
		return true;
	if (ptWorld.y < ss * GUARD)
		return true;
	if (ptWorld.x > (w - GUARD) * ss)
		return true;
	if (ptWorld.y > (h - GUARD) * ss)
		return true;
	return false;
}

void CSegmentedWorldDoc::MoveToCam(bool bAuto)
{
	AUTO_LOCK_BIG(g_csBigLock);
	CViewport *pvp = GetIEditor()->GetViewManager()->GetGameViewport();
	if (!pvp)
		return;
	Matrix34 mat = pvp->GetViewTM();
	Vec3 ptWorld = mat.GetTranslation();
	
	if (bAuto && !ShouldMove(ptWorld, m_uiWidth, m_uiHeight))
		return;

	CPoint ptHM = GetIEditor()->GetHeightmap()->WorldToHmap(ptWorld);
	TLocalCoords lcOfs(ptHM.x / SEGMENT_SIZE_UNITS, ptHM.y / SEGMENT_SIZE_UNITS);
	if (ptWorld.x < 0) lcOfs.lx--;
	if (ptWorld.y < 0) lcOfs.ly--;
	lcOfs.lx -= m_uiWidth / 2;
	lcOfs.ly -= m_uiHeight / 2;

	if (!MoveTo(m_wcOfs.wx + lcOfs.lx, m_wcOfs.wy + lcOfs.ly, m_uiWidth, m_uiHeight, bAuto))
		return;

	CPoint ofsHM(lcOfs.lx * SEGMENT_SIZE_UNITS, lcOfs.ly * SEGMENT_SIZE_UNITS);
	Vec3 ofsWorld = GetIEditor()->GetHeightmap()->HmapToWorld(ofsHM);
	mat.SetTranslation(ptWorld - ofsWorld);
	pvp->SetViewTM(mat);
}

bool GenPointListsFromIntersect(CRect const& rcOld, CRect const& rcNew, std::vector<CPoint>* pLeaving = NULL, std::vector<CPoint>* pEntering = NULL, std::vector<CPoint>* pKept = NULL)
{
	CRect rcOverlap;
	bool bOverlapped = rcOverlap.IntersectRect(rcOld, rcNew);

	if (pLeaving)
	{
		CPoint pt;
		for (pt.y = rcOld.top; pt.y < rcOld.bottom; ++pt.y)
		{
			for (pt.x = rcOld.left; pt.x < rcOld.right; ++pt.x)
			{
				if (!rcOverlap.PtInRect(pt))
					pLeaving->push_back(pt);
			}
		}
	}

	if (pEntering)
	{
		CPoint pt;
		for (pt.y = rcNew.top; pt.y < rcNew.bottom; ++pt.y)
		{
			for (pt.x = rcNew.left; pt.x < rcNew.right; ++pt.x)
			{
				if (!rcOverlap.PtInRect(pt))
					pEntering->push_back(pt);
			}
		}
	}

	if (pKept)
	{
		CPoint pt;
		for (pt.y = rcOverlap.top; pt.y < rcOverlap.bottom; ++pt.y)
		{
			for (pt.x = rcOverlap.left; pt.x < rcOverlap.right; ++pt.x)
			{
				pKept->push_back(pt);
			}
		}
	}

	return bOverlapped;
}

inline bool InRect(CRect &rcRef, CRect &rcTest)
{
	return rcTest.left >= rcRef.left && rcTest.right <= rcRef.right && rcTest.top >= rcRef.top && rcTest.bottom <= rcRef.bottom;
}

void CSegmentedWorldDoc::UpdatePackStatus(int iNewOffsetX, int iNewOffsetY, UINT nNewWidth, UINT nNewHeight)
{
	if (m_bEditMode && s_bLevelMultiPack)
	{
		TLocalCoords lc;
		CString sSegmentPath;
		CString sSegmentPakFileName;
		CString sFileBlockPath;
		GetIEditor()->Get3DEngine()->CloseTerrainTextureFile();

		for (lc.ly = 0; lc.ly < m_uiHeight; ++lc.ly) {
			for (lc.lx = 0; lc.lx < m_uiWidth; ++lc.lx) {
				TWorldCoords wc(lc);
				m_pPersistance->GetPakPath(sSegmentPakFileName, sSegmentPath, sFileBlockPath, wc, s_bLevelMultiPack, false);

				if(lc.lx >= iNewOffsetX && lc.lx < nNewWidth
				&& lc.ly >= iNewOffsetY && lc.ly < nNewHeight)
					gEnv->pCryPak->OpenPack(sSegmentPakFileName);
				else
					gEnv->pCryPak->ClosePack(sSegmentPakFileName);
			}
		}
	}
}

bool CSegmentedWorldDoc::MoveTo(int iNewOffsetX, int iNewOffsetY, UINT nNewWidth, UINT nNewHeight, bool bAuto)
{
	if (CSWManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return false;

	EVersionType vt = VT_CURRENT;
	CHeightmap *phm = GetIEditor()->GetHeightmap();
	uint nOldWidth = m_uiWidth, nOldHeight = m_uiHeight;
	int iOldOffsetX = m_wcOfs.wx, iOldOffsetY = m_wcOfs.wy;
	CRect rcOld(CPoint(m_wcOfs.wx, m_wcOfs.wy), CSize(nOldWidth, nOldHeight));
	CRect rcNew(CPoint(iNewOffsetX, iNewOffsetY), CSize(nNewWidth, nNewHeight));

	Vec2i worldMin, worldMax;
	GetWorldBounds(&worldMin, &worldMax);
	CRect rcWorld(worldMin.x, worldMin.y, worldMax.x + 1, worldMax.y + 1);

	// quit immediately if requested area locates out side of world boundaries
	if(!InRect(rcWorld, rcNew))
	{
		return false;
	}

	UpdatePackStatus(iNewOffsetX, iNewOffsetY, nNewWidth, nNewHeight);

	CPoint ofsSeg = rcNew.TopLeft() - rcOld.TopLeft();
	//Vec3 vOffOld2New;
	//Vec3 vOldSize;
	//Vec3 vNewSize;
	AABB bbOldInAbsUnits, bbNewInAbsUnits;
	Vec3 vOldOriginInAbs, vNewOriginInAbs;
	{
		//CPoint ofsHM(ofsSeg.x * SEGMENT_SIZE_UNITS, ofsSeg.y * SEGMENT_SIZE_UNITS);
		//vOffOld2New = phm->HmapToWorld(ofsHM);
		//vOldSize = phm->HmapToWorld(CPoint(rcOld.Width() * SEGMENT_SIZE_UNITS, rcOld.Height() * SEGMENT_SIZE_UNITS));
		//vNewSize = phm->HmapToWorld(CPoint(rcNew.Width() * SEGMENT_SIZE_UNITS, rcNew.Height() * SEGMENT_SIZE_UNITS));

		Vec3::value_type fMinZ = Vec3(VMIN).x, fMaxZ = Vec3(VMAX).x;

		bbOldInAbsUnits.min = phm->HmapToWorld( CPoint(rcOld.left * SEGMENT_SIZE_UNITS, rcOld.top * SEGMENT_SIZE_UNITS) );
		bbOldInAbsUnits.min.z = fMinZ;
		bbOldInAbsUnits.max = phm->HmapToWorld( CPoint(rcOld.right * SEGMENT_SIZE_UNITS, rcOld.bottom * SEGMENT_SIZE_UNITS) );
		bbOldInAbsUnits.max.z = fMaxZ;
		bbNewInAbsUnits.min = phm->HmapToWorld( CPoint(rcNew.left * SEGMENT_SIZE_UNITS, rcNew.top * SEGMENT_SIZE_UNITS) );
		bbNewInAbsUnits.min.z = fMinZ;
		bbNewInAbsUnits.max = phm->HmapToWorld( CPoint(rcNew.right * SEGMENT_SIZE_UNITS, rcNew.bottom * SEGMENT_SIZE_UNITS) );
		bbNewInAbsUnits.max.z = fMaxZ;

		vOldOriginInAbs = bbOldInAbsUnits.min;
		vOldOriginInAbs.z = 0.f;
		vNewOriginInAbs = bbNewInAbsUnits.min;
		vNewOriginInAbs.z = 0.f;
	}

	typedef std::vector<CPoint> TPointList;
	TPointList leavingSegs;
	TPointList enteringSegs;
	TPointList movingSegs;
	bool bOverlap = GenPointListsFromIntersect(rcOld, rcNew, &leavingSegs, &enteringSegs, &movingSegs);
	bool bResized = (nNewWidth != m_uiWidth || nNewHeight != m_uiHeight);
	bool bMoved = ofsSeg.x || ofsSeg.y;

	 // --== quit if no change at all ==--
	if (!bResized && !bMoved)
	{
		assert(leavingSegs.empty() && enteringSegs.empty());
		return false;
	}

	// --== lock thread ==--
	m_bSuspendThread = true;	// TODO: it not guaranteed thread-safe
	AUTO_LOCK_BIG(g_csBigLock);
	//AUTO_LOCK(g_cs);
	m_bSuspendThread = false;
	

	// --== save selection info ==--
	std::vector<GUID> selectionsObjectsGUIDs;
	GetIEditor()->GetObjectManager()->SWGetSelectionsObjectsGIUDs(selectionsObjectsGUIDs);

	CXmlArchive xmlArNamedSelection;
	xmlArNamedSelection.bLoading = false;
	xmlArNamedSelection.root = XmlHelpers::CreateXmlNode("NamedSelections");
	GetIEditor()->GetObjectManager()->SerializeNameSelection(xmlArNamedSelection.root, xmlArNamedSelection.bLoading);


	// --== get all CBaseObjects ==--
	CBaseObjectsArray arrOldObjects;
	bool bObjectsFilled = false;
	if (!bObjectsFilled) {
		GetObjectsFiltered(arrOldObjects);
		//GetIEditor()->GetObjectManager()->GetObjects(arrObjects);
		bObjectsFilled = true;
	}


	// --== save kept segments to Memory (because Editor objects are going to be deleted and re-added) ==--
	for ( TPointList::const_iterator iter = movingSegs.begin(); iter != movingSegs.end(); ++iter)
	{
		TWorldCoords wc(iter->x, iter->y);
		TSegmentData* pd = m_segs.GetSegmentData(wc, false);
		if (!pd->IsModified() && !pd->IsLayersModified() && !m_extobjs.ObjectData_IsModified(wc))
			continue;

		m_segs.SaveSegment(SF_NoSubmitSave, wc, arrOldObjects, 0, vt);
		m_extobjs.SaveForSegment(SF_NoSubmitSave, wc, arrOldObjects, 0, vt);
	}


	// --== save leaving segs to files, if there are any modification ==--
	{
		ESaveMode eSave = bAuto ? SAVE_CANCEL : SAVE_ASK;
		int64 iVersion = 0;

		for ( TPointList::const_iterator iter = leavingSegs.begin(); iter != leavingSegs.end(); ++iter)
		{
			TWorldCoords wc(iter->x, iter->y);
			TSegmentData* pd = m_segs.GetSegmentData(wc, false);

			bool srcDataModified = false;
			for(int i = SDB_SOURCEDATA_BEGIN; i < SDB_SOURCEDATA_END; i++)
			{
				srcDataModified |= pd->IsModified(i);
			}
			if (!srcDataModified && !pd->IsLayersModified() && !m_extobjs.ObjectData_IsModified(wc))
				continue;

			if (eSave == SAVE_ASK)
				eSave = AskForSave("Changed segments are leaving the loaded area. Do you want to save them?");
			if (eSave == SAVE_CANCEL)
				return false;
			else if (eSave == SAVE_NO)
				break;

			if (iVersion == 0)
				iVersion = GetVersionControl()->CreateChangeList(vt);
			m_segs.SaveSegment(SF_RegularSave, wc, arrOldObjects, iVersion, vt);
			m_extobjs.SaveForSegment(SF_RegularSave, wc, arrOldObjects, iVersion, vt);
		}

		if (eSave == SAVE_YES)
		{
			m_extobjs.SaveAll(SF_RegularSave,vt, iVersion);
			m_extobjs.SaveObjectXML( m_sPath );
		}
	}

	// --== add an undo but suspend it for now ==--
	// This undo must not be added (for now), because any undo will set the document to be modified, and 
	// that will cause unnecessary save & msgbox
	// TODO: The best way to solve this issue while still keep the undo function is to make it a deferred save
	//       by queuing info of changed segments
	//CUndo undo("SW Move");
	//if (GetIEditor()->IsUndoRecording())
	//	GetIEditor()->RecordUndo(new CUndoSWMove());

	GetIEditor()->GetDocument()->SetDocumentReady(false);
	GetIEditor()->SuspendUndo();


	// --== delete all vegetation ==--
	if (m_bEditMode) {
		AABB bb(bbOldInAbsUnits); bb.Move(-vOldOriginInAbs);
		assert(bb.min.x == 0.f && bb.min.y == 0.f);
		GetIEditor()->GetHeightmap()->GetVegetationMap()->ClearSegment(bb);
	}


	// --== delete objects in leaving area ==--
	//	(or all objects if resizing, since we're doing full Load() in this case)
	if (bMoved /*&& bOverlap*//* && !bResize*/)
	{
		DWORD i;
		GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);
		//if (!bObjectsFilled) {
		//	GetObjectsFiltered(arrObjects);
		//	//GetIEditor()->GetObjectManager()->GetObjects(arrObjects);
		//	bObjectsFilled = true;
		//}
		CBaseObjectsArray arrDel;
		for (i = 0; i < arrOldObjects.size(); ++i)
		{
			CBaseObject *po = arrOldObjects[i];
			if (!po)
				continue;
			//if (po->GetParent()) {
			//	arrObjects[i] = 0;
			//	continue;
			//}
			//if (po->GetGroup())	{
			//	arrObjects[i] = 0;
			//	continue;
			//}

			//Vec3 ptRltPosInNew = po->GetPos() - vOffOld2New;
			//if ((ptRltPosInNew.x < 0
			//	|| ptRltPosInNew.x >= vNewSize.x
			//	|| ptRltPosInNew.y < 0
			//	|| ptRltPosInNew.y >= vNewSize.y)
			//	/*&& !stl::find(selectionsObjectsGUIDs, po->GetId())*/) // if not in new
			Vec3 vAbsPos = po->GetPos() + vOldOriginInAbs;
			if ( bbNewInAbsUnits.IsContainPoint(vAbsPos) ) // if in new, then it need to be kept, skip
				continue;

			arrDel.push_back(po);	// TODO: don't have to unapply overlap area
			arrOldObjects[i] = 0;
		}
		for (i = 0; i < arrDel.size(); ++i)
		{
			CBaseObject *po = arrDel[i];
			if(m_extobjs.IsExternal(po))
			{
				TObjectData *pObjData = m_extobjs.FindObjectData(po);
				if (pObjData)
					pObjData->NoLongerApplied();
			}
		}

		for (i = 0; i < arrDel.size(); ++i)
		{
			CBaseObject *po = arrDel[i];
			DeleteObjectWithChilds(po);
			arrDel[i] = 0;
			//GetIEditor()->GetObjectManager()->DeleteObject(po);
		}
		GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(false);
	}
	//else if (bResize)
	//{
	//	GetIEditor()->GetObjectManager()->DeleteAllObjects();
	//	arrObjects.clear();
	//	bObjectsFilled = true;
	//}


	// --== save engine SID ==--
	int *pSIDs = NULL;
	if (m_bEditMode)
	{
		pSIDs = new int [m_uiWidth * m_uiHeight];
		memset(pSIDs,-1,sizeof(int)*m_uiWidth*m_uiHeight);

		CPoint ptOld;
		for (ptOld.y = m_wcOfs.wy; ptOld.y < m_wcOfs.wy + (int) m_uiHeight; ++ptOld.y) {
			for (ptOld.x = m_wcOfs.wx; ptOld.x < m_wcOfs.wx + (int) m_uiWidth; ++ptOld.x) {
				TWorldCoords wc(ptOld.x, ptOld.y);
				TSegmentData *pd = m_segs.GetSegmentData(wc, false);
				assert(pd);
				pSIDs[(ptOld.x - m_wcOfs.wx) + (ptOld.y - m_wcOfs.wy) * m_uiWidth] = pd->GetEngineSID();
			}
		}
	}


	// --== delete leaving segments ==--
	CPoint ptOld;
	for (TPointList::const_iterator iter = leavingSegs.begin(); iter != leavingSegs.end(); ++iter)
	{
		TWorldCoords wc(iter->x, iter->y);
		TSegmentData *pd = m_segs.GetSegmentData(wc, false);
		assert(pd);

		pd->NoLongerApplied();
		m_extobjs.ObjectData_NoLongerApplied(wc);	// TODO: redundant?
		if (pd->IsInUse())
		{
			if (!m_bEditMode)
				GetIEditor()->Get3DEngine()->GetITerrain()->DeleteSegment(pd->GetEngineSID(),true);
			pd->OnRemoveFromEngine();
		}
	}

	// this is not cosmetics only!!!
	// having a progress here disables all other progress during resizing
	// we do not want them to operate, because they let the system to "breed"
	// a mouse move would issue picking, which would access engine heightmap data,
	// which may not be initialized yet, resulting into a crash!
	//CWaitProgress progress( _T("Moving in segmented world") );
	//progress.Step(100); 


	// --== shifting world offset, all lc -> wc converting will be affected ==--
	m_uiWidth = nNewWidth;
	m_uiHeight = nNewHeight;
	m_wcOfs.wx = iNewOffsetX;
	m_wcOfs.wy = iNewOffsetY;


	// --== update UI ==--
	CGridMapDlg *pDlg = CGridMapDlg::Get();
	if (pDlg)
		pDlg->SetEditArea(iNewOffsetX, iNewOffsetY, nNewWidth, nNewHeight);


	// --== in resize case, Clear data and reload the whole level ==--
	if (bResized) {
		SAFE_DELETE(pSIDs);

		GetIEditor()->GetObjectManager()->DeleteAllObjects();
		WorldData_Revert(vt);
		//int nMaxTilesNum = GetIEditor()->Get3DEngine()->GetTerrainSize()/
		//	GetIEditor()->Get3DEngine()->GetTerrainTextureNodeSizeMeters();
		//phm->GetTerrainGrid()->InitSectorGrid(nMaxTilesNum);
		//phm->ClearModSectors();
		//Load(false);

		xmlArNamedSelection.bLoading = true;
		GetIEditor()->GetObjectManager()->SerializeNameSelection(xmlArNamedSelection.root, xmlArNamedSelection.bLoading);
		GetIEditor()->ResumeUndo();
		GetIEditor()->GetDocument()->SetDocumentReady(true);
		return true;

		//CXmlArchive ar("Temp");
		//ar.bLoading = false;
		//phm->GetVegetationMap()->Serialize(ar);
		//phm->Resize(w * SEGMENT_SIZE_UNITS, h * SEGMENT_SIZE_UNITS, GetIEditor()->GetHeightmap()->GetUnitSize(), true);
		//ar.bLoading = true;
		//phm->GetVegetationMap()->Serialize(ar);
		//CTerrainManager::GetTerrainManager().ReloadSurfaceTypes(false);
	}


	// --== Create segments that we are entering ==--
	//if (!bResize)
	if (m_bEditMode)
	{
		ITerrain* pTerrain = GetIEditor()->Get3DEngine()->GetITerrain();
		TLocalCoords lc;
		CString sSegmentPakFileName;
		CString sSegmentPath;
		CString sFileBlockPath;
		GetIEditor()->Get3DEngine()->CloseTerrainTextureFile();
		for (lc.ly = 0; lc.ly < nNewHeight; ++lc.ly)
		{
			for (lc.lx = 0; lc.lx < nNewWidth; ++lc.lx)
			{
				TSegmentData *pd = m_segs.GetSegmentData(lc, true);

				if ( lc.ly < nOldHeight && lc.lx < nOldWidth )// the part that are not exceeding old amount, we can reuse the SID we have.
				{
					int nSID = pSIDs[lc.lx + lc.ly * nOldWidth];			// all SID will be re-assigned according to relative pos, no one keeping its owned. -RL

					m_pPersistance->GetPakPath(sSegmentPakFileName, sSegmentPath, sFileBlockPath, pd->m_wc, s_bLevelMultiPack, false);
					sFileBlockPath = Path::FullPathToLevelPath(sFileBlockPath);
					pTerrain->SetSegmentPath(nSID, sFileBlockPath);
					pd->OnCreatedInEngine(nSID);
				}
				else	// new slot need to be created
				{
					
					Vec3 vOfs = CalcWorldBounds(lc).min;
					vOfs.z = 0;
					Vec3 vSize((float) SEGMENT_SIZE_METERS, (float) SEGMENT_SIZE_METERS, (float) SEGMENT_SIZE_METERS);
					m_pPersistance->GetPakPath(sSegmentPakFileName, sSegmentPath, sFileBlockPath, pd->m_wc, s_bLevelMultiPack, false);
					sFileBlockPath = Path::FullPathToLevelPath(sFileBlockPath);
					int nSID = pTerrain->CreateSegment(vSize, vOfs, sFileBlockPath);
					pd->OnCreatedInEngine(nSID);
					GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);
					//GetIEditor()->Get3DEngine()->LoadTerrainSurfacesFromXML(node, false, psd->m_nSID);
					CTerrainManager::GetTerrainManager().ReloadSurfaceTypes(false,false);
					GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(false);
				}
			}
		}
		delete pSIDs;
	}
	else
	{
		CreateSegments(GetIEditor()->Get3DEngine()->GetITerrain());
	}

	//if (!bOverlap)
	//{
	//	// DO full reload
	//	int nMaxTilesNum = GetIEditor()->Get3DEngine()->GetTerrainSize()/
	//		GetIEditor()->Get3DEngine()->GetTerrainTextureNodeSizeMeters();
	//	phm->GetTerrainGrid()->InitSectorGrid(nMaxTilesNum);
	//	phm->ClearModSectors();
	//	GetIEditor()->ResumeUndo();
	//	GetIEditor()->GetDocument()->SetDocumentReady(true);
	//	Load(false);
	//	xmlArNamedSelection.bLoading = true;
	//	GetIEditor()->GetObjectManager()->SerializeNameSelection(xmlArNamedSelection.root, xmlArNamedSelection.bLoading);
	//	return true;
	//}

	//if (bResize)
	//  AllocSegments();

	//GetIEditor()->GetErrorReport()->SetImmidiateMode(false);


	// --== move objects ==--
	if (bMoved && bOverlap)
	{
		GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);
		//if (!bObjectsFilled) {
		//	GetObjectsFiltered(arrObjects);
		//	//GetIEditor()->GetObjectManager()->GetObjects(arrObjects);
		//	bObjectsFilled = true;
		//}

		//Vec3 vMax = phm->HmapToWorld(CPoint(nNewWidth * SEGMENT_SIZE_UNITS, nNewHeight * SEGMENT_SIZE_UNITS));
		for (size_t i = 0; i < arrOldObjects.size(); ++i) {
			CBaseObject *po = arrOldObjects[i];
			if (!po)
				continue;
			//if (po->GetParent())
			//	continue;
			//if (po->GetGroup())
			//	continue;
			Vec3 vAbsPos = po->GetPos() + vOldOriginInAbs;
			if (!bbNewInAbsUnits.IsContainPoint(vAbsPos)) // if not in new skip
				continue;

			po->SetPos(vAbsPos - vNewOriginInAbs /*, TM_NOT_INVALIDATE*/);	// move to new coordination
			// reload is necessary for lights, because simple SetPos doesn't work
			if (po->GetRuntimeClass() == RUNTIME_CLASS(CEntityObject))
				po->OnEvent(EVENT_RELOAD_ENTITY);
		}
		GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(false);
	}
	arrOldObjects.clear();


	// --== offset terrain layer & modsector ==--
	if (bMoved && bOverlap)
	{
		CRGBLayer &TerrainRGBLayer = CTerrainManager::GetTerrainManager().GetHeightmap().m_TerrainRGBTexture;
		int ox = ofsSeg.x;
		int oy = ofsSeg.y;
		Seg2RGB(ox, oy);
		TerrainRGBLayer.Offset(-ox, -oy);

		// offset mod sectors
		GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);
		int iTexSectorSize = GetIEditor()->Get3DEngine()->GetTerrainTextureNodeSizeMeters();
		ox = ofsSeg.x * SEGMENT_SIZE_METERS / iTexSectorSize;
		oy = ofsSeg.y * SEGMENT_SIZE_METERS / iTexSectorSize;
		phm->OffsetModSectors(-ox, -oy);
		GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(false);
	}

	// Currently the heightmap not support resize, we have to reload in resizing case
	//// --== resize terrain layer & heightmap ==--
	//if (bResized)
	//{
	//	CHeightmap& hm = CTerrainManager::GetTerrainManager().GetHeightmap();
	//	CRGBLayer &TerrainRGBLayer = hm.m_TerrainRGBTexture;
	//
	//	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true); // need this?
	//	TerrainRGBLayer.Resize(nNewWidth,nNewHeight,SEGMENT_SIZE_METERS);
	//	hm.Resize(nNewWidth*SEGMENT_SIZE_METERS,nNewHeight*SEGMENT_SIZE_METERS,hm.GetUnitSize(),true); // try not clean old
	//	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(false);
	//}


	// --== Load new segs, reload kept segs, load external objects ==--

	CObjectArchive oa(GetIEditor()->GetObjectManager(), 0, true );
	CBaseObjectsArray arrNewObjects;
	if (m_bEditMode)
	{
		for (TPointList::const_iterator iter = movingSegs.begin(); iter != movingSegs.end(); ++iter)
		{
			TWorldCoords wc(iter->x,iter->y);
			TSegmentData* pd = m_segs.GetSegmentData(wc,true);
			assert(pd);
			pd->NoLongerApplied(SDB_Heights);
			pd->NoLongerApplied(SDB_LayerIDs);
			pd->NoLongerApplied(SDB_Vegetation);
			//pd->NoLongerApplied(SDB_RGBLayer);
			pd->NoLongerApplied(SDB_EngData);
			m_segs.LoadSegment(wc, arrNewObjects, oa, WS_EDITOR_TERRAIN | WS_ENGINE_TERRAIN);
		}
	}
	for (TPointList::const_iterator iter = enteringSegs.begin(); iter != enteringSegs.end(); ++iter)
	{
		TWorldCoords wc(iter->x,iter->y);
		DWORD dwFlags = WS_UPDATE_STATUS | (m_iDelayedLoading ? WS_EDITOR_TERRAIN | WS_ENGINE_TERRAIN : WS_LOAD_ALL);
		m_segs.LoadSegment(wc, arrNewObjects, oa, dwFlags);
	}

	OnMayUpdateLevelPak();

	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);
	oa.ResolveObjects();
	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(false);

	m_extobjs.ObjectData_Load(true);


	// --== restore selections ==--
	for (int i = 0; i < selectionsObjectsGUIDs.size(); i++)
	{
		CBaseObject* po = NULL;
		po = GetIEditor()->GetObjectManager()->FindObject(selectionsObjectsGUIDs[i]);
		if (po == NULL)
			SWLog("object in named selection not found\n", true);
	}

	xmlArNamedSelection.bLoading = true;
	GetIEditor()->GetObjectManager()->SerializeNameSelection(xmlArNamedSelection.root, xmlArNamedSelection.bLoading);


	// --== finalize ==--
	UpdateEngine();

	GetIEditor()->ResumeUndo();
	GetIEditor()->GetDocument()->SetDocumentReady(true);
	//GetIEditor()->GetErrorReport()->Display();
	CleanUp(false);


	return true;
}

void CSegmentedWorldDoc::CleanSectorsTexture(const TLocalCoords &lc)
{
	//bool bWasActive = GetIEditor()->Get3DEngine()->IsSegmentedWorldActive();
	//GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);
	int iTexSectorSize = GetIEditor()->Get3DEngine()->GetTerrainTextureNodeSizeMeters();
	int x = lc.lx*SEGMENT_SIZE_METERS / iTexSectorSize;
	int y = lc.ly*SEGMENT_SIZE_METERS / iTexSectorSize;
	int w = SEGMENT_SIZE_METERS / iTexSectorSize;
	int h = SEGMENT_SIZE_METERS / iTexSectorSize;
	
	CRect rc(CPoint(x, y), CSize(w, h));
	GetIEditor()->GetHeightmap()->UnlockSectorsTexture(rc);

	//GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(bWasActive);
}

void CSegmentedWorldDoc::SWDBG(int dbgOption)
{
	CSWDebug dbg;
	dbg.SWDBG(dbgOption);
}
void CSWDebug::SWDBG(int dbgOption)
{
	CSWDoc *psw = GetInternalSWDoc();

	string strDbg;
	if (dbgOption == 0)
	{
		OutputDebugString("------------------\n\n START\n\n----------------------\n\n\n");
		CBaseObjectsArray objects;
		psw->GetObjectsFiltered(objects);
		for (int i = 0; i < objects.size(); ++i)
		{
			CBaseObject* po = objects[i];

			string strGUID = (po ? GuidUtil::ToString(po->GetId()) : "NULL");
			
			strDbg.Format("%d  ->  %s\n", i, strGUID);
			OutputDebugString(strDbg);
		}
		OutputDebugString("------------------\n\n END\n\n----------------------\n\n\n");
	}

	if (dbgOption == 1 || dbgOption == 2)
	{
		OutputDebugString("------------------\n\n Object Layers\n\n----------------------\n\n\n");
		
		std::vector<CObjectLayer*> vLayers;
		typedef CLayerInfoManager::TLayerInfo TLayerInfo;
		std::vector<TLayerInfo> vLayerInfo;
		GetIEditor()->GetObjectManager()->GetLayersManager()->GetLayers(vLayers);
		for (int i = 0; i < vLayers.size(); i++)
		{
			TLayerInfo li;
			li.m_guid = vLayers[i]->GetGUID();
			li.m_nID = -1;
			li.m_strName = vLayers[i]->GetName();
			vLayerInfo.push_back(li);
			strDbg.Format("%d. \n\tGUID: %s\n\tName: %s\n", i, GuidUtil::ToString(li.m_guid), li.m_strName.c_str());
			OutputDebugString(strDbg);
		}

		if (dbgOption == 2)
		{
		//	//AUTO_LOCK(g_cs);
		//	if (!g_db->GetCachedConn())
		//		return;
		//	IDBConnection* pConn = g_db->GetCachedConn()->GetDBConnection();
		//	if (!pConn)
		//		return;
		//	IDBStatementPtr pStmt = pConn->CreateStatement();
		//	if (!pStmt)
		//		return;
		//	std::vector<TLayerInfo> vLayerInfo2;
		//	string sQuery;
		//	{

		//		sQuery.Format("SELECT GUIDLo, GUIDHi, id, name FROM Layers");
		//		if (!pStmt || !pStmt->Execute(sQuery.c_str())) 
		//		{
		//			gEnv->pLog->LogToConsole("Error executing statement: %s", pConn->GetRawErrorMessage());
		//			assert(0);
		//			return;
		//		}
		//		IDBResultSetPtr pResultSet = pStmt->GetResultSet();
		//		if (pResultSet)
		//		{
		//			while (true == pResultSet->Next())
		//			{
		//				TLayerInfo li;
		//				int64 guidLo;
		//				int64 guidHi;
		//				const char *chName = 0;
		//				if (!DBAPIHelper::Get(pResultSet, guidLo, guidHi, li.m_nID, chName))
		//					continue;
		//				psw->CalcGuid(li.m_guid, guidLo, guidHi);
		//				li.m_strName = chName;
		//				vLayerInfo2.push_back(li);
		//			}
		//		}
		//	}

		//	for (int j = 0; j < vLayerInfo.size(); j++)
		//	{
		//		TLayerInfo li = vLayerInfo[j];
		//		if (li.m_strName.empty())
		//		{
		//			assert(0);
		//			continue;
		//		}

		//		for (int i = 0; i < vLayerInfo2.size(); i++)
		//		{
		//			TLayerInfo li2 = vLayerInfo2[i];
		//			if (li2.m_strName.empty())
		//				assert(0);

		//			if (!strcmpi(li.m_strName.c_str(), li2.m_strName.c_str()))
		//			{
		//				//to do
		//				if (li2.m_guid == GUID_NULL)
		//				{
		//					assert(li.m_guid != GUID_NULL);
		//					li2.m_guid = li.m_guid;
		//					int64 guidLo;
		//					int64 guidHi;
		//					psw->CalcGuids(li2.m_guid, guidLo, guidHi);
		//					sQuery.Format("UPDATE Layers SET GUIDLo=%I64d, GUIDHi=%I64d, GUID='%s' WHERE id=%d", guidLo, guidHi, GuidUtil::ToString(li2.m_guid), li2.m_nID);
		//					if (!pStmt || !pStmt->Execute(sQuery.c_str())) 
		//					{
		//						gEnv->pLog->LogToConsole("Error executing statement: %s", pConn->GetRawErrorMessage());
		//						assert(0);
		//						return;
		//					}

		//					int64 nUpdateCount = pStmt->GetUpdateCount();
		//					strDbg.Format("%I64d records in Layers Updated!\n", nUpdateCount);
		//					OutputDebugString(strDbg);
		//				}
		//				continue;
		//			}

		//		}

		//	}
		//	
		}

	}
	//--------------------------------------------------------
	if (dbgOption == 3)
	{
		int h = psw->m_uiHeight;
		int w = psw->m_uiWidth;

		TWorldCoordsArray arrWc;
		TLocalCoords lc;
		for (lc.ly = 1; lc.ly < h-1; ++lc.ly) 
		{
			for (lc.lx = 1; lc.lx < w-1; ++lc.lx) 
			{
				TWorldCoords wc(lc);
				TSegmentData *pd = psw->SegmentAggr().GetSegmentData(wc, false);
				if (pd == NULL)
					continue;
				arrWc.push_back(wc);
			}
		}
		psw->m_mmgen.GenerateSegmentedMap(&arrWc);
	}
}

AABB CSegmentedWorldDoc::CalcWorldBounds( const TLocalCoords &lc )
{
	CPoint ptMin(lc.lx * SEGMENT_SIZE_UNITS, lc.ly * SEGMENT_SIZE_UNITS);
	Vec3 vMin = GetIEditor()->GetHeightmap()->HmapToWorld(ptMin);
	vMin.z = -100000.0f;

	CPoint ptMax(ptMin.x + SEGMENT_SIZE_UNITS, ptMin.y + SEGMENT_SIZE_UNITS);
	Vec3 vMax = GetIEditor()->GetHeightmap()->HmapToWorld(ptMax);
	vMax.z = 100000.0f;

	return AABB(vMin, vMax);
}

TLocalCoords CSegmentedWorldDoc::WorldToSegment( const Vec3 &v, bool bClamp /*= true*/ ) const
{
	CPoint ptHM = GetIEditor()->GetHeightmap()->WorldToHmap(v);
	if (ptHM.x < 0)
	{
		ptHM.x += - SEGMENT_SIZE_UNITS;
	}
	if (ptHM.y < 0)
	{
		ptHM.y += - SEGMENT_SIZE_UNITS;
	}

	int lx = (int) (ptHM.x / SEGMENT_SIZE_UNITS);
	int ly = (int) (ptHM.y / SEGMENT_SIZE_UNITS);
	if (bClamp)
	{
		if (lx < 0) lx = 0;
		else if (lx >= m_uiWidth) lx = m_uiWidth - 1;
		if (ly < 0) ly = 0;
		else if (ly >= m_uiHeight) ly = m_uiHeight - 1;
	}
	return TLocalCoords(lx, ly);
}

//-------------------------------------------------------------------------------------------
bool CSegmentedWorldDoc::LockLoadedWorld()
{
	for (int i = 0; i < WDB_COUNT; i++)
	{
		if (m_WorldData[i].Lock() == 0)
			return false;
	}

	if (m_segs.LockSegment(TLocalCoords(0, 0), TLocalCoords(m_uiWidth - 1, m_uiHeight - 1), true) == 0)
		return false;

	if (!m_segs.LockSegmentLayersData(TLocalCoords(0, 0), TLocalCoords(m_uiWidth - 1, m_uiHeight - 1), true))
		return false;
	
	return true;
}

void CSegmentedWorldDoc::SW_xmls_execScriptAndSave(const char* pcScriptFileName, const char* pcScriptInputFileName)
{

	m_strScriptFileName = pcScriptFileName;
	m_strScriptInputFileName = pcScriptInputFileName;
	if (!LockLoadedWorld())
	{
		SWLog("xmls_execScriptAndSave failed! - could not lock the world!\n");
		return;
	}

	if (!Save(false))
	{
		SWLog("xmls_execScriptAndSave failed! - could not save the world!\n");
		return;
	}

	SWLog("xmls_execScriptAndSave succeed!\n");
}

void CSegmentedWorldDoc::CallScript(XmlNodeRef& root)
{
	char *path=NULL;
	path=getcwd(path,256);
	
	string xmlName;
	xmlName.Format("%s\\xmlSWTemp.xml",path);
	root->saveToFile(xmlName.c_str());

	string strCMD;
	strCMD.Format("python %s\\%s \"%s\\%s\" \"%s\"", path, m_strScriptFileName.c_str(), path, m_strScriptInputFileName.c_str(), xmlName.c_str());
	system(strCMD.c_str());

	XmlNodeRef root2 = gEnv->pSystem->LoadXmlFromFile(xmlName.c_str());
	root = root2;
}
//-------------------------------------------------------------------------------------------
void CSegmentedWorldDoc::ConsoleCommand(IConsoleCmdArgs *pArgs)
{
	IConsole *pCon = GetISystem()->GetIConsole();
	int nArg = pArgs->GetArgCount();
	if (nArg <= 1) {
		gEnv->pLog->LogToConsole("%d,%d, %dx%d", m_wcOfs.wx, m_wcOfs.wy, m_uiWidth, m_uiHeight);
		return;
	}
	const char *pcmd = pArgs->GetArg(1);

	const char *pcCmds[1024];
	const char *pcHelps[1024];
	int nCmds = 0;

#define CMD(cmd, help) \
	pcCmds[nCmds] = cmd; \
	pcHelps[nCmds] = help; \
	++nCmds; \
	if (!strcmpi(pcmd, cmd))


	CMD("move", "[dx dy] - move loaded part (center around screen or offset in segments") 
	{
		if (nArg <= 2) {
			MoveToCam();
			return;
		}
		if (nArg < 4) {
			gEnv->pLog->LogToConsole("Invalid arguments");
			return;
		}
		int dx = atoi(pArgs->GetArg(2));
		int dy = atoi(pArgs->GetArg(3));

		MoveTo(m_wcOfs.wx + dx, m_wcOfs.wy + dy, m_uiWidth, m_uiHeight);

		CViewport *pvp = GetIEditor()->GetViewManager()->GetGameViewport();
		if (pvp) {
			Vec3 ptCam = GetIEditor()->GetHeightmap()->HmapToWorld(CPoint(m_uiWidth * SEGMENT_SIZE_UNITS / 2, m_uiHeight * SEGMENT_SIZE_UNITS / 2));
			Matrix34 mat = pvp->GetViewTM();
			ptCam.z = mat.GetTranslation().z;
			mat.SetTranslation(ptCam);
			pvp->SetViewTM(mat);
		}
		return;
	}

	CMD("goto", "x y - move loaded part to specified absolute segment coordinates") 
	{
		if (nArg < 4) {
			gEnv->pLog->LogToConsole("Invalid arguments");
			return;
		}
		int wx = atoi(pArgs->GetArg(2));
		int wy = atoi(pArgs->GetArg(3));

		m_bSuspendThread = true;
		AUTO_LOCK_BIG(g_csBigLock);
		m_bSuspendThread = false;

		MoveTo(wx, wy, m_uiWidth, m_uiHeight);

		CViewport *pvp = GetIEditor()->GetViewManager()->GetGameViewport();
		if (pvp) {
			Vec3 ptCam = GetIEditor()->GetHeightmap()->HmapToWorld(CPoint(m_uiWidth * SEGMENT_SIZE_UNITS / 2, m_uiHeight * SEGMENT_SIZE_UNITS / 2));
			Matrix34 mat = pvp->GetViewTM();
			ptCam.z = mat.GetTranslation().z;
			mat.SetTranslation(ptCam);
			pvp->SetViewTM(mat);
		}
		return;
	}

	CMD("size", "+|-|4|8|16 - change the size of the loaded part") 
	{
		if (nArg < 3) {
			gEnv->pLog->LogToConsole("Invalid arguments");
			return;
		}
		m_bSuspendThread = true;
		AUTO_LOCK_BIG(g_csBigLock);
		m_bSuspendThread = false;

		const char *parg = pArgs->GetArg(2);
		UINT w, h;
		if (!strcmp(parg, "+")) {
			w = m_uiWidth * 2;
			h = m_uiHeight * 2;
		} else if (!strcmp(parg, "-")) {
			w = m_uiWidth / 2;
			h = m_uiHeight / 2;
		} else
			w = h = (UINT) atoi(parg);

		if (w < 4 || (w & (w - 1)) != 0 || h < 2 || (h & (h - 1)) != 0) {
			gEnv->pLog->LogToConsole("Invalid arguments");
			return;
		}

		CPoint ofsSeg(((int) w - (int)m_uiWidth) / 2, ((int) h - (int) m_uiHeight) / 2);

		MoveTo(m_wcOfs.wx - ofsSeg.x, m_wcOfs.wy - ofsSeg.y, w, h);

		CViewport *pvp = GetIEditor()->GetViewManager()->GetGameViewport();
		if (pvp) {
			CPoint ofsHM(ofsSeg.x * SEGMENT_SIZE_UNITS, ofsSeg.y * SEGMENT_SIZE_UNITS);
			Vec3 vOfs = GetIEditor()->GetHeightmap()->HmapToWorld(ofsHM);
			Matrix34 mat = pvp->GetViewTM();
			mat.SetTranslation(mat.GetTranslation() + vOfs);
			pvp->SetViewTM(mat);
		}
		return;
	}

	CMD("auto", "[0|1] - set / toggle auto SW move")
	{
		if (nArg < 3)
			m_bAutoMoveTo = !m_bAutoMoveTo;
		else
			m_bAutoMoveTo = !!atoi(pArgs->GetArg(2));
		return;
	}
 
	CMD("delay", "0|1|2 - control delayed loading (0=off, 1=main thread, 2-background thread)")
	{
		if (nArg < 3) {
			gEnv->pLog->LogToConsole("Invalid arguments");
			return;
		}
		m_iDelayedLoading = atoi(pArgs->GetArg(2));
		return;
	}

	CMD("user", "[<number>] - view/change current user")
	{
		if (nArg < 3) {
			gEnv->pLog->LogToConsole("SW User ID: %s(%d)"
				//, g_db->GetUserName(), m_pVersionControl->GetUserID()
				, m_pVersionControl->GetUserID()
				);
			return;
		}
		OnDBUserIDChanged(atoi(pArgs->GetArg(2)));
		GetIEditor()->Notify( eNotify_OnSWLockUnlock );
		return;
	}

	CMD("lock", "- lock all loaded segments")
	{
		m_bSuspendThread = true;
		AUTO_LOCK_BIG(g_csBigLock);
		m_bSuspendThread = false;

		m_segs.LockSegment(TLocalCoords(0, 0), TLocalCoords(m_uiWidth - 1, m_uiHeight - 1));
		return;
	}

	CMD("unlock", "- unlock all loaded segments")
	{
		m_bSuspendThread = true;
		AUTO_LOCK_BIG(g_csBigLock);
		m_bSuspendThread = false;

		m_segs.UnlockSegment(TLocalCoords(0, 0), TLocalCoords(m_uiWidth - 1, m_uiHeight - 1));
		return;
	}

	CMD("commit", "- commit all saved segments")
	{
		m_bSuspendThread = true;
		AUTO_LOCK_BIG(g_csBigLock);
		m_bSuspendThread = false;

		Commit();
		return;
	}

	CMD("save", "- save segmented world")
	{
		m_bSuspendThread = true;
		AUTO_LOCK_BIG(g_csBigLock);
		m_bSuspendThread = false;

		Save(true);
		return;
	}

	CMD("revert", "[ov|cv|hv] - revert changes to specified version type, cv by default")
	{
		m_bSuspendThread = true;
		AUTO_LOCK_BIG(g_csBigLock);
		m_bSuspendThread = false;
		EVersionType vt;
		if (nArg >= 3) {
			vt = VT_COUNT;
			const char *pcVT = pArgs->GetArg(2);
			for (int iVT = 0; iVT < VT_COUNT; ++iVT) {
				if (stricmp(pcVT, g_pcVersionTypes[iVT]))
					continue;
				vt = (EVersionType) iVT;
				break;
			}
			if (vt == VT_COUNT) {
				gEnv->pLog->LogToConsole("Invalid version type");
				return;      
			}
		} else
			vt = VT_CURRENT;

		const char *pcMsg;
		//if (vt == VT_OFFICIAL)
		//	pcMsg = "Are you sure you want to revert ALL your changes to the LAST COMMITTED version?";
		//else
			pcMsg = "Are you sure you want to revert ALL your changes to the last saved version?";
		if (IDOK != MessageBox(0, pcMsg, "Revert?", MB_OKCANCEL))
			return;

		gEnv->pLog->LogToConsole("Reverting to version type: %s", g_pcVersionTypes[vt]);
		Revert(vt, false);
		return;
	}

	CMD("resolve", "- reverts all conflicted segments")
	{
		m_bSuspendThread = true;
		AUTO_LOCK_BIG(g_csBigLock);
		m_bSuspendThread = false;
		Revert(VT_CURRENT, true);
		return;
	}

	CMD("gd", "lock|unlock|save|commit|revert|reload - manipulate global data")
	{
		m_bSuspendThread = true;
		AUTO_LOCK_BIG(g_csBigLock);
		m_bSuspendThread = false;
		if (nArg < 3) {
			gEnv->pLog->LogToConsole("invalid arguments");
			return;
		}
		const char *pcAction = pArgs->GetArg(2);
		if (!strcmpi(pcAction, "lock")) 
		{
			for (int i = 0; i < WDB_COUNT; i++)
			{
				m_WorldData[i].Lock();
			}
			return;
		}
		
		if (!strcmpi(pcAction, "unlock")) 
		{
			for (int i = 0; i < WDB_COUNT; i++)
			{
				m_WorldData[i].Unlock();
			}
			return;
		}
		if (!strcmpi(pcAction, "save")) {
			assert(0);
			if (!WorldData_Save(SF_RegularSave, VT_CURRENT))
				gEnv->pLog->LogToConsole("Save Failed!");
			return;
		}
		if (!strcmpi(pcAction, "commit")) {
			//assert(0);
			//if (!WorldData_Save(SF_IntegrateSave, VT_OFFICIAL))
			//	gEnv->pLog->LogToConsole("Save Failed!");
			//else {
			//	m_WorldData[WDB_GD].Unlock();
			//	m_WorldData[WDB_TERRAIN_LAYERS].Unlock();
			//	m_WorldData[WDB_VEGETATION].Unlock();
			//}
			return;
		}
		if (!strcmpi(pcAction, "revert")) {
			//WorldData_Revert(VT_OFFICIAL);
			//m_WorldData[WDB_GD].Unlock();
			//m_WorldData[WDB_TERRAIN_LAYERS].Unlock();
			//m_WorldData[WDB_VEGETATION].Unlock();
			return;
		}
		if (!strcmpi(pcAction, "reload")) {
			WorldData_Revert(VT_CURRENT);
			return;
		}
		gEnv->pLog->LogToConsole("invalid arguments");
		return;
	}

	CMD("vt", "[ov|cv|hv] - view/change used version type")
	{
		if (nArg < 3) {
			gEnv->pLog->LogToConsole("Using version type: %s", g_pcVersionTypes[m_vt]);
			return;
		}
		m_bSuspendThread = true;
		AUTO_LOCK_BIG(g_csBigLock);
		m_bSuspendThread = false;

		const char *pcVT = pArgs->GetArg(2);
		for (int iVT = 0; iVT < VT_COUNT; ++iVT) {
			if (stricmp(pcVT, g_pcVersionTypes[iVT]))
				continue;
			m_vt = (EVersionType) iVT;
			Update(true); // Update Status - takes some time
			m_lastHotSaveTime = clock() + 5000; //added 5 second delay for first save ???
			//SWModified(false);
			return;
		}
		gEnv->pLog->LogToConsole("Invalid version type");
		return;
	}

	CMD("em", "- toggle editor/engine mode")
	{
		m_bSuspendThread = true;
		AUTO_LOCK_BIG(g_csBigLock);
		m_bSuspendThread = false;

		GetIEditor()->GetDocument()->SetDocumentReady(false);
		GetIEditor()->SuspendUndo();
		m_bEditMode = !m_bEditMode;
		TLocalCoords lc;
		for (lc.ly = 0; lc.ly < m_uiHeight; ++lc.ly) {
			for (lc.lx = 0; lc.lx < m_uiWidth; ++lc.lx) {
				TSegmentData *pd = m_segs.GetSegmentData(lc, false);
				if (!pd)
					continue;
				pd->NoLongerApplied();
				pd->NoLongerAppliedLayers();
				m_extobjs.ObjectData_NoLongerApplied(pd->m_wc);
			}
		}
		GetIEditor()->GetVegetationMap()->ClearObjects();
		GetIEditor()->GetObjectManager()->DeleteAllObjects();
		GetIEditor()->ResumeUndo();
		Load(false);
		return;
	}

	CMD("locktex", "- forcibly apply editor's terrain texture (causes slow down!)")
	{
		GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);
		CRect rc(0, 0, GetIEditor()->GetHeightmap()->GetWidth(), GetIEditor()->GetHeightmap()->GetHeight());
		GetIEditor()->GetHeightmap()->UpdateLayerTexture(rc);
		GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(false);
		return;
	}

	CMD("flush", "[memSize] - Flushing to memsize in Bytes")
	{
		m_bSuspendThread = true;
		AUTO_LOCK_BIG(g_csBigLock);
		m_bSuspendThread = false;

		if (nArg < 3) {
			CleanUp(false);
			return;
		}
		CleanUp(false,atoi(pArgs->GetArg(2)));
		return;
	}

	CMD("mem", "- Allocated cache size")
	{
		int cacheSize = m_segs.GetCacheSize();
		int segCount = m_segs.GetSegmentCount();
		if( cacheSize > (1024*1024))
			gEnv->pLog->LogToConsole("Used Memmory: %d MB , Loaded Segments: %d", (cacheSize/(1024*1024)), segCount);
		else if( cacheSize > 1024)
			gEnv->pLog->LogToConsole("Used Memmory: %d KB , Loaded Segments: %d", (cacheSize/(1024)), segCount);
		else
			gEnv->pLog->LogToConsole("Used Memmory: %d Bytes , Loaded Segments: %d", cacheSize, segCount);
		return;
	}

	CMD("map", "gen - manipulate world map")
	{
		if (nArg < 3) {
			gEnv->pLog->LogToConsole("invalid arguments");
			return;
		}
		const char *pcAction = pArgs->GetArg(2);
		if (!strcmpi(pcAction, "gen")) {
			//int res = GenerateWorldMap();
			m_mmgen.GenerateSegmentedMap();
			return;
		}
		
		gEnv->pLog->LogToConsole("invalid arguments");
		return;
	}

	CMD("terrain", "lock|unlock|reload|save|commit|rollback - manipulate GD terrain layers")
	{
		if (nArg < 3) {
			gEnv->pLog->LogToConsole("invalid arguments");
			return;
		}

		m_bSuspendThread = true;
		AUTO_LOCK_BIG(g_csBigLock);
		m_bSuspendThread = false;

		const char *pcAction = pArgs->GetArg(2);
		if (!strcmpi(pcAction, "lock")) {
			//AUTO_LOCK(g_cs);
			int res = m_WorldData[WDB_TERRAIN_LAYERS].Lock();
			return;
		}
		if (!strcmpi(pcAction, "unlock")) {
			//AUTO_LOCK(g_cs);
			int res = m_WorldData[WDB_TERRAIN_LAYERS].Unlock();
			return;
		}
		if (!strcmpi(pcAction, "reload")) {
			//AUTO_LOCK(g_cs);
			m_bSuspendThread = false;
			if (WorldData_ReloadTerrainLayers())
			{
				gEnv->pLog->LogToConsole("reload terrain layers - success");
			} else {
				gEnv->pLog->LogToConsole("reload terrain layers - fail");
			}
			return;
		}
		if (!strcmpi(pcAction, "rollback"))
		{
			////AUTO_LOCK(g_cs);
			//m_bSuspendThread = false;
			//m_WorldData[WDB_TERRAIN_LAYERS].Revert(VT_OFFICIAL);
			//if (WorldData_ReloadTerrainLayers())
			//{
			//	gEnv->pLog->LogToConsole("rollback terrain - success");
			//} else {
			//	gEnv->pLog->LogToConsole("rollback terrain - fail");
			//}
			//m_WorldData[WDB_TERRAIN_LAYERS].Unlock();
			return;
		}
		if (!strcmpi(pcAction, "save")) {

			//AUTO_LOCK(g_cs);
			m_bSuspendThread = false;
			
			int64 iVersion = -1;
			if (!WorldData_Save(SF_RegularSave, VT_CURRENT, iVersion, WDB_TERRAIN_LAYERS))
				gEnv->pLog->LogToConsole("terrain layers Save Failed!");
			else
				gEnv->pLog->LogToConsole("Terrain Layers Save - success");
			return;
		}
		if (!strcmpi(pcAction, "commit")) {
			////AUTO_LOCK(g_cs);
			//m_bSuspendThread = false;
			//int64 iVersion = -1;
			//if (!WorldData_Save(SF_IntegrateSave, VT_OFFICIAL, iVersion, WDB_TERRAIN_LAYERS))
			//	gEnv->pLog->LogToConsole("Terrain Layers Save Failed!");
			//else {
			//	m_WorldData[WDB_TERRAIN_LAYERS].Unlock();
			//	gEnv->pLog->LogToConsole("Terrain Layers Save - success");
			//}
			return;
		}
		
		gEnv->pLog->LogToConsole("invalid arguments");
		return;
	}

	CMD("vegeta", "lock|unlock|reload|save|commit|rollback - manipulate GD vegetation map")
	{
		if (nArg < 3) {
			gEnv->pLog->LogToConsole("invalid arguments");
			return;
		}

		m_bSuspendThread = true;
		AUTO_LOCK_BIG(g_csBigLock);
		m_bSuspendThread = false;

		const char *pcAction = pArgs->GetArg(2);
		if (!strcmpi(pcAction, "lock")) {
			//AUTO_LOCK(g_cs);
			/*ELockResult*/int res = m_WorldData[WDB_VEGETATION].Lock();
			GetIEditor()->Notify( eNotify_OnSWVegetationStatusChange );
			return;
		}
		if (!strcmpi(pcAction, "unlock")) {
			//AUTO_LOCK(g_cs);
			int res = m_WorldData[WDB_VEGETATION].Unlock();
			GetIEditor()->Notify( eNotify_OnSWVegetationStatusChange );
			return;
		}
		if (!strcmpi(pcAction, "reload")) {
			//AUTO_LOCK(g_cs);
			m_bSuspendThread = false;
			if (WorldData_ReloadVegetationMap())
			{
				gEnv->pLog->LogToConsole("reload vegetation - success");
			} else {
				gEnv->pLog->LogToConsole("reload vegetation - fail");
			}
			GetIEditor()->Notify( eNotify_OnSWVegetationStatusChange );
			return;
		}
		if (!strcmpi(pcAction, "rollback"))
		{
			////AUTO_LOCK(g_cs);
			//m_bSuspendThread = false;
			//m_WorldData[WDB_VEGETATION].Revert(VT_OFFICIAL);
			//if (WorldData_ReloadVegetationMap())
			//{
			//	gEnv->pLog->LogToConsole("rollback vegetation - success");
			//} else {
			//	gEnv->pLog->LogToConsole("rollback vegetation - fail");
			//}
			//m_WorldData[WDB_VEGETATION].Unlock();
			//GetIEditor()->Notify( eNotify_OnSWVegetationStatusChange );
			return;
		}
		if (!strcmpi(pcAction, "save")) {

			//AUTO_LOCK(g_cs);
			m_bSuspendThread = false;
			
			int64 iVersion = -1;
			if (!WorldData_Save(SF_RegularSave, VT_CURRENT, iVersion, WDB_VEGETATION))
				gEnv->pLog->LogToConsole("VegetationMap Save - fail");
			else
				gEnv->pLog->LogToConsole("VegetationMap Save - success");
			GetIEditor()->Notify( eNotify_OnSWVegetationStatusChange );
			return;
		}
		if (!strcmpi(pcAction, "commit")) {
			////AUTO_LOCK(g_cs);
			//m_bSuspendThread = false;
			//int64 iVersion = -1;
			//if (!WorldData_Save(SF_IntegrateSave, VT_OFFICIAL, iVersion, WDB_VEGETATION))
			//	gEnv->pLog->LogToConsole("VegetationMap Save Failed!");
			//else {
			//	m_WorldData[WDB_VEGETATION].Unlock();
			//	gEnv->pLog->LogToConsole("VegetationMap Save - success");
			//}
			//GetIEditor()->Notify( eNotify_OnSWVegetationStatusChange );
			return;
		}
		
		gEnv->pLog->LogToConsole("invalid arguments");
		return;
	}
	
	CMD("wdb", "wdbType wdbCC - WDB Actions") 
	{
		if (nArg < 4) {
			gEnv->pLog->LogToConsole("Invalid arguments");
			return;
		}
		EWDBType nWDBType = (EWDBType)atoi(pArgs->GetArg(2));
		EWDBConsoleComands nWDBCC = (EWDBConsoleComands)atoi(pArgs->GetArg(3));
		if (nWDBType < 0 || nWDBType >= WDB_COUNT)
		{
			gEnv->pLog->LogToConsole("Invalid WDBType");
			return;
		}

		if (nWDBCC < 0 || nWDBCC >= sw::WDBCC_COUNT)
		{
			gEnv->pLog->LogToConsole("Invalid WDBCC");
			return;
		}

		WorldDataCC(nWDBType, nWDBCC);

		return;
	}

	CMD("dbg", "[<number>] - debug")
	{
		if (nArg < 3) {
			SWDBG();
			return;
		}
		SWDBG(atoi(pArgs->GetArg(2)));
		return;
	}

	CMD("xmls_execScriptAndSave", "<script file> <scriptInput> - force save all, executing <script file> over xmls")
	{
		if (nArg < 3)
		{
			gEnv->pLog->LogToConsole("Invalid arguments");
			return;
		}

		const char *pcScriptFileName = pArgs->GetArg(2);
		const char *pcScriptInputFileName = "";
		if (nArg >= 4)
		{
			pcScriptInputFileName = pArgs->GetArg(3);
		}
		m_bSuspendThread = true;
		AUTO_LOCK_BIG(g_csBigLock);
		m_bSuspendThread = false;
		SetSpecialOperationType(SW_SO_XMLS_EXEC_SCRIPT_AND_SAVE);
		SW_xmls_execScriptAndSave(pcScriptFileName, pcScriptInputFileName);
		SetSpecialOperationType(SW_SO_NONE);

		TWorldName currWorld;
		currWorld.Name.Format("%s",m_chName);
		currWorld.isSegmented = true;

		//GetIEditor()->GetDocument()->OnCloseDocument();
		CSWManager::Get().OpenWorld(currWorld);
	}
#undef CMD

	if (!strcmpi(pcmd, "help"))
	{
		gEnv->pLog->LogToConsole("Available SW Doc commands:");
		for (int i = 0; i < nCmds; ++i)
			gEnv->pLog->LogToConsole("  %s %s", pcCmds[i], pcHelps[i]);
		return;
	}

	gEnv->pLog->LogToConsole("Unknown command: %s", pcmd);
}

bool CSegmentedWorldDoc::RevertAll()
{
	//bool ret = Revert(GetVersionType(),false);

	//if (WorldData_IsModified())
	//	WorldData_Revert(m_vt);

	//return ret;

	return true;
}

bool CSegmentedWorldDoc::_ObjectData_Revert_Filter( TObjectData* pObjData )
{
		if (!IsSegmentSelected(pObjData->GetSegCoords()))
			return false;
		// TO DO add some checks, asserts

		//TSegmentData* pd = GetSegmentData(pObjData->m_wc, false);
		//if (!pd)
		//	return false;
		//TLayerData* pl = pd->GetLayer(pObjData->m_LayerGUID, pObjData->m_strLayerName);
		//if (!pl)
		//	return false;
		return m_segs.GetSegmentLayer(pObjData->GetSegCoords(), pObjData->GetLayerGUID(), pObjData->GetLayerName());

		//bool bModified = pObjData->IsModified();
		//bool bLocked = pl->IsLockedByMe();
		//if (bConflictsOnly && (!bModified || bLocked))
		//	return false;


		return true;
}

void CSegmentedWorldDoc::CleanAllUnappliedLayerData( bool* pOutAnyChange )
{
	bool bAnyChange = false;

	uint32 h = m_uiHeight, w = m_uiWidth;
	TLocalCoords lc;
	for (lc.ly = 0; lc.ly < h; ++lc.ly)
	{
		for (lc.lx = 0; lc.lx < w; ++lc.lx) 
		{
			TSegmentData *pd = m_segs.GetSegmentData(lc, false);
			if (pd->NeedsToBeAppliedLayers())
			{
				for (TLayerMap::iterator it = pd->m_layersMap.begin(); it != pd->m_layersMap.end(); ++it)
				{
					TLayerData* pl = it->second;
					if ( pl->NeedsToBeApplied() )
					{
						//if (!pl->AppliedVersionIs(HRID_UNKNOWN))
						if (!pl->StateEqualTo(Versioned::S_Unapplied))
						{
							bAnyChange = true;
							pl->NoLongerApplied();
						}
					}
				}
			}
		}
	}

	if (pOutAnyChange) *pOutAnyChange = bAnyChange;
}

bool CSegmentedWorldDoc::IsBlockTypeNeeded( ESDBType eBT )
{
	if (eBT == SDB_Map || eBT == SDB_MapS2 || eBT == SDB_MapS3)
		return false;

	if (eBT == SDB_EngRGBData)
		return true;

	if (eBT == SDB_EngData || eBT == SDB_EngEntities || eBT == SDB_VisAreas)
		return !m_bEditMode;

	return m_bEditMode;
}

bool CSegmentedWorldDoc::IsWorldDataBlockNeeded( int wdbType )
{
	//if (wdbType == WDB_LEVELPAK)
	//	return !m_bEditMode;
	return true;
}

bool CSegmentedWorldDoc::WorldData_PrepareData( uint32 flagNeedSave, sw::ESaveFlag sflag, sw::EVersionType vt, sw::ELockResult* arrSaves )
{
	bool bSave = false;

	for (int i = 0; i < WDB_COUNT; i++)
	{
		assert(arrSaves[i] == LockResult_Failed);

		//if (nWDBType != WDB_COUNT && i != nWDBType)
		//	continue;
		if (!( flagNeedSave & (1<<i) ))
			continue;

		//if (i == WDB_LEVELPAK && !(vt == VT_OFFICIAL || CSWManager::IsAutomate())) //< 
		//	continue;

		if (sflag&SF_ForceSaveAll)
			arrSaves[i] = m_WorldData[i].Lock();
		else
		{
			if (i == WDB_LEVELGENERAL || i == WDB_TIMEOFDAY)
			{
				//always save when is Locked WDB_GD or WDB's which have not notification callbacks
				arrSaves[i] = ( (m_WorldData[i].IsLockedByMe() &&	m_WorldData[i].LoadedVersionIsVT(VT_CURRENT)) ? LockResult_AlreadyHave:LockResult_Failed );
			}
			else
			{
				//save if modified
				arrSaves[i] = ( (m_WorldData[i].IsLockedByMe() &&	m_WorldData[i].IsModified()) ? LockResult_AlreadyHave : LockResult_Failed );
			}
		}

		//SAFE_DELETE(m_WorldData[i].m_xmlAr);
		//if (arrSaves[i] && i != WDB_LEVELPAK)
		//{
		//	if (i == WDB_GD)
		//		m_WorldData[i].m_xmlAr = new CXmlArchive();
		//	else
		//		m_WorldData[i].m_xmlAr = new CXmlArchive(g_pcWDBNodeNames[i][WDBNT_ROOT]);

		//	m_WorldData[i].m_xmlAr->bLoading = false;
		//}
		//arrXmlAr[i] = m_WorldData[i].m_xmlAr;

		if (arrSaves[i] || !m_WorldData[i].IsSaved(vt))
		{
			bSave = true;
		}

	}

	if (!bSave )
		return false;


	// == save all xml files ==
	TDocMultiArchive arrXmlAr;
	FillXmlArArray(arrXmlAr, NULL);
	for (size_t i=0; i<WDB_XMLCOUNT; ++i)
	{
		if (!(flagNeedSave&(1<<i)))
			continue;
		SAFE_DELETE(m_WorldData[i].m_xmlAr);
		//if (arrSaves[i] && i != WDB_LEVELPAK)
		{
			if (i == WDB_LEVELGENERAL)
				m_WorldData[i].m_xmlAr = new CXmlArchive();
			else
				m_WorldData[i].m_xmlAr = new CXmlArchive(g_pcWDBNodeNames[i][WDBNT_ROOT]);

			m_WorldData[i].m_xmlAr->bLoading = false;
		}
		arrXmlAr[i] = m_WorldData[i].m_xmlAr;

		m_WorldData[i].GoToState(Versioned::S_Modified);
	}
	//GetIEditor()->GetHeightmap()->SetUnitSize(1);
	GetIEditor()->GetDocument()->Save(arrXmlAr);

	// ===== Add Version attrib =====
	{
		CXmlArchive& xmlArLevel = *arrXmlAr[WDB_LEVELGENERAL];
		assert(xmlArLevel.root && xmlArLevel.root->isTag("Level"));
		xmlArLevel.root->setAttr(g_szSWDStructVerAttr, CSWManager::GetSWDataStructureVersion());
	}


	// == save level.pak files ==
	if ( CSWManager::Get().IsMapConverting() /* && flagNeedSave&(1<<WDB_LEVELPAK)*/)
	{
		assert(CSWManager::Get().IsAutomate());
	//	if (arrSaves[WDB_LEVELPAK] == 0)
	//		arrSaves[WDB_LEVELPAK] = m_WorldData[WDB_LEVELPAK].Lock();

	//	if (arrSaves[WDB_LEVELPAK])
		{
			if (CSWManager::Get().IsAutomate())
			{
				GetIEditor()->GetGameEngine()->SetLevelPath(m_sPath);
			}
			CGameExporter gameExporter;
			gameExporter.Export(false,true,true, eLittleEndian, ".", false, NULL, false);

			//((CCryEditApp*)AfxGetApp())->OnFileExportToGameNoSurfaceTexture();
		}

	//	m_WorldData[WDB_LEVELPAK].GoToState(Versioned::S_Modified);
	}

	return true;
}

void CSegmentedWorldDoc::SetAsForceReloadLevel()
{
	for (int ii = 0; ii < WDB_XMLCOUNT; ++ii)
	{
		TWorldData& wd = m_WorldData[ii];
		wd.GoToState(Versioned::S_Reloading);
	}
}


bool CSegmentedWorldDoc::NeedReloadLevel()
{
	if (s_bMapConverting)
		return true;

	bool bAnyNeedApply = false;
	for (int ii = 0; ii < WDB_XMLCOUNT; ++ii)
	{
		TWorldData& wd = m_WorldData[ii];
		bAnyNeedApply = bAnyNeedApply || !wd.IsStateAfter(Versioned::S_Loaded);
	}

	return bAnyNeedApply;
}

void CSegmentedWorldDoc::OnWorldDataApplied()
{
	for (int ii = 0; ii < WDB_XMLCOUNT; ++ii)
	{
		TWorldData& wd = m_WorldData[ii];
			wd.GoToState(Versioned::S_Applied);
	}
}

void CSegmentedWorldDoc::DoAutoSave()
{
	if (IsHot() && IsSWModified())
	{
		clock_t curr_time = clock();

		if ((curr_time - m_lastHotSaveTime) > 2000 && !IsBusy())
		{
			OnSWModified(false);
			int64 nVer = 0;

			// TODO: do we need a procedure for autosave or reuse stock Sandbox functionality?
			//assert(m_vt == VT_HOT);
			//Save(SF_RegularSave, VT_HOT, nVer, NULL);
			m_lastHotSaveTime = curr_time;
			SWLog("Auto Save....");
		}
	}
}

void CSegmentedWorldDoc::OnMayUpdateLevelPak()
{
	if (m_bAutoUpdateLevelPak)
		DoUpdateLevelPak();
}

//---------------------------------------------------
//
// param fDir:  +1, if seg relative to editor-local; -1, if editor-local to seg relative
Vec3 CSegmentedWorldDoc::SegRelativeToEditorLocal( Vec3 const& vPos, TWorldCoords const& wcSeg, f32 fDir ) const
{
	TWorldCoords const& wc = wcSeg;  // TODO: uncomment this line to use relative pos in stored data.
	CPoint offsetInEditorLocal( (wc.wx - m_wcOfs.wx) * SEGMENT_SIZE_UNITS, (wc.wy - m_wcOfs.wy) * SEGMENT_SIZE_UNITS);
	Vec3 vOff = GetIEditor()->GetHeightmap()->HmapToWorld(offsetInEditorLocal);
	vOff *= fDir;
	vOff.z = 0.f;
	
	return vPos + vOff;
}

Vec3 CSegmentedWorldDoc::LocalToAbsolutePosition( Vec3 const& vPos, f32 fDir /*= 1.f*/ ) const
{
	CPoint offsetInEditorLocal(m_wcOfs.wx * SEGMENT_SIZE_UNITS, m_wcOfs.wy * SEGMENT_SIZE_UNITS);
	Vec3 vOff = GetIEditor()->GetHeightmap()->HmapToWorld(offsetInEditorLocal);
	vOff *= fDir;
	vOff.z = 0.f;
	
	return vPos + vOff;
}

CBaseObject* CSegmentedWorldDoc::CreateBaseObjectFromMetaData( REFGUID guid )
{
	return ObjectAggr().CreateBaseObjectFromMetaData(guid);
}

void CSegmentedWorldDoc::OnAddLink( CBaseObject *pObjectLinkOwner, CBaseObject *pObjectLinkTarget)
{
	//if (GetInternalSWDoc() != NULL)
		ObjectAggr().OnAddLink(pObjectLinkOwner, pObjectLinkTarget);
}

void CSegmentedWorldDoc::OnRemoveLink( CBaseObject *pObjectLinkOwner, CBaseObject *pObjectLinkTarget)
{
	//if (GetInternalSWDoc() != NULL)
		ObjectAggr().OnRemoveLink(pObjectLinkOwner, pObjectLinkTarget);
}

void CSegmentedWorldDoc::OnObjectMakeExternal(CBaseObject* pObject)
{
	//if (GetInternalSWDoc() != NULL)
		ObjectAggr().ObjectData_MakeExternal(NULL, pObject, false);
}
bool CSegmentedWorldDoc::CanModify(CBaseObject *pObject, bool bDelete, bool bLock)
{
	//if (GetInternalSWDoc() != NULL)
	{
		CBaseObject* pAdam = GetRootObject(pObject);
		bool result = CanObjectPlaceTo(pAdam->GetWorldPos(), pAdam->GetLayer(), bLock);
		if (bDelete && result)
		{
			if (pObject->IsKindOf(RUNTIME_CLASS(CGroup)))
				return result;
			
			for (int i = 0; i < pObject->GetChildCount(); i++)
			{
				CBaseObject* pChild = pObject->GetChild(i);
				if (!CanObjectPlaceTo(pChild->GetWorldPos(), pChild->GetLayer(), bLock))
					return false;
			}
		}
		return result;
	}
	return true;
}

bool CSegmentedWorldDoc::CanObjectMoveTo(CBaseObject *pObject, const Vec3 &newPos, CObjectLayer *pNewLayer, bool bLock)
{
	//if (GetInternalSWDoc() != NULL)
	{
		if(pObject->GetParent() || pObject->GetGroup())
		{
			CBaseObject* pAdam = GetRootObject(pObject);
			return CanModify(pAdam, false, bLock);
		}

		bool result = (CanObjectPlaceTo(pObject->GetWorldPos(), pObject->GetLayer(), bLock) && CanObjectPlaceTo(newPos, pNewLayer, bLock));
		return result;
	}
	return true;
}

bool CSegmentedWorldDoc::OnObjectMoveTo( CBaseObject *pObject, const Vec3 &newPos, CObjectLayer *pLayer)
{
	//if (GetInternalSWDoc() != NULL)
	{
		bool result = CanObjectMoveTo(pObject, newPos, pLayer);
		if (result) {
			OnObjectModified(pObject, pObject->GetWorldPos(), pObject->GetLayer());
			OnObjectModified(pObject, newPos, pLayer);
		}
		return result;
	}
	return true;
}

void CSegmentedWorldDoc::InitSegmentedWorldInfo()
{
	{
		const char *levelPath = GetLevelPath();
		SegmentAggr().LoadSegmentXML(levelPath);
		LayerInfoMgr().LoadLayerXML(levelPath);
		ObjectAggr().LoadObjectXML(levelPath);
		LayerInfoMgr().ObjectLayersInfo_Init();
	}
}

bool CSegmentedWorldDoc::CheckOpenParameters() const
{
	if (m_uiWidth*m_uiHeight > 32*32)
	{
		SWLog("The opening area is too large, please choose a smaller one!");
		return false;
	}

	return true;
}

bool CSegmentedWorldDoc::IsBusy()
{
	return GetAsyncKeyState(VK_LBUTTON);
}

void CSegmentedWorldDoc::SetTerrainLayersPanelItem( CXTPTaskPanelGroupItem * pPanelGroupItem )
{
	m_pTerrainLayerPanelGroupItem = pPanelGroupItem;
}

CString CSegmentedWorldDoc::NormalizeLevelPath( const char* szLevelFilePath )
{
	CString strTmp;

	// if path is relative, convert to absolute path first
	if ( ::PathIsRelative(szLevelFilePath) )
	{
		strTmp.ReleaseBuffer( GetCurrentDirectory(MAX_PATH,strTmp.GetBufferSetLength(MAX_PATH)) );
		strTmp = Path::AddPathSlash(strTmp) + szLevelFilePath;
		szLevelFilePath = (const char*)strTmp;
	}

	CString strPath = PathUtil::GetPath(szLevelFilePath);
	Path::ConvertSlashToBackSlash(strPath);		// Path::FullPathToGamePath() can only handle path with back slashes
	CString strRelativePathPath	= Path::FullPathToGamePath(strPath);

	// if the game\levels folder is in the root path of mastercd.
	if(strRelativePathPath.GetLength() > 2 && strRelativePathPath[0] != '.' && strRelativePathPath[1] != '.')
		strPath = Path::GamePathToFullPath(strRelativePathPath);

	return strPath;
}
