/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Stores all item parameters that don't mutate in instances...
						 Allows for some nice memory savings...

-------------------------------------------------------------------------
History:
- 3:4:2007   10:54 : Created by Márcio Martins

*************************************************************************/

#pragma once

#ifndef __ITEMSHAREDPARAMS_H__
#define __ITEMSHAREDPARAMS_H__

#include "GameParameters.h"
#include "IItem.h"
#include "ItemDefinitions.h"
#include "NanoSuitDefs.h"
#include "Audio/GameAudio.h"

struct SAimAnimsBlock
{
	ItemString anim[WeaponAimAnim::Total];
	
	void Read( const IItemParamsNode *pNode );
	void GetMemoryUsage( ICrySizer *s ) const
	{
		for (int i = 0; i < WeaponAimAnim::Total; ++i)
		{
			s->AddObject(anim[i]);
		}
	}	
};

struct SParams
{
	SParams()
		: selectable(true),
		droppable(true),
		pickable(true),
		mountable(true),
		usable(true),
		giveable(true),
		unique(true),
		mass(3.5f),
		fly_timer(750),
		cache_string_reserve(32),
		drop_impulse(12.5f),
		drop_impulse_pos(0.0075f,0,0.0075f),
		drop_angles(0,0,0),
		select_override(0.0f),
		heavy_weapon(false),
		raiseable(false),
		auto_droppable(false),
		auto_pickable(true),
		has_first_select(false),
		attach_to_back(false),
		scopeAttachment(0),
		attachment_gives_ammo(false),
		fp_offset(0.0f, 0.0f, 0.0f),
		hasAimAnims(false),
		ironsightAimAnimFactor(1.f),
		fast_select(false),
		idle("idle"),
		idle_empty("idle"),
		select("select"),
		select_empty("select"),
		blendBasePose(true),
		sprintToFireDelay(0.0f),
		sprintToZoomDelay(0.35f),
		runToSprintBlendTime(0.2f),
		sprintToRunBlendTime(0.2f)
	{
		pose = g_pItemStrings->nw;
		slidePose = "rifle";
		attachment[IItem::eIH_Right] = g_pItemStrings->right_item_attachment;
		attachment[IItem::eIH_Left] = g_pItemStrings->left_item_attachment;
		bone_attachment_01.clear();
		bone_attachment_02.clear();
	};

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(pose);
		s->AddObject(slidePose);
		s->AddObject(suffix);
		s->AddObject(suffixAG);
		s->AddObject(dual_wield_suffix);
		s->AddObject(dual_wield_pose);
		s->AddObject(display_name);
		s->AddObject(bone_attachment_01);
		s->AddObject(bone_attachment_02);
		s->AddObject(idle);
		s->AddObject(idle_empty);
		s->AddObject(select);
		s->AddObject(select_empty);
		
		for (int i = 0; i < IItem::eIH_Last; i++)
		{
			s->AddObject(attachment[i]);
		}
		s->AddObject(aimAnims);
		s->AddObject(mountedAimAnims);
		
		for (int i = 0; i < MountedTPAimAnim::Total; i++)
		{
			s->AddObject(mountedTPAimAnims[i]);
		}
	}
	

	bool    heavy_weapon ;
	bool		raiseable        ;
	bool		selectable       ;
	bool		droppable        ;
	bool		pickable         ;
	bool		mountable        ;
	bool		usable           ;
	bool		giveable         ;
	bool		unique           ;
	bool		auto_droppable	 ;
	bool		auto_pickable		 ;
	bool    attachment_gives_ammo;
	bool		has_first_select;
	bool		fast_select;
	float		sprintToFireDelay;
	float		sprintToZoomDelay;
	float		autoReloadDelay;
	float		runToSprintBlendTime;
	float		sprintToRunBlendTime;

	int			fly_timer;
	int			cache_string_reserve;
	int     scopeAttachment;

	float		mass;
	float		drop_impulse;
	float		select_override;

	Vec3		drop_impulse_pos;
	Vec3		drop_angles;
	Vec3		fp_offset;

	ItemString	pose;
	ItemString	slidePose;
	ItemString	suffix;
	ItemString	suffixAG;
	ItemString	attachment[IItem::eIH_Last];
	ItemString	dual_wield_suffix;
	ItemString	dual_wield_pose;
	ItemString	display_name;

	ItemString  bone_attachment_01;
	ItemString  bone_attachment_02;
	bool				attach_to_back;

	bool				hasAimAnims;
	bool				blendBasePose;
	float				ironsightAimAnimFactor;
	SAimAnimsBlock aimAnims;
	SAimAnimsBlock mountedAimAnims;
	ItemString	mountedTPAimAnims[MountedTPAimAnim::Total];

	ItemString idle;
	ItemString idle_empty;
	ItemString select;
	ItemString select_empty;
};


