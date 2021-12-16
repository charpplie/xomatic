////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:  MotionAdaptorManager.cpp
//  Version:    v1.00
//  Created:    24/4/2009 by Xiaomao Wu.
//  Compiler:   Visual Studio 2005 Professional
//  Description:

// -------------------------------------------------------------------------
//  History:
//    29/05/09: Changed from MotionAdaptorMainDlg to MotionAdaptorManger
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "..\CharacterEditor/MotionAdaptorManager.h"
#include "..\CharacterEditor/ModelViewportCE.h"
#include "AnimEventEditor/AnimEventEditor.h"
#include "AnimEventEditor/AnimEventControlView.h"
#include <stack>
#include "..\CharacterEditor/AnimUtils/VfileParser.h"
#include "..\CharacterEditor/CharacterEditor.h"

const int32 DEFAULT_SPEED_SMOOTH_SIZE = 100;
const int32 SUCCESS = 1;
const int32 FAILED = -1;
const f32 BIPED_FRAME_RATE = 30.0f;

IMPLEMENT_DYNAMIC(CMotionAdaptorManager, CDialog)

CMotionAdaptorManager::CMotionAdaptorManager(CWnd* pParent /*=NULL*/)
{
	m_CurrentVFileDir = " ";
	m_animationLoaded = false;
	m_inputPelvisId = -1;
	m_frameCount = 0;
	m_speedSmoothSize = DEFAULT_SPEED_SMOOTH_SIZE;

	m_pModelViewportCE = NULL;
	m_pCharacterEditor = NULL;
	m_bPlayMotion = false;
	m_addFrameTime = 0.0;

	m_fFrameNoSmooth = 0.0;
	m_fFrameNoSmoothRate = 0.0;

	m_bUDraggingPlaySlider = false;
	m_SpeedInterval = 10;
	m_SamplingRatio = 1.0f;
	m_ViconFrameHertz = .0;
	sliderPos = 0;

	m_bShowViconMarkers = true;
	m_bShowViconSkeleton = true;
	m_bFixInverseBending = true;
	m_bMotionRetargeting = true;;
	m_bAdditiveMotion = true;
	m_bAdjustFoot = true;
}

CMotionAdaptorManager::~CMotionAdaptorManager()
{
}



void CMotionAdaptorManager::InitBipedSkeleton()
{
	m_BipSkeleton.clear();

	//--------------> BoneNr.0
	m_BipSkeleton.push_back( SBipedJoint("Bip01", -1));
	//--------------> BoneNr.1
	m_BipSkeleton.push_back( SBipedJoint("Bip01 Pelvis", -1));
	//--------------> BoneNr.2
	m_BipSkeleton.push_back( SBipedJoint("Bip01 Spine", -1));
	//--------------> BoneNr.3
	m_BipSkeleton.push_back( SBipedJoint("Bip01 Spine1", -1));
	//--------------> BoneNr.4
	m_BipSkeleton.push_back( SBipedJoint("Bip01 Spine2", -1));
	//--------------> BoneNr.5
	m_BipSkeleton.push_back( SBipedJoint("Bip01 Spine3", -1));


	//--------------> BoneNr.6
	m_BipSkeleton.push_back( SBipedJoint("Bip01 Neck", -1));
	//--------------> BoneNr.7
	m_BipSkeleton.push_back( SBipedJoint("Bip01 Neck1", -1));
	//--------------> BoneNr.8
	m_BipSkeleton.push_back( SBipedJoint("Bip01 Head", -1));
	//--------------> BoneNr.9
	m_BipSkeleton.push_back( SBipedJoint("Bip01 L Clavicle", -1));
	//--------------> BoneNr.10
	m_BipSkeleton.push_back( SBipedJoint("Bip01 L UpperArm", -1));
	//--------------> BoneNr.11
	m_BipSkeleton.push_back( SBipedJoint("Bip01 L Forearm", -1));
	//--------------> BoneNr.12
	m_BipSkeleton.push_back( SBipedJoint("Bip01 L Hand", -1));
	//--------------> BoneNr.13
	m_BipSkeleton.push_back( SBipedJoint("Bip01 R Clavicle", -1));
	//--------------> BoneNr.14
	m_BipSkeleton.push_back( SBipedJoint("Bip01 R UpperArm", -1));
	//--------------> BoneNr.15
	m_BipSkeleton.push_back( SBipedJoint("Bip01 R Forearm", -1));
	//--------------> BoneNr.16
	m_BipSkeleton.push_back( SBipedJoint("Bip01 R Hand", -1));
	//--------------> BoneNr.17
	m_BipSkeleton.push_back( SBipedJoint("Bip01 L Thigh", -1));
	//--------------> BoneNr.18
	m_BipSkeleton.push_back( SBipedJoint("Bip01 L Calf", -1));

	m_BipSkeleton.push_back( SBipedJoint("Bip01 L HorseLink", -1));
	//--------------> BoneNr.19
	m_BipSkeleton.push_back( SBipedJoint("Bip01 L Foot", -1));
	//--------------> BoneNr.20
	m_BipSkeleton.push_back( SBipedJoint("Bip01 L Toe0", -1));
	//--------------> BoneNr.21
	m_BipSkeleton.push_back( SBipedJoint("Bip01 R Thigh", -1));
	//--------------> BoneNr.22
	m_BipSkeleton.push_back( SBipedJoint("Bip01 R Calf", -1));

	m_BipSkeleton.push_back( SBipedJoint("Bip01 R HorseLink", -1));
	//--------------> BoneNr.23
	m_BipSkeleton.push_back( SBipedJoint("Bip01 R Foot", -1));
	//--------------> BoneNr.24
	m_BipSkeleton.push_back( SBipedJoint("Bip01 R Toe0", -1));

	//--------------> BoneNr.25
	m_BipSkeleton.push_back( SBipedJoint("Shadow L Toe0", -1));
	//--------------> BoneNr.26
	m_BipSkeleton.push_back( SBipedJoint("Shadow R Toe0", -1));
}


