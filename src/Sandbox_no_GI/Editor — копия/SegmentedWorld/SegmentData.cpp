#include "StdAfx.h"
#include "InternalCommon.h"
#include "SegmentData.h"
#include "SegmentedWorldManager.h"
#include "SegmentedWorldDoc.h"
#include "LayerData.h"
#include "ISWDataPersistence.h"
#include "ISWVersionControl.h"

#include "CryMemBlock.h"
#include "Terrain/Heightmap.h"
#include "Objects/EntityObject.h"
#include "VegetationMap.h"
#include "Terrain/TerrainManager.h"
#include "GameExporter.h"					// for SaveDataForEngine


using namespace sw;	// without this line VA will confuse
SW_NAMESPACE_BEGIN();


bool TSegmentData::SavingFilterFunctor::Filter(CBaseObject const& obj, void* pRawContext)
{
	SavingFilterFunctor* pContext = (SavingFilterFunctor*)pRawContext;
	assert(pContext);

	CBaseObject const*po = &obj;
	if (po->GetParent())
		return false;

	if (po->GetGroup())
		return false;

	const Vec3& pt = po->GetPos();
	if (!ObjInBox(pContext->bounds, pt))
		return false;

	return true;
}

void TSegmentData::RebuildLayerRelations()
{
	for (TLayerMap::const_iterator iter = m_layersMap.begin(); iter != m_layersMap.end(); ++iter)
	{
		TLayerData *pLayer = iter->second;
		pLayer->m_bIsAParent = false;
	}
	for (TLayerMap::const_iterator iter = m_layersMap.begin(); iter != m_layersMap.end(); ++iter)
	{
		TLayerData *pLayer = iter->second;
		if (pLayer->HasParent())
			m_layersMap[pLayer->m_parentLayerGUID]->m_bIsAParent = true;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
TSegmentData::TSegmentData()
	:VersionedComposite( VDT_SEGTRUNK_COMP_HEAD
						,VerCap_Dynamic|VerCap_Lockable
						)
{
	assert((Versioned*)(TSegDataBlock*)0 == 0);
	VersionedComposite::InitVersion( reinterpret_cast<Versioned**>(m_Data+0),reinterpret_cast<Versioned**>(m_Data+SDB_COUNT));
	m_nSID = -1; // means not present in the engine
	for (int nBT = 0; nBT < SDB_COUNT; ++nBT)
		m_Data[nBT] = 0;

	m_pPrev = NULL;
	m_pNext = NULL;
	m_nMemoryUsage = 0;
	m_bNeedApplyLayers = false;
	UnloadLayers();
}

TSegmentData::TSegmentData( TSegmentData const& rhs )
	:VersionedComposite(rhs)
{
	this->Clone(rhs);
}
TSegmentData& TSegmentData::operator=(TSegmentData const& rhs)
{
	this->Clone(rhs);
	return *this;
}
void TSegmentData::Clone(TSegmentData const& rhs)
{
	*(TSuper*)this = rhs;

	//TDataBlock *m_Data[sw::SDB_COUNT];
	for (size_t ii = 0; ii < SDB_COUNT; ++ii)
		m_Data[ii] = rhs.m_Data[ii];

	m_wc = rhs.m_wc;
	m_nSID = rhs.m_nSID;
	m_pPrev = rhs.m_pPrev;
	m_pNext = rhs.m_pNext;
	m_nMemoryUsage = 0;			// should be recalculated later
	m_layersMap = rhs.m_layersMap;
	m_bNeedApplyLayers = rhs.m_bNeedApplyLayers;

	// we need to repoint the iterator pair when the object itself is being moved/copied
	SetChildIter((Versioned**)&m_Data[0],(Versioned**)&m_Data[SDB_COUNT]);
}

TSegmentData::~TSegmentData()
{	
	Unload();
	//removing from list
	RemoveFromList();

	UnloadLayers();
}

TSegDataBlock::TSegDataBlock(TParent* pParent, ESDBType blockType, TWorldCoords const& wc)
	:Versioned(VDT_SEGTRUNK,VerCap_AllBranches, blockType)
	,m_pParent(pParent)
{
	Versioned::InitVersion(true);
	if (!m_pParent) // if this is an independent DataBlock instance
	{
		m_lc.lx = wc.wx;
		m_lc.ly = wc.wy;
	}
	else
	{
		m_lc = wc;
	}
	m_eType = blockType;
}

TSegDataBlock *TSegmentData::GetBlock(ESDBType eBlockType, bool bCreate)
{
	TSegDataBlock *pb = m_Data[eBlockType];
	if (pb)
		return pb;
	if (!bCreate)
		return 0;
	AUTO_LOCK(g_cs);
	if (IsEngineDataBlock(eBlockType))
		pb = new TSegEngDataBlock(this,eBlockType,m_wc);
	else if (IsMinimapDataBlock(eBlockType))
		pb = new TSegMinimapDataBlock(this, eBlockType, m_wc);
	else
		pb = new TSegDataBlock(this,eBlockType,m_wc);

	m_Data[eBlockType] = pb;
	return pb;
}

bool TSegmentData::GetData(ESDBType eBlockType, CMemoryBlock &mem, TLocalCoords *plc)
{
	TSegDataBlock *pb = GetBlock(eBlockType, true);
	if (!pb)
		return false;
	//int nLoadedHRID;
	//{
	//	AUTO_LOCK(g_cs);
	//	nLoadedHRID = pb->m_nLoadedHRID;
	//}
	//if (nLoadedHRID != HRID_CACHE) 
	if (!pb->IsLoadedDataACache())
	{
		EVersionType vt = GetNeededVersionType();
		//int nHRID;
		//{
		//	AUTO_LOCK(g_cs);
		//	nHRID = pb->m_nHRIDs[vt];
		//}
		//if (nHRID == HRID_UNKNOWN) 
		if (pb->CheckVersion(vt,HRID_UNKNOWN))
		{
			UpdateStatus();
		}

		TWorldCoords wc;
		{
			AUTO_LOCK(g_cs);
			//nHRID = pb->m_nHRIDs[vt];
			wc = m_wc;
		}
		if (!pb->LoadByVT(*GetInternalSWDoc()->GetPersistence(), vt))
			return false;
	}
	AUTO_LOCK(g_cs);
	if (plc)
		*plc = pb->m_lc;
	if (pb->IsDataEmpty())
		return false;
	pb->GetData(mem);
	return true;
}

EVersionType TSegmentData::GetNeededVersionType()
{
	CSWDoc *psw = GetInternalSWDoc();
	if (true/*psw->GetVersionType() != VT_OFFICIAL*/)
		return psw->GetVersionType();
	AUTO_LOCK(g_cs);
	if (!Versioned::IsLockedByMe())
		return psw->GetVersionType();
	return VT_CURRENT;
}

bool TSegDataBlock::Load( CSCDataPersistence& persist, int64 nHRID)
{
	//#if !defined(USE_LOCAL_SAVE) || defined(USE_NATIVE_VC) || defined(USE_NULL_VC)
	{
		AUTO_LOCK(g_cs);
		if (Versioned::LoadedVersionIs(nHRID))
		{
			// If it's from some reloading states, even if we don't need to load anything, we still need to set it to `loaded'
			GoToStateAtLeast(S_Loaded);
			return true;
		}
	}
	if (nHRID < 0) 
	{
		assert(!"Invalid version requested");
		return false;
	}
	if (nHRID == HRID_MISSING) 
	{
		Unload();
		return true;
	}

	TArgStack arrAdditional;
	if (!persist.LoadRevision(*this,nHRID,m_data,arrAdditional))
	{
		Unload();
		return false;
	}

#ifndef SW_USE_INTEGRATED_METADATA
	//additional code
	bool bCorrupted = false;
	int nCol = 0;
	bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(m_lc.lx);
	bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(m_lc.ly);
	int nBlockType;
	bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(nBlockType) || nBlockType < 0 || nBlockType >= SDB_COUNT || nBlockType != m_eType;

	if (bCorrupted)
	{
		assert(!"Database corrupted!");
		Unload();
		return false; 
	}
#endif
	{
		TLocalCoords const& lc = m_lc;
		char chLog[256]; sprintf(chLog, "Loaded (%d,%d):%s(HRID %I64d, size %d)\n", lc.lx, lc.ly, GetSegDataBlockName(m_eType), nHRID, m_data.GetUncompressedSize());
		SWLog(chLog);
	}

	//m_nLoadedHRID = nHRID;
	Versioned::OnVersionLoaded(nHRID);

	return true;
}

bool TSegmentData::CreateData()
{
	CSWDoc *psw = GetInternalSWDoc();
	{
		int idSeg;
		TWorldCoords wc;
		{
			AUTO_LOCK(g_cs);
			wc = m_wc;
			idSeg = MakeSegmentID(m_wc);
		}

		// TODO: need transaction here. -RL
		// TRANSACTION_BEGIN()
		TArgStack args;
		args.push_back(wc.wx);
		args.push_back(wc.wy);
		bool bRet = psw->GetPersistence()->Add(*this,&args);
		assert(bRet);

		for (int ii=0;ii<SDB_COUNT; ++ii )
		{
			m_Data[ii]->CreateData();
		}
		// TRANSACTION_END()
	}
	return true;
}

bool TSegDataBlock::SaveData( int64 nVersion, EVersionType vt)
{
	TParent& parent = *m_pParent;
	TWorldCoords& wc = parent.m_wc;
	//assert(s_nSWMode != 0);
	CSWDoc *psw = GetInternalSWDoc();

	if(/*bNeedCreate*/IsMissingInPersist())
		if (!CreateData())
		{
			assert(0);
			return false;
		}

		TArgStack arrAdditionalSave;
		{
			AUTO_LOCK(g_cs);
			arrAdditionalSave.push_back(wc.wx);
			arrAdditionalSave.push_back(wc.wy);
			arrAdditionalSave.push_back(m_lc.lx);
			arrAdditionalSave.push_back(m_lc.ly);
		}

		return psw->GetPersistence()->CheckoutSaveSubmit(*this,m_data,nVersion,vt,arrAdditionalSave);
}

bool TSegDataBlock::DoIntegrate( EVersionType vt, int64 nVersion)
{
	//assert(s_nSWMode != 0 || CSegmentedWorld::IsMapConverting());
	CSWDoc *psw = GetInternalSWDoc();
	bool bModified;
	if (!psw->IsBlockTypeNeeded(this->m_eType) && LoadedVersionIs(HRID_MISSING))
	{
		// if it's a not needed block, just update versions to current type version (???)
		bModified = UpdateVersions(vt,GetVersion(psw->GetVersionType())); // TODO: improve it -RL
	}
	else // otherwise do it normally
		bModified = UpdateVersionsToLoaded(vt);

	if (bModified)
	{
		bool bOK = InsertLoadedVersionIntoHistory( vt, nVersion);
		assert(bOK);
		bOK = UpdateRevHead(vt);
		assert(bOK);
		return true;
	}

	return true; //< return true if succeeded
}

bool TSegDataBlock::DoSaveToDB(ESaveFlag sflag, EVersionType vt, int64 nVersion)
{
	//assert(s_nSWMode != 0 || CSegmentedWorld::IsMapConverting());
	CSWDoc *psw = GetInternalSWDoc();

	if (Versioned::IsLoadedDataACache()) 
	{	
		return Versioned::DoSave(sflag, vt, nVersion);
	}
	assert(!(sflag&SF_AsIntegrate));

	return true; //< return true if succeeded
}
//----------------------------------------------------
bool TSegmentData::DoIntegrate(EVersionType vt, int64 nVersion)
{
	//assert(s_nSWMode != 0 || CSegmentedWorld::IsMapConverting());
	CSWDoc *psw = GetInternalSWDoc();

	Unlock();

	assert(!m_bMissing);

	bool bRes = true;
	for (int nBT = 0; nBT < SDB_COUNT; ++nBT)
	{
		TSegDataBlock *pb = m_Data[nBT];
		if (!pb)
			continue;

		bRes = bRes && pb->DoIntegrate( vt, nVersion );
	}

	return bRes;
}

bool TSegmentData::DoSaveToDB(ESaveFlag sflag, EVersionType vt, int64 nVersion)
{
	//assert(s_nSWMode != 0 || CSegmentedWorld::IsMapConverting());
	CSWDoc *psw = GetInternalSWDoc();

	bool bMissing;
	{
		AUTO_LOCK(g_cs);
		bMissing = m_bMissing;
	}
	if (bMissing) 
	{
		CreateData();
		//AUTO_LOCK(g_cs);
		//m_bMissing = false;
	}

	bool bRes = true;
	//int nModified[SDB_COUNT + 1];
	//int cntModified = 0;
	for (int nBT = 0; nBT < SDB_COUNT; ++nBT)
	{
		TSegDataBlock *pb = m_Data[nBT];
		if (!pb)
			continue;

		bRes = bRes && pb->DoSaveToDB(sflag, vt, nVersion );
	}

	// ==== save data structure version for seg ====
	// should call saveData directly because it's just struct-version not version-controled data
	//Versioned::DoSave(sflag,vt,nVersion);
	SaveData(nVersion,vt);

	return bRes;
}

//-----------------------------------------------------------
bool TSegmentData::UpdateRevHeads(EVersionType vt, int pnBlockTypes[])
{
	bool bRet = true;

	// TODO: need transaction here. -RL
	// TRANSACTION_BEGIN()
	for (int* pBType = pnBlockTypes; *pBType != SDB_COUNT ; ++pBType)
	{
		TSegDataBlock* pb = m_Data[*pBType];
		assert(pb);
		bRet = bRet && pb->UpdateRevHead(vt);
	}
	// TRANSACTION_END()

	if (bRet)
	{
		TWorldCoords wc;
		{
			AUTO_LOCK(g_cs);
			wc = m_wc;
		}
		char chLog[256]; sprintf(chLog, "Saved (%d,%d) segment HRIDs\n", wc.wx, wc.wy);
		SWLog(chLog);
	}

	return bRet;
}


void TSegDataBlock::Unload()
{
	AUTO_LOCK(g_cs);
	//m_nLoadedHRID = HRID_MISSING;
	Versioned::Reset();
	m_data.Free();
}

void TSegmentData::Unload()
{
	AUTO_LOCK(g_cs);
	m_bMissing = true;
	m_nLockedBy = -1; // means not updated
	for (int nBT = 0; nBT < SDB_COUNT; ++nBT) 
	{
		delete m_Data[nBT];
		m_Data[nBT] = 0;
	}
}


void TSegDataBlock::AppliedLocally(const TLocalCoords &lc)
{
	AUTO_LOCK(g_cs);
	m_lc = lc;
	OnAppliedLocally();
}


bool TSegDataBlock::CreateData()
{
	return GetInternalSWDoc()->GetPersistence()->Add(*this);
}

TArgStack TSegDataBlock::MakePrimaryKey() const
{
	TArgStack arrPrimaryKey = m_pParent->MakePrimaryKey();

	arrPrimaryKey.push_back(m_eType);
	return arrPrimaryKey;
}

bool TSegmentData::NeedsReload()
{
	CSegmentedWorldDoc* pSW = GetInternalSWDoc();
	EVersionType vt = GetNeededVersionType();
	for (int nBT = 0; nBT < SDB_COUNT; ++nBT)
	{
		if (!GetInternalSWDoc()->IsBlockTypeNeeded((ESDBType)nBT))
			continue;
		TSegDataBlock *pb = m_Data[nBT];
		if (!pb)
			continue;
		if (pb->NeedsReload())
			return true;
	}
	return false;
}

void TSegmentData::Reload()
{
	EVersionType vt = GetNeededVersionType();
	for (int nBT = 0; nBT < SDB_COUNT; ++nBT)
	{
		TSegDataBlock *pb = m_Data[nBT];
		if (!pb)
			continue;
		if (pb->IsModified())
			continue;

		AUTO_LOCK(g_cs);
		if (pb->LoadedVersionIsVT(vt))
			continue;
		assert( !pb->IsLockedByMe()/*m_nLockedBy != g_db->GetUserID()*/ || GetInternalSWDoc()->IsHot());
		pb->LoadByVT(*GetInternalSWDoc()->GetPersistence(), vt);
	}
}

bool TSegmentData::IsModified(int nBlockType)
{
	if ( nBlockType == -1)
	{
		for (int nBT = 0; nBT < SDB_COUNT; ++nBT) 
		{
			TSegDataBlock *pb = m_Data[nBT];
			if (!pb)
				continue;
			if (pb->IsModified())
				return true;
		}
	} 
	else
	{
		TSegDataBlock *pb = m_Data[nBlockType];
		if (!pb)
			return false;
		if (pb->IsModified())
			return true;
	}
	return false;
}

bool TSegmentData::IsSaved(EVersionType vt)
{
	for (int nBT = 0; nBT < SDB_COUNT; ++nBT) 
	{
		TSegDataBlock *pb = m_Data[nBT];
		if (!pb)
			continue;
		if (!pb->IsSaved(vt))
			return false;
	}
	return true;
}


bool TSegmentData::NeedsToBeApplied(ESDBType eBlockType)
{
	if (!GetInternalSWDoc()->IsBlockTypeNeeded(eBlockType))
		return false;
	TSegDataBlock *pb = GetBlock(eBlockType, false);
	if (!pb)
		return false;
	AUTO_LOCK(g_cs);
	//if (pb->AppliedVersionIs(HRID_UNKNOWN))
	if (pb->StateEqualTo(S_Loaded))
		return true;
	//if (pb->AppliedVersionIs(HRID_EDITOR))
	if (pb->StateEqualTo(S_Modified))
		return false;
	if (pb->IsLoadedVersionApplied())
		return false;
	return true;
}

bool TSegmentData::NeedsToBeApplied()
{
	for (int nBT = 0; nBT < SDB_COUNT; ++nBT)
	{
		if (NeedsToBeApplied((ESDBType) nBT))
			return true;
	}
	return false;
}

bool TSegmentData::NeedToBeFastApplied()
{
	for (int nBT = SDB_SOURCEDATA_BEGIN; nBT != SDB_SOURCEDATA_END; ++nBT)
	{
		if (NeedsToBeApplied((ESDBType) nBT))
			return true;
	}
	return false;
}

void TSegmentData::Applied(ESDBType eBlockType)
{
	TSegDataBlock *pb = GetBlock(eBlockType, true);
	if (!pb)
		return;
	AUTO_LOCK(g_cs);
	//pb->SetAppliedHRID(pb->m_nLoadedHRID);
	pb->ApplyLoadedVersion();
	char chLog[256]; sprintf(chLog, "Applied (%d,%d):%s(HRID %I64d)\n", m_wc.wx, m_wc.wy, GetSegDataBlockName(eBlockType), DataInspector::m_nLoadedHRID(pb));
	SWLog(chLog);
}

void TSegmentData::NoLongerApplied(ESDBType eBlockType)
{
	TSegDataBlock *pb = GetBlock(eBlockType, true);
	if (!pb)
		return;
	AUTO_LOCK(g_cs);
	//pb->SetAppliedHRID(HRID_UNKNOWN);
	pb->NoLongerApplied();
}

void TSegmentData::NoLongerApplied()
{
	for (int nBT = SDB_SOURCEDATA_BEGIN; nBT != SDB_SOURCEDATA_END; ++nBT) 
	{
		TSegDataBlock *pb = m_Data[nBT];
		if (!pb)
			continue;
		AUTO_LOCK(g_cs);
		//pb->SetAppliedHRID(HRID_UNKNOWN);
		if (pb->IsStateAtLeast(S_Applied))
			pb->GoToState(S_Unapplied);
	}

	NoLongerAppliedLayers();
}

void TSegmentData::RefreshMemoryUsage()
{
	m_nMemoryUsage = 0;
	for (int nBT = 0; nBT < SDB_COUNT; ++nBT) 
	{
		TSegDataBlock *pb = m_Data[nBT];
		if (!pb)
			continue;
		AUTO_LOCK(g_cs);
		m_nMemoryUsage += pb->GetMemoryUsage();
	}

	AUTO_LOCK(g_cs);
	for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
	{
		TLayerData *pl = it->second;
		m_nMemoryUsage += pl->GetMemoryUsage();
	}
}

void TSegmentData::RemoveFromList()
{
	AUTO_LOCK(g_cs);
	CSWDoc *psw = GetInternalSWDoc();

	if (m_pPrev != NULL)
		m_pPrev->m_pNext = m_pNext;
	if (m_pNext != NULL)
		m_pNext->m_pPrev = m_pPrev;
	if (this == psw->SegmentAggr().m_pFirstSegmentData)
		psw->SegmentAggr().m_pFirstSegmentData = m_pNext;
	if (this == psw->SegmentAggr().m_pLastSegmentData)
		psw->SegmentAggr().m_pLastSegmentData = m_pPrev;
	m_pNext = NULL;
	m_pPrev = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// interface for Layers Data
//////////////////////////////////////////////////////////////////////////////////////////
void TSegmentData::UnloadLayers()
{
	AUTO_LOCK(g_cs);
	for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end();)
	{
		TLayerMap::iterator itCur = it; ++it;
		TLayerData* pl = itCur->second;
		m_layersMap.erase(itCur);
		delete pl;
	}
	m_layersMap.clear();
}
//---------------------------------------------------------------------------------------
TLayerData * TSegmentData::GetLayer( REFGUID guid, const char *strLayerName /*= NULL*/ )
{
	assert(guid != GUID_NULL);
	AUTO_LOCK(g_cs);
	TLayerMap::iterator it = m_layersMap.find(guid);
	if (it != m_layersMap.end())
	{
		TLayerData* pl = it->second;
		assert(pl);
		assert( !strLayerName || 0 == strcmpi( DataInspector::m_strName(pl).c_str(), strLayerName ) );
		return pl;
	}

	return 0;
}

TLayerData *TSegmentData::CreateLayer(REFGUID guid, const char *szName, bool* pbBrandNew, int nLayerId)
{
	if (pbBrandNew) *pbBrandNew = false;
	TLayerData* pl = GetLayer(guid,szName);
	if (pl)
		return pl;

	//// query layerid right at the time of creation, avoiding layerid==-1 condition
	//// TODO: BUT this is NOT cooperative-editing-safe, we should have temporary layerid (<0)
	////       and id merge mechanism (like link-time work)
	//assert(g_db->GetCachedConn());
	//int nLayerId = GetInternalSWDoc()->LayerIDMgr().ObjectLayersInfo_GetID(guid);
	//if (nLayerId<0)
	//{
	//	nLayerId = GetInternalSWDoc()->LayerIDMgr().RegisterNewLayer(guid,strLayerName);
	//	assert(nLayerId>0);
	//}

	pl = new TLayerData(guid,szName,m_wc,m_wc);
	if (nLayerId > 0)
		pl->m_nLayerID = nLayerId;
	m_layersMap.insert(TLayerPair(guid, pl));
	if (pbBrandNew) *pbBrandNew = true;
	return pl;
}
//---------------------------------------------------------------------------------------
bool TSegmentData::LoadLayerData(TLayerData *pl, CMemoryBlock &mem, TLocalCoords *plc)
{
	if (!pl)
		return false;
	bool bCanLoad;
	{
		AUTO_LOCK(g_cs);
		bCanLoad = !pl->IsLoadedDataACache();
	}
	if (bCanLoad) 
	{
		EVersionType vt = pl->GetNeededVersionType();
		int nHRID;
		TWorldCoords wc;
		{
			AUTO_LOCK(g_cs);
			nHRID = pl->GetVersion(vt);
			wc = m_wc;
		}
		if (nHRID == HRID_UNKNOWN) 
		{
			UpdateLayersStatus();
			AUTO_LOCK(g_cs);
			nHRID = pl->GetVersion(vt);
			wc = m_wc;
		}
		if (!pl->Load(wc, nHRID))
			return false;
	}
	AUTO_LOCK(g_cs);
	if (plc)
		*plc = pl->m_lc;
	if (pl->IsDataEmpty())
		return false;
	pl->GetData(mem);
	return true;
}
//-------------------------------------------------------------------------------------
void TSegmentData::UpdateLayersStatus()
{
	CSWDoc *psw = GetInternalSWDoc();
	typedef std::vector<GUID> TGuidArray;
	TGuidArray arrExistingLayers;

	CSegmentDataAggr::TSegLayerMap& seglayerMap = psw->SegmentAggr().m_mapSegLayer;
	if(seglayerMap.empty())
	{
		SetAllLayersAsMissing();
		return;
	}

	for (CSegmentDataAggr::TSegLayerMap::iterator iter = seglayerMap.begin(); iter != seglayerMap.end(); ++iter )
	{
		TSegLayerInfo *pSegInfo = &iter->second;

		if(m_wc != pSegInfo->wc)
			continue;

		assert(pSegInfo->layerGUID != GUID_NULL);

		{
			AUTO_LOCK(g_cs);
			TLayerData *pl = CreateLayer(pSegInfo->layerGUID, pSegInfo->layerName,NULL, pSegInfo->layerID);
			if (!pl->UpdateStatus())
				assert(0);
		}

		arrExistingLayers.push_back(pSegInfo->layerGUID);
	}

	if(!arrExistingLayers.empty())
	{
		assert(std::is_sorted(arrExistingLayers.begin(),arrExistingLayers.end(),guid_less_predicate()));

		TGuidArray arrOwnLayerGUIDs;
		for ( TLayerMap::const_iterator iter = m_layersMap.begin(); iter != m_layersMap.end(); ++iter)
			arrOwnLayerGUIDs.push_back(iter->first);

		assert(std::is_sorted(arrOwnLayerGUIDs.begin(),arrOwnLayerGUIDs.end(),guid_less_predicate()));
		TGuidArray arrLayersToBeDelete(max(arrOwnLayerGUIDs.size(),arrExistingLayers.size()));
		TGuidArray::const_iterator iterEndToBeDelete
			= std::set_difference(arrOwnLayerGUIDs.begin(),arrOwnLayerGUIDs.end()
								,arrExistingLayers.begin(),arrExistingLayers.end()
								,arrLayersToBeDelete.begin(), guid_less_predicate());

		for (std::vector<GUID>::const_iterator iter = arrLayersToBeDelete.begin(); iter != iterEndToBeDelete; ++iter)
		{
			m_layersMap[*iter]->SetAsMissingInPersist();
		}
		return;
	}
}
//---------------------------------------------------------------------------------------
bool TSegmentData::IsLayersModified()
{
	AUTO_LOCK(g_cs);
	for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
	{
		TLayerData* pl = it->second;
		if (pl->IsModified())
			return true;
	}
	return false;
}
//---------------------------------------------------------------------------------------
bool TSegmentData::IsLayersSaved(EVersionType vt)
{
	bool bGetNeededVT = (vt == VT_COUNT);
	AUTO_LOCK(g_cs);
	for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
	{
		TLayerData* pl = it->second;
		if (bGetNeededVT)
			vt = pl->GetNeededVersionType();
		if (!pl->IsSaved(vt))
			return false;
	}
	return true;
}
//---------------------------------------------------------------------------------------
void TSegmentData::ClearUnusedLayers()
{
	AUTO_LOCK(g_cs);

	// TODO: UNDER CONSTRUCTION -RL
	//RebuildLayerRelations();

	for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end();)
	{
		TLayerData *pl = it->second;
		if(pl->IsMissingInPersist() && !pl->IsModified() && !pl->IsAParent())
		{
			pl->IsAParent();
			TLayerMap::iterator itCur = it; ++it;
			m_layersMap.erase(itCur);
			delete pl;
		}
		else
			it++;
	}
}
//---------------------------------------------------------------------------------------
bool TSegmentData::IntegrateLayers( const TLocalCoords &lc, CBaseObjectsArray &arrObjects, int64 nVersion, EVersionType vt)
{
	ESaveFlag sflag = SF_IntegrateSave;
	//assert(vt == VT_OFFICIAL);
	assert( (sflag&SF_WithSubmit && nVersion>0) || (!(sflag&SF_WithSubmit) && nVersion<=0) );
	//AUTO_LOCK(g_cs);
	assert(!(CSWManager::Get().IsOfflineMode() && nVersion != 0));
	CSWDoc *psw = GetInternalSWDoc();
	typedef std::vector<CObjectLayer*> TLayersArray;
	TLayersArray layers;

	GetIEditor()->GetObjectManager()->GetLayersManager()->GetLayers( layers );


	for (int j = 0; j < layers.size(); j++)
	{
		CObjectLayer* objLayer = layers[j];
		string strLayerName = objLayer->GetName();
		if (strLayerName.empty())
		{
			assert(0);
			//continue;
		}
		GUID guid = objLayer->GetGUID();
		TLayerData* pl = CreateLayer(guid, strLayerName);
	}

	UpdateLayersStatus();

	bool bModified = /*(sflag&SF_ForceSaveAll) ||*/ IsLayersModified();

	//vm fix it!!!
	if (!bModified && IsLayersSaved(vt))
	{
		ClearUnusedLayers();
		return true;
	}

	bool bIsSWActive = GetIEditor()->Get3DEngine()->IsSegmentedWorldActive();
	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);

	TLayerMap clonedLayerMap;
	{
		AUTO_LOCK(g_cs);
		for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
			clonedLayerMap[it->first]=it->second;
	}
	bool bIsNeedToSaveEngData = false;
	for (TLayerMap::iterator it = clonedLayerMap.begin(); it != clonedLayerMap.end(); ++it)
	{

		TLayerData* pl = it->second;

		if (!bModified && pl->IsSaved(vt))
			continue;

		//if ((vt != VT_HOT) && !pl->Lock())
		//	continue;
		//if (pl->SaveLayer(sflag, lc, arrObjects, nVersion, vt))
		if (pl->PrepareData( sflag, lc, arrObjects) && pl->DoIntegrate( vt, nVersion))
			bIsNeedToSaveEngData = true;
	}

	ClearUnusedLayers();

	//Saving engine data
	if (bIsNeedToSaveEngData)
	{

		bool bSaveToEng = false;
		// if is marked as modified DataBlock will be saved in SaveSegment
		{
			AUTO_LOCK(g_cs);
			TSegDataBlock *pb;
			pb = GetBlock(SDB_EngData, true);
			if (pb && m_nSID >= 0 /*&& !(sflag&SF_ForceSaveAll)*/
				//&& !pb->AppliedVersionIs(HRID_EDITOR)
				&& !pb->StateEqualTo(S_Modified)
				)
			{	
				//pb->SetAppliedHRID(HRID_EDITOR);
				pb->GoToState(S_Modified);
				bSaveToEng = true;
			}

			pb = GetBlock(SDB_VisAreas, true);
			if (pb && m_nSID >= 0 /*&& !(sflag&SF_ForceSaveAll)*/ 
				//&& !pb->AppliedVersionIs(HRID_EDITOR)
				&& !pb->StateEqualTo(S_Modified)
				)
			{	
				//pb->SetAppliedHRID(HRID_EDITOR);
				pb->GoToState(S_Modified);
				bSaveToEng = true;
			}

			pb = GetBlock(SDB_EngEntities, true);
			if (pb /*&& !(sflag&SF_ForceSaveAll)*/ 
				//&& !pb->AppliedVersionIs(HRID_EDITOR)
				&& !pb->StateEqualTo(S_Modified)
				)
			{
				//pb->SetAppliedHRID(HRID_EDITOR);
				pb->GoToState(S_Modified);
				bSaveToEng = true;
			}
		}

		if(bSaveToEng)
		{
			// force save to db
			//SaveDataForEngine(sflag, lc, nVersion, vt, /*bSaveToDB*/true);
			//assert(sflag&SF_WithSubmit);
			SaveDataForEngine(sflag, lc, nVersion, vt);
		}
	}

	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(bIsSWActive);
	return true;
}
bool TSegmentData::SaveLayers(ESaveFlag sflag, const TLocalCoords &lc, CBaseObjectsArray &arrObjects, int64 nVersion, EVersionType vt)
{
	assert( (sflag&SF_WithSubmit && nVersion>0) || (!(sflag&SF_WithSubmit) && nVersion<=0) );
	//AUTO_LOCK(g_cs);
	CSWDoc *psw = GetInternalSWDoc();
	typedef std::vector<CObjectLayer*> TLayersArray;
	TLayersArray layers;

	GetIEditor()->GetObjectManager()->GetLayersManager()->GetLayers( layers );


	for (int j = 0; j < layers.size(); j++)
	{
		CObjectLayer* objLayer = layers[j];
		string strLayerName = objLayer->GetName();
		if (strLayerName.empty())
		{
			assert(0);
			//continue;
		}
		GUID guid = objLayer->GetGUID();
		TLayerData* pl = CreateLayer(guid, strLayerName);

		// fill-in parent info
		if (CObjectLayer* pParent = objLayer->GetParent())
			pl->m_parentLayerGUID = pParent->GetGUID();
	}

	// rebuild is-a-parent mark of all LayerDatas
	RebuildLayerRelations();

	UpdateLayersStatus();

	bool bModified = (sflag&SF_ForceSaveAll) || IsLayersModified();

	//vm fix it!!!
	if (!bModified && IsLayersSaved(vt))
	{
		ClearUnusedLayers();
		return true;
	}

	bool bIsSWActive = GetIEditor()->Get3DEngine()->IsSegmentedWorldActive();
	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);

	std::vector<TLayerData*> clonedLayerMap;
	{
		AUTO_LOCK(g_cs);
		clonedLayerMap.reserve(m_layersMap.size());
		uint ii = 0;
		for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
			clonedLayerMap.push_back(it->second);
	}
	
	CBaseObjectsArray objsInSeg;
	SavingFilterFunctor filter(psw->CalcWorldBounds(lc));
	GetIEditor()->GetObjectManager()->GetObjects(objsInSeg,filter.GetAsBaseFilter());

	bool bIsNeedToSaveEngData = false;
	for (std::vector<TLayerData*>::iterator it = clonedLayerMap.begin(); it != clonedLayerMap.end(); ++it)
	{
		TLayerData* pl = *it;

		if (!bModified && pl->IsSaved(vt))
			continue;

		//if ((vt != VT_HOT) && !pl->Lock())
		//	continue;
		if (pl->SaveLayer(sflag, lc, objsInSeg, nVersion, vt))
			bIsNeedToSaveEngData = true;
	}

	ClearUnusedLayers();

	//Saving engine data
	if (bIsNeedToSaveEngData)
	{

		bool bSaveToEng = false;
		// if is marked as modified DataBlock will be saved in SaveSegment
		{
			AUTO_LOCK(g_cs);
			TSegDataBlock *pb;
			pb = GetBlock(SDB_EngData, true);
			if (pb && m_nSID >= 0 && !(sflag&SF_ForceSaveAll) 
				//&& !pb->AppliedVersionIs(HRID_EDITOR)
				&& !pb->StateEqualTo(S_Modified)
				)
			{	
				//pb->SetAppliedHRID(HRID_EDITOR);
				pb->GoToState(S_Modified);
				bSaveToEng = true;
			}

			pb = GetBlock(SDB_VisAreas, true);
			if (pb && m_nSID >= 0 && !(sflag&SF_ForceSaveAll) 
				//&& !pb->AppliedVersionIs(HRID_EDITOR)
				&& !pb->StateEqualTo(S_Modified)
				)
			{	
				//pb->SetAppliedHRID(HRID_EDITOR);
				pb->GoToState(S_Modified);
				bSaveToEng = true;
			}

			pb = GetBlock(SDB_EngEntities, true);
			if (pb && !(sflag&SF_ForceSaveAll) 
				//&& !pb->AppliedVersionIs(HRID_EDITOR)
				&& !pb->StateEqualTo(S_Modified)
				)
			{
				//pb->SetAppliedHRID(HRID_EDITOR);
				pb->GoToState(S_Modified);
				bSaveToEng = true;
			}
		}
	}

	GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(bIsSWActive);
	return true;
}
//---------------------------------------------------------------------------------------
void TSegmentData::SaveDataForEngine(ESaveFlag sflag, const TLocalCoords &lc, int64 nVersion, EVersionType vt)
{
	sflag &= ~SF_AsIntegrate; //< always perform as save, not integrate, the flag is ignored.

	assert( (sflag&SF_WithSubmit && nVersion>0) || (!(sflag&SF_WithSubmit) && nVersion<=0) );
	//if (CSWManager::Get().IsOfflineMode()) return;
	CSWDoc *psw = GetInternalSWDoc();
	AABB bb = CSegmentedWorldDoc::CalcWorldBounds(lc);
	Vec3 segmentOffset = -Vec3(bb.min.x, bb.min.y, 0);

	if (sflag&SF_ForceSaveAll)
	{
		// dirty all
		for (ESDBTypeIterator ii = SDB_ENGINEDATA_BEGIN; ii != SDB_ENGINEDATA_END; ++ii)
		{
			if (ii == SDB_EngRGBData) continue; // skip surface texture
			TSegDataBlock* pb = GetBlock(ii,true);
			pb->GoToState(S_Modified);
		}
	}


	// save engine data
	TSegDataBlock *pb;

	pb = GetBlock(SDB_EngData, true);
	{
		AUTO_LOCK(g_cs);
		if (pb && m_nSID >= 0
			&& ((sflag&SF_ForceSaveAll)
				//|| pb->AppliedVersionIs(HRID_EDITOR)
				|| pb->StateEqualTo(S_Modified)
				)
			)
		{
			pb->ClearData();
			ITerrain *pTerrain = GetIEditor()->Get3DEngine()->GetITerrain();
			if (pTerrain) 
			{
				int iSize = pTerrain->GetCompiledDataSize(0, m_nSID);
				if (iSize > 0) 
				{
					CMemoryBlock mem;
					mem.Allocate(iSize);
					pTerrain->GetCompiledData((byte *) mem.GetBuffer(), iSize, NULL, NULL, NULL, GetPlatformEndian(), NULL, m_nSID, segmentOffset);
					//mem.Compress(pb->m_data);
					pb->SetData(mem);
				}
			}
			pb->AppliedLocally(lc);
		}
	}

	pb = GetBlock(SDB_VisAreas, sflag&SF_ForceSaveAll);
	{
		AUTO_LOCK(g_cs);
		if (pb && m_nSID >= 0
			&& ((sflag&SF_ForceSaveAll)
				//|| pb->AppliedVersionIs(HRID_EDITOR)
				|| pb->StateEqualTo(S_Modified)
				)
			)
		{
			pb->ClearData();
			ITerrain *pTerrain = GetIEditor()->Get3DEngine()->GetITerrain();
			IVisAreaManager* pVisAreaManager = GetIEditor()->Get3DEngine()->GetIVisAreaManager();
			if(pTerrain && pVisAreaManager)
			{
				std::vector<struct IStatObj*> * pStatObjTable = NULL;
				std::vector<IMaterial*> * pMatTable = NULL;
				std::vector<IStatInstGroup*> *pStatInstGroupTable = NULL;
				pTerrain->GetBrushTables(pStatObjTable, pMatTable, pStatInstGroupTable, m_nSID);

				pVisAreaManager->PrepareSegmentData(bb);
				int iSize = pVisAreaManager->GetCompiledDataSize(0);
				if(iSize >= 0)
				{
					CMemoryBlock mem;
					mem.Allocate(iSize);
					pVisAreaManager->GetCompiledData((byte *) mem.GetBuffer(), iSize, &pStatObjTable, &pMatTable, &pStatInstGroupTable, GetPlatformEndian(), NULL, segmentOffset);
					//mem.Compress(pb->m_data);
					pb->SetData(mem);
				}
			}
			pb->AppliedLocally(lc);
		}
	}

	pb = GetBlock(SDB_EngEntities, sflag&SF_ForceSaveAll);
	{
		AUTO_LOCK(g_cs);
		if (pb && ((sflag&SF_ForceSaveAll)
					//|| pb->AppliedVersionIs(HRID_EDITOR)
					|| pb->StateEqualTo(S_Modified)
				)
			) 
		{
			XmlNodeRef entitiesNode = gEnv->pSystem->CreateXmlNode("entities");
			CBaseObjectsArray objects;
			GetIEditor()->GetObjectManager()->GetObjects(objects);
			for (int i = 0; i < objects.size(); ++i) 
			{
				CBaseObject *obj = objects[i];
				if (!obj->GetLayer()->IsExportable())
					continue;
				if (!obj->IsKindOf(RUNTIME_CLASS(CEntityObject)))
					continue;
				Vec3 pos = obj->GetWorldTM().GetTranslation();
				if (!ObjInBox(bb, pos))
					continue;

				XmlNodeRef node = obj->Export("", entitiesNode);
				if (node)
				{
					if(!(obj->GetParent() && obj->GetParent()->IsKindOf(RUNTIME_CLASS(CEntityObject))))
					{
						pos += segmentOffset;
						node->setAttr("Pos", pos);
					}
				}
			}
			//if (entitiesNode->getChildCount() > 0) 
			{
				if (psw->GetSpecialOperationType() == CSegmentedWorldDoc::SW_SO_XMLS_EXEC_SCRIPT_AND_SAVE)
					psw->CallScript(entitiesNode);
				XmlString str = entitiesNode->getXML();
				CMemoryBlock tmp;
				tmp.Attach((void*)str.c_str(), str.length());
				//tmp.Compress(pb->m_data);
				pb->SetData(tmp);
			}
			pb->AppliedLocally(lc);
		}
	}

	if (!(sflag&SF_WithSubmit))
		return;

	const int arrEngData[]= {SDB_EngData, SDB_EngEntities, SDB_EngRGBData, SDB_VisAreas};
	if (/*g_db->GetCachedConn() &&*/ pb) 
	{
		bool bMissing;
		{
			AUTO_LOCK(g_cs);
			bMissing = m_bMissing;
		}
		if (bMissing) 
		{
			CreateData();
			AUTO_LOCK(g_cs);
			m_bMissing = false;
		}

		for (int ii = 0; ii < sizeof(arrEngData)/sizeof(arrEngData[0]); ++ii)
		{
			pb = m_Data[arrEngData[ii]];
			if (!pb)
				continue;
			pb->DoSaveToDB(sflag,vt,nVersion);
		}
	}
}
//---------------------------------------------------------------------------------------
bool TSegmentData::LoadLayers(const TLocalCoords &lc, CBaseObjectsArray &arrObjects, CObjectArchive &oa)
{
	//AUTO_LOCK(g_cs);
	//UpdateLayersStatus();
	CSWDoc *psw = GetInternalSWDoc();
	CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
	AABB bb = psw->CalcWorldBounds(lc);

	std::vector<std::pair<GUID,GUID> > layerChildToParentMap;
	
	std::vector<TLayerData*> clonedLayerMap;
	{
		AUTO_LOCK(g_cs);
		for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
			clonedLayerMap.push_back(it->second);
	}

	for(std::vector<TLayerData*>::iterator it = clonedLayerMap.begin(); it != clonedLayerMap.end(); ++it)
	{
		TLayerData *pl = *it;
		if(pl->NeedsToBeApplied())
		{
			CMemoryBlock mem;
			TLocalCoords lcOrg;
			if (LoadLayerData(pl, mem, &lcOrg))
			{
				const char *pcXML = (const char *) mem.GetBuffer();
				assert(mem.GetUncompressedSize() == 0); // should be already uncompressed
				size_t cbXML = mem.GetSize();

				XmlNodeRef objRoot = XmlHelpers::LoadXmlFromBuffer(pcXML, cbXML);

				assert(0 == strcmpi( objRoot->getTag(),"Objects")); //< new db data stores layer info too. -RL

				//CObjectLayer* pLayer = pLayerManager->CreateLayer(&pl->m_LayerGUID);
				//CLayerInfoManager::TLayerInfo* pLayerInfo = psw->LayerInfoMgr().FindLayer(pl->m_LayerGUID);
				////pLayer->Serialize( objRoot,true );
				//pLayer->Serialize(XmlHelpers::LoadXmlFromBuffer(pLayerInfo->m_strAttribXML.c_str(),pLayerInfo->m_strAttribXML.size())
				//				,true);

				//pLayerManager->AddLayer(pLayer);

				CObjectLayer* pLayer = pLayerManager->FindLayer(pl->m_LayerGUID);
				assert(pLayer);
				pLayerManager->SetCurrentLayer(pLayer);

				//GUID guidParent,guidThis;
				//if (objRoot->getAttr("ParentGUID",guidParent) && objRoot->getAttr("GUID",guidThis))
				//	layerChildToParentMap.push_back(std::make_pair(guidThis,guidParent));
				//if (pl->HasParent())
				//	layerChildToParentMap.push_back(std::make_pair( pl->m_LayerGUID, pl->m_parentLayerGUID));

				//XmlNodeRef nodeLayerObjects = objRoot->getChild(0);
				//assert(nodeLayerObjects && 0 == strcmpi("LayerObjects", nodeLayerObjects->getTag()));
				//objRoot = nodeLayerObjects;

				if (objRoot)
				{
					oa.EnableProgressBar(false);

					//copy-pasted and modified: GetIEditor()->GetObjectManager()->LoadObjects(oa);
					int numObjects = objRoot->getChildCount();
					for (int i = 0; i < numObjects; i++)
					{
						oa.node = objRoot->getChild(i);

						// convert object relative pos into "editor-local" pos
						assert( TWorldCoords(lcOrg) == m_wc );
						Vec3 vPos;
						oa.node->getAttr("Pos",vPos);
						vPos = psw->SegRelativeToEditorLocal(vPos,m_wc,1);
						oa.node->setAttr("Pos",vPos);

						CBaseObject *obj = oa.LoadObject(oa.node, 0);
						if (!obj)
						{
							CRY_ASSERT_MESSAGE(obj,"LoadObject Failed in SW");
							continue; // skip incorrect object -RL
						}
						// don't need to load children recursively
						//psw->LoadObjectWithChilds(obj, oa);
					}
				}

				pl->ApplyLoadedVersion();

				//TO DO;
				char chLog[256]; sprintf(chLog, "LoadedLayer %s: (%d,%d)\n", pl->m_strName.c_str(), m_wc.wx, m_wc.wy);
				SWLog(chLog);
			}
			else
			{
				// the layer has no content on current Segment
				// then it should be treated as missing

				// TODO: do we need to handle anything?? -RL
			}
		}

	}
	return true;
}

