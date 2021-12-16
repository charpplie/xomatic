/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Fire Mode Parameters

-------------------------------------------------------------------------
History:
- 26:02:2010   13:45 : Created by Claire Allan

*************************************************************************/
#ifndef __FIREMODEPARAMS_H__
#define __FIREMODEPARAMS_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "ItemSharedParams.h"

#define ResetValue(name, defaultValue) if (defaultInit) name=defaultValue; reader.Read(#name, name)
#define ResetValueEx(name, var, defaultValue) if (defaultInit) var=defaultValue; reader.Read(name, var)

struct SDetectorBurstEffectParams
{
	SDetectorBurstEffectParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);

		if (defaultInit)
		{
			effect.clear();
			scale=1.0f;
			offset.zero();
		}

		if (params)
		{
			const IItemParamsNode *tp=params->GetChild("thirdperson");
			if (tp)
			{
				effect = tp->GetAttribute("effect");
				tp->GetAttribute("scale", scale);
				tp->GetAttribute("offset", offset);
			}

			PreLoadAssets();
		}
	};

	void PreLoadAssets()
	{
		gEnv->pParticleManager->FindEffect(effect);
	}

	void GetMemoryUsage(ICrySizer * s) const 
	{
		s->AddObject(effect);
	}

	float	scale;
	Vec3  offset;
	ItemString effect;

};

struct SDetectorBurstParams
{
	SDetectorBurstParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);

		ResetValue(boxSize, Vec3(2.0f, 10.0f, 2.0f));

		ResetValue(impulse,					500.0f);
		ResetValue(impulse_vehicle_scale,	5.0f);
		ResetValue(impulse_human_scale, 5.0);

		ResetValue(damage, 50.0f);
		ResetValue(empEffectTime, 10.0f);
		ResetValue(blastDelay,								0.0f);
		ResetValue(hit_type, "impulse_hit");
	};


	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(hit_type);
	}

	Vec3		boxSize;

	float   impulse;
	float		impulse_human_scale;
	float		impulse_vehicle_scale;
	float		damage;
	float   empEffectTime;

	float    blastDelay;
	ItemString  hit_type;

};

struct SPlantParams
{
	SPlantParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);
		string	ammo_type;

		ResetValue(ammo_type,			"c4explosive");
		if (defaultInit || !ammo_type.empty())
			ammo_type_class = gEnv->pEntitySystem->GetClassRegistry()->FindClass(ammo_type.c_str());
		ResetValue(clip_size,			3);
		ResetValue(damage,				100);
		ResetValue(delay,					0.25f);
		ResetValue(selectDelay,		1.0f);
		ResetValue(simple,				false);
	};

	IEntityClass*	ammo_type_class;
	int			damage;
	int			clip_size;
	bool		simple;

	float		delay;
	float		selectDelay;

	void GetMemoryUsage(ICrySizer * s) const
	{
	}
};

struct SImpulseConeEffectParams
{
	SImpulseConeEffectParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);

		if (defaultInit)
		{
			effect[0].clear(); effect[1].clear();
			helper[0].clear(); helper[1].clear();
			scale[0]=scale[1]=1.0f;
		}

		if (params)
		{
			const IItemParamsNode *fp=params->GetChild("firstperson");
			if (fp)
			{
				effect[0] = fp->GetAttribute("effect");
				helper[0] = fp->GetAttribute("helper");
				fp->GetAttribute("scale", scale[0]);
			}

			const IItemParamsNode *tp=params->GetChild("thirdperson");
			if (tp)
			{
				effect[1] = tp->GetAttribute("effect");
				helper[1] = tp->GetAttribute("helper");
				tp->GetAttribute("scale", scale[1]);
			}

			PreLoadAssets();
		}
	};

	void PreLoadAssets()
	{
		for (int i = 0; i < 2; i++)
			gEnv->pParticleManager->FindEffect(effect[i]);
	}

	void GetMemoryUsage(ICrySizer * s) const
	{
		for (int i=0; i<2; i++)
		{
			s->AddObject(effect[i]);
			s->AddObject(helper[i]);
		}
	}

	float	scale[2];
	ItemString effect[2];
	ItemString helper[2];

};

