#############################################################################
## Crytek Source File
## Copyright (C) 2006, Crytek Studios
##
## Creator: Sascha Demetrio
## Date: Jul 31, 2006
## Description: GNU-make based build system
#############################################################################

PROJECT_TYPE := module
PROJECT_VCPROJ := CrySoundSystem.vcproj

-include $(PROJECT_CODE)/Project_override.mk

PROJECT_CPPFLAGS_COMMON += \
	-I$(CODE_ROOT)/CryEngine/CryCommon

fmodex_libdir := $(PROJECT_CODE)/FmodEx/lib/ps3

ifeq ($(MKOPTION_UNITYBUILD),1)
PROJECT_SOURCES_CPP_REMOVE := StdAfx.cpp\
	AudioDeviceFmodEx400.cpp \
	AudioDeviceXenon.cpp \
	CommandPlayerFmodEx400.cpp \
	CrySoundSystem.cpp \
	DebugLogger.cpp \
	Microphone.cpp \
	MicrophoneStream.cpp \
	MoodManager.cpp \
	MusicSystem/Decoder/ADPCMDecoder.cpp \
	MusicSystem/Decoder/FMODBankDecoder.cpp \
	MusicSystem/Decoder/OGGDecoder.cpp \
	MusicSystem/Decoder/PCMDecoder.cpp \
	MusicSystem/MusicCVars.cpp \
	MusicSystem/MusicSystem.cpp \
	MusicSystem/Pattern/MusicPattern.cpp \
	MusicSystem/Pattern/MusicPatternInstance.cpp \
	MusicSystem/RandGen/RandGen.cpp \
	PlatformSoundFmodEx400.cpp \
	PlatformSoundFmodEx400Event.cpp \
	PlatformSoundXenon.cpp \
	ReverbInstance_Classic_Reverb.cpp \
	ReverbInstance_FreeVerb.cpp \
	ReverbInstance_Princeton2016.cpp \
	ReverbInstance_RoomMachine844.cpp \
	ReverbManager.cpp \
	ReverbManagerDSP.cpp \
	ReverbManagerEAX.cpp \
	Sound.cpp \
	SoundAssetManager.cpp \
	SoundBuffer.cpp \
	SoundBufferFmodEx400.cpp \
	SoundBufferFmodEx400Event.cpp \
	SoundBufferFmodEx400Micro.cpp \
	SoundBufferFmodEx400Network.cpp \
	SoundBufferXenon.cpp \
	SoundSystem.cpp \
	SoundSystemCommon.cpp \
	XenonUtil/AtgApp.cpp \
	XenonUtil/AtgAudio.cpp \
	XenonUtil/AtgUtil.cpp

PROJECT_SOURCES_CPP_ADD += CrySoundSystem_uber.cpp

endif

# Disabled for cm_backend builds for now 
ifneq ($(MKOPTION_CRYCG_CM),1)
PROJECT_SCAN_CPP := MusicSystem/MusicSystem.cpp
endif

# vim:ts=8:sw=8