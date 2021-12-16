#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerCreateSphereTool.h
//  Created:     Feb/1/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerDrawTool.h"

class CBrushDesignerCreateSphereTool : public CBrushDesignerDrawTool
{
public:

	CBrushDesignerCreateSphereTool()
	{
		m_fAngle = 0;
		m_vCenterOnPlane = BrushVec2(0,0);
		m_MatTo001 = BrushMatrix34::CreateIdentity();
	}
	~CBrushDesignerCreateSphereTool(){}

	void Enter() override;
	void Leave() override;

	void BeginEditParams() override;
	void EndEditParams() override;

	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnLButtonUp( CViewport *view,UINT nFlags,CPoint point ) override {}
	void OnMouseMove( CViewport *view,UINT nFlags,CPoint point ) override;
	bool OnKeyDown( CViewport *view, uint32 nChar, uint32 nRepCnt, uint32 nFlags ) override;
	void OnEditorNotifyEvent( EEditorNotifyEvent event ) override;

	void UpdateSphere( float fRadius, int nSubdivisionNum );
	void UpdateHelperDisc( float fRadius, int nSubdivisionNum );
	void UpdateDesignerBasedOnSphereRegions( const BrushMatrix34& tm );

	void Display( DisplayContext &dc ) override;

	void FreezeDesigner() override;

private:

	BrushMatrix34 m_MatTo001;
	std::vector<CBrushRegion::RegionPtr> m_SphereRegions;
	BrushVec2 m_vCenterOnPlane;
	float m_fAngle;
};