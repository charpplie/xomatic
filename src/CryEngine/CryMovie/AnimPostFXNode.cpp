////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: AnimPostFXNode.cpp
//  Version:   v1.00
//  Created:   15-04-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "StdAfx.h"
#include "AnimPostFXNode.h"
#include "AnimSplineTrack.h"

#include <I3DEngine.h>

//////////////////////////////////////////////////////////////////////////
//!
class CFXNodeDescription : public _i_reference_target_t
{
public:
	//-----------------------------------------------------------------------------
	//!
	class CControlParamBase : public _i_reference_target_t
	{
	public:
		CControlParamBase(){}

		virtual void SetDefault(float val) = 0;
		virtual void SetDefault(bool val) = 0;

		virtual void GetDefault(float& val) const = 0;
		virtual void GetDefault(bool& val) const = 0;

		string m_name;

	protected:
		virtual ~CControlParamBase(){}
	};

	//-----------------------------------------------------------------------------
	//!
	template<typename T>
	class TControlParam : public CControlParamBase
	{
	public:
		virtual void SetDefault(float val) { assert(0); }
		virtual void SetDefault(bool val) { assert(0); }

		virtual void GetDefault(float& val) const { assert(0); }
		virtual void GetDefault(bool& val) const { assert(0); }

	protected:
		virtual ~TControlParam(){}

		T m_defaultValue;
	};

	//-----------------------------------------------------------------------------
	//!
	CFXNodeDescription(EAnimNodeType nodeType):m_nodeType(nodeType){}

	//-----------------------------------------------------------------------------
	//!
	template<typename T>
	void AddSupportedParam( const char *sName,EAnimValue eValueType, const char *sControlName, T defaultValue)
	{
		IAnimNode::SParamInfo param;
		param.name = sName;
		param.paramId = (int)m_nodeParams.size();
		param.valueType = eValueType;
		m_nodeParams.push_back( param );

		TControlParam<T>* control = new TControlParam<T>;
		control->m_name = sControlName;
		control->SetDefault(defaultValue);

		m_controlParams.push_back(control);
	}

	//-----------------------------------------------------------------------------
	//!
	EAnimNodeType m_nodeType;
	std::vector<IAnimNode::SParamInfo> m_nodeParams;
	std::vector< _smart_ptr<CControlParamBase> > m_controlParams;
};

//-----------------------------------------------------------------------------
template<> void CFXNodeDescription::TControlParam<float>::SetDefault(float val)
{ 
	m_defaultValue = val; 
}
template<> void CFXNodeDescription::TControlParam<bool>::SetDefault(bool val)
{
	m_defaultValue = val; 
}
template<> void CFXNodeDescription::TControlParam<float>::GetDefault(float& val) const
{
	val = m_defaultValue; 
}
template<> void CFXNodeDescription::TControlParam<bool>::GetDefault(bool& val) const
{
	val = m_defaultValue; 
}

//-----------------------------------------------------------------------------
CAnimPostFXNode::FxNodeDescriptionMap CAnimPostFXNode::s_fxNodeDescriptions;
bool CAnimPostFXNode::s_initialized = false;

//-----------------------------------------------------------------------------
CAnimPostFXNode::CAnimPostFXNode(CFXNodeDescription* pDesc)
:m_pDescription(pDesc)
{
}

