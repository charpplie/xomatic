#ifndef __ICRYMATCHMAKING_H__
#define __ICRYMATCHMAKING_H__

#include "ICryLobby.h"

typedef uint32 CrySessionHandle;

const CrySessionHandle CrySessionInvalidHandle = 0xffffffff;

#define CRYSESSION_CREATE_FLAG_SEARCHABLE		0x00000001			// If set the session will be searchable and returned in session searches. If set uses XSESSION_CREATE_USES_MATCHMAKING on live if not set uses SCE_NP_MATCHING2_ROOM_FLAG_ATTR_HIDDEN on NP

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ECrySessionUserDataType and SCrySessionUserData have been deprecated and the more general ECryLobbyUserDataType and SCryLobbyUserData
// should be used instead. This section is here for compatibility and will ideally be removed in future.
typedef ECryLobbyUserDataType ECrySessionUserDataType;
const ECrySessionUserDataType eCSUDT_Int64 = eCLUDT_Int64;
const ECrySessionUserDataType eCSUDT_Int32 = eCLUDT_Int32;
const ECrySessionUserDataType eCSUDT_Int16 = eCLUDT_Int16;
const ECrySessionUserDataType eCSUDT_Int8 = eCLUDT_Int8;
const ECrySessionUserDataType eCSUDT_Float64 = eCLUDT_Float64;
const ECrySessionUserDataType eCSUDT_Float32 = eCLUDT_Float32;
const ECrySessionUserDataType eCSUDT_Int64NoEndianSwap = eCLUDT_Int64NoEndianSwap;

// User data ID's should be defined to be values the underlying matchmaking service can use.
// In Live the session user data has to be defined with the xlast program which will produce an h file with the ID's.
// In NP due to limitations of the service, you should just use any UID (the matchmaking service will then place the values
// appropriately to fit) Key note : Only the first 8 uint32 fields registered will be searchable/filterable
// In CryLAN any UID can be used. It will happily use the same ID's defined for the underlying online service for each platform.
typedef SCryLobbyUserData SCrySessionUserData;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_SESSION_NAME_LENGTH	32

struct SCrySessionData
{
	SCrySessionUserData*	m_data;
	uint32								m_numData;
	uint32								m_numPublicSlots;
	uint32								m_numPrivateSlots;
	char									m_name[MAX_SESSION_NAME_LENGTH];
	bool									m_ranked;
};

enum ECrySessionSearchOperator
{
	eCSSO_Equal,
	eCSSO_NotEqual,
	eCSSO_LessThan,
	eCSSO_LessThanEqual,
	eCSSO_GreaterThan,
	eCSSO_GreaterThanEqual
};

struct SCrySessionSearchData
{
	SCrySessionUserData				m_data;
	ECrySessionSearchOperator	m_operator;
};

struct SCrySessionSearchParam
{
	uint32									m_type;
	SCrySessionSearchData*	m_data;
	uint32									m_numData;
	uint32									m_numFreeSlots;
	uint32									m_maxNumReturn;
	bool										m_ranked;
};

struct SCrySessionSearchResult
{
	SCrySessionData				m_data;
	CrySessionID					m_id;
	uint32								m_numFilledSlots;
};

// CryMatchmakingCallback
// taskID		-	Task ID allocated when the function was executed
// error		- Error code - eCLE_Success if the function succeeded or an error that occurred while processing the function
// arg			- Pointer to application-specified data
typedef void (*CryMatchmakingCallback)(CryLobbyTaskID taskID, ECryLobbyError error, void* arg);

// CryMatchmakingSessionCreateCallback
// taskID		-	Task ID allocated when the function was executed
// error		- Error code - eCLE_Success if the function succeeded or an error that occurred while processing the function
// h				- If successful a handle to the session created used to identify the session in other matchmaking functions
// arg			- Pointer to application-specified data
typedef void (*CryMatchmakingSessionCreateCallback)(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, void* arg);

// CryMatchmakingSessionSearchCallback
// taskID		-	Task ID allocated when the function was executed
// error		- Error code
//						eCLE_SuccessContinue if session contains a valid result and search is continuing.
//						eCLE_Success and session NULL if the search has finished and function succeeded or an error that occurred while processing the function
// session	- Pointer to a session search result if error was eCLE_SuccessContinue
// arg			- Pointer to application-specified data
typedef void (*CryMatchmakingSessionSearchCallback)(CryLobbyTaskID taskID, ECryLobbyError error, SCrySessionSearchResult* session, void* arg);

