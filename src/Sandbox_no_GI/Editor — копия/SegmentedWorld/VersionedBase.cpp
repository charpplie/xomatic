#include "stdafx.h"
#include "InternalCommon.h"
#include "VersionedBase.h"

#include "SegmentedWorldDoc.h"
//#include "DBManager.h"
#include "ISWDataPersistence.h"
#include "ISWVersionControl.h"

#include "ObjectData.h" // TODO: added for UpdateStatus, should be removed later


using namespace sw;	// without this line VA will confuse
SW_NAMESPACE_BEGIN();

//-------------------------------------------------------------------------------------
void Versioned::OnAppliedLocally()
{
	AUTO_LOCK(g_cs);
	//SetLoadedVersion(HRID_CACHE);
	//SetAppliedHRID(HRID_CACHE);

	GoToState(S_ModifSerialized);
}
//---------------------------------------------------------------------------------------
bool Versioned::IsSaved(EVersionType vt)const
{
	AUTO_LOCK(g_cs);
	//if (m_nLoadedHRID == HRID_MISSING)
	if ( m_bOptional && IsNotLoaded())
		return true;

	for (int nVT = vt; nVT < VT_COUNT; ++nVT)
	{
		//int64 nHRID = m_nHRIDs[nVT];
		//if (nHRID == m_nLoadedHRID)
		if (!LoadedVersionIsVT((EVersionType)nVT))
			return false;
	}

	return true;
}
//-------------------------------------------------------------------------------------
bool Versioned::NeedsReload()const
{
	CSegmentedWorldDoc* pSW = GetInternalSWDoc();
	EVersionType vt = GetNeededVersionType();
	if (!pSW->IsHot() && IsModified())
		return false;
	AUTO_LOCK(g_cs);
	//int64 nHRID = m_nHRIDs[vt];
	//if (m_nLoadedHRID == nHRID)
	if (LoadedVersionIsVT(vt))
		return false;
	assert( !IsLockedByMe()/*m_nLockedBy != g_db->GetUserID()*/ || pSW->IsHot());
	return true;
}

bool Versioned::NeedsToBeApplied()const
{
	CSWDoc *psw = GetInternalSWDoc();

	if ((m_vdt == VDT_LAYER || m_vdt == VDT_EXTOBJECT)
		&& !psw->IsLayerDataNeeded())
		return false;

	AUTO_LOCK(g_cs);

	//if ( AppliedVersionIs(HRID_EDITOR) )
	if (IsModified())
		return false;

	//if ( AppliedVersionIs(HRID_UNKNOWN) )
	if (m_state < S_Applied)
		return true;

	if (!IsLoadedDataACache())
	{
		EVersionType vt = ( (m_vdt==VDT_EXTOBJECT)? psw->GetVersionType() :  GetNeededVersionType() );
		//int nHRID = m_nHRIDs[vt];
		//if (nHRID < 0) 
		CRY_ASSERT_MESSAGE((GetVersion(vt) >= 0), "Invalid version requested");

		//if (m_nLoadedHRID == nHRID)
		if (LoadedVersionIsVT(vt))
		{
			//return (m_nAppliedHRID != m_nLoadedHRID);
			return !IsLoadedVersionApplied(); // if loaded version not applied, apply it -RL
		}
	}
	//if (m_nAppliedHRID == HRID_MISSING)
	//if (AppliedVersionIs(HRID_MISSING))
	if (m_state < S_Applied)
		return true;
	//if (m_nAppliedHRID == m_nLoadedHRID)
	if (IsLoadedVersionApplied())
		return false;
	return true;
}
//void Versioned::SetAppliedHRID(int64 nAppliedHRID)
//{
//	if (nAppliedHRID > 0)
//		m_nHRIDBeforeModify = nAppliedHRID;
//	m_nAppliedHRID = nAppliedHRID;
//}



void Versioned::SetVersion( EVersionType vt, int64 nHRID )
{
	m_nHRIDs[vt] = nHRID;
}

void Versioned::Reinit( int64 ov, int64 cv, int64 hv, int nLockedBy )
{
	assert( ov == cv && ov == hv);
	int64 nHRIDs[VT_COUNT];
	//nHRIDs[VT_OFFICIAL] = ov;
	nHRIDs[VT_CURRENT] = cv;
	//nHRIDs[VT_HOT] = hv;

	Versioned::Reinit(nHRIDs,nLockedBy);
}