//-----------------------------------------------------------------------------
void CAnimPostFXNode::Initialize()
{
	if(!s_initialized)
	{
		s_initialized = true;

		//////////////////////////////////////////////////////////////////////////
		//! Radial Blur
		{
			CFXNodeDescription* pDesc = new CFXNodeDescription(ANODE_RADIALBLUR);
			s_fxNodeDescriptions[ANODE_RADIALBLUR] = pDesc;
			pDesc->m_nodeParams.reserve(4);
			pDesc->m_controlParams.reserve(4);
			pDesc->AddSupportedParam<float>("Amount",AVALUE_FLOAT, "FilterRadialBlurring_Amount",0.0f);
			pDesc->AddSupportedParam<float>("ScreenPosX",AVALUE_FLOAT,"FilterRadialBlurring_ScreenPosX", 0.5f);
			pDesc->AddSupportedParam<float>("ScreenPosY",AVALUE_FLOAT,"FilterRadialBlurring_ScreenPosY", 0.5f);
			pDesc->AddSupportedParam<float>("BlurringRadius",AVALUE_FLOAT,"FilterRadialBlurring_Radius", 1.0f);
		}
		
		//////////////////////////////////////////////////////////////////////////
		//! Color Correction
		{
			CFXNodeDescription* pDesc = new CFXNodeDescription(ANODE_COLORCORRECTION);
			s_fxNodeDescriptions[ANODE_COLORCORRECTION] = pDesc;
			pDesc->m_nodeParams.reserve(8);
			pDesc->m_controlParams.reserve(8);
			pDesc->AddSupportedParam<float>("Cyan",AVALUE_FLOAT, "Global_User_ColorC",0.0f);
			pDesc->AddSupportedParam<float>("Magenta",AVALUE_FLOAT,"Global_User_ColorM", 0.0f);
			pDesc->AddSupportedParam<float>("Yellow",AVALUE_FLOAT,"Global_User_ColorY", 0.0f);
			pDesc->AddSupportedParam<float>("Luminance",AVALUE_FLOAT,"Global_User_ColorK", 0.0f);
			pDesc->AddSupportedParam<float>("Brightness",AVALUE_FLOAT, "Global_User_Brightness",1.0f);
			pDesc->AddSupportedParam<float>("Contrast",AVALUE_FLOAT,"Global_User_Contrast", 1.0f);
			pDesc->AddSupportedParam<float>("Saturation",AVALUE_FLOAT,"Global_User_Saturation", 1.0f);
			pDesc->AddSupportedParam<float>("Hue",AVALUE_FLOAT,"Global_User_ColorHue", 0.0f);
		}
		

		//////////////////////////////////////////////////////////////////////////
		//! Depth of Field
		{
			CFXNodeDescription* pDesc = new CFXNodeDescription(ANODE_DOF);
			s_fxNodeDescriptions[ANODE_DOF] = pDesc;
			pDesc->m_nodeParams.reserve(4);
			pDesc->m_controlParams.reserve(4);
			pDesc->AddSupportedParam<bool>("Enable",AVALUE_BOOL,"Dof_User_Active",false);
			pDesc->AddSupportedParam<float>("FocusDistance",AVALUE_FLOAT,"Dof_User_FocusDistance", 3.5f);
			pDesc->AddSupportedParam<float>("FocusRange",AVALUE_FLOAT,"Dof_User_FocusRange", 5.0f);
			pDesc->AddSupportedParam<float>("BlurAmount",AVALUE_FLOAT,"Dof_User_BlurAmount", 1.0f);
		}

		//////////////////////////////////////////////////////////////////////////
		//! HDR setup - expose couple controls to cinematics
		{
			CFXNodeDescription* pDesc = new CFXNodeDescription(ANODE_HDRSETUP);
			s_fxNodeDescriptions[ANODE_HDRSETUP] = pDesc;
			pDesc->m_nodeParams.reserve(2);
			pDesc->m_controlParams.reserve(2);
			pDesc->AddSupportedParam<float>("HDRBrightLevel",AVALUE_FLOAT,"Global_User_HDRBrightLevel", 0.0f);
			pDesc->AddSupportedParam<float>("HDRBrightThreshold",AVALUE_FLOAT,"Global_User_HDRBrightThreshold", 0.0f);
		}

		//////////////////////////////////////////////////////////////////////////
		//! Shadow setup - expose couple shadow controls to cinematics
		{
			CFXNodeDescription* pDesc = new CFXNodeDescription(ANODE_SHADOWSETUP);
			s_fxNodeDescriptions[ANODE_SHADOWSETUP] = pDesc;
			pDesc->m_nodeParams.reserve(1);
			pDesc->m_controlParams.reserve(1);
			pDesc->AddSupportedParam<float>("GSMCache",AVALUE_BOOL,"GSMCacheParam", true);
		}

	}
}

//-----------------------------------------------------------------------------
CAnimNode* CAnimPostFXNode::CreateNode(EAnimNodeType nodeType)
{
	CAnimPostFXNode::Initialize();

	FxNodeDescriptionMap::iterator itr = s_fxNodeDescriptions.find(nodeType);

	if(itr==s_fxNodeDescriptions.end())
		return 0;

	CFXNodeDescription* pDesc = itr->second;
	
	return new CAnimPostFXNode(pDesc);
}

//-----------------------------------------------------------------------------
EAnimNodeType CAnimPostFXNode::GetType() const
{
	return m_pDescription->m_nodeType;
}

//-----------------------------------------------------------------------------
int CAnimPostFXNode::GetParamCount() const
{
	return m_pDescription->m_nodeParams.size();
}

