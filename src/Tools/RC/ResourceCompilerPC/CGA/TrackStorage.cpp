#include "StdAfx.h"

#include "TrackStorage.h"
#include "AnimationCompiler.h"
#include "CGFContent.h"


CTrackStorage::CTrackStorage(void)
{
}

CTrackStorage::~CTrackStorage(void)
{
}

bool CTrackStorage::GetAnimFileTime(const string& name,  DWORD * ft)
{
	string tmp(name);
	FixString(tmp);

	std::size_t npos = tmp.find("animations");
	if (npos != string::npos)
		tmp.erase(0, npos);

	int p = tmp.find("//");
	while (p != string::npos)
	{
		tmp.erase(p,1 );
		p = tmp.find("//");
	}

	CryAutoLockRC lock(m_lock);
	// add new animation;
	uint32 num = FindAnimation(tmp);

	if (num == -1)
	{
		return false;
	}

	if (num > m_arrAnimTimes.size())
	{
		return false;
	}

	*ft = m_arrAnimTimes[num];

	return true;
}

uint32 CTrackStorage::FindAnimation(const string& name)
{
	CryAutoLockRC lock(m_lock);
	uint32 count = m_arrAnimNames.size();
	const char * pName = name.c_str();
	for (uint32 i = 0; i < count; ++i)
	{
		if (!stricmp(m_arrAnimNames[i].c_str(),pName ))
			return i;
	}

	return -1;
}

uint32 CTrackStorage::GetNumAnimations(GlobalAnimationHeaderCAF& header, string& name, DWORD timeStamp)
{
	string tmp(name);
	FixString(tmp);

	std::size_t npos = tmp.find("animations");
	if (npos != string::npos)
		tmp.erase(0, npos);

	int p = tmp.find("//");
	while (p != string::npos)
	{
		tmp.erase(p,1 );
		p = tmp.find("//");
	}


	CryAutoLockRC lock(m_lock);
	// add new animation;
	uint32 num = FindAnimation(tmp);
	if (num == -1)
	{
		m_arrGlobalAnimationHeaderCAF.push_back(header);
		m_arrAnimNames.push_back(tmp);
		m_arrAnimTimes.push_back(timeStamp);
		num = m_arrGlobalAnimationHeaderCAF.size() - 1;
	}

	return num;
}

void CTrackStorage::AddAnimation(const string& strFilePathDBA, GlobalAnimationHeaderCAF& header, string& name, bool bAddToDB, DWORD timeStamp)
{
	CryAutoLockRC lock(m_lock);
	uint32 num = GetNumAnimations(header, name, timeStamp);
	m_arrAnimTimes[num] = timeStamp;

	if (bAddToDB)
		AddAnimationToDatabase(strFilePathDBA, num, false);

}

void CTrackStorage::UpdateAnimation(const string& strFilePathDBA, GlobalAnimationHeaderCAF& header, string& name, bool bAddToDB, DWORD timeStamp, bool bFast)
{
	CryAutoLockRC lock(m_lock);
	uint32 num = GetNumAnimations(header, name, timeStamp);
	m_arrAnimTimes[num] = timeStamp;

	if (bAddToDB)
		UpdateAnimationInDatabase(strFilePathDBA, header, num, bFast);
}


bool CTrackStorage::IsRotationIdentical(const RotationControllerPtr& track1, const TrackRotationStoragePtr& track2)
{
	bool bShared(false);

	if (track1->GetNumKeys() == track2->GetNumKeys())
	{
		uint32 ticks = track1->GetNumKeys();

		if (ticks > 0)
			bShared = true;

		for (uint32 t = 0; t < ticks; ++t)
		{
			Quat pos0, pos1;
			track1->GetValueFromKey(t, pos0);
			track2->GetValue(t, pos1);
			if (!pos0.IsEquivalent(pos1,0.0000001f))
			{
				bShared = false;
				break;
			}
		}
	}

	return bShared;
}

bool CTrackStorage::IsPositionIdentical(const PositionControllerPtr& track1, const TrackPositionStoragePtr& track2)
{
	bool bShared(false);

	if (track1->GetNumKeys() == track2->GetNumKeys())
	{
		// compare position tracks
		uint32 ticks = track1->GetNumKeys();

		if (ticks > 0)
			bShared = true;

		for (uint32 t = 0; t < ticks; ++t)
		{
			Vec3 pos0, pos1;
			track1->GetValueFromKey(t, pos0);
			track2->GetValue(t, pos1);
			if (!pos0.IsEquivalent(pos1,0.0000001f))
			{
				bShared = false;
				break;
			}
		}
	}

	return bShared;
}


bool CTrackStorage::IsKeyTimesIdentical(const KeyTimesInformationPtr& track1, const KeyTimesInformationPtr& track2)
{
	bool bShared(false);
	if (track1->GetNumKeys() == track2->GetNumKeys())
	{
		uint32 ticks = track1->GetNumKeys();

		if (ticks > 0)
			bShared = true;

		for (uint32 t = 0; t < ticks; ++t)
		{
			if (track1->GetKeyValueFloat(t) != track2->GetKeyValueFloat(t))
			{
				bShared = false;
				break;

			}
		}
	}

	return bShared;

}
// worse method. only for testing purposes
void CTrackStorage::Analize(uint32& TrackShared, uint32& SizeDataShared, uint32& TotalTracks, uint32& TotalMemory,  CSkeletonInfo * currentSkeleton, ConvertContext& cc)
{
	CryAutoLockRC lock(m_lock);

	std::map<uint32, uint32> m_BonesCount;

	uint32 totalAnims = m_arrGlobalAnimationHeaderCAF.size();

	for (uint32 i = 0; i < totalAnims; ++i)
	{
		uint32 srcSize = m_arrGlobalAnimationHeaderCAF[i].m_arrController.size();

		for (uint32 k = 0; k < srcSize; k++)
		{
			CController * pSrc = dynamic_cast<CController *>(m_arrGlobalAnimationHeaderCAF[i].m_arrController[k].get());				
			if (pSrc )
			{

				if (pSrc->GetRotationController())
				{
					TotalMemory += pSrc->GetRotationController()->GetRotationStorage()->GetDataRawSize() + pSrc->GetRotationController()->GetKeyTimesInformation()->GetDataRawSize();
					++TotalTracks;
				}
				if (pSrc->GetPositionController())
				{
					TotalMemory += pSrc->GetPositionController()->GetPositionStorage()->GetDataRawSize() + pSrc->GetPositionController()->GetKeyTimesInformation()->GetDataRawSize();
					++TotalTracks;
				}


			}
			else
				continue;

			for (uint32 j = i +1; j < totalAnims; ++j)
			{

				uint32 dstSize = m_arrGlobalAnimationHeaderCAF[j].m_arrController.size();

				for (uint32 l = 0; l < dstSize; ++l)
				{

					CController * pDst = dynamic_cast<CController *>(m_arrGlobalAnimationHeaderCAF[j].m_arrController[l].get());

					if (pDst && pDst->m_bShared)
						continue;

					if (pSrc && pDst)
					{

						bool bAlready(false);

						if (pSrc->GetPositionController() && pDst->GetPositionController() &&(pSrc->GetPositionController()->GetNumKeys() == pDst->GetPositionController()->GetNumKeys()))
						{
							// compare position tracks
							uint32 ticks = pSrc->GetPositionController()->GetNumKeys();
							bool bShared(false);

							if (ticks > 0)
								bShared = true;

							for (uint32 t = 0; t < ticks; ++t)
							{
								Vec3 pos0, pos1;
								pSrc->GetPositionController()->GetValueFromKey(t, pos0);
								pDst->GetPositionController()->GetValueFromKey(t, pos1);
								if (!pos0.IsEquivalent(pos1,0.0000001f))
								{
									bShared = false;
									break;
								}
							}
							if (bShared)// && !pDst->m_bShared)
							{
								//								++TrackShared;
								//								SizeDataShared += pSrc->GetPositionController()->GetDataRawSize() + pSrc->GetPositionController()->GetKeyTimesInformation()->GetDataRawSize();
								bAlready = true;
							}

						}


						if (pSrc->GetRotationController() && pDst->GetRotationController() &&(pSrc->GetRotationController()->GetNumKeys() == pDst->GetRotationController()->GetNumKeys()))
						{
							// compare position tracks
							uint32 ticks = pSrc->GetRotationController()->GetNumKeys();
							bool bShared(false);

							if (ticks > 0)
								bShared = true;

							for (uint32 t = 0; t < ticks; ++t)
							{
								Quat pos0, pos1;
								pSrc->GetRotationController()->GetValueFromKey(t, pos0);
								pDst->GetRotationController()->GetValueFromKey(t, pos1);
								if (!pos0.IsEquivalent(pos1,0.0000001f))
								{
									bShared = false;
									break;
								}
							}
							if (bShared)// && !pDst->m_bShared)
							{
								//								if (!bAlready)
								//								++TrackShared;
								bAlready = true;
								//								SizeDataShared += pSrc->GetRotationController()->GetDataRawSize() + pSrc->GetRotationController()->GetKeyTimesInformation()->GetDataRawSize();
							}
						}

						if (bAlready)
						{
							pDst->m_bShared = true;
							//							m_arrGlobalAnimationHeaderCAF[j].m_arrController[l] = 0;
						}
					}
				}
			}
		}
	}


	for (uint32 i = 0; i < totalAnims; ++i)
	{
		uint32 srcSize = m_arrGlobalAnimationHeaderCAF[i].m_arrController.size();

		for (uint32 k = 0; k < srcSize; k++)
		{
			CController * pSrc = dynamic_cast<CController *>(m_arrGlobalAnimationHeaderCAF[i].m_arrController[k].get());				
			if (pSrc && pSrc->m_bShared )
			{
				m_BonesCount[pSrc->m_nControllerId] += 1;

				if (pSrc->GetRotationController())
				{
					SizeDataShared += pSrc->GetRotationController()->GetRotationStorage()->GetDataRawSize() + pSrc->GetRotationController()->GetKeyTimesInformation()->GetDataRawSize();
					++TrackShared;
				}
				if (pSrc->GetPositionController())
				{
					SizeDataShared += pSrc->GetPositionController()->GetPositionStorage()->GetDataRawSize() + pSrc->GetPositionController()->GetKeyTimesInformation()->GetDataRawSize();
					++TrackShared;
				}
			}
		}
	}



	std::map<uint32, uint32>::iterator it= m_BonesCount.begin();
	std::map<uint32, uint32>::iterator end = m_BonesCount.end();

	for (; it != end; ++it)
	{
		uint32 bones = currentSkeleton->m_SkinningInfo.m_arrBonesDesc.size();
		bool bNotFound(true);
		for (uint32 b =0; b < bones; ++b )
		{
			if (currentSkeleton->m_SkinningInfo.m_arrBonesDesc[b].m_nControllerID == it->first)
			{
				RCLog("Bone %s shared %d times", currentSkeleton->m_SkinningInfo.m_arrBonesDesc[b].m_arrBoneName, it->second);
				bNotFound = false;
				break;
			}
		}

		if (bNotFound)
		{
			RCLog("Unnamed bone(ID=%d) shared %d times", it->first, it->second);
		}

	}
}

