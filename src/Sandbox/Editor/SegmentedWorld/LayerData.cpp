#include "StdAfx.h"
#include "InternalCommon.h"
#include "LayerData.h"
#include "SegmentedWorldManager.h"
#include "SegmentedWorldDoc.h"
#include "ObjectData.h"
#include "ISWDataPersistence.h"
#include "ISWVersionControl.h"

#include "CryMemBlock.h"
#include "Objects/ObjectLayer.h"
#include "Objects/ObjectLayerManager.h"

using namespace sw;	// without this line VA will confuse

/////////////////////////////////////////////////////////////////////////////////////////
TLayerData::TLayerData()
:Versioned(VDT_LAYER)
{
	AUTO_LOCK(g_cs);
	Versioned::InitVersion();
	m_LayerGUID = GUID_NULL;
	m_strName.clear();
	m_nLayerID = -1;
}

TLayerData::TLayerData( REFGUID guid, string const& strLayerName, TWorldCoords const& wc, TLocalCoords const& lc )
:Versioned(VDT_LAYER)
{
	AUTO_LOCK(g_cs);
	Versioned::InitVersion();
	m_LayerGUID = guid;
	m_strName = strLayerName;
	m_parentLayerGUID = GUID_NULL;
	m_bIsAParent = false;
	m_nLayerID = -1;
	m_wc = wc;
	m_lc = lc;
}

//---------------------------------------------------------------------------------------
TLayerData::~TLayerData()
{
	Unload();
}
//---------------------------------------------------------------------------------------
void TLayerData::Unload()
{
	AUTO_LOCK(g_cs);
	Versioned::Reset();
	m_data.Free();
	//m_strName.clear();
	m_nLayerID = -1;
}

//---------------------------------------------------------------------------------------
bool TLayerData::Load(const TWorldCoords &wc, int64 nHRID)
{
	//#if !defined(USE_LOCAL_SAVE) || defined(USE_NATIVE_VC) || defined(USE_NULL_VC)
	if (Versioned::LoadedVersionIs(nHRID))
		return true;
	
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

	CSegmentedWorldDoc *psw = GetInternalSWDoc();

	{
		if (!psw->GetPersistence()->LoadRevision(*this,nHRID,m_data))
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
		int nLayerID;
		bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(nLayerID) || nLayerID < 0 || nLayerID != m_nLayerID;
		char* szLayerName;
		bCorrupted = bCorrupted || !arrAdditional[nCol++].SafeGet(szLayerName) || !szLayerName;
		m_strName.assign(szLayerName);

		if (bCorrupted)
		{
			assert(!"Database corrupted!");
			Unload();
			return false; 
		}
#endif

		char chLog[256]; sprintf(chLog, "Loaded (%d,%d):%s(HRID %I64d, size %d)\n", wc.wx, wc.wy, m_strName, nHRID, m_data.GetUncompressedSize());
		SWLog(chLog);

		Versioned::OnVersionLoaded(nHRID);

		return true;
	}
	return false;
}

//-------------------------------------------------------------------------------------
bool TLayerData::SaveData(int64 nVersion, EVersionType vt)
{
	if(/*bNeedCreate*/IsMissingInPersist())
		if (!CreateData())
		{
			assert(0);
			return false;
		}

		return GetInternalSWDoc()->GetPersistence()->CheckoutSaveSubmit(*this,m_data, nVersion, vt);
}

//-------------------------------------------------------------------------------------
void TLayerData::AppliedLocally(const TLocalCoords &lc)
{
	Versioned::OnAppliedLocally();
	m_lc = lc;
}
//-------------------------------------------------------------------------------------
bool TLayerData::SaveLayer( ESaveFlag sflag, const TLocalCoords &lc, CBaseObjectsArray &arrObjects, int64 nVersion, sw::EVersionType vt )
{
	assert( (sflag&SF_WithSubmit && nVersion>0) || (!(sflag&SF_WithSubmit) && nVersion<=0) );
	//assert(!(CSWManager::Get().IsOfflineMode() && nVersion != 0));

	if (!PrepareData( sflag, lc, arrObjects))
		return false;

	return DoSaveToDB(sflag, vt, nVersion);
}

bool TLayerData::DoIntegrate( sw::EVersionType vt, int64 nVersion )
{
	assert( nVersion>0 /*&& vt == VT_OFFICIAL*/ );
	//if (!(sflag&SF_WithSubmit))
	//	return false; // TODO: why this is different with that in TObjectData::DoSaveToDB() ?? -RL

	if (/*(vt != VT_HOT) &&*/ !Lock())
		return false;

	Unlock();

	if (Versioned::DoIntegrateOnly( vt, nVersion))
		return true;

	return false;
}

