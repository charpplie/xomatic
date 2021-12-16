/*

 Handles a multitude of PSN work including maintaining a state machine of connection to the service.

*/

#ifndef __CRYPSNSESSIONSUPPORT_H__
#define __CRYPSNSESSIONSUPPORT_H__

#pragma once

#if USE_PSN

#include <np.h>
#include <netex/net.h>
#include <netex/libnetctl.h>

#define MAX_PSN_ROOM_MEMBERS			(16)			// enough for c2


enum EPSNTask
{
	ePSNT_None,
	ePSNT_Search,
	ePSNT_Create,
	ePSNT_Join,
	ePSNT_Leave,
};

enum ePSNIndividualSessionState
{
	ePSNIIS_NothingInProgress = 0,

	ePSNIIS_StartSearchRoom,
	ePSNIIS_WaitingForSearchRoomCallback,
	ePSNIIS_RoomSearchResultsAvailable,

	ePSNIIS_StartCreateRoom,
	ePSNIIS_WaitingForCreateRoomCallback,
	ePSNIIS_WaitingForCreateRoomSignaling,
	ePSNIIS_RoomCreated,

	ePSNIIS_StartJoinRoom,
	ePSNIIS_WaitingForJoinRoomCallback,
	ePSNIIS_WaitingForJoinRoomSignaling,
	ePSNIIS_RoomJoined,

	ePSNIIS_StartLeaveRoom,
	ePSNIIS_WaitingForLeaveRoomCallback,
	ePSNIIS_RoomLeft,
};

enum EPSNMemberInfo
{
	ePSNMI_None				= 0,
	ePSNMI_Me				= BIT(0),
	ePSNMI_Owner			= BIT(1),
	ePSNMI_MeOwner			= ePSNMI_Me | ePSNMI_Owner,
	ePSNMI_Other			= BIT(2),
	ePSNMI_OtherOwner		= ePSNMI_Other | ePSNMI_Owner,
};

enum EPSNConnectionState
{
	ePSNCS_None,
	ePSNCS_Pending,
	ePSNCS_Dead,
	ePSNCS_Active
};

struct SRoomMember
{
	SceNpId										m_npId;
	uint32										m_conId;
	EPSNConnectionState							m_conState;
	in_addr										m_peerAddr;
	in_port_t									m_peerPort;
	SceNpMatching2RoomMemberId					m_memberId;
	EPSNMemberInfo								m_valid;
	CTimeValue									m_conTimer;
};

enum ECallbackEvent
{
	eCE_ConnectionAdd,
	eCE_ConnectionRemove,
	eCE_ConnectionUpdate,
	eCE_ConnectionOldOwner,
	eCE_ConnectionNewOwner
};

struct SCallbackEventData
{
	CrySessionHandle											m_sessionHandle;
	union
	{
		const SRoomMember*										m_pMemberInfo;
	};
};

typedef void (*CryLobbySupportCallback)(ECallbackEvent ecb, const SCallbackEventData &data, void *userArg);


class CCryPSNSupport;

class CCryPSNIndividualSessionSupport
{
	public:

		struct SSearchParamData
		{
			SceNpMatching2SearchRoomRequest				m_searchRequest;
			SceNpMatching2AttributeId					m_attrId[MAX_ATTRIB_IDS];
		};

		struct SCreateParamData
		{
			SceNpMatching2CreateJoinRoomRequest			m_createRequest;
			SceNpMatching2BinAttr						m_binAttrExternal[SCE_NP_MATCHING2_ROOM_BIN_ATTR_INTERNAL_NUM];
			SceNpMatching2IntAttr						m_intAttrExternal[SCE_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_NUM];
			char										m_binAttrExternalData[SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_MAX_SIZE * SCE_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_NUM];
			SceNpMatching2BinAttr						m_binAttrInternal;
			char										m_binAttrInternalData[SCE_NP_MATCHING2_ROOM_BIN_ATTR_INTERNAL_MAX_SIZE];
		};

