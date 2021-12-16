/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------

Description: Stores all shared weapon parameters (shared by class)...
							Allows for some memory savings...

-------------------------------------------------------------------------
History:
- 30:1:2008   10:54 : Benito G.R.

*************************************************************************/
#ifndef __WEAPONSHAREDPARAMS_H__
#define __WEAPONSHAREDPARAMS_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "IronSight.h"
#include "ItemDefinitions.h"
#include "HUD/HUD_Crosshair.h"
#include "FireModeParams.h"

typedef VectorMap<EStance, Vec3>	TStanceWeaponOffset;

struct SAIWeaponOffset
{
	SAIWeaponOffset(): useEyeOffset(false){};

	bool								useEyeOffset;

	TStanceWeaponOffset	stanceWeponOffset;
	TStanceWeaponOffset	stanceWeponOffsetLeanLeft;
	TStanceWeaponOffset	stanceWeponOffsetLeanRight;
};

struct SPlayerMovementModifiers
{
	SPlayerMovementModifiers()
		: movementSpeedScale(1.0f)
		, rotationSpeedScale(1.0f)
		, coverLeanRotationSpeedScale(1.2f)
		, firingMovementSpeedScale(1.0f)
		, firingRotationSpeedScale(1.0f) {}

	void GetMemoryUsage( ICrySizer *pSizer ) const{}

	float movementSpeedScale;
	float rotationSpeedScale;
	float coverLeanRotationSpeedScale;
	float firingMovementSpeedScale;
	float firingRotationSpeedScale;
};

struct SReloadMagazineParams
{
	SReloadMagazineParams()
		: magazineEvent("")
		, magazineAttachment("")
		, supportsMagazineSwap(false) {}

	void GetMemoryUsage(ICrySizer* s) const
	{
		s->AddObject(magazineAttachment);
		s->AddObject(magazineEvent);
	}

	ItemString magazineAttachment;
	ItemString magazineEvent;
	bool supportsMagazineSwap;
};

enum EPickAndThrowGrabTypes
{
	ePTGrabType_2H = 0,
	ePTGrabType_2H_Pole,
	ePTGrabType_1H,
	ePTGrabType_NPCFront,
	ePTGrabType_NPCBack,
	ePTGrabType_MaxGrabTypes,
};

struct SPickAndThrowParams
{
	SPickAndThrowParams()
	{}
	
	struct SGrabTypeParams
	{
		ItemString helper;   // on the object to be picked
		ItemString attachment;   // on the nanosuit
		ItemString grab_action;
		ItemString throw_action;
		ItemString drop_action;
		ItemString melee_action;
		ItemString melee_hit_action;
		ItemString melee_mfxLibrary;
		SAimAnimsBlock aimAnims;
		float melee_delay;
		float melee_damage_scale;
		float melee_impulse_scale;
		float timePicking;
		float timeToStickAtPicking;

		float timeThrowing;
		float timeToFreeAtThrowing;
		float throwSpeed;
		float dropSpeedForward;

		float timeDropping;
		float timeToFreeAtDropping;
		float timeToKillGrabbedNPC;
		
		SGrabTypeParams() 
		: melee_delay(0)
		, melee_damage_scale(5.0f)
		, melee_impulse_scale(5.0f)
		, timePicking(0)
		, timeToStickAtPicking(0)
		, timeThrowing(0)
		, timeToFreeAtThrowing(0)
		, throwSpeed(0)
		, timeDropping(0)
		, timeToFreeAtDropping(0)
		, dropSpeedForward(0)
		, timeToKillGrabbedNPC(0)
		{}
		void Read( const IItemParamsNode* pNode, const char* name );
	};
	
	void GetMemoryUsage(ICrySizer* s) const {};

	SGrabTypeParams grabTypesParams[ePTGrabType_MaxGrabTypes];
};

struct SWeaponAmmoParams
{
	SWeaponAmmoParams() {};

