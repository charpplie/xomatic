/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements amphibious movement type 

-------------------------------------------------------------------------
History:
- 13:06:2007: Created by MichaelR

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"
#include "GameCVars.h"
#include <GameUtils.h>

#include "IVehicleSystem.h"
#include "VehicleMovementAmphibious.h"


#define THREAD_SAFE 1

//------------------------------------------------------------------------
CVehicleMovementAmphibious::CVehicleMovementAmphibious()
{  
  m_boat.m_bNetSync = false; 
}

//------------------------------------------------------------------------
CVehicleMovementAmphibious::~CVehicleMovementAmphibious()
{
}

//------------------------------------------------------------------------
bool CVehicleMovementAmphibious::Init(IVehicle* pVehicle, const CVehicleParams& table)
{
  CVehicleParams stdWheeledTable = table.findChild("StdWheeled");
	CVehicleParams stdBoatTable = table.findChild("StdBoat");
  if (stdWheeledTable && stdBoatTable )
	{
		if (!CVehicleMovementStdWheeled::Init(pVehicle, stdWheeledTable))
			return false;

		if (!m_boat.Init(pVehicle, stdBoatTable))
			return false;

		return true;
	}
    
  return false;
}

//------------------------------------------------------------------------
void CVehicleMovementAmphibious::PostInit()
{
  CVehicleMovementStdWheeled::PostInit();
  m_boat.PostInit();
}


//------------------------------------------------------------------------
void CVehicleMovementAmphibious::Reset()
{
  CVehicleMovementStdWheeled::Reset();
  m_boat.Reset();
}


//------------------------------------------------------------------------
void CVehicleMovementAmphibious::PostPhysicalize()
{
  CVehicleMovementStdWheeled::PostPhysicalize();
  m_boat.PostPhysicalize();
}


//------------------------------------------------------------------------
bool CVehicleMovementAmphibious::StartEngine(EntityId driverId)
{
  if (!CVehicleMovementStdWheeled::StartEngine(driverId))
    return false;

  m_boat.StartEngine(driverId);

  return true;
}

//------------------------------------------------------------------------
void CVehicleMovementAmphibious::StopEngine()
{
  CVehicleMovementStdWheeled::StopEngine();
  m_boat.StopEngine();
}

//------------------------------------------------------------------------
void CVehicleMovementAmphibious::DisableEngine(bool disable)
{
  CVehicleMovementStdWheeled::DisableEngine(disable);
  m_boat.DisableEngine(disable);
}

//------------------------------------------------------------------------
void CVehicleMovementAmphibious::OnAction(const TVehicleActionId actionId, int activationMode, float value)
{
  CVehicleMovementStdWheeled::OnAction(actionId, activationMode, value);  
}

//------------------------------------------------------------------------
void CVehicleMovementAmphibious::OnEvent(EVehicleMovementEvent event, const SVehicleMovementEventParams& params)
{
  CVehicleMovementStdWheeled::OnEvent(event, params);
  m_boat.OnEvent(event, params);  
}

//------------------------------------------------------------------------
void CVehicleMovementAmphibious::OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params)
{
  CVehicleMovementStdWheeled::OnVehicleEvent(event, params);
  m_boat.OnVehicleEvent(event, params);
}

//------------------------------------------------------------------------
void CVehicleMovementAmphibious::Update(const float deltaTime)
{
  CVehicleMovementStdWheeled::Update(deltaTime);
  m_boat.Update(deltaTime);  
}

//------------------------------------------------------------------------
void CVehicleMovementAmphibious::UpdateRunSound(const float deltaTime)
{
  CVehicleMovementStdWheeled::UpdateRunSound(deltaTime);  

  if (m_pVehicle->GetGameObject()->IsProbablyDistant())
    return;

  SetSoundParam(eSID_Run, "swim", m_statusDyn.submergedFraction);

  if (Boosting())
    SetSoundParam(eSID_Boost, "swim", m_statusDyn.submergedFraction);
}


//////////////////////////////////////////////////////////////////////////
// NOTE: This function must be thread-safe. Before adding stuff contact MarcoC.
void CVehicleMovementAmphibious::ProcessMovement(const float deltaTime)
{  
  CVehicleMovementStdWheeled::ProcessMovement(deltaTime);    
    
  if (Submerged())
  {
    // assign movement action to boat (serialized by wheeled movement)
    m_boat.m_movementAction = m_movementAction;      
    m_boat.ProcessMovement(deltaTime);
  }  
}

//------------------------------------------------------------------------
void CVehicleMovementAmphibious::Serialize(TSerialize ser, EEntityAspects aspects) 
{
  CVehicleMovementStdWheeled::Serialize(ser, aspects);  
  
  if (ser.GetSerializationTarget() != eST_Network)
  {
    m_boat.Serialize(ser, aspects);
  }
}

//------------------------------------------------------------------------
void CVehicleMovementAmphibious::PostSerialize()
{
  CVehicleMovementStdWheeled::PostSerialize();
  m_boat.PostSerialize();
}

//------------------------------------------------------------------------
void CVehicleMovementAmphibious::ProcessEvent(SEntityEvent& event)
{  
  CVehicleMovementStdWheeled::ProcessEvent(event);
  m_boat.ProcessEvent(event);
}


//------------------------------------------------------------------------
void CVehicleMovementAmphibious::Boost(bool enable)
{
  CVehicleMovementStdWheeled::Boost(enable);  
  m_boat.Boost(enable);
}


void CVehicleMovementAmphibious::GetMemoryStatistics(ICrySizer * s)
{
  s->Add(*this);
}