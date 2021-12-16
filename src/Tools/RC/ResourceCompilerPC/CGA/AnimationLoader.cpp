#include "StdAfx.h"

#include "AnimationLoader.h"
#include "CGF\LoaderCAF.h"
#include "CompressionController.h"
#include "AnimationManager.h"
#include "TrackStorage.h"
#include <crc32.h>
#include "AnimationCompiler.h"

extern Crc32Gen g_crcGen;


CAnimationCompressor::CAnimationCompressor(CSkeletonInfo* pInfo) 
: m_pInfo(pInfo)
, m_bNewFormat(false)
, m_bOldFormat(false)
, m_bTCBFormat(false)
{
}

CAnimationCompressor::~CAnimationCompressor()
{
}


bool CAnimationCompressor::HasNewFormat()
{
	return m_bNewFormat;
}

bool CAnimationCompressor::LoadAnimationFromFileCAF(const char * name, const char * savename, ILoaderCGFListener * pListener, SAnimationDesc& animDesc, bool bNeedCalculate)
{
	CLoaderCAF cgfLoader;

	m_sName = savename;
	FixString(m_sName);


	if (bNeedCalculate)
	{
		cgfLoader.m_bLoadOldChunk = true;
	}

	CInternalSkinningInfo* pSkinningInfo = cgfLoader.LoadCAF(name,&m_ChunkFile, 0);

	if (!pSkinningInfo)
	{
		const char* errorMsg = cgfLoader.GetLastError();
		if (errorMsg)
			RCLogError( "Failed to load animation file %s - %s",(const char*)name,errorMsg );
		return false;
	}

	m_bNewFormat = pSkinningInfo->m_bNewFormat;
	m_bOldFormat = pSkinningInfo->m_bOldFormat;
	m_bTCBFormat = pSkinningInfo->m_bTCBFormat;
	//----------------------------------------------------------------------------------------------

	GlobalAnimationHeaderCAF& rGlobalAnim = m_GlobalAnimationHeader;


	rGlobalAnim.m_nTicksPerFrame	= TICKS_PER_FRAME;
	rGlobalAnim.m_fSecsPerTick		= SECONDS_PER_TICK;
	rGlobalAnim.m_nStartKey				= pSkinningInfo->m_nStart;
	rGlobalAnim.m_nEndKey					= pSkinningInfo->m_nEnd;
	//	assert(rGlobalAnim.m_nTicksPerFrame==0xa0);

	int32 fTicksPerFrame  = rGlobalAnim.m_nTicksPerFrame;
	f32		fSecsPerTick		= rGlobalAnim.m_fSecsPerTick;
	f32		fSecsPerFrame		= fSecsPerTick * fTicksPerFrame;
	rGlobalAnim.m_fStartSec = rGlobalAnim.m_nStartKey * fSecsPerFrame;
	rGlobalAnim.m_fEndSec  = rGlobalAnim.m_nEndKey * fSecsPerFrame;
	if(rGlobalAnim.m_fEndSec<=rGlobalAnim.m_fStartSec)
		rGlobalAnim.m_fEndSec  = rGlobalAnim.m_fStartSec+(1.0f/30.0f);
	assert(rGlobalAnim.m_fStartSec>=0);
	assert(rGlobalAnim.m_fEndSec>=0);



	rGlobalAnim.m_fSpeed		= pSkinningInfo->m_Speed;
	rGlobalAnim.m_fDistance = pSkinningInfo->m_Distance;
	rGlobalAnim.m_fSlope		= pSkinningInfo->m_Slope;

	rGlobalAnim.m_FootPlantVectors.m_LHeelEnd		= pSkinningInfo->m_LHeelEnd;
	rGlobalAnim.m_FootPlantVectors.m_LHeelStart = pSkinningInfo->m_LHeelStart;
	rGlobalAnim.m_FootPlantVectors.m_LToe0End		= pSkinningInfo->m_LToe0End;
	rGlobalAnim.m_FootPlantVectors.m_LToe0Start = pSkinningInfo->m_LToe0Start;

	rGlobalAnim.m_FootPlantVectors.m_RHeelEnd		= pSkinningInfo->m_RHeelEnd;
	rGlobalAnim.m_FootPlantVectors.m_RHeelStart = pSkinningInfo->m_RHeelStart;
	rGlobalAnim.m_FootPlantVectors.m_RToe0End		= pSkinningInfo->m_RToe0End;
	rGlobalAnim.m_FootPlantVectors.m_RToe0Start = pSkinningInfo->m_RToe0Start;

	rGlobalAnim.m_nFlags = pSkinningInfo->m_nAssetFlags;
	rGlobalAnim.m_StartLocation2 = pSkinningInfo->m_StartLocation;

	rGlobalAnim.m_FootPlantBits.assign(pSkinningInfo->m_FootPlantBits.begin(), pSkinningInfo->m_FootPlantBits.end());


	uint32 numController = pSkinningInfo->m_arrControllerId.size();
	rGlobalAnim.m_arrController.resize(numController);

	m_LastGoodAnimation = m_CompressedAnimation = m_GlobalAnimationHeader;
	for(uint32 i=0; i<numController; i++ )
	{
		rGlobalAnim.m_arrController[i] = (IController*)pSkinningInfo->m_pControllers[i];//pController;
		rGlobalAnim.m_arrController[i]->m_pqGlobalAnimationHeader = &rGlobalAnim;
	}

	static int volatile g_AddHeader;
	WriteLock lock(g_AddHeader);
	m_GAID = CAnimationManager::GetInst().m_arrGlobalAnimations.size();
	CAnimationManager::GetInst().m_arrGlobalAnimations.push_back(m_GlobalAnimationHeader);
	m_AnimDesc = animDesc;

	return true;
}

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

bool CAnimationCompressor::LoadAnimationFromFileAIM(const char* name, GlobalAnimationHeaderAIM& rAIM)
{
	CLoaderCAF cgfLoader;
	cgfLoader.m_bLoadOldChunk = true;

	CInternalSkinningInfo* pSkinningInfo = cgfLoader.LoadCAF(name,&m_ChunkFile, 0);

	if (!pSkinningInfo)
	{
		const char* errorMsg = cgfLoader.GetLastError();
		if (errorMsg)
			RCLogError( "Failed to load animation file %s - %s",(const char*)name,errorMsg );
		return false;
	}

	m_bNewFormat = pSkinningInfo->m_bNewFormat;
	m_bOldFormat = pSkinningInfo->m_bOldFormat;
	m_bTCBFormat = pSkinningInfo->m_bTCBFormat;
	//----------------------------------------------------------------------------------------------

	f32 fSecsPerFrame = SECONDS_PER_TICK * TICKS_PER_FRAME;
	uint32 nStartKey	= pSkinningInfo->m_nStart;
	uint32 nEndKey		= pSkinningInfo->m_nEnd;
	rAIM.m_fStartSec	= nStartKey * fSecsPerFrame;
	rAIM.m_fEndSec	  = nEndKey * fSecsPerFrame;
	if(rAIM.m_fEndSec<=rAIM.m_fStartSec)
		rAIM.m_fEndSec  = rAIM.m_fStartSec+(1.0f/30.0f);
	assert(rAIM.m_fStartSec>=0);
	assert(rAIM.m_fEndSec>=0);
	rAIM.m_fTotalDuration = rAIM.m_fEndSec-rAIM.m_fStartSec;

	uint32 numController = pSkinningInfo->m_arrControllerId.size();
	rAIM.m_arrController.resize(numController);
	for(uint32 i=0; i<numController; i++ )
	{
		rAIM.m_arrController[i] = (IController*)pSkinningInfo->m_pControllers[i];//pController;
	}

	rAIM.OnAssetCreated();
	rAIM.OnAssetLoaded();
	return true;
}



uint32 CAnimationCompressor::SaveAnimationToFile( const char * name, ILoaderCGFListener * pListener, const ConvertContext &cc, bool bSaveInfo, FILETIME timeStamp)
{
	CSaverCGF cgfSaver( name, m_ChunkFile);

	//save information here
	SFileVersion fv = cc.pRC->GetFileVersion();

	//if (pCGF.get()) 
	//{

	//	pCGF->GetExportInfo()->rc_version[0] = fv.v[0];
	//	pCGF->GetExportInfo()->rc_version[1] = fv.v[1];
	//	pCGF->GetExportInfo()->rc_version[2] = fv.v[2];
	//	pCGF->GetExportInfo()->rc_version[3] = fv.v[3];
	//	sprintf( pCGF->GetExportInfo()->rc_version_string," RCVer:%d.%d ",fv.v[2],fv.v[1] );
	//	cgfSaver.SetContent( pCGF.get() );
	//	// Only store 
	//	cgfSaver.SaveExportFlags();
	//}
	//cgfSaver.SaveNodes();

	if (bSaveInfo)
	{
		std::auto_ptr<char> mem;

		struct SPEED_INFO 
		{
			f32 Speed;
			f32 Distance;
			f32 Slope;
			uint32 m_Flags;
			f32 m_MoveDir[3];
			QuatT m_StartLocation;
		} speed;

		speed.Speed = m_GlobalAnimationHeader.m_fSpeed;
		speed.Distance = m_GlobalAnimationHeader.m_fDistance;
		speed.Slope= m_GlobalAnimationHeader.m_fSlope;
		speed.m_Flags=0;
		if (m_GlobalAnimationHeader.IsAssetCycle())
			speed.m_Flags |= CA_ASSET_CYCLE;
		if (m_AnimDesc.m_AdditiveAnimation)
			speed.m_Flags |= CA_ASSET_ADDITIVE;


		speed.m_StartLocation = m_GlobalAnimationHeader.m_StartLocation2;

		cgfSaver.SaveSpeedInfo2(&speed, sizeof(SPEED_INFO));
		//		cgfSaver.SaveSpeedInfo(&speed, sizeof(SPEED_INFO));

		if (m_AnimDesc.m_FootPlant)
		{
			struct FOOT_PLANT_INFO
			{
				int nPoses;

				f32 m_LHeelStart,m_LHeelEnd;
				f32 m_LToe0Start,m_LToe0End;
				f32 m_RHeelStart,m_RHeelEnd;
				f32 m_RToe0Start,m_RToe0End;
			} footplant;

			footplant.nPoses = m_GlobalAnimationHeader.m_FootPlantBits.size();

			footplant.m_LHeelEnd = m_GlobalAnimationHeader.m_FootPlantVectors.m_LHeelEnd;
			footplant.m_LHeelStart = m_GlobalAnimationHeader.m_FootPlantVectors.m_LHeelStart;
			footplant.m_LToe0Start = m_GlobalAnimationHeader.m_FootPlantVectors.m_LToe0Start;
			footplant.m_LToe0End = m_GlobalAnimationHeader.m_FootPlantVectors.m_LToe0End;

			footplant.m_RHeelEnd = m_GlobalAnimationHeader.m_FootPlantVectors.m_RHeelEnd;
			footplant.m_RHeelStart = m_GlobalAnimationHeader.m_FootPlantVectors.m_RHeelStart;
			footplant.m_RToe0Start = m_GlobalAnimationHeader.m_FootPlantVectors.m_RToe0Start;
			footplant.m_RToe0End = m_GlobalAnimationHeader.m_FootPlantVectors.m_RToe0End;

			int memsize = sizeof(FOOT_PLANT_INFO) + sizeof(uint8)*m_GlobalAnimationHeader.m_FootPlantBits.size();
			mem.reset(new char[memsize]);
			memcpy(mem.get(), &footplant, sizeof(FOOT_PLANT_INFO));
			memcpy((char*)(mem.get()) + sizeof(FOOT_PLANT_INFO),&m_GlobalAnimationHeader.m_FootPlantBits[0], sizeof(uint8)*m_GlobalAnimationHeader.m_FootPlantBits.size() );
			cgfSaver.SaveFootPlantInfo(mem.get(), memsize );
			// Force remove of the read only flag.


		}
		//	delete pCGF;
		SaveControllers(cgfSaver, m_GlobalAnimationHeader.m_arrController);
	}




	SetFileAttributes( name,FILE_ATTRIBUTE_ARCHIVE );

	m_ChunkFile.Write( name );

	FileUtil::SetFileTimes(name, timeStamp);

	const __int64 fileSize = FileUtil::GetFileSize(name);
	if (fileSize < 0)
	{
		RCLogError("Failed to get file size of '%s'", name);
		return ~0;
	}

	if (fileSize > 0xffFFffFFU)
	{
		RCLogError( "Unexpected huge file '%s' found", name);
		return ~0;
	}

	return (uint32)fileSize;
}


