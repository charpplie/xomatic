/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id: IrradianceVolumeRenderNode.cpp,v 1.0 2008/05/19 12:14:13 AntonKaplanyan Exp wwwrun $
$DateTime$
Description:  Routine for rendering and managing of irradiance volumetric data
-------------------------------------------------------------------------
History:
- 12:6:2008   12:14 : Created by Anton Kaplanyan
*************************************************************************/

#include "StdAfx.h"
#include "IrradianceVolumeRenderNode.h"

CIrradianceVolumeRenderNode::IrradianceVolumeSet CIrradianceVolumeRenderNode::ms_volumes;

void CIrradianceVolumeRenderNode::RegisterIrradianceVolume(CIrradianceVolumeRenderNode* p)
{
	IrradianceVolumeSet::iterator it(ms_volumes.find(p));
	assert(it == ms_volumes.end() && "CIrradianceVolumeRenderNode::RegisterIrradianceVolume() -- Object already registered!");
	if (it == ms_volumes.end())
	{
		ms_volumes.insert(p);
	}
}

void CIrradianceVolumeRenderNode::UnregisterIrradianceVolume(CIrradianceVolumeRenderNode* p)
{
	IrradianceVolumeSet::iterator it(ms_volumes.find(p));
	assert(it != ms_volumes.end() && "CIrradianceVolumeRenderNode::UnregisterIrradianceVolume() -- Object not registered or previously removed!");
	if (it != ms_volumes.end())
	{
		ms_volumes.erase(it);
	}
}


CIrradianceVolumeRenderNode::CIrradianceVolumeRenderNode()
:	m_WSBBox()
, m_pos(0, 0, 0)
, m_size(0, 0, 0)
, m_pMaterial(0)
, m_pRE(0)
, m_pRO(0)
, m_nGridWidth(0)
, m_nGridHeight(0)
, m_nGridDepth(0)
, m_nNumIterations(0)
, m_gridDimensions(0, 0, 0, 0)
, m_gridCellSize(0, 0, 0)
, m_maxGridCellSize(0.f)
, m_bIsValid(false)
{
	m_settings.m_mat.SetIdentity();
	m_settings.m_fDensity = 1.f;
	m_matInv.SetIdentity();
	m_WSBBox.Reset();

	m_pRE = (CREIrradianceVolume*) GetRenderer()->EF_CreateRE(eDATA_IrradianceVolume);
	if(m_pRE)
		m_pRE->m_pParent = this;
	else
		 assert(0);
	m_pRO = GetRenderer()->EF_GetObject(false);

	m_pMaterial = GetMatMan()->LoadMaterial("Materials/IrradianceVolume/Default", false);
	assert(m_pMaterial!=0);

	RegisterIrradianceVolume(this);
}


CIrradianceVolumeRenderNode::~CIrradianceVolumeRenderNode()
{
	UnregisterIrradianceVolume(this);

	if (m_pRE)
		m_pRE->Release(false);
	//if (m_pRO)
	//	GetRenderer()->EF_ObjRemovePermanent(m_pRO);

	Get3DEngine()->UnRegisterEntity(reinterpret_cast<IIrradianceVolumeRenderNode*>(this));
	Get3DEngine()->FreeRenderNodeState(reinterpret_cast<IIrradianceVolumeRenderNode*>(this));
}

void CIrradianceVolumeRenderNode::SetMatrix(const Matrix34& mat)
{
	Get3DEngine()->UnRegisterEntity(this);
	UpdateMetrics(mat);
	Get3DEngine()->RegisterEntity(this);
}

EERType CIrradianceVolumeRenderNode::GetRenderNodeType()
{
	return eERType_IrradianceVolume;
}


const char* CIrradianceVolumeRenderNode::GetEntityClassName() const
{
	return "IrradianceVolume";
}


const char* CIrradianceVolumeRenderNode::GetName() const
{
	return "IrradianceVolume";
}

Vec3 CIrradianceVolumeRenderNode::GetPos(bool bWorldOnly) const
{
	return m_pos;
}

