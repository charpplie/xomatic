#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerCreateConeTool.h
//  Created:     Feb/1/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerDrawTool.h"

class CBrushDesignerCreateConeTool : public CBrushDesignerDrawTool
{
public:

	CBrushDesignerCreateConeTool()
	{
		m_fAngle = 0;
		m_pBaseRegion = NULL;
		m_vCenterOnPlane = BrushVec2(0,0);
		m_ConePhase = eConePhase_PlaceFirstPoint;
	}
	~CBrushDesignerCreateConeTool(){}

	void Enter() override;
	void Leave() override;

	void BeginEditParams() override;
	void EndEditParams() override;

	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnLButtonUp( CViewport *view,UINT nFlags,CPoint point ) override{}
	void OnMouseMove( CViewport *view,UINT nFlags,CPoint point ) override;
	bool OnKeyDown( CViewport *view, uint32 nChar, uint32 nRepCnt, uint32 nFlags ) override;
	void Display( DisplayContext &dc ) override;
	void OnEditorNotifyEvent( EEditorNotifyEvent event ) override;

	bool IsPhaseFirstStepOnPrimitiveCreation() const override { return m_ConePhase == eConePhase_PlaceFirstPoint; }
	bool EnabledSeamlessSelection() const override{ return m_ConePhase == eConePhase_PlaceFirstPoint && !IsDesignerEmpty() ? true : false; }

	void UpdateAll( float fRadius, float fHeight, int nSubDivisionNum );	

private:

	void UpdateCone( float fHeight );
	void UpdateBaseRegion( float fRadius, int nNumOfSubdivision );

	enum EConePhase
	{
		eConePhase_PlaceFirstPoint,
		eConePhase_Radius,
		eConePhase_RaiseHeight,
		eConePhase_Done,
	};

	EConePhase m_ConePhase;
	BrushVec2 m_vCenterOnPlane;
	float m_fAngle;
	CBrushRegion::RegionPtr m_pBaseRegion;
};