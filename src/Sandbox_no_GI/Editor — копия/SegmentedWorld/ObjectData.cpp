#include "StdAfx.h"
#include "InternalCommon.h"
#include "ObjectData.h"
#include "SegmentedWorldManager.h"
#include "SegmentedWorldDoc.h"
#include "LayerData.h"
#include "SegmentData.h"
#include "ISWDataPersistence.h"
#include "ISWVersionControl.h"

#include "CryMemBlock.h"
#include "Terrain/Heightmap.h"



using namespace sw;	// without this line VA will confuse

SW_NAMESPACE_BEGIN();

/////////////////////////////////////////////////////////////////////////////////////////
TObjectData::TObjectData()
:Versioned(VDT_EXTOBJECT,VerCap_All&~VerCap_Lockable)
{
	AUTO_LOCK(g_cs);
	//m_bMissing = true;
	Versioned::InitVersion(false);
	m_nLayerID = -1;
	m_guid = GUID_NULL;
	m_bNeedInWorld = false;
	m_nFlags = 0;
}

TObjectData::TObjectData( REFGUID guidObj )
	:Versioned(VDT_EXTOBJECT, VerCap_All&~VerCap_Lockable)
{
	AUTO_LOCK(g_cs);
	Versioned::InitVersion(false);
	m_nLayerID = -1;
	m_guid = guidObj;
	m_bNeedInWorld = false;
	m_nFlags = 0;
}

//-------------------------------------------------------------------------------------
TObjectData::~TObjectData()
{
	Unload();
}
//-------------------------------------------------------------------------------------
void TObjectData::Unload()
{
	AUTO_LOCK(g_cs);


	//m_bMissing = true;
	//m_nLoadedHRID = HRID_MISSING;
	Versioned::Reset();
	m_data.Free();
	m_nLayerID = -1;
	m_bNeedInWorld = false;
	//m_arrLinks.clear();
	//m_arrLinkedFrom.clear();

}

void TObjectData::UnloadAndUnlink()
{
	// not used or already unloaded
	if (m_state == S_Init)
		return;

	TObjectData::Unload();

	for ( TGuidList::const_iterator iter = m_arrLinks.begin(); iter != m_arrLinks.end(); ++iter)
	{
		REFGUID guidLinkedTo = *iter;
		TObjectData* pObj = GetInternalSWDoc()->ObjectAggr().FindObjectData(guidLinkedTo);
		if (!pObj)
			continue;
		TGuidList& arrLinkedFrom = pObj->m_arrLinkedFrom;
		arrLinkedFrom.erase( std::find(arrLinkedFrom.begin(),arrLinkedFrom.end(),m_guid) );
	}
	m_arrLinks.clear();
}
//-------------------------------------------------------------------------------------
void TObjectData::SetParams(CBaseObject *pObject)
{
	if (CSWManager::IsActive())
		return;
	
	assert(pObject);
	assert(m_guid == pObject->GetId());
	CSegmentedWorldDoc *psw = GetInternalSWDoc();

	AUTO_LOCK(g_cs);
	//m_lc = CSegmentedWorld::WorldToSegment(pObject->GetPos());
	pObject = CSegmentedWorldDoc::GetRootObject(pObject);
	m_wc = psw->WorldToSegment(pObject->GetWorldPos(), false);//m_lc;
	AABB box;
	//pObject->GetBoundBox(box);
	psw->GetBoundBoxWithChilds(pObject, box);
	TWorldCoords wcBoxMin = psw->WorldToSegment(box.min, false);
	TWorldCoords wcBoxMax = psw->WorldToSegment(box.max, false);
	m_bboxSeg.Min = wcBoxMin;
	m_bboxSeg.Max = wcBoxMax;
	m_strLayerName = pObject->GetLayer()->GetName();
	m_LayerGUID = pObject->GetLayer()->GetGUID();
}
//-------------------------------------------------------------------------------------
bool TObjectData::SaveObject(ESaveFlag sflag, const TLocalCoords &lc/*, CBaseObject *pObject*/, int64 nVersion, EVersionType vt)
{
	assert( (sflag&SF_WithSubmit && nVersion>0) || (!(sflag&SF_WithSubmit) && nVersion<=0) );
	//assert(!(CSWManager::Get().IsOfflineMode() && sflag&SF_WithSubmit));
	if (!PrepareData(sflag, vt, lc))
			return false;

	//bool bRes = Save(lc, nVersion, vt);
	{
		AUTO_LOCK(g_cs);
		m_wc = TWorldCoords(lc);
	}
	return DoSaveToDB(sflag, vt,nVersion);
	/*if (nVersion > 0)
	{
		//if (vt == VT_OFFICIAL)
		//	Unlock();

		if (m_nLoadedHRID == HRID_CACHE) 
		{
			assert(m_nAppliedHRID == HRID_CACHE);
			if(!SaveData(lc, nVersion, vt))
				return false;
		}
		for (int nVT = vt; nVT < VT_COUNT; ++nVT)
		{
			if (m_nHRIDs[nVT] == m_nLoadedHRID)
				continue;
			bRes = true;
			m_nHRIDs[nVT] = m_nLoadedHRID;
		}

		if(bRes)
			SaveObjectDataHRIDs(vt, false);
	}
	*/
	//return bRes;
}
bool TObjectData::DoIntegrate( EVersionType vt, int64 nVersion )
{
	assert(!(CSWManager::Get().IsOfflineMode() && nVersion != 0));
	if ( nVersion <= 0)
		return true;// no need to save in offline mode, treat it as succeeded.

	bool bModified = Versioned::DoIntegrateOnly(vt,nVersion);

	if (bModified)
		OnPersistentDataChanged(vt);
	return true;
}
bool TObjectData::DoSaveToDB(ESaveFlag sflag, EVersionType vt, int64 nVersion )
{
	//assert(!(CSWManager::Get().IsOfflineMode() && nVersion != 0));
	if ( nVersion <= 0)
		return true;// no need to save in offline mode, treat it as succeeded.

	if (Versioned::IsLoadedDataACache()) 
	{
		if (!Versioned::DoSave(sflag, vt,nVersion))
			return false;

		OnPersistentDataChanged(vt);
		return true;
	}
	assert(!(sflag&SF_AsIntegrate));

	return true;
}

