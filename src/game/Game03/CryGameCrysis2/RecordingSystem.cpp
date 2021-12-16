#include "StdAfx.h"
#include "RecordingSystem.h"
#include "Game.h"
#include "IUIDraw.h"
#include "GameCVars.h"
#include "ScreenEffects.h"

#include <IViewSystem.h>
#include <list>

#include <stdio.h>
#include <platform.h>
#include "Actor.h"
#include "Player.h"
#include "Item.h"
#include "GameRules.h"

#include "IWeapon.h"
#include "Weapon.h"
#include "Single.h"

#include "HUD/HUD.h"

#include <IEntitySystem.h>
#include <ICryAnimation.h>

#include "HUD/HUD_Impl.h"
#include "Graphics/2DRenderUtils.h"
#include "HUD/ScreenLayoutManager.h"
#include "IVehicleSystem.h"
#include "WeaponSystem.h"
#include "Projectile.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"
#include "ThreatDetectorPerk.h"
#include "HMG.h"

#if 1
#define DBG_FNAME		do { CryLog("%s():%d", __FUNC__, __LINE__); } while (0);
#define DBG_FNAME_ALWAYS		do { CryLogAlways("%s():%d", __FUNC__, __LINE__); } while (0);
#else
#define DBG_FNAME	
#endif

#define RECORDING_SYSTEM_ENABLE		(1)
#ifdef WIN32
#define RECORDING_DEBUG		(1)
#else
#define RECORDING_DEBUG		(0)
#endif

#if RECORDING_DEBUG
static bool s_mallocFail = false;

void *TestMalloc(size_t size, size_t &allocated)
{
	if (s_mallocFail)
	{
		allocated = 0;
		return NULL;
	}

	allocated = size;
	return new char[size];
}
#define		UNIT_TEST_PASS_MALLOC()					do {s_mallocFail = false;} while (0);
#define		UNIT_TEST_FAIL_MALLOC()					do {s_mallocFail = true;} while (0);
#define		UNIT_TEST_MALLOC(size, allocated)		TestMalloc(size, allocated)

static int s_assertCount = 0;

void TestAssert(int val, char *message)
{
	s_assertCount++;
//	CRY_ASSERT_MESSAGE(val, message);
}

#define		UNIT_TEST_CLEAR_ASSERTS()				do {s_assertCount = 0;} while (0);
#define		UNIT_TEST_ASSERT(val, mess)				TestAssert((int) val, mess)

#endif

// --------------------------------------------------------------------------------

class CWeaponEventRecorder : public IWeaponEventListener
{
	virtual void OnShoot(IWeapon *pWeapon, EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType, const Vec3 &pos, const Vec3 &dir, const Vec3 &vel)
	{
		CRecordingSystem *crs = g_pGame->GetRecordingSystem();
		if (crs)
		{
			SRecording_OnShoot packet;
			packet.shooter = shooterId;
			crs->AddPacket(packet);
		}
	}
	virtual void OnStartFire(IWeapon *pWeapon, EntityId shooterId)
	{
		CryLog("OnStartFire, shooter id %d", shooterId);
	}
	virtual void OnStopFire(IWeapon *pWeapon, EntityId shooterId)
	{
		CryLog("OnStopFire, shooter id %d", shooterId);
	}
	virtual void OnStartReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType) {};
	virtual void OnEndReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType) {};
	virtual void OnOutOfAmmo(IWeapon *pWeapon, IEntityClass* pAmmoType) {};
	virtual void OnSetAmmoCount(IWeapon *pWeapon, EntityId shooterId) {};
	virtual void OnReadyToFire(IWeapon *pWeapon) {};

	virtual void OnPickedUp(IWeapon *pWeapon, EntityId actorId, bool destroyed) {};
	virtual void OnDropped(IWeapon *pWeapon, EntityId actorId) {};

	virtual void OnMelee(IWeapon* pWeapon, EntityId shooterId) {};

	virtual void OnStartTargetting(IWeapon *pWeapon) {};
	virtual void OnStopTargetting(IWeapon *pWeapon) {};

	virtual void OnSelected(IWeapon *pWeapon, bool selected) {};

	virtual void OnFireModeChanged(IWeapon *pWeapon, int currentFireMode) {};
};

static void DiscardingPacketStatic(SRecording_Packet *ps, float recordedTime)
{
	g_pGame->GetRecordingSystem()->DiscardingPacket(ps, recordedTime);
}

void CRecordingSystem::SetDefaultInitialState(SPlayerInitialState &initialState, EntityId playerEntityId)
{
	// Set up the player with a default weapon, shouldn't really need this but better to be safe
	SRecording_WeaponSelect weapon;
	weapon.entityId = playerEntityId;
	weapon.pWeaponClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("SCAR");
	initialState.weapon = weapon;
}

void CRecordingSystem::DiscardingPacket(SRecording_Packet *packet, float recordedTime)
{
	EntityId playerEntityId = 0;
	switch (packet->type)
	{
	case eTPP_ActorAnim:
	case eTPP_ActorUpperAnim:
	case eTPP_WeaponAnim:
		playerEntityId = ((SRecording_Animation*)packet)->eid;
		break;
	case eTPP_WeaponSelect:
		playerEntityId = ((SRecording_WeaponSelect*)packet)->entityId;
		break;
	case eTPP_VehicleChange:
		playerEntityId = ((SRecording_VehicleChange*)packet)->playerEntityId;
		break;
	case eTPP_EntitySpawn:
		{
			SRecording_EntitySpawn* pEntitySpawn = (SRecording_EntitySpawn*)packet;
			m_discardedEntitySpawns[pEntitySpawn->entityId] = *pEntitySpawn;
		}
		break;
	case eTPP_EntityRemoved:
		{
			SRecording_EntityRemoved* pEntityRemoved = (SRecording_EntityRemoved*)packet;
			// An entity removed packet cancels out an entity spawn packet
			m_discardedEntitySpawns.erase(pEntityRemoved->entityId);
		}
		break;
	case eTPP_ParticleCreated:
		{
			SRecording_ParticleCreated* pParticleCreated = (SRecording_ParticleCreated*)packet;
			if (pParticleCreated->entityId != 0)
			{
				// Only keep particle effects which are attached to entities, leave the others
				m_discardedParticleSpawns[pParticleCreated->pParticleEmitter] = *pParticleCreated;
			}
		}
		break;
	case eTPP_ParticleDeleted:
		{
			SRecording_ParticleDeleted* pParticleDeleted = (SRecording_ParticleDeleted*)packet;
			// A particle removed packet cancels out a particle spawn packet
			m_discardedParticleSpawns.erase(pParticleDeleted->pParticleEmitter);
		}
		break;
	}
	if (playerEntityId != 0)
	{
		int index = 0;
		EntityIntMap::iterator itPlayerInitialState = m_playerInitialStatesMap.find(playerEntityId);
		if (itPlayerInitialState != m_playerInitialStatesMap.end())
		{
			index = itPlayerInitialState->second;
		}
		else
		{
			// Find the first free index into the array
			bool alreadyUsed;
			do
			{
				alreadyUsed = false;
				for (itPlayerInitialState = m_playerInitialStatesMap.begin(); itPlayerInitialState != m_playerInitialStatesMap.end(); itPlayerInitialState++)
				{
					if (itPlayerInitialState->second == index)
					{
						// This index is taken
						if (gEnv->pEntitySystem->GetEntity(itPlayerInitialState->first) == NULL)
						{
							// If the entity no longer exists we can re-use it (this could happen if a player left the game)
							m_playerInitialStatesMap.erase(itPlayerInitialState);
						}
						else
						{
							// This index is already in use, try the next one in the sequence
							alreadyUsed = true;
							index++;
						}
						break;
					}
				}
			} while (alreadyUsed);
			if (index >= 32)
			{
				CRY_ASSERT_MESSAGE(true, "Index out of range occured during recording player initial state");
				return;
			}
			m_playerInitialStatesMap[playerEntityId] = index;
			SetDefaultInitialState(m_playerInitialStates[index], playerEntityId);
		}
		SPlayerInitialState &initialState = m_playerInitialStates[index];
		switch (packet->type)
		{
		case eTPP_ActorAnim:
			initialState.animationStartTime = recordedTime;
			initialState.animation = *((SRecording_Animation*)packet);
			break;
		case eTPP_ActorUpperAnim:
			initialState.upperAnimationStartTime = recordedTime;
			initialState.upperAnimation = *((SRecording_Animation*)packet);
			break;
		case eTPP_WeaponAnim:
			initialState.weaponAnimationStartTime = recordedTime;
			initialState.weaponAnimation = *((SRecording_Animation*)packet);
			break;
		case eTPP_WeaponSelect:
			initialState.weapon = *((SRecording_WeaponSelect*)packet);
			break;
		case eTPP_VehicleChange:
			initialState.vehicle = *((SRecording_VehicleChange*)packet);
			break;
		}
	}
}

int CRecordingSystem::m_replayEventGuard = 0;

CRecordingSystem::CRecordingSystem()
	: m_mode(ERECSYS_INACTIVE)
	, m_killer(0)
	, m_debugPlayBack(false)
	, m_spawningReplayEntity(false)
	, m_recordingPlaybackUpdate(false)
	, m_playbackMode(ePM_FirstPerson)
	, m_prevPlaybackMode(ePM_FirstPerson)
	, m_projectileType(ePM_Grenade)
	, m_playbackStartTime(0.f)
	, m_recordedStartTime(0.f)
	, m_playbackRequestTime(0.f)
	, m_queuedPacketsSize(0)
	, m_fppacketparts(0)
	, m_firstPersonDataSize(0)
	, m_tpdatasize(0)
	, m_killerIsFriendly(false)
	, m_newProjectilePosition(IDENTITY)
{
#if RECORDING_SYSTEM_ENABLE
	DBG_FNAME;

	if (gEnv->pInput)
		gEnv->pInput->AddEventListener(this);
	if (gEnv->pEntitySystem)
		gEnv->pEntitySystem->AddSink(this);
	if (gEnv->pParticleManager)
		gEnv->pParticleManager->AddEventListener(this);
	if (gEnv->pSoundSystem)
		gEnv->pSoundSystem->AddEventListener(this, false);

	m_pBuffer = new CRecordingBuffer(RECORDING_BUFFER_SIZE);
	m_pBuffer->SetPacketDiscardCallback(DiscardingPacketStatic);
	m_pBufferFirstPerson = new CRecordingBuffer(FP_RECORDING_BUFFER_SIZE);

	Reset();

	m_recordEntityClassFilter.insert(gEnv->pEntitySystem->GetClassRegistry()->FindClass("explosivegrenade"));
	m_recordEntityClassFilter.insert(gEnv->pEntitySystem->GetClassRegistry()->FindClass("flashbang"));
	m_recordEntityClassFilter.insert(gEnv->pEntitySystem->GetClassRegistry()->FindClass("LTagGrenade"));
	m_recordEntityClassFilter.insert(gEnv->pEntitySystem->GetClassRegistry()->FindClass("kvolt_bullet"));
	m_recordEntityClassFilter.insert(gEnv->pEntitySystem->GetClassRegistry()->FindClass("TunnellingGrenade"));
	m_recordEntityClassFilter.insert(gEnv->pEntitySystem->GetClassRegistry()->FindClass("Hologram"));
	m_recordEntityClassFilter.insert(gEnv->pEntitySystem->GetClassRegistry()->FindClass("QuadBike"));

	m_excludedParticleEffects.insert(gEnv->pParticleManager->FindEffect("crysis2_weapon_kvolt.screen_effect"));

	IParticleEffect* pKvoltNormal = gEnv->pParticleManager->FindEffect("crysis2_weapon_kvolt.kvolt_normal");
	IParticleEffect* pKvoltNormalHostile = gEnv->pParticleManager->FindEffect("crysis2_weapon_kvolt.kvolt_normal_hostile");
	IParticleEffect* pKvoltDirect = gEnv->pParticleManager->FindEffect("crysis2_weapon_kvolt.kvolt_direct");
	IParticleEffect* pKvoltDirectHostile = gEnv->pParticleManager->FindEffect("crysis2_weapon_kvolt.kvolt_direct_hostile");
	if (pKvoltNormal && pKvoltNormalHostile)
	{
		m_replacementParticleEffects[pKvoltNormal] = pKvoltNormalHostile;
		m_replacementParticleEffects[pKvoltNormalHostile] = pKvoltNormal;
	}
	if (pKvoltDirect && pKvoltDirectHostile)
	{
		m_replacementParticleEffects[pKvoltDirect] = pKvoltDirectHostile;
		m_replacementParticleEffects[pKvoltDirectHostile] = pKvoltDirect;
	}

	m_iwel = new CWeaponEventRecorder();
#endif
}

void CRecordingSystem::Reset(void)
{
	DBG_FNAME_ALWAYS;

	m_pBuffer->Reset();
	m_pBufferFirstPerson->Reset();
	m_firstPersonDataSize = 0;
	m_fppacketparts = 0;
	m_queuedPacketsSize = 0;
	m_tpdatasize = 0;

	for (TReplayParticleMap::iterator itParticle=m_replayParticles.begin(); itParticle!=m_replayParticles.end(); ++itParticle) 
	{
		itParticle->second->Release();
	}

	m_replayActors.clear();
	m_replayEntities.clear();
	m_replayParticles.clear();
	m_replaySounds.clear();

	m_weaponMap.clear();
	m_vehicleMap.clear();

	m_tempReplayEntities.clear();
	m_newParticles.clear();
	m_newActorEntities.clear();
	m_recordingEntities.clear();
	m_discardedParticleSpawns.clear();
	m_discardedEntitySpawns.clear();
	m_playerInitialStatesMap.clear();
	m_chrvelmap.clear();
	m_eaikm.clear();
	ClearSoundCache();

	if (m_mode == ERECSYS_PLAYBACK || m_mode == ERECSYS_PLAYBACK_START)
		m_mode = ERECSYS_INACTIVE;

	// We need to record vehicle spawns because the reset will have cleared those spawn packets (depending on reset order)
	IVehicleSystem *pVehicleSystem = g_pGame->GetIGameFramework()->GetIVehicleSystem();
	IVehicleIteratorPtr pVehIt = pVehicleSystem->CreateVehicleIterator();
	bool first = true;
	while(IVehicle *pVehicle = pVehIt->Next())
	{
		RecordEntitySpawn(pVehicle->GetEntity(), false, ENTITY_FLAG_CASTSHADOW);
	}
}

CRecordingSystem::~CRecordingSystem()
{
#if RECORDING_SYSTEM_ENABLE
	DBG_FNAME;

	if (gEnv->pInput)
		gEnv->pInput->RemoveEventListener(this);
	if (gEnv->pParticleManager)
		gEnv->pParticleManager->RemoveEventListener(this);
#endif

	SAFE_DELETE(m_pBuffer);
	SAFE_DELETE(m_pBufferFirstPerson);
	SAFE_DELETE(m_iwel);
}

