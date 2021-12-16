/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Definitions for typedefs and enumerated types specific to items

-------------------------------------------------------------------------
History:
- 22:10:2009   10:15 : Created by Claire Allan

*************************************************************************/
#ifndef __ITEMDEFINITIONS_H__
#define __ITEMDEFINITIONS_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "ItemString.h"

struct SAttachmentHelper;
struct SAccessoryParams;
struct SItemAction;
struct SCachedItemAnimation;
struct SLayer;
struct SParentFireModeParams;
struct SParentZoomModeParams;
struct SPlayerMovementModifiers;
struct SZoomModeParams;

typedef CryFixedStringT<256>										TempResourceName;
typedef std::vector<SAttachmentHelper>					THelperVector;
typedef std::vector<ItemString>									TInitialSetup;
typedef std::map<ItemString, bool>							TDualWieldSupportMap;
typedef std::map<IEntityClass*, int>						TAccessoryAmmoMap;
typedef std::map<ItemString, SAccessoryParams>	TAccessoryParamsMap;
typedef std::map<ItemString, SItemAction>				TActionMap;
typedef	std::vector<SCachedItemAnimation>				TAnimationIdsCache;
typedef std::map<ItemString, SLayer>						TLayerMap;
typedef std::vector<SParentFireModeParams>			TParentFireModeParamsVector;
typedef std::vector<SParentZoomModeParams>			TParentZoomModeParamsVector;

struct SWeaponAmmo
{
	SWeaponAmmo()
		: pAmmoClass(NULL)
		, count(0)
	{

	}

	SWeaponAmmo(IEntityClass* pClass, int ammoCount)
		: pAmmoClass(pClass)
		, count(ammoCount)
	{

	}

	void GetMemoryUsage(ICrySizer * s) const {}

	IEntityClass* pAmmoClass;
	int						count;
};

typedef std::vector<SWeaponAmmo>						TAmmoVector;

struct SWeaponAmmoUtils
{
	static const SWeaponAmmo* FindAmmoConst(const TAmmoVector& ammoVector, IEntityClass* pAmmoType)
	{
		const int ammoCount = ammoVector.size();
		for (int i = 0; i < ammoCount; ++i)
		{
			if (pAmmoType != ammoVector[i].pAmmoClass)
				continue;
				
			return &(ammoVector[i]); 
		}

		return NULL;
	}

	static SWeaponAmmo* FindAmmo(TAmmoVector& ammoVector, IEntityClass* pAmmoType)
	{
		const int ammoCount = ammoVector.size();
		for (int i = 0; i < ammoCount; ++i)
		{
			if (pAmmoType != ammoVector[i].pAmmoClass)
				continue;

			return &(ammoVector[i]); 
		}

		return NULL;
	}

	static void SetAmmo(TAmmoVector& ammoVector, IEntityClass* pClass, int count)
	{
		SWeaponAmmo* pAmmo = SWeaponAmmoUtils::FindAmmo(ammoVector, pClass);
		if (pAmmo == NULL)
		{
			ammoVector.push_back(SWeaponAmmo(pClass, count));
		}
		else
		{
			pAmmo->count = count;
		}
	}

	static int GetAmmoCount(const TAmmoVector& ammoVector, IEntityClass* pClass)
	{
		const SWeaponAmmo* pAmmo = SWeaponAmmoUtils::FindAmmoConst(ammoVector, pClass);
		
		return pAmmo ? pAmmo->count : 0;
	}
};

enum eItemChangeFlags
{
	eCF_Suit	,
	eCF_Hand	,
	eCF_Suffix,
	eCF_String
};

struct MountedTPAimAnim
{
	enum Type
	{
		Up, 
		Down,
		Total
	};
};

struct WeaponAimAnim
{
	enum Type
	{
		Base,
		Up, 
		Down,
		Left,
		Right,
		Front,
		Back,
		StrafeLeft,
		StrafeRight,
		Bump,
		Run,
		Idle,
		Sprint,
		SuperJump,
		Total
	};
};

struct WeaponAnimLayerID
{
	enum Type
	{
		Base,
		Vert,
		Horiz,
		Front,
		Side,
		Run,
		Idle,
		Bump,
		Total
	};
};

enum eGeometrySlot
{
	eIGS_FirstPerson = 0,
	eIGS_ThirdPerson,
	eIGS_Owner,
	eIGS_OwnerAnimGraph,
	eIGS_OwnerAnimGraphLooped,
	eIGS_Aux0,
	eIGS_Destroyed,
	eIGS_Aux1,
	eIGS_ThirdPersonAux,
	eIGS_Last,
	eIGS_LastAnimSlot = eIGS_OwnerAnimGraphLooped + 1,
	eIGS_LastLayerSlot = eIGS_ThirdPerson + 1
};


enum ELTAGGrenadeType
{
	ELTAGGrenadeType_RICOCHET,
	ELTAGGrenadeType_STICKY,
	ELTAGGrenadeType_LAST
};

struct SItemStrings
{
	SItemStrings();
	~SItemStrings();

	// here they are
	ItemString AmbienceSound;				// "AmbienceSound"
	ItemString airFrictionOn;				// "airFrictionOn"
	ItemString activate;						// "activate";
	ItemString begin_reload;				// "begin_reload";
	ItemString cannon;							// "cannon";
	ItemString change_firemode;			// "change_firemode";
	ItemString change_firemode_zoomed; // "change_firemode_zoomed";
	ItemString deactivate;					// "deactivate";
	ItemString deselect;						// "deselect";
	ItemString destroy;							// "destroy";
	ItemString drop;							  // "drop"
	ItemString enter_modify;				// "enter_modify";
	ItemString exit_reload_nopump;	// "exit_reload_nopump";
	ItemString exit_reload_pump;		// "exit_reload_pump";
	ItemString fire;								// "fire";
	ItemString idle;								// "idle"
	ItemString idle_break;					// "idle_break"
	ItemString idle_lastGrenade;		// "idle_lastGrenade"
	ItemString idle_raised;					// "idle_raised";
	ItemString leave_modify;				// "leave_modify";
	ItemString left_item_attachment; // "left_item_attachment";
	ItemString lock;								// "lock";
	ItemString lower;								// "lower";
	ItemString meleeReaction;				// "meleeReaction"
	ItemString modify_layer;				// "modify_layer";
	ItemString nw;									// "nw";
	ItemString pickedup;						// "pickedup";
	ItemString pickedup_ammo;				// "pickedup_ammo"
	ItemString raise;								// "raise";
	ItemString reload_shell;				// "reload_shell";
	ItemString right_item_attachment;// "right_item_attachment";
	ItemString rip_off;							// "rip_off"
	ItemString rotate_mounted;			// "rotate_mounted"
	ItemString first_select;				// "first_select"; (For the JAW)
	ItemString turret;							// "turret";
	ItemString enable_light;        
	ItemString disable_light;
	ItemString use_light;
	ItemString LAM;
	ItemString LAMRifle;
	ItemString LAMFlashLight;
	ItemString LAMRifleFlashLight;
	ItemString Silencer;
	ItemString SOCOMSilencer;

	ItemString lever_layer_1;
	ItemString lever_layer_2;

};

extern struct SItemStrings* g_pItemStrings;

#endif //__ITEMDEFINITIONS_H__