struct SImpulseConeParams
{
	SImpulseConeParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);

		ResetValue(hit_effect, "");
		ResetValue(spin_up_effect, "");
		ResetValue(angle,						7.5f);
		ResetValue(charge_time,			2.0f);
		ResetValue(cooldown,				0.25f);
		ResetValue(min_impulse,			50.0f);
		ResetValue(up_impulse,			500.0f);
		ResetValue(impulse,					500.0f);
		ResetValue(speed,					30.0f);		// kps
		ResetValue(impulse_dissipation,				2000.0f);	// impulse loss per second
		ResetValue(impulse_dissipation_colliding,	80000.0f);

		ResetValue(impulse_vehicle_scale,	5.0f);
		ResetValue(impulse_human_scale, 5.0);

		ResetValue(damage, 50.0f);
		ResetValue(hit_type, "impulse_hit");

		ResetValue(backimpulse_max_distance,	10.0f);
		ResetValue(backimpulse_decay,					0.85f);
		ResetValue(backimpulse_air_mult,			1.25f);
		ResetValue(backimpulse_ground_mult,		0.55f);
		ResetValue(suit_energy,								50.0f);

		ResetValue(spin_up_effect_helper, "charge_effect");
		ResetValue(spin_up_effect_helper_tp, "weapon_term");

		PreLoadAssets();
	};

	void PreLoadAssets()
	{
		gEnv->pParticleManager->FindEffect(hit_effect);
	}
	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(hit_effect);
		s->AddObject(spin_up_effect);
		s->AddObject(hit_type);
		s->AddObject(spin_up_effect_helper);
		s->AddObject(spin_up_effect_helper_tp);
	}

	float		angle;
	float		speed;

	float		charge_time;
	float		cooldown;

	float   min_impulse;
	float   impulse;
	float   up_impulse;
	float		impulse_human_scale;
	float		impulse_vehicle_scale;
	float		impulse_dissipation;
	float		impulse_dissipation_colliding;

	float		backimpulse_max_distance;
	float		backimpulse_decay;
	float		backimpulse_ground_mult;
	float		backimpulse_air_mult;

	float		suit_energy;
	float		damage;

	ItemString  hit_type;
	ItemString	hit_effect;
	ItemString  spin_up_effect;
	ItemString  spin_up_effect_helper;
	ItemString  spin_up_effect_helper_tp;
};

struct SMeleeParams
{
	SMeleeParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);
		ResetValue(helper,				"");
		ResetValue(range,					1.75f);
		ResetValue(damage,				32);
		ResetValue(damage_ai, 32);
		ResetValue(hit_type,			"melee");
		ResetValue(impulse,				50.0f);
		ResetValue(impulse_vehicle,	50.0f);
		ResetValue(delay,					0.5f);
		ResetValue(duration,			0.5f);
		ResetValue(knockdown_chance, 0.f);
		ResetValue(impulse_up_percentage, 0.f);
		ResetValue(trigger_client_reaction, false);
		const char* side = 0;
		if (!defaultInit)
			reader.Read("side", side);
		if (!side)
			meleeSide = eMeleeSide_RIGHT;
		else
		{
			if (strcmp(side, "left")==0)
				meleeSide = eMeleeSide_LEFT;
		}
	}

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(hit_type);
	}

	enum EMeleeSide
	{
		eMeleeSide_RIGHT = 1,
		eMeleeSide_LEFT
	};

	ItemString	helper;
	float				range;

	short				damage;
	short				damage_ai;
	ItemString	hit_type;

	float		impulse;
	float		impulse_vehicle;

	float		delay;
	float		duration;

	float		knockdown_chance;   // probability to knock down, 0-100. 
	float		impulse_up_percentage;

	EMeleeSide	meleeSide;
	bool trigger_client_reaction;
};

struct SThrowExParams
{
	SThrowExParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);
		ResetValue(delay, 				0.15f);
		ResetValue(drop_delay, 0.1f);
	}

	void GetMemoryUsage(ICrySizer * s) const {};

	float	delay;
	float drop_delay;
};