size_t compressFPData(SRecording_FPChar *data, int numpackets)
{
	CRY_FIXME( 27, 11, 2009, "Killcam allocation should be fixed!!! /PJH")
	SRecording_FPChar s_tempbuffer[FP_RECORDING_BUFFER_SIZE / sizeof(SRecording_FPChar)];
	int tempindex = 0; int lastWrittenToTempBuffer = 0;
	s_tempbuffer[tempindex++] = data[0];

	// compress data
	for (int i = 1; i < (numpackets - 1); i++)
	{
		SRecording_FPChar *last = &s_tempbuffer[tempindex - 1];
		SRecording_FPChar *next = &data[i + 1];

		bool discardPacket = true;

		if (next->frametime - last->frametime > 0.01f)
		{
			for (int j = (lastWrittenToTempBuffer + 1); j < i; j++)
			{
				SRecording_FPChar *current = &data[j];

				float lerpFrac = (current->frametime - last->frametime) / (next->frametime - last->frametime);

				// If any of the player flags changed then don't reject the packet
				if (current->playerFlags != last->playerFlags)
				{
					discardPacket = false;
					break;
				}

				// If the FOV has changed significantly (i.e. during zooming in and out) don't reject the packet
				float predictedFov = LERP(last->fov, next->fov, lerpFrac);
				float fovDifference = predictedFov - current->fov;
				if (fabs(fovDifference) > 0.02f)
				{
					discardPacket = false;
					break;
				}

				QuatT pred;
				pred.SetNLerp(last->camlocation, next->camlocation, lerpFrac);

				Vec3 preddiff;
				preddiff = pred.t - current->camlocation.t;
				float dotProd = pred.q | current->camlocation.q;

				// The rotation angle of the quaternion is defined by the equation below
				// float angle = 2*acos(dotProd);
				// We don't really care about the exact angle though, we only want to know if
				// the angle is within a certain tolerance.
				const float rotationTolerance = 0.9999f; // cos(angleTolerance/2);
				const float positionTolerance = 0.1f;
				if (dotProd < rotationTolerance || preddiff.GetLengthSquared() > sqr(positionTolerance))
				{
					discardPacket = false;
					break;
				}
			}
		}

		if (discardPacket == false)
		{
			s_tempbuffer[tempindex++] = data[i];
			lastWrittenToTempBuffer = i;
		}
	}

	s_tempbuffer[tempindex++] = data[numpackets - 1];

	memcpy(data, s_tempbuffer, sizeof(SRecording_FPChar) * tempindex);

	return (sizeof(SRecording_FPChar) * tempindex);
}

size_t CRecordingSystem::GetTPCameraData(uint8 **data)
{
	m_tpdatasize = m_pBuffer->GetData(m_tpdatabuffer, RECORDING_BUFFER_SIZE);
	if (data)
	{
		*data = m_tpdatabuffer;
	}
	return m_tpdatasize;
}

size_t CRecordingSystem::GetFirstPersonData(uint8 **data)
{
	size_t offset = 0;
	size_t datasize = 0;
	while (offset < m_pBufferFirstPerson->size())
	{
		SRecording_Packet *pPacket = (SRecording_Packet*)m_pBufferFirstPerson->at(offset);
		if (pPacket->type == eFPP_FPChar)
		{
			memcpy(m_firstPersonSendBuffer + datasize, pPacket, pPacket->size);
			datasize += pPacket->size;
			CRY_ASSERT_MESSAGE(datasize < sizeof(m_firstPersonSendBuffer), "Ran out of memory in m_firstPersonSendBuffer");
		}
		offset += pPacket->size;
	}

	CryLog("GetFPCameraData got %d packets before reduction", datasize / sizeof(SRecording_FPChar));

	if (datasize > 0)
	{
		float fpcamlimit = MAX_KILLCAM_LEN;
		SRecording_FPChar* pFirstFrame = (SRecording_FPChar*)m_firstPersonSendBuffer;
		SRecording_FPChar* pLastFrame = (SRecording_FPChar*)((m_firstPersonSendBuffer + datasize) - sizeof(SRecording_FPChar));
		float fpCamDataTime = pLastFrame->frametime - pFirstFrame->frametime;

		if (fpCamDataTime > fpcamlimit)
		{
			int myoffset = 0;

			while (myoffset < datasize)
			{
				SRecording_FPChar* pOffsetFrame = (SRecording_FPChar*)(m_firstPersonSendBuffer + myoffset);
				fpCamDataTime = pLastFrame->frametime - pOffsetFrame->frametime;

				if (fpCamDataTime < fpcamlimit)
					break;

				myoffset += sizeof(SRecording_FPChar);
			}

			memmove(m_firstPersonSendBuffer, m_firstPersonSendBuffer + myoffset, datasize - myoffset);
			datasize -= myoffset;
			CryLogAlways("using offset of %d, gives %.3f", myoffset, fpCamDataTime);
		}

		int numpackets = datasize / sizeof(SRecording_FPChar);
		CryLog("GetFPCameraData got %d packets after reduction", numpackets);
		datasize = compressFPData((SRecording_FPChar*)m_firstPersonSendBuffer, numpackets);
		numpackets = datasize / sizeof(SRecording_FPChar);
		CryLog("GetFPCameraData got %d packets after compression", numpackets);

		// Add on any other relevant packets to the end
		if (numpackets > 0)
		{
			offset = 0;
			float startFrameTime = ((SRecording_FPChar*)m_firstPersonSendBuffer)->frametime;
			while (offset < m_pBufferFirstPerson->size())
			{
				SRecording_Packet *pPacket = (SRecording_Packet*)m_pBufferFirstPerson->at(offset);
				bool addPacket = false;
				if (pPacket->type == eFPP_Flashed)
				{
					SRecording_Flashed* pFlashedPacket = (SRecording_Flashed*)pPacket;
					if (pFlashedPacket->frametime > startFrameTime)
					{
						addPacket = true;
					}
				}
				if (addPacket)
				{
					memcpy(m_firstPersonSendBuffer + datasize, pPacket, pPacket->size);
					datasize += pPacket->size;
					CRY_ASSERT_MESSAGE(datasize < sizeof(m_firstPersonSendBuffer), "Ran out of memory in m_firstPersonSendBuffer");
				}
				offset += pPacket->size;
			}
		}

		SwapFPCameraEndian(m_firstPersonSendBuffer, datasize, true);
	}
		
	*data = m_firstPersonSendBuffer;
	return datasize;
}

void CRecordingSystem::SetFirstPersonDataPart(uint8 *data, int part, int numparts, size_t datasize)
{
	int offset = CActor::KillCamData::DATASIZE * part;
	m_fppacketparts += (1<<part);
	CryLogAlways("SetFPCameraDataPart got parts %08x", m_fppacketparts);
	CRY_ASSERT_MESSAGE(datasize + offset <= sizeof(m_firstPersonData), "Ran out of memory in m_firstPersonData");
	memcpy(m_firstPersonData + offset, data, datasize);
	m_firstPersonDataSize += datasize;

	int gotparts = 0;
	int partssofar = m_fppacketparts;
	for (gotparts = 0; partssofar; gotparts++)
		partssofar &= partssofar - 1;

	if (gotparts == numparts)
	{
		CryLogAlways("We got all the parts now!!! size %d", m_firstPersonDataSize);
		SetFirstPersonData(m_firstPersonData, m_firstPersonDataSize);
	}
}

void CRecordingSystem::SetFirstPersonData(uint8 *data, size_t datasize)
{
	CRY_ASSERT_MESSAGE(datasize <= sizeof(m_firstPersonData), "Ran out of memory in m_firstPersonData");

	m_fppacketparts = ALL_PARTS;
	m_firstPersonDataSize = datasize;
	if (data != m_firstPersonData)
	{
		memcpy(m_firstPersonData, data, datasize);
	}

	SwapFPCameraEndian(m_firstPersonData, datasize, false);
}

void CRecordingSystem::StartRecording()
{
#if RECORDING_SYSTEM_ENABLE
	if (!g_pGameCVars->g_killcamEnable)
		return;

//	gEnv->pSystem->m_frameRateTestMinSleep->Set(5);
//	gEnv->pSystem->m_frameRateTestMaxSleep->Set(5);

	if (m_mode == ERECSYS_PLAYBACK || m_mode == ERECSYS_PLAYBACK_START)
	{
		StopPlayback();
	}

	DBG_FNAME;

	m_pBufferFirstPerson->Reset();
	m_mode = ERECSYS_RECORDING;
#endif
}

void CRecordingSystem::StopRecording()
{
#if RECORDING_SYSTEM_ENABLE
	if (!g_pGameCVars->g_killcamEnable)
		return;

	DBG_FNAME;

//	gEnv->pSystem->m_frameRateTestMinSleep->Set(0);
//	gEnv->pSystem->m_frameRateTestMaxSleep->Set(0);

	m_mode = ERECSYS_INACTIVE;
#endif
}

void CRecordingSystem::StartPlayback(EntityId killer, EntityId projectileId, int hitType, Vec3 impulse)
{
#if RECORDING_SYSTEM_ENABLE
	CCCPOINT(CRecordingSystem_StartPlayback);

	DBG_FNAME;

	if (!g_pGameCVars->g_killcamEnable)
		return;

//	gEnv->pSystem->m_frameRateTestMinSleep->Set(0);
//	gEnv->pSystem->m_frameRateTestMaxSleep->Set(0);

	m_killer = killer;
	m_projectileId = projectileId;
	if (hitType == g_pGame->GetGameRules()->GetHitTypeId("kvolt"))
	{
		m_projectileType = ePM_Kvolt;
		// The impulse is 0, 0, 0 if the hit comes from a KVolt lying on the ground, however this number
		// gets quantized when sent across the network so we need to use a tolerance
		if (impulse.GetLengthSquared() > 0.1)
		{
			// Don't follow projectiles that are from direct hits in kvolt
			m_projectileId = 0;
		}
	}
	else
	{
		m_projectileType = ePM_Grenade;
	}

	m_playbackMode = ePM_FirstPerson;
	m_prevPlaybackMode = ePM_FirstPerson;
	m_cameraSmoothing = false;
	CActor* pActor = (CActor*)g_pGame->GetIGameFramework()->GetClientActor();
	m_killerIsFriendly = pActor && pActor->IsFriendlyEntity(m_killer);
	m_newProjectilePosition.SetIdentity();

	GetTPCameraData();

	m_mode = ERECSYS_PLAYBACK_START;

	SHUDEvent deathCamStart(eHUDEvent_OnDeathcamStartPlay);
	deathCamStart.AddData(static_cast<void*>(&m_replayActors));
	deathCamStart.AddData((int)m_killer);
	CHUD::CallEvent(deathCamStart);

	g_pGame->GetHUD()->ActivateState("mp_deathcam");

	m_playbackStartTime = 0.f;
	m_recordedStartTime = 0.f;
	m_playbackRequestTime = gEnv->pTimer->GetAsyncTime().GetSeconds();
#endif
}

void CRecordingSystem::StopPlayback()
{
#if RECORDING_SYSTEM_ENABLE
	CCCPOINT(CRecordingSystem_StopPlayback);

	if (!g_pGameCVars->g_killcamEnable)
		return;

	DBG_FNAME;

	m_mode = ERECSYS_STOPPED;
	m_firstPersonDataSize = 0;
	m_fppacketparts = 0;
	m_killer = 0;

	TReplayActorMap::iterator itReplayActor;
	for (itReplayActor = m_replayActors.begin(); itReplayActor != m_replayActors.end(); ++itReplayActor)
	{
		// Remove the cloned player and gun
		gEnv->pEntitySystem->RemoveEntity(itReplayActor->second.chr_id);
		gEnv->pEntitySystem->RemoveEntity(itReplayActor->second.gun_id);
		// Show the real player
		IEntity *entity = gEnv->pEntitySystem->GetEntity(itReplayActor->first);
		if (entity)
		{
			HideEntityKeepingPhysics(entity, false);
		}
	}
	m_replayActors.clear();

	// Remove all the replay entities
	for (TReplayEntityMap::iterator itEntity=m_replayEntities.begin(); itEntity!=m_replayEntities.end(); ++itEntity) 
	{
		gEnv->pEntitySystem->RemoveEntity(itEntity->second);
	}
	m_replayEntities.clear();
	for (std::vector<EntityId>::iterator itEntity=m_tempReplayEntities.begin(); itEntity!=m_tempReplayEntities.end(); ++itEntity)
	{
		gEnv->pEntitySystem->RemoveEntity(*itEntity);
	}
	m_tempReplayEntities.clear();

	{
		REPLAY_EVENT_GUARD
		// Remove all the replay particles
		for (TReplayParticleMap::iterator itParticle=m_replayParticles.begin(); itParticle!=m_replayParticles.end(); ++itParticle) 
		{
			gEnv->pParticleManager->DeleteEmitter(itParticle->second);
			itParticle->second->Release();
		}
		m_replayParticles.clear();

		// Stop all replay sounds
		for (TReplaySoundMap::iterator itSound=m_replaySounds.begin(); itSound!=m_replaySounds.end(); ++itSound) 
		{
			ISound* pSound = gEnv->pSoundSystem->GetSound(itSound->second);
			if (pSound)
			{
				pSound->Stop();
			}
		}
		m_replaySounds.clear();
	}

	// Show all the entities that where hidden
	for (std::vector<EntityId>::iterator itEntity=m_recordingEntities.begin(); itEntity!=m_recordingEntities.end(); ++itEntity) 
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(*itEntity);
		if (pEntity)
		{
			HideEntityKeepingPhysics(pEntity, false);
		}
	}

	if(CScreenEffects* pScreenFX = g_pGame->GetScreenEffects())
	{
		pScreenFX->ProcessSuitStressed(false);
	}
	DisableFlashBangEffect();

	memset(m_firstPersonData, 0, sizeof(m_firstPersonData));

	SHUDEvent deathCamStop(eHUDEvent_OnDeathcamStopPlay);
	CHUD::CallEvent(deathCamStop);

	if (m_debugPlayBack)
	{
		// The HUD should remain in whatever state it is in until
		// the system requiring the next state updates it, expect
		// when debugging the kill cam.
		IActor *pClientActor=g_pGame->GetIGameFramework()->GetClientActor();
		CRY_ASSERT_MESSAGE( pClientActor, "RecordingSystem: No Actor!" );
		CRY_ASSERT_MESSAGE( !pClientActor->IsDead(), "RecordingSystem: Actor is dead when debugging the killcam!" );

		CHUD *pHUD = g_pGame->GetHUD();
		pHUD->ActivateDefaultState();

		StartRecording();
		m_debugPlayBack = false;
	}
#endif
}

void CRecordingSystem::OnPlayerFirstPersonChange(IEntity* pPlayerEntity, EntityId weaponId, bool firstPerson)
{
	m_weaponFPAiming.SetActive(firstPerson);
	m_weaponFPAiming.SnapSwitch(true);
	UpdatePlayerVisibility(pPlayerEntity, !firstPerson);

	CItem* pItem = (CItem*)g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(weaponId);
	if (pItem)
	{
		pItem->SetViewMode(firstPerson ? eIVM_FirstPerson : eIVM_ThirdPerson);
		CWeapon *pWeapon = static_cast<CWeapon *>(pItem->GetIWeapon());
		if (pWeapon)
		{
			pWeapon->SetOwnerClientOverride(firstPerson);
		}
	}
}

