#include "StdAfx.h"
#include "AGAnimation.h"
#include "AnimationGraphState.h"
#include "AnimationRandomizer.h"

#define LAYER_NAMES LAYER_NAMES_AGANIM

static const char * LAYER_NAMES[] = {
	"AnimationLayer1",
	"AnimationLayer2",
	"AnimationLayer3",
	"AnimationLayer4",
	"AnimationLayer5",
	"AnimationLayer6",
	"AnimationLayer7",
	"AnimationLayer8",
	"AnimationLayer9",
};


void CAGAnimation::EnterState( SAnimationStateData& data, bool dueToRollback )
{
	ICharacterInstance* pICharacter = data.pEntity->GetCharacter(0);
	if (!pICharacter)
	{
		GameWarning("Entity '%s' of class '%s' has no character attached", data.pEntity->GetName(), data.pEntity->GetClass()->GetName());
		return;
	}
	IAnimationSet* pAnimSet = pICharacter->GetIAnimationSet();

	CAnimationPlayerProxy *animPlayerProxy = data.animationProxy;
	CRY_ASSERT_MESSAGE(animPlayerProxy != NULL, "No animation player proxy defined in the data");

	if (!animPlayerProxy)
		return;

	if(gEnv->pSystem->IsDedicated())
		return;

	if ( !gEnv->bClient )
	{
		data.hurried = false;
	}
	else if ( m_animations[0].empty() )
	{
		if ( m_layer > 0 )
		{
			data.hurried = false;
			const CAnimation* anim = data.animationProxy->GetTopAnimation(data.pEntity, m_layer);
			if (anim && anim->m_AnimParams.m_nFlags & CA_LOOP_ANIMATION)
			{
				animPlayerProxy->StopAnimationInLayer(data.pEntity, m_layer, 0.3f );
			}
		}
	}
	else
	{
		if (m_animRandomizer)
		{
			m_animRandomizer->EnterState();
		}

		string animation0;
		if (m_animRandomizer)
		{
			if (!m_animRandomizer->HasInitialDelay())
			{
				animation0 = m_animRandomizer->GetNextAssetName();
			}
		}
		else
		{
			animation0 = data.pState->ExpandVariationInputs( m_animations[0].c_str() );
		}
		string animation1 = data.pState->ExpandVariationInputs( m_animations[1].c_str() );
		string aimPose0 = data.pState->ExpandVariationInputs( m_aimPoses[0].c_str() );
		string aimPose1 = data.pState->ExpandVariationInputs( m_aimPoses[1].c_str() );

		m_animationId = pAnimSet->GetAnimIDByName(animation0.c_str());
		if (m_animationId < 0 && !animation0.empty())
			GameWarning("Entity %s:cannot find animation %s", data.pEntity->GetName(),animation0.c_str());
		if (!animation1.empty())
			if ((pAnimSet->GetAnimIDByName(animation1.c_str())) < 0) 
				GameWarning("Entity %s:cannot find animation %s", data.pEntity->GetName(),animation1.c_str());

		StartNodeAnimation(m_animationId, data, dueToRollback);
	}
}