struct SBeamEffectParams
{
	SBeamEffectParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);

		if (defaultInit)
		{
			effect[0].clear(); effect[1].clear();
			helper[0].clear(); helper[1].clear();
			scale[0]=scale[1]=1.0f;
		}

		if (params)
		{
			const IItemParamsNode *fp=params->GetChild("firstperson");
			if (fp)
			{
				effect[0] = fp->GetAttribute("effect");
				helper[0] = fp->GetAttribute("helper");
				fp->GetAttribute("scale", scale[0]);
			}

			const IItemParamsNode *tp=params->GetChild("thirdperson");
			if (tp)
			{
				effect[1] = tp->GetAttribute("effect");
				helper[1] = tp->GetAttribute("helper");
				tp->GetAttribute("scale", scale[1]);
			}

			PreLoadAssets();
		}
	};

	void PreLoadAssets()
	{
		for (int i = 0; i < 2; i++)
			gEnv->pParticleManager->FindEffect(effect[i]);
	}

	void GetMemoryUsage(ICrySizer * s) const
	{
		for (int i=0; i<2; i++)
		{
			s->AddObject(effect[i]);
			s->AddObject(helper[i]);
		}
	}

	float	scale[2];
	ItemString effect[2];
	ItemString helper[2];

};

struct SBeamParams
{
	SBeamParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);

		ResetValue(hit_effect, "");
		ResetValue(hit_effect_scale, 1.0f);
		ResetValue(hit_decal, "");
		ResetValue(hit_decal_size, 0.45f);
		ResetValue(hit_decal_size_min, 0.f);
		ResetValue(hit_decal_lifetime, 60.0f);
		ResetValue(range,			75.0f);
		ResetValue(tick,			0.25f);
		ResetValue(ammo_tick,	0.15f);
		ResetValue(ammo_drain,2);

		PreLoadAssets();
	};

	void PreLoadAssets()
	{
		gEnv->pParticleManager->FindEffect(hit_effect);
	}
	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(hit_effect);
		s->AddObject(hit_decal);
	}


	float		hit_effect_scale;
	float		hit_decal_size;
	float		hit_decal_size_min;
	float		hit_decal_lifetime;

	float		range;
	float		tick;
	float		ammo_tick;
	int			ammo_drain;

	ItemString	hit_decal;
	ItemString	hit_effect;

};

struct SDrillerParams
{
	SDrillerParams() { Reset(); };

	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);

		ResetValue(blockMinDisplayTime, 0.1f);
		ResetValue(blockTestRange, 8.f);
	}

	float blockMinDisplayTime;
	float blockTestRange;

	void GetMemoryUsage(ICrySizer * s) const {}
};

struct SLTagGrenades
{
	SLTagGrenades()
	{
		Reset();
	}

	void Reset(const IItemParamsNode* params = 0, bool defaultInit = true)
	{
		CItemParamReader reader(params);
		ResetValueEx("sticky", grenades[ELTAGGrenadeType_STICKY],"");
		ResetValueEx("ricochet", grenades[ELTAGGrenadeType_RICOCHET],"");
	}

	void GetMemoryUsage(ICrySizer* s) const
	{
		for (int i = 0; i < ELTAGGrenadeType_LAST; ++i)
		{
			s->AddObject(grenades[i]);
		}
	}

	ItemString grenades[ELTAGGrenadeType_LAST];
};

struct SRapidParams
{
	SRapidParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);
		ResetValue(min_speed,	1.5f);
		ResetValue(max_speed,	3.0f);
		ResetValue(acceleration,	3.0f);
		ResetValue(deceleration,	-3.0f);
		ResetValue(barrel_attachment,	"");
		ResetValue(engine_attachment,	"");      
		ResetValue(camshake_rotate,	Vec3(0));
		ResetValue(camshake_shift,	Vec3(0));
		ResetValue(camshake_perShot,	0.0f);
		ResetValue(shoot_play_sound, false);
		ResetValue(min_firingTimeToStop, 0.1f);
	}

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(barrel_attachment);
		s->AddObject(engine_attachment);
	}

	Vec3 camshake_rotate;
	Vec3 camshake_shift;
	float camshake_perShot;
	float min_speed;
	float max_speed;
	float acceleration;
	float	deceleration;
	float min_firingTimeToStop;
	ItemString barrel_attachment;
	ItemString engine_attachment;		
	bool shoot_play_sound;

};