void CRecordingSystem::ApplyVehicleChange(const SRecording_VehicleChange *vehicleChange)
{
	TReplayActorMap::iterator find_iterator = m_replayActors.find(vehicleChange->playerEntityId);
	if (find_iterator != m_replayActors.end())
	{
		SReplayActorInfo &replayActorInfo = find_iterator->second;
		IEntity* pPlayerEntity = gEnv->pEntitySystem->GetEntity(replayActorInfo.chr_id);
		if (pPlayerEntity)
		{
			IEntity* pGunEntity = gEnv->pEntitySystem->GetEntity(replayActorInfo.gun_id);
			if (replayActorInfo.vehicleId != 0)
			{
				// Get off the previous vehicle
				pPlayerEntity->DetachThis();
				// And show the weapon
				if (pGunEntity)
				{
					pGunEntity->Hide(false);
				}
				if (vehicleChange->playerEntityId == m_killer && m_playbackMode == ePM_FirstPerson)
				{
					// Changed to first person mode
					OnPlayerFirstPersonChange(pPlayerEntity, replayActorInfo.gun_id, true);
				}
			}

			TReplayEntityMap::iterator itReplayVehicle = m_replayEntities.find(vehicleChange->vehicleEntityId);
			if (itReplayVehicle != m_replayEntities.end())
			{
				IEntity* pVehicleEntity = gEnv->pEntitySystem->GetEntity(itReplayVehicle->second);
				if (pVehicleEntity)
				{
					// Get onto the new vehicle
					pPlayerEntity->SetPosRotScale(Vec3(ZERO), Quat(IDENTITY), Vec3(1,1,1));
					pVehicleEntity->AttachChild(pPlayerEntity);
					// And hide the weapon
					if (pGunEntity)
					{
						pGunEntity->Hide(true);
					}
					if (vehicleChange->playerEntityId == m_killer && m_playbackMode == ePM_FirstPerson)
					{
						// Changed to third person mode
						OnPlayerFirstPersonChange(pPlayerEntity, replayActorInfo.gun_id, false);
					}
				}
				replayActorInfo.vehicleId = itReplayVehicle->second;
			}
			else
			{
				replayActorInfo.vehicleId = 0;
			}
		}
	}
}

void CRecordingSystem::ApplyWeaponSelect(const SRecording_WeaponSelect *weaponSelect)
{
	IEntity *pReplayEntity = NULL;
	EntityId *pGunId = NULL;
	TReplayActorMap::iterator find_iterator = m_replayActors.find(weaponSelect->entityId);
	if (find_iterator != m_replayActors.end())
	{
		SReplayActorInfo &replayActorInfo = find_iterator->second;
		pReplayEntity = gEnv->pEntitySystem->GetEntity(replayActorInfo.chr_id);
		pGunId = &replayActorInfo.gun_id;
	}
	else
	{
		TReplayEntityMap::iterator itReplayEntity = m_replayEntities.find(weaponSelect->entityId);
		if (itReplayEntity != m_replayEntities.end())
		{
			pReplayEntity = gEnv->pEntitySystem->GetEntity(itReplayEntity->second);
		}
	}
	if (pReplayEntity)
	{
		SEntitySpawnParams params;
		params.sName = "ReplayWeapon";
		params.pClass = weaponSelect->pWeaponClass;
		params.nFlags |= (ENTITY_FLAG_NO_PROXIMITY|ENTITY_FLAG_NEVER_NETWORK_STATIC|ENTITY_FLAG_CLIENT_ONLY);
		if (IEntity* pItemEnt = gEnv->pEntitySystem->SpawnEntity(params))
		{
			EntityId itemEntId = pItemEnt->GetId();
			IGameFramework *pGameFramework = gEnv->pGame->GetIGameFramework();
			IItemSystem *pItemSystem = pGameFramework->GetIItemSystem();
			if (IItem * pItem = pItemSystem->GetItem(itemEntId))
			{
				CItem *cItem = (CItem *) pItem;
				for (int i=0; i<MAX_WEAPON_ACCESSORIES; i++)
				{
					IEntityClass* pAccessoryClass = weaponSelect->pAccessoryClasses[i];
					if (pAccessoryClass)
					{
						cItem->AttachAccessory(pAccessoryClass->GetName(), true, true, true);
					}
				}
				if (weaponSelect->isRippedOff)
				{
					if (strcmp(weaponSelect->pWeaponClass->GetName(), "HMG") == 0)
					{
						CHMG* pHmg = (CHMG*)pItem;
						pHmg->FinishRipOff();
						pHmg->UnlinkMountedGun();
					}
				}
				cItem->StartUse(pReplayEntity->GetId());
				//cItem->AttachToHand(true);
				cItem->Select(true);
				if (weaponSelect->entityId == m_killer && m_playbackMode == ePM_FirstPerson)
				{
					CWeapon *pWeapon = static_cast<CWeapon *>(pItem->GetIWeapon());
					if (pWeapon)
					{
						pWeapon->SetCurrentFireMode("Single");
						pWeapon->SetOwnerClientOverride(true);
					}
					cItem->SetViewMode(eIVM_FirstPerson);
					m_weaponFPAiming.SnapSwitch(true);
				}

				cItem->CloakSync(false);
			}

			//pItemEnt->GetCharacter(0)->GetISkeletonAnim()->SetDebugging(1);

			// Remove the previous gun and update the id
			if (pGunId)
			{
				if (*pGunId != 0)
					gEnv->pEntitySystem->RemoveEntity(*pGunId);
				*pGunId = itemEntId;
			}
			else
			{
				m_tempReplayEntities.push_back(itemEntId);
			}
		}
	}
}

void CRecordingSystem::ApplyAnimPacket(const SRecording_Animation *animation, float recordedTime)
{
	IEntity *pReplayEntity = NULL;
	TReplayActorMap::iterator find_iterator = m_replayActors.find(animation->eid);
	if (find_iterator != m_replayActors.end())
	{
		SReplayActorInfo &replayActorInfo = find_iterator->second;
		switch (animation->type)
		{
		case eTPP_ActorAnim:
			// Don't apply third person animations to the killer if in first person playback mode
			if ((animation->eid != m_killer || m_playbackMode != ePM_FirstPerson) || replayActorInfo.vehicleId != 0)
			{
				pReplayEntity = gEnv->pEntitySystem->GetEntity(replayActorInfo.chr_id);
			}
			break;
		case eTPP_ActorUpperAnim:
			pReplayEntity = gEnv->pEntitySystem->GetEntity(replayActorInfo.chr_id);
			break;
		case eTPP_WeaponAnim:
			pReplayEntity = gEnv->pEntitySystem->GetEntity(replayActorInfo.gun_id);
			break;
		default:
			CRY_ASSERT_MESSAGE(false, "Unhandled animation packet type");
			break;
		}
	}
	else
	{
		TReplayEntityMap::iterator itReplayEntity = m_replayEntities.find(animation->eid);
		if (itReplayEntity != m_replayEntities.end())
		{
			pReplayEntity = gEnv->pEntitySystem->GetEntity(itReplayEntity->second);
		}
	}

	if (pReplayEntity)
	{
		CryLog("entity %d -> anim %d", animation->eid, animation->animID);

		ICharacterInstance* ici = pReplayEntity->GetCharacter(0);
		if (ici)
		{
			ISkeletonAnim* isa = ici->GetISkeletonAnim();
			if (isa)
			{
				int animId = animation->animID;
				if (animation->eid == m_killer && m_playbackMode == ePM_FirstPerson)
				{
					// If the player is the killer and we are in first person playback mode
					// then we need to play the first person animations and not the
					// 3rd person animations.
					IAnimationSet* pAnimationSet = ici->GetIAnimationSet();
					const char *animName = pAnimationSet->GetNameByAnimID(animation->animID);
					CryFixedStringT<256> modifiedAnimName = animName;
					modifiedAnimName.replace("_3p", "_1p");
					modifiedAnimName.replace("_3P", "_1P");
					animId = pAnimationSet->GetAnimIDByName(modifiedAnimName);
					if (animId == -1)
					{
						// If the first person animation can't be found then just use the 3rd person animation
						animId = animation->animID;
					}
				}

				if (recordedTime > 0)
				{
					PlayInitialAnimation(isa, ici->GetIAnimationSet(), animId, animation->animparams, recordedTime, animation->speedMultiplier);
				}
				else
				{
					int success = isa->StartAnimationById(animId, animation->animparams);
				}
				isa->SetLayerUpdateMultiplier(animation->animparams.m_nLayerID, animation->speedMultiplier);
				if (animation->type == eTPP_ActorAnim)
				{
					// TODO: Check this is really necessary... might be able to apply this just once instead of after each animation?
					isa->SetAnimationDrivenMotion(1);
				}
			}
		}
	}
}

void CRecordingSystem::PlayInitialAnimation(ISkeletonAnim* pSkeletonAnim, IAnimationSet* pAnimSet, int32 animId, const CryCharAnimationParams& params, float startTime, float speedMultiplier)
{
	float duration = pAnimSet->GetDuration_sec(animId);
	float animTime = m_recordedStartTime - startTime;
	animTime *= params.m_fPlaybackSpeed * speedMultiplier;
	CRY_ASSERT_MESSAGE(animTime >= 0, "Animation time shouldn't be less than 0");
	if (params.m_nFlags & CA_LOOP_ANIMATION)
	{
		animTime = fmod(animTime, duration);
	}
	else if (params.m_nFlags & CA_REPEAT_LAST_KEY)
	{
		animTime = min(animTime, duration);
	}
	else if (animTime >= duration)
	{
		// Animation has finished don't bother playing it...
		return;
	}
	pSkeletonAnim->StartAnimationById(animId, params);
	pSkeletonAnim->SetLayerTime(params.m_nLayerID, animTime / duration);
}

void CRecordingSystem::HideEntityKeepingPhysics(IEntity *pEntity, bool hide)
{
	SEntityEvent event(hide ? ENTITY_EVENT_INVISIBLE : ENTITY_EVENT_VISIBLE);
	for (int i = 0; i < ENTITY_PROXY_LAST; i++)
	{
		EEntityProxy proxyType = (EEntityProxy)i;
		// Everybody hide, but don't tell the physics.
		if (proxyType != ENTITY_PROXY_PHYSICS)
		{
			IEntityProxy *pProxy = pEntity->GetProxy(proxyType);
			if (pProxy)
				pProxy->ProcessEvent(event);
		}
	}

	// Propagate invisible flag to the child entities
	const int childCount = pEntity->GetChildCount();
	for (int i = 0; i < childCount; i++)
	{
		IEntity* pChild = pEntity->GetChild(i);
		if (pChild)
		{
			HideEntityKeepingPhysics(pChild, hide);
		}
	}
}

void CRecordingSystem::PlaybackStartingSetupEntities(void)
{
	REPLAY_EVENT_GUARD

	CGameRules *pGameRules = g_pGame->GetGameRules();

	CryLog("PlaybackStartingSetupEntities...");

	// Hide all the recorded entities
	for (std::vector<EntityId>::iterator itEntity = m_recordingEntities.begin(); itEntity != m_recordingEntities.end(); ++itEntity)
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(*itEntity);
		if (pEntity)
		{
			HideEntityKeepingPhysics(pEntity, true);
		}
	}

	// Spawn any entities that were created before the start of the playback
	TEntitySpawnMap::iterator itEntitySpawn;
	for (itEntitySpawn = m_discardedEntitySpawns.begin();  itEntitySpawn != m_discardedEntitySpawns.end(); ++itEntitySpawn)
	{
		ApplyEntitySpawn(&itEntitySpawn->second);
	}

	// Spawn any particles that were created before the start of the playback
	TParticleCreatedMap::iterator itParticleSpawn;
	for (itParticleSpawn = m_discardedParticleSpawns.begin(); itParticleSpawn != m_discardedParticleSpawns.end(); )
	{
		// TODO: Should set the age of the particle effect as well
		if (ApplyParticleCreated(&itParticleSpawn->second))
		{
			++itParticleSpawn;
		}
		else
		{
			// If the particle effect failed to spawn then remove it from the list to avoid the list
			// growing with invalid particle effects
			TParticleCreatedMap::iterator itErase = itParticleSpawn;
			++itParticleSpawn;
			m_discardedParticleSpawns.erase(itErase);
		}
	}


	// Hide all the players and create clones of them that we can do what we want with
	CGameRules::TPlayers  players;
	pGameRules->GetPlayersClient(players);

	for (CGameRules::TPlayers::const_iterator player=players.begin();player!=players.end(); ++player) 
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(*player);
		if (pEntity)
		{
			HideEntityKeepingPhysics(pEntity, true);

			{
				IEntityClass *pEntityClass =  gEnv->pEntitySystem->GetClassRegistry()->FindClass("Default");
				assert(pEntityClass);

				SEntitySpawnParams params;
				params.pClass = pEntityClass;
				params.sName = pEntity->GetName();

				params.vPosition = pEntity->GetPos();
				params.qRotation = pEntity->GetRotation();

				params.nFlags |= (ENTITY_FLAG_NEVER_NETWORK_STATIC|ENTITY_FLAG_CLIENT_ONLY);

				IEntity *pCloneEntity = gEnv->pEntitySystem->SpawnEntity(params, true);
				assert(pCloneEntity);
				//int slot = pEntity->LoadCharacter(0, "Objects/Characters/Human/us/nanosuit_v2/nanosuit_v2.cdf");
				int slot = pCloneEntity->LoadCharacter(0, pEntity->GetCharacter(0)->GetFilePath());

				pCloneEntity->SetFlags(pCloneEntity->GetFlags() | (ENTITY_FLAG_CASTSHADOW));

				ICharacterInstance* ici = pCloneEntity->GetCharacter(0);
				ISkeletonPose* isp = ici->GetISkeletonPose();
				isp->SetAimIKLayer(1);
				isp->SetAimIKTargetSmoothTime(0.0f);
				isp->SetAimIKFadeOut(0);

				PhysicalizeEntity(pCloneEntity, false);

				{
					SReplayActorInfo srai = {0};
					srai.chr_id = pCloneEntity->GetId();
					srai.gun_id = 0;
					srai.vehicleId = 0;
					srai.flags = 0;

					srai.drawPos = ZERO;
					srai.size = 0.f;
					srai.team = pGameRules->GetTeam(*player);

					m_replayActors[*player] = srai;
				}

				if (m_killer == *player && m_playbackMode == ePM_FirstPerson)
				{
					OnPlayerFirstPersonChange(pCloneEntity, 0, true);
				}

				// Set up the initial states of the clone players
				if (!ApplyPlayerInitialState(*player))
				{
					CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "The player doesn't have a valid initial state");
				}
			}
		}
	}
}

bool CRecordingSystem::ApplyPlayerInitialState(EntityId entityId)
{
	EntityIntMap::iterator itInitialState = m_playerInitialStatesMap.find(entityId);
	if (itInitialState != m_playerInitialStatesMap.end())
	{
		SPlayerInitialState &initialState = m_playerInitialStates[itInitialState->second];
		// If any of the initial state packets don't exist then all the variables should be 0
		// just check the size variable for convenience.
		if (initialState.weapon.size != 0)
			ApplyWeaponSelect(&initialState.weapon);
		if (initialState.vehicle.size != 0)
			ApplyVehicleChange(&initialState.vehicle);
		// NOTE: The animation packet needs to come after the vehicle change
		if (initialState.animation.size != 0)
			ApplyAnimPacket(&initialState.animation, initialState.animationStartTime);
		if (initialState.upperAnimation.size != 0)
			ApplyAnimPacket(&initialState.upperAnimation, initialState.upperAnimationStartTime);
		if (initialState.weaponAnimation.size != 0)
			ApplyAnimPacket(&initialState.weaponAnimation, initialState.weaponAnimationStartTime);
		return true;
	}
	return false;
}

