#include "stdafx.h"
#include "InternalCommon.h"
#include "WorldDataAggregator.h"

#include "WorldData.h"
#include "SegmentedWorldDoc.h"

#include "Terrain/TerrainManager.h"
#include "Terrain/SurfaceType.h"
#include "TerrainTexturePainter.h"

#include "GameExporter.h"
#include "VegetationTool.h"
#include "VegetationMap.h"
#include "ISourceControl.h"
#include "GameEngine.h"

using namespace sw;

SW_NAMESPACE_BEGIN();

CWorldDataAggre::CWorldDataAggre(TGlobal* pGlobal, TWorld* pWorld, TVersionMgr* pVersionMgr)
: m_pGlobal(pGlobal)
, m_pWorld(pWorld)
, m_pVersionMgr(pVersionMgr)
{
	for(int i = 0; i < WDB_COUNT; i++)
	{
		m_pWorldData[i] = new TWorldData();
		m_pWorldData[i]->m_eType = (EWDBType)i;
		m_pWorldData[i]->m_subtype = i;
	}
}

CWorldDataAggre::~CWorldDataAggre()
{
	for(int i = 0; i < WDB_COUNT; i++)
	{
		SAFE_DELETE(m_pWorldData[i]);
	}
}

bool CWorldDataAggre::WorldData_LoadToArchiveArray(TDocMultiArchive &arrXmlAr)
{
	WorldData_UpdateStatus();

	AUTO_LOCK(g_cs);
	FillXmlArArray(arrXmlAr, NULL);

	for (int i = 0; i < WDB_COUNT; i++)
	{
		TWorldData& worlddata = *m_pWorldData[i];
		EVersionType vt = worlddata.GetWorldDataNeededVersionType();//m_pVersionMgr->GetVersionType();

		if (!worlddata.LoadByVT(vt)) 
		{
			assert(0);
			continue;
		}

		// xml blocks have more trivial work to do
		if (i < WDB_XMLCOUNT)
		{
			m_pWorldData[i]->m_xmlAr->bLoading = true;
			arrXmlAr[i] = m_pWorldData[i]->m_xmlAr;

			// will be set to applied when data are actually reloaded by Editor
			//m_pWorldData[i].SetAppliedHRID(m_pWorldData[i].m_nLoadedHRID);
			//m_pWorldData[i].ApplyLoadedVersion();
		}
	}

	return true;
}

bool CWorldDataAggre::WorldData_UpdateStatus(int eWDType)
{
	if (eWDType != -1) {
		m_pWorldData[eWDType]->UpdateStatus();
		return true;
	}

	for (int iWBT = 0; iWBT < WDB_COUNT; iWBT++) {
		m_pWorldData[iWBT]->UpdateStatus();
	}

	return true;
}

const char *CWorldDataAggre::WorldData_GetStatusText(char *pBuf, int eWDType)
{
	AUTO_LOCK(g_cs);

	TWorldData& wd = *m_pWorldData[eWDType];
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
					,m_pVersionMgr->GetVersionControl()->GetLockedBy(wd,szLockedBy,ISWVersionControl::MAX_USERNAME_CBSIZE,"<N/A>")
					,DataInspector::m_nLockedBy(wd));
			else
				pBuf += sprintf(pBuf, "LOCKED by %s(%d)"
					,m_pVersionMgr->GetVersionControl()->GetLockedBy(wd,szLockedBy,ISWVersionControl::MAX_USERNAME_CBSIZE,"<N/A>")
					, DataInspector::m_nLockedBy(wd));

			if (!wd.LoadedVersionIsVT(m_pVersionMgr->GetVersionType()))
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

bool CWorldDataAggre::WorldData_IsModified()
{
	AUTO_LOCK(g_cs);
	for (int i = 0; i < WDB_COUNT; i++)
	{
		if (m_pWorldData[i]->IsModified())
			return true;
	}
	return false;
}