int32 CMotionAdaptorManager::Convert2Biped(const char* strFileName)
{
	if(!strFileName && !m_animationLoaded)
		return FAILED;

	// Vicon joints
	const SNodeDesc inputNodesSpec[] =	
	{	
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ), -1,  -1 },  // 0: Pelvis
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  0,  -1 },  // 1: Thorax
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  1,  -1 },	 // 2: Spine
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  2,  -1 },  // 3: Head
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  2,  -1 },	 // 4: Lclavicle
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  4,  -1 },  // 5: Lhumerus
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  5,  -1 },	 // 6: Lradius
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  6,  -1 },  // 7: Lhand
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  2,  -1 },	 // 8: Rclavicle
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  8,  -1 },  // 9: Rhumerus
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  9,  -1 },	 // 10: Rradius
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  10, -1 },  // 11: Rhand
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  0,  -1 },	 // 12: Lfemur
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  12, -1 },  // 13: Ltibia
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  13, -1 },	 // 14: Lfoot
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  14, -1 },  // 15: Ltoes
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  0,  -1 },	 // 16: Rfemur
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  16, -1 },  // 17: Rtibia
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  17, -1 },	 // 18: Rfoot
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  18, -1 }  // 19: Rtoes
	};

	// Biped joints
	SNodeDesc outputNodesSpec[] =	//removed const for testing (get default pose from file)
	{	
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1,  -1 },  // 0: Bip01
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1,  0  },  // 1: Bip01 Pelvis
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1,  -1 },	 // 2: Bip01 Spine
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1,  1 },  // 3: Bip01 Spine1
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1,  -1 },	 // 4: Bip01 Spine2
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1,  2 },  // 5: Bip01 Spine3
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1,  -1 },	 // 6: Bip01 Neck
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1,  -1 },  // 7: Bip01 Neck1
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1,  3 },	 // 8: Bip01 Head
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1,  4 },  // 9: Bip01 L Clavicle
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1,  5 },	 // 10: Bip01 L UpperArm
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1, 6 },  // 11: Bip01 L Forearm
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1,  7 },	 // 12: Bip01 L Hand
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1, 8 },  // 13: Bip01 R Clavicle
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1, 9 },	 // 14: Bip01 R UpperArm
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1, 10 },  // 15: Bip01 R Forearm
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1,  11 },	 // 16: Bip01 R Hand
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1, 12 },  // 17: Bip01 L Thigh
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1, 13 },	 // 18: Bip01 L Calf
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1, -1 },  // 19.1: Bip01 L HorseLink
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1, 14 },  // 19.2: Bip01 L Foot
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1, 15 },	 // 20: Bip01 L Toe0
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1, 16 },  // 21: Bip01 R Thigh
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1,  17 },	 // 22: Bip01 R Calf
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1, -1 },  // 23.1: Bip01 R HorseLink
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1, 18 },	 // 23.2: Bip01 R Foot
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1, 19 },  // 24: Bip01 R Toe0
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1, -1 },	 // 25: Shadow L Toe0
		{ QuatT(Vec3(.0f, .0f, .0f), Quat(1.0f, .0f, .0f, .0f) ),  -1, -1 },  // 26: Shadow R Toe0
	};


	const uint32 numInputNode = 20;
	const uint32 numOutputNode = 29;
	const int32 inputLfootId = 14;
	const int32 inputRfootId = 18;

	InitBipedSkeleton();

	// Get default poses from current character model
	GetDefaultPoses(outputNodesSpec, numOutputNode);

	// User should provide the pelvis id, for retargeting
	m_inputPelvisId = 0; // vicon pelvis id

	//------------------------------------------------------------------------------
	// These are the inputs we need to get from users

	// Need root joint id to calculate absolute poses
	const int32 outputRootJointId = GetBipedIDbyName("Bip01"); assert(outputRootJointId == 0);

	// Need pelvis id for motion regargeting
	const int32 outputPelvisID = GetBipedIDbyName("Bip01 Pelvis");assert(outputPelvisID == 1);

	// Need foot ids for feet adjustment (align with vicon markers)
	const int32 outputLfootId = GetBipedIDbyName("Bip01 L Foot"); assert(outputLfootId >= 0);
	const int32 outputRfootId = GetBipedIDbyName("Bip01 R Foot"); assert(outputRfootId >=0);

	// Need forearm ids to avoid inverse bending
	const int32 outputLForeArmId = GetBipedIDbyName("Bip01 L Forearm"); assert(outputLForeArmId >= 0);
	const int32 outputRForeArmId = GetBipedIDbyName("Bip01 R Forearm"); assert(outputRForeArmId >= 0);

	// Need calf ids to avoid inverse bending
	const int32 outputLCalfId = GetBipedIDbyName("Bip01 L Calf"); assert(outputLCalfId >= 0);
	const int32 outputRCalfId = GetBipedIDbyName("Bip01 R Calf"); assert(outputRCalfId >= 0);

	//------------------------------------------------------------------------------
	// Automatically search related joint Ids

	const int32 outputLForeArmChildId = GetFirstChildId(outputLForeArmId, outputNodesSpec, numOutputNode); assert(outputLForeArmChildId >=0);
	const int32 outputLForeArmParent_1 = outputNodesSpec[outputLForeArmId].parent; assert(outputLForeArmParent_1 >=0);
	const int32 outputLForeArmParent_2 = outputNodesSpec[outputLForeArmParent_1].parent; assert(outputLForeArmParent_2 >=0);
	
	const int32 outputRForeArmChildId = GetFirstChildId(outputRForeArmId, outputNodesSpec, numOutputNode); assert(outputRForeArmChildId >=0);
	const int32 outputRForeArmParent_1 = outputNodesSpec[outputRForeArmId].parent; assert(outputRForeArmParent_1 >=0);
	const int32 outputRForeArmParent_2 = outputNodesSpec[outputRForeArmParent_1].parent; assert(outputRForeArmParent_2 >=0);
	
	const int32 outputLCalfChild_1 = GetFirstChildId(outputLCalfId, outputNodesSpec, numOutputNode); assert(outputLCalfChild_1 >=0);
	const int32 outputLCalfChild_2 = GetFirstChildId(outputLCalfChild_1, outputNodesSpec, numOutputNode); assert(outputLCalfChild_2 >=0);
	const int32 outputLCalfParent = outputNodesSpec[outputLCalfId].parent; assert(outputLCalfParent >=0);

	const int32 outputRCalfChild_1 = GetFirstChildId(outputRCalfId, outputNodesSpec, numOutputNode); assert(outputRCalfChild_1 >=0);
	const int32 outputRCalfChild_2 = GetFirstChildId(outputRCalfChild_1, outputNodesSpec, numOutputNode); assert(outputRCalfChild_2 >=0);
	const int32 outputRCalfParent = outputNodesSpec[outputRCalfId].parent; assert(outputRCalfParent >=0);

	const int32 outputLfootChildId = GetFirstChildId(outputLfootId, outputNodesSpec, numOutputNode);	assert(outputLfootChildId >=0);
	const int32 outputRfootChildId = GetFirstChildId(outputRfootId, outputNodesSpec, numOutputNode);	assert(outputRfootChildId >=0);

	// Vicon motion
	std::vector< std::vector<QuatT> > pViconJoints(numInputNode);

	// Setup joint names and axis swappers
	SetupViconJointProperty(m_viconJointProperty);

	// Read Vfile if animation not loaded
	if(!m_animationLoaded)
	{
		m_viconJointsGlobal.clear();
		m_viconJointsGlobal.resize(numInputNode);

		if(SUCCESS == ReadVFile(strFileName, pViconJoints, m_viconJointsGlobal, inputNodesSpec, m_viconJointProperty, m_ViconMarkers, numInputNode, m_frameCount, m_ViconFrameHertz))
		{
			m_animationLoaded = true;
			m_viconJointsBackup = pViconJoints;
		}
		else
		{
			m_animationLoaded = false;
			return FAILED;
		}
	}
	else // V-file already loaded, load the backuped data
		pViconJoints = m_viconJointsBackup;

	// Setup name map for vicon markers
	std::map<string, int32> viconMarkerNameMap;
	for(int32 i=0; i< (int32)m_ViconMarkers.size(); ++i)
	{
		viconMarkerNameMap[ m_ViconMarkers[i].sViconMarker ] = i;
	}

	
	//------------------------------------------------------------------------------
	// Remappming from Vicon (input) to Biped (output)
	QuatT* singleFrameInput = new QuatT[numInputNode];
	QuatT* singleFrameOutput = new QuatT[numOutputNode];

	std::vector< std::vector<QuatT> > pBipedJoints(numOutputNode);
	for(int32 i=0; i< numOutputNode; ++i)
		pBipedJoints[i].resize(m_frameCount);

	// do the mapping
	for(int32 i=0; i< m_frameCount; ++i)
	{
		// fetch one frame from input stream
		for(int32 j=0; j< numInputNode; ++j)
		{
			singleFrameInput[j] = pViconJoints[j][i];
		}

		// mapping
		RemapFrom(singleFrameInput, singleFrameOutput, numOutputNode, outputNodesSpec);

		// save mapped frame
		for(int j=0; j< numOutputNode; ++j)
			pBipedJoints[j][i] = singleFrameOutput[j];
	}

	//delete memory

	delete[] singleFrameInput;
	singleFrameInput = NULL;

	delete[] singleFrameOutput;
	singleFrameOutput = NULL;

	//------------------------------------------------------------------------------
	// Resample Biped motion

	m_SamplingRatio = m_ViconFrameHertz / BIPED_FRAME_RATE;
	int32 finalFrameCount = (int32)(m_frameCount / m_SamplingRatio);
	
	m_sampledBipedJointsLocal.resize(numOutputNode);
	for(int32 i=0; i< numOutputNode; ++i)
	{
		m_sampledBipedJointsLocal[i].resize(finalFrameCount);
	}

	Resampling(pBipedJoints, m_sampledBipedJointsLocal, m_ViconMarkers, m_sampledViconMarkers, finalFrameCount, numOutputNode, m_ViconMarkers.size(), m_SamplingRatio);

	//------------------------------------------------------------------------------
	// Set local translations of Biped joints to the default ones.
	// Will keep pelvis's translations (has been set to the same as the vicon Pelvis)

	for(int32 i=0; i< numOutputNode; ++i)
	{
		for(int32 j=0; j< finalFrameCount; ++j)
		{
			if(i != outputPelvisID ) 
				m_sampledBipedJointsLocal[i][j].t = outputNodesSpec[i].defaultPose.t;
		}
	}

	//------------------------------------------------------------------------------
	// Calculate absolute poses

	CalcuAbsolutePoses(m_sampledBipedJointsLocal, m_sampledBipedJointsGlobal, outputPelvisID, outputRootJointId, numOutputNode, finalFrameCount, outputNodesSpec);

	//------------------------------------------------------------------------------
	// Fix inverse bending
	if(m_bFixInverseBending)
	{	
		std::vector<int32> outputLlegIds;
		outputLlegIds.push_back(outputLCalfParent);
		outputLlegIds.push_back(outputLCalfId);
		outputLlegIds.push_back(outputLCalfChild_1);
		outputLlegIds.push_back(outputLCalfChild_2);

		std::vector<int32> outputRlegIds;
		outputRlegIds.push_back(outputRCalfParent);
		outputRlegIds.push_back(outputRCalfId);
		outputRlegIds.push_back(outputRCalfChild_1);
		outputRlegIds.push_back(outputRCalfChild_2);

		std::vector<int32> outputLArmIds;
		outputLArmIds.push_back(outputLForeArmParent_2);
		outputLArmIds.push_back(outputLForeArmParent_1);
		outputLArmIds.push_back(outputLForeArmId);
		outputLArmIds.push_back(outputLForeArmChildId);

		std::vector<int32> outputRArmIds;
		outputRArmIds.push_back(outputRForeArmParent_2);
		outputRArmIds.push_back(outputRForeArmParent_1);
		outputRArmIds.push_back(outputRForeArmId);
		outputRArmIds.push_back(outputRForeArmChildId);

		FixInverseBending(m_sampledBipedJointsLocal, finalFrameCount, m_sampledBipedJointsGlobal, 
			outputPelvisID, outputLlegIds, outputRlegIds, outputLArmIds, outputRArmIds);
	}

	//------------------------------------------------------------------------------
	// Save input and output node specs.
	m_inputNodeSpec.clear();
	for(int32 i=0; i< numInputNode; ++i)
		m_inputNodeSpec.push_back(inputNodesSpec[i]);

	m_outputNodeSpec.clear();
	for(int32 i=0; i< numOutputNode; ++i)
		m_outputNodeSpec.push_back(outputNodesSpec[i]);


	//------------------------------------------------------------------------------
	// Additive motion

	if(m_bAdditiveMotion)
		ApplyAdditiveMotion(outputNodesSpec, numOutputNode, outputPelvisID, outputRootJointId, finalFrameCount);
	

	//------------------------------------------------------------------------------
	// Adjust fee with Vicon markers

	if(m_bAdjustFoot)
	{	
		std::vector<int32> outputLfeetIds;
		outputLfeetIds.push_back(outputLfootId);
		outputLfeetIds.push_back(outputLfootChildId);
		
		
		std::vector<int32> outputRfeetIds;
		outputRfeetIds.push_back(outputRfootId );
		outputRfeetIds.push_back(outputRfootChildId); 

		AdjustFeetWithViconMarkers(outputLfeetIds, outputRfeetIds, outputLfootId, outputRfootId,
			numOutputNode, finalFrameCount, m_SamplingRatio, outputRootJointId, outputNodesSpec, m_sampledViconMarkers, 
			viconMarkerNameMap, m_sampledBipedJointsLocal, m_sampledBipedJointsGlobal);
	}

	//------------------------------------------------------------------------------
	// Retargeting

	if(m_bMotionRetargeting)
	{
		Retarget(outputLfootId, outputRfootId, outputPelvisID, pViconJoints, m_sampledBipedJointsLocal, outputNodesSpec, finalFrameCount);
	}

	//------------------------------------------------------------------------------
	// Test biped skeleton
	for(int32 i=0; i< numOutputNode; ++i)
	{
		m_BipSkeleton[i].m_RelativePose2.resize(finalFrameCount);
		for(int32 j=0; j< finalFrameCount; ++j)
		{
			m_BipSkeleton[i].m_RelativePose2[j].q = m_sampledBipedJointsLocal[i][j].q ;	
			m_BipSkeleton[i].m_RelativePose2[j].t = m_sampledBipedJointsLocal[i][j].t;
		}
	}

	//------------------------------------------------------------------------------
	// Calculate speed

	CalcuMovingSpeed(m_speedSmoothSize);

	return SUCCESS;
}

int32 CMotionAdaptorManager::GetFirstChildId(const int32 jointId, const SNodeDesc outputNodeSpec[], const uint32 numOutputNode)
{
	int childId = -1;
	for(uint32 i=0; i< numOutputNode; ++i)
	{
		if(outputNodeSpec[i].parent == jointId) 
		{
			childId = i;
			break;
		}
	}

	return childId;
}

//------------------------------------------------------------------------------
//- Adjust orientations of each missing-joint segments
//------------------------------------------------------------------------------
int32 CMotionAdaptorManager::ApplyAdditiveMotion(const SNodeDesc outputNodesSpec[], const uint32 numOutputNode, const int32 outputPelvisID,
																										 const int32 outputRootJointId, const uint32 finalFrameCount)
{
	std::vector< std::vector<std::pair<int32, int32> > > missingBoneSegments;
	GetMissingBoneSegments(missingBoneSegments, outputNodesSpec, numOutputNode);

	for(int32 i=0; i< (int32)missingBoneSegments.size(); ++i)
	{
		std::vector<Quat> additive;
		f32 factor = 1.0f / (f32)missingBoneSegments[i].size();
		int32 beginOutputId = -1;

		//------------------------------------------------------------------------------
		// Check if at least two children is connect to the missing bone
		// Stop adjustment when >=2 children are not directly connected to the beginOutputId bone
		// since in this case we cannot easily find a plausible adjustment

		int32 numDirectConnectedChildren = 0;
		if(missingBoneSegments[i].size() ==3)
		{
			for(int32 j=0; j< 3; ++j)
			{
				if(outputNodesSpec[missingBoneSegments[i][j].second].parent == missingBoneSegments[i][j].first)
					++numDirectConnectedChildren;
			}		
			if(numDirectConnectedChildren <2)
				continue;	
		}

		//------------------------------------------------------------------------------
		// Get additive from children
		for(int32 j=0; j< (int32)missingBoneSegments[i].size(); ++j)
		{		
			beginOutputId = missingBoneSegments[i][j].first; assert(beginOutputId >= 0);

			if(beginOutputId == outputPelvisID) //not allowed to change pelvis currently
				break;

			int32 endOutputId = missingBoneSegments[i][j].second; assert(endOutputId >= 0);
			int32 beginInputId = outputNodesSpec[beginOutputId].source; assert(beginInputId >= 0);
			int32 endInputId = outputNodesSpec[endOutputId].source;  assert(endInputId >= 0);

			Vec3 bipedVec = m_sampledBipedJointsGlobal[endOutputId][0].t - m_sampledBipedJointsGlobal[beginOutputId][0].t;
			Vec3 viconVec = m_viconJointsGlobal[endInputId][0].t  - m_viconJointsGlobal[beginInputId][0].t;
			Vec3 crossVec = (viconVec % bipedVec).GetNormalized();
			Vec3 finalAxis = (!m_sampledBipedJointsGlobal[beginOutputId][0].q * crossVec).GetNormalized();

			f32 theta = acos (bipedVec.dot(viconVec) / (bipedVec.GetLength()*viconVec.GetLength()) );

			additive.push_back( Quat::CreateRotationAA(-theta, finalAxis) );
		}

		if(additive.empty())
			continue;

		// if multiple children exist, choose the first one as reference.
		Quat avgAdditive = additive[0];
		avgAdditive.Normalize();

		//------------------------------------------------------------------------------
		// Do the adjustment
		for(int32 j=0; j< finalFrameCount; ++j)
		{			
			m_sampledBipedJointsLocal[beginOutputId][j].q = m_sampledBipedJointsLocal[beginOutputId][j].q * avgAdditive;
		}

		//------------------------------------------------------------------------------
		// Reclaulte absolute poses since the relative poses have been changed
		CalcuAbsolutePoses(m_sampledBipedJointsLocal, m_sampledBipedJointsGlobal, outputPelvisID, outputRootJointId, numOutputNode, finalFrameCount, outputNodesSpec);
	}
	
	return SUCCESS;
}