struct SChargeParams
{
	SChargeParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);
		ResetValue(time,						0.5f);
		ResetValue(max_charges,			1);
		ResetValue(shoot_on_stop,		false);
		ResetValue(reset_spinup,		false);
	};

	void GetMemoryUsage(ICrySizer* s) const {};

	float		time;
	int			max_charges;
	bool		shoot_on_stop;
	bool		reset_spinup;
};

struct SThrowParams
{
	SThrowParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);
		ResetValue(throw_delay, 0.15f);
		ResetValue(prime_delay,	0.15f);
		ResetValue(prime_timer, true);
		ResetValue(prime_enabled, true);
	}

	void GetMemoryUsage(ICrySizer * s) const {};

	float throw_delay;
	float prime_delay;
	bool	prime_timer;
	bool  prime_enabled;
};

struct SBurstParams
{
	SBurstParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);
		ResetValue(nshots,			3);
		ResetValue(rate,				32);
		ResetValue(useBurstSoundParam,		false);
	}

	void GetMemoryUsage(ICrySizer * s) const	{};

	short nshots;
	short	rate;
	bool  useBurstSoundParam;

};

struct SShotgunParams
{
	SShotgunParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);

		string modeString;

		ResetValue(pellets,				10);
		ResetValue(pelletdamage,		20);
		ResetValue(spread,				.1f);
		ResetValue(secondary_damage,	false);
		ResetValue(ai_vs_player_damage,	32);
		ResetValue(partial_reload, true);

		if(defaultInit)
		{
			loadShellOnEndModes = 0;
		}

		const char* paramstring = NULL;

		reader.Read("loadShellOnEndModes", paramstring);

		if(paramstring)
		{
			loadShellOnEndModes = 0;

			char suitmodeString[20] = "";
			strcpy(suitmodeString, paramstring);
			char* pSuitmode = &suitmodeString[0];

			while(pSuitmode[0])
			{
				int pos = 0;

				while(pSuitmode[pos] && pSuitmode[pos] != '|')
				{
					pos++;
				}

				pSuitmode[pos] = 0;

				ENanoSuitMode mode = CItemSharedParams::TargetToNanoSuitMode(pSuitmode);

				if(mode != eNanoSuitMode_Invalid)
				{
					loadShellOnEndModes |= (1 << mode);
				}

				pSuitmode += pos;
			}
		}
	}

	void GetMemoryUsage(ICrySizer * s) const { }

	short pellets;
	short	pelletdamage;
	float spread;
	bool secondary_damage;
	bool partial_reload;
	short ai_vs_player_damage;
	uint8 loadShellOnEndModes;

};

struct SEffectParams
{
	SEffectParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{ 
		if (defaultInit)
		{
			effect[0].clear(); effect[1].clear();
			helper[0].clear(); helper[1].clear();
			scale[0]=scale[1]=1.0f;
			time[0]=time[1]=0.060f;
			offset.Set(0.0f,0.0f,0.0f);
			aiVisibilityRadius = 0.0f;
		}

		if (params)
		{
			params->GetAttribute("aiVisibilityRadius", aiVisibilityRadius);

			const IItemParamsNode *fp=params->GetChild("firstperson");
			if (fp)
			{ 
				effect[0] = fp->GetAttribute("effect");
				helper[0] = fp->GetAttribute("helper");					
				fp->GetAttribute("scale", scale[0]);
				fp->GetAttribute("time", time[0]);
				fp->GetAttribute("offset", offset);
			}

			const IItemParamsNode *tp=params->GetChild("thirdperson");
			if (tp)
			{ 
				effect[1] = tp->GetAttribute("effect");
				helper[1] = tp->GetAttribute("helper");					
				tp->GetAttribute("scale", scale[1]);
				tp->GetAttribute("time", time[1]);
				tp->GetAttribute("offset",offset);
			}

			PreLoadAssets();
		}
	}
	void PreLoadAssets()
	{
		for (int i = 0; i < 2; i++)
			gEnv->pParticleManager->FindEffect(effect[i]);
	}
	void GetMemoryUsage(ICrySizer * s) const
	{
		for (int i=0; i<2; i++)
		{
			s->AddObject(effect[i]);
			s->AddObject(helper[i]);
		}
	}

