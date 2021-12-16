#include "StdAfx.h"
#include "AGAdditive.h"
#include "AnimationGraphState.h"
#include "AnimationRandomizer.h"

CAGAdditive::CAGAdditive()
: m_animRandomizer(NULL)
{
}

CAGAdditive::~CAGAdditive()
{
	SAFE_DELETE(m_animRandomizer);
}

const IAnimationStateNodeFactory::Params * CAGAdditive::GetParameters()
{
	static const Params params[] = 
	{
		{true, "int",			"playMode",					"playMode",						"0"},
		{false, "bool",		"fadeOutOneLeaving","fadeOutOneLeaving",	"1"},
		{true, "int",			"layer",						"layer",							"4"},
		{false, "float",	"fadeOutTime",      "fadeOutTime"         "1.5"},
		{false, "float",	"blendInTime",      "blendInTime"         "1.2"},
		{false, "float",	"assetWeight",			"assetWeight" ,				"1.0"},
		{false, "float",	"assetSpeed",				"assetSpeed" ,				"1.0"},
		{false, "string", "assetName",				"assetName",					""},
		{0}
	};
	return params;
}

void CAGAdditive::EnteredState( SAnimationStateData& data )
{
}

void CAGAdditive::LeftState( SAnimationStateData& data, bool wasEntered )
{
	// Fade out if that is desired (and if no new animation has already been started in this layer)
	if (!m_fadeOutOneLeaving)
	{
		return;
	}

	ICharacterInstance* pICharacter = data.pEntity->GetCharacter(0);
	if (!pICharacter)
	{
		GameWarning("Entity '%s' of class '%s' has no character attached", data.pEntity->GetName(), data.pEntity->GetClass()->GetName());
		return;
	}
	ISkeletonAnim* pISkeletonAnim = pICharacter->GetISkeletonAnim();
	CRY_ASSERT(pISkeletonAnim);

	// Are there any animations in the layer at all (might have already finished if it was a one-shot)
	int nAnimsInFIFO = pISkeletonAnim->GetNumAnimsInFIFO( m_layer );
	if ( nAnimsInFIFO > 0 )
	{
		// Check whether this is the animation that was started by this modifier
		CAnimation& anim = pISkeletonAnim->GetAnimFromFIFO( m_layer, nAnimsInFIFO-1 );
		if (anim.m_AnimParams.m_nUserToken == m_animToken)
		{
			// no need to fade out the animation if it is not the last in the queue
			// This way it will not be messing with the transition times of the next animation
			pISkeletonAnim->StopAnimationInLayer( m_layer, m_fadeOutTime );
		}
	}
}