//------------------------------------------------------------------------------
//- Search for missing bone segments. 
//------------------------------------------------------------------------------
int32 CMotionAdaptorManager::GetMissingBoneSegments(std::vector< std::vector<std::pair<int32, int32> > >& segments, const SNodeDesc outputNodeSpec[], const uint32 numOutput)
{
	for(int32 i=0; i< numOutput; ++i)
	{
		// if current joint are missing, just skip, we always start from some existing joints
		// so if the motion of an end-effector joint is missing, we will not do addition adjustment.

		// Get parent
		int32 parent = outputNodeSpec[i].parent;

		if(outputNodeSpec[i].source!= -1 && outputNodeSpec[parent].source == -1) // the first non-missing child of a missing joint
		{
			int32 startId = i;
			int32 endId = -1;

			while(parent != -1 && outputNodeSpec[parent].source == -1 ) // loop until parent has source joint
				parent = outputNodeSpec[parent].parent;

			endId = parent; // the first parent that is not a missing joint		

			if(startId!= -1 && endId != -1)
			{
				bool endIdExist = false; // if multiple children link to a missing bone, choose the first one
				int32 index = -1;
				for(int32 j=0; j< (int32)segments.size(); ++j)
				{
					if(segments[j][0].second == endId)
					{
						endIdExist = true;
						index = j;
					}
				}
				if(!endIdExist)
				{
					std::vector< std::pair<int32, int32> > t;
					t.push_back( std::make_pair(startId, endId) );
					segments.push_back(t); // the order is exchanged to keep root-endeffector order
				}
				else
					segments[index].push_back( std::make_pair(startId, endId) );
			}
		}
		
	}

	//------------------------------------------------------------------------------
	// reverse the order so parents appear first

	for(int32 i=0; i< (int32)segments.size(); ++i)
	{
		for(int32 j=0; j< (int32)segments[i].size(); ++j )
		{
			int32 tmp = segments[i][j].first;
			segments[i][j].first = segments[i][j].second;
			segments[i][j].second = tmp;
		}		
	}

	return SUCCESS;
}

int32 CMotionAdaptorManager::CalcuAbsolutePoses(const std::vector<std::vector<QuatT> >& m_sampledBipedJointsLocal, std::vector< std::vector<QuatT> >& m_sampledBipedJointsGlobal,
																										 const int32 outputPelvisID, const int32 outputRootJointId, uint32 numOutputNode, const uint32 finalFrameCount, const SNodeDesc outputNodesSpec[])
{
	m_sampledBipedJointsGlobal.resize(numOutputNode);
	for(int32 i=0; i< numOutputNode; ++i)
	{
		m_sampledBipedJointsGlobal[i].resize(finalFrameCount);
	}

	for(int32 i=0; i< finalFrameCount; ++i)
	{
		for(int32 j=0; j< numOutputNode; ++j)
		{
			int32 parentId = outputNodesSpec[j].parent;
			if(j== outputRootJointId)
			{
				m_sampledBipedJointsGlobal[j][i].q = Quat::CreateIdentity();
				m_sampledBipedJointsGlobal[j][i].t = Vec3(.0f, .0f, .0f);
			}
			else if(j == outputPelvisID) // For pelvis: global = relative
			{
				m_sampledBipedJointsGlobal[j][i].q = m_sampledBipedJointsLocal[j][i].q;
				m_sampledBipedJointsGlobal[j][i].t = m_sampledBipedJointsLocal[j][i].t;
			}
			else if(parentId !=-1)
			{
				QuatT parentAbsolute;
				parentAbsolute.q = m_sampledBipedJointsGlobal[parentId][i].q;
				parentAbsolute.t = m_sampledBipedJointsGlobal[parentId][i].t;

				QuatT currentRelative;
				currentRelative.q = m_sampledBipedJointsLocal[j][i].q;
				currentRelative.t = m_sampledBipedJointsLocal[j][i].t;

				QuatT quatT = parentAbsolute * currentRelative;
				m_sampledBipedJointsGlobal[j][i].q = quatT.q;
				m_sampledBipedJointsGlobal[j][i].t = quatT.t;
			}
		}
	}
	return SUCCESS;
}

int32 CMotionAdaptorManager::FixInverseBending(std::vector< std::vector<QuatT> >& outputJoints, uint32 frameCount, std::vector< std::vector<QuatT> >& absoluteOutputPoses,
																										const int32 outputPelvisId, const std::vector<int32>& lLegJointIds, const std::vector<int32> rLegJointIds,
																										const std::vector<int32>& lArmIds, const std::vector<int32>& rArmIds)
{
	FixInverseBendingElbow(outputJoints, frameCount, absoluteOutputPoses, lArmIds);
	FixInverseBendingElbow(outputJoints, frameCount, absoluteOutputPoses, rArmIds);
	FixInverseBendingKnee(outputJoints, frameCount, absoluteOutputPoses,  outputPelvisId, lLegJointIds);
	FixInverseBendingKnee(outputJoints, frameCount, absoluteOutputPoses,  outputPelvisId, rLegJointIds);

	return SUCCESS;
}

//----------------------------------------------------------------------------------------------
// legJointIds: Calf's parent (0), Calf (1), Calf's first child (2), Calf's second child (3)
//----------------------------------------------------------------------------------------------
int32 CMotionAdaptorManager::FixInverseBendingKnee(std::vector< std::vector<QuatT> >& outputJoints, uint32 frameCount, std::vector< std::vector<QuatT> >& absoluteOutputPoses, 
																												const int32 outputPelvisId, const std::vector<int32>& legJointIds)
{
	for (int32 i=0; i<frameCount; ++i)
	{
		QuatT wThigh	=	absoluteOutputPoses[ legJointIds[0] ][i];
		QuatT wCalf		=	absoluteOutputPoses[ legJointIds[1] ][i];
		QuatT wFoot		=	absoluteOutputPoses[ legJointIds[2] ][i];
		QuatT wToe0 	=	absoluteOutputPoses[ legJointIds[3] ][i];

		Vec3 axisY = wCalf.q.GetColumn1();
		Vec3 axisZ = wCalf.q.GetColumn2();
		for (int32 t=0; t<200; t++)
		{
			Vec3 v0 = wThigh.t-wCalf.t;
			Vec3 v1 = wFoot.t -wCalf.t;
			Vec3 cross=v0%v1;
			f32 dot=cross|axisZ;
			if (dot>0.01f)
				break;
			wCalf.t += axisY*0.001f; 
		}

		Vec3 v0		= (absoluteOutputPoses[ legJointIds[0] ][i].t - absoluteOutputPoses[ legJointIds[1] ][i].t).GetNormalized();//(m_BipSkeleton[Thigh].m_AbsolutePose2[i].t-m_BipSkeleton[Calf].m_AbsolutePose2[i].t).GetNormalized();
		Vec3 v1		= (wThigh.t-wCalf.t).GetNormalized();
		Quat rel	= Quat::CreateRotationV0V1(v0,v1);

		absoluteOutputPoses[ legJointIds[0] ][i].q = rel * absoluteOutputPoses[ legJointIds[0] ][i].q;//m_BipSkeleton[Thigh].m_AbsolutePose2[i].q = rel*m_BipSkeleton[Thigh ].m_AbsolutePose2[i].q;
		outputJoints[ legJointIds[0] ][i] = absoluteOutputPoses[outputPelvisId][i].GetInverted() * absoluteOutputPoses[ legJointIds[0] ][i];//m_BipSkeleton[Thigh].m_RelativePose2[i] = m_BipSkeleton[Pelvis].m_AbsolutePose2[i].GetInverted() * m_BipSkeleton[Thigh].m_AbsolutePose2[i];	

		for(int32 j=1; j<(int32)legJointIds.size(); ++j)
			absoluteOutputPoses[ legJointIds[j] ][i] = absoluteOutputPoses[ legJointIds[j-1] ][i] * outputJoints[ legJointIds[j] ][i];


		Vec3 x1		= (wCalf.t-wFoot.t).GetNormalized();
		Vec3 x0		= (absoluteOutputPoses[ legJointIds[1] ][i].t - absoluteOutputPoses[ legJointIds[2] ][i].t).GetNormalized(); //(m_BipSkeleton[Calf].m_AbsolutePose2[i].t-m_BipSkeleton[Foot].m_AbsolutePose2[i].t).GetNormalized();
		Quat xrel	= Quat::CreateRotationV0V1(x0,x1);


		absoluteOutputPoses[  legJointIds[1] ][i].q = xrel * absoluteOutputPoses[  legJointIds[1] ][i].q;
		outputJoints[ legJointIds[1] ][i] = absoluteOutputPoses[ legJointIds[0] ][i].GetInverted() * absoluteOutputPoses [ legJointIds[1] ][i];

		absoluteOutputPoses[  legJointIds[2] ][i] = absoluteOutputPoses[  legJointIds[1] ][i] * outputJoints[ legJointIds[2] ][i];
		absoluteOutputPoses[  legJointIds[3] ][i] = absoluteOutputPoses[  legJointIds[2] ][i] * outputJoints[ legJointIds[3] ][i];

	}

	return SUCCESS;
}


//------------------------------------------------------------------------------
//- armJointIds contains: forearm's second parent (0), forearm's first parent (1), forearm (2), forearm's child (3)
//------------------------------------------------------------------------------
int32 CMotionAdaptorManager::FixInverseBendingElbow(std::vector< std::vector<QuatT> >& outputJoints, uint32 frameCount, std::vector< std::vector<QuatT> >& absoluteOutputPoses,
																												 const std::vector<int32>& armJointIds)
{
	for (int32 i=0; i<frameCount; i++)
	{
		QuatT wUArm	=	absoluteOutputPoses[ armJointIds[1] ][i];
		QuatT wFArm	=	absoluteOutputPoses[ armJointIds[2] ][i];
		QuatT wHand	=	absoluteOutputPoses[ armJointIds[3] ][i];

		Vec3 axisY = absoluteOutputPoses[ armJointIds[2] ][i].q.GetColumn1();
		Vec3 axisZ = absoluteOutputPoses[ armJointIds[2] ][i].q.GetColumn2();
		for (int32 t=0; t<200; t++)
		{
			Vec3 v0 = wUArm.t-wFArm.t;
			Vec3 v1 = wHand.t -wFArm.t;
			Vec3 cross=v0%v1;
			f32 dot=cross|axisZ;
			if (dot>0.01f)
				break;
			wFArm.t += axisY*0.001f; 
		}

		Vec3 v0		= (absoluteOutputPoses[ armJointIds[1] ][i].t - absoluteOutputPoses[ armJointIds[2] ][i].t).GetNormalized();// (m_BipSkeleton[UArm].m_AbsolutePose2[i].t-m_BipSkeleton[FArm].m_AbsolutePose2[i].t).GetNormalized();
		Vec3 v1		= (wUArm.t-wFArm.t).GetNormalized();
		Quat rel	= Quat::CreateRotationV0V1(v0,v1);

		absoluteOutputPoses[ armJointIds[1] ][i].q = rel * absoluteOutputPoses[ armJointIds[1] ][i].q;
		outputJoints[ armJointIds[1] ][i] = absoluteOutputPoses[ armJointIds[0] ][i].GetInverted() * absoluteOutputPoses[ armJointIds[1] ][i];

		absoluteOutputPoses[ armJointIds[2] ][i] = absoluteOutputPoses[ armJointIds[1] ][i] * outputJoints[ armJointIds[2] ][i];
		absoluteOutputPoses[ armJointIds[3] ][i] = absoluteOutputPoses[ armJointIds[2] ][i] * outputJoints[ armJointIds[3] ][i];

		Vec3 x1		= (wFArm.t-wHand.t).GetNormalized();
		Vec3 x0		= (absoluteOutputPoses[ armJointIds[2] ][i].t - absoluteOutputPoses[ armJointIds[3] ][i].t).GetNormalized();//(m_BipSkeleton[FArm].m_AbsolutePose2[i].t-m_BipSkeleton[Hand].m_AbsolutePose2[i].t).GetNormalized();
		Quat xrel	= Quat::CreateRotationV0V1(x0,x1);

		absoluteOutputPoses[ armJointIds[2] ][i].q = xrel * absoluteOutputPoses[ armJointIds[2] ][i].q;
		outputJoints[ armJointIds[2] ][i] = absoluteOutputPoses[ armJointIds[1] ][i].GetInverted() * absoluteOutputPoses[ armJointIds[2] ][i];

		absoluteOutputPoses[ armJointIds[3] ][i] = absoluteOutputPoses[ armJointIds[2] ][i] * outputJoints[ armJointIds[3] ][i];
	}

	return SUCCESS;
}


