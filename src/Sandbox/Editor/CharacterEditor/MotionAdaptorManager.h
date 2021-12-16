////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:  MotionAdaptorManager.h
//  Version:    v1.00
//  Created:    24/4/2009 by Xiaomao Wu.
//  Compiler:   Visual Studio 2005 Professional
//  Description:

// -------------------------------------------------------------------------
//  History:
//    29/05/09: Changed from MotionAdaptorMainDlg to MotionAdaptorManger
////////////////////////////////////////////////////////////////////////////

#ifndef __MOTIONADAPTORMANAGER_H__
#define __MOTIONADAPTORMANAGER_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "resource.h"
#include "..\CharacterEditor/ProgressSlider.h"
#include "AnimEventEditor/AnimEventControlView.h"
#include "ProgressSlider.h"
#include "AnimEventEditor/AnimEventControlView.h"
#include "afxwin.h"
class CModelViewportCE;
class IAnimEventEditor;
class IAnimEventView;

struct ISkeletonAnim;
struct CAnimation;

class CModelViewportCE;
class CCharacterEditor;

class CMotionAdaptorManager
{
	DECLARE_DYNAMIC(CMotionAdaptorManager)

public:
	CMotionAdaptorManager(CWnd* pParent = NULL);   // standard constructor
	~CMotionAdaptorManager();

	void MotionPlayback(ICharacterInstance* pInstance); 

	CModelViewportCE* m_pModelViewportCE;
	CCharacterEditor* m_pCharacterEditor;
	
	void SetModelViewportCE (CModelViewportCE* pMCE){m_pModelViewportCE = pMCE;}
	CModelViewportCE* GetModelViewportCE() {return m_pModelViewportCE;}
	void SetCharacterEditor(CCharacterEditor* pCE){m_pCharacterEditor = pCE;}
	CCharacterEditor* GetCharacterEditor() {return m_pCharacterEditor;}
	void ClearAnimation();
	void ResetAnimationProgress();
	void ResumeAnimationProgress();
	void OnConvertMotions(const std::vector<string>& vFilePaths);
	void OnBatchConvertMotions(const std::vector<string>& vFilePaths);
	void OnLoadVFiles(std::vector <string>& fileNames);
	void OnSaveMotion();
	void OnProgressSliderDrag(UINT pos);
	void OnPlayPauseMotions();
	void RedoConversion();
	void SetDispViconMarkers(const bool bDisplay) {m_bShowViconMarkers = bDisplay;}
	void SetDispViconSkeleton(const bool bDisplay) {m_bShowViconSkeleton = bDisplay;}
	void SetFixInverseBending(const bool bSet) {m_bFixInverseBending = bSet;}
	void SetMotionRetargeting(const bool bSet) {m_bMotionRetargeting = bSet;}
	void SetAdditiveMotion(const bool bSet) {m_bAdditiveMotion = bSet;}
	void SetAdjustFoot(const bool bSet) {m_bAdjustFoot = bSet;}
	void SetProgressSliderPos(const UINT& pos){sliderPos = pos;}
	bool GetDispViconMarkers() const {return m_bShowViconMarkers;}
	bool GetDispViconSkeleton() const {return m_bShowViconSkeleton;}
	bool GetFixInverseBending() const {return m_bFixInverseBending;}
	bool GetMotionRetargeting() const {return m_bMotionRetargeting;}
	bool GetAdditiveMotion() const {return m_bAdditiveMotion;}
	bool GetAdjustFoot() const {return m_bAdjustFoot;}
	uint32 GetViconJointNum() const {return (uint32)m_outputNodeSpec.size();}
	uint32 GetViconFrameCount() const {return m_frameCount;}
public:
	//------------------------------------------------------------------------------
	// Struct definition

	struct SNodeDesc
	{
		// Default pose.
		QuatT defaultPose;
		// Index of the parent node, -1 if root node.
		int32 parent;
		// Index of the source node, -1 if no source exists.
		int32 source;
	};

	struct tViconMarkers
	{	
		tViconMarkers() {};

		tViconMarkers(const char* szViconMarker)
		{
			sViconMarker = string(szViconMarker);
		}

		string	sViconMarker;
		std::vector<Vec3r> m_arrMarkerPos;
	};


	struct SBipedJoint
	{
		int32 m_hasJoint;
		int32 m_idxParent;
		int32 m_IsInitialized;
		const char* m_strJointName;
		uint32 m_CRC32;
		QuatT	m_DefRelativePose;			
		QuatT	m_DefAbsolutePose;			
		std::vector<QuatT> m_RelativePose2;			
		std::vector<QuatT> m_AbsolutePose2;			

		ILINE SBipedJoint(const char* jname, const int32 idParent) 
		{
			m_IsInitialized=0;
			m_CRC32=0xAA55AA55;
			m_hasJoint=0; 
			m_idxParent=idParent; 
			m_strJointName=jname; 
		};
	};

	struct SViconJointProperty
	{
		string jointName;
		Vec3i axisSwapper;
	};


private:
	void VerfyReadData(std::vector< std::vector<QuatT> >& data);
	void VerfyReadData(std::vector< std::vector<Vec3r> >& data);

	void RemapFrom(const QuatT *pInput, QuatT *pOutput, const uint32 outputCount, const SNodeDesc outputDescs[]);
	int32 GetMarkerIDbyName(const char* strMarkerName);
	int32 GetBipedIDbyName(const char* strJointName);
	void MotionPlaybackOneFrame(ICharacterInstance* pInstance, f32 fFrameNo);
	int32 HTR_Exporter(const char* strFileName);
	int32 CAF_Exporter(const char* strFileName);