CryCharAnimationParams CAGAnimation::GetAnimationParams( SAnimationStateData& data )
{
	CryCharAnimationParams params = data.params[m_layer];
	if ( data.overrideTransitionTime != 0.0f )
		params.m_fTransTime = data.overrideTransitionTime;

	if (data.overrides[m_layer].overrideStartAfter)
		params.m_nFlags |= CA_START_AFTER;
	if (data.overrides[m_layer].overrideStartAtKeyFrame)
	{
		params.m_nFlags |= CA_START_AT_KEYTIME;
		params.m_fKeyTime = data.overrides[m_layer].startAtKeyFrame;
	}

	// clear overrides after consuming
	data.overrides[m_layer].ClearOverrides();

	if (m_oneShot)
		params.m_nFlags &= ~CA_LOOP_ANIMATION;

	params.m_nLayerID = m_layer;
//	params.m_nCopyTimeFromLayer = -1;
	params.m_nUserToken = data.pState->GetCurrentToken();

	params.m_fPlaybackSpeed = data.pGameObject->GetChannelId() != 0 /*CCryAction::GetCryAction()->IsMultiplayer()*/
		? m_MPSpeedMultiplier : m_SPSpeedMultiplier;
	params.m_fUserData[eAGUD_AnimationControlledView] = data.animationControlledView? 1.0f : 0.0f;
	params.m_fUserData[eAGUD_MovementControlMethodH] = data.MovementControlMethodH;
	params.m_fUserData[eAGUD_MovementControlMethodV] = data.MovementControlMethodV;
	params.m_fUserData[eAGUD_AdditionalTurnMultiplier] = data.additionalTurnMultiplier;

	for (size_t i=eAGUD_NUM_BUILTINS; i<NUM_ANIMATION_USER_DATA_SLOTS; i++)
		params.m_fUserData[i] = data.userData[i];

//	params.nFlags |= 

	if (m_interruptCurrentAnim)
		params.m_nFlags &= ~(CA_START_AFTER | CA_START_AT_KEYTIME);

	if ( (m_layer == 0) && (data.canMix == false) )
		params.m_nFlags |= CA_DISABLE_MULTILAYER;

	if ( data.pState->IsUsingTriggeredTransition() || m_fullRootPriority )
		params.m_nFlags |= CA_FULL_ROOT_PRIORITY;

	return params;
}

EHasEnteredState CAGAnimation::HasEnteredState( SAnimationStateData& data )
{
	if (m_animations[0].empty())
	{
		if (m_layer!=0)
		{
			if (ICharacterInstance * pChar = data.pEntity->GetCharacter(0))
			{
				ISkeletonAnim* pSkel = pChar->GetISkeletonAnim();
				int nAnimsInFIFO = pSkel->GetNumAnimsInFIFO(m_layer);
				return nAnimsInFIFO ? eHES_Waiting : eHES_Entered;
			}
		}
		return eHES_Instant;
	}

	if (!gEnv->bClient)
		return eHES_Entered;

	const CAnimation* anim = data.animationProxy->GetAnimation(data.pEntity, m_layer, data.pState->GetCurrentToken());
	if (anim)
	{
		if (anim->m_bActivated || (data.pEntity->IsHidden() && data.pEntity->GetFlags()&ENTITY_FLAG_UPDATE_HIDDEN) || !(data.pEntity->GetSlotFlags(0)&ENTITY_SLOT_RENDER))
			return eHES_Entered;
		else
			return eHES_Waiting;      
	}

	return eHES_Instant;
}

bool CAGAnimation::CanLeaveState( SAnimationStateData& data )
{
	if (m_animations[0].empty())
		return true;

	bool found = false;
	bool activated = IsActivated( data, &found );

	if (found && activated && m_stayInStateUntil > 0.001f)
	{
		const CAnimation* anim = data.animationProxy->GetAnimation(data.pEntity, m_layer, data.pState->GetCurrentToken());
		if (anim)
		{
			if (anim->m_fAnimTime < m_forceInStateUntil)
				return false;
			if ((!data.queryChanged) && (anim->m_fAnimTime < m_stayInStateUntil))
				return false;
		}
	}

	if (found && m_ensureInStack && activated)
	{
		if (!IsTopOfStack(data))
			return false;
	}

	if (found)
		return activated;

	return true;
}

bool CAGAnimation::IsActivated(const SAnimationStateData& data, bool * pFound /* = NULL  */)
{
	bool activated = false;
	bool found = false;

	const CAnimation* anim = data.animationProxy->GetAnimation(data.pEntity, m_layer, data.pState->GetCurrentToken());
	if (anim)
	{
		found = true;

		// workaround: allow activation for hidden characters [MR/Craig]
		activated = anim->m_bActivated || (data.pEntity->IsHidden() && data.pEntity->GetFlags()&ENTITY_FLAG_UPDATE_HIDDEN) || !(data.pEntity->GetSlotFlags(0)&ENTITY_SLOT_RENDER);
	}

	if (pFound)
		*pFound = found;
	return activated;
}

