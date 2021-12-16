#pragma once
#include "ISWVersionControl.h"

class CNullVersionControl : public sw::ISWVersionControl
{
public:
	CNullVersionControl();

	virtual sw::ISWDataPersistence* SetPersistentStorage(sw::ISWDataPersistence* pNewPersist);
	virtual sw::ELockResult Lock( sw::Versioned& vd );
	virtual bool Unlock( sw::Versioned& vd );
	virtual int64 CreateChangeList(sw::EVersionType vt, const char* szDescription = NULL);
	virtual sw::TCheckOutContext CheckOut(sw::Versioned& vd, sw::EVersionType vt, int64 nChangeList);
	virtual bool Submit(sw::Versioned& vd);
	virtual bool DoCreateData( sw::Versioned& vd, const TArgStack* pMetaData = NULL );
	virtual bool Delete( sw::Versioned& vd );
	virtual bool Commit(int64 nChangelist);
	virtual bool GetRevision(sw::Versioned& vd, sw::EVersionType vt, int64 nRev = -1){return true;}

	virtual bool GetLockedBy( sw::Versioned& vd, int& nLockedBy );
	virtual const char* GetLockedBy( sw::Versioned& vd, char* pOutName, int32 cbSize, const char* pNAValue = NULL );
	virtual bool GetRevIDs( sw::Versioned& vd, int64 (&revs)[sw::VT_COUNT] );
	virtual int GetUserID(){ return 1;}
	sw::ISWDataPersistence* m_pPersist;
};
