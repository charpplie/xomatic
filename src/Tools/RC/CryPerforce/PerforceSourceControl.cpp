////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   PerforceSourceControl.cpp
//  Version:     v1.00
//  Created:     2009/01/12 by Sergey Sokov.
//  Compilers:   Visual Studio.NET 2005
//  Description: This is simplified/modified version of the code 
//               written by Sergiy Shaikin (see
//               Sandbox\Plugins\PerforcePlugin\PerforceSourceControl.cpp)
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"


#include "PerforceSourceControl.h"


ICrySourceControl* CPerforceSourceControl::Create()
{
	CPerforceSourceControl* p = new CPerforceSourceControl();
	if(p && p->m_bIsFailedToConnect)
	{
		delete p;
		p = 0;
	}
	return p;
}

void CPerforceSourceControl::Destroy(ICrySourceControl* pSC)
{
	if(pSC)
	{
		delete pSC;
		pSC = 0;
	}
}



static void SafeStrCpy(char* dst, size_t maxSizeInBytes, const char* src)
{
	assert(src);
	assert(dst);

	const size_t srcSizeInBytes = (strlen(src) + 1) * sizeof(src[0]);

	if(srcSizeInBytes > maxSizeInBytes)
	{
		MessageBox( NULL,"SafeStrCpy(): unexpected long string","Debug",MB_OK|MB_ICONERROR );
		assert(0);
	}

	memcpy(dst, src, srcSizeInBytes);
}


static void SafeStrCat(char* dst, size_t maxSizeInBytes, const char* src)
{
	assert(src);
	assert(dst);

	const size_t srcSizeInBytes = (strlen(src) + 1) * sizeof(src[0]);
    const size_t dstLen = strlen(dst);
	const size_t dstWithoutEndingZeroSizeInBytes = dstLen * sizeof(dst[0]);

	if(dstWithoutEndingZeroSizeInBytes + srcSizeInBytes > maxSizeInBytes)
	{
		MessageBox( NULL,"SafeStrCat(): unexpected long string","Debug",MB_OK|MB_ICONERROR );
		assert(0);
	}

	memcpy(&dst[dstLen], src, srcSizeInBytes);
}



CClientUser::CClientUser()
{
 	Reset();
}

CClientUser::~CClientUser()
{
 	Reset();
}


void CClientUser::Reset()
{
 	m_fileInfo.Reset();
 	m_userInfo.Reset();
 	m_timeInfo.Reset();
	m_e.Clear();
}


void CClientUser::HandleError(Error *e)
{
	if(e == 0)
	{
		return;
	}

	if(e->IsInfo())
	{
		e->Clear();
		return;
	}

	if(e->GetSeverity() < m_e.GetSeverity())
	{
		e->Clear();
		return;
	}

	m_e = *e;

    /* For debug only
	StrBuf  m;
	e->Fmt( &m );

	MessageBox( NULL,"CClientUser::HandleError() called","Debug",MB_OK|MB_ICONERROR );

	StrBuf  m;
	e->Fmt( &m );
	if ( strstr( m.Text(), "file(s) not in client view." ) )
		e->Clear();
	else if ( strstr( m.Text(), "no such file(s)" ) )
		e->Clear();
	else if ( strstr( m.Text(), "access denied" ) )
		e->Clear();
	*/

	e->Clear();
}


void CClientUser::Edit( FileSys *f1, Error *e )
{
	MessageBox( NULL,"CClientUser::Edit() called","Debug",MB_OK|MB_ICONERROR );
}


// All formatted ("tagged") Perforce's output goes through this function.
// We'll store incoming data in appropriate container variables, based
// on data tags.
void CClientUser::OutputStat( StrDict *varList )
{
	StrRef var, val;

	for(int i=0; varList->GetVar(i, var, val); ++i)
	{
		if(var=="clientFile")
		{
			SafeStrCpy(m_fileInfo.m_clientFileName, sizeof(m_fileInfo.m_clientFileName), val.Text());
		}
		else if(var=="depotFile")
		{
			SafeStrCpy(m_fileInfo.m_depotFileName, sizeof(m_fileInfo.m_depotFileName), val.Text());
		}
		else if((var=="user") || (var=="User"))
		{
			SafeStrCpy(m_userInfo.m_userName, sizeof(m_userInfo.m_userName), val.Text());
		}
		else if(var=="time")
		{
			m_timeInfo.m_time = val.Atoi(val.Text());
		}
		else if(var=="client")
		{
			SafeStrCpy(m_userInfo.m_clientName, sizeof(m_userInfo.m_clientName), val.Text());
		}
		else if(var=="FullName")
		{
			SafeStrCpy(m_userInfo.m_userFullName, sizeof(m_userInfo.m_userFullName), val.Text());
		}
		else if(var=="Email")
		{
			SafeStrCpy(m_userInfo.m_email, sizeof(m_userInfo.m_email), val.Text());
		}
		else
		{
			// MessageBox( NULL,"Unknown field found in OutputStat()","Debug",MB_OK|MB_ICONERROR );
		}
	}
}


void CClientUser::OutputBinary( const char* data, int size )
{
	assert(data);
	assert(size>=0);
	printf("OutputBinary() called. Size is %i\n", size);
}

void CClientUser::OutputText( const char* data, int length )
{
	assert(data);
	assert(length>=0);
	printf("OutputText(): ");
	while(length--)
	{
		printf("%c", (*data++));
	}
}

