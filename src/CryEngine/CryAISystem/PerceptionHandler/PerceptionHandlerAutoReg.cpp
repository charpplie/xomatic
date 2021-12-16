/*************************************************************************
  StalTech Source File.
  Copyright (C), StalTech Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Helper to register perception handlers to a factory
  
 -------------------------------------------------------------------------
  History:
  - 04:30:2009: Created by Kevin Kirst

*************************************************************************/

#include "StdAfx.h"
#include "PerceptionHandlerAutoReg.h"
#include "IPerceptionHandler.h"
#include "Puppet.h"

CPerceptionHandlerAutoRegBase* CPerceptionHandlerAutoRegBase::m_Registery[ePHT_COUNT] = {0};

//////////////////////////////////////////////////////////////////////////
CPerceptionHandlerAutoRegBase::CPerceptionHandlerAutoRegBase(int iTypeId) : m_iTypeId(ePHT_INVALID)
{
	const bool bValid = iTypeId > ePHT_INVALID && iTypeId < ePHT_COUNT;
	CRY_ASSERT(bValid);
	if (bValid)
	{
		m_iTypeId = iTypeId;
		m_Registery[iTypeId] = this;
	}
}

//////////////////////////////////////////////////////////////////////////
IPerceptionHandler* CPerceptionHandlerAutoRegBase::Create(int iTypeId, CWeakRef<CPuppet> refOwner)
{
	const bool bValid = iTypeId > ePHT_INVALID && iTypeId < ePHT_COUNT;
	CRY_ASSERT(bValid);
	CRY_ASSERT(refOwner.IsValid());

	IPerceptionHandler *pResult = NULL;

	if (!bValid || !m_Registery[iTypeId])
	{
		CryWarning(VALIDATOR_MODULE_AI, VALIDATOR_WARNING, "Attempting to create a Perception Handler for an unknown type \'%d\' (%s)", iTypeId, refOwner.GetAIObject()->GetName());
	}
	else
	{
		pResult = m_Registery[iTypeId]->OnCreate(refOwner);
	}

	return pResult;
}
