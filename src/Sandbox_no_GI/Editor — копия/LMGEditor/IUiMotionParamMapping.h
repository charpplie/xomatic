////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   IUiMotionParamMapping.h
//  Version:     v1.00
//  Created:     09/09/2009 by Pau Novau
//  Description: Interface for mapping Ui values to motion param values.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __I_Ui_Motion_Param_Mapping__h__
#define __I_Ui_Motion_Param_Mapping__h__
#pragma once

struct IUiMotionParamMapping
{
	typedef enum
	{
		X,
		X_TIMES_PARAM_RANGE,
		MAP_X_TO_PARAM_RANGE,
		X_TIMES_ASSET_RANGE,
		MAP_X_TO_ASSET_RANGE,
		Y,
		Y_TIMES_PARAM_RANGE,
		MAP_Y_TO_PARAM_RANGE,
		Y_TIMES_ASSET_RANGE,
		MAP_Y_TO_ASSET_RANGE,
		LENGTH_TIMES_PARAM_RANGE,
		LENGTH_TIMES_ASSET_RANGE,
		LENGTH_TIMES_RANGE,
		ANGLE_Y_AXIS
	} Mapping;

	virtual float GetValue( float minParamValue, float maxParamValue, float minAssetValue, float maxAssetValue ) const = 0;
};


#endif
