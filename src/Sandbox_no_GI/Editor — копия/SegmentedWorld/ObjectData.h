#ifndef __ObjectData_h__
#define __ObjectData_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "SegmentedWorldManager.h"
#include "SWCommon.h"
#include "VersionedBase.h"
#include "ObjectBinding.h"

SW_NAMESPACE_BEGIN();
// stores per-segment data, acts as a cache

enum ObjectDataFlags
{	
	EXTOBJFLAG_VT_OFFICIAL	= BIT(0),	// Is Object External in Official Version
	EXTOBJFLAG_VT_CURRENT	= BIT(1),	// Is Object External in Current Version
	EXTOBJFLAG_VT_HOT		= BIT(2),	// Is Object External in Hot Version 
	EXTOBJFLAG_NOW			= BIT(3),	// Is Object External Now
	EXTOBJFLAG_DISAPPROVED	= BIT(4),	// The object should be un-externalized when necessary
										// the process is designed to be delayed because we don't want go back&forth
};

//--------------------- TObjectData declaration
// used for objects which can be linked or placed on more than one segment
struct TObjectData : public sw::Versioned , public utils::Bindable
{
	typedef std::deque<GUID> TGuidList;
private:
	CMemoryBlock	m_data;							// data loaded from DB
	//------------------------------
	//         Members
	//------------------------------
	TWorldCoords	m_wc;								// world segment coordinates
	TRectI			m_bboxSeg;					// bounding box in seg coordinates
	TLocalCoords	m_lc;								// local segment coordinates when data was saved
	GUID					m_LayerGUID;				// object layer GUID
	int						m_nLayerID;				// object layerID
	string				m_strLayerName;		// name of Layer
	GUID					m_guid;							// GUID
	TGuidList				m_arrLinks;
	TGuidList				m_arrLinkedFrom;
	bool					m_bNeedInWorld;
	int						m_nFlags;


public:
	//------------------------------
	//  constructor and destructor
	//------------------------------
	TObjectData();
	explicit TObjectData(REFGUID guidObj);
	// unload
	void Unload();
	void UnloadAndUnlink();

protected:
	~TObjectData();
	// responses to events
	virtual bool IsMissingInPersist() const;
	virtual void OnPersistentDataChanged(sw::EVersionType vt);
	virtual void Reinit( int64 nHRIDs[sw::VT_COUNT], int nLockedBy );

public:
	void SetAsMissingInPersist();

public:
	CBaseObject* GetBaseObject()const;
	// take dimensions, position and etc from object
	void SetParams(CBaseObject *pObject);
	// save object to memory block and save to DB if needed
	bool SaveObject(sw::ESaveFlag sflag, const TLocalCoords &lc/*, CBaseObject *pObject*/, int64 nVersion, sw::EVersionType vt);

	bool PrepareData( sw::ESaveFlag sflag, sw::EVersionType vt, const TLocalCoords & lc );

	// called when the block has been changed locally (editor / engine wrote to m_data)
	void AppliedLocally(const TLocalCoords &lc);
	bool DoSaveToDB(sw::ESaveFlag sflag, sw::EVersionType vt, int64 nVersion );
	bool DoIntegrate( sw::EVersionType vt, int64 nVersion );

	// save object data in ObjectsHistory table, m_nLoadedHRID contains saved record id on success
	bool SaveData(int64 nVersion, sw::EVersionType vt);

	TArgStack MakePrimaryKey() const;
	virtual CString GetFileName(CSCDataPersistence& persist) const;
	bool DoSaveData( int64 nVersion, sw::EVersionType vt);
	bool SavePartitioningChanges(sw::EVersionType vt);
	// create object data in DB if doesn't exist already
	bool CreateData();
	// get GUIDLo, GUIDHi and GUID (string)
	void GetGuid(int64 &nGUIDLo, int64 &nGUIDHi) const;
	string GetGuidStr() const;
	// true if objectData needs to be (re)applied (applied and loaded hrids differ)
	bool NeedsToBeApplied();

	CBaseObject* LoadBaseObject(CObjectArchive &oa, bool bDeleteExisting);
	// returns uncompressed data (loaded if necessary) into mem
	bool LoadObjectData(CMemoryBlock &mem, TLocalCoords *plc = 0);
	// load object data if necessary (if loaded version differs), returns false on fail
	bool Load( int64 nHRID);
	bool LoadByVT( sw::EVersionType vt) {return Load(Versioned::GetVersion(vt));}
	//// mark as modified
	//void SetModified();
	//// revert ObjectData to specified version type, returns true if any HRID changes were made
	//bool Revert(sw::EVersionType vt, bool bRevertVersionInfo);
	//// returns true if block is saved
	//bool IsSaved(sw::EVersionType vt);
	// called when object is no longer presented in the engine/editor
	void NoLongerApplied();
	// 
	void InvalidateLinkedFromObjects();
	//
	void Init();

	//void SetAppliedHRID(int64 nAppliedHRID);
	//bool InsertLoadedVersionIntoHistory(sw::EVersionType vt, int64 nVersion);
	void SerializeWithLookupTable(CXmlArchive& ar);
	void SetNeededInWorld(bool bNewVal)			{ m_bNeedInWorld = bNewVal; }
	bool IsNeededInWorld()const					{ return m_bNeedInWorld; }
	TRectI const& GetSegBBox()const				{ return m_bboxSeg; }
	TWorldCoords const& GetSegCoords()const		{ return m_wc; }
	string const& GetLayerName()const			{ return m_strLayerName; }
	REFGUID GetLayerGUID()const					{ return m_LayerGUID; }
	REFGUID GetGUID()const						{ return m_guid; }
	int32 GetLayerID()const						{ return m_nLayerID; }
	void LinkTo(TObjectData& target);
	void UnlinkFrom(TObjectData& target);
	bool CheckDisapprove()const;

	//////////////////////////////////////////////////////////////////////////
	// Flags.
	//////////////////////////////////////////////////////////////////////////
	void SetFlags( int flags );;
	void ClearFlags( int flags ) { AUTO_LOCK(g_cs); m_nFlags &= ~flags; };
	bool CheckFlags( int flags ) const { AUTO_LOCK(g_cs); return (m_nFlags&flags) != 0; };
	void SyncDBFlags();
	//bool HasVersionConflict();

protected:
	// ==== Interface Bindable ====
	//void BeforeUnbind();		// Don't need to auto-delete itself when this is no longer referenced, it goes to cache instead


	int GetVersionsAsFlag()const;

	void SetData( CMemoryBlock &mem );
	void GetData( CMemoryBlock &mem );

	friend sw::CObjectDataAggr;
};

SW_NAMESPACE_END();

#endif // __ObjectData_h__
