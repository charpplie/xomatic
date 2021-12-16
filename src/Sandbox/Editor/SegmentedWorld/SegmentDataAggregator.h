#ifndef __SegmentDataAggregator_h__
#define __SegmentDataAggregator_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "VersionCommon.h"


SW_NAMESPACE_BEGIN();

struct TSegLayerKey
{
	TWorldCoords	wc;
	GUID			layerGUID;

	TSegLayerKey()
		:wc(0,0),layerGUID(GUID_NULL)
	{}

	TSegLayerKey(TSegLayerKey const& rhs)
		:wc(rhs.wc), layerGUID(rhs.layerGUID)
	{}
	TSegLayerKey(TWorldCoords const& wc, REFGUID layerGuidIn)
		:wc(wc), layerGUID(layerGuidIn)
	{}

	bool operator < (TSegLayerKey const& rhs) const
	{
		if (this->wc == rhs.wc)
			return guid_less_predicate()(this->layerGUID, rhs.layerGUID);
		return MakeSegmentID(this->wc) < MakeSegmentID(rhs.wc);
	}
private:
	bool operator == (TSegLayerKey const& rhs)const;
	bool operator == (TSegLayerKey const& rhs);
};

struct TSegLayerInfo	: TSegLayerKey
{
	string layerName;
	int layerID;
};

class CSegmentDataAggr
{
	typedef CSegmentedWorldDoc TGlobal;
	typedef CSegmentedWorldDoc TWorld;
	typedef CSegmentedWorldDoc TVersionMgr;

	typedef std::map<SegmentID, TSegmentData> TSegmentDataMap;
	typedef std::map<TSegLayerKey,TSegLayerInfo> TSegLayerMap;
public:
	CSegmentDataAggr(TGlobal* pGlobal, TWorld* pWorld, TVersionMgr* pVersionMgr );
	~CSegmentDataAggr();

	bool LoadSegment(const TLocalCoords &lc, CBaseObjectsArray &arrObjects, CObjectArchive &oa, DWORD dwFlags);
	bool SaveSegment(sw::ESaveFlag sflag, const TLocalCoords &lc, CBaseObjectsArray &arrObjects, int64 nVersion, sw::EVersionType vt = sw::VT_CURRENT);
	bool IntegrateSegment( const TLocalCoords &lc, CBaseObjectsArray &arrObjects, int64 nVersion, sw::EVersionType vt /*= sw::VT_CURRENT*/ );
	void ReloadVegetationInstances(const TLocalCoords &lc);
	void ReloadSegmentLayerID(const TLocalCoords &lc);
	TSegDataBlock* LoadIndependentDataBlock( CSCDataPersistence& persist,  int x, int y, ESDBType eType );
	TLayerData* GetSegmentLayer(const TWorldCoords &wc, REFGUID guid, const char *szName);
	TLayerData* CreateSegmentLayer(const TWorldCoords &wc, REFGUID guid, const char *szName, bool* pbBrandNew = NULL, int nLayerId = -1);
	//TSegmentData *GetSegmentData(const TWorldCoords &wc);
	//TSegmentData *CreateSegmentData(const TWorldCoords &wc);
	TSegmentData *GetSegmentData(const TWorldCoords &wc, bool bCreate, bool* pbBrandNew = NULL);
	TSegmentData *GetSegmentData( SegmentID segID, bool bCreate, bool* pbBrandNew = NULL) { return GetSegmentData( FromSegmentID(segID), bCreate, pbBrandNew); }
	void FetchSegDataBlocks( std::vector<TSegDataBlock*>& blocks, sw::ESDBType eBlockType, const Recti & worldSegRect );

	void UnloadSegmentData();

	void RefreshCasheSize();
	void CleanCache(int iMaxMem = MAX_CACHE_SIZE);

	void UpdateLayersStatus();

	int GetSegmentCount() { return (int)m_mapSegmentData.size(); }
	int GetCacheSize() { RefreshCasheSize(); return m_nCacheSize; }
	void FetchAll(std::vector<TSegmentData*> &arrSegmentData);

	bool SaveToDB(sw::ESaveFlag sflag, sw::EVersionType vt, int64 nVersion);
	bool IntegrateToDB(sw::EVersionType vt, int64 nVersion);
	bool Revert(sw::EVersionType vt, bool bConflictsOnly, TWorldCoordsArray &arrSegmentsToLoad);

	void SetModified(const TWorldCoords &wc, sw::ESDBType eBlockType);
	void SetModified(const TWorldCoords &wcMin, const TWorldCoords &wcMax, sw::ESDBType eBlockType);
	void SetLayerDataModified(const TWorldCoords &wc, REFGUID guid, const char *strLayerName);

	bool IsSegmentLocked(const TWorldCoords &wc);
	int  LockSegment(const TWorldCoords &wc); // 0: failed, 1 - was already locked, 2 - locked
	bool UnlockSegment(const TWorldCoords &wc);

	bool IsSegmentLocked(const TWorldCoords &wcMin, const TWorldCoords &wcMax);
	int  LockSegment(const TWorldCoords &wcMin, const TWorldCoords &wcMax, bool bIgnoreSelection = false); // 0: failed, 1 - was already locked, 2 - locked
	bool UnlockSegment(const TWorldCoords &wcMin, const TWorldCoords &wcMax);

	int  LockSegmentLayerData(const TWorldCoords &wc, REFGUID guid, const char *strLayerName); // 0: failed, 1 - was already locked, 2 - locked
	bool UnlockSegmentLayerData(const TWorldCoords &wc, REFGUID guid, const char *strLayerName);
	bool  LockSegmentLayersData(const TWorldCoords &wcMin, const TWorldCoords &wcMax, bool bIgnoreSelection = false);

	bool SubmitAllSegments();
	bool LoadSeLayerMapping(TWorldCoords const& wc);
	bool SaveSegLayerMapping();
protected:
	void PushSegmentData(TSegmentData *sd);
	bool CanUnloadSegmentData(TSegmentData *sd);
	void UnloadSegmentData(TSegmentData *sd);
	bool LoadSegLayerMappingsFromXML(XmlNodeRef xmlSegLayerMappings, TSegLayerMap &mapSegLayer);
	bool SaveSegLayerMappingsToXML(XmlNodeRef xmlSegLayerMappings, TSegLayerMap &mapSegLayer);
private:
	TGlobal* m_pGlobal; //< keep this pointer to fetch global info faster
	TWorld* m_pWorld;
	TVersionMgr* m_pVersionMgr;
	TSegmentData *m_pFirstSegmentData;
	TSegmentData *m_pLastSegmentData;
	int m_nCacheSize;
	TSegmentDataMap m_mapSegmentData;
	TSegLayerMap m_mapSegLayer;

	friend struct TSegmentData;
};

SW_NAMESPACE_END();

#endif // __SegmentDataAggregator_h__
