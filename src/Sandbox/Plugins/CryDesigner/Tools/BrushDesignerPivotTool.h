#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerPivotTool.h
//  Created:     Feb/4/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerPivotTool : public CBrushDesignerBaseTool
{
public:
	void Enter() override;
	void Leave() override;

	void Display( DisplayContext &dc ) override;

	bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags ) override;
	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnMouseMove( CViewport *view,UINT nFlags,CPoint point ) override;

	void BeginEditParams() override;
	void EndEditParams() override;

	enum EPivotSelectionType
	{
		ePST_BoundBox,
		ePST_Designer,
	};
	void SetSelectionType( EPivotSelectionType selectionType, bool bForce = false );

	void OnManipulatorDrag( CViewport *pView,ITransformManipulator *pManipulator,CPoint &p0,CPoint &p1,const BrushVec3 &value ) override;
	void OnManipulatorMouseEvent( CViewport *pView, ITransformManipulator *pManipulator, EMouseEvent event, CPoint &point, int flags, bool bHitGizmo ) override;

private:

	std::vector<BrushVec3> m_CandidateVertices;
	int m_nSelectedCandidate;
	int m_nPivotIndex;
	BrushVec3 m_PivotPos;
	BrushVec3 m_StartingDragManipulatorPos;
};
