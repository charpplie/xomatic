/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description:	Implementation of the ILevel interface. 
					MiniLevel provides a basic implementation
  
 -------------------------------------------------------------------------
  History:
  - 25:2:2010	18:30 : Created by Christian Helmich

*************************************************************************/

#include "StdAfx.h"
#include "MiniLevel.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// ctor & dtor

simpleframework::CMiniLevel::CMiniLevel(simpleframework::CMiniLevelInfo& levelInfo)
{
	LOG_CODE_COVERAGE();
	memcpy(&m_levelInfo, &levelInfo, sizeof(CMiniLevelInfo));
}

//-----------------------------------

simpleframework::CMiniLevel::~CMiniLevel()
{
	LOG_CODE_COVERAGE();
}

//-----------------------------------

simpleframework::CMiniLevel* simpleframework::CMiniLevel::Create(simpleframework::CMiniLevelInfo& levelInfo)
{
	LOG_CODE_COVERAGE();

	//CMiniLevel is basically just a holder of CMiniLevelInfo,
	//so no need to fragment memory by deleting/recreating
// 	static char	sMemory[sizeof(CMiniLevel)];
// 	return new (sMemory) CMiniLevel(levelInfo);
	return new CMiniLevel(levelInfo);
}

//-----------------------------------

void simpleframework::CMiniLevel::Destroy(simpleframework::CMiniLevel* pLevel)
{
	LOG_CODE_COVERAGE();
	CRY_ASSERT(pLevel);
	CRY_SAFE_RETURN(!pLevel, CRY_NO_RETURN_VALUE);
	SAFE_DELETE(pLevel);
	//pLevel->~CMiniLevel();
}

//-----------------------------------

void simpleframework::CMiniLevel::Recreate(simpleframework::CMiniLevel** ppLevel, simpleframework::CMiniLevelInfo& levelInfo)
{
	if(*ppLevel)
		Destroy(*ppLevel);
	*ppLevel = Create(levelInfo);
	CRY_ASSERT(*ppLevel);
}

// ctor & dtor/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// ILevel implementation

void simpleframework::CMiniLevel::Release()
{
	LOG_CODE_COVERAGE();
	Destroy(this);
}

//-----------------------------------

ILevelInfo* simpleframework::CMiniLevel::GetLevelInfo()
{
	LOG_CODE_COVERAGE();
	return &m_levelInfo;
}

// ILevel implementation/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include UNIQUE_VIRTUAL_WRAPPER(ILevel)

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