void Versioned::Reinit( int64 nHRIDs[VT_COUNT], int nLockedBy )
{
	if ( m_vcap & VerCap_AllBranches )
	{
		for (size_t ii=0;ii<VT_COUNT;++ii)
		{
			assert( nHRIDs[ii] != HRID_UNKNOWN);
			m_nHRIDs[ii] = nHRIDs[ii];
		}
	}

	m_bMissing = false;
	m_nLockedBy = nLockedBy;
}

void Versioned::SetAsMissingInPersist()
{
	AUTO_LOCK(g_cs);
	//assert(m_vcap & VerCap_Dynamic);

	m_bMissing = true;

	for (int vt = 0; vt < VT_COUNT; ++vt) 
	{
		m_nHRIDs[vt] = HRID_MISSING;
	}
	//m_nLockedBy = -1;
}

void Versioned::ApplyLoadedVersion()
{
	//SetAppliedHRID(m_nLoadedHRID);
	GoToState(S_Applied);
}

bool Versioned::IsLockedByMe()const
{
	AUTO_LOCK(g_cs);
	CSWDoc *psw = GetInternalSWDoc();
	return (m_nLockedBy == psw->GetVersionControl()->GetUserID());
}

bool Versioned::NeedUpateStatus( EVersionType vt /*= VT_UNSPECIFIED*/ )const
{
	// This function is originally extracted from TSegmentData::ReloadLayers()+10  & TSegmentData::NeedsToBeAppliedLayers()+10

	if (IsModified()) 
		return false;

	EVersionType vtFinal = (vt != VT_UNSPECIFIED)? vt : GetNeededVersionType();

	if (m_nHRIDs[vtFinal] != HRID_UNKNOWN) 
		return false;

	return true;
}

EVersionType Versioned::GetNeededVersionType()const
{
	CSWDoc *psw = GetInternalSWDoc();
	if (true/*psw->GetVersionType() != VT_OFFICIAL*/)
		return psw->GetVersionType();
	AUTO_LOCK(g_cs);
	//if (m_nLockedBy != g_db->GetUserID())
	if (!IsLockedByMe())
		return psw->GetVersionType();
	return VT_CURRENT;
}

//int64 Versioned::GetAppliedVersion()const
//{
//	return m_nAppliedHRID;
//}

//bool Versioned::IsLoaded()
//{
//	return m_nLoadedHRID != HRID_MISSING && m_nLoadedHRID != HRID_UNKNOWN;
//}

//bool Versioned::IsApplied()const
//{
//	return m_nAppliedHRID != HRID_MISSING && m_nAppliedHRID != HRID_UNKNOWN;
//}

bool Versioned::IsVersionConflicting( EVersionType vt )const
{
	// TODO: implement it -RL
	return false;
	//return m_nHRIDBeforeModify != m_nHRIDs[vt];
}

//void Versioned::UpdateVersionToLoaded( int nVT )
//{
//	m_nHRIDs[nVT] = m_nLoadedHRID;
//}
bool Versioned::UpdateVersionsToLoaded( EVersionType vt)
{
	return UpdateVersions(vt, m_nLoadedHRID );
}
bool Versioned::UpdateVersions( EVersionType vt, int64 nHRID )
{
	AUTO_LOCK(g_cs);
	bool bModified = false;
	for (int nVT = vt; nVT < VT_COUNT; ++nVT)
	{
		int64 oldvalue = m_nHRIDs[nVT];
		m_nHRIDs[nVT] = nHRID;

		if (oldvalue != nHRID)
			bModified = true;
	}

	return bModified;
}

bool Versioned::IsLocked()const
{
	return (m_nLockedBy > 0);
}

bool Versioned::IsMissingInPersist() const
{
	return m_bMissing;
}

bool Versioned::SyncMissingStatus()
{
	m_bMissing = true;

	bool bAnyHeadRev = false;
	for (int vt = 0; vt < VT_COUNT; ++vt)
		if (m_nHRIDs[vt] > 0)
			bAnyHeadRev = true; // data is not missing if any version exists

	m_bMissing = !bAnyHeadRev && (m_nLockedBy == -1);
	
	return m_bMissing;
}


