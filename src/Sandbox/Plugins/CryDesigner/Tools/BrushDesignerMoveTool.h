#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerMoveTool.h
//  Created:     Sep/1/2012 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#include "BrushDesignerBaseTool.h"
#include "Core/BrushDesigner.h"
#include "BrushDesignerSelectTool.h"

class CBrushDesigner;
class CBrushDesignerMovePipeline;

class CBrushDesignerMoveTool: public CBrushDesignerSelectTool
{
public:
	CBrushDesignerMoveTool( int pickFlag );
	CBrushDesignerMoveTool();
	~CBrushDesignerMoveTool();

	void Enter() override;
	void Leave() override;
	void OnLButtonDown( CViewport *pView,UINT nFlags,CPoint point ) override;
	void OnLButtonUp( CViewport *pView,UINT nFlags,CPoint point ) override;
	void OnMouseMove( CViewport *pView,UINT nFlags,CPoint point ) override;
	void OnManipulatorDrag( CViewport *pView,ITransformManipulator *pManipulator,CPoint &p0,CPoint &p1,const BrushVec3 &value ) override;
	void OnManipulatorMouseEvent( CViewport *pView, ITransformManipulator *pManipulator, EMouseEvent event, CPoint &point, int flags, bool bHitGizmo ) override;
	void OnEditorNotifyEvent( EEditorNotifyEvent event ) override;

	static void Transform( BUtil::SMainContext& mc, const BrushMatrix34& tm, bool bMoveTogether );

private:

	void StartTransformation( bool bSeparate );
	void TransformSelections( const BrushMatrix34& offsetTM );
	void EndTransformation();

	static void TransformSelections( BUtil::SMainContext& mc, CBrushDesignerMovePipeline& pipeline, const BrushMatrix34& offsetTM );

	BrushMatrix34 GetOffsetTMOnAlignedPlane( CViewport *pView, const BrushPlane& planeAlighedWithView, CPoint prevPos, CPoint currentPos );
	void InitializeMovementOnViewport( CViewport* pView, UINT nMouseFlags );

	BrushPlane m_PlaneAlignedWithView;
	std::unique_ptr<CBrushDesignerMovePipeline> m_Pipeline;
	bool m_bManipulatingGizmo;
	BrushVec3 m_SelectedElementNormal;
};