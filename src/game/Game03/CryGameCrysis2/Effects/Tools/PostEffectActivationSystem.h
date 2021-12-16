#ifndef _POST_EFFECT_ACTIVATION_SYSTEM_
#define _POST_EFFECT_ACTIVATION_SYSTEM_

#pragma once

// Defines
const int MAX_POST_EFFECT_NAME = 64;

//==================================================================================================
// Name: SPostEffectParam
// Desc: Post effect param used in the post effect activation system
// Author: James Chilvers
//==================================================================================================
struct SPostEffectParam
{
	SPostEffectParam()
	{
		name[0] = 0;
		activeValue = 0.0f;
		nonActiveValue = 0.0f;
	}

	void GetMemoryUsage(ICrySizer *pSizer) const {}

	char		name[MAX_POST_EFFECT_NAME];
	float		activeValue;
	float		nonActiveValue;
};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: CPostEffectActivationSystem
// Desc: Simple data driven system to activate post effects 
// Author: James Chilvers
//==================================================================================================
class CPostEffectActivationSystem
{
public:
	CPostEffectActivationSystem(){}
	~CPostEffectActivationSystem(){}

	void Initialise(const IItemParamsNode* postEffectListXmlNode); // Uses the xml node name for the post effect, and activeValue and nonActiveValue attributes
																																 // eg <Global_User_Brightness activeValue="3.0" nonActiveValue="1.0"/>
	void SetPostEffectsActive(bool isActive);

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(m_postEffectParam);
	}
private:
	PodArray<SPostEffectParam>		m_postEffectParam;
};//------------------------------------------------------------------------------------------------

#endif // _POST_EFFECT_ACTIVATION_SYSTEM_
