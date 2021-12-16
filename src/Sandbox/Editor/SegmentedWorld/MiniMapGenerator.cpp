#include "StdAfx.h"
#include "InternalCommon.h"
#include "MiniMapGenerator.h"

#include "SegmentedWorldDoc.h"
#include "SegmentedWorldMiniMapUpdater.h"
#include "SegmentData.h"
#include "GameEngine.h"
#include "Terrain/Heightmap.h"

using namespace sw;

void CMiniMapGenerator::OnScreenshotTaken( void* data, uint32 width, uint32 height )
{
	CWaitCursor wait;
	//CString strSegmentPakFileName;
	//CString sSegmentPath;
	//GetPakPath(strSegmentPakFileName, sSegmentPath, m_wcMiniMap.wx, m_wcMiniMap.wy);
	//string imageBMPFile;
	//imageBMPFile.Format("%sminimap\\%d_%d_map.bmp",m_sPath,m_wcMiniMap.wx, m_wcMiniMap.wy);

	uint8* buf=(uint8*)data;

	CImageEx scrShotImage;
	scrShotImage.Allocate(width, height);


	for(int y=0; y<height; y++)
		for(int x=0; x<width; x++)
			scrShotImage.ValueAt(x, height-1 - y) = RGBA8(buf[x*3+2 + y*width*3], buf[x*3+1 + y*width*3], buf[x*3 + y*width*3], 255);

	if (m_nScreenshotMode == 2)
	{
		int segW = m_minimap.textureWidth / m_nWidth;
		int segH = m_minimap.textureHeight / m_nHeight;
		while (m_wcArrMiniMap.size() > 0)
		{
			AUTO_LOCK(g_cs);
			TWorldCoords wc = m_wcArrMiniMap[m_wcArrMiniMap.size()-1];
			TLocalCoords lc = wc;
			CImageEx imageS3;
			int x = (lc.lx - m_lcTopLeft.lx) * segW;
			int y = (lc.ly - m_lcTopLeft.ly) * segH;
			scrShotImage.GetSubImage(x, y, segW, segH, imageS3);

			CopyToCache(wc, imageS3);

			m_wcArrMiniMap.pop_back();
		}
	}

	//CImageTIF imageTIF;
	//imageTIF.SaveRAW( imageTIFFile.c_str(), image.GetData(), image.GetWidth(), image.GetHeight(), 1, "Diffuse_lowQ" );
	//CImageUtil::SaveBitmap(imageBMPFile.c_str(), scrShotImage);

	assert(m_wcArrMiniMap.empty());
	GetIEditor()->SetConsoleVar( "e_ScreenShot",0 );
}


