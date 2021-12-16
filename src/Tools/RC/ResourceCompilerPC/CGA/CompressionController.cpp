#include "stdafx.h"

#include "AnimationLoader.h"
#include "CompressionController.h"
#include "ControllerPQ.h"

//#define POS_EPSILON (0.02f)  //if the change less then 2cm per meter, then we ignore this animation
//#define ROT_EPSILON (0.005f)


void CCompressonator::CreateWaveletCompression(GlobalAnimationHeaderCAF& header, GlobalAnimationHeaderCAF& newheader, CompressionInfo& info)
{

	//!!!!!!!!!!!!!!!!!!!!!!!!!!
	// FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!FIXME!
	return;

	GlobalAnimationHeaderCAF temp;
	temp = header;

	int TotalCompressed = 0;
	int OldCompression = 0;

	for (uint32 c = 0; c < header.m_arrController.size(); ++c )
	{

		CCompressedController * pNewController = new CCompressedController;

		CController * pCController  = dynamic_cast<CController*>(temp.m_arrController[c].get());

		if (!pCController)
		{
			assert(false);
			return;
		}

		pNewController->SetPositionController(pCController->GetPositionController());
		pNewController->SetRotationController(pCController->GetRotationController());

		pNewController->Compress(/*pCController);//*/header.m_arrController[c]);

		TotalCompressed += pNewController->GetCompressedSize();

		if (pCController->GetRotationController())
			OldCompression += pCController->GetRotationController()->GetRotationStorage()->GetDataRawSize();

		if (pCController->GetPositionController())
			OldCompression += pCController->GetPositionController()->GetPositionStorage()->GetDataRawSize();

		newheader.m_arrController[c] = pNewController;
		pNewController->m_pqGlobalAnimationHeader = &newheader;
		pNewController->m_nControllerId = pCController->m_nControllerId;
	}

	float res = (float)OldCompression / (float)TotalCompressed;




}

void CCompressonator::FillTrackInfo(GlobalAnimationHeaderCAF &header, uint32 c, uint32 &curControllerId, std::vector<Quat>& Rotations, std::vector<Vec3>& Positions, std::vector<int>& RotTimes, std::vector<int>& PosTimes)
{
	CControllerPQLog * pController = dynamic_cast<CControllerPQLog *>(header.m_arrController[c].get());

	if (pController)
	{
		Rotations.clear();
		Positions.clear();
		PosTimes.clear();
		RotTimes.clear();

		uint32 numTimes = pController->m_arrTimes.size();
		Rotations.reserve(numTimes);
		Positions.reserve(numTimes);
		PosTimes.reserve(numTimes);
		RotTimes.reserve(numTimes);

		curControllerId = pController->m_nControllerId;

		int32 oldtime =  -1;

		for (uint32 i=0; i<numTimes; ++i)
		{
			if (oldtime != pController->m_arrTimes[i])
			{
				oldtime = pController->m_arrTimes[i];

				Vec3 vRot = pController->m_arrKeys[i].vRotLog;
			//	if (vRot.IsValid()==0)
			//		vRot=Vec3(ZERO);

				if (vRot.x<-gf_PI || vRot.x>gf_PI)
					vRot.x=0;	
				if (vRot.y<-gf_PI || vRot.y>gf_PI)
					vRot.y=0;	
				if (vRot.z<-gf_PI || vRot.z>gf_PI)
					vRot.z=0;	

				Vec3 vPos = pController->m_arrKeys[i].vPos;
			//	if (vPos.IsValid()==0)
			//		vPos=Vec3(ZERO);

				int32 time = pController->m_arrTimes[i];

				Rotations.push_back(!exp(vRot));
				Positions.push_back(vPos);

				RotTimes.push_back(pController->m_arrTimes[i]);
				PosTimes.push_back(pController->m_arrTimes[i]);
			}
		}
	}
	else
	{
		CController * pCController  = dynamic_cast<CController*>(header.m_arrController[c].get());

		if (!pCController)
		{
			assert(false);
			return;
		}

		uint32 numRotKeys=pCController->GetRotationController()->GetNumKeys();
		Rotations.resize(numRotKeys);
		RotTimes.resize(numRotKeys);

		uint32 numPosKeys=pCController->GetPositionController()->GetNumKeys();
		Positions.resize(numPosKeys);
		PosTimes.resize(numPosKeys);

		curControllerId = pCController->m_nControllerId;

		for (uint32 i=0; i<numRotKeys; ++i)
		{
			pCController->GetRotationController()->GetValueFromKey(i, m_Rotations[i]);//!exp(pController->m_arrKeys[i].vRotLog);
			RotTimes[i] = int(pCController->GetRotationController()->GetTimeFromKey(i));
		}
		for (uint32 i=0; i<numPosKeys; ++i)
		{
			pCController->GetPositionController()->GetValueFromKey(i, m_Positions[i]);//pController->m_arrKeys[i].vPos;
			PosTimes[i] = int(pCController->GetPositionController()->GetTimeFromKey(i));
		}
	}
}