bool TLayerData::DoSaveToDB( ESaveFlag sflag, sw::EVersionType vt, int64 nVersion )
{
	assert((sflag&SF_WithSubmit && nVersion>0) || (!(sflag&SF_WithSubmit)&&nVersion<=0));
	if (!(sflag&SF_WithSubmit))
		return false; // TODO: why this is different with that in TObjectData::DoSaveToDB() ?? -RL

	if (/*(vt != VT_HOT) &&*/ !Lock())
		return false;

	//bool bVersionHistorySaved = false;
	if ( Versioned::IsLoadedDataACache() ) 
	{
		return Versioned::DoSave(sflag,vt,nVersion);
	}
			assert(!(sflag&SF_AsIntegrate));

	return false;

}
//-------------------------------------------------------------------------------------
bool TLayerData::CreateData()
{
	//	AUTO_LOCK(g_cs);
	CSegmentedWorldDoc *psw = GetInternalSWDoc();

	//LayerID
	bool bNeedCreateObjectLayer = false;
	string strName;
	GUID layerGUID;
	{
		AUTO_LOCK(g_cs);
		if (m_nLayerID == -1) //means missing
		{	
			if (m_strName.empty())
				assert(0);

			if (m_LayerGUID == GUID_NULL)
				assert(0);

			bNeedCreateObjectLayer = true;
			strName = m_strName;
			layerGUID = m_LayerGUID;
		}
	}

	if (bNeedCreateObjectLayer)
	{
		int nLayerID;

		nLayerID = psw->LayerInfoMgr().RegisterNewLayer(layerGUID, strName.c_str(), GUID_NULL);
		AUTO_LOCK(g_cs);
		m_nLayerID = nLayerID;

	}

	bool bMissing;
	{
		AUTO_LOCK(g_cs);
		if (m_nLayerID == -1)
		{
			assert(0);
			return false;
		}
		bMissing = Versioned::IsMissingInPersist();
	}
	if (bMissing) 
	{
		{
			TWorldCoords wc;
			int layerID;
			{
				AUTO_LOCK(g_cs);
				wc = m_wc;
				layerID = m_nLayerID;
			}
			int idSeg = MakeSegmentID(wc);

			bool bRet = psw->GetPersistence()->Add(*this);
			assert(bRet);
		}		
	}
	assert(!Versioned::IsMissingInPersist() && m_nLayerID != -1);
	return true;
}

bool TLayerData::IsMissingInPersist() const
{
	return Versioned::IsMissingInPersist() || m_nLayerID == -1;
}

//-------------------------------------------------------------------------------------
void TLayerData::NoLongerApplied()
{
	AUTO_LOCK(g_cs);
	//Versioned::SetAppliedHRID(HRID_UNKNOWN);
	GoToState(S_Unapplied);
	CSegmentedWorldDoc *psw = GetInternalSWDoc();
	psw->ObjectAggr().InvalidateExternalObjects(m_LayerGUID, m_lc );
}

//-------------------------------------------------------------------------------------
void TLayerData::OnLock()
{
	TSuper::OnLock();
	GetIEditor()->Notify( eNotify_OnSWLockUnlock );
}

