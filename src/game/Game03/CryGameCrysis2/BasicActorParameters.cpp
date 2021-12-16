//---------------------------------------
// Basic actor parameters structure, which stores game parameters (physics, stance, movement...) 
//
// 15-09-2008 Benito G.R.
// ----------------------------------------

#include "StdAfx.h"
#include "BasicActorParameters.h"
#include <ItemParams.h>
#include "ItemParamReader.h"

#undef ReadValue
#define ReadValue(param)	reader.Read(#param, param)

//--------------------------------------------------------
void SBasicActorLivingParams::ReadParams(const IItemParamsNode *params)
{
	if(!params)
		return;

	CItemParamReader reader(params);

	ReadValue(gravity);
	ReadValue(mass);
	ReadValue(player_mass);
	ReadValue(air_resistance);
	ReadValue(k_air_control);
	ReadValue(max_vel_ground);
	ReadValue(min_slide_angle);
	ReadValue(max_climb_angle);
	ReadValue(min_fall_angle);
	ReadValue(time_impulse_recover);
	ReadValue(inertia);
	ReadValue(inertia_accel);
	ReadValue(collider_mat);
}

//------------------------------------------------------
void SBasicActorPhysicParams::ReadParams(const IItemParamsNode *params)
{
	if(!params)
		return;

	CItemParamReader reader(params);

	ReadValue(flags);
	ReadValue(mass);
	ReadValue(player_mass);
	ReadValue(stiffness_scale);

	livingParams.ReadParams(params->GetChild("LivingParams"));
}
#undef ReadValue
