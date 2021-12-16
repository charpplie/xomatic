/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description:	Implementation of the IGameContext interface. 
					MiniGameContext provides a basic implementation
  
 -------------------------------------------------------------------------
  History:
  - 14:2:2010	11:00 : Created by Christian Helmich

*************************************************************************/

#include "StdAfx.h"
#include "MiniGameContext.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//- ctor & dtor

simpleframework::CMiniGameContext::CMiniGameContext()
{
	LOG_CODE_COVERAGE();
}

//-----------------------------------

simpleframework::CMiniGameContext::~CMiniGameContext()
{
	LOG_CODE_COVERAGE();
}

//- ctor & dtor/
//////////////////////////////////////////////////////////////////////////
//- IGameFramework implementation

//-- no idea #1
bool	simpleframework::CMiniGameContext::InitGlobalEstablishmentTasks(IContextEstablisher* pEst, int establishedToken)
{
	LOG_CODE_COVERAGE();
	return false;
}

//-----------------------------------

bool	simpleframework::CMiniGameContext::InitChannelEstablishmentTasks(IContextEstablisher* pEst, INetChannel* pChannel, int establishedToken)
{
	LOG_CODE_COVERAGE();
	return false;
}

//////////////////////////////////////////////////////////////////////////
//-- object

INetSendableHookPtr	simpleframework::CMiniGameContext::CreateObjectSpawner(EntityId id, INetChannel* pChannel)
{
	LOG_CODE_COVERAGE();
	return 0;
}

//-----------------------------------

bool	simpleframework::CMiniGameContext::SendPostSpawnObject(EntityId id, INetChannel* pChannel)
{
	LOG_CODE_COVERAGE();
	return false;
}

//-----------------------------------


void	simpleframework::CMiniGameContext::ControlObject(EntityId id, bool bHaveControl)
{
	LOG_CODE_COVERAGE();	
}

//-----------------------------------


ESynchObjectResult	simpleframework::CMiniGameContext::SynchObject(EntityId id, NetworkAspectType nAspect, uint8 nCurrentProfile, TSerialize ser, bool verboseLogging)
{
	LOG_CODE_COVERAGE();
	return eSOR_Failed;
}

//-----------------------------------


void	simpleframework::CMiniGameContext::BoundObject(EntityId id, NetworkAspectType nAspects)
{
	LOG_CODE_COVERAGE();
}

//-----------------------------------


void	simpleframework::CMiniGameContext::UnboundObject(EntityId id)
{
	LOG_CODE_COVERAGE();
}

//-- object/
//////////////////////////////////////////////////////////////////////////
//-- no idea #2

INetAtSyncItem*	simpleframework::CMiniGameContext::HandleRMI(bool bClient, EntityId objID, uint8 funcID, TSerialize ser, INetChannel* pChannel)
{
	LOG_CODE_COVERAGE();
	return 0;
}

//-----------------------------------

void	simpleframework::CMiniGameContext::PassDemoPlaybackMappedOriginalServerPlayer(EntityId id)
{
	LOG_CODE_COVERAGE();
}

//-- no idea #2/
//////////////////////////////////////////////////////////////////////////
//-- aspect

bool	simpleframework::CMiniGameContext::SetAspectProfile(EntityId id, NetworkAspectType nAspect, uint8 nProfile)
{
	LOG_CODE_COVERAGE();
	return false;
}

//-----------------------------------


uint8	simpleframework::CMiniGameContext::GetDefaultProfileForAspect(EntityId id, NetworkAspectType aspectID)
{
	LOG_CODE_COVERAGE();
	return 0;
}

//-----------------------------------


uint32	simpleframework::CMiniGameContext::HashAspect(EntityId id, NetworkAspectType nAspect)
{
	LOG_CODE_COVERAGE();
	return 0;
}

//-- aspect/
//////////////////////////////////////////////////////////////////////////
//-- object update

void	simpleframework::CMiniGameContext::BeginUpdateObjects(CTimeValue physTime, INetChannel* pChannel)
{
	LOG_CODE_COVERAGE();
}

//-----------------------------------


void	simpleframework::CMiniGameContext::EndUpdateObjects()
{
	LOG_CODE_COVERAGE();
}

//-- object update/
//////////////////////////////////////////////////////////////////////////
//-- network frame

void	simpleframework::CMiniGameContext::OnStartNetworkFrame()
{
	LOG_CODE_COVERAGE();
}

//-----------------------------------


void	simpleframework::CMiniGameContext::OnEndNetworkFrame()
{
	LOG_CODE_COVERAGE();
}

//-- network frame/
//////////////////////////////////////////////////////////////////////////
//-- no idea #3

CTimeValue	simpleframework::CMiniGameContext::GetPhysicsTime()
{
	LOG_CODE_COVERAGE();
	return CTimeValue();
}

//-----------------------------------

void	simpleframework::CMiniGameContext::PlaybackBreakage(int breakId, INetBreakagePlaybackPtr pBreakage)
{
	LOG_CODE_COVERAGE();
}

//-- no idea #3/
//////////////////////////////////////////////////////////////////////////
//-- no idea #4

string	simpleframework::CMiniGameContext::GetConnectionString(bool fake) const
{
	LOG_CODE_COVERAGE();
	return "";
}

//-- no idea #4/
//////////////////////////////////////////////////////////////////////////
//-- no idea #5

void	simpleframework::CMiniGameContext::CompleteUnbind(EntityId id)
{
	LOG_CODE_COVERAGE();
}

//-- no idea #5/
//////////////////////////////////////////////////////////////////////////
//-- memory statistics

void	simpleframework::CMiniGameContext::GetMemoryStatistics(ICrySizer* pSizer)
{
	LOG_CODE_COVERAGE();
	//CHILD_MEM_STATISTICS
}

//-- memory statistics/

//- IGameFramework implementation/
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
