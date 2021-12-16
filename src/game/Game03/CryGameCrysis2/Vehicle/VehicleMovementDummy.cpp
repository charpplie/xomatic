/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a dummy vehicle movement, for prop vehicles

-------------------------------------------------------------------------
History:
- 05:03:2010: Created by Steve Humphreys

*************************************************************************/

#include "StdAfx.h"
#include "VehicleMovementDummy.h"

#include "GameCVars.h"

bool CVehicleMovementDummy::Init(IVehicle* pVehicle, const CVehicleParams& table)
{
	m_pVehicle = pVehicle;
	return true;
}

void CVehicleMovementDummy::Physicalize()
{
	SEntityPhysicalizeParams physicsParams(m_pVehicle->GetPhysicsParams());	

	physicsParams.type = PE_WHEELEDVEHICLE;	  

	pe_params_car carParams;
	carParams.nWheels = m_pVehicle->GetWheelCount();
	physicsParams.pCar = &carParams;

	m_pVehicle->GetEntity()->Physicalize(physicsParams);

	IPhysicalEntity *pPhysEnt = m_pVehicle->GetEntity()->GetPhysics();
	if (pPhysEnt && g_pGameCVars->pVehicleQuality->GetIVal()==1)
	{
		pe_params_flags pf; pf.flagsOR = wwef_fake_inner_wheels;
		pe_params_foreign_data pfd; pfd.iForeignFlagsOR = PFF_UNIMPORTANT;
		pPhysEnt->SetParams(&pf);
		pPhysEnt->SetParams(&pfd);
	}
}