void CAnimationCompressor::MakeErrors(uint32 numBones, CompressionInfo &compInfo, int16 master, int16 footskel, SAnimationDesc * pDesc, CSkeletonInfo * pSkeleton, int weapon_bone)
{

	compInfo.m_Info.clear();

	if (m_AnimDesc.m_CompressionQuality == 0)
	{
		for (uint32 i=0; i<numBones; i++)
		{
			CompressionLevelInfo info;
			info.boneLevel = 0; //not used yet
			info.m_bPosCompression = eNoCompression;//eCompression;
			info.m_bRotCompression = eNoCompression;// eCompression;
			info.m_bSclCompression = eNoCompression;
			info.m_DeletePos = 1; //m_AnimDesc.m_DeletePosController;
			info.m_DeleteRot = 1; //m_AnimDesc.m_DeleteRotController;

			info.m_PositionError = 0.00000001f;
			info.m_RotationError = 0.00000001f;
			info.m_ScaleError = 0.00000001f;

			info.m_ScaleFormat = eNoCompressVec3;
			info.m_PositionFormat = eNoCompressVec3;
			info.m_RotationFormat = eSmallTree64BitExtQuat;//eSmallTree64BitQuat;//eNoCompressQuat;

			compInfo.m_Info.push_back(info);

		}

	}
	else
	{

		// trying to use preset info
		if (pSkeleton && pDesc->m_Preset.m_BoneInfoMap.size() > 0)
		{

			uint32 bones = pSkeleton->m_SkinningInfo.m_arrBonesDesc.size();

			for (uint32 i=0; i<numBones; i++)
			{

				uint32 currController = pSkeleton->m_SkinningInfo.m_arrBonesDesc[i].m_nControllerID;

				const CompressionPreset::BonePreset * pCurrBone = pDesc->m_Preset.FindBonePreset(currController);
				if (pCurrBone)
				{
					// use information from preset

					CompressionLevelInfo info;
					info.boneLevel = 0; //not used yet
					info.m_bPosCompression = eCompression;
					info.m_bRotCompression = eCompression;
					info.m_DeletePos = 1; //m_AnimDesc.m_DeletePosController;
					info.m_DeleteRot = 1; //m_AnimDesc.m_DeleteRotController;

					info.m_PositionError = pCurrBone->m_fPosError;
					info.m_RotationError = pCurrBone->m_fRotError;

					info.m_PositionFormat = pCurrBone->m_PosCompressionFormat;
					info.m_RotationFormat = pCurrBone->m_RotCompressionFormat;

					compInfo.m_Info.push_back(info);

				}
				else
				{
					DefaultError(master, compInfo, i, footskel);
				}

			}

		}
		else
		{
			for (uint32 i=0; i<numBones; i++)
			{
				DefaultError(master, compInfo, i, footskel);
			}
		}

	}

	// weapon_bone workaround
	if (weapon_bone >=0 && weapon_bone < numBones) {
		compInfo.m_Info[weapon_bone].m_DeletePos = 0;
		compInfo.m_Info[weapon_bone].m_DeleteRot = 0;
	}
}

//e:\CryEngine2\Game\animations\animations.cba /refresh /SkipDBA /file="E:\CryEngine2\Game\Animations\human\male\combat\combat_run_rifle_right_fast_01.caf" /wait
//e:\CryEngine2\Game\animations\animations.cba /refresh /SkipDBA /file="E:\CryEngine2\Game\Animations\alien\grunt\behavior\idle\stand_idle_idle_01.caf" /wait
//e:\CryEngine2\Game\animations\animations.cba /refresh /SkipDBA /file="E:\CryEngine2\Game\Animations\human\male\gesamte_hierarchie.caf" /wait

// Modified version of AnalyseAndModifyAnimations function
bool CAnimationCompressor::ProcessAnimation(ConvertContext& cc, ILoaderCGFListener * pListener, CTrackStorage * pStorage, bool bNeedCalculate)
{
	if (bNeedCalculate==0)
		return true;


	CAnimationManager::GetInst().m_arrGlobalAnimations[m_GAID] = m_GlobalAnimationHeader;


	const char * animname = m_sName.c_str();
	Matrix34 m0	= m_pInfo->m_SkinningInfo.m_arrBonesDesc[0].m_DefaultB2W;
	Matrix34 m1	= m_pInfo->m_SkinningInfo.m_arrBonesDesc[1].m_DefaultB2W;

	m_pInfo->m_SkinningInfo.m_arrBonesDesc[0].m_DefaultB2W.SetIdentity();
	m_pInfo->m_SkinningInfo.m_arrBonesDesc[0].m_DefaultW2B.SetIdentity();

	m_pInfo->m_SkinningInfo.m_arrBonesDesc[1].m_DefaultW2B=m_pInfo->m_SkinningInfo.m_arrBonesDesc[0].m_DefaultB2W.GetInverted();


	m_AnimDesc.m_fRootQuality=0.0001f;
	EvaluateStartLocation(cc);

	ReplaceRootByLocator(cc, 1);  //replace root by locator, apply simple compression to the root-joint and re-transform all children
	ReplaceRootByLocator(cc, 0);  //apply simple compression to the root-joint and re-transform all children

	DetectCycleAnimations(pListener);
	EvaluateSpeed(pListener);


	//----------------------------------------------------------------------------------
	//---                          calculate footplants                  ---------------
	//----------------------------------------------------------------------------------
	int16 lheel_idx = -1;
	int16 rheel_idx = -1;
	int16 ltoe_idx = -1;
	int16 rtoe_idx = -1;
	int16 master = -1;
	int16 weaponbone = -1;

	CCompressonator compController;
	CompressionInfo compInfo;

	uint32 numJoints=0;
	if (m_pInfo)
		numJoints=  m_pInfo->m_SkinningInfo.m_arrBonesDesc.size();
	for (uint32 i=0; i<numJoints; i++)
	{
		const char *  BoneName =  m_pInfo->m_SkinningInfo.m_arrBonesDesc[i].m_arrBoneName;
		if (0 == stricmp(BoneName,"Bip01 L Heel"))
			lheel_idx=i;
		if (0 == stricmp(BoneName,"Bip01 L Toe0"))
			ltoe_idx=i;
		if (0 == stricmp(BoneName,"Bip01 R Heel"))
			rheel_idx=i;
		if (0 == stricmp(BoneName,"Bip01 R Toe0"))
			rtoe_idx=i;

		if (0 == stricmp(BoneName,"weapon_bone"))
			weaponbone=i;
	}


	std::vector< std::vector<DebugJoint> > g_arrSkeletons;

	bool bFootplant = true;
	uint32 footskel = 0;
	if (lheel_idx>0 && rheel_idx>0  && ltoe_idx>0 && rtoe_idx>0 )
	{
		footskel=1;
#ifdef PRINTOUT
		RCLog("human foot-skeleton detected");
#endif
	}

	if (m_AnimDesc.m_FootPlant && footskel)
	{
#ifdef PRINTOUT
		RCLog("calculating footplant bits");
#endif
		SetFootplantBitsAutomatically( g_arrSkeletons,  0 , 0, lheel_idx,rheel_idx,ltoe_idx,rtoe_idx, m_AnimDesc);

		if (m_GlobalAnimationHeader.IsAssetCycle())
		{
			if (m_GlobalAnimationHeader.m_fDistance>0.2f)
			{
				SFootPlant& rFootPlantVectors = m_GlobalAnimationHeader.m_FootPlantVectors;
				SetFootplantVectors(	g_arrSkeletons,/*i*/0, rFootPlantVectors,/*GlobalID*/ 0 );
			}
		}

	}



	//------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------

	int fails = 3;

	m_GlobalAnimationHeader.m_nTicksPerFrame	= TICKS_PER_FRAME;	//pSkinningInfo->m_nTicksPerFrame;
	m_GlobalAnimationHeader.m_fSecsPerTick		= SECONDS_PER_TICK;	//pSkinningInfo->m_secsPerTick;

	if (bFootplant && footskel && m_AnimDesc.m_FootPlant && m_AnimDesc.m_CompressionQuality)
		//	if (bFootplant && m_AnimDesc.m_CompressionQuality)
	{
		bool q = true;
		bool res = false;
		float coeff = 1.0f;
		float step = 10.0f;

		if (m_GlobalAnimationHeader.m_arrController.size() > numJoints)
			numJoints = m_GlobalAnimationHeader.m_arrController.size();

		uint32 PrintDebugText=1;
		while(q)
		{
			compInfo.m_Info.clear();

			MakeCompression(cc,numJoints, coeff, compInfo, compController, eSmallTree64BitExtQuat, footskel,PrintDebugText);
			PrintDebugText=0;

			std::vector< std::vector<DebugJoint> > tmpSkeletons;
			CreateSkeletonArray( tmpSkeletons, m_CompressedAnimation);

			// calculate differences between compressed and not compressed
			bool p1 = GetError(tmpSkeletons, g_arrSkeletons, lheel_idx, 0.001f * (float)m_AnimDesc.m_CompressionQuality);
			bool p2 = GetError(tmpSkeletons, g_arrSkeletons, rheel_idx, 0.001f * (float)m_AnimDesc.m_CompressionQuality);

			if (p1 && p2)
			{
				//try decrease errors
				coeff *= step;
				m_LastGoodAnimation = m_CompressedAnimation;
				res = true;
				if (0.00000001f * coeff > 1.0f)
				{
					// maybe aim pose?
					q = false;
					MakeErrors(numJoints, compInfo, master, footskel, &m_AnimDesc, m_pInfo, weaponbone);
					compController.CreateCompression(cc,m_GlobalAnimationHeader, m_GlobalAnimationHeader,  compInfo, m_pInfo, m_AnimDesc, m_GAID,0,0);
				}

			}
			else
			{
				if (fails-- < 0)
				{
					q = false;
					// Rollback to the previuos
					if (res)
					{

						MakeCompression(cc, numJoints, coeff, compInfo, compController, eAutomaticQuat, footskel,0);
						m_GlobalAnimationHeader.m_arrController = m_LastGoodAnimation.m_arrController;
					}
					else
					{
						q = false;

						MakeErrors(numJoints, compInfo, master, footskel, &m_AnimDesc, m_pInfo, weaponbone);
						compController.CreateCompression(cc,m_GlobalAnimationHeader, m_GlobalAnimationHeader,  compInfo, m_pInfo, m_AnimDesc, m_GAID,0,0);
					}

				}

				step = 1.3f;
				coeff = coeff / 2.0f;								
			}

		}
	}
	else
	{
		uint32 PrintDebugText=1;
		uint32 numController = m_GlobalAnimationHeader.m_arrController.size();
		if (numController > numJoints)
			numJoints = numController;

		MakeErrors(numJoints, compInfo, master, footskel, &m_AnimDesc, m_pInfo, weaponbone);
		{
			// need fixup for the hunter animation. problems with loss precision 
			// !!!!!!!!!!!!!! OLD CONVERSION !!!!!!!!!!!!!!!!!!!
			compController.CreateCompression(cc,m_GlobalAnimationHeader, m_GlobalAnimationHeader,  compInfo, m_pInfo, m_AnimDesc, m_GAID,PrintDebugText,1);
		}
	}

	return true;
}

