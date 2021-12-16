#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushRegionMesh.h
//  Created:     April/16/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#include "BrushRegion.h"

class CBrushRegionMesh : public CRefCountBase
{

public:

	CBrushRegionMesh();
	~CBrushRegionMesh();

	void SetRegion( CBrushRegion::RegionPtr pRegion, bool bForce, const Matrix34& worldTM = Matrix34::CreateIdentity(), int dwRndFlags = 0, int nViewDistRatio = 100, int nMinSpec = 0, uint8 materialLayerMask = 0 );
	void SetRegions( const std::vector<CBrushRegion::RegionPtr>& regionList, bool bForce, const Matrix34& worldTM = Matrix34::CreateIdentity(), int dwRndFlags = 0, int nViewDistRatio = 100, int nMinSpec = 0, uint8 materialLayerMask = 0 );
	void SetWorldTM( const Matrix34& worldTM );
	void SetMaterialName( const CString& name );
	void ReleaseResources();

private:

	void ApplyMaterial();
	void UpdateStatObjAndRenderNode( const BUtil::SMeshInfo& mesh, const Matrix34& worldTM, int dwRndFlags, int nViewDistRatio, int nMinSpec, uint8 materialLayerMask );
	void ReleaseRenderNode();
	void CreateRenderNode();

	std::vector<CBrushRegion::RegionPtr> m_pRegions;
	IStatObj* m_pStatObj;
	IRenderNode* m_pRenderNode;
	CString m_MaterialName;

};