void CRecordingSystem::QueueSendRequest(SKillCamSendRequest data)
{
	if (data.shooter != data.victim)
	{
		CryLog("Queueing killcam data send from %d to %d...", data.shooter, data.victim);

		data.m_time = gEnv->pTimer->GetFrameStartTime().GetSeconds();
		m_killcamQueue.push_back(data);
	}
}

float CRecordingSystem::GetLiveBufferLenSeconds(void)
{
	// TODO: Implement me, this is only used for debugging purposes though...
	CRY_ASSERT_MESSAGE(false, "This function has not been implemented yet");
	return 0;
#if 0
	CRecordingBuffer::packetIterator it = m_pBuffer->GetPacketListBegin();

	float starttime = -1.f;
	float endtime = -1.f;

	while (it != m_pBuffer->GetPacketListEnd())
	{
		CRecordingBuffer::packetStruct *ps = *it;
		SRecording_Packet *packet = (SRecording_Packet *) ps->data;

		switch(packet->type)
		{
		case eRBPT_FrameData:
			{
				SRecording_FrameData *sfd = (SRecording_FrameData *) ps->data;
				if (starttime == -1.f)
					starttime = sfd->frametime;
				endtime = sfd->frametime;
				break;
			}
		default:
			break;
		}

		it++;
	}

	if (starttime == -1.f || endtime == -1.f)
		return 0.f;

	return (endtime - starttime);
#endif
}

float CRecordingSystem::GetBufferLenSeconds(void)
{
	uint8 *startingplace = m_tpdatabuffer;
	uint8 *endingplace = startingplace + m_tpdatasize;

	float starttime = -1.f;
	float endtime = -1.f;

	while (startingplace < endingplace)
	{
		SRecording_Packet *packet = (SRecording_Packet *) startingplace;

		switch(packet->type)
		{
			case eRBPT_FrameData:
			{
				SRecording_FrameData *sfd = (SRecording_FrameData *) startingplace;
				if (starttime == -1.f)
					starttime = sfd->frametime;
				endtime = sfd->frametime;
				break;
			}
			default:
				break;
		}

		startingplace = startingplace + packet->size;
	}

	if (starttime == -1.f || endtime == -1.f)
		return 0.f;

	return (endtime - starttime);
}

void CRecordingSystem::FillOutFPCamera(SRecording_FPChar *cam, float playbacktime)
{
	int upperoffset = 0;
	//CryLog("Looking for fpcameraoffset for %.2f", playbacktime);

	SRecording_FPChar* pFPCharData = (SRecording_FPChar*)m_firstPersonData;

	while (true)
	{
		if (upperoffset * sizeof(SRecording_FPChar) >= m_firstPersonDataSize || pFPCharData[upperoffset].type != eFPP_FPChar)
		{
			upperoffset--;
			break;
		}
		
		float fptime = pFPCharData[upperoffset].frametime - pFPCharData[0].frametime;

		if (fptime > playbacktime)
			break;

		upperoffset++;
	}

	int loweroffset = upperoffset - 1;
	if (loweroffset >= 0)
	{
		CRY_ASSERT_MESSAGE(pFPCharData[loweroffset].type == eFPP_FPChar, "This packet type is not valid");
		CRY_ASSERT_MESSAGE(pFPCharData[upperoffset].type == eFPP_FPChar, "This packet type is not valid");

		float loweroffsettime = pFPCharData[loweroffset].frametime - pFPCharData[0].frametime;
		float upperoffsettime = pFPCharData[upperoffset].frametime - pFPCharData[0].frametime;
		float timediff = upperoffsettime - loweroffsettime;

		CRY_ASSERT_MESSAGE(timediff != 0, "The time difference must not be 0");
		float lerpval = (playbacktime - loweroffsettime) / timediff;

		//CryLog("killcam 1p lerping between offsets %d and %d by %.2f", loweroffset, upperoffset, lerpval);

		cam->camlocation.SetNLerp(pFPCharData[loweroffset].camlocation, pFPCharData[upperoffset].camlocation, lerpval);
		cam->relativePosition.SetNLerp(pFPCharData[loweroffset].relativePosition, pFPCharData[upperoffset].relativePosition, lerpval);
		cam->fov = LERP(pFPCharData[loweroffset].fov, pFPCharData[upperoffset].fov, lerpval);
		cam->frametime = LERP(pFPCharData[loweroffset].frametime, pFPCharData[upperoffset].frametime, lerpval);
		cam->playerFlags = pFPCharData[loweroffset].playerFlags;

		Vec3 velocity = pFPCharData[upperoffset].camlocation.t - pFPCharData[loweroffset].camlocation.t;
		velocity /= timediff;
		Quat deltaRot = pFPCharData[loweroffset].camlocation.q.GetInverted() * pFPCharData[upperoffset].camlocation.q;
		Ang3 eulerAngles = ((Ang3)deltaRot) / timediff;
		m_weaponParams.velocity = velocity;
		m_weaponParams.inputMove = velocity;
		m_weaponParams.inputRot = eulerAngles;
	}
}

void CRecordingSystem::PlaybackRecordedFirstPersonActions(float playbacktime)
{
	if (m_firstPersonDataSize > 0)
	{
		SRecording_FPChar* pFPCharData = (SRecording_FPChar*)m_firstPersonData;
		if (pFPCharData->type == eFPP_FPChar)
		{
			float startTime = pFPCharData->frametime;
			float adjustedTime = startTime + playbacktime;
			int offset = 0;
			uint8* pPos = m_firstPersonData;
			bool removePacket = false;
			while (pPos < m_firstPersonData + m_firstPersonDataSize)
			{
				removePacket = false;
				SRecording_Packet* pPacket = (SRecording_Packet*)pPos;
				switch (pPacket->type)
				{
				case eFPP_FPChar:
					// Skip this one
					break;
				case eFPP_Flashed:
				{
					SRecording_Flashed* pFlashedPacket = (SRecording_Flashed*)pPacket;
					if (pFlashedPacket->frametime <= adjustedTime)
					{
						ApplyFlashed(pFlashedPacket);
						removePacket = true;
					}
					break;
				}
				default:
					CRY_ASSERT_MESSAGE(false, "Unrecognised packet type found");
					break;
				}
				if (removePacket)
				{
					m_firstPersonDataSize -= pPacket->size;
					// Move the rest of the packets up to this position
					size_t sizeLeft = (m_firstPersonData + m_firstPersonDataSize) - pPos;
					memmove(pPos, pPos + pPacket->size, sizeLeft);
				}
				else
				{
					pPos += pPacket->size;
				}
			}
		}
	}
}

void CRecordingSystem::DropTPFramesUpTo(float trimTime)
{
	uint8 *startingplace = m_tpdatabuffer;
	uint8 *endingplace = startingplace + m_tpdatasize;

	float recordedTime = 0;
	while (startingplace < endingplace)
	{
		SRecording_Packet *packet = (SRecording_Packet *) startingplace;

		if (packet->type == eRBPT_FrameData)
		{
			SRecording_FrameData *sfd = (SRecording_FrameData *) startingplace;
			recordedTime = sfd->frametime;
			if (sfd->frametime >= trimTime)
			{
				// We've reached the trim time, don't drop any more frames
				break;
			}
		}
		else
		{
			DiscardingPacket(packet, recordedTime);
		}

		startingplace = startingplace + packet->size;
	}
	if (startingplace != m_tpdatabuffer)
	{
		memmove(m_tpdatabuffer, startingplace, (endingplace - startingplace));
		m_tpdatasize = (uint32) (endingplace - startingplace);
	}
}

void CRecordingSystem::DropTPFrame(bool recordDiscarded)
{
	uint8 *startingplace = m_tpdatabuffer;
	uint8 *endingplace = startingplace + m_tpdatasize;

	int frame = 0;
	float recordedTime = 0;
	while (startingplace < endingplace)
	{
		SRecording_Packet *packet = (SRecording_Packet *) startingplace;

		if (packet->type == eRBPT_FrameData)
		{
			SRecording_FrameData *sfd = (SRecording_FrameData *) startingplace;
			recordedTime = sfd->frametime;
			frame++;
			if (frame == 2)
			{
				break;
			}
		}
		else if (recordDiscarded)
		{
			DiscardingPacket(packet, recordedTime);
		}

		startingplace = startingplace + packet->size;
	}
	memmove(m_tpdatabuffer, startingplace, (endingplace - startingplace));
	m_tpdatasize = (uint32) (endingplace - startingplace);
}

void CRecordingSystem::RecordTPInfo(void)
{
	CGameRules *cgr = g_pGame->GetGameRules();
	CGameRules::TPlayers  players;

	if (cgr)
	{
		cgr->GetPlayersClient(players);

		if (players.size() > 0)
		{
			for (CGameRules::TPlayers::const_iterator player=players.begin();player!=players.end(); ++player) 
			{
				IEntity *entity = gEnv->pEntitySystem->GetEntity(*player);

				if (entity)
				{
					CActor *pActor = (CActor *) gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(*player);
					if ( pActor )
					{
						RecordTPCharPacket(entity, pActor);

						EntityId itemId = pActor->GetCurrentItemId(false);
						CWeapon *cw = pActor->GetWeapon(itemId);
						if (cw)
						{
							EntityWeaponMap::iterator itWeapon = m_weaponMap.find(*player);
							if (itWeapon == m_weaponMap.end() || itWeapon->second.weaponPtr != cw)
							{
								// Switching weapon
								SWeaponInfo gun(cw, cw->IsMounted());
								m_weaponMap[*player] = (gun);
								CryLog("Adding entry for %s '%s', adding event listener to the gun...", entity->GetClass()->GetName(), entity->GetName());
								cw->AddEventListener(m_iwel, "CRecordingSystem for the killcam");
								RecordWeaponSelection(pActor, cw);
							}
							else if (itWeapon->second.isMounted != cw->IsMounted())
							{
								// Switching from mounted to unmounted (or vice versa)
								itWeapon->second.isMounted = cw->IsMounted();
								RecordWeaponSelection(pActor, cw);
							}
						}

						IVehicle* pVehicle = pActor->GetLinkedVehicle();
						EntityId vehicleId = 0;
						if (pVehicle)
						{
							vehicleId = pVehicle->GetEntityId();
						}
						EntityVehicleMap::iterator itVehicle = m_vehicleMap.find(*player);
						if (itVehicle == m_vehicleMap.end() || itVehicle->second != vehicleId)
						{
							// The player must have changed vehicle
							m_vehicleMap[*player] = vehicleId;
							SRecording_VehicleChange vehicleChange;
							vehicleChange.playerEntityId = *player;
							vehicleChange.vehicleEntityId = vehicleId;
							m_pBuffer->AddPacket(vehicleChange);
						}
					}
				}
			}
		}
	}
}

void CRecordingSystem::RecordTPCharPacket(IEntity *pEntity, CActor *pActor)
{
	SRecording_TPChar chr;
	chr.eid = pEntity->GetId();
	chr.entitylocation.t = pEntity->GetPos();
	chr.entitylocation.q = pEntity->GetRotation();
	if (m_eaikm[chr.eid].aimIKEnabled)
		chr.playerFlags |= eTPF_AimIk;
	chr.aimdir = m_eaikm[chr.eid].aimIKTarget;
	chr.velocity = m_chrvelmap[chr.eid];
	if (pActor->IsDead())
		chr.playerFlags |= eTPF_Dead;
	const SActorStats *pActorStats = pActor->GetActorStats();
	if (pActorStats && pActorStats->isRagDoll)
		chr.playerFlags |= eTPF_Ragdoll;
	if (pActor->IsPlayer() &&  ((CPlayer*)pActor)->IsPerkActive(ePerk_ThreatDetector))
		chr.playerFlags |= eTPF_HasThreatDetectorPerk;
	IEntityRenderProxy* pRenderProxy = static_cast<IEntityRenderProxy*>(pEntity->GetProxy(ENTITY_PROXY_RENDER));
	if (pRenderProxy)
	{
		//Diesel cut
		//memcpy(chr.layerEffectParams, pRenderProxy->GetEffectLayerParams(), 2 * sizeof(uint32));
		if (pRenderProxy->GetMaterialLayersMask() & MTL_LAYER_CLOAK)
			chr.playerFlags |= eTPF_Cloaked;
	}

	m_pBuffer->AddPacket(chr);
}

void CRecordingSystem::AddPacket(const SRecording_Packet &packet)
{
	if (g_pGameCVars->g_killcamEnable)
	{
		m_pBuffer->AddPacket(packet);
	}
}

void CRecordingSystem::QueueAddPacket(const SRecording_Packet &packet)
{
	if (g_pGameCVars->g_killcamEnable)
	{
		if (packet.size + m_queuedPacketsSize < RECORDING_BUFFER_QUEUE_SIZE)
		{
			memcpy(m_pQueuedPackets + m_queuedPacketsSize, &packet, packet.size);
			m_queuedPacketsSize += packet.size;
		}
		else
		{
			CRY_ASSERT_MESSAGE(false, "Ran out of memory queueing packets");
		}
	}
}

void CRecordingSystem::AddQueuedPackets()
{	
	uint8* pPacket = m_pQueuedPackets;
	while (pPacket < m_pQueuedPackets + m_queuedPacketsSize)
	{
		SRecording_Packet &packet = *(SRecording_Packet*)pPacket;
		m_pBuffer->AddPacket(packet);
		pPacket += packet.size;
	}
	m_queuedPacketsSize = 0;
}

