#include "StdAfx.h"
#include "AmmoParams.h"
#include "ItemParamReader.h"
#include "IAgent.h"
#include "GameRules.h"
#include "Effects/GameEffects/HitRecoilGameEffect.h"


SScaledEffectParams::SScaledEffectParams(const IItemParamsNode* scaledEffect)
: ppname(0)
, radius(0.0f)
, delay(2.0f)
, fadeInTime(1.0f)
, fadeOutTime(1.0f)
, maxValue(1.0f)
, aiObstructionRadius(5.0f)
{
	CItemParamReader reader(scaledEffect);
	reader.Read("postEffect", ppname);
	if (ppname)
	{
		reader.Read("radius", radius);
		reader.Read("delay", delay);
		reader.Read("fadeintime", fadeInTime);
		reader.Read("fadeouttime", fadeOutTime);
		reader.Read("maxvalue", maxValue);
		reader.Read("AIObstructionRadius", aiObstructionRadius);
	}
}

SCollisionParams::SCollisionParams(const IItemParamsNode* collision)
: pParticleEffect(0)
, sound(0)
, scale(1.0f)
{
	CItemParamReader reader(collision);

	reader.Read("sound", sound);
	reader.Read("scale", scale);

	const char* effect;
	reader.Read("effect", effect);

	if (effect && effect[0])
	{
		pParticleEffect = gEnv->pParticleManager->FindEffect(effect);
		if (pParticleEffect)
			pParticleEffect->AddRef();
	}
}

SCollisionParams::~SCollisionParams()
{
	SAFE_RELEASE(pParticleEffect);
}

SExplosionParams::SExplosionParams(const IItemParamsNode* explosion)
: minRadius(2.5f)
, maxRadius(5.0f)
, soundRadius(5.0f)
, minPhysRadius(2.5f)
, maxPhysRadius(5.0f)
, pressure(200.0f)
, holeSize(0.0f)
, terrainHoleSize(3.0f)
, effectScale(1)
, effectName(0)
, maxblurdist(10)
, hitTypeId(0)
, pParticleEffect(0)
, friendlyFire(eFriendyFireNone)
, type("frag")
{
	const char* effect = 0;

	CItemParamReader reader(explosion);
	reader.Read("max_radius", maxRadius);
	minRadius = maxRadius * 0.8f;
	maxPhysRadius = min(maxRadius, 5.0f);
	minPhysRadius = maxPhysRadius * 0.8f;

	reader.Read("min_radius", minRadius);
	reader.Read("sound_radius", soundRadius);
	reader.Read("min_phys_radius", minPhysRadius);
	reader.Read("max_phys_radius", maxPhysRadius);
	reader.Read("pressure", pressure);
	reader.Read("hole_size", holeSize);
	reader.Read("terrain_hole_size", terrainHoleSize);
	reader.Read("effect", effect);
	reader.Read("effect_scale", effectScale);
	reader.Read("radialblurdist", maxblurdist);
	reader.Read("type", type);

	const char* friendlyFireType = "none";
	reader.Read("friendly_fire", friendlyFireType);

	if(strcmpi(friendlyFireType, "none") == 0)
	{
		friendlyFire = eFriendyFireNone;
	}
	else if (strcmpi(friendlyFireType, "self") == 0)
	{
		friendlyFire = eFriendyFireSelf;
	}
	else if(strcmpi(friendlyFireType, "team") == 0)
	{
		friendlyFire = eFriendyFireTeam;
	}
	else
	{
		CRY_ASSERT_MESSAGE(false, "Unrecognised friendly_fire type");
	}

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules)
		hitTypeId = pGameRules->GetHitTypeId(type.c_str());

	if (effect && effect[0])
	{
		effectName=effect;
		pParticleEffect = gEnv->pParticleManager->FindEffect(effect);
		if (pParticleEffect)
			pParticleEffect->AddRef();
	}
}

SExplosionParams::~SExplosionParams()
{
	SAFE_RELEASE(pParticleEffect);
}