//------------------------------------------------------------------------------
//- inputLfeetIds: Ids of lFoot and the child of lFoot (Vicon)
//  inputRfeetIds: Ids of rFoot and the child of rFoot (Vicon)
//  outputLfootId: Id of lFoot (Biped)
//  outputRfootId: Id of rFoot (Biped)
//  outputRootJointId: e. g. "Bip01"
//  frameCount: frame count after resampling
//------------------------------------------------------------------------------
int32 CMotionAdaptorManager::AdjustFeetWithViconMarkers(const std::vector<int32>& outputLfeetIds, const std::vector<int32>& outputRfeetIds,
																														 const int32 outputLfootId, const int32 outputRfootId, const uint32 outputJointCount, const uint32 frameCount, 
																														 const uint32 sampleingRatio, const int32 outputRootJointId, const SNodeDesc* pOutputNodeSpec,
																														 const std::vector<tViconMarkers>& viconMarkers, const std::map<string, int32>& viconMarkerNameMap,
																														 std::vector< std::vector<QuatT>>& pOutputNodes, std::vector< std::vector<QuatT> >& outputAbsolutePoses)
{
	if(viconMarkers.empty()) // no markers available
		return FAILED;

	int32 lhee= -1, ltoe= -1, lmt5= -1, lmt8= -1;

	lhee = stl::find_in_map(viconMarkerNameMap, "LHEE", -1);
	ltoe = stl::find_in_map(viconMarkerNameMap, "LTOE", -1);
	lmt5 = stl::find_in_map(viconMarkerNameMap, "LMT5", -1);
	lmt8 = stl::find_in_map(viconMarkerNameMap, "LMT8", -1);

	int32 rhee= -1, rtoe= -1, rmt5= -1, rmt8= -1;

	rhee = stl::find_in_map(viconMarkerNameMap, "RHEE", -1);
	rtoe = stl::find_in_map(viconMarkerNameMap, "RTOE", -1);
	rmt5 = stl::find_in_map(viconMarkerNameMap, "RMT5", -1);
	rmt8 = stl::find_in_map(viconMarkerNameMap, "RMT8", -1);


	for (int32 i=0; i<frameCount; i++)
	{
		if (lhee>-1 && ltoe>-1 && lmt5>-1 && lmt8>-1)
		{
			Vec3 posLHEE = viconMarkers[lhee].m_arrMarkerPos[i];
			Vec3 posLMT5 = viconMarkers[lmt5].m_arrMarkerPos[i];
			Vec3 posLMT8 = viconMarkers[lmt8].m_arrMarkerPos[i];
			Vec3 posMID = (posLMT5+posLMT8)*0.5f;

			Vec3 axisY = (posMID-posLHEE).GetNormalized();
			Vec3 axisZ = (posLMT8-posLMT5).GetNormalized();
			Vec3 axisX = (axisY%axisZ).GetNormalized();
			axisZ = (axisX%axisY).GetNormalized();

			Matrix33 m33;	
			m33.SetFromVectors(axisX,axisY,axisZ);
			int32 ortho = m33.IsOrthonormalRH(0.1f);	assert( ortho ); //conversion error

			if (ortho)
			{
				outputAbsolutePoses[outputLfootId][i].q = Quat(m33).GetNormalized();
			}
		}

		if (rhee>-1 && rtoe>-1 && rmt5>-1 && rmt8>-1)
		{
			Vec3 posRHEE = viconMarkers[rhee].m_arrMarkerPos[i];
			Vec3 posRTOE = viconMarkers[rtoe].m_arrMarkerPos[i];
			Vec3 posRMT5 = viconMarkers[rmt5].m_arrMarkerPos[i];
			Vec3 posRMT8 = viconMarkers[rmt8].m_arrMarkerPos[i];
			Vec3 posMID = (posRMT5+posRMT8)*0.5f;

			Vec3 axisY = (posMID-posRHEE).GetNormalized();
			Vec3 axisZ = (posRMT5-posRMT8).GetNormalized();
			Vec3 axisX = (axisY%axisZ).GetNormalized();
			axisZ = (axisX%axisY).GetNormalized();

			Matrix33 m33;	m33.SetFromVectors(axisX,axisY,axisZ);
			int32 ortho = m33.IsOrthonormalRH(0.01f);	assert( ortho ); //conversion error
			if (ortho)
			{
				outputAbsolutePoses[outputRfootId][i].q = Quat(m33).GetNormalized();
			}

		}
	}

	//calculate the relative quaternion
	for(int32 i= 0; i< (int32)outputLfeetIds.size(); ++i)
	{
		int32 current = outputLfeetIds[i];
		int32 parent = pOutputNodeSpec[current].parent;
		assert(parent != -1);
		for (int32 j=0; j<frameCount; ++j)
		{
			pOutputNodes[ current ][j].q = !outputAbsolutePoses[parent][j].q * outputAbsolutePoses[ current ][j].q;
			pOutputNodes[ current ][j].q.Normalize();
		}					
	}

	//calculate the relative quaternion
	for(int32 i= 0; i< (int32)outputRfeetIds.size(); ++i)
	{
		int32 current = outputRfeetIds[i];
		int32 parent = pOutputNodeSpec[current].parent;
		assert(parent != -1);
		for (int32 j=0; j<frameCount; ++j)
		{
			pOutputNodes[ current ][j].q = !outputAbsolutePoses[parent][j].q * outputAbsolutePoses[ current ][j].q;
			pOutputNodes[ current ][j].q.Normalize();
		}					
	}

	return SUCCESS;
}

//------------------------------------------------------------------------------
//- Set joint name and axis swapping vectors
//------------------------------------------------------------------------------

int32 CMotionAdaptorManager::SetupViconJointProperty(std::vector<SViconJointProperty>& m_viconJointProperty)
{
	m_viconJointProperty.clear();
	SViconJointProperty jp;

	jp.jointName = "Pelvis"; jp.axisSwapper =    Vec3i(3, 1, 2); //0
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Thorax"; jp.axisSwapper =    Vec3i(3, 1, 2); //1
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Spine"; jp.axisSwapper =     Vec3i(3, 1, 2); //2
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Head"; jp.axisSwapper =      Vec3i(3, 1, 2); //3
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Lclavicle"; jp.axisSwapper = Vec3i(2, -1, 3); //4
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Lhumerus"; jp.axisSwapper =  Vec3i(-3, -1, 2); //5
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Lradius"; jp.axisSwapper =   Vec3i(-3, -1, 2); //6
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Lhand"; jp.axisSwapper =     Vec3i(-3, -2, -1); //7
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Rclavicle"; jp.axisSwapper = Vec3i(-2, -1, -3); //8
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Rhumerus"; jp.axisSwapper =  Vec3i(-3, -1, 2); //9
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Rradius"; jp.axisSwapper =   Vec3i(-3, -1, 2); //10
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Rhand"; jp.axisSwapper =     Vec3i(-3, 2, 1); //11
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Lfemur"; jp.axisSwapper =    Vec3i(-3, 1, -2); //12
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Ltibia"; jp.axisSwapper =    Vec3i(-3, 1, -2); //13
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Lfoot"; jp.axisSwapper =     Vec3i(-3, 1, -2); //14
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Ltoes"; jp.axisSwapper =     Vec3i(1, 3, -2); //15
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Rfemur"; jp.axisSwapper =    Vec3i(-3, 1, -2); //16
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Rtibia"; jp.axisSwapper =    Vec3i(-3, 1, -2); //17
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Rfoot"; jp.axisSwapper =     Vec3i(-3, 1, -2); //18
	m_viconJointProperty.push_back(jp);

	jp.jointName = "Rtoes"; jp.axisSwapper =     Vec3i(1, 3, -2); //19
	m_viconJointProperty.push_back(jp);

	return SUCCESS;
}

//------------------------------------------------------------------------------
//- Motion retargeting based on the vertical component of the pelvis->foot vector
//------------------------------------------------------------------------------
int32 CMotionAdaptorManager::Retarget( const int32 outputLfootId, const int32 outputRfootId, const int32 outputPelvisId,
																					const std::vector< std::vector<QuatT> >& pInputNodes, std::vector< std::vector<QuatT> >& m_sampledBipedJointsLocal,
																					const SNodeDesc outputNodesSpec[], const int frameCount)
{
	uint32 viconPelvisID =outputNodesSpec[outputPelvisId].source;
	assert(viconPelvisID != -1);


	int32 inputLfootId  = outputNodesSpec[outputLfootId].source;
	int32 intputRfootId = outputNodesSpec[outputRfootId].source;

	Vec3 lLegLenBiped = (m_sampledBipedJointsGlobal[outputLfootId][0].t - m_sampledBipedJointsGlobal[outputPelvisId][0].t);
	Vec3 rLegLenBiped = (m_sampledBipedJointsGlobal[outputRfootId][0].t - m_sampledBipedJointsGlobal[outputPelvisId][0].t);

	Vec3 lLegLenVicon = (m_viconJointsGlobal[inputLfootId][0].t - m_viconJointsGlobal[viconPelvisID][0].t);
	Vec3 rLegLenVicon = (m_viconJointsGlobal[intputRfootId][0].t - m_viconJointsGlobal[viconPelvisID][0].t);

	f32 scale = (lLegLenBiped.z + rLegLenBiped.z) / (lLegLenVicon.z + rLegLenVicon.z);

	for (int32 i=0; i<frameCount; i++)
		m_sampledBipedJointsLocal[outputPelvisId][i].t = pInputNodes[viconPelvisID][i*m_SamplingRatio].t * scale;
	
	return SUCCESS;
}

//------------------------------------------------------------------------------
//- Resample a stream
//  will allocate memory for each joint in this function
//------------------------------------------------------------------------------

int32 CMotionAdaptorManager::Resampling(const std::vector< std::vector<QuatT> >& nodes, std::vector< std::vector<QuatT> >& resampledNodes, 
																						const std::vector<tViconMarkers>& markerNodes, std::vector<tViconMarkers>& resampledMarkerNodes, 
																						const int32 outputFrameCount, const int32 jointCount, const int markerCount, const f32 samplingRatio)
{
	//------------------------------------------------------------------------------
	// resample joint motion
	for(int32 i=0; i< jointCount; ++i	)
	{
		for(int32 j=0; j< outputFrameCount; ++j)
		{
			int32 start = (int32)(j*samplingRatio);
			int32 end = (int32)((j+1)*samplingRatio);

			f32 factor = 1.0f / (end-start+1);
			QuatT qt;
			qt.q = nodes[i][start].q*factor;
			qt.t = nodes[i][start].t*factor;
			for(int32 k=start+1; k<=end; ++k)
			{
				qt.q %= nodes[i][k].q*factor; // use % to make sure quaternions locate on the same semi-sphere
				qt.t += nodes[i][k].t*factor;
			}
			resampledNodes[i][j].q = qt.q.GetNormalized();
			resampledNodes[i][j].t = qt.t;
		}
	}	

	//------------------------------------------------------------------------------
	// resample vicon markers
	resampledMarkerNodes.resize(markerCount);
	for(int32 i=0; i< markerCount; ++i	)
	{
		for(int32 j=0; j< outputFrameCount; ++j)
		{
			int32 start = (int32)(j*samplingRatio);
			int32 end = (int32)((j+1)*samplingRatio);

			f32 factor = 1.0f / (end-start+1);
			Vec3 t = markerNodes[i].m_arrMarkerPos[start]*factor;
			for(int32 k=start+1; k<=end; ++k)
			{
				t  += markerNodes[i].m_arrMarkerPos[k]*factor;
			}
			resampledMarkerNodes[i].m_arrMarkerPos.push_back(t);
		}
	}	


	return SUCCESS;
}