void Versioned::OnLock() 
{
	AUTO_LOCK(g_cs);
	m_nLockedBy = GetInternalSWDoc()->GetVersionControl()->GetUserID();
}

void Versioned::OnUnlock()
{
	AUTO_LOCK(g_cs);
	m_nLockedBy = 0;
}

//void Versioned::OnVersionTypeLoaded( EVersionType vt )
//{
//	//AUTO_LOCK(g_cs);
//	//for (int nVT = vt; nVT < VT_COUNT; ++nVT)
//	//	m_nHRIDs[nVT] = m_nLoadedHRID;
//	UpdateVersionsToLoaded(vt);
//}

int64 Versioned::GetVersion() const
{
	CSWDoc *psw = GetInternalSWDoc();
	EVersionType vt = psw->GetVersionType();

	return Versioned::GetVersion(vt);
}

bool Versioned::RevertVersionTo( EVersionType vt )
{
	int64 nHRID;
	{AUTO_LOCK(g_cs);
	nHRID = GetVersion(vt);
	}
	// update less official versions(type values are greater) to a specified version
	// e.g.: throw saved version and roll-back to committed version, throw hot version and go back to saved version
	return UpdateVersions((EVersionType)(vt+1),nHRID);
}

void Versioned::ModifiedLocally()
{
	AUTO_LOCK(g_cs);
	//SetAppliedHRID(HRID_EDITOR);
	GoToState(S_Modified);
	GetInternalSWDoc()->OnSWModified();
}

void Versioned::OnCreatedInPersist()
{
	AUTO_LOCK(g_cs);
	m_bMissing = false;
}

ELockResult Versioned::Lock()
{
	assert(m_vcap & VerCap_Lockable);
	if (CSWManager::Get().IsOfflineMode()) return LockResult_Offline;// because the CreateData may fail
	CSegmentedWorldDoc *pSW = GetInternalSWDoc();
	UpdateStatus();
	bool bMissing;
	{
		AUTO_LOCK(g_cs);
		bMissing = m_bMissing;
	}
	if (bMissing)
	{
		if (!CreateData())
			return LockResult_Failed;
		//AUTO_LOCK(g_cs);
		//m_bMissing = false;
	}

//#ifdef USE_DB_VC
//	if (m_vdt == VDT_WORLD)
//	{
//		// TODO: why world data need to do this check?? -RL
//		AUTO_LOCK(g_cs);
//		//if (m_nLoadedHRID != m_nHRIDs[pSW->GetVersionType()])
//		if (!Versioned::LoadedVersionIsVT(pSW->GetVersionType()))
//			return LockResult_Failed;
//	}
//#endif

	//assert(g_db->GetCachedConn());
	//if (g_db->GetCachedConn())
	{
		//IDBPreparedStatement *pStatement;
		//char chStatement[64];

		//pStatement = g_db->GetCachedConn()->GetPreparedStatement(WORLD_DATA_LOCK);
		//sprintf(chStatement, "WORLD_DATA_LOCK");

		//if (!pStatement)
		//{	
		//	char chLog[256]; sprintf(chLog, "Cannot create %s query.\n", chStatement);
		//	SWLog(chLog);
		//	return 0;
		//}

		//int nType;
		////int nDBUserID;
		//{
		//	AUTO_LOCK(g_cs);
		//	nType = m_eType;
		//	//nDBUserID = g_db->GetUserID();
		//}
		//if (!DBAPIHelper::Exec(pStatement, nDBUserID, nType))
		//{
		//	char chLog[256]; sprintf(chLog, "%s query failed.\n", chStatement);
		//	SWLog(chLog);
		//	return 0;
		//}

		//if (!pStatement->GetUpdateCount())
		//	return 0;

		////additional code
		//AUTO_LOCK(g_cs);
		//m_nLockedBy = g_db->GetUserID();
		//return 2;
		return pSW->GetVersionControl()->Lock(*this);
	}

	//!!! not implemented for file version / source control
	//m_nLockedBy = g_db->GetUserID();
	OnLock();
	return LockResult_Succeeded;
}

