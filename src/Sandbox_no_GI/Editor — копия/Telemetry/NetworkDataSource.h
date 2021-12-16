////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   NetworkDataSource.h
//  Version:     v1.00
//  Created:     10/05/11 by Steve Humphreys
//  Description: Loads data across the network (eg from a database server)
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __NETWORKDATASOURCE_H__
#define __NETWORKDATASOURCE_H__

#include "IDataSource.h"

namespace Telemetry
{

class CTelemetryRepository;

class CNetworkDataSource : public IDataSource
{
public:
	CNetworkDataSource(CTelemetryRepository& rep);
	virtual ~CNetworkDataSource();

	virtual bool Open();
	virtual bool Update();
	virtual void Close();

private:

	CTelemetryRepository& m_repository;
};

}

#endif //__NETWORKDATASOURCE_H__