void CTrackStorage::UpdateAnimationInDatabase(const string& strFilePathDBA, GlobalAnimationHeaderCAF& header, uint32 num, bool bFast)
{
	CryAutoLockRC lock(m_lock);
	m_arrGlobalAnimationHeaderCAF[num] = header;
	/*
	uint32 numControllers = m_arrGlobalAnimationHeaderCAF[num].m_arrController.size();
	m_arrGlobalAnimationHeaderCAF[num].m_NameForSave = m_arrAnimNames[num];

	for (uint32 c = 0; c < numControllers; ++c)
	{
	// 
	CController * pController = dynamic_cast<CController *>(m_arrGlobalAnimationHeaderCAF[num].m_arrController[c].get());

	if (!pController)
	continue;

	CControllerInfo info;

	info.m_nControllerID = pController->m_nControllerId;

	if (pController->GetPositionController())
	{
	// Check identical
	KeyTimesInformationPtr pKeyTimes = pController->GetPositionController()->GetKeyTimesInformation();

	m_arrKeyTimes.push_back(pKeyTimes);
	pController->m_nPositionKeyTimesTrackId = m_arrKeyTimes.size() - 1;


	PositionControllerPtr pKeys = pController->GetPositionController();

	m_arrPositionTracks.push_back(pKeys->GetPositionStorage());
	pController->m_nPositionTrackId = m_arrPositionTracks.size() - 1;

	info.m_nPosKeyTimeTrack = pController->m_nPositionKeyTimesTrackId;
	info.m_nPosTrack = pController->m_nPositionTrackId;
	}

	if (pController->GetRotationController())
	{
	KeyTimesInformationPtr pKeyTimes = pController->GetRotationController()->GetKeyTimesInformation();

	// add new track
	m_arrKeyTimes.push_back(pKeyTimes);
	pController->m_nRotationKeyTimesTrackId = m_arrKeyTimes.size() - 1;

	RotationControllerPtr pKeys = pController->GetRotationController();

	m_arrRotationTracks.push_back(pKeys->GetRotationStorage());
	pController->m_nRotationTrackId = m_arrRotationTracks.size() - 1;

	info.m_nRotKeyTimeTrack = pController->m_nRotationKeyTimesTrackId;
	info.m_nRotTrack = pController->m_nRotationTrackId;

	}

	m_arrGlobalAnimationHeaderCAF[num].m_arrControlerInfo.push_back(info);
	}

	*/

	AddAnimationToDatabase(strFilePathDBA, num, bFast);
}

void CTrackStorage::AddAnimationToDatabase(const string& strFilePathDBA, uint32 num, bool bFast)
{
	CryAutoLockRC lock(m_lock);
	uint32 numControllers = m_arrGlobalAnimationHeaderCAF[num].m_arrController.size();

	const char* pname = m_arrAnimNames[num];
	m_arrGlobalAnimationHeaderCAF[num].SetFilePathCAF(m_arrAnimNames[num]);

	for (uint32 c = 0; c < numControllers; ++c)
	{
		CController * pController = dynamic_cast<CController *>(m_arrGlobalAnimationHeaderCAF[num].m_arrController[c].get());
		if (!pController)
			continue;

		CControllerInfo info;

		info.m_nControllerID = pController->m_nControllerId;

		if (pController->GetPositionController())
		{
			// Check identical
			KeyTimesInformationPtr pKeyTimes = pController->GetPositionController()->GetKeyTimesInformation();

			if (bFast)
				pController->m_nPositionKeyTimesTrackId = -1;
			else
				pController->m_nPositionKeyTimesTrackId = FindKeyTimesTrack(pKeyTimes);

			if (pController->m_nPositionKeyTimesTrackId == -1)
			{
				// add new track
				m_arrKeyTimes.push_back(pKeyTimes);
				pController->m_nPositionKeyTimesTrackId = m_arrKeyTimes.size() - 1;
				m_arrKeyTimesRemap.insert(std::make_pair<int, int>(pKeyTimes->GetNumKeys(), pController->m_nPositionKeyTimesTrackId));
			}


			PositionControllerPtr pKeys = pController->GetPositionController();

			if (bFast)
				pController->m_nPositionTrackId = -1;
			else
				pController->m_nPositionTrackId = FindPositionTrack(pKeys);

			if (pController->m_nPositionTrackId == -1)
			{
				m_arrPositionTracks.push_back(pKeys->GetPositionStorage());
				pController->m_nPositionTrackId = m_arrPositionTracks.size() - 1;
				m_arrKeyPosRemap.insert(std::make_pair<int, int>(pKeys->GetNumKeys(), pController->m_nPositionTrackId));
			}

			info.m_nPosKeyTimeTrack = pController->m_nPositionKeyTimesTrackId;
			info.m_nPosTrack = pController->m_nPositionTrackId;
		}

		if (pController->GetRotationController())
		{
			KeyTimesInformationPtr pKeyTimes = pController->GetRotationController()->GetKeyTimesInformation();

			if (bFast)
				pController->m_nRotationKeyTimesTrackId = - 1;
			else
				pController->m_nRotationKeyTimesTrackId = FindKeyTimesTrack(pKeyTimes);

			if (pController->m_nRotationKeyTimesTrackId == -1)
			{
				// add new track
				m_arrKeyTimes.push_back(pKeyTimes);
				pController->m_nRotationKeyTimesTrackId = m_arrKeyTimes.size() - 1;
				m_arrKeyTimesRemap.insert(std::make_pair<int, int>(pKeyTimes->GetNumKeys(), pController->m_nRotationKeyTimesTrackId));
			}


			RotationControllerPtr pKeys = pController->GetRotationController();

			if (bFast)
				pController->m_nRotationTrackId = - 1;		
			else
				pController->m_nRotationTrackId = FindRotationTrack(pKeys);

			if (pController->m_nRotationTrackId == -1)
			{
				m_arrRotationTracks.push_back(pKeys->GetRotationStorage());
				pController->m_nRotationTrackId = m_arrRotationTracks.size() - 1;
				m_arrKeyRotRemap.insert(std::make_pair<int, int>(pKeys->GetNumKeys(), pController->m_nRotationTrackId));
			}

			info.m_nRotKeyTimeTrack = pController->m_nRotationKeyTimesTrackId;
			info.m_nRotTrack = pController->m_nRotationTrackId;
		}

		m_arrGlobalAnimationHeaderCAF[num].m_arrControlerInfo.push_back(info);
	}
}