bool CWorldDataAggre::GetGDStatus(int nWDBType, int &nWDBState, int &nLockedBy)
{
	if ( CSegmentedWorldManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return false;
	if (GetIEditor()->IsInGameMode() || gEnv->IsEditorGameMode())
		return false;
	if (GetISystem()->IsQuitting())
		return false;

	if (m_pGlobal->m_bSuspendThread)
		return false;
	AUTO_LOCK_BIG(g_csBigLock);
	if (CSegmentedWorldManager::IsActive() || !GetIEditor()->GetDocument()->IsDocumentReady())
		return false;
	if (GetIEditor()->IsInGameMode() || gEnv->IsEditorGameMode())
		return false;
	if (GetISystem()->IsQuitting())
		return false;


	if (m_pWorldData[nWDBType]->IsMissingInPersist())
	{
		nLockedBy = 0;
		nWDBState = sw::WDBState_Missing;
		return true;
	} 
	else 
	{
		nLockedBy = DataInspector::m_nLockedBy(m_pWorldData[nWDBType]);

		//if (m_pWorldData[nWDBType].m_nLoadedHRID != m_pWorldData[nWDBType].m_nHRIDs[m_pVersionMgr->GetVersionType()])
		if (!m_pWorldData[nWDBType]->LoadedVersionIsVT(m_pVersionMgr->GetVersionType()))
		{
			//nLockedBy = m_pWorldData[nWDBType].m_nLockedBy;
			if (m_pWorldData[nWDBType]->IsLockedByMe())
			{
				nWDBState = sw::WDBState_Conflicted;
				return true;
			}

			nWDBState = sw::WDBState_OutOfDate;
			return true;
		}

		if (!m_pWorldData[nWDBType]->IsLocked())
		{
			//nLockedBy = 0;
			nWDBState = sw::WDBState_Default;
			return true;
		}	
		else if (m_pWorldData[nWDBType]->IsLockedByMe())
		{
			//nLockedBy = m_pWorldData[nWDBType].m_nLockedBy;
			nWDBState = sw::WDBState_LockedByMe;
			return true;
		}	
		else
		{
			//nLockedBy = m_pWorldData[nWDBType].m_nLockedBy;
			nWDBState = sw::WDBState_LockedByOther;
			return true;
		}
		
	}

	CRY_ASSERT_MESSAGE(0,"missing condition");
	return true;
}

bool CWorldDataAggre::WorldData_ReloadTerrainLayers()
{
	AUTO_LOCK(g_cs);
	//int nType = (int)WDB_TERRAIN_LAYERS;
	TWorldData& worlddata = *m_pWorldData[WDB_TERRAIN_LAYERS];


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

	GetIEditor()->GetTerrainManager()->SerializeSurfaceTypes( (*worlddata.m_xmlAr) );
	GetIEditor()->GetTerrainManager()->SerializeLayerSettings( (*worlddata.m_xmlAr) );
	
	GetIEditor()->GetTerrainManager()->ReloadSurfaceTypes();

	m_pWorld->ReloadSegmentLayerId();

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

bool CWorldDataAggre::WorldData_ReloadVegetationMap()
{
	AUTO_LOCK(g_cs);
	//int nType = WDB_VEGETATION;
	TWorldData& worlddata = *m_pWorldData[WDB_VEGETATION];
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

	CVegetationMap* pVegMap = GetIEditor()->GetVegetationMap();

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

	m_pWorld->ReloadSegmentVegetationInstance();

	GetIEditor()->GetDocument()->SetDocumentReady(true);
	GetIEditor()->GetTerrainManager()->ReloadSurfaceTypes(true, false);

	if (bNeedToResetTool) 
	{
		pTool = new CVegetationTool;
		GetIEditor()->SetEditTool( pTool );
		GetIEditor()->SelectRollUpBar( ROLLUP_TERRAIN );
		AfxGetMainWnd()->RedrawWindow(NULL,NULL,RDW_INVALIDATE|RDW_ALLCHILDREN);
	}

	return true;
}

bool CWorldDataAggre::WorlData_ReloadIfNeeded()
{
	// reload if needed
	bool bRes = true;
	bool bNeedReload;
	{
		AUTO_LOCK(g_cs);
		bNeedReload = !m_pWorldData[WDB_TERRAIN_LAYERS]->LoadedVersionIsVT(m_pVersionMgr->GetVersionType());
	}
	if (bNeedReload)
	{
		CRY_ASSERT(!m_pWorldData[WDB_TERRAIN_LAYERS]->IsLocked());
		bRes = WorldData_ReloadTerrainLayers();
	}

	{
		AUTO_LOCK(g_cs);
		bNeedReload = !m_pWorldData[WDB_VEGETATION]->LoadedVersionIsVT(m_pVersionMgr->GetVersionType());
	}
	if (bNeedReload)
	{
		CRY_ASSERT(!m_pWorldData[WDB_VEGETATION]->IsLockedByMe());
		bRes = WorldData_ReloadVegetationMap() && bRes;
	}
	return bRes;
}

bool CWorldDataAggre::WorldData_PrepareData( uint32 flagNeedSave, sw::ESaveFlag sflag, sw::EVersionType vt, sw::ELockResult* arrSaves )
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
			arrSaves[i] = m_pWorldData[i]->Lock();
		else
		{
			if (i == WDB_LEVELGENERAL)
			{
				//always save when is Locked WDB_GD or WDB's which have not notification callbacks
				arrSaves[i] = ( (m_pWorldData[i]->IsLockedByMe() &&	m_pWorldData[i]->LoadedVersionIsVT(VT_CURRENT)) ? LockResult_AlreadyHave:LockResult_Failed );
			}
			else
			{
				//save if modified
				arrSaves[i] = ( (m_pWorldData[i]->IsLockedByMe() &&	m_pWorldData[i]->IsModified()) ? LockResult_AlreadyHave : LockResult_Failed );
			}
		}

		//SAFE_DELETE(m_pWorldData[i].m_xmlAr);
		//if (arrSaves[i] && i != WDB_LEVELPAK)
		//{
		//	if (i == WDB_GD)
		//		m_pWorldData[i].m_xmlAr = new CXmlArchive();
		//	else
		//		m_pWorldData[i].m_xmlAr = new CXmlArchive(g_pcWDBNodeNames[i][WDBNT_ROOT]);

		//	m_pWorldData[i].m_xmlAr->bLoading = false;
		//}
		//arrXmlAr[i] = m_pWorldData[i].m_xmlAr;

		if (arrSaves[i] || !m_pWorldData[i]->IsSaved(vt))
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
		SAFE_DELETE(m_pWorldData[i]->m_xmlAr);
		//if (arrSaves[i] && i != WDB_LEVELPAK)
		{
			if (i == WDB_LEVELGENERAL)
				m_pWorldData[i]->m_xmlAr = new CXmlArchive();
			else
				m_pWorldData[i]->m_xmlAr = new CXmlArchive(GetWorldDataBlockName((EWDBType)i));

			m_pWorldData[i]->m_xmlAr->bLoading = false;
		}
		arrXmlAr[i] = m_pWorldData[i]->m_xmlAr;

		m_pWorldData[i]->GoToState(Versioned::S_Modified);
	}
	//GetIEditor()->GetHeightmap()->SetUnitSize(1);
	GetIEditor()->GetDocument()->Save(arrXmlAr);

	// ===== Add Version attrib =====
	{
		CXmlArchive& xmlArLevel = *arrXmlAr[WDB_LEVELGENERAL];
		assert(xmlArLevel.root && xmlArLevel.root->isTag("Level"));
		xmlArLevel.root->setAttr(g_szSWDStructVerAttr, CSegmentedWorldManager::GetSWDataStructureVersion());
	}


	// == save level.pak files ==
	if (GetIEditor()->GetSegmentedWorldManager()->IsMapConverting() /* && flagNeedSave&(1<<WDB_LEVELPAK)*/)
	{
		assert(GetIEditor()->GetSegmentedWorldManager()->IsAutomate());
		//	if (arrSaves[WDB_LEVELPAK] == 0)
		//		arrSaves[WDB_LEVELPAK] = m_pWorldData[WDB_LEVELPAK].Lock();

		//	if (arrSaves[WDB_LEVELPAK])
		{
			if (GetIEditor()->GetSegmentedWorldManager()->IsAutomate())
			{
				GetIEditor()->GetGameEngine()->SetLevelPath(m_pGlobal->GetLevelPath());
			}
			int flags = eExp_ReloadTerrain;
			CGameExporter gameExporter;
			gameExporter.Export(flags, eLittleEndian, ".");

			//((CCryEditApp*)AfxGetApp())->OnFileExportToGameNoSurfaceTexture();
		}

		//	m_pWorldData[WDB_LEVELPAK].GoToState(Versioned::S_Modified);
	}

	return true;
}

