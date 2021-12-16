#include "stdafx.h"
#include "InternalCommon.h"
#include "ObjectDataAggregator.h"

#include "ObjectData.h"
#include "LayerData.h"
#include "SegmentedWorldDoc.h"

#include <CryEditDoc.h>
#include "Objects/ObjectLayer.h"
#include "Objects/ObjectLayerManager.h"
#include <ISourceControl.h>


//#include "DBManager.h"
//#include "SegmentedWorldQueries.h"
//using namespace ns_SW_STATEMENTS;



using namespace sw;	// without this line VA will confuse
SW_NAMESPACE_BEGIN();

CBaseObject* CObjectDataAggr::CreateBaseObjectFromMetaData( REFGUID guid )
{
	if (guid == GUID_NULL)
		return NULL;

	if (!m_pGlobal->IsSWFindObjectEnabled())
		return NULL;

	TObjectData *pObjData = FindObjectData(guid);
	if(!pObjData)
		return NULL;

	CBaseObject *pResult;
	
	pObjData->UpdateStatus();
	CObjectArchive oa(GetIEditor()->GetObjectManager(), 0, true );
	bool bSWWasActive = CSegmentedWorldManager::IsActive();
	GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(true);
	//if (pObjData->NeedsToBeApplied())
	{
		pResult = pObjData->LoadBaseObject(oa, true);
		if (pResult)
		{
			pResult->SetSWMetaData(pObjData);
			AUTO_LOCK(g_cs);
			pObjData->SetNeededInWorld(true);
		}
	}
	oa.ResolveObjects();

	GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(bSWWasActive);


	return pResult;
}
TObjectData* CObjectDataAggr::GetObjectData( REFGUID guid, bool* pbCreated /*= NULL*/ )
{
	if (pbCreated) *pbCreated = false;

	AUTO_LOCK(g_cs);
	if (TObjectData* pObjData = FindObjectData(guid))
		return pObjData;
	
	// not found, then create one

	if (pbCreated) *pbCreated = true;

	return DoCreateObjectData(guid);
}

TObjectData* CObjectDataAggr::GetObjectData( CBaseObject* pObj, bool* pbCreated /*= NULL*/ )
{
	if (pbCreated) *pbCreated = false;

	if (TObjectData* pMetaData = FindObjectData(pObj))
		return pMetaData;

	// not found, the create one

	if (pbCreated) *pbCreated = true;

	// create a new one
	TObjectData* pObjData = DoCreateObjectData(pObj->GetId());
	pObj->SetSWMetaData(pObjData);
	return pObjData;
}

TObjectData* CObjectDataAggr::FindObjectData(REFGUID guid)
{
	AUTO_LOCK(g_cs);
	TObjectDataMap::iterator iter = m_objectDataMap.find(guid);
	if (iter != m_objectDataMap.end())
		return iter->second;
	return NULL;
}

TObjectData* CObjectDataAggr::FindObjectData( CBaseObject* pObj )
{
	return (TObjectData*)pObj->GetSWMetaData();
}

