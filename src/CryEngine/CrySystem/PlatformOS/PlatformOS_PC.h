////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek.
// -------------------------------------------------------------------------
//  File name:   IPlatformOS_PC.h
//  Created:     18/12/2009 by Alex Weighell.
//  Description: Interface to the Platform OS
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __PLATFORMOS_PC_H_
#define __PLATFORMOS_PC_H_

#if !defined(XENON) && !defined(PS3)

#include "IPlatformOS.h"

class CPlatformOS_PC : public IPlatformOS
{
public:

	// Called each frame to update the platform listener
	VIRTUAL void Tick(float realFrameTime) {}

	// Local user profile functions to check/initiate user sign in:
	// See IPlatformOS.h for documentation.
	VIRTUAL unsigned int	UserGetMaximumSignedInUsers() const;
	VIRTUAL bool			UserIsSignedIn(unsigned int userIndex) const;
	VIRTUAL bool			UserDoSignIn(unsigned int numUsersRequested);
	VIRTUAL bool			UserGetName(unsigned int userIndex, IPlatformOS::TUserName& outName) const;
	VIRTUAL bool			UserSelectStorageDevice(unsigned int userIndex) { return true; } // always hard drive
	VIRTUAL bool			GetUserProfilePreference(IPlatformOS::EUserProfilePreference ePreference, SUserProfileVariant& outResult) const;

	VIRTUAL IPlatformOS::ISaveReaderPtr SaveGetReader(const IPlatformOS::TFileName& fileName);
	VIRTUAL IPlatformOS::ISaveWriterPtr SaveGetWriter(const IPlatformOS::TFileName& fileName);

	VIRTUAL void AddListener(IPlatformListener* pListener, const char* szName) {}
	VIRTUAL void RemoveListener(IPlatformListener* pListener) {}
	VIRTUAL void NotifyListeners(SPlatformEvent& event) {}
	
	VIRTUAL IPlatformOS::EMsgBoxResult DebugMessageBox(const char* body, const char* title, unsigned int flags=0) const;
};

#endif
#endif
