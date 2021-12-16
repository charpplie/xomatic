//==================================================================================================
// Name: CPostEffectActivationSystem
// Desc: Simple data driven system to activate post effects 
// Author: James Chilvers
//==================================================================================================

// Includes
#include "StdAfx.h"
#include "PostEffectActivationSystem.h"
#include "IItemSystem.h"

//--------------------------------------------------------------------------------------------------
// Name: Initialise
// Desc: Initialises post effect activation system from data
//       Uses the xml node name for the post effect, and activeValue and nonActiveValue attributes
//--------------------------------------------------------------------------------------------------
void CPostEffectActivationSystem::Initialise(const IItemParamsNode* postEffectListXmlNode)
{
	if(postEffectListXmlNode)
	{
		const IItemParamsNode* postEffectXmlNode = NULL;
		SPostEffectParam* param = NULL;
		int postEffectCount = postEffectListXmlNode->GetChildCount();
		m_postEffectParam.resize(postEffectCount);
		for(int i=0; i<postEffectCount; i++)
		{
			param = &m_postEffectParam[i];
			postEffectXmlNode = postEffectListXmlNode->GetChild(i);
			strcpy_s(param->name,MAX_POST_EFFECT_NAME,postEffectXmlNode->GetName());
			postEffectXmlNode->GetAttribute("activeValue",param->activeValue);
			postEffectXmlNode->GetAttribute("nonActiveValue",param->nonActiveValue);
		}
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetPostEffectsActive
// Desc: Sets active status of post effects
//--------------------------------------------------------------------------------------------------
void CPostEffectActivationSystem::SetPostEffectsActive(bool isActive)
{
	SPostEffectParam* param = NULL;
	float value = 0.0f;
	for(int i=0; i<m_postEffectParam.Size(); i++)
	{
		param = &m_postEffectParam[i];
		value = (isActive) ? param->activeValue : param->nonActiveValue;
		gEnv->p3DEngine->SetPostEffectParam( param->name, value );
	}
}//-------------------------------------------------------------------------------------------------