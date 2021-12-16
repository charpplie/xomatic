#pragma once

#include "BrushDesignerDrawTool.h"

class CBrushDesignerCreateBoxTool : public CBrushDesignerDrawTool
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

	void UpdateBoxWithBoundaryCheck( const BrushVec3& v0, const BrushVec3& v1, BrushFloat fHeight );

	void Display( DisplayContext &dc ) override;

	bool IsPhaseFirstStepOnPrimitiveCreation() const override { return m_Phase == eBoxPhase_PlaceFirstPoint; }

	void OnEditorNotifyEvent( EEditorNotifyEvent event ) override;
	bool EnabledSeamlessSelection() const override { return m_Phase == eBoxPhase_PlaceFirstPoint && !IsDesignerEmpty() ? true : false; }

	BrushVec3 (&GetBottomRectangleTwoVertices())[2] { return m_v; }

private:

	void FreezeDesigner() override;
	void UpdateBox( const BrushVec3& v0, const BrushVec3& v1, BrushFloat fHeight );

	enum EBoxPhase
	{
		eBoxPhase_PlaceFirstPoint,
		eBoxPhase_DrawRectangle,
		eBoxPhase_RaiseHeight,
		eBoxPhase_Done,
	};

	EBoxPhase m_Phase;	
	bool m_bIsOverOpposite;
	CBrushRegion::RegionPtr m_pCapRegion;
	bool m_bStartedUndo;
	BrushVec3 m_v[2];
};