void CAGAnimation::LeaveState( SAnimationStateData& data )
{
	if (m_animations[0].empty())
		return;

	const CAnimation* anim = data.animationProxy->GetAnimation(data.pEntity, m_layer, data.pState->GetCurrentToken());
	if (anim)
	{
		if (!anim->m_bActivated)
		{
			data.animationProxy->RemoveAnimationInLayer(data.pEntity, m_layer, data.pState->GetCurrentToken());
		}
		else if (anim->m_fAnimTime >= m_forceInStateUntil && anim->m_fAnimTime < m_stayInStateUntil)
			data.overrides[m_layer].ClearOverrides();
	}

	if (m_forceLeaveWhenFinished)
		data.pState->InvalidateQueriedState();
}

void CAGAnimation::DebugDraw( SAnimationStateData& data, IRenderer * pRenderer, int x, int& y, int yIncrement )
{
	float white[] = {1,1,1,1};

	const char* szAnimation0 = m_animations[0].c_str();
	ICharacterInstance* pCharacter = data.pEntity->GetCharacter(0);
	IAnimationSet* pAnimSet = pCharacter ? pCharacter->GetIAnimationSet() : NULL;
	if (pAnimSet && (m_animationId >= 0))
	{
		szAnimation0 = pAnimSet->GetNameByAnimID(m_animationId);
	}

	pRenderer->Draw2dLabel( (float)x,(float)y, 1.f, white, false, "anim: %s", szAnimation0 );
	y += yIncrement;
	if (m_ensureInStack)
	{
		pRenderer->Draw2dLabel( (float)x,(float)y, 1.f, white, false, "ensure top of stack: %d", IsTopOfStack(data) );
		y += yIncrement;
	}
	if (m_forceLeaveWhenFinished)
	{
		pRenderer->Draw2dLabel( (float)x,(float)y, 1.f, white, false, "force leave state when finished" );
		y += yIncrement;
	}
	if (m_forceInStateUntil < m_stayInStateUntil)
	{
		pRenderer->Draw2dLabel( (float)x,(float)y, 1.f, white, false, "stay in state until: %f (%f forced)", m_stayInStateUntil, m_forceInStateUntil );
		y += yIncrement;
	}
	else if (m_stayInStateUntil)
	{
		pRenderer->Draw2dLabel( (float)x,(float)y, 1.f, white, false, "stay in state until: %f", m_stayInStateUntil );
		y += yIncrement;
	}
	pRenderer->Draw2dLabel( (float)x,(float)y, 1.f, white, false, "is activated: %d", IsActivated(data) );
	y += yIncrement;
}

void CAGAnimation::Paused(SAnimationStateData& data, bool paused)
{
	if (!paused)
	{
		if (!IsAnimTopOfStack(m_animationId, data))
			StartNodeAnimation(m_animationId, data, false);

		data.animationProxy->SetAnimTime(data.pEntity, m_layer, data.pState->GetCurrentToken(), m_fAnimTimeWhenPaused);
	}
	else
	{
		const CAnimation* pAnim = data.animationProxy->GetAnimation(data.pEntity, m_layer, data.pState->GetCurrentToken());
		if (pAnim)
			m_fAnimTimeWhenPaused = pAnim->m_fAnimTime;
	}
}

