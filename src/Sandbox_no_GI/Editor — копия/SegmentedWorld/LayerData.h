#ifndef __LayerData_h__
#define __LayerData_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "SWCommon.h"
#include "VersionedBase.h"



// TLayerData
struct TLayerData : public sw::Versioned
{
	typedef sw::Versioned TSuper;
private:
	CMemoryBlock m_data; // data loaded from DB

public:
	TLocalCoords m_lc; // local segment coordinates when data was saved
	TWorldCoords m_wc;
	string m_strName; // name of Layer
	int m_nLayerID; // ID of Layer
	GUID m_LayerGUID;	// guid of Layer
	GUID m_parentLayerGUID; // guid of parent layer
	bool m_bIsAParent;		// is referenced by other layers

protected:
	virtual void OnLock();
	TArgStack MakePrimaryKey() const;
	virtual CString GetFileName(CSCDataPersistence& persist) const;
public:
	bool IsMissingInPersist() const;

public:
	TLayerData();
	TLayerData( REFGUID guid, string const& strLayerName, TWorldCoords const& wc, TLocalCoords const& lc );
	~TLayerData();


	// load layer data if necessary (if loaded version differs), returns false on fail
	bool Load(const TWorldCoords &wc, int64 nHRID);
	bool LoadByVT( TWorldCoords wc, sw::EVersionType vt ) { return Load(wc, Versioned::GetVersion(vt)); }
	// save layer data in LayersHistory table, m_nLoadedHRID contains saved record id on success
	bool SaveData(int64 nVersion, sw::EVersionType vt);

	//bool DoSaveData( int64 nVersion, sw::EVersionType vt);

	// revert the layer data to specified version type, returns true if any HRID changes were made
	//bool Revert(const TWorldCoords &wc, sw::EVersionType vt, bool bRevertVersionInfo);
	//// what version type should be used by the editor for this layer
	//sw::EVersionType GetNeededVersionType();
	// save layer and returns either HRID is modified or not
	bool SaveLayer(sw::ESaveFlag sflag, const TLocalCoords &lc, CBaseObjectsArray &arrObjects, int64 nVersion, sw::EVersionType vt);

	bool PrepareData( sw::ESaveFlag sflag, const TLocalCoords & lc, CBaseObjectsArray &arrObjects );
	bool DoSaveToDB(sw::ESaveFlag sflag, sw::EVersionType vt, int64 nVersion);
	bool DoIntegrate( sw::EVersionType vt, int64 nVersion );

	bool Submit();

	// save layer's HRIDs
	//bool SaveHRIDs(sw::EVersionType vt);
	//// true if needs to be (re)applied (loaded and applied HRIDs differ)
	//bool NeedsToBeApplied();

	void NoLongerApplied();


	bool IsDataEmpty()const {return m_data.IsEmpty() || m_data.GetSize() == 0;}
	size_t GetMemoryUsage()const {return m_data.GetSize();}
	void GetData(CMemoryBlock& mem)const
	{
#ifndef SW_NO_DATACOMPRESS
		m_data.Uncompress(mem);
#else
		mem.Free();
		mem = m_data;
#endif
	}
	void SetData(CMemoryBlock& mem)
	{
#ifndef SW_NO_DATACOMPRESS
		mem.Compress(m_data);
#else
		m_data.Free();
		m_data = mem;
#endif
	}

	bool HasParent() const { return m_parentLayerGUID != GUID_NULL; }
	bool IsAParent() const { return m_bIsAParent; }

protected:
	// create layer data in DB if doesn't exist already
	bool CreateData();
	// unload data
	void Unload();
	//
	//void InvalidateExternalObjects();
	//
	//bool InsertIntoVersionHistory(int64 nHRID, int vt, int64 nVersion);
	//bool InsertLoadedVersionIntoHistory(sw::EVersionType vt, int64 nVersion);
	// called when the layer data has been changed locally (editor / engine wrote to m_data)
	void AppliedLocally(const TLocalCoords &lc);
	friend struct DataInspector;
};

#endif // __LayerData_h__