void CRecordingSystem::Update(float frameTime)
{	
#if RECORDING_SYSTEM_ENABLE
	if (!g_pGameCVars->g_killcamEnable)
		return;

	m_pBuffer->Update();

	RecordTPInfo();
	RecordEntityInfo();
	RecordParticleInfo();
	RecordActorWeapon();
	AddQueuedPackets();

	/*if (g_pGameCVars->g_animatorDebug)
	{
		DebugDrawAnimationData();
	}*/

	if (m_killcamQueue.size() >= 1)
	{
		SKillCamSendRequest req = m_killcamQueue.front();
		float now = gEnv->pTimer->GetFrameStartTime().GetSeconds();
		if ((now - req.m_time) > KILL_CAM_KICK_IN_TIME)
		{
			CryLog("%.2f seconds elapsed, sending killcam data send from %d to %d...", KILL_CAM_KICK_IN_TIME, req.shooter, req.victim);

			m_killcamQueue.pop_front();

			if (gEnv->bClient)
			{
				if (IActor *pVictim=g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(req.victim))
				{
					CActor* pCVictim = static_cast<CActor*>(pVictim);
					pCVictim->ClientSendKillcamData(req.shooter, req.victim);
				}
			}
			else
			{
				if (IActor *pShooter=g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(req.shooter))
				{
					CActor* pCShooter = static_cast<CActor*>(pShooter);
					if (IActor *pVictim=g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(req.victim))
						pCShooter->ServerSendKillcamData(pVictim);
				}
			}
		}
	}

	{
#if RECORDING_SYSTEM_EXTRA_DEBUG
		IRenderer *pRenderer = gEnv->pRenderer;
		float col2[] = {1.f, 0.f, 0.f, 1.f};

		if (m_mode == ERECSYS_RECORDING)
		{
			pRenderer->Draw2dLabel(70.f, 50.f, 2, col2, false, "RECORDING");
		}

		if (m_mode == ERECSYS_PLAYBACK)
		{
			float col2[] = {0.f, 1.f, 0.f, 1.f};
			pRenderer->Draw2dLabel(70.f, 50.f, 2, col2, false, "PLAYBACK");
		}

		char temp[255];
		sprintf(temp, "%d fp packets -- %d frames -- tp buffer contains %.2f seconds", m_pBufferFirstPerson->GetNumPackets(), m_pBuffer->GetNumFrames(), GetLiveBufferLenSeconds());
		pRenderer->Draw2dLabel(70.f, 70.f, 2, col2, false, temp);
#endif
	}

	if (m_mode == ERECSYS_RECORDING)
	{
		IViewSystem *cvs = gEnv->pGame->GetIGameFramework()->GetIViewSystem();
		IView *activeView = cvs->GetActiveView();
		if (activeView)
		{
//			static int s_count = 0;
//			static int s_val = (rand() % 10) + 5;

			{
				const SViewParams *vp = activeView->GetCurrentParams();
				IEntity *entity = gEnv->pEntitySystem->GetEntity(activeView->GetLinkedId());

				//			if (((s_count++)%s_val) == 0)
				{
					SRecording_FPChar cam;
					cam.camlocation.q = vp->rotation;
					cam.camlocation.t = vp->position;
					cam.fov = vp->fov;
					cam.frametime = gEnv->pTimer->GetAsyncTime().GetSeconds();
					cam.playerFlags = 0;
					cam.relativePosition.SetIdentity();

					if (entity)
					{
						cam.relativePosition.t = entity->GetPos() - cam.camlocation.t;
						cam.relativePosition.q = cam.camlocation.q.GetInverted() * entity->GetRotation();
						/*CryLogAlways("Relative position, t: %f, %f, %f, q: %f, %f, %f, %f",
							cam.relativePosition.t.x, cam.relativePosition.t.y, cam.relativePosition.t.z,
							cam.relativePosition.q.v.x, cam.relativePosition.q.v.y, cam.relativePosition.q.v.z, cam.relativePosition.q.w);*/
						CActor *pActor = (CActor *) g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entity->GetId());
						if (pActor)
						{
							SActorStats* actorStats = pActor->GetActorStats();
							if (!actorStats->inAir)
								cam.playerFlags |= eFPF_OnGround;
							if (actorStats->bSprinting)
								cam.playerFlags |= eFPF_Sprinting;
							if (pActor->IsSuperJumping())
								cam.playerFlags |= eFPF_SuperJump;
							if (pActor->IsThirdPerson())
								cam.playerFlags |= eFPF_ThirdPerson;
							SMovementState movementState;
							pActor->GetMovementController()->GetMovementState(movementState);
							CWeapon* pWeapon = pActor->GetWeapon(pActor->GetCurrentItemId());
							if (pWeapon)
							{
								EZoomState zoomState = pWeapon->GetZoomState();
								if (zoomState == eZS_ZoomingIn || zoomState == eZS_ZoomedIn)
								{
									cam.playerFlags |= eFPF_StartZoom;
								}
							}
							IVehicle* pVehicle = pActor->GetLinkedVehicle();
							if (pVehicle)
							{
								IEntity* pVehicleEntity = pVehicle->GetEntity();
								if (pVehicleEntity)
								{
									cam.relativePosition.t = pVehicleEntity->GetPos() - cam.camlocation.t;
									cam.relativePosition.q = cam.camlocation.q.GetInverted() * pVehicleEntity->GetRotation();
								}
							}
						}
					}
					m_pBufferFirstPerson->AddPacket(cam);
					//				s_val = (rand() % 10) + 5;
				}
			}

		}
	}

	if (m_mode == ERECSYS_PLAYBACK_START)
	{
		if (m_fppacketparts == ALL_PARTS)
		{
			CryLogAlways("Replay got first person camera data (%d bytes)...", m_firstPersonDataSize);

			float fpCamDataTime = GetFPCamDataTime();

			CryLogAlways("Replay got first person camera data (%.3f seconds)...", fpCamDataTime);
			CryLogAlways("Replay got third person camera data (%.3f seconds - %d bytes)...", GetBufferLenSeconds(), m_tpdatasize);

			float currentTime = gEnv->pTimer->GetAsyncTime().GetSeconds();

			m_recordedStartTime = m_playbackRequestTime - fpCamDataTime - g_pGameCVars->g_killcamPlaybackSyncDelay;
			m_playbackStartTime = currentTime;

			DropTPFramesUpTo(m_recordedStartTime);

			m_torsoAimIK.Reset();
			m_torsoAimIK.Enable();
			m_weaponFPAiming.SnapSwitch(true);
			m_weaponFPAiming.SetActive(true);

			// Deactivate flash bang effect
			DisableFlashBangEffect();

			// Remove any existing tracers
			g_pGame->GetWeaponSystem()->GetTracerManager().Reset();

			PlaybackStartingSetupEntities();

			// Initialise the first person camera position
			FillOutFPCamera(&m_firstPersonCharacter, 0);
		}
		else
		{
			return;
		}
	}

	if (m_mode == ERECSYS_PLAYBACK || m_mode == ERECSYS_PLAYBACK_START)
	{
		if (m_tpdatasize <= 0)
		{
			StopPlayback();
		}
		else
		{
			m_recordingPlaybackUpdate = true;
			UpdatePlayback(frameTime);
			m_recordingPlaybackUpdate = false;
		}
	}
#endif // RECORDING_SYSTEM_ENABLE
}

float CRecordingSystem::GetFPCamDataTime()
{
	SRecording_FPChar* pFPCharData = (SRecording_FPChar*)m_firstPersonData;

	if (m_firstPersonDataSize == 0 || pFPCharData[0].type != eFPP_FPChar)
		return 0;

	float startTime = pFPCharData[0].frametime;
	float endTime = startTime;

	int index = 0;
	while (index * sizeof(SRecording_FPChar) < m_firstPersonDataSize)
	{
		if (pFPCharData[index].type != eFPP_FPChar)
		{
			break;
		}

		endTime = pFPCharData[index].frametime;
		index++;
	}

	return endTime - startTime;
}

void CRecordingSystem::UpdatePlayback(float frameTime)
{
	float time = gEnv->pTimer->GetAsyncTime().GetSeconds();

	const int MaxIterations = 10;
	int i;
	for (i=0; i<MaxIterations; i++)
	{
		float recordedFrameTime = PlaybackRecordedFrame(frameTime);

		float timeSinceRecordedStart = recordedFrameTime - m_recordedStartTime;
		float timeSincePlaybackStart = time - m_playbackStartTime;

		//CryLog("frametime %.2f time %.2f timesinceplay %.4f recordtime %.4f", recordedFrameTime, time, timeSincePlaybackStart, timeSinceRecordedStart);

		FillOutFPCamera(&m_firstPersonCharacter, timeSinceRecordedStart);
		PlaybackRecordedFirstPersonActions(timeSinceRecordedStart);

		if (m_playbackMode == ePM_FirstPerson || m_playbackMode == ePM_ThirdPerson)
		{
			if (m_firstPersonCharacter.playerFlags & eFPF_ThirdPerson)
				m_playbackMode = ePM_ThirdPerson;
			else
				m_playbackMode = ePM_FirstPerson;
		}

		float timeDifference = timeSinceRecordedStart - timeSincePlaybackStart;
		const float TimeDifferenceTolerance = 0.05f;
		if (timeDifference > -TimeDifferenceTolerance)
		{
			if (timeDifference > TimeDifferenceTolerance)
			{
				// Playback is ahead of recording, let's wait a while
				float sleeptime = timeDifference - TimeDifferenceTolerance;
				CrySleep((int) (sleeptime * 1000));
				CryLog("KillCam playback is ahead of recording by %.4f seconds, sleeping", timeDifference);
			}
			break;
		}
		else
		{
			// Playback is lagging compared to recording, we should play the next frame straight away to try and catch up
			CryLog("KillCam playback is lagging by %.4f seconds, playing next frame to catch up", -timeDifference);
		}
	}

	if ((m_playbackMode == ePM_FirstPerson) != (m_prevPlaybackMode == ePM_FirstPerson))
	{
		// When switching to and from first person playback update the killer entity
		TReplayActorMap::iterator itKiller = m_replayActors.find(m_killer);
		if (itKiller != m_replayActors.end())
		{
			// Don't need to switch if the killer is sitting on a vehicle
			if (itKiller->second.vehicleId == 0)
			{
				IEntity* pKillerEntity = gEnv->pEntitySystem->GetEntity(itKiller->second.chr_id);
				if (pKillerEntity)
				{
					OnPlayerFirstPersonChange(pKillerEntity, itKiller->second.gun_id, m_playbackMode == ePM_FirstPerson);
				}
			}
		}
		m_prevPlaybackMode = m_playbackMode;
	}
}

float CRecordingSystem::PlaybackRecordedFrame(float frameTime)
{
	REPLAY_EVENT_GUARD

	float recordedFrameTime = 0.f;
	bool firstframe = true;
	uint8 *startingplace = m_tpdatabuffer;
	uint8 *endingplace = startingplace + m_tpdatasize;

	while (startingplace < endingplace)
	{
		SRecording_Packet *packet = (SRecording_Packet *) startingplace;

		switch(packet->type)
		{
		case eRBPT_FrameData:
			if (firstframe)
			{
				firstframe = false;
			}
			else
			{
				m_mode = ERECSYS_PLAYBACK;
				DropTPFrame(false);
				return recordedFrameTime;
			}
			recordedFrameTime = ((SRecording_FrameData*)startingplace)->frametime;
			break;
		case eTPP_OnShoot:
			{
				SRecording_OnShoot shoot;
				memcpy(&shoot, startingplace, sizeof(SRecording_OnShoot));
				TReplayActorMap::iterator find_iterator = m_replayActors.find(shoot.shooter);

				if (find_iterator != m_replayActors.end())
				{
					SReplayActorInfo srai = find_iterator->second;
					IGameFramework *pGameFramework = gEnv->pGame->GetIGameFramework();
					IItemSystem		*pItemSystem = pGameFramework->GetIItemSystem();

					if (IItem * pItem = pItemSystem->GetItem(srai.gun_id))
					{
						CWeapon *cw = static_cast<CWeapon *>(pItem->GetIWeapon());

						if (cw)
						{
							IFireMode *ifm = cw->GetFireMode(cw->GetCurrentFireMode());
							if (ifm)
							{
								ifm->ReplayShoot();
							}
						}
					}
				}
				break;
			}
		case eTPP_TPChar:
			{
				UpdateThirdPersonPosition((SRecording_TPChar*)startingplace, frameTime);
				break;
			}
		case eTPP_WeaponSelect:
			{
				ApplyWeaponSelect((SRecording_WeaponSelect*)startingplace);
				break;
			}
		case eTPP_VehicleChange:
			{
				ApplyVehicleChange((SRecording_VehicleChange*)startingplace);
				break;
			}
		case eTPP_ActorAnim:
		case eTPP_ActorUpperAnim:
		case eTPP_WeaponAnim:
			{
				ApplyAnimPacket((SRecording_Animation*)startingplace);
				break;
			}
		case eTPP_SpawnCustomParticle:
			{
				ApplySpawnCustomParticle((SRecording_SpawnCustomParticle*)startingplace);
				break;
			}
		case eTPP_ParticleCreated:
			{
				ApplyParticleCreated((SRecording_ParticleCreated*)startingplace);
				break;
			}
		case eTPP_ParticleDeleted:
			{
				ApplyParticleDeleted((SRecording_ParticleDeleted*)startingplace);
				break;
			}
		case eTPP_EntitySpawn:
			{
				ApplyEntitySpawn((SRecording_EntitySpawn*)startingplace);
				break;
			}
		case eTPP_EntityRemoved:
			{
				ApplyEntityRemoved((SRecording_EntityRemoved*)startingplace);
				break;	
			}
		case eTPP_EntityLocation:
			{
				ApplyEntityLocation((SRecording_EntityLocation*)startingplace);
				break;
			}
		case eTPP_PlaySound:
			{
				ApplyPlaySound((SRecording_PlaySound*)startingplace);
				break;
			}
		case eTPP_StopSound:
			{
				ApplyStopSound((SRecording_StopSound*)startingplace);
				break;
			}
		case eTPP_BulletTrail:
			{
				ApplyBulletTrail((SRecording_BulletTrail*)startingplace);
				break;
			}
		default:
			{
				CRY_ASSERT_MESSAGE(false, "KillCam replay unrecognised packet type");
				break;
			}
		}

		startingplace = startingplace + packet->size;
	}

	DropTPFrame(false);
	return recordedFrameTime;
}

void CRecordingSystem::PostUpdate()
{
	if (m_mode == ERECSYS_PLAYBACK)
	{
		UpdateFirstPersonPosition();
		// Update the position of the projectile we are following here to avoid frame lag
		if (m_projectileId != 0 && !m_newProjectilePosition.IsIdentity())
		{
			TReplayEntityMap::iterator itProjectile = m_replayEntities.find(m_projectileId);
			if (itProjectile != m_replayEntities.end())
			{
				EntityId replayEntityId = itProjectile->second;
				IEntity *pReplayEntity = gEnv->pEntitySystem->GetEntity(replayEntityId);
				if (pReplayEntity)
				{
					pReplayEntity->SetPosRotScale(m_newProjectilePosition.t, m_newProjectilePosition.q, Vec3(1.0f));
				}
			}
		}
	}
}

