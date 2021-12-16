#ifndef __SegmentData_h__
#define __SegmentData_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "SegmentedWorldManager.h"
#include "SWCommon.h"
#include "VersionedBase.h"




SW_NAMESPACE_BEGIN();

struct SNamedMemoryBlock
{
	CMemoryBlock mem;
	string name;
};

struct TSegDataBlock : public sw::Versioned
{
	typedef TSegmentData TParent;
private:
	std::vector<SNamedMemoryBlock> m_data; // data loaded from DB

	TParent* m_pParent;
public:
	sw::ESDBType m_eType; // data block
	TLocalCoords m_lc; // local segment coordinates when data was saved

public:
	TSegDataBlock(TParent* pParent, sw::ESDBType blockType, TWorldCoords const& wc);
	TParent* GetParent()const {return m_pParent;}
	bool IsIndependent()const {return !m_pParent;}

	// load data block if necessary (if loaded version differs), returns false on fail
	bool Load( CSCDataPersistence& persist, int64 nHRID);
	bool LoadByVT( CSCDataPersistence& persist, sw::EVersionType vt) { return Load(persist,Versioned::GetVersion(vt)); }

	TWorldCoords const& GetWC()const;
	virtual CString GetFileName( CSCDataPersistence& persist ) const;
	// save data block in SegmentsHistory table, m_nLoadedHRID contains saved record id on success
	bool SaveData( int64 nVersion, sw::EVersionType vt);

	// unload data
	void Unload();

	//// revert the block to specified version type, returns true if any HRID changes were made
	//bool Revert(sw::EVersionType vt, bool bHRIDs);

	// called when the block has been changed locally (editor / engine wrote to m_data)
	void AppliedLocally(const TLocalCoords &lc);

	bool IsDataEmpty(uint32 index = 0)const {assert(index < m_data.size()); return m_data[index].mem.IsEmpty() || (m_data[index].mem.GetSize() == 0);}
	size_t GetMemoryUsage(uint32 index = 0)const {assert(index < m_data.size()); return m_data[index].mem.GetSize();}
	size_t GetMemoryBlockCount() const { return m_data.size(); }

	void ReserveBlocks(uint32 size) { m_data.resize(size); }
	void SetData(CImageEx& img, uint32 index = 0);
	void GetData(CMemoryBlock& mem, uint32 index = 0)const;
	CMemoryBlock &GetMemoryBlock(uint32 index = 0){assert(index < m_data.size()); return m_data[index].mem;}
	SNamedMemoryBlock &GetNamedMemoryBlock(uint32 index = 0) {assert(index < m_data.size()); return m_data[index];}
	void ClearData()
	{
		for(int i = 0; i < m_data.size(); ++i)
			m_data[i].mem.Free();
	}
	//bool SyncMissingStatus();

	virtual bool CreateData();
	virtual bool IsMissingInPersist() const{return false;}
	bool DoSaveToDB(ESaveFlag sflag, EVersionType vt, int64 nVersion);

	//bool DoSave( ESaveFlag sflag, int64 nVersion, EVersionType vt );

	bool DoIntegrate( EVersionType vt, int64 nVersion);
	virtual void NoLongerApplied(){GoToState(S_Unapplied);}
};

struct TSegEngDataBlock : public TSegDataBlock
{
public:
	TSegEngDataBlock(TParent* pParent, sw::ESDBType blockType, TWorldCoords const& wc);
	virtual bool CanGoToState(EState from, EState s) const;
	virtual bool IsStateAfter(EState testee, EState refPoint) const;
	virtual void OnSavedDataToDB( int64 id );
	virtual void NoLongerApplied();
};
struct TSegMinimapDataBlock : public TSegDataBlock
{
public:
	TSegMinimapDataBlock(TParent* pParent, sw::ESDBType blockType, TWorldCoords const& wc);
	virtual bool CanGoToState(EState from, EState s) const;
	virtual bool IsStateAfter(EState testee, EState refPoint) const;
	virtual void OnSavedDataToDB( int64 id );
};

struct TSegmentData : public sw::VersionedComposite
{
	typedef TSegmentData TSelf;
	typedef sw::VersionedComposite TSuper;
	typedef std::map<GUID, sw::TLayerData*, guid_less_predicate> TLayerMap;

private:
	int m_nSID;        // engine segment ID
public:
	TSegDataBlock *m_Data[sw::SDB_COUNT];
	TWorldCoords m_wc; // segment world coordinates
	TSegmentData *m_pPrev; // used for creating list to manage the cache
	TSegmentData *m_pNext; // used for creating list to manage the cache
	int m_nMemoryUsage; // size of SegmentData
	TLayerMap m_layersMap;
	bool m_bNeedApplyLayers; //used only in ::Update(bool bFromThread) for performance optimization is set from thread and unset from main thread

public:
	TSegmentData();
	TSegmentData(TSelf const& rhs );
	TSegmentData& operator=(TSelf const& rhs);
	void Clone(TSelf const& rhs);
	~TSegmentData();

public:
	bool IsInUse() const { return m_nSID >= 0; }
	int GetEngineSID() const { assert(IsInUse()); return m_nSID; }
	void OnCreatedInEngine(int nSID) { m_nSID = nSID; }
	void OnRemoveFromEngine() { m_nSID = -1; }

