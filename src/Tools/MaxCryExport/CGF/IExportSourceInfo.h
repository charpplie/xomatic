//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IEXPORTSOURCEINFO_H__
#define __IEXPORTSOURCEINFO_H__

#include <ctime>
#include <string>

class IExportSourceInfo
{
public:
	virtual ~IExportSourceInfo() {}
	virtual std::time_t GetExportTime() = 0;
	virtual std::string GetFileName() = 0;
};

#endif //__IEXPORTSOURCEINFO_H__
