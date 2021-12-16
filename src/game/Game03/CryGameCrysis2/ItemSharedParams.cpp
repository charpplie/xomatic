/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 3:4:2005   15:11 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "ItemSharedParams.h"
#include "IItem.h"
#include "ItemDefinitions.h"

#define ReadValue(hold, param)	reader.Read(#param, hold.param)
#define ReadValueEx(hold, name, param)	reader.Read(#name, hold.param)
#define ReadPointerValue(hold, param) reader.Read(#param, hold->param)

#define ReadOptionalParams(paramString, param) {										\
	const IItemParamsNode* paramsNode	= pRoot->GetChild(paramString);	\
	if(paramsNode) {																									\
		if(!p##param) { p##param = new S##param; }											\
		Read##param(paramsNode);																				\
	} else { SAFE_DELETE(p##param); } }										

CItemSharedParams::CItemSharedParams() :
m_refs(0), 
m_valid(false), 
pFlashlightParams(NULL), 
pLaserParams(NULL), 
pMountParams(NULL),
pDogTagParams(NULL),
pBTBParams(NULL)
{};

CItemSharedParams::~CItemSharedParams()
{
	SAFE_DELETE(pMountParams);
	SAFE_DELETE(pFlashlightParams);
	SAFE_DELETE(pLaserParams);
	SAFE_DELETE(pDogTagParams);
	SAFE_DELETE(pBTBParams);
};

void CItemSharedParams::GetMemoryUsage(ICrySizer *pSizer) const
{		
	pSizer->AddObject(this, sizeof(*this));
	pSizer->AddObject(params);	
	pSizer->AddObject(actions);
	pSizer->AddObject(helpers);	
	pSizer->AddObject(geometry);	
	pSizer->AddObject(damageLevels);	
	pSizer->AddObject(initialSetup);	
	pSizer->AddObject(layers);
	pSizer->AddObject(accessoryparams);
	pSizer->AddObject(dualWieldSupport);
	pSizer->AddObject(pMountParams);
	pSizer->AddObject(pFlashlightParams);
	pSizer->AddObject(pLaserParams);
	pSizer->AddObject(pDogTagParams);
	pSizer->AddObject(pBTBParams);
	//pSizer->AddObject(m_cachedAnimationResourceNames);
}

//------------------------------------------------------------------------
bool CItemSharedParams::ReadItemParams(const IItemParamsNode* pRoot)
{
	if (!pRoot)
	{
		return false;
	}

	const IItemParamsNode* params					= pRoot->GetChild("params");
	const IItemParamsNode* geometry				= pRoot->GetChild("geometry");
	const IItemParamsNode* actions				= pRoot->GetChild("actions");
	const IItemParamsNode* layers					= pRoot->GetChild("layers");
	const IItemParamsNode* accessories		= pRoot->GetChild("accessories");
	const IItemParamsNode* damagelevels		= pRoot->GetChild("damagelevels");
	const IItemParamsNode* accessoryAmmo	= pRoot->GetChild("accessoryAmmo");

	if(params)				ReadParams(params);
	if(actions)				ReadActions(actions);
	if(geometry)			ReadGeometry(geometry);
	if(layers)				ReadLayers(layers);
	if(accessories)		ReadAccessories(accessories);
	if(damagelevels)	ReadDamageLevels(damagelevels);
	if(accessoryAmmo)	ReadAccessoryAmmo(accessoryAmmo);
	
	ReadOptionalParams("flashlight", FlashlightParams);
	ReadOptionalParams("laser", LaserParams);
	ReadOptionalParams("dogTag", DogTagParams);
	ReadOptionalParams("btbParams", BTBParams);
	
	SetValid(true); 

	return true;
}

//------------------------------------------------------------------------			
bool CItemSharedParams::ReadParams(const IItemParamsNode *pNode)
{
	CItemParamReader reader(pNode);
	ReadValue(params, selectable);
	ReadValue(params, droppable);
	ReadValue(params, pickable);
	ReadValue(params, mountable);
	ReadValue(params, usable);
	ReadValue(params, giveable);
	ReadValue(params, unique);
	ReadValue(params, mass);
	ReadValue(params, fly_timer);
	ReadValue(params, cache_string_reserve);
	ReadValue(params, drop_impulse);
	ReadValue(params, drop_impulse_pos);
	ReadValue(params, drop_angles);
	ReadValue(params, pose);
	ReadValue(params, slidePose);
	ReadValue(params, suffix);
	ReadValue(params, suffixAG);
	ReadValue(params, select_override);
	ReadValueEx(params, attachment_right, attachment[IItem::eIH_Right]);
	ReadValueEx(params, attachment_left, attachment[IItem::eIH_Left]);
	ReadValue(params, dual_wield_suffix);
	ReadValue(params, heavy_weapon);
	ReadValue(params, raiseable);
	ReadValue(params, auto_droppable);
	ReadValue(params, auto_pickable);
	ReadValue(params, has_first_select);
	ReadValue(params, attach_to_back);
	ReadValue(params, scopeAttachment);
	ReadValue(params, attachment_gives_ammo);
	ReadValue(params, fp_offset);
	ReadValue(params, display_name);
	ReadValueEx(params, bone_attachment_01, bone_attachment_01);
	ReadValueEx(params, bone_attachment_02, bone_attachment_02);
	ReadValue(params, fast_select);
	ReadValue(params, idle);
	ReadValue(params, idle_empty);
	ReadValue(params, select);
	ReadValue(params, select_empty);
	ReadValue(params, blendBasePose);
	ReadValue(params, sprintToFireDelay);
	ReadValue(params, sprintToZoomDelay);
	ReadValue(params, autoReloadDelay);
	ReadValue(params, runToSprintBlendTime);
	ReadValue(params, sprintToRunBlendTime);

	if (const IItemParamsNode *dualWield = pNode->GetChild("dualWield"))
	{
		int n = dualWield->GetChildCount();
		for (int i=0; i<n; i++)
		{
			const IItemParamsNode *item = dualWield->GetChild(i);
			if (!stricmp(dualWield->GetChildName(i), "item"))
			{
				const char *name = item->GetAttribute("value");
				if (name && name[0])
					dualWieldSupport.insert(TDualWieldSupportMap::value_type(name, true));
			}
			else if (!stricmp(dualWield->GetChildName(i), "suffix"))
			{
				const char *suffix = item->GetAttribute("value");
				if (suffix)
					params.dual_wield_suffix = suffix;
			}
			else if (!stricmp(dualWield->GetChildName(i), "pose"))
			{
				const char *pose = item->GetAttribute("value");
				if (pose)
					params.dual_wield_pose = pose;
			}
		}
	}

	if (const IItemParamsNode *mountedParams = pNode->GetChild("mount"))
	{
		if(!pMountParams)
		{
			pMountParams = new SMountParams;
		}

		CItemParamReader reader(mountedParams);
		ReadPointerValue(pMountParams, pivot);
		ReadPointerValue(pMountParams, body_distance);
		ReadPointerValue(pMountParams, left_hand_helper);
		ReadPointerValue(pMountParams, right_hand_helper);
		ReadPointerValue(pMountParams, ground_distance);
		ReadPointerValue(pMountParams, fpBody_offset);
		ReadPointerValue(pMountParams, fpBody_offset_ironsight);
	}
	else if (pMountParams)
	{
		delete pMountParams;
		pMountParams = NULL;
	}

	if (const IItemParamsNode *pAimAnimsNode = pNode->GetChild("aimAnims"))
	{
		CItemParamReader reader(pAimAnimsNode);
		params.hasAimAnims = true;

		ReadValue(params, ironsightAimAnimFactor);
		
		params.aimAnims.Read( pAimAnimsNode );
	}

	CRY_TODO(27, 10, 2009, "Benito : Move this out, only for mounted weapons (not needed for others)");

	if (const IItemParamsNode *pMountedAimAnimsNode = pNode->GetChild("mountedAimAnims"))
	{
		params.mountedAimAnims.Read( pMountedAimAnimsNode );
		const char* tpToken[MountedTPAimAnim::Total] =
		{
			"TP_Up", "TP_Down"
		};

		CItemParamReader reader( pMountedAimAnimsNode );
		for (int i=0; i<MountedTPAimAnim::Total; i++)
		{
			reader.Read(tpToken[i], params.mountedTPAimAnims[i]);
		}
	}
	
	return true;
}

//------------------------------------------------------------------------
void SAimAnimsBlock::Read( const IItemParamsNode *pNode )
{
	const char *token[WeaponAimAnim::Total] = 
	{
		"Base",	"Up", "Down", "Left", "Right", "Front", "Back", "StrafeLeft", "StrafeRight", "Bump", "Run", "Idle", "Sprint", "SuperJump"
	};
	CItemParamReader reader( pNode );

	for (int i=0; i<WeaponAimAnim::Total; i++)
	{
		reader.Read( token[i], anim[i]);
	}
}


#undef ReadValue

//------------------------------------------------------------------------
bool CItemSharedParams::ReadGeometry(const IItemParamsNode *pNode)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	// read the helpers
	helpers.resize(0);
	geometry.resize(0);

	const IItemParamsNode *attachments = pNode->GetChild("boneattachments");
	if (attachments)
	{
		int n = attachments->GetChildCount();
		helpers.reserve(n);
		SAttachmentHelper helper;
		for (int i=0; i<n; i++)
		{
			const IItemParamsNode *attachment = attachments->GetChild(i);
			const char *slot = attachment->GetAttribute("target");
			const char *name = attachment->GetAttribute("name");
			const char *bone = attachment->GetAttribute("bone");

			int islot = TargetToSlot(slot);
			if (islot == eIGS_Last)
			{
				GameWarning("Invalid attachment helper target for item! Skipping...");
				continue;
			}

			if (!name || !bone)
			{
				GameWarning("Invalid attachment helper specification for item! Skipping...");
				continue;
			}

			helper.name = name;
			helper.bone = bone;
			helper.slot = islot;

			helpers.push_back(helper);
		}
	}

	int n = pNode->GetChildCount();
	geometry.reserve(n);

	for (int i=0; i<n; i++)
	{
		const IItemParamsNode *child = pNode->GetChild(i);
		int islot = TargetToSlot(pNode->GetChildName(i));

		if (islot != eIGS_Last)
		{
			SGeometryDef geometryDef;

			geometryDef.slot = islot;
			child->GetAttribute("position", geometryDef.pos);
			child->GetAttribute("angles", geometryDef.angles);
			child->GetAttribute("scale", geometryDef.scale);
			
			const char *hand = child->GetAttribute("hand");
			geometryDef.modelPath = child->GetAttribute("name");
	
			int idx = 0;
			
			if (hand && hand[0])
			{
				if (!stricmp(hand, "right"))
					idx = 1;
				else if (!stricmp(hand, "left"))
					idx = 2;
				else
				{
					GameWarning("Invalid hand '%s' loading item!", hand);
				}
			}

			geometryDef.hand = idx;
			geometryDef.angles = DEG2RAD(geometryDef.angles);
			
			geometry.push_back(geometryDef);
		}
	}

	return true;
}

//------------------------------------------------------------------------
bool CItemSharedParams::ReadActions(const IItemParamsNode* pNode)
{
	actions.clear();
	int n = pNode->GetChildCount();

	for (int i=0; i<n; i++)
	{
		const IItemParamsNode *actionparams = pNode->GetChild(i);
		if (actionparams)
		{
			SItemAction action;
			if (ReadAction(actionparams, &action))
			{	
				const char *name = actionparams->GetAttribute("name");
				actions.insert(TActionMap::value_type(name, action));
			}
		}
	}

	return true;
}

//------------------------------------------------------------------------
bool CItemSharedParams::ReadAction(const IItemParamsNode *actionparams, SItemAction *pAction)
{
	const char *actionName = actionparams->GetAttribute("name");
	if (!actionName)
	{
		GameWarning("Missing action name for item! Skipping...");
		return false;
	}

	int children=0;
	actionparams->GetAttribute("children", children);
	pAction->children=children!=0;

	pAction->timeOverride = -1.0f;
	actionparams->GetAttribute("time", pAction->timeOverride);

	pAction->fpOverlayOverride = 1.0f;
	actionparams->GetAttribute("overlayOverride", pAction->fpOverlayOverride);

	pAction->fpOverlayBlendTime = 0.3f;
	actionparams->GetAttribute("overlayBlendInTime", pAction->fpOverlayBlendTime);

	int n = actionparams->GetChildCount();
	for (int i=0; i<n; i++)
	{
		const IItemParamsNode *child = actionparams->GetChild(i);
		const char *childName = actionparams->GetChildName(i);
		if (!stricmp(childName, "sound"))
		{
			const char *name = child->GetAttribute("name");
			if (!name)
			{
				GameWarning("Missing name of sound for action '%s' in item! Skipping...", actionName);
				return false;
			}

			const char *slot = child->GetAttribute("target");
			int islot = TargetToSlot(slot);

			if ((islot != eIGS_FirstPerson) && (islot != eIGS_ThirdPerson))
			{
				GameWarning("Invalid sound target '%s' for action '%s' in item! Skipping...", slot, actionName);
				return false;
			}

			if (!pAction->sound[islot].name.empty())
			{
				GameWarning("Sound target '%s' for action '%s' in item already specified! Skipping...", slot, actionName);
				return false;
			}

			float radius = 1.0f; child->GetAttribute("radius", radius);
			float sphere = 0.0f; child->GetAttribute("sphere", sphere);
			int isstatic = 0; child->GetAttribute("static", isstatic);
			int issynched =0; child->GetAttribute("synched", issynched);
			pAction->sound[islot].name = name;
			pAction->sound[islot].airadius = radius;
			pAction->sound[islot].sphere = sphere;
			pAction->sound[islot].isstatic = isstatic!=0;
			pAction->sound[islot].issynched = issynched!=0;
		}
		else if (!stricmp(childName, "animation"))
		{
			const char *name = child->GetAttribute("name");
			if (!name)
			{
				GameWarning("Missing name of animation for action '%s' in item! Skipping...", actionName);
				return false;
			}

			const char *slot = child->GetAttribute("target");
			int islot = TargetToSlot(slot);

			if (islot >= eIGS_LastAnimSlot)
			{
				GameWarning("Invalid animation target '%s' for action '%s' in item! Skipping...", slot, actionName);
				return false;
			}

			float speed = 1.0f; child->GetAttribute("speed", speed);
			float blend = 0.125f; child->GetAttribute("blendTime", blend);
			SAnimation animation;

			const char *camera_helper = child->GetAttribute("camera_helper");
			if (camera_helper && camera_helper[0])
			{
				int pos=1; child->GetAttribute("camera_position", pos);
				int rot=1; child->GetAttribute("camera_rotation", rot);
				int follow=0; child->GetAttribute("camera_follow", follow);
				int reorient=0; child->GetAttribute("camera_reorient", reorient);

				animation.camera_helper = camera_helper;
				animation.camera_pos=pos!=0;
				animation.camera_rot=rot!=0;
				animation.camera_follow=follow!=0;
				animation.camera_reorient=reorient!=0;
			}

			int releaseCameraBone=0;
			child->GetAttribute("releaseCameraBone", releaseCameraBone);

			animation.name = name;
			animation.speed = speed;
			animation.blend = blend;
			animation.releaseCameraBone = releaseCameraBone!=0;

			pAction->animation[islot].push_back(animation);
		}
		else if (!stricmp(childName, "forceFeedback"))
		{
			child->GetAttribute("time", pAction->forceFeedback.time);
			child->GetAttribute("amplifierA", pAction->forceFeedback.amplifierA);
			child->GetAttribute("amplifierB", pAction->forceFeedback.amplifierB);
			child->GetAttribute("combatModeMultiplier", pAction->forceFeedback.combatModeMultiplier);
		}
		else if (!stricmp(childName, "switchHand"))
		{
			SItemAction::SHandSwitch newHandSwitch;

			const char* mode = child->GetAttribute("mode");
			child->GetAttribute("switchToHand", newHandSwitch.switchToHand);
			child->GetAttribute("switchToHandAnimFraction", newHandSwitch.switchToHandAnimFraction);
			child->GetAttribute("resetHandAnimFraction", newHandSwitch.resetHandAnimFraction);

			newHandSwitch.associatedSuitMode = TargetToNanoSuitMode(mode);

			if(newHandSwitch.associatedSuitMode != eNanoSuitMode_Invalid)
			{
				pAction->handSwitch.push_back(newHandSwitch);
			}
			else
			{
				GameWarning("Invalid suitmode '%s' associated with hand switch parameters", mode ? mode : "");
			}
		}
		else
		{
			GameWarning("Unknown param '%s' for action '%s' in item! Skipping...", childName, actionName);
			return false;
		}
	}

	return true;
}

//------------------------------------------------------------------------
bool CItemSharedParams::ReadDamageLevels(const IItemParamsNode* pNode)
{
	damageLevels.resize(0);
	int n = pNode->GetChildCount();

	damageLevels.reserve(n);

	for (int i=0; i<n; i++)
	{
		const IItemParamsNode *levelparams = pNode->GetChild(i);
		if (levelparams)
		{
			SDamageLevel level;

			levelparams->GetAttribute("min_health", level.min_health);
			levelparams->GetAttribute("max_health", level.max_health);
			levelparams->GetAttribute("scale", level.scale);

			const char *helper = levelparams->GetAttribute("helper");
			const char *effect = levelparams->GetAttribute("effect");

			if (effect)
				level.effect = effect;
			if (helper)
				level.helper = helper;

			damageLevels.push_back(level);
		}
	}

	return true;
}

//------------------------------------------------------------------------
bool CItemSharedParams::ReadLayers(const IItemParamsNode* pNode)
{
	layers.clear();

	int n = pNode->GetChildCount();

	for (int i=0; i<n; i++)
	{
		const IItemParamsNode *layer = pNode->GetChild(i);
		if (layer)
		{
			SLayer lyr;
			if (ReadLayer(layer, &lyr))
			{	
				const char *name = layer->GetAttribute("name");
				layers.insert(TLayerMap::value_type(name, lyr));
			}
		}
	}

	return true;
}

//------------------------------------------------------------------------
bool CItemSharedParams::ReadLayer(const IItemParamsNode *layer, SLayer *pLayer)
{
	const char *layerName = layer->GetAttribute("name");
	if (!layerName)
	{
		GameWarning("Missing layer name for item! Skipping...");
		return false;
	}

	pLayer->isstatic = false;
	int isstatic;
	if (layer->GetAttribute("static", isstatic))
	{
		pLayer->isstatic = isstatic!=0;
	}

	for (int i=0; i<eIGS_LastLayerSlot; i++)
	{
		pLayer->id[i] = -1;
	}

	int n = layer->GetChildCount();
	for (int i=0; i<n; i++)
	{
		const IItemParamsNode *child = layer->GetChild(i);
		if (!stricmp(layer->GetChildName(i), "animation"))
		{
			const char *name = child->GetAttribute("name");
			if (!name)
			{
				GameWarning("Missing name of animation for layer '%s' in item! Skipping...", layerName);
				return false;
			}

			const char *slot = child->GetAttribute("target");
			int islot = TargetToSlot(slot);

			if (islot >= eIGS_LastLayerSlot)
			{
				GameWarning("Invalid animation target '%s' of for layer '%s' in item! Skipping...", slot, layerName);
				return false;
			}

			pLayer->name[islot] = name;
			pLayer->id[islot] = 0; child->GetAttribute("layerId", pLayer->id[islot]);
		}
		else if (!stricmp(layer->GetChildName(i), "bones"))
		{
			int nb = child->GetChildCount();
			for (int b=0; b<nb; b++)
			{
				const IItemParamsNode *bone = child->GetChild(b);
				if (!stricmp(child->GetChildName(b), "bone"))
				{
					const char *name = bone->GetAttribute("name");
					if (!name)
					{
						GameWarning("Missing name of bone for layer '%s' in item! Skipping...", layerName);
						return false;
					}

					pLayer->bones.push_back(name);
				}
			}
		}
		else
		{
			GameWarning("Unknown param '%s' for layer '%s' in item! Skipping...", layer->GetChildName(i), layerName);
			return false;
		}
	}

	return true;
}

//------------------------------------------------------------------------
bool CItemSharedParams::ReadAccessories(const IItemParamsNode* pNode)
{
	accessoryparams.clear();
	initialSetup.resize(0);

	int n = pNode->GetChildCount();

	for (int i=0; i<n; i++)
	{
		const IItemParamsNode *child = pNode->GetChild(i);

		if (!stricmp(pNode->GetChildName(i), "accessory"))
		{
			SAccessoryParams params;

			if (!ReadAccessoryParams(child, &params))
				continue;

			const char *name = child->GetAttribute("name");
			accessoryparams.insert(TAccessoryParamsMap::value_type(name, params));

		}
		else if (!stricmp(pNode->GetChildName(i), "initialsetup"))
		{
			int na = child->GetChildCount();
			for (int k=0; k<na; k++)
			{
				const IItemParamsNode *accessory = child->GetChild(k);
				if (!stricmp(child->GetChildName(k), "accessory"))
				{
					const char *name = accessory->GetAttribute("name");
					if (!name || !name[0])
					{
						GameWarning("Missing accessory name for initial setup in item! Skipping...");
						continue;
					}

					initialSetup.push_back(name);
				}
				else
				{
					GameWarning("Unknown param '%s' in initial setup for item! Skipping...", child->GetChildName(k));
					continue;
				}
			}
		}
	}

	return true;
}

//------------------------------------------------------------------------
bool CItemSharedParams::ReadAccessoryParams(const IItemParamsNode* accessory, SAccessoryParams* params)
{
	const char *name = accessory->GetAttribute("name");
	if (!name || !name[0])
	{
		GameWarning("Missing accessory name for item! Skipping...");
		return false;
	}

	const IItemParamsNode *attach = accessory->GetChild("attach");
	const IItemParamsNode *detach = accessory->GetChild("detach");

	if (!attach || !detach)
	{
		GameWarning("Missing attach/detach details for accessory '%s' in item! Skipping...", name);
		return false;
	}

	params->attach_action = attach->GetAttribute("action");
	params->attach_helper = attach->GetAttribute("helper");
	params->attach_layer = attach->GetAttribute("layer");

	int attachToOwner = 0;
	attach->GetAttribute("attachToOwner", attachToOwner);
	params->attachToOwner = attachToOwner!=0;

	params->detach_action = detach->GetAttribute("action");

	string firemodes = accessory->GetAttribute("firemodes");
	int curPos = 0;
	string curToken, nextToken;
	nextToken = firemodes.Tokenize(",", curPos);
	while (!nextToken.empty())
	{
		curToken = nextToken;
		curToken.Trim();
#ifdef ITEM_USE_SHAREDSTRING
		params->firemodes.push_back(curToken.c_str());
#else
		params->firemodes.push_back(curToken);
#endif
		nextToken = firemodes.Tokenize(",", curPos);
	}

	firemodes = accessory->GetAttribute("disableFiremodes");
	curPos = 0;
	nextToken = firemodes.Tokenize(",", curPos);
	while (!nextToken.empty())
	{
		curToken = nextToken;
		curToken.Trim();
#ifdef ITEM_USE_SHAREDSTRING
		params->disableFiremodes.push_back(curToken.c_str());
#else
		params->disableFiremodes.push_back(curToken);
#endif
		nextToken = firemodes.Tokenize(",", curPos);
	}

	params->switchToFireMode = accessory->GetAttribute("switchToFireMode");
	params->secondaryFireMode = accessory->GetAttribute("secondaryFireMode");
	params->zoommode = accessory->GetAttribute("zoommode");
	params->zoommodeSecondary = accessory->GetAttribute("zoommodeSecondary");

	int exclusive = 0;
	accessory->GetAttribute("exclusive", exclusive);
	params->exclusive = exclusive!=0;
	int client_only=1;
	accessory->GetAttribute("client_only", client_only);
	params->client_only= client_only!=0;

	params->params = accessory->GetChild("params");

	return true;
}

//-----------------------------------------------------------------------
bool CItemSharedParams::ReadAccessoryAmmo(const IItemParamsNode* pNode)
{
	bonusAccessoryAmmo.clear();

	int n = pNode->GetChildCount();

	for (int i=0; i<n; i++)
	{
		const IItemParamsNode *ammo = pNode->GetChild(i);
		if (!strcmpi(ammo->GetName(), "ammo"))
		{
			int amount = 0;

			const char* name = ammo->GetAttribute("name");
			IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
			assert(pClass);

			ammo->GetAttribute("amount", amount);

			if (amount)
			{
				bonusAccessoryAmmo[pClass]=amount;
			}
		}
	}

	return true;
}

//------------------------------------------------------
bool CItemSharedParams::ReadFlashlightParams(const IItemParamsNode* pNode)
{ 
	const IItemParamsNode* fp = pNode->GetChild("firstperson");
	const IItemParamsNode* tp = pNode->GetChild("thirdperson");

	if (fp)
	{
		fp->GetAttribute("light_range", pFlashlightParams->light_range[0]);
		fp->GetAttribute("light_fov", pFlashlightParams->light_fov[0]);
		fp->GetAttribute("light_color", pFlashlightParams->light_color[0]);
		fp->GetAttribute("light_diffuse_mul", pFlashlightParams->light_diffuse_mul[0]);
		fp->GetAttribute("light_hdr_dyn", pFlashlightParams->light_hdr_dyn[0]);
		fp->GetAttribute("light_dir", pFlashlightParams->light_dir[0]);
		pFlashlightParams->light_texture[0] = fp->GetAttribute("light_texture");
		pFlashlightParams->light_material[0] = fp->GetAttribute("light_material");
	}

	if (tp)
	{
		tp->GetAttribute("light_range", pFlashlightParams->light_range[1]);
		tp->GetAttribute("light_fov", pFlashlightParams->light_fov[1]);
		tp->GetAttribute("light_color", pFlashlightParams->light_color[1]);
		tp->GetAttribute("light_diffuse_mul", pFlashlightParams->light_diffuse_mul[1]);
		tp->GetAttribute("light_hdr_dyn", pFlashlightParams->light_hdr_dyn[1]);
		tp->GetAttribute("light_dir", pFlashlightParams->light_dir[1]);
		pFlashlightParams->light_texture[1] = tp->GetAttribute("light_texture");
		pFlashlightParams->light_material[1] = tp->GetAttribute("light_material");
	}

	return true;
}

//------------------------------------------------------
bool CItemSharedParams::ReadLaserParams(const IItemParamsNode* pNode)
{ 
	const IItemParamsNode* fp = pNode->GetChild("firstperson");
	const IItemParamsNode* tp = pNode->GetChild("thirdperson");

	if (fp)
	{
		pLaserParams->laser_dot[0] = fp->GetAttribute("laser_dot");
		fp->GetAttribute("laser_range", pLaserParams->laser_range[0]);
		fp->GetAttribute("foward_dir_Y", pLaserParams->foward_dir_Y);
	}

	if (tp)
	{
		pLaserParams->laser_geometry_tp = tp->GetAttribute("laser_geometry_tp");
		pLaserParams->laser_dot[1] = tp->GetAttribute("laser_dot");
		tp->GetAttribute("laser_range", pLaserParams->laser_range[1]);
		tp->GetAttribute("laser_max_scale", pLaserParams->laser_max_scale);
		tp->GetAttribute("laser_max_len", pLaserParams->laser_max_len);
	}

	return true;
}

bool CItemSharedParams::ReadDogTagParams(const IItemParamsNode* pNode)
{ 
	CItemParamReader reader(pNode);
		
	const char* particleEffect;
	reader.Read("twinkle", particleEffect);
	pDogTagParams->pEffect = gEnv->pParticleManager->FindEffect(particleEffect, "CItemSharedParams::ReadItemParams");

	const char* audioSignal;
	reader.Read("pickupAudio", audioSignal);
	pDogTagParams->pickupAudio = g_pGame->GetGameAudio()->GetSignalID(audioSignal);

	return true;
}

//------------------------------------------------------------------------
bool CItemSharedParams::ReadBTBParams(const IItemParamsNode* pNode)
{
	CItemParamReader reader(pNode);
	reader.Read("pickupTime", pBTBParams->pickupTime);
	reader.Read("deployTime", pBTBParams->deployTime);
	reader.Read("defuseTime", pBTBParams->defuseTime);
	reader.Read("explodeTime", pBTBParams->explodeTime);
	reader.Read("tapToDropTime", pBTBParams->tapToDropTime);

	pBTBParams->deployTime = max(pBTBParams->deployTime, 0.0001f);
	pBTBParams->defuseTime = max(pBTBParams->defuseTime, 0.0001f);
	pBTBParams->pickupTime = max(pBTBParams->pickupTime, 0.0001f);

	const char *pEffectName = 0;
	reader.Read("explosion", pEffectName);
	if (pEffectName)
	{
		pBTBParams->pExplosionEffect = gEnv->pParticleManager->FindEffect(pEffectName, "CItemSharedParams::ReadItemParams");
	}
	
	const char *pTarget = 0;
	reader.Read("targetClass", pTarget);
	
	if (pTarget)
	{
		pBTBParams->pTargetClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(pTarget);
	}

	return true;
}

//------------------------------------------------------------------------
int CItemSharedParams::TargetToSlot(const char *slot)
{
	int islot = eIGS_Last;
	if (slot)
	{
		if (!stricmp(slot, "firstperson"))
			islot = eIGS_FirstPerson;
		else if (!stricmp(slot, "thirdperson"))
			islot = eIGS_ThirdPerson;
		else if (!stricmp(slot, "owner"))
			islot = eIGS_Owner;
		else if (!stricmp(slot, "aux0"))
			islot = eIGS_Aux0;
		else if (!stricmp(slot, "ownerGraph"))
			islot = eIGS_OwnerAnimGraph;
		else if (!stricmp(slot, "ownerGraphLoop"))
			islot = eIGS_OwnerAnimGraphLooped;
		else if (!stricmp(slot, "destroyed"))
			islot = eIGS_Destroyed;
		else if (!stricmp(slot, "thirdpersonAux"))
			islot = eIGS_ThirdPersonAux;
		else if (!stricmp(slot, "aux1"))
			islot = eIGS_Aux1;
	}

	return islot;
}

//------------------------------------------------------------------------
ENanoSuitMode CItemSharedParams::TargetToNanoSuitMode(const char *mode)
{
	ENanoSuitMode suitMode = eNanoSuitMode_Invalid;
	if (mode)
	{
		if (!stricmp(mode, "pow"))
		{
			suitMode = eNanoSuitMode_Power;
		}
		else if (!stricmp(mode, "arm"))
		{
			suitMode = eNanoSuitMode_Armor;
		}
		else if (!stricmp(mode, "tac"))
		{
			suitMode = eNanoSuitMode_Tactical;
		}
		else if (!stricmp(mode, "sth"))
		{
			suitMode = eNanoSuitMode_Stealth;
		}
	}

	return suitMode;
}