SFlashbangParams::SFlashbangParams(const IItemParamsNode* flashbang)
: maxRadius(25.0f),
blindAmount(0.7f),
flashbangBaseTime(2.5f)
{
	CItemParamReader reader(flashbang);
	reader.Read("max_radius", maxRadius);
	reader.Read("blindAmount", blindAmount);
	reader.Read("flashbangBaseTime", flashbangBaseTime);
}

SGravityParams::SGravityParams(const IItemParamsNode* gravityParams):
areaRadius(5.0f),
areaHeight(2.0f),
fallOff(0.0f),
fallOffSize(ZERO),
gravity(ZERO),
damping(0.0f),
uniform(true),
lifeTime(8.0f)
{
	CItemParamReader reader(gravityParams);
	reader.Read("areaRadius",areaRadius);
	reader.Read("areaHeight",areaHeight);
	reader.Read("fallOff",fallOff);
	reader.Read("gravity",gravity);
	reader.Read("fallOffSize",fallOffSize);
	reader.Read("damping",damping);
	reader.Read("uniform",uniform);
	reader.Read("lifetime", lifeTime);
}

SBeaconParams::SBeaconParams(const IItemParamsNode* beacon):
totalActiveLifeTime(10.0f),
spawnProjectile_Delay(5.0f),
spawnDistance(50.0f),
spawnAngleRange(15.0f),
activeLightColor(ZERO)
{

	CItemParamReader reader(beacon);
	reader.Read("totalActiveLifeTime", totalActiveLifeTime);

	reader.Read("spawnProjectile_Delay", spawnProjectile_Delay);
	reader.Read("spawnProjectile_FirstDelay", spawnProjectile_FirstDelay);
	reader.Read("spawnProjectile_Amount", spawnProjectile_Amount);
	reader.Read("spawnProjectile_Damage", spawnProjectile_Damage);
	reader.Read("spawnDistance", spawnDistance);
	reader.Read("spawnAngleRange", spawnAngleRange);
	reader.Read("activeLightColor", activeLightColor);

	reader.Read("spawnProjectile", spawnProjectile);
	reader.Read("activeFX", activeFX);
	reader.Read("activeSound", activeSound);

}

SImplosiveParams::SImplosiveParams(const IItemParamsNode* implosive):
lifeTime(3.0f),
radius(10.0f),
update_tick(0.5f),
effectScale(1.0f),
maxForce(5.0f),
spinImpulseScale(1.5f),
attractImpulseScale(5.0f),
upImpulseScale(1.0f)
{
	CItemParamReader reader(implosive);

	reader.Read("lifetime", lifeTime);
	reader.Read("radius", radius);
	reader.Read("update_tick", update_tick);
	reader.Read("effectScale", effectScale);
	reader.Read("effect", effect);
	reader.Read("sound", sound);
	reader.Read("maxForce", maxForce);
	reader.Read("spinImpulseScale", spinImpulseScale);
	reader.Read("attractImpulseScale", attractImpulseScale);
	reader.Read("upImpulseScale", upImpulseScale);
}


STrailParams::STrailParams(const IItemParamsNode* trail)
:	sound(0)
, effect(0)
, effect_fp(0)
, scale(1.0f)
, prime(true)
{
	CItemParamReader reader(trail);
	reader.Read("sound", sound);
	if (sound && !sound[0])
		sound = 0;

	reader.Read("effect", effect);
	if (effect)
	{
		if(!effect[0])
		{
			effect = 0;
		}
		else
		{
			// If the effect doesn't exist, then null out the ptr so the effect isn't looked for again
			if(gEnv->pParticleManager->FindEffect(effect) == 0)
			{
				effect = 0;
			}
		}
	}



	reader.Read("effect_fp", effect_fp);
	if (effect_fp && !effect_fp[0])
		effect_fp = 0;

	reader.Read("scale", scale);
	reader.Read("prime", prime);
}

