/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: TunnellingGrenade

-------------------------------------------------------------------------
History:
- 01/05/2009 - Created By Tom Berry

*************************************************************************/
#ifndef __TunnelingGRENADE_H__
#define __TunnelingGRENADE_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Rocket.h"

static uint32 DRILLER_PROBE_TYPES = ent_terrain|ent_static|ent_sleeping_rigid|ent_rigid;
static uint32 DRILLER_PROBE_FLAGS = (geom_colltype_ray/*|geom_colltype13*/)<<rwi_colltype_bit|rwi_colltype_any|rwi_force_pierceable_noncoll|rwi_ignore_solid_back_faces|8;

struct IAttachment;

class CTunnellingGrenade : public CRocket
{
private:
	typedef CRocket inherited;

public:

	static const NetworkAspectType ASPECT_TUNNELLER_POSITION	= eEA_GameServerDynamic;
	static const NetworkAspectType ASPECT_TUNNELLER_STATIC = eEA_GameServerStatic;

	struct Flags
	{
		enum e_type
		{
			HasPlayedPenetrateFX = (1<<0),
			PenetrateFailed = (1<<1)
		};
	};

	enum TunnellerTimer
	{
		ePTIMER_DESCEND_TIME
	};

	enum e_mode
	{
		Airbourne,
		Penetrating,
		PenetrateTunnelling,
		Primed
	};

	enum EDrillingEffect
	{
		eDrillingEffect_wait,
		eDrillingEffect_request,
		eDrillingEffect_played
	};

	struct Surface
	{
		float entryDist;
		float depth;
		Vec3  entryPt;
		Vec3  exitPt;
	};

	typedef std::vector<Surface>      TSurfaceList;

	static bool CanPierceMaterialType(int matID);
	static void AnalyseSurfaces(const ray_hit *hits, int totalHits, const Vec3 &probeDir, TSurfaceList &surfaceList, bool finalPass);

	CTunnellingGrenade();
	virtual ~CTunnellingGrenade();
  
	// CRocket	
	virtual bool Init(IGameObject *pGameObject);
	virtual void Update(SEntityUpdateContext &ctx, int updateSlot);

	virtual void FullSerialize(TSerialize ser);
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);
	virtual void HandleEvent(const SGameObjectEvent &event);
	virtual void ProcessEvent(SEntityEvent &event);

	virtual void LoadGeometry();

	bool IsExiting() const { return m_exitEffect > eDrillingEffect_wait; }
	void SetExiting(bool drilling) { m_exitEffect = drilling && m_exitEffect == eDrillingEffect_wait ? eDrillingEffect_request : m_exitEffect; }

	Vec3 GetCurrentPosition() const { return m_groundPos; }
	void SetCurrentPosition(Vec3 pos) { if(!pos.IsZero()) { GetEntity()->SetPos(pos); m_groundPos = pos; } }
	
protected:

private:  

	static int s_attachNameID;

	bool DoSurfaceEffect(const char *effectName, const Vec3 &pos, const Vec3 &offset) const;

	void PlayAnimation(const char *animName, int flags);
	void PlayAnimation(const char *animName, int flags, int eventTimer, float atProp = 1.0f);

	void UpdateSound(float dt);

	void DrillActor(IEntity* pEntity, Vec3& drillPos, int jointId);
	void AttachToCharacter(ICharacterInstance* pCharacter, const char* boneName);

	EntityId GetDrilledActor() const { return m_drilledActorId; }
	void SetDrilledActor(EntityId actorId);

	uint32 m_flags;

	//--- Animations
	const char *m_enterSurfaceAnim;
	const char *m_launchAnim;
	const char *m_drillActorEffect;

	Quat	m_attachRot;
	Vec3	m_attachPos;
	Vec3	m_groundEntryPt;
	Vec3	m_groundDir;
	Vec3	m_groundExitPt;
	Vec3	m_groundPos;

	float	m_penetrationDepth;
	float m_failedExplosionScale;
	float m_drilledDist;
	
	IAttachment* m_characterAttachment;
	IAttachment* m_effectAttachment;

	EDrillingEffect m_entryEffect;
	EDrillingEffect m_exitEffect;
	
	int	m_attachJoint;
	EntityId m_drilledActorId;

	uint8	m_mode;
	uint8	m_lastReflectedMode;

	//--- Sound
	IEntitySoundProxy* m_pSoundProxy;
	tSoundID m_soundId;
	float m_soundDig;
};


#endif // __TunnelingGRENADE_H__