//------------------------------------------------------------------------------
//- Get default Biped poses from loaded character model
//------------------------------------------------------------------------------
int32 CMotionAdaptorManager::GetDefaultPoses(SNodeDesc outputNodesSpec[], const int numBipedJoints)
{
	for(int i=0; i<numBipedJoints; ++i)
	{
		outputNodesSpec[i].defaultPose.t = Vec3(.0f, .0f, .0f);
		outputNodesSpec[i].defaultPose.q.CreateIdentity();
	}

	ICharacterInstance* pInstance = m_pModelViewportCE->GetCharacterBase();
	ISkeletonAnim* pISkeletonAnim = pInstance->GetISkeletonAnim();
	ISkeletonPose* pISkeletonPose = pInstance->GetISkeletonPose();

	int32 numMismatchedJoints = 0;
	for (int i=1; i<numBipedJoints; i++)
	{
		const char* strJointName = m_BipSkeleton[i].m_strJointName;
		int idx=pISkeletonPose->GetJointIDByName(strJointName);
		if (idx<0)
		{
			++numMismatchedJoints;
			continue;
		}
		outputNodesSpec[i].defaultPose = pISkeletonPose->GetDefaultRelJointByID(idx);

		int32 p = pISkeletonPose->GetParentIDByID(idx);
		const char* strParentJointName = pISkeletonPose->GetJointNameByID(p);
		int32 nLocalParentidx=GetBipedIDbyName(strParentJointName);	
		m_BipSkeleton[i].m_idxParent = nLocalParentidx;
		outputNodesSpec[i].parent = nLocalParentidx;
	} 

	int32 ltoe0 = GetBipedIDbyName("Bip01 L Toe0");
	int32 rtoe0 = GetBipedIDbyName("Bip01 R Toe0");
	int32 ltoe1 = GetBipedIDbyName("Shadow L Toe0");
	int32 rtoe1 = GetBipedIDbyName("Shadow R Toe0");

	outputNodesSpec[ltoe1] = outputNodesSpec[ltoe0];
	outputNodesSpec[rtoe1] = outputNodesSpec[rtoe0];
	m_BipSkeleton[ltoe1].m_idxParent = m_BipSkeleton[ltoe0].m_idxParent;
	m_BipSkeleton[rtoe1].m_idxParent = m_BipSkeleton[rtoe0].m_idxParent;

	outputNodesSpec[ltoe1].defaultPose.t.z = 0.0;
	outputNodesSpec[rtoe1].defaultPose.t.z = 0.0;

	return SUCCESS;
}

void CMotionAdaptorManager::VerfyReadData(std::vector< std::vector<QuatT> >& data)
{
	uint32 s = data.size();
	for(uint32 i=0; i< s; ++i)
	{
		uint32 t = data[i].size();
		for(uint32 j=0; j< t; ++j)
		{
			if(!data[i][j].IsValid())
			{
				assert(0);
				data[i][j].q = Quat::CreateIdentity();
				data[i][j].t = Vec3(.0f, .0f, .0f);
			}
		}		
	}
}

void CMotionAdaptorManager::VerfyReadData(std::vector< std::vector<Vec3r> >& data)
{
	#define VALID_NUM(x) ( x > -1e+20 && x < 1e+20 ? 1:0)

	uint32 s = data.size();
	for(uint32 i=0; i< s; ++i)
	{
		uint32 t = data[i].size();
		for(uint32 j=0; j< t; ++j)
		{
			const Vec3r& v = data[i][j];
			if( !VALID_NUM(v.x) || !VALID_NUM(v.y) || !VALID_NUM(v.z))
			{
				assert(0);
				data[i][j] = Vec3r(.0, .0, .0);
			}
		}		
	}
}

//------------------------------------------------------------------------------
//- pViconJoints will return the RELATIVE orientations and GLOBAL positions
//- readGlobalData will return the GLOBAL orientations and GLOBAL positions
//  Missing joints will be interpolated
//------------------------------------------------------------------------------
int32 CMotionAdaptorManager::ReadVFile(const char* strFileName, std::vector< std::vector<QuatT> >& pViconJoints, std::vector< std::vector<QuatT> >& readGlobalData, 
																						const SNodeDesc viconNodesSpec[], const std::vector<SViconJointProperty>& m_viconJointProperty,
																						std::vector<tViconMarkers>& viconMarkers, const uint32 viconJointCount, uint32& frameCount, f64& frameHertz)
{
	std::vector< std::vector<QuatT> > viconGlobal; // Global data read directly from Vicon
	std::vector< std::vector<QuatT> > viconLocal;  // Local data read directly from Vicon
	std::vector< std::vector<QuatT> > mappedLocal; // Local data mapped to input stream
	mappedLocal.resize(readGlobalData.size());

	std::vector<bool> hasAnimation(readGlobalData.size(), false); // Whether a joint has corresponding Vicon animation-data

	CVfileParser rp(strFileName);
	std::vector<string> names;

	if(-1 == rp.GetJointData("GLOBAL BODIES", viconGlobal, names))
	{
		// If no global body presents, try to read local data
		rp.GetJointData("LOCAL BODIES", viconLocal, names);

		VerfyReadData(viconLocal);

		// If no lables found, current V-file is invalid.
		if(names.empty())
		{
			MessageBox(NULL, "Invalid file: No labels found.", "V-file read error", MB_ICONERROR|MB_OK);
			return FAILED;
		}

		frameCount = rp.m_VfileRawParser.GetFrameCount();
		frameHertz = rp.m_VfileRawParser.GetFrameHertz();
		
		//------------------------------------------------------------------------------
		// Map vicon local data to input stream
		for(int32 i=0; i< (int32)viconLocal.size(); ++i)
		{
			string strRead = names[i];
			strRead.MakeUpper();

			int32 idx=-1;
			uint32 numViconJoints = (uint32)m_viconJointProperty.size();
			for(uint32 v=0; v<numViconJoints; v++) 
			{
				string strDefined = m_viconJointProperty[v].jointName;
				strDefined.MakeUpper();

				if (strRead == strDefined)
				{
					idx = v;
					break; //if not equal, then continue
				}
			}
			if(idx == -1) // cannot find corresponding joint in inputSpec: e.g. "Box" joint
				continue;

			mappedLocal[idx] = viconLocal[i];		
			hasAnimation[idx] = true;
		}
		
		//------------------------------------------------------------------------------
		// For a missing joint, create an local identity QT for it
		for(int32 i=0; i<viconJointCount; ++i)
		{
			if(!hasAnimation[i])
			{
				QuatT qt(Vec3(.0f, .0f, .0f), Quat::CreateIdentity());
				for(int32 j=0; j< frameCount; ++j)
					mappedLocal[i].push_back(qt);
			}
		}

		//------------------------------------------------------------------------------
		// Calculate the global data based on read local data
		for(int32 i=0; i<viconJointCount; ++i)
		{
			QuatT qt;
			int32 iParentId = viconNodesSpec[i].parent;
			if(iParentId != -1 && !mappedLocal[i].empty()){
				for(int32 j=0; j<frameCount; ++j)
				{
					qt = readGlobalData[iParentId][j] * mappedLocal[i][j];
					readGlobalData[i].push_back(qt);
				}		
			}
			else if(iParentId == -1)// The root node, set relative = global
			{
				for(int32 j=0; j<frameCount; ++j)
					readGlobalData[i].push_back(mappedLocal[i][j]);
			}
		}
	}
	else // Global data exist
	{
		VerfyReadData(viconGlobal);

		frameCount = rp.m_VfileRawParser.GetFrameCount();
		frameHertz = rp.m_VfileRawParser.GetFrameHertz();

		for(int i=0; i<(int32)viconGlobal.size(); ++i)
		{
			const char* VicName1 = names[i];
			int32 idx=-1;
			uint32 numViconJoints = (uint32)m_viconJointProperty.size();
			for(uint32 v=0; v<numViconJoints; v++) 
			{
				const char* VicName2 = m_viconJointProperty[v].jointName.c_str();
				int32 eg = stricmp(VicName1,VicName2);
				if (eg)
					continue; //if not equal, then continue
				idx=v;
			}
			if(idx == -1) // connot find corresponding joint in inputspec: e.g. "Box" joint
				continue;

			readGlobalData[idx] = viconGlobal[i];		
			hasAnimation[idx] = true;
		}

	}

	//------------------------------------------------------------------------------
	// Display frame hertz
	char txt[64];
	sprintf(txt, "Vicon Frame Rate: %.2f", frameHertz);
	m_pCharacterEditor->GetMotionAdaptorDlg()->SetViconFrameRateText(txt);
	sprintf(txt, "Biped Frame Rate: %.2f", BIPED_FRAME_RATE);
	m_pCharacterEditor->GetMotionAdaptorDlg()->SetBipedFrameRateText(txt);

	//------------------------------------------------------------------------------
	// Get marker positions
	std::vector< std::vector<Vec3r> > markerPos;
	std::vector<string> markerNames;
	rp.GetMarkerPositions(markerPos, markerNames);

	VerfyReadData(markerPos);

	viconMarkers.resize(0);
	for(int32 i=0; i< (int32)markerPos.size(); ++i)
	{
		tViconMarkers marker;
		marker.sViconMarker = markerNames[i];
		marker.m_arrMarkerPos = markerPos[i];
		viconMarkers.push_back(marker);		
	}

	//------------------------------------------------------------------------------
	// Interpolate missing Vicon joints
	for(int32 i=0; i< viconJointCount; ++i)
	{
		if(!hasAnimation[i])
		{
			// Search children
			std::vector<uint32> childrenIds;
			for(int32 p=0; p< viconJointCount; ++p)
			{
				if(viconNodesSpec[p].parent == i)
					childrenIds.push_back(p);
			}

			assert(childrenIds.size() > 1); // will use the first child if multi-children exist
			assert(hasAnimation[ viconNodesSpec[i].parent] && hasAnimation[ childrenIds[0] ] ); //assume parent and child have animation data
			readGlobalData[i].resize(frameCount);
			for(int32 p =0; p< frameCount; ++p)
			{
				readGlobalData[i][p].q = readGlobalData[ viconNodesSpec[i].parent ][p].q * 0.5 + readGlobalData[ childrenIds[0] ][p].q * 0.5;
				readGlobalData[i][p].q.Normalize();
				readGlobalData[i][p].t = readGlobalData[ viconNodesSpec[i].parent ][p].t * 0.5 + readGlobalData[ childrenIds[0] ][p].t *0.5;
			}
		}
	}

	//------------------------------------------------------------------------------
	// Axis swapping
	for(int32 i=0; i< viconJointCount; ++i)
	{
		for(int32 j=0; j< frameCount; ++j)
		{
			Quat q0 = readGlobalData[i][j].q;
			Vec3 axisX = q0.GetColumn(fabs_tpl(m_viconJointProperty[i].axisSwapper.x)-1) * f32(sgn(m_viconJointProperty[i].axisSwapper.x));
			Vec3 axisY = q0.GetColumn(fabs_tpl(m_viconJointProperty[i].axisSwapper.y)-1) * f32(sgn(m_viconJointProperty[i].axisSwapper.y));
			Vec3 axisZ = q0.GetColumn(fabs_tpl(m_viconJointProperty[i].axisSwapper.z)-1) * f32(sgn(m_viconJointProperty[i].axisSwapper.z));
			Matrix33 m33;	m33.SetFromVectors(axisX,axisY,axisZ);

			int32 ortho = m33.IsOrthonormalRH(0.01f);
			assert( ortho ); 
			readGlobalData[i][j].q = Quat(m33);
		}
	}

	//------------------------------------------------------------------------------
	// Convert to relative quaternions


	for(int32 i=0; i<viconJointCount; ++i)
	{
		QuatT qt;
		int32 iParentId = viconNodesSpec[i].parent;
		if(iParentId != -1){
			for(int32 j=0; j<frameCount; ++j)
			{
				qt.q = !readGlobalData[iParentId][j].q * readGlobalData[i][j].q;
				qt.t = readGlobalData[i][j].t;// will set to global positions of vicon joints
				pViconJoints[i].push_back(qt);
			}		
		}
		else // The root node, set relative = global
			for(int32 j=0; j<frameCount; ++j)
				pViconJoints[i].push_back(readGlobalData[i][j]);

	}


	return SUCCESS;
}

