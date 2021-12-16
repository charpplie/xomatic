#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerDrawRectangleTool.h
//  Created:     May/7/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerDrawTool.h"

class CBrushDesignerDrawRectangleTool : public CBrushDesignerDrawTool
{
public:

	void Enter() override;
	void Leave() override;

	void BeginEditParams() override;
	void EndEditParams() override;

	void OnLButtonDown( CViewport *view, UINT nFlags, CPoint point ) override;
	void OnLButtonUp( CViewport *view, UINT nFlags, CPoint point ) override;
	void OnMouseMove( CViewport *view, UINT nFlags, CPoint point ) override;
	bool OnKeyDown( CViewport *view, uint32 nChar, uint32 nRepCnt, uint32 nFlags ) override;
	void OnEditorNotifyEvent( EEditorNotifyEvent event ) override;

	void Display( DisplayContext &dc ) override;

	bool IsPhaseFirstStepOnPrimitiveCreation() const override { return m_Phase == eRectanglePhase_PlaceFirstPoint; }
	bool EnabledSeamlessSelection() const override { return m_Phase == eRectanglePhase_PlaceFirstPoint && !IsDesignerEmpty() ? true : false; }

	void StoreSeparateStatus() override { m_bSeparatedNewShape = !GetPickedRegion() && GetAsyncKeyState(VK_SHIFT); }
	BrushVec3 (&GetRectangleTwoVertices())[2] { return m_v; }

	void UpdateRectangle( const BrushVec3& v0, const BrushVec3& v1, bool bRenderFace );

private:	

	enum ERectanglePhase
	{
		eRectanglePhase_PlaceFirstPoint,
		eRectanglePhase_DrawRectangle,
		eRectanglePhase_Done,
	};

	ERectanglePhase m_Phase;
	BrushVec3 m_v[2];
	
};