#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerDrawTool.h
//  Created:     May/5/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"
#include "Core/BrushDesignerSpotManager.h"

class CBrushDesignerDrawTool : public CBrushDesignerBaseTool, public CBrushDesignerSpotManager
{
public:

	CBrushDesignerDrawTool()
	{
		m_EditMode = eEditMode_None;
		m_pIntermediateRegion = NULL;
	}
	virtual ~CBrushDesignerDrawTool(){}

	virtual void Leave() override
	{
		__super::Leave();
		m_EditMode = eEditMode_None;
		ResetCurrentSpot();
	}

	virtual void OnLButtonDown( CViewport *view, UINT nFlags, CPoint point ) override;
	virtual void OnLButtonUp( CViewport *view, UINT nFlags, CPoint point ) override;
	virtual void OnMouseMove( CViewport *view, UINT nFlags, CPoint point ) override;
	virtual bool OnKeyDown( CViewport *view, uint32 nChar, uint32 nRepCnt, uint32 nFlags ) override;
	virtual void Display( DisplayContext &dc ) override;

	virtual bool IsPhaseFirstStepOnPrimitiveCreation() const override { return m_EditMode == eEditMode_Beginning; }

	void DisplayCurrentSpot( DisplayContext &dc );

	bool EnabledSeamlessSelection() const override { return m_EditMode == eEditMode_Beginning && !IsDesignerEmpty() ? true : false; }

protected:

	bool UpdateCurrentSpotPosition( CViewport *view, UINT nFlags, CPoint point, bool bKeepInitialPlane, bool bSearchAllShelves = false );
	void UpdateDrawnRegion( const BrushVec2& p0, const BrushVec2& p1 );

protected:

	enum EEditMode
	{
		eEditMode_Beginning,
		eEditMode_Editing,
		eEditMode_Done,
		eEditMode_None
	};

	enum ELineState
	{
		eLineState_Diagonal,
		eLineState_ParallelToAxis,
		eLineState_Cross
	};

protected:

	BUtil::STexInfo GetTexInfo() const;
	int GetMatID() const;	

	void SetIntermediateRegion( CBrushRegion::RegionPtr pRegion ){m_pIntermediateRegion = pRegion;}
	CBrushRegion::RegionPtr GetIntermediateRegion() const{return m_pIntermediateRegion;}
	void DrawIntermediateRegion( DisplayContext &dc );

	EEditMode GetEditMode() const{return m_EditMode;}
	void SetEditMode( EEditMode editMode ){m_EditMode = editMode;}

	virtual void FreezeDesigner() override;

	static void MakeRectangle( const BrushPlane& plane, const BrushVec2& startPos, const BrushVec2& endPos, std::vector<BrushVec3>& outVertices );
	static bool IsPointInTriangle( const BrushVec2& vTriV0, const BrushVec2& vTriV1, const BrushVec2& vTriV2, const BrushVec2& vPoint );	

private:

	EEditMode m_EditMode;
	CBrushRegion::RegionPtr m_pIntermediateRegion;

};