void CAGAnimation::Update( SAnimationStateData& data )
{
	if (data.isPaused)
		return;

	if (m_forceLeaveWhenFinished && IsTopOfStack(data))
	{
		const CAnimation* anim = data.animationProxy->GetAnimation(data.pEntity, m_layer, data.pState->GetCurrentToken());
		if (anim)
		{
			const float waitUntilTime = 0.99f;
			if (anim->m_fAnimTime >= waitUntilTime)
			{
				data.pState->ForceLeaveCurrentState();
			}
		}
	}

	if (m_animRandomizer)
	{
		m_animRandomizer->Update();

		if (m_animRandomizer->IsItTimeForAnimationSwitch())
		{
			// start next one
			string animName = m_animRandomizer->GetNextAssetName();
			m_animRandomizer->SetAnimationParams(m_params);

			ICharacterInstance * pCharacter = data.pEntity->GetCharacter(0);
			if (pCharacter)
			{
				IAnimationSet* pIAnimationSet = pCharacter->GetIAnimationSet();
				m_animationId = pIAnimationSet->GetAnimIDByName(animName);

				StartNodeAnimation(m_animationId, data, false);
			}
		}
	}
}

bool CAGAnimation::IsTopOfStack( const SAnimationStateData& data )
{
	ICharacterInstance * pCharacter = data.pEntity->GetCharacter(0);
	if (!pCharacter)
		return true;
	IAnimationSet* pIAnimationSet = pCharacter->GetIAnimationSet();
	ISkeletonAnim* pSkeletonAnim = pCharacter->GetISkeletonAnim();

	int nAnimsCurrently = pSkeletonAnim->GetNumAnimsInFIFO(m_layer);
	switch (nAnimsCurrently)
	{
	case 0:
		// no animation playing, better get one
		return true;
	case 1:
		{
			// this is the last animation in the queue - must be on top of the stack
			return true;
			/*
			CAnimation& animation = pSkeletonAnim->GetAnimFromFIFO(m_layer,0);
			int32 id = (animation.m_LMG0.m_nLMGID >= 0) ? animation.m_LMG0.m_nLMGID : animation.m_LMG0.m_nAnimID[0];

			int32 id0 = pIAnimationSet->GetAnimIDByName( m_animations[0].c_str() );
			int32 id1 = pIAnimationSet->GetAnimIDByName( m_animations[1].c_str() );

			return (id == id0) || (id == id1);
			*/
		}
		break;
	default:
		// > 1 animation playing... wait
		return false;
	}
}

void CAGAnimation::GetCompletionTimes( SAnimationStateData& data, CTimeValue start, CTimeValue& hard, CTimeValue& sticky )
{
	hard = 0.0f;
	if (m_stickyOutTime > 0)
		sticky = start + m_animationLength - m_stickyOutTime;
	else
		sticky = 0.0f;
}

const IAnimationStateNodeFactory::Params * CAGAnimation::GetParameters()
{
	static const Params params[] = 
	{
		{true,  "string", "animation1",    "Animation 1",                                      "" },
		{true,  "string", "animation2",    "Animation 2",                                      "" },
		{true,  "string", "aimPose",    "AimPose 1",                                      "" },
		{true,  "string", "aimPose2",    "AimPose 2",                                      "" },
		{true,  "bool",   "ensureInStack", "Wait for animation to be playing?",                "0"},
		{true,  "float",  "stickyOutTime", "Sticky out time (or <0 for non-sticky playback)",  "-1"},
		{true,  "bool",   "forceEnableAiming", "Force enable aiming and usage of aimposes",  "0"},
		{true,  "bool",   "forceLeaveWhenFinished", "Force leaving this state when finished",  "0"},
		{true,  "float",  "speedMultiplier", "Speed multiplier (increases playback speed on this animation)", "1"},
		{true,  "float",  "MPSpeedMultiplier", "Multiplayer speed multiplier (increases playback speed on this animation in multiplayer)", "1"},
		{true,  "bool",   "stopCurrentAnimation", "Stop the current animation?",                "0"},
		{true,  "bool",   "interruptCurrAnim", "Interrupt current animation?",                "0"},
		{true,	"bool",   "noAim", "If true, aim IK has no influence on the character when this state is active", "0"},
		{true,  "bool",   "fullRootPriority", "If true, set CA_FULL_ROOT_PRIORITY animation flag", "0"},
		{0}
	};
	return params;
}

