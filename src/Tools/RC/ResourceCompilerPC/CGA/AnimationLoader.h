////////////////////////////////////////////////////////////////////////////
//
// Crytek Engine Source File.
// Copyright (C), Crytek Studios, 2001-2006.
// -------------------------------------------------------------------------
// File name: AnimationInfoLoader.h
// Version: v1.00
// Created: 22/6/2006 by Alexey Medvedev.
// Compilers: Visual Studio.NET 2005
// Description:
// -------------------------------------------------------------------------
// History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef _ANIMATION_LOADER
#define _ANIMATION_LOADER

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



#include <CryCharAnimationParams.h>
#include "Controller.h"
#include "AnimationInfoLoader.h"
#include "IXMLSerializer.h"
#include "CGF\CGFLoader.h"
#include "CGF\CGFSaver.h"
#include "CGF\LoaderCAF.h"
#include "ControllerPQLog.h"
#include "ControllerPQ.h"
#include "SkeletonInfo.h"
#include "CGFContent.h"
#include "ConvertContext.h"
#include "CryVersion.h"
#include "CompressionController.h"
#include "GlobalAnimationHeader.h"
#include "GlobalAnimationHeaderCAF.h"
#include "GlobalAnimationHeaderAIM.h"

struct DebugJoint
{
	DebugJoint()
	{
		m_idxParent = -1;
		m_RelativeQuat.SetIdentity();
		m_AbsoluteQuat.SetIdentity();
	}

	int16 m_idxParent; //index of the parent-joint
	QuatT m_RelativeQuat; //current relative rotation and position of animation
	QuatT m_AbsoluteQuat;
};


class ILoaderCGFListener;
class CSkeletonInfo;
struct ConvertContext;
class CContentCGF;
class CTrackStorage;

class CAnimationCompressor
{
public:
	CAnimationCompressor(CSkeletonInfo * pInfo);
	~CAnimationCompressor(void);

	enum EDeleteMethod { eAll, eOld, eNew, eSkipDelete};

	bool LoadAnimationFromFileCAF(const char* name, const char* savename, ILoaderCGFListener* pListener, SAnimationDesc& animDesc, bool bNeedCalculate);
	bool ProcessAnimation(ConvertContext& cc, ILoaderCGFListener* pListener, CTrackStorage* pStorage, bool bNeedCalculate);
	uint32 SaveAnimationToFile(const char* name, ILoaderCGFListener* pListener, const ConvertContext& cc, bool bSaveInfo, FILETIME timeStamp);

	bool LoadAnimationFromFileAIM(const char* name, GlobalAnimationHeaderAIM& rAIM);
	void ProcessAimPoses(CSkinningInfo* pModelSkeleton, const char* animname, GlobalAnimationHeaderAIM& rAIM, int32 nDirectionIdx,uint32 nAnimTokenCRC32); 


	void SaveToDB(const string& strFilePathDBA, CTrackStorage * pStorage, bool bUpdate);

	void MakeCompression(ConvertContext& cc, uint32 numBones, float coeff, CompressionInfo &compInfo, CCompressonator &compController, uint32 format, uint32 footskel,uint32 PrintDebugText)
	{
		CreateLoopError(numBones, coeff, compInfo, format, footskel);
		compController.CreateCompression(cc,m_GlobalAnimationHeader, m_CompressedAnimation,  compInfo, m_pInfo, m_AnimDesc, m_GAID,PrintDebugText,0);//, m_pInfo);
	}

	void CreateLoopError(uint32 numBones, float coeff, CompressionInfo &compInfo, uint32 format, uint32 footskel)
	{
		compInfo.m_Info.clear();

		compInfo.m_Info.resize(numBones);

		for (uint32 i=0; i<numBones; i++)
		{
			CompressionLevelInfo info;
			info.boneLevel = 0; //not used yet
			info.m_bPosCompression = eCompression;//eCompression;
			info.m_bRotCompression = eCompression;// eCompression;
			info.m_bSclCompression = eNoCompression;

			info.m_PositionError = 0.00000001f * coeff;
			info.m_RotationError = 0.00000001f * coeff;

			info.m_ScaleFormat = eNoCompressVec3;
			info.m_PositionFormat = eNoCompressVec3;
			info.m_RotationFormat = format;//eSmallTree64BitExtQuat;//eSmallTree64BitQuat;//eNoCompressQuat;
			info.m_DeletePos = 1; 
			info.m_DeleteRot = 1; 

			compInfo.m_Info[i] = info;

		}

	}

	void MakeErrors(uint32 numBones, CompressionInfo &compInfo, int16 master, int16 footskel, SAnimationDesc * pDesc, CSkeletonInfo * pSkeleton, int weapon_bone);