bool CWorldDataAggre::WorldData_Save( ESaveFlag sflag, sw::EVersionType vt, int64 nVersion /*= -1*/, EWDBType nWDBType /*= (int)sw::WDB_COUNT*/ )
{
	// make a set of flags for what type we want to process
	assert(WDB_COUNT<32);
	size_t nWDBFlags = 1<<nWDBType;
	if (nWDBType == WDB_COUNT)
		nWDBFlags -= 1; //< little trick

	return WorldData_Save(sflag,vt,nVersion,nWDBFlags);
}

bool CWorldDataAggre::WorldData_Save( ESaveFlag sflag, sw::EVersionType vt, int64 nVersion /*= -1*/, uint32 nWDBFlags /*= (1<<WDB_COUNT)-1*/ )
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
			if (!m_pWorldData[i]->DoSave(sflag,vt,nVersion))
					{
						if (arrSaves[i] == LockResult_Succeeded)
							m_pWorldData[i]->Unlock();
					bRes = false;
				}
			}
			assert(!(sflag&SF_AsIntegrate));
			//else if (sflag&SF_AsIntegrate)
			//{
			//m_pWorldData[i].DoIntegrateOnly(vt,nVersion);
			//}
	}
	//}
	return bRes;
}

bool CWorldDataAggre::WorldDataCC(EWDBType nWDBType, int nWDBCC)
{
	AUTO_LOCK_BIG(g_csBigLock);
	if (GetIEditor()->GetSegmentedWorldManager()->IsOfflineMode()) return true;
	bool bRes = false;
	int64 nVersion = -1;
	switch (nWDBCC)
	{
	case sw::WDBCC_Lock:
		{
			bRes = (m_pWorldData[nWDBType]->Lock() != 0);
		}
		break;
	case sw::WDBCC_Save:
		{
			m_pGlobal->m_bSuspendThread = false;
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
			//	m_pWorldData[nWDBType].Revert(VT_OFFICIAL);
			//	if (nWDBType == WDB_TERRAIN_LAYERS)
			//	{
			//		bRes = WorldData_ReloadTerrainLayers();
			//	} 
			//	else if (nWDBType == WDB_VEGETATION)
			//	{
			//		bRes = WorldData_ReloadVegetationMap();
			//	}
			//	m_pWorldData[nWDBType].Unlock();
			//}
			//else
			//{
			//	bRes = WorldData_Revert(VT_OFFICIAL);
			//}
		}
		break;
	case sw::WDBCC_Unlock:
		{
			bRes = m_pWorldData[nWDBType]->Unlock();
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

bool CWorldDataAggre::TryLockModified(std::vector<TWorldData*> &wdJustLocked)
{
	WorldData_UpdateStatus();
	for (int i = 0; i < WDB_COUNT; i++)
	{
		if (m_pWorldData[i]->IsModified())
		{
			//bool bVersionConflict = m_WorldData[i].m_nHRIDBeforeModify != m_WorldData[i].m_nHRIDs[m_vt];
			bool bVersionConflict = m_pWorldData[i]->IsVersionConflicting(m_pVersionMgr->GetVersionType());
			if (bVersionConflict)
				return false;
			
			ELockResult nLockRes = (ELockResult)m_pWorldData[i]->Lock();
			if (nLockRes == LockResult_Failed)
				return false;
			if (nLockRes == LockResult_Succeeded)
				wdJustLocked.push_back(m_pWorldData[i]);
		}
	}
	WorldData_UpdateStatus();

	return true;
}

bool CWorldDataAggre::SetModified(int nType)
{
	if (m_pWorldData[nType]->Lock()) {
		m_pWorldData[nType]->ModifiedLocally();
		return true;
	}
	AUTO_LOCK(g_cs);
	CString msg;
	char szLockedBy[ISWVersionControl::MAX_USERNAME_CBSIZE];
	msg.Format("Access denied!!!\r\n\r\n%s is Locked by %s(%d)!",
		GetWorldDataBlockName((EWDBType)nType),
		//g_db->GetUserName( DataInspector::m_nLockedBy(m_WorldData[nType])),
		m_pVersionMgr->GetVersionControl()->GetLockedBy(*m_pWorldData[nType],szLockedBy,ISWVersionControl::MAX_USERNAME_CBSIZE,"<N/A>"),
		DataInspector::m_nLockedBy(m_pWorldData[nType]));
	AfxMessageBox( _T(msg), MB_ICONEXCLAMATION|MB_OK );
	return false;
}

bool CWorldDataAggre::HasConflict()
{
	bool bConflictFound = false;
	for (int i = 0; i < WDB_COUNT; i++)
	{
		//if (m_WorldData[i].m_nHRIDBeforeModify != m_WorldData[i].m_nHRIDs[m_vt])
		if (m_pWorldData[i]->IsVersionConflicting(m_pVersionMgr->GetVersionType()))
		{
			bConflictFound = true;
			break;
		}

		if (!bConflictFound && GetIEditor()->GetSegmentedWorldManager()->IsOfflineMode() && 
				m_pWorldData[i]->IsLocked()&& 
				!m_pWorldData[i]->IsLockedByMe()&& 
				m_pWorldData[i]->IsModified())
		{
			bConflictFound = true;
		}

		if (!bConflictFound && !GetIEditor()->GetSegmentedWorldManager()->IsOfflineMode() &&
				!m_pWorldData[i]->IsLockedByMe()&& 
				m_pWorldData[i]->IsModified())
		{
			bConflictFound = true;
		}
	}
	return bConflictFound;
}

bool CWorldDataAggre::Lock()
{
	for (int i = 0; i < WDB_COUNT; i++)
	{
		if (m_pWorldData[i]->Lock() == 0)
			return false;
	}
	return true;
}

void CWorldDataAggre::SetStatus(sw::Versioned::EState state)
{
	for (int ii = 0; ii < WDB_XMLCOUNT; ++ii)
	{
		TWorldData& wd = *m_pWorldData[ii];
		wd.GoToState(state);
	}
}

bool CWorldDataAggre::NeedApply()
{
	bool bAnyNeedApply = false;
	for (int ii = 0; ii < WDB_XMLCOUNT; ++ii)
	{
		TWorldData& wd = *m_pWorldData[ii];
		bAnyNeedApply = bAnyNeedApply || !wd.IsStateAfter(Versioned::S_Loaded);
	}

	return bAnyNeedApply;
}

SW_NAMESPACE_END();