/*************************************************************************
  StalTech Source File.
  Copyright (C), StalTech Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Manages and updates target track groups for agents to
				determine which target an agent should select
  
 -------------------------------------------------------------------------
  History:
  - 02:01:2010: Created by Kevin Kirst

*************************************************************************/

#ifndef __TARGET_TRACK_MANAGER_H__
#define __TARGET_TRACK_MANAGER_H__

#include "ITargetTrackManager.h"
#include "TargetTrackCommon.h"

class CTargetTrackGroup;
class CTargetTrack;
struct ITargetTrackModifier;

struct SAIEVENT;
struct SAIPotentialTarget;

class CTargetTrackManager : public ITargetTrackManager
{
public:
	CTargetTrackManager();
	~CTargetTrackManager();

	void Init();
	void Shutdown();
	void Reset(IAISystem::EResetReason reason);
	void DebugDraw();
	bool ReloadConfig();
	void OnObjectRemoved(CAIObject *pObject);
	void Serialize(TSerialize ser);

	bool IsEnabled() const;

	// Target group accessors
	virtual bool SetTargetClassThreat(tAIObjectID aiObjectId, float fClassThreat);
	virtual float GetTargetClassThreat(tAIObjectID aiObjectId) const;
	virtual int GetTargetLimit(tAIObjectID aiObjectId) const;

	// Agent registration to use target tracks
	bool RegisterAgent(tAIObjectID aiObjectId, const char* szConfig, int nTargetLimit = 0);
	bool UnregisterAgent(tAIObjectID aiObjectId);
	bool ResetAgent(tAIObjectID aiObjectId);
	bool SetAgentEnabled(tAIObjectID aiObjectId, bool bEnable);

	// Incoming stimulus handling
	virtual bool HandleStimulusEvent(tAIObjectID aiTargetId, const char* szStimulusName, const Vec3 &vPos, float fRadius);
	virtual bool HandleStimulusEvent(tAIObjectID aiObjectId, tAIObjectID aiTargetId, const char* szStimulusName, const Vec3 &vPos);
	bool HandleStimulusEvent(tAIObjectID aiObjectId, const SAIEVENT *pAIEvent, TargetTrackHelpers::EAIEventStimulusType eType);
	virtual bool TriggerPulse(tAIObjectID aiObjectId, const char* szStimulusName, const char* szPulseName);

	// Outgoing desired target handling
	void Update(tAIObjectID aiObjectId);
	bool GetDesiredTarget(tAIObjectID aiObjectId, uint32 uDesiredTargetMethod, tAIObjectID &outTargetId, SAIPotentialTarget* &pOutTargetInfo);
	int GetDesiredTargetCount(tAIObjectID aiTargetId, tAIObjectID aiIgnoreId = 0) const;
	int GetPotentialTargetCount(tAIObjectID aiTargetId, tAIObjectID aiIgnoreId = 0) const;

private:
	// Target track pool management
	CTargetTrack* GetUnusedTargetTrackFromPool();
	void AddTargetTrackToPool(CTargetTrack *pTrack);

	class CTargetTrackPoolProxy : public TargetTrackHelpers::ITargetTrackPoolProxy
	{
	public:
		CTargetTrackPoolProxy(CTargetTrackManager *pManager) : m_pManager(pManager)
		{
			assert(m_pManager);
		}

		virtual CTargetTrack* GetUnusedTargetTrackFromPool()
		{
			return m_pManager->GetUnusedTargetTrackFromPool();
		}

		virtual void AddTargetTrackToPool(CTargetTrack *pTrack)
		{
			m_pManager->AddTargetTrackToPool(pTrack);
		}

	private:
		CTargetTrackManager *m_pManager;
	};

private:
	// Target track config accessing
	bool GetTargetTrackConfig(uint32 uNameHash, TargetTrackHelpers::STargetTrackConfig const* &pOutConfig) const;
	bool GetTargetTrackStimulusConfig(uint32 uNameHash, uint32 uStimulusHash, TargetTrackHelpers::STargetTrackStimulusConfig const* &pOutConfig) const;
	const ITargetTrackModifier* GetTargetTrackModifier(uint32 uId) const;

	class CTargetTrackConfigProxy : public TargetTrackHelpers::ITargetTrackConfigProxy
	{
	public:
		CTargetTrackConfigProxy(CTargetTrackManager *pManager) : m_pManager(pManager)
		{
			assert(m_pManager);
		}

