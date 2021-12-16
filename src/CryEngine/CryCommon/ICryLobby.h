#ifndef __ICRYLOBBY_H__
#define __ICRYLOBBY_H__

#pragma once

#include "CryLobbyEvent.h"		// Seperated out for readability

struct ICryMatchMaking;
struct ICryVoice;
struct ICryTelemetry;
struct ICryStats;

#define CRYLOBBY_USER_NAME_LENGTH	20

typedef uint32 CryLobbyTaskID;
const CryLobbyTaskID CryLobbyInvalidTaskID = 0xffffffff;

struct SCryUserID : public CMultiThreadRefCount
{
};

typedef _smart_ptr<SCryUserID>	CryUserID;
const CryUserID CryUserInvalidID = NULL;

struct SCrySessionID : public CMultiThreadRefCount
{
};

typedef _smart_ptr<SCrySessionID>	CrySessionID;
const CrySessionID CrySessionInvalidID = NULL;

enum ECryLobbyError
{
	eCLE_Success=0,								// Task is successfully started if returned from function and successfully finished if callback is called with this error.
	eCLE_SuccessContinue=1,				// For tasks that return multiple results the callback will be called with this error if there is a valid result and the callback will be called again.
	eCLE_ServiceNotSupported=2,		// The service is not supported on this platform.
	eCLE_AlreadyInitialised=3,		// Service has already been initialised.
	eCLE_NotInitialised=4,				// Service has not been initialised.
	eCLE_TooManyTasks=5,					// The task could not be started because too many tasks are already running.
	eCLE_OutOfMemory=6,						// Not enough memory to complete task.
	eCLE_OutOfSessionUserData=7,	// Trying to register too much session user data
	eCLE_UserDataNotRegistered=8,	// Using a session user data id that has not been registered.
	eCLE_UserDataTypeMissMatch=9,	// Live - The data type of the session user data is not compatible with the data type defined in the xlast program.
	eCLE_TooManySessions=10,			// The session could not be created because there are too many session already created.
	eCLE_InvalidSession=11,				// The specified session handle does not exist.
	eCLE_InvalidRequest=12,				// The task being performed is invalid.
	eCLE_SPAFileOutOfDate=13,			// Live - The SPA file used doesn't match the one on the live servers.
	eCLE_ConnectionFailed=14,			// Connection to session host failed.
	eCLE_SessionFull=15,					// Can't join session because it is full.
	eCLE_SessionWrongState=16,		// The session is in the wrong state for the requested operation to be performed.
	eCLE_UserNotSignedIn=17,			// The user specified is not signed in.
	eCLE_InvalidParam=18,					// An invalid parameter was passed to function.
	eCLE_TimeOut=19,							// The current task has timed out waiting for a response
	eCLE_InsufficientPrivileges=20,	// User had insufficient privileges to perform the requested task. In Live a silver account is being used when a gold account is required.
	eCLE_AlreadyInSession=21,			// Trying to join a session that has already been joined.
	eCLE_LeaderBoardNotRegistered=22,	// Using a leaderboard id that has not been registered.
	eCLE_UserNotInSession = 23,		// Trying to write to a leaderboard for a user who is not in the session.
	eCLE_OutOfUserData=24,				// Trying to register too much user data
	eCLE_NoUserDataRegistered=25,	// Trying to read/write user data when no data has been registered.
	eCLE_ReadDataNotWritten=26,		// Trying to read user data that has never been written.
	eCLE_UserDataMissMatch=27,		// Trying to write user data that is different to registered data.
	eCLE_InvalidUser=28,					// Trying to use an invalid user id.

	eCLE_InternalError,

	eCLE_NumErrors
};

enum ECryLobbyService
{
	eCLS_LAN,
	eCLS_Online,
	eCLS_NumServices
};

typedef uint32 CryLobbyUserDataID;

enum ECryLobbyUserDataType
{
	eCLUDT_Int64,
	eCLUDT_Int32,
	eCLUDT_Int16,
	eCLUDT_Int8,
	eCLUDT_Float64,
	eCLUDT_Float32,
	eCLUDT_Int64NoEndianSwap
};

struct SCryLobbyUserData
{
	CryLobbyUserDataID			m_id;
	ECryLobbyUserDataType		m_type;

	union
	{
		int64									m_int64;
		f64										m_f64;
		int32									m_int32;
		f32										m_f32;
		int16									m_int16;
		int8									m_int8;
	};
};