bool CMiniMapGenerator::GenerateSegmentedMap(TWorldCoordsArray* pArrWorldCoords)
{
	AUTO_LOCK(g_cs);
	//if (GetIEditor()->GetSegmentedWorldManager()->IsOfflineMode()) return false;
	ICVar * pVar = gEnv->pConsole->GetCVar("e_ScreenShot");
	bool bInscreenShotState = (pVar && pVar->GetIVal()!=0);
	if (bInscreenShotState)
		return false;

	m_wcArrMiniMap.clear();
	TLocalCoords lc;
	int w = m_pWorld->GetLoadedSize().x;
	int h = m_pWorld->GetLoadedSize().y;

	for (lc.ly = 0; lc.ly < h; ++lc.ly) 
	{
		for (lc.lx = 0; lc.lx < w; ++lc.lx) 
		{
			TWorldCoords wc(lc);
			TSegmentData *pd = m_pWorld->SegmentAggr().GetSegmentData(wc, false);
			if (pd == NULL)
				continue;
			if (!m_pUI->IsInSelectMode())
			{
				if (!pd->NeedRegenerateMiniMap())
					continue;
			}
			else
			{
				if (!m_pUI->IsSegmentSelected(wc))
					continue;
			}

			m_wcArrMiniMap.push_back(wc);
		}
	}

	if (pArrWorldCoords != NULL)
	{
		for (int i = 0; i < pArrWorldCoords->size(); i++)
		{
			TLocalCoords lc((*pArrWorldCoords)[i]);
			if (lc.lx < 0 || lc.ly < 0 || lc.lx >= w || lc.ly >= h)
				continue;
			TSegmentData *pd = m_pWorld->SegmentAggr().GetSegmentData(lc, false);
			if (pd == NULL)
				continue;
			stl::push_back_unique(m_wcArrMiniMap, lc);
		}
	}

	if (m_wcArrMiniMap.size() <= 0)
		return true;

	//m_nScrShotState = 1;
	m_nScreenshotMode = 2;
	
	m_AdditionalConstClearList.clear();
	m_AdditionalConstClearList["r_HDRRendering"]	= gEnv->pConsole->GetCVar("r_HDRRendering")->GetFVal();
	m_AdditionalConstClearList["r_PostProcessEffects"]	=	gEnv->pConsole->GetCVar("r_PostProcessEffects")->GetFVal();
	m_AdditionalConstClearList["r_SSAO"]	= gEnv->pConsole->GetCVar("e_Lods")->GetFVal();
	m_AdditionalConstClearList["e_Fog"]		= gEnv->pConsole->GetCVar("e_Fog")->GetFVal();
	
	// disable necessary cvars to avoid glitch in screenshot
	gEnv->pConsole->GetCVar("r_HDRRendering")->Set(0);
	gEnv->pConsole->GetCVar("r_PostProcessEffects")->Set(0);
	gEnv->pConsole->GetCVar("r_SSAO")->Set(0);
	gEnv->pConsole->GetCVar("e_Fog")->Set(0);

	// because minimap generating use update() to the rest steps
	GetIEditor()->GetSegmentedWorldManager()->DeferredCommandQueue().Pause(true);

	return true;
}

//-----------------------------------------------------------------------------------------------
bool CMiniMapGenerator::DoGenerateSegmentedMap(int lx, int ly, int w, int h)
{
	TLocalCoords lc(lx,ly);
	Recti rcLoaded = m_pWorld->GetLoadedRect();

	if (!rcLoaded.InRect(TWorldCoords(lc)))
		return false;

	AUTO_LOCK(g_cs);
	ICVar * pVar = gEnv->pConsole->GetCVar("e_ScreenShot");
	bool bInscreenShotState = (pVar && pVar->GetIVal()==-6 || pVar->GetIVal()==-5);
	TWorldCoords wc = lc;
	if (wc == m_wcMiniMap && bInscreenShotState)
		return true;

	m_wcMiniMap = wc;
	m_lcTopLeft = lc;

	int size = max(w,h);
	size = UpperBase2PowerNum((uint32)size);
	m_nWidth = size;
	m_nHeight = size;

	int startX = m_lcTopLeft.lx * SEGMENT_SIZE_METERS;
	int endX = startX + m_nWidth * SEGMENT_SIZE_METERS;

	int startY = m_lcTopLeft.ly * SEGMENT_SIZE_METERS;
	int endY = startY + m_nHeight * SEGMENT_SIZE_METERS;

	int nCenterY = startX + (endX - startX) / 2;
	int nCenterX = startY + (endY - startY) / 2;

	int scrShotWidth = m_nWidth * SEGMENT_SIZE_METERS / 2;
	int scrShotHeight = m_nHeight * SEGMENT_SIZE_METERS / 2;

	m_minimap.orientation = 0;
	m_minimap.vCenter.x = nCenterY;
	m_minimap.vCenter.y = nCenterX;
	m_minimap.vExtends.x = scrShotWidth;
	m_minimap.vExtends.y = scrShotWidth;
	m_minimap.textureWidth = scrShotWidth / 2;
	m_minimap.textureHeight = scrShotHeight / 2;

	Generate(false);

	//
	if(!bInscreenShotState)
		GetIEditor()->SetConsoleVar( "e_ScreenShot",-6 );
	//else
	//	GetIEditor()->SetConsoleVar( "e_ScreenShot",5 );
	m_nScreenshotMode = 2;

	return true;
}

