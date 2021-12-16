
#include "StdAfx.h"
#include "AnimationRandomizer.h"



CAnimationRandomizer::CAnimationRandomizer() 
: m_switchAssetsOverTime(false)
, m_timeAlignBetweenAssets(false)
, m_lastAnimId(-1)
, m_probabilitySum(0.0f)
, m_elapsedTimeFromEnterState(0.0f)
, m_initialDelayApplied(false)
, m_initialDelay(0.0f)
{
}

bool CAnimationRandomizer::Load( XmlNodeRef node )
{
	// Safety checking
	if (strcmp(node->getTag(), "RandomizerSetup") != 0)
	{
		CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "AnimationGraph: XML node is not an AnimationRadnomizer node. Cannot load data. Aborting.");
		return false;
	}

	// Read in general data
	XmlNodeRef childNode = node->findChild("AssetSwitching");
	if (childNode)
	{
		childNode->getAttr("switchAssetsOverTime", m_switchAssetsOverTime);
		childNode->getAttr("switchProbabilitySeconds", m_switchProbabilitySeconds);
		childNode->getAttr("initialDelaySeconds", m_initialDelay);
	}
	childNode = node->findChild("TransitionOptions");
	if (childNode)
	{
		childNode->getAttr("timeAlignAssets", m_timeAlignBetweenAssets);
	}

	// Read in Animation Assets
	XmlNodeRef assetNode = node->findChild("Assets");
	if (!assetNode)
	{
		CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "AnimationGraph: Randomizer XML Node has no 'Assets' child node. Aborting.");
		return false;
	}

	SAnimation newAnim;
	int numChilds = assetNode->getChildCount();
	for (int i = 0; i < numChilds; ++i)
	{
		XmlNodeRef child = assetNode->getChild(i);
		if (strcmp(child->getTag(), "Animation") != 0)
			continue;

		newAnim.animName = child->getAttr("name");
		child->getAttr("probability", newAnim.probability);
		child->getAttr("canBeStartedTwiceInARow", newAnim.canBeStartedTwiceInARow);
		child->getAttr("looping", newAnim.looping);
		child->getAttr("transitionTime", newAnim.transitionTime);
		m_probabilitySum += newAnim.probability;
		m_animations.push_back(newAnim);
	}

	// sanity checking, to avoid unnecessary updates and calculations
	// no need for asset switching if there is only one (or none)
	if (m_animations.size() < 2)
		m_switchAssetsOverTime = false;

	if (m_animations.empty())
	{
		CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "AnimationGraph: Randomizer found no assets in xml node. Aborting.");
		return false;
	}

	return true;
}

string CAnimationRandomizer::GetNextAssetName()
{
	int animCount = m_animations.size();
	CRY_ASSERT(animCount);

	if (animCount == 1)
	{
		return m_animations[0].animName;
	}

	int nextAssetId = -1;
	bool valid;
	do 
	{
		valid = true;
		float randomNumber = (cry_rand() / (float)RAND_MAX) * m_probabilitySum;

		for (int i = 0; i < animCount; ++i)	
		{
			float assetProb = m_animations[i].probability;
			if (randomNumber <= assetProb)
			{
				// maybe this animation was already selected as the last one and should not be started twice in a row
				if (m_animations[i].canBeStartedTwiceInARow == false && m_lastAnimId == i)
				{
					valid = false;
					break;
				}

				nextAssetId = i;
				break;
			}

			randomNumber -= assetProb;
		}
	} while (!valid);

	CRY_ASSERT(nextAssetId >= 0);
	if (nextAssetId >= 0)
	{
		m_lastAnimId = nextAssetId;
		return m_animations[nextAssetId].animName;
	}
	else
	{
		CryLog("CAnimationRandomizer::GetNextAssetName invalid nextAssetId {%d}", nextAssetId);
		return string();
	}
}

bool CAnimationRandomizer::IsItTimeForAnimationSwitch()
{
	if (!m_switchAssetsOverTime)
		return false;

	if (HasInitialDelay() && !m_initialDelayApplied)
	{
		m_initialDelayApplied = m_initialDelay < m_elapsedTimeFromEnterState;
		return m_initialDelayApplied;
	}

	// Explanation:
	// Switch randomly once every x seconds (x = m_switchProbabilitySeconds)
	// Probability this frame = (frameTime / x)

	// Is it time for an asset switch?
	float randomNumber = (cry_rand() / (float)RAND_MAX);
	float frameTime = gEnv->pTimer->GetFrameTime();
	if (randomNumber < (frameTime / m_switchProbabilitySeconds))
	{
		return true;
	}

	return false;
}

void CAnimationRandomizer::SetAnimationParams( CryCharAnimationParams& params )
{
	if (m_lastAnimId < 0)
	{
		CryLog("CAnimationRandomizer::SetAnimationParams invalid m_lastAnimId {%d}", m_lastAnimId);
		return;
	}

	// time warping
	if (m_timeAlignBetweenAssets)
		params.m_nFlags |= CA_TRANSITION_TIMEWARPING;
	else
		params.m_nFlags &= ~CA_TRANSITION_TIMEWARPING;

	SAnimation anim = m_animations[m_lastAnimId];

	// looping
	if (anim.looping)
		params.m_nFlags |= CA_LOOP_ANIMATION;
	else
		params.m_nFlags &= ~CA_LOOP_ANIMATION;

	// transition time
	params.m_fTransTime = anim.transitionTime;
}

void CAnimationRandomizer::EnterState()
{
	m_elapsedTimeFromEnterState = 0.0f;
	m_initialDelayApplied = false;
}

void CAnimationRandomizer::Update()
{
	m_elapsedTimeFromEnterState += gEnv->pTimer->GetFrameTime();
}

bool CAnimationRandomizer::HasInitialDelay() const
{
	return m_initialDelay > 0.0f;
}