// CryMatchmakingSessionJoinCallback
// taskID		-	Task ID allocated when the function was executed
// error		- Error code - eCLE_Success if the function succeeded or an error that occurred while processing the function
// h				- If successful a handle to the session created used to identify the session in other matchmaking functions
// ip, port	- If successful the address that can be used to communicate with the host.
//						NAT traversal will have been done by the underlying SDK so communication will always be possible with this address
//						but game connection could still fail if host quits before game connection is done.
// arg			- Pointer to application-specified data
typedef void (*CryMatchmakingSessionJoinCallback)(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, uint32 ip, uint16 port, void* arg);

struct ICryMatchMaking
{
public:
	// SessionRegisterUserData
	// This function must be called by all peers before any other matchmaking calls.
	// It defines the applications custom data used for it's sessions.
	// This data is advertised when a host creates a session and gets returned when a session search is performed
	// data				- Pointer to an array of SCrySessionUserData that defines the applications custom data for a session
	// numData		- Number of items in the data array
	// taskID			- Pointer to buffer to store the task ID to identify this call in the callback
	// cb					- Callback function that is called when function completes
	// cbArg			- Pointer to application-specified data that is passed to the callback
	// return			- eCLE_Success if function successfully started or an error code if function failed to start
	virtual ECryLobbyError SessionRegisterUserData(SCrySessionUserData* data, uint32 numData, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg) = 0;

	// SessionCreate
	// Called by the host to create a session and make it available for others to join.
	// users			- Pointer to an array of local users who will be joining the session after creation. The first user will be the session owner
	//							The numbers given should be the pad numbers the users are using.
	//							It is important that these are correct on live as multiple accounts can be signed in one for each pad.
	//							If the wrong pad numbers are passed in then the wrong accounts will be used for the session.
	//							Remember even on a single account machine and single player game the user doesn't have to be using and signed in on pad 0
	// numUsers		- Number of users in the users array
	// flags			- Session create flags that control how the session behaves
	// data				- Pointer to a SCrySessionData that describes the session
	// taskID			- Pointer to buffer to store the task ID to identify this call in the callback
	// cb					- Callback function that is called when function completes
	// cbArg			- Pointer to application-specified data that is passed to the callback
	// return			- eCLE_Success if function successfully started or an error code if function failed to start
	virtual ECryLobbyError SessionCreate(uint32* users, int numUsers, uint32 flags, SCrySessionData* data, CryLobbyTaskID* taskID, CryMatchmakingSessionCreateCallback cb, void* cbArg) = 0;
	
	// SessionMigrate
	// Called by the migrating host to ensure the lobby information is correct for the migrated session
	// h					- Handle of the current session we are migrating
	// users			- Pointer to an array of local users who will be joining the session after creation. The first user will be the session owner
	//							The numbers given should be the pad numbers the users are using.
	//							It is important that these are correct on live as multiple accounts can be signed in one for each pad.
	//							If the wrong pad numbers are passed in then the wrong accounts will be used for the session.
	//							Remember even on a single account machine and single player game the user doesn't have to be using and signed in on pad 0
	// numUsers		- Number of users in the users array
	// flags			- Session create flags that control how the session behaves
	// data				- Pointer to a SCrySessionData that describes the session
	// taskID			- Pointer to buffer to store the task ID to identify this call in the callback
	// cb					- Callback function that is called when function completes
	// cbArg			- Pointer to application-specified data that is passed to the callback
	// return			- eCLE_Success if function successfully started or an error code if function failed to start
	virtual ECryLobbyError SessionMigrate(CrySessionHandle h,uint32* pUsers, int numUsers, uint32 flags, SCrySessionData* pData, CryLobbyTaskID* pTaskID, CryMatchmakingSessionCreateCallback pCB, void* pCBArg) = 0;