void CMiniMapGenerator::CopyToCache( const TWorldCoords& wc, CImageEx& scrShotImage )
{
	TSegmentData *pd = m_pWorld->SegmentAggr().GetSegmentData(wc, true);
	TSegDataBlock *pb;
	pb = pd->GetBlock(SDB_MapS3, true);
	if (pb) 
	{
		//scrShotImage.Compress(pb->m_data);
		pb->SetData(scrShotImage);
		pb->AppliedLocally(wc);
	}

	CImageEx imageS2;
	CImageUtil::DownScaleSquareTextureTwice(scrShotImage, imageS2, CImageUtil::CLAMP);
	pb = pd->GetBlock(SDB_MapS2, true);
	if (pb) 
	{
		//imageS2.Compress(pb->m_data);
		pb->SetData(imageS2);
		pb->AppliedLocally(wc);
	}

	CImageEx image;
	CImageUtil::DownScaleSquareTextureTwice(imageS2, image, CImageUtil::CLAMP);
	pb = pd->GetBlock(SDB_Map, true);
	if (pb)
	{
		//image.Compress(pb->m_data);
		pb->SetData(image);
		pb->AppliedLocally(wc);
	}
}
void CMiniMapGenerator::SaveSegmentedMap()
{
	//if (CSWManager::Get().IsOfflineMode()) return;
	TLocalCoords lc;
	int h = m_pWorld->GetLoadedSize().y;
	int w = m_pWorld->GetLoadedSize().x;
	int64 nVersion = -1;
	EVersionType vt = VT_CURRENT/*VT_OFFICIAL*/;
	for (lc.ly = 0; lc.ly < h; ++lc.ly) 
	{
		for (lc.lx = 0; lc.lx < w; ++lc.lx) 
		{
			std::vector<ESDBType> arrDBTypes;
			TSegmentData *pd = m_pWorld->SegmentAggr().GetSegmentData(lc, false);
			if (pd == NULL)
				continue;
			TSegDataBlock *pb;
			pb = pd->GetBlock(SDB_MapS3, false);
			if (pb != NULL)
				arrDBTypes.push_back(SDB_MapS3);

			pb = pd->GetBlock(SDB_MapS2, false);
			if (pb != NULL)
				arrDBTypes.push_back(SDB_MapS2);

			pb = pd->GetBlock(SDB_Map, false);
			if (pb != NULL)
				arrDBTypes.push_back(SDB_Map);

			if (arrDBTypes.size() > 0)
			{
				if (nVersion == -1)
				{
					nVersion = m_pVC->CreateChangeList(vt);
					if (nVersion <= 0)
						return;
				}
				ForceSaveDataBlocks(arrDBTypes, pd->m_wc, nVersion, vt);
			}
			
		}
	}
}
bool CMiniMapGenerator::ForceSaveDataBlocks(const std::vector<ESDBType> &arrDataBlockTypes, const TWorldCoords &wc, int64 nVersion, EVersionType vt)
{

	if (arrDataBlockTypes.size() <= 0)
		return true;

	TSegmentData *pd = m_pWorld->SegmentAggr().GetSegmentData(wc, false);
	if (pd == NULL)
		return false;

	if (/*g_db->GetCachedConn() &&*/ pd) 
	{
		bool bMissing;
		{
			AUTO_LOCK(g_cs);
			bMissing = pd->IsMissingInPersist();
		}
		if (bMissing) 
		{
			pd->CreateData();
			//AUTO_LOCK(g_cs);
			//pd->m_bMissing = false;
			assert(!pd->IsMissingInPersist());
		}

		//bool bRes = true;
		//int nModified[SDB_COUNT + 1];
		//int cntModified = 0;
		TSegDataBlock* pb = NULL;
		for (int nBT = 0; nBT < arrDataBlockTypes.size(); ++nBT)
		{
			pb = pd->m_Data[arrDataBlockTypes[nBT]];
			if (!pb)
				continue;

			////int nLoadedHRID;
			////{
			////	AUTO_LOCK(g_cs);
			////	nLoadedHRID = pb->m_nLoadedHRID;
			////}

			//if (pb->IsLoadedDataACache()) 
			//{
			//	{
			//		AUTO_LOCK(g_cs);
			//		assert(pb->IsLoadedVersionApplied());
			//	}
			//	if (!pb->SaveData(/*m_wc,*/ nVersion, vt))
			//	{
			//		bRes = false;
			//		continue;
			//	}

			//	//AUTO_LOCK(g_cs);
			//	//for (int nVT = vt; nVT < VT_COUNT; ++nVT)
			//	//{
			//	//	pb->m_nHRIDs[nVT] = pb->m_nLoadedHRID;
			//	//}
			//	pb->UpdateVersionsToLoaded(vt);
			//	nModified[cntModified++] = arrDataBlockTypes[nBT];
			//}

			assert(nVersion>0);
			pb->DoSaveToDB(SF_RegularSave,vt,nVersion);
				}

		//if (cntModified) 
				//{
		//	nModified[cntModified++] = SDB_COUNT;
		//	pd->UpdateRevHead(vt, nModified);
				//}
	}
	return true;
}

