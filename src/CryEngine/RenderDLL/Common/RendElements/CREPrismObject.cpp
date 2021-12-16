#include "StdAfx.h"

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)

#include "CREPrismObject.h"
#include "../../../CryCommon/IEntityRenderState.h"

CREPrismObject::CREPrismObject()
	:CRendElementBase(), m_center(0, 0, 0)
{
	mfSetType(eDATA_PrismObject);
	mfUpdateFlags(FCEF_TRANSFORM);
}

void CREPrismObject::mfPrepare()
{
	gRenDev->FX_CheckOverflow(0, 0, this);
	gRenDev->m_RP.m_pRE = this;
	gRenDev->m_RP.m_RendNumIndices = 0;
	gRenDev->m_RP.m_RendNumVerts = 0;
}

float CREPrismObject::mfDistanceToCameraSquared(Matrix34& matInst)
{
	return (gRenDev->GetRCamera().Orig - m_center).GetLengthSquared();
}


#endif // EXCLUDE_DOCUMENTATION_PURPOSE
