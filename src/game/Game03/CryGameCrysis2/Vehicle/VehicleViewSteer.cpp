/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a third person view for vehicles

*************************************************************************/
#include "StdAfx.h"

#include "IViewSystem.h"
#include "IVehicleSystem.h"
#include "IWeapon.h"
#include "VehicleSystem/VehicleSeat.h"
#include "VehicleViewSteer.h"
#include "IActorSystem.h"
#include "Game.h"
#include "GameCVars.h"
#include "VehicleMovementBase.h"

#include "Cry_GeoIntersect.h"


const char* CVehicleViewSteer::m_name = "SteerThirdPerson";

static float g_CameraRadius = 0.5f;

//------------------------------------------------------------------------
CVehicleViewSteer::CVehicleViewSteer()
{
	m_position.zero();
	m_angReturnSpeed1 = 0.f;
	m_angReturnSpeed2 = 0.f;
	m_angSpeedCorrection0 = 0.f;
	m_lookAt0=Vec3(0.f, 0.f, 1.5f);
	m_localSpaceCameraOffset=Vec3(0.f, -3.f, 1.f);
	m_maxRotation.zero();
	m_maxRotation2.zero();
	m_stickSensitivity.Set(0.5f,0.5f,0.5f);
	m_stickSensitivity2.Set(0.5f,0.5f,0.5f);
	m_inheritedElev = 0.2f;
	m_pivotOffset=0.f;
	m_flags = eVCam_goingForwards | eVCam_canRotate;
	m_forwardFlags = eVCam_rotationSpring | eVCam_rotationClamp;
	m_backwardsFlags = m_forwardFlags;
	m_radiusRelaxRate=10.f;
	m_radiusDampRate=1.f;
	m_radiusVelInfluence=0.1f;
	m_radiusMin=0.9f;
	m_radiusMax=1.1f;
}

//------------------------------------------------------------------------
CVehicleViewSteer::~CVehicleViewSteer()
{
}

static void getFlag(CVehicleParams& params, const char* key, int& flags, int flag)
{
	int tmp = flags & flag;
	params.getAttr(key, tmp);
	if (tmp)
		flags |= flag;
	else
		flags &= ~flag;
}

//------------------------------------------------------------------------
bool CVehicleViewSteer::Init(CVehicleSeat* pSeat, const CVehicleParams& table)
{
	if (!CVehicleViewGameBase::Init(pSeat, table))
		return false;

	m_pSeat = pSeat;
	m_pVehicle = pSeat->GetVehicle();

	CVehicleParams params = table.findChild(m_name);
	if (!params)
		return false;

	CVehicleParams posParams = params.findChild("Pos");
	CVehicleParams rotationParams = params.findChild("Rotation");
	CVehicleParams motionParams = params.findChild("Motion");
	CVehicleParams backwardsParams = params.findChild("Backwards");
	CVehicleParams radiusParams = params.findChild("Radius");

	if (posParams)
	{
		posParams.getAttr("offset", m_localSpaceCameraOffset);
		posParams.getAttr("aim", m_lookAt0);
		posParams.getAttr("pivotOffset", m_pivotOffset);
	}

	if (rotationParams)
	{
		rotationParams.getAttr("rotationMax", m_maxRotation);
		rotationParams.getAttr("rotationMax2", m_maxRotation2);
		rotationParams.getAttr("stickSensitivity", m_stickSensitivity);
		rotationParams.getAttr("stickSensitivity2", m_stickSensitivity2);
		rotationParams.getAttr("inheritedElev", m_inheritedElev);
		getFlag(rotationParams, "canRotate", m_flags, eVCam_canRotate);
		m_inheritedElev = clamp(m_inheritedElev, 0.f, 1.f);
	}

	if (motionParams)
	{
		motionParams.getAttr("returnSpeed", m_angReturnSpeed1);
		motionParams.getAttr("returnSpeed2", m_angReturnSpeed2);
		motionParams.getAttr("angFollow", m_angSpeedCorrection0);
	}

	if (backwardsParams)
	{
		getFlag(backwardsParams, "clamp", m_backwardsFlags, eVCam_rotationClamp);
		getFlag(backwardsParams, "returnSpring", m_backwardsFlags, eVCam_rotationSpring);
	}

	if (radiusParams)
	{
		radiusParams.getAttr("min", m_radiusMin);
		radiusParams.getAttr("max", m_radiusMax);
		radiusParams.getAttr("relaxRate", m_radiusRelaxRate);
		radiusParams.getAttr("dampRate", m_radiusDampRate);
		radiusParams.getAttr("velInfluence", m_radiusVelInfluence);
	}

	m_maxRotation = DEG2RAD(m_maxRotation);
	m_maxRotation2 = DEG2RAD(m_maxRotation2);

	Reset();
	return true;
}

