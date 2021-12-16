/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id: ICrySourceControl.h,v 1.0 2009/01/09 14:49:23 SergeySokov Exp wwwrun $
$DateTime$
Description:  interface for source control system
-------------------------------------------------------------------------
History:
- 09:01:2008 17:49 Created by Sergey Sokov
*************************************************************************/

#pragma once

#ifndef __ICRYSOURCECONTROL_H__
#define __ICRYSOURCECONTROL_H__


#include <time.h>


class CSourceControlFileInfo
{
public:
	CSourceControlFileInfo()
	{
		Reset();
	}

	~CSourceControlFileInfo()
	{
	}

	void Reset()
	{
		m_clientFileName[0] = 0;
		m_depotFileName[0] = 0;
	}

	static const int MAX_CLIENT_FILE_NAME_LENGTH = 1023;
	static const int MAX_DEPOT_FILE_NAME_LENGTH = 1023;

	char m_clientFileName[MAX_CLIENT_FILE_NAME_LENGTH+1];
	char m_depotFileName[MAX_DEPOT_FILE_NAME_LENGTH+1];
};


class CSourceControlUserInfo
{
public:
	CSourceControlUserInfo()
	{
		Reset();
	}

	~CSourceControlUserInfo()
	{
	}

	void Reset()
	{
		m_userName[0] = 0;
		m_userFullName[0] = 0;
		m_clientName[0] = 0;
		m_email[0] = 0;
	}

	static const int MAX_USER_NAME_LENGTH = 127;
	static const int MAX_USER_FULL_NAME_LENGTH = 127;
	static const int MAX_CLIENT_NAME_LENGTH = 127;
	static const int MAX_EMAIL_LENGTH = 127;

	char m_userName[MAX_USER_NAME_LENGTH+1];
	char m_userFullName[MAX_USER_FULL_NAME_LENGTH+1];
	char m_clientName[MAX_CLIENT_NAME_LENGTH+1];
	char m_email[MAX_EMAIL_LENGTH+1];
};


class CSourceControlTimeInfo
{
public:
	CSourceControlTimeInfo()
	{
		Reset();
	}

	~CSourceControlTimeInfo()
	{
	}

	void Reset()
	{
		m_time = 0;
	}

	// Returned string is in ctime_s() format (for example: "Fri Apr 25 13:51:23 2003\n")
	void GetAsStr(char* buffer, size_t bufferSizeInBytes)
	{
		if((buffer==0) || (bufferSizeInBytes<=0))
		{
			return;
		}
		time_t t = (time_t)m_time;
		if(ctime_s(buffer, bufferSizeInBytes, &t) != 0)
		{
			buffer[0] = 0;
		}
	}

	int m_time;
};


class ICrySourceControl
{
public:
	virtual ~ICrySourceControl() {}

	virtual bool GetLastCheckOutInfo(
		const char* fileName, 
		const char* clientName, 
		CSourceControlFileInfo& fileInfo, 
		CSourceControlUserInfo& userInfo, 
		CSourceControlTimeInfo& timeInfo) = 0;
};

#endif // __ICRYSOURCECONTROL_H__