void CIrradianceVolumeRenderNode::Render(const SRendParams& rParam)
{
	FUNCTION_PROFILER_3DENGINE;

	if(m_pRO)
	{
		CRenderObject* pObj(GetRenderer()->EF_GetObject(false, m_pRO->m_Id));
		if (pObj && m_pRE && m_pMaterial)
		{
			assert(m_bIsValid);
			SShaderItem shaderItem(m_pMaterial->GetShaderItem(0));
			m_pRO->m_ObjFlags |= FOB_TRANS_MASK;
			GetRenderer()->EF_AddEf(m_pRE, shaderItem, m_pRO, EFSLIST_DEFERRED_PREPROCESS, 0);
		}
	}
}


IPhysicalEntity* CIrradianceVolumeRenderNode::GetPhysics() const	
{
	return 0;
}

void CIrradianceVolumeRenderNode::SetPhysics(IPhysicalEntity*)
{
}

void CIrradianceVolumeRenderNode::SetMaterial(IMaterial* pMat)
{
	m_pMaterial = pMat;
}


IMaterial* CIrradianceVolumeRenderNode::GetMaterial(Vec3* pHitPos)
{
	return m_pMaterial;
}

float CIrradianceVolumeRenderNode::GetMaxViewDist()
{
	if (GetMinSpecFromRenderNodeFlags(m_dwRndFlags) == CONFIG_DETAIL_SPEC)
		return max(GetCVars()->e_ViewDistMin, GetBBox().GetRadius() * GetCVars()->e_ViewDistRatioDetail * GetViewDistRatioNormilized());
	return max(GetCVars()->e_ViewDistMin, GetBBox().GetRadius() * GetCVars()->e_ViewDistRatio * GetViewDistRatioNormilized());
}

void CIrradianceVolumeRenderNode::Precache()
{
}

void CIrradianceVolumeRenderNode::GetMemoryUsage(ICrySizer* pSizer)
{
	SIZER_COMPONENT_NAME(pSizer, "IrradianceVolumeRenderNode");
	pSizer->AddObject(this, sizeof(*this));
}

