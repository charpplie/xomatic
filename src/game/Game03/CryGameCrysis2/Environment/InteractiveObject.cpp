/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Crysis2 interactive object, for playing co-operative animations with player

-------------------------------------------------------------------------
History:
- 10:12:2009: Created by Benito G.R.

*************************************************************************/

#include "StdAfx.h"
#include "InteractiveObject.h"
#include "InteractiveObjectRegistry.h"
#include "ScriptBind_InteractiveObject.h"

#include "../Game.h"
#include "../Actor.h"


CInteractiveObjectEx::CInteractiveObjectEx()
: m_state(eState_NotUsed)
, m_physicalizationAfterAnimation(PE_NONE)
, m_interactionRadius(1.5f)
, m_interactionAngle(70.0f)
, m_currentLoadedCharacterCrc(0)
, m_currentInteractionCrc(0)
{
	m_alignmentHelperLocation.SetIdentity();
}


CInteractiveObjectEx::~CInteractiveObjectEx()
{
	if (g_pGame)
	{
		g_pGame->GetInteractiveObjectsRegistry().UnregisterInteractiveObject(GetEntityId());
		g_pGame->GetInteractiveObjectScriptBind()->Detach(GetEntityId());
	}
}

bool CInteractiveObjectEx::Init( IGameObject *pGameObject )
{
	SetGameObject(pGameObject);

	if (!Reset())
		return false;

	if(!GetGameObject()->BindToNetwork())
		return false;

	g_pGame->GetInteractiveObjectScriptBind()->AttachTo(this);

	return true;
}

void CInteractiveObjectEx::InitClient( int channelId )
{

}

void CInteractiveObjectEx::PostInit( IGameObject *pGameObject )
{

}

void CInteractiveObjectEx::PostInitClient( int channelId )
{

}

void CInteractiveObjectEx::Release()
{
	delete this;
}

void CInteractiveObjectEx::FullSerialize( TSerialize ser )
{

}

bool CInteractiveObjectEx::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	return true;
}

void CInteractiveObjectEx::PostSerialize()
{

}

void CInteractiveObjectEx::SerializeSpawnInfo( TSerialize ser )
{

}

ISerializableInfoPtr CInteractiveObjectEx::GetSpawnInfo()
{
	return 0;
}

void CInteractiveObjectEx::Update( SEntityUpdateContext &ctx, int updateSlot )
{

}

void CInteractiveObjectEx::PostUpdate( float frameTime )
{

}

void CInteractiveObjectEx::PostRemoteSpawn()
{

}

void CInteractiveObjectEx::HandleEvent( const SGameObjectEvent &goEvent )
{

}

void CInteractiveObjectEx::ProcessEvent( SEntityEvent &entityEvent )
{
	switch (entityEvent.event)
	{
	case ENTITY_EVENT_RESET:
		{
			Reset();
		}
		break;
	}
}

void CInteractiveObjectEx::SetChannelId( uint16 id )
{

}

void CInteractiveObjectEx::SetAuthority( bool auth )
{

}

void CInteractiveObjectEx::GetMemoryUsage(ICrySizer *pSizer) const
{
	pSizer->AddObject(this, sizeof(*this));
}

bool CInteractiveObjectEx::Reset()
{
	m_state = eState_NotUsed;

	SmartScriptTable entityProperties;
	IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
	if(!pScriptTable || !pScriptTable->GetValue("Properties", entityProperties))
		return false;

	//Physics properties
	SmartScriptTable physicProperties;
	if (entityProperties->GetValue("Physics", physicProperties))
	{
		int physicsValue = 0;
		if (physicProperties->GetValue("bRigidBody", physicsValue) && (physicsValue != 0))
		{
			m_physicalizationAfterAnimation = PE_RIGID;
		}
	}

	//Model and interaction
	const char* interactionName = NULL;
	SmartScriptTable interactionProperties;
	if (entityProperties->GetValue("Interaction", interactionProperties))
	{
		interactionProperties->GetValue("Interaction", interactionName);
		interactionProperties->GetValue("InteractionRadius", m_interactionRadius);
		interactionProperties->GetValue("InteractionAngle", m_interactionAngle);

		const char* objectModel = NULL;
		interactionProperties->GetValue("object_Model", objectModel);
		assert(objectModel);

		uint32 crcForModel = GetCrcForName(objectModel);
		if (crcForModel != m_currentLoadedCharacterCrc)
		{
			GetEntity()->LoadCharacter(0, objectModel);

			m_currentLoadedCharacterCrc = crcForModel;
		}
		else if (ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0))
		{
			ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose();
			assert(pSkeletonPose);
			if (pSkeletonPose)
			{
				pSkeletonPose->SetDefaultPose();	//Reset anims
			}
		}

		Physicalize(PE_STATIC);
	}

	//Register
	if (IsValidInteractionName(interactionName))
	{
		uint32 crcForInteraction = GetCrcForName(interactionName);

		if (crcForInteraction != m_currentInteractionCrc)
		{
			g_pGame->GetInteractiveObjectsRegistry().RegisterInteractiveObject(GetEntityId(), interactionName);

			m_currentInteractionCrc = crcForInteraction;
		}

		const SInteractionParams* pInteractionParams = g_pGame->GetInteractiveObjectsRegistry().GetInteractionParamsByName(interactionName);
		if (pInteractionParams)
		{
			CalculateAlignmentHelperLocation(pInteractionParams->helperName.c_str());
		}
		else
		{
			GameWarning("Interaction '%s' not found, double check PlayerInteractions.xml", interactionName);
		}
	}
	else
	{
		GameWarning("Invalid interaction name provided for entity '%s'", GetEntity()->GetName());
	}

	return true;
}

