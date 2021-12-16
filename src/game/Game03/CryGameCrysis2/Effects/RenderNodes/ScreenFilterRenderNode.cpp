//==================================================================================================
// Name: CScreenFilterRenderNode
// Desc: Screen filter render node
// Author: James Chilvers
//==================================================================================================

// Includes
#include "StdAfx.h"
#include "ScreenFilterRenderNode.h"
#include "Effects/RenderElements/CREScreenFilter.h"
#include <IRenderAuxGeom.h>

//--------------------------------------------------------------------------------------------------
// Name: CScreenFilterRenderNode
// Desc: Constructor
//--------------------------------------------------------------------------------------------------
CScreenFilterRenderNode::CScreenFilterRenderNode()
{
	m_matrix.SetIdentity();
	m_WSBBox.Reset();
	m_maxViewDist = 10000.0f;
	m_material = NULL;

	m_screenFilterRenderElement = new CREScreenFilter;
	m_gameEffectRenderElement	= (CREGameEffect*)gEnv->pRenderer->EF_CreateRE(eDATA_GameEffect);
	m_gameEffectRenderElement->SetPrivateImplementation(m_screenFilterRenderElement);
	m_screenFilterRenderElement->Initialise(m_gameEffectRenderElement);
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: ~CScreenFilterRenderNode
// Desc: Destructor
//--------------------------------------------------------------------------------------------------
CScreenFilterRenderNode::~CScreenFilterRenderNode()
{
	if(m_material)
	{
		m_material->Release();
	}

	if(m_gameEffectRenderElement)
		m_gameEffectRenderElement->Release(false);

	gEnv->p3DEngine->UnRegisterEntity(this);
	gEnv->p3DEngine->FreeRenderNodeState(this);
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetParams
// Desc: Sets parameters
//--------------------------------------------------------------------------------------------------
void CScreenFilterRenderNode::SetParams(const SScreenFilterRenderNodeParams& params)
{
	UpdatePosFromCamera();

	SetMaterial(params.material);
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: UpdatePosFromCamera
// Desc: Updates position from camera
//--------------------------------------------------------------------------------------------------
void CScreenFilterRenderNode::UpdatePosFromCamera()
{
	// Set world space bounding box
	const float radius = 2.0f;
	m_WSBBox.min.Set(-radius,-radius,-radius);
	m_WSBBox.max.Set(radius,radius,radius);

	Matrix34 mat;
	mat.SetIdentity();
	mat.SetTranslation(gEnv->pRenderer->GetCamera().GetPosition());
	SetMatrix(mat);
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: GetName
// Desc: Gets the node's name
//--------------------------------------------------------------------------------------------------
const char* CScreenFilterRenderNode::GetName() const
{
	return "ScreenFilter";
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: GetRenderNodeType
// Desc: Gets the type of the render node
//--------------------------------------------------------------------------------------------------
EERType CScreenFilterRenderNode::GetRenderNodeType()
{
	return eERType_GameEffect;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: GetEntityClassName
// Desc: Gets the entity's class name
//--------------------------------------------------------------------------------------------------
const char* CScreenFilterRenderNode::GetEntityClassName() const 
{ 
	return "ScreenFilter"; 
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: GetMemoryUsage
// Desc: Gets the node's memory usage
//--------------------------------------------------------------------------------------------------

//Diesel cut
//void CScreenFilterRenderNode::GetMemoryUsage(ICrySizer* sizer) const

//Diesel new
//void CScreenFilterRenderNode::GetMemoryStatistics(ICrySizer* s)
//{
	//SIZER_COMPONENT_NAME(s,"ScreenFilterRenderNode");
	//SIZER_SUBCOMPONENT_NAME(s, "ScreenFilterRenderNode");
	//s->AddObject(this,sizeof(*this));
	//s->Add(*this);
//}
//Diesel new
//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetMaterial
// Desc: Sets the node's material
//--------------------------------------------------------------------------------------------------
void CScreenFilterRenderNode::SetMaterial(IMaterial* material) 
{ 
	if (material == m_material)
		return;

	if(m_material)
	{
		m_material->Release();
	}

	m_material = material;

	if(m_material)
	{
		m_material->AddRef();
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: GetMaterial
// Desc: Gets the node's material
//--------------------------------------------------------------------------------------------------
IMaterial* CScreenFilterRenderNode::GetMaterial(Vec3* hitPos) 
{ 
	return m_material;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetMatrix
// Desc: Sets the node's matrix and updates the bbox
//--------------------------------------------------------------------------------------------------
void CScreenFilterRenderNode::SetMatrix(const Matrix34& matrix)
{
	gEnv->p3DEngine->UnRegisterEntity(this);
	m_matrix = matrix;
	m_WSBBox.Move(-m_WSBBox.GetCenter());
	m_WSBBox.SetTransformedAABB(matrix,m_WSBBox);
	gEnv->p3DEngine->RegisterEntity(this);
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: GetPos
// Desc: Gets the node's position
//--------------------------------------------------------------------------------------------------
Vec3 CScreenFilterRenderNode::GetPos(bool worldOnly) const
{
	return m_matrix.GetTranslation();
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: GetBBox
// Desc: Gets the node's bbox
//--------------------------------------------------------------------------------------------------
const AABB CScreenFilterRenderNode::GetBBox() const 
{ 
	return m_WSBBox; 
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetBBox
// Desc: Sets the node's bbox
//--------------------------------------------------------------------------------------------------
void CScreenFilterRenderNode::SetBBox(const AABB& worldSpaceBoundingBox) 
{ 
	m_WSBBox = worldSpaceBoundingBox;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: GetMaxViewDist
// Desc: Gets the node's maximum view distance
//--------------------------------------------------------------------------------------------------
float CScreenFilterRenderNode::GetMaxViewDist()
{
	return m_maxViewDist;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: GetPhysics
// Desc: Gets the node's physics
//--------------------------------------------------------------------------------------------------
IPhysicalEntity* CScreenFilterRenderNode::GetPhysics() const 
{ 
	return NULL; 
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetPhysics
// Desc: Sets the node's physics
//--------------------------------------------------------------------------------------------------
void CScreenFilterRenderNode::SetPhysics(IPhysicalEntity* physics) 
{

}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: GetMaterialOverride
// Desc: Returns the material override
//--------------------------------------------------------------------------------------------------
IMaterial* CScreenFilterRenderNode::GetMaterialOverride()
{
	return m_material;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Render
// Desc: Renders the node
//--------------------------------------------------------------------------------------------------
void CScreenFilterRenderNode::Render(const SRendParams& renderParams)
{
#if DEBUG_GAME_FX_SYSTEM
	switch(GAME_FX_SYSTEM.GetDebugView())
	{
		case eGAME_FX_DEBUG_VIEW_BoundingBox:
		{
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(m_WSBBox,false,Col_White,eBBD_Faceted);
			break;
		}
		case eGAME_FX_DEBUG_VIEW_BoundingSphere:
		{
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(GetPos(),m_maxViewDist,Col_White);
			break;
		}
	}
#endif

	if(m_material && m_screenFilterRenderElement)
	{
		// Create temp render object to submit this node to the renderer
		CRenderObject *pRenderObject = gEnv->pRenderer->EF_GetObject(true);

		if(pRenderObject)
		{
			const CCamera& camera = gEnv->pRenderer->GetCamera();

			// Set render element parameters
			SScreenFilterRenderElementParams* reParams = m_screenFilterRenderElement->GetParams();

			// Calculate size of quad in-front of camera
			if((reParams->fovH != camera.GetHorizontalFov()) || (reParams->fovV != camera.GetFov()))
			{
				reParams->fovH = camera.GetHorizontalFov();
				reParams->fovV = camera.GetFov();
				reParams->distanceFromCamera = camera.GetNearPlane() + 0.001f; // Have to be slightly behind near plane, otherwise sorting issues

				reParams->halfWidth = tan(reParams->fovH * 0.5f)*(reParams->distanceFromCamera);
				reParams->halfHeight = tan(reParams->fovV * 0.5f)*(reParams->distanceFromCamera);
			}

			reParams->centre = camera.GetPosition();

			// Set render object properties
			pRenderObject->m_II.m_Matrix  = camera.GetMatrix();
			pRenderObject->m_ObjFlags			|= FOB_TRANS_MASK;
			pRenderObject->m_fSort				= 0;
			pRenderObject->m_fDistance		= renderParams.fDistance;

			// Add render element and render object to render list
			gEnv->pRenderer->EF_AddEf(m_gameEffectRenderElement, m_material->GetShaderItem(), pRenderObject, EFSLIST_GENERAL, 0);
		}
	}
}//-------------------------------------------------------------------------------------------------




#include "Nodes/G2FlowBaseNode.h"



class CFlowNode_ScreenFilter : public CFlowBaseNode
{
private:
	enum EScreenFilterInput
	{
		ESFI_Enable,
		ESFI_Disable,
		ESFI_Material,
	};

public:
	CFlowNode_ScreenFilter( SActivationInfo * pActInfo )
		:	m_pCurrentMaterial(0)
	{
		m_screenEffect = GAME_FX_SYSTEM.CreateRenderNode<CScreenFilterRenderNode>();
		m_screenEffect->SetRndFlags(ERF_HIDDEN, true);
	};

	virtual ~CFlowNode_ScreenFilter()
	{
		GAME_FX_SYSTEM.DeleteRenderNode((IRenderNode**)&m_screenEffect);
	}

//Diesel cut
	//virtual void GetMemoryUsage(ICrySizer * s) const
	//{
	//	s->Add(*this);
	//}

//Diesel new
	virtual void GetMemoryStatistics(ICrySizer * s) 
	{
		s->Add(*this);
	}

	virtual void GetConfiguration(SFlowNodeConfig &config)
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void("enable",	"Enables the effect"),
			InputPortConfig_Void("disable",	"Disables the effect"),
			InputPortConfig<string>("material",	"Material name used in the screen filter."),
			{0}
		};

		static const SOutputPortConfig out_config[] = {
			{0}
		};

		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
	{
		switch (event)
		{
			case eFE_Initialize:
				m_screenEffect->SetRndFlags(ERF_HIDDEN, true);
				break;
			case eFE_Activate:
				OnActivate(pActInfo);
				break;
			case eFE_Update:
				m_screenEffect->UpdatePosFromCamera();
				break;
		}
	}

private:

	void OnActivate(SActivationInfo* pActInfo)
	{
		if (IsPortActive(pActInfo, ESFI_Enable))
		{
			m_screenEffect->SetRndFlags(ERF_HIDDEN, false);
			pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
			SetMaterial(pActInfo);
		}
		else if (IsPortActive(pActInfo, ESFI_Disable))
		{
			m_screenEffect->SetRndFlags(ERF_HIDDEN, true);
			pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
		}
		else if (IsPortActive(pActInfo, ESFI_Material))
		{
			SetMaterial(pActInfo);
		}
	}


	void SetMaterial(SActivationInfo *pActInfo)
	{
		if (m_pCurrentMaterial)
			m_pCurrentMaterial->Release();

		const string& material = GetPortString(pActInfo, ESFI_Material);
		m_pCurrentMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(material.c_str(), false);
		if (!m_pCurrentMaterial)
			return;
		m_screenEffect->SetMaterial(m_pCurrentMaterial);
		m_pCurrentMaterial->AddRef();
	}


	CScreenFilterRenderNode* m_screenEffect;
	IMaterial* m_pCurrentMaterial;
};


REGISTER_FLOW_NODE("MaterialFX:ScreenFilter", CFlowNode_ScreenFilter);
