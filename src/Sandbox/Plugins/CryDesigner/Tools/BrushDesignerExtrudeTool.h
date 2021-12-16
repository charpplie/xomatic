#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerExtrudeTool.h
//  Created:     Sep/1/2012 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#include "BrushDesignerBaseTool.h"
#include "Core/BrushArgumentEx.h"
#include "Core/BrushDesigner.h"

class CBrushDesignerExtrudeTool : public CBrushDesignerBaseTool
{
public:

	enum EResizeStatus
	{
		eRS_None,
		eRS_Resizing
	};

	CBrushDesignerExtrudeTool()
	{
		m_ResizeStatus = eRS_None;
		m_pScaledRegion = new CBrushRegion;
	}
	virtual ~CBrushDesignerExtrudeTool();

	void Enter() override;
	void Leave() override;

	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnLButtonUp( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnLButtonDblClk( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnMouseMove( CViewport *view,UINT nFlags,CPoint point ) override;

	void Display( DisplayContext &dc ) override;
	void OnEditorNotifyEvent( EEditorNotifyEvent event ) override;
	
	static void Extrude( BUtil::SMainContext& mc, CBrushRegion::RegionPtr pRegion, float fHeight, float fScale );

protected:	

	void RaiseHeight( const CPoint& point, CViewport* view, int nFlags );
	bool AlignHeight( CBrushRegion::RegionPtr pCapRegion, CViewport *view, const CPoint& point );

	bool StartPushPull(CViewport *view,UINT nFlags,CPoint point);

	struct SExtrusionContext
	{
		SExtrusionContext() : 
			pObject(NULL), 
			pBrush(NULL), 
			pDesigner(NULL), 
			pRegion(NULL), 
			pArgumentBrush(NULL), 
			bFirstUpdate(false), 
			bTouchedMirrorPlane(false), 
			pushPull(BUtil::ePP_None),
			initPushPull(BUtil::ePP_None),
			fScale(0),
			bUpdateBrush(true)
		{
		}

		CBaseObject* pObject;
		CBaseBrush* pBrush;
		CBrushDesigner* pDesigner;
		CBrushRegion::RegionPtr pRegion;
		BUtil::EPushPull pushPull;
		BUtil::EPushPull initPushPull;
		bool bUpdateBrush;		

		CBrushArgument::BrushArgumentPtr pArgumentBrush;
		bool bFirstUpdate;
		bool bTouchedMirrorPlane;
		bool bIsLocatedAtOpposite;
		BrushFloat fScale;
		std::vector<CBrushRegion::RegionPtr> backupRegions;
		std::vector<CBrushRegion::RegionPtr> mirroredBackupRegions;
	};
	static bool PrepareExtrusion( SExtrusionContext& ec );
	static void MakeArgumentBrush( SExtrusionContext& ec );
	static void FinishPushPull( SExtrusionContext& ec );
	static void CheckBoundary( SExtrusionContext& ec );
	static void UpdateDesigner( SExtrusionContext& ec );

	void RaiseLowerRegion( CViewport *view, UINT nFlags, CPoint point );
	void ResizeRegion( CViewport *view, UINT nFlags, CPoint point );
	void SelectRegion( CViewport *view, UINT nFlags, CPoint point );

protected:

	SExtrusionContext m_ec;

	struct SActionInfo
	{
		SActionInfo() : m_Type(BUtil::ePP_None), m_Distance(0)	{}
		BUtil::EPushPull m_Type;
		BrushFloat m_Distance;
	};	
	SActionInfo m_PrevAction;

	CBrushRegion::RegionPtr m_pScaledRegion;
	EResizeStatus m_ResizeStatus;
	BrushVec2 m_ResizeStartScreenPos;
	SLButtonInfo m_LButtonInfo;
};