SWhizParams::SWhizParams(const IItemParamsNode* whiz)
: sound(0)
, armourPiercingSound(0)
, speed(20.0f)
, probability(0.85f)
, distanceSq(4.7f)
{
	CItemParamReader reader(whiz);
	reader.Read("sound", sound);
	if (sound && !sound[0])
		sound = 0;
	reader.Read("armourPiercingSound", armourPiercingSound);
	if(armourPiercingSound && !armourPiercingSound[0])
		armourPiercingSound = 0;
	reader.Read("speed", speed);
	reader.Read("probability", probability);
	reader.Read("distance", distanceSq);
	distanceSq = distanceSq * distanceSq;
}

SLTagGrenadeParams::SLTagGrenadeParams(const IItemParamsNode* pParams)
{
	m_sticky.ReadParams(pParams->GetChild("Sticky"));
	m_ricochet.ReadParams(pParams->GetChild("Ricochet"));
}

SLTagGrenadeParams::SCommonParams::SCommonParams()
: m_safeExplosion(0.0f)
, m_boxDimension(3.0f)
, m_armTime(2.0f)
, m_ltagLifetime(0.0f)
{

}


void SLTagGrenadeParams::SCommonParams::ReadParams(const IItemParamsNode* pParams)
{
	if (pParams == NULL)
		return;

	CItemParamReader reader(pParams);

	reader.Read("ltagLifetime", m_ltagLifetime);
	reader.Read("safeexplosion", m_safeExplosion);
	reader.Read("box_dimension", m_boxDimension);
	reader.Read("armTime", m_armTime);

	reader.Read("armSound", m_armSound);
	reader.Read("disarmSound", m_disarmSound);
	reader.Read("armedSound", m_armedSound);
	reader.Read("bounceSound", m_bounceSound);

	reader.Read("fireMaterial", m_fireMaterial);
	reader.Read("armMaterial", m_armMaterial);
	reader.Read("disarmMaterial", m_disarmMaterial);
	reader.Read("armedMaterial", m_armedMaterial);
	reader.Read("bounceMaterial", m_bounceMaterial);
	reader.Read("armedSafeMaterial", m_armedSafeMaterial);

	reader.Read("fireTrail", m_fireTrail);
	reader.Read("armTrail", m_armTrail);
	reader.Read("disarmTrail", m_disarmTrail);
	reader.Read("armedTrail", m_armedTrail);
	reader.Read("bounceTrail", m_bounceTrail);
	reader.Read("armedSafeTrail", m_armedSafeTrail);

	reader.Read("explosionEffect", m_explosionEffect);

}

SGrenadeParams::SGrenadeParams(const IItemParamsNode* pParams)
: onImpactLifetime(0.f)
, collisionDamage(0.0f)
, detonateOnActorImpact(false)
, detonateOnImpact(false)
{
	CRY_ASSERT(pParams);

	CItemParamReader reader(pParams);

	reader.Read("onImpactLifetime", onImpactLifetime);
	reader.Read("collisionDamage", collisionDamage);
	reader.Read("detonateOnActorImpact", detonateOnActorImpact);
	reader.Read("detonateOnImpact", detonateOnImpact);
}

SVolatileSpikeParams::SVolatileSpikeParams(const IItemParamsNode* pParams)
: steamEffect(NULL)
, explosionEffect(NULL)
, actualExplosionTime(0)
, projectileLifeAfterExplosion(0.0f)
, visualExplosionTime(0.0f)
{
	CRY_ASSERT(pParams);

	CItemParamReader reader(pParams);

	reader.Read("projectileLifeAfterExplosion", projectileLifeAfterExplosion);
	float time = 0;
	reader.Read("actualExplosionTime", time);
	actualExplosionTime = static_cast<int>(time * 1000.0f);
	
	reader.Read("visualExplosionTime", visualExplosionTime);
}