	// finds or creates data block
	TSegDataBlock *GetBlock(sw::ESDBType eBlockType, bool bCreate);

	// returns uncompressed data (loaded if necessary) into mem
	bool GetData(sw::ESDBType eBlockType, CMemoryBlock &mem, TLocalCoords *plc = 0, uint32 index = 0);

	virtual bool SaveData( int64 nVersion, EVersionType vt);

	// what version type should be used by the editor for this segment
	sw::EVersionType GetNeededVersionType(); 

	// create segment data in DB if doesn't exist already
	bool CreateData();
	// save all modified data blocks
	bool DoSaveToDB(ESaveFlag sflag, EVersionType vt, int64 nVersion);
	bool DoIntegrate( EVersionType vt, int64 nVersion);
	void PrepareData( ESaveFlag sflag, sw::EVersionType vt, const TLocalCoords &lc );

	// save necessary HRIDs
	bool UpdateRevHeads(sw::EVersionType vt, int pnBlockTypes[] /*= 0*/);

	// unload all data blocks
	void Unload();

	// true if any of the blocks needs to be reloaded
	bool NeedsReload();

	// reload necessary data blocks
	void Reload();

	// true if any of the blocks is marked as modified
	bool IsModified(int nBlockType = -1);

	// true if locally loaded versions match the given version type
	bool IsSaved(sw::EVersionType vt);

	virtual CString GetFileName(CSCDataPersistence& persist) const;

	// revert blocks to specified version type, returns true if any HRID changes were made
	bool Revert(sw::EVersionType vt);

	// true if the specified block needs to be (re)applied (loaded and applied HRIDs differ)
	bool NeedsToBeApplied(sw::ESDBType eBlockType);

	// true if any of the blocks needs to be (re)applied (loaded and applied HRIDs differ)
	bool NeedsToBeApplied();
	bool NeedToBeFastApplied();
	// called when data block is updated to the engine/editor
	void Applied(sw::ESDBType eBlockType);

	// called when specific block is no longer presented in the engine/editor
	void NoLongerApplied(sw::ESDBType eBlockType);

	// called when segment is no longer presented in the engine/editor
	void NoLongerApplied();

	// called when segment is modified or created
	void RefreshMemoryUsage();

	// return memory usage of segment
	int GetMemoryUsage()const { return m_nMemoryUsage; }

	// Remove from list
	void RemoveFromList();

	bool Submit();


	/////////////////////////
	// Layers Data
	void UnloadLayers();
	// finds or creates layer data
	TLayerData *GetLayer(REFGUID guid, const char *strLayerName = NULL);
	TLayerData *CreateLayer(REFGUID guid, const char *szName, bool* pbBrandNew = NULL, int nLayerId = -1);
	// returns uncompressed data (loaded if necessary) into mem
	bool LoadLayerData(TLayerData *pl, CMemoryBlock &mem, TLocalCoords *plc = 0);
	// update m_bMissing, m_nLocked by and m_nHRIDs
	void UpdateLayersStatus();

	void SetAllLayersAsMissing();

	// returns either Layers are modified or not
	bool IsLayersModified();
	// returns either Layers are saved or not
	bool IsLayersSaved(sw::EVersionType vt);
	// clear all unused layers
	void ClearUnusedLayers();

	void RebuildLayerRelations();

	// save all modified layers
	bool SaveLayers(ESaveFlag sflag, const TLocalCoords &lc, CBaseObjectsArray &arrObjects, int64 nVersion, EVersionType vt);
	bool IntegrateLayers( const TLocalCoords &lc, CBaseObjectsArray &arrObjects, int64 nVersion, EVersionType vt);
	// load layers
	bool LoadLayers(const TLocalCoords &lc, CBaseObjectsArray &arrObjects, CObjectArchive &oa);
	// revert layers to specified version type, returns true if any HRID changes were made
	bool RevertLayers(const TWorldCoords &wc, sw::EVersionType vt, bool bUpdateStatus);
	// lock LayersData, returns true on success
	bool LockLayers();
	// unlock the LayersData
	void UnlockLayers();
	// returns true if has layer locked by user
	bool HasLockedLayer();
	// true if any of the layers needs to be reloaded
	bool NeedsReloadLayers();
	// reload necessary layers
	void ReloadLayers();
	// true if any of the layers needs to be (re)applied (loaded and applied HRIDs differ)
	bool NeedsToBeAppliedLayers();
	// save engine data
	void SaveDataForEngine(ESaveFlag sflag, const TLocalCoords &lc, int64 nVersion, EVersionType vt);
	// called when segment is no longer presented in the engine/editor
	void NoLongerAppliedLayers();

	// Mini map
	bool NeedRegenerateMiniMap();

	void Init(const TWorldCoords& wc);
	bool GetFileBlockData(CMemoryBlock &mem, const TWorldCoords &wc, ESDBType eFileBlockType);

protected:
	struct SavingFilterFunctor
	{
		typedef SavingFilterFunctor TSelf;
		AABB bounds;

		SavingFilterFunctor(AABB const& bb)
			:bounds(bb)
		{}
		static bool CALLBACK Filter( CBaseObject const& obj, void* pContext);

		BaseObjectFilterFunctor GetAsBaseFilter()
		{
			return BaseObjectFilterFunctor( &TSelf::Filter, this);
		}
	};
};

SW_NAMESPACE_END();

#endif // __SegmentData_h__