	ItemString effect[2];
	ItemString helper[2];
	float	scale[2];
	float	time[2];
	float aiVisibilityRadius;
	Vec3 offset;   
};

struct STracerParams
{
	STracerParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);
		ResetValue(geometry,	"");
		ResetValue(effect,		"");
		ResetValue(geometryFP, "");
		ResetValue(effectFP,  "");
		ResetValue(speed,			200.0f);
		ResetValue(speedFP,    400.0f);
		ResetValue(lifetime,	1.5f);
		ResetValue(frequency,	1);
		ResetValue(delayBeforeDestroy, 0.f);
		ResetValueEx("helper_fp", helper[0],	"");
		ResetValueEx("helper_tp",	helper[1],	"");

		PreLoadAssets();
	};

	ItemString	geometry;
	ItemString	effect;
	ItemString  geometryFP;
	ItemString  effectFP;
	float		speed;
	float   speedFP;
	float		lifetime;
	float		delayBeforeDestroy;
	int			frequency;
	ItemString	helper[2];

	void PreLoadAssets()
	{
		gEnv->pParticleManager->FindEffect(effect);
		gEnv->pParticleManager->FindEffect(effectFP);
	}
	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(geometry);
		s->AddObject(effect);
		s->AddObject(geometryFP);
		s->AddObject(effectFP);
		s->AddObject(helper[0]);
		s->AddObject(helper[1]);
	}
};


struct SHeatingParams
{
	SHeatingParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);
		ResetValue(attack,		0.0f);
		ResetValue(duration,	5.0f);
		ResetValue(decay,			(1.0f/3.0f));

		ResetValueEx("helper_fp", helper[0],	"");
		ResetValueEx("helper_tp",	helper[1],	"");
		ResetValueEx("effect_fp", effect[0],	"");
		ResetValueEx("effect_tp",	effect[1],	"");

		PreLoadAssets();
	}

	float		attack;
	float		duration;
	float		decay;

	void PreLoadAssets()
	{
		for (int i = 0; i < 2; i++)
			gEnv->pParticleManager->FindEffect(effect[i]);
	}
	void GetMemoryUsage(ICrySizer * s) const
	{
		for (int i=0; i<2; i++)
		{
			s->AddObject(helper[i]);
			s->AddObject(effect[i]);
		}
	}

	ItemString	helper[2];
	ItemString	effect[2];
};

struct SBulletBeltParams
{
	SBulletBeltParams() { Reset(); }

	void Reset(const IItemParamsNode *params = NULL, bool defaultInit = true)
	{
		CItemParamReader reader(params);

		ResetValue(jointName,	"");
		ResetValue(beltRefillReloadFraction, 1.0f);
		ResetValue(numBullets,	0);
	}

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(jointName);
	}

	ItemString jointName;
	float beltRefillReloadFraction;
	uint16 numBullets;
};