void CCompressonator::CreateCompression(ConvertContext& cc, GlobalAnimationHeaderCAF& header, GlobalAnimationHeaderCAF& newheader, CompressionInfo& info, CSkeletonInfo * skeleton, SAnimationDesc& desc, int GAID, uint32 PrintDebugText, uint32 nCompressRoot )
{
	if (desc.m_AdditiveAnimation==0)
		CreateCompression_OverrideAsset(cc,header,newheader,info,skeleton,desc,GAID,PrintDebugText,nCompressRoot);
	else 
		CreateCompression_AdditiveAsset(cc,header,newheader,info,skeleton,desc,GAID,PrintDebugText,nCompressRoot);
}


void CCompressonator::CreateCompression_OverrideAsset(ConvertContext& cc, GlobalAnimationHeaderCAF& header, GlobalAnimationHeaderCAF& newheader, CompressionInfo& info, CSkeletonInfo * skeleton, SAnimationDesc& desc, int GAID, uint32 PrintDebugText, uint32 nCompressRoot )
{
	assert(desc.m_AdditiveAnimation==0);
	uint32 nExportedJointsCounter=0;

	uint32 curControllerId = -1;

	uint32 numController = header.m_arrController.size();
	for (uint32 c=1; c<numController; ++c)
	{
		curControllerId = 0;

		CControllerPQLog * pController = dynamic_cast<CControllerPQLog *>(header.m_arrController[c].get());
		if (pController)
		{
			curControllerId = pController->m_nControllerId;
		}
		else
		{
			CController * pCController  = dynamic_cast<CController*>(header.m_arrController[c].get());
			if (!pCController)
			{
				assert(false);
				return;
			}
			curControllerId = pCController->m_nControllerId;
		}

	}


	//---------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------
	for (uint32 c=0; c<numController; ++c)
	{
		CompressionLevelInfo *currentInfo = info.GetInfo(c);
		if (currentInfo == 0)
			currentInfo = info.GetInfo(3);

		FillTrackInfo(header, c, curControllerId, m_Rotations, m_Positions, m_RotTimes, m_PosTimes);
		bool bIdenticalPos(true);
		bool bIdenticalRot(true);


		int boneNumber = -1;
		const char* pBoneName=0;
		uint32 numJoints = skeleton->m_SkinningInfo.m_arrBonesDesc.size();
		for (uint32 b=0; b<numJoints; ++b)
		{
			if (skeleton->m_SkinningInfo.m_arrBonesDesc[b].m_nControllerID == curControllerId)
			{
				boneNumber = b;
				pBoneName = skeleton->m_SkinningInfo.m_arrBonesDesc[b].m_arrBoneName;
				break;
			}
		}

		if (boneNumber<0)
		{
			//Skeleton has no corresponding joint. Skip this controller 
			currentInfo->m_bPosCompression = eSkipTrack;
			currentInfo->m_bRotCompression = eSkipTrack;
		}

		//are all rotation-keys nearly identical?
		Quat rot = m_Rotations[0];
		uint32 numRotKeys = m_Rotations.size();
		for (uint32 i=1; i<numRotKeys; ++i)
		{	
			if ( !rot.IsEquivalent(m_Rotations[i],desc.m_fROT_EPSILON) )
			{
				bIdenticalRot = false;
				break;
			}
		}

		//are all position-keys nearly identical?
		Vec3 pos = m_Positions[0];
		f32 pos_epsilon = desc.m_fPOS_EPSILON;
		if (boneNumber>0)
		{
			int32 offset = skeleton->m_SkinningInfo.m_arrBonesDesc[boneNumber].m_nOffsetParent;
			int32 pidx = boneNumber+offset;	assert(pidx>-1);
			Vec3 p34=skeleton->m_SkinningInfo.m_arrBonesDesc[    pidx    ].m_DefaultB2W.GetTranslation();
			Vec3 c34=skeleton->m_SkinningInfo.m_arrBonesDesc[ boneNumber ].m_DefaultB2W.GetTranslation();
			Vec3 BoneSegment = p34-c34;
			pos_epsilon = (BoneSegment*desc.m_fPOS_EPSILON).GetLength(); 
			pos_epsilon = min(pos_epsilon,0.02f); 
		}


		uint32 numPosKeys = m_Positions.size();
		for (uint32 i=1; i<numPosKeys; ++i)
		{	
			Vec3 vBoneSeg = m_Positions[i];
			if ( !pos.IsEquivalent(m_Positions[i],pos_epsilon) )
			{
				bIdenticalPos = false;
				break;
			}
		}

		bool bSinglePos(false);
		bool bSingleRot(false);

		if (bIdenticalRot || bIdenticalPos)
		{
			// check with reference model
			if (skeleton == 0)
			{
				bIdenticalRot = false;
				bIdenticalPos = false;
			}

			// get relative quat from skeleton
			if (boneNumber < 0)
			{
				bIdenticalRot = false;
				bIdenticalPos = false;
			}
			else
			{
				f32 pos_epsilon = desc.m_fPOS_EPSILON;
				QuatT relRigValue = QuatT(skeleton->m_SkinningInfo.m_arrBonesDesc[boneNumber].m_DefaultB2W);
				if (boneNumber)
				{
					int32 offset = skeleton->m_SkinningInfo.m_arrBonesDesc[boneNumber].m_nOffsetParent;
					int32 pidx = boneNumber+offset;	assert(pidx>-1);
					Matrix34 p34=skeleton->m_SkinningInfo.m_arrBonesDesc[ pidx ].m_DefaultB2W;
					Matrix34 c34=skeleton->m_SkinningInfo.m_arrBonesDesc[ boneNumber ].m_DefaultB2W;
					relRigValue = QuatT(p34.GetInverted() * c34);
					pos_epsilon = (relRigValue.t*desc.m_fPOS_EPSILON).GetLength(); 
					pos_epsilon = min(pos_epsilon,0.02f); 
				}

				//is this rotation-channel identical with rig?
				if ( bIdenticalRot && !rot.IsEquivalent(relRigValue.q,desc.m_fROT_EPSILON) )
					bIdenticalRot = false;

				//is this position-channel identical with rig?
				if ( bIdenticalPos && !pos.IsEquivalent(relRigValue.t,max(pos_epsilon,0.0001f) ))
					bIdenticalPos = false;
			}
		}
//m:\CryEngine3_Dev\GameCrysis2\animations\animations.cba  /file="m:\CryEngine3_Dev\GameCrysis2\Animations\human\male\weapons\ay69\3p\stand_tac_reloadFull_ay69_add_3p_02" /skipdba /wait
//m:\CryEngine3_Dev\GameCrysis2\animations\animations.cba  /cleanupfast=1 /p=X360 /dest="m:\CryEngine3_Dev\GameCrysis2\Anims_Converted\Animations" /wait

		if (boneNumber==0)
		{
			bIdenticalRot = false;
			bIdenticalPos = false;
		}





		//		 no need to store this track. remove it
		if (bIdenticalPos && bIdenticalRot)
		{
			//	RCLog("delelete controller    JointName: %s  ", pBoneName);
			newheader.m_arrController[c] = 0;
			continue;
		}


		if (pBoneName && PrintDebugText )
		{
#ifdef PRINTOUT
			RCLog("Rot %d Pos %d  id:%02d  Name:%s",!bIdenticalRot,!bIdenticalPos,nExportedJointsCounter,pBoneName);
#endif
			nExportedJointsCounter++;
		}

		if (boneNumber==0 && nCompressRoot==0) 
		{
			newheader.m_arrController[c] = (CController*)header.m_arrController[c].get();
			continue; //we don't want to compress the root twice
		}

		if ( bIdenticalPos )
			currentInfo->m_bPosCompression = eSkipTrack;

		if ( bIdenticalRot )
			currentInfo->m_bRotCompression = eSkipTrack;

		ECompressionInformation types[] = {	eNoCompressQuat, eSmallTree48BitQuat, eSmallTree64BitExtQuat}; 
		uint32 typesSize = 3;

		if (currentInfo->m_RotationFormat != eAutomaticQuat)
		{
			typesSize = 1;
			types[0] = (ECompressionInformation)currentInfo->m_RotationFormat;
		}

		//CController * pNewController[typesSize];
		std::vector<CController*> pNewController(typesSize);

		uint32 minSize = -1;
		uint32 minIndex;

		for (uint32 format = 0; format < typesSize; ++ format)
		{
			pNewController[format] = new CController;
			currentInfo->m_RotationFormat = types[format];

			if (currentInfo->m_bRotCompression != eSkipTrack)
				CompressRotations(pNewController[format], currentInfo, bSingleRot);

			if (currentInfo->m_bPosCompression != eSkipTrack)
				CompressPositions(pNewController[format], currentInfo, bSinglePos);

			pNewController[format]->m_nControllerId = header.m_arrController[c]->GetID();

			uint32 currentSize = pNewController[format]->SizeOfThis();
			if (currentSize < minSize)
			{
				minSize = currentSize;
				minIndex = format;
			}
		}

		if (currentInfo->m_bRotCompression == eUseOld && currentInfo->m_bPosCompression == eUseOld)
			pNewController[minIndex] = (CController*)header.m_arrController[c].get();
		header.m_arrController[c]->AddRef();


		pNewController[minIndex]->m_pqGlobalAnimationHeader = &newheader;
		newheader.m_arrController[c] = pNewController[minIndex];

		for (uint32 format = 0; format < typesSize; ++ format)
		{
			if (format != minIndex)
				delete pNewController[format] ;
		}
	}
}