void CRecordingSystem::UpdateView(SViewParams &viewParams)
{
	QuatT targetCameraPosition = IDENTITY;
	// Get the target camera position from the projectile we are following
	if (m_projectileId != 0)
	{
		TReplayEntityMap::iterator itProjectileEntity = m_replayEntities.find(m_projectileId);
		if (itProjectileEntity != m_replayEntities.end())
		{
			IEntity* pProjectileEntity = gEnv->pEntitySystem->GetEntity(itProjectileEntity->second);
			if (pProjectileEntity)
			{
				Vec3 projPos = pProjectileEntity->GetWorldPos();
				if (!projPos.IsZero())
				{
					TReplayActorMap::iterator itVictim = m_replayActors.find(g_pGame->GetIGameFramework()->GetClientActorId());
					if (itVictim != m_replayActors.end())
					{
						IEntity* pVictimEntity = gEnv->pEntitySystem->GetEntity(itVictim->second.chr_id);
						if (pVictimEntity)
						{
							m_playbackMode = ePM_ProjectileFollow;
							Vec3 victimFocusPos = pVictimEntity->GetWorldPos();
							Vec3 relative = victimFocusPos - projPos;
							float distToVictim = relative.NormalizeSafe();
							float cameraDist = g_pGameCVars->g_killcamProjectileDistance;
							if (cameraDist + distToVictim < g_pGameCVars->g_killcamProjectileMinimumVictimDist)
							{
								cameraDist = g_pGameCVars->g_killcamProjectileMinimumVictimDist - distToVictim;
							}
							Vec3 camPos = projPos - relative * cameraDist;
							camPos.z += g_pGameCVars->g_killcamProjectileHeightOffset;
							victimFocusPos.z += g_pGameCVars->g_killcamProjectileVictimHeightOffset;
							Vec3 direction = victimFocusPos - camPos;
							ray_hit rayHit;
							// TODO: Could defer this line test if it proves to be a performance problem
							if (gEnv->pPhysicalWorld->RayWorldIntersection(victimFocusPos, -direction, ent_static, rwi_stop_at_pierceable, &rayHit, 1))
							{
								camPos = rayHit.pt;
							}
							direction.Normalize();
							targetCameraPosition.t = camPos;
							targetCameraPosition.q = Quat::CreateRotationVDir(direction);
							if (m_projectileType == ePM_Kvolt)
							{
								m_cameraSmoothing = true;
							}
							else if (m_projectileType == ePM_Grenade)
							{
								if (distToVictim <= g_pGameCVars->g_killcamGrenadeSmoothingDist)
								{
									m_cameraSmoothing = true;
								}
							}
						}
					}
				}
			}
		}
	}
	if (m_playbackMode == ePM_ProjectileFollow && targetCameraPosition.IsIdentity())
	{
		// The projectile must have exploded and is no longer valid, switch to static camera mode.
		m_playbackMode = ePM_Static;
		m_cameraSmoothing = true;
	}
	if (m_playbackMode == ePM_FirstPerson || m_playbackMode == ePM_ThirdPerson)
	{
		targetCameraPosition = m_firstPersonCharacter.camlocation;
		viewParams.fov = m_firstPersonCharacter.fov;
		m_cameraSmoothing = false;
		if (m_playbackMode == ePM_ThirdPerson)
		{
			TReplayActorMap::iterator itKiller = m_replayActors.find(m_killer);
			if (itKiller != m_replayActors.end())
			{
				EntityId vehicleId = itKiller->second.vehicleId;
				if (vehicleId != 0)
				{
					IEntity* pVehicle = gEnv->pEntitySystem->GetEntity(vehicleId);
					if (pVehicle)
					{
						// Position the camera using the vehicle as a reference point to avoid jittering
						targetCameraPosition.t = pVehicle->GetPos() - m_firstPersonCharacter.relativePosition.t;
					}
				}
			}
		}
	}
	else if (m_playbackMode == ePM_Static)
	{
		// Keep the last camera position
		targetCameraPosition.t = viewParams.GetPositionLast();
		targetCameraPosition.q = viewParams.GetRotationLast();
		// But look towards the victim
		TReplayActorMap::iterator itVictim = m_replayActors.find(g_pGame->GetIGameFramework()->GetClientActorId());
		if (itVictim != m_replayActors.end())
		{
			IEntity* pVictimEntity = gEnv->pEntitySystem->GetEntity(itVictim->second.chr_id);
			if (pVictimEntity)
			{
				Vec3 victimFocusPos = pVictimEntity->GetWorldPos();
				victimFocusPos.z += g_pGameCVars->g_killcamProjectileVictimHeightOffset;
				Vec3 direction = victimFocusPos - targetCameraPosition.t;
				direction.Normalize();
				targetCameraPosition.q = Quat::CreateRotationVDir(direction);
			}
		}
	}
	if (m_cameraSmoothing && g_pGameCVars->g_killcamSmoothing > 0)
	{
		QuatT prevLoc(viewParams.GetPositionLast(), viewParams.GetRotationLast());
		QuatT smoothedLoc;
		// Get a frame rate independant smoothing factor
		float fFrameTime = gEnv->pTimer->GetFrameTime();
		float weight = pow(0.5f, fFrameTime / g_pGameCVars->g_killcamSmoothing);
		smoothedLoc.SetNLerp(targetCameraPosition, prevLoc, weight);
		viewParams.position = smoothedLoc.t;
		viewParams.rotation = smoothedLoc.q;
	}
	else
	{
		viewParams.position = targetCameraPosition.t;
		viewParams.rotation = targetCameraPosition.q;
	}
}

void CRecordingSystem::UpdateFirstPersonPosition()
{
	if (m_killer != 0 && m_playbackMode == ePM_FirstPerson)
	{
		TReplayActorMap::iterator find_iterator = m_replayActors.find(m_killer);

		if (find_iterator != m_replayActors.end())
		{
			const SReplayActorInfo &srai = find_iterator->second;
			EntityId replayEntityId = srai.chr_id;
			IEntity *replayEntity = gEnv->pEntitySystem->GetEntity(replayEntityId);

			if (replayEntity)
			{
				// If this is the killer then we need to synchronise the position with the camera position
				QuatT entityLocation = m_firstPersonCharacter.camlocation;
				entityLocation.t += m_firstPersonCharacter.relativePosition.t;
				entityLocation.q *= m_firstPersonCharacter.relativePosition.q;
				Vec3 unitVector(1, 1, 1);

				ICharacterInstance* pCharacter = replayEntity->GetCharacter(0);

				Vec3 aimDirection = m_firstPersonCharacter.camlocation.q.GetColumn1();
				Vec3 viewOffset(ZERO);

				m_weaponParams.characterInst = pCharacter;
				m_weaponParams.skelAnim = pCharacter->GetISkeletonAnim();

				bool isZoomed = false;
				IGameFramework *pGameFramework = gEnv->pGame->GetIGameFramework();
				IItemSystem *pItemSystem = pGameFramework->GetIItemSystem();
				bool releaseCameraBone = false;
				if (IItem *pItem = pItemSystem->GetItem(srai.gun_id))
				{
					viewOffset = ((CItem*)pItem)->GetParams().fp_offset;
					CWeapon *pWeapon = static_cast<CWeapon *>(pItem->GetIWeapon());
					if (pWeapon)
					{
						bool enableWeaponAim = pWeapon->UpdateAimAnims(m_weaponParams, releaseCameraBone);
						bool shouldZoom = (m_firstPersonCharacter.playerFlags & eFPF_StartZoom) != 0;
						EZoomState zoomState = pWeapon->GetZoomState();
						bool zoomStarted = (zoomState == eZS_ZoomingIn || zoomState == eZS_ZoomedIn);
						if (shouldZoom != zoomStarted)
						{
							IZoomMode* pZoomMode = pWeapon->GetZoomMode(pWeapon->GetCurrentZoomMode());
							if (pZoomMode)
							{
								if (shouldZoom)
									pZoomMode->StartZoom();
								else
									pZoomMode->StopZoom();
							}
						}
						isZoomed = pWeapon->IsZoomed();
					}
				}

				if (srai.vehicleId == 0)
				{
					replayEntity->SetPosRotScale(entityLocation.t, entityLocation.q, unitVector);

					m_torsoAimIK.SetCameraBonePinning(!releaseCameraBone);

					CIKTorsoAim_Helper::SIKTorsoParams IKParams(pCharacter, NULL, aimDirection, viewOffset, m_firstPersonCharacter.camlocation.t);
					m_torsoAimIK.Update(IKParams);
				}

				m_weaponParams.aimDirection = aimDirection;
				m_weaponParams.groundDistance = fabs_tpl(gEnv->p3DEngine->GetTerrainElevation(entityLocation.t.x, entityLocation.t.y) - entityLocation.t.z);
				m_weaponParams.isOnGround = (m_firstPersonCharacter.playerFlags & eFPF_OnGround) != 0;
				m_weaponParams.isSprinting = (m_firstPersonCharacter.playerFlags & eFPF_Sprinting) != 0;
				m_weaponParams.isZoomed = isZoomed;
				m_weaponParams.position = entityLocation.t;
				m_weaponParams.superJump = (m_firstPersonCharacter.playerFlags & eFPF_SuperJump) != 0;
				m_weaponFPAiming.Update(m_weaponParams);
			}

			if (CItem *pItem = (CItem*)g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(srai.gun_id))
			{
				if (pItem->IsMounted())
				{
					const SMountParams* pMountParams = pItem->GetMountedParams();
					QuatT location = m_firstPersonCharacter.camlocation;
					location.t -= location.q * pMountParams->fpBody_offset;		// TODO: Use fpBody_offset_ironsight when ironsighting (need smooth transition)
					pItem->GetEntity()->SetPosRotScale(location.t, location.q, Vec3(1.0f));
				}
			}
		}
		else
		{
			CryLogAlways("RecordingSystem Could not find killer with ID: %d", m_killer);
		}
	}
}

void CRecordingSystem::PhysicalizeEntity(IEntity *pEntity, bool ragdoll)
{
	SEntityPhysicalizeParams pp;
	
	pp.nSlot = 0;
	pp.mass = 80.0f; //never ragdollize without mass [Anton]
	pp.fStiffnessScale = 1200;
	pp.bCopyJointVelocities = true;

	pe_player_dimensions playerDim;
	pe_player_dynamics playerDyn;

	playerDyn.gravity.z = 15.0f;
	playerDyn.kInertia = 5.5f;

	pp.pPlayerDimensions = &playerDim;
	pp.pPlayerDynamics = &playerDyn;

	if (ragdoll)
	{
		pp.type = PE_ARTICULATED;
	}
	else
	{
		pp.type = PE_LIVING;
	}

	pEntity->Physicalize(pp);

	IPhysicalEntity *pPhysicalEntity = pEntity->GetPhysics();
	if (pPhysicalEntity)
	{
		if (ragdoll)
		{
			pe_simulation_params sp;
			sp.gravity = sp.gravityFreefall = Vec3(0, 0, -13.f);
			sp.dampingFreefall = 0.0f;
			sp.mass = pp.mass * 2.0f;
			pPhysicalEntity->SetParams(&sp);

			ICharacterInstance *pCharacter = pEntity->GetCharacter(0);
			if (pCharacter)
			{
				pCharacter->GetISkeletonAnim()->StopAnimationsAllLayers();
				pCharacter->GetISkeletonPose()->SetLookIK(false,0,ZERO);
				//pCharacter->EnableStartAnimation(false);
				pCharacter->GetISkeletonPose()->Fall();
			}
			
			// Make sure the ragdolls don't collide with other objects not in the replay
			pe_params_part pp;
			pp.flagsAND = 0;
			pp.flagsColliderAND = pp.flagsColliderOR = geom_colltype6;
			pPhysicalEntity->SetParams(&pp);
		}
	}
}

void CRecordingSystem::UpdateThirdPersonPosition(const SRecording_TPChar *tpchar, float frameTime)
{
	IEntity *pReplayEntity = NULL;
	EntityId gunId = 0;
	TReplayActorMap::iterator find_iterator = m_replayActors.find(tpchar->eid);
	bool updatePosition = ((tpchar->playerFlags & eTPF_Ragdoll) == 0);

	if (find_iterator != m_replayActors.end())
	{
		SReplayActorInfo& srai = find_iterator->second;
		pReplayEntity = gEnv->pEntitySystem->GetEntity(srai.chr_id);
		if (srai.vehicleId != 0 || (tpchar->eid == m_killer && m_playbackMode == ePM_FirstPerson))
		{
			// Don't update the player if he is sitting in a vehicle or it is in first person
			updatePosition = false;
		}
		if (tpchar->playerFlags & eTPF_Dead)
			srai.flags |= eRAF_Dead;
		else
			srai.flags &= ~eRAF_Dead;
		if (tpchar->playerFlags & eTPF_HasThreatDetectorPerk)
			srai.flags |= eRAF_HasThreatDetectorPerk;
		else
			srai.flags &= ~eRAF_HasThreatDetectorPerk;
		gunId = srai.gun_id;
	}
	else
	{
		TReplayEntityMap::iterator itReplayEntity = m_replayEntities.find(tpchar->eid);
		if (itReplayEntity != m_replayEntities.end())
		{
			pReplayEntity = gEnv->pEntitySystem->GetEntity(itReplayEntity->second);
		}
	}
	if (pReplayEntity)
	{
		IPhysicalEntity* pPhysicalEntity = pReplayEntity->GetPhysics();
		bool isRagdoll = (pPhysicalEntity && pPhysicalEntity->GetType() == PE_ARTICULATED);
		if ((tpchar->playerFlags & eTPF_Ragdoll) && !isRagdoll)
		{
			PhysicalizeEntity(pReplayEntity, true);
		}
		else if (!(tpchar->playerFlags & eTPF_Ragdoll) && isRagdoll)
		{
			PhysicalizeEntity(pReplayEntity, false);
		}

		bool wasCloaked = false;
		bool isCloaked = ((tpchar->playerFlags & eTPF_Cloaked) != 0);
		bool fade = (m_mode != ERECSYS_PLAYBACK_START);

		IEntityRenderProxy* pRenderProxy = static_cast<IEntityRenderProxy*>(pReplayEntity->GetProxy(ENTITY_PROXY_RENDER));
		if (pRenderProxy)
		{
			/*
			// Convert the layer params into Vec4s. This is silly because SetEffectLayerParams then converts them back to uint32
			Vec4 layerEffects[2];
			for (int i=0; i<2; i++)
			{
			uint val = tpchar->layerEffectParams[i];
			layerEffects[i] = Vec4(((val>>24)&0xFF)/255.f, ((val>>16)&0xFF)/255.f, ((val>>8)&0xFF)/255.f, ((val>>0&0xFF)/255.f));
			}
			pRenderProxy->SetEffectLayerParams(layerEffects);*/
			// Shouldn't really do this but it is more efficient than converting to Vec4 and back again
			
//Diesel cut
			//memcpy(const_cast<uint32*>(pRenderProxy->GetEffectLayerParams()), tpchar->layerEffectParams, 2 * sizeof(uint32));
			
			wasCloaked = ((pRenderProxy->GetMaterialLayersMask() & MTL_LAYER_CLOAK) != 0);
			if (isCloaked != wasCloaked)
			{
				// Update the player cloak state
				CloakEnable(pRenderProxy, isCloaked, fade);
				IGameFramework *pGameFramework = gEnv->pGame->GetIGameFramework();
				IItemSystem *pItemSystem = pGameFramework->GetIItemSystem();
				if (CItem *pItem = (CItem*)pItemSystem->GetItem(gunId))
				{
					// Also update the cloak state of the weapon and its attachments
					pItem->CloakSync(fade);
				}
			}
		}

		if (updatePosition)
		{
			Vec3 myscale(1, 1, 1);
			pReplayEntity->SetPosRotScale(tpchar->entitylocation.t, tpchar->entitylocation.q, myscale);

			ICharacterInstance* ici = pReplayEntity->GetCharacter(0);
			if (ici)
			{
				IAnimationSet* pAnimationSet = ici->GetIAnimationSet();
				ISkeletonAnim* isa = ici->GetISkeletonAnim();
				ISkeletonPose* isp = ici->GetISkeletonPose();

				float lookaheadTime = 0.0f;
				uint32 animCount = isa->GetNumAnimsInFIFO(0);
				for (uint32 animIndex = 0; animIndex < animCount; ++animIndex)
				{
					const CAnimation& anim = isa->GetAnimFromFIFO(0, animIndex);
					lookaheadTime = max(lookaheadTime, anim.m_Parametric.m_fDesiredLocalLocationLookaheadTime);
				}

				isa->SetDesiredLocalLocation(QuatT(tpchar->velocity, IDENTITY), lookaheadTime, frameTime, 1.0f);
				isp->SetAimIK((tpchar->playerFlags & eTPF_AimIk) != 0, tpchar->aimdir);
			}
		}
	}
	else
	{
		CryLogAlways("RecordingSystem Could not find player with ID: %d", tpchar->eid);
	}
}

