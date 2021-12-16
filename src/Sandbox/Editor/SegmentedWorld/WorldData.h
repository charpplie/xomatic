#ifndef __WorldData_h__
#define __WorldData_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "SWCommon.h"
#include "VersionedBase.h"

struct TWorldData : public sw::Versioned
{
private:
	CMemoryBlock m_data; // data loaded from DB

public:
	EWDBType m_eType; // data block type
	CXmlArchive* m_xmlAr;

public:
	TWorldData();
	~TWorldData();

protected:
	// template methods
	virtual CString GetFileName(CSCDataPersistence& persist) const;

	bool WorldData_XmlToMemBlock( CXmlArchive &xmlAr, CMemoryBlock& mem );
	bool WorldData_MemBlockToXml( CMemoryBlock& mem, CXmlArchive &xmlAr );

public:
	// load if necessary (if loaded version differs), returns false on fail
	bool Load(int64 nHRID);
	bool LoadByVT(sw::EVersionType vt){ return Load(Versioned::GetVersion(vt));}

	// save if modified
	bool SaveData(int64 nVersion, sw::EVersionType vt);
	//bool DoSaveToDB( sw::ESaveFlag sflag, sw::EVersionType vt, int64 nVersion );
	bool DoSave( sw::ESaveFlag sflag, sw::EVersionType vt, int64 nVersion );

	bool PrepareData();

	bool DoIntegrate( sw::EVersionType vt, int64 nVersion);
	//static bool DBVC_DoSaveData( Versioned& vd, enum SW_STATEMENTS queryStmtIDs[], std::vector<TAny> &argsAdditionalSaveHRID, std::vector<TAny> argsSave, CMemoryBlock &datablock, int64 nVersion, sw::EVersionType vt, int64 nPrevHRID, std::vector<TAny> &argsPrimaryKey);

	// create world data in DB if doesn't exist already
	bool CreateData();

	// true if locally loaded version matches the given version type
	//bool IsSaved(sw::EVersionType vt);

	// revert to specified version type, returns true if any changes were made
	//bool Revert(sw::EVersionType vt);

	void Unload();

	void SetAppliedHRID(int64 nAppliedHRID);

	sw::EVersionType GetWorldDataNeededVersionType();

	//bool DoSaveData( sw::Versioned& vd, int64 nVersion, sw::EVersionType vt, int64 nPrevHRID, std::vector<TAny> const& arrAdditionalSave, std::vector<TAny> const& arrAdditionalSaveHRID, std::vector<TAny> const& arrPrimaryKey );

	void GetData(CMemoryBlock& mem)const;
	void SetData(CMemoryBlock& mem);


	friend class CUpgradePipe_Version0ToVersion1;
};

#endif // __WorldData_h__