bool TSegmentData::LockLayers()
{
	if (CSWManager::Get().IsOfflineMode()) return true;

	CSegmentedWorldDoc *pSW = GetInternalSWDoc();
	bool bLockedAll = true;
	for(TLayerMap::iterator iter = this->m_layersMap.begin(); iter != m_layersMap.end(); ++iter)
	{
		ELockResult lr = pSW->GetVersionControl()->Lock(*iter->second);
		if (lr == LockResult_Failed)
			bLockedAll = false;
	}

	UpdateLayersStatus();

	return bLockedAll;
}
//---------------------------------------------------------------------------------------
void TSegmentData::UnlockLayers()
{
	CSegmentedWorldDoc *psw = GetInternalSWDoc();

	for(TLayerMap::iterator iter = this->m_layersMap.begin(); iter != m_layersMap.end(); ++iter)
	{
		psw->GetVersionControl()->Unlock(*iter->second);
	}

	UpdateLayersStatus();
}
//---------------------------------------------------------------------------------------
bool TSegmentData::HasLockedLayer()
{
	AUTO_LOCK(g_cs);
	CSWDoc *psw = GetInternalSWDoc();
	for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
	{
		TLayerData *pl = it->second;
		if(pl->IsLockedByMe())
			return true;
	}
	return false;
}
//---------------------------------------------------------------------------------------
bool TSegmentData::NeedsReloadLayers()
{
	if (!GetInternalSWDoc()->IsLayerDataNeeded())
		return false;
	AUTO_LOCK(g_cs);
	for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
	{
		TLayerData* pl = it->second;
		if ( pl->NeedsReload() )
			return true;
	}
	return false;
}
//---------------------------------------------------------------------------------------
void TSegmentData::ReloadLayers()
{
	EVersionType vt = GetNeededVersionType();
	bool bNeedStatusUpdate = false;

	{
		AUTO_LOCK(g_cs);
		for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
		{
			TLayerData* pl = it->second;
			if (pl->LoadedVersionIsVT(vt))
				continue;
			bNeedStatusUpdate = bNeedStatusUpdate || pl->NeedUpateStatus(vt);
		}
	}

	if (bNeedStatusUpdate)
		UpdateLayersStatus();

	std::vector<TLayerData*> clonedLayerMap;
	{
		AUTO_LOCK(g_cs);
		for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
			clonedLayerMap.push_back(it->second);
	}
	for (std::vector<TLayerData*>::iterator it = clonedLayerMap.begin(); it != clonedLayerMap.end(); ++it)
	{
		TLayerData* pl = *it;
		if (pl->IsModified())
			continue;
		if (pl->LoadedVersionIsVT(vt))
			continue;

		CRY_ASSERT_MESSAGE( (pl->GetVersion(vt) != HRID_UNKNOWN) , "Invalid version requested in ReloadLayers");
		
		TWorldCoords wc;
		{
			AUTO_LOCK(g_cs);
			assert(!pl->IsLockedByMe()|| GetInternalSWDoc()->IsHot());
			wc = m_wc;
		}

		pl->LoadByVT(wc,vt);
	}
}
//---------------------------------------------------------------------------------------
bool TSegmentData::NeedsToBeAppliedLayers()
{
	if (!GetInternalSWDoc()->IsLayerDataNeeded())
		return false;

	bool bNeedUpdateStatus = false;
	{
		AUTO_LOCK(g_cs);
		for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
		{
			TLayerData* pl = it->second;
			bNeedUpdateStatus = bNeedUpdateStatus || pl->NeedUpateStatus();
		}
	}

	if (bNeedUpdateStatus)
		UpdateLayersStatus();

	AUTO_LOCK(g_cs);
	for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
	{
		TLayerData* pl = it->second;
		//if (pl->AppliedVersionIs(HRID_EDITOR))
		if (pl->StateEqualTo(S_Modified))
			continue;

		//if (!pl->LoadedVersionIs(HRID_CACHE) )
		if (!pl->StateEqualTo(S_ModifSerialized))
		{
			EVersionType vt = pl->GetNeededVersionType();
			int nHRID = pl->GetVersion(vt);
			if (nHRID < 0) 
			{
				assert(!"Invalid version requested");
				continue;
			}
			if (pl->LoadedVersionIs(nHRID))
			{
				if(!pl->IsLoadedVersionApplied())
					return true;
				else
					continue;
			}
			//if (pl->AppliedVersionIs(HRID_MISSING))
			if (pl->StateEqualTo(S_Loaded))
				return true;
		}
	}

	for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
	{
		TLayerData* pl = it->second;
		if ( pl->NeedsToBeApplied() )
			return true;
	}
	return false;
}
//---------------------------------------------------------------------------------------
void TSegmentData::NoLongerAppliedLayers()
{
	AUTO_LOCK(g_cs);
	for(TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it)
	{
		TLayerData *pl = it->second;
		pl->NoLongerApplied();
	}
}

