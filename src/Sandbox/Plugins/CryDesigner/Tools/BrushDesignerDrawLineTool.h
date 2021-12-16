#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerDrawLineTool.h
//  Created:     May/7/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerDrawTool.h"

class CBrushDesignerDrawLineTool : public CBrushDesignerDrawTool
{
public:

	CBrushDesignerDrawLineTool() : m_bAlignedToRecentSpotLine(false)
	{
	}
	virtual ~CBrushDesignerDrawLineTool(){}
	virtual void Enter() override
	{
		__super::Enter();
		m_bHasValidRecentEdge = false;
	}
	virtual void Leave() override
	{
		Complete();
		ResetAllSpots();
		__super::Leave();
	}

	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnLButtonUp( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnMouseMove( CViewport *view,UINT nFlags,CPoint point ) override;
	virtual void Display( DisplayContext &dc ) override;
	virtual bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags ) override;

	bool IsPhaseFirstStepOnPrimitiveCreation() const override;

protected:

	virtual void CreateRegionFromSpots( bool bCloseRegion, const SpotList& spotList );
	bool IntersectExisintingLines( const BrushVec3& v0, const BrushVec3& v1, int* pOutSpotIndex = NULL ) const;

	ELineState GetLineState() const{ return m_LineState; }
	void SetLineState( ELineState lineState ){ m_LineState = lineState; }

	ELineState GetAlienedPointWithAxis( const BrushVec3& v0, const BrushVec3& v1, const BrushPlane& plane, BrushFloat angle, std::vector<BrushEdge3D>* pAxisList, BrushVec3& outPos ) const;	
	void AddRegionWithCurrentSameAsFirst();
	void RegisterClosedRegion();

	void RegisterEitherEndSpotList();
	void RegisterSpotListAfterBreaking();

	void AlignEdgeWithPrincipleAxises( IDisplayViewport* view, bool bAlign );
	void PutCurrentSpot();

	void Complete();

	void OnEditorNotifyEvent( EEditorNotifyEvent event ) override;

private:

	ELineState m_LineState;

	SSpot m_RecentSpotOnEdge;
	BrushEdge3D m_RecentEdge;
	bool m_bHasValidRecentEdge;
	bool m_bAlignedToRecentSpotLine;
	bool m_bAlignedToAnotherEdge;

};