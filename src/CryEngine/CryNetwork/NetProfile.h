#ifndef __NET_PROFILE_H__
#define __NET_PROFILE_H__

#include "INetwork.h"

#if NET_PROFILE_ENABLE

extern SNetProfileStackEntry *g_netProfileCurrent;

void netProfileInitialise(bool isMultiplayer);
void netProfileShutDown();
void netProfileTick();
void netProfileAddBits(float bits, bool read);
void netProfileStartProfile();
void netProfileRegisterCVars();
bool netProfileGetChildFromCurrent(const char* name, SNetProfileStackEntry** entry, bool rmi);
void netProfileRegisterBeginCall(const char* name, SNetProfileStackEntry** entry, float budget, bool rmi);
void netProfileBeginFunction(SNetProfileStackEntry* entry, bool read);
void netProfileEndFunction();
bool netProfileIsInitialised();
SNetProfileStackEntry* netProfileGetNullProfile();
void netProfileRenderStats(float x, float y);
void netProfileRegisterWithPerfHUD();

#define NET_PROFILE_ADD_WRITE_BITS(bits) if(netProfileIsInitialised()) { netProfileAddBits(bits, false); }
#define NET_PROFILE_INITIALISE(isMultiplayer) netProfileInitialise(isMultiplayer)
#define NET_PROFILE_SHUTDOWN() netProfileShutDown()
#define NET_PROFILE_TICK() if(netProfileIsInitialised()) { netProfileTick(); }
#define NET_PROFILE_REG_CVARS() netProfileRegisterCVars()

#else

#define NET_PROFILE_ADD_WRITE_BITS(bits)
#define NET_PROFILE_INITIALISE(isMultiplayer)
#define NET_PROFILE_TICK()
#define NET_PROFILE_SHUTDOWN()
#define NET_PROFILE_REG_CVARS()

#endif
#endif // __NET_PROFILE_H__