////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   UiMotionParamMappings.h
//  Version:     v1.00
//  Created:     05/08/2009 by Pau Novau
//  Description: Container that creates and stores mappings between ui and
//               motion params.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Ui_Motion_Param_Mappings__h__
#define __Ui_Motion_Param_Mappings__h__
#pragma once

#include "LMGEditor/IUiMotionParamMapping.h"
#include <ICryAnimation.h>

class CTestJoystickControl;
class CTestSliderControl;

class CUiMotionParamMappings
{
public:
	CUiMotionParamMappings();
	virtual ~CUiMotionParamMappings();

	void CreateMapping( EMotionParamID paramId, IUiMotionParamMapping::Mapping mapping, CTestSliderControl* pSliderControl );
	void CreateMapping( EMotionParamID paramId, IUiMotionParamMapping::Mapping mapping, CTestJoystickControl* pJoystickControl, float lengthMin, float lengthMax );

	void RemoveMapping( EMotionParamID paramId );
	void ClearMappings();

	IUiMotionParamMapping* GetMapping( EMotionParamID paramId );

protected:
	void SetMapping( EMotionParamID paramId, IUiMotionParamMapping* pMotionParamMapping );

private:
	IUiMotionParamMapping* m_uiMotionParamMappings[ eMotionParamID_COUNT ];

};

#endif