struct SMountParams
{
	SMountParams()
		:	body_distance(0.55f),
		ground_distance(0.0f),
		fpBody_offset(0.0f,-1.0f,-0.1f),
		fpBody_offset_ironsight(0.0f,-1.0f,-0.1f)
	{};

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(pivot);
		s->AddObject(left_hand_helper);
		s->AddObject(right_hand_helper);
	}

	float		body_distance;
	float		ground_distance;
	Vec3		fpBody_offset;
	Vec3		fpBody_offset_ironsight;
	ItemString	pivot;
	ItemString	left_hand_helper;
	ItemString	right_hand_helper;
};

struct SLayer
{
	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(bones);		
		for (int i=0; i<eIGS_Last; i++)
			s->AddObject(name[i]);
	}

	std::vector<ItemString>	bones;
	ItemString	name[eIGS_LastLayerSlot];
	int			id[eIGS_LastLayerSlot];
	bool		isstatic;
};

struct SAnimation
{
	SAnimation()
		: 
		camera_pos(true), 
		camera_rot(true), 
		camera_follow(false), 
		camera_reorient(false), 
		releaseCameraBone(false) 
		{};

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(name);
		s->AddObject(camera_helper);
	}

	ItemString	name;
	ItemString	camera_helper;
	float				speed;
	float				blend;
	bool				camera_pos;
	bool				camera_rot;
	bool				camera_follow;
	bool				camera_reorient;
	bool				releaseCameraBone;
};

struct SEffect
{
	SEffect() {};

	void GetMemoryUsage(ICrySizer *s) const
	{
		s->AddObject(name);
		s->AddObject(helper);
	}

	ItemString name;
	ItemString helper;
};

struct SForceFeedback
{
	SForceFeedback()
		: time(0), amplifierA(0), amplifierB(0), combatModeMultiplier(1) {}

	float time;
	float amplifierA;
	float amplifierB;

	float combatModeMultiplier;
};

struct SCameraShake
{
	SCameraShake()
		: time(0)
		, shift(0, 0, 0)
		, rotate(0, 0, 0) {}

	Vec3		shift;
	Vec3		rotate;
	float		time;
};

struct SAudio
{
	SAudio():	isstatic(false), sphere(0.0f), airadius(0.0f),issynched(false) {};

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(name);
	}

	ItemString		name;
	float			    airadius;
	float			    sphere;
	bool			    isstatic;
	bool          issynched;
};

struct SItemAction
{
	struct SHandSwitch
	{
		SHandSwitch() : switchToHandAnimFraction(0.f), resetHandAnimFraction(-1.f), switchToHand(IItem::eIH_Last), associatedSuitMode(eNanoSuitMode_Invalid) {}
		void GetMemoryUsage(ICrySizer * s) const {}

		float					switchToHandAnimFraction;
		float					resetHandAnimFraction;
		int						switchToHand;
		ENanoSuitMode associatedSuitMode;
	};

	typedef std::vector<SHandSwitch>	THandSwitchVector;
	
	void GetMemoryUsage(ICrySizer * s) const
	{
		for (int i=0; i<eIGS_LastAnimSlot; i++)
		{
			s->AddObject(animation[i]);			
		}

		s->AddObject(handSwitch);

		for(int i = 0; i < 2; i++)
		{
			s->AddObject(sound[i]);
		}
	}

	std::vector<SAnimation>	animation[eIGS_LastAnimSlot];
	THandSwitchVector	handSwitch;
	SAudio					sound[2];
	SForceFeedback	forceFeedback;
				
	float					timeOverride;
	float					fpOverlayOverride;
	float					fpOverlayBlendTime;
	bool					children;
};

struct SDamageLevel
{
	SDamageLevel(): max_health(100), min_health(0) {};

	int max_health;
	int min_health;
	float scale;
	ItemString effect;
	ItemString helper;

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(effect);
		s->AddObject(helper);
	}
};

