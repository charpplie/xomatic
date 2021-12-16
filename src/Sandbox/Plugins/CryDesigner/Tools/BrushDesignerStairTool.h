#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerStairTool.h
//  Created:     April/25/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerDrawTool.h"
#include "Core/BrushDesignerAdjustHeightHelper.h"

class CBrushDesignerStairTool : public CBrushDesignerDrawTool
{

public:

	CBrushDesignerStairTool();
	~CBrushDesignerStairTool();

	void Enter() override;
	void Leave() override;

	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnLButtonUp( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnMouseMove( CViewport *view,UINT nFlags,CPoint point ) override;

	bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags ) override;
	void Display( DisplayContext &dc ) override;
	void OnEditorNotifyEvent( EEditorNotifyEvent event ) override;

	void BeginEditParams() override;
	void EndEditParams() override;	

	bool IsPhaseFirstStepOnPrimitiveCreation() const override { return m_StairMode == eStairMode_PlaceFirstPoint; }

	void UpdateStair();
	void UpdateStair( BrushFloat fWidth, BrushFloat fHeight, BrushFloat fDepth );

	struct SOutputParameterForStairCreation
	{
		SOutputParameterForStairCreation() : pCapRegion(NULL)
		{
		}
		std::vector<CBrushRegion::RegionPtr> regions;
		std::vector<CBrushRegion::RegionPtr> regionsNeedPostProcess;
		CBrushRegion::RegionPtr pCapRegion;
	};

	static void CreateStair( 
		const BrushVec3& vStartPos, 
		const BrushVec3& vEndPos, 
		BrushFloat fBoxWidth, 
		BrushFloat fBoxDepth, 
		BrushFloat fBoxHeight, 
		const BrushPlane& floorPlane, 
		float fStepRise, 
		bool bXDirection, 
		bool bMirrored, 
		bool bRotationBy90Degree,
		CBrushRegion::RegionPtr pBaseRegion,
		SOutputParameterForStairCreation& out );

private:

	enum EStairMode
	{
		eStairMode_PlaceFirstPoint,
		eStairMode_CreateRectangle,
		eStairMode_CreateBox,
		eStairMode_Done
	};

	EStairMode m_StairMode;

	void PlaceFirstPoint( CViewport *view, UINT nFlags, CPoint point );
	void CreateRectangle( CViewport *view, UINT nFlags, CPoint point );
	void CreateBox( CViewport *view, UINT nFlags, CPoint point );

	void GetRectangleVertices( BrushVec3& outV0, BrushVec3& outV1, BrushVec3& outV2, BrushVec3& outV3 );
	static CBrushRegion::RegionPtr CreateRegion( const std::vector<BrushVec3>& vList, bool bFlip, CBrushRegion::RegionPtr pBaseRegion );
	void AcceptUndo();
	void FreezeDesigner() override;

	BrushVec3 m_BottomVertices[4];
	BrushVec3 m_TopVertices[4];
	BrushFloat m_fBoxHeight;
	BrushFloat m_fBoxWidth;
	BrushFloat m_fBoxDepth;
	bool m_bXDirection;
	BrushPlane m_FloorPlane;
	CBrushRegion::RegionPtr m_pCapRegion;
	bool m_bIsOverOpposite;

	BrushVec3 m_vStartPos;
	BrushVec3 m_vEndPos;

	std::vector<CBrushRegion::RegionPtr> m_RegionsNeedPostProcess;
	_smart_ptr<CBrushDesigner> m_pUndoDesigner;
};