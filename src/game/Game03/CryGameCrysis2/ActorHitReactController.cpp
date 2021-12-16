
#include "StdAfx.h"
#include "ActorHitReactController.h"
#include "Actor.h"
#include "GameRules.h"
#include "StringUtils.h"
#include "GameRulesModules/IGameRulesDamageHandlingModule.h"

ActorHitReactController::HitReactType::HitReactType()
:
	m_hitTypeMask(0),
	m_hitLimbMask(0xffffffff),
	m_flags(0),
	m_minSpeed(0.0f),
	m_numVariants(0)
{
	m_type[0] = '\0';
}

ActorHitReactController::HitReactType::HitReactType(const char *type)
:
	m_hitTypeMask(0),
	m_hitLimbMask(0xffffffff),
	m_flags(0),
	m_minSpeed(0.0f),
	m_numVariants(0)
{
	SetType(type);
}

void ActorHitReactController::HitReactType::SetType(const char *type)
{
	int typeLen = strlen(type)+1;
	CRY_ASSERT_MESSAGE(strlen(type) < MAX_LEN_HR_TYPE, string().Format("HitReact type %s too long , limit is %d chars! Truncating type.", type, MAX_LEN_HR_TYPE));
	strncpy(m_type, type, min(typeLen, MAX_LEN_HR_TYPE));
}

ActorHitReactController::ActorHitReactController()
:
	m_initialised(false)
{
}

ActorHitReactController::~ActorHitReactController()
{
}

#define PARSE_FLAG(flags, buff, x)		if (CryStringUtils::stristr(buff, #x)) \
																			{	flags |= HitReactType::x;	}

void ActorHitReactController::Initialise(CActor *actor)
{
	if (!gEnv->bMultiplayer)
		return;

	const IItemParamsNode *actorParams		= g_pGame->GetIGameFramework()->GetIActorSystem()->GetActorParams(actor->GetActorClass());
	const IItemParamsNode *hitReactParams = actorParams ? actorParams->GetChild("HitReactParams") : NULL;
	const IItemParamsNode *deathParams		= actorParams ? actorParams->GetChild("DeathParams") : NULL;

	ICharacterInstance * pCharacterInstance = actor->GetEntity()->GetCharacter(0);
	assert(pCharacterInstance);

	ISkeletonPose * pSkeletonPose = pCharacterInstance->GetISkeletonPose();
	assert(pSkeletonPose);

	IGameRulesDamageHandlingModule *damHanModule = g_pGame->GetGameRules()->GetDamageHandlingModule();

	if (deathParams)
	{
		int numChildren = deathParams->GetChildCount();

		m_options.resize(numChildren);
		for (int i=0; i<numChildren; i++)
		{
			const IItemParamsNode *child = deathParams->GetChild(i);
			const char *type			= child->GetAttribute("type");
			const char *flags			= child->GetAttribute("flags");

			HitReactType &hitReactType = m_options[i];
			hitReactType.SetType(type);
			child->GetAttribute("variants", hitReactType.m_numVariants);

			if (flags)
			{
				PARSE_FLAG(hitReactType.m_flags, flags, OrientateToImpact);
				PARSE_FLAG(hitReactType.m_flags, flags, RagdollOnCollision);
				PARSE_FLAG(hitReactType.m_flags, flags, TriggerRagdoll);
			}
			CRY_ASSERT_MESSAGE((hitReactType.m_flags & (HitReactType::TriggerRagdoll|HitReactType::RagdollOnCollision)) != (HitReactType::TriggerRagdoll|HitReactType::RagdollOnCollision), string().Format("Cannot have trigger ragdoll & ragdoll on collision on the same death anim! %s", type));

			child->GetAttribute("moveSpeed", hitReactType.m_minSpeed);

			int numClauses = child->GetChildCount();
			for (int clause=0; clause<numClauses; clause++)
			{
				const char *childName = child->GetChildName(clause);
				CRY_ASSERT_MESSAGE(stricmp(childName, "filter") == 0, string().Format("Invalid parameter %s to HitReactParams", childName));
				const IItemParamsNode *childAtt = child->GetChild(clause);

				const char *hittype	= childAtt->GetAttribute("hittype");
				if (hittype)
				{
					if (strcmp(hittype, "all") == 0)
					{
						hitReactType.m_hitTypeMask = 0xffffffff;
					}
					else
					{
						int hitTypeID = g_pGame->GetGameRules()->GetHitTypeId(hittype);
						if (hitTypeID == 0)
						{
							CRY_ASSERT_MESSAGE((hitTypeID == 0), string().Format("Invalid hittype defined %s", hittype));
							continue;
						}
						hitReactType.m_hitTypeMask |= (1<<hitTypeID);
					}
				}

				const char *hitlimb	= childAtt->GetAttribute("hitlimb");
				if (hitlimb && damHanModule)
				{
					CRY_TODO(12, 2, 2010, "Disabled joint filtering for now, this whole class is going very SOON");
#if 0
					int8 jointId = damHanModule->GetPartIdFromName(hitlimb);
					CRY_ASSERT_MESSAGE((jointId >= 0), string().Format("Invalid hitLimb defined %s", hitlimb));
					if (jointId >= 0)
					{
						if (hitReactType.m_hitLimbMask == 0xffffffff)
						{
							hitReactType.m_hitLimbMask = 0;
						}

						hitReactType.m_hitLimbMask |= (1<<jointId);
					}
#endif //0
				}
			}
		}		
	}

	m_initialised = true;
}

