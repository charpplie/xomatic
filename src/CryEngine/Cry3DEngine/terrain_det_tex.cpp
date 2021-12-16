////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terrain_det_tex.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: terrain detail textures
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "terrain.h"
#include "terrain_sector.h"
#include "3dEngine.h"

void CTerrain::SetDetailLayerProperties(int nId, float fScaleX, float fScaleY, 
																				uint8 ucProjAxis, const char * szSurfName, 
																				const PodArray<int> & lstnVegetationGroups, IMaterial * pMat, 
                                        int nSID)
{
	if(nId>=0 && nId<MAX_SURFACE_TYPES_COUNT)
	{
		strncpy(m_SSurfaceType[nSID][nId].szName, szSurfName, sizeof(m_SSurfaceType[nSID][nId].szName));
		m_SSurfaceType[nSID][nId].fScale = fScaleX;
		m_SSurfaceType[nSID][nId].ucDefProjAxis = ucProjAxis;
		m_SSurfaceType[nSID][nId].ucThisSurfaceTypeId = nId;
		m_SSurfaceType[nSID][nId].lstnVegetationGroups.Reset();
		m_SSurfaceType[nSID][nId].lstnVegetationGroups.AddList(lstnVegetationGroups);
		m_SSurfaceType[nSID][nId].pLayerMat = pMat;
		if(m_SSurfaceType[nSID][nId].pLayerMat && !m_bEditor)
			CTerrain::Get3DEngine()->PrintMessage("  Layer %d - %s has material %s", nId, szSurfName, pMat->GetName());
	}
	else
	{
		Warning("CTerrain::SetDetailTextures: LayerId is out of range: %d: %s", nId, szSurfName);
		assert(!"CTerrain::SetDetailTextures: LayerId is out of range");
	}
}

void CTerrain::LoadSurfaceTypesFromXML(XmlNodeRef pDetTexTagList, int nSID)
{
  LOADING_TIME_PROFILE_SECTION(GetSystem());

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, 0, "Surface types");

	CTerrain::Get3DEngine()->PrintMessage("Loading terrain layers for segment %d ...", nSID);

 	if (!pDetTexTagList)
		return;

	for (int nId = 0; nId < pDetTexTagList->getChildCount() && nId < MAX_SURFACE_TYPES_COUNT; nId++)
	{
		XmlNodeRef pDetLayer = pDetTexTagList->getChild(nId);
		IMaterialManager* pMatMan = Get3DEngine()->GetMaterialManager();
		const char * pMatName = pDetLayer->getAttr("DetailMaterial");
		IMaterial * pMat = (pMatName[0] && !(GetCVars()->e_VoxTer && !m_bEditor)) ? pMatMan->LoadMaterial(pMatName) : NULL;

		float fScaleX=1.f; pDetLayer->getAttr("DetailScaleX",fScaleX);
		float fScaleY=1.f; pDetLayer->getAttr("DetailScaleY",fScaleY);
		uint8 projAxis = pDetLayer->getAttr("ProjAxis")[0];

		if(!pMat || pMat == pMatMan->GetDefaultMaterial())
		{
      if(!GetCVars()->e_VoxTer)
			  Error("CTerrain::LoadSurfaceTypesFromXML: Error loading material: %s", pMatName);
			pMat = pMatMan->GetDefaultTerrainLayerMaterial();
		}

		if(pMat)
		{
			pMat->m_fDefautMappingScale = fScaleX;
			Get3DEngine()->InitMaterialDefautMappingAxis(pMat);
		}

		PodArray<int> lstnVegetationGroups;
		for (int nGroup = 0; nGroup < pDetLayer->getChildCount(); nGroup++)
		{
			XmlNodeRef pVegetationGroup = pDetLayer->getChild(nGroup);
			int nVegetationGroupId=-1;
			if(pVegetationGroup->isTag("VegetationGroup") && pVegetationGroup->getAttr("Id",nVegetationGroupId))
				lstnVegetationGroups.Add(nVegetationGroupId);
		}

		SetDetailLayerProperties(nId, fScaleX, fScaleY, projAxis, pDetLayer->getAttr("Name"), lstnVegetationGroups, pMat, nSID);
	}
}

void CTerrain::UpdateSurfaceTypes(int nSID)
{
	if(GetParentNode(nSID))
		GetParentNode(nSID)->UpdateDetailLayersInfo(true);
}
