#pragma once
///////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushPrimitive.h
//  Created:     5/5/2010 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#include "Tools/BrushDesignerDrawLineTool.h"
#include "BrushArgument.h"
#include "BaseBrushCreator.h"
#include "BrushRegion.h"

class CBrushPrimitive 
{

public:

	CBrushPrimitive( CBaseBrushCreator* pBrush = NULL) :
			m_pBrushCreator(pBrush)
	{
	}

	void CreateBox( const BrushVec3 &mins, const BrushVec3& maxs, std::vector<CBrushRegion::RegionPtr>* pOutRegionList = NULL ) const;
	void CreateSphere( const BrushVec3& mins, const BrushVec3& maxs, int numSides, std::vector<CBrushRegion::RegionPtr>* pOutRegionList = NULL ) const;
	void CreateSphere( const BrushVec3& vCenter, float radius, int numSides, std::vector<CBrushRegion::RegionPtr>* pOutRegionList = NULL ) const;
	void CreateCylinder( const BrushVec3& mins, const BrushVec3& maxs, int numSides, std::vector<CBrushRegion::RegionPtr>* pOutRegionList = NULL ) const;
	void CreateCylinder( CBrushRegion::RegionPtr pBaseDiscRegion, float fHeight, std::vector<CBrushRegion::RegionPtr>* pOutRegionList = NULL ) const;
	void CreateCone( const BrushVec3& mins, const BrushVec3& maxs, int numSides, std::vector<CBrushRegion::RegionPtr>* pOutRegionList = NULL ) const;
	void CreateCone( CBrushRegion::RegionPtr pBaseDiscRegion, float fHeight, std::vector<CBrushRegion::RegionPtr>* pOutRegionList = NULL ) const;
	void CreateRectangle( const BrushVec3& mins, const BrushVec3& maxs, std::vector<CBrushRegion::RegionPtr>* pOutRegionList = NULL ) const;
	void CreateDisc( const BrushVec3& mins, const BrushVec3& maxs, int numSides, std::vector<CBrushRegion::RegionPtr>* pOutRegionList = NULL ) const;

private:

	void CreateCircle( const BrushVec3& mins, const BrushVec3& maxs, int numSides, std::vector<BrushVec3>& outVertexList ) const;

	_smart_ptr<CBaseBrushCreator> m_pBrushCreator;

};

class CSolidCustomPrimitive : public CBrushDesignerDrawLineTool
{
public:
	CSolidCustomPrimitive( CBaseObject* pBaseObject, CBaseBrushCreator* pBrushCreator )
		: CBrushDesignerDrawLineTool()
	{
		m_State = eSCS_DrawBase;
		m_pBrushCreator = pBrushCreator;
		m_pObject = pBaseObject;
	}
	~CSolidCustomPrimitive(){}

	void OnLButtonDown( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnLButtonUp( CViewport *view,UINT nFlags,CPoint point ) override;
	void OnMouseMove( CViewport *view,UINT nFlags,CPoint point ) override;
	bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags ) override;
	void Display( DisplayContext &dc )override;
	BrushMatrix34 GetWorldTM() const override
	{
		static BrushMatrix34 identityTM = BrushMatrix34::CreateIdentity();
		return identityTM;
	}
	MouseCreateResult GetMouseEvent() const{
		MouseCreateResult result;
		if( m_State == eSCS_DrawBase || m_State == eSCS_RaiseHeight )
			result = MOUSECREATE_CONTINUE;
		else if( m_State == eSCS_End )
			result = MOUSECREATE_OK;
		else if( m_State == eSCE_Cancel )
			result = MOUSECREATE_ABORT;
		else
			assert(0);
		return result;
	}

private:

	void ApplyInitialPosToBrush( const CPoint& point );
	bool GetHitPosition( const CPoint& point, BrushVec3& outPos );

	void UpdateBaseFace( CViewport *view,UINT nFlags,CPoint point );
	void RaiseHeight( CViewport *view,UINT nFlags,CPoint point );
	void CreateRegionFromSpots( bool bClosedRegion, const SpotList& spotList );

	enum EStateCreateSolid
	{
		eSCS_DrawBase,
		eSCS_RaiseHeight,
		eSCE_Cancel,
		eSCS_End
	};

private:

	CBaseBrushCreator* GetBrushCreator() const;

	mutable _smart_ptr<CBaseBrushCreator> m_pBrushCreator;

	CBrushArgument::BrushArgumentPtr m_ArgumentBrush;
	CPoint m_mouseDownPos;	
	EStateCreateSolid m_State;
	CBaseObject* m_pObject;
};