		bool GetTargetTrackConfig(uint32 uNameHash, TargetTrackHelpers::STargetTrackConfig const* &pOutConfig) const
		{
			return m_pManager->GetTargetTrackConfig(uNameHash, pOutConfig);
		}

		bool GetTargetTrackStimulusConfig(uint32 uNameHash, uint32 uStimulusHash, TargetTrackHelpers::STargetTrackStimulusConfig const* &pOutConfig) const
		{
			return m_pManager->GetTargetTrackStimulusConfig(uNameHash, uStimulusHash, pOutConfig);
		}

		const ITargetTrackModifier* GetTargetTrackModifier(uint32 uId) const
		{
			return m_pManager->GetTargetTrackModifier(uId);
		}

	private:
		CTargetTrackManager *m_pManager;
	};

private:
	// Config load helpers
	bool LoadConfigs(XmlNodeRef &pRoot);
	bool LoadConfigStimuli(TargetTrackHelpers::STargetTrackConfig *pConfig, XmlNodeRef &pStimuliElement, bool bHasTemplate);
	bool LoadConfigModifiers(TargetTrackHelpers::STargetTrackStimulusConfig *pStimulusConfig, XmlNodeRef &pModifiersElement);
	bool LoadConfigPulses(TargetTrackHelpers::STargetTrackStimulusConfig *pStimulusConfig, XmlNodeRef &pPulsesElement);
	bool ApplyStimulusTemplates();
	bool ApplyStimulusTemplate(TargetTrackHelpers::STargetTrackConfig *pConfig, const TargetTrackHelpers::STargetTrackConfig *pParent);

	// Serialize helpers
	void Serialize_Write(TSerialize ser);
	void Serialize_Read(TSerialize ser);
	bool RegisterAgent(tAIObjectID aiObjectId, uint32 uConfigHash, int nTargetLimit = 0);

	// Stimulus helpers
	static tAIObjectID GetAIObjectId(EntityId entityId);
	bool HandleStimulusEvent(CTargetTrackGroup *pGroup, TargetTrackHelpers::STargetTrackStimulusEvent &stimulusEvent);
	bool CheckConfigUsesStimulus(uint32 uConfigHash, uint32 uStimulusNameHash) const;
	bool CheckStimulusHostile(tAIObjectID aiObjectId, tAIObjectID aiTargetId, uint32 uConfigHash, uint32 uStimulusNameHash) const;
	void TranslateVisualStimulus(TargetTrackHelpers::STargetTrackStimulusEvent &stimulusEvent, const SAIEVENT *pAIEvent) const;
	void TranslateSoundStimulus(TargetTrackHelpers::STargetTrackStimulusEvent &stimulusEvent, const SAIEVENT *pAIEvent) const;
	void TranslateBulletRainStimulus(TargetTrackHelpers::STargetTrackStimulusEvent &stimulusEvent, const SAIEVENT *pAIEvent) const;

	static uint32 GetConfigNameHash(const string& sName);
	static uint32 GetStimulusNameHash(const string& sStimulus);
	static uint32 GetPulseNameHash(const string& sPulse);

	void PrepareModifiers();
	void DeleteConfigs();
	void DeleteAgents();

	typedef std::map<uint32, TargetTrackHelpers::STargetTrackConfig*> TConfigContainer;
	TConfigContainer m_Configs;

	typedef std::map<tAIObjectID, CTargetTrackGroup*> TAgentContainer;
	TAgentContainer m_Agents;
	
	typedef std::vector<CTargetTrack*> TTargetTrackPoolContainer;
	TTargetTrackPoolContainer m_TargetTrackPool;

	typedef std::vector<ITargetTrackModifier*> TModifierContainer;
	TModifierContainer m_Modifiers;

	typedef std::map<tAIObjectID, float> TClassThreatContainer;
	TClassThreatContainer m_ClassThreatValues;

	CTargetTrackPoolProxy *m_pTrackPoolProxy;
	CTargetTrackConfigProxy *m_pTrackConfigProxy;

#ifdef TARGET_TRACK_DEBUG
	void DebugDrawConfig(int nMode);
	void DebugDrawTargets(int nMode, char const* szAgentName);
	void DebugDrawAgent(char const* szAgentName);

	// Used to give the agent being debugged one last frame to clean up when debug drawing is turned off for him
	tAIObjectID m_uLastDebugAgent;
#endif //TARGET_TRACK_DEBUG
};

#endif //__TARGET_TRACK_MANAGER_H__
