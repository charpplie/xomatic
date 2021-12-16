////////////////////////////////////////////////////////////////////////////
//
// Crytek Engine Source File.
// Copyright (C), Crytek Studios, 2001-2006.
// -------------------------------------------------------------------------
// File name: TrackStorage.h
// Version: v1.00
// Created: 22/8/2006 by Alexey Medvedev.
// Compilers: Visual Studio.NET 2005
// Description: Storage for database of tracks.
// -------------------------------------------------------------------------
// History:
//
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "AnimationLoader.h"
#include "..\CGF\LoaderDBA.h"
#include "ResourceCompiler.h"

struct DBStatistics
{
	DBStatistics() : m_iNumAnimations(0), m_iNumControllers(0), m_iNumKeyTimes(0), m_iNumPositions(0), m_iNumRotations(0), m_iSavedBytes(0) {}
	int m_iNumAnimations;

	int m_iNumControllers;

	int m_iNumKeyTimes;
	int m_iNumPositions;
	int m_iNumRotations;


	int m_iSavedBytes;

};

struct GlobalAnimationHeaderCAF;

class CTrackStorage
{
public:
	CTrackStorage(void);
	~CTrackStorage(void);

	bool GetAnimFileTime(const string&  name, DWORD * ft);

	bool LoadDataBase(const char * name);

	void AddAnimation(const string& strFilePathDBA, GlobalAnimationHeaderCAF& header, string& name, bool bAddToDB, DWORD timeStamp);
	// Main difference - we dont search tracks. Just only add new tracks and override settings. Fastest method.
	void UpdateAnimation(const string& strFilePathDBA, GlobalAnimationHeaderCAF& header, string& name, bool bAddToDB, DWORD timeStamp, bool bFast);

	void Analize(uint32& TrackShader, uint32& SizeDataShared, uint32& TotalTracks, uint32& TotalMemory, CSkeletonInfo * currentSkeleton, ConvertContext& cc);

	void Clear()
	{
		m_arrGlobalAnimationHeaderCAF.clear();
		m_arrAnimNames.clear();
		m_arrKeyTimes.clear();
		m_arrRotationTracks.clear();
		m_arrPositionTracks.clear();
		m_arrKeyTimesRemap.clear();
		m_arrKeyPosRemap.clear();
		m_arrKeyRotRemap.clear();

	}

	uint32  FindKeyTimesTrack(KeyTimesInformationPtr& pKeyTimes)
	{
		//uint32 numKeyTimes = m_arrKeyTimes.size(); 

		//for (uint32 k = 0; k < numKeyTimes; ++k)
		//{
		//	if (IsKeyTimesIdentical(pKeyTimes, m_arrKeyTimes[k]))
		//	{
		//		return k;
		//	}
		//}

		TVectorRemap::iterator it =  m_arrKeyTimesRemap.find(pKeyTimes->GetNumKeys());

		if (it != m_arrKeyTimesRemap.end()) {
			for (;it != m_arrKeyTimesRemap.end(); ++it) {
				if (IsKeyTimesIdentical(pKeyTimes, m_arrKeyTimes[it->second]))
				{
					return it->second;
				}
			}
		}
		return -1;
	}

	uint32  FindPositionTrack(PositionControllerPtr& pKeys)
	{

		TVectorRemap::iterator it =  m_arrKeyPosRemap.find(pKeys->GetNumKeys());

		if (it != m_arrKeyPosRemap.end()) {
			for (;it != m_arrKeyPosRemap.end(); ++it) {
				if (IsPositionIdentical(pKeys, m_arrPositionTracks[it->second]))
				{
					return it->second;
				}
			}
		}

		//uint32 numKeys = m_arrPositionTracks.size(); 

		//for (uint32 k = 0; k < numKeys; ++k)
		//{
		//	if (IsPositionIdentical(pKeys, m_arrPositionTracks[k]))
		//	{
		//		return k;
		//	}
		//}
		return -1;
	}

	uint32  FindRotationTrack(RotationControllerPtr& pKeys)
	{
		TVectorRemap::iterator it =  m_arrKeyRotRemap.find(pKeys->GetNumKeys());

		if (it != m_arrKeyRotRemap.end()) {
			for (;it != m_arrKeyRotRemap.end(); ++it) {
				if (IsRotationIdentical(pKeys, m_arrRotationTracks[it->second]))
				{
					return it->second;
				}
			}
		}

		//uint32 numKeys = m_arrRotationTracks.size(); 
		//for (uint32 k = 0; k < numKeys; ++k)
		//{
		//	if (IsRotationIdentical(pKeys, m_arrRotationTracks[k]))
		//	{
		//		return k;
		//	}
		//}
		return -1;
	}

	void SaveDataBase903(const char * name);
	void SaveDataBase904(const char * name);

	const DBStatistics& GetStatistics()
	{
		return m_Statistics;
	}

protected:
	bool IsRotationIdentical(const RotationControllerPtr& track1, const TrackRotationStoragePtr&  track2);
	bool IsPositionIdentical(const PositionControllerPtr& track1, const TrackPositionStoragePtr& track2);
	bool IsKeyTimesIdentical(const KeyTimesInformationPtr& track1, const KeyTimesInformationPtr& track2);
	// Add single animation to db
	void AddAnimationToDatabase(const string& strFilePathDBA, uint32 num, bool bFast);
	// update anim information in db
	void UpdateAnimationInDatabase(const string& strFilePathDBA, GlobalAnimationHeaderCAF& header, uint32 num, bool bFast);
	// get number animation from m_arrAnimations. If anim doesnt exist - new num created;
	uint32 GetNumAnimations(GlobalAnimationHeaderCAF& header, string& name, DWORD timeStamp);
	//
	uint32 FindAnimation(const string& name);

	void AnalizeKeyTimes();

	void CreateBitsetKeyTimes(int k);
	void CreateStartStopKeyTimes(int k);
public:
	std::vector<GlobalAnimationHeaderCAF> m_arrGlobalAnimationHeaderCAF;
	TNamesVector m_arrAnimNames;
	TFileTimeVector m_arrAnimTimes;
	std::vector<QuatT> m_arrStartDirs;
	DynArray<KeyTimesInformationPtr> m_arrKeyTimes;
	DynArray<TrackRotationStoragePtr> m_arrRotationTracks;
	DynArray<TrackPositionStoragePtr> m_arrPositionTracks;

	typedef std::multimap<int, int> TVectorRemap;

	TVectorRemap m_arrKeyTimesRemap;
	TVectorRemap m_arrKeyPosRemap;
	TVectorRemap m_arrKeyRotRemap;

	//int m_iSavedBytes;
	DBStatistics m_Statistics;
	CryCriticalSectionRC m_lock;
};