bool Versioned::Unlock()
{
	assert(m_vcap & VerCap_Lockable);
	//AUTO_LOCK(g_cs);
	CSegmentedWorldDoc *pSW = GetInternalSWDoc();

	if (m_vdt == VDT_WORLD)
		UpdateStatus();

	{
		AUTO_LOCK(g_cs);
		if (!IsLockedByMe())
			return false;
	}

	//assert(g_db->GetCachedConn());
	//if (g_db->GetCachedConn())
	{
		//IDBPreparedStatement *pStatement;
		//char chStatement[64];
		//pStatement = g_db->GetCachedConn()->GetPreparedStatement(WORLD_DATA_UNLOCK);
		//sprintf(chStatement, "WORLD_DATA_UNLOCK");

		//if (!pStatement)
		//{	
		//	char chLog[256]; sprintf(chLog, "Cannot create %s query.\n", chStatement);
		//	SWLog(chLog);
		//	return false;
		//}

		////int nType;
		////int nDBUserID;
		////nType = m_eType;
		////nDBUserID = g_db->GetUserID();
		//if (!DBAPIHelper::Exec(pStatement, nType, nDBUserID))
		//{
		//	char chLog[256]; sprintf(chLog, "%s query failed.\n", chStatement);
		//	SWLog(chLog);
		//	return false;
		//}

		////additional code
		//if (!pStatement->GetUpdateCount())
		//	return false;

		//AUTO_LOCK(g_cs);
		//m_nLockedBy = 0;
		//return true;
		return pSW->GetVersionControl()->Unlock(*this);
	}
	//!!! not implemented for file version / source control
	//m_nLockedBy = 0;
	Versioned::OnUnlock();
	return true;
}

void Versioned::OnSavedDataToDB( int64 hrid )
{
	AUTO_LOCK(g_cs);
	//if ( Versioned::IsLoadedVersionApplied() )
	//	SetAppliedHRID(hrid);
	GoToState(S_Applied);
	SetLoadedVersion(hrid);
}
bool Versioned::InsertLoadedVersionIntoHistory( EVersionType vt, int64 nVersion )
{
	// WORLD_DATA_INSERT_TO_VERSION_HISTORY; SD_DATA_BLOCK_INSERT_TO_VERSION_HISTORY;
	// SD_OBJECT_LAYERS_INSERT_TO_VERSION_HISTORY;EXTERNAL_OBJECTS_INSERT_TO_VERSION_HISTORY;

	int64 nHRID;
	{
		AUTO_LOCK(g_cs);
		nHRID = m_nLoadedHRID;
	}
	CSWDoc *psw = GetInternalSWDoc();
	//assert(g_db->GetCachedConn());
	//if (g_db->GetCachedConn())
	{
		//TArgStack arrArgs;
		//arrArgs.push_back(nHRID);
		//arrArgs.push_back(vt);
		//arrArgs.push_back(nVersion);
		////arrArgs.insert(arrArgs.end(),arrAdditional.begin(),arrAdditional.end());
		//return g_db->ExecPreparedStatement(m_vdt,StmtOp_AddToVH,arrArgs);
	}
	return false;
}

bool Versioned::UpdateRevHead( EVersionType vt )
{
	//if (CSWManager::Get().IsOfflineMode()) return true;
	//CSWDoc *psw = GetInternalSWDoc();

	//assert(g_db->GetCachedConn());
	//if (g_db->GetCachedConn())
	//{
	//	TArgStack args;
	//	for (int curVT = vt; curVT< VT_COUNT;++curVT )
	//	{
	//		assert(m_nLoadedHRID == Versioned::GetVersion((EVersionType)curVT));
	//		args.push_back(Versioned::GetVersion((EVersionType)curVT));
	//	}
	//	TArgStack arrPK = MakePrimaryKey();
	//	args.insert(args.end(),arrPK.begin(),arrPK.end());
	//	if (m_vdt ==VDT_WORLD) // TODO: remove this special case when we can
	//		args.push_back(g_db->GetUserID());

	//	uint64 nUpdateCount = 0;
	//	if (!g_db->ExecPreparedStatement(m_vdt,StmtOp_SetHead,args,&nUpdateCount,vt))
	//		return false;

	//	//if (!pStatement->GetUpdateCount())
	//	if (0 == nUpdateCount)
	//		return false;

	//	return true;
	//}
	//return false;

	return true;
}

