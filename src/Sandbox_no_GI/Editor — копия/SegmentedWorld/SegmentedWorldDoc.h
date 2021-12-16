#ifndef __SegmentedWorldDoc_h__
#define __SegmentedWorldDoc_h__

#if _MSC_VER > 1000
#pragma once
#endif


#include "SegmentedWorldManager.h"
#include "SWCommon.h"
#include "WorldData.h"
#include "SegmentDataAggregator.h"
#include "ObjectDataAggregator.h"
#include "LayerInfoManager.h"
#include "ISWVersionControl.h"
#include "ISWDataPersistence.h"
#include "MiniMapGenerator.h"
#include "ViewStates.h"

#include "CryEditDoc.h"


class ICachedDBConnection;
class CXmlArchive;
class CMemoryBlock;
using sw::TSegmentData;

extern bool s_bLevelMultiPack;

SW_NAMESPACE_BEGIN();
SW_NAMESPACE_END();


// make this a class, so that everyone can easily declare friend with.
class CSWDebug{ public: void SWDBG(int dbgOption); };


class CSWDocAdapter : public CCryEditDoc
{
	CSegmentedWorldDoc* m_pSWDoc;				// data model for SW
protected:
	CSWDocAdapter();
	DECLARE_DYNCREATE(CSWDocAdapter);

public:
	virtual ~CSWDocAdapter();

protected:
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL LoadXmlArchiveArray( TDocMultiArchive& arrXmlAr, const CString& relativeLevelName, const CString& levelPath );
	virtual void ReleaseXmlArchiveArray( TDocMultiArchive& arrXmlAr ) {}
	virtual void Load(TDocMultiArchive &arrXmlAr,const CString &szFilename,bool bReloadEngineLevel );
	virtual void StartStreamingLoad();
	virtual void SerializeViewSettings( CXmlArchive &xmlAr ){}	// skip
	virtual void SyncCurrentMissionContent( bool bRetrieve );
	virtual void RepositionVegetation(){}						// skip
	virtual BOOL DoSave(LPCTSTR lpszPathName, BOOL bReplace);
	virtual BOOL OnSaveDocument( LPCTSTR lpszPathName );
	BOOL DoSaveDocument( LPCTSTR filename, CCryEditDoc::TSaveDocContext& context );
	virtual void DeleteContents();
};


class CSegmentedWorldDoc : public ISegmentedWorldDoc
{
protected:
	CSegmentedWorldDoc();
	~CSegmentedWorldDoc();

public:
	bool Init(bool bLoading, const char *pcPath, bool bForce, sw::TPersistencePtr pPersist);
	virtual const char *GetWorldName() { return m_chName; }
	CRect GetLoadedRect() const { return CRect(m_wcOfs.wx, m_wcOfs.wy, m_wcOfs.wx + m_uiWidth, m_wcOfs.wy + m_uiHeight); }
	Vec2i GetLoadedSize() const { return Vec2i(m_uiWidth,m_uiHeight);}

	inline bool IsHot() const { return CSWManager::Get().IsHot(); }
	bool NeedHotHistoryBeDeleted() const { return CSWManager::Get().NeedHotHistoryBeDeleted(); }
	
	sw::TPersistencePtr GetPersistence(){return m_pPersistance;}
	sw::TVersionControlPtr GetVersionControl(){return m_pVersionControl; }

	bool Update(bool bFromThread);
	void UpdateOnIdle();

	void DoAutoSave();
	void ConsoleCommand(IConsoleCmdArgs *pArgs);
	static void ModuleConsoleCommand(IConsoleCmdArgs *pArgs);

	// ui
	static int s_nRollUpCtrlID;			// Current rollUpCtrl ID
	static bool IsSegmentSelected(const TWorldCoords &wc);
	static bool IsInSelectMode();
	sw::TViewStates& GetViewStates()	{ return m_viewStates; }

	// As a segment data manager
	CSegmentDataAggr& SegmentAggr() { return m_segs; }
	CLayerInfoManager& LayerInfoMgr(){return m_layerInfoMgr;}


	// As a world version manager
	bool TryLockModified(std::vector<TWorldData*> &wdJustLocked, std::vector<TSegmentData*> &sdJustLocked, std::vector<TLayerData*> &ldJustLocked);
	bool TryUnLockJustLocked(std::vector<TWorldData*> &wdJustLocked, std::vector<TSegmentData*> &sdJustLocked, std::vector<TLayerData*> &ldJustLocked);
	bool TryResolveSave(bool bSave, bool bLockModified); //when going from offline to online try lock modifications and save succeed locks.
	virtual bool Revert(sw::EVersionType vt, bool bConflictsOnly){return true;}
	virtual bool RevertAll();
	bool Save(sw::ESaveFlag sflag, sw::EVersionType vt, int64 &nVer, const char* pcDescription);
	virtual bool Save(bool bModifiedOnly);
	virtual bool Load(bool bFull);
	bool CommitToOfficial( const char* pcDescription);
	bool IsForceSaving() const { return m_bForceSaving; }

