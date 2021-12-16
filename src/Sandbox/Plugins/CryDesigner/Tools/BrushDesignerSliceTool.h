#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerSliceTool.h
//  Created:     Aug/8/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerSliceTool : public CBrushDesignerBaseTool
{

public:

	CBrushDesignerSliceTool();

	virtual void Display( DisplayContext &dc ) override;

	virtual void BeginEditParams() override;
	virtual void EndEditParams() override;

	void Enter() override;
	void Leave() override;

	void CenterPivot();

	void SliceFrontPart();
	void SliceBackPart();
	void Clip();
	void Divide();
	void AlignSlicePlane( const BrushVec3& normalDir );
	void InvertSlicePlane();

	void SetNumberSlicePlane( int numberSlicePlane );
	int GetNumberSlicePlane() const {	return m_NumberSlicePlane;	}

	void OnManipulatorDrag( CViewport *pView,ITransformManipulator *pManipulator,CPoint &p0,CPoint &p1,const BrushVec3 &value ) override;
	void OnManipulatorMouseEvent( CViewport *pView, ITransformManipulator *pManipulator, EMouseEvent event, CPoint &point, int flags, bool bHitGizmo ) override;

protected:

	struct ETraverseLineInfo
	{
		ETraverseLineInfo( CBrushRegion::RegionPtr pRegion, const BrushEdge3D& edge, const BrushPlane& slicePlane ) : 
		m_pRegion(pRegion),
		m_Edge(edge),
		m_SlicePlane(slicePlane)
		{
		}
		CBrushRegion::RegionPtr m_pRegion;
		BrushEdge3D m_Edge;
		BrushPlane m_SlicePlane;
	};

	typedef std::vector<ETraverseLineInfo> TraverseLineList;
	typedef std::vector<TraverseLineList> TraverseLineLists;

	void UpdateRestTraverseLineSet();
	void UpdateSlicePlane();
	virtual bool UpdateManipulatorInMirrorMode( const BrushMatrix34& offsetTM )	{	return false;	}

	void GenerateLoop( const BrushPlane& slicePlane, TraverseLineList& outLineList ) const;
	BrushVec3 GetLoopPivotPoint() const;

	void DrawOutlines( DisplayContext& dc );
	void DrawOutline( DisplayContext& dc, TraverseLineList& lineList );
	virtual void UpdateGizmo();

	TraverseLineList m_MainTraverseLines;
	TraverseLineLists m_RestTraverseLineSet;

	static int m_NumberSlicePlane;	

	BrushVec3 m_PrevGizmoPos;
	BrushVec3 m_GizmoPos;

	BrushPlane m_SlicePlane;
	BrushVec3 m_CursorPos;

};