	void DefaultError(int16 master, CompressionInfo & compInfo, uint32 bone, int16 footskel)
	{

		CompressionLevelInfo info;
		info.boneLevel = 0; //not used yet
		info.m_bPosCompression = eCompression;//eCompression;
		info.m_bRotCompression = eCompression;// eCompression;
		info.m_bSclCompression = eNoCompression;
		info.m_DeletePos = 1; //m_AnimDesc.m_DeletePosController;
		info.m_DeleteRot = 1; //m_AnimDesc.m_DeleteRotController;
		info.m_ScaleFormat = eNoCompressVec3;
		info.m_PositionFormat = eNoCompressVec3;

		if (master == -1)
		{
			info.m_PositionError = 0.00000001f * (float)m_AnimDesc.m_CompressionQuality;
			info.m_RotationError = 0.00000001f* (float)m_AnimDesc.m_CompressionQuality;
			info.m_RotationFormat = /*eSmallTree64BitExtQuat;//*/eAutomaticQuat;//eSmallTree64BitQuat;//eNoCompressQuat;
		}
		else
		{
			// after increasing precision 64Bit we can use this errors
			info.m_PositionError = 0.000000001f * (float)m_AnimDesc.m_CompressionQuality;
			info.m_RotationError = 0.000000001f * (float)m_AnimDesc.m_CompressionQuality;
			info.m_RotationFormat = eSmallTree64BitExtQuat;//eSmallTree64BitQuat;//eNoCompressQuat;
		}

		if ((bone == 0 ) && footskel)
		{
			if (m_AnimDesc.m_fRootQuality == 0.0f)
			{
				info.m_bPosCompression = eNoCompression;//eCompression;
				info.m_bRotCompression = eNoCompression;// eCompression;
			}
			else
				info.m_PositionError = 0.005f * (float)m_AnimDesc.m_CompressionQuality;
//			info.m_bPosCompression = eUseOld;
	//		info.m_bRotCompression = eUseOld;
		}

		compInfo.m_Info.push_back(info);

	}
	void DeleteOldChunk(EDeleteMethod delFlag, bool bEraseAnotherChunks);

	bool HasNewFormat();

	bool HasOldFormat() const
	{
		return m_bOldFormat;
	}

	bool HasTCBFormat() const
	{
		return m_bTCBFormat;
	}


	//Vec3 GetPosition(IController * pController, float )
	bool CompareKeyTimes(KeyTimesInformationPtr& ptr1, KeyTimesInformationPtr& ptr2);

	void EvaluateStartLocation(ConvertContext& cc);
	void ReplaceRootByLocator(ConvertContext& cc, uint32 CheckLocator);
	void EvaluateSpeed(ILoaderCGFListener * pListener);//uint32 AnimID);
	void DetectCycleAnimations(ILoaderCGFListener * pListener);//uint32 AnimID);

	void CreateNewController(std::vector<int>& arrFullTimes, std::vector<PQLog>& arrFullQTKeys, int numController, float fRotErr = 0.000000001f, float fPosErr = 0.000000001f );

	void SetFootplantBitsAutomatically( std::vector< std::vector<DebugJoint> >& arrSkeletons, int32 nAnimID,int32 nGlobalAnimID,int32 lHidx,int32 rHidx,int32 lTidx,int32 rTidx, const SAnimationDesc& desc );
	void CreateSkeletonArray( std::vector< std::vector<DebugJoint> >& arrSkeletons, GlobalAnimationHeaderCAF& header );
	void SetFootplantVectors( std::vector< std::vector<DebugJoint> >& arrSkeletons, uint32 nAnimID, SFootPlant& rFootPlants, uint32 nGlobalAnimID );


	int  SaveControllers(CSaverCGF& saver, /*std::vector<IController* >*/TControllersVector& m_arrController);
	void SaveCController(CController * pController, CSaverCGF& saver);
	void SaveWaveletController(CCompressedController * pController, CSaverCGF& saver);
	bool GetError(std::vector< std::vector<DebugJoint> >& arrSkeletons1, std::vector< std::vector<DebugJoint> >& arrSkeletons2, int nJoint, float error);

	DWORD		m_dwTimestamp;

	GlobalAnimationHeaderCAF m_GlobalAnimationHeader;
	GlobalAnimationHeaderCAF m_CompressedAnimation;
	GlobalAnimationHeaderCAF m_LastGoodAnimation;
	CSkeletonInfo * m_pInfo;
	SAnimationDesc m_AnimDesc;
	string m_sName;
	CChunkFile m_ChunkFile;
	bool m_bNewFormat;
	bool m_bOldFormat;
	bool m_bTCBFormat;
	std::vector<ColorRGB> g_arrDistMap24;

	int m_GAID;

};

#endif