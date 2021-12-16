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

#ifndef	__MINI_GAME_CONTEXT_H__
#define __MINI_GAME_CONTEXT_H__

#include <IGameFramework.h>
#include <IEntitySystem.h>
#include <INetworkService.h>
#include "../GameCore/GameConfig.h"
#include "../GameGlobals/GameCVars.h"


namespace simpleframework
{
	class CMiniGameContext :
		public IGameContext
		//,
// 		public IGameQuery,
// 		public IEntitySystemSink,
// 		public IConsoleVarSink,
// 		public IHostMigrationEventListener
	{
	public:
		CMiniGameContext();
		VIRTUAL	~CMiniGameContext();


		//IGameContext implementation
		//- no idea #1
		VIRTUAL	bool		InitGlobalEstablishmentTasks(IContextEstablisher* pEst, int establishedToken);
		VIRTUAL	bool		InitChannelEstablishmentTasks(IContextEstablisher* pEst, INetChannel* pChannel, int establishedToken);
		
		//- object
		VIRTUAL	INetSendableHookPtr	CreateObjectSpawner(EntityId id, INetChannel* pChannel);
		VIRTUAL	bool				SendPostSpawnObject(EntityId id, INetChannel* pChannel);
		VIRTUAL	void				ControlObject(EntityId id, bool bHaveControl);
		VIRTUAL	ESynchObjectResult	SynchObject(EntityId id, NetworkAspectType nAspect, uint8 nCurrentProfile, TSerialize ser, bool verboseLogging);
		VIRTUAL	void				BoundObject(EntityId id, NetworkAspectType nAspects);
		VIRTUAL	void				UnboundObject(EntityId id);
		
		//- no idea #2
		VIRTUAL	INetAtSyncItem*	HandleRMI(bool bClient, EntityId objID, uint8 funcID, TSerialize ser, INetChannel* pChannel);
		VIRTUAL	void		PassDemoPlaybackMappedOriginalServerPlayer(EntityId id);
				
		//- aspect
		VIRTUAL	bool		SetAspectProfile(EntityId id, NetworkAspectType nAspect, uint8 nProfile);		
		VIRTUAL	uint8		GetDefaultProfileForAspect(EntityId id, NetworkAspectType aspectID);
		VIRTUAL	uint32		HashAspect(EntityId id, NetworkAspectType nAspect);

		//- object update		
		VIRTUAL	void		BeginUpdateObjects(CTimeValue physTime, INetChannel* pChannel);
		VIRTUAL	void		EndUpdateObjects();

		//- network frame
		VIRTUAL	void		OnStartNetworkFrame();
		VIRTUAL	void		OnEndNetworkFrame();		

		//- no idea #3
		VIRTUAL	CTimeValue	GetPhysicsTime();		
		VIRTUAL	void		PlaybackBreakage(int breakId, INetBreakagePlaybackPtr pBreakage);

		//- no idea #4
		VIRTUAL	string		GetConnectionString(bool fake = false) const;

		//- no idea #5
		VIRTUAL	void		CompleteUnbind(EntityId id);

		//- memory statistics
		VIRTUAL	void		GetMemoryStatistics(ICrySizer* pSizer);
		//IGameContext implementation/
	private:

	};

} //namespace simpleframework

#endif //__MINI_GAME_CONTEXT_H__