#ifndef __ObjectDataAggregator_h__
#define __ObjectDataAggregator_h__

#if _MSC_VER > 1000
#pragma once
#endif


#include "VersionCommon.h"

SW_NAMESPACE_BEGIN();

class CObjectDataAggr
{
	typedef CSegmentedWorldDoc TGlobal;
	typedef CSegmentedWorldDoc TWorld;
	typedef CSegmentedWorldDoc TVersionMgr;

	typedef std::map<GUID, TObjectData*, guid_less_predicate> TObjectDataMap;
	typedef std::map<SegmentID, std::vector<TObjectData*> > TObjectLayerMap;

public:
	CObjectDataAggr(TGlobal* pGlobal, TWorld* pWorld, TVersionMgr* pVersionMgr )
		:m_pGlobal(pGlobal),m_pWorld(pWorld),m_pVersionMgr(pVersionMgr)
	{}
	// TODO: all of these prefixes are no longer needed
	TObjectData* GetObjectData(REFGUID guid, bool* pbCreated = NULL);
	TObjectData* GetObjectData( CBaseObject* pObj, bool* pbCreated = NULL );
	TObjectData* FindObjectData(REFGUID guid);
	TObjectData* FindObjectData( CBaseObject* pObj);
	CBaseObject* CreateBaseObjectFromMetaData( REFGUID guid );
	bool IsExternal(CBaseObject *pObject, bool *bCreatedNow = NULL );
	bool ObjectData_Load(bool bUpdateStatus);
	void UnloadAll();
	bool SaveForSegment(sw::ESaveFlag sflag, const TLocalCoords &lc, CBaseObjectsArray &arrObjects, int64 nVersion, sw::EVersionType vt = sw::VT_CURRENT);
	bool ObjectData_Integrate(const TLocalCoords &lc, CBaseObjectsArray &arrObjects, int64 nVersion, sw::EVersionType vt);
	void ObjectData_UpdateStatus();
	void ObjectData_SetModified(CBaseObject *pObject, bool bDelete, CBaseObject* pLinkTarget = NULL);
	bool ObjectData_Revert(sw::EVersionType vt, bool bConflictsOnly, Functor1wRet<TObjectData*,bool>& filter);
	bool ObjectData_NeedsToBeApplied();
	void ObjectData_Reload();
	bool ObjectData_NeedsReload();
	bool ObjectData_IsModified(const TLocalCoords &lc);
	bool ObjectData_IsSaved(const TLocalCoords &lc, sw::EVersionType vt);
	void ObjectData_NoLongerApplied(const TWorldCoords &wc);
	//void ObjectData_Commit(int64 nVersion);
	//void ObjectData_RollBack();
	void ObjectData_GetAppliedHrids(const TLocalCoords &lc, std::vector<int64> &arrHrids);
	void ObjectData_MakeExternal(TObjectData* pObjData, CBaseObject* pObject, bool bForceModify = false);
	void OnAddLink( CBaseObject* pObjectLinkOwner, CBaseObject* pObjectLinkTarget);
	void OnRemoveLink( CBaseObject* pObjectLinkOwner, CBaseObject* pObjectLinkTarget);
	//void EnableSWFindObject(bool bEnable) {s_bSWFindObjectEnabled = bEnable;};
	//bool IsSWFindObjectEnabled() {return s_bSWFindObjectEnabled;}
	//bool CanUngroup( CBaseObject* pObject);
	//bool CanMakeGroup();
	//void SaveObjectWithChilds( CBaseObject* pObject, CObjectArchive &ar );
	//void LoadObjectWithChilds( CBaseObject* pObject, CObjectArchive &ar, Vec3 *pOfs);
	//void DeleteObjectWithChilds( CBaseObject* pObject);
	void InvalidateExternalObjects( REFGUID layerGUID, TLocalCoords const& lc );
	void ResetNeedInWorld();
	void CleanAllUnappliedData( bool* bAnyCleaned = NULL, bool* bAnyDontNeed = NULL );
	size_t FetchAll_NeedToBeApplied( std::vector<TObjectData*>& arrUnapplied );
	size_t FetchAll_External( std::vector<TObjectData*>& arrExternal );
	bool SaveToDB(sw::ESaveFlag sflag, sw::EVersionType vt, int64 nVersion );
	bool IntegrateExternalObjects(sw::EVersionType vt, int64 nVersion );
	size_t FetchAll_InRect( std::vector<TObjectData*>& arrOut, const CRect & rect );
	//void InvalidateObjectWithChilds( CBaseObject* pObject);
	//void PreLoadObjects( TWorldCoordsArray &arrSegmentsToLoad, CBaseObjectsArray &arrObjects, bool bUpdateStatus);
	//int CreateObjectLayer(REFGUID guid, const string &strLayerName);
	void SetAllAsMissing();
	// Disapprove an external object if necessary
	void RecheckExternal( CBaseObject* pObject, CBaseObject* pLinkObject );
	void DeleteExtObj(TObjectData* pObjData);

	bool LoadObjLayerMapping(const TWorldCoords& wc);
	bool SaveObjLayerMapping();
protected:
	TObjectData* DoCreateObjectData( REFGUID guid );

private:
	TObjectDataMap m_objectDataMap;
	TObjectDataMap	m_deletedObjects;
	TObjectLayerMap m_mapObjLayer;
	TGlobal* m_pGlobal; //< keep this pointer to fetch global info faster
	TWorld* m_pWorld;
	TVersionMgr* m_pVersionMgr;
};

SW_NAMESPACE_END();

#endif // __ObjectDataAggregator_h__