typedef struct SFlashlightParams
{   
	SFlashlightParams() 
	{ 
		light_range[0] = light_range[1] = 0.f;
		light_fov[0] = light_fov[1] = 0.f;        
		light_color[0] = light_color[1] = Vec3(1,1,1);
		light_diffuse_mul[0] = light_diffuse_mul[1] = 1.f;        
		light_hdr_dyn[0] = light_hdr_dyn[1] = 0.f;
		light_dir[0] = light_dir[1] = Vec3(0,1,0);
		light_texture[0].clear(); light_texture[1].clear();
		light_material[0].clear(); light_material[1].clear();
	}

	void GetMemoryUsage(ICrySizer * s) const
	{
		for(int i = 0; i < 2; i++)
		{
			s->AddObject(light_texture[i]);
			s->AddObject(light_material[i]);
		}
	}

	Vec3  light_color[2];
	Vec3  light_dir[2];
	float light_diffuse_mul[2];
	float light_range[2];
	float light_fov[2];
	float light_hdr_dyn[2];
	ItemString light_texture[2];
	ItemString light_material[2];    

} SFlashlightParams;

typedef struct SLaserParams
{   
	SLaserParams() 
	{ 
		laser_geometry_tp.clear();
		laser_dot[0].clear(); laser_dot[1].clear();
		laser_range[0] = laser_range[1] = 50.0f;
		laser_max_len = 20.0f;
		laser_max_scale = 16.0f;
		foward_dir_Y = 1;
	}

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(laser_dot[0]); 
		s->AddObject(laser_dot[1]);
		s->AddObject(laser_geometry_tp);
	}

	ItemString laser_geometry_tp;
	ItemString laser_dot[2];
	float			 laser_range[2];
	float			 laser_max_scale;
	float			 laser_max_len; 
	int				 foward_dir_Y;

} SLaserParams;

struct SAccessoryParams
{
	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(attach_helper);
		s->AddObject(attach_action);
		s->AddObject(attach_layer);
		s->AddObject(detach_action);
		s->AddObject(switchToFireMode);
		s->AddObject(secondaryFireMode);
		s->AddObject(zoommode);
		s->AddObject(zoommodeSecondary);
		
		s->AddObject(firemodes);		
		s->AddObject(disableFiremodes);		
	}

	ItemString		attach_helper;
	ItemString		attach_action;
	ItemString		attach_layer;
	ItemString		detach_action;
	ItemString		switchToFireMode;
	ItemString		secondaryFireMode;
	ItemString		zoommode;
	ItemString		zoommodeSecondary;
	std::vector<ItemString>	firemodes;
	std::vector<ItemString>	disableFiremodes; 
	const IItemParamsNode *params;
	bool      exclusive;
	bool			client_only;
	bool			attachToOwner;
};

struct SGeometryDef
{
	SGeometryDef() :	modelPath(""), pos(0.f, 0.f, 0.f), angles(0.f, 0.f, 0.f), scale(1.f), slot(eIGS_Last), hand(0) {};

	ItemString modelPath;
	Vec3 pos;
	Ang3 angles;
	float scale;
	int slot;
	int hand;

	void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(modelPath);
	}
};


struct SAttachmentHelper
{
	ItemString	name;
	ItemString	bone;
	int			slot;
	void GetMemoryUsage(ICrySizer * s) const
	{		
		s->AddObject(name);
		s->AddObject(bone);
	}
};

struct SCachedItemAnimation
{

	enum ECacheVariableFlags
	{
		eCVF_SuitMode = BIT(0),
		eCVF_Hand		 = BIT(1),
		eCVF_Suffix	 = BIT(2)
	};

	SCachedItemAnimation( const uint32 hash, const int8 hand, const int8 mode, const uint32 suffixHash)
		: m_stringHash(hash)
		, m_hand(hand)
		, m_suitMode(mode)
		, m_suffixHash(suffixHash)
		, m_animationId(-1)
		, m_variableFlags(0)
	{}

	inline uint32 GetChangeFlags(const SCachedItemAnimation &n) const
	{
		uint32 ret = 0;
		ret |= ((n.m_stringHash!=m_stringHash) << eCF_String);
		ret |= ((n.m_hand!=m_hand) << eCF_Hand);
		ret |= ((n.m_suitMode!=m_suitMode) << eCF_Suit);
		ret |= ((n.m_suffixHash!=m_suffixHash) << eCF_Suffix);

		return ret;
	}

	bool operator==( const SCachedItemAnimation &n ) const
	{
		return	(n.m_stringHash==m_stringHash) && 
						((n.m_hand==m_hand) || IsVariableFlagNotSet(eCVF_Hand)) && 
						((n.m_suitMode==m_suitMode) || IsVariableFlagNotSet(eCVF_SuitMode)) && 
						((n.m_suffixHash==m_suffixHash) || IsVariableFlagNotSet(eCVF_Suffix));
	}

