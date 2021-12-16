#include "StdAfx.h"


#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)

#include "PrismRenderNode.h"

CPrismRenderNode::CPrismRenderNode() :m_pMaterial(0)
{
	m_mat.SetIdentity();
	m_WSBBox = AABB(Vec3(-1, -1, -1), Vec3(1, 1, 1));
	m_pRE = (CREPrismObject*) GetRenderer()->EF_CreateRE(eDATA_PrismObject);
//	m_pMaterial = GetMatMan()->LoadMaterial("Materials/VolumeData/Default", false);
	m_dwRndFlags |= ERF_CASTSHADOWMAPS;
}

CPrismRenderNode::~CPrismRenderNode()
{
	if (m_pRE)
		m_pRE->Release(false);

	Get3DEngine()->UnRegisterEntity(this);
	Get3DEngine()->FreeRenderNodeState(this);
}

void CPrismRenderNode::SetMatrix(const Matrix34& mat)
{
	m_mat = mat;
	m_WSBBox.SetTransformedAABB(mat, AABB(Vec3(-1, -1, -1), Vec3(1, 1, 1)));
	Get3DEngine()->RegisterEntity(this);
}

EERType CPrismRenderNode::GetRenderNodeType()
{
	return eERType_PrismObject;
}

const char* CPrismRenderNode::GetName() const
{
	return "PrismObject";
}

Vec3 CPrismRenderNode::GetPos(bool bWorldOnly) const
{
	return m_mat.GetTranslation();
}

void CPrismRenderNode::Render(const SRendParams& rParam)
{
  FUNCTION_PROFILER_3DENGINE;

	if(!m_pMaterial)
		return;

	CRenderObject *pRO = GetRenderer()->EF_GetObject(true);		// pointer could be cached

	// set basic render object properties
	pRO->m_II.m_Matrix = m_mat;
	pRO->m_ObjFlags |= FOB_TRANS_MASK;
	pRO->m_fSort = 0;
	pRO->m_fDistance = rParam.fDistance;

	// transform camera into object space
	const CCamera& cam(GetCamera());
	Vec3 viewerPosWS(cam.GetPosition());

	m_pRE->m_center = m_mat.GetTranslation();
 
	if(pRO)
	{
		SShaderItem shaderItem(m_pMaterial->GetShaderItem(0));

//		if (rParam.nTechniqueID > 0)
	//		shaderItem.m_nTechnique = shaderItem.m_pShader->GetTechniqueID(shaderItem.m_nTechnique, rParam.nTechniqueID);

//		int afterWater(GetObjManager()->IsAfterWater(m_mat.GetTranslation(), GetCamera().GetPosition(), Get3DEngine()->GetWaterLevel()) ? 1 : 0);
//		GetRenderer()->EF_AddEf(m_pRE, shaderItem, pRO, EFSLIST_TRANSP, afterWater);
		GetRenderer()->EF_AddEf(m_pRE, shaderItem, pRO, EFSLIST_GENERAL, 0);
	}
}

float CPrismRenderNode::GetMaxViewDist()
{
	return 1000.0f;
}

void CPrismRenderNode::GetMemoryUsage(ICrySizer* pSizer)
{
	SIZER_COMPONENT_NAME(pSizer, "PrismRenderNode");
	pSizer->AddObject(this, sizeof(*this));
}

#endif // EXCLUDE_DOCUMENTATION_PURPOSE
