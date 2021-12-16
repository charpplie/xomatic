#ifndef __CRYLIVEMATCHMAKING_H__
#define __CRYLIVEMATCHMAKING_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "Lobby/CryMatchMaking.h"

class CCryLiveVoice;

struct SCryLiveSessionID : public SCrySessionID
{
	XSESSION_INFO		m_info;
	DWORD						m_gameType;
	BOOL						m_fromInvite;
};

class CCryLiveMatchMaking : public CCryMatchMaking
{
public:
													CCryLiveMatchMaking(CCryLobby* lobby, CCryLobbyService* service);

					ECryLobbyError	Initialise();
					void						Tick(CTimeValue tv);

					void						OnPacket(const TNetAddress& addr, const uint8* data, uint32 length);
					bool						GetRemoteUserConnectionInfo(CryLobbyConnectionID connectionID, uint32 remoteUser, CryMatchMakingConnectionUID* uid, XUID* xuid, uint32* localUser);
					HANDLE					GetLiveSessionHandle(CrySessionHandle h);

	virtual ECryLobbyError	SessionRegisterUserData(SCrySessionUserData* data, uint32 numData, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg);
	virtual ECryLobbyError	SessionCreate(uint32* users, int numUsers, uint32 flags, SCrySessionData* data, CryLobbyTaskID* taskID, CryMatchmakingSessionCreateCallback cb, void* cbArg);
	virtual ECryLobbyError	SessionMigrate(CrySessionHandle h,uint32* pUsers, int numUsers, uint32 flags, SCrySessionData* pData, CryLobbyTaskID* pTaskID, CryMatchmakingSessionCreateCallback pCB, void* pCBArg);
	virtual ECryLobbyError	SessionUpdate(CrySessionHandle h, SCrySessionUserData* data, uint32 numData, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg);
	virtual ECryLobbyError	SessionStart(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg);
	virtual ECryLobbyError	SessionEnd(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg);
	virtual ECryLobbyError	SessionDelete(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg);
	virtual ECryLobbyError	SessionSearch(uint32 user, SCrySessionSearchParam* param, CryLobbyTaskID* taskID, CryMatchmakingSessionSearchCallback cb, void* cbArg);
	virtual ECryLobbyError	SessionJoin(uint32* users, int numUsers, CrySessionID id, CryLobbyTaskID* taskID, CryMatchmakingSessionJoinCallback cb, void* cbArg);

#if NETWORK_HOST_MIGRATION
	virtual void						HostMigrationInitiate(void);
	virtual ECryLobbyError	HostMigrationServer(void);
	virtual bool						GetNewHostAddress(char* address);
#endif

private:
	enum ETask
	{
		eT_SessionRegisterUserData,
		eT_SessionCreate,
		eT_SessionMigrate,
		eT_SessionUpdate,
		eT_SessionStart,
		eT_SessionWaitArbitrationStart,
		eT_SessionHostWaitArbitrationStart,
		eT_SessionWaitArbitrationFinish,
		eT_SessionArbitration,
		eT_SessionEnd,
		eT_SessionDeleteWaitStart,
		eT_SessionDelete,
		eT_SessionSearch,
		eT_SessionJoinWaitStart,
		eT_SessionJoin,
		eT_SessionJoinLocal,
		eT_SessionRequestJoin,
		eT_SessionJoinModifySession,
		eT_SessionWaitJoinRemoteFromRequestJoin,
		eT_SessionJoinRemoteFromRequestJoin,
		eT_SessionJoinRemote,
		eT_SessionLeaveRemote,
		eT_SessionMigrateHostServer,
		eT_SessionMigrateHostClient,
		eT_SessionMigrateHostFinish
	};

	struct	SRegisteredUserData
	{
		SCrySessionUserData		data[MAX_MATCHMAKING_SESSION_USER_DATA];
		uint32								num;
	};

	struct  SSession : public CCryMatchMaking::SSession
	{
		ULONGLONG							nonce;
		HANDLE								handle;
		DWORD									flags;
		DWORD									gameType;
		DWORD									gameMode;
		DWORD									numPublicSlots;
		DWORD									numPrivateSlots;
		XSESSION_INFO					info;