	//////////////////////////////////////////////////////////////////////////
	// As an object manager
	CObjectDataAggr& ObjectAggr() {return m_extobjs;}
	void ObjectData_RollBack();
	bool CanUngroup( CBaseObject* pObject);
	bool CanMakeGroup();
	static void GetBoundBoxWithChilds( CBaseObject* pObject, AABB &box );
	bool IsObjectBig( CBaseObject* pObject);
	void SaveObjectWithChildren( CBaseObject* pObject, CObjectArchive &ar, TWorldCoords const& wcSeg );
	void LoadObjectChildrenRecursively( CBaseObject* pObject, CObjectArchive &ar );
	void DeleteObjectWithChilds( CBaseObject* pObject);
	void InvalidateObjectWithChilds( CBaseObject* pObject);
	static bool CanAccessChilds( CBaseObject* pObject);
	void PreLoadObjects( TWorldCoordsArray &arrSegmentsToLoad, CBaseObjectsArray &arrObjects, bool bUpdateStatus);

	// As a world/global info collector
	TLocalCoords WorldToSegment(const Vec3 &v, bool bClamp = true) const;
	bool IsInLoadedWorld(const TLocalCoords &lc) const { return ((lc.lx >= 0) && (lc.lx < m_uiWidth) && (lc.ly >= 0) && (lc.ly < m_uiHeight)); }
	CString const& GetLevelPath() const { return m_sPath; }
	static CString NormalizeLevelPath(const char* szPath);

	// states
	inline void OnSWModified(bool bModify = true) { m_bModified = bModify; }
	inline bool IsSWModified() { return m_bModified; }
	volatile bool m_bSuspendThread; // flag to indicate background thread should be suspended

	// configures
	sw::EVersionType GetVersionType()const {return m_vt;}
	void EnableSWFindObject(bool bEnable) {s_bSWFindObjectEnabled = bEnable;}	// TODO: NOT THREAD-SAFE!! use TLS -RL
	bool IsSWFindObjectEnabled() {return s_bSWFindObjectEnabled;}				// TODO: NOT THREAD-SAFE!! use TLS -RL
	bool IsWorldDataBlockNeeded(int wdbType);
	inline bool IsLayerDataNeeded() { return m_bEditMode; }

	// utils
	static AABB CalcWorldBounds(const TLocalCoords &lc);
	static CBaseObject* GetRootObject(CBaseObject *pObject);
	bool IsBlockTypeNeeded(sw::ESDBType eBT);
	bool GetEngDataFromLevelPak(CMemoryBlock &mem, const TWorldCoords &wc, sw::ESDBType eFileBlockType);

	//////////////////////////////////////////////////////////////////////////
	// interface from ISWDoc, object related
	virtual void InitSegmentedWorldInfo();
	CBaseObject* CreateBaseObjectFromMetaData( REFGUID guid );
	void OnAddLink( CBaseObject *pObjectLinkOwner, CBaseObject *pObjectLinkTarget);
	void OnRemoveLink( CBaseObject *pObjectLinkOwner, CBaseObject *pObjectLinkTarget);
	void OnObjectMakeExternal(CBaseObject* pObject);
	bool CanObjectMoveTo(CBaseObject *pObject, const Vec3 &newPos, CObjectLayer *pNewLayer, bool bLock = false);
	bool OnObjectMoveTo( CBaseObject *pObject, const Vec3 &newPos, CObjectLayer *pLayer);
	virtual bool IsMultiSelObjectLockedByMe();
	virtual bool IsLockedByMe(CBaseObject *pObject);
	virtual bool LockMultiSelObjects(bool bModify);
	virtual bool LockObject(CBaseObject *pObject, bool bModify);
	virtual bool CanObjectPlaceTo(const Vec3 &newPos, CObjectLayer *pLayer, bool bLock = false);
	virtual bool CanModifyMultiSel(bool bDelete, bool bLock);

	virtual void OnHeightmapModified( const AABB &bounds );
	virtual void OnVegetationModified( const AABB &bounds );
	virtual void OnObjectModified( CBaseObject *pObject,bool bDelete = false, CBaseObject* pLinkTarget = NULL);
	virtual void OnObjectModified( CBaseObject *pObject, const Vec3 &newPos, CObjectLayer *pNewLayer);
	virtual void OnTerrainRGBLayerPainted(int32 posx,int32 posy,int w,int h, bool bIsInWorldCoord = false);
	virtual void OnTerrainLayerIDPainted(int32 posx,int32 posy,int w,int h, bool bIsInWorldCoord = false);
	virtual bool OnGenerateSurfaceTexture(int wx, int wy );

