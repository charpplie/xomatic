#include "StdAfx.h"
#include "InternalCommon.h"
#include "ISegmentedWorldDoc.h"
#include "ViewStates.h"



//////////////////////////////////////////////////////////////////////////
// Null Object for SWDoc
//
//	Helps those subsystems that been injected with SW code, provides an easier
//	way to call SW procedure with no need to care about whether it's a
//	SW level or not. which also makes code cleaner(since the guilty of injection)
//////////////////////////////////////////////////////////////////////////
class CNullSegmentedWorldDoc : public ISegmentedWorldDoc
{
public:
	CNullSegmentedWorldDoc()
		:ISegmentedWorldDoc(true)
	{}
protected:
	virtual void GetSizeInUnits(uint64 &w, uint64 &h)	{}
	virtual void GetSizeInSegments(UINT &w, UINT &h) {}
	virtual void GetOffsetInSegments(int &x, int &y) {}
	//virtual UINT GetSegmentSizeInUnits() {}
	//virtual int GetSegmentSizeMeters() {return 0;}
	virtual Recti GetLoadedRect() const { return Recti(0, 0, 0, 0); }
	virtual bool GetWorldBounds(Vec2i* pMin, Vec2i* pMax) {return false;}
	virtual Vec3 SegRelativeToEditorLocal( Vec3 const& vPos, TWorldCoords const& wc, f32 fDir = 1.f ) const { return vPos; }

	virtual void MoveToCam(bool bAuto = false) {}
	virtual bool MoveTo(int x, int y, UINT w, UINT h, bool bAuto = false) {return false;}
	virtual void SetContinuousLoading(bool bEnable) {}

	virtual void InitSegmentedWorldInfo() {}
	virtual bool Save(bool bModifiedOnly, bool bLevelCreation) {return false;}
	virtual bool Load(bool bFull) {return false;}

	virtual bool WorldData_LoadToArchiveArray(TDocMultiArchive &arrXmlAr) {return false;}

	// support for world map UI
	virtual void OnWorldMapUIMoved(const Vec2 &vOrig) {}

	// support for RGBLayer, hopefuly will be removed
	virtual bool GetRGBLayerData(int lx, int ly, CMemoryBlock &mem) {return false;}

	virtual const char *GetWorldName() {return ""; }
	virtual sw::TViewStates& GetViewStates() {static sw::TViewStates __nullStates; return __nullStates;}
	virtual void SetTerrainLayersPanelItem(CXTPTaskPanelGroupItem * pPanelGroupItem) {}
	virtual bool GetModifiedSegments(const CRect &worldSegRect, std::map<int,TSegmentState> &mapSegStates, int &nLayerId, bool bCheckLock, bool bCheckMissing, bool bFromThread) {return false;}
	virtual bool GetGDState(int nWDBType, int &nWDBState, int &nLockedBy) {return false;}


	virtual bool NeedReloadLevel() {return true;}
	virtual void OnWorldDataApplied() {}
	//GD's notification callbacks
	virtual bool OnWorldDataModified(int nType) {return true;}
	//static bool OnTerrainLayersModified();
	//static bool OnVegetationMapModified();

	// change notification callbacks, same as console synchronization
	virtual void OnHeightmapModified( const AABB &bounds ) {}
	virtual void OnVegetationModified( const AABB &bounds ) {}
	virtual void OnTerrainRGBLayerPainted(int32 posx,int32 posy,int w,int h, bool bIsInWorldCoord = false) {}
	virtual void OnTerrainLayerIDPainted(int32 posx,int32 posy,int w,int h, bool bIsInWorldCoord = false) {}
	virtual bool OnGenerateSurfaceTexture(int wx, int wy ) { return false; }

	virtual void OnObjectModified( CBaseObject *pObject, bool bDeleted = false, CBaseObject* pLinkObject = NULL) {}

	// Objects check and notification
	virtual bool IsSWFindObjectEnabled() { return false;}
	virtual bool CanUngroup( CBaseObject *pObject) { return true;}
	virtual bool CanMakeGroup() { return true;}
	virtual bool IsMultiSelObjectLockedByMe() { return true;}
	virtual bool IsLockedByMe(CBaseObject *pObject) { return true;}
	virtual bool LockMultiSelObjects(bool bModify) { return true;}
	virtual bool LockObject(CBaseObject *pObject, bool bModify) { return true;}
	virtual bool CanObjectPlaceTo(const Vec3 &newPos, CObjectLayer *pLayer, bool bLock = false) { return true;}
	virtual bool CanModifyMultiSel(bool bDelete, bool bLock) { return true;}
	virtual bool CanModify(CBaseObject *pObject, bool bDelete, bool bLock) { return true;}
	virtual bool CanObjectMoveTo(CBaseObject *pObject, const Vec3 &newPos, CObjectLayer *pNewLayer, bool bLock = false) { return true;}
	virtual bool OnObjectMoveTo( CBaseObject *pObject, const Vec3 &newPos, CObjectLayer *pLayer) { return true;}
	virtual CBaseObject* CreateBaseObjectFromMetaData( REFGUID guid ) { return NULL;}
	virtual void EnableSWFindObject(bool bEnable) { }
	virtual void OnAddLink( CBaseObject *pObjectLinkOwner, CBaseObject *pObjectLinkTarget) {}
	virtual void OnRemoveLink( CBaseObject *pObjectLinkOwner, CBaseObject *pObjectLinkTarget) { }
	virtual void OnObjectMakeExternal(CBaseObject* pObject) { }


	// Heightmap check
	virtual bool CanModify( const AABB &bounds ) { return true; }
	virtual bool CanModify( int x1, int y1, int x2, int y2, bool bIsInWorldCoord, bool bWithAdditionalSafeZone = true ) { return true; }

	virtual bool IsVegetationMapLockedByMe() { return true; }
	
	// ISegmentsManager
	virtual void WorldVecToGlobalSegVec(const Vec3 &inPos, Vec3 &outPos, Vec2 &outAbsCoords) {}
	virtual void GlobalSegVecToLocalSegVec(const Vec3 &inPos, const Vec2 &inAbsCoords, Vec3 &outPos) {}
	virtual Vec3 WorldVecToLocalSegVec(const Vec3 &vPos) { return vPos; }
	virtual Vec3 LocalToAbsolutePosition( Vec3 const& vPos, f32 fDir = 1.f ) const { return vPos; }
	virtual void GetTerrainSizeInMeters(int &x, int &y) {}
	virtual int GetSegmentSizeInMeters() { return SEGMENT_SIZE_METERS; }
	virtual bool CreateSegments(ITerrain *pTerrain) { return true; }
	virtual bool DeleteSegments(ITerrain *pTerrain) { return true; }
	virtual bool FindSegment(ITerrain *pTerrain, const Vec3 &pt, int &nSID) { return true; }
	virtual bool FindSegmentCoordByID(int nSID, int &x, int &y) { return true; }
	virtual void ForceLoadSegments(unsigned int flags) {}
	virtual bool PushEntityToSegment(EntityId id, bool bLocal = true){ return false; }
	// ~ISegmentsManager

};

ISegmentedWorldDoc& ISegmentedWorldDoc::GetNullObject()
{
	static CNullSegmentedWorldDoc __nullDoc;

	return __nullDoc;
}