	void GetMemoryUsage(ICrySizer* s) const
	{
		s->AddObject(ammo);
		s->AddObject(bonusAmmo);
		s->AddObject(accessoryAmmo);
		s->AddObject(minDroppedAmmo);
	}

	TAmmoVector	ammo;
	TAmmoVector	bonusAmmo;
	TAmmoVector	accessoryAmmo;
	TAmmoVector	minDroppedAmmo;
};

struct STurretSearchParams
{
	STurretSearchParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);
		ResetValue(light_helper, "");

		if (defaultInit)
		{
			hints.resize(0);
			light_helper.clear();
			light_texture.clear();
			light_material.clear();
			light_color = Vec3(1,1,1);
			light_diffuse_mul = 1.f;
			light_hdr_dyn = 0.f;
		}

		if (params)
		{
			const IItemParamsNode *phints = params->GetChild("hints");
			if (phints && phints->GetChildCount())
			{
				Vec2 h;
				int n = phints->GetChildCount();
				hints.resize(n);

				for (int i=0; i<n; ++i)
				{
					const IItemParamsNode *hint = phints->GetChild(i);
					if (hint->GetAttribute("x", h.x) && hint->GetAttribute("y", h.y))
					{
						Limit(h.x, -1.f, 1.f);
						Limit(h.y, -1.f, 1.f);
						hints[i]=h;
					}
				}
			}                
			if (const IItemParamsNode* pLight = params->GetChild("light"))
			{
				light_helper = pLight->GetAttribute("helper");
				light_texture = pLight->GetAttribute("texture");
				light_material = pLight->GetAttribute("material");
				pLight->GetAttribute("color", light_color);
				pLight->GetAttribute("diffuse_mul", light_diffuse_mul);
				pLight->GetAttribute("hdr_dyn", light_hdr_dyn);
			}
		}
	}

	void GetMemoryUsage(ICrySizer* s) const
	{
		s->AddObject(hints);
		s->AddObject(light_helper);
		s->AddObject(light_texture);
		s->AddObject(light_material);
	}

	std::vector<Vec2> hints;
	ItemString light_helper;
	ItemString light_texture;
	ItemString light_material;
	Vec3   light_color;
	float  light_diffuse_mul;
	float  light_hdr_dyn;

};

struct STurretFireParams
{
	STurretFireParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{ 
		CItemParamReader reader(params);
		ResetValue(deviation_speed, 0.f);
		ResetValue(deviation_amount, 0.f);
		ResetValue(randomness, 0.f);

		if (defaultInit)
		{
			hints.resize(0);        
		}

		if (params)
		{        
			const IItemParamsNode *phints = params->GetChild("hints");
			if (phints && phints->GetChildCount())
			{
				Vec2 h;
				int n = phints->GetChildCount();
				hints.resize(n);

				for (int i=0; i<n; ++i)
				{
					const IItemParamsNode *hint = phints->GetChild(i);
					if (hint->GetAttribute("x", h.x) && hint->GetAttribute("y", h.y))
					{ 
						hints[i]=h;
					}
				}
			}   
		}
	}

	void GetMemoryUsage(ICrySizer* s) const
	{
		s->AddObject(hints);
	}

	std::vector<Vec2> hints;
	float deviation_speed;
	float deviation_amount;
	float randomness;
};

struct STurretParams
{
	STurretParams() : desiredHoverHeight(0.f) {};

	void GetMemoryUsage(ICrySizer* s) const
	{
		s->AddObject(searchParams);
		s->AddObject(fireParams);
		s->AddObject(radarHelper);
		s->AddObject(tripodHelper);
		s->AddObject(barrelHelper);
		s->AddObject(fireHelper);
		s->AddObject(rocketHelper);
		s->AddObject(rocketFireMode);
	}

	STurretSearchParams searchParams;
	STurretFireParams		fireParams;

	ItemString radarHelper;
	ItemString tripodHelper;
	ItemString barrelHelper;
	ItemString fireHelper;
	ItemString rocketHelper;
	ItemString rocketFireMode;