	virtual bool CanModify( const TWorldCoords &wc );
	virtual bool CanModify( const AABB &bounds );				//height map check
	virtual bool CanModify( int x1, int y1, int x2, int y2, bool bIsInWorldCoord, bool bWithAdditionalSafeZone = true );
	bool CanModify(CBaseObject *pObject, bool bDelete, bool bLock);
	virtual void WorldVecToGlobalSegVec(const Vec3 &inPos, Vec3 &outPos, Vec2 &outAbsCoords);
	virtual void GlobalSegVecToLocalSegVec(const Vec3 &inPos, const Vec2 &inAbsCoords, Vec3 &outPos);
	virtual void GetTerrainSizeInMeters(int &x, int &y);
	virtual bool CreateSegments(ITerrain *pTerrain);
	virtual bool DeleteSegments(ITerrain *pTerrain);
	virtual bool FindSegment(ITerrain *pTerrain, Vec3 pt, int &nSID);
	virtual bool GetRGBLayerData(int lx, int ly, CMemoryBlock &mem);

	// Position converting
	virtual void GetSizeInUnits(uint64 &w, uint64 &h);
	virtual void GetSizeInSegments(UINT &w, UINT &h);
	virtual void GetOffsetInSegments(int &x, int &y);
	virtual Vec3 SegRelativeToEditorLocal( Vec3 const& vPos, TWorldCoords const& wc, f32 fDir ) const;
	virtual Vec3 LocalToAbsolutePosition( Vec3 const& vPos, f32 fDir = 1.f) const;

	virtual void SerializeViewSettings(IXmlNode *sw, bool bLoading);
	virtual void MoveToCam(bool bAuto = false);
	virtual bool MoveTo(int x, int y, UINT w, UINT h, bool bAuto = false);
	virtual bool ExportToGame(const char *pszGamePath, EEndian eExportEndian, CPakFile &pakFile, bool bForce = false);

	virtual void OnWorldMapUIMoved(const Vec2 &ptOfs, int nZoomLevel);
	virtual bool IsVegetationMapLockedByMe();
	virtual bool CanModifyWorldData(sw::EWDBType eType, bool bLock);
	virtual bool OnWorldDataModified(int nType);
	void SetAsForceReloadLevel();
	virtual bool NeedReloadLevel();
	virtual void OnWorldDataApplied();

	virtual void SetTerrainLayersPanelItem(CXTPTaskPanelGroupItem * pPanelGroupItem);

	virtual bool GetModifiedSegments(const CRect &worldSegRect, std::map<int,TSegmentState> &mapSegStates, int &nLayerId, bool bCheckLock, bool bCheckMissing, bool bFromThread);
	virtual bool GetModifiedSegmentsEx(const CRect &worldSegRect, std::map<int,TSegmentState> &mapSegStates, int &nLayerId, bool bCheckLock, bool bCheckMissing, bool bFromThread);

protected:
	bool CheckOpenParameters() const;
	bool GetWorldBounds(Vec2i* pMin, Vec2i* pMax);
	void UpdateEngine();
	bool CleanUp(bool bAll, int iMaxMem = MAX_CACHE_SIZE);

	void Commit();
	void OnUnlock();

	//fileBlocks
	bool GenerateLevelPakFromFileBlocks(const TWorldCoords &wc, sw::ESDBType eFileBlockType, CPakFile &pakFile, bool bForce = false);
	void DoUpdateLevelPak();
	void OnMayUpdateLevelPak();

	// world data
	//######################################################################################################
	//bool WorldData_Init();	// -1 means All
	bool WorldData_UpdateStatus(int eWDType = -1);	// -1 means All
	const char *WorldData_GetStatusText(char *pBuf, int eWDType = -1);	// -1 means All
	virtual bool GetGDState(int nWDBType, int &nWDBState, int &nLockedBy);
	bool WorlData_ReloadIfNeeded();
	bool WorldData_XmlToMemBlock(CXmlArchive &xmlAr, TWorldData& wd);
	//bool WorldData_MemBlockToXml( TWorldData& wd, CXmlArchive &xmlAr);
	bool WorldData_BinToMemBlock(const char* szFilePath, TWorldData& wd);
	//bool WorldData_MemBlockToBin(TWorldData& wd, const char* szFilePath);
	bool WorldData_Save(sw::ESaveFlag sflag, sw::EVersionType vt, int64 nVersion = -1, sw::EWDBType nWDBType = sw::WDB_COUNT);
	bool WorldData_Save( sw::ESaveFlag sflag, sw::EVersionType vt, int64 nVersion, uint32 nWDBFlags );
	bool WorldData_PrepareData( uint32 flagNeedSave, sw::ESaveFlag sflag, sw::EVersionType vt, sw::ELockResult* arrSaves );
	bool WorldData_IsModified();
	virtual bool WorldData_LoadToArchiveArray(TDocMultiArchive &arrXmlAr);
	bool WorldData_Revert(sw::EVersionType vt);
	bool WorldData_ReloadTerrainLayers(); //---Terrain Layers
	bool WorldData_ReloadVegetationMap(); //---VegetationMap

