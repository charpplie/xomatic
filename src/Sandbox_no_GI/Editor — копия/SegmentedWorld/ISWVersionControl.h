#pragma once

#include "SWFwd.h"
#include "VersionCommon.h"
#include "AnyType.h"



SW_NAMESPACE_BEGIN();

class ISWVersionControl : public _i_reference_target_t
{
public:
	enum{MAX_USERNAME_CBSIZE = 128};
public:
	virtual TPersistenceWeakPtr SetPersistentStorage(TPersistenceWeakPtr pNewPersist) =0;
	virtual sw::ELockResult Lock( sw::Versioned& vd ) =0;
	virtual bool Unlock( sw::Versioned& vd ) =0;
	virtual int64 CreateChangeList(sw::EVersionType vt, const char* szDescription = NULL) =0;
	virtual TCheckOutContext CheckOut(sw::Versioned& vd, sw::EVersionType vt, int64 nChangeList) =0;
	virtual bool Submit(sw::Versioned& vd) =0;
	virtual bool DoCreateData( sw::Versioned& vd, const TArgStack* pMetaData = NULL ) =0;
	virtual bool Delete( sw::Versioned& vd ) =0;
	virtual bool Commit(int64 nChangelist) =0;
	virtual bool GetRevision(sw::Versioned& vd, sw::EVersionType vt, int64 nRev = -1) =0;

	virtual bool GetLockedBy( Versioned& vd, int& nLockedBy ) =0;
	virtual const char* GetLockedBy( sw::Versioned& vd, char* pOutName, int32 cbSize, const char* pNAValue = NULL ) =0;
	virtual bool GetRevIDs( Versioned& vd, int64 (&revs)[sw::VT_COUNT] ) =0;
	virtual int GetUserID() =0;

};

SW_NAMESPACE_END();
