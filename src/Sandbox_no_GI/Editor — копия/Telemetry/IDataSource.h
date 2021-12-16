////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   IDataSource.h
//  Version:     v1.00
//  Created:     10/05/11 by Steve Humphreys
//  Description: Base class for handling incoming telemetry data
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __IDATASOURCE_H__
#define __IDATASOURCE_H__

namespace Telemetry
{

struct IDataSource
{
	virtual ~IDataSource() {};

	// select and open the data source (file / DB connection / etc)
	virtual bool Open() = 0;

	// process incoming data. Return true when complete
	virtual bool Update() = 0;

	// close the data source
	virtual void Close() = 0;
};

}

#endif