void CAnimationCompressor::SaveToDB(const string& strFilePathDBA, CTrackStorage * pStorage, bool bUpdate)
{

	static volatile int g_lockMemDB;
	WriteLock lock(g_lockMemDB);

	if (pStorage)
	{
		if (bUpdate)
			pStorage->UpdateAnimation(strFilePathDBA,m_GlobalAnimationHeader, m_sName, true, m_dwTimestamp, false);
		else
			pStorage->AddAnimation(strFilePathDBA,m_GlobalAnimationHeader, m_sName, true, m_dwTimestamp);
	}
	//	lock.Unlock();
}







void CAnimationCompressor::ReplaceRootByLocator(ConvertContext& cc, uint32 CheckLocator)
{
	uint32 numJoints = m_pInfo->m_SkinningInfo.m_arrBonesDesc.size();
	if (numJoints==0)
		return; 

	uint32 locator_locoman01 = g_crcGen.GetCRC32("Locator_Locomotion");
	IController* pLocomotionController = m_GlobalAnimationHeader.GetController(locator_locoman01);
	if (CheckLocator) 
	{
		if (pLocomotionController==0)
			return;
#ifdef PRINTOUT
		RCLog("ReplaceRootByLocator: Animation has Locomotion Locator");
#endif
	}

//	const char* pRootNameParam = m_AnimDesc.m_strRoot;
	const char* pRootName = m_pInfo->m_SkinningInfo.m_arrBonesDesc[0].m_arrBoneName;
	int32 RootParent = m_pInfo->m_SkinningInfo.m_arrBonesDesc[0].m_nOffsetParent;
	//	assert( strcmp(pRootNameParam,pRootName)==0 );
	uint32 RootControllerID = g_crcGen.GetCRC32(pRootName);;
	IController* pRootController = m_GlobalAnimationHeader.GetController(RootControllerID);
	if (pRootController==0)
	{
		RCLog("Can't find root-joint with name: %s",pRootName);
		return;
	}

	if (CheckLocator==0) 
		pLocomotionController=pRootController;

//	const char* pPelvisNameParam = m_AnimDesc.m_strChild_of_Root;
	const char* pPelvisName = m_pInfo->m_SkinningInfo.m_arrBonesDesc[1].m_arrBoneName;
	std::vector<IController*> arrpChildController;

	uint32 nDirectChildren=0;
	for (uint32 i=0; i<numJoints; i++)
	{
		int32 nParent = m_pInfo->m_SkinningInfo.m_arrBonesDesc[ i].m_nOffsetParent;
		const char* pChildName  = m_pInfo->m_SkinningInfo.m_arrBonesDesc[ i].m_arrBoneName;

		int32 IsLocator = strcmp(pChildName,"Locator_Locomotion")==0;
		if (IsLocator)
			continue;
		int32 IsRoot = strcmp(pChildName,pRootName)==0;
		if (IsRoot)
			continue;

		const char* pParentName = m_pInfo->m_SkinningInfo.m_arrBonesDesc[ i+nParent].m_arrBoneName;
		int32 idx = m_pInfo->m_SkinningInfo.m_arrBonesDesc[ i+nParent].m_nOffsetParent;
		if (idx==0)
		{
			uint32 ChildControllerID = g_crcGen.GetCRC32(pChildName);;
			IController* pChildController = m_GlobalAnimationHeader.GetController(ChildControllerID);
			if (pChildController)
			{
				arrpChildController.push_back(pChildController);
				nDirectChildren++;
			}
		}
		int32 ddd=0;
	}


	//evaluate the distance and the duration of this animation (we use it to calculate the speed)
	Diag33 Scale;
	CInfo ERot0	=	pRootController->GetControllerInfo();
	if (ERot0.realkeys==0)
		return;


	QuatT absFirstKey;

	GlobalAnimationHeaderCAF& rCAF = CAnimationManager::GetInst().m_arrGlobalAnimations[m_GAID];
	pLocomotionController->GetOPS(rCAF.NTime2KTime(0),absFirstKey.q,absFirstKey.t,Scale);	
	//for some reason there is some noise in the original animation. 
	if ( 1.0f-fabsf(absFirstKey.q.w)<0.00001f && fabsf(absFirstKey.q.v.x)<0.00001f && fabsf(absFirstKey.q.v.y)<0.00001f && fabsf(absFirstKey.q.v.z)<0.00001f)
		absFirstKey.q.SetIdentity();


	std::vector<int> arrTimes;					arrTimes.resize(ERot0.realkeys);
	std::vector<QuatT> arrRootKeys;			arrRootKeys.resize(ERot0.realkeys);



	std::vector< std::vector<QuatT> > arrChildAbsKeys;	
	std::vector< std::vector<PQLog> > arrChildPQKeys;	

	uint32 numChildController = arrpChildController.size();
	arrChildAbsKeys.resize(numChildController);
	arrChildPQKeys.resize(numChildController);
	for (uint32 c=0; c<numChildController; c++)
	{
		arrChildAbsKeys[c].resize(ERot0.realkeys);
		arrChildPQKeys[c].resize(ERot0.realkeys);
	}


	GlobalAnimationHeaderCAF& rGlobalAnimHeader = m_GlobalAnimationHeader;
	int32 startkey	=	int32(rGlobalAnimHeader.m_fStartSec*TICKS_PER_SECOND);
	int32 stime	=	startkey;
	for (int32 t=0; t<ERot0.realkeys; t++)
	{
		f32 normalized_time=0.0f;
		if (stime!=startkey)
			normalized_time=f32(stime-startkey)/(TICKS_PER_FRAME*(ERot0.realkeys-1));

		QuatT absRootKey;
		GlobalAnimationHeaderCAF& rCAF = CAnimationManager::GetInst().m_arrGlobalAnimations[m_GAID];
		pRootController->GetOPS(rCAF.NTime2KTime(normalized_time),absRootKey.q,absRootKey.t,Scale);	
		arrRootKeys[t] = absFirstKey.GetInverted() * absRootKey;
		for (uint32 c=0; c<numChildController; c++)
		{
			QuatT relChildKey;
			if (arrpChildController[c])
			{
				arrpChildController[c]->GetOPS(rCAF.NTime2KTime(normalized_time),relChildKey.q,relChildKey.t,Scale);	
				arrChildAbsKeys[c][t] = arrRootKeys[t]*relChildKey; //child of root
			}
		}
		arrTimes[t]=stime;
		stime += TICKS_PER_FRAME;
	}


	Diag33 s;	
//	GlobalAnimationHeaderCAF& rCAF = CAnimationManager::GetInst().m_arrGlobalAnimations[m_GAID];
	QuatT fkey; pLocomotionController->GetOPS(rCAF.NTime2KTime(0.0f),fkey.q,fkey.t,s);	
	QuatT mkey; pLocomotionController->GetOPS(rCAF.NTime2KTime(0.5f),mkey.q,mkey.t,s);	
	QuatT lkey; pLocomotionController->GetOPS(rCAF.NTime2KTime(1.0f),lkey.q,lkey.t,s);	

	stime	=	startkey;
	std::vector<PQLog> arrRootPQKeys;
	for (int32 key=0; key<ERot0.realkeys; key++)
	{
		f32 normalized_time=0.0f;
		if (stime!=startkey)
			normalized_time=f32(stime-startkey)/(TICKS_PER_FRAME*(ERot0.realkeys-1));

		QuatT relLocatorKey;	pLocomotionController->GetOPS(rCAF.NTime2KTime(normalized_time),relLocatorKey.q,relLocatorKey.t,Scale);	

#ifdef PRINTOUT
		if ( fabsf(relLocatorKey.q.v.x>0.0001f) )
			RCLog("Key %d in locator has rotation around x-axis (%f). Only rotations around z-axis (=vertical axis) are allowed",key,relLocatorKey.q.v.x);
		if ( fabsf(relLocatorKey.q.v.y>0.0001f) )
			RCLog("Key %d in Locator has rotation around y-axis (%f). Only rotations around z-axis (=vertical axis) are allowed",key,relLocatorKey.q.v.y);
#endif

		relLocatorKey = absFirstKey.GetInverted()*relLocatorKey;

		//convert into the old FarCry format
		PQLog pqlog; pqlog.vRotLog=log(!relLocatorKey.q);	pqlog.vPos=relLocatorKey.t;
		arrRootPQKeys.push_back(pqlog);

		stime += TICKS_PER_FRAME;
	}



	int Bip01 = -1;
	for (int i=0; i<m_GlobalAnimationHeader.m_arrController.size(); ++i)
	{
		if (m_GlobalAnimationHeader.m_arrController[i] == pRootController) 	
			Bip01=i;
	}
	CreateNewController(arrTimes, arrRootPQKeys, Bip01, m_AnimDesc.m_fRootQuality, m_AnimDesc.m_fRootQuality);



	//--------------------------------------------------------------------------------------
	//---                     calculate the slope value                                 ----
	//--------------------------------------------------------------------------------------
	rGlobalAnimHeader.m_fSlope=0;
	Vec3 p0 = fkey.t;
	Vec3 p1 = lkey.t;
	Vec3 vdir=Vec3(ZERO);
	
	f32 fLength = (p1-p0).GetLength();
	if (fLength>0.01f) //only if there is enough movement
		vdir=((p1-p0)*fkey.q).GetNormalized();
	f64 l = sqrt(vdir.x*vdir.x + vdir.y*vdir.y);
	if (l>0.0001)	
	{
		f32 rad			= f32( atan2(-vdir.z*(vdir.y/l),l) );
		f32 deg			= RAD2DEG(rad);
		rGlobalAnimHeader.m_fSlope=rad;
	}

	//--------------------------------------------------------------------------------
//	GlobalAnimationHeaderCAF& rCAF = CAnimationManager::GetInst().m_arrGlobalAnimations[m_GAID];
	pRootController = m_GlobalAnimationHeader.GetController(RootControllerID);
	Vec3 fkey2;
	pRootController->GetP(rCAF.NTime2KTime(0),fkey2);	
	Vec3 lkey2;
	pRootController->GetP(rCAF.NTime2KTime(1),lkey2);	

	stime=startkey;
	for (int32 t=0; t<ERot0.realkeys; t++)
	{
		QuatT absRootKey;	   

		f32 normalized_time=0.0f;
		if (stime!=startkey)
			normalized_time=f32(stime-startkey)/(TICKS_PER_FRAME*(ERot0.realkeys-1));

		pRootController->GetOPS(rCAF.NTime2KTime(normalized_time),absRootKey.q,absRootKey.t,Scale);	
		for (uint32 c=0; c<numChildController; c++)
		{
			QuatT relChildKeys = absRootKey.GetInverted() * arrChildAbsKeys[c][t]; //relative Pivot
			arrChildPQKeys[c][t].vRotLog	=	log(!relChildKeys.q);
			arrChildPQKeys[c][t].vPos		=	relChildKeys.t ;
		}
		stime += TICKS_PER_FRAME;
	}


	for (uint32 c=0; c<numChildController; c++)
	{
		int nChildID = -1;
		uint32 numControllers = m_GlobalAnimationHeader.m_arrController.size();
		for (int i=0; i<numControllers; ++i)
		{
			if (m_GlobalAnimationHeader.m_arrController[i] == arrpChildController[c]) 	
				nChildID = i;
		}
		assert(nChildID>=0);
		CreateNewController(arrTimes, arrChildPQKeys[c], nChildID);
	}

}






