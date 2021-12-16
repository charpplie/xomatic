////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   SegmentedWorldMiniMapUpdater.h
//  Version:     v1.00
//  Created:     14/02/2009 by Veli.
//  Compilers:   Visual Studio 2008
//  Description: Utility to work with world map
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SW_MINI_MAP_UPDATER_H__
#define __SW_MINI_MAP_UPDATER_H__
#pragma once

#include "SWFwd.h"

struct TSegStsClr
{
	enum ESegStsClr {
		SSClr_Default = 0,
		SSClr_Purple, //Conflict - modified but not locked by me
		SSClr_Red, //selected is locked by other
		SSClr_Orange, //something is locked by other, nothing is locked by us
		SSClr_Cyan, //something locked by other, something locked by us
		SSClr_Blue, //selected locked and modified by me
		SSClr_Green, //something locked by us
		SSClr_Gray //missing
	};
};

struct TSegmentState
{
	struct TState
	{
		int m_nStatus;
		int m_nLockedBy;
		string m_strUserName;
		string m_strName;

		void SyncLockInfo(struct TLayerData& pLayer);
	};

	struct TLayerState : public TState
	{
		string m_strLayerName;
	};
	enum ESegmentLockState {
		SWSLS_LOCKED_BY_ME = BIT(0),
		SWSLS_LOCKED_BY_OTHER = BIT(1),
		SWSLS_MODIFIED = BIT(2),
		SWSLS_MISSING = BIT(3),
		SWSLS_OUT_OF_DATE = BIT(4),
	};
	int nSegmentID;
	std::map<int,TLayerState> m_mapLayersStatus;
	TState m_SegmentDataStatus;
	TSegmentState() {m_SegmentDataStatus.m_nStatus = 0; m_SegmentDataStatus.m_nLockedBy = 0; m_mapLayersStatus.clear();}
	int GetSelectedState(int nSelectType, int nLayerId);
	bool HasFlag(int eSWSLS);
	int GetSegStsClr(int nSelectType, int nLayerId);
};

struct ISWMiniMapUpdaterCallback
{
	virtual void SendImage(int nSegmentID, CImageEx* pImage) = 0;
};

struct ISWMiniMapLockStatusUpdaterCallback
{
	virtual void SetStatus(std::map<int, DWORD> &stateMap, std::map<int,TSegmentState> &mapSegStates) = 0;
	virtual void SetSegInfoUpdateSuspended(bool bSuspend = true) = 0;
};

struct ISWGDLockStatusUpdaterCallback
{
	virtual void SetGDStatus(int nWDBType, int nWDBState, int nLockedBy, const char* pchLockedBy) = 0;
};

struct TWorldDataState {
	//ISegmentedWorld::EWDBType eWDBType;
	int m_nWDBState;
	int m_nLockedBy;
	string m_strLockedBy;
	string m_strName;
	void SetState(int eWDBState, int nLockedBy, const char* pchLockedBy) { m_nWDBState = eWDBState; m_nLockedBy =  nLockedBy; m_strLockedBy = pchLockedBy; }
};


typedef std::map<int,TSegmentState> TMapSegStates;

struct TSegImg 
{
	enum EImageSizes {
		SIZE_32X = 0,
		SIZE_64X,
		SIZE_128X,
		SIZES_COUNT
	};

	TSegImg();
	~TSegImg();
	int GetNeededSize(int nImgSize);
	
	int m_nSegmentID;
	int64 m_nAppliedHrid;
	int64 m_nHrids[SIZES_COUNT];
	int64 m_nLoadedHrids[SIZES_COUNT];
	CImageEx* m_pImgs[SIZES_COUNT];
};

struct TImgInfo
{
	int m_nSegmentID;
	int nImgSize;
	int64 m_nHrid;
	CImageEx* pImg;
	TImgInfo() {pImg = NULL; m_nHrid = -1;}
	~TImgInfo() {SAFE_DELETE(pImg); m_nHrid = -1;}
};

typedef std::map<int32, TSegImg> TSegImgMap;
typedef std::vector<TImgInfo> TImgInfoMap;


class CSWMiniMapUpdater
{
public:
	enum ELockStatusTypes {
		LOCK_STATUS_TYPE_NOTHING = 0,
		LOCK_STATUS_TYPE_SELECTED_OBJECTLAYER,
		LOCK_STATUS_TYPE_OBJECTLAYERS,
		LOCK_STATUS_TYPE_SEGMENTS,
	};

	static CSWMiniMapUpdater* Get();
	CSWMiniMapUpdater(void);
	~CSWMiniMapUpdater(void);

	//public methods
public:
	static bool Init(CString strWorldName, Recti* prcWorldMap=NULL);
	void SetSWMiniMapUpdaterCallback(ISWMiniMapUpdaterCallback* pSWMiniMapUpdaterCallback);
	void SetSWMiniMapLockStatusUpdaterCallback(ISWMiniMapLockStatusUpdaterCallback* pSWMiniMapLockStatusUpdaterCallback);
	void SetSWGDLockStatusUpdaterCallBack(ISWGDLockStatusUpdaterCallback* pSWGDLockStatusUpdaterCallback);
	void OnSizeChange(int nMPerPIX, bool bUpdate = true);
	void OnWorldRectChange(Recti rcWorldSeg, bool bUpdate = true);
	void Update();

	void TriggerManuallyUpdate();
	bool ReloadModified(const Recti &worldSegRect, TSegImg::EImageSizes eImgSize);

	//protected methods
protected:
	TSegImg* GetSegImg(int nSegmentID, bool bCreate = true);
	CImageEx* GetImage(int nSegmentID, TSegImg::EImageSizes eImgSize);
	
	void SetSize(TSegImg::EImageSizes imgSize);
	void SetSegRect(const Recti &newSegRect);
	bool UpdateStatus(const Recti &worldSegRect, TSegImg::EImageSizes eImgSize);
	bool HasSomethingToLoad(const Recti &worldSegRect, TSegImg::EImageSizes eImgSize);
	void ApplyNeeded(const Recti &worldSegRect, TSegImg::EImageSizes eImgSize);
	void NoLongerApplied();
	bool Load(int nSegmentID, int nImgSize, int64 nHrid);
	bool Reload(int nSegmentID, int nImgSize);
	bool Unload(int nSegmentID, int nImgSize);
	void SetWorldName(const char* pcWorldName);
	

	//members
protected:
	static CSWMiniMapUpdater* s_pThis;
	static std::map<int,string> s_UserNamesCache;
	//std::map<int,string> m_LayersNamesCache;

	TSegImg::EImageSizes m_CurrSize;
	Recti m_CurrSegRect;
	string m_strWorldName;
	sw::ISWDataPersistence* m_pPersist;
	sw::ISWVersionControl* m_tmpVC;	// it's readonly access, null vc is enough

	TSegImgMap m_mapSegImages;
	TImgInfoMap m_mapImagesToApply;
	ISWMiniMapUpdaterCallback* m_pSWMiniMapUpdaterCallback;
	ISWMiniMapLockStatusUpdaterCallback* m_pSWMiniMapLockStatusUpdaterCallback;
	ISWGDLockStatusUpdaterCallback* m_pSWGDLockStatusUpdaterCallback;
	
public:
	static int s_nDBUserID;
	static int s_nRollUpCtrlID;
	static bool s_bSuspend;
	static CryCriticalSection g_cs;
};
//*/

#endif //__SW_MINI_MAP_UPDATER_H__
