//==================================================================================================
// Name: CREScreenFilter
// Desc: Screen filter render element
// Author: James Chilvers
//==================================================================================================

// Includes
#include "StdAfx.h"
#include "Effects/GameEffectsSystem.h"
#include "Effects/RenderNodes/ScreenFilterRenderNode.h"
#include "CREScreenFilter.h"
#include "IEntityRenderState.h"
#include "I3DEngine.h"

//--------------------------------------------------------------------------------------------------
// Name: CREScreenFilter
// Desc: Constructor
//--------------------------------------------------------------------------------------------------
CREScreenFilter::CREScreenFilter()
{
	m_reGameEffect = NULL;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: ~CREScreenFilter
// Desc: Destructor
//--------------------------------------------------------------------------------------------------
CREScreenFilter::~CREScreenFilter()
{

}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Initialise
// Desc: Initializes render element
//--------------------------------------------------------------------------------------------------
void CREScreenFilter::Initialise(CREGameEffect* reGameEffect)
{
	m_reGameEffect = reGameEffect;
	if(m_reGameEffect)
	{
		m_reGameEffect->mfUpdateFlags(FCEF_TRANSFORM);
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: mfPrepare
// Desc: Prepares render element for rendering
//--------------------------------------------------------------------------------------------------
void CREScreenFilter::mfPrepare()
{

}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: mfDistanceToCameraSquared
// Desc: Returns distance to camera squared
//--------------------------------------------------------------------------------------------------
float CREScreenFilter::mfDistanceToCameraSquared(Matrix34& matInst)
{
	return (gEnv->p3DEngine->GetCurrentCamera().GetPosition() - m_params.centre).GetLengthSquared();
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: mfDraw
// Desc: Draws Screen Filter
//--------------------------------------------------------------------------------------------------
bool CREScreenFilter::mfDraw(CShader* ef, SShaderPass* sfm)
{
	const int VERT_COUNT = 4;
	SVF_P3F_C4B_T2F verts[VERT_COUNT] = 
	{
		{Vec3(-m_params.halfWidth, m_params.distanceFromCamera, -m_params.halfHeight), {{0}}, Vec2(0.0f, 1.0f)},
		{Vec3(m_params.halfWidth, m_params.distanceFromCamera, -m_params.halfHeight), {{0}}, Vec2(1.0f, 1.0f)},
		{Vec3(-m_params.halfWidth, m_params.distanceFromCamera, m_params.halfHeight), {{0}}, Vec2(0.0f, 0.0f)},
		{Vec3(m_params.halfWidth, m_params.distanceFromCamera, m_params.halfHeight), {{0}}, Vec2(1.0f, 0.0f)}
	};

	CVertexBuffer vertexBuffer(verts,eVF_P3F_C4B_T2F);
	gEnv->pRenderer->DrawPrimitives(&vertexBuffer,VERT_COUNT);

	return true;
}//-------------------------------------------------------------------------------------------------