	ILINE void SetAnimationId(int animationId) { m_animationId = animationId; }
	ILINE int GetAnimationId() const { return m_animationId; }
	void GetMemoryUsage( ICrySizer *pSizer ) const{}

	ILINE void SetVariableFlag(ECacheVariableFlags flag) { m_variableFlags |= flag; }
	ILINE bool IsVariableFlagNotSet(ECacheVariableFlags flag) const { return ((m_variableFlags&flag) == 0); } 

private:

	uint32				m_suffixHash;
	uint32				m_stringHash;
	int						m_animationId;
	int8					m_suitMode;
	int8					m_hand;
	int8					m_variableFlags;
};

struct SDogTagParams
{
	SDogTagParams() : pEffect(NULL), pickupAudio(INVALID_AUDIOSIGNAL_ID) {};

	void GetMemoryUsage(ICrySizer * s) const {};

	IParticleEffect*	pEffect;
	TAudioSignalID		pickupAudio;
};

struct SBTBParams
{
	SBTBParams() : 
		pExplosionEffect(NULL),
		pTargetClass(NULL),
		pickupTime(0.f),
		deployTime(0.f),
		defuseTime(0.f),
		explodeTime(0.f),
		tapToDropTime(0.f) {};

	void GetMemoryUsage(ICrySizer * s) const {};
	
	IEntityClass*			pTargetClass;
	IParticleEffect*	pExplosionEffect;
	float pickupTime;
	float deployTime;
	float defuseTime;
	float explodeTime;
	float tapToDropTime;
};


class CItemSharedParams : public IGameSharedParameters
{
public:

	typedef std::vector<SDamageLevel>								TDamageLevelVector;
	typedef std::vector<SGeometryDef>								TGeometryDefVector;

	CItemSharedParams();
	virtual ~CItemSharedParams();

	//IGameSharedParameters
	virtual void AddRef() const { ++m_refs; };
	virtual uint32 GetRefCount() const { return m_refs; };
	virtual void Release() const { 
		if (--m_refs == 0)
			delete this;
	};

	virtual bool Valid() const { return m_valid; };
	virtual void SetValid(bool valid) { m_valid=valid; m_cachedAnimationIDs.reserve(params.cache_string_reserve); };

	virtual void GetMemoryUsage(ICrySizer *s) const;

	virtual const char* GetDataType() const { return "ItemParams"; }
	//~IGameSharedParameters

	bool ReadItemParams(const IItemParamsNode* pRoot);
	static ENanoSuitMode TargetToNanoSuitMode(const char *mode);

	TActionMap						actions;
	TAccessoryParamsMap		accessoryparams;
	THelperVector					helpers;
	TLayerMap							layers;
	TDualWieldSupportMap	dualWieldSupport;
	SParams								params;
	TGeometryDefVector		geometry;
	TDamageLevelVector		damageLevels;
	TInitialSetup					initialSetup;
	TAccessoryAmmoMap			bonusAccessoryAmmo;
	SFlashlightParams*		pFlashlightParams;
	SLaserParams*					pLaserParams;
	SMountParams*					pMountParams;
	SDogTagParams*				pDogTagParams;
	SBTBParams*						pBTBParams;

	mutable TAnimationIdsCache	m_cachedAnimationIDs;

protected:
	bool ReadParams(const IItemParamsNode* pNode);
	bool ReadGeometry(const IItemParamsNode* pNode);
	bool ReadActions(const IItemParamsNode* pNode);
	bool ReadAction(const IItemParamsNode *actionparams, SItemAction* pAction);
	bool ReadDamageLevels(const IItemParamsNode* pNode);
	bool ReadLayers(const IItemParamsNode* pNode);
	bool ReadLayer(const IItemParamsNode* layer, SLayer* pLayer);
	bool ReadAccessories(const IItemParamsNode* pNode);
	bool ReadAccessoryParams(const IItemParamsNode* accessory, SAccessoryParams* params);
	bool ReadAccessoryAmmo(const IItemParamsNode* pNode);
	bool ReadFlashlightParams(const IItemParamsNode* pNode);
	bool ReadLaserParams(const IItemParamsNode* pNode);
	bool ReadDogTagParams(const IItemParamsNode* pNode);
	bool ReadBTBParams(const IItemParamsNode* pNode);
	int	 TargetToSlot(const char* name);
	
	mutable uint32	m_refs;
	bool					m_valid;

};

#endif //__ITEMSHAREDPARAMS_H__
