#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushArgument.h
//  Created:     1/Sep/2011 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushRegion.h"
#include "BaseBrush.h"
#include "BrushDesigner.h"

class CBrushDesignerDB;

class CBrushArgument : public CRefCountBase
{
public:

	CBrushArgument( 
		CBrushRegion* pRegion,
		CBaseObject* pObject,
		std::vector<CBrushRegion::RegionPtr>* perpendicularRegions,
		CBrushDesignerDB* pDB );
	virtual ~CBrushArgument();

	enum EBrushArgumentUpdate
	{
		eBAU_None = 0,
		eBAU_UpdateBrush = 1,
	};

	virtual void Update(EBrushArgumentUpdate updateOp);

	void SetHeight( BrushFloat fHeight );
	BrushFloat GetHeight() const { return m_fHeight; }
	const BrushPlane& GetCurrentCapPlane() { return m_CapPlane; }
	const BrushPlane& GetBasePlane() { return m_BasePlane; }
	CBrushRegion::RegionPtr GetCapRegion() const;
	bool GetSideRegionList( std::vector<CBrushRegion::RegionPtr>& outRegions ) const;
	bool GetRegionList( std::vector<CBrushRegion::RegionPtr>& outRegions ) const;

	AABB GetBoundBox() const;

protected:

	virtual void AddCapRegions();
	void AddSideRegions();

	void UpdateRegionVertices2Plane( const BrushPlane& targetPlane );
	void UpdateBrush();
	bool FindPlaneWithEdge( const BrushEdge3D& edge, const BrushPlane& hintPlane, BrushPlane& outPlane );

protected:

	_smart_ptr<CBaseObject> m_pBaseObject;
	_smart_ptr<CBaseBrush> m_pBrush;
	_smart_ptr<CBrushDesigner> m_pDesigner;

	CBrushRegion::RegionPtr m_pRegion;
	CBrushRegion::RegionPtr m_pInitialRegion;
	CBrushRegion::RegionPtr m_pRegionWithoutInnerRegions;

	CBrushRegion::RegionPtr m_pInitialOutsideRegionWithoutBridgeEdges;
	std::vector<CBrushRegion::RegionPtr> m_InitialInsideRegionsWithoutBrideEdges;	

	f64 m_fHeight;
	BrushPlane m_BasePlane;
	BrushPlane m_CapPlane;

	typedef std::pair<BrushEdge3D,BrushPlane> EdgeBrushPlanePair;
	std::vector<EdgeBrushPlanePair> m_EdgePlanePairs;

	CBrushDesignerDB* m_pDB;

public:
	typedef _smart_ptr<CBrushArgument> BrushArgumentPtr;
};