void CCompressonator::CreateCompression_AdditiveAsset(ConvertContext& cc, GlobalAnimationHeaderCAF& header, GlobalAnimationHeaderCAF& newheader, CompressionInfo& info, CSkeletonInfo * skeleton, SAnimationDesc& desc, int GAID, uint32 PrintDebugText, uint32 nCompressRoot )
{
	assert(desc.m_AdditiveAnimation);
	uint32 nExportedJointsCounter=0;

	uint32 nStartKey1 = newheader.m_nStartKey;
	uint32 nStartKey2 = header.m_nStartKey;

	uint32 curControllerId = -1;


	uint32 numController = header.m_arrController.size();
	for (uint32 c=1; c<numController; ++c)
	{
		curControllerId = 0;

		CControllerPQLog * pController = dynamic_cast<CControllerPQLog *>(header.m_arrController[c].get());
		if (pController)
		{
			curControllerId = pController->m_nControllerId;
		}
		else
		{
			CController * pCController  = dynamic_cast<CController*>(header.m_arrController[c].get());
			if (!pCController)
			{
				assert(false);
				return;
			}
			curControllerId = pCController->m_nControllerId;
		}

	}


	//---------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------
	for (uint32 c=0; c<numController; ++c)
	{
		CompressionLevelInfo *currentInfo = info.GetInfo(c);
		if (currentInfo == 0)
			currentInfo = info.GetInfo(3);

		FillTrackInfo(header, c, curControllerId, m_Rotations, m_Positions, m_RotTimes, m_PosTimes);
		bool bIdenticalPos(true);
		bool bIdenticalRot(true);

		if (c>0)
		{
			//This is the part where we generate the additive animation
			uint32 numRotTimes = m_RotTimes.size();
			uint32 numRotKeys  = m_Rotations.size();
			assert(numRotKeys>1);
			if (numRotKeys>1)
			{

				for (uint32 k=1; k<numRotKeys; k++)
				{
					m_Rotations[k] = m_Rotations[k] * !m_Rotations[0];
					f32 dot = fabsf((Quat(IDENTITY)|m_Rotations[k])-1.0f);
					if (dot>desc.m_fROT_EPSILON) 
						bIdenticalRot=false;
				}
				m_RotTimes.erase( m_RotTimes.begin() );
				m_Rotations.erase( m_Rotations.begin() );
			}

			uint32 numPosTimes = m_PosTimes.size();
			uint32 numPosKeys  = m_Positions.size();
			assert(numPosKeys>1);
			if (numPosKeys>1)
			{

				f32 fLength = m_Positions[0].GetLength();
				for (uint32 k=1; k<numPosKeys; k++)
				{
					m_Positions[k] = m_Positions[k]-m_Positions[0];
					if (m_Positions[k].GetLength()>(fLength*desc.m_fPOS_EPSILON))
						bIdenticalPos=false;
				}
				m_PosTimes.erase( m_PosTimes.begin() );
				m_Positions.erase( m_Positions.begin() );
			}
		}



		int boneNumber = -1;
		const char* pBoneName=0;
		uint32 numJoints = skeleton->m_SkinningInfo.m_arrBonesDesc.size();
		for (uint32 b=0; b<numJoints; ++b)
		{
			if (skeleton->m_SkinningInfo.m_arrBonesDesc[b].m_nControllerID == curControllerId)
			{
				boneNumber = b;
				pBoneName = skeleton->m_SkinningInfo.m_arrBonesDesc[b].m_arrBoneName;
				break;
			}
		}

		if (boneNumber<0)
		{
			//Skeleton has no corresponding joint. Skip this controller 
			currentInfo->m_bPosCompression = eSkipTrack;
			currentInfo->m_bRotCompression = eSkipTrack;
		}

		//are all rotation-keys nearly identical?

		//are all position-keys nearly identical?
		bool bSinglePos(false);
		bool bSingleRot(false);


		//		 no need to store this track. remove it
		if (bIdenticalPos && bIdenticalRot)
		{
			//	RCLog("delelete controller    JointName: %s  ", pBoneName);
			newheader.m_arrController[c] = 0;
			continue;
		}


		if (pBoneName && PrintDebugText )
		{
#ifdef PRINTOUT
			RCLog("Rot %d Pos %d  id:%02d  Name:%s",!bIdenticalRot,!bIdenticalPos,nExportedJointsCounter,pBoneName);
#endif
			nExportedJointsCounter++;
		}

		if (boneNumber==0 && nCompressRoot==0) 
		{
			newheader.m_arrController[c] = (CController*)header.m_arrController[c].get();
			continue; //we don't want to compress the root twice
		}

		if ( bIdenticalPos )
			currentInfo->m_bPosCompression = eSkipTrack;

		if ( bIdenticalRot )
			currentInfo->m_bRotCompression = eSkipTrack;

		ECompressionInformation types[] = {	eNoCompressQuat, eSmallTree48BitQuat, eSmallTree64BitExtQuat}; 
		uint32 typesSize = 3;

		if (currentInfo->m_RotationFormat != eAutomaticQuat)
		{
			typesSize = 1;
			types[0] = (ECompressionInformation)currentInfo->m_RotationFormat;
		}

		//CController * pNewController[typesSize];
		std::vector<CController *> pNewController(typesSize);

		uint32 minSize = -1;
		uint32 minIndex;

		for (uint32 format = 0; format < typesSize; ++ format)
		{
			pNewController[format] = new CController;
			currentInfo->m_RotationFormat = types[format];

			if (currentInfo->m_bRotCompression != eSkipTrack)
				CompressRotations(pNewController[format], currentInfo, bSingleRot);

			if (currentInfo->m_bPosCompression != eSkipTrack)
				CompressPositions(pNewController[format], currentInfo, bSinglePos);

			pNewController[format]->m_nControllerId = header.m_arrController[c]->GetID();

			uint32 currentSize = pNewController[format]->SizeOfThis();
			if (currentSize < minSize)
			{
				minSize = currentSize;
				minIndex = format;
			}
		}

		if (currentInfo->m_bRotCompression == eUseOld && currentInfo->m_bPosCompression == eUseOld)
			pNewController[minIndex] = (CController*)header.m_arrController[c].get();
		header.m_arrController[c]->AddRef();


		pNewController[minIndex]->m_pqGlobalAnimationHeader = &newheader;
		newheader.m_arrController[c] = pNewController[minIndex];

		for (uint32 format = 0; format < typesSize; ++ format)
		{
			if (format != minIndex)
				delete pNewController[format] ;
		}
	}
}




