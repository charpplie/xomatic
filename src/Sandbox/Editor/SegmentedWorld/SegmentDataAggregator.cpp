#include "stdafx.h"
#include "InternalCommon.h"
#include "SegmentDataAggregator.h"

#include "SegmentData.h"
#include "LayerData.h"
#include "SegmentedWorldDoc.h"
#include "ISWDataPersistence.h"

#include "Terrain/Heightmap.h"
#include "Terrain/TerrainManager.h"
#include "VegetationMap.h"
#include "Util/PakFile.h"


using namespace sw;	// without this line VA will confuse
SW_NAMESPACE_BEGIN();

CSegmentDataAggr::CSegmentDataAggr(TGlobal* pGlobal, TWorld* pWorld, TVersionMgr* pVersionMgr)
: m_pGlobal(pGlobal)
,m_pWorld(pWorld)
,m_pVersionMgr(pVersionMgr)
,m_nCacheSize(0)
,m_pFirstSegmentData(NULL)
,m_pLastSegmentData(NULL)
{
}

CSegmentDataAggr::~CSegmentDataAggr()
{
	m_pFirstSegmentData = NULL;
	m_pLastSegmentData = NULL;
	m_nCacheSize = 0;
}

bool CSegmentDataAggr::LoadSegment(const TLocalCoords &lc, CBaseObjectsArray &arrObjects, CObjectArchive &oa, DWORD dwFlags)
{
	AUTO_LOCK(g_cs);
	TWorldCoords wc(lc);
	TSegmentData *pd = GetSegmentData(wc, true);
	if (!pd)
		return false;

	PushSegmentData(pd);
	
	LoadSeLayerMapping(pd->m_wc);

	if (dwFlags & WS_UPDATE_STATUS)
	{
		pd->UpdateStatus();
		pd->UpdateLayersStatus();
	}

	GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(true);

	AABB bb = m_pWorld->CalcWorldBounds(lc);

	CPoint ptPos(lc.lx * SEGMENT_SIZE_UNITS, lc.ly * SEGMENT_SIZE_UNITS);
	CSize size(SEGMENT_SIZE_UNITS, SEGMENT_SIZE_UNITS);
	CRect rc(ptPos, size);

	bool bUpdatedTerrain = false;
	bool bUpdatedVegetation = false;
	bool bUpdatedObjects = false;

	if (dwFlags & WS_EDITOR_TERRAIN) {
		CMemoryBlock mem;
		// load heightmap
		if (pd->NeedsToBeApplied(SDB_Heights)) {
			if (pd->GetData(SDB_Heights, mem))
				GetIEditor()->GetHeightmap()->ImportSegmentHeights(mem, rc);
			else
				GetIEditor()->GetHeightmap()->ClearSegmentHeights(rc);
			pd->Applied(SDB_Heights);
			bUpdatedTerrain = true;
		}

		// load terrain layers
		if (pd->NeedsToBeApplied(SDB_LayerIDs)) {
			if (pd->GetData(SDB_LayerIDs, mem))
				GetIEditor()->GetHeightmap()->ImportSegmentLayerIDs(mem, rc);
			else
				GetIEditor()->GetHeightmap()->ClearSegmentLayerIDs(rc);
			pd->Applied(SDB_LayerIDs);
			bUpdatedTerrain = true;
		}

		// load vegetation
		if (pd->NeedsToBeApplied(SDB_Vegetation)) {
			GetIEditor()->GetVegetationMap()->ClearSegment(bb);
			if (pd->GetData(SDB_Vegetation, mem)) {
				Vec3 vOfs = GetIEditor()->GetHeightmap()->HmapToWorld(ptPos);
				GetIEditor()->GetVegetationMap()->ImportSegment(mem, vOfs);
			}
			pd->Applied(SDB_Vegetation);
			bUpdatedVegetation = true;
		}

		// RGBLayer handles SW by itself
		if (pd->NeedsToBeApplied(SDB_RGBLayer)) {
			int rx = lc.lx; int ry = lc.ly; Seg2RGB(rx, ry);
      GetIEditor()->GetHeightmap()->GetRGBLayer()->UnloadTile(rx, ry);
			if (pd->GetData(SDB_RGBLayer, mem))
      {
      }
			pd->Applied(SDB_RGBLayer);
		}
		
		// load SDB_ModSectors
		if (pd->NeedsToBeApplied(SDB_ModSectors)) {
			m_pWorld->CleanSectorsTexture(lc);
			if (pd->GetData(SDB_ModSectors, mem))
			{
				int iTexSectorSize = GetIEditor()->Get3DEngine()->GetTerrainTextureNodeSizeMeters();
				int xSect = lc.lx*SEGMENT_SIZE_METERS / iTexSectorSize;
				int ySect = lc.ly*SEGMENT_SIZE_METERS / iTexSectorSize;
				int wSect = SEGMENT_SIZE_METERS / iTexSectorSize;
				int hSect = SEGMENT_SIZE_METERS / iTexSectorSize;

				CRect rcSect(CPoint(xSect, ySect), CSize(wSect, hSect));
				GetIEditor()->GetHeightmap()->ImportSegmentModSectors(mem, rcSect);
			}
			pd->Applied(SDB_ModSectors);
		}
	}

	if (dwFlags & WS_EDITOR_TERRAIN) {
	//  CRGBLayer &TerrainRGBLayer = CTerrainManager::GetTerrainManager().GetHeightmap().m_TerrainRGBTexture;

	// variant 1: extremely slow
	//  float fWidth = (float) m_uiWidth;
	//  float fHeight = (float) m_uiHeight;
	//  float fLeft = sx / fWidth;
	//  float fTop = sy / fHeight;
	//  float fRight = (sx + 1) / fWidth;
	//  float fBottom = (sy + 1) / fHeight;
	//  const char *pcFN = GetSegmentFileName(sx, sy, "bmp");
	//  UINT32 dummy;
	//  TerrainRGBLayer.ImportExportBlock(pcFN, fLeft, fTop, fRight, fBottom, &dummy, true);

	// variant 2: works, but no longer needed
		//XmlNodeRef pRGBLayerNode = ar.root->findChild("RGBLayer");
		//if (pRGBLayerNode) {
		//  uint32 width = 0, height = 0;
		//  pRGBLayerNode->getAttr("width", width);
		//  pRGBLayerNode->getAttr("height", height);
		//  void *pData;
		//  int nSize;
		//  if (width > 0 && height > 0 && ar.pNamedData->GetDataBlock("RGBLayer", pData, nSize)) {
		//		CImage image;
		//    image.Attach((unsigned int *) pData, width, height);
		//    uint32 w = GetIEditor()->GetHeightmap()->GetWidth();
		//    uint32 h = GetIEditor()->GetHeightmap()->GetHeight();
		//    TerrainRGBLayer.SetSubImageStretched((float)rc.left/w, (float)rc.top/h, (float)rc.right/w, (float)rc.bottom/h, image);
		//  }
		//}
	}

	if (dwFlags & WS_EDITOR_OBJECTS) {
		if(pd->NeedsToBeAppliedLayers())
		{
			pd->LoadLayers(lc, arrObjects, oa);
			bUpdatedObjects = true;
		}
	}


	if ((dwFlags & WS_FINALIZE)) {
		if (bUpdatedTerrain)
			GetIEditor()->GetHeightmap()->UpdateEngineTerrain(ptPos.x, ptPos.y, SEGMENT_SIZE_UNITS, SEGMENT_SIZE_UNITS, true, true);

		//if (bUpdatedTerrain)
		//	if (dwFlags & WS_EDITOR_TERRAIN)
		//		GetIEditor()->GetHeightmap()->UpdateEngineTerrain(ptPos.x - 1, ptPos.y - 1, SEGMENT_SIZE_UNITS + 2, SEGMENT_SIZE_UNITS + 2, true, true);
		//	else
		//	  GetIEditor()->Get3DEngine()->GetITerrain()->Compile();
			
		if (bUpdatedObjects)
			oa.ResolveObjects();
	}

	//if ((dwFlags & WS_FINALIZE) && (dwFlags & WS_EDITOR_TERRAIN))
	//  GetIEditor()->GetHeightmap()->UpdateLayerTexture(rc);

	GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(false);

	//if ((dwFlags & WS_FINALIZE) && bUpdatedVegetation) //!!! to be removed
	//	GetIEditor()->GetHeightmap()->GetVegetationMap()->PlaceObjectsOnTerrain();
	
	return true;
}