void ActorHitReactController::OnKill(CActor *actor, const HitInfo* hitInfo)
{
	CRY_ASSERT_MESSAGE(m_initialised, "[ActorHitReactController::OnKill] Controller not initialised!");

	CRY_ASSERT_MESSAGE(hitInfo->type<32,	 "ActorHitReactController: Overrunning hit type mask!");
	CRY_ASSERT_MESSAGE((hitInfo->partId<32) || (hitInfo->partId == 0xffff), "ActorHitReactController: Overrunning hit part ID mask!");

	int hitTypeFlag = (1<<hitInfo->type);
	int hitLimbFlag = (hitInfo->partId == 0xffff) ? 0 : (1<<hitInfo->partId);
	const float speed = actor->GetActorStats()->velocity.GetLength();

	HitReactType *bestHitReactType = NULL;
	const uint32 numOptions = m_options.size();
	for (uint32 i=0; i<numOptions; i++)
	{
		if ((hitTypeFlag & m_options[i].m_hitTypeMask) 
			&& ((m_options[i].m_hitLimbMask == 0xffffffff) || (hitLimbFlag & m_options[i].m_hitLimbMask))
			&& (speed >= m_options[i].m_minSpeed))
		{
			bestHitReactType = &m_options[i];
			break;
		}
	}

	if (bestHitReactType)
	{
		if (bestHitReactType->m_flags & HitReactType::TriggerRagdoll)
		{
			if (gEnv->bServer)
			{
				actor->SetAspectProfile(eEA_Physics, eAP_Ragdoll);
				CryLog("ActorHitReactController::OnKill: %s Setting ragdoll", actor->GetEntity()->GetName());
			}
		}
		else
		{
			actor->GetAnimationGraphState()->SetInput("Action", "death");
			actor->GetAnimationGraphState()->SetVariationInput("HitType", bestHitReactType->m_type);
			CryFixedStringT<3> variantStr;
			int variant = (cry_rand()%bestHitReactType->m_numVariants) + 1;
			variantStr.Format("%02d", variant);
			actor->GetAnimationGraphState()->SetVariationInput("variation", variantStr.c_str());

			if (gEnv->bServer && (bestHitReactType->m_flags & HitReactType::RagdollOnCollision))
			{
				actor->SetRagdollOnCollision();
			}
		}

		if (bestHitReactType->m_flags & HitReactType::OrientateToImpact)
		{
			Ang3 angFacing;
			angFacing.Set(0.0f, 0.0f, cry_atan2f(hitInfo->dir.x, -hitInfo->dir.y));
			actor->SetAngles(angFacing);

			IEntity *pEntity = actor->GetEntity();

			Matrix34 tm = Matrix33( Quat::CreateRotationXYZ( angFacing ) );
			tm.SetTranslation( pEntity->GetWorldPos() );
			pEntity->SetWorldTM(tm);		
		}
	}

	//--- Send a death event out to let FlowGraph know, MP is bypassing the usual system
	SEntityEvent event( ENTITY_EVENT_SCRIPT_EVENT );
	event.nParam[0] = (INT_PTR)"Dead";
	event.nParam[1] = IEntityClass::EVT_BOOL;
	bool bValue = true;
	event.nParam[2] = (INT_PTR)&bValue;
	actor->GetEntity()->SendEvent( event );
}
