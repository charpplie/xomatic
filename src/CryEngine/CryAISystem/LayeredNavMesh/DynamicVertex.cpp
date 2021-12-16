
#include "StdAfx.h"

#include "DynamicVertex.h"

#include "IAIDebugRenderer.h"


#ifdef DEBUG_DRAW
void StaticVtxData::DebugDraw (const ::Vec3 & pos) const
{
	IAIDebugRenderer* pRenderer = gEnv->pAISystem->GetAIDebugRenderer();
	pRenderer->Draw3dLabelEx (pos, 2.5f, ColorB(255, 255, 255), false, true, "static:%d", m_vtxIndex);
}

void SShapeVtxData::DebugDraw (const ::Vec3 & pos) const
{
	IAIDebugRenderer* pRenderer = gEnv->pAISystem->GetAIDebugRenderer();
	pRenderer->Draw3dLabelEx (pos, 2.5f, ColorB(255, 255, 255), false, true,
		"SShape:%p[%d]", m_id, m_vtxNumber);
}

void CrossVtxData::DebugDraw (const ::Vec3 & pos) const
{
	const Vec3 spacing (0.0f, 0.0f, -0.35f);
	m_static0.DebugDraw (pos + 0*spacing);
	m_static1.DebugDraw (pos + 1*spacing);
	m_sshape0.DebugDraw (pos + 2*spacing);
	m_sshape1.DebugDraw (pos + 3*spacing);
}

void ObstacleCrossVtxData::DebugDraw (const ::Vec3 & pos) const
{
	const Vec3 spacing (0.0f, 0.0f, -0.35f);
	m_sshape00.DebugDraw (pos + 0*spacing);
	m_sshape01.DebugDraw (pos + 1*spacing);
	m_sshape10.DebugDraw (pos + 2*spacing);
	m_sshape11.DebugDraw (pos + 3*spacing);
}

void VtxAttribs::Attrib::DebugDraw (const ::Vec3 & pos) const
{
	switch (m_type)
	{
	case USER_DATA_NAV_POLY:
		(reinterpret_cast <const StaticVtxData*> (this))->DebugDraw (pos);
		break;
	case USER_DATA_OBSTACLE_SSHAPE:
		(reinterpret_cast <const SShapeVtxData*> (this))->DebugDraw (pos);
		break;
	case USER_DATA_CROSS_VTX:
		(reinterpret_cast <const CrossVtxData*> (this))->DebugDraw (pos);
		break;
	default:
		assert (0 && "unknown nav mesh vertex attrib type");
		break;
	}
}
#endif // DEBUG_DRAW

