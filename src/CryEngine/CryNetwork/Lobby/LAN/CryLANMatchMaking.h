#ifndef __CRYLANMATCHMAKING_H__
#define __CRYLANMATCHMAKING_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "Lobby/CryMatchMaking.h"

#define MAX_LAN_SEARCH_SERVERS			128
#define MAX_LAN_SEARCHES						2

struct SCryLANSessionID : public SCrySessionID
{
	CrySessionHandle	m_h;
	uint32						m_ip; 
	uint16						m_port;
};

class CCryLANMatchMaking : public CCryMatchMaking
{
public:
													CCryLANMatchMaking(CCryLobby* lobby, CCryLobbyService* service);

					ECryLobbyError	Initialise();
					void						Tick(CTimeValue tv);

					void						OnPacket(const TNetAddress& addr, const uint8* data, uint32 length);

	virtual ECryLobbyError	SessionRegisterUserData(SCrySessionUserData* data, uint32 numData, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg);
	virtual ECryLobbyError	SessionCreate(uint32* users, int numUsers, uint32 flags, SCrySessionData* data, CryLobbyTaskID* taskID, CryMatchmakingSessionCreateCallback cb, void* cbArg);
	virtual ECryLobbyError	SessionMigrate(CrySessionHandle h,uint32* pUsers, int numUsers, uint32 flags, SCrySessionData* pData, CryLobbyTaskID* pTaskID, CryMatchmakingSessionCreateCallback pCB, void* pCBArg);
	virtual ECryLobbyError	SessionUpdate(CrySessionHandle h, SCrySessionUserData* data, uint32 numData, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg);
	virtual ECryLobbyError	SessionStart(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg);
	virtual ECryLobbyError	SessionEnd(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg);
	virtual ECryLobbyError	SessionDelete(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg);
	virtual ECryLobbyError	SessionSearch(uint32 user, SCrySessionSearchParam* param, CryLobbyTaskID* taskID, CryMatchmakingSessionSearchCallback cb, void* cbArg);
	virtual ECryLobbyError	SessionJoin(uint32* users, int numUsers, CrySessionID id, CryLobbyTaskID* taskID, CryMatchmakingSessionJoinCallback cb, void* cbArg);
	virtual void						SessionJoinFromConsole(void);

#if NETWORK_HOST_MIGRATION
	virtual void						HostMigrationInitiate(void);
	virtual ECryLobbyError	HostMigrationServer(void);
	virtual bool						GetNewHostAddress(char* address);
	virtual void						HostMigrationSetNewServerAddress(TNetAddress& address);
#endif

private:
	enum ETask
	{
		eT_SessionRegisterUserData,
		eT_SessionCreate,
		eT_SessionUpdate,
		eT_SessionStart,
		eT_SessionEnd,
		eT_SessionDelete,
		eT_SessionSearch,
		eT_SessionJoin,
		eT_SessionMigrateHostServer,
		eT_SessionMigrateHostClient,
		eT_SessionMigrateHostFinish
	};

					ECryLobbyError	StartTask(ETask task, CryMatchMakingTaskID* mmTaskID, CryLobbyTaskID* lTaskID, CrySessionHandle h, void* cb, void* cbArg);
					void						StartTaskRunning(CryMatchMakingTaskID mmTaskID);
					void						StopTaskRunning(CryMatchMakingTaskID mmTaskID);
					void						EndTask(CryMatchMakingTaskID mmTaskID);
					ECryLobbyError	CreateSessionHandle(CrySessionHandle *h, bool host, uint32 flags, int numUsers);
					CryMatchMakingConnectionID AddRemoteConnection(CrySessionHandle h, CryLobbyConnectionID connectionID, CryMatchMakingConnectionUID uid, uint32 ip, uint16 port, uint32 numUsers);
	virtual void						FreeRemoteConnection(CrySessionHandle h, CryMatchMakingConnectionID id);
					ECryLobbyError	CreateSessionSearchHandle(CrySessionHandle *h);
					ECryLobbyError	SetSessionUserData(CrySessionHandle h, SCrySessionUserData* data, uint32 numData);
					ECryLobbyError	SetSessionData(CrySessionHandle h, SCrySessionData* data);
					size_t					CalculateServerDataSize(CrySessionHandle h);
					void						SendServerData(const TNetAddress& addr, const uint8* data, uint32 length);
					void						ServerDataToGame(CryMatchMakingTaskID mmTaskID, uint32 ip, uint16 port, TMemHdl params, uint32 length);
					void						ProcessServerData(const TNetAddress& addr, const uint8* data, uint32 length);
					CrySessionHandle	FindSessionFromServerID(CrySessionHandle h);

					void						StartSessionJoin(CryMatchMakingTaskID mmTaskID);
					void						TickSessionJoin(CryMatchMakingTaskID mmTaskID);
					void						ProcessSessionRequestJoin(const TNetAddress& addr, const uint8* data, uint32 length);
					void						ProcessSessionRequestJoinResult(const TNetAddress& addr, const uint8* data, uint32 length);
					void						ProcessSessionAddRemoteConnections(const TNetAddress& addr, const uint8* data, uint32 length);

					void						StartSessionDelete(CryMatchMakingTaskID mmTaskID);

#if NETWORK_HOST_MIGRATION
					void						HostMigrationServerNT(CryMatchMakingTaskID mmTaskID);
					ECryLobbyError	HostMigrationClient(CrySessionHandle h, CryMatchMakingTaskID hostTaskID);
					void						TickHostMigrationClientNT(CryMatchMakingTaskID mmTaskID);
					void						ProcessHostMigrationFromServer(const TNetAddress& addr, const uint8* pData, uint32 length);
					void						ProcessHostMigrationFromClient(const TNetAddress& addr, const uint8* pData, uint32 length);
					void						TickHostMigrationFinishNT(CryMatchMakingTaskID mmTaskID);
#endif

	struct	SRegisteredUserData
	{
		SCrySessionUserData		data[MAX_MATCHMAKING_SESSION_USER_DATA];
		uint32								num;
	}												m_registeredUserData;

	struct	SSession : public CCryMatchMaking::SSession
	{
		SCrySessionUserData		userData[MAX_MATCHMAKING_SESSION_USER_DATA];
		SCrySessionData				data;
		SCryLANSessionID			id;
		uint32								flags;
		uint32								numFilledSlots;

		struct SLConnection : public CCryMatchMaking::SSession::SLConnection
		{
		}											localConnection;

		struct SRConnection : public CCryMatchMaking::SSession::SRConnection
		{
			uint32							ip;
			uint16							port;
			bool								m_migrated;
		}											remoteConnection[MAX_LOBBY_CONNECTIONS];
	};

	struct	SSearch
	{
		SCryLANSessionID			servers[MAX_LAN_SEARCH_SERVERS];
		uint32								numServers;
		bool									used;
	}												m_search[MAX_LAN_SEARCHES];

	struct	STask : public CCryMatchMaking::STask
	{
		CTimeValue						lastUpdate;
		uint32								ticks;
	};

	SSession								m_sessions[MAX_MATCHMAKING_SESSIONS];
	STask										m_task[MAX_MATCHMAKING_TASKS];
};


#endif // __CRYMATCHMAKING_H__