//---------------------------------------------------------------------------------------
bool TSegmentData::NeedRegenerateMiniMap()
{
	return true;
}

void TSegmentData::SetAllLayersAsMissing()
{
	AUTO_LOCK(g_cs);
	for (TLayerMap::iterator it = m_layersMap.begin(); it != m_layersMap.end(); ++it) 
	{
		TLayerData *pl = it->second;
		pl->SetAsMissingInPersist();
	}
}

void TSegmentData::Init( const TWorldCoords& wc )
{
	m_wc = wc;
	m_nSID = -1;
	//d.m_bMissing = true;
	//d.m_nLockedBy = 0;
	Versioned::InitVersion();
	for (int iBT = 0; iBT < SDB_COUNT; ++iBT)
		GetBlock((ESDBType) iBT, true);
}

TArgStack TSegmentData::MakePrimaryKey() const
{
	AUTO_LOCK(g_cs);
	int idSeg = MakeSegmentID(m_wc);
	TArgStack arrPrimaryKey;
	arrPrimaryKey.push_back(idSeg);
	return arrPrimaryKey;
}

void TSegmentData::PrepareData( ESaveFlag sflag, sw::EVersionType vt, const TLocalCoords &lc, TWorldCoords wc )
{
	TSegmentData * pd = this;
	CSWDoc *psw = GetInternalSWDoc();

	CPoint ptPos(lc.lx * SEGMENT_SIZE_UNITS, lc.ly * SEGMENT_SIZE_UNITS);
	CSize size(SEGMENT_SIZE_UNITS, SEGMENT_SIZE_UNITS);
	CRect rc(ptPos, size);
	AABB bb = psw->CalcWorldBounds(lc);

	// save sector info
	TSegDataBlock *pb;

	// save heightmap
	pb = pd->GetBlock(SDB_Heights, sflag&SF_ForceSaveAll);
	if (pb && ((sflag&SF_ForceSaveAll)
			//|| pb->AppliedVersionIs(HRID_EDITOR)
			|| pb->StateEqualTo(S_Modified)
			)
		) 
	{
		GetIEditor()->GetHeightmap()->ExportSegmentHeights(pb->DataReceiver(), rc);
		pb->AppliedLocally(lc);
	}

	// save terrain layers
	pb = pd->GetBlock(SDB_LayerIDs, sflag&SF_ForceSaveAll);
	if (pb && ((sflag&SF_ForceSaveAll)
			//|| pb->AppliedVersionIs(HRID_EDITOR)
			|| pb->StateEqualTo(S_Modified)
			)
		) 
	{
		GetIEditor()->GetHeightmap()->ExportSegmentLayerIDs(pb->DataReceiver(), rc);
		pb->AppliedLocally(lc);
	}

	// save vegetation
	pb = pd->GetBlock(SDB_Vegetation, sflag&SF_ForceSaveAll);
	if (pb && ((sflag&SF_ForceSaveAll)
			//|| pb->AppliedVersionIs(HRID_EDITOR)
			|| pb->StateEqualTo(S_Modified)
			)
		) 
	{
		Vec3 vOfs = -GetIEditor()->GetHeightmap()->HmapToWorld(ptPos);
		GetIEditor()->GetHeightmap()->GetVegetationMap()->ExportSegment(pb->DataReceiver(), bb, vOfs);
		pb->AppliedLocally(lc);
	}

	// save RGB layer
	pb = pd->GetBlock(SDB_RGBLayer, sflag&SF_ForceSaveAll);
	if (pb && ((sflag&SF_ForceSaveAll)
			//|| pb->AppliedVersionIs(HRID_EDITOR)
			|| pb->StateEqualTo(S_Modified)
			)
		) 
	{
		int x = lc.lx;
		int y = lc.ly;
		Seg2RGB(x, y);
		if (CTerrainManager::GetTerrainManager().GetHeightmap().m_TerrainRGBTexture.ExportSegment(pb->DataReceiver(), x, y))
			pb->AppliedLocally(lc);
	}

	// save entities
	// don't save to db, because it has already been done in SaveLayers, at the top of this function.

	////pd->SaveDataForEngine(sflag, lc, nVersion, vt, /*bSaveToDB*/false);
	//pd->SaveDataForEngine(sflag&~SF_WithSubmit, lc, -1, vt);

	/*
	pb = pd->GetBlock(SDB_EngEntities, sflag&SF_ForceSaveAll);
	if (pb && (sflag&SF_ForceSaveAll || pb->m_nAppliedHRID == HRID_EDITOR)) 
	{
	XmlNodeRef entitiesNode = gEnv->pSystem->CreateXmlNode("entities");
	CBaseObjectsArray objects;
	GetIEditor()->GetObjectManager()->GetObjects(objects);
	for (int i = 0; i < objects.size(); ++i) 
	{
	CBaseObject *obj = objects[i];
	if (!obj->GetLayer()->IsExportable())
	continue;
	if (!obj->IsKindOf(RUNTIME_CLASS(CEntity)))
	continue;
	Vec3 pos = obj->GetWorldTM().GetTranslation();
	if (!ObjInBox(bb, pos))
	continue;

	XmlNodeRef node = obj->Export("", entitiesNode);
	if (node) 
	{
	pos -= Vec3(bb.min.x, bb.min.y, 0);
	node->setAttr("Pos", pos);
	}
	}
	if (entitiesNode->getChildCount() > 0) 
	{
	XmlString str = entitiesNode->getXML();
	CMemoryBlock tmp;
	tmp.Attach((void*)str.c_str(), str.length());
	tmp.Compress(pb->m_data);
	}
	else 
	{
	pb->m_data.Free();
	}
	pb->AppliedLocally(lc);
	}

	// save engine data
	pb = pd->GetBlock(SDB_EngData, sflag&SF_ForceSaveAll);
	if (pb && (sflag&SF_ForceSaveAll || pb->m_nAppliedHRID == HRID_EDITOR) && pd->m_nSID >= 0) 
	{
	pb->m_data.Free();
	ITerrain *pTerrain = GetIEditor()->Get3DEngine()->GetITerrain();
	if (pTerrain) 
	{
	int iSize = pTerrain->GetCompiledDataSize(0, pd->m_nSID);
	if (iSize > 0) 
	{
	CMemoryBlock mem;
	mem.Allocate(iSize);
	pTerrain->GetCompiledData((byte *) mem.GetBuffer(), iSize, NULL, NULL, GetPlatformEndian(), NULL, pd->m_nSID);
	mem.Compress(pb->m_data);
	}
	}
	pb->AppliedLocally(lc);
	}
	*/


	//// save  engine RGB data
	//pb = pd->GetBlock(SDB_EngRGBData, sflag&SF_ForceSaveAll);
	//if (pb && ((sflag&SF_ForceSaveAll)
	//			//|| pb->AppliedVersionIs(HRID_EDITOR)
	//			|| pb->StateEqualTo(S_Modified)
	//		) && pd->m_nSID >= 0
	//	) 
	//{
	//	psw->GetFileBlockData(pb->DataReceiver(), wc, SFileBlock_EngRGBData);
	//	pb->AppliedLocally(lc);
	//}

	/*
	pb = pd->GetBlock(SDB_Map, sflag&SF_ForceSaveAll);
	if (pb && (sflag&SF_ForceSaveAll || pb->m_nAppliedHRID == HRID_EDITOR) && pd->m_nSID >= 0) 
	{
	GetFileBlockData(pb->m_data, wc, SFileBlock_Map);
	pb->AppliedLocally(lc);
	}
	*/

	// save  Mod sectors
	pb = pd->GetBlock(SDB_ModSectors, sflag&SF_ForceSaveAll);
	if (pb && ((sflag&SF_ForceSaveAll)
				//|| pb->AppliedVersionIs(HRID_EDITOR)
				|| pb->StateEqualTo(S_Modified)
				) && pd->m_nSID >= 0
			) 
	{
		int iTexSectorSize = GetIEditor()->Get3DEngine()->GetTerrainTextureNodeSizeMeters();
		int xSect = lc.lx*SEGMENT_SIZE_METERS / iTexSectorSize;
		int ySect = lc.ly*SEGMENT_SIZE_METERS / iTexSectorSize;
		int wSect = SEGMENT_SIZE_METERS / iTexSectorSize;
		int hSect = SEGMENT_SIZE_METERS / iTexSectorSize;

		CRect rcSect(CPoint(xSect, ySect), CSize(wSect, hSect));
		GetIEditor()->GetHeightmap()->ExportSegmentModSectors(pb->DataReceiver(), rcSect);
		pb->AppliedLocally(lc);
	}
	
}

