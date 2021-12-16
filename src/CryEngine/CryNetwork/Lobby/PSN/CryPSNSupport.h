/*

 Handles a multitude of PSN work including maintaining a state machine of connection to the service.

*/

#ifndef __CRYPSNSUPPORT_H__
#define __CRYPSNSUPPORT_H__

#pragma once

#if USE_PSN

#include <np.h>
#include <netex/net.h>
#include <netex/libnetctl.h>

#define MAX_PSN_SERVER_IDS				(4)				// If we need to go beyond 4 we may have to think about refactoring to avoid overallocations


enum EPSNOnlineState
{													//-- Only use a limited set of these with FlagPendingError()
	ePSNOS_Uninitialised = 0,
	ePSNOS_WaitCallback,							//-- Callback waiting state
	ePSNOS_Offline,									//-- Has nothing set - Must start whole state machine from scratch.
	ePSNOS_DialogFinished,
	ePSNOS_DiscoverNpId,
	ePSNOS_SignedIn,								//-- PSNID is valid, but there is no context
	ePSNOS_ContextReady,
	ePSNOS_ReadyToEnumerateServers,					//-- PSNID is valid, ContextID is valid
	ePSNOS_GettingNextServer,
	ePSNOS_ServerInfoRetrieved,
	ePSNOS_ServerListDone,
	ePSNOS_ServerJoined,
	ePSNOS_WorldInfoRetrieved,
	ePSNOS_Online,									//-- We're connected to a server and world.
};

enum EPSNPendingError
{
	ePSNPE_None = 0,
	ePSNPE_NPLibraryNotInitialised,				// npInit has not been called
	ePSNPE_NetLibraryNotInitialised,			// netCtlInit has not been called
	ePSNPE_NotConnected,						// Failed to establish connection to NP servers
	ePSNPE_InternetDisabled,					// User has disabled internet connection
	ePSNPE_NoPSNAccount,						// User does not have an NP account
	ePSNPE_CableDisconnected,					// User network cable is disconnected
	ePSNPE_SignInCancelled,						// User cancelled sign in process
	ePSNPE_Offline,								// No longer connected to NP servers
	ePSNPE_ContextError,						// Problem with internal context establishment
	ePSNPE_NoServersAvailable,					// There are no servers available (all down for maintainance, or full)
	ePSNPE_NoWorldsAvailable,					// There are no available worlds to join (all full)
	ePSNPE_OutOfMemory,							// Failed to allocate internal resources
	ePSNPE_InternalError,						// Generic error - usually indicates a logic problem in internal state machine
};



class CCryPSNIndividualSessionSupport;

class CCryPSNSupport
{
public:
	CCryPSNSupport(SceNpCommunicationId* pCommId,SceNpCommunicationPassphrase* pPassPhrase, CryLobbySupportCallback cb, void *cbArg, CCryLobby* pLobby);
	~CCryPSNSupport();

	bool								Initialise();

	void								CleanTo(EPSNOnlineState state);
	void								Tick();				// keep psn in check (currently calls cellSysutilCheckCallback - will need moving ultimately)

	bool								ErrorPending() const { return m_pendingErrorCondition != ePSNPE_None; }
	EPSNPendingError					GetPendingError() const { return m_pendingErrorCondition; }
	void								ClearPendingError();

	SceNpMatching2ContextId																			GetContext() const { return m_ctxId; }
	uint32																							GetSignalingContext() const { return m_sigCtxId; }
	const SceNpMatching2World*	const																GetWorldInfo() const { return &m_curWorldInfo; }
	SceNpSignalingNetInfo																			GetNetInfo() const { return m_netInfo; }

	CCryLobby* const					GetLobby() const { return m_pLobby; }
	CCryPSNIndividualSessionSupport*	GetSessionSupport(CrySessionHandle sessionHandle);

	void								HandlePSNError(int retCode);
	void								FlagPendingError(EPSNPendingError errorCode, EPSNOnlineState newState);

private:

	void								TransitionForwards();

	// State Transition Handlers
	void								TransitionToInitialisedOffline();
	void								TransitionToWaitingForDialogOpen();
	void								TransitionToWaitingForDialogClose();
	void								TransitionToSignedIn();
	void								TransitionToContextFound();
	void								TransitionToPeerInfoSet();
	void								TransitionToFirstServer();
	void								TransitionToServerInfoDiscovery();
	void								TransitionToNextServer();
	void								TransitionToServerContextCreated();
	void								TransitionToWorldInfoDiscovery();
	void								TransitionToOnline();

	TMemHdl								GetTemporaryEventDataBufferHandle(SceNpMatching2ContextId id, SceNpMatching2EventKey eventKey, size_t dataSize);

	// Callback handlers
	static void							SystemCallback(uint64 status, uint64 param, void *userData);
	static void							ContextCallback(SceNpMatching2ContextId id,SceNpMatching2Event event,SceNpMatching2EventCause eventCause,int errorCode,void *arg);
	static void							RequestCallback(SceNpMatching2ContextId id,SceNpMatching2RequestId reqId,SceNpMatching2Event myEvent,SceNpMatching2EventKey eventKey,int errorCode,size_t dataSize,void *arg);
	static void							RoomEventCallback(SceNpMatching2ContextId id,SceNpMatching2RoomId roomId,SceNpMatching2Event myEvent,SceNpMatching2EventKey eventKey,int errorCode,size_t dataSize,void *arg);
	static void							SignalingCallback(uint32 id, uint32 subject, int event, int error_code, void *arg);

	//-- Callback to update lobby
	CCryLobby*							m_pLobby;
	CryLobbySupportCallback				m_pSupportCb;
	void*								m_pSupportArg;

	//-- Context Information
	SceNpMatching2ContextId				m_ctxId;
	uint32								m_sigCtxId;

	//-- State machine 
	EPSNOnlineState						m_curState;
	EPSNPendingError					m_pendingErrorCondition;

	//-- Server details
	SceNpMatching2ServerId									m_serverIdsList[MAX_PSN_SERVER_IDS];
	SceNpMatching2GetServerInfoResponse 					m_serverList[MAX_PSN_SERVER_IDS];
	uint32													m_serverIdCur;
	uint32													m_serverIdCnt;

	SceNpMatching2World										m_curWorldInfo;
	TMemHdl													m_worldInfoAllocHandle;

	SceNpMatching2RequestId									m_reqId;
	CellNetCtlNetStartDialogResult							m_dialogResult;

	SceNpId													m_myNpId;					// NpId of signed in user.
	SceNpCommunicationId*									m_pCommId;
	SceNpCommunicationPassphrase*							m_pPassPhrase;
	SceNpSignalingNetInfo									m_netInfo;

	CCryPSNIndividualSessionSupport*						m_pSessionSupporters[MAX_MATCHMAKING_SESSIONS];
};

#endif // USE_PSN

#endif // __CRYPSNSUPPORT_H__
