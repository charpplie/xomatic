#include "StdAfx.h"
#include "InternalCommon.h"
#include "SegmentedWorldMiniMapUpdater.h"
//#include "DBWrapper.h"
#include "SCDataPersistence.h"
#include "NullVersionControl.h"
#include "SegmentedWorldManager.h"
#include "SegmentedWorldDoc.h"
#include "LayerData.h"
#include "SegmentData.h"

#include "CryMemBlock.h"




using namespace sw;

CryCriticalSection CSWMiniMapUpdater::g_cs;


//############################################################################################
// TSegImg
//-------------------------------------------------------------------------------------
TSegImg::TSegImg()
{ 
	for (int i = 0; i < (int)(SIZES_COUNT); i++)
	{
		m_nHrids[i] = -1;
		m_nLoadedHrids[i] = -1;
		m_pImgs[i]=NULL;
	}
	m_nAppliedHrid = -1;
	m_nSegmentID = 0; 
}
//-------------------------------------------------------------------------------------
TSegImg::~TSegImg()
{
	for (int i = 0; i < (int)(SIZES_COUNT); i++)
		SAFE_DELETE(m_pImgs[i]);
}
//-------------------------------------------------------------------------------------
int TSegImg::GetNeededSize(int nImgSize)
{
	if (m_nLoadedHrids[nImgSize] != -1 && m_pImgs[nImgSize])
		return nImgSize;
	for (int i = 0; i < SIZES_COUNT; i++)
	{
		if (m_nLoadedHrids[i] != -1 && m_pImgs[i])
			return i;
	}
	return nImgSize;
}
//########################################################################################
//TSegmentState
//-------------------------------------------------------------------------------------
int TSegmentState::GetSelectedState(int nSelectType, int nLayerId)
{
	if (nSelectType == CSWMiniMapUpdater::LOCK_STATUS_TYPE_SEGMENTS)
	{
		return m_SegmentDataStatus.m_nStatus;
	}

	if (nSelectType == CSWMiniMapUpdater::LOCK_STATUS_TYPE_OBJECTLAYERS || nSelectType == CSWMiniMapUpdater::LOCK_STATUS_TYPE_SELECTED_OBJECTLAYER)
	{
		std::map<int,TLayerState>::iterator it = m_mapLayersStatus.find(nLayerId);
		if (it == m_mapLayersStatus.end())
			return TSegmentState::SWSLS_MISSING;
		return it->second.m_nStatus;
	}
	return TSegmentState::SWSLS_MISSING;
}
//-------------------------------------------------------------------------------------
bool TSegmentState::HasFlag(int eSWSLS)
{
	if((m_SegmentDataStatus.m_nStatus&eSWSLS) != 0)
		return true;

	for (std::map<int,TLayerState>::iterator it = m_mapLayersStatus.begin(); it != m_mapLayersStatus.end(); ++it)
	{
		int nLayerStatus = it->second.m_nStatus;
		if((nLayerStatus&eSWSLS) != 0)
			return true;
	}
	return false;
}
//-------------------------------------------------------------------------------------
int TSegmentState::GetSegStsClr(int nSelectType, int nLayerId)
{

	//PURPLE
	//segment
	if (m_SegmentDataStatus.m_nStatus&TSegmentState::SWSLS_OUT_OF_DATE)
		return TSegStsClr::SSClr_Purple;

	if (GetIEditor()->GetSegmentedWorldManager()->IsOfflineMode())
	{
		if (
			(m_SegmentDataStatus.m_nStatus&TSegmentState::SWSLS_MODIFIED) != 0 && 
			(m_SegmentDataStatus.m_nStatus&TSegmentState::SWSLS_LOCKED_BY_OTHER) != 0
			)
		{
			return TSegStsClr::SSClr_Purple;
		}
	}
	else
	{
		if (
			(m_SegmentDataStatus.m_nStatus&TSegmentState::SWSLS_MODIFIED) != 0 && 
			(m_SegmentDataStatus.m_nStatus&TSegmentState::SWSLS_LOCKED_BY_ME) == 0
			)
		{
			return TSegStsClr::SSClr_Purple;
		}
	}
	

	//object layers
	for (std::map<int,TLayerState>::iterator it2 = m_mapLayersStatus.begin(); it2 != m_mapLayersStatus.end(); ++it2)
	{
		int nLayerStatus = it2->second.m_nStatus;

		if (nLayerStatus&TSegmentState::SWSLS_OUT_OF_DATE)
			return TSegStsClr::SSClr_Purple;

		if (GetIEditor()->GetSegmentedWorldManager()->IsOfflineMode())
		{
			if (
				(nLayerStatus&TSegmentState::SWSLS_MODIFIED) != 0 && 
				(nLayerStatus&TSegmentState::SWSLS_LOCKED_BY_OTHER) != 0
				)
			{
				return TSegStsClr::SSClr_Purple;
			}
		}
		else
		{
			if (
				(nLayerStatus&TSegmentState::SWSLS_MODIFIED) != 0 && 
				(nLayerStatus&TSegmentState::SWSLS_LOCKED_BY_ME) == 0
				)
			{
				return TSegStsClr::SSClr_Purple;
			}
		}
	}

//RED
	int nSelectedLockStatus = GetSelectedState(nSelectType, nLayerId);
	{
		if (
			(nSelectedLockStatus&TSegmentState::SWSLS_LOCKED_BY_OTHER) != 0
			)
		{
			return TSegStsClr::SSClr_Red;
		}
	}

//ORANGE and CYAN
	if (HasFlag(TSegmentState::SWSLS_LOCKED_BY_OTHER))
	{
		if(HasFlag(TSegmentState::SWSLS_LOCKED_BY_ME) || (GetIEditor()->GetSegmentedWorldManager()->IsOfflineMode() && HasFlag(TSegmentState::SWSLS_MODIFIED)))
			return TSegStsClr::SSClr_Cyan;
		else
			return TSegStsClr::SSClr_Orange;
	}

//BLUE
	if (HasFlag(TSegmentState::SWSLS_MODIFIED))
		return TSegStsClr::SSClr_Blue;

//GREEN
	if (HasFlag(TSegmentState::SWSLS_LOCKED_BY_ME))
		return TSegStsClr::SSClr_Green;
//GRAY
	if ((nSelectedLockStatus&TSegmentState::SWSLS_MISSING) != 0)
		return TSegStsClr::SSClr_Gray;

	return TSegStsClr::SSClr_Default;
}
//########################################################################################
//updatethread
//-------------------------------------------------------------------------------------
struct TSWMapUpdaterThread : public CrySimpleThread<>, public IEditorNotifyListener {
	static TSWMapUpdaterThread *Get()
	{
		static TSWMapUpdaterThread *s_pThis = 0;
		if (!s_pThis)
			s_pThis = new TSWMapUpdaterThread();
		return s_pThis;
	}