SAmmoParams::SAmmoParams(const IItemParamsNode* pItemParams_, const IEntityClass* pEntityClass_)
: flags(0)
, serverSpawn(false)
, predictSpawn(false)
, reusable(false)
, lifetime(0.0f)
, showtime(0.0f)
, armTime(0.0f)
, aiType(AIOBJECT_NONE)
, bulletType(-1)
, hitPoints(-1)
, noBulletHits(false)
, quietRemoval(false)
, physicalizationType(ePT_None)
, mass(1.0f)
, speed(0.0f)
, maxLoggedCollisions(1)
, traceable(0)
, spin(ZERO)
, spinRandom(ZERO)
, pSurfaceType(0)
, pParticleParams(0)
, pScaledEffect(0)
, pCollision(0)
, pExplosion(0)
, pFlashbang(0)
, pWhiz(0)
, pRicochet(0)
, pTrail(0)
, pTrailUnderWater(0)
, pItemParams(0)
, pEntityClass(0)
, sleepTime(0.0f)
, fpGeometryIsCached(false)
, pGravityParams(0)
, pBeaconParams(0)
, pImplosiveParams(0)
, pLTagParams(0)
, pGrenadeParams(0)
, pVSpikeParams(0)
{
	Init(pItemParams_, pEntityClass_);
}

SAmmoParams::~SAmmoParams()
{
	delete pScaledEffect;
	delete pCollision;
	delete pExplosion;
	delete pFlashbang;
	delete pWhiz;
	delete pRicochet;
	delete pTrail;
	delete pTrailUnderWater;
	SAFE_DELETE(pGravityParams);
	SAFE_DELETE(pBeaconParams);
	SAFE_DELETE(pImplosiveParams);
	SAFE_DELETE(pLTagParams);
	SAFE_DELETE(pGrenadeParams);
	SAFE_DELETE(pVSpikeParams);
	SAFE_RELEASE(pItemParams);
	effectsMap.clear();
	materialsMap.clear();
}

void SAmmoParams::Init(const IItemParamsNode* pItemParams_, const IEntityClass* pEntityClass_)
{
	pItemParams = pItemParams_;
	pEntityClass = pEntityClass_;
	fpGeometryName = "";

	if (!pItemParams || !pEntityClass)
	{
		assert(0);
		return;
	}

	pItemParams->AddRef();

	//This ones first, they cache data for other functions
	LoadEffects();
	LoadMaterials();

	LoadFlagsAndParams();
	LoadPhysics();
	LoadGeometry();
	LoadScaledEffect();
	LoadCollision();
	LoadExplosion();
	LoadFlashbang();
	LoadTrailsAndWhizzes();
	LoadGravityParams();
	LoadBeaconParams();
	LoadImplosiveParams();
	LoadLTagGrenadeParams();
	LoadGrenadeParams();
	LoadVolatileSpikeParams();
}

void SAmmoParams::GetMemoryUsage(ICrySizer *pSizer) const
{
	pSizer->AddObject(this, sizeof(*this));
	pSizer->AddObject(fpGeometryName);
	pSizer->AddObject(pItemParams);
	
}

void SAmmoParams::LoadFlagsAndParams()
{
	const IItemParamsNode* flagsNode = pItemParams->GetChild("flags");
	if (flagsNode)
	{
		int flag=0;
		CItemParamReader reader(flagsNode);
		reader.Read("ClientOnly", flag); flags |= flag?ENTITY_FLAG_CLIENT_ONLY:0; flag=0;
		reader.Read("ServerOnly", flag); flags |= flag?ENTITY_FLAG_SERVER_ONLY:0; flag=0;
		reader.Read("ServerSpawn", serverSpawn);
		if (serverSpawn)
			reader.Read("PredictSpawn", predictSpawn);
		else
			reader.Read("Reusable", reusable);
	}

	const IItemParamsNode* paramsNode = pItemParams->GetChild("params");
	if (paramsNode)
	{
		CItemParamReader reader(paramsNode);
		reader.Read("lifetime", lifetime);
		reader.Read("showtime", showtime);
		reader.Read("armTime", armTime);
		reader.Read("sleepTime", sleepTime);
		reader.Read("bulletType",bulletType);
		reader.Read("hitPoints",hitPoints);
		reader.Read("noBulletHits",noBulletHits);
		reader.Read("quietRemoval",quietRemoval);
		
		string hitRecoil, hitRecoilArmorMode;
		reader.Read("hitRecoil", hitRecoil);
		reader.Read("hitRecoilArmorMode", hitRecoilArmorMode);
		m_hitRecoilId = CHitRecoilGameEffect::GetHitRecoilId(hitRecoil);
		m_hitRecoilIdArmorMode = CHitRecoilGameEffect::GetHitRecoilId(hitRecoilArmorMode);

		const char* typeName=0;
		reader.Read("aitype", typeName);
		
		if (typeName && typeName[0])
		{
			if (!stricmp(typeName, "grenade"))
				aiType=AIOBJECT_GRENADE;
			else if (!stricmp(typeName, "rpg"))
				aiType=AIOBJECT_RPG;
		}
	}
}