void TObjectData::OnPersistentDataChanged( EVersionType vt )
{
	bool bRet = SavePartitioningChanges(vt);
	assert(bRet);
}
//-------------------------------------------------------------------------------------
void TObjectData::AppliedLocally(const TLocalCoords &lc)
{
	//AUTO_LOCK(g_cs);
	//m_nLoadedHRID = HRID_CACHE;
	//SetAppliedHRID(HRID_CACHE);
	OnAppliedLocally();
	m_lc = lc;
}
bool TObjectData::IsMissingInPersist() const
{
	return Versioned::IsMissingInPersist() || m_nLayerID == -1;
}
//-------------------------------------------------------------------------------------
bool TObjectData::SaveData(int64 nVersion, EVersionType vt)
{
	CSWDoc *psw = GetInternalSWDoc();

	// Insert new layer if not exist
	//bool bNeedCreate;
	//{
	//	AUTO_LOCK(g_cs);
	//	bNeedCreate = IsMissingInDB();
	//}
	if(/*bNeedCreate*/IsMissingInPersist())
		if (!CreateData())
		{
			assert(0);
			return false;
		}

//#if !defined(USE_LOCAL_SAVE) || defined(USE_NATIVE_VC) || defined(USE_NULL_VC)

	TArgStack arrAdditionalSave;
	//int64 nPrevHRID;
	{
		AUTO_LOCK(g_cs);
		int idSeg;
		idSeg = MakeSegmentID(m_wc);
		//int64 guidLo, guidHi;
		//GetGuid(guidLo, guidHi);
		string strGuid = GetGuidStr();
		//arrAdditionalSave.push_back(guidLo); 
		//arrAdditionalSave.push_back(guidHi); 
		arrAdditionalSave.push_back(strGuid.c_str()); 
		arrAdditionalSave.push_back(m_bboxSeg.Min.x); 
		arrAdditionalSave.push_back(m_bboxSeg.Min.y); 
		arrAdditionalSave.push_back(m_bboxSeg.Max.x); 
		arrAdditionalSave.push_back(m_bboxSeg.Max.y); 
		arrAdditionalSave.push_back(idSeg);
		arrAdditionalSave.push_back(m_wc.wx);
		arrAdditionalSave.push_back(m_wc.wy);
		arrAdditionalSave.push_back(m_nLayerID);
		arrAdditionalSave.push_back(m_lc.lx);
		arrAdditionalSave.push_back(m_lc.ly);


		//nPrevHRID = Versioned::GetVersion(vt);
	}

	//assert(g_db->GetCachedConn());
	//if (g_db->GetCachedConn())
		return GetInternalSWDoc()->GetPersistence()->CheckoutSaveSubmit(*this,m_data, nVersion, vt, arrAdditionalSave);
	//return false;

//	assert(g_db->GetCachedConn());
//	if (g_db->GetCachedConn())
//	{
//		IDBPreparedStatement *pStatement;
//		char chStatement[64];
//		pStatement = g_db->GetCachedConn()->GetPreparedStatement(EXTERNAL_OBJECTS_SAVE);
//		sprintf(chStatement, "EXTERNAL_OBJECTS_SAVE");
//
//		if (!pStatement)
//		{	
//			char chLog[256]; sprintf(chLog, "Cannot create %s query.\n", chStatement);
//			SWLog(chLog);
//			assert(0);
//			return false;
//		}
//
//		//set params
//		int idSeg = GetSegmentID(m_wc);
//		int64 guidLo, guidHi;
//		string strGuid;
//		GetGuid(guidLo, guidHi, strGuid);
//
//		bool bResult;
//		{
//			AUTO_LOCK(g_cs);
//			bResult =  DBAPIHelper::SetParam(pStatement, 0, guidLo) && 
//				DBAPIHelper::SetParam(pStatement, 1, guidHi) && 
//				DBAPIHelper::SetParam(pStatement, 2, strGuid.c_str()) &&
//				DBAPIHelper::SetParam(pStatement, 3, m_wcBoxMin.wx) &&
//				DBAPIHelper::SetParam(pStatement, 4, m_wcBoxMin.wy) &&
//				DBAPIHelper::SetParam(pStatement, 5, m_wcBoxMax.wx) &&
//				DBAPIHelper::SetParam(pStatement, 6, m_wcBoxMax.wy) &&
//				DBAPIHelper::SetParam(pStatement, 7, idSeg) &&
//				DBAPIHelper::SetParam(pStatement, 8, m_wc.wx) &&
//				DBAPIHelper::SetParam(pStatement, 9, m_wc.wy) &&
//				DBAPIHelper::SetParam(pStatement, 10, m_nLayerID) &&
//				DBAPIHelper::SetParam(pStatement, 11, m_lc.lx) &&
//				DBAPIHelper::SetParam(pStatement, 12, m_lc.ly) &&
//				DBAPIHelper::SetParam(pStatement, 13, m_data.GetUncompressedSize()) &&
//				pStatement->SetBinaryByIndex(14, m_data.GetSize() ? (const byte*)(CryMemBlock(m_data.GetBuffer(), m_data.GetSize()).pv) : NULL, m_data.GetSize()) &&
//				DBAPIHelper::SetParam(pStatement, 15, nVersion) &&
//				DBAPIHelper::SetParam(pStatement, 16, (int)vt) &&
//				DBAPIHelper::SetParam(pStatement, 17, m_nHRIDs[vt]);
//		}
//
//		if (!bResult)
//		{
//			char chLog[256]; sprintf(chLog, "%s query failed.\n", chStatement);
//			SWLog(chLog);
//			assert(0);
//			return false;
//		}
//		//execute
//		if (!DBAPIHelper::Exec(pStatement))
//		{
//			char chLog[256]; sprintf(chLog, "%s query failed.\n", chStatement);
//			SWLog(chLog);
//			assert(0);
//			return false;
//		}
//
//		//additional code
//		int64 id = pStatement->GetAutoIncrementedValue();
//		if (!id)
//			return false;
//
//		char chLog[256]; sprintf(chLog, "Saved (%d,%d):Object - %s(HRID %I64d, size %d)\n", m_wc.wx, m_wc.wy, strGuid, id, m_data.GetUncompressedSize());
//		SWLog(chLog);
//
//		{
//			AUTO_LOCK(g_cs);
//			//if (m_data.GetSize() == 0)
//			//	id = 0;
//			if (m_nAppliedHRID == m_nLoadedHRID)
//				SetAppliedHRID(id);
//			m_nLoadedHRID = id;
//		}
//
//		//hrids version history
//		{
//			IDBPreparedStatement *pStmt;
//			pStmt = g_db->GetCachedConn()->GetPreparedStatement(EXTERNAL_OBJECTS_SAVE_VERSION_HISTORY);
//			sprintf(chStatement, "EXTERNAL_OBJECTS_SAVE_VERSION_HISTORY");
//			if (pStmt)
//			{	
//				if (DBAPIHelper::Exec(pStmt, id, (int)vt, nVersion))
//				{
//					char chLog[256]; sprintf(chLog, "hrids History: %I64d -> %I64d.\n", id, nVersion);
//					SWLog(chLog);
//				}
//				else
//				{
//					char chLog[256]; sprintf(chLog, "%s query failed.\n", chStatement);
//					SWLog(chLog);
//				}
//			}
//			else
//			{
//				char chLog[256]; sprintf(chLog, "Cannot create %s query.\n", chStatement);
//				SWLog(chLog);
//			}
//		}
//
//		//HV: Delete Record
//		if(psw->NeedHotHistoryBeDeleted())
//		{
//			bool bMustDelete;
//			int64 nHRIDForDel;
//			{
//				AUTO_LOCK(g_cs);
//				bMustDelete = m_nHRIDs[VT_HOT] != m_nHRIDs[VT_CURRENT] && m_nHRIDs[VT_HOT] != m_nHRIDs[VT_OFFICIAL];
//				nHRIDForDel = m_nHRIDs[VT_HOT];
//			}
//			if (bMustDelete)
//			{
//				IDBPreparedStatement *pStatement2;
//				pStatement2 = g_db->GetCachedConn()->GetPreparedStatement(EXTERNAL_OBJECTS_DELETE);
//				sprintf(chStatement, "EXTERNAL_OBJECTS_DELETE");
//
//				bool bDeleted = false;
//				
//				if (pStatement2)
//				{	
//					if (DBAPIHelper::Exec(pStatement2, nHRIDForDel))
//					{
//						char chLog[256]; sprintf(chLog, "Deleted ObjectsHistory record with id: %I64d from.\n", nHRIDForDel);
//						SWLog(chLog);
//						//m_nHRIDs[VT_HOT] = m_nLoadedHRID;
//						bDeleted = (pStatement2->GetUpdateCount()>0);
//						
//					}
//					else
//					{
//						char chLog[256]; sprintf(chLog, "%s query failed.\n", chStatement);
//						SWLog(chLog);
//					}
//				}
//				else
//				{
//					char chLog[256]; sprintf(chLog, "Cannot create %s query.\n", chStatement);
//					SWLog(chLog);
//				}
//
//				//delete from hrids version history
//				if (bDeleted)
//				{
//					IDBPreparedStatement *pStmt;
//					pStmt = g_db->GetCachedConn()->GetPreparedStatement(EXTERNAL_OBJECTS_DELETE_VERSION_HISTORY);
//					sprintf(chStatement, "EXTERNAL_OBJECTS_DELETE_VERSION_HISTORY");
//					if (pStmt)
//					{	
//						if (DBAPIHelper::Exec(pStmt, nHRIDForDel))
//						{
//							char chLog[256]; sprintf(chLog, "objects hrids History record deleted: %I64d.\n", nHRIDForDel);
//							SWLog(chLog);
//						}
//						else
//						{
//							char chLog[256]; sprintf(chLog, "%s query failed.\n", chStatement);
//							SWLog(chLog);
//						}
//					}
//					else
//					{
//						char chLog[256]; sprintf(chLog, "Cannot create %s query.\n", chStatement);
//						SWLog(chLog);
//					}
//				}
//			}
//		}
//
//		return true;
//	}
//#else
//	CString name = GetFileName();
//	if (!SaveMemBlockToFile(name, m_data))
//		return false;
//
//	char chLog[256]; sprintf(chLog, "Saved Object - %s\n", GetGuidStr());
//	SWLog(chLog);
//
//	Versioned::OnSavedDataToDB(1);
//	return true;
//#endif
}
bool TObjectData::SavePartitioningChanges(EVersionType vt)
{
	if (CSWManager::Get().IsOfflineMode()) return true;
	CSWDoc *psw = GetInternalSWDoc();
	int idSeg = MakeSegmentID(m_lc);
	//int64 guidLo, guidHi;
	//GetGuid(guidLo, guidHi);

	////assert(g_db->GetCachedConn());
	////if (!g_db->GetCachedConn())
	////	return false;

	//TArgStack args;
	//for (int curVT = vt; curVT< VT_COUNT; ++curVT )
	//{
	//	args.push_back(m_wcBoxMin.wx);
	//	args.push_back(m_wcBoxMin.wy);
	//	args.push_back(m_wcBoxMax.wx);
	//	args.push_back(m_wcBoxMax.wy);
	//	args.push_back(idSeg);
	//	args.push_back(m_wc.wx);
	//	args.push_back(m_wc.wy);
	//	args.push_back(m_nLayerID);

	//	//args.push_back(Versioned::GetVersion(curVT));
	//}
	//TArgStack arrPK = MakePrimaryKey();
	//args.insert(args.end(),arrPK.begin(),arrPK.end());

	//uint64 nUpdateCount = 0;
	//if (!g_db->ExecPreparedStatement(m_vdt,StmtOp_SetPartioningInfo,args,&nUpdateCount,vt))
	//	return false;

	//// mysql works as that if no data is ACTUALLY changed, it doesn't count, so the following check is meaningless
	////if (nUpdateCount == 0)
	////	return false;

	return true;
}