//chunk for x86
void CTrackStorage::SaveDataBase903(const char * name)
{
	CryAutoLockRC lock(m_lock);
	CChunkFile chunkFile;
	CSaverCGF saver(name, chunkFile);

	// main storage
	std::vector<char> storage;

	// Save Controllers 
	std::vector<uint16> sizesKeyTimes;
	//	std::vector<uint8> formatKeyTimes;

	uint32 keyTimes =  m_arrKeyTimes.size();
	sizesKeyTimes.reserve(keyTimes);
	//	formatKeyTimes.reserve(keyTimes);

	uint32 totalSize(0);

	AnalizeKeyTimes();

	printf("Saved bytes %i", m_Statistics.m_iSavedBytes);

	// build remap 

	// KeyTimes remap
	std::vector<uint32> ktRemap(keyTimes);
	std::vector<uint32> ktRevRemap(keyTimes);
	std::vector<uint32> ktFormats(eBitset + 1);
	memset(&ktFormats[0], 0, ktFormats.size() * sizeof(uint32));
	std::vector<uint32> ktCount(eBitset + 1);
	memset(&ktCount[0], 0, ktCount.size() * sizeof(uint32));

	for(uint32 k = 0; k < keyTimes; ++k)
	{
		++ktFormats[m_arrKeyTimes[k]->GetFormat()];
	}

	for (uint32 i = 1; i < eBitset + 1; ++i)
	{
		//ktFormats[i] += ktFormats[i-1]; // base offset
		ktCount[i] +=  ktCount[i-1] + ktFormats[i-1];
	}

	for(uint32 k = 0; k < keyTimes; ++k)
	{
		uint16 curFormat = m_arrKeyTimes[k]->GetFormat();
		ktRemap[k] =  ktCount[curFormat];// + ktFormats[curFormat];
		ktRevRemap[ktRemap[k]] = k;
		++ktCount[curFormat];
	}

	// Positions remap
	uint32 keyPos = m_arrPositionTracks.size();
	std::vector<uint32> pRemap(keyPos);
	std::vector<uint32> pRevRemap(keyPos);
	std::vector<uint32> pFormats(eAutomaticQuat);
	memset(&pFormats[0], 0, pFormats.size() * sizeof(uint32));
	std::vector<uint32> pCount(eAutomaticQuat);
	memset(&pCount[0], 0, pCount.size() * sizeof(uint32));

	for(uint32 k = 0; k < keyPos; ++k)
	{
		++pFormats[m_arrPositionTracks[k]->GetFormat()];
	}

	for (uint32 i = 1; i < eAutomaticQuat; ++i)
	{
		//pFormats[i] += pFormats[i-1]; // base offset
		pCount[i] += pCount[i-1] + pFormats[i-1];
	}

	for(uint32 k = 0; k < keyPos; ++k)
	{
		uint16 curFormat = m_arrPositionTracks[k]->GetFormat();
		pRemap[k] = pCount[curFormat];// + pFormats[curFormat];
		pRevRemap[pRemap[k]] = k;
		++pCount[curFormat];
	}

	// Rotations remap
	uint32 keyRot = m_arrRotationTracks.size();
	std::vector<uint32> rRemap(keyRot);
	std::vector<uint32> rRevRemap(keyRot);
	std::vector<uint32> rFormats(eAutomaticQuat);
	memset(&rFormats[0], 0, rFormats.size() * sizeof(uint32));
	std::vector<uint32> rCount(eAutomaticQuat);
	memset(&rCount[0], 0, rCount.size() * sizeof(uint32));

	for(uint32 k = 0; k < keyRot; ++k)
	{
		++rFormats[m_arrRotationTracks[k]->GetFormat()];
	}

	for (uint32 i = 1; i < eAutomaticQuat; ++i)
	{
		//		rFormats[i] += rFormats[i-1]; // base offset
		rCount[i] += rCount[i-1] + rFormats[i-1];
	}

	for(uint32 k = 0; k < keyRot; ++k)
	{
		uint16 curFormat = m_arrRotationTracks[k]->GetFormat();
		rRemap[k] = rCount[curFormat];// + rFormats[curFormat];
		rRevRemap[rRemap[k]] = k;
		++rCount[curFormat];
	}



	for(uint32 k = 0; k < keyTimes; ++k)
	{
		// store keytime format in the size
		uint16 keyTime = m_arrKeyTimes[k]->GetDataCount();
		totalSize += m_arrKeyTimes[k]->GetDataRawSize();
		uint8 format = m_arrKeyTimes[k]->GetFormat();
		sizesKeyTimes.push_back(keyTime);
	}

	totalSize += ktFormats.size() * sizeof(uint32) + sizesKeyTimes.size() * sizeof(uint16);

	std::vector<uint16> sizesPos;
	sizesPos.reserve(keyPos);

	for (uint32 p = 0; p < keyPos; ++p)
	{
		uint16 numKeyPos = m_arrPositionTracks[p]->GetDataCount();
		totalSize += m_arrPositionTracks[p]->GetDataRawSize();
		uint8 format = m_arrPositionTracks[p]->GetFormat();
		sizesPos.push_back(numKeyPos);
	}

	totalSize += pFormats.size() * sizeof(uint32) + sizesPos.size() * sizeof(uint16);

	std::vector<uint16> sizesRot;
	sizesRot.reserve(keyRot);

	for (uint32 r = 0; r < keyRot; ++r)
	{
		uint16 numKeyRot = m_arrRotationTracks[r]->GetDataCount();
		totalSize += m_arrRotationTracks[r]->GetDataRawSize();
		uint8 format = m_arrRotationTracks[r]->GetFormat();

		sizesRot.push_back(numKeyRot);
	}

	totalSize += rFormats.size() * sizeof(uint32) + sizesRot.size() * sizeof(uint16);

	// count size for storing 
	uint32 totalAnims = m_arrGlobalAnimationHeaderCAF.size();
	m_arrStartDirs.resize(m_arrGlobalAnimationHeaderCAF.size());

	for (uint32 i = 0; i < totalAnims; ++i)
	{

		uint32 numControllers = m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo.size();
		totalSize += sizeof(CStoredSkinningInfo);
		totalSize += m_arrGlobalAnimationHeaderCAF[i].m_FootPlantBits.size() * sizeof(uint8) + sizeof(uint16);

		totalSize += m_arrGlobalAnimationHeaderCAF[i].m_FilePath.size() + sizeof(uint16);
		// counter for ControllerInfo
		totalSize += sizeof(uint16) + numControllers * sizeof(CControllerInfo);
	}

	// add real data
	storage.resize(totalSize);
	uint32 currentPointer = 0;

	// keytime sizes

	std::vector<uint16> tmp(sizesKeyTimes.size());

	for (uint32 i = 0; i < sizesKeyTimes.size(); ++i)
	{
		tmp[ktRemap[i]] = sizesKeyTimes[i];
	}


#define SWAP 	if (g_pAnimationCompiler->GetBehavior().m_bNeedSwapEndian) 

	SWAP	SwapEndianness(&tmp[0], tmp.size());

	memcpy(&storage[currentPointer], &tmp[0], sizesKeyTimes.size() * sizeof(uint16));
	currentPointer += sizesKeyTimes.size() * sizeof(uint16);
	SWAP	SwapEndianness(&ktFormats[0], ktFormats.size());
	memcpy(&storage[currentPointer], &ktFormats[0], ktFormats.size() * sizeof(uint32));
	currentPointer += ktFormats.size() * sizeof(uint32);

	tmp.resize(sizesPos.size());

	for (uint32 i = 0; i < sizesPos.size(); ++i)
	{
		tmp[pRemap[i]] = sizesPos[i];
	}

	// pos sizes
	SWAP	SwapEndianness(&tmp[0], tmp.size());
	memcpy(&storage[currentPointer], &tmp[0], sizesPos.size() * sizeof(uint16));
	currentPointer += sizesPos.size() * sizeof(uint16);
	SWAP	SwapEndianness(&pFormats[0], pFormats.size());
	memcpy(&storage[currentPointer], &pFormats[0], pFormats.size()* sizeof(uint32));
	currentPointer += pFormats.size() * sizeof(uint32);


	tmp.resize(sizesRot.size());

	for (uint32 i = 0; i < sizesRot.size(); ++i)
	{
		tmp[rRemap[i]] = sizesRot[i];
	}

	SWAP	SwapEndianness(&tmp[0], tmp.size());
	memcpy(&storage[currentPointer], &tmp[0], sizesRot.size() * sizeof(uint16));
	currentPointer += sizesRot.size() * sizeof(uint16);
	SWAP	SwapEndianness(&rFormats[0], rFormats.size());
	memcpy(&storage[currentPointer], &rFormats[0], rFormats.size() * sizeof(uint32));
	currentPointer += rFormats.size() * sizeof(uint32);


	// copy raw data
	for(uint32 kk = 0; kk < keyTimes; ++kk)
	{

		uint32 k = ktRevRemap[kk];

		SWAP m_arrKeyTimes[k]->SwapBytes();

		memcpy(&storage[currentPointer], m_arrKeyTimes[k]->GetData(), m_arrKeyTimes[k]->GetDataRawSize());
		currentPointer += m_arrKeyTimes[k]->GetDataRawSize();
//		assert(m_arrKeyTimes[k]->GetDataRawSize() == (m_arrKeyTimes[k]->GetDataRawSize() / m_arrKeyTimes[k]->GetDataCount()) * m_arrKeyTimes[k]->GetDataCount());
	}

	// copy raw data
	for(uint32 pp = 0; pp < keyPos; ++pp)
	{
		uint32 p = pRevRemap[pp];

		SWAP m_arrPositionTracks[p]->SwapBytes();
		memcpy(&storage[currentPointer], m_arrPositionTracks[p]->GetData(), m_arrPositionTracks[p]->GetDataRawSize());
		currentPointer += m_arrPositionTracks[p]->GetDataRawSize();
//		assert( m_arrPositionTracks[p]->GetDataRawSize() == (m_arrPositionTracks[p]->GetDataRawSize() / m_arrPositionTracks[p]->GetDataCount()) * m_arrPositionTracks[p]->GetDataCount());
	}

	// copy raw data
	for(uint32 rr = 0; rr < keyRot; ++rr)
	{
		uint32 r = rRevRemap[rr];
		SWAP m_arrRotationTracks[r]->SwapBytes();
		memcpy(&storage[currentPointer], m_arrRotationTracks[r]->GetData(),m_arrRotationTracks[r]->GetDataRawSize());
		currentPointer += m_arrRotationTracks[r]->GetDataRawSize();
//		assert(m_arrRotationTracks[r]->GetDataRawSize() == (m_arrRotationTracks[r]->GetDataRawSize() / m_arrRotationTracks[r]->GetDataCount()) * m_arrRotationTracks[r]->GetDataCount());
	}


	CONTROLLER_CHUNK_DESC_0903 chunk;
	chunk.chdr.ChunkType = ChunkType_Controller;
	chunk.chdr.ChunkVersion = CONTROLLER_CHUNK_DESC_0903::VERSION;
	chunk.numKeyPos = keyPos;
	chunk.numKeyRot = keyRot;
	chunk.numKeyTime = keyTimes;
	chunk.numAnims = totalAnims;

	// Save animations list

	for (uint32 i = 0; i < totalAnims; ++i)
	{
		CStoredSkinningInfo info;

		info.m_LHeelEnd = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_LHeelEnd;
		SWAP	SwapEndianness(info.m_LHeelEnd);
		info.m_LHeelStart = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_LHeelStart;
		SWAP	SwapEndianness(info.m_LHeelStart);
		info.m_LToe0Start = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_LToe0Start;
		SWAP	SwapEndianness(info.m_LToe0Start);
		info.m_LToe0End = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_LToe0End;
		SWAP	SwapEndianness(info.m_LToe0End);
		info.m_RHeelEnd = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_RHeelEnd;
		SWAP	SwapEndianness(info.m_RHeelEnd);
		info.m_RHeelStart = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_RHeelStart;
		SWAP	SwapEndianness(info.m_RHeelStart);
		info.m_RToe0Start = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_RToe0Start;
		SWAP	SwapEndianness(info.m_RToe0Start);
		info.m_RToe0End = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_RToe0End;
		SWAP	SwapEndianness(info.m_RToe0End);
		info.m_Distance = m_arrGlobalAnimationHeaderCAF[i].m_fDistance;
		SWAP	SwapEndianness(info.m_Distance);
		info.m_Speed = m_arrGlobalAnimationHeaderCAF[i].m_fSpeed;
		SWAP	SwapEndianness(info.m_Speed);
		info.m_Slope = m_arrGlobalAnimationHeaderCAF[i].m_fSlope;
		SWAP	SwapEndianness(info.m_Slope);
		info.m_nAssetFlags = m_arrGlobalAnimationHeaderCAF[i].m_nFlags;
		SWAP	SwapEndianness(info.m_nAssetFlags);
		
		info.m_nTicksPerFrame	= m_arrGlobalAnimationHeaderCAF[i].m_nTicksPerFrame;
		SWAP	SwapEndianness(info.m_nTicksPerFrame);

		info.m_secsPerTick		= m_arrGlobalAnimationHeaderCAF[i].m_fSecsPerTick;
		SWAP	SwapEndianness(info.m_secsPerTick);
		info.m_nStart					= m_arrGlobalAnimationHeaderCAF[i].m_nStartKey;
		SWAP	SwapEndianness(info.m_nStart);
		info.m_nEnd						= m_arrGlobalAnimationHeaderCAF[i].m_nEndKey;
		SWAP	SwapEndianness(info.m_nEnd);

		m_arrStartDirs[i] = m_arrGlobalAnimationHeaderCAF[i].m_StartLocation2;
		SWAP	SwapEndianness(m_arrStartDirs[i].q.w);
		SWAP	SwapEndianness(m_arrStartDirs[i].q.v.x);
		SWAP	SwapEndianness(m_arrStartDirs[i].q.v.y);
		SWAP	SwapEndianness(m_arrStartDirs[i].q.v.z);
		SWAP	SwapEndianness(m_arrStartDirs[i].t.x);
		SWAP	SwapEndianness(m_arrStartDirs[i].t.y);
		SWAP	SwapEndianness(m_arrStartDirs[i].t.z);
	

		// fill info

		// Name length
		uint16 strSize = m_arrGlobalAnimationHeaderCAF[i].m_FilePath.size();
		SWAP	SwapEndianness(strSize);
		memcpy(&storage[currentPointer], &strSize, sizeof(uint16));
		currentPointer += sizeof(uint16);

		// name
		memcpy(&storage[currentPointer], m_arrGlobalAnimationHeaderCAF[i].m_FilePath.c_str(),m_arrGlobalAnimationHeaderCAF[i].m_FilePath.size());
		currentPointer += m_arrGlobalAnimationHeaderCAF[i].m_FilePath.size();

		// info
		///!!!!

		memcpy(&storage[currentPointer], &info, sizeof(info));
		currentPointer += sizeof(info);

		// footplants
		uint16 footplans = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantBits.size();
		SWAP	SwapEndianness(footplans);
		memcpy(&storage[currentPointer], &footplans, sizeof(uint16));
		currentPointer += sizeof(uint16);

		footplans = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantBits.size();
		memcpy(&storage[currentPointer], &(m_arrGlobalAnimationHeaderCAF[i].m_FootPlantBits[0]), footplans);
		currentPointer += footplans;

		// 
		uint16 controllerInfo = m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo.size();
		SWAP	SwapEndianness(controllerInfo);
		memcpy(&storage[currentPointer], &controllerInfo, sizeof(uint16));
		currentPointer += sizeof(uint16);
		controllerInfo = m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo.size();

		///!!!!!!!!!

		std::vector<CControllerInfo> infos(m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo.size());
		for (uint32  c =0; c < m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo.size(); ++c)
		{
			if (m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nPosKeyTimeTrack != -1)
				infos[c].m_nPosKeyTimeTrack = ktRemap[m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nPosKeyTimeTrack];
			else
				infos[c].m_nPosKeyTimeTrack = - 1;
			SWAP	SwapEndianness(infos[c].m_nPosKeyTimeTrack);

			if (m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nRotKeyTimeTrack != - 1)
				infos[c].m_nRotKeyTimeTrack = ktRemap[m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nRotKeyTimeTrack];
			else
				infos[c].m_nRotKeyTimeTrack = - 1;
			SWAP	SwapEndianness(infos[c].m_nRotKeyTimeTrack);

			if (m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nPosTrack != -1)
				infos[c].m_nPosTrack = pRemap[m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nPosTrack];
			else
				infos[c].m_nPosTrack = -1;
			SWAP	SwapEndianness(infos[c].m_nPosTrack);

			if (m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nRotTrack != -1)
				infos[c].m_nRotTrack = rRemap[m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nRotTrack];
			else
				infos[c].m_nRotTrack = -1;
			SWAP	SwapEndianness(infos[c].m_nRotTrack);

			infos[c].m_nControllerID = m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nControllerID;

			SWAP	SwapEndianness(infos[c].m_nControllerID);
		}

		memcpy(&storage[currentPointer], &(infos[0]), controllerInfo * sizeof(CControllerInfo));
		currentPointer += controllerInfo * sizeof(CControllerInfo);
	}

	saver.SaveControllerDB903(chunk, &storage[0], storage.size());

	CONTROLLER_CHUNK_DESC_0901 timeStepsChunk;
	timeStepsChunk.chdr.ChunkType = ChunkType_Controller;
	timeStepsChunk.chdr.ChunkVersion = CONTROLLER_CHUNK_DESC_0901::VERSION;
	timeStepsChunk.numTimesteps = m_arrAnimTimes.size();

	saver.SaveController901(timeStepsChunk, &m_arrAnimTimes[0], m_arrAnimTimes.size() * sizeof(DWORD));

	CONTROLLER_CHUNK_DESC_0902 startDirsChunk;
	startDirsChunk.chdr.ChunkType = ChunkType_Controller;
	startDirsChunk.chdr.ChunkVersion = CONTROLLER_CHUNK_DESC_0902::VERSION;
	startDirsChunk.numStartDirs = m_arrStartDirs.size();

	SWAP	SwapEndianness(&m_arrStartDirs[0],m_arrStartDirs.size() );

	saver.SaveController902(startDirsChunk, &m_arrStartDirs[0], m_arrStartDirs.size() * sizeof(QuatT));

	/*
	if (pChunkDesc->hdr.ChunkVersion == CONTROLLER_CHUNK_DESC_0902::VERSION)
	{
	CONTROLLER_CHUNK_DESC_0902* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0902*)pChunkDesc->data;
	swappod(*pCtrlChunk);

	m_attStartDirs.resize(pCtrlChunk->numStartDirs);

	char *pData = (char*)(pCtrlChunk+1);

	memcpy(&(m_attStartDirs[0]), pData, pCtrlChunk->numStartDirs* sizeof(DWORD) );
	}
	*/

	// Create the directory (including intermediate ones), if not already there.
	string windowPath = name; windowPath.replace('/', '\\');
	MakeSureDirectoryPathExists(windowPath.c_str());

	SetFileAttributes( name,FILE_ATTRIBUTE_ARCHIVE );
	chunkFile.Write( name );
}