bool CSegmentDataAggr::IntegrateSegment(  const TLocalCoords &lc, CBaseObjectsArray &arrObjects, int64 nVersion, sw::EVersionType vt /*= sw::VT_CURRENT*/ )
{
	ESaveFlag sflag = SF_IntegrateSave;
	assert( nVersion>0);

	AUTO_LOCK(g_cs);
	assert(!(GetIEditor()->GetSegmentedWorldManager()->IsOfflineMode() && sflag&SF_WithSubmit));
	TWorldCoords wc(lc);
	TSegmentData *pd = GetSegmentData(wc, false/*bool(sflag&SF_ForceSaveAll)*/);
	if (!pd)
		return false;

	// save layers
	pd->IntegrateLayers( lc, arrObjects, nVersion, vt);
	//pd->SaveLayers(lc, arrObjects, false, nVersion, vt);
	
	// save data blocks
	bool bModified = /*(sflag&SF_ForceSaveAll) ||*/ pd->IsModified();

	if (!bModified && pd->IsSaved(vt))
		return true;

	if (/*(vt != VT_HOT) &&*/ !LockSegment(lc))
		return !bModified;

	GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(true);

	pd->PrepareData(sflag,vt,lc);

	//assert( bool(sflag&SF_WithSubmit) == (nVersion > 0) );
	//if (sflag&SF_WithSubmit)
	//{
		pd->DoIntegrate( vt,nVersion);
	//}

	GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(false);

	return true;
}
bool CSegmentDataAggr::SaveSegment( ESaveFlag sflag, const TLocalCoords &lc, CBaseObjectsArray &arrObjects, int64 nVersion, sw::EVersionType vt /*= sw::VT_CURRENT*/ )
{
	assert( (sflag&SF_WithSubmit && nVersion>0) || (!(sflag&SF_WithSubmit) && nVersion<=0) );
	AUTO_LOCK(g_cs);
	//assert(!(s_nSWMode == 0 && sflag&SF_WithSubmit));
	TWorldCoords wc(lc);
	TSegmentData *pd = GetSegmentData(wc, sflag&SF_ForceSaveAll);
	if (!pd)
		return false;

	TLocalCoords tmplc(lc);
	if(sflag & SF_LevelCreation)
		tmplc.lx = tmplc.ly = 0;

	// save layers
	//pd->SaveLayers(lc, arrObjects, false, nVersion, vt);
	pd->SaveLayers(sflag, tmplc, arrObjects, nVersion, vt);
	
	// save data blocks
	bool bModified = (sflag&SF_ForceSaveAll) || pd->IsModified();

	if (!bModified && pd->IsSaved(vt))
		return true;

	if (/*(vt != VT_HOT) &&*/ !LockSegment(lc))
		return !bModified;

	GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(true);

	pd->PrepareData(sflag,vt,tmplc);

	assert( bool(sflag&SF_WithSubmit) == (nVersion > 0) );
	if (sflag&SF_WithSubmit)
	{
		pd->DoSaveToDB(sflag, vt,nVersion);
	}

	GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(false);

	return true;
}

