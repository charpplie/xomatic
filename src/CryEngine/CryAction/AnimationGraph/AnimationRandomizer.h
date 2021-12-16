#ifndef __ANIMATION_RANDOMIZER_H__
#define __ANIMATION_RANDOMIZER_H__

#pragma once


class CAnimationRandomizer
{
public:
	CAnimationRandomizer();

	bool Load( XmlNodeRef node );

	//! Returns an animation name from the set, according to probabilities
	//! and whether or not an asset can be started twice in a row
	string GetNextAssetName();

	//! Returns false if the randomized assets are not supposed to be
	//! switching automatically after some time.
	//! Returns true if the assets are set up to switch and the randomization
	//! indicates it is time for one. (Call GetNextAssetName once this returns true)
	bool		IsItTimeForAnimationSwitch();

	//! Returns whether the asset switching is generally enabled.
	//! This will not indicate that it is time to switch assets. (use ::IsItTimeForAnimationSwitch())
	bool		GetAnimationSwitchingEnabled() const { return m_switchAssetsOverTime; }

	//! Modifies the passed in parameters according to the next assets needs (looping, time-warping and transition time)
	void		SetAnimationParams( CryCharAnimationParams& params );

	//!  To be called on EnterState
	void		EnterState();
	
	//!  To be called on Update
	void		Update();

	//!  Return whether initialDelaySeconds has been set or not
	bool		HasInitialDelay() const;

private:

	struct SAnimation
	{
		bool		canBeStartedTwiceInARow;
		bool		looping;
		float		probability;
		float		transitionTime;
		string	animName;

		SAnimation()
			: canBeStartedTwiceInARow(true)
			, looping(false)
			, probability(1.0f)
			, transitionTime(0.2f)
		{
		}
	};

	bool		m_switchAssetsOverTime;
	bool		m_timeAlignBetweenAssets;
	int			m_lastAnimId;
	float		m_probabilitySum;
	float		m_switchProbabilitySeconds;
	std::vector<SAnimation> m_animations;

	float		m_initialDelay;
	bool		m_initialDelayApplied;
	float		m_elapsedTimeFromEnterState;
};



#endif // __ANIMATION_RANDOMIZER_H__