struct SFireParams
{
	SFireParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);
		string ammo_type;
		ResetValue(suffix,				"");
		ResetValue(suffixAG,			"");
		ResetValue(rate,					400);
		ResetValue(clip_size,			30);
		ResetValue(max_clips,			20);
		ResetValue(hit_type,			"bullet");
		ResetValue(ammo_type,			"");
		if (defaultInit || !ammo_type.empty())
			ammo_type_class = gEnv->pEntitySystem->GetClassRegistry()->FindClass(ammo_type.c_str());
		ResetValue(fillAmmoReloadFraction, 1.0f);
		ResetValue(offset,				0.15f);      
		ResetValue(bullet_chamber,false);
		ResetValue(slider_layer,	"");
		ResetValue(slider_layer_time,	500);
		ResetValue(damage,				32);
		ResetValue(damage_drop_per_meter, 0.f);
		ResetValue(damage_drop_min_distance, 0.f);
		ResetValue(damage_drop_min_damage, 0.f);
		ResetValue(secondary_damage, false);
		ResetValue(ai_infiniteAmmo, false);
		ResetValue(ai_vs_player_damage, 32);
		ResetValue(ai_reload_time, 2.5f);
		ResetValue(crosshair, CHUD_Crosshair::eHCH_Normal);
		ResetValue(unzoomed_cock,	false);
		ResetValue(no_cock,				false);
		ResetValueEx("helper_fp", helper[0],	"");
		ResetValueEx("helper_tp",	helper[1],	"");
		ResetValue(barrel_count,  1);

		ResetValue(spin_up_time,	0.0f);
		ResetValue(spin_down_time,0.0f);
		ResetValue(autoaim,		false);
		ResetValue(autoaim_zoom , false);
		ResetValue(autoaim_distance, 10.f);
		ResetValue(autoaim_tolerance, 1.f);
		ResetValue(autoaim_locktime, 0.85f);
		ResetValue(autoaim_minvolume, 1.f);
		ResetValue(autoaim_maxvolume, 1000.f);
		ResetValue(autoaim_timeout, false);
		ResetValue(autoaim_autofiringdir, true);
		ResetValue(track_projectiles, false);
		ResetValue(aim_helper, false);
		ResetValue(aim_helper_delay, 0.5f);
		ResetValue(sound_variation,false);
		ResetValue(fake_fire_rate, 0);
		ResetValue(crosshair_assist_range, 0.0f);
		ResetValue(ironsight_fire_anim_damp, 1.f);
		ResetValue(powermode_fire_anim_damp, 0.5f);
		ResetValue(powermode_ffeedback_damp, 0.5f);

		ResetValue(totally_silent, false);
		ResetValue(knocks_target, false);
		ResetValue(min_damage_for_knockDown, 0 );
		ResetValue(knockdown_chance_leg, 0);
		ResetValue(min_damage_for_knockDown_leg, 0 );
		ResetValue(bullet_pierceability_modifier, 0);
		ResetValue(is_silenced, false);
	}

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(helper[0]);
		s->AddObject(helper[1]);
		s->AddObject(suffix);
		s->AddObject(suffixAG);
		s->AddObject(hit_type);
		s->AddObject(slider_layer);
	}

	short		rate;
	short		clip_size;
	short		max_clips;
	float		fillAmmoReloadFraction;
	float		offset;
	ItemString	helper[2];
	short   barrel_count;

	bool		unzoomed_cock;
	bool		no_cock;

	int			damage;
	float		damage_drop_per_meter;
	float		damage_drop_min_distance;
	float		damage_drop_min_damage;
	bool    secondary_damage;
	bool		ai_infiniteAmmo;
	int     ai_vs_player_damage;
	float		ai_reload_time;

	CHUD_Crosshair::eHCH_Types	crosshair;

	ItemString	hit_type;
	IEntityClass* ammo_type_class;

	ItemString	suffix;
	ItemString	suffixAG;

	bool		bullet_chamber;
	ItemString	slider_layer;
	int			slider_layer_time;

	float		spin_up_time;
	float		spin_down_time;

	bool	  autoaim;
	bool		autoaim_zoom;
	float   autoaim_distance;
	float   autoaim_tolerance;
	float   autoaim_locktime;
	float   autoaim_minvolume;
	float   autoaim_maxvolume;
	bool    autoaim_timeout;
	bool    autoaim_autofiringdir;
	bool		track_projectiles;
	bool		aim_helper;
	float		aim_helper_delay;

	bool  sound_variation;				//For the FY71 (usual weapon of the enemy)
	int   fake_fire_rate;					//Fake hurricanes fire rate
	float crosshair_assist_range;
	float min_damage_for_knockDown;
	int		knockdown_chance_leg;
	float min_damage_for_knockDown_leg;

	float ironsight_fire_anim_damp;
	float powermode_fire_anim_damp;
	float powermode_ffeedback_damp;

	bool totally_silent;			// this will disable the near misses and whizz perception on enemies
	bool knocks_target;				// always fall and play hit targets
	int8 bullet_pierceability_modifier; // added to bullets pierceability - allowing things like the silencer to effect the bullet property whilst still using the same ammo as unsilenced firing
	bool is_silenced;					// silencer is active - AI shouldn't react as much to bullets fired, Shooting should not cause the player to be revealed on the radar etc.
};

struct SFiremodeActions
{
	typedef std::map<ItemString, ItemString>	TFiremodeActionMap;

