#ifndef __ICRYLOBBYEVENT_H__
#define __ICRYLOBBYEVENT_H__

#pragma once

enum ECryLobbySystemEvent
{
	eCLSE_EthernetState,			// returns CryLobbyEthernetStateData	(gives status of ethernet cable e.g. unplugged)
	eCLSE_OnlineState,				// returns CryLobbyOnlineStateData		(gives status of online connection e.g. signed in)
	eCLSE_InviteAccepted,			// returns CryLobbyInviteAcceptedData
	eCLSE_NatType							// returns CryLobbyNatTypeData
};

enum ECableState
{
	eCS_Unknown=-1,
	eCS_Unplugged=0,
	eCS_Connected,
};

enum EOnlineState
{
	eOS_Unknown=-1,
	eOS_SignedOut=0,
	eOS_SigningIn,
	eOS_SignedIn,
};

enum ENatType
{
	eNT_Unknown=-1,
	eNT_Open=0,
	eNT_Moderate,
	eNT_Strict
};

// CryLobbyEthernetStateData
// curState will indicate the new state of the ethernet interface
struct SCryLobbyEthernetStateData
{
	ECableState				m_curState;
};

// CryLobbyOnlineStateData
// curState will indicate the new state of the user identified by user (expect multiple of these)
struct SCryLobbyOnlineStateData
{
	uint32						m_user;
	EOnlineState			m_curState;
};

struct SCryLobbyInviteAcceptedData;			// Defined in ICryLobby

// SCryLobbyNatTypeData
// m_curState - Current nat type
struct SCryLobbyNatTypeData
{
	ENatType					m_curState;
};

union UCryLobbyEventData
{
	SCryLobbyEthernetStateData*		pEthernetStateData;
	SCryLobbyOnlineStateData*			pOnlineStateData;
	SCryLobbyInviteAcceptedData*	pInviteAcceptedData;
	SCryLobbyNatTypeData*					pNatTypeData;
};

typedef void (*CryLobbyEventCallback)(UCryLobbyEventData eventData, void *userParam);

#endif // __ICRYLOBBYEVENT_H__