	//WorldData Console Commands handle
	bool WorldDataCC(sw::EWDBType nWDBType, sw::EWDBConsoleComands nWDBCC);
	//######################################################################################################
	bool _ObjectData_Revert_Filter( TObjectData* pObj);
	void CleanAllUnappliedLayerData( bool* pOutAnyChange = NULL);
	inline bool ShouldMove(const Vec3 &ptWorld, UINT w, UINT h);
	void CleanSectorsTexture(const TLocalCoords &lc);
	void GetObjectsFiltered(CBaseObjectsArray &arrObjects);
	bool IsBusy();
	bool NeedToShowLayerDataStatus();
	void DrawBoxes();
	void DrawBoxesEx();

	void SWDBG(int dbgOption = 0);
	void OnDBUserIDChanged(int nDBUserID);

	void UpdatePackStatus(int iNewOffsetX, int iNewOffsetY, UINT nNewWidth, UINT nNewHeight);

public:
	static bool s_bAutomate;
	static bool s_bMapConverting;  // TODO: should find a better way to do it -RL
	TWorldCoords m_wcOfs; // offset of the loaded portion, in segments
protected:
	static bool s_bSWFindObjectEnabled;		// flag for overriding ObjectManager::FindObject()
	UINT m_uiWidth, m_uiHeight; // size of the loaded portion, in segments
	CString m_sPath;
	char m_chName[64]; // world name, also primary key in worlds DB
	bool m_bEditMode; // if false, only engine data will be used
	bool m_bForceSaving;	// This is a global switch which may affect state machines of SW Objects during Saving procedure.
							// Because pass an EnvVar struct all the way through the calls of the structure tree seems really lame...

	// Realtime update
	Matrix34 m_matLastView; // used to detect camera movement, only used when delayed loading in main thread
	float m_fNextContentLoadTime; // when next delayed load can happen, only used when delayed loading in main thread
	bool m_bAutoUpdateLevelPak;
	int m_iDelayedLoading; // delayed segments loading: 0 - don't, 1 - from main thread (bad), 2 - from update thread (does not work)
	bool m_bNeedApplyObjectData; //used only in ::Update(bool bFromThread) for performance optimization is set from thread and unset from main thread
	bool m_bAutoMoveTo; // whether to automatically load around camera

	// components
	TWorldData m_WorldData[sw::WDB_COUNT];
	sw::CSegmentDataAggr m_segs;
	sw::CObjectDataAggr m_extobjs;
	CLayerInfoManager m_layerInfoMgr;
	sw::TPersistencePtr m_pPersistance;
	sw::TVersionControlPtr m_pVersionControl;
	sw::CMiniMapGenerator m_mmgen;			//screenShot (MiniMap)

	// Version Type & Hot Save
	sw::EVersionType m_vt;					// which version should be displayed in the editor
	clock_t m_lastHotSaveTime;
	bool m_bDeleteHotHistoryRecord;
	bool m_bModified;

	// Misc
	sw::TViewStates m_viewStates;			// view settings persistence variables
	CXTPTaskPanelGroupItem *m_pTerrainLayerPanelGroupItem;

public:
	//some special operations stuff
	static const int SW_SO_NONE = 0;
	static const int SW_SO_XMLS_EXEC_SCRIPT_AND_SAVE = 1;
	static int s_nSpecialOperationType;
	string m_strScriptFileName;
	string m_strScriptInputFileName;
	int GetSpecialOperationType() { return s_nSpecialOperationType; }
	void SetSpecialOperationType( int nSpecialOperationType ) { s_nSpecialOperationType = nSpecialOperationType; }
	void SW_xmls_execScriptAndSave(const char* pcScriptFileName, const char* pcScriptInputFileName);
	void CallScript(XmlNodeRef& root);
	bool LockLoadedWorld();


	friend CSegmentedWorldManager;	// for construction/destruction
	friend CSWDocAdapter;
	friend class CSWDebug;
	friend class CSegmentDataAggr;
};

#endif // __SegmentedWorldDoc_h__