bool CAGAnimation::Init( const XmlNodeRef& node, IAnimationGraphPtr pGraph )
{
	m_animRandomizer = NULL;

	bool ok = true;
	ok &= node->getAttr("ensureInStack", m_ensureInStack);
	ok &= node->getAttr("stickyOutTime", m_stickyOutTime);
	if (!node->getAttr("forceEnableAiming", m_forceEnableAiming))
		m_forceEnableAiming = false;
	ok &= node->getAttr("forceLeaveWhenFinished", m_forceLeaveWhenFinished);

	m_animations[0] = pGraph->RegisterVariationInputs( node->getAttr("animation1") );
	m_animations[1] = pGraph->RegisterVariationInputs( node->getAttr("animation2") );
	m_aimPoses[0] = pGraph->RegisterVariationInputs( node->getAttr("aimPose") );
	m_aimPoses[1] = pGraph->RegisterVariationInputs( node->getAttr("aimPose2") );
	if (m_aimPoses[0] == "none")
		m_aimPoses[0] = "";
	if (m_aimPoses[1] == "none")
		m_aimPoses[1] = "";

	m_SPSpeedMultiplier = 1.0f;
	if (node->getAttr("speedMultiplier")[0])
		node->getAttr("speedMultiplier", m_SPSpeedMultiplier);
	m_MPSpeedMultiplier = m_SPSpeedMultiplier;
	if (node->getAttr("MPSpeedMultiplier")[0])
		node->getAttr("MPSpeedMultiplier", m_MPSpeedMultiplier);
	m_stayInStateUntil = 0;
	node->getAttr("stayInStateUntil", m_stayInStateUntil);
	m_forceInStateUntil = m_stayInStateUntil;
	if (node->getAttr("forceInStateUntil")[0])
	{
		node->getAttr("forceInStateUntil", m_forceInStateUntil);
		if ( m_forceInStateUntil > m_stayInStateUntil )
			m_stayInStateUntil = m_forceInStateUntil;
	}

	m_stopCurrentAnimation = false;
	node->getAttr("stopCurrentAnimation", m_stopCurrentAnimation);

	m_interruptCurrentAnim = false;
	node->getAttr("interruptCurrAnim", m_interruptCurrentAnim);

	m_directToLayer0 = false;
	node->getAttr("directToLayer0", m_directToLayer0);

	if (m_forceLeaveWhenFinished)
		this->flags |= eASNF_Update;

	m_oneShot = false;
	node->getAttr("oneShot", m_oneShot);

	m_noAim = false;
	node->getAttr("noAim", m_noAim);

	m_animationLength = 0.0f;

	m_fullRootPriority = false;
	node->getAttr("fullRootPriority", m_fullRootPriority);

	return ok;
}

SAnimationDesc CAGAnimation::GetDesc( IEntity * pEntity, const CAnimationGraphState* pState )
{
	CRY_ASSERT(pEntity);
	SAnimationDesc desc;

	ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
	if (pCharacter == NULL)
		return desc;

	IAnimationSet* pAnimSet = pCharacter->GetIAnimationSet();
	if (pAnimSet == NULL)
		return desc;

	string animation0;
	if (m_animationId < 0)
	{
		animation0 = pState == NULL ? m_animations[0].c_str() : pState->ExpandVariationInputs( m_animations[0].c_str() ).c_str();
		m_animationId = pAnimSet->GetAnimIDByName(animation0.c_str());
		if (m_animationId < 0)
			return desc;
	}
	else
	{
		animation0 = pAnimSet->GetNameByAnimID(m_animationId);
	}

	uint32 flags = pAnimSet->GetAnimationFlags(m_animationId);
	if ((flags & CA_ASSET_CREATED) == 0)
		return desc;

	CryAnimationPath path = pAnimSet->GetAnimationPath(animation0.c_str());
	const QuatT& a = path.m_key0;
	const QuatT& b = path.m_key1;
	desc.movement.translation = b.t - a.t;
	desc.movement.rotation = a.q.GetInverted() * b.q;
	desc.movement.duration = pAnimSet->GetDuration_sec( m_animationId );
	desc.startLocation = pAnimSet->GetAnimationStartLocation(animation0.c_str());
	desc.properties = pAnimSet->GetAnimationSelectionProperties(animation0.c_str());
	desc.initialized = pState != NULL && animation0 == m_animations[0];
	return desc;
}

