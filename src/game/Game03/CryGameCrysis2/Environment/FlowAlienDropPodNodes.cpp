/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Flowgraph nodes for the Alien drop pod logic
  
 -------------------------------------------------------------------------
  History:
  - 02:15:2009: Created by Kevin Kirst

*************************************************************************/

#include "StdAfx.h"
#include "Nodes/G2FlowBaseNode.h"
#include "Environment/AlienDropPod.h"

//////////////////////////////////////////////////////////////////////////
class CFlowNode_SpawnAlienDropPod : public CFlowBaseNode, public CAlienDropPodListener
{
public:
	//////////////////////////////////////////////////////////////////////////
	CFlowNode_SpawnAlienDropPod(SActivationInfo *pActInfo)
	{
		
	}

	//////////////////////////////////////////////////////////////////////////
	virtual ~CFlowNode_SpawnAlienDropPod(void)
	{
		IEntitySystem *pEntitySystem = gEnv ? gEnv->pEntitySystem : NULL;
		if (pEntitySystem)
		{
			TLaunchedPodsContainer::const_iterator itContainerEnd = m_LaunchedPods.end();
			for (TLaunchedPodsContainer::iterator itContainer = m_LaunchedPods.begin(); itContainer != itContainerEnd; ++itContainer)
			{
				const EntityId idLaunchedPod = *itContainer;
				CAlienDropPod *pLaunchedPod = static_cast<CAlienDropPod*>(g_pGame->GetIGameFramework()->QueryGameObjectExtension(idLaunchedPod, "AlienDropPod"));
				if (pLaunchedPod)
				{
					pLaunchedPod->RemoveListener(this);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
//c2
	//virtual void GetMemoryUsage(ICrySizer * s) const
	//{
	//	s->Add(*this);
	//}
//312
	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}


	//////////////////////////////////////////////////////////////////////////
	IFlowNodePtr Clone(SActivationInfo * pActInfo)
	{
		return new CFlowNode_SpawnAlienDropPod(pActInfo);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void Serialize(SActivationInfo *pActInfo, TSerialize ser)
	{
		ser.Value("m_LaunchedPods", m_LaunchedPods);
	}

	enum EInputPorts
	{
		EIP_Spawn,
		EIP_PodArchetype,
		EIP_SpawnOffset,
		EIP_DestinationId,
		EIP_DestinationPos,
		EIP_MaxForce,
		EIP_ComeToRest,
		EIP_BounceCount,
		EIP_BounceForce,
		EIP_LinkedAI,
		EIP_Territory,
		EIP_Wave,
	};

	enum EOutputPorts
	{
		EOP_Failed,
		EOP_PodFired,
		EOP_PodBouncedPos,
		EOP_PodBouncedCount,
		EOP_PodLanded,
		EOP_AlienSpawned,
	};

	//////////////////////////////////////////////////////////////////////////
	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static SInputPortConfig inputs[] =
		{
			InputPortConfig_Void("Spawn", _HELP("Call to spawn the drop pod")),
			InputPortConfig<string>("PodArchetype", _HELP("Archetype to use for the drop pod. Leave blank for default"), _HELP("Pod Archetype"), _UICONFIG("enum_global:entity_archetypes")),
			InputPortConfig<Vec3>("Offset", _HELP("Offset from owning entity, where the drop pod should be created")),
			InputPortConfig<EntityId>("DestinationId", _HELP("Entity Id of what to fire the drop pod towards - Used in place of Destination Pos if specified"), _HELP("Destination Id")),
			InputPortConfig<Vec3>("DestinationPos", _HELP("Position to fire the drop pod towards - Ignored if Destination Id is specified"), _HELP("Destination Position")),
			InputPortConfig<float>("LaunchForce", 50.0f, _HELP("Force applied to pod as it is launched. Force is slowly lost with each bounce."), _HELP("Launch Force")),
			InputPortConfig<bool>("ComeToRest", true, _HELP("Specify if the pod should need to come to rest before opening or if it can open immediately after the last bounce"), _HELP("Come To Rest")),
			InputPortConfig<int>("Bounces", 0, _HELP("Number of times the pod can bounce before it opens")),
			InputPortConfig<float>("BounceForce", 1.0f, _HELP("Modifier for the force applied per bounce"), _HELP("Bounce Force")),
			InputPortConfig<EntityId>("LinkedAI", _HELP("Optional: Link the drop pod to an existing AI. The AI will be beamed to the pod when it opens instead of creating a new AI"), _HELP("Linked AI")),
			InputPortConfig<string>("Territory", "<None>", _HELP("Optional: Territory to place the newly spawned AI in"), NULL, NULL),
			InputPortConfig<string>("Wave", "<None>", _HELP("Optional: Wave to place the newly spawned AI in"), NULL, NULL),
			{0}
		};

		static const SOutputPortConfig outputs[] =
		{
			OutputPortConfig_Void("Failed", _HELP("Called if pod failed to launch or spawn an Alien")),
			OutputPortConfig<Vec3>("PodFired", _HELP("Called if pod was fired successfully, returning starting position of pod"), _HELP("Pod Fired")),
			OutputPortConfig<Vec3>("PodBouncedPos", _HELP("Called when pod bounces, returning position of bounce"), _HELP("Pod Bounce Point")),
			OutputPortConfig<int>("PodBouncedCnt", _HELP("Called when pod bounces, returning number of bounces thus far"), _HELP("Pod Bounce Count")),
			OutputPortConfig<Vec3>("PodLanded", _HELP("Called when pod has fully landed (stopped rolling), returning final resting point"), _HELP("Pod Landed")),
			OutputPortConfig<EntityId>("AlienSpawned", _HELP("Called when Alien is spawned, outputting the EntityId of the new Alien"), _HELP("Alien Spawned")),
			{0}
		};

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Spawn an alien drop pod, to be fired at the designated point, creating the specified Alien AI there."
			"Owning entity should be what spawns the drop pod i.e., the drop ship.");
		config.SetCategory(EFLN_APPROVED);
		config.nFlags |= EFLN_TARGET_ENTITY;
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
	{
		if (eFE_Initialize == event)
		{
			m_actInfo = *pActInfo;
		}
		else if (eFE_Activate == event && IsPortActive(pActInfo, EIP_Spawn))
		{
			const bool bResult = HandlePodSpawn(pActInfo);
			if (!bResult)
			{
				ActivateOutput(pActInfo, EOP_Failed, true);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void OnPodLaunched(bool bSuccess, const Vec3& vPos, const SAlienPodLaunchProperties& launchProperties)
	{
		if (bSuccess)
		{
			ActivateOutput(&m_actInfo, EOP_PodFired, vPos);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void OnPodBounced(int nBounceCount, const Vec3& vPos)
	{
		ActivateOutput(&m_actInfo, EOP_PodBouncedPos, vPos);
		ActivateOutput(&m_actInfo, EOP_PodBouncedCount, nBounceCount);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void OnPodLanded(const Vec3& vPos)
	{
		ActivateOutput(&m_actInfo, EOP_PodLanded, vPos);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void OnPodOpened(EntityId alienSpawnedId)
	{
		if (alienSpawnedId > 0)
		{
			ActivateOutput(&m_actInfo, EOP_AlienSpawned, alienSpawnedId);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void OnPodDestroyed(EntityId alienPodId)
	{
		stl::find_and_erase(m_LaunchedPods, alienPodId);
	}

private:
	//////////////////////////////////////////////////////////////////////////
	bool HandlePodSpawn(SActivationInfo *pActInfo)
	{
		bool bResult = false;

		if (pActInfo->pEntity)
		{
			IEntitySystem *pEntitySystem = gEnv->pEntitySystem;
			CRY_ASSERT(pEntitySystem);

			const Vec3 vStartPos = pActInfo->pEntity->GetWorldPos() + GetPortVec3(pActInfo, EIP_SpawnOffset);
			Vec3 vDestination(ZERO);
			EntityId destinationId = GetPortEntityId(pActInfo, EIP_DestinationId);
			if (IEntity *pDestinationEntity = pEntitySystem->GetEntity(destinationId))
			{
				vDestination = pDestinationEntity->GetWorldPos();
			}
			else
			{
				vDestination = GetPortVec3(pActInfo, EIP_DestinationPos);
			}

			const string sPodArchetype = GetPortString(pActInfo, EIP_PodArchetype);

			SEntitySpawnParams params;
			params.sName = "AlienDropPod";
			params.pArchetype = (!sPodArchetype.empty() ? pEntitySystem->LoadEntityArchetype(sPodArchetype.c_str()) : NULL);
			params.pClass = pEntitySystem->GetClassRegistry()->FindClass("AlienDropPod");
			params.vPosition = vStartPos;

			IEntity *pPodEntity = pEntitySystem->SpawnEntity(params);
			if (pPodEntity)
			{
				CAlienDropPod *pPod = static_cast<CAlienDropPod*>(g_pGame->GetIGameFramework()->QueryGameObjectExtension(pPodEntity->GetId(), "AlienDropPod"));
				if (pPod)
				{
					pPod->AddListener(this);

					// Launch it
					SAlienPodLaunchProperties launchProperties;
					launchProperties.vDestination = vDestination;
					launchProperties.sTerritory = GetPortString(pActInfo, EIP_Territory);
					launchProperties.sWave = GetPortString(pActInfo, EIP_Wave);
					launchProperties.fMaxForce = GetPortFloat(pActInfo, EIP_MaxForce);
					launchProperties.bComeToRest = GetPortBool(pActInfo, EIP_ComeToRest);
					launchProperties.fBounceForce = GetPortFloat(pActInfo, EIP_BounceForce);
					launchProperties.nBounceCount = GetPortInt(pActInfo, EIP_BounceCount);
					launchProperties.idHost = pActInfo->pEntity->GetId();
					launchProperties.idLinkedAI = GetPortEntityId(pActInfo, EIP_LinkedAI);

					bResult = pPod->LaunchPod(launchProperties);
				}
			}
				
			if (bResult)
			{
				m_LaunchedPods.push_back(pPodEntity->GetId());
			}
			else
			{
				// Not a valid entity, remove it
				if (pPodEntity)
					pEntitySystem->RemoveEntity(pPodEntity->GetId());

				GameWarning("Alien drop pod failed to launch from AI:SpawnAlienDropPod Flowgraph node");
			}
		}
		else
		{
			GameWarning("No owning entity specified for AI:SpawnAlienDropPod Flowgraph node");
		}

		return bResult;
	}

private:
	typedef std::vector<EntityId> TLaunchedPodsContainer;
	TLaunchedPodsContainer m_LaunchedPods;

	SActivationInfo m_actInfo;
};


//////////////////////////////////////////////////////////////////////////
// Register nodes

REGISTER_FLOW_NODE("AI:SpawnAlienDropPod", CFlowNode_SpawnAlienDropPod);