void CAGAdditive::EnterState( SAnimationStateData& data, bool dueToRollback )
{
	ICharacterInstance* pICharacter = data.pEntity->GetCharacter(0);
	if (!pICharacter)
	{
		GameWarning("Entity '%s' of class '%s' has no character attached", data.pEntity->GetName(), data.pEntity->GetClass()->GetName());
		return;
	}
	ISkeletonAnim* pISkeletonAnim = pICharacter->GetISkeletonAnim();
	IAnimationSet* pAnimSet = pICharacter->GetIAnimationSet();
	CRY_ASSERT(pISkeletonAnim);
	CRY_ASSERT(pAnimSet);

	if(gEnv->pSystem->IsDedicated())
		return;

	if (m_playMode == STOP_LAYER)
	{
		// stop last asset in that layer (all others are already fading out)
		int nAnimsInFIFO = pISkeletonAnim->GetNumAnimsInFIFO( m_layer );
		if ( nAnimsInFIFO > 0 )
		{
			pISkeletonAnim->StopAnimationInLayer( m_layer, m_fadeOutTime );
		}
	}
	else
	{
		if (m_animRandomizer)
		{
			m_animRandomizer->EnterState();
		}

		// start the asset either once or looping
		string animationName;
		if (m_animRandomizer)
		{
			if (!m_animRandomizer->HasInitialDelay())
			{
				animationName = m_animRandomizer->GetNextAssetName();
			}
		}
		else
		{
			animationName = data.pState->ExpandVariationInputs( m_assetName );
		}

		int animId = pAnimSet->GetAnimIDByName(animationName.c_str());
		if (animId < 0)
		{
			animId = pAnimSet->GetAnimIDByName("additive_null");
			if (animId < 0)
			{
				if (!animationName.empty())
					GameWarning("Entity %s:cannot find animation %s", data.pEntity->GetName(), animationName.c_str());

				// still stop the asset in the layer
				if ( pISkeletonAnim->GetNumAnimsInFIFO( m_layer ) > 0 )
				{
					pISkeletonAnim->StopAnimationInLayer( m_layer, m_blendInTime );
				}

				bool loopingRandomizer = (m_animRandomizer && m_animRandomizer->GetAnimationSwitchingEnabled() && (m_playMode == LOOP_ASSET));
				if (!loopingRandomizer)
					return;
			}
		}

		m_animToken = cry_rand32();
		CryCharAnimationParams animParams;
		animParams.m_fLayerWeight = 1.0f;  // this is not where the additive weight is set (see further down)
		animParams.m_nLayerID = m_layer;
		animParams.m_fTransTime = m_blendInTime;
		animParams.m_nUserToken = m_animToken;
		animParams.m_fPlaybackSpeed = m_assetSpeed;
		animParams.m_nFlags |= CA_ALLOW_ANIM_RESTART;
		if (m_playMode == LOOP_ASSET)
		{
			animParams.m_nFlags |= CA_LOOP_ANIMATION;

			// is this asset randomized and should alternate from time to time?
			if (m_animRandomizer && m_animRandomizer->GetAnimationSwitchingEnabled())
			{
				m_params = animParams;
				this->flags |= eASNF_Update;
			}
		}

		pISkeletonAnim->StartAnimationById( animId, animParams );
		//data.animationProxy->StartAnimationById( data.pEntity, animId, animParams );
		pISkeletonAnim->SetAdditiveWeight(m_layer, m_assetWeight);
	}
}

void CAGAdditive::LeaveState( SAnimationStateData& data )
{
}

void CAGAdditive::DebugDraw( SAnimationStateData& data, IRenderer * pRenderer, int x, int& y, int yIncrement )
{
	/*
	float white[] = {1,1,1,1};
	if (!m_onEnter.empty())
	{
	pRenderer->Draw2dLabel( (float)x, (float)y, 1.f, white, false, "onEnter: %s", m_onEnter.c_str() );
	y += yIncrement;
	}
	if (!m_onExit.empty())
	{
	pRenderer->Draw2dLabel( (float)x, (float)y, 1.f, white, false, "onExit: %s", m_onExit.c_str() );
	y += yIncrement;
	}
	*/
}

bool CAGAdditive::CanLeaveState( SAnimationStateData& data )
{
	return true;
}

bool CAGAdditive::Init( const XmlNodeRef& node, IAnimationGraphPtr pGraph )
{
	m_playMode = PLAY_ONCE;
	m_fadeOutOneLeaving = true;
	m_layer = 4;
	m_fadeOutTime = 1.5f;
	m_blendInTime = 1.2f;
	m_assetWeight = 1.0f;
	m_assetSpeed = 1.0f;
	m_assetName.clear();
	m_animToken = 0;

	node->getAttr("playMode", m_playMode);
	node->getAttr("fadeOutOneLeaving", m_fadeOutOneLeaving);
	node->getAttr("layer", m_layer);
	node->getAttr("fadeOutTime", m_fadeOutTime);
	node->getAttr("blendInTime", m_blendInTime);
	node->getAttr("assetWeight", m_assetWeight);
	node->getAttr("assetSpeed", m_assetSpeed);
	m_assetName = pGraph->RegisterVariationInputs( node->getAttr("assetName") );


	// Sanity check all parameters and return false if they just don't make sense
	if (m_playMode != STOP_LAYER && m_assetName.empty())
	{
		CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "AnimationGraph: Additive node could not initialize, Asset Name String is empty.\nThe Editor does not export empty asset strings - please do not manually edit the animation graph xml file.");
		return false;
	}

	return true;
}

