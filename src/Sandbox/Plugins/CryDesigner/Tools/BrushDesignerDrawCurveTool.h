#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerDrawCurveTool.h
//  Created:     May/7/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerDrawLineTool.h"

class CBrushDesignerDrawCurveTool : public CBrushDesignerDrawLineTool
{
public:

	CBrushDesignerDrawCurveTool()
	{
		m_ArcState = eArcState_ChooseFirstPoint;
	}
	virtual ~CBrushDesignerDrawCurveTool(){}

	void Leave();
	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ){}
	void OnLButtonUp( CViewport *view,UINT nFlags,CPoint point );
	void OnMouseMove( CViewport *view,UINT nFlags,CPoint point );
	void Display( DisplayContext &dc );
	bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags );

	void BeginEditParams() override;
	void EndEditParams() override;

	bool IsPhaseFirstStepOnPrimitiveCreation() const override;

protected:

	void PrepareArcSpots( CViewport *view,UINT nFlags,CPoint point );
	void PrepareBeizerSpots( CViewport *view,UINT nFlags,CPoint point );

protected:

	enum EDrawingArcState
	{
		eArcState_ChooseFirstPoint,
		eArcState_ChooseLastPoint,
		eArcState_ControlMiddlePoint
	};

	ELineState m_LineState;
	EDrawingArcState m_ArcState;
	SSpot m_LastSpot;
};