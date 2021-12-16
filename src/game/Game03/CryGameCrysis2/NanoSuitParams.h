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

#pragma once

#ifndef _NANOSUIT_PARAMS_H_
#define _NANOSUIT_PARAMS_H_

#include "NanoSuitDefs.h"
#include "Audio/AudioSignalPlayer.h"

class CNanoSuit;

class CNanoSuitParams
{
public:

	struct SActionCost
	{
		SActionCost()
		{
			energyCost = 0.0f;
		}

		float energyCost;
	};

	struct SActionFFeedback
	{
		SActionFFeedback()
			: time(0.0f)
			, amplifierA(0.2f)
			, amplifierB(0.2f)
		{

		}

		void GetMemoryUsage(ICrySizer* s) const
		{
			s->AddObject(name);
		}
		
		ItemString name;
		float time;
		float amplifierA;
		float amplifierB;
	};

	struct SActionScreenFX
	{
		SActionScreenFX()
		{
			blendOutTime = 0.0f;
		}

		void GetMemoryUsage(ICrySizer* s) const
		{
			s->Add(libraryName);
			s->Add(effectName);
		}

		ItemString libraryName;
		ItemString effectName;
		float			 blendOutTime;
	};

	struct SActionCameraShake
	{
		SActionCameraShake()
			: time(0)
			, frequency(0)
			, shift(0, 0, 0)
			, rotate(0, 0, 0)
			, viewAngleAttenuation(true)
			, randomDirection(false) {}

		void GetMemoryUsage(ICrySizer* s) const {}

		Vec3		shift;
		Vec3		rotate;
		float		time;
		float		frequency;
		bool		viewAngleAttenuation;
		bool		randomDirection;
	};

	struct SNanoSuitAction
	{
		SNanoSuitAction()
		{
		}

		void GetMemoryUsage(ICrySizer* s) const
		{
			s->AddObject(screenFX);
			s->AddObject(fFeedback);
			s->AddObject(cameraShake);
		}

		SActionCost					cost;
		SActionFFeedback		fFeedback;
		SActionScreenFX			screenFX;
		SActionCameraShake	cameraShake;
		CAudioSignalPlayer	audioSignals[2];
	};

	struct SNanoSuitSpecialStateReadability
	{
		SNanoSuitSpecialStateReadability()
		{
			particleFXSlot = -1;
			particleFXOffset.zero();
		}

		void GetMemoryUsage(ICrySizer* s) const
		{
			s->AddObject(particleFX);
		}

		CAudioSignalPlayer audioSignals[2];

		SActionFFeedback ffeedBack;

		// Particle (surface) fx
		ItemString particleFX;
		Vec3			 particleFXOffset;
		int				 particleFXSlot;
		bool			 isSurfaceFX;
	};

	struct SNanoSuitSpecialState
	{
		SNanoSuitSpecialState()
			: bAllowRestart(false)
			, bRunning(false)
			, fTime(0.0f)
			, fExternalTimer(0.0f)
			, fRunningTime(0.0f)
		{
		}
	
		void GetMemoryUsage(ICrySizer* s) const
		{
			s->AddObject(onEnterAction);
			s->AddObject(onLeaveAction);
			s->AddObject(readability);
		}

		SNanoSuitSpecialStateReadability readability;
		ItemString onEnterAction;
		ItemString onLeaveAction;

		float fTime, fExternalTimer, fRunningTime;
		bool bAllowRestart;
		bool bRunning;
	};

	struct SSuitLayerEffects
	{
		SSuitLayerEffects()
			: light_attachment("suitmode_light")
			, light_radius(1.5f)
			, light_color(0.0f, 0.0f, 1.0f)
			, light_specularMult(2.0f)
			, light_diffuseMult(1.0f)
			, light_hdrDynamic(1.0f)
		{
			layerEffect[eNanoSuitMode_Power].Set(0.0f, 1.0f, 0.0f);
			layerEffect[eNanoSuitMode_Armor].Set(1.0f, 0.0f, 0.0f);
			layerEffect[eNanoSuitMode_Stealth].Set(0.0f, 0.0f, 1.0f);
			layerEffect[eNanoSuitMode_Tactical].Set(0.0f, 0.0f, 0.0f);
		}

		const Vec3& GetLayerConfig(int index) const 
		{ 
			assert((index >= 0) && (index<eNanoSuitMode_Last));

			return layerEffect[index];
		}

		ItemString	light_attachment;
		Vec3				light_color;
		float				light_radius;
		float				light_specularMult;
		float				light_diffuseMult;
		float				light_hdrDynamic;
		Vec3				layerEffect[eNanoSuitMode_Last];
	};

	struct SSuitKnife
	{
		SSuitKnife()
			: object("")
			, attachment("knife")
			, pKnifeObject(NULL)
		{

		}

		~SSuitKnife()
		{
			ReleaseKnife();
		}

		void ReadParams(const struct IItemParamsNode* pParams);

		ILINE const char* GetKnifeObjectName() const
		{
			return object.c_str();
		}

		ILINE const char* GetKnifeAttachment() const
		{
			return attachment.c_str();
		}

		void ReleaseKnife()
		{
			SAFE_RELEASE(pKnifeObject);
		}

	private:

		ItemString	object;
		ItemString	attachment;
		IStatObj*		pKnifeObject;
	};

	struct SMaximumSuitPerk
	{
		SMaximumSuitPerk()
			: diffuseColor(0.0f,0.0f,0.0f,0.0f)
			, specularColor(0.0f,0.0f,0.0f,0.0f)
			, emissiveColor(0.0f,0.0f,0.0f,0.0f)
			, glow(0.0f)
			, specularLevel(0.0f)
			, nanoSuitMaterial(NULL)
			, maximumSuitMaterial(NULL) {}