//------------------------------------------------------------------------
bool CVehicleViewSteer::Init(CVehicleSeat* pSeat)
{
	if (!CVehicleViewGameBase::Init(pSeat))
		return false;

	m_pSeat = pSeat;
	m_pVehicle = pSeat->GetVehicle();

	Reset();
	return true;
}

//------------------------------------------------------------------------
void CVehicleViewSteer::Reset()
{
	CVehicleViewGameBase::Reset();
	
	IEntity * pEntity = m_pVehicle->GetEntity();
	assert(pEntity);
	m_rotatingAction.zero();
	m_position = pEntity->GetWorldTM() * m_localSpaceCameraOffset;
	Vec3 entityPos = pEntity->GetPos();
	CalcLookAt(pEntity->GetWorldTM());
	m_lastOffset = m_position - m_lookAt;
	m_lastOffsetBeforeElev = m_lastOffset;
	m_angReturnSpeed = 0.f;
	m_angSpeedCorrection = 0.f;
	// Going Forwards
	m_flags &= ~m_backwardsFlags;
	m_flags |= eVCam_goingForwards | m_forwardFlags;
	m_rotation.zero();
	m_radius = fabsf(m_localSpaceCameraOffset.y);
	m_curYaw = 0.f;
	m_lastVehVel.zero();
	m_radiusVel =0.f;
}

void CVehicleViewSteer::CalcLookAt(const Matrix34& entityTM)
{
	Vec3 tmp(m_lookAt0.x, m_lookAt0.y, m_lookAt0.z + m_pivotOffset);
	m_lookAt = entityTM * tmp;
	m_lookAt.z -= m_pivotOffset;
}

//------------------------------------------------------------------------
void CVehicleViewSteer::OnAction(const TVehicleActionId actionId, int activationMode, float value)
{
	//CVehicleViewGameBase::OnAction(actionId, activationMode, value);

	if (m_flags & eVCam_canRotate)
	{
		if (actionId == eVAI_RotateYaw)
		{
			m_rotatingAction.z += value;
		}
		else if (actionId == eVAI_RotatePitch)
		{		
			m_rotatingAction.x += value;
		}
	}
}

//------------------------------------------------------------------------
void CVehicleViewSteer::OnStartUsing(EntityId passengerId)
{
	CVehicleViewGameBase::OnStartUsing(passengerId);
}

//------------------------------------------------------------------------
void CVehicleViewSteer::OnStopUsing()
{
	EntityId passengerId = m_pSeat->GetPassenger();

	CVehicleViewGameBase::OnStopUsing();
}

