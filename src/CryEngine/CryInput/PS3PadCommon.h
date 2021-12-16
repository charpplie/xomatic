/********************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006-2008.
---------------------------------------------------------------------
File name:   PS3PadCommon.h
Description: Common items for JoyPad for PS3
---------------------------------------------------------------------
History:
- 05:05:2008 : Created by Kevin Kirst

*********************************************************************/

#ifndef __PS3PADCOMMON_H__
#define __PS3PADCOMMON_H__

//this enables toggled keys that do not exists: toggled when threshold of x/y/z-rotation axis is crossed
//#define USE_LEAN_workaround

//deadzone used for analog stick position (must move beyond this amount to be reported)
#define PS3_STICK_DEADZONE (24)
#define fPS3_STICK_DEADZONE (24.0f / 128.0f)	//Should be kept inline with above value. Above value not used to avoid int -> float

// libusbpad supported pad mapping
static const int sPadMappings[40] =
{
	6, 7, 4, 5, 3, 7, 3, 6, 3, 5, 3, 4, 3, 2, 3, 3, 3, 0, 3, 1, 2, 0, 2, 3, 2, 1, 2, 2, 2, 4, 2, 5, 2, 6, 2, 7, 20, 21, 22, 23,
};

// define the device id's (they are the offsets into the button array
#define PS3_BUTTON_SELECT 	8
#define PS3_BUTTON_L3				10
#define PS3_BUTTON_R3				11
#define PS3_BUTTON_START		9
#define PS3_BUTTON_UP				12
#define PS3_BUTTON_RIGHT		13
#define PS3_BUTTON_DOWN			14
#define PS3_BUTTON_LEFT			15
#define PS3_BUTTON_L2				6
#define PS3_BUTTON_R2				7
#define PS3_BUTTON_L1				4
#define PS3_BUTTON_R1				5
#define PS3_BUTTON_TRIANGLE	3
#define PS3_BUTTON_CIRCLE		2
#define PS3_BUTTON_CROSS		1
#define PS3_BUTTON_SQUARE		0
#define PS3_STICK_LEFT_X		16
#define PS3_STICK_RIGHT_X		17
#define PS3_STICK_LEFT_Y		18
#define PS3_STICK_RIGHT_Y		19
#define PS3_ROT_X						20
#define PS3_ROT_Y						21
#define PS3_ROT_Z						22
#define PS3_ROTX_KEY_L			23
#define PS3_ROTX_KEY_R			24
#define PS3_ROTZ_KEY_D			25
#define PS3_ROTZ_KEY_U			26

#if defined(USE_LEAN_workaround)
	//when this threshold is crossed, a not in real existing key is toggled 
	//	this way jump/leaning can be implemented by the controller rotation feature
	#define LEAN_THRESHOLD 0.5f
	#define LEAN_THRESHOLD_RESET 0.3f
#endif

#define NORMALIZE_INPUT_AFTER_DEADZONE 1

namespace
{
	ILINE const float MapPadValue256(const int cInput)
	{
#if NORMALIZE_INPUT_AFTER_DEADZONE
		//This code avoids the sudden jump from 0 to a higher than expected value, providing smoother control input
		const float fRawInput = ((float)cInput / 127.5f) - 1.f;
		const float fSign = (float)__fsel(fRawInput, 1.0f, -1.0f);
		const float fInputDeadZone = max(fabsf(fRawInput) - fPS3_STICK_DEADZONE, 0.0f);
		const float fInvDeadzone = 1.0f - fPS3_STICK_DEADZONE;

		const float fNewInput = fInputDeadZone * fSign / fInvDeadzone;

		return fNewInput;
#else
		return (abs(128-cInput)>PS3_STICK_DEADZONE ? ((float)cInput / 127.5f) - 1.f : 0.f);
#endif
	}

	ILINE const float MapPadValue1024(const int cInput)
	{
		return (abs(512-cInput)>PS3_STICK_DEADZONE ? (float)(cInput - 512) / 127.5f : 0.f);
	}
}

#endif //__PS3PADCOMMON_H__