void SAmmoParams::LoadPhysics()
{
	const IItemParamsNode *physics = pItemParams->GetChild("physics");

	if (!physics)
		return;

	const char *typ=physics->GetAttribute("type");
	if (typ)
	{
		if (!strcmpi(typ, "particle"))
		{
			physicalizationType = ePT_Particle;
		}
		else if (!strcmpi(typ, "rigid"))
		{
			physicalizationType = ePT_Rigid;
		}
		else if(!strcmpi(typ, "static"))
		{
			physicalizationType = ePT_Static;
		}
		else
		{
			GameWarning("Unknow physicalization type '%s' for projectile '%s'!", typ, pEntityClass->GetName());
		}
	}

	CItemParamReader reader(physics);
	if(physicalizationType != ePT_Static)
	{
		reader.Read("mass", mass);
		reader.Read("speed", speed);
		reader.Read("max_logged_collisions", maxLoggedCollisions);
		reader.Read("traceable", traceable);
		reader.Read("spin", spin);
		reader.Read("spin_random", spinRandom);
	}

	// material
	const char *material=0;
	reader.Read("material", material);
	if (material)
	{
		pSurfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeByName(material);
	}

	if (physicalizationType == ePT_Particle)
	{
		pParticleParams = new pe_params_particle();
		float radius=0.005f;
		reader.Read("radius", radius);
		pParticleParams->thickness = radius*2.0f;
		pParticleParams->size = radius*2.0f;
		reader.Read("air_resistance", pParticleParams->kAirResistance);
		reader.Read("water_resistance", pParticleParams->kWaterResistance);
		reader.Read("gravity", pParticleParams->gravity);
		reader.Read("water_gravity", pParticleParams->waterGravity);
		reader.Read("thrust", pParticleParams->accThrust);
		reader.Read("lift", pParticleParams->accLift);
		reader.Read("min_bounce_speed", pParticleParams->minBounceVel);
		reader.Read("pierceability", pParticleParams->iPierceability);
		reader.Read("rollAxis", pParticleParams->rollAxis);

		if (pSurfaceType)
			pParticleParams->surface_idx = pSurfaceType->GetId();

		int flag=0;
		reader.Read("single_contact", flag);
		pParticleParams->flags = flag?particle_single_contact:0; flag=0;
		reader.Read("no_roll", flag);
		pParticleParams->flags |= flag?particle_no_roll:0; flag=0;
		reader.Read("no_spin", flag);
		pParticleParams->flags |= flag?particle_no_spin:0; flag=0;
		reader.Read("no_path_alignment", flag);
		pParticleParams->flags |= flag?particle_no_path_alignment:0; flag=0;

		pParticleParams->mass= mass;

		//Particles might contain a pierceability table
		const IItemParamsNode *pPierceabilityLevels = physics->GetChild("pierceabilityLevels");
		if (pPierceabilityLevels != NULL)
		{
			CItemParamReader pierceabilityLevelsReader(pPierceabilityLevels);
			pierceabilityLevelsReader.Read("level1", pierceabilityParams.damageFallOffLevels[SPierceabilityParams::ePL_Level1]);
			pierceabilityLevelsReader.Read("level2", pierceabilityParams.damageFallOffLevels[SPierceabilityParams::ePL_Level2]);
			pierceabilityLevelsReader.Read("level3", pierceabilityParams.damageFallOffLevels[SPierceabilityParams::ePL_Level3]);
			pierceabilityLevelsReader.Read("maxPenetrationThickness", pierceabilityParams.maxPenetrationThickness);
			pierceabilityLevelsReader.Read("pierceability", pierceabilityParams.pierceability);
		}
	}
}