void CRecordingSystem::CloakEnable(IEntityRenderProxy* pRenderProxy, bool enable, bool fade)
{
	// Code taken from CItem::CloakEnable
	uint8 mask = pRenderProxy->GetMaterialLayersMask();
	uint32 blend = pRenderProxy->GetMaterialLayersBlend();
	if (!fade)
	{
		blend = (blend & 0xffff00ff) | (enable?0xff00 : 0x0000);
	}
	else
	{
		blend = (blend & 0xffff00ff) | (enable?0x0000 : 0xff00);
	}

	if (enable)
		mask = mask|MTL_LAYER_CLOAK;
	else
		mask = mask&~MTL_LAYER_CLOAK;
	
	pRenderProxy->SetMaterialLayersMask(mask);
	pRenderProxy->SetMaterialLayersBlend(blend);
}

void CRecordingSystem::ApplyEntitySpawn(const SRecording_EntitySpawn *entitySpawn)
{
	if (m_replayEntities.count(entitySpawn->entityId) > 0)
	{
		// Don't spawn the same entity twice
		return;
	}

	SEntitySpawnParams params;
	if (entitySpawn->pClass)
	{
		params.pClass = entitySpawn->pClass;
	}
	else
	{
		params.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Default");
	}
	params.sName = "ReplayEntity";
	params.nFlags = (ENTITY_FLAG_NO_PROXIMITY|ENTITY_FLAG_NEVER_NETWORK_STATIC|ENTITY_FLAG_CLIENT_ONLY | entitySpawn->entityFlags);
	params.vPosition = entitySpawn->entitylocation.t;
	params.qRotation = entitySpawn->entitylocation.q;
	m_spawningReplayEntity = true;
	if (IEntity* pEntity = gEnv->pEntitySystem->SpawnEntity(params))
	{
		if (entitySpawn->szCharacterSlot)
			pEntity->LoadCharacter(0, entitySpawn->szCharacterSlot);
		if (entitySpawn->szStatObjSlot)
			pEntity->LoadGeometry(0, entitySpawn->szStatObjSlot);
		m_replayEntities[entitySpawn->entityId] = pEntity->GetId();
		/*CProjectile* pProjectile = g_pGame->GetWeaponSystem()->GetProjectile(pEntity->GetId());
		if (pProjectile)
		{
			// If this is a projectile then start off the trail effect and set the flag to indicate
			// that it is a replay projectile
			pProjectile->TrailEffect(true);
			pProjectile->SetProjectileFlags(CProjectile::ePFlag_killCamReplay, true);
		}*/
	}
	// Set up the initial states of the entity if it has one (used for the hologram)
	ApplyPlayerInitialState(entitySpawn->entityId);
	m_spawningReplayEntity = false;
}

void CRecordingSystem::ApplyEntityRemoved(const SRecording_EntityRemoved *entityRemoved)
{
	TReplayEntityMap::iterator itEntity = m_replayEntities.find(entityRemoved->entityId);
	if (itEntity != m_replayEntities.end())
	{
		EntityId replayEntityId = itEntity->second;
		gEnv->pEntitySystem->RemoveEntity(replayEntityId);
		m_replayEntities.erase(itEntity);
	}
}

void CRecordingSystem::ApplyEntityLocation(const SRecording_EntityLocation *entityLocation)
{
	if (entityLocation->entityId != m_projectileId)
	{
		TReplayEntityMap::iterator itEntity = m_replayEntities.find(entityLocation->entityId);
		if (itEntity != m_replayEntities.end())
		{
			EntityId replayEntityId = itEntity->second;
			IEntity *pReplayEntity = gEnv->pEntitySystem->GetEntity(replayEntityId);
			if (pReplayEntity)
			{
				Vec3 myscale(1, 1, 1);
				pReplayEntity->SetPosRotScale(entityLocation->entitylocation.t, entityLocation->entitylocation.q, myscale);
			}
		}
	}
	else
	{
		m_newProjectilePosition = entityLocation->entitylocation;
	}
}

void CRecordingSystem::ApplySpawnCustomParticle(const SRecording_SpawnCustomParticle *spawnCustomParticle)
{
	if (spawnCustomParticle->pParticleEffect)
	{
		IParticleEmitter* pParticleEmitter = spawnCustomParticle->pParticleEffect->Spawn(true, spawnCustomParticle->location);
	}
}

bool CRecordingSystem::ApplyParticleCreated(const SRecording_ParticleCreated *particleCreated)
{
	bool success = false;
	if (particleCreated->pParticleEffect)
	{
		//CryLogAlways("Spawning replay particle effect: %s", particleCreated->pParticleEffect->GetName());
		IParticleEffect* pParticleEffect = particleCreated->pParticleEffect;
		if (!m_killerIsFriendly)
		{
			ParticleEffectMap::iterator itReplacementEffect = m_replacementParticleEffects.find(particleCreated->pParticleEffect);
			if (itReplacementEffect != m_replacementParticleEffects.end())
			{
				pParticleEffect = itReplacementEffect->second;
			}
		}
		if (particleCreated->entityId != 0)
		{
			// Attach the particle effect to the replay entity
			TReplayEntityMap::iterator itReplayEntity = m_replayEntities.find(particleCreated->entityId);
			if (itReplayEntity != m_replayEntities.end())
			{
				IEntity *pEntity = gEnv->pEntitySystem->GetEntity(itReplayEntity->second);
				if (pEntity)
				{
					// The particle effect will be removed when the entity is destroyed
					pEntity->LoadParticleEmitter(particleCreated->entitySlot, pParticleEffect);
					success = true;
				}
			}
		}
		else
		{
			if (m_replayParticles.count(particleCreated->pParticleEmitter) == 0)
			{
				// The particle effect is not attached to any entity
				IParticleEmitter* pParticleEmitter = pParticleEffect->Spawn(true, particleCreated->location);
				// We need to keep track of it so that it can be removed at the right time
				m_replayParticles[particleCreated->pParticleEmitter] = pParticleEmitter;
				pParticleEmitter->AddRef();
				success = true;
			}
		}
	}
	return success;
}

void CRecordingSystem::ApplyParticleDeleted(const SRecording_ParticleDeleted *particleDeleted)
{
	TReplayParticleMap::iterator itParticle = m_replayParticles.find(particleDeleted->pParticleEmitter);
	if (itParticle != m_replayParticles.end())
	{
		IParticleEmitter* pReplayParticleEmitter = itParticle->second;
		gEnv->pParticleManager->DeleteEmitter(pReplayParticleEmitter);
		pReplayParticleEmitter->Release();
		m_replayParticles.erase(itParticle);
	}
}

bool CRecordingSystem::OnInputEvent(const SInputEvent &rInputEvent)
{
#if RECORDING_SYSTEM_ENABLE
	if (gEnv->pConsole->IsOpened() || !g_pGameCVars->g_killcamEnable || !g_pGameCVars->g_killcamDebug)
		return false;

	if ((rInputEvent.keyId == eKI_P && rInputEvent.deviceId == eDI_Keyboard && rInputEvent.state == eIS_Pressed))
	{
		if (m_mode == ERECSYS_PLAYBACK)
		{
			StopPlayback();
		}
		else
		{
			StartPlayback(LOCAL_PLAYER_ENTITY_ID, 0, 0, ZERO);

			uint8 *dataptr;
			size_t s = GetFirstPersonData(&dataptr);
			SetFirstPersonData(dataptr, s);
			m_debugPlayBack = true;
		}
	}

#if PETE_TEST_CODE
	if ((rInputEvent.keyId == eKI_J && rInputEvent.deviceId == eDI_Keyboard && rInputEvent.state == eIS_Pressed))
	{
		CGameRules::TPlayers  players;
		CGameRules *cgr = g_pGame->GetGameRules();
		if (cgr)
		{
			cgr->GetPlayersClient(players);

			for (CGameRules::TPlayers::const_iterator player=players.begin();player!=players.end(); ++player) 
			{
				IEntity *entity = gEnv->pEntitySystem->GetEntity(*player);

				if (entity)
				{
					CActor *pActor = (CActor *) gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(*player);
					const SActorStats *sas = pActor->GetActorStats();

					if (!sas || !sas->isRagDoll)
					{
						pActor->RagDollize(true);
					}
				}
			}
		}
	}
#endif
#endif
	return false;
}

void CRecordingSystem::RecordEntitySpawn(IEntity* pEntity, bool recordClass, uint32 entityFlags)
{
	SRecording_EntitySpawn entitySpawn;
	entitySpawn.pClass = NULL;
	entitySpawn.szStatObjSlot = NULL;
	entitySpawn.szCharacterSlot = NULL;
	entitySpawn.entityFlags = entityFlags;
	if (recordClass)
	{
		entitySpawn.pClass = pEntity->GetClass();
	}
	else
	{
		IStatObj* staticObject = pEntity->GetStatObj(0);
		if (staticObject)
			entitySpawn.szStatObjSlot = staticObject->GetFilePath();
		ICharacterInstance* characterInstance = pEntity->GetCharacter(0);
		if (characterInstance)
			entitySpawn.szCharacterSlot = characterInstance->GetFilePath();
	}
	entitySpawn.entitylocation.t = pEntity->GetPos();
	entitySpawn.entitylocation.q = pEntity->GetRotation();
	entitySpawn.entityId = pEntity->GetId();
	m_pBuffer->AddPacket(entitySpawn);
	m_recordingEntities.push_back(pEntity->GetId());
	CActor *pActor = (CActor*)gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId());
	if (pActor)
	{
		// If it is an actor then record the initial weapon (used for hologram)
		m_newActorEntities.push_back(pEntity->GetId());
	}
}

void CRecordingSystem::RecordActorWeapon()
{
	std::vector<EntityId>::iterator itEntity;
	for (itEntity = m_newActorEntities.begin(); itEntity != m_newActorEntities.end(); ++itEntity)
	{
		CActor *pActor = (CActor*)gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(*itEntity);
		if (pActor)
		{
			// If it is an actor then record the initial weapon (used for hologram)
			EntityId itemId = pActor->GetCurrentItemId(false);
			CWeapon *pCurrentWeapon = pActor->GetWeapon(itemId);
			if (pCurrentWeapon)
			{
				RecordWeaponSelection(pActor, pCurrentWeapon);
			}
		}
	}
	m_newActorEntities.clear();
}

void CRecordingSystem::RecordWeaponSelection(CActor *pActor, CWeapon *pCurrentWeapon)
{
	IEntity *pWeaponEntity = gEnv->pEntitySystem->GetEntity(pCurrentWeapon->GetEntityId());
	if (pCurrentWeapon && pWeaponEntity)
	{
		SRecording_WeaponSelect weaponSelect;
		weaponSelect.entityId = pActor->GetEntityId();
		weaponSelect.pWeaponClass = pWeaponEntity->GetClass();
		const CItem::TAccessoryMap &accessories = pCurrentWeapon->GetAccessories();
		CItem::TAccessoryMap::const_iterator itAccessory = accessories.begin();
		for (int i=0; i<MAX_WEAPON_ACCESSORIES; i++)
		{
			if (itAccessory != accessories.end())
			{
				IEntity *pAttachmentEntity = gEnv->pEntitySystem->GetEntity(itAccessory->second);
				if (pAttachmentEntity)
				{
					weaponSelect.pAccessoryClasses[i] = pAttachmentEntity->GetClass();
				}
				else
				{
					weaponSelect.pAccessoryClasses[i] = NULL;
				}
				++itAccessory;
			}
			else
			{
				weaponSelect.pAccessoryClasses[i] = NULL;
			}
		}
		weaponSelect.isRippedOff = pCurrentWeapon->IsMountable() && !pCurrentWeapon->IsMounted();
		m_pBuffer->AddPacket(weaponSelect);
	}
}

//typedef std::map<EntityId, CProjectile *>	TProjectileMap;

void CRecordingSystem::RecordEntityInfo()
{
	/*const TProjectileMap &projectiles = g_pGame->GetWeaponSystem()->GetProjectiles();
	TProjectileMap::const_iterator itProjectile;
	for (itProjectile = projectiles.begin(); itProjectile != projectiles.end(); ++itProjectile)
	{
		std::set<EntityId>::iterator itRecorded = m_recordingEntities.find(itProjectile->first);
		if (itRecorded == m_recordingEntities.end())
		{
			RecordEntitySpawn(itProjectile->second->GetEntity());
		}
	}*/
	std::vector<EntityId>::iterator itEntity;
	for (itEntity = m_recordingEntities.begin(); itEntity != m_recordingEntities.end(); )
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(*itEntity);
		if (pEntity)
		{
			CActor *pActor = (CActor*)gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(*itEntity);
			if (pActor)
			{
				// If this entity is an actor then record it as a TPChar packet which contains more information
				// such as cloak state, aim IK and velocity
				RecordTPCharPacket(pEntity, pActor);
			}
			else
			{
				SRecording_EntityLocation entityLocation;
				entityLocation.entityId = *itEntity;
				entityLocation.entitylocation.t = pEntity->GetPos();
				entityLocation.entitylocation.q = pEntity->GetRotation();
				m_pBuffer->AddPacket(entityLocation);
			}
			itEntity++;
		}
		else
		{
			// Shouldn't really ever reach this point because CRecordingSystem::OnRemove
			// should have got rid of it. But leave this here to be safe.
			SRecording_EntityRemoved entityRemoved;
			entityRemoved.entityId = *itEntity;
			m_pBuffer->AddPacket(entityRemoved);
			itEntity = m_recordingEntities.erase(itEntity);
		}
	}
}

void CRecordingSystem::RecordParticleInfo()
{
	std::vector<SRecording_ParticleCreated>::iterator itParticle;
	for (itParticle = m_newParticles.begin(); itParticle != m_newParticles.end(); itParticle++)
	{
		SRecording_ParticleCreated &particle = *itParticle;
		IParticleEmitter* pEmitter = particle.pParticleEmitter;

		if (m_mode == ERECSYS_PLAYBACK || m_mode == ERECSYS_PLAYBACK_START)
		{
			// Remove any particles from the real world during playback, I'm assuming here that the
			// particle effect is going to be fairly short lived and won't be missed if it is not
			// visible when coming out of the killcam replay.
			REPLAY_EVENT_GUARD
			gEnv->pParticleManager->DeleteEmitter(pEmitter);
		}
		else
		{
			if (m_excludedParticleEffects.count(particle.pParticleEffect) == 0)
			{
				// Record if the particle is attached to any entity or not
				particle.entityId = pEmitter->GetAttachedEntityId();
				particle.entitySlot = pEmitter->GetAttachedEntitySlot();
				m_pBuffer->AddPacket(particle);
			}
		}
	}
	m_newParticles.clear();
}

bool CRecordingSystem::OnBeforeSpawn(SEntitySpawnParams &params)
{
	if (m_recordingPlaybackUpdate && !m_spawningReplayEntity && m_recordEntityClassFilter.count(params.pClass))
	{
		// Don't allow spawning of any recorded entities during replay. This is to prevent the grenade's NetShoot
		// from spawning a grenade, otherwise we will end up spawning two grenades.
		return false;
	}
	return true;
}