void CSegmentDataAggr::ReloadSegmentLayerID(const TLocalCoords &lc)
{
	AUTO_LOCK(g_cs);
	TWorldCoords wc(lc);
	TSegmentData *pd = GetSegmentData(wc, true);
	if (!pd)
		return;

	PushSegmentData(pd);

	CPoint ptPos(lc.lx * SEGMENT_SIZE_UNITS, lc.ly * SEGMENT_SIZE_UNITS);
	CSize size(SEGMENT_SIZE_UNITS, SEGMENT_SIZE_UNITS);
	CRect rc(ptPos, size);

	CMemoryBlock mem;
	if (pd->GetData(SDB_LayerIDs, mem))
		GetIEditor()->GetHeightmap()->ImportSegmentLayerIDs(mem, rc);
	else
		GetIEditor()->GetHeightmap()->ClearSegmentLayerIDs(rc);
	pd->Applied(SDB_LayerIDs);

}

void CSegmentDataAggr::ReloadVegetationInstances(const TLocalCoords &lc)
{
	AUTO_LOCK(g_cs);
	TWorldCoords wc(lc);
	TSegmentData *pd = GetSegmentData(wc, true);
	if (!pd)
		return;

	PushSegmentData(pd);

	AABB bb = m_pWorld->CalcWorldBounds(lc);

	CPoint ptPos(lc.lx * SEGMENT_SIZE_UNITS, lc.ly * SEGMENT_SIZE_UNITS);
	CSize size(SEGMENT_SIZE_UNITS, SEGMENT_SIZE_UNITS);
	CRect rc(ptPos, size);

	CMemoryBlock mem;

	GetIEditor()->GetVegetationMap()->ClearSegment(bb);
	if (pd->GetData(SDB_Vegetation, mem)) 
	{
		Vec3 vOfs = GetIEditor()->GetHeightmap()->HmapToWorld(ptPos);
		GetIEditor()->GetVegetationMap()->ImportSegment(mem, vOfs);
	}
	pd->Applied(SDB_Vegetation);
}

