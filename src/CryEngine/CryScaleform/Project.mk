
#############################################################################
## Crytek Source File
## Copyright (C) 2006, Crytek Studios
##
## Creator: Sascha Demetrio
## Date: Jul 31, 2006
## Description: GNU-make based build system
#############################################################################

PROJECT_TYPE := module
PROJECT_VCPROJ := CryScaleform.vcproj

-include $(PROJECT_CODE)/Project_override.mk
-include $(PROJECT_CODE)/build_ver.mk

PROJECT_CPPFLAGS += \
	-I$(CODE_ROOT)/CryEngine/CryCommon \
	-I$(CODE_ROOT)/SDKs/Scaleform/Include \
	-I$(CODE_ROOT)/SDKs \
	-I$(PROJECT_CODE)/XML/Expat \
	-I$(PROJECT_CODE)/zlib

ifeq ($(OPTION_PROFILE),1)
	ifneq ($(MKOPTION_SNC),1)
		ifeq ($(findstring Linux,$(ARCH)),Linux)
			PROJECT_XFLAGS += $(ARCH_XFLAGS)
		else
			PROJECT_XFLAGS += -O2 $(PS3_MATHOPT)
		endif
	else
		PROJECT_XFLAGS += -O3 -Xrelaxalias=3 --diag_suppress=227 --diag_suppress=1628 $(PS3_MATHOPT)
	endif
endif

PROJECT_CPPFLAGS_COMMON += \
	-I$(CODE_ROOT)/CryEngine/CryCommon\
	-I$(CODE_ROOT)/Tools/PS3JobManager

ifeq ($(MKOPTION_UNITYBUILD),1)
PROJECT_SOURCES_CPP_REMOVE := StdAfx.cpp \
	../CryCommon/EngineSettingsManager.cpp \
	../CryCommon/ResourceCompilerHelper.cpp \
	Scaleform/ConfigScaleform.cpp \
	Scaleform/FlashPlayerInstance.cpp \
	Scaleform/GAllocatorCryMem.cpp \
	Scaleform/GFileCryPak.cpp \
	Scaleform/GImageInfoXRender.cpp \
	Scaleform/GRendererXRender.cpp \
	Scaleform/GTextureXRender.cpp \
	Scaleform/SharedResources.cpp \
	Scaleform/SharedStates.cpp \
	zlib/zlib_spu.cpp


PROJECT_SOURCES_CPP_ADD += \
	CryScaleform_main_uber.cpp
endif

ifeq ($(ARCH),PS3-cell)
  PROJECT_SOURCES_C_REMOVE += zlib/%
endif

#ifeq ($(findstring Linux,$(ARCH)),Linux)
 # used to syncronize Win32 host with consoles
# PROJECT_SOURCES_CPP_REMOVE := NotificationNetwork.cpp
#endif

#PROJECT_SCAN_CPP := zlib/zlib_spu.cpp

# vim:ts=8:sw=8