void CIrradianceVolumeRenderNode::UpdateMetrics(const Matrix34& mx, const bool recursive/* = false*/)
{
	m_bIsValid = true;

	Matrix34 mat = mx.GetInverted();
	// shift x,y,z to [0;1] interval
	mat.m03 += .5f;
	mat.m13 += .5f;
	mat.m23 += .5f;

	SIVSettings newSettings = m_settings;
	newSettings.m_mat = mat;

	Matrix34 matInv = mat.GetInverted();
	const Vec3 size = Vec3(matInv.GetColumn0().GetLength(), matInv.GetColumn1().GetLength(), matInv.GetColumn2().GetLength());
	// calc grid dimensions
	assert(m_settings.m_fDensity >= .1 && m_settings.m_fDensity <= 10.f);
	Vec3 vcMinTexels = size * m_settings.m_fDensity;

	if(!recursive && !ValidateConditions(newSettings))
	{
		vcMinTexels.CheckMax(Vec3(3, 3, 3));
		vcMinTexels.CheckMin(Vec3(32, 32, 32));
		vcMinTexels /= m_settings.m_fDensity;
		Vec3 oldPos = matInv.TransformPoint(Vec3(.5f, .5f, .5f));
		matInv.SetColumn(0, matInv.GetColumn0().GetNormalized() * vcMinTexels.x);
		matInv.SetColumn(1, matInv.GetColumn1().GetNormalized() * vcMinTexels.y);
		matInv.SetColumn(2, matInv.GetColumn2().GetNormalized() * vcMinTexels.z);
		matInv.SetTranslation(oldPos);
		UpdateMetrics(matInv, true);
		return;
	}

	m_settings.m_mat = mat;
	m_matInv = matInv;
	m_WSBBox.SetTransformedAABB(m_matInv, AABB(Vec3(0, 0, 0), Vec3(1, 1, 1)));
	m_pos = m_matInv.TransformPoint(Vec3(.5f, .5f, .5f));

	m_nGridWidth = (int)vcMinTexels.x;
	m_nGridHeight = (int)vcMinTexels.y;
	m_nGridDepth = (int)vcMinTexels.z;

	// initialize iterations number
	m_nNumIterations = min(GetCVars()->e_GIIterations, max(m_nGridWidth, max(m_nGridHeight, m_nGridDepth)));

	// calc dimensions
	m_gridDimensions = Vec4((float)m_nGridWidth, (float)m_nGridHeight, (float)m_nGridDepth, 0.f);

	// single grid cell size
	m_gridCellSize = Vec3(m_size.x / m_gridDimensions.x, m_size.y / m_gridDimensions.y, m_size.z / m_gridDimensions.z);
	m_maxGridCellSize = max(m_gridCellSize.x, max(m_gridCellSize.y, m_gridCellSize.z));

	if(m_pRE)
	{
		CREIrradianceVolume::Settings* pFillSettings = m_pRE->GetFillSettings();
		pFillSettings->m_pos = m_pos;
		pFillSettings->m_bbox = m_WSBBox;
		pFillSettings->m_nGridWidth = m_nGridWidth;
		pFillSettings->m_nGridHeight = m_nGridHeight;
		pFillSettings->m_nGridDepth = m_nGridDepth;
		pFillSettings->m_nNumIterations = m_nNumIterations;
		pFillSettings->m_mat = m_settings.m_mat;
		pFillSettings->m_matInv = m_matInv;
		pFillSettings->m_gridDimensions = m_gridDimensions;			// grid size
		pFillSettings->m_invGridDimensions = Vec4(1.f / m_nGridWidth, 1.f / m_nGridHeight, 1.f / m_nGridDepth, 0);

		m_pRE->Invalidate();	// invalidate 
	}
}

bool CIrradianceVolumeRenderNode::TryInsertLight( const CDLight &light )
{
	// TODO: optimize
	static ICVar* isEnabled = GetConsole()->GetCVar("r_IrradianceVolumes");
	assert(isEnabled);
	if(!isEnabled || !isEnabled->GetIVal())
		return false;
	assert(light.m_Flags&DLF_IRRAD_VOLUMES);

	static const AABB bbIdentity(Vec3(0, 0, 0), Vec3(1, 1, 1));
	const Vec3 vRadius = Vec3(light.m_fRadius, light.m_fRadius, light.m_fRadius);
	AABB lightBox(light.m_Origin - vRadius, light.m_Origin + vRadius);
	lightBox.SetTransformedAABB(m_settings.m_mat, lightBox);
	if(light.m_fRadius > 4.f * m_maxGridCellSize && // check if the light is big enough for the grid precision
		bbIdentity.IsIntersectBox(lightBox))
	{
		assert(m_pRE);
		if(m_pRE && (m_pRE->GetFlags() & CREIrradianceVolume::efGIVolume) == 0)
		{
			m_pRE->InsertLight(light);
			return true;
		}
	}
	return false;
}

bool CIrradianceVolumeRenderNode::TryInsertLightIntoVolumes( const CDLight &light )
{
	// find for suitable existing grid
	bool bRes = false;
	IrradianceVolumeSet::const_iterator itEnd = ms_volumes.end();
	for(IrradianceVolumeSet::iterator it = ms_volumes.begin();it != itEnd;++it)
		bRes |= (*it)->TryInsertLight(light);

	return bRes;
}

void CIrradianceVolumeRenderNode::GetMatrix(Matrix34& mxGrid) const
{
	mxGrid = m_settings.m_mat;
	mxGrid.m03 -= .5f;
	mxGrid.m13 -= .5f;
	mxGrid.m23 -= .5f;
	mxGrid.Invert();
}