//-------------------------------------------------------------------------------------
bool CObjectDataAggr::IsExternal(CBaseObject *pObject, bool *pbCreatedNowOut)
{
	//return false;
	if(pbCreatedNowOut)
		*pbCreatedNowOut = false;

	if(!pObject)
		return false;
	if (pObject->GetGroup())
		return false;
	
	TObjectData* pObjData = FindObjectData(pObject);

	if (pObjData && pObjData->CheckFlags(EXTOBJFLAG_NOW))
		return true;

	if (pObject->GetParent())
		return false;


	if (IsObjectBig(pObject))
	{
		if(pbCreatedNowOut)
		{
			bool bCreatedNow;
			pObjData = GetObjectData(pObject, &bCreatedNow);
			// if just created, forward its state to applied
			if (bCreatedNow)
			{
				pObjData->GoToState(Versioned::S_Loaded);
				pObjData->GoToState(Versioned::S_Applied);
			}
			pObjData->SetFlags(EXTOBJFLAG_NOW);

			*pbCreatedNowOut = bCreatedNow;
		}

		return true;
	}

	return false;
}
//-------------------------------------------------------------------------------------
bool CObjectDataAggr::ObjectData_Load(bool bUpdateStatus)
{
	if(bUpdateStatus)
	{
		{
			AUTO_LOCK(g_cs);
			for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
			{
				TObjectData *pObjData = it->second;
				pObjData->SetNeededInWorld(false);
			}
		}
		ObjectData_UpdateStatus();
	}
	CObjectArchive oa(GetIEditor()->GetObjectManager(), 0, true );
	bool bSWWasActive = CSegmentedWorldManager::IsActive();
	GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(true);

	std::vector<TObjectData*> arrObjData;
	{
		AUTO_LOCK(g_cs);
		for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
		{
			TObjectData *pObjData = it->second;
			if (pObjData->NeedsToBeApplied())
			{
				if (CBaseObject* pBaseObj = pObjData->LoadBaseObject(oa, true))
					pBaseObj->SetSWMetaData(pObjData);
			}
		}
	}

	oa.ResolveObjects();

	GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(bSWWasActive);
	return true;
}
//-------------------------------------------------------------------------------------
bool CObjectDataAggr::ObjectData_Integrate(const TLocalCoords &lc, CBaseObjectsArray &arrObjects, int64 nVersion, EVersionType vt)
{
	ESaveFlag sflag = SF_IntegrateSave;
	assert(nVersion>0);
	assert(!(GetIEditor()->GetSegmentedWorldManager()->IsOfflineMode() && nVersion != 0));
	for (DWORD i = 0; i < arrObjects.size(); ++i) 
	{
		CBaseObject *pObject = arrObjects[i];
		if (!IsExternal(pObject))
			continue;

		TWorldCoords wc = lc;
		TWorldCoords obj_wc = m_pGlobal->WorldToSegment(pObject->GetWorldPos(), false);
		if(obj_wc.wx != wc.wx || obj_wc.wy != wc.wy)
			continue;

		//TObjectData *pObjData = ObjectData_Get(pObject->GetId(), true);
		assert(m_objectDataMap.find(pObject->GetId())!= m_objectDataMap.end());
		//if(sflag&SF_ForceSaveAll)
		//{
		//	if(m_pWorld->SegmentAggr().LockSegmentLayerData(lc, pObject->GetLayer()->GetGUID(), pObject->GetLayer()->GetName()))
		//		pObjData->SetParams(pObject);
		//}
		////TLocalCoords lc = WorldToSegment(pObject->GetPos());
		////pObjData->SaveObject(pObjData->m_wc/*, pObject*/, bModifiedOnly, nVersion, vt);
	}

	ObjectData_UpdateStatus();

	bool bModified = /*(sflag&SF_ForceSaveAll) ||*/ ObjectData_IsModified(lc);// IsLayersModified();

	if (!bModified && ObjectData_IsSaved(lc, vt)) //IsLayersSaved(vt))
		return true;
	
	std::vector<TObjectData*> arrObjectData;
	{
		AUTO_LOCK(g_cs);
		for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
		{
			TObjectData *pObjData = it->second;
			TWorldCoords wc = lc;
			if(pObjData->GetSegCoords() != wc)
				continue;

			if (!bModified && pObjData->IsSaved(vt))
				continue;
			arrObjectData.push_back(pObjData);
		}
	}

	for (int i = 0; i < arrObjectData.size(); i++)
	{
		TObjectData *pObjData = arrObjectData[i];
		GUID layerGUID;
		string strLayerName;
		{
			AUTO_LOCK(g_cs);
			layerGUID = pObjData->GetLayerGUID();
			strLayerName = pObjData->GetLayerName();
		}
		if (/*(vt != VT_HOT) &&*/ !m_pWorld->SegmentAggr().LockSegmentLayerData(lc, layerGUID, strLayerName))
			continue;

		GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(true);
		//pObjData->SaveObject(sflag, lc/*pObjData->m_wc*//*, pObject*/, nVersion, vt);

		if (!pObjData->PrepareData(sflag, vt, lc))
		{ assert(0); continue;}

		{
			AUTO_LOCK(g_cs);
			assert(TWorldCoords(lc) == pObjData->GetSegCoords());
			//pObjData->m_wc = TWorldCoords(lc);
		}
		pObjData->DoIntegrate( vt,nVersion);

		GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(false);
		// TO DO EngData
	}

	return true;
}
bool CObjectDataAggr::SaveForSegment(ESaveFlag sflag, const TLocalCoords &lc, CBaseObjectsArray &arrObjects, int64 nVersion, EVersionType vt)
{
	assert( (sflag&SF_WithSubmit && nVersion>0) || (!(sflag&SF_WithSubmit) && nVersion<=0) );
	//assert(!(s_nSWMode == 0 && nVersion != 0));

	// create external objects if needed
	for (DWORD i = 0; i < arrObjects.size(); ++i) 
	{
		CBaseObject *pObject = arrObjects[i];
		if (IsExternal(pObject))
		{
			TWorldCoords wc = lc;
			TWorldCoords obj_wc = m_pGlobal->WorldToSegment(pObject->GetWorldPos(), false);
			if(obj_wc.wx != wc.wx || obj_wc.wy != wc.wy)
				continue;

			TObjectData *pObjData = GetObjectData(pObject);
			if(sflag&SF_ForceSaveAll)
			{
				if(m_pWorld->SegmentAggr().LockSegmentLayerData(lc, pObject->GetLayer()->GetGUID(), pObject->GetLayer()->GetName()))
					pObjData->SetParams(pObject);
			}
			//TLocalCoords lc = WorldToSegment(pObject->GetPos());
			//pObjData->SaveObject(pObjData->m_wc/*, pObject*/, bModifiedOnly, nVersion, vt);
		}
	}

	ObjectData_UpdateStatus();

	bool bModified = (sflag&SF_ForceSaveAll) || ObjectData_IsModified(lc);// IsLayersModified();

	if (!bModified && ObjectData_IsSaved(lc, vt)) //IsLayersSaved(vt))
		return true;
	
	std::vector<TObjectData*> arrObjectData;
	{
		AUTO_LOCK(g_cs);
		for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
		{
			TObjectData *pObjData = it->second;
			TWorldCoords wc = lc;
			if(pObjData->GetSegCoords() != wc)
				continue;

			if (!bModified && pObjData->IsSaved(vt))
				continue;
			arrObjectData.push_back(pObjData);
		}
	}

	for (int i = 0; i < arrObjectData.size(); i++)
	{
		TObjectData *pObjData = arrObjectData[i];
		GUID layerGUID;
		string strLayerName;
		{
			AUTO_LOCK(g_cs);
			layerGUID = pObjData->GetLayerGUID();
			strLayerName = pObjData->GetLayerName();
		}
		if (/*(vt != VT_HOT) &&*/ !m_pWorld->SegmentAggr().LockSegmentLayerData(lc, layerGUID, strLayerName))
			continue;

		GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(true);
		if (pObjData->CheckDisapprove())
		{
			// reactive the xml node of the object
			GetInternalSWDoc()->SegmentAggr().SetLayerDataModified(lc,layerGUID,strLayerName);
			// then delete ExtObject
			DeleteExtObj(pObjData);
		}
		else
			pObjData->SaveObject(sflag, lc/*pObjData->m_wc*//*, pObject*/, nVersion, vt);
		GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(false);
		// TO DO EngData
	}

	//// delete those marked as deleted
	//for (TGuidList::const_iterator iter = m_lstObjectDeleted; iter != m_lstObjectDeleted.end(); ++iter)
	//{
	//	m_pGlobal->GetPersistence()-> (*iter;
	//}

	return true;
}
//-------------------------------------------------------------------------------------
void CObjectDataAggr::ObjectData_UpdateStatus()
{
	TObjectDataMap arrUpdatedObjectData;


	Recti rectWorld;
	{
		Recti rect = m_pWorld->GetLoadedRect();
		rectWorld.Min = rect.Min;
		rectWorld.Max.x = rect.Max.x - 1;
		rectWorld.Max.y = rect.Max.y - 1;
	}

	for(TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
	{
		TObjectData *pObjData = it->second;
		//Recti const& rcBBox = pObjData->GetSegBBox();

		//bool bInWorld = (pObjData->m_wcBoxMin.wx >= nMinX && pObjData->m_wcBoxMax.wx <= nMaxX &&
		//	pObjData->m_wcBoxMin.wy >= nMinY && pObjData->m_wcBoxMax.wy <= nMaxY) ||
		//	(pObjData->m_wc.wx >= nMinX && pObjData->m_wc.wx <= nMaxX && pObjData->m_wc.wy >= nMinY && pObjData->m_wc.wy <= nMaxY);
		bool bInWorld = rectWorld.InRect(pObjData->GetSegBBox()) || rectWorld.InRect(pObjData->GetSegCoords());

		if(!bInWorld)
			continue;

		CString pcFN = pObjData->GetFileName(*GetInternalSWDoc()->GetPersistence());
		ICryPak *pIPak = GetIEditor()->GetSystem()->GetIPak();
		_finddata_t fd;
		intptr_t fhandle;
		fhandle = pIPak->FindFirst(pcFN, &fd);
		if (fhandle != -1)
		{
			int64 nHRIDs[VT_COUNT];
			uint32 attr = CFileUtil::GetAttributes(pcFN);
			if(attr & SCC_FILE_ATTRIBUTE_MANAGED)
			{
				bool bOK = m_pVersionMgr->GetVersionControl()->GetRevIDs(*pObjData, nHRIDs);
				assert(bOK);
			}
			else
				nHRIDs[0] = 1;

			pObjData->SetAllVersions(nHRIDs);
			pObjData->SetMissingInDB(false);
			pObjData->SetNeededInWorld(true);
			pIPak->FindClose(fhandle);
		}
		else
		{
			pObjData->SetMissingInDB(true);
		}
	}
}
//-------------------------------------------------------------------------------------
void CObjectDataAggr::ObjectData_SetModified(CBaseObject *pObject, bool bDelete, CBaseObject* pLinkTarget)
{
	if (CSegmentedWorldManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return;
	if (GetIEditor()->IsInGameMode() || gEnv->IsEditorGameMode())
		return;
	if (GetISystem()->IsQuitting())
		return;
	if(GetIEditor()->GetSegmentedWorldManager()->IsIgnoringChanges())
		return;

	if(!IsExternal(pObject))
		return;

	if (pObject == pLinkTarget)
		return;

	// this is an undo operation for deleting object
	TObjectDataMap::iterator iterDeleted;
	if (!bDelete && (iterDeleted = m_deletedObjects.find(pObject->GetId())) != m_deletedObjects.end())
	{
		m_objectDataMap.insert(*iterDeleted);
		m_deletedObjects.erase(iterDeleted);
	}

	TObjectData *pObjData = GetObjectData(pObject);
	if (pObjData)
	{
		pObjData->SetParams(pObject);
		//pObjData->SetModified();
		pObjData->ModifiedLocally();

		if (bDelete)
		{
			std::vector<GUID> arrGuids;
			{
				AUTO_LOCK(g_cs);
				for (int i = 0; i < pObjData->m_arrLinkedFrom.size(); i++)
				{
					arrGuids.push_back(pObjData->m_arrLinkedFrom[i]);
				}
			}

			for (int i = 0; i < arrGuids.size(); i++)
			{
				CBaseObject *pObjectLinkFrom = GetIEditor()->GetObjectManager()->FindObject(arrGuids[i]);
				if (pObjectLinkFrom != pObject)
				{
					if (pLinkTarget)
						m_pWorld->OnObjectModified(pObjectLinkFrom, false, pLinkTarget);
					else
						m_pWorld->OnObjectModified(pObjectLinkFrom, false, pObject);
				}
			}

			// move to the 'deleted' list
			{
				TObjectDataMap::iterator iter = m_objectDataMap.find(pObjData->GetGUID());
				assert(iter != m_objectDataMap.end());
				m_deletedObjects.insert(*iter);
				m_objectDataMap.erase(iter);
				// Don't need to unbind, the CBaseObject is in Undo list, it might get resurrected
				// when Undo is cleared, then the CBaseObject will be actually deleted and Unbind will be called
				//iter->second->Unbind();
			}
		}
	}
}
//-------------------------------------------------------------------------------------
void CObjectDataAggr::UnloadAll()
{
	AUTO_LOCK(g_cs);
	for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
	{
		TObjectData* pObjData = it->second;
		delete pObjData;
	}
	m_objectDataMap.clear();
	m_deletedObjects.clear();
}

void CObjectDataAggr::DeleteExtObj(TObjectData* pObjData)
{
	GUID guid = pObjData->GetGUID();
	if (m_objectDataMap.erase(guid))
	{
		GetInternalSWDoc()->GetPersistence()->Delete(*pObjData);
		pObjData->UnloadAndUnlink(); // override default unload behavior
		delete pObjData;
	}
}

//-------------------------------------------------------------------------------------
bool CObjectDataAggr::ObjectData_NeedsToBeApplied()
{
	AUTO_LOCK(g_cs);
	for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
	{
		TObjectData* pObjData = it->second;
		if (pObjData->NeedsToBeApplied())
			return true;
	}
	return false;
}
//-------------------------------------------------------------------------------------
void CObjectDataAggr::ObjectData_Reload()
{
	std::vector<TObjectData*> arrObjectData;
	{
		AUTO_LOCK(g_cs);
		for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
		{
			TObjectData* pObjData = it->second;
			if (!pObjData->CheckFlags(EXTOBJFLAG_NOW))
				continue;
			if (pObjData->IsModified())
				continue;
			// TO DO add some checks, asserts

			//int64 nHRID = pObjData->m_nHRIDs[m_pVersionMgr->GetVersionType()];
			//if (pObjData->m_nLoadedHRID == nHRID)
			if (pObjData->LoadedVersionIsVT(m_pVersionMgr->GetVersionType()))
				continue;
			arrObjectData.push_back(pObjData);
		}
	}
	for (int i = 0; i < arrObjectData.size(); i++)
	{
		TObjectData* pObjData = arrObjectData[i];
		if (!pObjData->CheckFlags(EXTOBJFLAG_NOW))
			continue;
		if (pObjData->IsModified())
			continue;
		// TO DO add some checks, asserts
		EVersionType vt;
		{
			//some kind of double check
			AUTO_LOCK(g_cs);
			vt = m_pVersionMgr->GetVersionType();
			if (pObjData->LoadedVersionIsVT(vt))
				continue;
		}
		pObjData->LoadByVT( vt);
	}
}
//-------------------------------------------------------------------------------------
bool CObjectDataAggr::ObjectData_NeedsReload()
{
	if (!GetInternalSWDoc()->IsLayerDataNeeded())
		return false;

	AUTO_LOCK(g_cs);
	for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
	{
		TObjectData* pObjData = it->second;
		if (!pObjData->CheckFlags(EXTOBJFLAG_NOW))
			continue;
		if (!GetInternalSWDoc()->IsHot() && pObjData->IsModified())
			continue;

		// TO DO add some checks, asserts

		//int64 nHRID = pObjData->m_nHRIDs[m_pVersionMgr->GetVersionType()];
		//if (pObjData->m_nLoadedHRID == nHRID)
		if (pObjData->LoadedVersionIsVT(m_pVersionMgr->GetVersionType()))
			continue;
		
		return true;
	}
	return false;
}
//-------------------------------------------------------------------------------------
bool CObjectDataAggr::ObjectData_IsModified(const TLocalCoords &lc)
{
	AUTO_LOCK(g_cs);
	for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
	{
		TObjectData* pObjData = it->second;
		TWorldCoords wc = lc;
		if (!pObjData->CheckFlags(EXTOBJFLAG_NOW))
			continue;
		if(pObjData->GetSegCoords() != wc)
			continue;
		if (pObjData->IsModified())
			return true;
	}
	return false;
}

//-------------------------------------------------------------------------------------
bool CObjectDataAggr::ObjectData_IsSaved(const TLocalCoords &lc, EVersionType vt)
{
	AUTO_LOCK(g_cs);
	for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
	{
		TObjectData* pObjData = it->second;
		TWorldCoords wc = lc;
		if(pObjData->GetSegCoords() != wc)
			continue;
		if(!pObjData->IsSaved(vt))
			return false;
	}
	return true;
}
//-------------------------------------------------------------------------------------
void CObjectDataAggr::ObjectData_NoLongerApplied(const TWorldCoords &wc)
{
	std::vector<TObjectData*> arrObjectData;
	{
		AUTO_LOCK(g_cs);
		for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
		{
			TObjectData *pObjData = it->second;
			if(pObjData->GetSegCoords() != wc)
				continue;
			arrObjectData.push_back(pObjData);
		}
	}
	for (int i = 0; i < arrObjectData.size(); i++)
	{
		TObjectData *pObjData = arrObjectData[i];
		{
			AUTO_LOCK(g_cs);
			if(pObjData->GetSegCoords() != wc)
				continue;
		}
		pObjData->NoLongerApplied();
	}
}
void CObjectDataAggr::ObjectData_MakeExternal(TObjectData* pObjData, CBaseObject* pObject, bool bForceModify)
{
	if (CSegmentedWorldManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return;
	if (GetIEditor()->IsInGameMode() || gEnv->IsEditorGameMode())
		return;
	if (GetISystem()->IsQuitting())
		return;
	if(GetIEditor()->GetSegmentedWorldManager()->IsIgnoringChanges())
		return;

	if (!pObjData)
	{
		pObjData = GetObjectData(pObject);
	}

	bool bModifyObject = bForceModify;
	bool bModifyObjectParent = bForceModify;
	if (!pObjData->CheckFlags(EXTOBJFLAG_NOW))
	{
		GetInternalSWDoc()->OnObjectModified(pObject); // modify layerData
		pObjData->SetFlags(EXTOBJFLAG_NOW);
		bModifyObject = true;
	}

	if (pObject->GetParent())
	{
		if(bModifyObject)
			ObjectData_SetModified(pObject, false); // modify linked object
		CBaseObject* pAdamTarget = CSegmentedWorldDoc::GetRootObject(pObject);
		TObjectData* pObjDataAdamTarget = GetObjectData(pAdamTarget);
		if (!pObjDataAdamTarget->CheckFlags(EXTOBJFLAG_NOW))
		{
			pObjDataAdamTarget->SetFlags(EXTOBJFLAG_NOW); // parent must be external object too
			bModifyObjectParent = true;
		}
		if (bModifyObjectParent)
			ObjectData_SetModified(pAdamTarget, false); // modify parent of linked object
	}

	if (bModifyObject)
		GetInternalSWDoc()->OnObjectModified(pObject); // modify ObjectData
}
//-------------------------------------------------------------------------------------
// fill applied hrid's of all objects intersecting with segment
void CObjectDataAggr::ObjectData_GetAppliedHrids(const TLocalCoords &lc, std::vector<int64> &arrHrids)
{
	arrHrids.clear();
	AUTO_LOCK(g_cs);
	for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
	{
		TObjectData* pObjData = it->second;
		TWorldCoords wc = lc;
		if (!pObjData->CheckFlags(EXTOBJFLAG_NOW))
			continue;

		//if ((pObjData->m_wcBoxMin.wx > wc.wx) || (wc.wx > pObjData->m_wcBoxMax.wx))
		//	continue;
		//if ((pObjData->m_wcBoxMin.wy > wc.wy) || (wc.wy > pObjData->m_wcBoxMax.wy))
		//	continue;
		if (!pObjData->GetSegBBox().InRect((Vec2i)wc))
			continue;
		
		//arrHrids.push_back(pObjData->GetAppliedVersion());
		assert(pObjData->StateEqualTo(Versioned::S_Applied));
		arrHrids.push_back(pObjData->GetLoadedVersion());
	}
}
void CObjectDataAggr::InvalidateExternalObjects( REFGUID layerGUID, TLocalCoords const& lc )
{
	AUTO_LOCK(g_cs);
	TWorldCoords wc(lc);

	bool bWasActive = CSegmentedWorldManager::IsActive();
	for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
	{
		TObjectData* pObjData = it->second;
		if (pObjData->NeedsToBeApplied()) //already invalidated
			continue;

		if (pObjData->m_wc == wc && pObjData->m_LayerGUID == layerGUID)
		{
			if (pObjData->CheckFlags(EXTOBJFLAG_NOW))
			{
				if(pObjData->IsModified())
				{
					GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(true);
					pObjData->SaveObject(SF_NoSubmitSave, pObjData->m_lc/*pObjData->m_wc*//*, pObject*/, 0, m_pVersionMgr->GetVersionType());
					GetIEditor()->Get3DEngine()->SetSegmentOperationInProgress(bWasActive);
				}
			}
			pObjData->NoLongerApplied();
		}
	}
}
//-------------------------------------------------------------------------------------
void CObjectDataAggr::OnAddLink( CBaseObject *pObjectLinkOwner, CBaseObject *pObjectLinkTarget)
{
	// CS - need to fix external objects
	return;
	assert(pObjectLinkOwner);
	assert(pObjectLinkTarget);

	TObjectData* pObjDataOwner = GetObjectData(pObjectLinkOwner);
	TObjectData* pObjDataTarget = GetObjectData(pObjectLinkTarget);
	pObjDataOwner->SetParams(pObjectLinkOwner);
	char chLog[1024];
	char *pBuf = chLog;
	pBuf += sprintf(pBuf,"Linking %s to ", pObjDataOwner->GetGuidStr().c_str());
	sprintf(pBuf,"%s\n", pObjDataTarget->GetGuidStr());
	SWLog(chLog, true);
	{
		AUTO_LOCK(g_cs);
		pObjDataOwner->LinkTo(*pObjDataTarget);
	}
	
	//notification
	ObjectData_MakeExternal(pObjDataTarget, pObjectLinkTarget, true);
	//TO DO
}
//-------------------------------------------------------------------------------------
void CObjectDataAggr::OnRemoveLink( CBaseObject *pObjectLinkOwner, CBaseObject *pObjectLinkTarget)
{
	if (!pObjectLinkOwner || !pObjectLinkTarget)
	{
		assert(0);
		return;
	}

	TObjectData* pObjDataOwner = GetObjectData(pObjectLinkOwner);
	TObjectData* pObjDataTarget = GetObjectData(pObjectLinkTarget);

	char chLog[1024];
	char *pBuf = chLog;
	pBuf += sprintf(pBuf,"Removing Link from %s to ", pObjDataOwner->GetGuidStr());
	sprintf(pBuf,"%s\n", pObjDataTarget->GetGuidStr());
	SWLog(chLog, true);
	{
		AUTO_LOCK(g_cs);
		pObjDataOwner->UnlinkFrom(*pObjDataTarget);
	}

	//if (pObjDataTarget->m_arrLinkedFrom.size() == 0 && pObjDataTarget->CheckFlags(OBJDATAFLAG_EXTERNAL_NOW))
	//{
	//	CSWManager::OnObjectModified(pObjectLinkTarget);
	//	pObjDataTarget->ClearFlags(OBJDATAFLAG_EXTERNAL_NOW);
	//	CSWManager::OnObjectModified(pObjectLinkTarget);
	//}
	//TO DO
}

void CObjectDataAggr::ResetNeedInWorld()
{
	AUTO_LOCK(g_cs);
	for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
	{
		TObjectData *pObjData = it->second;
		pObjData->m_bNeedInWorld = false;
	}
}

void CObjectDataAggr::CleanAllUnappliedData( bool* pOutAnyCleaned, bool* pOutAnyDontNeed )
{
	bool bAnyCleaned = false, bAnyDontNeed = false;
	for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
	{
		TObjectData *pObjData = it->second;
		if(pObjData->NeedsToBeApplied()
			//&& pObjData->m_nAppliedHRID != HRID_UNKNOWN
			//&& !pObjData->AppliedVersionIs(HRID_UNKNOWN)
			&& !pObjData->StateEqualTo(Versioned::S_Unapplied)
			)
		{
			bAnyCleaned = true;
			pObjData->NoLongerApplied();
		}
		else
			bAnyDontNeed = true;
	}


	if (pOutAnyCleaned) *pOutAnyCleaned = bAnyCleaned;
	if (pOutAnyDontNeed) *pOutAnyDontNeed = bAnyDontNeed;
}

size_t CObjectDataAggr::FetchAll_NeedToBeApplied( std::vector<TObjectData*>& arrUnapplied )
{
	AUTO_LOCK(g_cs);

	size_t nInitialSize = arrUnapplied.size();
	for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
	{
		TObjectData *pObjData = it->second;
		if (pObjData->NeedsToBeApplied())
		{	
			arrUnapplied.push_back(pObjData);
		}
	}

	return arrUnapplied.size() - nInitialSize; // return how many has been added (fast,but not thread-safe) -RL
}

size_t CObjectDataAggr::FetchAll_External( std::vector<TObjectData*>& arrExternal )
{
	size_t nOriginalSize = arrExternal.size();
	for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
	{
		TObjectData* pObjData = it->second;

		if (!pObjData)
			continue;
		if (!pObjData->CheckFlags(EXTOBJFLAG_NOW | EXTOBJFLAG_VT_HOT | EXTOBJFLAG_VT_CURRENT | EXTOBJFLAG_VT_OFFICIAL))
			continue;

		arrExternal.push_back(pObjData);
	}

	return arrExternal.size() - nOriginalSize;
}

bool CObjectDataAggr::IntegrateExternalObjects(EVersionType vt, int64 nVersion )
{
	bool bRes = true;
	for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
	{
		TObjectData* pObjData = it->second;
		if (!pObjData)
			continue;
		if (!pObjData->CheckFlags(EXTOBJFLAG_NOW))
			continue;
		if (!m_pGlobal->IsSegmentSelected(pObjData->m_wc))
			continue;
		if (/*!(sflag&SF_ForceSaveAll) &&*/ pObjData->IsSaved(vt) && !pObjData->IsModified())
			continue;
		if (!m_pWorld->SegmentAggr().LockSegmentLayerData(pObjData->m_wc, pObjData->m_LayerGUID, pObjData->m_strLayerName))
			continue;

		if (!pObjData->DoIntegrate( vt,nVersion))
			bRes = false;
	}

	return bRes;
}
bool CObjectDataAggr::SaveToDB(ESaveFlag sflag, EVersionType vt, int64 nVersion )
{
	bool bRes = true;
	for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
	{
		TObjectData* pObjData = it->second;
		if (!pObjData)
			continue;
		if (!pObjData->CheckFlags(EXTOBJFLAG_NOW))
			continue;
		if (!m_pGlobal->IsSegmentSelected(pObjData->m_wc))
			continue;
		if (!(sflag&SF_ForceSaveAll) && pObjData->IsSaved(vt) && !pObjData->IsModified())
			continue;
		if (!m_pWorld->SegmentAggr().LockSegmentLayerData(pObjData->m_wc, pObjData->m_LayerGUID, pObjData->m_strLayerName))
			continue;

		if (!pObjData->DoSaveToDB(sflag, vt,nVersion))
			bRes = false;
	}

	// delete them in file but not in memory, if the deletion is undone we can recreate it in P4
	for (TObjectDataMap::iterator iter = m_deletedObjects.begin(); iter != m_deletedObjects.end(); ++iter)
	{
		GetInternalSWDoc()->GetPersistence()->Delete(*iter->second);
	}

	return bRes;
}

size_t CObjectDataAggr::FetchAll_InRect( std::vector<TObjectData*>& arrOut, const CRect & rect )
{
	size_t nOriginalSize = arrOut.size();
	for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it) 
	{
		TObjectData *pObjData = it->second;

		if (pObjData->m_wc.wx < rect.left)
			continue;
		if (pObjData->m_wc.wx > rect.right)
			continue;
		if (pObjData->m_wc.wy < rect.top)
			continue;
		if (pObjData->m_wc.wy > rect.bottom)
			continue;
		
		arrOut.push_back(pObjData);
	}

	return arrOut.size() - nOriginalSize;
}

void CObjectDataAggr::SetAllAsMissing()
{
	for (TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it) 
	{
		TObjectData *pObjData = it->second;
		pObjData->SetAsMissingInPersist();
	}
}

bool CObjectDataAggr::LoadObjLayerMapping(const TWorldCoords& wc)
{
	TPersistencePtr persist = GetInternalSWDoc()->GetPersistence();
	CString sXML = persist->GetEditorDataPath(wc) + SW_ObjectInfoFile;

	XmlNodeRef xml = XmlHelpers::LoadXmlFromFile(sXML);
	if(!xml || !xml->isTag("ObjLayerMappings"))
		return false;
	
	CXmlArchive xmlAr;
	xmlAr.bLoading = true;

	for(int i = 0; i < xml->getChildCount(); i++)
	{
		XmlNodeRef obj = xml->getChild(i);

		GUID guid;
		obj->getAttr("GUID", guid);
		TObjectData *pObjData = GetObjectData(guid);

		// read metadata info from extobj file
		sXML = pObjData->GetFileName(*persist);
		XmlNodeRef xmlObj = XmlHelpers::LoadXmlFromFile(sXML);
		if(xmlObj)
		{
			xmlAr.root = xmlObj->findChild("Object");
			pObjData->SerializeWithLookupTable(xmlAr);
		}
	}

	return true;
}

bool CObjectDataAggr::SaveObjLayerMapping()
{
	TObjectDataMap mapGlobalObj;
	m_mapObjLayer.clear();

	SegmentID segid;
	CString sXML;
	XmlNodeRef xml;
	CXmlArchive xmlAr;
	xmlAr.bLoading = false;
	Recti rcWorld = GetInternalSWDoc()->RcWorldBounds();
	TPersistencePtr persist = GetInternalSWDoc()->GetPersistence();
	for(TObjectDataMap::iterator it = m_objectDataMap.begin(); it != m_objectDataMap.end(); ++it)
	{
		TObjectData *pObjData = it->second;
		
		// save metadata info to extobj file
		sXML = pObjData->GetFileName(*persist);
		xml = XmlHelpers::LoadXmlFromFile(sXML);
		if(xml)
		{
			xmlAr.root = xml->findChild("Object");
			pObjData->SerializeWithLookupTable(xmlAr);
		}
		xml->saveToFile(sXML);
		
		// prepare obj map
		Recti rect = pObjData->GetSegBBox();
		if(!rcWorld.InRect(rect))
		{
			mapGlobalObj.insert(std::make_pair(pObjData->GetGUID(), pObjData));
			continue;
		}

		for(int y = rect.Min.y; y <= rect.Max.y; y++)
		{
			for(int x = rect.Min.x; x <= rect.Max.x; x++)
			{
				segid = MakeSegmentID(x, y);
				m_mapObjLayer[segid].push_back(pObjData);
			}
		}
	}

	{
		sXML = persist->GetLevelPath() + SW_WORLD_DATA_BASE_NAME + SW_ObjectInfoFile;
		xml = XmlHelpers::CreateXmlNode("ObjLayerMappings");
		if(xml)
		{
			for(TObjectDataMap::iterator it = mapGlobalObj.begin(); it != mapGlobalObj.end(); ++it)
			{
				XmlNodeRef obj = XmlHelpers::CreateXmlNode("Object");
				obj->setAttr("GUID", it->first);
				xml->addChild(obj);
			}
		}
		xml->saveToFile(sXML);
	}

	for(TObjectLayerMap::iterator it = m_mapObjLayer.begin(); it != m_mapObjLayer.end(); ++it)
	{
		TWorldCoords wc = FromSegmentID(it->first);
		sXML = persist->GetEditorDataPath(wc) + SW_ObjectInfoFile;
		xml = XmlHelpers::CreateXmlNode("ObjLayerMappings");
		if(!xml)
			continue;

		std::vector<TObjectData*> &objArray = it->second;
		for(int i = 0; i < objArray.size(); i++)
		{
			XmlNodeRef obj = XmlHelpers::CreateXmlNode("Object");
			obj->setAttr("GUID", objArray[i]->GetGUID());
			xml->addChild(obj);
		}

		xml->saveToFile(sXML);
	}

	return true;
}

void CObjectDataAggr::RecheckExternal( CBaseObject* pObject, CBaseObject* pLinkObject )
{
	if (pLinkObject)
		return;

	TObjectData* pObjData = FindObjectData(pObject);
	if (!pObjData || !pObjData->CheckFlags(EXTOBJFLAG_NOW))
		return;

	pObjData->ClearFlags(EXTOBJFLAG_DISAPPROVED);

	// if it WAS external but now it's not still "big" then turn it back to a non-external object
	if (!IsObjectBig(pObject))
	{
		pObjData->SetFlags(EXTOBJFLAG_DISAPPROVED);
	}
}

TObjectData* CObjectDataAggr::DoCreateObjectData( REFGUID guid )
{
	TObjectData *pObjData;
	TObjectDataMap::iterator iter;
	if ( m_deletedObjects.end() != (iter = m_deletedObjects.find(guid)))
	{
		pObjData = iter->second;
		m_deletedObjects.erase(iter);
	}
	else
		pObjData = new TObjectData(guid);

	m_objectDataMap.insert(std::make_pair(guid, pObjData));

	return pObjData;
}

SW_NAMESPACE_END();