void CAnimationCompressor::EvaluateSpeed(ILoaderCGFListener * pListener)//uint32 AnimID)
{
	uint32 numJoints = m_pInfo->m_SkinningInfo.m_arrBonesDesc.size();
	if (numJoints==0)
		return;

	uint32 TicksPerSecond = TICKS_PER_SECOND;
	f32 fStart		=	m_GlobalAnimationHeader.m_fStartSec;
	f32 fDuration	=	m_GlobalAnimationHeader.m_fEndSec - m_GlobalAnimationHeader.m_fStartSec;
	f32 fDistance	=	0.0f;

	m_GlobalAnimationHeader.m_fDistance = 0.0f;
	m_GlobalAnimationHeader.m_fSpeed    = 0.0f;


//	const char* pRootNameParam = m_AnimDesc.m_strRoot;
	const char* pRootName = m_pInfo->m_SkinningInfo.m_arrBonesDesc[0].m_arrBoneName;

	IController * pController = m_GlobalAnimationHeader.GetController(m_pInfo->m_SkinningInfo.m_arrBonesDesc[0].m_nControllerID);
	if (pController==0)
		return;
	CInfo ERot0	=	pController->GetControllerInfo();
	if (ERot0.realkeys<2)
		return;

	GlobalAnimationHeaderCAF& rCAF = CAnimationManager::GetInst().m_arrGlobalAnimations[m_GAID];

	Vec3 SPos0,SPos1;	
	f32 newtime = 0; //fStart*TicksPerSecond;
	pController->GetP( rCAF.NTime2KTime(newtime), SPos0 );
	for (f32 t=0.01f; t<=1.0f; t=t+0.01f)
	{
		//newtime = fStart*TicksPerSecond + t*TicksPerSecond*fDuration;
		newtime = t;
		pController->GetP( rCAF.NTime2KTime(newtime), SPos1 );
		if (SPos0 != SPos1 )
		{
			fDistance += (SPos0-SPos1).GetLength();
			SPos0=SPos1;
		}
		else
		{
			int a  = 0;
		}
	}

	m_GlobalAnimationHeader.m_fDistance			=	fDistance;
	if (fDuration)
		m_GlobalAnimationHeader.m_fSpeed      = fDistance/fDuration;

	if (fDuration<0.001f)	
	{
		RCLogWarning ("CryAnimation: Animation-asset '%s' has just one keyframe. Impossible to detect Duration", m_GlobalAnimationHeader.GetFilePath());
	}
}




void CAnimationCompressor::DetectCycleAnimations(ILoaderCGFListener * pListener)//uint32 AnimID)
{
	GlobalAnimationHeaderCAF& rCAF = CAnimationManager::GetInst().m_arrGlobalAnimations[m_GAID];

	uint32 numJoints = m_pInfo->m_SkinningInfo.m_arrBonesDesc.size();
	bool equal=1;
	for (uint32 j=0; j<numJoints; j++)
	{
		if (m_pInfo->m_SkinningInfo.m_arrBonesDesc[j].m_nOffsetParent == 0)
			continue;

		int AnimID = 0;
		IController* pController = m_GlobalAnimationHeader.GetController(m_pInfo->m_SkinningInfo.m_arrBonesDesc[j].m_nControllerID);;//m_GlobalAnimationHeader.m_arrController[AnimID];//pModelJoint[j].m_arrControllersMJoint[AnimID];
		if (pController==0)
			continue;

		int GlobalAnimationID = m_GAID;
		Quat SRot;	pController->GetO(rCAF.NTime2KTime(0), SRot );
		Quat ERot	=	pController->GetControllerInfo().quat;

		Ang3 sang=Ang3(SRot);
		Ang3 eang=Ang3(ERot);

		CInfo info	=	pController->GetControllerInfo();
		bool status  = SRot.IsEquivalent(ERot,0.1f);
		equal &= status;
	}

	if (equal)
		m_GlobalAnimationHeader.m_nFlags |= CA_ASSET_CYCLE;
	else
		m_GlobalAnimationHeader.m_nFlags &= ~CA_ASSET_CYCLE;

	if (m_AnimDesc.m_AdditiveAnimation)
		m_GlobalAnimationHeader.OnAssetAdditive();
}




void CAnimationCompressor::CreateNewController( std::vector<int>& arrFullTimes, std::vector<PQLog>& arrFullQTKeys, int numController, float fRotErr, float fPosErr) 
{
	CController * pNewController = new CController;



	uint32 i;

	CCompressonator compressonator;

	int32 oldtime =  -1;
	for ( i = 0; i < arrFullTimes.size(); ++i)
	{
		if (oldtime != arrFullTimes[i])
		{
			oldtime = arrFullTimes[i];

			compressonator.m_RotTimes.push_back(arrFullTimes[i]);
			compressonator.m_PosTimes.push_back(arrFullTimes[i]);
			Quat q = !exp(arrFullQTKeys[i].vRotLog);
			compressonator.m_Rotations.push_back(q);
			compressonator.m_Positions.push_back(arrFullQTKeys[i].vPos);
			//pTime->AddKeyTime( arrFullTimes[i]);
			//Quat q = !exp(arrFullQTKeys[i].vRotLog);
			//pRotStorage->AddValue(q);
			//pPosStorage->AddValue(arrFullQTKeys[i].vPos);
		}
	}

	CompressionLevelInfo info;
	info.m_RotationFormat = eSmallTree64BitExtQuat;
	info.m_RotationError = fRotErr;
	info.m_PositionError = fPosErr;
	info.m_PositionFormat = eNoCompressVec3;
	if (info.m_RotationError != 0.0f)
		info.m_bRotCompression = eCompression;
	else
		info.m_bRotCompression = eNoCompression;
	if (info.m_PositionError != 0.0f)
		info.m_bPosCompression = eCompression;
	else
		info.m_bPosCompression = eNoCompression;
	compressonator.CompressPositions(pNewController, &info, false);
	compressonator.CompressRotations(pNewController, &info, false);

	pNewController->m_nControllerId = m_GlobalAnimationHeader.m_arrController[numController]->GetID();
	pNewController->m_pqGlobalAnimationHeader = &m_GlobalAnimationHeader;
	m_GlobalAnimationHeader.m_arrController[numController] = pNewController;
}