bool CInteractiveObjectEx::CanUse( EntityId entityId ) const
{
	//Only client can use these objects (for now at least)
	bool userIsClient = (g_pGame->GetIGameFramework()->GetClientActorId() == entityId);
	bool canUse = (userIsClient) && (m_state == eState_NotUsed) && IsUserNearEnough(entityId);

	return canUse;
}

void CInteractiveObjectEx::Use( EntityId entityId )
{
	if (m_state == eState_NotUsed)
	{
		StartUse(entityId);
	}
}

void CInteractiveObjectEx::StartUse( EntityId entityId )
{
	//Notify the user, interaction starts
	CActor* pActor = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(entityId));

	if (pActor)
	{
		pActor->StartInteractiveAction(GetEntityId());
	}

	m_state = eState_InUse;
}

void CInteractiveObjectEx::Done( EntityId entityId )
{
	if (m_physicalizationAfterAnimation == PE_RIGID)
	{
		Physicalize(PE_RIGID);
	}

	m_state = eState_Done;
}

void CInteractiveObjectEx::StopUse( EntityId entityId )
{
	Done(entityId);
}

void CInteractiveObjectEx::CalculateAlignmentHelperLocation(const char* helperName)
{
	m_alignmentHelperLocation.t = GetEntity()->GetWorldPos();
	m_alignmentHelperLocation.q = GetEntity()->GetWorldRotation();

	if (ICharacterInstance* pObjectCharacter = GetEntity()->GetCharacter(0))
	{
		int16 jointId = pObjectCharacter->GetISkeletonPose()->GetJointIDByName(helperName);
		if (jointId >= 0)
		{
			const QuatT& helperLocation = pObjectCharacter->GetISkeletonPose()->GetAbsJointByID(jointId);
			m_alignmentHelperLocation.t = m_alignmentHelperLocation.t + (m_alignmentHelperLocation.q * helperLocation.t);
			m_alignmentHelperLocation.q = m_alignmentHelperLocation.q * helperLocation.q;
		}
		else
		{
			GameWarning("Helper '%s' not found, default to object location as target player position.", helperName);
		}
	}
	else
	{
		GameWarning("No character, default to object location as target player position.");
	}
}

bool CInteractiveObjectEx::IsUserNearEnough( EntityId userId ) const
{
	IEntity* pUserEntity = gEnv->pEntitySystem->GetEntity(userId);
	if (pUserEntity)
	{
		const float angleLimitDot = (float)__fsel(-m_interactionAngle, -1.0f, cos_tpl(DEG2RAD(m_interactionAngle)));
		const float distanceLimit = m_interactionRadius;
		QuatT userLocation(pUserEntity->GetWorldPos(), pUserEntity->GetWorldRotation());

		const float actualAngleDot = m_alignmentHelperLocation.q.GetColumn1().Dot(userLocation.q.GetColumn1()) > angleLimitDot;
		const float actualDistance = (m_alignmentHelperLocation.t - userLocation.t).len();

		if ((actualDistance < distanceLimit) && (actualAngleDot > angleLimitDot))
		{
			return true;
		}
	}

	return false;
}

bool CInteractiveObjectEx::IsValidInteractionName( const char* interactionName ) const
{
	return (interactionName && (interactionName[0]!='\0'));
}

uint32 CInteractiveObjectEx::GetCrcForName( const char* name ) const
{
	assert(name);

	return gEnv->pSystem->GetCrc32Gen()->GetCRC32(name);
}

void CInteractiveObjectEx::Physicalize( pe_type physicsType )
{
	IPhysicalEntity* pPhysics = GetEntity()->GetPhysics();
	pe_type currentPhysicsType = pPhysics ? pPhysics->GetType() : PE_NONE;

	if (physicsType != currentPhysicsType)
	{
		SEntityPhysicalizeParams physicParams;
		physicParams.type = physicsType;
		physicParams.nSlot = 0;
		GetEntity()->Physicalize(physicParams);
	}
}