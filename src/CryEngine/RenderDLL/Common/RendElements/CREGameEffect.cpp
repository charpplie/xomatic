//==================================================================================================
// Name: CREGameEffect
// Desc: Render element that uses the IREGameEffect interface for its functionality
// Author: James Chilvers
//==================================================================================================

// Includes
#include "StdAfx.h"
#include "CREGameEffect.h"

//--------------------------------------------------------------------------------------------------
// Name: CREGameEffect
// Desc: Constructor
//--------------------------------------------------------------------------------------------------
CREGameEffect::CREGameEffect()
{
	m_pImpl = NULL;
	mfSetType(eDATA_GameEffect);
	mfUpdateFlags( FCEF_TRANSFORM );
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: ~CREGameEffect
// Desc: Destructor
//--------------------------------------------------------------------------------------------------
CREGameEffect::~CREGameEffect()
{
	SAFE_DELETE(m_pImpl);
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: mfPrepare
// Desc: Prepares rendering
//--------------------------------------------------------------------------------------------------
void CREGameEffect::mfPrepare()
{
	gRenDev->FX_CheckOverflow(0,0,this);
	gRenDev->m_RP.m_pRE							= this;
	gRenDev->m_RP.m_RendNumIndices	= 0;
	gRenDev->m_RP.m_RendNumVerts		= 0;

	if(m_pImpl)
	{
		m_pImpl->mfPrepare();
	}
}//-------------------------------------------------------------------------------------------------
 
//--------------------------------------------------------------------------------------------------
// Name: mfDistanceToCameraSquared
// Desc: Returns distance to camera squared
//--------------------------------------------------------------------------------------------------
float CREGameEffect::mfDistanceToCameraSquared(Matrix34& matInst)
{
	if(m_pImpl)
	{
		return m_pImpl->mfDistanceToCameraSquared(matInst);
	}
	return 0.0f;
}//-------------------------------------------------------------------------------------------------