bool CTrackStorage::LoadDataBase(const char * name)
{
	CryAutoLockRC lock(m_lock);
	CLoaderDBA  loader;

	CInternalDatabaseInfo * pInfo = loader.LoadDBA(name);
	if (pInfo == 0 )
		return false;

	//
	/*
	THeaderVector m_arrGlobalAnimationHeaderCAF;
	TNamesVector m_arrAnimNames;
	DynArray<KeyTimesInformationPtr> m_arrKeyTimes;
	DynArray<RotationControllerPtr> m_arrRotationTracks;
	DynArray<PositionControllerPtr> m_arrPositionTracks;
	*/

	m_arrKeyTimes = pInfo->m_arrKeyTimes;
	m_arrRotationTracks = pInfo->m_arrRotationTracks;
	m_arrPositionTracks = pInfo->m_arrPositionTracks;
	m_arrGlobalAnimationHeaderCAF.resize(pInfo->m_Headers.size());
	m_arrAnimNames.resize(pInfo->m_AnimationNames.size());
	m_arrAnimTimes.resize(pInfo->m_arrTimesteps.size());
	m_arrStartDirs.resize(pInfo->m_attStartDirs.size());
	memcpy(&m_arrAnimTimes[0],&pInfo->m_arrTimesteps[0], pInfo->m_arrTimesteps.size()*sizeof(DWORD));

	uint32 numHeaders = pInfo->m_Headers.size();
	for (uint32 i=0; i<numHeaders; i++)
	{
		// Add animations directly to TracksStorage

		CCommonSkinningInfo * pCSInfo = &pInfo->m_Headers[i];

		GlobalAnimationHeaderCAF& newHeader = m_arrGlobalAnimationHeaderCAF[i];

		newHeader.m_nTicksPerFrame = pCSInfo->m_nTicksPerFrame;
		newHeader.m_fSecsPerTick = pCSInfo->m_secsPerTick;
		newHeader.m_nStartKey = pCSInfo->m_nStart;
		newHeader.m_nEndKey = pCSInfo->m_nEnd;
		newHeader.m_fSpeed = pCSInfo->m_Speed;
		newHeader.m_fDistance = pCSInfo->m_Distance;
		newHeader.m_fSlope = pCSInfo->m_Slope;
		newHeader.m_nFlags = pCSInfo->m_nAssetFlags;

	//	if (pCSInfo->m_nAssetFlags&CA_ASSET_CYCLE)
//			newHeader.OnAssetCycle();
//		if (pCSInfo->m_nAssetFlags&CA_ASSET_ADDITIVE)
//			newHeader.IsAssetAdditive();

		newHeader.m_FootPlantVectors.m_LHeelStart = pCSInfo->m_LHeelStart,
			newHeader.m_FootPlantVectors.m_LHeelEnd = pCSInfo->m_LHeelEnd;
		newHeader.m_FootPlantVectors.m_LToe0Start = pCSInfo->m_LToe0Start;
		newHeader.m_FootPlantVectors.m_LToe0End = pCSInfo->m_LToe0End;
		newHeader.m_FootPlantVectors.m_RHeelStart = pCSInfo->m_RHeelStart;
		newHeader.m_FootPlantVectors.m_RHeelEnd = pCSInfo->m_RHeelEnd;
		newHeader.m_FootPlantVectors.m_RToe0Start = pCSInfo->m_RToe0Start;
		newHeader.m_FootPlantVectors.m_RToe0End = pCSInfo->m_RToe0End;
		newHeader.m_FootPlantBits.assign(pCSInfo->m_FootPlantBits.begin(), pCSInfo->m_FootPlantBits.end());
		newHeader.m_arrControlerInfo = pInfo->m_ControllerInfos[i];
		newHeader.SetFilePathCAF(pInfo->m_AnimationNames[i]);

		m_arrAnimNames[i] = pInfo->m_AnimationNames[i];
		m_arrStartDirs[i] = pInfo->m_attStartDirs[i];
	}

	return true;
}