CString TSegDataBlock::GetFileName( CSCDataPersistence& persist ) const
{
	CString sFileName;
	CString sPath;

	TWorldCoords const& wc = GetWC();
	sPath = persist.GetEditorDataPath(wc);
	sFileName.Format("%s%s.%s", sPath, GetSegDataBlockName(m_eType), "dat");

	return sFileName;
}

void TSegDataBlock::SetData( CImage& img )
{
	img.Compress(m_data);
}

void TSegDataBlock::SetData( CMemoryBlock& mem )
{
//#ifndef SW_NO_DATACOMPRESS
	mem.Compress(m_data);
//#else
//	m_data.Free();
//	m_data = mem;
//#endif
}

void TSegDataBlock::GetData( CMemoryBlock& mem ) const
{
//#ifndef SW_NO_DATACOMPRESS
	m_data.Uncompress(mem);
//#else
//	mem.Free();
//	mem = m_data;
//#endif
}

TWorldCoords const& TSegDataBlock::GetWC() const
{
	COMPILE_TIME_ASSERT( &((TWorldCoords*)0)->wx == &((TLocalCoords*)0)->lx );
	COMPILE_TIME_ASSERT( &((TWorldCoords*)0)->wy == &((TLocalCoords*)0)->ly );
	if (IsIndependent())
		return *reinterpret_cast<TWorldCoords const*>(&m_lc);	// Tricky but as long as these two structures are equivalent it's fine
	else
		return m_pParent->m_wc;
}


