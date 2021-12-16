////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek.
// -------------------------------------------------------------------------
//  File name:   PlatformOS_PC.cpp
//  Created:     11/02/2010 by Alex McCarthy.
//  Description: Implementation of the IPlatformOS interface for PC
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include <StdAfx.h>

#if !defined(XENON) && !defined(PS3)

#if defined(WIN32) || defined(WIN64)
#define PLATFORM_WINDOWS
#endif

#include "PlatformOS_PC.h"
#include "SaveReaderWriter_CryPak.h"


IPlatformOS* IPlatformOS::Create()
{
	return new CPlatformOS_PC();
}

bool CPlatformOS_PC::GetUserProfilePreference(EUserProfilePreference ePreference, SUserProfileVariant& outResult) const
{
	return false;
}

unsigned int CPlatformOS_PC::UserGetMaximumSignedInUsers() const
{
	return 1;
}

bool CPlatformOS_PC::UserIsSignedIn(unsigned int userIndex) const
{
	return true;
}

bool CPlatformOS_PC::UserDoSignIn(unsigned int numUsersRequested)
{
	return true;
}

bool CPlatformOS_PC::UserGetName(unsigned int userIndex, IPlatformOS::TUserName& outName) const
{
#ifdef PLATFORM_WINDOWS
	DWORD numChars = outName.MAX_SIZE;
	BOOL e = GetUserNameA(outName.m_strBuf, &numChars);
	return e ? true : false;
#else
	outName.assign( gEnv->pSystem->GetUserName() );
	return true;
#endif
}


IPlatformOS::ISaveReaderPtr CPlatformOS_PC::SaveGetReader(const IPlatformOS::TFileName& fileName)
{
	return ISaveReaderPtr(new CSaveReader_CryPak(fileName));
}

IPlatformOS::ISaveWriterPtr CPlatformOS_PC::SaveGetWriter(const IPlatformOS::TFileName& fileName)
{
	return ISaveWriterPtr(new CSaveWriter_CryPak(fileName));
}

IPlatformOS::EMsgBoxResult
CPlatformOS_PC::DebugMessageBox( const char* body, const char* title, unsigned int flags ) const
{
#ifdef PLATFORM_WINDOWS
	int winresult = CryMessageBox( body, title, MB_OKCANCEL );
	return (winresult == IDOK) ? eMsgBox_OK : eMsgBox_Cancel;
#else
	CRY_ASSERT_MESSAGE(false, "DebugMessageBox not implemented on non-windows platforms!");
	return eMsgBox_OK; // [AlexMcC|30.03.10]: Ok? Cancel? Dunno! Uh-oh :( This is only used in CryPak.cpp so far, and for that use it's better to return ok
#endif
}


#endif // !defined(XENON) && !defined(PS3)