// trying compress keytimes information
void CTrackStorage::AnalizeKeyTimes()
{
	CryAutoLockRC lock(m_lock);
	uint32 keyTimes =  m_arrKeyTimes.size();

	for(uint32 k = 0; k < keyTimes; ++k)
	{
		// store keytime format in the size

		if (m_arrKeyTimes[k]->GetFormat() < eF32StartStop)
		{

			uint16 keyTime = m_arrKeyTimes[k]->GetNumKeys();

			uint16 trackLength = (uint16)(m_arrKeyTimes[k]->GetKeyValueFloat(keyTime - 1) - m_arrKeyTimes[k]->GetKeyValueFloat(0));

			uint16 u8Length = (trackLength >> 3) + 3 * sizeof(uint16);

			if (u8Length < m_arrKeyTimes[k]->GetDataRawSize())
			{
				// convert to bitset
				m_Statistics.m_iSavedBytes += m_arrKeyTimes[k]->GetDataRawSize();
				CreateBitsetKeyTimes(k);
				m_Statistics.m_iSavedBytes -= m_arrKeyTimes[k]->GetDataRawSize();
			}
		}
		//if (keyTime == trackLength)
		//{
		//	// 
		//	m_Statistics.m_iSavedBytes +=  m_arrKeyTimes[k]->GetDataRawSize();
		//	CreateStartStopKeyTimes(k);
		//	m_Statistics.m_iSavedBytes -=  m_arrKeyTimes[k]->GetDataRawSize();
		//}
		//else

	}
}