CString TSegmentData::GetFileName(CSCDataPersistence& persist) const
{
	CString sFileName;
	CString sPath;

	sPath = persist.GetEditorDataPath(m_wc);
	sFileName.Format("%s%s", sPath, SW_SegmentMainFile);

	return sFileName;
}

bool TSegmentData::Submit()
{
	bool ret = true;
	for ( TLayerMap::iterator iter = m_layersMap.begin(); iter != m_layersMap.end(); ++iter)
	{
		ret = ret && iter->second->Submit();
	}
	return ret;
}

bool TSegmentData::SaveData( int64 nVersion, EVersionType vt )
{
	if(/*bNeedCreate*/IsMissingInPersist())
	{
		if (!CreateData())
		{
			assert(0);
			return false;
		}
	}

	// the rev id for root segment node is never used, but persistence will check it when we saving the DStructVersion,
	// so just fake it
	for (size_t ii = 0; ii < VT_COUNT; ++ii)
		this->m_nHRIDs[ii] = 1;
	m_state = S_ModifSerialized;



	XmlNodeRef xml = XmlHelpers::CreateXmlNode(g_szSegmentMainTag);
	xml->setAttr(g_szSWDStructVerAttr,CSWManager::GetSWDataStructureVersion());
	XmlString str = xml->getXML();
	CMemoryBlock mem;
	mem.Attach((void*)str.c_str(),str.size());

	int nLockedBy = -1;
	GetInternalSWDoc()->GetVersionControl()->GetLockedBy(*this,nLockedBy);

	// if not originally locked dont submit, it will ruin the lock state
	if (nLockedBy != 0)
		return GetInternalSWDoc()->GetPersistence()->CheckoutSave(*this,mem,nVersion,vt,TArgStack());

	return GetInternalSWDoc()->GetPersistence()->CheckoutSaveSubmit(*this,mem,nVersion,vt,TArgStack());
}


