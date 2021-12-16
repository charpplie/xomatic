////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   NetworkDataSource.cpp
//  Version:     v1.00
//  Created:     10/05/11 by Steve Humphreys
//  Description: Load data across the network (eg from a database server)
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "NetworkDataSource.h"

#include "TelemetryRepository.h"

namespace Telemetry
{

CNetworkDataSource::CNetworkDataSource(CTelemetryRepository& repo)
	: m_repository(repo)
{
}

CNetworkDataSource::~CNetworkDataSource()
{
}

//////////////////////////////////////////////////////////////////////////

bool CNetworkDataSource::Open()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CNetworkDataSource::Update()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CNetworkDataSource::Close()
{

}

}