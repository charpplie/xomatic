#include "stdafx.h"
#include "InternalCommon.h"
#include "NativeSCMAdapter.h"

#include "SegmentedWorldDoc.h" // for TVersionMgr
#include "ISourceControl.h"
#include "SCDataPersistence.h"

//#include "DBManager.h"
//#include "SegmentedWorldQueries.h"
//using namespace ns_SW_STATEMENTS;


using namespace sw;


CNativeSCMAdapter::CNativeSCMAdapter( TVersionMgr* pVerMgr, ISourceControl* pSCM )
	: m_pVerMgr(pVerMgr)
	, m_pSCM(pSCM)
	, m_bChangeListCreated(false)
	, m_pPersist(NULL)
{

}

bool CNativeSCMAdapter::DoCreateData( Versioned& vd, const TArgStack* pMetaData )
{
	// TODO: currently it's easier to add the file to SCM while saving it(which won't be found, then we create).
	CString strFilePath = vd.GetFileName(*m_pPersist);
	bool bOK = m_pSCM->Add( strFilePath, 0, ADD_AS_BINARY_FILE );
	assert(bOK);
	bOK = m_pSCM->CheckIn(strFilePath);
	assert(bOK);

	AUTO_LOCK(g_cs);
	vd.OnCreatedInPersist();

	return true;
}

bool CNativeSCMAdapter::Delete( sw::Versioned& vd )
{
	CString strFilePath = vd.GetFileName(*m_pPersist);
	bool bOK = m_pSCM->Delete(strFilePath, 0 );
	assert(bOK);

	// if it's not OK, I think we still need to set it as missing.
	vd.OnDeletedInPersist();
	return true;
}

ELockResult CNativeSCMAdapter::Lock( Versioned& vd )
{
	if(CSWManager::Get().IsOfflineMode()) return LockResult_Offline;

	{
		AUTO_LOCK(g_cs);
		if (vd.IsLockedByMe())
			return LockResult_AlreadyHave;
		if (vd.IsLocked())
			return (m_pVerMgr->IsHot() ? LockResult_Hot : LockResult_Failed);
	}

	CString strFilePath = vd.GetFileName(*m_pPersist);
	bool bOK = m_pSCM->Lock(strFilePath);
	assert(bOK);

	//additional code
	vd.OnLock();

	return LockResult_Succeeded;

}


int64 CNativeSCMAdapter::CreateChangeList(EVersionType vt, const char* szDescription)
{
	// TODO: maybe we need a real change list
	m_bChangeListCreated = true;
	if (szDescription)
		m_desc = szDescription;
	return 2;
}

bool CNativeSCMAdapter::Unlock( Versioned& vd )
{
	CString strFilePath = vd.GetFileName(*m_pPersist);
	bool bOK = m_pSCM->Unlock(strFilePath);
	assert(bOK);

	//additional code
	vd.OnUnlock();
	return true;
}

bool CNativeSCMAdapter::Commit( int64 nChangelist )
{
	bool bRet = true;
	int64 nVer = nChangelist;
	return bRet;
}

TCheckOutContext CNativeSCMAdapter::CheckOut(Versioned& vd, sw::EVersionType vt, int64 nChangeList)
{
	if (vd.IsCheckedOut())
		return vd.GetCheckOutCtx();

	CString strFilePath = vd.GetFileName(*m_pPersist);
	uint32 nAttr = m_pSCM->GetFileAttributesAndFileName(strFilePath,0);
	if ( nAttr&SCC_FILE_ATTRIBUTE_LOCKEDBYANOTHER)
	{
		return TCheckOutContext(); // locked by another, failed
	}
	else if ( !(nAttr & SCC_FILE_ATTRIBUTE_CHECKEDOUT) )
	{
		// if not checked out by us and not locked by other
		bool bOK = m_pSCM->CheckOut(strFilePath);
		assert(bOK);
	}

	int64 hrid = -1;
	bool bOK = m_pSCM->GetFileRev(strFilePath,&hrid,NULL);
	assert(bOK && hrid >0);
	hrid++; // revID should be 1-bigger than current file rev, it's for next submit


	vd.GetCheckOutCtx() = TCheckOutContext(hrid,nChangeList,vt);
	return vd.GetCheckOutCtx();
}