//------------------------------------------------------------------------
void CVehicleViewSteer::Update(float dt)
{
	IEntity * pEntity = m_pVehicle->GetEntity();
	assert(pEntity);

	CVehicleMovementBase * pVehicleMovement = static_cast<CVehicleMovementBase*>(m_pVehicle->GetMovement());
	const pe_status_dynamics& dynStatus = pVehicleMovement->GetPhysicsDyn();

	SMovementState movementState;	
	pVehicleMovement->GetMovementState(movementState);
	const float pedal = pVehicleMovement->GetEnginePedal();
	const float maxSpeed = movementState.maxSpeed;
	const Matrix34& pose = m_pAimPart ? m_pAimPart->GetWorldTM() : pEntity->GetWorldTM();
	const Vec3 entityPos = pose.GetColumn3();
	const Vec3 xAxis = pose.GetColumn0();
	const Vec3 yAxis = pose.GetColumn1();
	const Vec3 zAxis = pose.GetColumn2();
	const float forwardSpeed = dynStatus.v.dot(yAxis);
	const float speedNorm = clamp(forwardSpeed / maxSpeed, 0.0f, 1.0f);
	const Vec3 maxRotation = m_maxRotation + speedNorm * (m_maxRotation2 - m_maxRotation);

	CalcLookAt(pose);
	if (m_lookAt.IsValid())
	{
		if (!m_lastOffset.IsValid())
		{
			m_position = pose * m_localSpaceCameraOffset;
			m_lastOffset = m_position - m_lookAt;
			m_lastOffsetBeforeElev = m_lastOffset;
		}

		Vec3 offset = m_lastOffsetBeforeElev;

		if (pedal<0.1f && forwardSpeed<1.0f)
		{
			// Going Backwards
			m_flags &= ~(eVCam_goingForwards | m_forwardFlags);
			m_flags |= m_backwardsFlags;
		}

		if (offset.dot(yAxis) < 0.8f && forwardSpeed>1.f)
		{
			// Going Forwards
			m_flags &= ~m_backwardsFlags;
			m_flags |= eVCam_goingForwards | m_forwardFlags;
		}
			
		float sensitivity = (1.f-speedNorm) * m_stickSensitivity.z + speedNorm * m_stickSensitivity2.z;
		float rotate = -m_rotatingAction.z * sensitivity;
		rotate = rotate * dt;
		
		if (zAxis.z > 0.1f)
		{
			// Safe to update curYaw
			Vec3 projectedX = xAxis; projectedX.z = 0.f;
			Vec3 projectedY = yAxis; projectedY.z = 0.f;
			const float newYaw = atan2_tpl(offset.dot(projectedX), -(offset.dot(projectedY)));
			const float maxChange = DEG2RAD(270.f)*dt;
			const float delta = clamp(newYaw - m_curYaw, -maxChange, +maxChange);
			m_curYaw += delta;
		}
	
		// Rotation Action
		{
			if (m_flags & eVCam_rotationClamp)
			{
				float newYaw = clamp(m_curYaw + rotate, -maxRotation.z, +maxRotation.z);
				rotate = newYaw - m_curYaw;
				rotate = clamp(newYaw - m_curYaw, -fabsf(rotate), +fabsf(rotate));
				m_rotation.z += rotate;
			}
			else
			{
				m_rotation.z=0.f;
			}

			if (speedNorm > 0.1f)
			{
				float reduce = dt * 1.f;
				m_rotation.z = m_rotation.z - reduce*m_rotation.z/(fabsf(m_rotation.z) + reduce);
			}
		}

		// Ang Spring
		{
			float angSpeedCorrection = dt*dt*m_angSpeedCorrection/(dt*m_angSpeedCorrection+1.f)*dynStatus.w.z;
			if ((m_flags & eVCam_rotationSpring)==0)
			{
				m_angReturnSpeed = 0.f;
				angSpeedCorrection = 0.f;
			}

			float difference = m_rotation.z - m_curYaw;
			float relax = difference * (m_angReturnSpeed*dt) / ((m_angReturnSpeed*dt) + 1.f);

			const float delta = +relax + angSpeedCorrection + rotate;
			m_curYaw += delta;

			Matrix33 rot = Matrix33::CreateRotationZ(delta);
			offset = rot * offset;

			// Lerp the spring speed
			float angSpeedTarget = m_angReturnSpeed1 + speedNorm * (m_angReturnSpeed2 - m_angReturnSpeed1);
			m_angReturnSpeed += (angSpeedTarget - m_angReturnSpeed) * (dt/(dt+0.3f));
			m_angSpeedCorrection += (m_angSpeedCorrection0 - m_angSpeedCorrection) * (dt/(dt+0.3f));
		}

		if (!offset.IsValid()) offset = m_lastOffset;

		// Velocity influence
		Vec3 displacement = -((2.f-speedNorm) * dt) * dynStatus.v;// - yAxis*(0.0f*speedNorm*(yAxis.dot(dynStatus.v))));

		float dot = offset.dot(displacement);
		if (dot < 0.f)
		{
			displacement = displacement + offset * -0.1f * (offset.dot(displacement) / offset.GetLengthSquared());
		}
		offset = offset + displacement;

		const float radius0 = fabsf(m_localSpaceCameraOffset.y);
		const float minRadius = radius0 * m_radiusMin;
		const float maxRadius = radius0 * m_radiusMax;
		float radiusXY = sqrtf(sqr(offset.x) + sqr(offset.y));

		Vec3 offsetXY = offset; offsetXY.z = 0.f;
		Vec3 accelerationV = (dynStatus.v - m_lastVehVel);
		float acceleration = offsetXY.dot(accelerationV) / radiusXY;

		m_lastVehVel = dynStatus.v;
		m_radiusVel -= acceleration;
		m_radius += m_radiusVel * dt - dt*m_radiusVelInfluence * offsetXY.dot(dynStatus.v)/radiusXY;
		m_radiusVel *= expf(-dt*m_radiusDampRate);
		m_radius += (radius0-m_radius)*(dt*m_radiusRelaxRate)/(dt*m_radiusRelaxRate+1.f);
		m_radius = clamp(m_radius, minRadius, maxRadius);
		offset = offset * (m_radius/radiusXY);
	
		// Vertical motion
		float targetOffsetHeight = m_localSpaceCameraOffset.z * (m_radius/radius0);
		float oldOffsetHeight = offset.z;
		offset.z += (targetOffsetHeight - offset.z)*(dt/(dt+0.3f));
		Limit(offset.z, targetOffsetHeight - 2.f, targetOffsetHeight + 2.f);
		float verticalChange = offset.z - oldOffsetHeight;

		m_lastOffsetBeforeElev = offset;
		
		// Add up and down camera tilt
		{
			offset.z -= verticalChange;
			m_rotation.x += dt * m_stickSensitivity.x * m_rotatingAction.x;
			m_rotation.x = clamp(m_rotation.x, -maxRotation.x, +maxRotation.x);

			float elevAngleVehicle = m_inheritedElev * yAxis.z; 		// yAxis.z == approx elevation angle

			float elevationAngle = m_rotation.x - elevAngleVehicle;

			float sinElev, cosElev;
			cry_sincosf(elevationAngle, &sinElev, &cosElev);
			float horizLen = sqrtf(offset.GetLengthSquared2D());
			float horizLenNew = horizLen * cosElev - sinElev * offset.z;
			if (horizLen>1e-4f)
			{
				horizLenNew /= horizLen;
				offset.x *= horizLenNew;
				offset.y *= horizLenNew;
				offset.z = offset.z * cosElev + sinElev * horizLen;
			}
			offset.z += verticalChange;
		}

		if (!offset.IsValid()) offset = m_lastOffset;

		m_position = m_lookAt + offset;

		// intersection check...
		{
			IPhysicalEntity* pSkipEntities[10];
			int nSkip = 0;
			if(m_pVehicle)
			{
				nSkip = m_pVehicle->GetSkipEntities(pSkipEntities, 10);
			}

			primitives::sphere sphere;
			sphere.center = m_lookAt;
			sphere.r = g_CameraRadius;
			Vec3 dir = m_position-m_lookAt;

			geom_contact *pContact = 0;          
			float hitDist = gEnv->pPhysicalWorld->PrimitiveWorldIntersection(sphere.type, &sphere, dir, ent_static|ent_terrain|ent_rigid|ent_sleeping_rigid,
					&pContact, 0, (geom_colltype_player<<rwi_colltype_bit) | rwi_stop_at_pierceable, 0, 0, 0, pSkipEntities, nSkip);
			if(hitDist > 0.0f)
			{
				// Interpolate the offset
				Vec3 newPos = m_lookAt + hitDist * dir.GetNormalizedSafe();
				offset = newPos - m_lookAt;
			}
		}
				
		Interpolate(m_lastOffset, offset, 10.f, dt);

		m_position = m_lookAt + m_lastOffset;
	}
	else
	{
		CRY_ASSERT_MESSAGE(0, "camera will fail because lookat position is invalid");
	}

	//float sensitivity = (m_pSensitivity) ? m_pSensitivity->GetFVal() : 1.0f;
	m_rotatingAction.zero();
}