void CAnimationCompressor::EvaluateStartLocation(ConvertContext& cc)
{
	uint32 numJoints = m_pInfo->m_SkinningInfo.m_arrBonesDesc.size();
	if (numJoints==0)
		return;

	uint32 locator_locoman01 = g_crcGen.GetCRC32("Locator_Locomotion");
	IController* pLocomotionController = m_GlobalAnimationHeader.GetController(locator_locoman01);
	GlobalAnimationHeaderCAF& rCAF = CAnimationManager::GetInst().m_arrGlobalAnimations[m_GAID];

//	const char* pRootNameParam = m_AnimDesc.m_strRoot;
	const char* pRootName = m_pInfo->m_SkinningInfo.m_arrBonesDesc[0].m_arrBoneName;
	//	assert( strcmp(pRootNameParam,pRootName)==0 );
	uint32 RootControllerID = g_crcGen.GetCRC32(pRootName);;
	IController* pRootController = m_GlobalAnimationHeader.GetController(RootControllerID);

	GlobalAnimationHeaderCAF& rGlobalAnimHeader = m_GlobalAnimationHeader;

	Diag33 Scale; 
	QuatT absFirstKey;	absFirstKey.SetIdentity();
	if (pLocomotionController)
	{
#ifdef PRINTOUT
		RCLog("EvaluateStartLocation:: Animation has Locomotion Locator");
#endif
		pLocomotionController->GetOPS(rCAF.NTime2KTime(0),absFirstKey.q,absFirstKey.t,Scale);	
		//for some reason there is some noise in the original animation. 
		if ( 1.0f-fabsf(absFirstKey.q.w)<0.00001f && fabsf(absFirstKey.q.v.x)<0.00001f && fabsf(absFirstKey.q.v.y)<0.00001f && fabsf(absFirstKey.q.v.z)<0.00001f)
			absFirstKey.q.SetIdentity();
	}
	else if (pRootController)
	{
#ifdef PRINTOUT
		RCLog("EvaluateStartLocation:: Animation is using Bip01 as Locomotion Locator");
#endif
		pRootController->GetOPS(rCAF.NTime2KTime(0),absFirstKey.q,absFirstKey.t,Scale);	
		//for some reason there is some noise in the original animation. 
		if ( 1.0f-fabsf(absFirstKey.q.w)<0.00001f && fabsf(absFirstKey.q.v.x)<0.00001f && fabsf(absFirstKey.q.v.y)<0.00001f && fabsf(absFirstKey.q.v.z)<0.00001f)
			absFirstKey.q.SetIdentity();
	}
	rGlobalAnimHeader.m_StartLocation2 = absFirstKey;
}




void CAnimationCompressor::CreateSkeletonArray(  std::vector< std::vector<DebugJoint> >& arrSkeletons, GlobalAnimationHeaderCAF& header  ) 
{

	Diag33 temp;
	GlobalAnimationHeaderCAF& rGAH = header;
	GlobalAnimationHeaderCAF& rCAF = CAnimationManager::GetInst().m_arrGlobalAnimations[m_GAID];

	f32 duration = rGAH.m_fEndSec - rGAH.m_fStartSec;

	f64 PosesPerKey = (1.0/60.0);
	uint32 HowManyPoses = uint32(duration/PosesPerKey);
	if (HowManyPoses==0)
		HowManyPoses=1;

	//	rGAH.m_FootPlantBits.resize(HowManyPoses, 0);

	arrSkeletons.resize(HowManyPoses);
	uint32 numJoints = m_pInfo->m_SkinningInfo.m_arrBonesDesc.size();

	for(uint32 i=0; i<HowManyPoses; i++)
		arrSkeletons[i].resize(numJoints);

	const CryBoneDescData* pModelJoint = &m_pInfo->m_SkinningInfo.m_arrBonesDesc[0];
	for (uint32 s=0; s<HowManyPoses; s++) 
	{
		//evaluate all controllers for this animation
		f32 newtime = (1.0f/HowManyPoses)*s; 
		for (uint32 j=0; j<numJoints; j++)
		{
			////m_DefaultRel; 
			if (pModelJoint[j].m_nOffsetParent != 0)
			{
				arrSkeletons[s][j].m_RelativeQuat= QuatT(pModelJoint[j+pModelJoint[j].m_nOffsetParent].m_DefaultW2B * pModelJoint[j].m_DefaultB2W);
			}
			else
			{
				arrSkeletons[s][j].m_RelativeQuat= QuatT(pModelJoint[j].m_DefaultB2W);
			}

			IController* pController =  rGAH.GetController(pModelJoint[j].m_nControllerID);
			if (pController)
				pController->GetOPS(rCAF.NTime2KTime(newtime), arrSkeletons[s][j].m_RelativeQuat.q, arrSkeletons[s][j].m_RelativeQuat.t, temp );
		}

		//calculate absolute joints
		numJoints = m_pInfo->m_SkinningInfo.m_arrBonesDesc.size();
		arrSkeletons[s][0].m_AbsoluteQuat = arrSkeletons[s][0].m_RelativeQuat;
		for (uint32 i=1; i<numJoints; i++)
		{
			int16 idx = arrSkeletons[s][i].m_idxParent =	i + m_pInfo->m_SkinningInfo.m_arrBonesDesc[i].m_nOffsetParent;//m_pModel->m_arrModelJoints[i].m_idxParent;
			if (idx >= 0)
			{
				arrSkeletons[s][i].m_AbsoluteQuat	= arrSkeletons[s][idx].m_AbsoluteQuat * arrSkeletons[s][i].m_RelativeQuat;
				arrSkeletons[s][i].m_AbsoluteQuat.q.Normalize();
			}
		}

	}
}


float sqrt_error_quat(Quat& q1, Quat& q2)
{

	Float4Storage dist;

	dist.x= fabs(q1.v.x) - fabs(q2.v.x);
	dist.y= fabs(q1.v.y) - fabs(q2.v.y);
	dist.z= fabs(q1.v.z) - fabs(q2.v.z);
	dist.w= fabs(q1.w) - fabs(q2.w);

	float curerr = dist.x * dist.x  + dist.y * dist.y + dist.z * dist.z + dist.w * dist.w;
	return sqrtf(curerr);
}


bool CAnimationCompressor::GetError( std::vector< std::vector<DebugJoint> >& arrSkeletons1, std::vector< std::vector<DebugJoint> >& arrSkeletons2, int nJoint, float error ) 
{
	float maxerror = 0.0f;
	uint32 index = 0; 
	bool passed(true);

	for (uint32 i = 0, num = arrSkeletons1.size(); i < num; ++i)
	{

		float currerr = sqrt_error_quat(arrSkeletons1[i][nJoint].m_AbsoluteQuat.q, arrSkeletons2[i][nJoint].m_AbsoluteQuat.q);
		if (currerr > error)
		{
			maxerror = currerr;
			index = i;
			passed = false;
		}
	}

	return passed;
}


#define LHEEL (0x01)
#define RHEEL (0x02)
#define LTOE0 (0x04)
#define RTOE0 (0x08)

#define POSES (0x080)


void CAnimationCompressor::SetFootplantBitsAutomatically( std::vector< std::vector<DebugJoint> >& arrSkeletons, int32 nAnimID,int32 nGlobalAnimID,int32 lHidx,int32 rHidx,int32 lTidx,int32 rTidx, const SAnimationDesc& desc )
{

	CreateSkeletonArray( arrSkeletons, m_GlobalAnimationHeader);

	//#define FLOORDIST (0.03f)
	//#define SPEED_XY (0.005f)

	GlobalAnimationHeaderCAF& rGlobalAnimHeader = m_GlobalAnimationHeader;
	
	std::vector<f32> g_arrLHeelVelocity;
	std::vector<f32> g_arrLToe0Velocity;
	std::vector<f32> g_arrRHeelVelocity;
	std::vector<f32> g_arrRToe0Velocity;


	uint32 numPoses=arrSkeletons.size();

	if (lHidx>0 && rHidx>0 && lTidx>0 && rTidx>0)
	{
		//arrFootPlants.resize(numPoses);
		//nGlobalAnimID
		rGlobalAnimHeader.m_FootPlantBits.resize(numPoses, 0);
		for (uint32 i=0; i<numPoses; i++)	
			rGlobalAnimHeader.m_FootPlantBits[i]=0;

		g_arrLHeelVelocity.resize(numPoses);
		g_arrRHeelVelocity.resize(numPoses);
		g_arrLToe0Velocity.resize(numPoses);
		g_arrRToe0Velocity.resize(numPoses);

		for (uint32 s=0; s<numPoses; s++) 
		{
			g_arrLHeelVelocity[s]=1000.0f;
			g_arrRHeelVelocity[s]=1000.0f;

			g_arrLToe0Velocity[s]=1000.0f;
			g_arrRToe0Velocity[s]=1000.0f;
		}


		Plane plane;
		f32 sloperad = rGlobalAnimHeader.m_fSlope;
		plane.n=Matrix33::CreateRotationX( sloperad ) * Vec3(0,0,1);
		plane.d=0;

		f32 dist=0;
		Vec3 sq; Vec3 zp;
		for (uint32 s=1; s<numPoses; s++) 
		{
			sq= arrSkeletons[s][lHidx].m_AbsoluteQuat.t - arrSkeletons[s-1][lHidx].m_AbsoluteQuat.t; sq.z=0;
			zp=(arrSkeletons[s][lHidx].m_AbsoluteQuat.t + arrSkeletons[s-1][lHidx].m_AbsoluteQuat.t )*0.5f;
			dist=/*fabsf*/(plane|zp);
			g_arrLHeelVelocity[s]=(dist < desc.m_fFloorDist) ? sq.GetLength() : 1000.0f;
			sq= arrSkeletons[s][rHidx].m_AbsoluteQuat.t - arrSkeletons[s-1][rHidx].m_AbsoluteQuat.t; sq.z=0;
			zp=(arrSkeletons[s][rHidx].m_AbsoluteQuat.t + arrSkeletons[s-1][rHidx].m_AbsoluteQuat.t )*0.5f;
			dist=/*fabsf*/(plane|zp);
			g_arrRHeelVelocity[s]=(dist < desc.m_fFloorDist) ? sq.GetLength() : 1000.0f;;

			sq= arrSkeletons[s][lTidx].m_AbsoluteQuat.t - arrSkeletons[s-1][lTidx].m_AbsoluteQuat.t; sq.z=0;
			zp=(arrSkeletons[s][lTidx].m_AbsoluteQuat.t + arrSkeletons[s-1][lTidx].m_AbsoluteQuat.t)*0.5f;
			dist=/*fabsf*/(plane|zp);
			g_arrLToe0Velocity[s]=(dist < desc.m_fFloorDist) ? sq.GetLength() : 1000.0f;
			sq= arrSkeletons[s][rTidx].m_AbsoluteQuat.t - arrSkeletons[s-1][rTidx].m_AbsoluteQuat.t; sq.z=0;
			zp=(arrSkeletons[s][rTidx].m_AbsoluteQuat.t + arrSkeletons[s-1][rTidx].m_AbsoluteQuat.t )*0.5f;
			dist=/*fabsf*/(plane|zp);
			g_arrRToe0Velocity[s]=(dist < desc.m_fFloorDist) ? sq.GetLength() : 1000.0f;

		}

		g_arrLHeelVelocity[0]=g_arrLHeelVelocity[numPoses-1];
		g_arrRHeelVelocity[0]=g_arrRHeelVelocity[numPoses-1];
		g_arrLToe0Velocity[0]=g_arrLToe0Velocity[numPoses-1];
		g_arrRToe0Velocity[0]=g_arrRToe0Velocity[numPoses-1];

		for (uint32 s=0; s<numPoses; s++) 
		{
			if (g_arrLHeelVelocity[s]<desc.m_fSpeedXY)
				rGlobalAnimHeader.m_FootPlantBits[s]|=LHEEL;
			if (g_arrRHeelVelocity[s]<desc.m_fSpeedXY)
				rGlobalAnimHeader.m_FootPlantBits[s]|=RHEEL;

			if (g_arrLToe0Velocity[s]<desc.m_fSpeedXY)
				rGlobalAnimHeader.m_FootPlantBits[s]|=LTOE0;
			if (g_arrRToe0Velocity[s]<desc.m_fSpeedXY)
				rGlobalAnimHeader.m_FootPlantBits[s]|=RTOE0;
		}

	}
}




