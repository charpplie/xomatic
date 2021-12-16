#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2014 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerMagnetTool.h
//  Created:     May/5/2014 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#include "BrushDesignerSelectTool.h"

class CBrushDesignerMagnetTool : public CBrushDesignerSelectTool
{
public:

	void Enter() override;
	void Leave() override;

	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnMouseMove( CViewport *view,UINT nFlags,CPoint point ) override;
	bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags ) override;

	void Display( DisplayContext &dc ) override;

private:

	enum EMagnetToolPhase
	{
		eMTP_ChooseFirstPoint,
		eMTP_ChooseUpPoint,
		eMTP_ChooseMoveToTargetPoint,
	};	

	struct SSourceVertex
	{
		SSourceVertex( const BrushVec3& v ) : position(v), color(BUtil::kElementBoxColor){}
		SSourceVertex( const BrushVec3& v, const ColorB c ) : position(v), color(c){}
		BrushVec3 position;
		ColorB color;
	};

	void PrepareChooseFirstPointStep();
	void InitializeSelectedRegionBeforeTransform();
	void AddVertexToList( const BrushVec3& vertex, ColorB color, std::vector<SSourceVertex>& vertices );
	void SwitchSides();
	void AlignSelectedRegion();

	EMagnetToolPhase m_Phase;
	std::vector<SSourceVertex> m_SourceVertices;
	int m_nSelectedSourceVertex;
	int m_nSelectedUpVertex;
	CBrushRegion::RegionPtr m_pInitRegion;
	BrushVec3 m_TargetPos;
	BrushVec3 m_PickedPos;
	BrushVec3 m_vTargetUpDir;
	bool m_bPickedTargetPos;
	bool m_bSwitchedSides;
};