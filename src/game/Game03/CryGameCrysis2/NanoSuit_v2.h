/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id:$
$DateTime$
Description:  Nanosuit v2 
-------------------------------------------------------------------------
History:
- 3-3-2008: Created by Benito G.R.

*************************************************************************/

# pragma once

#ifndef _NANOSUIT_V2_H_
#define _NANOSUIT_V2_H_

#if !defined(_RELEASE)
	#define DEBUG_NANOSUIT
#endif

#include "NanoSuitDefs.h"
#include "NanoSuitParams.h"
#include "SuitShapeDeformation.h"

class CPlayer;
struct IItemParamsNode;
struct SActorStats;
struct IAttachment;
struct ILightSource;

class CNanoSuit
{

private:

	friend class CNanoModePower;
	friend class CNanoModeArmor;
	friend class CNanoModeStealth;
	friend class CNanoModeTactical;
	
public:

		struct INanoSuitListener
		{
			virtual void ModeChanged(ENanoSuitMode mode) = 0;			
			virtual void EnergyChanged(float energy) = 0;					
			virtual void OnSuitStateChanged(ENanoSuitState state) = 0;
			virtual void OnModeSelected(ENanoSuitMode mode) = 0;
			virtual void OnSuitPowerActivated(const bool activated) = 0;

			virtual void GetMemoryUsage(ICrySizer *pSizer ) const{};
		};

		CNanoSuit(CPlayer& _owner);
		~CNanoSuit();

		void Reset();

		void Update(float frameTime);
		void ActivateMode(ENanoSuitMode mode);
		void ActivateLastMode();
		void ProcessEvent(const SNanoSuitEvent& event);
		void EnterSlowMotion();
		void ExitSlowMotion();
		void AllowMode( ENanoSuitMode mode, bool allow );
		bool IsModeAllowed( ENanoSuitMode mode );

		void ShowKnife(bool show);

		ILINE const SNanoSuitGameParameters& GetGameParams() const { return m_suitGameParams; }
		ILINE CPlayer& GetOwner() const { return m_suitOwner; }
		ILINE bool IsSuitPowerActive() const { return m_suitGameParams.IsSuitPowerActive(); }

		void ScaleEnergyUse (float scale) { m_suitGameParams.GetEnergy().ScaleEnergyUse(scale); }

		// listener 
		void AddListener(INanoSuitListener* pListener);
		void RemoveListener(INanoSuitListener* pListener);

		// Memory stats
		void GetMemoryUsage(ICrySizer *pSizer ) const;

		// Network
		void NetSerialize( TSerialize ser, EEntityAspects aspect );

		// Serialization
		void Serialize( TSerialize ser );

		void OnSuitModeSelected(ENanoSuitMode mode);

		static void ReloadSuitData();
		void OnSuitDataChanged();

		//Recording system stuff
		static const char* GetNanoSuitModeName(int mode);

		void SetMaximumSuitMaterial(bool setMaterial);

	private:

		void Init();
		void Release();
		ILINE bool IsReseting() const { return m_reseting; }

		void SynchNanoSuitMaterial();
		void StopRunningSpecialStates();
		tSoundID PlaySound(const char* soundName, bool is2DSound);
		void StopSound(tSoundID soundId);

		void SwitchToSuitState(ENanoSuitState newState);
		void OnLeaveState(ENanoSuitState leavingState);
		void OnEnterState(ENanoSuitState enteringState);
		void ActivateSuitPower(bool activate);
		void UpdateSoundParam();

		void UpdateOutlineColor();
		
		void UpdateAIClassThreat() const;

		//Action, state (only accessible inside suit/modes)
		void ProcessSuitAction(const char* actionName, bool playSound = true);
		void ActivateSuitState(const char* stateName, bool activate, float timer = 0.0f, bool playSound = true);

		// Notify listeners
		void OnEnergyChanged(float prevEnergy);
		void OnSuitModeChanged();
		void OnSuitStateChanged();
		void OnSuitPowerActivated(const bool activated);


		// Common suit update 
		void UpdateCommon(float frameTime);
		void UpdateSuitModeTimers(float frameTime);
		void PostStateUpdate(float prevEnergy);

		// Initialize stuff		
		void CreateNanoModes();

		// Suit layer effects
		void StartModeTransition();
		void EndModeTransition();
		void UpdateModeTransition();
		void UpdateActiveMode(float frameTime);
		IAttachment* GetCharacterAttachment(const char* attachmentName);
		ILightSource* GetLightSource(IAttachment* pAttachment);
		void SetUpLightAttachment(IAttachment* pAttachment);
		void SetLayerEffectParameters(const Vec3& layerConfig, float transitionWeight);

		// Suit knife
		void CreateSuitKnife();

#ifdef DEBUG_NANOSUIT
		void DebugDraw(float frameTime);
#endif

		typedef std::vector<INanoMode *>									TNanoModeVector;
		typedef std::vector<INanoSuitListener *>					TNanoSuitListenersVector;

		CPlayer&				m_suitOwner;
		CNanoSuitShapeDeformation m_suitShapeDeformation;

		SNanoSuitGameParameters	m_suitGameParams;

		CNanoSuitParams::TNanoSuitSpecialStates m_specialSuitStates;

		INanoMode*			m_pNanoMode;
		TNanoModeVector	m_nanoModes;

		TNanoSuitListenersVector m_listeners;
		
		ENanoSuitState m_lastState;
		ENanoSuitMode	 m_lastMode;

		CNanoSuitParams& s_nanoSuitParams;
		static CNanoSuitParams& GetNanoSuitParams();
		float					m_suitModeTimer; // Times how long in current suit mode
		float					m_curretSuitShaderLayerTransitionWeight;

		bool					m_notifiedAfterModeActivation; // there is a delayed notification to the current nanosuit mode a fixed time after activation
		bool					m_modeTransitioning;
		bool					m_suitWasStressed;
		bool					m_reseting;
		bool					m_hasSilhouette;
		
		bool          m_allowedModes[eNanoSuitMode_Last];

		static const char* s_nanoSuitModeName[eNanoSuitMode_Last];
};

#endif