void SAmmoParams::LoadGeometry()
{
	const IItemParamsNode *geometry = pItemParams->GetChild("geometry");
	if (!geometry)
		return;

	const IItemParamsNode *firstperson = geometry->GetChild("firstperson");
	if (firstperson)
	{
		const char *modelName = firstperson->GetAttribute("name");

		if (modelName && modelName[0])
		{
			Ang3 angles(0,0,0);
			Vec3 position(0,0,0);
			float scale=1.0f;
			firstperson->GetAttribute("position", position);
			firstperson->GetAttribute("angles", angles);
			firstperson->GetAttribute("scale", scale);

			fpLocalTM = Matrix34(Matrix33::CreateRotationXYZ(DEG2RAD(angles)));
			fpLocalTM.Scale(Vec3(scale, scale, scale));
			fpLocalTM.SetTranslation(position);

			fpGeometryName = modelName;
		}
	}
}

void SAmmoParams::CacheGeometry() const
{
	if(fpGeometryIsCached == false)
	{
		g_pGame->GetIGameFramework()->GetIItemSystem()->CacheGeometry(pItemParams->GetChild("geometry"));
		fpGeometryIsCached = true;
	}
}

void SAmmoParams::LoadScaledEffect()
{
	const IItemParamsNode* scaledEffect = pItemParams->GetChild("scaledeffect");
	if (scaledEffect)
	{
		pScaledEffect = new SScaledEffectParams(scaledEffect);
		if (!pScaledEffect->ppname)
		{
			delete pScaledEffect;
			pScaledEffect = 0;
		}
	}
}

void SAmmoParams::LoadCollision()
{
	const IItemParamsNode* collision = pItemParams->GetChild("collision");
	if (collision)
	{
		pCollision = new SCollisionParams(collision);
		if (!pCollision->pParticleEffect && !pCollision->sound)
		{
			delete pCollision;
			pCollision = 0;
		}
	}
}

void SAmmoParams::LoadExplosion()
{
	const IItemParamsNode* explosion = pItemParams->GetChild("explosion");
	if (explosion)
		pExplosion = new SExplosionParams(explosion);
}

void SAmmoParams::LoadFlashbang()
{
	const IItemParamsNode* flashbang = pItemParams->GetChild("flashbang");
	if (flashbang)
		pFlashbang = new SFlashbangParams(flashbang);
}

void SAmmoParams::LoadGravityParams()
{
	const IItemParamsNode* gravity = pItemParams->GetChild("gravity");
	if(gravity)
		pGravityParams = new SGravityParams(gravity);
}

void SAmmoParams::LoadImplosiveParams()
{
	const IItemParamsNode* implosive = pItemParams->GetChild("implosive");
	if(implosive)
		pImplosiveParams = new SImplosiveParams(implosive);
}

void SAmmoParams::LoadBeaconParams()
{
	const IItemParamsNode* beacon = pItemParams->GetChild("beacon");
	if(beacon)
		pBeaconParams = new SBeaconParams(beacon);
}

void SAmmoParams::LoadTrailsAndWhizzes()
{
	const IItemParamsNode* whiz = pItemParams->GetChild("whiz");
	if (whiz)
	{
		pWhiz = new SWhizParams(whiz);
		if (!pWhiz->sound)
		{
			delete pWhiz;
			pWhiz = 0;
		}
	}
	
	const IItemParamsNode* ricochet = pItemParams->GetChild("ricochet");
	if (ricochet)
	{
		pRicochet = new SWhizParams(ricochet);
		if (!pRicochet->sound)
		{
			delete pRicochet;
			pRicochet = 0;
		}
	}

	const IItemParamsNode* trail = pItemParams->GetChild("trail");
	if (trail)
	{
		pTrail = new STrailParams(trail);
		if (!pTrail->sound && !pTrail->effect)
		{
			delete pTrail;
			pTrail = 0;
		}
	}

	const IItemParamsNode* trailUnderWater = pItemParams->GetChild("trailUnderWater");
	if (trailUnderWater)
	{
		pTrailUnderWater = new STrailParams(trailUnderWater);
		if (!pTrailUnderWater->sound && !pTrailUnderWater->effect)
		{
			delete pTrailUnderWater;
			pTrailUnderWater = 0;
		}
	}
}

