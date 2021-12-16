#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerBaseTool.h
//  Created:     8/12/2011 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Core/BrushPlane.h"
#include "Core/BrushRegion.h"
#include "Core/BrushRegionMesh.h"
#include "Core/BaseBrush.h"
#include "Objects/AxisGizmo.h"

class CBrushDesignerEditTool;
struct ITransformManipulator;
struct IDisplayViewport;
class CBrushDesignerElementManager;

class CBrushDesignerBaseTool : public CRefCountBase
{
public:

	CBrushDesignerBaseTool()
	{
		m_Plane = BrushPlane( BrushVec3(0,0,1), 0 );
		m_pPickedRegion = NULL;
	}

	virtual ~CBrushDesignerBaseTool(){}

	virtual void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ){}
	virtual void OnLButtonUp( CViewport *view,UINT nFlags,CPoint point ){}
	virtual void OnLButtonDblClk( CViewport *view,UINT nFlags,CPoint point ){}
	virtual void OnRButtonDown( CViewport *view,UINT nFlags,CPoint point ){}
	virtual void OnRButtonUp( CViewport *view,UINT nFlags,CPoint point ){}
	virtual void OnMButtonDown( CViewport *view,UINT nFlags,CPoint point ){}
	virtual void OnMouseMove( CViewport *view,UINT nFlags,CPoint point );
	virtual void OnMouseWheel( CViewport *view,UINT nFlags,CPoint point ){}

	virtual bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags );
	virtual void Display( DisplayContext &dc );
	virtual void OnManipulatorDrag( CViewport *view,ITransformManipulator *pManipulator,CPoint &p0,CPoint &p1,const BrushVec3 &value ) {}
	virtual void OnManipulatorMouseEvent( CViewport *view, ITransformManipulator *pManipulator, EMouseEvent event, CPoint &point, int flags, bool bHitGizmo ) {}
	virtual void BeginEditParams(){}
	virtual void EndEditParams(){}

	virtual void Enter();
	virtual void Leave();

	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event ) {}

	virtual CString GetStatusText() const;
	virtual void MaterialChanged() {};
	virtual void SetSubMatID( int nSubMatID, CBrushDesigner* pDesigner ) {}

	const BrushPlane& GetPlane() const{return m_Plane;}
	void SetPlane( const BrushPlane& plane ){m_Plane = plane;}

	void ClearRegionSelections(){m_pSelectionMesh = NULL;}

	void CreateObjectGizmo();
	void ReleaseObjectGizmo();

	int GetPanelIndex() const;

	bool SelectDesignerObject( CPoint point );

	virtual bool IsCircleTypeRotateGizmo() { return false; }
	virtual bool EnabledSeamlessSelection() const { return true; }

	virtual bool IsPhaseFirstStepOnPrimitiveCreation() const { return true; }

	virtual void FreezeDesigner();