	TSWMapUpdaterThread()
	{
		GetIEditor()->RegisterNotifyListener(this);
		Start();
	}

	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event )
	{
		if (event != eNotify_OnIdleUpdate)
			return;
		//if (CSWMiniMapUpdater::Get())
		//	CSWMiniMapUpdater::Get()->Update();
	}

	virtual void Run()
	{
		//gEnv->pDB->GetConnector("mysql")->InitThread();

		while (!GetISystem()->IsQuitting()) {
#ifndef SW_NO_REALTIME_UPDATE
			CSWMiniMapUpdater *pSW_MMU;
			pSW_MMU = CSWMiniMapUpdater::Get();
			if (pSW_MMU && !CSWMiniMapUpdater::s_bSuspend)
				pSW_MMU->Update();
#endif
			Sleep(1000);
		}

		//gEnv->pDB->GetConnector("mysql")->EndThread();
	}
};
//-------------------------------------------------------------------------------------
//############################################################################################
//CSWMiniMapUpdater
//-------------------------------------------------------------------------------------
CSWMiniMapUpdater* CSWMiniMapUpdater::s_pThis = NULL;
bool CSWMiniMapUpdater::s_bSuspend = false;
//-------------------------------------------------------------------------------------
int CSWMiniMapUpdater::s_nRollUpCtrlID = 0;
int CSWMiniMapUpdater::s_nDBUserID = -1;


