#ifndef _ANIMATION_MANAGER
#define _ANIMATION_MANAGER


#include "AnimationLoader.h"

// Fake class for controllers
struct CAnimationManager
{

	static CAnimationManager& GetInst()
	{
		static CAnimationManager AnimationManagerObj; 
		return AnimationManagerObj;
	}

	CAnimationManager()
	{
		m_arrGlobalAnimations.resize(1);	
	}

	std::vector<GlobalAnimationHeaderCAF> m_arrGlobalAnimations; 
	std::vector<GlobalAnimationHeaderCAF> m_arrGlobalAnimations2;
	std::vector<GlobalAnimationHeaderAIM> m_arrGlobalAIM;

};


#endif