//-------------------------------------------------------------------------------------
bool TObjectData::CreateData()
{
	//AUTO_LOCK(g_cs);
	CSWDoc *psw = GetInternalSWDoc();

	int nLayerID;
	GUID layerGuid;
	string strLayerName;
	bool bMissing;
	{
		AUTO_LOCK(g_cs);
		nLayerID = m_nLayerID;
		layerGuid = m_LayerGUID;
		strLayerName = m_strLayerName;
	}
	if (nLayerID == -1) //means missing
	{	
		nLayerID = psw->LayerInfoMgr().RegisterNewLayer(layerGuid, strLayerName.c_str(), GUID_NULL);
		{
			AUTO_LOCK(g_cs);
			m_nLayerID = nLayerID;
		}
	}

	if (nLayerID == -1)
	{
		assert(0);
		return false;
	}
	{
		AUTO_LOCK(g_cs);
		bMissing = Versioned::IsMissingInPersist();
	}

	if (bMissing) 
	{
		{
			//set params
			int idSeg = MakeSegmentID(m_wc);
			int64 guidLo, guidHi;
			GetGuid(guidLo, guidHi);
			string strGuid = GetGuidStr();

			////additional code
			//AUTO_LOCK(g_cs);
			TArgStack args;
			args.push_back(strGuid.c_str());
			args.push_back(m_bboxSeg.Min.x);
			args.push_back(m_bboxSeg.Min.y);
			args.push_back(m_bboxSeg.Max.x);
			args.push_back(m_bboxSeg.Max.y);
			args.push_back(idSeg);
			args.push_back(m_wc.wx);
			args.push_back(m_wc.wy);
			args.push_back(m_nLayerID);
			args.push_back(m_bboxSeg.Min.x);
			args.push_back(m_bboxSeg.Min.y);
			args.push_back(m_bboxSeg.Max.x);
			args.push_back(m_bboxSeg.Max.y);
			args.push_back(idSeg);
			args.push_back(m_wc.wx);
			args.push_back(m_wc.wy);
			args.push_back(m_nLayerID);
			args.push_back(m_bboxSeg.Min.x);
			args.push_back(m_bboxSeg.Min.y);
			args.push_back(m_bboxSeg.Max.x);
			args.push_back(m_bboxSeg.Max.y);
			args.push_back(idSeg);
			args.push_back(m_wc.wx);
			args.push_back(m_wc.wy);
			args.push_back(m_nLayerID);
			assert(args.size() == 27-2);
			bool bRet = psw->GetPersistence()->Add(*this,&args);
			assert(bRet);
		}
	}

	return true;
}
//-------------------------------------------------------------------------------------
void TObjectData::GetGuid(int64 &nGUIDLo, int64 &nGUIDHi) const
{
	nGUIDLo = m_guid.Data4[0];
	for (int i = 1; i < 8 ; i++)
		nGUIDLo = nGUIDLo<<8|m_guid.Data4[i];

	nGUIDHi = m_guid.Data1;
	nGUIDHi = nGUIDHi<<16|m_guid.Data2;
	nGUIDHi = nGUIDHi<<16|m_guid.Data3;

	//strGUID = GuidUtil::ToString(m_guid);
}
string TObjectData::GetGuidStr() const
{
	return GuidUtil::ToString(m_guid);
}
//-------------------------------------------------------------------------------------
bool TObjectData::NeedsToBeApplied()
{
	if (!GetInternalSWDoc()->IsLayerDataNeeded())
		return false;

	AUTO_LOCK(g_cs);
	//if (m_nAppliedHRID == HRID_EDITOR)
	if (StateEqualTo(S_Modified))
		return false;

	if (!m_bNeedInWorld)
	{
		//return (m_nLoadedHRID == HRID_MISSING && m_nAppliedHRID != m_nLoadedHRID); // when record is deleted needs to reapply
		return IsStateAfter(S_Loaded) && m_data.IsEmpty();
	}

	//if (m_nAppliedHRID == HRID_UNKNOWN)
	if (StateEqualTo(S_Loaded))
		return true;

	//if (m_nLoadedHRID != HRID_CACHE) 
	if (IsStateBefore(S_ModifSerialized))
	{
		EVersionType vt = GetInternalSWDoc()->GetVersionType();//GetNeededVersionType();
		int nHRID = m_nHRIDs[vt];
		if (nHRID < 0) 
		{
			assert(!"Invalid version requested");
		}
		if (m_nLoadedHRID == nHRID)
		{
			//return (m_nAppliedHRID != m_nLoadedHRID);
			return StateEqualTo(S_Loaded);
		}
	}
	//if (m_nAppliedHRID == HRID_MISSING)
	//	return true;
	//if (m_nAppliedHRID == m_nLoadedHRID)
	//	return false;
	return true;
}
//-------------------------------------------------------------------------------------
bool TObjectData::LoadObjectData(CMemoryBlock &mem, TLocalCoords *plc)
{
	bool bCanLoad;
	int64 nHRID;
	EVersionType vt;
	GUID guid;
	{
		AUTO_LOCK(g_cs);
		vt = GetInternalSWDoc()->GetVersionType();
		bCanLoad = !Versioned::IsLoadedDataACache();
		nHRID = m_nHRIDs[vt];
		guid = m_guid;
	}
	if (bCanLoad) 
	{	
		if (nHRID == HRID_UNKNOWN) 
		{
			GetInternalSWDoc()->ObjectAggr().ObjectData_UpdateStatus();
			{
				AUTO_LOCK(g_cs);
				nHRID = m_nHRIDs[vt];
				guid = m_guid;
			}
		}
		if (!Load( nHRID))
			return false;
	}
	AUTO_LOCK(g_cs);
	if (plc)
		*plc = m_lc;
	if (!m_data.GetSize())
		return false;
	GetData(mem);
	return true;
}
//-------------------------------------------------------------------------------------
bool TObjectData::Load( int64 nHRID)
{
//#if !defined(USE_LOCAL_SAVE) || defined(USE_NATIVE_VC) || defined(USE_NULL_VC)
	{
		AUTO_LOCK(g_cs);
		if (m_nLoadedHRID == nHRID)
			return true;
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

	CSWDoc *psw = GetInternalSWDoc();

	{
		TArgStack arrAdditional;
		if (!psw->GetPersistence()->LoadRevision(*this,nHRID,m_data,arrAdditional))
		{
			Unload();
			return false;
		}

#ifndef SW_USE_INTEGRATED_METADATA
		AUTO_LOCK(g_cs);
		bool bCorrupted = false;
		int nCol = 0;
		//GUIDLo,GUIDHi
		int64 guidLo, guidHi;

		///////////////////////////////////////
		bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(guidLo);
		bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(guidHi);
		CSWManager::CalcGuid(m_guid, guidLo, guidHi);
		bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(m_wcBoxMin.wx);
		bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(m_wcBoxMin.wy);
		bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(m_wcBoxMax.wx);
		bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(m_wcBoxMax.wy);
		bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(m_wc.wx);
		bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(m_wc.wy);
		bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(m_nLayerID) || m_nLayerID < 0;
		bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(m_lc.lx);
		bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(m_lc.ly);

		//layer
		bCorrupted = bCorrupted || !psw->LayerInfoMgr().ObjectLayersInfo_GetGUID(m_nLayerID, m_LayerGUID, m_strLayerName);

		if (bCorrupted)
		{
			assert(!"Database corrupted!");
			Unload();
			return false; 
		}
#else
		bool bOK = psw->LayerInfoMgr().ObjectLayersInfo_GetGUID(m_nLayerID, m_LayerGUID, m_strLayerName);
		assert(bOK);
#endif

		char chLog[256]; sprintf(chLog, "Loaded Object(%s):(HRID %I64d, size %d)\n", GuidUtil::ToString(m_guid), nHRID, m_data.GetUncompressedSize());
		SWLog(chLog);

		//m_nLoadedHRID = nHRID;
		Versioned::OnVersionLoaded(nHRID);

		return true;
	}
	return false;
}

//-------------------------------------------------------------------------------------
void TObjectData::NoLongerApplied()
{
	GUID guid;
	{
		AUTO_LOCK(g_cs);
		//if (m_nAppliedHRID == HRID_UNKNOWN)
		if (StateEqualTo(S_Unapplied))
			return;

		//SetAppliedHRID(HRID_UNKNOWN);
		GoToState(S_Unapplied);
		guid = m_guid;
	}
	InvalidateLinkedFromObjects();

	CSWDoc* psw = GetInternalSWDoc();
	//bool bWasSWFindObjectEnabled = psw->IsSWFindObjectEnabled();
	//psw->EnableSWFindObject(false);
	//CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(guid);
	//psw->EnableSWFindObject(bWasSWFindObjectEnabled);
	CBaseObject* pObject = GetBaseObject();
	if (pObject)
		psw->InvalidateObjectWithChilds(CSWDoc::GetRootObject(pObject));
}

//-------------------------------------------------------------------------------------
void TObjectData::InvalidateLinkedFromObjects()
{
	CSWDoc *psw = GetInternalSWDoc();
	AUTO_LOCK(g_cs);
	bool bWasActive = GetIEditor()->Get3DEngine()->IsSegmentedWorldActive();
	for (int i = 0, n = m_arrLinkedFrom.size(); i < n; i++)
	{
		TObjectData* pObjData = psw->ObjectAggr().FindObjectData(m_arrLinkedFrom[i]);
		if (!pObjData)
			continue;

		if (pObjData->CheckFlags(EXTOBJFLAG_NOW))
		{
			if(pObjData->IsModified())
			{
				GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);
				pObjData->SaveObject(SF_NoSubmitSave, pObjData->m_lc/*pObjData->m_wc*//*, pObject*/, 0, psw->GetVersionType());
				GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(bWasActive);
			}

			//bool bWasSWFindObjectEnabled = psw->IsSWFindObjectEnabled();
			//psw->EnableSWFindObject(false);
			//CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pObjData->m_guid);
			//psw->EnableSWFindObject(bWasSWFindObjectEnabled);
			CBaseObject* pObject = pObjData->GetBaseObject();
			CBaseObject* pAdam = GetInternalSWDoc()->GetRootObject(pObject);
			if (pAdam != pObject)
			{
				TObjectData* pAdamObjData = GetInternalSWDoc()->ObjectAggr().FindObjectData(pAdam);
				assert(pAdamObjData);
				assert(pAdamObjData->CheckFlags(EXTOBJFLAG_NOW));
				if(pAdamObjData->IsModified())
				{
					GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);
					pAdamObjData->SaveObject(SF_NoSubmitSave, pAdamObjData->m_lc/*pObjData->m_wc*//*, pObject*/, 0, psw->GetVersionType());
					GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(bWasActive);
				}
				pAdamObjData->NoLongerApplied();
			}

		}
		else
		{
			//bool bWasSWFindObjectEnabled = psw->IsSWFindObjectEnabled();
			//psw->EnableSWFindObject(false);
			//CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pObjData->m_guid);
			//psw->EnableSWFindObject(bWasSWFindObjectEnabled);
			CBaseObject* pObject = pObjData->GetBaseObject();
			if (pObject)
			{
				if (pObject->GetParent())
				{
					pObject = CSegmentedWorldDoc::GetRootObject(pObject);
				}

				TWorldCoords wc =  psw->WorldToSegment(pObject->GetWorldPos(), false);
				//TSegmentData *pd = psw->GetSegmentData(wc, false);
				//if (!pd)
				//	continue;
				CObjectLayer* ol = pObject->GetLayer();
				//TLayerData *pl = pd->GetLayer(ol->GetGUID(), ol->GetName());
				TLayerData *pl = psw->SegmentAggr().GetSegmentLayer(wc, ol->GetGUID(), ol->GetName());
				if (!pl)
					continue;
				if (pl->IsModified())
				{
					GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(true);
					CBaseObjectsArray arrObjects;
					//psw->GetObjectsFiltered(arrObjects);
					pl->SaveLayer(SF_NoSubmitSave, pl->m_lc, arrObjects, 0, psw->GetVersionType());
					GetIEditor()->Get3DEngine()->SetSegmentedWorldActive(bWasActive);
				}
				pl->NoLongerApplied();
			}
		}


		//if (pObjData->m_nAppliedHRID != HRID_UNKNOWN)
		if (!pObjData->StateEqualTo(Versioned::S_Unapplied))
			pObjData->NoLongerApplied();
	}

	for (int i = 0, n = m_arrLinks.size(); i < n; i++)
	{
		TObjectData* pObjData = GetInternalSWDoc()->ObjectAggr().FindObjectData(m_arrLinks[i]);
		if (pObjData)
		{
			if (
				//pObjData->m_nAppliedHRID != HRID_UNKNOWN
				!pObjData->StateEqualTo(Versioned::S_Unapplied)
				&& !GetInternalSWDoc()->IsInLoadedWorld(pObjData->m_wc)
				)
				pObjData->NoLongerApplied();
		}
	}

	m_arrLinkedFrom.clear();
	m_arrLinks.clear();
}
//-------------------------------------------------------------------------------------
CBaseObject* TObjectData::LoadBaseObject(CObjectArchive &oa, bool bDeleteExisting)
{
	AUTO_LOCK(g_cs);
	CBaseObject* result = NULL;
	CSWDoc *psw = GetInternalSWDoc();
	
	if (false && bDeleteExisting)
	{
		//bool bWasFindObjectEnabled = psw->IsSWFindObjectEnabled();
		//psw->EnableSWFindObject(false);
		//CBaseObject *po = GetIEditor()->GetObjectManager()->FindObject(m_guid);
		//psw->EnableSWFindObject(bWasFindObjectEnabled);
		CBaseObject* pObject = GetBaseObject();
		if (pObject)
		{
			psw->DeleteObjectWithChilds(pObject);
		}
	}

	CMemoryBlock mem;
	if (LoadObjectData(mem))
	{
		SetFlags(EXTOBJFLAG_NOW);

		assert(mem.GetSize());

		const char *pcXML = (const char *) mem.GetBuffer();
		XmlNodeRef objRoot = XmlHelpers::LoadXmlFromBuffer(pcXML, strlen(pcXML));
		if (objRoot)
		{
			oa.EnableProgressBar(false);

			//TLocalCoords lc;
			//lc.lx = m_wc.wx - psw->m_wcOfs.wx;
			//lc.ly = m_wc.wy - psw->m_wcOfs.wy;

			//copy-pasted and modified: GetIEditor()->GetObjectManager()->LoadObjects(oa);
			int numObjects = objRoot->getChildCount(); // actually it is always one here
			bool bWasFindObjectEnabled = psw->IsSWFindObjectEnabled();
			
			for (int i = 0; i < numObjects; i++)
			{
				oa.node = objRoot->getChild(i);

				if (oa.node->isTag("LinkToObject"))
				{
					GUID parentID;
					oa.node->getAttr( "ParentId", parentID);
					psw->EnableSWFindObject(bWasFindObjectEnabled);
					// parent must be loaded from FindObject if needed
					//CBaseObject* pParentObject = psw->FindObject(parentID);
					CBaseObject* pParentObject = GetIEditor()->GetObjectManager()->FindObject(parentID);
					psw->EnableSWFindObject(false);
					if (pParentObject)
					{
						result = GetIEditor()->GetObjectManager()->FindObject(m_guid);
					}
					psw->EnableSWFindObject(bWasFindObjectEnabled);
				}
				else
				{
					psw->EnableSWFindObject(false);

					// convert object relative pos into "editor-local" pos before loading
					Vec3 vPos;
					oa.node->getAttr("Pos",vPos);
					vPos = psw->SegRelativeToEditorLocal(vPos,m_wc,1);
					oa.node->setAttr("Pos",vPos);

					result = oa.LoadObject(oa.node, 0);
					if (!result)
						continue;
					psw->LoadObjectChildrenRecursively(result, oa);
					psw->EnableSWFindObject(bWasFindObjectEnabled);
				}
			}
			psw->EnableSWFindObject(bWasFindObjectEnabled);
		}

		//char chLog[256]; sprintf(chLog, "Loaded Object %s\n", GuidUtil::ToString(m_guid));
		//SWLog(chLog);
	}
	else
	{
		ClearFlags(EXTOBJFLAG_NOW);
		GoToState(S_Loaded);					// the file is empty but we can think it as "Loaded"
	}

	//SetAppliedHRID(m_nLoadedHRID);
	GoToState(S_Applied);

	char chLog[256]; sprintf(chLog, "Applied Object %s\n", GuidUtil::ToString(m_guid));
	SWLog(chLog);
	return result;
}
//-------------------------------------------------------------------------------------
void TObjectData::SyncDBFlags()
{
	// clear the three flags and set again
	int flags = m_nFlags & ~(EXTOBJFLAG_VT_OFFICIAL|EXTOBJFLAG_VT_CURRENT|EXTOBJFLAG_VT_HOT);
	{
		AUTO_LOCK(g_cs);
		for (int vt = 0; vt < VT_COUNT; ++vt) 
			flags |= (m_nHRIDs[vt] > 0)? BIT(vt) : 0;
	}
	SetFlags(flags);

	//need to init params from status table
	bool bNeedInit;
	{
		AUTO_LOCK(g_cs);
		bNeedInit = (!m_bMissing && m_nLayerID == -1);
	}
	if (bNeedInit)
	{
		Init();
	}
}

