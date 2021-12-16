#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerStairProfileTool.h
//  Created:     May/29/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerDrawTool.h"

class CBrushDesignerStairProfileTool : public CBrushDesignerDrawTool
{

public:

	CBrushDesignerStairProfileTool();
	~CBrushDesignerStairProfileTool();

	void Enter() override;
	void Leave() override;

	void BeginEditParams() override;
	void EndEditParams() override;

	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnMouseMove( CViewport *view,UINT nFlags,CPoint point ) override;

	bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags ) override;
	void Display( DisplayContext &dc ) override;

protected:

	void CreateCandidates();
	void DrawCandidateStair( DisplayContext &dc, int nIndex, const ColorB& color );	

	enum ESideStairMode
	{
		eSideStairMode_PlaceFirstPoint,
		eSideStairMode_DrawDiagonal,
		eSideStairMode_SelectDirection,
	};

	ESideStairMode m_SideStairMode;
	std::vector<SSpot> m_CandidateStairs[2];
	BrushLine m_BorderLine;
	int m_nSelectedCandidate;
	SSpot m_LastSpot;
	int m_nSelectedRegionIndex;
	bool m_bOnDesignerObject;

};