		PodArray<int>					bodyAttachmentIndexArray;  // Used to set the material on the relevant body parts
		PodArray<ItemString>	bodyAttachmentNameArray;   //
		ColorF								diffuseColor;
		ColorF								specularColor;
		ColorF								emissiveColor;
		float									glow;
		float									specularLevel;
		IMaterial*						nanoSuitMaterial;
		IMaterial*						maximumSuitMaterial;
	};

	typedef std::map<ItemString, SNanoSuitAction>			TNanoSuitActions;
	typedef std::map<ItemString, SNanoSuitSpecialState> TNanoSuitSpecialStates;
	typedef std::map<ItemString, SActionCost>			TActionCostPresets;
	typedef std::map<ItemString, SActionFFeedback>		TActionFFeedbackPresets;
	typedef std::map<ItemString, SActionScreenFX>		TActionScreenFXPresets;
	typedef std::map<ItemString, SActionCameraShake>	TActionCameraShakePresets;
	typedef std::set<CNanoSuit*>	TSuitOwners;

public:

	CNanoSuitParams();
	~CNanoSuitParams();

	void LoadXMLData(const char* file);
	
	ILINE bool IsDataLoaded() const { return m_dataLoaded; }
	void InvalidateData();

	ILINE const TNanoSuitSpecialStates& GetSuitStates() const { return m_specialStatesMap; }
	ILINE const TNanoSuitActions& GetSuitActions() const { return m_actionsMap; }
	ILINE TNanoSuitActions& GetSuitActions() { return m_actionsMap; }

	ILINE const SNanoSuitGameParameters& GetDefaultGameParams() const { return m_defaultGameParams; } 

	ILINE const SNanoSuitEnergyParams& GetModeEnergyMod(int index) const { return m_modeEnergyMods[index]; }
	ILINE const SNanoSuitProperties& GetModePassiveProperties(int index) const { return m_modePassiveMods[index]; }
	ILINE const SNanoSuitProperties& GetModeActiveProperties(int index) const { return m_modeActiveMods[index]; }

	ILINE const SSuitLayerEffects& GetLayerEffectParams() const { return m_layerEffectParams; }
	ILINE const SSuitKnife& GetSuitKnifeInfo() const { return m_suitKnife; }

	ILINE const SMaximumSuitPerk& GetMaximumSuitPerkParams() const { return m_maximumSuitParams; }

	void AddNanoSuit(CNanoSuit* pNanoSuit);
	void RemoveNanoSuit(CNanoSuit* pNanoSuit);

	CAudioSignalPlayer* GetEnergyBarAudioSignal() {return &m_energyBarAudio;}

private:

	void NotifyDataChanged();

	void ReadNanoSuitEnergyParams(const struct IItemParamsNode* pParams);
	void ReadNanoSuitGameParams(const struct IItemParamsNode* pParams);
	void LoadLayerEffectParams(const struct IItemParamsNode* pParams);
	void ReadSuitActions(const struct IItemParamsNode* pParams);
	void ReadAction(const struct IItemParamsNode* pParams, SNanoSuitAction& action);
	void ReadSuitStates(const struct IItemParamsNode* pParams);
	void ReadState(const struct IItemParamsNode* pParams, SNanoSuitSpecialState& state);

	void ReadActionCost(const struct IItemParamsNode* pParams, SActionCost* pCost);
	void ReadActionFFeedback(const struct IItemParamsNode* pParams, SActionFFeedback* pFfeedback);
	void ReadActionScreenFx(const struct IItemParamsNode* pParams, SActionScreenFX* pScreenFx);
	void ReadActionCameraShake(const struct IItemParamsNode* pParams, SActionCameraShake* pCameraShake);

	void ReadModeParams(ENanoSuitMode mode, const struct IItemParamsNode* pParams);
	void ReadModeEnergyMod(const struct IItemParamsNode* pParams, SNanoSuitEnergyParams& energyMod);
	void ReadModeGameParamsMod(const struct IItemParamsNode* pParams, SNanoSuitProperties& passiveMod, SNanoSuitProperties& activeMod);

	void ReadCloakParams(const struct IItemParamsNode* pParams);
	void ReadMaximumSuitPerkParams(const struct IItemParamsNode* pParams);

	void CacheBodyPartIndexArray(CNanoSuit* pNanoSuit);
	void CreateMaximumSuitMaterials(CNanoSuit* pNanoSuit);
	void InvalidateMaximumSuitData();

	SNanoSuitGameParameters m_defaultGameParams;

	TNanoSuitActions			m_actionsMap;
	TActionCostPresets		m_actionCostPresets;
	TActionFFeedbackPresets m_actionFFeedbackPresets;
	TActionScreenFXPresets  m_actionScreenFXPresets;
	TActionCameraShakePresets m_actionCameraShakePresets;
	TNanoSuitSpecialStates		m_specialStatesMap;

	SSuitLayerEffects			m_layerEffectParams;
	SSuitKnife						m_suitKnife;

	SNanoSuitEnergyParams m_modeEnergyMods[eNanoSuitMode_Last];
	SNanoSuitProperties		m_modePassiveMods[eNanoSuitMode_Last];
	SNanoSuitProperties		m_modeActiveMods[eNanoSuitMode_Last];

	CAudioSignalPlayer		m_energyBarAudio;

	SMaximumSuitPerk			m_maximumSuitParams;
	
	// For suit reloading parameters
	TSuitOwners m_nanoSuitOwners;

	bool	m_dataLoaded;		
};

#endif //_NANOSUIT_PARAMS_H_
