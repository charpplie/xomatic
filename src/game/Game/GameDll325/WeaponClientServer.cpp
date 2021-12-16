/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$

	-------------------------------------------------------------------------
	History:
	- 15:2:2006   12:50 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Weapon.h"
#include "Actor.h"
#include "Game.h"
#include "GameRules.h"

/*
#define CHECK_OWNER_REQUEST()	\
	{ \
		uint16 channelId=m_pGameFramework->GetGameChannelId(pNetChannel);	\
		IActor *pOwnerActor=GetOwnerActor(); \
		if (!pOwnerActor || pOwnerActor->GetChannelId()!=channelId) \
		{ \
			CryLogAlways("[gamenet] Disconnecting %s. Bogus weapon action '%s' request! %s %d!=%d (%s!=%s)", \
			pNetChannel->GetName(), __FUNCTION__, pOwnerActor?pOwnerActor->GetEntity()->GetName():"null", \
			pOwnerActor?pOwnerActor->GetChannelId():0, channelId,\
			pOwnerActor?pOwnerActor->GetEntity()->GetName():"null", \
			m_pGameFramework->GetIActorSystem()->GetActorByChannelId(channelId)?m_pGameFramework->GetIActorSystem()->GetActorByChannelId(channelId)->GetEntity()->GetName():"null"); \

			return false; \
		} \
	} \
*/

#define CHECK_OWNER_REQUEST()	\
	{ \
		uint16 channelId=m_pGameFramework->GetGameChannelId(pNetChannel);	\
		IActor *pOwnerActor=GetOwnerActor(); \
		if (pOwnerActor && pOwnerActor->GetChannelId()!=channelId && !IsDemoPlayback()) \
			return true; \
	}

//------------------------------------------------------------------------
int CWeapon::NetGetCurrentAmmoCount() const
{
	if (!m_fm)
		return 0;

	return GetAmmoCount(m_fm->GetAmmoType());
}

//------------------------------------------------------------------------
void CWeapon::NetSetCurrentAmmoCount(int count)
{
	if (!m_fm)
		return;

	SetAmmoCount(m_fm->GetAmmoType(), count);
}

//------------------------------------------------------------------------
void CWeapon::NetShoot(const Vec3 &hit, int predictionHandle)
{
	if (m_fm)
		m_fm->NetShoot(hit, predictionHandle);
}

//------------------------------------------------------------------------
void CWeapon::NetShootEx(const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle)
{
	if (m_fm)
		m_fm->NetShootEx(pos, dir, vel, hit, extra, predictionHandle);
}

//------------------------------------------------------------------------
void CWeapon::NetStartFire()
{
	if (m_fm)
		m_fm->NetStartFire();
}

//------------------------------------------------------------------------
void CWeapon::NetStopFire()
{
	if (m_fm)
		m_fm->NetStopFire();
}

//------------------------------------------------------------------------
void CWeapon::NetStartMeleeAttack(bool weaponMelee)
{
	if (weaponMelee && m_melee)
		m_melee->NetStartFire();
	else if (m_fm)
		m_fm->NetStartFire();
}

//------------------------------------------------------------------------
void CWeapon::NetMeleeAttack(bool weaponMelee, const Vec3 &pos, const Vec3 &dir)
{
	if (weaponMelee && m_melee)
	{
		m_melee->NetShootEx(pos, dir, ZERO, ZERO, 1.0f, 0);
		if (IsServer())
			m_pGameplayRecorder->Event(GetOwner(), GameplayEvent(eGE_WeaponMelee, 0, 0, (void *)GetEntityId()));
	}
	else if (m_fm)
	{
		m_fm->NetShootEx(pos, dir, ZERO, ZERO, 1.0f, 0);
		if (IsServer())
			m_pGameplayRecorder->Event(GetOwner(), GameplayEvent(eGE_WeaponMelee, 0, 0, (void *)GetEntityId()));
	}
}

//------------------------------------------------------------------------
void CWeapon::NetZoom(float fov)
{
	if (CActor *pOwner=GetOwnerActor())
	{
		if (pOwner->IsClient())
			return;

		SActorParams *pActorParams = pOwner->GetActorParams();
		if (!pActorParams)
			return;

		pActorParams->viewFoVScale = fov;
	}
}