void CAnimationCompressor::SetFootplantVectors( std::vector< std::vector<DebugJoint> >& arrSkeletons, uint32 nAnimID, SFootPlant& rFootPlants, uint32 nGlobalAnimID )
{

#define lm0 (10)
#define lm1 (11)
#define rm0 (13)
#define rm1 (14)

#define lx0 (16)
#define lx1 (17)
#define rx0 (18)
#define rx1 (19)


	GlobalAnimationHeaderCAF& rGlobalAnimHeader = m_GlobalAnimationHeader;
	int32 poses0 = rGlobalAnimHeader.m_FootPlantBits.size();

	g_arrDistMap24.resize(poses0*20);
	for (int32 i=0; i<(poses0*20); i++)
	{
		g_arrDistMap24[i].r=0;
		g_arrDistMap24[i].g=0;
		g_arrDistMap24[i].b=0;
	}

	int32 hposes=poses0/2;
	for (int32 i=0; i<poses0; i++)
	{

		uint8 FootStep = rGlobalAnimHeader.m_FootPlantBits[i];
		if (FootStep&LHEEL)
			g_arrDistMap24[i+poses0*0].b=0xff;
		if (FootStep&LTOE0)
			g_arrDistMap24[i+poses0*1].r=0xff;

		if (FootStep&RHEEL)
			g_arrDistMap24[i+poses0*5].b=0xff;
		if (FootStep&RTOE0)
			g_arrDistMap24[i+poses0*6].r=0xff;

		//---------------------------------------------------
		if (i<hposes)
		{
			assert((i+hposes)<poses0);
			if (FootStep&LHEEL)
				g_arrDistMap24[i+poses0*lm0+hposes].b=0xff;
			if (FootStep&LTOE0)
				g_arrDistMap24[i+poses0*lm1+hposes].r=0xff;
		} 
		else 
		{
			if (FootStep&LHEEL)
				g_arrDistMap24[i+poses0*lm0-hposes].b=0xff;
			if (FootStep&LTOE0)
				g_arrDistMap24[i+poses0*lm1-hposes].r=0xff;
		}

		if (FootStep&RHEEL)
			g_arrDistMap24[i+poses0*rm0].b=0xff;
		if (FootStep&RTOE0)
			g_arrDistMap24[i+poses0*rm1].r=0xff;
	}		

	f32 r=0.05f;
	//delete area in footsteps
	for (f32 i=0.0f; i<r; i=i+0.001f)
	{
		//start of step
		uint32 idx0=(uint32)(i*(poses0));
		g_arrDistMap24[idx0+poses0*lm0].b=0;
		g_arrDistMap24[idx0+poses0*lm1].r=0;
		g_arrDistMap24[idx0+poses0*rm0].b=0;
		g_arrDistMap24[idx0+poses0*rm1].r=0;

		//end of step
		uint32 idx1=(uint32)( (1.0f-r+i) *(poses0));
		assert(idx1<(uint32)poses0);
		g_arrDistMap24[idx1+poses0*lm0].b=0;
		g_arrDistMap24[idx1+poses0*lm1].r=0;
		g_arrDistMap24[idx1+poses0*rm0].b=0;
		g_arrDistMap24[idx1+poses0*rm1].r=0;
	}

	int16 lhs=-1;
	int16 lhe=-1;
	int16 lts=-1;
	int16 lte=-1;
	int16 rhs=-1;
	int16 rhe=-1;
	int16 rts=-1;
	int16 rte=-1;
	for (int32 i=0,t=poses0-1; i<poses0; i++,t--)
	{
		if (lhs==-1)
			if (g_arrDistMap24[i+poses0*lm0].b==0xff) lhs=i;
		if (lts==-1)
			if (g_arrDistMap24[i+poses0*lm1].r==0xff) lts=i;
		if (lhe==-1)
			if (g_arrDistMap24[t+poses0*lm0].b==0xff) lhe=t;
		if (lte==-1)
			if (g_arrDistMap24[t+poses0*lm1].r==0xff) lte=t;

		if (rhs==-1)
			if (g_arrDistMap24[i+poses0*rm0].b==0xff) rhs=i;
		if (rts==-1)
			if (g_arrDistMap24[i+poses0*rm1].r==0xff) rts=i;
		if (rhe==-1)
			if (g_arrDistMap24[t+poses0*rm0].b==0xff) rhe=t;
		if (rte==-1)
			if (g_arrDistMap24[t+poses0*rm1].r==0xff) rte=t;
	}

	if (lhs!=-1 && lhe!=-1)
		for (int32 i=lhs; i<lhe; i++)	g_arrDistMap24[i+poses0*lm0].b=0xff;
	if (lts!=-1 && lte!=-1)
		for (int32 i=lts; i<lte; i++)	g_arrDistMap24[i+poses0*lm1].r=0xff;

	if (rhs!=-1 && rhe!=-1)
		for (int32 i=rhs; i<rhe; i++)	g_arrDistMap24[i+poses0*rm0].b=0xff;
	if (rts!=-1 && rte!=-1)
		for (int32 i=rts; i<rte; i++)	g_arrDistMap24[i+poses0*rm1].r=0xff;

	//------------------------------------------------------------------------------------

	lhs=-1;
	lhe=-1;
	lts=-1;
	lte=-1;
	rhs=-1;
	rhe=-1;
	rts=-1;
	rte=-1;
	for (int32 i=0,t=poses0-1; i<poses0; i++,t--)
	{
		if (lhs==-1)
			if (g_arrDistMap24[i+poses0*lm0].b==0xff) lhs=i;
		if (lts==-1)
			if (g_arrDistMap24[i+poses0*lm1].r==0xff) lts=i;
		if (lhe==-1)
			if (g_arrDistMap24[t+poses0*lm0].b==0xff) lhe=t;
		if (lte==-1)
			if (g_arrDistMap24[t+poses0*lm1].r==0xff) lte=t;

		if (rhs==-1)
			if (g_arrDistMap24[i+poses0*rm0].b==0xff) rhs=i;
		if (rts==-1)
			if (g_arrDistMap24[i+poses0*rm1].r==0xff) rts=i;
		if (rhe==-1)
			if (g_arrDistMap24[t+poses0*rm0].b==0xff) rhe=t;
		if (rte==-1)
			if (g_arrDistMap24[t+poses0*rm1].r==0xff) rte=t;
	}

	if (lhs!=-1 && lhe!=-1 && rhs!=-1 && rhe!=-1)
	{
		if ( (lhe-hposes)>rhs )
		{
			int32 slh=((lhe-hposes)+rhs)/2;
			g_arrDistMap24[slh+hposes+poses0*lm0].g=0xff;	//left heel start
			g_arrDistMap24[slh+poses0*rm0].g=0x3f;	//right heel end
			for (int32 i=(slh+hposes); i<poses0; i++)
				g_arrDistMap24[i+poses0*lm0].b=0;	//left heel start
			for (int32 i=0; i<slh; i++)
				g_arrDistMap24[i+poses0*rm0].b=0;	//right heel start
		}

		if ( (lhs+hposes)<rhe )
		{
			int32 elh=((lhs+hposes)+rhe)/2;
			g_arrDistMap24[elh-hposes+poses0*lm0].g=0x7f;	//left heel start
			g_arrDistMap24[elh+poses0*rm0].g=0x3f;	//right heel end
			for (int32 i=0; i<(elh-hposes); i++)
				g_arrDistMap24[i+poses0*lm0].b=0;	//left heel start
			for (int32 i=elh; i<poses0; i++)
				g_arrDistMap24[i+poses0*rm0].b=0;	//right heel end
		}
	}

	if (lts!=-1 && lte!=-1 && rts!=-1 && rte!=-1)
	{
		if ( (lte-hposes)>rts )
		{
			int32 slt=((lte-hposes)+rts)/2;
			//g_arrDistMap24[slt+hposes+poses0*lm1].g=0xff;	//right toe start
			//g_arrDistMap24[slt+poses0*rm1].g=0x3f;	//left toe start
			for (int32 i=(slt+hposes); i<poses0; i++)
				g_arrDistMap24[i+poses0*lm1].r=0;	//right toe start
			for (int32 i=0; i<slt; i++)
				g_arrDistMap24[i+poses0*rm1].r=0;	//left toe start
		}

		if ( (lts+hposes)<rte )
		{
			int32 elt=((lts+hposes)+rte)/2;
			//g_arrDistMap24[elt-hposes+poses0*lm1].g=0x7f;	//left toe start
			//g_arrDistMap24[elt+poses0*rm1].g=0x3f;	//right toe end
			for (int32 i=0; i<(elt-hposes); i++)
				g_arrDistMap24[i+poses0*lm1].r=0;	//left toe start
			for (int32 i=elt; i<poses0; i++)
				g_arrDistMap24[i+poses0*rm1].r=0;	//right toe end
		}
	}

	//-----------------------------------------------------------------------------

	for (int32 i=0; i<poses0; i++)
	{
		if (i<hposes)
		{
			g_arrDistMap24[i+poses0*lx0+hposes].b=g_arrDistMap24[i+poses0*lm0].b;	//left toe start
			g_arrDistMap24[i+poses0*lx1+hposes].r=g_arrDistMap24[i+poses0*lm1].r;	//left toe start
		}
		else
		{
			g_arrDistMap24[i+poses0*lx0-hposes].b=g_arrDistMap24[i+poses0*lm0].b;	//left toe start
			g_arrDistMap24[i+poses0*lx1-hposes].r=g_arrDistMap24[i+poses0*lm1].r;	//left toe start
		}

		g_arrDistMap24[i+poses0*rx0].b=g_arrDistMap24[i+poses0*rm0].b;	//left toe start
		g_arrDistMap24[i+poses0*rx1].r=g_arrDistMap24[i+poses0*rm1].r;	//left toe start
	}

	for (int32 i=0; i<poses0; i++)
	{
		uint32 footplant=0;
		if (g_arrDistMap24[i+poses0*lx0].b==0xff)
			footplant|=LHEEL;
		if (g_arrDistMap24[i+poses0*lx1].r==0xff)
			footplant|=LTOE0;
		if (g_arrDistMap24[i+poses0*rx0].b==0xff)
			footplant|=RHEEL;
		if (g_arrDistMap24[i+poses0*rx1].r==0xff)
			footplant|=RTOE0;

		rGlobalAnimHeader.m_FootPlantBits[i]=footplant;
	}

	//------------------------------------------------------------------

	lhs=-1;
	lhe=-1;
	lts=-1;
	lte=-1;
	rhs=-1;
	rhe=-1;
	rts=-1;
	rte=-1;
	for (int32 i=0,t=poses0-1; i<poses0; i++,t--)
	{
		if (lhs==-1)
			if (g_arrDistMap24[i+poses0*lm0].b==0xff) lhs=i;
		if (lts==-1)
			if (g_arrDistMap24[i+poses0*lm1].r==0xff) lts=i;
		if (lhe==-1)
			if (g_arrDistMap24[t+poses0*lm0].b==0xff) lhe=t;
		if (lte==-1)
			if (g_arrDistMap24[t+poses0*lm1].r==0xff) lte=t;

		if (rhs==-1)
			if (g_arrDistMap24[i+poses0*rm0].b==0xff) rhs=i;
		if (rts==-1)
			if (g_arrDistMap24[i+poses0*rm1].r==0xff) rts=i;
		if (rhe==-1)
			if (g_arrDistMap24[t+poses0*rm0].b==0xff) rhe=t;
		if (rte==-1)
			if (g_arrDistMap24[t+poses0*rm1].r==0xff) rte=t;
	}


	f32 pose = f32(poses0);

	if (lhs!=-1)
		rFootPlants.m_LHeelStart	=	lhs/pose;
	if (lhe!=-1)
		rFootPlants.m_LHeelEnd		=	lhe/pose;
	if (lts!=-1)
		rFootPlants.m_LToe0Start	=	lts/pose;
	if (lte!=-1)
		rFootPlants.m_LToe0End		=	lte/pose;

	if (rhs!=-1)
		rFootPlants.m_RHeelStart	=	rhs/pose;;
	if (rhe!=-1)
		rFootPlants.m_RHeelEnd		=	rhe/pose;;
	if (rts!=-1)
		rFootPlants.m_RToe0Start	=	rts/pose;;
	if (rte!=-1)
		rFootPlants.m_RToe0End		=	rte/pose;;

}