void TObjectData::Init()
{
	bool bNotNeedInit;
	{
		AUTO_LOCK(g_cs);
		bNotNeedInit = (m_bMissing || m_nLayerID != -1);
	}
	if (bNotNeedInit)
		return;

	assert(0&&"Not Implemented");

	CSWDoc *psw = GetInternalSWDoc();
}

TArgStack TObjectData::MakePrimaryKey() const
{
	AUTO_LOCK(g_cs);
	int64 guidHi,guidLo;
	GetGuid(guidLo,guidHi);

	TArgStack arrPrimaryKey;
	arrPrimaryKey.push_back(guidLo);
	arrPrimaryKey.push_back(guidHi);
	return arrPrimaryKey;
}


CString TObjectData::GetFileName(CSCDataPersistence& persist) const
{
	CString sFileName = GetInternalSWDoc()->GetLevelPath() + SW_ExternalObjectDirPath + GetGuidStr().c_str();
	return sFileName;
}

void TObjectData::Reinit( int64 nHRIDs[VT_COUNT], int nLockedBy )
{
	Versioned::Reinit(nHRIDs,nLockedBy);
	SyncDBFlags();
}

void TObjectData::SetAsMissingInPersist()
{
	Versioned::SetAsMissingInPersist();
	//SyncDBFlags();
}