int64 Versioned::GetHotVersionToDelete()const
{
	//AUTO_LOCK(g_cs);
	//bool bNeedDelete = m_nHRIDs[VT_HOT] != m_nHRIDs[VT_CURRENT] && m_nHRIDs[VT_HOT] != m_nHRIDs[VT_OFFICIAL];
	//if (bNeedDelete)
	//	return m_nHRIDs[VT_HOT];

	assert(HRID_MISSING == 0);
	return HRID_MISSING;
}

void Versioned::SetMissingInDB( bool bMissing )
{
	m_bMissing = bMissing;
	if (m_bMissing)
		m_nLockedBy = -1;
}

void Versioned::SetLockedBy( int nLockedBy )
{
	m_nLockedBy = nLockedBy;
	if (m_nLockedBy > 0)
		m_bMissing = false;
}

void Versioned::GetAllVersions( int64 (&nHRIDs)[VT_COUNT] ) const
{
	AUTO_LOCK(g_cs);
	for (int i = 0; i < (int)VT_COUNT; i++)
	{
		nHRIDs[i] = m_nHRIDs[i];
	}
}

void Versioned::SetAllVersions( int64 (&nHRIDs)[VT_COUNT] )
{
	for (int vt = 0; vt < VT_COUNT; ++vt)
	{	
		m_nHRIDs[vt] = nHRIDs[vt];
	}
}

void Versioned::InitVersion( bool bOptional /*= false*/ )
{
	Reset();
	m_bOptional = bOptional;

	m_state = S_Init;
	// TODO: merge these cases
	switch (m_vdt)
	{
	case VDT_WORLD:
		{
			//m_nAppliedHRID = HRID_UNKNOWN;
			//m_nHRIDBeforeModify = HRID_UNKNOWN;
			for (int vt = 0; vt < VT_COUNT; ++vt)
				m_nHRIDs[vt] = HRID_UNKNOWN;
		}
		break;
	case VDT_SEGTRUNK:
		{
			//m_nAppliedHRID = HRID_UNKNOWN;
			//m_nHRIDBeforeModify = HRID_UNKNOWN;
			for (int vt = 0; vt < VT_COUNT; ++vt)
				m_nHRIDs[vt] = HRID_UNKNOWN;
		}
		break;
	case VDT_LAYER:
	case VDT_EXTOBJECT:
		{
			m_nLockedBy = 0;
			//m_nAppliedHRID = HRID_MISSING;
			//m_nHRIDBeforeModify = HRID_MISSING;
			for (int vt = 0; vt < VT_COUNT; ++vt)
				m_nHRIDs[vt] = HRID_UNKNOWN;
		}
		break;
	case VDT_SEGTRUNK_COMP_HEAD:
		{
			m_nLockedBy = 0;
		}
		break;
	default:
		assert(0);
	}
}

void Versioned::Reset()
{
	m_bMissing = true;
	m_nLockedBy = -1; // means not updated
	m_nLoadedHRID = HRID_MISSING;
	m_state = S_Init;
}