	SFiremodeActions() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);
		ResetValue(fire,									"fire");
		ResetValue(fire_cock,							"fire");
		ResetValue(cock,									"cock");
		ResetValue(empty_clip,						"empty_clip");
		ResetValue(shot_last_bullet,			"shot_last_bullet");
		ResetValue(reload,								"reload");
		ResetValue(reload_chamber_full,		"reload_chamber_full");
		ResetValue(reload_chamber_empty,	"reload_chamber_empty");
		ResetValue(spin_up,								"spin_up");
		ResetValue(spin_down,							"spin_down");
		ResetValue(spin_down_tail,				"spin_down_tail");
		ResetValue(overheating,						"overheating");
		ResetValue(cooldown,							"cooldown");
		ResetValue(null_fire,							"null_fire");
		ResetValue(blast,									"blast");
		ResetValue(attack,								"attack");
		ResetValue(attack_superPower,			"");
		ResetValue(attack_superJump,			"");
		ResetValue(hit,										"hit");
		ResetValue(fire_normal,						"fire_normal");
		ResetValue(fire_last2,						"fire_last2");
		ResetValue(fire_last1,						"fire_last1");
		ResetValue(fire_last0,						"fire_last0");
		ResetValue(automatic_fire,				"automatic_fire");
		ResetValue(plant,									"plant");
		ResetValue(intoPlant,							"intoPlant");
		ResetValueEx("throw", throwit,		"throw");
		ResetValueEx("drop", dropit,			"drop");
		ResetValue(hold,									"hold");
		ResetValue(prime,									"prime");
		ResetValue(pre_fire,							"pre_fire");
		ResetValue(primed_loop,						"primed_loop");
		ResetValue(charge,								"charge");
		ResetValue(uncharge,							"uncharge");
		ResetValue(rapid_fire,						"rapid_fire");
		ResetValue(stop_rapid_fire,				"stop_rapid_fire");
		ResetValue(burst_fire,						"burst_fire");
		ResetValue(last2_fire,						"burst_fire");
		ResetValue(last1_fire,						"burst_fire");
	
		if(defaultInit)
		{
			changeFiremodeActions.clear();
		}

		if(params)
		{
			const IItemParamsNode* changeFiremodeParams = params->GetChild("changeFiremode");

			if(changeFiremodeParams)
			{
				int numChildren = changeFiremodeParams->GetChildCount();

				for (int i = 0; i < numChildren; i++)
				{
					const IItemParamsNode* child = changeFiremodeParams->GetChild(i);

					ItemString firemode = child->GetAttribute("name");
					ItemString action = child->GetAttribute("value");

					changeFiremodeActions[firemode] = action;
				}
			}
		}

	}

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(fire);
		s->AddObject(fire_cock);
		s->AddObject(cock);
		s->AddObject(empty_clip);
		s->AddObject(shot_last_bullet);
		s->AddObject(reload);
		s->AddObject(reload_chamber_full);
		s->AddObject(reload_chamber_empty);
		s->AddObject(spin_up);
		s->AddObject(spin_down);
		s->AddObject(spin_down_tail);
		s->AddObject(overheating);
		s->AddObject(cooldown);
		s->AddObject(null_fire);
	}

	ItemString fire;
	ItemString fire_cock;
	ItemString cock;
	ItemString empty_clip;
	ItemString shot_last_bullet;
	ItemString reload;
	ItemString reload_chamber_full;
	ItemString reload_chamber_empty;
	ItemString spin_up;
	ItemString spin_down;
	ItemString spin_down_tail;
	ItemString overheating;
	ItemString cooldown;
	ItemString null_fire;
	ItemString blast;
	ItemString attack;
	ItemString attack_superPower;
	ItemString attack_superJump;
	ItemString hit;
	ItemString fire_normal;
	ItemString fire_last2;
	ItemString fire_last1;
	ItemString fire_last0;
	ItemString automatic_fire;
	ItemString plant;
	ItemString intoPlant;
	ItemString throwit;
	ItemString dropit;
	ItemString hold;
	ItemString prime;
	ItemString pre_fire;
	ItemString primed_loop;
	ItemString charge;
	ItemString uncharge;
	ItemString rapid_fire;
	ItemString stop_rapid_fire;
	ItemString burst_fire;
	ItemString last2_fire;
	ItemString last1_fire;

	TFiremodeActionMap changeFiremodeActions;
};