		struct SLConnection : public CCryMatchMaking::SSession::SLConnection
		{
			DWORD								users[MAX_LIVE_LOCAL_USERS];
			BOOL								privateSlot[MAX_LIVE_LOCAL_USERS];
		}											localConnection;

		struct SRConnection : public CCryMatchMaking::SSession::SRConnection
		{
			XUID								xuid[MAX_LIVE_LOCAL_USERS];
			BOOL								privateSlot[MAX_LIVE_LOCAL_USERS];
			XNADDR							xnaddr;

			enum ERemoteConnectionState
			{
				eRCS_None,
				eRCS_WaitingToJoin,
				eRCS_Joining,
				eRCS_JoiningButWantToLeave,
				eRCS_Joined,
				eRCS_WaitingToLeave,
				eRCS_Leaving
			}										state;

			uint16							port;
			uint8								localUser[MAX_LIVE_LOCAL_USERS]; // The user index 0-3 of the user on their console
			uint8								localUser2Index[MAX_LIVE_LOCAL_USERS];
			bool								registered;
			bool								m_migrated;
		}											remoteConnection[MAX_LOBBY_CONNECTIONS];

		CryMatchMakingTaskID	remoteConnectionTaskID;

		bool									remoteConnectionProcessingToDo;
		bool									started;
		bool									unregisteredConnectionsKicked;
	};

	struct  STask : public CCryMatchMaking::STask
	{
		uint32								user;
		XOVERLAPPED						overlapped;
	};

					ECryLobbyError	StartTask(ETask etask, bool startRunning, uint32 user, CryMatchMakingTaskID* mmTaskID, CryLobbyTaskID* lTaskID, CrySessionHandle h, void* cb, void* cbArg);
					void						StartSubTask(ETask etask, CryMatchMakingTaskID mmTaskID);
					void						StartTaskRunning(CryMatchMakingTaskID mmTaskID);
					void						StopTaskRunning(CryMatchMakingTaskID mmTaskID);
					void						EndTask(CryMatchMakingTaskID mmTaskID);
					ECryLobbyError	TickTask(CryMatchMakingTaskID mmTaskID);
					ECryLobbyError	CreateSessionHandle(CrySessionHandle *h, bool host, uint32* users, int numUsers);
					CryMatchMakingConnectionID AddRemoteConnection(CrySessionHandle h, CryLobbyConnectionID connectionID, CryMatchMakingConnectionUID uid, XNADDR* xnaddr, uint16 port, uint32 numUsers, XUID* xuids, uint8* localUser, BOOL* privateSlots);
	virtual	void						FreeRemoteConnection(CrySessionHandle h, CryMatchMakingConnectionID id);
					void						SetRemoteConnectionState(CrySessionHandle h, CryMatchMakingConnectionID id, SSession::SRConnection::ERemoteConnectionState state);
					ECryLobbyError	SetSessionUserData(CrySessionHandle h, SCrySessionUserData* data, uint32 numData);
					CrySessionHandle	FindSessionFromID(XNKID xnkid);

					void						StartSessionCreate(CryMatchMakingTaskID mmTaskID);
					void						TickSessionCreate(CryMatchMakingTaskID mmTaskID);

					void						StartSessionMigrate(CryMatchMakingTaskID mmTaskID);
					void						TickSessionMigrate(CryMatchMakingTaskID mmTaskID);

					void						SessionJoinLocal(CryMatchMakingTaskID mmTaskID);
					void						TickSessionJoinLocal(CryMatchMakingTaskID mmTaskID);

					void						StartSessionSearch(CryMatchMakingTaskID mmTaskID);
					void						TickSessionSearch(CryMatchMakingTaskID mmTaskID);
					void						EndSessionSearch(CryMatchMakingTaskID mmTaskID);

					void						StartSessionUpdate(CryMatchMakingTaskID mmTaskID);