bool Versioned::UpdateStatus()
{
//	assert(g_db->IsConnReady());
//
//	DBPrepStmtResultSet pResultSet = g_db->ExecPreparedStatement(m_vdt,StmtOp_UpdateStatus0,MakePrimaryKey());
//	assert(pResultSet);
//
//	if (!pResultSet || !pResultSet->Next())
//	{
//		//if (!(m_vcap & VerCap_Dynamic) )
//		//	assert(0);
//		goto lb_UpdateStatus_Failed;
//	}
//
//	{
//		AUTO_LOCK(g_cs);
//		bool bSucceeded = true;
//		int64 ov,cv,hv; ov = cv = hv = HRID_MISSING;
//		int nLockedBy = -1;
//		int nCol = 0;
//		if (this->m_vcap&VerCap_Lockable)
//		{
//			bSucceeded = bSucceeded && pResultSet->GetInt32ByIndex(nCol++,nLockedBy);
//		}
//		if (this->m_vcap & VerCap_AllVersions)
//		{
//			bSucceeded = bSucceeded && pResultSet->GetInt64ByIndex(nCol++,ov);
//			bSucceeded = bSucceeded && pResultSet->GetInt64ByIndex(nCol++,cv);
//			bSucceeded = bSucceeded && pResultSet->GetInt64ByIndex(nCol++,hv);
//		}
//
//		if (!bSucceeded)
//			goto lb_UpdateStatus_Failed;
//
//		this->Reinit(ov,cv,hv,nLockedBy);
//	}
//	return true;
//
//lb_UpdateStatus_Failed:
//	{
//		char chLog[256]; sprintf(chLog, "updateStatus [datatype:%d] query returned 0 rows.\n", m_vdt);
//		SWLog(chLog);
//
//		// TODO: remove this ugly special case
//		if (m_vdt == VDT_OBJECT)
//		{
//			AUTO_LOCK(g_cs);
//			// object don't set the m_bMissing flag, don't know why
//			int bOldVal = m_bMissing;
//			assert(bOldVal);
//			SetAsMissing();
//			m_bMissing = bOldVal;
//			((TObjectData*)this)->SyncDBFlags();
//		}
//		else
//			this->SetAsMissing();
//
//		return false;
//	}

	ISWVersionControl* pVC = GetInternalSWDoc()->GetVersionControl();
	int64 revs[VT_COUNT] = {};
	int nLockedBy = -1;
	pVC->GetRevIDs(*this,revs);
	if (!pVC->GetLockedBy(*this,nLockedBy))
	{
		assert(VT_COUNT == 1);

		// TODO: remove this ugly special case
		if (m_vdt == VDT_EXTOBJECT)
		{
			AUTO_LOCK(g_cs);
			// object don't set the m_bMissing flag, don't know why
			int bOldVal = m_bMissing;
			assert(bOldVal);
			SetAsMissingInPersist();
			m_bMissing = bOldVal;
			((TObjectData*)this)->SyncDBFlags();
		}
		else
			SetAsMissingInPersist();

		return false;
	}

	Reinit(revs,nLockedBy);
	return true;
}

bool Versioned::DoSave( sw::ESaveFlag sflag, sw::EVersionType vt, int64 nVersion )
{
	//assert(AppliedVersionIs(HRID_CACHE));
	assert(StateEqualTo(S_ModifSerialized));
	assert(!(sflag&SF_AsIntegrate));//< only pure intergration is allowed, NO modify-and-integrate
	if (!SaveData(/*wc,*/ nVersion, vt))
	{
		assert(0);
		//bRes = false;
		return false;
	}
	//bVersionHistorySaved = true;

	bool bModified = UpdateVersionsToLoaded(vt);
	assert(bModified);
	bool bOK = UpdateRevHead(vt);
	assert(bOK);
	return true;
}

bool Versioned::DoIntegrateOnly( EVersionType vt, int64 nVersion )
{
	bool bModified = UpdateVersionsToLoaded(vt);

	if (bModified)
	{
		//assert(vt==VT_OFFICIAL/* && !bVersionHistorySaved*/);
		bool bOK = InsertLoadedVersionIntoHistory(vt,nVersion);
		assert(bOK);
		bOK = UpdateRevHead(vt);
		assert(bOK);
		return true;
	}	
	return false;
}

