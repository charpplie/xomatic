#ifndef __CRYPSNMATCHMAKING_H__
#define __CRYPSNMATCHMAKING_H__

#pragma once

#if USE_PSN

#include "Lobby/CryMatchMaking.h"

#include <np.h>

#define MAX_PSN_LOCAL_USERS				(1)

#define MAX_ATTRIB_IDS						(10)			// 8 searchable ints + 2 binary packets

#include "CryPSNSessionSupport.h"
#include "CryPSNSupport.h"

struct SCryPSNSessionID : public SCrySessionID
{
	SceNpMatching2RoomId			m_roomId;				// ordered by size in current sdk (3.0.0)
	SceNpMatching2LobbyId			m_lobbyId;
	SceNpMatching2WorldId			m_worldId;
	uint32										m_gameType;
	SceNpMatching2ServerId		m_serverId;
	bool											m_fromInvite;
};

class CCryPSNMatchMaking : public CCryMatchMaking
{
public:
													CCryPSNMatchMaking(CCryLobby* lobby,CCryLobbyService* service);
	virtual									~CCryPSNMatchMaking();

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
	virtual void						SessionDisconnectRemoteConnection(CrySessionHandle gh, const TNetAddress& addr);

#if NETWORK_HOST_MIGRATION
	virtual void						HostMigrationInitiate(void);
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
		eT_SessionEnd,
		eT_SessionDelete,
		eT_SessionSearch,
		eT_SessionJoin,
	};

	struct SMappingToPSNLobbyData
	{
		uint32								integerField:1;
		uint32								fieldOffset:31;			// either 0-7 (representing integer field), or 0-511 (representing offset into binary chunk)
	};

	struct	SRegisteredUserData
	{
		SCrySessionUserData			data[MAX_MATCHMAKING_SESSION_USER_DATA];
		SMappingToPSNLobbyData	mapping[MAX_MATCHMAKING_SESSION_USER_DATA];
		uint32									num;
	};

	struct  SSession : public CCryMatchMaking::SSession
	{
		SceNpMatching2RoomId			m_roomId;				// ordered by size in current sdk (3.0.0)
		SceNpMatching2LobbyId			m_lobbyId;
		SceNpMatching2WorldId			m_worldId;

		uint32									flags;
		uint32									gameType;
		uint32									gameMode;
		uint32									numPublicSlots;
		uint32									numPrivateSlots;

		SceNpMatching2ServerId		m_serverId;
		

		struct SLConnection : public CCryMatchMaking::SSession::SLConnection
		{
			DWORD								users[MAX_PSN_LOCAL_USERS];
			BOOL								privateSlot[MAX_PSN_LOCAL_USERS];
		}											localConnection;

		struct SRConnection : public CCryMatchMaking::SSession::SRConnection
		{
			SceNpMatching2RoomMemberId	memberId;
			bool												registered;
		}											remoteConnection[MAX_LOBBY_CONNECTIONS];

		CryMatchMakingTaskID	remoteConnectionTaskID;

		bool									remoteConnectionProcessingToDo;
		bool									started;
		bool									unregisteredConnectionsKicked;
	};

	struct  STask : public CCryMatchMaking::STask
	{
		uint32								user;
	};

					CryMatchMakingConnectionID FindConnectionIDFromMemberID(SSession* session,SceNpMatching2RoomMemberId memberId);

					void						AddRemote(const CrySessionHandle sessionHandle, const SRoomMember* member);
					void						RemoveRemote(const CrySessionHandle sessionHandle, const SRoomMember* member);
					void						ConfigureRemote(const CrySessionHandle sessionHandle, const SRoomMember* member);

					static void			SupportCallback(ECallbackEvent ecb, const SCallbackEventData &data, void *userArg);

					ECryLobbyError	MapSupportErrorToLobbyError(EPSNPendingError pendingError);

					ECryLobbyError	StartTask(ETask etask, bool startRunning, uint32 user, CryMatchMakingTaskID* mmTaskID, CryLobbyTaskID* lTaskID, CrySessionHandle h, void* cb, void* cbArg);
					void						StartTaskRunning(CryMatchMakingTaskID mmTaskID);
					void						StopTaskRunning(CryMatchMakingTaskID mmTaskID);
					void						EndTask(CryMatchMakingTaskID mmTaskID);

					ECryLobbyError	CreateSessionHandle(CrySessionHandle *h, bool host, uint32* users, int numUsers);

					void						StartSessionCreate(CryMatchMakingTaskID mmTaskID);
					void						TickSessionCreate(CryMatchMakingTaskID mmTaskID);

					void						StartSessionMigrate(CryMatchMakingTaskID mmTaskID);
					void						TickSessionMigrate(CryMatchMakingTaskID mmTaskID);

					void						StartSessionSearch(CryMatchMakingTaskID mmTaskID);
					void						TickSessionSearch(CryMatchMakingTaskID mmTaskID);
					void						EndSessionSearch(CryMatchMakingTaskID mmTaskID);
					
					void						StartSessionJoin(CryMatchMakingTaskID mmTaskID);
					void						TickSessionJoin(CryMatchMakingTaskID mmTaskID);
					void						EndSessionJoin(CryMatchMakingTaskID mmTaskID);
					
					void						StartSessionDelete(CryMatchMakingTaskID mmTaskID);
					void						TickSessionDelete(CryMatchMakingTaskID mmTaskID);

	CCryPSNSupport					*m_lobbySupport;

	SRegisteredUserData			m_registeredUserData;
	SSession								m_sessions[MAX_MATCHMAKING_SESSIONS];
	STask										m_task[MAX_MATCHMAKING_TASKS];

	uint32									m_oldNatType;
	uint32									m_hostMigrationSafetySleep;
	CrySessionHandle						m_hostMigrationSession;
};

#endif // USE_PSN

#endif // __CRYPSNMATCHMAKING_H__