// return true if difference between 2 quats is bigger than error
bool error_quat(Quat& q1, Quat& q2, float error)
{
	Float4Storage dist;

	dist.x= fabs(q1.v.x) - fabs(q2.v.x);
	dist.y= fabs(q1.v.y) - fabs(q2.v.y);
	dist.z= fabs(q1.v.z) - fabs(q2.v.z);
	dist.w= fabs(q1.w) - fabs(q2.w);

	float curerr = dist.x * dist.x  + dist.y * dist.y + dist.z * dist.z + dist.w * dist.w;
	if ( curerr > error)//0.00001f)
	{
		return true;
	}

	return false;
}

bool error_vec3(Vec3& q1, Vec3& q2, float error)
{
	Vec3 dist;

	dist.x= fabs(q1.x) - fabs(q2.x);
	dist.y= fabs(q1.y) - fabs(q2.y);
	dist.z= fabs(q1.z) - fabs(q2.z);
	//	dist.w= fabs(q1.w) - fabs(q2.w);

	float curerr = dist.x * dist.x  + dist.y * dist.y + dist.z * dist.z;
	if ( curerr > error)//0.00001f)
	{
		return true;
	}

	return false;
}

BaseCompressedQuat * GetCompressedQuat(int format, int count)
{ 
	switch(format)
	{
	case eNoCompress:
	case eNoCompressQuat:
		return new TNoCompressedQuat[count];

	case eShotInt3Quat:
		return new TShortInt3CompressedQuat[count];

	case eSmallTreeDWORDQuat:
		return new TSmallTreeDWORDCompressedQuat[count];

	case eSmallTree48BitQuat:
		return new TSmallTree48BitCompressedQuat[count];

	case eSmallTree64BitQuat:
		return new TSmallTree64BitCompressedQuat[count];

	case eSmallTree64BitExtQuat:
		return new TSmallTree64BitExtCompressedQuat[count];

	case ePolarQuat:
		return new TPolarQuatQuat[count];
	}

	return 0;
}