void CMotionAdaptorManager::CalcuMovingSpeed(const int32 iSmoothSize)
{
	m_MovingSpeed.clear();
	f32 fFrameTime = 1.0f / m_ViconFrameHertz;
	const std::vector<QuatT>& pelvis = m_viconJointsGlobal[m_inputPelvisId];

	f32 fSpeed;
	m_MovingSpeed.push_back(0); // frame 0
	for(uint32 i=1; i< m_frameCount; ++i){
		const Vec3& a = pelvis[i].t;
		const Vec3& b = pelvis[i-1].t;
		Vec3 c = b-a;
		fSpeed = c.GetLength() / fFrameTime;
		m_MovingSpeed.push_back(fSpeed);
	}

	//------------------------------------------------------------------------------
	// Smooth speed

	std::vector<f32> speedBack = m_MovingSpeed;
	int halfSmoothSize = iSmoothSize/2;
	for(uint32 i=0; i< m_frameCount; ++i){
		int actualCalculated = 0;
		f32 fSum = 0.0f;
		for(int j= -halfSmoothSize; j<=halfSmoothSize; ++j){
			if(i+j >=0 && i+j < m_frameCount){
				fSum += speedBack[i+j];
				++actualCalculated;
			}
		}
		m_MovingSpeed[i] = fSum / actualCalculated;
	}
}



void CMotionAdaptorManager::ResetAnimationProgress()
{
	m_pCharacterEditor->GetMotionAdaptorDlg()->SetProgressSliderPos(0);
	m_pCharacterEditor->GetMotionAdaptorDlg()->SetPlayPauseText("Play");
	m_pCharacterEditor->GetMotionAdaptorDlg()->SetProgressEditText("0");
	m_pCharacterEditor->GetMotionAdaptorDlg()->SetBipedSpeedText("0.00");

	m_addFrameTime = 0.0;

	m_bPlayMotion = false;
}

void CMotionAdaptorManager::ResumeAnimationProgress()
{
	m_bPlayMotion = false;
}
void CMotionAdaptorManager::ClearAnimation()
{
	m_animationLoaded = false;
	m_BipSkeleton.clear();
	m_ViconMarkers.clear();
	m_MovingSpeed.clear();

	m_viconJointsGlobal.clear();
	m_sampledBipedJointsGlobal.clear();
	m_sampledBipedJointsLocal.clear();
	m_sampledViconMarkers.clear();
	m_frameCount = 0;

	m_pCharacterEditor->GetMotionAdaptorDlg()->SetProgressSliderPos(0);
	m_pCharacterEditor->GetMotionAdaptorDlg()->SetPlayPauseText("Play");
	m_pCharacterEditor->GetMotionAdaptorDlg()->SetProgressEditText("0");
	m_pCharacterEditor->GetMotionAdaptorDlg()->SetViconFrameRateText("Vicon Frame Rate:");
	m_pCharacterEditor->GetMotionAdaptorDlg()->SetBipedFrameRateText("Biped Frame Rate:");
	m_bPlayMotion = false;
}



bool CMotionAdaptorManager::CheckCorrespondingJointNames()
{
	ICharacterInstance* pInstance = m_pModelViewportCE->GetCharacterBase();
	ISkeletonPose* pISkeletonPose = pInstance->GetISkeletonPose();
	uint32 iJointNum = pISkeletonPose->GetJointCount();

	std::vector<string> errorJoints;
	for(int i=0; i<iJointNum; ++i){
		const char* strJointName = pISkeletonPose->GetJointNameByID(i);
		int32 id = GetBipedIDbyName(strJointName);
		if(id == -1)
			errorJoints.push_back(strJointName);
	}

	if(errorJoints.empty())
		return true;

	string msg = "Fail to find corresponding joints:\n";
	for(int i=0; i<(int)errorJoints.size(); ++i){
		if(i != errorJoints.size()-1)
			msg += errorJoints[i] + string("\n");
	}

	MessageBox(NULL, NULL, msg, MB_OK);
	return false;

}


inline void CMotionAdaptorManager::MotionPlaybackOneFrame(ICharacterInstance* pInstance, f32 fFrameNo)
{
	extern uint32 g_ypos;
	float color1[4] = {1,1,1,1};
	IRenderer* pIRenderer = GetIEditor()->GetSystem()->GetIRenderer();
	IRenderAuxGeom* pAuxGeom = GetIEditor()->GetSystem()->GetIRenderer()->GetIRenderAuxGeom();
	SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
	pAuxGeom->SetRenderFlags( renderFlags );

	size_t numViconKeys120 =0;
	numViconKeys120 = m_frameCount;
	if (numViconKeys120==0)
	{
		m_addFrameTime=0.0f;
		return; //probably file not loaded
	}


	if (fFrameNo>=numViconKeys120)
	{
		m_addFrameTime = 0.0f; 
		fFrameNo=0;
		return;
	}

	pIRenderer->Draw2dLabel(12,g_ypos,1.5f,color1,false,"addFrameTime: %f  fFrameNo: %f",m_addFrameTime,fFrameNo );
	g_ypos+=14;

	//--------------------------------------------------------------------
	//----                  Draw the Markers                           ---
	//--------------------------------------------------------------------
	if(m_bShowViconMarkers)
	{
		size_t numMarkers = m_ViconMarkers.size();
		if (numMarkers)
		{
			size_t MFrames = m_ViconMarkers[0].m_arrMarkerPos.size();
			if (fFrameNo<MFrames)
			{
				for (uint32 m=0; m<numMarkers; m++)
				{
					const char* pMarkerName = m_ViconMarkers[m].sViconMarker.c_str();
					Vec3 p=m_ViconMarkers[m].m_arrMarkerPos[uint32(fFrameNo)];
					pAuxGeom->DrawSphere(	p,0.015f, RGBA8(0xff,0xff,0xff,0xff) );
				}
			}

			int32 mid = GetMarkerIDbyName("LHEE");
			if (mid>-1)
			{
				Vec3 p=m_ViconMarkers[mid].m_arrMarkerPos[uint32(fFrameNo)];
				pAuxGeom->DrawSphere(	p,0.005f, RGBA8(0xff,0xff,0xff,0xff) );
			}
			mid=GetMarkerIDbyName("LTOE");
			if (mid>-1)
			{
				Vec3 p=m_ViconMarkers[mid].m_arrMarkerPos[uint32(fFrameNo)];
				pAuxGeom->DrawSphere(	p,0.005f, RGBA8(0xff,0xff,0xff,0xff) );
			}
			mid=GetMarkerIDbyName("LMT5");
			if (mid>-1)
			{
				Vec3 p=m_ViconMarkers[mid].m_arrMarkerPos[uint32(fFrameNo)];
				pAuxGeom->DrawSphere(	p,0.005f, RGBA8(0xff,0xff,0xff,0xff) );
			}
			mid=GetMarkerIDbyName("LMT8");
			if (mid>-1)
			{
				Vec3 p=m_ViconMarkers[mid].m_arrMarkerPos[uint32(fFrameNo)];
				pAuxGeom->DrawSphere(	p,0.005f, RGBA8(0xff,0xff,0xff,0xff) );
			}

			mid=GetMarkerIDbyName("RHEE");
			if (mid>-1)
			{
				Vec3 p=m_ViconMarkers[mid].m_arrMarkerPos[uint32(fFrameNo)];
				pAuxGeom->DrawSphere(	p,0.005f, RGBA8(0xff,0xff,0xff,0xff) );
			}
			mid=GetMarkerIDbyName("RTOE");
			if (mid>-1)
			{
				Vec3 p=m_ViconMarkers[mid].m_arrMarkerPos[uint32(fFrameNo)];
				pAuxGeom->DrawSphere(	p,0.005f, RGBA8(0xff,0xff,0xff,0xff) );
			}
			mid=GetMarkerIDbyName("RMT5");
			if (mid>-1)
			{
				Vec3 p=m_ViconMarkers[mid].m_arrMarkerPos[uint32(fFrameNo)];
				pAuxGeom->DrawSphere(	p,0.005f, RGBA8(0xff,0xff,0xff,0xff) );
			}
			mid=GetMarkerIDbyName("RMT8");
			if (mid>-1)
			{
				Vec3 p=m_ViconMarkers[mid].m_arrMarkerPos[uint32(fFrameNo)];
				pAuxGeom->DrawSphere(	p,0.005f, RGBA8(0xff,0xff,0xff,0xff) );
			}

		}
	}

	//--------------------------------------------------------------------
	//----               Draw the Vicon-Skeleton                       ---
	//--------------------------------------------------------------------

	if(m_bShowViconSkeleton){
		size_t SFrames = 0;
		if(!m_viconJointsGlobal.empty())
			SFrames = m_viconJointsGlobal[0].size();

		if (fFrameNo<SFrames)
		{
			uint32 numViconJoints = m_viconJointsGlobal.size();
			for(uint32 j=0; j<numViconJoints; j++)
			{
				Vec3 t= m_viconJointsGlobal[j][uint32(fFrameNo)].t;
				pAuxGeom->DrawSphere(	t,0.01f, RGBA8(0xff,0x3f,0x00,0xff) );
				int32 p = m_inputNodeSpec[j].parent;
				if (p<0)
					continue;
				Vec3 pc = m_viconJointsGlobal[p][uint32(fFrameNo)].t;
				pAuxGeom->DrawLine(pc,RGBA8(0xff,0x00,0x00,0xff), t,RGBA8(0xff,0x00,0x00,0xff) );
			} 
		}
	}


	//------------------------------------------------------------------------------------
	//----                       draw the Biped Skeleton                                ----
	//------------------------------------------------------------------------------------

	size_t numKeys = m_frameCount;

	uint32 nFramesDiv4 = uint32(fFrameNo)/m_SamplingRatio;

	uint32 f0 = uint32(nFramesDiv4);
	uint32 f1 = f0+1;
	if(m_BipSkeleton.empty())
		return;

	if(f0 >=m_BipSkeleton[1].m_RelativePose2.size() || f1>=m_BipSkeleton[1].m_RelativePose2.size())
		return;

	f32 t	=	(f32)( (fFrameNo/m_SamplingRatio)-f0 );
	pIRenderer->Draw2dLabel(12,g_ypos,1.5f,color1,false,"blend: %f",t);
	g_ypos+=14;

	uint32 numBipedJoints = m_BipSkeleton.size();

	if (f1<numKeys)
	{

		{
			int32 j=GetBipedIDbyName("Bip01 L Forearm"); assert(j>-1);
			Vec3 axisZ = m_sampledBipedJointsGlobal[j][f0].q.GetColumn2();
			Vec3 v0 = m_sampledBipedJointsGlobal[ GetBipedIDbyName("Bip01 L UpperArm") ][f0].t- m_sampledBipedJointsGlobal[j][f0].t;
			Vec3 v1 = m_sampledBipedJointsGlobal[ GetBipedIDbyName("Bip01 L Hand") ][f0].t-m_sampledBipedJointsGlobal[j][f0].t;
			Vec3 cross=v0%v1;
			f32 dot=cross|axisZ;
			if (dot<=0)
			{
				pIRenderer->Draw2dLabel(12,g_ypos,1.5f,color1,false,"Inverse Bending on left elbow");
				g_ypos+=14;
			}
		}

		{
			int32 j=GetBipedIDbyName("Bip01 R Forearm"); assert(j>-1);
			Vec3 axisZ = m_sampledBipedJointsGlobal[j][f0].q.GetColumn2();
			Vec3 v0 = m_sampledBipedJointsGlobal[ GetBipedIDbyName("Bip01 R UpperArm") ][f0].t-m_sampledBipedJointsGlobal[j][f0].t;
			Vec3 v1 = m_sampledBipedJointsGlobal[ GetBipedIDbyName("Bip01 R Hand") ][f0].t-m_sampledBipedJointsGlobal[j][f0].t;
			Vec3 cross=v0%v1;
			f32 dot=cross|axisZ;
			if (dot<=0)
			{
				pIRenderer->Draw2dLabel(12,g_ypos,1.5f,color1,false,"Inverse Bending on right elbow");
				g_ypos+=14;
			}
		}


		{
			int32 j=GetBipedIDbyName("Bip01 L Calf"); assert(j>-1);
			Vec3 axisZ = m_sampledBipedJointsGlobal[j][f0].q.GetColumn2();
			Vec3 v0 = m_sampledBipedJointsGlobal[ GetBipedIDbyName("Bip01 L Thigh") ][f0].t-m_sampledBipedJointsGlobal[j][f0].t;
			Vec3 v1 = m_sampledBipedJointsGlobal[ GetBipedIDbyName("Bip01 L Foot") ][f0].t-m_sampledBipedJointsGlobal[j][f0].t;
			Vec3 cross=v0%v1;
			f32 dot=cross|axisZ;
			if (dot<=0)
			{
				pIRenderer->Draw2dLabel(12,g_ypos,1.5f,color1,false,"Inverse Bending on left leg");
				g_ypos+=14;
			}
		}

		{
			int32 j=GetBipedIDbyName("Bip01 R Calf"); assert(j>-1);
			Vec3 axisZ = m_sampledBipedJointsGlobal[j][f0].q.GetColumn2();
			Vec3 v0 = m_sampledBipedJointsGlobal[ GetBipedIDbyName("Bip01 R Thigh") ][f0].t-m_sampledBipedJointsGlobal[j][f0].t;
			Vec3 v1 = m_sampledBipedJointsGlobal[ GetBipedIDbyName("Bip01 R Foot")  ][f0].t-m_sampledBipedJointsGlobal[j][f0].t;
			Vec3 cross=v0%v1;
			f32 dot=cross|axisZ;
			if (dot<=0)
			{
				pIRenderer->Draw2dLabel(12,g_ypos,1.5f,color1,false,"Inverse Bending on right leg");
				g_ypos+=14;
			}
		}


		ISkeletonAnim* pISkeletonAnim = pInstance->GetISkeletonAnim();
		ISkeletonPose* pISkeletonPose = pInstance->GetISkeletonPose();
		for (int i=0;i<numBipedJoints;i++)
		{
			const char* strJointName = m_BipSkeleton[i].m_strJointName;
			int idx=pISkeletonPose->GetJointIDByName(strJointName);
			if (idx<0)
				continue;

			QuatT pjoint0	=	m_BipSkeleton[i].m_RelativePose2[f0];
			QuatT pjoint1	=	m_BipSkeleton[i].m_RelativePose2[f1];
			QuatT pjoint;
			pjoint.q.SetNlerp(pjoint0.q,pjoint1.q,t);
			pjoint.t.SetLerp( pjoint0.t,pjoint1.t,t);
			pISkeletonPose->SetPostProcessQuat(idx, pjoint); 
		} 
	}

}
//-----------------------------------------------------------------------------
// Name: MotionPlayback()
//-----------------------------------------------------------------------------
void CMotionAdaptorManager::MotionPlayback( ICharacterInstance* pInstance ) 
{
	f32 fFrameTime = GetIEditor()->GetSystem()->GetITimer()->GetFrameTime();

	f64 fFrameNo;
	fFrameNo = 0;

	uint32 frameTotal = m_frameCount;

	if(!m_bPlayMotion&& m_bUDraggingPlaySlider) // User dragging slider
	{
		fFrameNo = sliderPos/(f64)(CProgressSlider::NUM_INCREMENTS) * frameTotal;
		SmoothCD(m_fFrameNoSmooth, m_fFrameNoSmoothRate, fFrameTime, fFrameNo, 0.10f);
		MotionPlaybackOneFrame(pInstance, m_fFrameNoSmooth);
		fFrameNo = m_fFrameNoSmooth;
	}
	else if(!m_bPlayMotion && !m_bUDraggingPlaySlider){ // User paused the motion
		fFrameNo = sliderPos /(f64)(CProgressSlider::NUM_INCREMENTS) * frameTotal;
		MotionPlaybackOneFrame(pInstance, fFrameNo);
		m_bUDraggingPlaySlider = false;
	}
	else{ // Normal playback
		m_addFrameTime += fFrameTime;
		fFrameNo = m_addFrameTime*m_ViconFrameHertz;
		int32 pos = (int32)( (f32)(fFrameNo)/frameTotal*CProgressSlider::NUM_INCREMENTS); 
		m_pCharacterEditor->GetMotionAdaptorDlg()->SetProgressSliderPos(pos);
		MotionPlaybackOneFrame(pInstance, fFrameNo);
		m_bUDraggingPlaySlider = false;
	}

	if(fFrameNo > m_MovingSpeed.size() - 1)
		fFrameNo = m_MovingSpeed.size() -1;

	if(!m_MovingSpeed.empty())
	{
		char txt[128];
		sprintf(txt, "Biped Speed (m/s): %.2f", m_MovingSpeed[fFrameNo]);
		m_pCharacterEditor->GetMotionAdaptorDlg()->SetBipedSpeedText(txt);
	}

	if(frameTotal>0)
	{
		char txt[128];
		sprintf(txt, "%.3f", (f32)(fFrameNo) / frameTotal);
		m_pCharacterEditor->GetMotionAdaptorDlg()->SetProgressEditText(txt);
	}
}