void CAGAdditive::Release()
{
	delete this;
}

IAnimationStateNode * CAGAdditive::Create()
{
	return this;
}

const char * CAGAdditive::GetCategory()
{
	return "Additive";
}

const char * CAGAdditive::GetName()
{
	return "Additive";
}

void CAGAdditive::SerializeAsFile( bool reading, AG_FILE *file )
{
	SerializeAsFile_NodeBase(reading, file);
	FileSerializationHelper h(reading, file);
	h.Value(&m_playMode);
	h.Value(&m_fadeOutOneLeaving);
	h.Value(&m_layer);
	h.Value(&m_fadeOutTime);
	h.Value(&m_blendInTime);
	h.Value(&m_assetWeight);
	h.Value(&m_assetSpeed);
	//h.Value(&m_animToken);
	CCryName temp(m_assetName);
	h.Value(&temp);
	m_assetName = temp.c_str();

	if (reading)
	{
		string animName = m_assetName;
		if (animName.find(".xml", 0) != string::npos)
		{
			const char* graphPath = "Animations\\graphs\\";
			string filename = m_assetName;
			XmlNodeRef node = GetISystem()->LoadXmlFile( (graphPath + filename).c_str() );
			if (!node)
			{
				CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "AnimationGraph: Additive node could not load randomizer file %s.", (graphPath + filename).c_str());
				return;
			}

			// Create Asset Randomizer and give him the child node of this one
			m_animRandomizer = new CAnimationRandomizer();

			// TODO: instead of loading an xml file, load the data from the graph file directly in Init and convert to AG
			// --> this needs to be done after the editor interface has been created

			m_animRandomizer->Load(node);
		}

	}
}

void CAGAdditive::Update( SAnimationStateData& data )
{
	bool printDebug = (CAnimationGraphCVars::Get().m_debugAdditives != 0);

	if (printDebug)
		CryLogAlways("AGAdditive: Update entered, this pointer: %X, randomizer pointer: %X", (UINT_PTR)this, (UINT_PTR)m_animRandomizer);

	if (m_animRandomizer)
	{
		m_animRandomizer->Update();

		if (m_animRandomizer->IsItTimeForAnimationSwitch())
		{
			if (printDebug)
				CryLogAlways("AGAdditive: New Asset needed.");

			// start next one
			string animName = m_animRandomizer->GetNextAssetName();
			if (printDebug)
				CryLogAlways("AGAdditive: Next Asset Name: %s", animName.c_str());
			m_animRandomizer->SetAnimationParams(m_params);

			ICharacterInstance* pICharacter = data.pEntity->GetCharacter(0);
			ISkeletonAnim* pISkeletonAnim = pICharacter->GetISkeletonAnim();
			IAnimationSet* pAnimSet = pICharacter->GetIAnimationSet();
			if (printDebug)
				CryLogAlways("AGAdditive: All interfaces retrieved.");

			int id = pAnimSet->GetAnimIDByName(animName);
			if (id < 0)
			{
				if (printDebug)
					CryLogAlways("AGAdditive: Asset not found, replacing with additive_null");
				id = pAnimSet->GetAnimIDByName("additive_null");

				if (id < 0)
				{
					if (printDebug)
						CryLogAlways("AGAdditive: additive_null not found, leaving Update early.");
					return;
				}
			}

			bool result = pISkeletonAnim->StartAnimationById( id, m_params );
			//bool result = data.animationProxy->StartAnimationById( data.pEntity, id, m_params );
			if (printDebug)
				CryLogAlways("AGAdditive: StartAnimation Result: 0x%08x", result);

			if ( result )
			{
				const CAnimation* anim = data.animationProxy->GetAnimation(data.pEntity, m_layer, m_params.m_nUserToken);
				CRY_ASSERT( anim != NULL );
			}
		}
	}

	if (printDebug)
		CryLogAlways("AGAdditive: Update left.");
}