//////////////////////////////////////////////////////////////////////////
// TSegEngDataBlock
//////////////////////////////////////////////////////////////////////////
TSegEngDataBlock::TSegEngDataBlock( TParent* pParent, sw::ESDBType blockType, TWorldCoords const& wc )
	: TSegDataBlock(pParent, blockType, wc)
{
	assert(blockType >= SDB_ENGINEDATA_BEGIN && blockType < SDB_ENGINEDATA_END);
}

bool TSegEngDataBlock::CanGoToState( EState from, EState s ) const
{
	switch (from)
	{
	case S_Unapplied:	// unapplied is exactly the same as Init, except we have the Init separated to detect uninitialized data.
	case S_Init:
		return 0
			|| (s == S_Modified)	// for Exported data, they generate to Engine objects first (then we dump them to files).
			|| ( s == S_Unapplied)	// Exported data treat Unapplied as state `Empty'
			;
	case S_Modified: // means the depended source data is changed, so this one is dirty/out-dated and need to be re-generated
		return 0
			|| (s == S_Modified) // more modification
			|| (s == S_ModifSerialized) // serialized to CMemoryBlock
			;
	case S_ModifSerialized:
		return 0
			|| (s == S_Unapplied)
			|| (s == S_Loaded) // submitted
			|| (s == S_Modified) // new changes came before saving, need to re-serialize
			;
	case S_Loaded:
		return 0
			|| (s == S_Unapplied)
			|| (s == S_Modified) // new change
			;
	default:
		assert(0&&"Corrupted state data");
	}
	return false;
}

