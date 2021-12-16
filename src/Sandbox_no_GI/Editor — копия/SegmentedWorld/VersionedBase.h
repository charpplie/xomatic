#pragma once

//#include "SWCommon.h"
#include "VersionCommon.h"
#include "AnyType.h"





SW_NAMESPACE_BEGIN();

	//extern CryCriticalSection g_cs;


enum 
{
	VerCap_Lockable			=1<<0,
	VerCap_Dynamic			=1<<1,	//< it means the data is creatable & deletable (or can be missing)

	VerCap_Commitable		=1<<2,	//< the data can be published as VT_OFFICIAL
	VerCap_Savable			=1<<3,	//< it means the data can be saved privately, as VT_CURRENT
	VerCap_OfflineModif		=1<<4,	//< it means the data can be modified in offline mode, as VT_HOT
	VerCap_AllBranches		=VerCap_Commitable | VerCap_Savable | VerCap_OfflineModif, // TODO: currently only AllVersions are used -RL
	VerCap_All				=0xFFffFFff,
};
typedef uint32 EVersionedCapability;

// this class is used to manage version numbers in memory
struct Versioned
{
public:
	enum EState{ S_Init, S_Loaded, S_Applied, S_Modified, S_ModifSerialized, S_Reloading, S_Unapplied };
public:
	EVersionedDataType	m_vdt;
	int32				m_subtype;
protected:
	EVersionedCapability m_vcap;
	EState m_state;
	bool m_bOptional;

	int64 m_nLoadedHRID; // history record id of the loaded data (in m_data)
	TCheckOutContext m_checkOutCtx;

	//--- SourceControl status cache ---
	int64 m_nHRIDs[VT_COUNT]; // versions stored in the DB
	int m_nLockedBy; // id of the user who has locked the layer in segment, 0 - not locked
	int m_bMissing;    // whether data is missing in DB


public:
	explicit Versioned( EVersionedDataType vdt, EVersionedCapability vcap = VerCap_All, int32 subtype = -1 )
		:m_vdt(vdt),m_vcap(vcap),m_state(S_Init), m_subtype(subtype)
	{
		Reset();
	}
	void GoToState(EState s);
	void GoToStateAtLeast(EState s) { if (!IsStateAtLeast(s)) GoToState(s); }
	bool StateEqualTo(EState s) const { return m_state == s; }
	bool IsStateAfter( EState s ) const { return IsStateAfter(m_state, s); }
	bool IsStateBefore( EState s ) const { return IsStateAfter(s,m_state); }

	bool IsStateAtLeast(EState s) const {return StateEqualTo(s) || IsStateAfter(s);}
	bool DiscardModification( bool *pOutShouldReload = NULL );
protected:
	virtual bool CanGoToState(EState from, EState s) const;
	virtual bool IsStateAfter(EState from, EState s) const;

	void InitVersion( bool bOptional = false);
	void Reset();
public:
	// TODO: move to CDBVersionControl
	bool InsertLoadedVersionIntoHistory( sw::EVersionType vt, int64 nVersion);

public:
	bool IsCheckedOut()const {return (bool)m_checkOutCtx == true;}
	TCheckOutContext& GetCheckOutCtx() {return m_checkOutCtx;}
	TCheckOutContext const& GetCheckOutCtx()const {return m_checkOutCtx;}

	//-- general procedures for all data types
	// try to lock this instance, result 0: failed, 1 - was already locked, 2 - locked
	ELockResult Lock();
	// unlock this instance, returns false if it wasn't locked
	bool Unlock();
	bool UpdateRevHead(sw::EVersionType vt);
	virtual bool UpdateStatus();
	virtual void SetAsMissingInPersist();
	bool DoSave( sw::ESaveFlag sflag, sw::EVersionType vt, int64 nVersion );
	bool DoIntegrateOnly( EVersionType vt, int64 nVersion);

	//-- template methods
	virtual void Unload() =0;
	virtual TArgStack MakePrimaryKey() const =0;
	// methods for local saves
	virtual CString GetFileName( CSCDataPersistence& persist ) const = 0;
	virtual bool CreateData() =0;
	virtual void Reinit( int64 nHRIDs[VT_COUNT], int nLockedBy );
	virtual bool SaveData( int64 nVersion, EVersionType vt) =0;

	//-- event callbacks
	virtual void OnLock();
	virtual void OnUnlock();
	virtual void OnCreatedInPersist();
	virtual void OnDeletedInPersist();
	// since db will produce an auto-incremented hrid during the save, we need to update our values to that.
	virtual void OnSavedDataToDB( int64 hrid );
	virtual void OnPersistentDataChanged(sw::EVersionType vt){}
	void OnVersionLoaded( int64 nHRID ) {  GoToState(S_Loaded); SetLoadedVersion(nHRID); }
	//void OnVersionTypeLoaded( sw::EVersionType vt );
	void OnAppliedLocally();

	//-- overridable conditions
	virtual bool IsMissingInPersist() const;

public:
	void Reinit( int64 ov, int64 cv, int64 hv, int nLockedBy );
	bool IsOptional()const {return m_bOptional;}
	bool CheckVcap(EVersionedCapability vcap ) const {return (m_vcap&vcap) == vcap; }