public:

	static bool BinarySearchForScale( BrushFloat fValidScale, BrushFloat fInvalidScale, int nCount, CBrushRegion& region, BrushFloat& fOutScale );

	CBrushDesigner* GetDesigner() const;
	CBaseBrush* GetBrush() const;
	CBaseObject* GetBaseObject() const;
	BUtil::SMainContext GetMainContext() const;

	static void AddMirroredRegion( CBrushDesigner* pDesigner, CBrushRegion::RegionPtr pRegion, int opDesignerType );
	static void AddMirroredOpenRegion( CBrushDesigner* pDesigner, CBrushRegion::RegionPtr pRegion, bool bOnlyAdd );
	static void RemoveMirroredRegion( CBrushDesigner* pDesigner, CBrushRegion::RegionPtr pRegion );
	static void DrillMirroredRegion( CBrushDesigner* pDesigner, CBrushRegion::RegionPtr pRegion, bool bRemainFrame = false );
	static void RemoveRegionWithSpecificFlagsFromList( CBrushDesigner* pDesigner, std::vector<CBrushRegion::RegionPtr>& regionList, int nFlags );
	static void RemoveRegionWithoutSpecificFlagsFromList( CBrushDesigner* pDesigner, std::vector<CBrushRegion::RegionPtr>& regionList, int nFlags );
	static void CreateMirroredRegions( CBrushDesigner* pDesigner);
	static void UpdateMirroredPartWithPlane( CBrushDesigner* pDesigner, const BrushPlane& plane );
	static void EraseMirroredEdge( CBrushDesigner* pDesigner, const BrushEdge3D& edge );

	static bool MakeListConsistingOfArc( const BrushVec2& vOutsideVertex, const BrushVec2& vBaseVertex0, const BrushVec2& vBaseVertex1, int nSegmentCount, std::vector<BrushVec2>& outVertexList );
	template<class T>
	static bool ComputeCircumradiusAndCircumcenter( const T& v0, const T& v1, const T& v2, BrushFloat* outCircumradius, T* outCircumcenter );

	void DisplayDimensionHelper( DisplayContext &dc, int nShelf = -1 );
	void DisplayDimensionHelper( DisplayContext &dc, const AABB& aabb );

	bool IsOverDoubleClickTime( UINT time ) const{return GetTickCount()-time > GetDoubleClickTime();}
	bool IsTwoPointEquivalent( const CPoint& p0, const CPoint&p1 ) const	{	return std::abs(p0.x-p1.x)>2 || std::abs(p0.y-p1.y)>2 ? false : true; }

	static void UpdateGameResource( CBaseObject* pObject );
	static bool IsFrameRemainInRemovingFace( CBaseObject* pObject );
	bool IsDesignerEmpty() const;

	static CBrushDesignerEditTool* GetEditTool();

	void UpdateBrush();

protected:

	void CancelDesigner();

	bool IsSeparateStatus() const;
	virtual void StoreSeparateStatus() { m_bSeparatedNewShape = GetPickedRegion() && !GetAsyncKeyState(VK_SHIFT) ? false : true; }
	void Separate1stStep();
	void Separate2ndStep();
	bool m_bSeparatedNewShape;

	enum EMouseAction
	{
		eMouseAction_Nothing,
		eMouseAction_LButtonDown,
		eMouseAction_LButtonUp,
		eMouseAction_LButtonDoubleClick
	};

	struct SLButtonInfo
	{
		SLButtonInfo()
		{
			m_DownPos = CPoint(0,0);
			m_LastAction = eMouseAction_Nothing;
			m_DownTimeStamp = 0;
		}
		CPoint m_DownPos;
		EMouseAction m_LastAction;
		UINT m_DownTimeStamp;
	};

	virtual BrushMatrix34 GetWorldTM() const;

	static void UpdateSelectionMesh( CBrushRegion::RegionPtr pRegion, CBaseBrush* pBrush, CBaseObject* pObj,  bool bForce = false );
	static _smart_ptr<CBrushRegionMesh> GetSelectionMesh(){return m_pSelectionMesh;}

	CBrushRegion::RegionPtr GetPickedRegion() const	{ return m_pPickedRegion; }
	void SetPickedRegion( CBrushRegion::RegionPtr pRegion ){ m_pPickedRegion = pRegion; }

	void SetTempRegion( CBrushRegion::RegionPtr pRegion ) { m_pTempRegion = pRegion; }
	CBrushRegion::RegionPtr GetTempRegion() const { return m_pTempRegion; }

	void UpdateShelf( int nShelf );
	void UpdateTMManipulator( const BrushVec3& localPos, const BrushVec3& localNormal );	
	void UpdateTMManipulatorBasedOnElements( CBrushDesignerElementManager* elements );

	void ReleaseSelectionMesh();

	BrushMatrix34 GetOffsetTM( ITransformManipulator *pManipulator, const BrushVec3& vOffset ) const;
	void Sync();

	static _smart_ptr<CBrushRegionMesh> m_pSelectionMesh;
	_smart_ptr<CAxisGizmo> m_pObjectGizmo;

private:

	CBrushRegion::RegionPtr m_pPickedRegion;
	CBrushRegion::RegionPtr m_pTempRegion;
	BrushPlane m_Plane;

};