bool TSegEngDataBlock::IsStateAfter( EState testee, EState refPoint ) const
{
	switch (refPoint)
	{
	case S_Unapplied:
	case S_Init:
		return (testee == S_Modified) || (testee == S_ModifSerialized) || (testee == S_Loaded);
	case S_Modified:
		return (testee == S_ModifSerialized) || (testee == S_Loaded);
	case S_ModifSerialized:
		return (testee == S_Loaded); // submitted
	case S_Loaded:
		return (testee == S_Modified);
	default:
		assert(0&&"Corrupted state data");
		return false;
	}
}

void TSegEngDataBlock::OnSavedDataToDB( int64 id )
{
	// for these type (export data) only update loaded version, not applied version
	GoToState(S_Loaded);
	Versioned::SetLoadedVersion(id);
}

void TSegEngDataBlock::NoLongerApplied()
{
	GoToState(S_Unapplied);
}


//////////////////////////////////////////////////////////////////////////
// TSegMinimapDataBlock
//////////////////////////////////////////////////////////////////////////
TSegMinimapDataBlock::TSegMinimapDataBlock( TParent* pParent, sw::ESDBType blockType, TWorldCoords const& wc )
	: TSegDataBlock(pParent, blockType, wc)
{
	assert(blockType >= SDB_MAP_BEGIN && blockType < SDB_MAP_END);
}

