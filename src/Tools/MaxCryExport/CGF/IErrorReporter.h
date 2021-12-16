//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IERRORREPORTER_H__
#define __IERRORREPORTER_H__

class IErrorReporter
{
public:
	enum ErrorLevel
	{
		Info,
		Warning,
		Error
	};
	virtual void Report(ErrorLevel level, const std::string& sMessage) = 0;
};

#endif //__IERRORREPORTER_H__