struct SInitialiseParams
{
	SInitialiseParams(){};

	void GetMemoryUsage(ICrySizer* s) const
	{
		s->AddObject(modeName);
		s->AddObject(modeType);
	}
	
	ItemString modeName;
	ItemString modeType;
	bool enabled;
	bool secondary;
	bool isMelee;
};

struct SFireModeParams
{
	SFireModeParams(){};

	void GetMemoryUsage(ICrySizer* s) const
	{	
/*	
		s->AddObject(fireparams);
		s->AddObject(tracerparams);
		s->AddObject(armourPiercingPerkTracerParams);
		s->AddObject(actions);
		s->AddObject(muzzleflash);
		s->AddObject(muzzlesmoke);
		s->AddObject(reject);
		s->AddObject(spinup);
		s->AddObject(heatingparams);
		s->AddObject(recoilparamsCopy);
		s->AddObject(spreadparamsCopy);
		s->AddObject(bulletBelletParams);
		s->AddObject(shotgunparams);
		s->AddObject(burstparams);
		s->AddObject(burstactions);
		s->AddObject(throwactions);
		s->AddObject(throwparams);
		s->AddObject(throwExActions);
		s->AddObject(throwExParams);
		s->AddObject(automaticactions);
		s->AddObject(chargeeffect);
		s->AddObject(chargeparams);
		s->AddObject(chargeactions);
		s->AddObject(rapidactions);
		s->AddObject(rapidparams);
		s->AddObject(lTagActions);
		s->AddObject(lTagGrenades);
		s->AddObject(drillerParams);
		s->AddObject(beamparams);
		s->AddObject(beamactions);
		s->AddObject(effectparams);
		s->AddObject(hitbeameffectparams);
		s->AddObject(meleeparams);
		s->AddObject(meleeactions);
		s->AddObject(coneparams);
		s->AddObject(coneactions);
		s->AddObject(coneeffectparams);
		s->AddObject(plantparams);
		s->AddObject(plantactions);
		s->AddObject(detectorburstparams);
		s->AddObject(detectorburstactions);
		s->AddObject(detectorbursteffectparams);
*/
	}

	SFireParams				fireparams;
	STracerParams			tracerparams;
	STracerParams			armourPiercingPerkTracerParams;
	SFiremodeActions	actions;
	SEffectParams			muzzleflash;
	SEffectParams			muzzlesmoke;
	SEffectParams			reject;
	SEffectParams			spinup;  
	SRecoilParams			recoilparamsCopy;		
	SProceduralRecoilParams proceduralRecoilParams;
	SSpreadParams			spreadparamsCopy;		
	SHeatingParams		heatingparams;
	SBulletBeltParams bulletBelletParams;
	SShotgunParams		shotgunparams;
	SBurstParams			burstparams;
	SThrowParams			throwparams;
	SThrowExParams		throwExParams;
	SEffectParams			chargeeffect;	
	SChargeParams			chargeparams;
	SRapidParams			rapidparams;
	SLTagGrenades			lTagGrenades;
	SDrillerParams		drillerParams;
	SBeamParams				beamparams;
	SBeamEffectParams	effectparams;
	SBeamEffectParams	hitbeameffectparams;
	SMeleeParams	meleeparams;
	SImpulseConeParams					coneparams;
	SImpulseConeEffectParams		coneeffectparams;
	SPlantParams								plantparams;
	SDetectorBurstParams				detectorburstparams;
	SDetectorBurstEffectParams	detectorbursteffectparams;
};

struct SParentFireModeParams
{
	SParentFireModeParams() {};

	typedef std::map<ItemString, SFireModeParams> TAccessoryFireModeMap;

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(baseFireMode);
		pSizer->AddObject(initialiseParams);		
		pSizer->AddObject(accessoryChangedParams);		
	}

	SFireModeParams baseFireMode;
	SInitialiseParams	initialiseParams;
	TAccessoryFireModeMap	accessoryChangedParams;
};

#endif //__FIREMODEPARAMS_H__