std::map<int,string> CreateMap()
{
	std::map<int,string> m;
	m.clear();
	return m;
}
std::map<int,string> CSWMiniMapUpdater::s_UserNamesCache = CreateMap();
//-------------------------------------------------------------------------------------
CSWMiniMapUpdater* CSWMiniMapUpdater::Get()
{	
	return CSWMiniMapUpdater::s_pThis;
}


CSWMiniMapUpdater::CSWMiniMapUpdater(void)
{
	m_mapSegImages.clear();
	m_mapImagesToApply.clear();
	m_pSWMiniMapUpdaterCallback = NULL;
	m_pSWMiniMapLockStatusUpdaterCallback = NULL;
	m_pSWGDLockStatusUpdaterCallback = NULL;
	//m_vt = VT_CURRENT;
	m_CurrSize = TSegImg::SIZE_32X;
	m_CurrSegRect = Recti(0, 0, 16, 16);
	//m_LayersNamesCache.clear();

	m_tmpVC = new CNullVersionControl();
	//m_pPersist = new CSCDataPersistence( m_tmpVC );
	m_pPersist = NULL;
}
//-------------------------------------------------------------------------------------
CSWMiniMapUpdater::~CSWMiniMapUpdater(void)
{
	SAFE_DELETE(m_pPersist);
	//DestroyDBConn();
	//m_pConn = NULL;
	m_pSWMiniMapUpdaterCallback = NULL;
	m_pSWMiniMapLockStatusUpdaterCallback = NULL;
	m_pSWGDLockStatusUpdaterCallback = NULL;
	m_mapSegImages.clear();
	m_mapImagesToApply.clear();
	//m_strDBName.clear();
	//m_vt = -1;
	m_CurrSize = TSegImg::SIZE_128X;
	//m_LayersNamesCache.clear();
}
//-------------------------------------------------------------------------------------
bool CSWMiniMapUpdater::Init(CString strWorldName, Recti* prcWorldMap)
{
	s_bSuspend = true;
	AUTO_LOCK_MMUP(g_cs);
	TSegImg::EImageSizes imgSize = TSegImg::SIZE_128X;
	if (s_pThis)
	{
		//imgSize = s_pThis->m_CurrSize;
		//if (strWorldName.IsEmpty() || strcmpi(s_pThis->m_strDBName.c_str(), strWorldName))
		{
			delete s_pThis;
			s_pThis = NULL;
		}
	}	

	if (strWorldName.IsEmpty())
		return false;

	if (!CSWMiniMapUpdater::s_pThis)
		CSWMiniMapUpdater::s_pThis = new CSWMiniMapUpdater();
	//if (!Get()->CreateDBConn(strWorldName))
	//{
	//	SAFE_DELETE(CSWMiniMapUpdater::s_pThis);
	//	return false;
	//}
	
	//if (prcWorldMap != NULL)
	//{
	//	Recti newSegRect;
	//	WorldToSegRect(*prcWorldMap, newSegRect);
	//	//Get()->SetSegRect(newSegRect);
	//}
	Get()->SetSize(imgSize);
	Get()->SetWorldName(strWorldName);
	//Get()->InitDBUserID();
	
	TSWMapUpdaterThread::Get();
	s_bSuspend = false;
	return true;
}
//-------------------------------------------------------------------------------------
void CSWMiniMapUpdater::SetSize(TSegImg::EImageSizes imgSize)
{	
	m_CurrSize = imgSize;
}
//-------------------------------------------------------------------------------------
void CSWMiniMapUpdater::SetSegRect(const Recti &newSegRect)
{
	m_CurrSegRect = newSegRect;
}