	float desiredHoverHeight;
};

struct SZoomParams
{
	SZoomParams(){ Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);

		ResetValue(layer, "zoom_layer");
		ResetValue(suffix, "ironsight");
		ResetValue(suffixAG, "");
		ResetValue(dof_mask, "");
		ResetValue(blur_amount, 0.0f);
		ResetValue(blur_mask, "");
		ResetValue(hbob_ratio, 1.75f);
		ResetValue(recoil_ratio, 1.0f);

		ResetValue(zoom_in_time, 0.35f);
		ResetValue(zoom_out_time, 0.35f);
		ResetValue(zoom_out_delay, 0.35f);
		ResetValue(stage_time, 0.055f);
		ResetValue(scope_mode, false);
		ResetValue(scope_nearFov, 55.0f);

		ResetValue(hide_weapon, false);
		ResetValue(target_snap_enabled, true);

		dof = dof_mask.empty() ? false : true;

		if (defaultInit)
		{
			stages.resize(0);
			stages.push_back(1.5f);
		}

		if (params)
		{
			const IItemParamsNode *pstages=params->GetChild("stages");
			if (pstages)
			{
				stages.resize(0); 
				int n=pstages->GetChildCount();

				for (int i=0; i<n; i++)
				{
					const IItemParamsNode *stage = pstages->GetChild(i);
					float v = 1.0f; stage->GetAttribute("value", v);
					stages.push_back(v);
				}
			}
		}
	}
	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(stages);
		s->AddObject(layer);
		s->AddObject(suffix);
		s->AddObject(suffixAG);
		s->AddObject(dof_mask);
		s->AddObject(blur_mask);
	}

	std::vector<float>	stages;
	ItemString					layer;
	ItemString					suffix;
	ItemString					suffixAG;
	ItemString					dof_mask;
	float								blur_amount;
	ItemString					blur_mask;
	float								hbob_ratio;
	float								recoil_ratio;
	float								zoom_in_time;
	float								zoom_out_time;
	float								zoom_out_delay;
	float								stage_time;

	float								scope_nearFov;
	bool								scope_mode;
	bool								hide_weapon;
	bool								dof;
	bool								target_snap_enabled;
};

struct SZoomActions
{
	SZoomActions() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);
		ResetValue(zoom_in,			"zoom_in");
		ResetValue(zoom_out,		"zoom_out");
		ResetValue(idle,				"idle");
	}

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(zoom_in);
		s->AddObject(zoom_out);
		s->AddObject(idle);
	}

	ItemString	zoom_in;
	ItemString	zoom_out;
	ItemString	idle;
};

struct SZoomSway
{
	SZoomSway() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);
		ResetValue(maxX,			0.0f);
		ResetValue(maxY,		  0.0f);
		ResetValue(stabilizeTime,				3.0f);
		ResetValue(strengthScale, 0.6f);
		ResetValue(strengthScaleTime, 0.75f);
		ResetValue(minScale , 0.15f);
		ResetValue(scaleAfterFiring, 0.5f);
		ResetValue(crouchScale, 0.8f);
	}

	float maxX;
	float maxY;
	float stabilizeTime;
	float strengthScale;
	float strengthScaleTime;
	float minScale;
	float scaleAfterFiring;

	//Stance modifiers
	float	crouchScale;

	void GetMemoryUsage(ICrySizer * s) const {};
};

struct SScopeParams
{
	SScopeParams() { Reset(); };
	void Reset(const IItemParamsNode *params=0, bool defaultInit=true)
	{
		CItemParamReader reader(params);

		ResetValue(scope, "scope_default");
		ResetValue(dark_out_time, 0.15f);
		ResetValue(dark_in_time, 0.15f);
	};

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(scope);
	}

	ItemString	scope;
	float		dark_out_time;
	float		dark_in_time;
};

struct SZoomModeParams
{
	SZoomModeParams() {};