bool CNativeSCMAdapter::Submit(Versioned& vd)
{
	if (!vd.IsCheckedOut())
	{
		CRY_ASSERT_MESSAGE(0,"Cannot submit, not checked out yet");
		return false;
	}
	TCheckOutContext& ctx = vd.GetCheckOutCtx();

	{
		const char* szDesc = NULL;
		if (m_bChangeListCreated)
			szDesc = m_desc.c_str();
		CString strFilePath = vd.GetFileName(*m_pPersist);

		int64 nHeadRev;
		bool bOK = m_pSCM->GetFileRev(strFilePath,NULL,&nHeadRev);
		assert(bOK);
		if (ctx.m_nRev > nHeadRev)
		{
			bOK = m_pSCM->CheckIn(strFilePath,szDesc);
			assert(bOK);
		}
		else // if someone has submitted something into history before us, we should discard our change
		{
			assert(0);
			bOK = m_pSCM->UndoCheckOut(strFilePath);
			assert(bOK);
		}
		//}
	}

	//HV: Delete Record
	if(m_pVerMgr->NeedHotHistoryBeDeleted())
	{
		// TODO: delete Hot
	}

	ctx.Clear();

	return true;
}

bool CNativeSCMAdapter::GetLockedBy( sw::Versioned& vd, int& nLockedBy )
{
	char szUser[MAX_PATH];

	if (GetLockedBy(vd, szUser, MAX_PATH))
	{
		nLockedBy = UserName2Id(szUser);
		return true;
	}

	return false;
}

const char* CNativeSCMAdapter::GetLockedBy( sw::Versioned& vd, char* pOutName, int32 cbSize, const char* pNAValue /*= NULL*/ )
{
	CString strFilePath = vd.GetFileName(*m_pPersist);
	int32 nResult = m_pSCM->IsLockedByUs(strFilePath);

	switch(nResult)
	{
	case 0:
		strcpy(pOutName, "");
		break;
	case 1:
		{
			bool bOK = m_pSCM->GetOtherLockOwner(strFilePath,pOutName,cbSize);
			if (!bOK)
			{ assert(0); return pNAValue; }
		}
		break;
	case 2:
		{
			bool bOK = m_pSCM->GetUserName(pOutName,cbSize);
			if (!bOK)
			{ assert(0); return pNAValue; }
		}
		break;
	default:
		return pNAValue;
	}

	return pOutName;
}

bool CNativeSCMAdapter::GetRevIDs( sw::Versioned& vd, int64 (& revs)[VT_COUNT] )
{
	if (!vd.CheckVcap(VerCap_AllBranches))
		return false;

	int64 nHeadRev;
	CString strFilePath = vd.GetFileName(*m_pPersist);
	const char* sz = strFilePath;
	bool bOK = m_pSCM->GetFileRev(strFilePath.GetString(),NULL,&nHeadRev);
	//assert(bOK);

	for (size_t ii =0; ii < VT_COUNT; ++ii)
		revs[ii] = nHeadRev;

	return bOK;
}

int CNativeSCMAdapter::UserName2Id( const char* sz )
{
	if (!sz || sz[0] == '\0')
		return 0;

	assert( strlen(sz)>=3);
	return  *(int*)sz & 0x7FffFFff; // TODO: make it a real unique id
}

int CNativeSCMAdapter::GetUserID()
{
	char szUserName[MAX_PATH];
	m_pSCM->GetUserName(szUserName,MAX_PATH);

	return UserName2Id(szUserName);
}

bool CNativeSCMAdapter::GetRevision( Versioned& vd, sw::EVersionType vt, int64 nRev /*= -1*/ )
{
	assert(nRev>0 || nRev==-1);
	if (nRev < 0)
		return m_pSCM->GetLatestVersion(vd.GetFileName(*m_pPersist));
	else
		return m_pSCM->GetRevision(vd.GetFileName(*m_pPersist),nRev);
}

TPersistenceWeakPtr CNativeSCMAdapter::SetPersistentStorage( TPersistenceWeakPtr pNewPersist )
{
	TPersistenceWeakPtr pOld = m_pPersist;
	m_pPersist = pNewPersist;
	return pOld;
}