struct SConfigurationParams
{
	uint32		m_fourCCID;
	union
	{
		uint64	m_64;
		void*		m_pData;
		uint32	m_32;
		uint16	m_16;
		uint8		m_8;
	};
};
typedef void (*CryLobbyConfigurationCallback)(ECryLobbyService service, SConfigurationParams *requestedParams, uint32 paramCount);

typedef void (*CryLobbyCallback)(ECryLobbyService service, ECryLobbyError error, void* arg);

// CryLobbyInviteAcceptedData 
// If the underlying SDK has a frontend invites can be accepted from then this callback will be called if an invite is accepted this way.
// When this call back is called the game must switch to the given service and join the session.
// If the user is in a session already then the old session must be deleted before the join is started.
// Structure returned when a registered callback is triggered for invite acceptance
// service	- Which of the CryLobby services this is for
// user			- id of local user this pertains to
// id				- session identifier of which session to join
struct SCryLobbyInviteAcceptedData
{
	ECryLobbyService	m_service;
	uint32						m_user;
	CrySessionID			m_id;
};

struct ICryLobbyService
{
public:
	virtual ICryMatchMaking*	GetMatchMaking() = 0;
	virtual ICryVoice*				GetVoice() = 0;
	virtual ICryStats*				GetStats() = 0;
	virtual ICryTelemetry*		GetTelemetry() = 0;

	// GetUserID
	// Get the user id of a user signed in to this service locally.
	// user				- The pad number of the local user.
	// return			- The CryUserID of the user.
	virtual	CryUserID					GetUserID(uint32 user) = 0;
};

struct ICryLobby
{
public:
	// Initialise
	// Initialise a lobby service
	// service		- The service to be initialised
	// cb					- Callback function that is called when function completes
	// cbArg			- Pointer to application-specified data that is passed to the callback
	// return			- eCLE_Success if function successfully started or an error code if function failed to start
	virtual ECryLobbyError		Initialise(ECryLobbyService service, CryLobbyConfigurationCallback cfgCb, CryLobbyCallback cb, void* cbArg) = 0;

	// Terminate
	// Shut down a lobby service
	// service		- The service to be shut down
	// cb					- Callback function that is called when function completes
	// cbArg			- Pointer to application-specified data that is passed to the callback
	// return			- eCLE_Success if function successfully started or an error code if function failed to start
	virtual ECryLobbyError		Terminate(ECryLobbyService service, CryLobbyCallback cb, void* cbArg) = 0;

	// SetLobbyService
	// Set the current lobby service
	// service		- The service to use
	// return			- The service that was in use before the call
	virtual ECryLobbyService	SetLobbyService(ECryLobbyService service) = 0;

	// GetLobbyServiceType
	// Get the current lobby service type
	// return			- The ECryLobbyService of the current lobby service
	virtual ECryLobbyService	GetLobbyServiceType() = 0;

	// GetLobbyService
	// Get the current lobby service
	// return			- Pointer to an ICryLobbyService for the current lobby service
	virtual ICryLobbyService*	GetLobbyService() = 0;

	// GetMatchMaking
	// Get the current matchmaking service
	// return			- Pointer to an ICryMatchMaking associated with the current lobby service
	virtual ICryMatchMaking*	GetMatchMaking() = 0;

	// GetVoice
	// Get the current voice service
	// return			- Pointer to an ICryVoice associated with the current lobby service
	virtual ICryVoice*	GetVoice() = 0;

	// GetStats
	// Get the current stats service
	// return			- Pointer to an ICryStats associated with the current lobby service
	virtual ICryStats*	GetStats() = 0;

	// GetLobbyService
	// Get the requested lobby service
	// service		- The service to wanted
	// return			- Pointer to an ICryLobbyService for the lobby service requested
	virtual ICryLobbyService*	GetLobbyService(ECryLobbyService service) = 0;

	// RegisterEventInterest
	// Register interest in particular lobby events (such as invite accepted), the passed in callback will be fired when the event happens.
	// cb					- Callback function that will be called
	// arg				- Pointer to application-specified data
	virtual void RegisterEventInterest(ECryLobbySystemEvent eventOfInterest, CryLobbyEventCallback cb,void *userArg) = 0;
};


#endif // __ICRYLOBBY_H__