#include "StdAfx.h"
#include "FlowBaseNode.h"

class CFlowNode_EnvLighting : public CFlowBaseNode
{
public:
	CFlowNode_EnvLighting( SActivationInfo * pActInfo )
	{
	}

	enum EInputPorts
	{
		eIP_TimeOfDay = 0,
	};

	enum EOutputPorts
	{
		eOP_SunDirection = 0,
	};

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<float>("TimeOfDay"),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<Vec3>("SunDirection"),
			{0}
		};
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_OBSOLETE);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo,eIP_TimeOfDay))
				Update(pActInfo);
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

private:

	void Update(SActivationInfo * pActInfo)
	{
		float timeOfDay = GetPortFloat(pActInfo, eIP_TimeOfDay);
		float longitude = timeOfDay / 24.0f * gf_PI * 2.0f;
		float latitude = -45.0f * gf_PI / 180.0f;
		Vec3 sunDir( sinf(longitude) * cosf(latitude), sinf(longitude) * sinf(latitude), cosf(longitude) );

		CRY_ASSERT(!"Direct call of I3DEngine::SetSunDir() is not supported anymore. Use Time of day featue instead.");
//		gEnv->p3DEngine->SetSunDir(sunDir);
		ActivateOutput(pActInfo, eOP_SunDirection, sunDir);
	}
};

class CFlowNode_EnvMoonDirection : public CFlowBaseNode
{
public:
	CFlowNode_EnvMoonDirection( SActivationInfo * pActInfo )
	{
	}

	enum EInputPorts
	{
		eIP_Get = 0,
		eIP_Set,
		eIP_Latitude,
		eIP_Longitude,
		eIP_ForceUpdate,
	};

	enum EOutputPorts
	{
		eOP_Latitude = 0,
		eOP_Longitude,
	};

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void("Get", _HELP("Get the current Latitude/Longitude")),
			InputPortConfig_Void("Set", _HELP("Set the Latitude/Longitude")),
			InputPortConfig<float>("Latitude",  0.0f, _HELP("Latitude to be set")),
			InputPortConfig<float>("Longitude", 35.0f, _HELP("Longitude to be set")),
			InputPortConfig<bool>("ForceUpdate", false, _HELP("Force Immediate Update of Sky if true. USE ONLY IN SPECIAL CASES, is heavy on performance!") ),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<float>("Latitude", _HELP("Current Latitude")),
			OutputPortConfig<float>("Longitude", _HELP("Current Longitude")),
			{0}
		};
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.sDescription = _HELP("Set the 3DEngine's Moon Direction");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo,eIP_Get))
			{
				Vec3 v;
				gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_MOONROTATION, v);
				ActivateOutput(pActInfo, eOP_Latitude, v.x);
				ActivateOutput(pActInfo, eOP_Longitude, v.y);
			}
			if (IsPortActive(pActInfo, eIP_Set))
			{
				Vec3 v(ZERO);
				v.x = GetPortFloat(pActInfo, eIP_Latitude);
				v.y = GetPortFloat(pActInfo, eIP_Longitude);
				gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_MOONROTATION, v);
				ActivateOutput(pActInfo, eOP_Latitude, v.x);
				ActivateOutput(pActInfo, eOP_Longitude, v.y);
				bool forceUpdate = GetPortBool( pActInfo, eIP_ForceUpdate );
				ITimeOfDay* pTOD = gEnv->p3DEngine->GetTimeOfDay();
				if (forceUpdate && pTOD )
					pTOD->Update( true, true );
			}
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

};


class CFlowNode_EnvSun : public CFlowBaseNode
{
public:
	CFlowNode_EnvSun( SActivationInfo * pActInfo )
	{
	}

	enum EInputPorts
	{
		eIP_Get = 0,
		eIP_Set,
		eIP_Latitude,
		eIP_Longitude,
		eIP_ForceUpdate,
	};

	enum EOutputPorts
	{
		eOP_Latitude = 0,
		eOP_Longitude,
	};

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void("Get", _HELP("Get the current Latitude/Longitude")),
			InputPortConfig_Void("Set", _HELP("Set the Latitude/Longitude")),
			InputPortConfig<float>("Latitude",  0.0f, _HELP("Latitude to be set")),
			InputPortConfig<float>("Longitude", 35.0f, _HELP("Longitude to be set")),
			InputPortConfig<bool>("ForceUpdate", false, _HELP("Force Immediate Update of Sky if true. USE ONLY IN SPECIAL CASES, is heavy on performance!") ),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<float>("Latitude", _HELP("Current Latitude")),
			OutputPortConfig<float>("Longitude", _HELP("Current Longitude")),
			{0}
		};
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.sDescription = _HELP("Set the 3DEngine's Sun Data");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo,eIP_Get))
			{
				Vec3 v;
				gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_SUNROTATION, v);
				ActivateOutput(pActInfo, eOP_Latitude, v.x);
				ActivateOutput(pActInfo, eOP_Longitude, v.y);
			}
			if (IsPortActive(pActInfo, eIP_Set))
			{
				Vec3 v(ZERO);
				v.x = GetPortFloat(pActInfo, eIP_Latitude);
				v.y = GetPortFloat(pActInfo, eIP_Longitude);
				gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_SUNROTATION, v);
				bool forceUpdate = GetPortBool( pActInfo, eIP_ForceUpdate );
				ITimeOfDay* pTOD = gEnv->p3DEngine->GetTimeOfDay();
				if (pTOD)
					pTOD->Update( true, forceUpdate );
					
				ActivateOutput(pActInfo, eOP_Latitude, v.x);
				ActivateOutput(pActInfo, eOP_Longitude, v.y);
			}
			break;
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

};


REGISTER_FLOW_NODE("Environment:Lighting", CFlowNode_EnvLighting);
REGISTER_FLOW_NODE_SINGLETON("Environment:MoonDirection", CFlowNode_EnvMoonDirection);
REGISTER_FLOW_NODE_SINGLETON("Environment:Sun", CFlowNode_EnvSun);
