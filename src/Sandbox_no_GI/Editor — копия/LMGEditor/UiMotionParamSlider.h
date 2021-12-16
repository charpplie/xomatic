////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   UiMotionParamSlider.h
//  Version:     v1.00
//  Created:     09/09/2009 by Pau Novau
//  Description: Maps Ui values from a slider to a motion param.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Ui_Motion_Param_Slider__h__
#define __Ui_Motion_Param_Slider__h__
#pragma once

#include "LMGEditor/IUiMotionParamMapping.h"

class CTestSliderControl;

class CUiMotionParamSlider
	: public IUiMotionParamMapping
{
public:
	CUiMotionParamSlider( Mapping mapping, CTestSliderControl* pSliderControl );

	virtual float GetValue( float minParamValue, float maxParamValue, float minAssetValue, float maxAssetValue ) const;

private:
	Mapping m_mapping;
	CTestSliderControl* m_pSliderControl;
};


#endif