bool TObjectData::PrepareData( ESaveFlag sflag, EVersionType vt, const TLocalCoords & lc )
{
	CSWDoc *psw = GetInternalSWDoc();
	bool bAppliedIsEditor;
	GUID guid;
	{
		AUTO_LOCK(g_cs);
		//bAppliedIsEditor = (m_nAppliedHRID == HRID_EDITOR);
		//bAppliedIsEditor = Versioned::AppliedVersionIs(HRID_EDITOR);
		bAppliedIsEditor = StateEqualTo(S_Modified);
		guid = m_guid;
	}
	if (sflag&SF_ForceSaveAll || bAppliedIsEditor)
	{
		//bool bWasFindObjectEnabled = psw->IsSWFindObjectEnabled();
		//psw->EnableSWFindObject(false);
		//CBaseObject *pObject = GetIEditor()->GetObjectManager()->FindObject(guid);
		//psw->EnableSWFindObject(bWasFindObjectEnabled);
		CBaseObject* pObject = GetBaseObject();
		if (!psw->ObjectAggr().IsExternal(pObject))
			pObject = NULL;

		if(pObject)
		{
			SetParams(pObject);

			XmlNodeRef objRoot = XmlHelpers::CreateXmlNode("Objects");
			CObjectArchive oa(GetIEditor()->GetObjectManager(), objRoot, false );
			if (pObject->GetParent())
			{
				oa.node = objRoot->newChild("LinkToObject");
				oa.node->setAttr( "ParentId", CSegmentedWorldDoc::GetRootObject(pObject)->GetId());
			}
			else
			{
				oa.node = objRoot->newChild("Object");
				psw->SaveObjectWithChildren(pObject, oa, m_wc);
			}
			//pObject->Serialize(oa);

			////save linked from
			//if(!m_arrLinkedFrom.empty())
			//{
			//	XmlNodeRef xmlNodeLinkedFrom = oa.node->newChild("LinkedFrom");
			//	for (int i = 0,num = m_arrLinkedFrom.size(); i < num; i++)
			//	{
			//		XmlNodeRef linkNode = xmlNodeLinkedFrom->newChild( "LinkOwner" );
			//		linkNode->setAttr( "LinkOwnerId",m_arrLinkedFrom[i] );
			//	}
			//}

			if (psw->GetSpecialOperationType() == CSegmentedWorldDoc::SW_SO_XMLS_EXEC_SCRIPT_AND_SAVE)
				psw->CallScript(objRoot);
			XmlString str = objRoot->getXML();
			CMemoryBlock tmp;
			tmp.Attach((void *) str.c_str(), str.length() + 1);
			AUTO_LOCK(g_cs);
			SetData(tmp);
		}
		else
		{
			AUTO_LOCK(g_cs);
			m_data.Free();
		}

		GUID layerGuid;
		string strLayerName;
		{
			AUTO_LOCK(g_cs);
			layerGuid = m_LayerGUID;
			strLayerName = m_strLayerName;
		}

		if (/*(vt != VT_HOT) &&*/ !psw->SegmentAggr().LockSegmentLayerData(lc, layerGuid, strLayerName))
			return false;

		AppliedLocally(lc);
	}

	return true;
}

