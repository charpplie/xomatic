#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerTextureMappingTool.h
//  Created:     May/6/2012 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushDesignerSelectTool.h"

class CBrushDesigner;
class CBrushDesignerTextureMappingToolPanel;

class CBrushDesignerTextureMappingTool : public CBrushDesignerSelectTool
{
public:

	CBrushDesignerTextureMappingTool();
	~CBrushDesignerTextureMappingTool();

	void Enter() override;
	void Leave() override;

	void BeginEditParams() override;
	void EndEditParams() override;	

	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnLButtonUp( CViewport *view,UINT nFlags,CPoint point ) override;
	bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags ) override;
	void OnEditorNotifyEvent( EEditorNotifyEvent event ) override;

	void ApplyTextureInfo( const BUtil::STexInfo& texInfo, bool bAdd );
	void FitTexture( float fTileU, float fTileV );	
	void SetSubMatID( int nSubMatID, CBrushDesigner* pDesigner ) override;

	void SelectRegionsByMatID( int matID );
	bool GetTexInfoOfSelectedRegion( BUtil::STexInfo& outTexInfo ) const;	

	void OnManipulatorDrag( CViewport *pView,ITransformManipulator *pManipulator,CPoint &p0,CPoint &p1,const BrushVec3 &value ) override;
	void OnManipulatorMouseEvent( CViewport *pView, ITransformManipulator *pManipulator, EMouseEvent event, CPoint &point, int flags, bool bHitGizmo ) override;

	bool IsCircleTypeRotateGizmo() override { return true; }
	void RecordTextureMappingUndo( const char *sUndoDescription ) const;

	static void AssignMatID( int matID );

public:

	static void CloseTextureMappingToolPanel();

	static int m_nDesignerTextureMappingToolPanelId;
	static CBrushDesignerTextureMappingToolPanel* m_pDesignerTextureMappingToolPanel;

private:

	bool IsPicking() const;
	bool QueryRegion( const BrushVec3& localRaySrc, const BrushVec3& localRayDir, int& nOutRegionIndex, bool& bOutNew ) const;
	void SetTexInfoToRegion( CBrushRegion::RegionPtr pRegion, const BUtil::STexInfo& texInfo );
	void MoveSelectedElements();

private:

	struct STextureContext
	{
		void Init()
		{
			m_TexInfos.clear();
		}

		BrushVec3 m_MouseDownPos;
		std::vector< std::pair<CBrushRegion::RegionPtr,BUtil::STexInfo> > m_TexInfos;
	};
	STextureContext m_MouseDownContext;
};