//-------------------------------------------------------------------------------------
void CSWMiniMapUpdater::SetSWMiniMapUpdaterCallback(ISWMiniMapUpdaterCallback* pSWMiniMapUpdaterCallback)
{
	//CRect rc = m_CurrSegRect;
	//if (m_pSWMiniMapUpdaterCallback != pSWMiniMapUpdaterCallback)
	//	NoLongerApplied();
	m_pSWMiniMapUpdaterCallback = pSWMiniMapUpdaterCallback;
	//if(m_pSWMiniMapUpdaterCallback != NULL)
	//	ApplyNeeded(rc, m_CurrSize);
}
//-------------------------------------------------------------------------------------
void CSWMiniMapUpdater::SetSWMiniMapLockStatusUpdaterCallback(ISWMiniMapLockStatusUpdaterCallback* pSWMiniMapLockStatusUpdaterCallback)
{
	m_pSWMiniMapLockStatusUpdaterCallback = pSWMiniMapLockStatusUpdaterCallback;
}
//-------------------------------------------------------------------------------------
void CSWMiniMapUpdater::SetSWGDLockStatusUpdaterCallBack(ISWGDLockStatusUpdaterCallback* pSWGDLockStatusUpdaterCallback)
{
	m_pSWGDLockStatusUpdaterCallback = pSWGDLockStatusUpdaterCallback;
}
////-------------------------------------------------------------------------------------
TSegImg* CSWMiniMapUpdater::GetSegImg(int nSegmentID, bool bCreate)
{
	TSegImgMap::iterator it = m_mapSegImages.find(nSegmentID);
	if (it != m_mapSegImages.end())
		return &(it->second);
	if (!bCreate)
		return NULL;
	TSegImg &si = m_mapSegImages[nSegmentID];
	si.m_nSegmentID = nSegmentID;
	return &si;
}
//-------------------------------------------------------------------------------------
CImageEx* CSWMiniMapUpdater::GetImage(int nSegmentID, TSegImg::EImageSizes eImgSize)
{
	TSegImg* si = GetSegImg(nSegmentID);
	if (si == NULL)
		return NULL;

	return si->m_pImgs[eImgSize];
}
//-------------------------------------------------------------------------------------
void CSWMiniMapUpdater::Update()
{
	//AUTO_LOCK_MMUP(g_cs);
	if (s_bSuspend)
		return;

	if (!m_pSWMiniMapUpdaterCallback && 
			!m_pSWMiniMapLockStatusUpdaterCallback && 
			!m_pSWGDLockStatusUpdaterCallback)
		return;
	Recti rect = m_CurrSegRect;
	TSegImg::EImageSizes imgSize = m_CurrSize;

	//if (m_pSWMiniMapLockStatusUpdaterCallback)
	//	UpdateLockStatus(GetDBConn(), rect);

	if (s_bSuspend)
		return;

	//if (m_pSWGDLockStatusUpdaterCallback)
	//	UpdateGDLockStatus(GetDBConn());

	if (s_bSuspend)
		return;

	if (m_pSWMiniMapUpdaterCallback)
	{

		if (!UpdateStatus(rect, imgSize))
			return; // data is not ready

		if (s_bSuspend)
			return;
		if (m_pSWMiniMapLockStatusUpdaterCallback && HasSomethingToLoad(rect, imgSize))
		{
			m_pSWMiniMapLockStatusUpdaterCallback->SetSegInfoUpdateSuspended(true);
		}

		ReloadModified(rect, imgSize);
		if (s_bSuspend)
			return;
		ApplyNeeded(rect, imgSize);
		if (m_pSWMiniMapLockStatusUpdaterCallback)
		{
			m_pSWMiniMapLockStatusUpdaterCallback->SetSegInfoUpdateSuspended(false);
		}
	}
	//TODO
}
//-------------------------------------------------------------------------------------
void CSWMiniMapUpdater::OnSizeChange(int nMPerPIX, bool bUpdate)
{
	TSegImg::EImageSizes imgSize = m_CurrSize;
	if (nMPerPIX > 40)
		imgSize = TSegImg::SIZE_128X;
	else if (nMPerPIX > 20)
		imgSize = TSegImg::SIZE_64X;
	else
		imgSize = TSegImg::SIZE_32X;
	
  //CLAMP(imgSize, (int)(TSegImg::SIZE_32X), (int)(TSegImg::SIZE_128X));
	if (m_CurrSize != imgSize)
	{
		NoLongerApplied();
		bUpdate = true;
	}
	
	SetSize(imgSize);
	if (bUpdate)
		Update();
}
//-------------------------------------------------------------------------------------
void CSWMiniMapUpdater::OnWorldRectChange(Recti rcWorldSeg, bool bUpdate)
{
	//Recti newSegRect;
	//WorldToSegRect(rcWorldMt, newSegRect);

	//if(m_CurrSegRect != newSegRect)
	{
		SetSegRect(rcWorldSeg);
	} //else
   // return;

  //if (bUpdate)
	//  Update();
 }