bool TLayerData::PrepareData( ESaveFlag sflag, const TLocalCoords & lc, CBaseObjectsArray &arrObjects )
{
	CSegmentedWorldDoc *psw = GetInternalSWDoc();
	AABB bb = psw->CalcWorldBounds(lc);
	DWORD dwCount = 0;
	DWORD dwExternalObjectCount = 0;

	//*******************************
	// TODO: new implementation with hierarchical ObjectLayer back

	/*
	XmlNodeRef LayersContainer = XmlHelpers::CreateXmlNode("InterableLayersRoot");
	CObjectArchive oa(GetIEditor()->GetObjectManager(), LayersContainer, false );

	CObjectLayerManager* pLayerMgr = GetIEditor()->GetObjectManager()->GetLayersManager();
	CObjectLayer* pLayer = pLayerMgr->FindLayer(this->m_LayerGUID);
	//pLayerMgr->SaveExternalLayer(pLayer, oa, &filterWrapper );
	pLayerMgr->ExportLayerIterableAddSelf(pLayer,oa);

	// add the "LayerObjects" node for each layer
	for (uint ii = 0; ii < LayersContainer->getChildCount(); ++ii)
		LayersContainer->getChild(ii)->addChild(XmlHelpers::CreateXmlNode("LayerObjects"));

	// only operate the current layer
	XmlNodeRef nodeCurLayer = LayersContainer->getChild(0);			//< the first child should be the current layer. -RL
	assert(nodeCurLayer);
	assert( pLayer->GetName() == nodeCurLayer->getAttr("Name") );
	XmlNodeRef nodeLayerObjects = nodeCurLayer->findChild("LayerObjects");

	// populate layers with objects belong to them
	for(uint i = 0; i < arrObjects.size(); ++i)
	{
		CBaseObject *po = arrObjects[i];
		assert(po);
		{
			AUTO_LOCK(g_cs);
			if (0 != strcmpi(po->GetLayer()->GetName(), m_strName))
				continue;
		}
		if (psw->ObjectData_IsNeeded(po)) // this object will be saved with ObjectData
		{
			++dwExternalObjectCount;
			continue;
		}

		oa.node = nodeLayerObjects->newChild("Object");
		psw->SaveObjectWithChilds(po, oa);
		//oa.SaveObject(po);

		++dwCount;
	}

	assert(nodeCurLayer->findChild("LayerObjects"));
	XmlNodeRef objRoot = nodeCurLayer;
	//*/


	//******************************
	// TODO: the old way shall be replaced

	//*
	XmlNodeRef objRoot = XmlHelpers::CreateXmlNode("Objects");
	CObjectArchive oa(GetIEditor()->GetObjectManager(), objRoot, false );
	for (DWORD i = 0; i < arrObjects.size(); ++i) 
	{
		CBaseObject *po = arrObjects[i];
		if (!po)
			continue;
		if (po->GetParent())
			continue;
		if (po->GetGroup())
			continue;
		{
			AUTO_LOCK(g_cs);
			if (strcmpi(po->GetLayer()->GetName(),m_strName))
				continue;
		}

		assert(ObjInBox(bb, po->GetPos()));

		if (psw->ObjectAggr().IsExternal(po)) // this object will be saved with ObjectData
		{
			++dwExternalObjectCount;
			continue;
		}

		++dwCount;

		oa.node = objRoot->newChild("Object");
		assert(TWorldCoords(lc) == m_wc);
		psw->SaveObjectWithChildren(po, oa, m_wc);
		//po->Serialize(oa);
	}
	//*/

	bool bNeedSave = dwCount && (IsModified() || (sflag&SF_ForceSaveAll));
	bool bAppliedIsHRID_EDITOR;
	{
		AUTO_LOCK(g_cs);
		//bAppliedIsHRID_EDITOR = (m_nAppliedHRID == HRID_EDITOR);
		//bAppliedIsHRID_EDITOR = Versioned::AppliedVersionIs(HRID_EDITOR);
		bAppliedIsHRID_EDITOR = (m_state == S_Modified);
	}
	if (bNeedSave && ((sflag&SF_ForceSaveAll) || bAppliedIsHRID_EDITOR))
	{
		if (dwCount) 
		{
			if (psw->GetSpecialOperationType() == CSegmentedWorldDoc::SW_SO_XMLS_EXEC_SCRIPT_AND_SAVE)
				psw->CallScript(objRoot);
			XmlString str = objRoot->getXML();

			CMemoryBlock tmp;
			tmp.Attach((void *) str.c_str(), str.size());
			AUTO_LOCK(g_cs);
#ifndef SW_NO_DATACOMPRESS
			tmp.Compress(m_data);
#else
			// Memory block treat Size as alloc size, and it prevent realloc if new data
			// is smaller than orginal one. so to make the Size as real data size, we have to
			// free before assign.
			// TODO: the better way to do it is: use MemoryBlock as raw container, content should
			// manage the size by itself.
			m_data.Free(); 
			m_data = tmp;
#endif
		}
		else
		{
			AUTO_LOCK(g_cs);
			m_data.Free();
		}
		AppliedLocally(lc);
	}

	return bNeedSave;
}

CString TLayerData::GetFileName(CSCDataPersistence& persist) const
{
	CString sFileName;
	CString sPath;
	
	sPath = persist.GetEditorDataPath(m_wc);

	CLayerInfoManager::TLayerInfo* pInfo = GetInternalSWDoc()->LayerInfoMgr().FindLayer(m_LayerGUID);
	assert(pInfo);
	sFileName.Format("%s%s%s%s", sPath, LAYER_PATH, pInfo->m_strFullName, LAYER_FILE_EXTENSION);
	Path::ConvertSlashToBackSlash(sFileName);

	return sFileName;
}

bool TLayerData::Submit()
{
	return true;
}


