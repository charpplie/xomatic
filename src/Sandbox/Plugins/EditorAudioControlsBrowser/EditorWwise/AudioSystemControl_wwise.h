// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include <IAudioConnection.h>
#include <IAudioSystemControl.h>

namespace AudioControls
{
	enum EWwiseControlTypes
	{
		eWCT_INVALID = 0,
		eWCT_WWISE_EVENT = 1,
		eWCT_WWISE_RTPC = 2,
		eWCT_WWISE_SWITCH = 4,
		eWCT_WWISE_AUX_BUS = 8,
		eWCT_WWISE_SOUND_BANK = 16
	}; 

	class IAudioSystemControl_wwise : public IAudioSystemControl
	{
	public:
		IAudioSystemControl_wwise() {}
		IAudioSystemControl_wwise(const string& name, CID id, TImplControlType type);
		virtual ~IAudioSystemControl_wwise() {}
	};
}