BaseCompressedVec3 * GetCompressedVec3(int format, int count)
{
	switch(format)
	{
	case eNoCompressVec3:
	default:
		return new TBaseCompressedVec3[count];
	}

	return 0;
}


void  CCompressonator::CompressRotations(CController * pController, CompressionLevelInfo * pInfo, bool bAddFirst)
{
	// create new controller
	uint32 numKeys = m_Rotations.size();

	KeyTimesInformationPtr pTimes;// = new F32KeyTimesInformation;//CKeyTimesInformation;

	if (m_RotTimes[numKeys-1] < 256)
		pTimes = new ByteKeyTimesInformation;
	else
		if (m_RotTimes[numKeys-1] < 65536)
			pTimes = new UINT16KeyTimesInformation;
		else
			pTimes = new F32KeyTimesInformation;


	RotationControllerPtr pNewTrack  =  RotationControllerPtr(new RotationTrackInformation);
	TrackRotationStoragePtr pStorage = ControllerHelper::GetRotationControllerPtr(pInfo->m_RotationFormat);
	pNewTrack->SetRotationStorage(pStorage);

	pNewTrack->SetKeyTimesInformation(pTimes);
	pController->SetRotationController( pNewTrack);

	if (bAddFirst)
	{
		pTimes->AddKeyTime( f32(m_RotTimes[0]) );
		pStorage->AddValue( m_Rotations[0] );
	}
	else
	{
		unsigned int nextstep = 1;
		uint32 count = 0;
		uint32 lastKey = 0;

		std::auto_ptr<BaseCompressedQuat> currentcompressed( GetCompressedQuat(pInfo->m_RotationFormat,1));
		std::auto_ptr<BaseCompressedQuat> lastcompressed(GetCompressedQuat(pInfo->m_RotationFormat,1)); 

		for (unsigned int iKey = 0; iKey < numKeys; iKey+= nextstep)
		{
			if (pInfo->m_bRotCompression == eCompression)
			{
				currentcompressed->FromQuat(m_Rotations[iKey]);
				Quat tmp = currentcompressed->ToQuat();

				unsigned int iNextKey = iKey + 2;
				for(; iNextKey < numKeys; ++iNextKey)
				{
					lastcompressed->FromQuat(m_Rotations[iNextKey]);
					Quat last = lastcompressed->ToQuat();
					bool result = false;
					for (unsigned int iTest =1; iTest <= iNextKey - iKey; ++iTest)
					{
						float step = (float)iTest/((float)iNextKey - (float)iKey);

						Quat interpolated;
						interpolated.SetNlerp(tmp, last, step);

						if (error_quat(interpolated, m_Rotations[iKey+iTest], pInfo->m_RotationError))
						{
							result = true;
							break;
						}
					}

					if (result)
					{
						// test failed
						iNextKey -= 1;
						break;
					}
				}

				nextstep = iNextKey - iKey; 
				++count;
			}

			pTimes->AddKeyTime( f32(m_RotTimes[iKey]) );
			pStorage->AddValue(m_Rotations[iKey]);
			lastKey = iKey;
		}

		if ((lastKey < numKeys - 1))// && (!m_Rotations[numKeys-1].IsEquivalent(m_Rotations[lastKey])))
		{
			pTimes->AddKeyTime( f32(m_RotTimes[numKeys-1]) );
			pStorage->AddValue(m_Rotations[numKeys - 1]);
			++count;
		}
	}
}

