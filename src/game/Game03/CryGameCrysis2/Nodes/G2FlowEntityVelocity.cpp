/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:  Flow nodes for working with XBox 360 controller
-------------------------------------------------------------------------
History:
- 31:3:2008        : Created by Kevin

*************************************************************************/

#include "StdAfx.h"
#include "Nodes/G2FlowBaseNode.h"
#include "IEntitySystem.h"

class CFlowEntityVelocity : public CFlowBaseNode
{
public:
	CFlowEntityVelocity(SActivationInfo * pActInfo)
	{
	}

	~CFlowEntityVelocity()
	{
	}

	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CFlowEntityVelocity(pActInfo);
	}

//Diesel cut
	//virtual void GetMemoryUsage(ICrySizer * s) const
	//{
	//	s->Add(*this);
	//}

//Diesel new
	virtual void GetMemoryStatistics(ICrySizer * s) 
	{
		s->Add(*this);
	}

	virtual void Serialize(SActivationInfo *pActInfo, TSerialize ser)
	{
		if (ser.IsReading())
		{
			m_actInfo = *pActInfo;
		}
	}

	enum EOutputPorts
	{
		EOP_Velocity_Horizontal = 0,
		EOP_Velocity_Vertical,
		EOP_Velocity_ForwardBackward,
		EOP_Velocity_RightLeft,
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			//InputPortConfig<float>("MaxTotalDuration", 2.0f, _HELP("Maximum duration of whole gesture sequence.")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<float>("Velocity_Horizontal", _HELP("Magnitude of horizontal entity velocity.")),
			OutputPortConfig<float>("Velocity_Vertical", _HELP("Magnitude of vertical entity velocity.")),
			OutputPortConfig<float>("Velocity_ForwardBackward", _HELP("Magnitude of forward (positive) and backward (negative) entity velocity.")),
			OutputPortConfig<float>("Velocity_RightLeft", _HELP("Magnitude of rightward (positive) and leftward (negative) entity velocity.")),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Outputs different components of an entity's velocity.");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				m_actInfo = *pActInfo;
				if (m_actInfo.pGraph != NULL)
					m_actInfo.pGraph->SetRegularlyUpdated(m_actInfo.myID, true);
			}
			break;

		case eFE_SetEntityId:
			{
				m_entityId = pActInfo->pEntity ? pActInfo->pEntity->GetId() : 0;
			}
			break;

		case eFE_Activate:
			{
				if (m_actInfo.pGraph != NULL)
					m_actInfo.pGraph->SetRegularlyUpdated(m_actInfo.myID, true);
			}
			break;

		case eFE_Update:
			{
				Vec3 velocity_world(ZERO);
				Vec3 entity_forward(ZERO);
				Vec3 entity_right(ZERO);
				Vec3 entity_up(ZERO);

				IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_entityId);
				if (pEntity != NULL)
				{
					IPhysicalEntity* pPhysEnt = pEntity->GetPhysics();
					if (pPhysEnt != NULL)
					{
						pe_status_dynamics pDyn;
						if (pPhysEnt->GetStatus(&pDyn) != 0)
						{
							velocity_world = pDyn.v;
						}
					}

					Quat entity_orientation = pEntity->GetWorldRotation();
					entity_forward = entity_orientation.GetColumn1();
					entity_right = entity_orientation.GetColumn0();
					entity_up = entity_orientation.GetColumn2();
				}
				else
					return;

				float velocity_vertical = velocity_world * entity_up;
				Vec3 velocity_vertical_vector = velocity_vertical * entity_up;
				Vec3 velocity_horizontal_vector = velocity_world - velocity_vertical_vector;
				float velocity_horizontal = velocity_horizontal_vector.GetLength();
				float velocity_forwardbackward = velocity_world * entity_forward;
				float velocity_rightleft = velocity_world * entity_right;
				
#define RoundTwoDecimals(x) ((float)(int)((x)*100.0f)*0.01f)
				velocity_horizontal = RoundTwoDecimals(velocity_horizontal);
				velocity_vertical = RoundTwoDecimals(velocity_vertical);
				velocity_forwardbackward = RoundTwoDecimals(velocity_forwardbackward);
				velocity_horizontal = RoundTwoDecimals(velocity_horizontal);

				ActivateOutput(&m_actInfo, EOP_Velocity_Horizontal, velocity_horizontal);
				ActivateOutput(&m_actInfo, EOP_Velocity_Vertical, velocity_vertical);
				ActivateOutput(&m_actInfo, EOP_Velocity_ForwardBackward, velocity_forwardbackward);
				ActivateOutput(&m_actInfo, EOP_Velocity_RightLeft, velocity_rightleft);
			}
			break;
		}

	}

private:
	SActivationInfo m_actInfo; // Activation info instance
	EntityId m_entityId;
};

REGISTER_FLOW_NODE("Entity:Velocity", CFlowEntityVelocity);