void TObjectData::SetData( CMemoryBlock &mem )
{
#ifndef SW_NO_DATACOMPRESS
	mem.Compress(m_data);
#else
	m_data.Free();
	m_data = mem;
#endif
}

void TObjectData::GetData( CMemoryBlock & mem )
{
#ifndef SW_NO_DATACOMPRESS
	m_data.Uncompress(mem);
#else
	mem.Free();
	mem = m_data;
#endif
}

void TObjectData::SerializeWithLookupTable( CXmlArchive& ar )
{
	if (ar.bLoading)
	{
		XmlNodeRef obj = ar.root;
		obj->getAttr("GUID",m_guid);
		Vec2 tmp;
		obj->getAttr("wc", tmp);
		m_wc = TWorldCoords(tmp.x, tmp.y);
		obj->getAttr("wcBoxMin", tmp);
		//m_wcBoxMin = TWorldCoords(tmp.x, tmp.y);
		m_bboxSeg.Min = tmp;
		obj->getAttr("wcBoxMax", tmp);
		//m_wcBoxMax = TWorldCoords(tmp.x, tmp.y);
		m_bboxSeg.Max = tmp;
		obj->getAttr("LayerGUID", m_LayerGUID);
		m_strLayerName = obj->getAttr("LayerName");
		obj->getAttr("LayerID", m_nLayerID);
	}
	else
	{
		XmlNodeRef obj = XmlHelpers::CreateXmlNode("Object");
		assert(obj);

		obj->setAttr("GUID", m_guid);
		obj->setAttr("wc", Vec2(m_wc.wx, m_wc.wy));
		obj->setAttr("wcBoxMin", Vec2(m_bboxSeg.Min.x, m_bboxSeg.Min.y));
		obj->setAttr("wcBoxMax", Vec2(m_bboxSeg.Max.x, m_bboxSeg.Max.y));
		obj->setAttr("LayerGUID", m_LayerGUID);
		obj->setAttr("LayerName", m_strLayerName);
		obj->setAttr("LayerID", m_nLayerID);

		ar.root->addChild(obj);
	}

}