void CTrackStorage::CreateBitsetKeyTimes(int k)
{
	CryAutoLockRC lock(m_lock);
	DynArray<uint16> data;
	uint16 start = uint16(m_arrKeyTimes[k]->GetKeyValueFloat(0));
	uint16 stop = uint16(m_arrKeyTimes[k]->GetKeyValueFloat(m_arrKeyTimes[k]->GetNumKeys()-1));

	data.push_back(start);
	data.push_back(stop);
	data.push_back(m_arrKeyTimes[k]->GetNumKeys());

	uint16 currentWord(0);
	uint16 currentTime(0);
	//bool bLast(false);
	int j(1), i(0);
	for (i =0, j =0; i < stop - start + 1/*m_arrKeyTimes[k]->GetNumKeys()*/; ++i, ++j)
	{
		if (j == 16)
		{
			data.push_back(currentWord);
			currentWord = 0;
			//							bLast = true;
			j = 0;
		}

		uint16 curData = -1;
		if (currentTime < m_arrKeyTimes[k]->GetNumKeys())
			curData = uint16(m_arrKeyTimes[k]->GetKeyValueFloat(currentTime) - start);

		uint16 val(0);
		if  (curData < i)
		{
			int a = 0;
		}
		if ( i == curData)
		{
			val = 1 << j;
			++currentTime;
		}

		if (i == 0 || i == stop - start)
		{
			val = 1 << j;
		}

		currentWord += val;
	}

	if (j)
	{
		data.push_back(currentWord);
	}

	KeyTimesInformationPtr verify = m_arrKeyTimes[k];
	m_arrKeyTimes[k] = ControllerHelper::GetKeyTimesControllerPtr(eBitset);
	m_arrKeyTimes[k]->ResizeKeyTime(data.size());
	//std::copy()
	if (m_arrKeyTimes[k]->GetDataRawSize() != data.size() * sizeof(short int)) {
		int a = 0;
	}
	memcpy(m_arrKeyTimes[k]->GetData(), &data[0], m_arrKeyTimes[k]->GetDataRawSize());


	for (int t = 0; t < verify->GetNumKeys(); ++t) 
	{
		if ( verify->GetKeyValueFloat(t) != m_arrKeyTimes[k]->GetKeyValueFloat(t))
		{
			int tada = 0;
		}
	}
}

void CTrackStorage::CreateStartStopKeyTimes(int k)
{
	CryAutoLockRC lock(m_lock);
	f32 start = m_arrKeyTimes[k]->GetKeyValueFloat(0);
	f32 stop = m_arrKeyTimes[k]->GetKeyValueFloat(m_arrKeyTimes[k]->GetNumKeys()-1);

	if (m_arrKeyTimes[k]->GetFormat() == eF32)
		m_arrKeyTimes[k] = ControllerHelper::GetKeyTimesControllerPtr(eF32StartStop);
	else
		if (m_arrKeyTimes[k]->GetFormat() == eUINT16)
			m_arrKeyTimes[k] = ControllerHelper::GetKeyTimesControllerPtr(eUINT16StartStop);
		else
			if (m_arrKeyTimes[k]->GetFormat() == eByte)
				m_arrKeyTimes[k] = ControllerHelper::GetKeyTimesControllerPtr(eByteStartStop);

	m_arrKeyTimes[k]->AddKeyTime(start);
	m_arrKeyTimes[k]->AddKeyTime(stop);
}