TLayerData* CSegmentDataAggr::GetSegmentLayer(const TWorldCoords &wc, REFGUID guid, const char *szName)
{
	TSegmentData* pd = GetSegmentData(wc, false);
	if (!pd)
		return NULL;
	return pd->GetLayer(guid, szName);
}

TLayerData* CSegmentDataAggr::CreateSegmentLayer( const TWorldCoords &wc, REFGUID guid, const char *szName, bool* pbBrandNew /*= NULL*/, int nLayerId /*= -1*/ )
{
	TSegmentData* pd = GetSegmentData(wc, true, pbBrandNew);
	if(!pd)
		return NULL;
	return pd->CreateLayer(guid, szName, pbBrandNew, nLayerId);
}

TSegmentData *CSegmentDataAggr::GetSegmentData(const TWorldCoords &wc, bool bCreate, bool* pbBrandNew)
{
	if (pbBrandNew) *pbBrandNew = false;
	int id = MakeSegmentID(wc);
	AUTO_LOCK(g_cs);
	TSegmentDataMap::iterator it = m_mapSegmentData.find(id);
	if (it != m_mapSegmentData.end())
		return &(it->second);
	if (!bCreate)
		return 0;

	TSegmentData &d = m_mapSegmentData[id];
	d.Init(wc);

	if (pbBrandNew) *pbBrandNew = true;
	return &d;
}

void CSegmentDataAggr::UnloadSegmentData()
{
	m_mapSegLayer.clear();
	m_mapSegmentData.clear();
	m_pFirstSegmentData = NULL;
	m_pLastSegmentData = NULL;
	RefreshCasheSize();
}

void CSegmentDataAggr::PushSegmentData(TSegmentData *sd)
{
	if (sd == NULL)
		return;
	AUTO_LOCK(g_cs);

	if (m_pFirstSegmentData == NULL) {
		m_pFirstSegmentData = sd;
		m_pLastSegmentData = sd;
		sd->m_pPrev = NULL;
		sd->m_pNext = NULL;
		return;
	}
	if (m_pLastSegmentData == sd)
		return;
	//moving segment data to top of the list
	sd->RemoveFromList();
	sd->m_pPrev = m_pLastSegmentData;
	m_pLastSegmentData->m_pNext = sd;
	sd->m_pNext = NULL;
	m_pLastSegmentData = sd;

	// refreshing cached segmented world size
	RefreshCasheSize();
}

bool CSegmentDataAggr::CanUnloadSegmentData(TSegmentData *sd)
{
	if (!sd->IsSaved(sd->GetNeededVersionType()))
		return false;
	{
		AUTO_LOCK(g_cs);
		if (sd->m_wc.wx >= m_pWorld->m_wcOfs.wx && sd->m_wc.wx < m_pWorld->m_wcOfs.wx + m_pWorld->m_uiWidth &&
			sd->m_wc.wy >= m_pWorld->m_wcOfs.wy && sd->m_wc.wy < m_pWorld->m_wcOfs.wy + m_pWorld->m_uiHeight)
			return false;
	}
	if (!sd->IsLayersSaved(VT_COUNT))
		return false;
	return true;
}

