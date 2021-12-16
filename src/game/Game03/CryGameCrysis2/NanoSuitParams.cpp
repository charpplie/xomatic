/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  Contains parameterization for the suit
-------------------------------------------------------------------------
History:
- 6-11-2008: Benito G.R.

*************************************************************************/

#include "StdAfx.h"
#include "NanoSuitParams.h"
#include "NanoSuit_v2.h"
#include "Player.h"


CNanoSuitParams::CNanoSuitParams()
: m_dataLoaded(false) 
{
	
}

//////////////////////////////////////////////////////////////////////////
CNanoSuitParams::~CNanoSuitParams()
{
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::InvalidateData()
{
	m_dataLoaded = false;

	m_actionCameraShakePresets.clear();
	m_actionCostPresets.clear();
	m_actionFFeedbackPresets.clear();
	m_actionScreenFXPresets.clear();
	m_actionsMap.clear();
	m_specialStatesMap.clear();
	m_suitKnife.ReleaseKnife();

	InvalidateMaximumSuitData();
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::NotifyDataChanged()
{
	for (TSuitOwners::iterator it = m_nanoSuitOwners.begin(); it != m_nanoSuitOwners.end(); ++it)
	{
		(*it)->OnSuitDataChanged();
	}
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::LoadXMLData( const char* file )
{
	if(m_dataLoaded)
		return;

	XmlNodeRef rootNode = gEnv->pSystem->LoadXmlFile(file);

	if (!rootNode || strcmpi(rootNode->getTag(), "NanoSuit"))
	{
		GameWarning("Could not load nanosuit data. Invalid XML file '%s'! ", file);
		return;
	}

	IItemParamsNode *paramNode = g_pGame->GetIGameFramework()->GetIItemSystem()->CreateParams();
	paramNode->ConvertFromXML(rootNode);

	ReadNanoSuitEnergyParams(paramNode->GetChild("Energy"));
	ReadNanoSuitGameParams(paramNode->GetChild("GameParameters"));

	LoadLayerEffectParams(paramNode->GetChild("SuitLayerEffects"));
	m_suitKnife.ReadParams(paramNode->GetChild("SuitKnife"));

	ReadSuitActions(paramNode->GetChild("SuitActions"));
	ReadSuitStates(paramNode->GetChild("SuitStates"));

	ReadModeParams(eNanoSuitMode_Power, paramNode->GetChild("PowerParams"));
	ReadModeParams(eNanoSuitMode_Armor, paramNode->GetChild("ArmorParams"));
	ReadModeParams(eNanoSuitMode_Stealth, paramNode->GetChild("StealthParams"));
	ReadModeParams(eNanoSuitMode_Tactical, paramNode->GetChild("TacticalParams"));

	ReadCloakParams(paramNode->GetChild("CloakParams"));
	ReadMaximumSuitPerkParams(paramNode->GetChild("MaximumSuitPerkParams"));
	
	paramNode->Release();

	m_dataLoaded = true;

	NotifyDataChanged();
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::ReadNanoSuitEnergyParams( const struct IItemParamsNode* pParams )
{
	if(!pParams)
		return;

	CItemParamReader reader(pParams);

	SNanoSuitEnergyParams baseParams;
	ReadValue(baseParams, maximumEnergy);
	ReadValue(baseParams, energyCriticalMercyTime);
	ReadValue(baseParams, regenerationRate);
	ReadValue(baseParams, regenerationDelay);
	ReadValue(baseParams, disableTime);

	const char* stringValue = 0;
	reader.Read("energyBarAudio", stringValue);
	if (stringValue)
	{
		m_energyBarAudio.SetSignal(stringValue);
	}

	m_defaultGameParams.GetEnergy().SetBaseParameters(baseParams);
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::ReadNanoSuitGameParams( const struct IItemParamsNode* pParams )
{
	if(!pParams)
		return;

	CItemParamReader reader(pParams);

	SNanoSuitProperties properties;
	ReadValue(properties, speedScale);
	ReadValue(properties, sprintSpeedScale);
	ReadValue(properties, superJumpScale);
	ReadValue(properties, meleeStrengthScale);
	ReadValue(properties, meleeStrengthVehicleScale);
	ReadValue(properties, meleeDamageScale);
	ReadValue(properties, throwStrengthScale);
	ReadValue(properties, noiseSupression);
	ReadValue(properties, damageAbsorption);
	ReadValue(properties, camouflage);
	ReadValue(properties, aiClassThreat);

	m_defaultGameParams.SetBaseGameParameters(properties);
	m_defaultGameParams.m_state = eNanoSuitState_Disabled;
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::ReadSuitActions( const struct IItemParamsNode* pParams )
{
	if(!pParams)
		return;

	for(int i=0; i<pParams->GetChildCount(); i++)
	{
		const IItemParamsNode* pEntry = pParams->GetChild(i);
		if (!pEntry)
			continue;

		const char* entryName = pEntry->GetName();
		const char* name = pEntry->GetAttribute("name");
		if (!name)
			continue;

		if (strcmp(entryName, "action") == 0)
		{
			SNanoSuitAction action;
			ReadAction(pEntry, action);
			m_actionsMap.insert(TNanoSuitActions::value_type(name,action));
		}
		else if (strcmp(entryName, "energyPreset") == 0)
		{
			SActionCost preset;
			ReadActionCost(pEntry, &preset);
			m_actionCostPresets.insert(TActionCostPresets::value_type(name, preset));
		}
		else if (strcmp(entryName, "forceFeedbackPreset") == 0)
		{
			SActionFFeedback preset;
			ReadActionFFeedback(pEntry, &preset);
			m_actionFFeedbackPresets.insert(TActionFFeedbackPresets::value_type(name, preset));
		}
		else if (strcmp(entryName, "screenFxPreset") == 0)
		{
			SActionScreenFX preset;
			ReadActionScreenFx(pEntry, &preset);
			m_actionScreenFXPresets.insert(TActionScreenFXPresets::value_type(name, preset));
		}
		else if (strcmp(entryName, "cameraShakePreset") == 0)
		{
			SActionCameraShake preset;
			ReadActionCameraShake(pEntry, &preset);
			m_actionCameraShakePresets.insert(TActionCameraShakePresets::value_type(name, preset));
		}
		else
		{
			gEnv->pLog->LogWarning("Unknown param type in xml file nanosuit.xml");
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::ReadAction( const struct IItemParamsNode* pParams, SNanoSuitAction& action )
{
	if(const IItemParamsNode* energyCost = pParams->GetChild("energy"))
		ReadActionCost(energyCost, &action.cost);
	if(const IItemParamsNode* sound = pParams->GetChild("sound"))
	{
		action.audioSignals[0].SetSignal(sound->GetAttributeSafe("firstperson"));
		action.audioSignals[1].SetSignal(sound->GetAttributeSafe("thirdperson"));
	}
	if(const IItemParamsNode* ffback = pParams->GetChild("forceFeedback"))
		ReadActionFFeedback(ffback, &action.fFeedback);
	if(const IItemParamsNode* screenFX = pParams->GetChild("screenFX"))
		ReadActionScreenFx(screenFX, &action.screenFX);
	if (const IItemParamsNode* cameraShake = pParams->GetChild("cameraShake"))
		ReadActionCameraShake(cameraShake, &action.cameraShake);
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::ReadSuitStates( const struct IItemParamsNode* pParams )
{
	if(!pParams)
		return;

	for(int i=0; i<pParams->GetChildCount(); i++)
	{
		const IItemParamsNode *stateParams = pParams->GetChild(i);
		if (stateParams)
		{
			if(const char* name = stateParams->GetAttribute("name"))
			{			
				SNanoSuitSpecialState state;

				ReadState(stateParams, state);

				m_specialStatesMap.insert(TNanoSuitSpecialStates::value_type(name,state));		
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::ReadState( const struct IItemParamsNode* pParams, SNanoSuitSpecialState& state )
{
	pParams->GetAttribute("time", state.fTime);

	int temp = 0;
	pParams->GetAttribute("allowRestart", temp);
	state.bAllowRestart = (temp != 0);

	if(const IItemParamsNode* pOnEnterAction = pParams->GetChild("actionOnEnter"))
	{
		state.onEnterAction = pOnEnterAction->GetAttribute("name");		
	}
	if(const IItemParamsNode* pOnLeaveAction = pParams->GetChild("actionOnLeave"))
	{
		state.onLeaveAction = pOnLeaveAction->GetAttribute("name");
	}

	if(const IItemParamsNode* pReadability = pParams->GetChild("Readability"))
	{
		if(const IItemParamsNode* soundLoop = pReadability->GetChild("soundLoop"))
		{
			state.readability.audioSignals[0].SetSignal(soundLoop->GetAttributeSafe("firstperson"));
			state.readability.audioSignals[1].SetSignal(soundLoop->GetAttribute("thirdperson"));
		}
		if(const IItemParamsNode* ffeedBack = pReadability->GetChild("forceFeedback"))
		{
			state.readability.ffeedBack.name = ffeedBack->GetAttributeSafe("effectName");
			ffeedBack->GetAttribute("amplifierA", state.readability.ffeedBack.amplifierA);
			ffeedBack->GetAttribute("amplifierB", state.readability.ffeedBack.amplifierB);
			state.readability.ffeedBack.time = 0.2f; //Set it to constant value, it will apply continuosly
		}
		if(const IItemParamsNode* particleFX = pReadability->GetChild("particleFX"))
		{
			state.readability.particleFX = particleFX->GetAttribute("name");
			particleFX->GetAttribute("offset", state.readability.particleFXOffset);
			int isSurfaceFX = 0;
			particleFX->GetAttribute("surfaceFX", isSurfaceFX);
			state.readability.isSurfaceFX = (isSurfaceFX != 0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
template<class T>
static inline bool GetActionPreset(const std::map<ItemString, T>& presets, const struct IItemParamsNode* pParams, T* pAction)
{
	const char* preset = pParams->GetAttribute("preset");
	if (!preset)
		return false;

	typename std::map<ItemString, T>::const_iterator it = presets.find(preset);
	if (it == presets.end())
	{
		gEnv->pLog->LogWarning("preset not found while loading nanosuit.xml: '%s'", preset);
		return false;
	}
	*pAction = it->second;
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::ReadActionCost( const struct IItemParamsNode* pParams, SActionCost* pCost )
{
	if (!GetActionPreset(m_actionCostPresets, pParams, pCost))
	{
		pParams->GetAttribute("cost", pCost->energyCost);
	}
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::ReadActionFFeedback( const struct IItemParamsNode* pParams, SActionFFeedback* pFfeedback )
{
	if (!GetActionPreset(m_actionFFeedbackPresets, pParams, pFfeedback))
	{
		pFfeedback->name = pParams->GetAttributeSafe("effectName");
		pParams->GetAttribute("time", pFfeedback->time);
		pParams->GetAttribute("amplifierA", pFfeedback->amplifierA);
		pParams->GetAttribute("amplifierB", pFfeedback->amplifierB);
	}
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::ReadActionScreenFx( const struct IItemParamsNode* pParams, SActionScreenFX* pScreenFx )
{
	if (!GetActionPreset(m_actionScreenFXPresets, pParams, pScreenFx))
	{
		pScreenFx->libraryName = pParams->GetAttribute("library");
		pScreenFx->effectName	= pParams->GetAttribute("effect");
		pParams->GetAttribute("blendOutTime", pScreenFx->blendOutTime);
	}
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::ReadActionCameraShake( const struct IItemParamsNode* pParams, SActionCameraShake* pCameraShake )
{
	if (!GetActionPreset(m_actionCameraShakePresets, pParams, pCameraShake))
	{
		pParams->GetAttribute("shift", pCameraShake->shift);
		pParams->GetAttribute("rotate", pCameraShake->rotate);
		pParams->GetAttribute("time", pCameraShake->time);
		pParams->GetAttribute("frequency", pCameraShake->frequency);
		int viewAngleAttenuation = 1;
		int randomDirection = 0;
		pParams->GetAttribute("viewAngleAttenuation", viewAngleAttenuation);
		pParams->GetAttribute("randomDirection", randomDirection);
		pCameraShake->viewAngleAttenuation = (viewAngleAttenuation != 0);
		pCameraShake->randomDirection = (randomDirection != 0);
	}
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::LoadLayerEffectParams( const struct IItemParamsNode* pParams )
{
	if(pParams == NULL)
		return;

	CItemParamReader reader(pParams);

	ReadValue(m_layerEffectParams, light_attachment);
	ReadValue(m_layerEffectParams, light_radius);
	ReadValue(m_layerEffectParams, light_color);
	ReadValue(m_layerEffectParams, light_specularMult);
	ReadValue(m_layerEffectParams, light_diffuseMult);
	ReadValue(m_layerEffectParams, light_hdrDynamic);
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::ReadModeParams( ENanoSuitMode mode, const struct IItemParamsNode* pParams )
{
	if (pParams == NULL)
		return;

	ReadModeEnergyMod(pParams->GetChild("EnergyMod"), m_modeEnergyMods[mode]);
	ReadModeGameParamsMod(pParams->GetChild("GameParametersMod"), m_modePassiveMods[mode], m_modeActiveMods[mode]);
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::ReadModeEnergyMod( const struct IItemParamsNode* pParams, SNanoSuitEnergyParams& energyMod )
{
	if(!pParams)
		return;

	CItemParamReader reader(pParams);

	ReadValue(energyMod, consumptionRateMin);
	ReadValue(energyMod, consumptionRateMax);
	ReadValue(energyMod, maxSpeedReferenceForConsumption);
	ReadValue(energyMod, speedToConsumptionRateCurve);
	ReadValue(energyMod, regenerationDelay);
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::ReadModeGameParamsMod( const struct IItemParamsNode* pParams, SNanoSuitProperties& passiveMod, SNanoSuitProperties& activeMod )
{
	if(!pParams)
		return;

	CItemParamReader reader(pParams);

	ReadAttributeByName(passiveMod, speedScale, "addPassive", 0.0f);
	ReadAttributeByName(passiveMod, sprintSpeedScale, "addPassive", 0.0f);
	ReadAttributeByName(passiveMod, rotationSpeedScale, "addPassive", 0.0f);
	ReadAttributeByName(passiveMod, superJumpScale, "addPassive", 0.0f);
	ReadAttributeByName(passiveMod, meleeStrengthScale, "addPassive", 0.0f);
	ReadAttributeByName(passiveMod, meleeStrengthVehicleScale, "addPassive", 0.0f);
	ReadAttributeByName(passiveMod, meleeDamageScale, "addPassive", 0.0f);
	ReadAttributeByName(passiveMod, throwStrengthScale, "addPassive", 0.0f);
	ReadAttributeByName(passiveMod, noiseSupression, "addPassive", 0.0f);
	ReadAttributeByName(passiveMod, camouflage, "addPassive", 0.0f);
	ReadAttributeByName(passiveMod, damageAbsorption, "addPassive", 0.0f);
	ReadAttributeByName(passiveMod, damageAbsorptionMelee, "addPassive", 0.0f);
	ReadAttributeByName(passiveMod, hitReactionAbsorption, "addPassive", 0.0f);
	ReadAttributeByName(passiveMod, aiClassThreat, "addPassive", 0.0f);

	ReadAttributeByName(activeMod, speedScale, "addActive", 0.0f);
	ReadAttributeByName(activeMod, sprintSpeedScale, "addActive", 0.0f);
	ReadAttributeByName(activeMod, rotationSpeedScale, "addActive", 0.0f);
	ReadAttributeByName(activeMod, superJumpScale, "addActive", 0.0f);
	ReadAttributeByName(activeMod, meleeStrengthScale, "addActive", 0.0f);
	ReadAttributeByName(activeMod, meleeStrengthVehicleScale, "addActive", 0.0f);
	ReadAttributeByName(activeMod, meleeDamageScale, "addActive", 0.0f);
	ReadAttributeByName(activeMod, throwStrengthScale, "addActive", 0.0f);
	ReadAttributeByName(activeMod, noiseSupression, "addActive", 0.0f);
	ReadAttributeByName(activeMod, camouflage, "addActive", 0.0f);
	ReadAttributeByName(activeMod, damageAbsorption, "addActive", 0.0f);
	ReadAttributeByName(activeMod, damageAbsorptionMelee, "addActive", 0.0f);
	ReadAttributeByName(activeMod, hitReactionAbsorption, "addActive", 0.0f);
	ReadAttributeByName(activeMod, aiClassThreat, "addActive", 0.0f);
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::AddNanoSuit(CNanoSuit* pNanoSuit) 
{ 
	// Load data for 1st nano suit if not already loaded
	if(IsDataLoaded() == false)
	{
		pNanoSuit->ReloadSuitData();
	}

	m_nanoSuitOwners.insert(pNanoSuit); 

	if(m_maximumSuitParams.nanoSuitMaterial == NULL)
	{
		CacheBodyPartIndexArray(pNanoSuit);
		CreateMaximumSuitMaterials(pNanoSuit);
	}
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::RemoveNanoSuit(CNanoSuit* pNanoSuit) 
{
	if(m_nanoSuitOwners.size() == 1)
	{
		// Between games the nano suit params should be reloaded to allow 
		// changing between SP and MP
		InvalidateData();
	}

	m_nanoSuitOwners.erase(pNanoSuit); 
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::CacheBodyPartIndexArray(CNanoSuit* pNanoSuit)
{
	// Cache body part index array to avoid string comparisons
	SEntitySlotInfo slotInfo;
	pNanoSuit->GetOwner().GetEntity()->GetSlotInfo(0, slotInfo);

	if (slotInfo.pCharacter != NULL)
	{
		IAttachmentManager* attachmentManager = slotInfo.pCharacter->GetIAttachmentManager();
		const int nameCount = m_maximumSuitParams.bodyAttachmentNameArray.size();
		const int attachmentCount = attachmentManager->GetAttachmentCount();

		for(int n=0; n<nameCount; n++)
		{
			for(int a=0; a<attachmentCount; a++)
			{
				const char* attachmentName = attachmentManager->GetInterfaceByIndex(a)->GetName();
				const char* maxSuitAttachmentName = m_maximumSuitParams.bodyAttachmentNameArray[n].c_str();
				if(strcmp(attachmentManager->GetInterfaceByIndex(a)->GetName(),maxSuitAttachmentName)==0)
				{
					m_maximumSuitParams.bodyAttachmentIndexArray.push_back(a);
					break;
				}
			}
		}
	}	
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::CreateMaximumSuitMaterials(CNanoSuit* pNanoSuit)
{
	if(pNanoSuit)
	{
		IEntity* entity = pNanoSuit->GetOwner().GetEntity();

		if(entity)
		{
			// Get nano suit material
			IEntityRenderProxy* renderProxy = static_cast<IEntityRenderProxy*>(entity->GetProxy(ENTITY_PROXY_RENDER));
			
			if(renderProxy)
			{
				m_maximumSuitParams.nanoSuitMaterial = renderProxy->GetRenderMaterial();

				if(m_maximumSuitParams.nanoSuitMaterial)
				{
					// Clone material for maximum suit material
					m_maximumSuitParams.nanoSuitMaterial->AddRef();
					m_maximumSuitParams.maximumSuitMaterial = gEnv->p3DEngine->GetMaterialManager()->CloneMaterial(m_maximumSuitParams.nanoSuitMaterial);
					if(m_maximumSuitParams.maximumSuitMaterial)
					{
						m_maximumSuitParams.maximumSuitMaterial->AddRef();

						// Setup maximum suit material
						int subMaterialCount = m_maximumSuitParams.maximumSuitMaterial->GetSubMtlCount();
						IMaterial* subMaterial = NULL;
						for(int i=0; i<subMaterialCount; i++)
						{
							subMaterial = m_maximumSuitParams.maximumSuitMaterial->GetSubMtl(i);

							if(subMaterial)
							{
								IRenderShaderResources* shaderResources = subMaterial->GetShaderItem().m_pShaderResources;
								if(shaderResources)
								{
									shaderResources->GetDiffuseColor() = m_maximumSuitParams.diffuseColor;
									shaderResources->GetEmissiveColor() = m_maximumSuitParams.emissiveColor;
									shaderResources->GetSpecularColor() = m_maximumSuitParams.specularColor;
									shaderResources->GetGlow() = m_maximumSuitParams.glow;
									shaderResources->GetSpecularShininess() = m_maximumSuitParams.specularLevel;
								}
							}
						}
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
static void OnChangeCloakRefractionScale(ICVar* cvar)
{
	IMaterial* defaultLayersMaterial = gEnv->p3DEngine->GetMaterialManager()->GetDefaultLayersMaterial();
	if(defaultLayersMaterial)
	{
		const IMaterialLayer* layer = NULL;
		int layerCount = defaultLayersMaterial->GetLayerCount();

		UParamVal	refractionParam;
		refractionParam.m_Float = cvar->GetFVal();

		for(int l=0; l<layerCount; l++)
		{
			layer = defaultLayersMaterial->GetLayer(l);

			if(layer)
			{
				const SShaderItem& currShaderItem = layer->GetShaderItem();      
				IShader* shader = currShaderItem.m_pShader;

				if(shader)
				{
					if(SShaderParam::SetParam("RefrBumpScale", &(currShaderItem.m_pShaderResources->GetParameters()), refractionParam))
					{
						currShaderItem.m_pShaderResources->UpdateConstants(shader);
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::ReadCloakParams(const struct IItemParamsNode* pParams)
{
	if(!pParams)
		return;

	// Read and set light scale
	const IItemParamsNode* lightScaleParam = pParams->GetChild("lightScale");
	if(lightScaleParam)
	{
		ICVar* lightScaleCVar = gEnv->pConsole->GetCVar("r_CloakLightScale");
		if(lightScaleCVar)
		{
			float cloakLightScale = 1.0f;
			bool readParam = lightScaleParam->GetAttribute("value",cloakLightScale);

			if(readParam)
			{
				lightScaleCVar->Set(cloakLightScale);
			}
		}
	}

	// Set callback for refraction scale
	ICVar* refractionScaleCVar = gEnv->pConsole->GetCVar("g_cloakRefractionScale");
	if(refractionScaleCVar)
	{
		refractionScaleCVar->SetOnChangeCallback(OnChangeCloakRefractionScale);
	}

	// Read and set refraction scale
	const IItemParamsNode* refractionScaleParam = pParams->GetChild("refractionScale");
	if(refractionScaleParam)
	{
		if(refractionScaleCVar)
		{
			float cloakRefractionScale = 4.0f;
			bool readParam = refractionScaleParam->GetAttribute("value",cloakRefractionScale);
			if(readParam)
			{
				refractionScaleCVar->Set(cloakRefractionScale);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::ReadMaximumSuitPerkParams(const struct IItemParamsNode* pParams)
{
	if(!pParams)
		return;

	// Read body attachments
	const IItemParamsNode* bodyAttachmentData = pParams->GetChild("bodyAttachments");
	if(bodyAttachmentData)
	{
		const int attachmentNameCount = bodyAttachmentData->GetChildCount();
		const IItemParamsNode* attachmentNode = NULL;
		const char* attachmentName = NULL;

		m_maximumSuitParams.bodyAttachmentNameArray.resize(attachmentNameCount);

		for(int n=0; n<attachmentNameCount; n++)
		{
			attachmentNode = bodyAttachmentData->GetChild(n);
			attachmentName = attachmentNode->GetAttribute("value");

			m_maximumSuitParams.bodyAttachmentNameArray[n] = attachmentName;
		}
	}

	// Read material params
	Vec3 color;
	const IItemParamsNode* diffuseColorData = pParams->GetChild("diffuseColor");
	if(diffuseColorData)
	{
		diffuseColorData->GetAttribute("value",color);
		m_maximumSuitParams.diffuseColor.Set(color.x,color.y,color.z,1.0f);
	}

	const IItemParamsNode* specularColorData = pParams->GetChild("specularColor");
	if(specularColorData)
	{
		specularColorData->GetAttribute("value",color);
		m_maximumSuitParams.specularColor.Set(color.x,color.y,color.z,1.0f);
	}

	const IItemParamsNode* emissiveColorData = pParams->GetChild("emissiveColor");
	if(emissiveColorData)
	{
		emissiveColorData->GetAttribute("value",color);
		m_maximumSuitParams.emissiveColor.Set(color.x,color.y,color.z,1.0f);
	}

	CItemParamReader reader(pParams);
	ReadValue(m_maximumSuitParams, glow);
	ReadValue(m_maximumSuitParams, specularLevel);
}

//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::InvalidateMaximumSuitData()
{
	SAFE_RELEASE(m_maximumSuitParams.nanoSuitMaterial);
	SAFE_RELEASE(m_maximumSuitParams.maximumSuitMaterial);
	m_maximumSuitParams.bodyAttachmentIndexArray.Clear();
	m_maximumSuitParams.bodyAttachmentNameArray.Clear();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CNanoSuitParams::SSuitKnife::ReadParams( const struct IItemParamsNode* pParams )
{
	if (pParams)
	{
		object = pParams->GetAttributeSafe("object");
		attachment = pParams->GetAttributeSafe("attachment");

		if (!object.empty())
		{
			pKnifeObject = gEnv->p3DEngine->LoadStatObj(object.c_str(), NULL, NULL, false); //false is to force no streaming
			if (pKnifeObject)
			{
				pKnifeObject->AddRef();
			}
		}
	}
}