void CTrackStorage::SaveDataBase904(const char * name)
{
	CryAutoLockRC lock(m_lock);
	CChunkFile chunkFile;
	CSaverCGF saver(name, chunkFile);

	// main storage
	std::vector<char> storage;

	// Save Controllers 
	std::vector<uint16> sizesKeyTimes;
	//	std::vector<uint8> formatKeyTimes;

	uint32 keyTimes =  m_arrKeyTimes.size();
	sizesKeyTimes.reserve(keyTimes);
	//	formatKeyTimes.reserve(keyTimes);

	uint32 totalSize(0);

	AnalizeKeyTimes();

	printf("Saved bytes %i", m_Statistics.m_iSavedBytes);

	// build remap 

	// KeyTimes remap
	std::vector<uint32> ktRemap(keyTimes);
	std::vector<uint32> ktRevRemap(keyTimes);
	std::vector<uint32> ktFormats(eBitset + 1);
	std::vector<uint32> ktCount(eBitset + 1);

	memset(&ktFormats[0], 0, ktFormats.size() * sizeof(uint32));
	memset(&ktCount[0], 0, ktCount.size() * sizeof(uint32));

	for(uint32 k = 0; k < keyTimes; ++k)
	{
		++ktFormats[m_arrKeyTimes[k]->GetFormat()];
	}

	for (uint32 i = 1; i < eBitset + 1; ++i)
	{
		//ktFormats[i] += ktFormats[i-1]; // base offset
		ktCount[i] +=  ktCount[i-1] + ktFormats[i-1];
	}

	for(uint32 k = 0; k < keyTimes; ++k)
	{
		uint16 curFormat = m_arrKeyTimes[k]->GetFormat();
		ktRemap[k] =  ktCount[curFormat];// + ktFormats[curFormat];
		ktRevRemap[ktRemap[k]] = k;
		++ktCount[curFormat];
	}

	// Positions remap
	uint32 keyPos = m_arrPositionTracks.size();

	std::vector<uint32> pRemap(keyPos);
	std::vector<uint32> pRevRemap(keyPos);
	std::vector<uint32> pFormats(eAutomaticQuat);
	std::vector<uint32> pCount(eAutomaticQuat);

	memset(&pFormats[0], 0, pFormats.size() * sizeof(uint32));
	memset(&pCount[0], 0, pCount.size() * sizeof(uint32));

	for(uint32 k = 0; k < keyPos; ++k)
	{
		++pFormats[m_arrPositionTracks[k]->GetFormat()];
	}

	for (uint32 i = 1; i < eAutomaticQuat; ++i)
	{
		//pFormats[i] += pFormats[i-1]; // base offset
		pCount[i] += pCount[i-1] + pFormats[i-1];
	}

	for(uint32 k = 0; k < keyPos; ++k)
	{
		uint16 curFormat = m_arrPositionTracks[k]->GetFormat();
		pRemap[k] = pCount[curFormat];// + pFormats[curFormat];
		pRevRemap[pRemap[k]] = k;
		++pCount[curFormat];
	}

	// Rotations remap
	uint32 keyRot = m_arrRotationTracks.size();

	std::vector<uint32> rRemap(keyRot);
	std::vector<uint32> rRevRemap(keyRot);
	std::vector<uint32> rFormats(eAutomaticQuat);
	std::vector<uint32> rCount(eAutomaticQuat);

	memset(&rFormats[0], 0, rFormats.size() * sizeof(uint32));
	memset(&rCount[0], 0, rCount.size() * sizeof(uint32));

	for(uint32 k = 0; k < keyRot; ++k)
	{
		++rFormats[m_arrRotationTracks[k]->GetFormat()];
	}

	for (uint32 i = 1; i < eAutomaticQuat; ++i)
	{
		//		rFormats[i] += rFormats[i-1]; // base offset
		rCount[i] += rCount[i-1] + rFormats[i-1];
	}

	for(uint32 k = 0; k < keyRot; ++k)
	{
		uint16 curFormat = m_arrRotationTracks[k]->GetFormat();
		rRemap[k] = rCount[curFormat];// + rFormats[curFormat];
		rRevRemap[rRemap[k]] = k;
		++rCount[curFormat];
	}

	std::vector<uint32> keyTimeOffsets(keyTimes);

	for(uint32 k = 0; k < keyTimes; ++k)
	{
		// store keytime format in the size
		uint16 keyTime = m_arrKeyTimes[k]->GetDataCount();
		sizesKeyTimes.push_back(keyTime);
	}

	// copy raw data
	for(uint32 kk = 0; kk < keyTimes; ++kk)
	{
		uint32 k = ktRevRemap[kk];
		keyTimeOffsets[kk] = totalSize;
		totalSize += m_arrKeyTimes[k]->GetDataRawSize();
		totalSize += totalSize % 4 > 0 ? ( 4 - totalSize % 4) : 0;
	}

	std::vector<uint16> sizesPos;
	sizesPos.reserve(keyPos);

	std::vector<uint32> keyPosOffsets(keyPos);

	for (uint32 p = 0; p < keyPos; ++p)
	{
		uint16 numKeyPos = m_arrPositionTracks[p]->GetDataCount();
		uint8 format = m_arrPositionTracks[p]->GetFormat();
		sizesPos.push_back(numKeyPos);
	}

	for(uint32 pp = 0; pp < keyPos; ++pp)
	{
		uint32 p = pRevRemap[pp];
		keyPosOffsets[pp] = totalSize;
		totalSize += m_arrPositionTracks[p]->GetDataRawSize();
		totalSize += totalSize % 4 > 0 ? ( 4 - totalSize % 4) : 0;
	}



	std::vector<uint16> sizesRot;
	sizesRot.reserve(keyRot);

	std::vector<uint32> keyRotOffsets(keyRot+1);

	for (uint32 r = 0; r < keyRot; ++r)
	{

		uint16 numKeyRot = m_arrRotationTracks[r]->GetDataCount();
		sizesRot.push_back(numKeyRot);
	}


	for(uint32 rr = 0; rr < keyRot; ++rr)
	{
		uint32 r = rRevRemap[rr];
		keyRotOffsets[rr] = totalSize;
		totalSize += m_arrRotationTracks[r]->GetDataRawSize();
		totalSize += totalSize % 4 > 0 ? ( 4 - totalSize % 4) : 0;
	}

	keyRotOffsets[keyRot] = totalSize;

	int startCalculated = totalSize;

	totalSize += pFormats.size() * sizeof(uint32) + sizesPos.size() * sizeof(uint16);
	totalSize += ktFormats.size() * sizeof(uint32) + sizesKeyTimes.size() * sizeof(uint16);
	totalSize += rFormats.size() * sizeof(uint32) + sizesRot.size() * sizeof(uint16);

	totalSize += (keyRot + keyPos + keyTimes + 1) * sizeof(uint32);

	startCalculated = totalSize - startCalculated;
	startCalculated += startCalculated% 4 ? 4 - startCalculated % 4: 0;
	// count size for storing 
	uint32 totalAnims = m_arrGlobalAnimationHeaderCAF.size();
	m_arrStartDirs.resize(m_arrGlobalAnimationHeaderCAF.size());

	for (uint32 i = 0; i < totalAnims; ++i)
	{

		uint32 numControllers = m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo.size();
		totalSize += sizeof(CStoredSkinningInfo);
		totalSize += m_arrGlobalAnimationHeaderCAF[i].m_FootPlantBits.size() * sizeof(uint8) + sizeof(uint16);

		totalSize += m_arrGlobalAnimationHeaderCAF[i].m_FilePath.size() + sizeof(uint16);
		// counter for ControllerInfo
		totalSize += sizeof(uint16) + numControllers * sizeof(CControllerInfo);
	}

	// add real data
	storage.resize(totalSize + 4*sizeof(int));
	uint32 currentPointer = 0;

	// keytime sizes

	std::vector<uint16> tmp(sizesKeyTimes.size());

	for (uint32 i = 0; i < sizesKeyTimes.size(); ++i)
	{
		tmp[ktRemap[i]] = sizesKeyTimes[i];
	}

	SWAP	SwapEndianness(&tmp[0], tmp.size());
	memcpy(&storage[currentPointer], &tmp[0], sizesKeyTimes.size() * sizeof(uint16));
	currentPointer += sizesKeyTimes.size() * sizeof(uint16);
	SWAP	SwapEndianness(&ktFormats[0], ktFormats.size());
	memcpy(&storage[currentPointer], &ktFormats[0], ktFormats.size() * sizeof(uint32));
	currentPointer += ktFormats.size() * sizeof(uint32);

	tmp.resize(sizesPos.size());

	for (uint32 i = 0; i < sizesPos.size(); ++i)
	{
		tmp[pRemap[i]] = sizesPos[i];
	}

	// pos sizes
	SWAP	SwapEndianness(&tmp[0], tmp.size());
	memcpy(&storage[currentPointer], &tmp[0], sizesPos.size() * sizeof(uint16));
	currentPointer += sizesPos.size() * sizeof(uint16);
	SWAP	SwapEndianness(&pFormats[0], pFormats.size());
	memcpy(&storage[currentPointer], &pFormats[0], pFormats.size()* sizeof(uint32));
	currentPointer += pFormats.size() * sizeof(uint32);


	tmp.resize(sizesRot.size());

	for (uint32 i = 0; i < sizesRot.size(); ++i)
	{
		tmp[rRemap[i]] = sizesRot[i];
	}

	SWAP	SwapEndianness(&tmp[0], tmp.size());
	memcpy(&storage[currentPointer], &tmp[0], sizesRot.size() * sizeof(uint16));
	currentPointer += sizesRot.size() * sizeof(uint16);

	SWAP	SwapEndianness(&rFormats[0], rFormats.size());
	memcpy(&storage[currentPointer], &rFormats[0], rFormats.size() * sizeof(uint32));
	currentPointer += rFormats.size() * sizeof(uint32);



	SWAP SwapEndianness(&keyTimeOffsets[0], keyTimeOffsets.size());
	memcpy(&storage[currentPointer], &keyTimeOffsets[0], keyTimeOffsets.size() * sizeof(uint32));
	currentPointer += keyTimeOffsets.size() * sizeof(uint32);

	SWAP SwapEndianness(&keyPosOffsets[0], keyPosOffsets.size());
	memcpy(&storage[currentPointer], &keyPosOffsets[0], keyPosOffsets.size() * sizeof(uint32));
	currentPointer += keyPosOffsets.size() * sizeof(uint32);

//	keyRotOffsets[keyRot] += currentPointer % 4;
	SWAP SwapEndianness(&keyRotOffsets[0], keyRotOffsets.size());
	memcpy(&storage[currentPointer], &keyRotOffsets[0], keyRotOffsets.size() * sizeof(uint32));
	currentPointer += keyRotOffsets.size() * sizeof(uint32);


	int realPointer = currentPointer - 4;

	currentPointer += currentPointer % 4 > 0 ? ( 4 - currentPointer % 4) : 0;

#ifdef DEBUG
	if (currentPointer != startCalculated)
		__debugbreak();
#endif

	uint32 startControllers(currentPointer);
	// copy raw data
	SWAP SwapEndianness(&keyTimeOffsets[0], keyTimeOffsets.size());
	SWAP SwapEndianness(&keyPosOffsets[0], keyPosOffsets.size());
	SWAP SwapEndianness(&keyRotOffsets[0], keyRotOffsets.size());



	for(uint32 kk = 0; kk < keyTimes; ++kk)
	{
		uint32 k = ktRevRemap[kk];
		SWAP m_arrKeyTimes[k]->SwapBytes();
		memcpy(&storage[currentPointer], m_arrKeyTimes[k]->GetData(), m_arrKeyTimes[k]->GetDataRawSize());
		// 
		//if ( keyTimeOffsets[kk] != (currentPointer - startControllers)) {
		//	printf("ooooops kk=%i diff % i, keyTimeOffsets[kk]=%i, currentPointer=%i", kk,  currentPointer - startControllers, keyTimeOffsets[kk], (currentPointer - startControllers));
		//	}

		currentPointer += m_arrKeyTimes[k]->GetDataRawSize();
		currentPointer += currentPointer % 4 > 0 ? ( 4 - currentPointer % 4) : 0;
	}


	// copy raw data

	for(uint32 pp = 0; pp < keyPos; ++pp)
	{
		uint32 p = pRevRemap[pp];
		SWAP m_arrPositionTracks[p]->SwapBytes();
		if (currentPointer % 4 != 0) {
			int a = 0;
		}
		memcpy(&storage[currentPointer], m_arrPositionTracks[p]->GetData(), m_arrPositionTracks[p]->GetDataRawSize());
		//if ( keyPosOffsets[pp] != (currentPointer - startControllers)) {
		//	printf("ooooops pp=%i diff % i, keyPosOffsets[pp]=%i, currentPointer=%i", pp,  currentPointer - startControllers, keyPosOffsets[pp], currentPointer);
		//	}
		currentPointer += m_arrPositionTracks[p]->GetDataRawSize();
		currentPointer += currentPointer % 4 > 0 ? ( 4 - currentPointer % 4) : 0;
	}


	// copy raw data

	for(uint32 rr = 0; rr < keyRot; ++rr)
	{
		uint32 r = rRevRemap[rr];
		SWAP m_arrRotationTracks[r]->SwapBytes();
		if (currentPointer % 4 != 0) {
			int a = 0;
		}

		memcpy(&storage[currentPointer], m_arrRotationTracks[r]->GetData(),m_arrRotationTracks[r]->GetDataRawSize());

		//if ( keyRotOffsets[rr] != (currentPointer - startControllers)) {
		//	printf("ooooops rr=%i diff % i, keyRotOffsets[rr]=%i, currentPointer=%i", rr,  currentPointer - startControllers, keyRotOffsets[rr], currentPointer);
		//	}

		currentPointer += m_arrRotationTracks[r]->GetDataRawSize();
		currentPointer += currentPointer % 4 > 0 ? ( 4 - currentPointer % 4) : 0;
	}



	CONTROLLER_CHUNK_DESC_0904 chunk;
	chunk.chdr.ChunkType = ChunkType_Controller;
	chunk.chdr.ChunkVersion = CONTROLLER_CHUNK_DESC_0904::VERSION;
	chunk.numKeyPos = keyPos;
	chunk.numKeyRot = keyRot;
	chunk.numKeyTime = keyTimes;
	chunk.numAnims = totalAnims;

	// Save animations list
	if (keyRotOffsets[keyRot] != currentPointer - startControllers) {
		__debugbreak();
	}

	for (uint32 i = 0; i < totalAnims; ++i)
	{
		CStoredSkinningInfo info;

		uint16 strSize = m_arrGlobalAnimationHeaderCAF[i].m_FilePath.size();
		SWAP	SwapEndianness(strSize);
		memcpy(&storage[currentPointer], &strSize, sizeof(uint16));
		currentPointer += sizeof(uint16);

		info.m_LHeelEnd = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_LHeelEnd;
		SWAP	SwapEndianness(info.m_LHeelEnd);
		info.m_LHeelStart = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_LHeelStart;
		SWAP	SwapEndianness(info.m_LHeelStart);
		info.m_LToe0Start = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_LToe0Start;
		SWAP	SwapEndianness(info.m_LToe0Start);
		info.m_LToe0End = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_LToe0End;
		SWAP	SwapEndianness(info.m_LToe0End);
		info.m_RHeelEnd = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_RHeelEnd;
		SWAP	SwapEndianness(info.m_RHeelEnd);
		info.m_RHeelStart = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_RHeelStart;
		SWAP	SwapEndianness(info.m_RHeelStart);
		info.m_RToe0Start = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_RToe0Start;
		SWAP	SwapEndianness(info.m_RToe0Start);
		info.m_RToe0End = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantVectors.m_RToe0End;
		SWAP	SwapEndianness(info.m_RToe0End);

		info.m_Distance = m_arrGlobalAnimationHeaderCAF[i].m_fDistance;
		SWAP	SwapEndianness(info.m_Distance);
		info.m_Speed = m_arrGlobalAnimationHeaderCAF[i].m_fSpeed;
		SWAP	SwapEndianness(info.m_Speed);
		info.m_Slope = m_arrGlobalAnimationHeaderCAF[i].m_fSlope;
		SWAP	SwapEndianness(info.m_Slope);

		info.m_nAssetFlags = m_arrGlobalAnimationHeaderCAF[i].m_nFlags;
		SWAP	SwapEndianness(info.m_nAssetFlags);

		info.m_nTicksPerFrame	= m_arrGlobalAnimationHeaderCAF[i].m_nTicksPerFrame;
		SWAP	SwapEndianness(info.m_nTicksPerFrame);
		info.m_secsPerTick		= m_arrGlobalAnimationHeaderCAF[i].m_fSecsPerTick;
		SWAP	SwapEndianness(info.m_secsPerTick);
		info.m_nStart					= m_arrGlobalAnimationHeaderCAF[i].m_nStartKey;
		SWAP	SwapEndianness(info.m_nStart);
		info.m_nEnd						= m_arrGlobalAnimationHeaderCAF[i].m_nEndKey;
		SWAP	SwapEndianness(info.m_nEnd);

		m_arrStartDirs[i] = m_arrGlobalAnimationHeaderCAF[i].m_StartLocation2;
		SWAP	SwapEndianness(m_arrStartDirs[i].q.w);
		SWAP	SwapEndianness(m_arrStartDirs[i].q.v.x);
		SWAP	SwapEndianness(m_arrStartDirs[i].q.v.y);
		SWAP	SwapEndianness(m_arrStartDirs[i].q.v.z);
		SWAP	SwapEndianness(m_arrStartDirs[i].t.x);
		SWAP	SwapEndianness(m_arrStartDirs[i].t.y);
		SWAP	SwapEndianness(m_arrStartDirs[i].t.z);

		// fill info

		// Name length

		// name
		memcpy(&storage[currentPointer], m_arrGlobalAnimationHeaderCAF[i].m_FilePath.c_str(),m_arrGlobalAnimationHeaderCAF[i].m_FilePath.size());
		currentPointer += m_arrGlobalAnimationHeaderCAF[i].m_FilePath.size();

		// info
		///!!!!

		memcpy(&storage[currentPointer], &info, sizeof(info));
		currentPointer += sizeof(info);

		// footplants
		uint16 footplans = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantBits.size();
		SWAP	SwapEndianness(footplans);
		memcpy(&storage[currentPointer], &footplans, sizeof(uint16));
		currentPointer += sizeof(uint16);

		footplans = m_arrGlobalAnimationHeaderCAF[i].m_FootPlantBits.size();
		memcpy(&storage[currentPointer], &(m_arrGlobalAnimationHeaderCAF[i].m_FootPlantBits[0]), footplans);
		currentPointer += footplans;

		// 
		uint16 controllerInfo = m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo.size();
		SWAP	SwapEndianness(controllerInfo);
		memcpy(&storage[currentPointer], &controllerInfo, sizeof(uint16));
		currentPointer += sizeof(uint16);
		controllerInfo = m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo.size();

		///!!!!!!!!!

		std::vector<CControllerInfo> infos(m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo.size());
		for (uint32  c =0; c < m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo.size(); ++c)
		{
			if (m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nPosKeyTimeTrack != -1)
				infos[c].m_nPosKeyTimeTrack = ktRemap[m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nPosKeyTimeTrack];
			else
				infos[c].m_nPosKeyTimeTrack = - 1;
			SWAP	SwapEndianness(infos[c].m_nPosKeyTimeTrack);

			if (m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nRotKeyTimeTrack != - 1)
				infos[c].m_nRotKeyTimeTrack = ktRemap[m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nRotKeyTimeTrack];
			else
				infos[c].m_nRotKeyTimeTrack = - 1;
			SWAP	SwapEndianness(infos[c].m_nRotKeyTimeTrack);

			if (m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nPosTrack != -1)
				infos[c].m_nPosTrack = pRemap[m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nPosTrack];
			else
				infos[c].m_nPosTrack = -1;
			SWAP	SwapEndianness(infos[c].m_nPosTrack);

			if (m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nRotTrack != -1)
				infos[c].m_nRotTrack = rRemap[m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nRotTrack];
			else
				infos[c].m_nRotTrack = -1;
			SWAP	SwapEndianness(infos[c].m_nRotTrack);

			infos[c].m_nControllerID = m_arrGlobalAnimationHeaderCAF[i].m_arrControlerInfo[c].m_nControllerID;

			SWAP	SwapEndianness(infos[c].m_nControllerID);

		}

		memcpy(&storage[currentPointer], &(infos[0]), controllerInfo * sizeof(CControllerInfo));
		currentPointer += controllerInfo * sizeof(CControllerInfo);
	}

	saver.SaveControllerDB904(chunk, &storage[0], storage.size());

	CONTROLLER_CHUNK_DESC_0901 timeStepsChunk;
	timeStepsChunk.chdr.ChunkType = ChunkType_Controller;
	timeStepsChunk.chdr.ChunkVersion = CONTROLLER_CHUNK_DESC_0901::VERSION;
	timeStepsChunk.numTimesteps = m_arrAnimTimes.size();

	saver.SaveController901(timeStepsChunk, &m_arrAnimTimes[0], m_arrAnimTimes.size() * sizeof(DWORD));

	CONTROLLER_CHUNK_DESC_0902 startDirsChunk;
	startDirsChunk.chdr.ChunkType = ChunkType_Controller;
	startDirsChunk.chdr.ChunkVersion = CONTROLLER_CHUNK_DESC_0902::VERSION;
	startDirsChunk.numStartDirs = m_arrStartDirs.size();

	saver.SaveController902(startDirsChunk, &m_arrStartDirs[0], m_arrStartDirs.size() * sizeof(QuatT));

	// Create the directory (including intermediate ones), if not already there.
	string windowPath = name; windowPath.replace('/', '\\');
	MakeSureDirectoryPathExists(windowPath.c_str());

	SetFileAttributes( name,FILE_ATTRIBUTE_ARCHIVE );
	chunkFile.Write( name );
}
