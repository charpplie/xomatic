#############################################################################
## Crytek Source File
## Copyright (C) 2006, Crytek Studios
##
## Creator: Sascha Demetrio
## Date: Jul 31, 2006
## Description: GNU-make based build system
#############################################################################

PROJECT_TYPE := module
PROJECT_VCPROJ := CryPhysics.vcproj

-include $(PROJECT_CODE)/Project_override.mk

PROJECT_CPPFLAGS_COMMON += \
	-I$(CODE_ROOT)/CryEngine/CryCommon

ifeq ($(OPTION_PROFILE),1)
  ifneq ($(MKOPTION_SNC),1)
    PROJECT_XFLAGS += -O2 $(PS3_MATHOPT)
  else
    PROJECT_XFLAGS += -O3 -Xrelaxalias=3 -Xfastmath=0 --diag_suppress=285 --diag_suppress=176\
      --diag_suppress=109
  endif  
endif

ifeq ($(MKOPTION_UNITYBUILD),1)
PROJECT_SOURCES_CPP_REMOVE := StdAfx.cpp\
	CryPhysics.cpp \
	aabbtree.cpp \
	articulatedentity.cpp \
	boolean2d.cpp \
	boolean3d.cpp \
	boxgeom.cpp \
	capsulegeom.cpp \
	cylindergeom.cpp \
	geoman.cpp \
	geometry.cpp \
	heightfieldbv.cpp \
	heightfieldgeom.cpp \
	intersectionchecks.cpp \
	jobq.cpp \
	rwi.cpp \
	linunprojectionchecks.cpp \
	livingentity.cpp \
	matrixnm.cpp \
	obbtree.cpp \
	overlapchecks.cpp \
	particleentity.cpp \
	physarea.cpp \
	physicalentity.cpp \
	physicalplaceholder.cpp \
	physicalworld.cpp \
	qhull.cpp \
	raybv.cpp \
	raygeom.cpp \
	rigidbody.cpp \
	rigidentity.cpp \
	ropeentity.cpp \
	rotunprojectionchecks.cpp \
	singleboxtree.cpp \
	softentity.cpp \
	spheregeom.cpp \
	tetrlattice.cpp \
	trimesh.cpp \
	utils.cpp \
	voxelbv.cpp \
	voxelgeom.cpp \
	waterman.cpp \
	wheeledvehicleentity.cpp \
	worldump.cpp

PROJECT_SOURCES_CPP_ADD += CryPhysicsSPU.cpp\
  CryPhysics_uber.cpp
endif

# Disabled for cm_backend builds for now 
ifneq ($(MKOPTION_CRYCG_CM),1)
PROJECT_SCAN_CPP := CryPhysicsSPU.cpp
endif

# vim:ts=8:sw=8