void CSegmentDataAggr::UnloadSegmentData(TSegmentData *sd)
{
	if (sd == NULL)
		return;

	//clear segment data
	sd->Unload();
	sd->UnloadLayers();

	AUTO_LOCK(g_cs);
	int id = MakeSegmentID(sd->m_wc);
	TSegmentDataMap::iterator it = m_mapSegmentData.find(id);
	if (it != m_mapSegmentData.end())
		m_mapSegmentData.erase(it);

	sd = NULL;
	// refreshing cached segmented world size
	RefreshCasheSize();
}

void CSegmentDataAggr::RefreshCasheSize()
{
	AUTO_LOCK(g_cs);
	m_nCacheSize = 0;
	if (m_pFirstSegmentData == NULL)
		return;
	if (m_pLastSegmentData == NULL)
		return;
	TSegmentData *sd = m_pFirstSegmentData;
	while (sd != NULL) {
		sd->RefreshMemoryUsage();
		m_nCacheSize += sd->GetMemoryUsage();
		sd = sd->m_pNext;
	}
}

void CSegmentDataAggr::CleanCache(int iMaxMem)
{
	TSegmentData *sd = m_pFirstSegmentData;
	RefreshCasheSize();
	while (sd != NULL && m_nCacheSize > iMaxMem) {
		TSegmentData *sdNext = sd->m_pNext;
		if (CanUnloadSegmentData(sd))
			UnloadSegmentData(sd);
		sd = sdNext;
	}
}

void CSegmentDataAggr::UpdateLayersStatus()
{
	for (TSegmentDataMap::iterator it = m_mapSegmentData.begin(); it != m_mapSegmentData.end(); ++it) 
	{
		TSegmentData &sd = it->second;
		sd.UpdateLayersStatus();
	}
}

bool CSegmentDataAggr::IntegrateToDB(sw::EVersionType vt, int64 nVersion)
{
	bool bRes = true;

	for (TSegmentDataMap::iterator it = m_mapSegmentData.begin(); it != m_mapSegmentData.end(); ++it)
	{
		TSegmentData &sd = it->second;
		if (!m_pGlobal->IsSegmentSelected(sd.m_wc))
			continue;
		if (/*!(sflag&SF_ForceSaveAll) &&*/ !sd.IsModified() && sd.IsSaved(vt))
			continue;
		if (!sd.DoIntegrate(vt,nVersion))
			bRes = false;
	}

	for (TSegmentDataMap::iterator it = m_mapSegmentData.begin(); it != m_mapSegmentData.end(); ++it)
	{
		TSegmentData &sd = it->second;
		if (!m_pGlobal->IsSegmentSelected(sd.m_wc))
			continue;
		if (/*!(sflag&SF_ForceSaveAll) &&*/ !sd.IsLayersModified() && sd.IsLayersSaved(vt))
			continue;

		for (auto it2 = sd.m_layersMap.begin(); it2 != sd.m_layersMap.end(); ++it2)
		{

			TLayerData* pl = it2->second;

			if (/*!(sflag&SF_ForceSaveAll) &&*/ !pl->IsModified() && pl->IsSaved(vt))
				continue;

			assert(nVersion>0);
			pl->DoSaveToDB(SF_RegularSave,vt,nVersion);
		}
	}

	return bRes;
}
bool CSegmentDataAggr::SaveToDB(sw::ESaveFlag sflag, sw::EVersionType vt, int64 nVersion)
{
	bool bRes = true;

	for (TSegmentDataMap::iterator it = m_mapSegmentData.begin(); it != m_mapSegmentData.end(); ++it)
	{
		TSegmentData &sd = it->second;
		if (!m_pGlobal->IsSegmentSelected(sd.m_wc))
			continue;
		if (!(sflag&SF_ForceSaveAll) && !sd.IsModified() && sd.IsSaved(vt))
			continue;
		if (!sd.DoSaveToDB(sflag, vt,nVersion))
			bRes = false;
	}

	for (TSegmentDataMap::iterator it = m_mapSegmentData.begin(); it != m_mapSegmentData.end(); ++it)
	{
		TSegmentData &sd = it->second;
		if (!m_pGlobal->IsSegmentSelected(sd.m_wc))
			continue;
		if (!(sflag&SF_ForceSaveAll) && !sd.IsLayersModified() && sd.IsLayersSaved(vt))
			continue;

		for (auto it2 = sd.m_layersMap.begin(); it2 != sd.m_layersMap.end(); ++it2)
		{

			TLayerData* pl = it2->second;

			if (!(sflag&SF_ForceSaveAll) && !pl->IsModified() && pl->IsSaved(vt))
				continue;

			assert(nVersion>0);
			pl->DoSaveToDB(SF_RegularSave,vt,nVersion);
		}
	}

	return bRes;
}