					void						StartSessionStart(CryMatchMakingTaskID mmTaskID);
					void						StartSessionStartMain(CryMatchMakingTaskID mmTaskID, CrySessionHandle h, CryMatchMakingTaskID hostTaskID);
					void						TickSessionStart(CryMatchMakingTaskID mmTaskID);
					void						ProcessSessionStart(const TNetAddress& addr, const uint8* data, uint32 length);
					void						TickSessionWaitArbitrationStart(CryMatchMakingTaskID mmTaskID);
					void						TickSessionHostWaitArbitrationStart(CryMatchMakingTaskID mmTaskID);
					void						StartSessionArbitration(CryMatchMakingTaskID mmTaskID);
					void						TickSessionArbitration(CryMatchMakingTaskID mmTaskID);
					void						ProcessSessionIHaveRegistered(const TNetAddress& addr, const uint8* data, uint32 length);
					void						TickSessionWaitArbitrationFinish(CryMatchMakingTaskID mmTaskID);
					void						ProcessSessionRegistrationFinished(const TNetAddress& addr, const uint8* data, uint32 length);
					void						StartSessionStartAfterArbitration(CryMatchMakingTaskID mmTaskID);

					void						StartSessionEnd(CryMatchMakingTaskID mmTaskID);
					void						StartSessionEndMain(CryMatchMakingTaskID mmTaskID, CrySessionHandle h);
					void						TickSessionEnd(CryMatchMakingTaskID mmTaskID);
					void						ProcessSessionEnd(const TNetAddress& addr, const uint8* data, uint32 length);

					void						StartSessionJoin(CryMatchMakingTaskID mmTaskID);
					void						TickSessionJoinWaitStart(CryMatchMakingTaskID mmTaskID);
					void						TickSessionJoin(CryMatchMakingTaskID mmTaskID);
					void						TickSessionRequestJoin(CryMatchMakingTaskID mmTaskID);
					void						EndSessionJoin(CryMatchMakingTaskID mmTaskID);
					void						TickSessionWaitJoinRemoteFromRequestJoin(CryMatchMakingTaskID mmTaskID);
					void						ProcessSessionRequestJoin(CryMatchMakingTaskID mmTaskID, const TNetAddress& addr, const uint8* data, uint32 length);
					void						TickSessionJoinModifySession(CryMatchMakingTaskID mmTaskID);
					void						TickSessionJoinRemoteFromRequestJoin(CryMatchMakingTaskID mmTaskID);
					void						ProcessSessionRequestJoinResult(const TNetAddress& addr, const uint8* data, uint32 length);
					void						ProcessSessionAddRemoteConnections(const TNetAddress& addr, const uint8* data, uint32 length);

					void						StartSessionDelete(CryMatchMakingTaskID mmTaskID);
					void						TickSessionDeleteWaitStart(CryMatchMakingTaskID mmTaskID);
					void						TickSessionDelete(CryMatchMakingTaskID mmTaskID);

					void						InformVoiceLocalUsersChanged();
					void						InformVoiceRemoteUsersJoined(CrySessionHandle h, CryMatchMakingConnectionID id);
					void						InformVoiceRemoteUsersLeft(CrySessionHandle h, CryMatchMakingConnectionID id);

#if NETWORK_HOST_MIGRATION
					void						HostMigrationServerNT(CryMatchMakingTaskID mmTaskID);
					void						TickHostMigrationServerNT(CryMatchMakingTaskID mmTaskID);
					ECryLobbyError	HostMigrationClient(CrySessionHandle h, XSESSION_INFO* pInfo, CryMatchMakingTaskID hostTaskID);
					void						TickHostMigrationClientNT(CryMatchMakingTaskID mmTaskID);
					void						ProcessHostMigrationFromServer(const TNetAddress& addr, const uint8* pData, uint32 length);
					void						ProcessHostMigrationFromClient(const TNetAddress& addr, const uint8* pData, uint32 length);
					void						TickHostMigrationFinishNT(CryMatchMakingTaskID mmTaskID);
#endif

	SRegisteredUserData			m_registeredUserData;
	SSession								m_sessions[MAX_MATCHMAKING_SESSIONS];
	STask										m_task[MAX_MATCHMAKING_TASKS];
};


#endif // __CRYLIVEMATCHMAKING_H__
