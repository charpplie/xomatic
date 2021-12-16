#############################################################################
## Crytek Source File
## Copyright (C) 2006, Crytek Studios
##
## Creator: Sascha Demetrio
## Date: Jul 31, 2006
## Description: GNU-make based build system
#############################################################################

PROJECT_TYPE := module
PROJECT_VCPROJ := CryAnimation.vcproj

-include $(PROJECT_CODE)/Project_override.mk

PROJECT_CPPFLAGS_COMMON := \
	-I$(CODE_ROOT)/CryEngine/CryCommon

ifeq ($(MKOPTION_UNITYBUILD),1)
PROJECT_SOURCES_CPP_REMOVE := stdafx.cpp\
	AnimAction/AALocomotion.cpp \
	AnimAction/AARandom.cpp \
	AnimAction/AASequence.cpp \
	AnimAction/AnimActionActor.cpp \
	AnimAction/AnimActionManager.cpp \
	FacialAnimation/EyeMovementFaceAnim.cpp \
	FacialAnimation/FaceAnimSequence.cpp \
	FacialAnimation/FaceAnimation.cpp \
	FacialAnimation/FaceChannelKeyCleanup.cpp \
	FacialAnimation/FaceChannelSmoothing.cpp \
	FacialAnimation/FaceEffector.cpp \
	FacialAnimation/FaceEffectorLibrary.cpp \
	FacialAnimation/FaceJoystick.cpp \
	FacialAnimation/FaceState.cpp \
	FacialAnimation/FacialInstance.cpp \
	FacialAnimation/FacialModel.cpp \
	FacialAnimation/LipSync.cpp \
	AnimEventLoader.cpp \
	AnimationBase.cpp \
	AnimationManager.cpp \
	AnimationThreadTask.cpp \
	AttachmentManager.cpp \
	CalParser.cpp \
	CharacterInstance.cpp \
	CharacterManager.cpp \
	ControllerPQ.cpp \
	ControllerPQLog.cpp \
	ControllerTCB.cpp \
	CryModEffMorph.cpp \
	Command_Commands.cpp \
	Command_Buffer.cpp \
	DecalManager.cpp \
	LMG_Parse.cpp\
	LoaderCAF.cpp \
	LoaderCGA.cpp \
	LoaderCHR.cpp \
	LoaderDBA.cpp \
	Model.cpp \
	ModelAimIK.cpp \
	ModelAnimationSet.cpp \
	ModelMesh.cpp \
	ModelSkeleton.cpp \
	Morphing.cpp \
	PathExpansion.cpp \
	PrototypeCode.cpp \
	SkinInstance.cpp \
	_Render.cpp \
	_RenderSkins.cpp \
	cvars.cpp \
	LMG_ComputeWeight.cpp \
	LMG_ExtractParameters.cpp \
	LMG_GetCapabilities.cpp \
	PoseModifier/FeetLock.cpp\
	PoseModifier/PoseBlenderAim.cpp \
	PoseModifier/PoseBlenderAim2.cpp \
	PoseModifier/IKTorsoAim.cpp \
	PoseModifier/PoseModifierHelper.cpp \
	PoseModifier/LookAt.cpp\
	SDI.cpp \
	Skeleton.cpp \
	SkeletonAnim.cpp \
	SkeletonAnim_BlendMan.cpp \
	SkeletonAnim_Params.cpp \
	SkeletonAnim_Queue.cpp \
	SkeletonAnim_Commands.cpp \
	SkeletonEffectManager.cpp \
	SkeletonPose.cpp \
	SkeletonPose_Debug.cpp \
	SkeletonPose_FA.cpp \
	SkeletonPose_IK2B.cpp \
	SkeletonPose_IKAim.cpp \
	SkeletonPose_IKArmEx.cpp \
	SkeletonPose_IKCCD.cpp \
	SkeletonPose_IKLook.cpp \
	SkeletonPose_Physics.cpp \
	SkeletonPose_Process.cpp \
	wavelets/CompressedData.cpp \
	wavelets/Compression.cpp \
	wavelets/DaubechiWavelet.cpp \
	wavelets/MeyerWavelet.cpp \
	wavelets/Wavelet.cpp

PROJECT_SOURCES_CPP_ADD += PS3PPU_AnimationExt.cpp\
  PS3PPU_Animation.cpp
endif

# Disabled for cm_backend builds for now 
ifneq ($(MKOPTION_CRYCG_CM),1)
PROJECT_SCAN_CPP := PS3SPU_CommandBuffer.cpp
endif 

ifneq ($(wildcard $(PROJECT_DIR)/Project_override.mk),)
  include $(PROJECT_DIR)/Project_override.mk
endif

# vim:ts=8:sw=8