	// true if needs to be (re)applied (loaded and applied HRIDs differ)
	bool NeedsToBeApplied()const;
	bool NeedUpateStatus(EVersionType vt = VT_UNSPECIFIED)const;
	bool NeedsReload()const;
	// what version type should be used by the editor for this layer
	EVersionType GetNeededVersionType()const;
	int64 GetHotVersionToDelete()const;
	// returns true if block is modified
	bool IsModified() const											{ AUTO_LOCK(g_cs); return m_state == S_ModifSerialized || m_state == S_Modified; }
	bool LoadedVersionIs( int64 nHRID )const						{ AUTO_LOCK(g_cs); return m_nLoadedHRID == nHRID; }
	bool LoadedVersionIsVT(sw::EVersionType vt)const				{ AUTO_LOCK(g_cs); return m_nLoadedHRID == m_nHRIDs[vt];}
	bool LoadedVersionIsNewerThanVT(sw::EVersionType vt)const		{ AUTO_LOCK(g_cs); return m_nLoadedHRID > m_nHRIDs[vt];}
	//bool AppliedVersionIs( int64 nHRID )const						{ AUTO_LOCK(g_cs); return m_nAppliedHRID == nHRID; }
	//bool AppliedVersionIsOtherThan( int64 nHRID )const				{ AUTO_LOCK(g_cs); return m_nAppliedHRID != nHRID && m_nAppliedHRID != HRID_UNKNOWN;}
	//bool AppliedVersionIsOtherThanVT( sw::EVersionType vt )const	{ AUTO_LOCK(g_cs); return m_nAppliedHRID != GetVersion(vt) && m_nAppliedHRID != HRID_UNKNOWN;}
	bool AppliedIsNewerThan(Versioned const& rhs,int vt)const		{ AUTO_LOCK(g_cs); return IsStateAtLeast(S_Applied) && m_nLoadedHRID > rhs.m_nHRIDs[vt]; }
	bool IsLoadedDataACache()const									{ return StateEqualTo(S_ModifSerialized);/*return LoadedVersionIs(HRID_CACHE);*/}
	bool IsLoadedVersionApplied()const								{ AUTO_LOCK(g_cs); return m_state == S_Applied || m_state == S_Modified || m_state == S_ModifSerialized; }
	bool CheckLoadedVersion()const									{ AUTO_LOCK(g_cs); return m_nLoadedHRID > 0; }
	//bool CheckAppliedVersion()const									{ AUTO_LOCK(g_cs); return m_nAppliedHRID > 0; }
	bool CheckVersion( sw::EVersionType vt, int64 nHRID )const		{ AUTO_LOCK(g_cs); return m_nHRIDs[vt] == nHRID; }
	bool CheckVersion(sw::EVersionType vt)const						{ AUTO_LOCK(g_cs); return m_nHRIDs[vt] > 0; }
	bool IsSaved(EVersionType vt)const; 
	//bool IsLoaded();
	bool IsNotLoaded()const											{ AUTO_LOCK(g_cs); return (m_nLoadedHRID == HRID_MISSING); }
	bool IsApplied()const;
	//bool IsDeleted()const											{ AUTO_LOCK(g_cs);return (m_nLoadedHRID == HRID_MISSING && m_nAppliedHRID != m_nLoadedHRID);}
	bool IsVersionConflicting(EVersionType vt)const;
	// return true, if any version updated
	bool IsLockedByMe()const;
	bool IsLocked()const;

	int64 GetLoadedVersion()const						{ return m_nLoadedHRID; }
	//int64 GetAppliedVersion()const;
	int64 GetVersion(sw::EVersionType vt)const			{ return m_nHRIDs[vt]; }
	int64 GetVersion()const;
	void GetAllVersions( int64 (&nHRIDs)[VT_COUNT] )const;

	//void SetAppliedHRID(int64 nAppliedHRID); // TODO: rename to SetAppliedVersion()
	void SetVersion( sw::EVersionType vt, int64 nHRID );
	void SetAllVersions( int64 (&nHRIDs)[VT_COUNT] );
	void ApplyLoadedVersion();
	bool UpdateVersionsToLoaded( sw::EVersionType vt = sw::VT_CURRENT);
	// return true, if there is anything that be reverted
	bool RevertVersionTo(sw::EVersionType vt);
	void SetMissingInDB(bool bMissing);
	void SetLockedBy( int nLockedBy );
	// mark as modified
	void ModifiedLocally();
	bool SyncMissingStatus();
	bool UpdateVersions( sw::EVersionType vt, int64 nHRID);

protected:
	void SetLoadedVersion( int64 nHRID )				{ m_nLoadedHRID = nHRID; }
	friend struct DataInspector;
};


class VersionedComposite : public Versioned
{
	typedef VersionedComposite TSelf;
public:
	typedef Versioned* TChildPtr;
	typedef TChildPtr* TChildIterator;
private:
	TChildIterator m_ppChildBegin;
	TChildIterator m_ppChildEnd;
	bool m_bOverrideChildOp;
public:

	explicit VersionedComposite(EVersionedDataType vdt,EVersionedCapability capmask,Versioned** ppChildBegin=NULL,TChildIterator ppChildEnd=NULL)
		:Versioned(vdt,capmask)
		,m_bOverrideChildOp(true)
	{
		SetChildIter(ppChildBegin,ppChildEnd);
	}
	void Clone(TSelf const& rhs)
	{

	}
	void InitVersion(TChildIterator ppChildBegin,TChildIterator ppChildEnd,bool bOptional=false)
	{
		Versioned::InitVersion(bOptional);
		SetChildIter(ppChildBegin,ppChildEnd);
	}
	void SetChildIter(TChildIterator ppChildBegin,TChildIterator ppChildEnd)
	{
		m_ppChildBegin = ppChildBegin;
		m_ppChildEnd = ppChildEnd;
	}
	virtual void OnCreatedInPersist();

	virtual bool UpdateStatus();

	virtual CString GetFileName(CSCDataPersistence& persist);
};




SW_NAMESPACE_END();