	// SessionUpdate
	// Called by the host to update the applications custom data. Only the data items that need updating need to be given.
	// h					- Handle to the session to be updated
	// data				- Pointer to an array of SCrySessionUserData that contains the applications custom data that will be updated for the session
	// numData		- Number of items in the data array
	// taskID			- Pointer to buffer to store the task ID to identify this call in the callback
	// cb					- Callback function that is called when function completes
	// cbArg			- Pointer to application-specified data that is passed to the callback
	// return			- eCLE_Success if function successfully started or an error code if function failed to start
	virtual ECryLobbyError SessionUpdate(CrySessionHandle h, SCrySessionUserData* data, uint32 numData, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg) = 0;

	// SessionStart
	// Called by the host when gameplay starts.
	// In Live ranked sessions the session becomes unjoinable between SessionStart and SessionEnd
	// h					- Handle to the session to start
	// taskID			- Pointer to buffer to store the task ID to identify this call in the callback
	// cb					- Callback function that is called when function completes
	// cbArg			- Pointer to application-specified data that is passed to the callback
	// return			- eCLE_Success if function successfully started or an error code if function failed to start
	virtual ECryLobbyError SessionStart(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg) = 0;

	// SessionEnd
	// Called by the host when gameplay ends.
	// h					- Handle to the session to end
	// taskID			- Pointer to buffer to store the task ID to identify this call in the callback
	// cb					- Callback function that is called when function completes
	// cbArg			- Pointer to application-specified data that is passed to the callback
	// return			- eCLE_Success if function successfully started or an error code if function failed to start
	virtual ECryLobbyError SessionEnd(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg) = 0;

	// SessionDelete
	// Called by all members of the session to delete it when they have finished with it.
	// h					- Handle to the session to delete
	// taskID			- Pointer to buffer to store the task ID to identify this call in the callback
	// cb					- Callback function that is called when function completes
	// cbArg			- Pointer to application-specified data that is passed to the callback
	// return			- eCLE_Success if function successfully started or an error code if function failed to start
	virtual ECryLobbyError SessionDelete(CrySessionHandle h, CryLobbyTaskID* taskID, CryMatchmakingCallback cb, void* cbArg) = 0;

	// SessionSearch
	// Performs a search for sessions that match the specified search criteria.
	// user				- The pad number of the user doing the search.
	//							In Live the TrueSkill system is used. Each session has a skill value worked out from the members skill values.
	//							The skill value of the user doing the search is used to return sessions that have skill values closest to the users value.
	// param			- Pointer to a SCrySessionSearchParam that defines how sessions should be filtered
	// taskID			- Pointer to buffer to store the task ID to identify this call in the callback
	// cb					- Callback function that is called when function completes
	// cbArg			- Pointer to application-specified data that is passed to the callback
	// return			- eCLE_Success if function successfully started or an error code if function failed to start
	virtual ECryLobbyError SessionSearch(uint32 user, SCrySessionSearchParam* param, CryLobbyTaskID* taskID, CryMatchmakingSessionSearchCallback cb, void* cbArg) = 0;

	// SessionJoin
	// Called to join a session previously created by a session host. Session id's can be obtained from a session search or from a session invite notification.
	// users			-	Pointer to an array of pad numbers of the local users who will be joining the session
	// numUsers		- Number of users in the users array
	// id					-	A CrySessionID of the session that will be joined
	// taskID			- Pointer to buffer to store the task ID to identify this call in the callback
	// cb					- Callback function that is called when function completes
	// cbArg			- Pointer to application-specified data that is passed to the callback
	// return			- eCLE_Success if function successfully started or an error code if function failed to start
	virtual ECryLobbyError SessionJoin(uint32* users, int numUsers, CrySessionID id, CryLobbyTaskID* taskID, CryMatchmakingSessionJoinCallback cb, void* cbArg) = 0;

	// SessionJoinFromConsole
	// Attempts to join the first searchable hosted session at the ip:port specified by cl_serveraddr
	// users			-	Pointer to an array of pad numbers of the local users who will be joining the session
	virtual void SessionJoinFromConsole(void) = 0;

	// CancelTask
	// Cancel the given task. The task will still be running in the background but the callback will not be called when it finishes.
	// taskID			- The task to cancel
	virtual void CancelTask(CryLobbyTaskID taskID) = 0;
};



#endif // __ICRYMATCHMAKING_H__
