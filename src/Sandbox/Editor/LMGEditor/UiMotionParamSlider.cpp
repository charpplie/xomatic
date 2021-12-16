////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   UiMotionParamSlider.cpp
//  Version:     v1.00
//  Created:     09/09/2009 by Pau Novau
//  Description: Maps Ui values from a slider to a motion param.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "..\LMGEditor/UiMotionParamSlider.h"

#include "..\LMGEditor/TestSliderControl.h"


CUiMotionParamSlider::CUiMotionParamSlider( Mapping mapping, CTestSliderControl* pSliderControl )
: m_mapping( mapping )
, m_pSliderControl( pSliderControl )
{
	assert( m_pSliderControl != NULL );
}


float CUiMotionParamSlider::GetValue( float minParamValue, float maxParamValue, float minAssetValue, float maxAssetValue ) const
{
	float paramRange = maxParamValue - minParamValue;
	float assetRange = maxAssetValue - minAssetValue;

	switch ( m_mapping )
	{
	case X:
		return m_pSliderControl->GetValue().x;

	case X_TIMES_PARAM_RANGE:
		return minParamValue + ( m_pSliderControl->GetValue().x * paramRange );

	case X_TIMES_ASSET_RANGE:
		return minAssetValue + ( m_pSliderControl->GetValue().x * assetRange );

	case MAP_X_TO_PARAM_RANGE:
		return minParamValue + ( m_pSliderControl->GetValueRelativeToRange().x * paramRange );

	case MAP_X_TO_ASSET_RANGE:
		return minAssetValue + ( m_pSliderControl->GetValueRelativeToRange().x * assetRange );

	case Y:
		return m_pSliderControl->GetValue().y;

	case Y_TIMES_PARAM_RANGE:
		return minParamValue + ( m_pSliderControl->GetValue().y * paramRange );

	case Y_TIMES_ASSET_RANGE:
		return minAssetValue + ( m_pSliderControl->GetValue().y * assetRange );

	case MAP_Y_TO_PARAM_RANGE:
		return minParamValue + ( m_pSliderControl->GetValueRelativeToRange().y * paramRange );

	case MAP_Y_TO_ASSET_RANGE:
		return minAssetValue + ( m_pSliderControl->GetValueRelativeToRange().y * assetRange );

	default:
		break;
	}

	return 0;
}