void CRecordingSystem::OnSpawn(IEntity *pEntity, SEntitySpawnParams &params)
{
	//CryLogAlways("Spawning entity (%s) of type: %s", params.sName, params.pClass->GetName());
	if (m_recordEntityClassFilter.count(params.pClass) && !m_spawningReplayEntity)
	{
		RecordEntitySpawn(pEntity, false);
		if (m_mode == ERECSYS_PLAYBACK || m_mode == ERECSYS_PLAYBACK_START)
		{
			HideEntityKeepingPhysics(pEntity, true);
		}
	}
}

bool CRecordingSystem::OnRemove(IEntity *pEntity)
{
	//CryLogAlways("Removing entity (%s) of type: %s", pEntity->GetName(), pEntity->GetClass()->GetName());
	if (m_recordEntityClassFilter.count(pEntity->GetClass()))
	{
		std::vector<EntityId>::iterator itEntity;
		itEntity = std::find(m_recordingEntities.begin(), m_recordingEntities.end(), pEntity->GetId());
		if (itEntity != m_recordingEntities.end())
		{
			SRecording_EntityRemoved entityRemoved;
			entityRemoved.entityId = pEntity->GetId();
			m_pBuffer->AddPacket(entityRemoved);
			m_recordingEntities.erase(itEntity);
		}
	}
	return true;
}

void CRecordingSystem::OnCreateEmitter(IParticleEmitter* pEmitter, bool bIndependent, Matrix34 const& mLoc, const IParticleEffect* pEffect, const ParticleParams* pParams)
{
	// Only record particles which are not being spawned by a killcam replay
	if (m_replayEventGuard == 0)
	{
		SRecording_ParticleCreated particle;
		particle.pParticleEmitter = pEmitter;
		particle.pParticleEffect = const_cast<IParticleEffect*>(pEffect);
		particle.location = mLoc;
		particle.entityId = 0;//pEmitter->GetAttachedEntityId(); This won't have been set yet
		particle.entitySlot = 0;//pEmitter->GetAttachedEntitySlot();

		m_newParticles.push_back(particle);

		//CryLogAlways("Emitter Created: %s (%x)", pEffect->GetName(), pEmitter);
	}
}

void CRecordingSystem::OnDeleteEmitter(IParticleEmitter* pEmitter)
{
	// Only record particles which are not being deleted by a killcam replay
	if (m_replayEventGuard == 0)
	{
		// If the particle emitter is added and removed in the same frame then discard both
		// the particle added and removed packets
		bool deleted = false;
		std::vector<SRecording_ParticleCreated>::iterator itParticle;
		for (itParticle = m_newParticles.begin(); itParticle != m_newParticles.end(); )
		{
			if (itParticle->pParticleEmitter == pEmitter)
			{
				itParticle = m_newParticles.erase(itParticle);
				deleted = true;
			}
			else
			{
				itParticle++;
			}
		}
		
		/*TReplayParticleMap::iterator itReplayParticle;
		for (itReplayParticle = m_replayParticles.begin(); itReplayParticle != m_replayParticles.end(); )
		{
			if (itReplayParticle->second == pEmitter)
			{
				pEmitter->Release();
				itReplayParticle = m_replayParticles.erase(itReplayParticle);
			}
			else
			{
				itReplayParticle++;
			}
		}*/

		if (!deleted)
		{
			SRecording_ParticleDeleted particle;
			particle.pParticleEmitter = pEmitter;

			m_pBuffer->AddPacket(particle);
		}

		//CryLogAlways("Emitter Deleted: (%x)", pEmitter);
	}
}

void CRecordingSystem::OnSoundSystemEvent(ESoundSystemCallbackEvent event, ISound *pSound)
{
	// Only record sounds which are not being spawned by a killcam replay
	if (m_replayEventGuard == 0)
	{
		if (event == SOUNDSYSTEM_EVENT_ON_START)
		{
			// Don't record voice sounds, these tend to be audible only be a select group of players
			// Sandbox sounds are used for preloading it seems, so don't need to record those either
			ESoundSemantic semantic = pSound->GetSemantic();
			if (semantic != eSoundSemantic_OnlyVoice && semantic != eSoundSemantic_Sandbox)
			{
				const char* name = pSound->GetName();
				SRecording_PlaySound playSound;
				playSound.position = pSound->GetPosition();
				playSound.soundId = pSound->GetId();
				playSound.szName = CacheSoundString(name);
				if (playSound.szName)
				{
					playSound.flags = pSound->GetFlags();
					playSound.soundSemantic = semantic;
					m_pBuffer->AddPacket(playSound);
				}
			}
			if (m_mode == ERECSYS_PLAYBACK || m_mode == ERECSYS_PLAYBACK_START)
			{
				// Remove any sounds from the real world during playback, I'm assuming here that the
				// sound effect is going to be fairly short lived and won't be missed if it is not
				// audible when coming out of the killcam replay.
				REPLAY_EVENT_GUARD
				pSound->Stop();
			}
		}
		else if (event == SOUNDSYSTEM_EVENT_ON_STOP)
		{
			SRecording_StopSound stopSound;
			stopSound.soundId = pSound->GetId();
			m_pBuffer->AddPacket(stopSound);
		}
	}
}

bool CRecordingSystem::OnEmitTracer(const CTracerManager::STracerParams &params)
{
	if (m_mode == ERECSYS_PLAYBACK && m_replayEventGuard == 0)
	{
		return false;
	}
	return true;
}

void CRecordingSystem::OnPerkEvent(CPlayer* pPlayer, EPlayerPlugInEvent perkEvent, void *pData)
{
	if (perkEvent == EPE_BulletTrail)
	{
		STrailInfo* pTrailInfo = (STrailInfo*)pData;
		const SAmmoParams& ammoParams = pTrailInfo->pProjectile->GetAmmoParams();
		if ((ammoParams.bulletType != -1) &&
			ammoParams.physicalizationType == ePT_Particle &&
			!pTrailInfo->pProjectile->CheckAnyProjectileFlags(CProjectile::ePFlag_threatTrailEmitted))
		{
			SRecording_BulletTrail bulletTrail;
			bulletTrail.start = pTrailInfo->pProjectile->GetInitialPos();
			bulletTrail.end = pTrailInfo->pos;
			CActor* pActor = (CActor*)g_pGame->GetIGameFramework()->GetClientActor();
			bulletTrail.friendly = pActor && pActor->IsFriendlyEntity(pTrailInfo->pProjectile->GetOwnerId());
			m_pBuffer->AddPacket(bulletTrail);
		}
	}
}

void CRecordingSystem::DebugDrawAnimationData(void)
{
	// TODO: Implement me, this is only used for debugging purposes though...
	CRY_ASSERT_MESSAGE(false, "This function has not been implemented yet");
#if 0
	const float FONT_SIZE = 1.0f;
	const float FONT_COLOUR[4] = {0.0f, 1.0f, 1.0f, 1.0f};
	const float TEXT_XPOS = 100.0f;
	const float TEXT_YPOS = 400.0f;
	const float TEXT_INC  = 16.0f;

	if (m_pBuffer->GetNumPackets() > 0)
	{
		float xPos = TEXT_XPOS;
		float yPos = TEXT_YPOS;

		CRecordingBuffer::packetIterator it = m_pBuffer->GetPacketListEnd();
		it--;
		while (1)
		{
			CRecordingBuffer::packetStruct *ps = *it;
			SRecording_Packet *packet = (SRecording_Packet *) ps->data;

			switch(packet->type)
			{
			case eTPP_TPAnim:
				{
					SRecording_TPAnim *sad = (SRecording_TPAnim *) ps->data;

					IEntity *replayEntity = gEnv->pEntitySystem->GetEntity(sad->eid);
					if (replayEntity)
					{
						ICharacterInstance* ici = replayEntity->GetCharacter(0);
						IAnimationSet* pAnimationSet = ici->GetIAnimationSet();
						const char *animName = pAnimationSet->GetNameByAnimID( sad->animID );

						gEnv->pRenderer->Draw2dLabel(xPos, yPos, FONT_SIZE, FONT_COLOUR, false, animName);

						yPos += TEXT_INC;
					}
					break;
				}
			default:
				break;
			}

			if (it == m_pBuffer->GetPacketListBegin())
				break;
			it--;
		}
	}
#endif
}

void CRecordingSystem::SwapFPCameraEndian(uint8* pBuffer, size_t size, bool sending)
{
#if !defined(_RELEASE)
	if (eBigEndian)	// Swap endian on PC
	{
		for (uint8* pPos = pBuffer; pPos < pBuffer + size; )
		{
			SRecording_Packet* pPacket = (SRecording_Packet*)pPos;
			uint16 packetSize = 0;
			if (sending)
			{
				// If we are sending the data then the packet size must be accessed before swapping
				packetSize = pPacket->size;
			}
			SwapEndian(pPacket->size, true);
			SwapEndian(pPacket->type, true);
			if (!sending)
			{
				// If we are receiving the data then the packet size must be accessed after swapping
				packetSize = pPacket->size;
			}
			CRY_ASSERT_MESSAGE(packetSize <= 255, "Packet size is larger than expected, has something gone wrong with endian swapping?");
			CRY_ASSERT_MESSAGE(packetSize != 0, "Packet size is zero, the buffer is corrupt?");
			if (pPacket->type == eFPP_FPChar)
			{
				SRecording_FPChar* pFPCharPacket = (SRecording_FPChar*)pPacket;
				SwapEndian(pFPCharPacket->camlocation, true);
				SwapEndian(pFPCharPacket->relativePosition, true);
				SwapEndian(pFPCharPacket->fov, true);
				SwapEndian(pFPCharPacket->frametime, true);
				SwapEndian(pFPCharPacket->playerFlags, true);
			}
			else if (pPacket->type == eFPP_Flashed)
			{
				SRecording_Flashed* pFlashedPacket = (SRecording_Flashed*)pPacket;
				SwapEndian(pFlashedPacket->frametime, true);
				SwapEndian(pFlashedPacket->duration, true);
				SwapEndian(pFlashedPacket->blindAmount, true);
			}
			pPos += packetSize;
		}
	}
#endif
}

void CRecordingSystem::DisableFlashBangEffect()
{
	// Deactivate flash bang effect
	gEnv->p3DEngine->SetPostEffectParam("Flashbang_Time", 0.0f);
	gEnv->p3DEngine->SetPostEffectParam("FlashBang_BlindAmount", 0.0f);
	gEnv->p3DEngine->SetPostEffectParam("Flashbang_DifractionAmount", 0.0f);
	gEnv->p3DEngine->SetPostEffectParam("Flashbang_Active", 0.0f);
}

void CRecordingSystem::OnPlayerFlashed(float duration, float blindAmount)
{
	if (g_pGameCVars->g_killcamEnable)
	{
		SRecording_Flashed packet;
		packet.frametime = gEnv->pTimer->GetAsyncTime().GetSeconds();
		packet.duration = duration;
		packet.blindAmount = blindAmount;
		m_pBufferFirstPerson->AddPacket(packet);
	}
}

void CRecordingSystem::ApplyFlashed(const SRecording_Flashed *pFlashed)
{
	gEnv->p3DEngine->SetPostEffectParam("Flashbang_Time", pFlashed->duration * 2);
	gEnv->p3DEngine->SetPostEffectParam("FlashBang_BlindAmount", pFlashed->blindAmount);
	gEnv->p3DEngine->SetPostEffectParam("Flashbang_DifractionAmount", pFlashed->duration);
	gEnv->p3DEngine->SetPostEffectParam("Flashbang_Active", 1.0f);
}

void CRecordingSystem::ApplyPlaySound(const SRecording_PlaySound* pPlaySound)
{
	if (m_replaySounds.count(pPlaySound->soundId) == 0)
	{
		//CryLogAlways("Re-playing sound effect: %s", pPlaySound->szName);
		ISound* pSound = gEnv->pSoundSystem->CreateSound(pPlaySound->szName, pPlaySound->flags);
		pSound->SetPosition(pPlaySound->position);
		pSound->SetMatrix(Matrix34::CreateTranslationMat(pPlaySound->position));
		pSound->SetSemantic(pPlaySound->soundSemantic);
		pSound->Play();
		m_replaySounds[pPlaySound->soundId] = pSound->GetId();
	}
}

void CRecordingSystem::ApplyStopSound(const SRecording_StopSound* pStopSound)
{
	TReplaySoundMap::iterator itReplaySound = m_replaySounds.find(pStopSound->soundId);
	if (itReplaySound != m_replaySounds.end())
	{
		ISound* pSound = gEnv->pSoundSystem->GetSound(itReplaySound->second);
		if (pSound)
		{
			pSound->Stop();
		}
		m_replaySounds.erase(itReplaySound);
	}
}

void CRecordingSystem::ClearSoundCache()
{
	// It is only safe to clear this cache immediatly after m_pBuffer has been reset
	*(uint16*)m_soundNames = 0;
}

const char* CRecordingSystem::CacheSoundString(const char* name)
{
	// Store the name in a buffer so that it will still be valid when we want to play it back again
	// alternatively if the name is already in the buffer then return a pointer to that.
	// Ideally the sound system would allow you to create a sound by ID rather than by name, but that
	// is currently not possible.
	int offset = 0;
	while (true)
	{
		uint16 stringLength = *(uint16*)(m_soundNames + offset);
		if (stringLength > 0)
		{
			offset += 2;
			if (strcmp(m_soundNames + offset, name) == 0)
			{
				return m_soundNames + offset;
			}
			offset += stringLength + 1;
		}
		else
		{
			break;
		}
	}
	uint16 stringLength = strlen(name);
	// +1 for NULL terminator, +2 for string size, +2 for string size of next string
	if (offset + stringLength + 5 <= SOUND_NAMES_SIZE)
	{
		*(uint16*)(m_soundNames + offset) = stringLength;		// Set the size of the new string
		offset += 2;
		memcpy(m_soundNames + offset, name, stringLength + 1);		// Copy the new string into the buffer
		*(uint16*)(m_soundNames + offset + stringLength + 1) = 0;		// Assign zero size to the next string
		return m_soundNames + offset;		// Return a pointer to the cached string
	}
	else
	{
		CRY_ASSERT_MESSAGE(false, "Ran out of memory in the sound string cache");
		return NULL;
	}
}

void CRecordingSystem::ApplyBulletTrail(const SRecording_BulletTrail* pBulletTrail)
{
	// Only show bullet trails which are unfriendly to the killer. Note: pBulletTrail->friendly
	// refers to the friendlyness of the bullet to the victim (not the killer).
	if (m_killerIsFriendly != pBulletTrail->friendly)
	{
		TReplayActorMap::iterator itKiller = m_replayActors.find(m_killer);
		if (itKiller != m_replayActors.end() && itKiller->second.flags & eRAF_HasThreatDetectorPerk)
		{
			IEntity* pEntity = gEnv->pEntitySystem->GetEntity(itKiller->second.chr_id);
			Vec3 pos = pEntity->GetPos();
			float t;
			const CPerk::SPerkVars * perkVars = CPerk::GetInstance()->GetVars();
			if (Distance::Point_LinesegSq(pos, Lineseg(pBulletTrail->start, pBulletTrail->end), t) < sqr(perkVars->perk_threatTrails_BulletRenderRange))
			{
				ThreatDetectorPerk::CreateBulletTrail(pBulletTrail->start, pBulletTrail->end);
			}
		}
	}
}