//////////////////////////////////////////////////////////////////////////
inline int32 CMotionAdaptorManager::GetMarkerIDbyName(const char* strMarkerName)
{
	int32 idx=-1; 
	size_t numViconMarkers = m_ViconMarkers.size();
	if (numViconMarkers==0)
		return -1;

	for(uint32 i=0; i<numViconMarkers; i++)
	{
		uint32 res = stricmp(m_ViconMarkers[i].sViconMarker.c_str(),strMarkerName);
		if (res==0)
		{
			idx=i; break;
		}
	}
	return idx;
}


//////////////////////////////////////////////////////////////////////////
inline int32 CMotionAdaptorManager::GetBipedIDbyName(const char* strJointName)
{
	int32 idx=-1; 
	uint32 numBipedJoints = m_BipSkeleton.size();
	for(uint32 i=0; i<numBipedJoints; i++)
	{
		uint32 res = stricmp(m_BipSkeleton[i].m_strJointName, strJointName);
		if (res==0)
		{
			idx=i; break;
		}
	}

	return idx;
}

int32 CMotionAdaptorManager::CAF_Exporter( const char* strFileName ) 
{
	ICryPak* g_pIPak = gEnv->pCryPak;

	FILE* fExport =  g_pIPak->FOpen(strFileName, "wt");
	if (fExport==0)
	{
		char szRes[ 128 ];
		sprintf( szRes, "Unable to load the following file: %s\n\nEither file does not exist or path is invalid",strFileName , MB_OK);
		MessageBox(NULL, NULL, szRes, MB_OK);
		return -1;
	}

	//------------------------------------
	//---  write the file-header       ---
	//------------------------------------
	FILE_HEADER fh;

	g_pIPak->FWrite( &fh, sizeof(uint8), sizeof(FILE_HEADER), fExport );
	//         Same question for "CHUNK-HEADER"

	//-------------------------------------
	//---  write the number of chunks   ---
	//-------------------------------------
	uint32 numBipedJoints = m_outputNodeSpec.size();
	g_pIPak->FWrite( &numBipedJoints, sizeof(uint8), sizeof(uint32), fExport );  


	//-------------------------------------
	//---  write the chunk headers      ---
	//-------------------------------------
	CHUNK_HEADER ch[0x100]; 
	ch[0].ChunkType			= ChunkType_Timing; //Xiaomao: enum-4 bytes
	ch[0].ChunkVersion	= 0x0918;
	ch[0].FileOffset		= numBipedJoints*sizeof(CHUNK_HEADER) + 0x28;
	ch[0].ChunkID				= 0;
	uint32 nControllerOffset = ch[0].FileOffset+sizeof(TIMING_CHUNK_DESC_0918);
	size_t numKeys = m_frameCount / m_SamplingRatio;
	for (uint32 x=0; x<numBipedJoints; x++)
	{
		ch[1+x].ChunkType			= ChunkType_Controller;
		ch[1+x].ChunkVersion	= 0x0827;
		ch[1+x].FileOffset		= nControllerOffset;
		ch[1+x].ChunkID				=	x;
		nControllerOffset += sizeof(CONTROLLER_CHUNK_DESC_0827);
		nControllerOffset += uint32(numKeys*sizeof(CryKeyPQLog));
	}
	g_pIPak->FWrite( &ch[0], sizeof(uint8), (1+numBipedJoints)*sizeof(CHUNK_HEADER), fExport );  



	//-------------------------------------
	//---  write the timing             ---
	//-------------------------------------
	TIMING_CHUNK_DESC_0918 Timing;
	Timing.global_range.start	=	0;
	Timing.global_range.end		=	uint32(numKeys);
	g_pIPak->FWrite( &Timing, sizeof(uint8), sizeof(TIMING_CHUNK_DESC_0918), fExport );  


	//-------------------------------------
	//---  write the controller header  ---
	//-------------------------------------
	for(uint32 j=0; j<numBipedJoints; j++)
	{
		const char* pBonename = m_BipSkeleton[j].m_strJointName;
		if (m_BipSkeleton[j].m_hasJoint==0)
			continue;

		CONTROLLER_CHUNK_DESC_0827 controller;
		controller.numKeys				=	uint32(numKeys);
		controller.nControllerId	=	m_BipSkeleton[j].m_CRC32;
		g_pIPak->FWrite( &controller, sizeof(uint8), sizeof(CONTROLLER_CHUNK_DESC_0827), fExport );  

		std::vector<CryKeyPQLog> arrPQLogs;	
		arrPQLogs.resize(numKeys);
		uint32 time=0;
		for (uint32 k=0; k<numKeys; k++ )
		{				
			Quat q = m_sampledBipedJointsLocal[j][k].q;
			Vec3 t = m_sampledBipedJointsLocal[j][k].t*100.0f;
			arrPQLogs[k].nTime	=time;
			arrPQLogs[k].vRotLog=log(!q);
			arrPQLogs[k].vPos		=t;
			time += 0xa0;
		}
		g_pIPak->FWrite( &arrPQLogs[0], sizeof(uint8), numKeys*sizeof(CryKeyPQLog), fExport );  
	}

	g_pIPak->FClose( fExport );
	return 0;
}

// replace ' ' with '_'
void CMotionAdaptorManager::ChangeBoneName(const char* name, char newName[])
{
	int lenName = strlen(name);
	memcpy(newName, name, lenName*sizeof(char));
	for(int i=0; i< lenName; ++i)
	{
		if( newName[i] == ' ')
			newName[i] = '_';
	}
	newName[lenName] = '\0';
}