		CCryPSNIndividualSessionSupport(CrySessionHandle sessionHandle, CCryPSNSupport* pPSNSupporter, CryLobbySupportCallback cb, void *cbArg);
		~CCryPSNIndividualSessionSupport();

		void Clean();
		void Tick();

		bool HandleRequestEvent(SceNpMatching2ContextId id, SceNpMatching2RequestId reqId, SceNpMatching2Event myEvent, SceNpMatching2EventKey eventKey, size_t dataSize);

	SSearchParamData&																				AccessSearchParamsData() { return m_searchParam; }
	const	SceNpMatching2SearchRoomResponse* const													GetSearchResult() const;
	void																							ClearSearchResult();

	SCreateParamData&																				AccessCreateParamsData() { return m_createParam; }
	const	SceNpMatching2CreateJoinRoomResponse* const												GetCreateResult() const;
	void																							ClearCreateResult();

	SceNpMatching2JoinRoomRequest&																	AccessJoinParamsData() { return m_joinParam; }
	const	SceNpMatching2JoinRoomResponse* const													GetJoinResult() const;
	void																							ClearJoinResult();

	// There are no results for leave room
	SceNpMatching2LeaveRoomRequest&																	AccessLeaveParamsData() { return m_leaveParam; }
	void																							ClearLeaveResult();

	bool														ReadyForTask() const { return (m_curTask == ePSNT_None); }
	bool														StartTask(EPSNTask taskType);
	bool														TaskComplete(EPSNTask taskType);
	void														AbortTask(EPSNTask taskType);
	bool																							InTheDesiredState();

	SceNpMatching2RoomId																			GetRoomId() const { return m_roomId; }

	void																							ClearMemberInfo();
	SceNpMatching2RoomMemberId																		GetMyMemberId() const { return m_myMemberId; }
	SceNpMatching2RoomMemberId																		GetOwnerMemberId() const { return m_ownerMemberId; }
	const SRoomMember* const																		GetMemberFromRoomMemberId(SceNpMatching2RoomMemberId memberId);
	SRoomMember* const																				GetMemberFromConnectionId(uint32 conId);
	void								AddRoomMember(const SceNpMatching2RoomMemberDataInternal* pMemberInfo, EPSNMemberInfo info);
	void								RemoveRoomMember(const SceNpMatching2RoomMemberDataInternal* pMemberInfo);
	void								ChangeRoomOwner(SceNpMatching2RoomMemberId prevOwner, SceNpMatching2RoomMemberId newOwner);


private:

	EPSNConnectionState					CheckSignaling();
	void								UpdateSignaling();

	//-- session state machine
	void								TransitionToSearching();
	void								TransitionToCreating();
	void								TransitionToJoining();
	void								TransitionToLeaving();

	void								AddInitialRoomMembers(const SceNpMatching2RoomMemberDataInternalList* pMemberList);
	void								RemoveAllRoomMembers();

	CrySessionHandle					m_sessionHandle;
	CCryPSNSupport*						m_pPSNSupport;

	//-- Callback to update lobby
	CryLobbySupportCallback				m_pSupportCb;
	void*								m_pSupportArg;

	EPSNTask							m_curTask;
	ePSNIndividualSessionState			m_curState;
	ePSNIndividualSessionState			m_wantedState;
	SceNpMatching2RequestId				m_sessionReqId;

	SRoomMember										m_members[MAX_PSN_ROOM_MEMBERS];
	SceNpMatching2RoomMemberId						m_myMemberId;
	SceNpMatching2RoomMemberId						m_ownerMemberId;
	SceNpMatching2RoomId							m_roomId;

	SceNpMatching2RequestId							m_reqId;

	SSearchParamData								m_searchParam;
	SCreateParamData								m_createParam;
	SceNpMatching2JoinRoomRequest					m_joinParam;
	SceNpMatching2LeaveRoomRequest					m_leaveParam;

	TMemHdl											m_searchResponseAllocHandle;
	TMemHdl											m_createResponseAllocHandle;
	TMemHdl											m_joinResponseAllocHandle;
};



#endif // USE_PSN

#endif // __CRYPSNSESSIONSUPPORT_H__
