
#include "StdAfx.h"
#include <ISystem.h>
#include <Cry_Math.h>
#include "FlowBaseNode.h"


class CRotateEntityTo_Node : public CFlowBaseNode
{
	CTimeValue m_startTime;
	CTimeValue m_endTime;
	Quat m_targetQuat;
	Quat m_sourceQuat;
	Vec3 m_rotVel;

public:
	CRotateEntityTo_Node( SActivationInfo * pActInfo )
	{
		m_targetQuat.SetIdentity();
		m_sourceQuat.SetIdentity();
	};

	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CRotateEntityTo_Node(pActInfo);
	}

	void Serialize(SActivationInfo *pActInfo, TSerialize ser)
	{
		ser.BeginGroup("Local");
		ser.Value("m_startTime", m_startTime);
		ser.Value("m_endTime", m_endTime);
		ser.Value("m_sourceQuat", m_sourceQuat);
		ser.Value("m_targetQuat", m_targetQuat);
		ser.EndGroup();
	}

	enum EInPorts
	{
		IN_DEST = 0,
		IN_DYN_DEST,
		IN_DURATION,
		IN_START,
		IN_STOP
	};
	enum EOutPorts
	{
		OUT_CURRENT = 0,
		OUT_CURRENT_RAD,
		OUT_DONE
	};

	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<Vec3>( "Destination",_HELP("Destination") ),
			InputPortConfig<bool>( "DynamicUpdate", true, _HELP("Dynamic update of Destination [follow-during-movement]"), _HELP("DynamicUpdate") ),
			InputPortConfig<float>( "Time",_HELP("Duration in seconds") ),
			InputPortConfig_Void( "Start", _HELP("Trigger this port to start the movement") ),
			InputPortConfig_Void( "Stop",  _HELP("Trigger this port to stop the movement") ),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<Vec3>("Current",_HELP("Current Rotation in Degrees")),
			OutputPortConfig<Vec3>("CurrentRad",_HELP("Current Rotation in Radian")),
			OutputPortConfig_Void("DoneTrigger", _HELP("Triggered when destination rotation is reached or rotation stopped"), 	_HELP("Done")),
			{0}
		};
		config.sDescription = _HELP( "Rotate an entity during a defined period of time" );
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}

	void DegToQuat(const Vec3& rotation, Quat& destQuat)
	{
		Ang3 temp;
		// snap the angles 
		temp.x = Snap_s360(rotation.x);
		temp.y = Snap_s360(rotation.y);
		temp.z = Snap_s360(rotation.z);
		const Ang3 angRad = DEG2RAD(temp);
		destQuat.SetRotationXYZ(angRad);
	}

	void Interpol(const float fTime, SActivationInfo* pActInfo)
	{
		Quat theQuat;
		theQuat.SetSlerp(m_sourceQuat, m_targetQuat, fTime);
		if (pActInfo->pEntity)
		{
			pActInfo->pEntity->SetRotation(theQuat);
			if (pActInfo->pEntity->GetPhysics())
			{
				pe_action_set_velocity asv;
				asv.w=m_rotVel; asv.bRotationAroundPivot=1;
				pActInfo->pEntity->GetPhysics()->Action(&asv);
			}
		}
		Ang3 angles = Ang3(theQuat);
		Ang3 anglesDeg = RAD2DEG(angles);
		ActivateOutput(pActInfo, OUT_CURRENT_RAD, Vec3(angles));
		ActivateOutput(pActInfo, OUT_CURRENT, Vec3(anglesDeg));
	}

	void StopInterpol(SActivationInfo* pActInfo)
	{
		if (pActInfo->pEntity && pActInfo->pEntity->GetPhysics())
		{
			pe_action_set_velocity asv; 
			asv.w.zero(); asv.bRotationAroundPivot=1;
			pActInfo->pEntity->GetPhysics()->Action(&asv);
		}
	}

	void SnapToTarget(SActivationInfo* pActInfo)
	{
		if (pActInfo->pEntity)
		{
			pActInfo->pEntity->SetRotation(m_targetQuat);
			if (pActInfo->pEntity->GetPhysics())
			{
				pe_action_set_velocity asv;
				asv.w.zero(); asv.bRotationAroundPivot=1;
				pActInfo->pEntity->GetPhysics()->Action(&asv);

				pe_params_pos pos;
				pos.pos = pActInfo->pEntity->GetWorldPos();
				pos.q = m_targetQuat;
				pActInfo->pEntity->GetPhysics()->SetParams(&pos);
			}
		}
		Ang3 angles = Ang3(m_targetQuat);
		Ang3 anglesDeg = RAD2DEG(angles);
		ActivateOutput(pActInfo, OUT_CURRENT_RAD, Vec3(angles));
		ActivateOutput(pActInfo, OUT_CURRENT, Vec3(anglesDeg));
	}

	virtual void ProcessEvent( EFlowEvent event,SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, IN_STOP)) // Stop
				{
					pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
					ActivateOutput(pActInfo, OUT_DONE, true);
					StopInterpol(pActInfo);
				}
				if (IsPortActive(pActInfo, IN_DEST) && GetPortBool(pActInfo, IN_DYN_DEST) == true)
				{
					const Vec3& destRotDeg = GetPortVec3(pActInfo,	IN_DEST);
					DegToQuat(destRotDeg, m_targetQuat);
				}
				if (IsPortActive(pActInfo,IN_START)) {
					m_startTime = gEnv->pTimer->GetFrameStartTime();
					m_endTime = m_startTime + GetPortFloat(pActInfo, IN_DURATION);

					const Vec3& destRotDeg = GetPortVec3(pActInfo,	IN_DEST);
					DegToQuat(destRotDeg, m_targetQuat);

					IEntity *pEnt = pActInfo->pEntity;
					if(pEnt)
						m_sourceQuat = pEnt->GetRotation();
					else
						m_sourceQuat = m_targetQuat;

					m_rotVel = log(m_targetQuat*!m_sourceQuat) * (2.0f/max(0.01f,(m_endTime-m_startTime).GetSeconds()));

					Interpol(0.0f, pActInfo);
					pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true	);
				}
				break;
			}

		case eFE_Initialize:
			{
				pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
				break;
			}

		case eFE_Update:
			{
				CTimeValue curTime = gEnv->pTimer->GetFrameStartTime();
				const float fDuration = (m_endTime - m_startTime).GetSeconds();
				float fPosition;
				if (fDuration <= 0.0)
					fPosition = 1.0;
				else
				{
					fPosition = (curTime - m_startTime).GetSeconds() / fDuration;
					fPosition = clamp(fPosition, 0.0f, 1.0f);
				}
				if (curTime >= m_endTime)
				{
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
					ActivateOutput(pActInfo, OUT_DONE, true);
					SnapToTarget(pActInfo);
				}
				else
				{
					Interpol(fPosition, pActInfo);
				}
				break;
			}
		};
	};

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_RotateSpeed : public CFlowBaseNode
{
public:
	CFlowNode_RotateSpeed(SActivationInfo * pActInfo) : m_flags(0), m_speed(0.0f), m_interpolate(0.0f)
	{

	}

	~CFlowNode_RotateSpeed()
	{

	}

	IFlowNodePtr Clone(SActivationInfo * pActInfo)
	{
		return new CFlowNode_RotateSpeed(pActInfo);
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

	virtual void Serialize(SActivationInfo *pActInfo, TSerialize ser)
	{
		ser.Value("m_flags", m_flags);
		ser.Value("m_speed", m_speed);
		ser.Value("m_interpolate", m_interpolate);
		ser.Value("m_rot", m_rot);
		ser.Value("m_dest", m_dest);
		ser.Value("m_iniRot", m_iniRot);

		if (ser.IsReading())
		{
			if (EF_RUNNING == (m_flags&EF_RUNNING))
			{
				pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
				ActivateOutput(pActInfo, EOP_Start, true);
			}
			ActivateOutput(pActInfo, EOP_SpeedChanged, m_speed);
		}
	}

	enum EInputPorts
	{
		EIP_Destination = 0,
		EIP_Speed,
		EIP_Start,
		EIP_Stop,
		EIP_Reset,
	};

	enum EOutputPorts
	{
		EOP_Start = 0,
		EOP_Stop,
		EOP_Finish,
		EOP_SpeedChanged,
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig<Vec3>("Destination", _HELP("Angles that define the final orientation in local space")),
			InputPortConfig<float>("Speed", 1.0f, _HELP("How fast to rotate (degrees per second)")),
			InputPortConfig_Void("Start", _HELP("Start/resume rotating the entity using the desired speed")),
			InputPortConfig_Void("Stop", _HELP("Stop/pause rotating the entity")),
			InputPortConfig_Void("Reset", _HELP("Reset entity rotation to its starting point (when start was first called)")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig_Void("Start", _HELP("Called when rotation is started")),
			OutputPortConfig_Void("Stop", _HELP("Called when rotation is stopped")),
			OutputPortConfig_Void("Finish", _HELP("Called when rotation is completed")),
			OutputPortConfig<float>("SpeedChange", _HELP("Called when speed is updated")),
			{0}
		};
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Rotate an entity to a final destination at a configurable speed");
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
		case eFE_SetEntityId:
			{
				// Stop rotation and reset first start
				m_flags = 0;
				m_speed = 0.0f;
				m_interpolate = 0.0f;
				pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
				m_flags &= ~(EF_RUNNING|EF_FIRSTSTART);
			}
			break;

		case eFE_Activate:
			{
				IEntity *pEntity = pActInfo->pEntity;
				if (!pEntity)
					return;
					
				if (IsPortActive(pActInfo, EIP_Start))
				{
					if (EF_RUNNING == (m_flags&EF_RUNNING))
						return;

					if (EF_FIRSTSTART != (m_flags&EF_FIRSTSTART))
					{
						m_flags |= EF_FIRSTSTART;
						m_iniRot = pEntity->GetRotation();
						m_interpolate = 0.0f;
						m_rot = m_iniRot;
					}
					if (EF_FIRSTSTART == (m_flags&EF_FIRSTSTART))
					{
						pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
						m_flags |= EF_RUNNING;
						ActivateOutput(pActInfo, EOP_Start, true);

						GetNewDestination(pActInfo);
						m_rot = pEntity->GetRotation();
						GetNewSpeed(pActInfo);
						m_interpolate = 0.0f;
					}
				}
				if (IsPortActive(pActInfo, EIP_Stop))
				{
					if (EF_RUNNING != (m_flags&EF_RUNNING))
						return;
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
					m_flags &= ~EF_RUNNING;
					ActivateOutput(pActInfo, EOP_Stop, true);
				}
				if (IsPortActive(pActInfo, EIP_Reset))
				{
					if (EF_FIRSTSTART == (m_flags&EF_FIRSTSTART))
					{
						pEntity->SetRotation(m_iniRot);
						m_rot = m_iniRot;
						m_interpolate = 0.0f;
					}
				}
				if (IsPortActive(pActInfo, EIP_Destination))
				{
					GetNewDestination(pActInfo);
					m_rot = pEntity->GetRotation();
					GetNewSpeed( pActInfo );
						
					m_interpolate = 0.0f;
				}
				
				if (IsPortActive(pActInfo, EIP_Speed))
				{
					GetNewSpeed(pActInfo);
				}
				
			}
			break;

		case eFE_Update:
			{
				if (EF_RUNNING != (m_flags&EF_RUNNING))
					return;

				// Interpolation value increased by speed
				const float speed = m_speed * gEnv->pTimer->GetFrameTime();
				m_interpolate += speed;

				// Update rotation
				Quat quat;
				quat.SetSlerp(m_rot, m_dest, min(m_interpolate,1.0f));
				pActInfo->pEntity->SetRotation(quat);

				// Are we at the desired point?
				if (m_interpolate >= 1.0f)
				{
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
					m_flags &= ~EF_RUNNING;
					ActivateOutput(pActInfo, EOP_Finish, true);
				}
			}
			break;
		}
	}

	//! Gets new destination and converts it
	void GetNewDestination(SActivationInfo *pActInfo)
	{
		const Vec3& v = GetPortVec3(pActInfo, EIP_Destination);
		const Ang3 ang(DEG2RAD(v.x), DEG2RAD(v.y), DEG2RAD(v.z));
		m_dest = Quat::CreateRotationXYZ(ang);
	}

	//! Gets new speed and converts it
	void GetNewSpeed(SActivationInfo *pActInfo)
	{
		Quat qOriInv = m_rot.GetInverted();
		Quat qDist = qOriInv * m_dest;
		assert(qDist.IsValid());
		assert(qDist.IsUnit());
		
		float angDiff = RAD2DEG( 2 * acosf(qDist.w) );
		if (angDiff>180.f)
		{
			angDiff = 360.0f - angDiff;
		}

		const float desiredSpeed = fabsf(GetPortFloat(pActInfo, EIP_Speed));
		const float speed = (fabsf(angDiff) < 0.01f) ? 0.0f : (desiredSpeed * (1.0f/360.f)) * (360.f / angDiff);
		assert(NumberValid(speed));
		if (speed != m_speed)
		{
			m_speed = speed;
			ActivateOutput(pActInfo, EOP_SpeedChanged, desiredSpeed);
		}
	}

private:
	// EFlags - Settings for node
	enum EFlags
	{
		EF_FIRSTSTART = 0x01,
		EF_RUNNING = 0x02,
	};

	unsigned char	m_flags;		// See EFlags
	float			m_speed;		// Rotation speed (deg per sec)
	float			m_interpolate;	// Interpolation ratio for SLERP
	Quat			m_iniRot;			// Initial rotation (on first start)
	Quat			m_rot; // initial rotation in the current movement
	Quat			m_dest;			// Destination rotation
};


REGISTER_FLOW_NODE( "Movement:RotateEntityTo", CRotateEntityTo_Node );
REGISTER_FLOW_NODE( "Movement:RotateEntitySpeed", CFlowNode_RotateSpeed);