int32 CMotionAdaptorManager::HTR_Exporter(const char* strFileName)
{
	ICryPak* g_pIPak = gEnv->pCryPak;

	FILE* fExport =  g_pIPak->FOpen(strFileName, "wt");
	if (fExport==0)
	{
		char szRes[ 128 ];
		sprintf( szRes, "Unable to load the following file: %s\n\nEither file does not exist or path is invalid",strFileName );
		MessageBox(NULL, NULL, szRes, MB_OK);
		return -1;
	}

	//------------------------------------------------------------------------------
	// Export header

	g_pIPak->FPrintf(fExport,"#Comment line ignore any data following # character \n");
	g_pIPak->FPrintf(fExport,"#Hierarchical Translation and Rotation (.htr) file \n");
	g_pIPak->FPrintf(fExport,"[Header]	\n");	
	g_pIPak->FPrintf(fExport,"#Header keywords are followed by a single value \n");
	g_pIPak->FPrintf(fExport,"FileType HTR		#Single word string\n");
	g_pIPak->FPrintf(fExport,"DataType HTRS		#Translation followed by rotation and scale data\n");
	g_pIPak->FPrintf(fExport,"FileVersion 1		#integer\n");

	uint32 numBipedJoints = (uint32)m_outputNodeSpec.size();
	g_pIPak->FPrintf(fExport,"NumSegments %d		#integer\n",numBipedJoints);

	size_t numFrames = m_frameCount / m_SamplingRatio;
	g_pIPak->FPrintf(fExport,"NumFrames %d		#integer\n",numFrames);

	int32 nFrameRate= m_ViconFrameHertz / m_SamplingRatio; // ivo are you reading framrate from Vicon? //Xiaomao: make sure if we need to fix it.
	g_pIPak->FPrintf(fExport,"DataFrameRate %d		#integer, data frame rate in this file\n",nFrameRate);
	g_pIPak->FPrintf(fExport,"EulerRotationOrder ZYX\n");

	// ivo i cuoldnt find how to specify data in meters in HTR format (since you already divided by 1000).
	// So i will leave mm (millimiters) here and specify 1000 for scale. Result should be the same.
	g_pIPak->FPrintf(fExport,"CalibrationUnits mm\n");
	g_pIPak->FPrintf(fExport,"RotationUnits Degrees\n");
	g_pIPak->FPrintf(fExport,"GlobalAxisofGravity Y\n");
	g_pIPak->FPrintf(fExport,"BoneLengthAxis Y\n");
	g_pIPak->FPrintf(fExport,"ScaleFactor 1.00\n"); // ivo see previous note
	g_pIPak->FPrintf(fExport,"[SegmentNames&Hierarchy]\n");
	g_pIPak->FPrintf(fExport,"#CHILD	PARENT\n");

	// export the hierarchy 
	char newName[128], newNameParent[128];
	for(uint32 v=0; v<numBipedJoints; v++)
	{
		const char* pBonename = m_BipSkeleton[v].m_strJointName;

		ChangeBoneName(pBonename, newName); //replace " " with "_"
		const char* pParentBonename = NULL;
		int ii = m_outputNodeSpec[v].parent;
		if (m_outputNodeSpec[v].parent != -1)
		{
			int32 p = m_BipSkeleton[v].m_idxParent;
			pParentBonename=m_BipSkeleton[p].m_strJointName;
			ChangeBoneName(pParentBonename, newNameParent);
		}
		g_pIPak->FPrintf(fExport,"%s %s\n",newName,pParentBonename?newNameParent:"GLOBAL");

	} //v


	g_pIPak->FPrintf(fExport,"[BasePosition]\n");
	g_pIPak->FPrintf(fExport,"#SegmentName Tx, Ty, Tz, Rx, Ry, Rz, BoneLength\n");

	//------------------------------------------------------------------------------
	// Export base pose

	//ivo Each line in this section indicates how each bone is initially orientated within
	//it’s own local coordinate system.
	for(uint32 v=0; v<numBipedJoints; v++)
	{
		const char* pBonename = m_BipSkeleton[v].m_strJointName;

		ChangeBoneName(pBonename, newName);

		Quat q= m_outputNodeSpec[v].defaultPose.q;
		Vec3 t= m_outputNodeSpec[v].defaultPose.t;

		// ivo do you have bone lenght somewhere after conversion?
		float fBoneLen=0.0f;
		if (m_outputNodeSpec[v].parent != -1)
		{
			fBoneLen = m_outputNodeSpec[v].defaultPose.t.GetLength();
		}

		Ang3 Ang = RAD2DEG(Ang3::GetAnglesXYZ(q));						

		t.Set(0,0,0);
		Ang.Set(0,0,0);
		if (v==0)
			Ang.Set(-90,0,0);
		fBoneLen=10.0f;

		g_pIPak->FPrintf(fExport,"%s %f %f %f %f %f %f %f \n",newName,t.x,t.y,t.z,Ang.x,Ang.y,Ang.z,fBoneLen);

	} //v

	//------------------------------------------------------------------------------
	// Export motion data joint by joint

	g_pIPak->FPrintf(fExport,"#Beginning of Data. Separated by tabs\n");
	g_pIPak->FPrintf(fExport,"#Fr	Tx	Ty	Tz	Rx	Ry	Rz	SF\n");

	for(uint32 v=0; v<numBipedJoints; v++)
	{
		const char* pBonename = m_BipSkeleton[v].m_strJointName;

		g_pIPak->FPrintf(fExport,"[%s]\n",pBonename);
		for (uint32 k=0; k<numFrames; k++ )
		{				
			//Ivo do you have bone length somewhere after conversion?
			Quat q = m_sampledBipedJointsLocal[v][k].q;
			Vec3 t = m_sampledBipedJointsLocal[v][k].t*1000.0f;
			float fBoneLen= t.GetLength();
			Ang3 Ang = RAD2DEG(Ang3::GetAnglesXYZ(q));
			g_pIPak->FPrintf(fExport,"%d %f %f %f %f %f %f %f\n",k+1,  t.x,t.y,t.z, Ang.x,Ang.y,Ang.z,  fBoneLen);
		}
	}
	g_pIPak->FClose(fExport);

	return 0;

}
// Uses the source index of pOutputDescs to index into the input nodes and
// perform the remapping.
void CMotionAdaptorManager::RemapFrom(const QuatT *pInput, QuatT *pOutput, const uint32 outputCount, const SNodeDesc outputDescs[])
{
	QuatT inputNode, outputNode;
	for(int i=0; i<outputCount; ++i)
	{
		int iInputInd = outputDescs[i].source;

		// corresponding joint exists
		if(iInputInd != -1)
		{
			pOutput[i] = pInput[iInputInd];
		}
		// Joint doesn't exit
		else
		{
			pOutput[i] = outputDescs[i].defaultPose;
		}
	}
}

void CMotionAdaptorManager::RedoConversion()
{
	Convert2Biped(NULL);
}

void CMotionAdaptorManager::OnPlayPauseMotions()
{
	m_bPlayMotion = !m_bPlayMotion;
	if(m_bPlayMotion){
		m_pCharacterEditor->GetMotionAdaptorDlg()->SetPlayPauseText("Pause");
	}
	else{
		m_pCharacterEditor->GetMotionAdaptorDlg()->SetPlayPauseText("Play");
	}
}

void CMotionAdaptorManager::OnBatchConvertMotions(const std::vector<string>& vFilePaths)
{
	if(vFilePaths.empty())
		return;

	
	std::vector<CString> files;
	CFileDialog dlg(FALSE, "htr", NULL, OFN_OVERWRITEPROMPT, "HTR (*.htr)|*.htr|CAF(*.caf)|*.caf||");

	if(dlg.DoModal() != IDOK)
		return;

	string path = dlg.GetPathName().GetBuffer();
	string extName = path.substr(path.size()-3);
	string dirName = PathUtil::GetParentDirectory(path) + string("\\"); // Path of the folder in which converted motions will be stored in

	m_bPlayMotion = false;

	uint32 numFiles = (uint32)vFilePaths.size();

	for(uint32 i=0; i<numFiles; ++i)
	{		
		string fullPath = m_CurrentVFileDir + vFilePaths[i];
		
		char msg[128];
		sprintf(msg, "%d of %d: ", i+1, numFiles);

		string batchInfo = string("Batch info: converting ") + string(msg) + vFilePaths[i];
		m_pCharacterEditor->GetMotionAdaptorDlg()->SetBatchConvertInfoText(batchInfo.c_str());
		
		ClearAnimation();

		ISkeletonAnim* pISkeletonAnim = 	m_pModelViewportCE->GetCharacterBase()->GetISkeletonAnim();

		pISkeletonAnim->StopAnimationsAllLayers();		

		if(Convert2Biped(fullPath.c_str()) == FAILED)
		{
			ClearAnimation();
			return;
		}

		//------------------------------------------------------------------------------
		// Save files
		if(!m_animationLoaded)
		{
			MessageBox(NULL, NULL, "Please load motion to convert first.", MB_OK);
			return;
		}

		string nameNoExt = vFilePaths[i].substr(0, vFilePaths[i].find_last_of("."));
		
		if(extName.MakeLower() == string("htr"))
		{
			string saveName = dirName + nameNoExt + string(".htr");
			HTR_Exporter(saveName.c_str());
		}
		else if(extName.MakeLower() == string("caf"))
		{
			string saveName = dirName + nameNoExt + string(".caf");
			CAF_Exporter(saveName.c_str());
		}
	}
	


	

}

void CMotionAdaptorManager::OnConvertMotions(const std::vector<string>& vFilePaths)
{
	if(vFilePaths.empty())
		return;

	m_bPlayMotion = false;

	string fullPath = m_CurrentVFileDir + vFilePaths[0];
	ClearAnimation();

	ISkeletonAnim* pISkeletonAnim = 	m_pModelViewportCE->GetCharacterBase()->GetISkeletonAnim();

	pISkeletonAnim->StopAnimationsAllLayers();		


	//------------------------------------------------------------------------------
	// Cleanup motion data


	if(Convert2Biped(fullPath.c_str()) == FAILED)
	{
		ClearAnimation();
		return;
	}

	if(m_pModelViewportCE->m_Button_MOVE)
		ResetAnimationProgress();
	else 
	{
		ResumeAnimationProgress();
		m_bPlayMotion = true;
	}

	m_pCharacterEditor->GetMotionAdaptorDlg()->SetPlayPauseText("Pause");
}

void CMotionAdaptorManager::OnLoadVFiles(std::vector <string>& fileNames)
{
	ClearAnimation();

	std::vector<CString> files;
	// SelectMultipleFiles will limit users to select files in the engine folder.
	// SelectFiles is more general but doesn't work with Bin64
	if( CFileUtil::SelectMultipleFiles( EFILE_TYPE_ANY,files,"Vicon Motion (*.v)|*.v||","" ) )
	{
		ISkeletonAnim* pISkeletonAnim = 	m_pModelViewportCE->GetCharacterBase()->GetISkeletonAnim();

		pISkeletonAnim->StopAnimationsAllLayers();		

		uint32 iSize = (uint32)files.size();
		for(uint32 i=0; i<iSize; ++i)
		{
			string name = files[i].GetBuffer();
			size_t ind = name.find_last_of("/\\");
			m_CurrentVFileDir = name.substr(0, ind+1);
			string onlyName = name.substr(ind+1);
			fileNames.push_back(onlyName);
		}
	}
}


void CMotionAdaptorManager::OnSaveMotion()
{
	if(!m_animationLoaded)
	{
		MessageBox(NULL, NULL, "Please load motion to convert first.", MB_OK);
		return;
	}
	std::vector<CString> files;
	CFileDialog dlg(FALSE, "htr", NULL, OFN_OVERWRITEPROMPT, "HTR (*.htr)|*.htr|CAF(*.caf)|*.caf||");

	if(dlg.DoModal() != IDOK)
		return;


	string path = dlg.GetPathName().GetBuffer();
	string extName = path.substr(path.size()-3);
	if(extName.MakeLower() == string("htr"))
	{
		HTR_Exporter(path.c_str());
	}
	else if(extName.MakeLower() == string("caf"))
	{
		CAF_Exporter(path.c_str());
	}
}

void CMotionAdaptorManager::OnProgressSliderDrag(UINT pos)
{
	m_bPlayMotion = false;
	m_pCharacterEditor->GetMotionAdaptorDlg()->SetPlayPauseText("Play");
	m_bUDraggingPlaySlider = true;
	sliderPos = pos;
}