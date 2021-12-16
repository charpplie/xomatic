#pragma once
#include "SWFwd.h"
#include "SWCommon.h"



// Forward declares
class CHyperGraph;
class CBaseObject;


struct ISegmentedWorldDoc : public ISegmentsManager
{
	static ISegmentedWorldDoc& GetNullObject();
	inline bool IsNull()const {return m_bNull;}
	inline bool IsOk() const {return !IsNull();}
	inline operator bool() const {return !IsNull(); }
	inline bool operator! () const { return IsNull(); }
	//ISegmentedWorldDoc* operator->() {return this;}

	virtual ~ISegmentedWorldDoc(){}
	virtual void GetSizeInUnits(uint64 &w, uint64 &h) = 0; // size of the loaded portion, in height map units
	virtual void GetSizeInSegments(UINT &w, UINT &h) = 0; // size of the loaded portion, in segments
	virtual void GetOffsetInSegments(int &x, int &y) = 0; // offset of the loaded portion, relative to world's (0,0), in segments
	//virtual UINT GetSegmentSizeInUnits() = 0; // size of one segment, in height map units
	//virtual int GetSegmentSizeMeters() = 0;
	virtual CRect GetLoadedRect() const = 0;
	virtual bool GetWorldBounds(Vec2i* pMin, Vec2i* pMax) = 0;
	virtual Vec3 SegRelativeToEditorLocal( Vec3 const& vPos, TWorldCoords const& wc, f32 fDir = 1.f ) const =0;

	virtual void MoveToCam(bool bAuto = false) = 0;
	virtual bool MoveTo(int x, int y, UINT w, UINT h, bool bAuto = false) = 0;
	bool MoveTo(const CRect& rcArea, bool bAuto = false) { return MoveTo(rcArea.left,rcArea.top,rcArea.Width(),rcArea.Height(),bAuto); }

	virtual void InitSegmentedWorldInfo() =0;
	virtual bool Save(bool bModifiedOnly) = 0;
	virtual bool Load(bool bFull) = 0;
	virtual bool ExportToGame(const char *pszGamePath, EEndian eExportEndian, CPakFile &pakFile, bool bForce = false) = 0;

	virtual bool WorldData_LoadToArchiveArray(TDocMultiArchive &arrXmlAr) = 0;

	// support for world map UI
	//virtual void InitWorldMapUI() = 0;
	virtual void OnWorldMapUIMoved(const Vec2 &ptOfs, int nZoomLevel) = 0;

	// support for RGBLayer, hopefuly will be removed
	virtual bool GetRGBLayerData(int lx, int ly, CMemoryBlock &mem) = 0;

	virtual const char *GetWorldName() = 0;
	virtual sw::TViewStates& GetViewStates() =0;

	virtual void SetTerrainLayersPanelItem(CXTPTaskPanelGroupItem * pPanelGroupItem) = 0;

	virtual bool GetModifiedSegments(const CRect &worldSegRect, std::map<int,TSegmentState> &mapSegStates, int &nLayerId, bool bCheckLock, bool bCheckMissing, bool bFromThread) = 0;

	virtual bool GetGDState(int nWDBType, int &nWDBState, int &nLockedBy) = 0;


	virtual bool NeedReloadLevel() =0;
	virtual void OnWorldDataApplied() =0;
	//GD's notification callbacks
	virtual bool OnWorldDataModified(int nType) =0;
	//static bool OnTerrainLayersModified();
	//static bool OnVegetationMapModified();

	// change notification callbacks, same as console synchronization
	virtual void OnHeightmapModified( const AABB &bounds ) =0;
	virtual void OnVegetationModified( const AABB &bounds ) =0;
	virtual void OnTerrainRGBLayerPainted(int32 posx,int32 posy,int w,int h, bool bIsInWorldCoord = false) =0;
	virtual void OnTerrainLayerIDPainted(int32 posx,int32 posy,int w,int h, bool bIsInWorldCoord = false) =0;
	virtual bool OnGenerateSurfaceTexture(int wx, int wy ) =0;

	virtual void OnObjectModified( CBaseObject *pObject, bool bDeleted = false, CBaseObject* pLinkObject = NULL) =0;
	void OnObjectCreated( CBaseObject *pObject)	{ OnObjectModified(pObject, false); }
	void OnObjectDelete( CBaseObject *pObject) { OnObjectModified(pObject, true); }

	// Objects check and notification
	virtual bool IsSWFindObjectEnabled() =0;
	virtual bool CanUngroup( CBaseObject *pObject) =0;
	virtual bool CanMakeGroup() =0;
	virtual bool IsMultiSelObjectLockedByMe() =0;
	virtual bool IsLockedByMe(CBaseObject *pObject) =0;
	bool IsLockedByMe(CHyperGraph* pGraph);
	virtual bool LockMultiSelObjects(bool bModify) =0;
	virtual bool LockObject(CBaseObject *pObject, bool bModify) =0;
	bool LockGraph(CHyperGraph* pGraph, bool bModify);
	virtual bool CanObjectPlaceTo(const Vec3 &newPos, CObjectLayer *pLayer, bool bLock = false) =0;
	virtual bool CanModifyMultiSel(bool bDelete, bool bLock) =0;
	virtual bool CanModify(CBaseObject *pObject, bool bDelete, bool bLock) =0;
	bool CanModify(CHyperGraph* pGraph, bool bLock);
	virtual bool CanObjectMoveTo(CBaseObject *pObject, const Vec3 &newPos, CObjectLayer *pNewLayer, bool bLock = false) =0;
	virtual bool OnObjectMoveTo( CBaseObject *pObject, const Vec3 &newPos, CObjectLayer *pLayer) =0;
	virtual CBaseObject* CreateBaseObjectFromMetaData( REFGUID guid ) =0;
	virtual void EnableSWFindObject(bool bEnable) =0;
	virtual void OnAddLink( CBaseObject *pObjectLinkOwner, CBaseObject *pObjectLinkTarget) =0;
	virtual void OnRemoveLink( CBaseObject *pObjectLinkOwner, CBaseObject *pObjectLinkTarget) =0;
	virtual void OnObjectMakeExternal(CBaseObject* pObject) =0;

	// Heightmap check
	virtual bool CanModify( const AABB &bounds ) =0;
	virtual bool CanModify( int x1, int y1, int x2, int y2, bool bIsInWorldCoord, bool bWithAdditionalSafeZone = true ) =0;

	// vegetation
	virtual bool IsVegetationMapLockedByMe() =0;

	//virtual CSCDataPersistence* GetPersistance() =0;
	//virtual sw::ISWVersionControl* GetVersionControl() =0;

	// utils

protected:
	explicit ISegmentedWorldDoc(bool bNull) :m_bNull(bNull) {}
	bool m_bNull;	// is a Null Object?
};