bool IsOldChunk(CChunkFile::ChunkDesc * ch)
{
	return (ch->hdr.ChunkVersion < CONTROLLER_CHUNK_DESC_0829::VERSION ) || (ch->hdr.ChunkVersion ==  CONTROLLER_CHUNK_DESC_0831::VERSION);
}

bool IsNewChunk(CChunkFile::ChunkDesc * ch)
{
	return !IsOldChunk(ch);
}

void CAnimationCompressor::DeleteOldChunk(EDeleteMethod delFlag, bool bEraseAnotherChunks)
{

	// cleanup all chunks
	if (bEraseAnotherChunks)
	{
		while (true) {
			CChunkFile::ChunkDesc *cd = m_ChunkFile.FindChunkByType(ChunkType_SpeedInfo);
			if (cd)
				m_ChunkFile.DeleteChunkId( cd->hdr.ChunkID );
			else
				break;
		}

		while (true) {
			CChunkFile::ChunkDesc *cd = m_ChunkFile.FindChunkByType(ChunkType_FootPlantInfo);
			if (cd)
				m_ChunkFile.DeleteChunkId( cd->hdr.ChunkID );
			else
				break;
		}

		while (true) {
			CChunkFile::ChunkDesc *cd = m_ChunkFile.FindChunkByType(ChunkType_BoneNameList);
			if (cd)
				m_ChunkFile.DeleteChunkId( cd->hdr.ChunkID );
			else
				break;
		}
	}

	// delete controller info
	if (delFlag != eSkipDelete)
	{
		uint32 numChunck = m_ChunkFile.NumChunks();
		for (uint32 i=0; i<numChunck; i++)
		{

			CChunkFile::ChunkDesc *cd = m_ChunkFile.GetChunk(i);
			if (cd)
			{
				if (cd->hdr.ChunkType != ChunkType_Controller)
					continue;

				if (delFlag == eAll || ((delFlag == eOld) && IsOldChunk(cd) ) || ((delFlag == eNew) && IsNewChunk(cd)))
				{
					m_ChunkFile.DeleteChunkId( cd->hdr.ChunkID );
					--numChunck;
					i = 0;
				}
			}
		}
	}


}


bool CAnimationCompressor::CompareKeyTimes(KeyTimesInformationPtr& ptr1, KeyTimesInformationPtr& ptr2)
{

	if (ptr1->GetNumKeys() != ptr2->GetNumKeys())
	{
		return false;
	}

	for (uint32 i = 0; i < ptr1->GetNumKeys(); ++i)
	{
		if (ptr1->GetKeyValueFloat(i) != ptr2->GetKeyValueFloat(i))
		{
			return false;
		}
	}
	return true;
}


struct CAnimChunkData
{
	std::vector<char> m_data;

	template <class T>
	void Add( const T& object )
	{
		AddData( &object,sizeof(object) );
	}
	void AddData( const void *pSrcData,int nSrcDataSize )
	{
		const char* src = static_cast<const char*>(pSrcData);
		if(m_data.empty())
			m_data.assign( src, src + nSrcDataSize);
		else
			m_data.insert( m_data.end(), src, src + nSrcDataSize);
	}
	void* data()
	{
		return &m_data.front();
	}

	size_t size()const{return m_data.size();}
};



int  CAnimationCompressor::SaveControllers(CSaverCGF& saver, TControllersVector& m_arrController)
{

	uint32 numController = m_arrController.size();
	for (uint32 numChunks = 0, end = m_arrController.size();  numChunks < end; ++numChunks)
	{
		{
			CControllerPQLog *pController  = dynamic_cast<CControllerPQLog *>(m_arrController[numChunks].get());

			if (pController)
			{
				std::auto_ptr<CryKeyPQLog>  pKeys;

				int numKeys = m_arrController[numChunks]->GetControllerInfo().numKeys;//pInfo->m_arrTracksTimes[numChunks].size();
				int controllerID = m_arrController[numChunks]->GetID();//pInfo->m_arrControllerId[numChunks];

				pKeys.reset(new CryKeyPQLog[numKeys]);

				CControllerPQLog * pController = (CControllerPQLog *)m_arrController[numChunks].get();
				for (uint32 i=0; i<numKeys; ++i)
				{
					pKeys.get()[i].nTime =  pController->m_arrTimes[i]  ;//pInfo->m_arrTracksTimes[numChunks].operator [](i); //TrackTimes[i];
					pKeys.get()[i].vPos = pController->m_arrKeys[i].vPos * 100.0f;;//pInfo->m_arrTracksPQLog[numChunks].operator [](i).vPos  * 100.0f;
					pKeys.get()[i].vRotLog = pController->m_arrKeys[i].vRotLog;//pInfo->m_arrTracksPQLog[numChunks].operator [](i).vRotLog;
				}

				saver.SaveController( numKeys, controllerID, pKeys.get(), numKeys );

				continue;
			}
		}

		{
			// new format controller
			CCompressedController * pController = dynamic_cast<CCompressedController *>(m_arrController[numChunks].get());
			if (pController)
			{
				SaveWaveletController(pController, saver);
				continue;
			}
		}

		{
			// new format controller
			CController * pController = dynamic_cast<CController *>(m_arrController[numChunks].get());
			if (pController)
			{
				SaveCController(pController, saver);
				continue;
			}
		}
	}

	return 1;
}

