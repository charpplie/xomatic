#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerEditOffsetTool.h
//  Created:     May/7/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerBaseTool.h"

class CBrushDesignerOffsetTool : public CBrushDesignerBaseTool
{
public:
	CBrushDesignerOffsetTool() : m_fScale(0)
	{
	}
	virtual ~CBrushDesignerOffsetTool(){}

	void Leave() override;
	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnLButtonUp( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnMouseMove( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnLButtonDblClk( CViewport *view,UINT nFlags,CPoint point ) override;
	void Display( DisplayContext &dc ) override;

	static void ApplyOffset( CBrushDesigner* pDesigner, CBrushRegion::RegionPtr pScaledRegion, CBrushRegion::RegionPtr pOriginalRegion, bool bCreateBridgeEdges );

private:

	CBrushRegion::RegionPtr QueryOffsetRegion( CViewport *view, CPoint point ) const;
	void AddScaledRegion();
	BrushFloat ApplyScaleToSelectedRegion( CBrushRegion::RegionPtr pRegion, BrushFloat fScale );

	CBrushRegion::RegionPtr m_pOffsetedRegion;
	CBrushRegion::RegionPtr m_pSelectedRegion;
	SLButtonInfo m_LButtonInfo;
	CPoint m_PrevPos;
	BrushFloat m_fScale;
	static BrushFloat m_fPrevScale;

};