bool Versioned::CanGoToState( EState from, EState s ) const
{
	bool bForceSaving = (!GetInternalSWDoc())? false : GetInternalSWDoc()->IsForceSaving();

	// TODO:  Probably should have separate FSMs for exported game data & minimap data
	//		AND once it becomes too complicated, we should use 2D lookup table instead
	//		Exported data should not be managed with Versioned source data anyway
	switch (from)
	{
	case S_Init:
		return 0
			|| (s == S_Loaded)
			|| (s == S_Reloading)
			|| (bForceSaving && s == S_ModifSerialized) // in the case we generate the data directly (e.g by converting).
			|| (m_vdt == VDT_SEGTRUNK && m_subtype == SDB_ModSectors && s == S_Applied) // TODO: remove the special case -RL
			|| (m_vdt == VDT_WORLD && s == S_Modified); // for WorldData, they are generated to source objects first (then we dump them to files).
			;
	case S_Loaded:
		return 0
			|| (s == S_Loaded)		// no harm if load again,right?
			|| (s == S_Applied)
			|| (s == S_Reloading)
			|| ( s == S_Unapplied) // Loaded is somewhat "Unapplied", so it's ok to say it explicitly.
			|| ( bForceSaving && s == S_ModifSerialized); // During ForceSaving proc, dump data to buffer anyway.
			;
	case S_Applied:
		return 0
			|| (s == S_Modified)
			|| (s == S_Reloading)
			|| (s == S_Unapplied)
			|| ( bForceSaving && s == S_ModifSerialized); // During ForceSaving proc, dump data to buffer anyway.
			;
	case S_Modified:
		return 0
			|| (s == S_Modified)
			|| (s == S_ModifSerialized)
			|| (s == S_Unapplied)
			|| (s == S_Applied); // discard changes
	case S_ModifSerialized:
		return 0
			|| (s == S_Applied) // submitted
			|| (s == S_Unapplied)
			;
	case S_Reloading:
		return 0
			|| (s == S_Loaded)
			|| (s == S_Init) // failed reloading should be reset to Init
			|| (s == S_Reloading); // it's ok to reload again
			;
	case S_Unapplied:
		return (s == S_Applied) || (s == S_Loaded) || (s == S_Reloading);
	default:
		assert(0&&"Corrupted state data");
	}
	return false;
}

bool Versioned::IsStateAfter( EState testee, EState refPoint ) const
{
	switch (refPoint)
	{
	case S_Init:
		return (testee == S_Loaded) || (testee == S_Applied) || (testee == S_Modified) || (testee == S_ModifSerialized);
	case S_Loaded:
		return (testee == S_Applied) || (testee == S_Modified) || (testee == S_ModifSerialized);
	case S_Applied:
		return (testee == S_Modified) || (testee == S_ModifSerialized);
	case S_Modified:
		return (testee == S_ModifSerialized);
	case S_ModifSerialized:
		return (testee == S_Applied); // submitted
	case S_Reloading:
		return (testee == S_Loaded);
	case S_Unapplied:
		return (testee == S_Applied) || (testee == S_Loaded) || (testee == S_Reloading);
	default:
		assert(0&&"Corrupted state data");
	}
	return false;
}

void Versioned::GoToState(EState s)
{
	if (!CanGoToState(m_state,s))
	{
		assert(0&&"Trying to goto a state illegally");
		//return;
	}

	EState old = m_state;
	m_state = s;

	// PostProcess for instant states
	switch (s)
	{
	case S_Reloading:
		//GoToState(S_Loaded);
		break;
	}
}


bool Versioned::DiscardModification( bool *pOutShouldReload )
{
	if (pOutShouldReload) *pOutShouldReload = false;

	if (IsStateAtLeast(S_Modified))
	{
		EState oldstate = m_state;
		m_state = S_Applied;

		if (oldstate == S_ModifSerialized && pOutShouldReload)
			*pOutShouldReload = true;

		return true;
	}

	return false;
}

void Versioned::OnDeletedInPersist()
{
	SetAsMissingInPersist();
}

bool VersionedComposite::UpdateStatus()
{
	bool bSucceeded = Versioned::UpdateStatus();

	// we may still need to call update on child -- to set them to missing. (this is a bad design methodology)
	int bChildSucceeded = 1;
	for (TChildIterator iter = m_ppChildBegin; iter != m_ppChildEnd; ++iter)
		bChildSucceeded &= (*iter)->UpdateStatus()?1:0;

	return bSucceeded && bChildSucceeded;
}

void VersionedComposite::OnCreatedInPersist()
{
	if (m_vcap&VerCap_Dynamic)
		Versioned::OnCreatedInPersist();
	else if (!m_bOverrideChildOp)
		for ( TChildIterator iter = m_ppChildBegin; iter != m_ppChildEnd; ++iter )
		{
			TChildPtr ptr = *iter;
			if (ptr)
				ptr->OnCreatedInPersist();
		}
}

CString VersionedComposite::GetFileName(CSCDataPersistence& persist)
{
	assert(0);
	CString temp("NULL");
	return temp;
}

SW_NAMESPACE_END();