//------------------------------------------------------------------------
void CVehicleViewSteer::UpdateView(SViewParams &viewParams, EntityId playerId)
{
	static bool doUpdate = true;

	if (!doUpdate) return;

	if (m_position.IsValid())
	{
		viewParams.position = m_position;
	}
	else
	{
		CRY_ASSERT_MESSAGE(0, "camera position invalid");
	}

	Vec3 dir = (m_lookAt - m_position).GetNormalizedSafe();
	if (dir.IsValid() && dir.GetLengthSquared() > 0.01f)
	{
		viewParams.rotation = Quat::CreateRotationVDir(dir);
	}
	else
	{
		CRY_ASSERT_MESSAGE(0, "camera rotation invalid");
	}

	// set view direction on actor
	IActor* pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId);
	if(pActor && pActor->IsClient())
	{
		pActor->SetViewInVehicle(viewParams.rotation);
	}

	viewParams.nearplane = g_CameraRadius + 0.1f;
}

//------------------------------------------------------------------------
void CVehicleViewSteer::Serialize(TSerialize serialize, EEntityAspects aspects)
{
	CVehicleViewGameBase::Serialize(serialize, aspects);

	if (serialize.GetSerializationTarget() != eST_Network)
	{
		serialize.Value("position", m_position);
	}
}


DEFINE_VEHICLEOBJECT(CVehicleViewSteer);