Vec2 CAGAnimation::GetMinMaxSpeed( IEntity * pEntity )
{
	if (ICharacterInstance * pInst = pEntity->GetCharacter(0))
	{
		int id = pInst->GetIAnimationSet()->GetAnimIDByName(m_animations[0].c_str());
		if (id >= 0)
		{
			return pInst->GetIAnimationSet()->GetMinMaxSpeedAsset_msec(id);
		}
	}
	return Vec2(0,0);
}

void CAGAnimation::Release()
{
	delete this;
}

IAnimationStateNode * CAGAnimation::Create()
{
	return this;
}

const char * CAGAnimation::GetCategory()
{
	return LAYER_NAMES[m_layer];
}

const char * CAGAnimation::GetName()
{
	return LAYER_NAMES[m_layer];
}

void CAGAnimation::SerializeAsFile(bool reading, AG_FILE *file)
{
	SerializeAsFile_NodeBase(reading, file);

	FileSerializationHelper h(reading, file);

	h.Value(&m_layer);
	h.Value(&m_ensureInStack);
	h.Value(&m_forceEnableAiming);
	h.Value(&m_forceLeaveWhenFinished);
	h.Value(&m_stopCurrentAnimation);
	h.Value(&m_interruptCurrentAnim);
	h.Value(&m_noAim);
	h.Value(&m_fullRootPriority);
	h.Value(&m_directToLayer0);
	h.Value(&m_stickyOutTime);
	h.Value(m_animations, 2);
	h.Value(m_aimPoses, 2);
	h.Value(&m_SPSpeedMultiplier);
	h.Value(&m_MPSpeedMultiplier);
	h.Value(&m_stayInStateUntil);
	h.Value(&m_forceInStateUntil);
	h.Value(&m_oneShot);

	if (reading)
	{
		string animName = m_animations[0].c_str();
		if (animName.find(".xml", 0) != string::npos)
		{
			const char* graphPath = "Animations/graphs/";
			XmlNodeRef node = GetISystem()->LoadXmlFile( (graphPath + animName).c_str() );
			if (!node)
			   return;

			// Create Asset Randomizer and give him the child node of this one
			m_animRandomizer = new CAnimationRandomizer();

			// TODO: instead of loading an xml file, load the data from the graph file directly in Init and convert to AG
			// --> this needs to be done after the editor interface has been created
			//ok &= m_animRandomizer->Load(node->findChild("RandomizerSetup"));

			m_animRandomizer->Load(node);

		}

	}
}

CAGAnimation::~CAGAnimation()
{
	SAFE_DELETE(m_animRandomizer);
}

