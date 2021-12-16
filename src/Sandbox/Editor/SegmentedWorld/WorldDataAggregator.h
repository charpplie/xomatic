#ifndef __WorldDataAggregator_h__
#define __WorldDataAggregator_h__

#include "SWCommon.h"
#include "VersionedBase.h"

SW_NAMESPACE_BEGIN();

class CWorldDataAggre
{
	typedef CSegmentedWorldDoc TGlobal;
	typedef CSegmentedWorldDoc TWorld;
	typedef CSegmentedWorldDoc TVersionMgr;

public:
	CWorldDataAggre(TGlobal* pGlobal, TWorld* pWorld, TVersionMgr* pVersionMgr);
	~CWorldDataAggre();
	
	bool WorldData_LoadToArchiveArray(TDocMultiArchive &arrXmlAr);

	// status
	bool WorldData_UpdateStatus(int eWDType = -1);	// -1 means All
	const char *WorldData_GetStatusText(char *pBuf, int eWDType = -1);	// -1 means All
	bool WorldData_IsModified();
	bool GetGDStatus(int nWDBType, int &nWDBState, int &nLockedBy);

	// reload
	bool WorldData_ReloadTerrainLayers(); //---Terrain Layers
	bool WorldData_ReloadVegetationMap(); //---VegetationMap
	bool WorlData_ReloadIfNeeded();

	// save
	bool WorldData_PrepareData( uint32 flagNeedSave, sw::ESaveFlag sflag, sw::EVersionType vt, sw::ELockResult* arrSaves );
	bool WorldData_Save(sw::ESaveFlag sflag, sw::EVersionType vt, int64 nVersion = -1, sw::EWDBType nWDBType = sw::WDB_COUNT);
	bool WorldData_Save( sw::ESaveFlag sflag, sw::EVersionType vt, int64 nVersion, uint32 nWDBFlags );

	bool WorldDataCC(sw::EWDBType nWDBType, int nWDBCC);
	bool TryLockModified(std::vector<TWorldData*> &wdJustLocked);
	bool SetModified(int nType);
	bool HasConflict();
	bool Lock();
	void SetStatus(sw::Versioned::EState state);
	bool NeedApply();

private:
	TGlobal* m_pGlobal; //< keep this pointer to fetch global info faster
	TWorld* m_pWorld;
	TVersionMgr* m_pVersionMgr;

	TWorldData *m_pWorldData[sw::WDB_COUNT];
};

SW_NAMESPACE_END();

#endif // __WorldDataAggregator_h__