//------------------------------------------------------------------------
void CWeapon::RequestShoot(IEntityClass* pAmmoType, const Vec3 &pos, const Vec3 &dir, const Vec3 &vel, const Vec3 &hit, float extra, int predictionHandle, bool forceExtended)
{
	GetGameObject()->Pulse('bang');
	CActor *pActor=GetOwnerActor();
	if (pActor)
		pActor->GetGameObject()->Pulse('bang');

	if (gEnv->bClient && pActor && pActor->IsClient())
	{
		if (IsServerSpawn(pAmmoType) || forceExtended)
			GetGameObject()->InvokeRMI(CWeapon::SvRequestShootEx(), SvRequestShootExParams(pos, dir, vel, hit, extra, predictionHandle), eRMI_ToServer);
		else
			GetGameObject()->InvokeRMI(CWeapon::SvRequestShoot(), SvRequestShootParams(pos, dir, hit, predictionHandle), eRMI_ToServer);
	}
	else
	{
		int to=gEnv->bClient?eRMI_ToRemoteClients:eRMI_ToAllClients;
		if (IsServerSpawn(pAmmoType) || forceExtended)
		{
			GetGameObject()->InvokeRMI(CWeapon::ClShoot(), ClShootParams(pos+dir*5.0f, predictionHandle), to);
			if (!gEnv->bClient)
				NetShootEx(pos, dir, vel, hit, extra, predictionHandle);
		}
		else
		{
			GetGameObject()->InvokeRMI(CWeapon::ClShoot(), ClShootParams(hit, predictionHandle), to);
			if (!gEnv->bClient)
				NetShoot(hit, predictionHandle);
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::RequestMeleeAttack(bool weaponMelee, const Vec3 &pos, const Vec3 &dir)
{
	GetGameObject()->Pulse('bang');
	CActor *pActor=GetOwnerActor();
	if (pActor)
		pActor->GetGameObject()->Pulse('bang');

	if (gEnv->bClient && pActor && pActor->IsClient())
		GetGameObject()->InvokeRMI(CWeapon::SvRequestMeleeAttack(), RequestMeleeAttackParams(weaponMelee, pos, dir), eRMI_ToServer);
	else
	{
		int to=gEnv->bClient?eRMI_ToRemoteClients:eRMI_ToAllClients;
		GetGameObject()->InvokeRMI(CWeapon::ClMeleeAttack(), RequestMeleeAttackParams(weaponMelee, pos, dir), to);
		if (!gEnv->bClient)
			NetMeleeAttack(weaponMelee, pos, dir);
	}
}

//------------------------------------------------------------------------
void CWeapon::RequestStartFire()
{
	GetGameObject()->Pulse('bang');
	CActor *pActor=GetOwnerActor();
	if (pActor)
		pActor->GetGameObject()->Pulse('bang');

	if (gEnv->bClient && pActor && pActor->IsClient())
		GetGameObject()->InvokeRMI(CWeapon::SvRequestStartFire(), EmptyParams(), eRMI_ToServer);
	else
	{
		int to=gEnv->bClient?eRMI_ToRemoteClients:eRMI_ToAllClients;
		GetGameObject()->InvokeRMI(CWeapon::ClStartFire(), EmptyParams(), to);
	}
}

//------------------------------------------------------------------------
void CWeapon::RequestStartMeleeAttack(bool weaponMelee)
{
	GetGameObject()->Pulse('bang');
	CActor *pActor=GetOwnerActor();
	if (pActor)
		pActor->GetGameObject()->Pulse('bang');

	if (gEnv->bClient && pActor && pActor->IsClient())
		GetGameObject()->InvokeRMI(CWeapon::SvRequestStartMeleeAttack(), RequestStartMeleeAttackParams(weaponMelee), eRMI_ToServer);
	else
	{
		int to=gEnv->bClient?eRMI_ToRemoteClients:eRMI_ToAllClients;
		GetGameObject()->InvokeRMI(CWeapon::ClStartMeleeAttack(), RequestStartMeleeAttackParams(weaponMelee), to);
		if (!gEnv->bClient)
			NetStartMeleeAttack(weaponMelee);
	}
}

//------------------------------------------------------------------------
void CWeapon::RequestZoom(float fov)
{
	GetGameObject()->Pulse('bang');
	CActor *pActor=GetOwnerActor();
	if (pActor)
		pActor->GetGameObject()->Pulse('bang');

	if (gEnv->bClient && pActor && pActor->IsClient())
		GetGameObject()->InvokeRMI(CWeapon::SvRequestZoom(), ZoomParams(fov), eRMI_ToServer);
	else
	{
		int to=gEnv->bClient?eRMI_ToRemoteClients:eRMI_ToAllClients;
		GetGameObject()->InvokeRMI(CWeapon::ClZoom(), ZoomParams(fov), to);
		if (!gEnv->bClient)
			NetZoom(fov);
	}
}


//------------------------------------------------------------------------
void CWeapon::RequestStopFire()
{
	GetGameObject()->Pulse('bang');
	CActor *pActor=GetOwnerActor();
	if (pActor)
		pActor->GetGameObject()->Pulse('bang');

	if (gEnv->bClient && pActor && pActor->IsClient())
		GetGameObject()->InvokeRMI(CWeapon::SvRequestStopFire(), EmptyParams(), eRMI_ToServer);
	else
	{
		int to=gEnv->bClient?eRMI_ToRemoteClients:eRMI_ToAllClients;
		GetGameObject()->InvokeRMI(CWeapon::ClStopFire(), EmptyParams(), to);
	}
}

//------------------------------------------------------------------------
void CWeapon::RequestReload()
{
	GetGameObject()->Pulse('bang');
	CActor *pActor=GetOwnerActor();
	if (pActor)
		pActor->GetGameObject()->Pulse('bang');

	if (gEnv->bClient && pActor && pActor->IsClient())
		GetGameObject()->InvokeRMI(SvRequestReload(), EmptyParams(), eRMI_ToServer);
	else
	{
		int to=gEnv->bClient?eRMI_ToRemoteClients:eRMI_ToAllClients;
		GetGameObject()->InvokeRMI(CWeapon::ClReload(), EmptyParams(), to);
	}
}

//-----------------------------------------------------------------------
void CWeapon::RequestCancelReload()
{
	GetGameObject()->Pulse('bang');
	CActor *pActor=GetOwnerActor();
	if (pActor)
		pActor->GetGameObject()->Pulse('bang');

	if (gEnv->bClient && pActor && pActor->IsClient())
		GetGameObject()->InvokeRMI(SvRequestCancelReload(), EmptyParams(), eRMI_ToServer);
	else
	{
		int to=gEnv->bClient?eRMI_ToRemoteClients:eRMI_ToAllClients;
		GetGameObject()->InvokeRMI(CWeapon::ClCancelReload(), EmptyParams(), to);
	}
}

//------------------------------------------------------------------------
void CWeapon::RequestFireMode(int fmId)
{
	GetGameObject()->Pulse('bang');
	CActor *pActor=GetOwnerActor();
	if (pActor)
		pActor->GetGameObject()->Pulse('bang');

	if (gEnv->bClient && pActor && pActor->IsClient())
	{
		if (gEnv->bServer)
			SetCurrentFireMode(fmId);	// serialization will fix the rest.
		else
			GetGameObject()->InvokeRMI(SvRequestFireMode(), SvRequestFireModeParams(fmId), eRMI_ToServer);
	}
}

//------------------------------------------------------------------------
void CWeapon::RequestLock(EntityId id, int partId)
{
	IActor *pActor=m_pGameFramework->GetClientActor();
	if (!pActor || pActor->IsClient())
	{
		if (gEnv->bServer)
		{
			if (m_fm)
				m_fm->Lock(id, partId);

			GetGameObject()->InvokeRMI(CWeapon::ClLock(), LockParams(id, partId), eRMI_ToRemoteClients);
		}
		else
			GetGameObject()->InvokeRMI(SvRequestLock(), LockParams(id, partId), eRMI_ToServer);
	}
}

//------------------------------------------------------------------------
void CWeapon::RequestUnlock()
{
	IActor *pActor=m_pGameFramework->GetClientActor();
	if (!pActor || pActor->IsClient())
		GetGameObject()->InvokeRMI(SvRequestUnlock(), EmptyParams(), eRMI_ToServer);
}

//------------------------------------------------------------------------
void CWeapon::RequestWeaponRaised(bool raise)
{
	if(gEnv->bMultiplayer)
	{
		CActor* pActor = GetOwnerActor();
		if(pActor && pActor->IsClient())
		{
			if (gEnv->bServer)
				GetGameObject()->InvokeRMI(ClWeaponRaised(), WeaponRaiseParams(raise), eRMI_ToRemoteClients|eRMI_NoLocalCalls);
			else
				GetGameObject()->InvokeRMI(SvRequestWeaponRaised(), WeaponRaiseParams(raise), eRMI_ToServer);
		}
	}
}

//------------------------------------------------------------------------
void CWeapon::SendEndReload()
{
	GetGameObject()->Pulse('bang');
	if (CActor *pActor=GetOwnerActor())
		pActor->GetGameObject()->Pulse('bang');

	int channelId=0;
	if (CActor* pActor = GetOwnerActor())
		channelId=pActor->GetChannelId();

	GetGameObject()->InvokeRMI(ClEndReload(), EmptyParams(), eRMI_ToClientChannel|eRMI_NoLocalCalls, channelId);
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestStartFire)
{
	CHECK_OWNER_REQUEST();

	GetGameObject()->InvokeRMI(CWeapon::ClStartFire(), params, eRMI_ToOtherClients|eRMI_NoLocalCalls, 
		m_pGameFramework->GetGameChannelId(pNetChannel));

	CActor *pActor=GetActorByNetChannel(pNetChannel);
	IActor *pLocalActor=m_pGameFramework->GetClientActor();
	bool isLocal = pLocalActor && pActor && (pLocalActor->GetChannelId() == pActor->GetChannelId());

	if (!isLocal)
		NetStartFire();

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestStopFire)
{
	CHECK_OWNER_REQUEST();

	GetGameObject()->InvokeRMI(CWeapon::ClStopFire(), params, eRMI_ToOtherClients|eRMI_NoLocalCalls, 
		m_pGameFramework->GetGameChannelId(pNetChannel));

	CActor *pActor=GetActorByNetChannel(pNetChannel);
	IActor *pLocalActor=m_pGameFramework->GetClientActor();
	bool isLocal = pLocalActor && pActor && (pLocalActor->GetChannelId() == pActor->GetChannelId());

	if (!isLocal)
		NetStopFire();

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClStartFire)
{
	NetStartFire();

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClStopFire)
{
	NetStopFire();

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestShoot)
{
	CHECK_OWNER_REQUEST();

	bool ok=true;
	CActor *pActor=GetActorByNetChannel(pNetChannel);
	if (!pActor || pActor->GetHealth()<=0)
		ok=false;

	if (ok)
	{
		if (IActor *pOwner=GetOwnerActor())
			pOwner->GetGameObject()->Pulse('bang');
		GetGameObject()->Pulse('bang');

		static ray_hit rh;

		IEntity* pEntity = NULL;
		if ( gEnv->pPhysicalWorld->RayWorldIntersection(params.pos, params.dir*4096.0f, ent_all & ~ent_terrain, rwi_stop_at_pierceable|rwi_ignore_back_faces, &rh, 1) )
			pEntity = gEnv->pEntitySystem->GetEntityFromPhysics(rh.pCollider);
		if (pEntity)
		{
			if(INetContext* pNC = gEnv->pGame->GetIGameFramework()->GetNetContext())
			{
				if(pNC->IsBound(pEntity->GetId()))
				{
					AABB bbox; pEntity->GetWorldBounds(bbox);
					bool hit0 = bbox.GetRadius() < 1.0f; // this (radius*2) must match the value in CompressionPolicy.xml ("hit0")
					Vec3 hitLocal = pEntity->GetWorldTM().GetInvertedFast() * rh.pt;
					//GetGameObject()->InvokeRMI(CWeapon::ClShootX(), ClShootXParams(pEntity->GetId(), hit0, hitLocal, params.predictionHandle),
					//	eRMI_ToOtherClients|eRMI_NoLocalCalls, m_pGameFramework->GetGameChannelId(pNetChannel));
					GetGameObject()->InvokeRMIWithDependentObject(CWeapon::ClShootX(), ClShootXParams(pEntity->GetId(), hit0, hitLocal, params.predictionHandle),
						eRMI_ToOtherClients|eRMI_NoLocalCalls, pEntity->GetId(), m_pGameFramework->GetGameChannelId(pNetChannel));

				}
			}
		}
		else
			GetGameObject()->InvokeRMI(CWeapon::ClShoot(), ClShootParams(params.hit, params.predictionHandle), 
				eRMI_ToOtherClients|eRMI_NoLocalCalls, m_pGameFramework->GetGameChannelId(pNetChannel));

		IActor *pLocalActor=m_pGameFramework->GetClientActor();
		bool isLocal = pLocalActor && (pLocalActor->GetChannelId() == pActor->GetChannelId());

		if (!isLocal)
			NetShoot(params.hit, params.predictionHandle);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestShootEx)
{
	CHECK_OWNER_REQUEST();

	bool ok=true;
	CActor *pActor=GetActorByNetChannel(pNetChannel);
	if (!pActor || pActor->GetHealth()<=0)
		ok=false;

	if (ok)
	{
		if (IActor *pOwner=GetOwnerActor())
			pOwner->GetGameObject()->Pulse('bang');
		GetGameObject()->Pulse('bang');

		GetGameObject()->InvokeRMI(CWeapon::ClShoot(), ClShootParams(params.pos+params.dir*5.0f, params.predictionHandle),
			eRMI_ToOtherClients|eRMI_NoLocalCalls, m_pGameFramework->GetGameChannelId(pNetChannel));

		IActor *pLocalActor=m_pGameFramework->GetClientActor();
		bool isLocal = pLocalActor && (pLocalActor->GetChannelId() == pActor->GetChannelId());

		if (!isLocal)
			NetShootEx(params.pos, params.dir, params.vel, params.hit, params.extra, params.predictionHandle);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClShoot)
{
	NetShoot(params.hit, params.predictionHandle);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClShootX)
{
	if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity(params.eid))
	{
		Vec3 hit = pEntity->GetWorldTM() * params.hit;
		NetShoot(hit, params.predictionHandle);
	}
	else
	{
		GameWarning("ClShootX: invalid entity id %.8x", params.eid);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestStartMeleeAttack)
{
	CHECK_OWNER_REQUEST();

	GetGameObject()->InvokeRMI(CWeapon::ClStartMeleeAttack(), params, eRMI_ToOtherClients, 
		m_pGameFramework->GetGameChannelId(pNetChannel));

	CActor *pActor=GetActorByNetChannel(pNetChannel);
	IActor *pLocalActor=m_pGameFramework->GetClientActor();
	bool isLocal = pLocalActor && pActor && (pLocalActor->GetChannelId() == pActor->GetChannelId());

	if (!isLocal)
		NetStartMeleeAttack(params.wmelee);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClStartMeleeAttack)
{
	NetStartMeleeAttack(params.wmelee);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestMeleeAttack)
{
	CHECK_OWNER_REQUEST();

	bool ok=true;
	CActor *pActor=GetActorByNetChannel(pNetChannel);
	if (!pActor || pActor->GetHealth()<=0)
		ok=false;

	if (ok)
	{
		GetGameObject()->InvokeRMI(CWeapon::ClMeleeAttack(), params,
			eRMI_ToOtherClients|eRMI_NoLocalCalls, m_pGameFramework->GetGameChannelId(pNetChannel));

		IActor *pLocalActor=m_pGameFramework->GetClientActor();
		bool isLocal = pLocalActor && (pLocalActor->GetChannelId() == pActor->GetChannelId());

		if (!isLocal)
			NetMeleeAttack(params.wmelee, params.pos, params.dir);

		m_pGameplayRecorder->Event(GetOwner(), GameplayEvent(eGE_WeaponMelee, 0, 0, (void *)GetEntityId()));
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClMeleeAttack)
{
	NetMeleeAttack(params.wmelee, params.pos, params.dir);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestZoom)
{
	CHECK_OWNER_REQUEST();

	bool ok=true;
	CActor *pActor=GetActorByNetChannel(pNetChannel);
	if (!pActor || pActor->GetHealth()<=0)
		ok=false;

	if (ok)
	{
		GetGameObject()->InvokeRMI(CWeapon::ClZoom(), params,
			eRMI_ToOtherClients|eRMI_NoLocalCalls, m_pGameFramework->GetGameChannelId(pNetChannel));

		IActor *pLocalActor=m_pGameFramework->GetClientActor();
		bool isLocal = pLocalActor && (pLocalActor->GetChannelId() == pActor->GetChannelId());

		if (!isLocal)
			NetZoom(params.fov);

		int event=eGE_ZoomedOut;
		if (params.fov<0.99f)
			event=eGE_ZoomedIn;
		m_pGameplayRecorder->Event(GetOwner(), GameplayEvent(event, 0, 0, (void *)GetEntityId()));
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClZoom)
{
	NetZoom(params.fov);

	return true;
}


//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestFireMode)
{
	CHECK_OWNER_REQUEST();

	SetCurrentFireMode(params.id);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClSetFireMode)
{
	SetCurrentFireMode(params.id);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestReload)
{
	CHECK_OWNER_REQUEST();
	
	bool ok=true;
	CActor *pActor=GetActorByNetChannel(pNetChannel);
	if (!pActor || pActor->GetHealth()<=0)
		ok=false;

	if (ok)
	{
		GetGameObject()->InvokeRMI(CWeapon::ClReload(), params, eRMI_ToOtherClients|eRMI_NoLocalCalls, m_pGameFramework->GetGameChannelId(pNetChannel));

		IActor *pLocalActor=m_pGameFramework->GetClientActor();
		bool isLocal = pLocalActor && (pLocalActor->GetChannelId() == pActor->GetChannelId());

		if (!isLocal && m_fm)
			m_fm->Reload(0);
		
		
//		m_pGameplayRecorder->Event(GetOwner(), GameplayEvent(eGE_WeaponReload, m_fm->GetAmmoType()->GetName(), m_fm->GetClipSize(), (void *)GetEntityId()));
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClReload)
{
	if (m_fm)
	{
		if(m_zm)
			m_fm->Reload(m_zm->GetCurrentStep());
		else
			m_fm->Reload(false);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClEndReload)
{
	if(m_fm)
		m_fm->NetEndReload();

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestCancelReload)
{
	CHECK_OWNER_REQUEST();

	if(m_fm)
	{
		m_fm->CancelReload();
		GetGameObject()->InvokeRMI(CWeapon::ClCancelReload(), params, eRMI_ToRemoteClients);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClCancelReload)
{
	if(m_fm)
		m_fm->CancelReload();

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClLock)
{
	if (m_fm)
		m_fm->Lock(params.entityId, params.partId);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClUnlock)
{
	if (m_fm)
		m_fm->Unlock();

	return true;
}


//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestLock)
{
	CHECK_OWNER_REQUEST();

	if (m_fm)
		m_fm->Lock(params.entityId, params.partId);

	GetGameObject()->InvokeRMI(CWeapon::ClLock(), params, eRMI_ToRemoteClients);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestUnlock)
{
	CHECK_OWNER_REQUEST();

	if (m_fm)
		m_fm->Unlock();

	GetGameObject()->InvokeRMI(CWeapon::ClUnlock(), params, eRMI_ToRemoteClients);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, SvRequestWeaponRaised)
{
	CHECK_OWNER_REQUEST();

	GetGameObject()->InvokeRMI(CWeapon::ClWeaponRaised(), params, eRMI_ToAllClients);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CWeapon, ClWeaponRaised)
{
	CActor* pActor = GetOwnerActor();
	if(pActor && !pActor->IsClient())
		RaiseWeapon(params.raise);

	return true;
}