void CClientUser::OutputInfo( char level, const char* data )
{
	assert(data);
	printf("OutputInfo(): level %i: %s", level, data);
}

void CClientUser::OutputError( const char* data )
{
	assert(data);
	printf("OutputError(): %s", data);
}


int CClientUser::OutputBinary( char* data, int length )
{
	OutputBinary((const char*)data, length);
	return 0;
}

int CClientUser::OutputText( char* data, int length )
{
	OutputText((const char*)data, length);
	return 0;
}

int CClientUser::OutputInfo( char level, char* data )
{
	OutputInfo(level, (const char*)data);
	return 0;
}

int CClientUser::OutputError( char* data )
{
	OutputError((const char*)data);
	return 0;
}


const CSourceControlFileInfo& CClientUser::GetFileInfo() const
{
	return m_fileInfo;
}

const CSourceControlUserInfo& CClientUser::GetUserInfo() const
{
	return m_userInfo;
}

const CSourceControlTimeInfo& CClientUser::GetTimeInfo() const
{
	return m_timeInfo;
}

Error& CClientUser::GetError()
{
	return m_e;
}


////////////////////////////////////////////////////////////

CPerforceSourceControl::CPerforceSourceControl()
{
	m_cu.Reset();

	m_bIsFailedToConnect = false;

	m_client.SetProtocol("tag", "");	// forcing "tagged" output for all commands

	Error e;
	m_client.Init( &e );

	if ( e.Test() )
	{
		m_bIsFailedToConnect = true;
	}
}

CPerforceSourceControl::~CPerforceSourceControl()
{
	m_cu.Reset();

	Error e;
	m_client.Final( &e );

	// m_e.Abort();   <-- commented out becuase Abort() not exists in (old) Perforce API we using at the moment
}

bool CPerforceSourceControl::GetLastCheckOutInfo(
	const char* fileName, 
	const char* clientName, 
	CSourceControlFileInfo& fileInfo, 
	CSourceControlUserInfo& userInfo, 
	CSourceControlTimeInfo& timeInfo)
{
	assert(fileName);
	assert(fileName[0]);

	char str[MAX_PATH];

	m_cu.Reset();

	if(m_cu.GetError().Test())
	{
		return false;
	}

	if(m_client.Dropped())
	{
		return false;
	}

	m_client.SetClient(clientName?clientName:"");

	// Obtain name of the file in the depot
	{
		SafeStrCpy(str, sizeof(str), fileName);
		char* argv[] = 
		{ 
			&str[0] 
		};
		m_client.SetArgv( sizeof(argv)/sizeof(argv[0]), argv );

		m_client.Run( "fstat", &m_cu );

		m_client.WaitTag();

		if(m_cu.GetError().Test())
		{
			return false;
		}

		fileInfo = m_cu.GetFileInfo();

		if((fileInfo.m_clientFileName[0]==0) || (fileInfo.m_depotFileName[0]==0))
		{
			return false;
		}
	}

	// Obtain information about last-known user who worked on the file
	{
		SafeStrCpy(str, sizeof(str), fileInfo.m_depotFileName);
		char* argv[] = 
		{ 
			"-m",
			"1",
			&str[0] 
		};
		m_client.SetArgv( sizeof(argv)/sizeof(argv[0]), argv );

		m_client.Run( "changes", &m_cu );

		m_client.WaitTag();

		if(m_cu.GetError().Test())
		{
			return false;
		}

		userInfo = m_cu.GetUserInfo();
		timeInfo = m_cu.GetTimeInfo();

		if(userInfo.m_userName[0]==0)
		{
			return false;
		}
	}

	// Obtain additional information about the user (eMail, full name)
	{
		SafeStrCpy(str, sizeof(str), userInfo.m_userName);
		char* argv[] = 
		{ 
			&str[0] 
		};
		m_client.SetArgv( sizeof(argv)/sizeof(argv[0]), argv );

		m_client.Run( "users", &m_cu );

		m_client.WaitTag();

		if(m_cu.GetError().Test())
		{
			return false;
		}

		userInfo = m_cu.GetUserInfo();
	}

	return true;
}

void CPerforceSourceControl::MakeFullFileName( const char* fileName, char* fullFileName )
{
	assert(fileName);
	assert(fileName[0]);
	assert(fullFileName);

	fullFileName[0] = 0;
	GetCurrentDirectory(MAX_PATH, fullFileName);
    SafeStrCat(fullFileName, sizeof(fullFileName), "\\");
    SafeStrCat(fullFileName, sizeof(fullFileName), fileName);
}


/*
bool CPerforceSourceControl::Run(const char* func, int nArgs, char* argv[], bool bOnlyFatal, bool bTestPasswd)
{
	bool bRet = false;

	m_cu.Reset();

	m_client.SetArgv( nArgs, argv );
	m_client.Run( func, &m_cu );
	m_client.WaitTag();

	if(bOnlyFatal)
	{
		if(!m_cu.GetError().IsFatal())
		{
			bRet = true;
		}
	}
	else
	{
		if(!m_cu.GetError().Test())
		{
			bRet = true;
		}
	}

//	if(m_cu.m_e.GetSeverity()==E_FAILED && bTestPasswd && (m_cu.m_e.GetGeneric()==0x0004 || m_cu.m_e.GetGeneric()==0x0024)) // password expired
//	{
//		SetPasswd();
//	}

	m_cu.GetError().Clear();
	return bRet;
}
*/