void CCompressonator::CompressPositions(CController * pController, CompressionLevelInfo * pInfo, bool bAddFirst)
{
	// create new controller
	uint32 numKeys = m_Positions.size();

	KeyTimesInformationPtr pTimes;// = new F32KeyTimesInformation;//CKeyTimesInformation;
	/*float compressionMaxError = 0.00000001f;*/
	if (m_PosTimes[numKeys-1] < 256)
		pTimes = new ByteKeyTimesInformation;
	else
		if (m_PosTimes[numKeys-1] < 65536)
			pTimes = new UINT16KeyTimesInformation;
		else
			pTimes = new F32KeyTimesInformation;

	//	PositionControllerPtr pNewTrack = ControllerHelper::GetPositionControllerPtr(pInfo->m_PositionFormat);
	PositionControllerPtr pNewTrack = PositionControllerPtr(new PositionTrackInformation);

	TrackPositionStoragePtr pStorage = ControllerHelper::GetPositionControllerPtr(pInfo->m_PositionFormat);

	pNewTrack->SetPositionStorage(pStorage);


	//GetCompressedPositionTrack(pInfo->m_PositionFormat);
	pNewTrack->SetKeyTimesInformation(pTimes);
	pController->SetPositionController( pNewTrack);

	if (bAddFirst)
	{
		pTimes->AddKeyTime(f32(m_PosTimes[0]));
		pStorage->AddValue(m_Positions[0]);
	}
	else
	{
		unsigned int nextstep = 1;
		uint32 count = 0;
		unsigned int iLastKey = 0;

		for (unsigned int iKey = 0; iKey < numKeys; iKey+= nextstep)
		{
			if (pInfo->m_bPosCompression == eCompression)
			{
				std::auto_ptr<BaseCompressedVec3> currentcompressed( GetCompressedVec3(pInfo->m_PositionFormat,1));
				currentcompressed->FromVec3(m_Positions[iKey]);
				Vec3 tmp = currentcompressed->ToVec3();

				unsigned int iNextKey = iKey + 2;
				for(; iNextKey < numKeys; ++iNextKey)
				{
					std::auto_ptr<BaseCompressedVec3> lastcompressed(GetCompressedVec3(pInfo->m_PositionFormat,1)); 
					lastcompressed->FromVec3(m_Positions[iNextKey]);
					Vec3 last = lastcompressed->ToVec3();


					bool result = false;
					for (unsigned int iTest =1; iTest <= iNextKey - iKey; ++iTest)
					{
						float step = (float)iTest/((float)iNextKey - (float)iKey);
						//slerp_quats(interpolated, step ,tmp, last); 
						Vec3 interpolated;
						interpolated.SetLerp(tmp, last, step);

						if (error_vec3(interpolated, m_Positions[iKey+iTest], pInfo->m_PositionError))
						{
							result = true;
							break;
						}
					}

					if (result)
					{
						// test failed
						iNextKey -= 1;
						break;
					}
				}

				nextstep = iNextKey - iKey; 
				++count;
			}

			iLastKey = iKey;
			pTimes->AddKeyTime(f32(m_PosTimes[iKey]));
			pStorage->AddValue(m_Positions[iKey]);
		}

		if ((iLastKey < numKeys - 1))
		{
			pTimes->AddKeyTime( f32(m_PosTimes[numKeys-1]) ) ;
			pStorage->AddValue(m_Positions[numKeys - 1]);
			++count;
		}
	}
}



