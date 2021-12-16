#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerCreateCylinderTool.h
//  Created:     Feb/1/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerDrawTool.h"

class CBrushDesignerCreateCylinderTool : public CBrushDesignerDrawTool
{
public:

	CBrushDesignerCreateCylinderTool()
	{
		m_fAngle = 0;
		m_pBaseRegion = NULL;
		m_vCenterOnPlane = BrushVec2(0,0);
		m_CylinderPhase = eCylinderPhase_PlaceFirstPoint;
	}
	~CBrushDesignerCreateCylinderTool(){}

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

	void UpdateHeightWithBoundaryCheck( BrushFloat fHeight );
	void UpdateAll( float fRadius, float fHeight, int nSubDivisionNum );

	bool IsPhaseFirstStepOnPrimitiveCreation() const override { return m_CylinderPhase == eCylinderPhase_PlaceFirstPoint; }
	bool EnabledSeamlessSelection() const override { return m_CylinderPhase == eCylinderPhase_PlaceFirstPoint && !IsDesignerEmpty() ? true : false; }

private:

	void FreezeDesigner() override;
	void UpdateHeight( float fHeight );
	void UpdateBaseRegion( float fRadius, int nNumOfSubdivision );

	enum ECylinderPhase
	{
		eCylinderPhase_PlaceFirstPoint,
		eCylinderPhase_Radius,
		eCylinderPhase_RaiseHeight,
		eCylinderPhase_Done,
	};

	ECylinderPhase m_CylinderPhase;
	BrushVec2 m_vCenterOnPlane;
	float m_fAngle;
	bool m_bIsOverOpposite;
	CBrushRegion::RegionPtr m_pBaseRegion;
	CBrushRegion::RegionPtr m_pCapRegion;
};