	void GetMemoryUsage(ICrySizer* s) const
	{
		s->AddObject(zoomParams);
		s->AddObject(actions);
		s->AddObject(zoomSway);
		s->AddObject(spreadModParams);
		s->AddObject(recoilModParams);
		s->AddObject(scopeParams);	
	}

	SZoomParams				zoomParams;
	SZoomActions			actions;
	SZoomSway					zoomSway;
	SSpreadModParams	spreadModParams;
	SRecoilModParams	recoilModParams;
	SScopeParams			scopeParams;
};

struct SParentZoomModeParams
{
	SParentZoomModeParams() {};

	typedef std::map<ItemString, SZoomModeParams> TAccessoryZoomModeMap;

	void GetMemoryUsage(ICrySizer* s) const 
	{
		s->AddObject(baseZoomMode);
		s->AddObject(initialiseParams);	
		s->AddObject(accessoryChangedParams);
	};

	SZoomModeParams baseZoomMode;
	SInitialiseParams initialiseParams;
	TAccessoryZoomModeMap accessoryChangedParams;
};

class CWeaponSharedParams : public IGameSharedParameters
{
	typedef std::map<int, _smart_ptr<IGameSharedParameters> > TSharedMap;
	typedef std::vector<SPlayerMovementModifiers> TMovementModifiers;

public:
	CWeaponSharedParams(): m_refs(0), m_valid(false), pickupSound(""), pPickAndThrowParams(NULL), pTurretParams(NULL) {};
	virtual ~CWeaponSharedParams();

	//IGameSharedParameters
	virtual void AddRef() const { ++m_refs; };
	virtual uint32 GetRefCount() const { return m_refs; };
	virtual void Release() const { 
		if (--m_refs == 0)
			delete this;
	};

	virtual bool Valid() const { return m_valid; };
	virtual void SetValid(bool valid) { m_valid=valid; };

	virtual void GetMemoryUsage(ICrySizer *s) const;

	virtual const char* GetDataType() const { return "WeaponParams"; }
	//~IGameSharedParameters

	void ReadWeaponParams(const IItemParamsNode* pRoot, CItemSharedParams* pItemParams);

	AIWeaponDescriptor					aiWeaponDescriptor;
	SAIWeaponOffset							aiWeaponOffsets;	
	SPlayerMovementModifiers		defaultMovementModifiers;
	SReloadMagazineParams				reloadMagazineParams;
	TMovementModifiers					zoomedMovementModifiers;

	SWeaponAmmoParams						ammoParams;
	TParentFireModeParamsVector firemodeParams;
	TParentZoomModeParamsVector zoommodeParams;
	ItemString									pickupSound;

	SPickAndThrowParams*				pPickAndThrowParams;
	STurretParams*							pTurretParams;

protected:
	void ReadFireModeParams(const IItemParamsNode* pNode, CItemSharedParams* pItemParams, const char* meleeFireMode);
	void ReadFireMode(const IItemParamsNode* pFMNode, SFireModeParams* pFireMode, bool defaultInit);
	void ReadZoomModeParams(const IItemParamsNode* pNode, CItemSharedParams* pItemParams);
	void ReadZoomMode(const IItemParamsNode* pZMNode, SZoomModeParams* pZoomMode, bool defaultInit);
	void ReadAmmoParams(const IItemParamsNode* pNode);
	void ReadReloadMagazineParams(const IItemParamsNode* pNode);
	void ReadMovementModifierParams(const IItemParamsNode* pNode);
	void ReadPickAndThrowParams(const IItemParamsNode* pNode);
	void ReadAIParams(const IItemParamsNode* pNode);
	void ReadAIOffsets(const IItemParamsNode *aiOffsetData);
	void ReadTurretParams(const IItemParamsNode* pTurret, const IItemParamsNode* pParams);
	const IItemParamsNode* FindAccessoryPatchForMode(const char* modeType, const char *modeName, const SAccessoryParams* pAccessory);
	
	mutable uint32	m_refs;
	bool					m_valid;

private:
	void ResetInternal();
};

#endif //__WEAPONSHAREDPARAMS_H__