void CSegmentDataAggr::SetModified(const TWorldCoords &wc, ESDBType eBlockType)
{
	AUTO_LOCK(g_cs);
	TSegmentData *pd = GetSegmentData(wc, true);
	TSegDataBlock *pb = pd->GetBlock(eBlockType, true);
	//pb->SetAppliedHRID(HRID_EDITOR);
	pb->GoToState(Versioned::S_Modified);
	m_pGlobal->OnSWModified();
}

void CSegmentDataAggr::SetModified(const TWorldCoords &wcMin, const TWorldCoords &wcMax, ESDBType eBlockType)
{
	TWorldCoords wc;
	for (wc.wy = wcMin.wy; wc.wy <= wcMax.wy; ++wc.wy) {
		for (wc.wx = wcMin.wx; wc.wx <= wcMax.wx; ++wc.wx) {
			SetModified(wc, eBlockType);
		}
	}
	//SWModified();
}

void CSegmentDataAggr::SetLayerDataModified(const TWorldCoords &wc, REFGUID guid, const char *strLayerName)
{
	AUTO_LOCK(g_cs);
	//TSegmentData *pd = GetSegmentData(wc, true);
	//TLayerData *pl = pd->CreateLayer(guid, strLayerName);
	bool bBrandNewLayer = false;
	TLayerData *pl = CreateSegmentLayer(wc, guid, strLayerName, &bBrandNewLayer );

	// brandnew layer starts as S_Init, so we need to forward the state to S_Applied
	if (bBrandNewLayer)
	{
		pl->GoToState(Versioned::S_Loaded);
		pl->GoToState(Versioned::S_Applied);
	}

	// If there are no applied version yet, should not apply EDITOR version by brute force.
	//if (pl->AppliedVersionIs(HRID_MISSING) || pl->AppliedVersionIs(HRID_UNKNOWN))
	//	;
	//else
	//	pl->SetAppliedHRID(HRID_EDITOR);
	pl->GoToState(Versioned::S_Modified);

	m_pGlobal->OnSWModified();
}

void CSegmentDataAggr::FetchAll(std::vector<TSegmentData*> &arrSegmentData)
{
	for(TSegmentDataMap::iterator it = m_mapSegmentData.begin(); it != m_mapSegmentData.end(); ++it)
	{
		TSegmentData *pSegData = &it->second;
		arrSegmentData.push_back(pSegData);
	}
}

bool CSegmentDataAggr::IsSegmentLocked(const TWorldCoords &wc)
{
	//AUTO_LOCK(g_cs);
	TSegmentData *pd = GetSegmentData(wc, true);
	pd->UpdateStatus();
	AUTO_LOCK(g_cs);
	//return pd->m_nLockedBy == g_db->GetUserID();
	return pd->IsLockedByMe();
}

int CSegmentDataAggr::LockSegment(const TWorldCoords &wc)
{
	TSegmentData *pd = GetSegmentData(wc, true);
	if (!pd)
		return 0;
	return pd->Lock();
}

bool CSegmentDataAggr::UnlockSegment(const TWorldCoords &wc)
{
	//AUTO_LOCK(g_cs);
	TSegmentData *pd = GetSegmentData(wc, true);
	if (!pd)
		return false;
	return pd->Unlock();
}

bool CSegmentDataAggr::IsSegmentLocked(const TWorldCoords &wcMin, const TWorldCoords &wcMax)
{
	TWorldCoords wc;
	for (wc.wy = wcMin.wy; wc.wy <= wcMax.wy; ++wc.wy) {
		for (wc.wx = wcMin.wx; wc.wx <= wcMax.wx; ++wc.wx) {
			if (!IsSegmentLocked(wc))
				return false;
		}
	}
	return true;
}

