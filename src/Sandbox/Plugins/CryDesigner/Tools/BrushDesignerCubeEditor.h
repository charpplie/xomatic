#pragma once

#include "BrushDesignerBaseTool.h"
#include "Core/BrushDesignerSpotManager.h"
#include "IDataBaseManager.h"

class CBrushDesignerCubeEditor : public CBrushDesignerBaseTool, public IDataBaseManagerListener
{
public:

	void Enter() override;
	void Leave() override;

	void BeginEditParams() override;
	void EndEditParams() override;

	void OnLButtonDown( CViewport *view, UINT nFlags, CPoint point ) override;
	void OnLButtonUp( CViewport *view, UINT nFlags, CPoint point ) override;
	void OnMouseMove( CViewport *view, UINT nFlags, CPoint point ) override;
	void OnMouseWheel( CViewport *view,UINT nFlags,CPoint point ) override;

	void Display( DisplayContext &dc ) override;

	void MaterialChanged() override;
	void SetSubMatID( int nSubMatID, CBrushDesigner* pDesigner ) override;

	void OnDataBaseItemEvent( IDataBaseItem *pItem,EDataBaseItemEvent event ) override;

	enum EEditorMode
	{
		eEditorMode_Add,
		eEditorMode_Remove,
		eEditorMode_Paint,
		eEditorMode_Invalid
	};

private:

	void DisplayBrush( DisplayContext& dc );
	EEditorMode GetEditMode() const;
	std::vector<CBrushRegion::RegionPtr> GetBrushRegions( const AABB& aabb ) const;
	void AddCube( const AABB& brushAABB );
	void RemoveCube( const AABB& brushAABB );
	void PaintCube( const AABB& brushAABB );
	AABB GetBrushBox( CViewport *view, CPoint point );
	AABB GetBrushBox( const BrushVec3& vSnappedPos, const BrushVec3& vPickedPos, const BrushVec3& vNormal );

	bool GetBrushPos( CViewport *view, CPoint point, BrushVec3& outSnappedPos, BrushVec3& outPickedPos, BrushVec3* pOutNormal );	
	BrushVec3 Snap( const BrushVec3& vPos ) const;

	void AddBrush( const AABB& aabb );

	AABB m_BrushAABB;
	std::vector<AABB> m_BrushAABBs;
	CPoint m_CurMousePos;

	struct SDrawStraight
	{
		SDrawStraight() : 
			m_bPressingShift(false),
			m_StraightDir(0,0,0),
			m_StartingPos(0,0,0),
			m_StartingNormal(0,0,0)
		{
		}
		BrushVec3 m_StraightDir;
		BrushVec3 m_StartingPos;
		BrushVec3 m_StartingNormal;
		bool m_bPressingShift;
	};
	SDrawStraight m_DS;
};