	void ChangeBoneName(const char* name, char newName[]);
	void InitBipedSkeleton();
	void CalcuMovingSpeed(const int32 iSmoothSize);
	bool CheckCorrespondingJointNames();
	int32 Convert2Biped(const char* strFileName);
	int32 ReadVFile(const char* strFileName, std::vector< std::vector<QuatT> >& pViconJoints, std::vector< std::vector<QuatT> >& readGlobalData, 
		const SNodeDesc viconNodesSpec[], const std::vector<SViconJointProperty>& viconJointProperty,
		std::vector<tViconMarkers>& viconMarkers, const uint32 viconJointCount, uint32& frameCount, f64& frameHertz);

	int32 GetDefaultPoses(SNodeDesc outputNodesSpec[], const int numBipedJoints);
	int32 Resampling(const std::vector< std::vector<QuatT> >& nodes, std::vector< std::vector<QuatT> >& resampledNodes, 
		const std::vector<tViconMarkers>& markerNodes, std::vector<tViconMarkers>& resampledMarkerNodes, 
		const int32 outputFrameCount, const int32 jointCount, const int markerCount, const f32 samplingRatio);

	int32 Retarget( const int32 outputLfootId, const int32 outputRfootId, const int32 outputPelvisId,
		const std::vector< std::vector<QuatT> >& pInputNodes, std::vector< std::vector<QuatT> >& m_sampledBipedJointsLocal,
		const SNodeDesc outputNodesSpec[], const int frameCount);

	int32 SetupViconJointProperty(std::vector<SViconJointProperty>& viconJointProperty);

	int32 AdjustFeetWithViconMarkers(const std::vector<int32>& outputLfeetIds, const std::vector<int32>& outputRfeetIds,
		const int32 outputLfootId, const int32 outputRfootId, const uint32 outputJointCount, const uint32 frameCount, 
		const uint32 sampleingRatio, const int32 outputRootJointId, const SNodeDesc* pOutputNodeSpec,
		const std::vector<tViconMarkers>& viconMarkers, const std::map<string, int32>& viconMarkerNameMap,
		std::vector< std::vector<QuatT>>& pOutputNodes, std::vector< std::vector<QuatT> >& outputAbsolutePoses);

	int32 FixInverseBendingKnee(std::vector< std::vector<QuatT> >& outputJoints, uint32 frameCount, std::vector< std::vector<QuatT> >& absoluteOutputPoses,
		const int32 outputPelvisId, const std::vector<int32>& legJointIds);

	int32 FixInverseBendingElbow(std::vector< std::vector<QuatT> >& outputJoints, uint32 frameCount, std::vector< std::vector<QuatT> >& absoluteOutputPoses,
		const std::vector<int32>& armJointIds);

	int32 FixInverseBending(std::vector< std::vector<QuatT> >& outputJoints, uint32 frameCount, std::vector< std::vector<QuatT> >& absoluteOutputPoses,
		const int32 outputPelvisId, const std::vector<int32>& lLegJointIds, const std::vector<int32> rLegJointIds,
		const std::vector<int32>& lArmIds, const std::vector<int32>& rArmIds);

	int32 CalcuAbsolutePoses(const std::vector<std::vector<QuatT> >& m_sampledBipedJointsLocal,	std::vector< std::vector<QuatT> >& m_sampledBipedJointsGlobal,
		const int32 outputPelvisID, const int32 outputRootJointId, uint32 numOutputNode, const uint32 finalFrameCount, const SNodeDesc outputNodesSpec[]);

	int32 GetMissingBoneSegments(std::vector< std::vector<std::pair<int32, int32> > >& segments, const SNodeDesc outputNodeSpec[], const uint32 numOutput);

	int32 ApplyAdditiveMotion(const SNodeDesc outputNodesSpec[], const uint32 numOutputNode, const int32 outputPelvisID,
		const int32 outputRootJointId, const uint32 finalFrameCount);

	int32 GetFirstChildId(const int32 jointId, const SNodeDesc outputNodeSpec[], const uint32 numOutputNode);


private:
	int32 m_speedSmoothSize;
	bool m_bPlayMotion;
	f64 m_addFrameTime;
	f64 m_fFrameNoSmooth;
	f64 m_fFrameNoSmoothRate;
	bool m_bUDraggingPlaySlider;
	int32 m_SpeedInterval;

	UINT sliderPos;

	std::vector<f32> m_MovingSpeed;
	string m_CurrentVFileDir;// Just the directory, not including v file names.
	bool m_bShowViconMarkers;
	bool m_bShowViconSkeleton;
	bool m_bFixInverseBending;
	bool m_bMotionRetargeting;
	bool m_bAdditiveMotion;
	bool m_bAdjustFoot;

	std::vector<tViconMarkers> m_ViconMarkers;
	std::vector<SBipedJoint> m_BipSkeleton;
	std::vector<SViconJointProperty> m_viconJointProperty;

	std::vector<tViconMarkers> m_sampledViconMarkers;
	std::vector< std::vector<QuatT> > m_sampledBipedJointsLocal;
	std::vector< std::vector<QuatT> > m_sampledBipedJointsGlobal;
	std::vector< std::vector<QuatT> > m_viconJointsGlobal;

	std::vector< std::vector<QuatT> > m_viconJointsBackup;
	
	uint32 m_frameCount; // vicon frames before resampling
	uint32 m_inputPelvisId;
	bool m_animationLoaded;

	std::vector<SNodeDesc> m_inputNodeSpec;
	std::vector<SNodeDesc> m_outputNodeSpec;

	f32 m_SamplingRatio;
	f64 m_ViconFrameHertz;

};

#endif