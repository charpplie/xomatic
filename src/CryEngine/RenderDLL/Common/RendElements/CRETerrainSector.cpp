#include "StdAfx.h"
#include "RendElement.h"
#include "CRETerrainSector.h"
#include "I3DEngine.h"

void CRECommon::mfPrepare()
{
  gRenDev->FX_CheckOverflow(0, 0, this);

  gRenDev->m_RP.m_pRE = this;
  gRenDev->m_RP.m_RendNumIndices = 0;
  gRenDev->m_RP.m_RendNumVerts = 0;
}

void CREFarTreeSprites::mfPrepare()
{
  gRenDev->FX_CheckOverflow(0, 0, this);

  gRenDev->m_RP.m_pRE = this;
  gRenDev->m_RP.m_RendNumIndices = 0;
  gRenDev->m_RP.m_RendNumVerts = 0;
}

bool CREFarTreeSprites::mfDraw(CShader *ef, SShaderPass *sfm)
{    
  gRenDev->DrawObjSprites(m_arrVegetationSprites[gRenDev->m_RP.m_nProcessThreadID][SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID]-1]);
  return true;
}

bool CRETerrainDetailTextureLayers::mfDraw(CShader *ef, SShaderPass *sfm)
{    
  return true;
}

bool CRETerrainParticles::mfDraw(CShader *ef, SShaderPass *sfm)
{    
  return true;
}

