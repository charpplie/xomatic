#pragma once
#include "ISWVersionControl.h"


class CNativeSCMAdapter : public sw::ISWVersionControl
{
	typedef CSegmentedWorldManager TVersionMgr;
public:
	explicit CNativeSCMAdapter(TVersionMgr* pVerMgr, ISourceControl* pSCM);
	virtual sw::TPersistenceWeakPtr SetPersistentStorage(sw::TPersistenceWeakPtr pNewPersist);
	virtual sw::ELockResult Lock( sw::Versioned& vd );
	virtual bool Unlock( sw::Versioned& vd );
	virtual int64 CreateChangeList(sw::EVersionType vt, const char* szDescription = NULL);
	virtual sw::TCheckOutContext CheckOut(sw::Versioned& vd, sw::EVersionType vt, int64 nChangeList);
	virtual bool Submit(sw::Versioned& vd);
	virtual bool DoCreateData( sw::Versioned& vd);
	virtual bool Add( sw::Versioned& vd);
	virtual bool Delete( sw::Versioned& vd);
	virtual bool Commit(int64 nChangelist);
	virtual bool GetRevision(sw::Versioned& vd, sw::EVersionType vt, int64 nRev = -1);
	bool GetLockedBy( sw::Versioned& vd, int& nLockedBy );
	const char* GetLockedBy( sw::Versioned& vd, char* pOutName, int32 cbSize, const char* pNAValue = NULL );
	bool GetRevIDs( sw::Versioned& vd, int64 (& revs)[sw::VT_COUNT] );
	virtual int GetUserID();
protected:
	bool CommitByVDT( sw::EVersionedDataType vdt, int64 nVersion);
	int UserName2Id(const char* sz);
protected:
	TVersionMgr* m_pVerMgr;
	ISourceControl* m_pSCM;
	sw::TPersistenceWeakPtr m_pPersist;

	bool m_bChangeListCreated;
	string m_desc;
};