//-------------------------------------------------------------------------------------
bool CSWMiniMapUpdater::UpdateStatus(const Recti &worldSegRect, TSegImg::EImageSizes eImgSize)
{
	AUTO_LOCK_MMUP(g_cs);
	if (this != CSWMiniMapUpdater::Get())
		return false;
	int nImgSize = (int)(eImgSize);
	
	bool bAnyMissing = false;

	auto psw = GetInternalSWDoc();
	if (psw)
	{
		CSegmentDataAggr* segs = NULL;
		segs = &psw->SegmentAggr();
		std::vector<TSegDataBlock*> blocks;
		segs->FetchSegDataBlocks( blocks, sw::ESDBType(SDB_Map+nImgSize), worldSegRect );
		for (std::vector<TSegDataBlock*>::iterator iter = blocks.begin(); iter != blocks.end(); ++iter)
		{
			TSegDataBlock* pDB = *iter;
			TSegImg* psi = GetSegImg( MakeSegmentID(pDB->GetParent()->m_wc) );
			assert(psi);
			int64 nHeadRev = pDB->GetVersion();
			if (pDB->IsMissingInPersist())
			{
				nHeadRev = HRID_MISSING; // even if it has a revID it still can be missing
				bAnyMissing = true;
			}

			psi->m_nHrids[nImgSize] = nHeadRev;
		}
	}
	else
	{
		for (int x = worldSegRect.Min.x; x < worldSegRect.Max.x; ++x)
			for (int y = worldSegRect.Min.y; y < worldSegRect.Max.y; ++y)
			{
				TSegImg* psi = GetSegImg( MakeSegmentID(x,y) );
				assert(psi);

				psi->m_nHrids[nImgSize] = HRID_HEADREV;
			}
	}
		
	return !bAnyMissing;
}
//-------------------------------------------------------------------------------------
bool CSWMiniMapUpdater::ReloadModified(const Recti &worldSegRect, TSegImg::EImageSizes eImgSize)
{
	//AUTO_LOCK_MMUP(g_cs);
	int nImgSize = (int)eImgSize;
#ifndef _RELEASE
	char szLogt[MAX_PATH];
	sprintf(szLogt, "SegRect: [%d %d %d %d]\n", worldSegRect.Min.y, worldSegRect.Max.y, worldSegRect.Min.x, worldSegRect.Max.x);
	OutputDebugString(szLogt);
#endif

	for (int wy = worldSegRect.Min.y; wy < worldSegRect.Max.y; wy++)
		for (int wx = worldSegRect.Min.x; wx < worldSegRect.Max.x; wx++)
		{
			if (s_bSuspend)
				return true;
			AUTO_LOCK_MMUP(g_cs);
			if (this != CSWMiniMapUpdater::Get())
				return false;
			int nSegmentId = MakeSegmentID(wx, wy);
			TSegImg* psi = GetSegImg(nSegmentId, false);
			if (!psi)
				continue;

			bool bReload = false;
			if (psi->m_nHrids[nImgSize] != HRID_MISSING && psi->m_nHrids[nImgSize] != psi->m_nLoadedHrids[nImgSize])
			{
				Load(nSegmentId, nImgSize, psi->m_nHrids[nImgSize]);
			}

		}
	return true;
}
//-------------------------------------------------------------------------------------
bool CSWMiniMapUpdater::HasSomethingToLoad(const Recti &worldSegRect, TSegImg::EImageSizes eImgSize)
{
	int nImgSize = (int)eImgSize;
	for (int wy = worldSegRect.Min.y; wy < worldSegRect.Max.y; wy++)
		for (int wx = worldSegRect.Min.x; wx < worldSegRect.Max.x; wx++)
		{
			if (s_bSuspend)
				return true;
			AUTO_LOCK_MMUP(g_cs);
			if (this != CSWMiniMapUpdater::Get())
				return false;
			int nSegmentId = MakeSegmentID(wx, wy);
			TSegImg* psi = GetSegImg(nSegmentId, false);
			if (!psi)
				continue;

			bool bReload = false;
			if (psi->m_nHrids[nImgSize] != psi->m_nLoadedHrids[nImgSize])
			{
				return true;
			}
		}
		return false;
}
////-------------------------------------------------------------------------------------
bool CSWMiniMapUpdater::Load(int nSegmentID, int nImgSize, int64 nHrid)
{
	TSegImg* psi = GetSegImg(nSegmentID, false);
	if (!psi)
		return false;
	//if (nHrid == psi->m_nLoadedHrids[nImgSize])
	//	return true;
	//if (psi->m_nLoadedHrids[nImgSize] != -1)
	//{
	//if(!Unload(nSegmentID, nImgSize))
	//	assert(0);
	//}
	if (nHrid == 0)
	{
		psi->m_nLoadedHrids[nImgSize] = 0;
		return true;
	}

	TSegDataBlock* pDB = NULL;
	auto psw = GetInternalSWDoc();
	if (psw)
	{
		CSegmentDataAggr* pSegs = NULL;
		pSegs = &psw->SegmentAggr();
		TSegmentData* pSeg = pSegs->GetSegmentData( nSegmentID, false );
		pDB = (!pSeg)? NULL : pSeg->GetBlock( ESDBType(SDB_Map + nImgSize), false);

		if (pDB)
		{
			assert(nHrid > 0 && nHrid <= pDB->GetVersion());

			if (!pDB->Load(*psw->GetPersistence(),nHrid))
			{
				if (pDB->IsDataEmpty() && nHrid == 1) // this must be an empty entry just created, it should be treated as missing
				{
					psi->m_nHrids[nImgSize] = HRID_MISSING;
					pDB->GoToState(Versioned::S_Init);
					return false;
				}

				assert(0);
			}
		}
	}
	else
	{
		CSegmentDataAggr* pSegs = NULL;
		pSegs = new CSegmentDataAggr(0,0,0);

		TWorldCoords wc = FromSegmentID( nSegmentID);
		

		pDB = pSegs->LoadIndependentDataBlock( *m_pPersist, wc.wx, wc.wy, (ESDBType)(SDB_Map + nImgSize) );
		if (!pDB || pDB->IsDataEmpty() )
		{
			psi->m_nHrids[nImgSize] = HRID_MISSING;
		}
	}

	if (!pDB)
		return false;
		
	assert(!pDB->IsDataEmpty());

	CImageEx* img = new CImageEx();	// new it because we need to send it to others
	switch (nImgSize)
	{
	case 0:
		img->Allocate(32,32);
		break;
	case 1:
		img->Allocate(64,64);
		break;
	default:
		assert(nImgSize == 2);
		img->Allocate(128,128);
	}
	img->Uncompress(pDB->GetMemoryBlock());

	int wx;
	int wy;
	GetWorldCoords(nSegmentID, wx, wy);
#ifndef _RELEASE
	char szLog[MAX_PATH];
	sprintf(szLog, "Map for seg:%d ImgSize:%d [%d,%d] loaded\n", nSegmentID, nImgSize, wx, wy);
	OutputDebugString(szLog);
#endif
	//if (nImgSize == (int)m_CurrSize)
	{
		AUTO_LOCK_MMUP(g_cs);
		//if (!m_pSWMiniMapUpdaterCallback)
		//	return false;
		assert(m_pSWMiniMapUpdaterCallback);
		m_pSWMiniMapUpdaterCallback->SendImage(nSegmentID, img);
		if (psi->m_pImgs[nImgSize])
		{
			delete psi->m_pImgs[nImgSize];
			psi->m_pImgs[nImgSize] = NULL;
		}
		psi->m_pImgs[nImgSize] = img;
		psi->m_nLoadedHrids[nImgSize]=nHrid;
		psi->m_nAppliedHrid = nHrid;
	}

	return true;
}