int CSegmentDataAggr::LockSegment(const TWorldCoords &wcMin, const TWorldCoords &wcMax, bool bIgnoreSelection)
{
	//TWorldCoordsArray arrLocked;
	int res = 1;
	TWorldCoords wc;
	for (wc.wy = wcMin.wy; wc.wy <= wcMax.wy; ++wc.wy) {
		for (wc.wx = wcMin.wx; wc.wx <= wcMax.wx; ++wc.wx) {
			if (!bIgnoreSelection && !m_pGlobal->IsSegmentSelected(wc))
				continue;
			int r = LockSegment(wc);
			if (!r) {
				//for (int i = 0; i < arrLocked.size(); ++i)
				//	UnlockSegment(arrLocked[i]);
				//return 0;
				res = 0;
			}
			if (r == 2 && res == 1) {
				res = 2;
				//arrLocked.push_back(wc);
			}
		}
	}
	return res;
}

bool CSegmentDataAggr::UnlockSegment(const TWorldCoords &wcMin, const TWorldCoords &wcMax)
{
	bool res = false;
	TWorldCoords wc;
	for (wc.wy = wcMin.wy; wc.wy <= wcMax.wy; ++wc.wy) {
		for (wc.wx = wcMin.wx; wc.wx <= wcMax.wx; ++wc.wx) {
			if (UnlockSegment(wc))
				res = true;
		}
	}
	return res;
}

bool  CSegmentDataAggr::LockSegmentLayersData(const TWorldCoords &wcMin, const TWorldCoords &wcMax, bool bIgnoreSelection)
{
	TWorldCoords wc;
	for (wc.wy = wcMin.wy; wc.wy <= wcMax.wy; ++wc.wy) 
	{
		for (wc.wx = wcMin.wx; wc.wx <= wcMax.wx; ++wc.wx) 
		{
			if (!bIgnoreSelection && !m_pGlobal->IsSegmentSelected(wc))
				continue;

			TSegmentData *pd = GetSegmentData(wc, true);
			if (!pd)
				return false;
			if (!pd->LockLayers())
				return false;
		}
	}
	return true;
}

int  CSegmentDataAggr::LockSegmentLayerData(const TWorldCoords &wc, REFGUID guid, const char *strLayerName)
{	
	TLayerData *pl = CreateSegmentLayer(wc, guid, strLayerName);

	assert(pl);
	if (!pl)
		return 0;
	return pl->Lock();
}

bool CSegmentDataAggr::UnlockSegmentLayerData(const TWorldCoords &wc, REFGUID guid, const char *strLayerName)
{
	TLayerData *pl = CreateSegmentLayer(wc, guid, strLayerName);

	if (!pl)
		return false;
	return pl->Unlock();
}

bool CSegmentDataAggr::LoadSegLayerMappingsFromXML(XmlNodeRef xmlSegLayerMappings, TSegLayerMap &mapSegLayer)
{
	if (!xmlSegLayerMappings || !xmlSegLayerMappings->isTag("SegLayerMappings"))
		return false;

	for(int i = 0; i < xmlSegLayerMappings->getChildCount(); i++)
	{
		XmlNodeRef seg = xmlSegLayerMappings->getChild(i);
		if(!seg->isTag("Segment"))
			continue;

		TSegLayerInfo segInfo;
		Vec2 tmp;
		seg->getAttr("wc", tmp);
		segInfo.wc.wx = tmp.x;
		segInfo.wc.wy = tmp.y;
		seg->getAttr("LayerGUID", segInfo.layerGUID);
		segInfo.layerName = seg->getAttr("LayerName");
		seg->getAttr("LayerID", segInfo.layerID);

		mapSegLayer.insert(std::make_pair((TSegLayerKey&)segInfo, segInfo));
	}

	return true;
}