void CIrradianceVolumeRenderNode::SetDensity( const float fDensity )
{
	SIVSettings newSettings = m_settings;
	newSettings.m_fDensity = fDensity;
	if(ValidateConditions(newSettings))
	{
		assert(m_settings.m_fDensity >= .1 && m_settings.m_fDensity <= 30.f);
		m_settings.m_fDensity = fDensity;
		m_bIsValid = false;
	}
}

bool CIrradianceVolumeRenderNode::AutoFit( const TArray<CDLight>& lightsToFit )
{
	if(lightsToFit.empty())
		return false;

	float minLightRadius = lightsToFit[0].m_fRadius;

	Vec3 _size(lightsToFit[0].m_fRadius, lightsToFit[0].m_fRadius, lightsToFit[0].m_fRadius);
	AABB bound(lightsToFit[0].m_Origin - _size, lightsToFit[0].m_Origin + _size);
	for(uint32 i=1;i<lightsToFit.size();++i)
	{
		Vec3 size(lightsToFit[i].m_fRadius, lightsToFit[i].m_fRadius, lightsToFit[i].m_fRadius);
		bound.Expand(lightsToFit[i].m_Origin - size);
		bound.Expand(lightsToFit[i].m_Origin + size);
		minLightRadius = min(minLightRadius, lightsToFit[i].m_fRadius);
	}

	if(minLightRadius <= 0.f)
	{
		gEnv->pLog->LogWarning("Irradiance volume fit: One light in the group has not positive radius!");
		return false;
	}

	// check if we have enough density to size to these dimensions
	Vec3 gridSize = bound.GetSize() + Vec3(.01f, .01f, .01f); // scale a bit 
	Vec3i gridSizeInCells = Vec3i(int(gridSize.x / m_settings.m_fDensity), int(gridSize.y / m_settings.m_fDensity), int(gridSize.z / m_settings.m_fDensity));
	if(gridSizeInCells.x < 2 || gridSizeInCells.y < 2 || gridSizeInCells.z < 2
		|| gridSizeInCells.x > nMaxGridSize || gridSizeInCells.y > nMaxGridSize || gridSizeInCells.z > nMaxGridSize)
	{
		gEnv->pLog->LogWarning("Irradiance volume fit: try either to compact lights or to decrease the lights' radii or to increase the grid's density");
		return false;
	}

	// adjust density
	float recomendedDensity = minLightRadius / 16;
	if(m_settings.m_fDensity > recomendedDensity)
		SetDensity(recomendedDensity);

	Matrix34 mxNew;
	mxNew = Matrix33::CreateScale(gridSize);
	mxNew.SetTranslation(bound.GetCenter());
	SetMatrix(mxNew);

	return true;
}

void CIrradianceVolumeRenderNode::EnableSpecular( const bool bEnabled )
{
	if(m_pRE)
		m_pRE->EnableSpecular(bEnabled);
}

bool CIrradianceVolumeRenderNode::IsSpecularEnabled() const
{
	if(m_pRE)
		return m_pRE->m_bHasSpecular;
	return false;
}

bool CIrradianceVolumeRenderNode::ValidateConditions( const SIVSettings& settings )
{
	// ignore checks for GI volumes
	if(m_pRE && m_pRE->GetFlags() & CREIrradianceVolume::efGIVolume)
		return true;

	const Matrix34 matInv = settings.m_mat.GetInverted();
	const Vec3 size = Vec3(matInv.GetColumn0().GetLength(), matInv.GetColumn1().GetLength(), matInv.GetColumn2().GetLength());
	// calc grid dimensions
	assert(settings.m_fDensity >= .1 && settings.m_fDensity <= 30.f);
	const Vec3 vcMinTexels = size * settings.m_fDensity;
	if((int)vcMinTexels.x > nMaxGridSize)
		return false;
	if((int)vcMinTexels.x < 2)
		return false;
	if((int)vcMinTexels.y > nMaxGridSize)
		return false;
	if((int)vcMinTexels.y < 2)
		return false;
	if((int)vcMinTexels.z > nMaxGridSize)
		return false;
	if((int)vcMinTexels.z < 2)
		return false;
	return true;
}