bool CMiniMapGenerator::Step()
{
	if(m_nScreenshotMode != 0 && m_wcArrMiniMap.empty())		// we are done!
	{
		AUTO_LOCK(g_cs);
		SaveSegmentedMap();
		CSWMiniMapUpdater* pUpdater = CSWMiniMapUpdater::Get();
		if (pUpdater)
		{
			//CRect rt(0,0, m_pWorld->m_uiWidth-1,m_pWorld->m_uiHeight-1);
			//pUpdater->ReloadModified(rt, TSegImg::SIZE_32X);
			pUpdater->Update();
		}

		ResetToDefault();
		m_nScreenshotMode = 0;

		// unpause deferred console command queue
		GetIEditor()->GetSegmentedWorldManager()->DeferredCommandQueue().Pause(false);
	}
	else if(m_nScreenshotMode == 2 && m_wcArrMiniMap.size() > 0)
	{
		AUTO_LOCK(g_cs);
		int startX = -1;
		int startY = -1;
		int endX = -1;
		int endY = -1;
		for (int i = 0; i < m_wcArrMiniMap.size(); i++)
		{
			TLocalCoords lc = m_wcArrMiniMap[i];
			if (lc.lx < startX || startX == -1)
				startX = lc.lx;
			if (lc.ly < startY || startY == -1)
				startY = lc.ly;
			if (lc.lx > endX || endX == -1)
				endX = lc.lx;
			if (lc.ly > endY || endY == -1)
				endY = lc.ly;
		}
		DoGenerateSegmentedMap(startX,startY,endX-startX+1,endY-startY+1);
	}
	else if (m_nScreenshotMode == 0)
	{
		return false;
	}

	return true;
}

void CMiniMapGenerator::Reset()
{
	AUTO_LOCK(g_cs);
	m_nScreenshotMode = 0; 
	m_wcArrMiniMap.clear();
	m_wcMiniMap.wx = 0;
	m_wcMiniMap.wy = 0;
	m_lcTopLeft.lx = -1;
	m_lcTopLeft.ly = -1;
	m_nWidth = -1;
	m_nHeight = -1;
	m_AdditionalConstClearList.clear();
}

void CMiniMapGenerator::ResetToDefault()
{
	CTerrainMiniMapTool::ResetToDefault();

	ICVar * pVar = gEnv->pConsole->GetCVar("e_ScreenShot");
	if(pVar && pVar->GetIVal()==0)
	{
		for(std::map<string,float>::iterator it=m_AdditionalConstClearList.begin();it!=m_AdditionalConstClearList.end();++it)
		{
			ICVar* pVar	=	gEnv->pConsole->GetCVar(it->first.c_str());
			if(pVar)
				pVar->Set(it->second);
		}
		m_AdditionalConstClearList.clear();
	}
}