bool TSegMinimapDataBlock::CanGoToState( EState from, EState s ) const
{
	switch (from)
	{
	case S_Init:
		return 0
			|| (s == S_Modified)
			|| (s ==  S_ModifSerialized)	// Minimap can be generated directly.
			|| (s == S_Loaded)				// can also be loaded directly if there is one
			;
	case S_Modified: // means the depended source data is changed, so this one is dirty/out-dated and need to be re-generated
		return 0
			|| (s == S_Modified) // more modification
			|| (s == S_ModifSerialized) // regenerated
			;
	case S_ModifSerialized:
		return 0
			|| (s == S_Loaded) // submitted
			|| (s == S_Modified) // new changes came before saving, need to re-serialize
			;
	case S_Loaded:
		return 0
			|| (s == S_Modified) // new change
			|| (s == S_ModifSerialized) // force generate and replace
			;
	default:
		assert(0&&"Corrupted state data");
	}
	return false;
}

bool TSegMinimapDataBlock::IsStateAfter( EState testee, EState refPoint ) const
{
	switch (refPoint)
	{
	case S_Init:
		return (testee == S_Modified) || (testee == S_ModifSerialized) || (testee == S_Loaded);
	case S_Modified:
		return (testee == S_ModifSerialized) || (testee == S_Loaded);
	case S_ModifSerialized:
		return (testee == S_Loaded); // submitted
	case S_Loaded:
		return (testee == S_Modified);
	default:
		assert(0&&"Corrupted state data");
		return false;
	}
}
void TSegMinimapDataBlock::OnSavedDataToDB( int64 id )
{
	// for these type (export data) only update loaded version, not applied version
	GoToState(S_Loaded);
	Versioned::SetLoadedVersion(id);
}

SW_NAMESPACE_END();
