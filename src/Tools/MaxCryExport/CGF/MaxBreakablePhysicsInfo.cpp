//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MaxBreakablePhysicsInfo.h"

MaxBreakablePhysicsInfo::MaxBreakablePhysicsInfo(int nGranularity)
:	nGranularity(nGranularity)
{
}

MaxBreakablePhysicsInfo::~MaxBreakablePhysicsInfo()
{
}

int MaxBreakablePhysicsInfo::GetGranularity()
{
	return this->nGranularity;
}