//-----------------------------------------------------------------------------
bool CAnimPostFXNode::GetParamInfo( int nIndex, SParamInfo &info ) const
{
	if (nIndex >= 0 && nIndex < (int)m_pDescription->m_nodeParams.size())
	{
		info = m_pDescription->m_nodeParams[nIndex];
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool CAnimPostFXNode::GetParamInfoFromId( int paramId, SParamInfo &info ) const
{
	for (size_t i = 0; i < m_pDescription->m_nodeParams.size(); ++i)
	{
		if (m_pDescription->m_nodeParams[i].paramId == paramId)
		{
			info = m_pDescription->m_nodeParams[i];
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
void CAnimPostFXNode::CreateDefaultTracks()
{
	for (size_t i = 0; i < m_pDescription->m_nodeParams.size(); ++i)
	{
		IAnimTrack* pTrack = CreateTrack(m_pDescription->m_nodeParams[i].paramId);

		//Setup default value
		EAnimValue valueType = m_pDescription->m_nodeParams[i].valueType;
		if( valueType == AVALUE_FLOAT)
		{
			C2DSplineTrack* pFloatTrack = static_cast<C2DSplineTrack*>(pTrack);
			float val(0);
			m_pDescription->m_controlParams[i]->GetDefault(val);
			pFloatTrack->SetDefaultValue(Vec2(0,val));
		}
	}
}

//-----------------------------------------------------------------------------
void CAnimPostFXNode::Animate( SAnimContext &ac )
{
	for (size_t i = 0; i < m_tracks.size(); ++i)
	{
		IAnimTrack* pTrack = m_tracks[i].track;
		assert(pTrack);
		size_t paramIndex = (size_t)m_tracks[i].paramId;
		assert(paramIndex < m_pDescription->m_nodeParams.size());

		if(ac.trackMask && ((ac.trackMask & (1 << pTrack->GetType())) == 0))
			continue;

		EAnimValue valueType = m_pDescription->m_nodeParams[paramIndex].valueType;

		// sorry: quick & dirty solution for c2 shipping - custom type handling for shadows - make this properly after shipping
		if( GetType() == ANODE_SHADOWSETUP && valueType == AVALUE_BOOL) 
		{			
			bool val(false);
			pTrack->GetValue(ac.time, val);
			gEnv->p3DEngine->SetShadowsGSMCache(val);
		}
		else
		if(valueType == AVALUE_FLOAT)
		{
			float val(0);
			pTrack->GetValue(ac.time, val);
			gEnv->p3DEngine->SetPostEffectParam(m_pDescription->m_controlParams[paramIndex]->m_name.c_str(), val);
		}
		else if(valueType == AVALUE_BOOL)
		{
			bool val(false);
			pTrack->GetValue(ac.time, val);
			gEnv->p3DEngine->SetPostEffectParam(m_pDescription->m_controlParams[paramIndex]->m_name.c_str(), (val? 1.f:0.f) );
		}
	}
}

//-----------------------------------------------------------------------------
void CAnimPostFXNode::Reset()
{
	CAnimNode::Reset();

	// Reset each postFX param to its default.
	for (size_t i = 0; i < m_tracks.size(); ++i)
	{
		IAnimTrack* pTrack = m_tracks[i].track;
		assert(pTrack);
		size_t paramIndex = (size_t)m_tracks[i].paramId;
		assert(paramIndex < m_pDescription->m_nodeParams.size());

		EAnimValue valueType = m_pDescription->m_nodeParams[paramIndex].valueType;

		// sorry: quick & dirty solution for c2 shipping - custom type handling for shadows - make this properly after shipping
		if( GetType() == ANODE_SHADOWSETUP && valueType == AVALUE_BOOL) 
		{			
			bool val(false);
			m_pDescription->m_controlParams[paramIndex]->GetDefault(val);
			gEnv->p3DEngine->SetShadowsGSMCache(val);
		}
		else
		if(valueType == AVALUE_FLOAT)
		{
			float val(0);
			m_pDescription->m_controlParams[paramIndex]->GetDefault(val);
			gEnv->p3DEngine->SetPostEffectParam(m_pDescription->m_controlParams[paramIndex]->m_name.c_str(), val);
		}
		else if(valueType == AVALUE_BOOL)
		{
			bool val(false);
			m_pDescription->m_controlParams[paramIndex]->GetDefault(val);
			gEnv->p3DEngine->SetPostEffectParam(m_pDescription->m_controlParams[paramIndex]->m_name.c_str(), (val? 1.f:0.f) );
		}
	}
}