//-------------------------------------------------------------------------------------
void CSWMiniMapUpdater::ApplyNeeded(const Recti &worldSegRect, TSegImg::EImageSizes eImgSize)
{
	//AUTO_LOCK_MMUP(g_cs);

	if (m_pSWMiniMapUpdaterCallback == NULL)
	{
		NoLongerApplied();
		return;
	}
	int nImgSize = (int)eImgSize;
	for (int wy = worldSegRect.Min.y; wy < worldSegRect.Max.y; wy++)
		for (int wx = worldSegRect.Min.x; wx < worldSegRect.Max.x; wx++)
		{
			if (s_bSuspend)
				return;
			AUTO_LOCK_MMUP(g_cs);
			if (this != CSWMiniMapUpdater::Get())
				return;
			int nSegmentId = MakeSegmentID(wx, wy);
			TSegImg* psi = GetSegImg(nSegmentId, false);
			if (!psi)
				continue;

			int nSizeToApply = psi->GetNeededSize(nImgSize);
			if(psi->m_nAppliedHrid != psi->m_nLoadedHrids[nSizeToApply])
			{
				
				if (!m_pSWMiniMapUpdaterCallback)
				{
					NoLongerApplied();
					return;
				}
				m_pSWMiniMapUpdaterCallback->SendImage(nSegmentId, psi->m_pImgs[nSizeToApply]);
				psi->m_nAppliedHrid = psi->m_nLoadedHrids[nSizeToApply];
			}
		}
}
//-------------------------------------------------------------------------------------
void CSWMiniMapUpdater::NoLongerApplied()
{
	AUTO_LOCK_MMUP(g_cs);
	for(TSegImgMap::iterator it = m_mapSegImages.begin(); it != m_mapSegImages.end(); ++it)
	{
		TSegImg* psi = &(it->second);
		psi->m_nAppliedHrid = -1;
	}
}

void CSWMiniMapUpdater::TriggerManuallyUpdate()
{
	NoLongerApplied();
	Update();
}
//-------------------------------------------------------------------------------------
void CSWMiniMapUpdater::SetWorldName(const char* pcWorldName)
{
	m_strWorldName.Format("%s",pcWorldName);

	// also change the level path in persistance module
	CString sPath = gEnv->pCryPak->GetGameFolder() + string("\\levels\\");
	sPath += m_strWorldName.c_str();

	//m_pPersist->SetLevelPath(sPath);
	SAFE_DELETE(m_pPersist);
	m_pPersist = new CSCDataPersistence(m_tmpVC,sPath);
}

//########################################################################################

void TSegmentState::TState::SyncLockInfo( struct TLayerData& pLayer )
{
	this->m_nLockedBy = DataInspector::m_nLockedBy(pLayer);
}
