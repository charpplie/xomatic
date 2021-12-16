////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   FileDataSource.h
//  Version:     v1.00
//  Created:     10/05/11 by Steve Humphreys
//  Description: Loads new-format data from a file
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __FILEDATASOURCE_H__
#define __FILEDATASOURCE_H__

#include "IDataSource.h"

namespace Telemetry
{
	class CTelemetryRepository;

	class CFileDataSource : public IDataSource
	{
	public:
		CFileDataSource(CTelemetryRepository& rep);
		virtual ~CFileDataSource();

		virtual bool Open();
		virtual bool Update();
		virtual void Close();

	private:
		bool OpenFile(char* filename, unsigned int bufSize, const char* filter, const char* title);
		void Connect();

		CTelemetryRepository& m_repository;
	};

}

#endif //__FILEDATASOURCE_H__