void TObjectData::LinkTo( TObjectData& target )
{
	this->m_arrLinks.push_back(target.GetGUID());
	//if (!stl::push_back_unique(pObjDataOwner->m_arrLinks, pObjDataTarget->m_guid))
	//{
	//	SWLog("Link already exist\n", true);
	//}

	target.m_arrLinkedFrom.push_back(this->GetGUID());

	//if (!stl::push_back_unique(pObjDataTarget->m_arrLinkedFrom, pObjDataOwner->m_guid))
	//{
	//	SWLog("LinkedTo already exist\n", true);
	//}
}

void TObjectData::UnlinkFrom( TObjectData& target )
{
	if (!stl::find_and_erase(this->m_arrLinks, target.m_guid))
	{
		SWLog("Link not found\n", true);
	}

	if (!stl::find_and_erase(target.m_arrLinkedFrom, this->m_guid))
	{
		SWLog("LinkedTo not found\n", true);
	}
}

void TObjectData::SetFlags( int flags )
{
	AUTO_LOCK(g_cs); m_nFlags |= flags;

	// if flagged external again, remove old disapprove flag
	if (flags == EXTOBJFLAG_NOW)
		flags &= ~EXTOBJFLAG_DISAPPROVED;
}

bool TObjectData::CheckDisapprove() const
{
	if (!(m_nFlags & EXTOBJFLAG_DISAPPROVED))
		return false;

	CBaseObject* pRoot = CSWDoc::GetRootObject(GetBaseObject());
	if (pRoot)
	{
		TObjectData* pRootObj = GetInternalSWDoc()->ObjectAggr().FindObjectData(pRoot);
		if (pRootObj && pRootObj != this)
			if (!pRootObj->CheckDisapprove())
				return false;
	}

	for (TGuidList::const_iterator iter = m_arrLinkedFrom.begin(); iter != m_arrLinkedFrom.end(); ++iter)
	{
		REFGUID guid = *iter;

		TObjectData* pObj = GetInternalSWDoc()->ObjectAggr().FindObjectData(guid);
		if (!pObj->CheckDisapprove())
			return false;
	}

	return true;
}

CBaseObject* TObjectData::GetBaseObject() const
{
	//try to get it in a fast way first
	if (CBaseObject* pObjBinded = (CBaseObject*)GetBindingHost())
		return pObjBinded;

	bool bWasEnabled = GetInternalSWDoc()->IsSWFindObjectEnabled();
	GetInternalSWDoc()->EnableSWFindObject(false);
	CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(m_guid);
	GetInternalSWDoc()->EnableSWFindObject(bWasEnabled);
	return pObj;
}


SW_NAMESPACE_END();