void CAnimationCompressor::SaveCController(CController * pController, CSaverCGF& saver)
{
	CONTROLLER_CHUNK_DESC_0829 chunk;

	uint32 nSize = 0;

	bool bPosUseRot(false);
	bool bScaleUseRot(false);
	bool bScaleUsePos(false);

	ZeroStruct(chunk);

	chunk.chdr.ChunkType = ChunkType_Controller;
	chunk.chdr.ChunkVersion = CONTROLLER_CHUNK_DESC_0829::VERSION;

	int s = sizeof(chunk);
	CAnimChunkData Data;

	chunk.nControllerId = pController->GetID();
	SWAP SwapEndianness(chunk.nControllerId);

	RotationControllerPtr pRotation = pController->GetRotationController();
	KeyTimesInformationPtr pRotTimes;
	if (pRotation)
	{
		pRotTimes = pRotation->GetKeyTimesInformation();
		chunk.numRotationKeys = pRotation->GetNumKeys();
		SWAP SwapEndianness(chunk.numRotationKeys);

		chunk.RotationFormat = pRotation->GetFormat();
		SWAP SwapEndianness(chunk.RotationFormat);


		chunk.RotationTimeFormat = pRotTimes->GetFormat();
		SWAP SwapEndianness(chunk.RotationTimeFormat);

		//copy to chunk
		SWAP pRotation->GetRotationStorage()->SwapBytes();
		Data.AddData(pRotation->GetRotationStorage()->GetData(), pRotation->GetRotationStorage()->GetDataRawSize());
		SWAP pRotation->GetRotationStorage()->SwapBytes();

		SWAP pRotTimes->SwapBytes();
		Data.AddData(pRotTimes->GetData(), pRotTimes->GetDataRawSize());
		SWAP pRotTimes->SwapBytes();

		//// FOR TESTS!!!
		//FILE * f = fopen("e:\\compquat.txt", "wb");
		//Vec3 pos;
		//
		//for (uint32 a = 0; a < pRotation->GetNumKeys(); ++a)
		//{
		//	f32 time = pRotation->GetKeyTimesInformation()->GetKeyValueFloat(a);
		//	
		//	Quat quats;
		//	pRotation->GetValueFromKey(a, quats);
		//	Ang3 ang;
		//	ang = Ang3::GetAnglesXYZ(quats);
		//	fprintf(f, "%f %f %f %f %f %f %f\n", time, pos.x, pos.y, pos.z, ang.x, ang.y, ang.z);
		//}
		//fclose(f);
		//// END
	}

	PositionControllerPtr pPosition = pController->GetPositionController();
	KeyTimesInformationPtr pPosTimes;
	if (pPosition)
	{
		pPosTimes = pPosition->GetKeyTimesInformation();
		chunk.numPositionKeys = pPosition->GetNumKeys();
		SWAP SwapEndianness(chunk.numPositionKeys);

		chunk.PositionFormat = pPosition->GetFormat();
		SWAP SwapEndianness(chunk.PositionFormat);

		chunk.PositionKeysInfo = CONTROLLER_CHUNK_DESC_0829::eKeyTimePosition;

		SWAP pPosition->GetPositionStorage()->SwapBytes();
		Data.AddData(pPosition->GetPositionStorage()->GetData(), pPosition->GetPositionStorage()->GetDataRawSize());
		SWAP pPosition->GetPositionStorage()->SwapBytes();

		//// FOR TESTS!!!
		//FILE * f = fopen("e:\\compressed.txt", "wb");
		//Vec4 quat;
		//
		//for (uint32 a = 0; a < pPosition->GetNumKeys(); ++a)
		//{
		//	f32 time = pPosition->GetKeyTimesInformation()->GetKeyValueFloat(a);
		//	
		//	Vec3 pos;
		//	pPosition->GetValueFromKey(a, pos);
		//	fprintf(f, "%f %f %f %f %f %f %f\n", time, quat.x, quat.y, quat.z, pos.x, pos.y, pos.z);
		//}
		//fclose(f);
		//// END

		if (pRotation && CompareKeyTimes(pPosTimes, pRotTimes))
		{
			chunk.PositionKeysInfo = CONTROLLER_CHUNK_DESC_0829::eKeyTimeRotation;
		}
		else
		{
			SWAP pPosTimes->SwapBytes();

			Data.AddData(pPosTimes->GetData(), pPosTimes->GetDataRawSize());
			SWAP pPosTimes->SwapBytes();
			chunk.PositionTimeFormat = pPosTimes->GetFormat();
		}
	}

	uint32 numData = Data.size();
	if (numData)
		saver.SaveController829(chunk, Data.data(), numData );
}



void CAnimationCompressor::SaveWaveletController(CCompressedController * pController, CSaverCGF& saver)
{
	CONTROLLER_CHUNK_DESC_0830 chunk;
	CONTROLLER_CHUNK_DESC_0829 chunk1;

	uint32 nSize = 0;

	bool bPosUseRot(false);
	bool bScaleUseRot(false);
	bool bScaleUsePos(false);

	ZeroStruct(chunk);

	int s = sizeof(chunk);
	int s1 = sizeof(chunk1);

	chunk.chdr.ChunkType = ChunkType_Controller;
	chunk.chdr.ChunkVersion = CONTROLLER_CHUNK_DESC_0830::VERSION;

	CAnimChunkData Data;

	chunk.nControllerId = pController->GetID();

	RotationControllerPtr pRotation = pController->GetRotationController();
	KeyTimesInformationPtr pRotTimes;// = pRotation->GetKeyTimesInformation();

	bool bWaveletRot(false);
	bool bWaveletPos(false);

	chunk.ChunkType = 0;

	if (pController->m_Rotations0.size() > 0)
	{
		// we have a wavelet compression
		bWaveletRot = true;
		pRotTimes = pRotation->GetKeyTimesInformation();
		chunk.RotationTimeFormat = pRotTimes->GetFormat();
		chunk.RotationFormat = pController->m_iRotationFormat;

		chunk.numRotationKeys = pRotTimes->GetNumKeys();

		Data.AddData(&(pController->m_Rotations0[0]), pController->m_Rotations0.size() * sizeof(int));
		Data.AddData(&(pController->m_Rotations1[0]), pController->m_Rotations1.size() * sizeof(int));
		Data.AddData(&(pController->m_Rotations2[0]), pController->m_Rotations2.size() * sizeof(int));
		Data.AddData(pRotTimes->GetData(), pRotTimes->GetDataRawSize());

		chunk.ChunkType = 1;
	}
	else
		if (pRotation)
		{
			pRotTimes = pRotation->GetKeyTimesInformation();
			chunk.numRotationKeys = pRotation->GetNumKeys();
			chunk.RotationFormat = pRotation->GetFormat();
			chunk.RotationTimeFormat = pRotTimes->GetFormat();

			//copy to chunk
			Data.AddData(pRotation->GetRotationStorage()->GetData(), pRotation->GetRotationStorage()->GetDataRawSize());
			Data.AddData(pRotTimes->GetData(), pRotTimes->GetDataRawSize());

			//// FOR TESTS!!!
			//FILE * f = fopen("e:\\compquat.txt", "wb");
			//Vec3 pos;
			//
			//for (uint32 a = 0; a < pRotation->GetNumKeys(); ++a)
			//{
			//	f32 time = pRotation->GetKeyTimesInformation()->GetKeyValueFloat(a);
			//	
			//	Quat quats;
			//	pRotation->GetValueFromKey(a, quats);
			//	Ang3 ang;
			//	ang = Ang3::GetAnglesXYZ(quats);
			//	fprintf(f, "%f %f %f %f %f %f %f\n", time, pos.x, pos.y, pos.z, ang.x, ang.y, ang.z);
			//}
			//fclose(f);
			//// END

		}

		PositionControllerPtr pPosition = pController->GetPositionController();
		KeyTimesInformationPtr pPosTimes;


		if (pController->m_Positions0.size() > 0)
		{
			pPosTimes = pPosition->GetKeyTimesInformation();

			chunk.PositionFormat = pController->m_iPositionFormat;//pPosition->GetFormat();
			chunk.PositionKeysInfo = CONTROLLER_CHUNK_DESC_0829::eKeyTimePosition;
			chunk.numPositionKeys = pPosition->GetKeyTimesInformation()->GetNumKeys();

			Data.AddData(&(pController->m_Positions0[0]), pController->m_Positions0.size() * sizeof(int));
			Data.AddData(&(pController->m_Positions1[0]), pController->m_Positions1.size() * sizeof(int));
			Data.AddData(&(pController->m_Positions2[0]), pController->m_Positions2.size() * sizeof(int));

			if (pRotation && CompareKeyTimes(pPosTimes, pRotTimes))
			{
				chunk.PositionKeysInfo = CONTROLLER_CHUNK_DESC_0829::eKeyTimeRotation;
			}
			else
			{
				Data.AddData(pPosTimes->GetData(), pPosTimes->GetDataRawSize());
				chunk.PositionTimeFormat = pPosTimes->GetFormat();
			}

			chunk.ChunkType |= 2;
		}
		else
			if (pPosition)
			{
				pPosTimes = pPosition->GetKeyTimesInformation();
				chunk.numPositionKeys = pPosition->GetKeyTimesInformation()->GetNumKeys();
				chunk.PositionFormat = pPosition->GetFormat();
				chunk.PositionKeysInfo = CONTROLLER_CHUNK_DESC_0829::eKeyTimePosition;


				Data.AddData(pPosition->GetPositionStorage()->GetData(), pPosition->GetPositionStorage()->GetDataRawSize());

				// FOR TESTS!
				//FILE * f = fopen("e:\\compressed.txt", "wb");
				//Vec4 quat;
				//
				//for (uint32 a = 0; a < pPosition->GetNumKeys(); ++a)
				//{
				//	f32 time = pPosition->GetKeyTimesInformation()->GetKeyValueFloat(a);
				//	
				//	Vec3 pos;
				//	pPosition->GetValueFromKey(a, pos);
				//	fprintf(f, "%f %f %f %f %f %f %f\n", time, quat.x, quat.y, quat.z, pos.x, pos.y, pos.z);
				//}
				//fclose(f);
				//// END

				if (pRotation && CompareKeyTimes(pPosTimes, pRotTimes))
				{
					chunk.PositionKeysInfo = CONTROLLER_CHUNK_DESC_0829::eKeyTimeRotation;
				}
				else
				{
					Data.AddData(pPosTimes->GetData(), pPosTimes->GetDataRawSize());
					chunk.PositionTimeFormat = pPosTimes->GetFormat();
				}
			}

			saver.SaveController830(chunk, Data.data(), Data.size() );
}


IController* GlobalAnimationHeaderCAF::GetController(uint32 nControllerID)
{
	uint32 numController = m_arrController.size();
	for (uint32 i=0; i<numController; i++) 
	{
		if ( m_arrController[i] && m_arrController[i]->GetID()==nControllerID )
			return m_arrController[i];
	}
	return NULL;
}

