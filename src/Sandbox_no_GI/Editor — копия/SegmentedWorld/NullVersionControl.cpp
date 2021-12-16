#include "stdafx.h"
#include "InternalCommon.h"
#include "NullVersionControl.h"
#include "SCDataPersistence.h"
//#include "DBManager.h"
#include "VersionedBase.h"

using namespace sw;

CNullVersionControl::CNullVersionControl()
	:m_pPersist(NULL)
{
}

bool CNullVersionControl::DoCreateData( Versioned& vd, const TArgStack* pMetaData )
{
	//assert(vd.IsMissingInDB());
	
	int64 revIDs[VT_COUNT]={1};
	vd.Reinit(revIDs,0);

	vd.OnCreatedInPersist();
	return true;
}

bool CNullVersionControl::Delete( Versioned& vd )
{
	CString strFilePath = vd.GetFileName(*m_pPersist);
	bool bOK = CFileUtil::DeleteFile(strFilePath);
	assert(bOK);
	vd.OnDeletedInPersist();
	return true;
}

ELockResult CNullVersionControl::Lock( Versioned& vd )
{
	assert( DataInspector::m_nLockedBy(vd) != -1);
	{
		AUTO_LOCK(g_cs);
		if (vd.IsLockedByMe())
			return LockResult_AlreadyHave;

		assert(!vd.IsLocked());
	}

	vd.OnLock();
	return LockResult_Offline;
}

int64 CNullVersionControl::CreateChangeList(EVersionType vt, const char* szDescription)
{
	return 333;
}

bool CNullVersionControl::Unlock( Versioned& vd )
{
	if (!vd.IsLocked())
	{assert(0); return false;}

	vd.OnUnlock();
	return true;
}

bool CNullVersionControl::Commit( int64 nChangelist )
{
	return true;
}

TCheckOutContext CNullVersionControl::CheckOut(Versioned& vd, sw::EVersionType vt, int64 nChangeList)
{
	if (vd.IsCheckedOut())
		return vd.GetCheckOutCtx();

	assert(vd.GetVersion() != HRID_UNKNOWN);
	assert(vd.GetVersion() != HRID_MISSING);
	int64 newRevID = vd.GetVersion(vt) + 1;

	vd.GetCheckOutCtx() = TCheckOutContext(newRevID,nChangeList,vt);
	return vd.GetCheckOutCtx();
}

bool CNullVersionControl::Submit(Versioned& vd)
{
	if (!vd.IsCheckedOut())
		return false;

	TCheckOutContext& ctx = vd.GetCheckOutCtx();

	int64 newHeadRevIDs[VT_COUNT];
	for (int ii=0; ii < VT_COUNT; ++ii)
		ii = ctx.m_nRev;

	vd.SetAllVersions( newHeadRevIDs );

	ctx.Clear();
	return true;
}

bool CNullVersionControl::GetLockedBy( sw::Versioned& vd, int& nLockedBy )
{
	if (true /*vd.m_vdt == VDT_SEGTRUNK*/)
	{
		CString strFName = vd.GetFileName( *m_pPersist );
		ICryPak *pIPak = GetIEditor()->GetSystem()->GetIPak();

		_finddata_t fd;
		intptr_t fhandle = pIPak->FindFirst(strFName, &fd);

		if (fhandle != -1)
		{
			DWORD dwAttr = GetFileAttributes(strFName); //GetIEditor()->GetSourceControl()->GetFileAttributes(pcFN);
			//if (dwAttr & SCC_FILE_ATTRIBUTE_MANAGED) {
			//  if (dwAttr & SCC_FILE_ATTRIBUTE_BYANOTHER)
			//    SetLockStatus(WS_LOCKED_EXTERNAL);
			//  else if (dwAttr & SCC_FILE_ATTRIBUTE_CHECKEDOUT)
			//    SetLockStatus(WS_LOCKED_LOCAL);
			//  else
			//    SetLockStatus(WS_NOT_LOCKED);
			//} else {
			if (dwAttr & FILE_ATTRIBUTE_READONLY/*SCC_FILE_ATTRIBUTE_READONLY*/)
				nLockedBy = GetUserID() + 1; // Locked by others, nonzero and different than m_nDBUserID
			else
				nLockedBy = GetUserID();
			//}
			pIPak->FindClose(fhandle);
			return true;
		}
	}
	else
	{
		nLockedBy = GetUserID();
		return true;
	}

	return false;
}

const char* CNullVersionControl::GetLockedBy( sw::Versioned& vd, char* pOutName, int32 cbSize, const char* pNAValue /*= NULL */ )
{
	if (cbSize <= 1)
		return NULL;
	static const char szName[] = "<OFFLINE USER>";
	strncpy(pOutName, szName, std::min((size_t)cbSize,sizeof(szName)));
	pOutName[cbSize-1] = 0;
	return pOutName;
}

bool CNullVersionControl::GetRevIDs( sw::Versioned& vd, int64 (&revs)[sw::VT_COUNT] )
{
	if (vd.GetVersion() == HRID_UNKNOWN)
	{
		assert(vd.StateEqualTo(Versioned::S_Init));
		
		CString strFName = vd.GetFileName(*m_pPersist);
		ICryPak *pIPak = GetIEditor()->GetSystem()->GetIPak();

		FILE* fhandle = pIPak->FOpen(strFName,"rb");

		int64 nHeadRev;
		if (fhandle > 0)
		{
			size_t fsize = pIPak->FGetSize(fhandle);
			nHeadRev = fsize?2:1;  // 1 created, 2 written

			pIPak->FClose(fhandle);
		}
		else
		{
			nHeadRev = HRID_MISSING;
		}

		for (int ii=0; ii < sw::VT_COUNT; ++ii)
			revs[ii] = nHeadRev;

		return nHeadRev > HRID_MISSING;
	}

	vd.GetAllVersions(revs); // should always return those values themselves, because no other is changing it.
	return true;
}

ISWDataPersistence* CNullVersionControl::SetPersistentStorage( ISWDataPersistence* pNewPersist )
{
	ISWDataPersistence* pOld = m_pPersist;
	m_pPersist = (CSCDataPersistence*)pNewPersist;
	return pOld;
}