bool CAGAnimation::StartNodeAnimation(int id, SAnimationStateData& data, bool dueToRollback)
{
	if (id < 0)
		return false;

	ICharacterInstance* pICharacter = data.pEntity->GetCharacter(0);
	if (!pICharacter)
	{
		GameWarning("Entity '%s' of class '%s' has no character attached", data.pEntity->GetName(), data.pEntity->GetClass()->GetName());
		return false;
	}

	CAnimationPlayerProxy *animPlayerProxy = data.animationProxy;
	CRY_ASSERT_MESSAGE(animPlayerProxy != NULL, "No animation player proxy defined in the data");
	if (!animPlayerProxy)
	{
		return false;
	}

	IAnimationSet* pAnimSet = pICharacter->GetIAnimationSet();

	m_params = GetAnimationParams(data);
	if (m_stickyOutTime > 0.0f)
	{
		//--- TSB: Only calculate the animation length if we're going to need it.
		m_animationLength = fabsf( pAnimSet->GetDuration_sec(id) / m_params.m_fPlaybackSpeed );
	}

	CryCharAnimationParams backParams;
	const CAnimation* anim = data.animationProxy->GetTopAnimation(data.pEntity, m_layer);
	if (anim)
	{
		backParams = anim->m_AnimParams;
		if (backParams.m_nFlags & CA_LOOP_ANIMATION)
		{
			if (m_params.m_nFlags & CA_START_AFTER)
			{
				//GameWarning("Trying to start an animation after a looping animation; ignoring START_AFTER flag");
				m_params.m_nFlags &= ~CA_START_AFTER;
			}
		}
	}

	if (dueToRollback)
	{
		if (m_params.m_nFlags & CA_REPEAT_LAST_KEY)
			return true;

		// this animation has already started fading out
		// we need to add it to the queue again (modified flags should help)
		m_params.m_nFlags |= CA_ALLOW_ANIM_RESTART;
		m_params.m_nFlags |= CA_TRANSITION_TIMEWARPING;
		m_params.m_nFlags &= ~CA_START_AFTER;
		m_params.m_nFlags &= ~CA_START_AT_KEYTIME;
		m_params.m_fTransTime = 0.01f;
	}

	if (data.hurried)
	{
		data.hurried = false;
		m_params.m_nFlags &= ~(CA_START_AT_KEYTIME | CA_START_AFTER);
		m_params.m_fTransTime = 0.2f;
	}

	if (m_stopCurrentAnimation)
		animPlayerProxy->StopAnimationInLayer(data.pEntity, m_params.m_nLayerID, 0.0f );

	if (m_noAim)
	{
		const int inputId = data.pState->GetInputId("FirstPerson");
		if (inputId != -1)
		{
			if (data.pState->GetInputAsFloat(inputId) == 1.0f)
			{
				ISkeletonPose* pISkeletonPose = pICharacter->GetISkeletonPose();
				pISkeletonPose->SetAimIK(false, Vec3(ZERO), 3, 0.0f);
			}
		}
	}

	bool result = false;

	if (m_animRandomizer && m_animRandomizer->GetAnimationSwitchingEnabled() && m_params.m_nFlags & CA_LOOP_ANIMATION)
		this->flags |= eASNF_Update;

	result = animPlayerProxy->StartAnimationById(
		data.pEntity, 
		id, 
		m_params );

	if ( result )
	{
		const CAnimation* anim = data.animationProxy->GetAnimation(data.pEntity, m_layer, m_params.m_nUserToken);
		CRY_ASSERT( anim != NULL );
	}
	else
	{
		backParams.m_nUserToken = m_params.m_nUserToken;
	}

	return result;
}

bool CAGAnimation::IsAnimTopOfStack(int id, const SAnimationStateData& data) const
{
	ICharacterInstance* pCharacter = data.pEntity->GetCharacter(0);
	ISkeletonAnim* pSkeletonAnim = pCharacter ? pCharacter->GetISkeletonAnim() : NULL;

	bool bIsAnimTopOfStack = false;

	if (pSkeletonAnim)
	{
		const int iNumAnims = pSkeletonAnim->GetNumAnimsInFIFO(m_layer);
		if (iNumAnims > 0)
		{
			const CAnimation& anim = pSkeletonAnim->GetAnimFromFIFO(m_layer, iNumAnims - 1);

			int32 topAnimId = (anim.m_Parametric.m_nParametricID >= 0) ? anim.m_Parametric.m_nParametricID : anim.m_Parametric.m_nAnimID[0];
			bIsAnimTopOfStack = (anim.m_AnimParams.m_nUserToken == m_params.m_nUserToken) && (id == topAnimId);
		}
	}

	return bIsAnimTopOfStack;
}

#undef LAYER_NAMES