bool CSegmentDataAggr::SaveSegLayerMappingsToXML(XmlNodeRef xmlSegLayerMappings, TSegLayerMap &mapSegLayer)
{
	bool bSucceed = true;
	for ( TSegLayerMap::const_iterator iter = mapSegLayer.begin(); iter != mapSegLayer.end(); ++iter )
	{
		TSegLayerInfo const& seglayerInfo = iter->second;

		XmlNodeRef seg = XmlHelpers::CreateXmlNode("Segment");
		if(!seg)
		{ bSucceed = false; break;}

		seg->setAttr("wc", Vec2(seglayerInfo.wc.wx, seglayerInfo.wc.wy));
		seg->setAttr("LayerGUID", seglayerInfo.layerGUID);
		seg->setAttr("LayerName", seglayerInfo.layerName);
		seg->setAttr("LayerID", seglayerInfo.layerID);

		xmlSegLayerMappings->addChild(seg);
	}

	return bSucceed;
}

bool CSegmentDataAggr::LoadSeLayerMapping(TWorldCoords const& wc)
{
	CString sXML = GetInternalSWDoc()->GetPersistence()->GetEditorDataPath(wc) + SW_SegmentInfoFile;
	
	if(!CFileUtil::FileExists(sXML))
		return false;

	XmlNodeRef xml = XmlHelpers::LoadXmlFromFile(sXML);
	if(!(xml && LoadSegLayerMappingsFromXML(xml, m_mapSegLayer)))
		return false;

	return true;
}

bool CSegmentDataAggr::SaveSegLayerMapping()
{
	m_mapSegLayer.clear();

	uint64 size;
	CString layerFileName;
	TPersistencePtr persist = GetInternalSWDoc()->GetPersistence();
	for (TSegmentDataMap::iterator it = m_mapSegmentData.begin(); it != m_mapSegmentData.end(); ++it)
	{
		TSegLayerMap segLayer;
		TSegmentData *pSegData = &it->second;

		for(auto il = pSegData->m_layersMap.begin(); il != pSegData->m_layersMap.end(); ++il)
		{
			TLayerData *pLayerData = il->second;
			TSegLayerInfo segLayerInfo;
			segLayerInfo.wc = pSegData->m_wc;
			segLayerInfo.layerGUID = pLayerData->m_LayerGUID;
			segLayerInfo.layerName = pLayerData->m_strName;
			segLayerInfo.layerID = pLayerData->m_nLayerID;

			// skip empty layer files
			layerFileName = pLayerData->GetFileName(*persist);
			CFileUtil::GetDiskFileSize(layerFileName, size);
			if(!size)
				continue;

			m_mapSegLayer.insert(std::make_pair( (TSegLayerKey&)segLayerInfo, segLayerInfo ));
			segLayer.insert(std::make_pair( (TSegLayerKey&)segLayerInfo, segLayerInfo ));
		}

		
		CString sXML = persist->GetEditorDataPath(pSegData->m_wc) + SW_SegmentInfoFile;
		XmlNodeRef xml = XmlHelpers::CreateXmlNode("SegLayerMappings");
		if(xml)
		{
			if(xml && SaveSegLayerMappingsToXML(xml, segLayer))
				xml->saveToFile(sXML);
		}
	}
	return true;
}

bool CSegmentDataAggr::SubmitAllSegments()
{
	bool ret = true;
	for ( TSegmentDataMap::iterator iter = m_mapSegmentData.begin(); iter != m_mapSegmentData.end(); ++iter)
	{
		ret = ret && iter->second.Submit();
	}

	return ret;
}

void CSegmentDataAggr::FetchSegDataBlocks( std::vector<TSegDataBlock*>& blocks, sw::ESDBType eBlockType, const Recti & worldSegRect )
{
	for ( TSegmentDataMap::iterator iter = m_mapSegmentData.begin(); iter != m_mapSegmentData.end(); ++iter)
	{
		TSegmentData& sd = iter->second;
		if (!worldSegRect.InRect(Vec2i(sd.m_wc.wx, sd.m_wc.wy)))
			continue;

		TSegDataBlock* p = sd.GetBlock(eBlockType,false);
		if (p)
			blocks.push_back(p);
	}
}

TSegDataBlock* CSegmentDataAggr::LoadIndependentDataBlock( CSCDataPersistence& persist, int x, int y, ESDBType eType )
{
	TSegDataBlock* pBlock = new TSegDataBlock(NULL, eType, TWorldCoords(x,y));



	if (!pBlock->Load( persist, HRID_HEADREV))
		return NULL;

	return pBlock;
}


SW_NAMESPACE_END();