void SAmmoParams::LoadLTagGrenadeParams()
{
	const IItemParamsNode* pLTagGrenade = pItemParams->GetChild("LTagProgrammableGrenade");
	if(pLTagGrenade == NULL)
		return;

	pLTagParams = new SLTagGrenadeParams(pLTagGrenade);
}

void SAmmoParams::LoadGrenadeParams()
{
	const IItemParamsNode* pGrenade = pItemParams->GetChild("Grenade");
	if(pGrenade == NULL)
		return;

	pGrenadeParams = new SGrenadeParams(pGrenade);
}

void SAmmoParams::LoadEffects()
{
	const IItemParamsNode* effectListNode = pItemParams->GetChild("effects");
	if(effectListNode)
	{
		int effectCount = effectListNode->GetChildCount();
		for(int i=0; i<effectCount; i++)
		{
			const IItemParamsNode* effectNode = effectListNode->GetChild(i);
			if(effectNode)
			{
				const char* referenceName = effectNode->GetAttribute("referenceName");
				const char* name = effectNode->GetAttribute("name");

				CRY_ASSERT_MESSAGE(referenceName,"Effect listed in xml has no reference name");
				CRY_ASSERT_MESSAGE(name,"Effect listed in xml has no name");

				effectsMap[referenceName] = gEnv->pParticleManager->FindEffect(name);
			}
		}
	}
}

IParticleEffect* SAmmoParams::GetCachedEffect(const char* effectReferenceName) const
{
	if(effectReferenceName)
	{
		EffectsMap::const_iterator iter = effectsMap.find(CONST_TEMP_STRING(effectReferenceName));
		if (iter != effectsMap.end())
		{
			return iter->second;
		}
	}

	return NULL;
}

void SAmmoParams::LoadMaterials()
{
	const IItemParamsNode* materialListNode = pItemParams->GetChild("materials");
	if(materialListNode)
	{
		int materialCount = materialListNode->GetChildCount();
		for(int i=0; i<materialCount; i++)
		{
			const IItemParamsNode* materialNode = materialListNode->GetChild(i);
			if(materialNode)
			{
				const char* referenceName = materialNode->GetAttribute("referenceName");
				const char* name = materialNode->GetAttribute("name");

				CRY_ASSERT_MESSAGE(referenceName,"Material listed in xml has no reference name");
				CRY_ASSERT_MESSAGE(name,"Material listed in xml has no name");

				materialsMap[referenceName] = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(name,false);
			}
		}
	}
}

void SAmmoParams::LoadVolatileSpikeParams()
{
	const IItemParamsNode* pVolatileSpike = pItemParams->GetChild("VolatileSpike");
	if(pVolatileSpike == NULL)
		return;

	pVSpikeParams = new SVolatileSpikeParams(pVolatileSpike);

	pVSpikeParams->steamEffect = GetCachedEffect("steam");
	pVSpikeParams->explosionEffect = GetCachedEffect("explosion");

	// Read decal material
	char refName[] = "decal0";
	for(int i=0; i<10; i++)
	{
		refName[5] = i + '0';
		IMaterial* newMaterial = GetCachedMaterial(refName);
		if(newMaterial)
		{
			pVSpikeParams->decalMaterials.push_back(newMaterial);

			// Precache material's textures so decal won't pop on
			newMaterial->PrecacheChunkTextures(500.0f,0,1,NULL); 
		}
		else
		{
			break;
		}
	}
}

IMaterial* SAmmoParams::GetCachedMaterial(const char* materialReferenceName) const
{
	if(materialReferenceName)
	{
		MaterialsMap::const_iterator iter = materialsMap.find(CONST_TEMP_STRING(materialReferenceName));
		if (iter != materialsMap.end())
		{
			return iter->second;
		}
	}

	return NULL;
}
