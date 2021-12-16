#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerDrawDiscTool.h
//  Created:     May/7/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerDrawTool.h"

class CBrushDesignerDrawDiscTool : public CBrushDesignerDrawTool
{
public:

	CBrushDesignerDrawDiscTool()
	{
		m_fAngle = 0;
	}
	~CBrushDesignerDrawDiscTool(){}

	void Enter() override;
	void Leave() override;

	void BeginEditParams() override;
	void EndEditParams() override;

	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point )  override;
	void OnLButtonUp( CViewport *view,UINT nFlags,CPoint point ) override{}
	void OnMouseMove( CViewport *view,UINT nFlags,CPoint point ) override;
	bool OnKeyDown( CViewport *view, uint32 nChar, uint32 nRepCnt, uint32 nFlags ) override;
	void OnEditorNotifyEvent( EEditorNotifyEvent event ) override;
	void UpdateDisc( float fRadius, int nSubdivisionNum );
	void Display( DisplayContext &dc ) override;

	void RegisterDrawnRegionToDesigner();

	void StoreSeparateStatus() override { m_bSeparatedNewShape = !GetPickedRegion() && GetAsyncKeyState(VK_SHIFT); }

private:

	BrushVec2 m_vCenterOnPlane;
	float m_fAngle;
};