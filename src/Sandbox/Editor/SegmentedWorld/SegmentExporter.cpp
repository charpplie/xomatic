#include "StdAfx.h"
#include "InternalCommon.h"
#include "SegmentExporter.h"
#include "SegmentedWorldDoc.h"
#include "SegmentData.h"
#include "Terrain/TerrainManager.h"
#include "Terrain/TerrainGrid.h"
#include <INavigationSystem.h>
#include "GameEngine.h"
#include "AI\NavDataGeneration\Navigation.h"

using namespace sw;

bool CSegmentExporter::OpenSegmentPacksForWrite()
{
	auto& pSW = GetIEditor()->GetSegmentedWorldDoc();
	
	int nIndex = 0;
	CString sSegmentPath;

	for (int ly = 0; ly < m_nHeight; ++ly)
	{
		for (int lx = 0; lx < m_nWidth; ++lx)
		{
			nIndex = lx + ly * m_nWidth;
			SSegmentPakHelper *psphelper = &m_segPak[nIndex];

			TWorldCoords wc(TLocalCoords(lx, ly));
			CSegmentedWorldManager::GetPakPath(psphelper->m_sPath, sSegmentPath, psphelper->m_sBlockPath, wc.wx, wc.wy, m_bMultiPak);
			psphelper->m_sCtcFilename.Format("%s\\%s", psphelper->m_sBlockPath, COMPILED_TERRAIN_TEXTURE_FILE_NAME);

			if(m_bMultiPak)
			{
				m_pGameExporter->CloseLevelPack(*psphelper, true);
				if(!m_pGameExporter->OpenLevelPack(*psphelper, false))
				{
					assert(!"could not open segment pak for writing");
					return false;
				}
			}
		}
	}
	return true;
}

struct ExportSurfaceTextureThread : public CrySimpleThread<>
{
	CGameExporter *m_pge;
	SSegmentPakHelper *m_psphelper;
	float m_fLeft;
	float m_fTop;
	float m_fSizeX;
	float m_fSizeY;

	ExportSurfaceTextureThread()
	{
		SetName("ExportSurfaceTexture");
	}

	ExportSurfaceTextureThread(CGameExporter *pge)
	{
		m_pge = pge;
	}

	void Init(SSegmentPakHelper *pSegmentPakHelper, float fLeft, float fTop, float fSizeX, float fSizeY)
	{
		m_psphelper = pSegmentPakHelper;
		m_fLeft = fLeft;
		m_fTop = fTop;
		m_fSizeX = fSizeX;
		m_fSizeY = fSizeY;
	}

	virtual void Run()
	{
		m_pge->ExportSurfaceTexture(m_psphelper->m_pakFile, m_psphelper->m_sCtcFilename, m_fLeft, m_fTop, m_fSizeX, m_fSizeY);
	}
};

bool CSegmentExporter::ExportSurfaceTexture()
{
	auto& pSW = GetIEditor()->GetSegmentedWorldDoc();

	CWaitProgress progress("Generating Surface Texture");
	
	bool bMT = true;
	int iThread = 0;
	int numThread = GetIEditor()->GetSystem()->GetLogicalCPUCount();
	std::vector<ExportSurfaceTextureThread *> arrThread;
	if(bMT)
	{
		GetIEditor()->ShowStatusText(false);

		arrThread.resize(numThread);
		for(iThread = 0; iThread < numThread; iThread++)
			arrThread[iThread] = new ExportSurfaceTextureThread(m_pGameExporter);
	}

	int nIndex = 0;
	float fSizeX = 1.0f / m_nWidth;
	float fSizeY = 1.0f / m_nHeight;

	for (int ly = 0; ly < m_nHeight; ++ly)
	{
		for (int lx = 0; lx < m_nWidth; ++lx)
		{
			nIndex = lx + ly * m_nWidth;
			SSegmentPakHelper *psphelper = &m_segPak[nIndex];
	
			TWorldCoords wc(TLocalCoords(lx, ly));
			if (!pSW.OnGenerateSurfaceTexture(wc.wx, wc.wy))
				continue;

			if(bMT)
			{
				while(1)
				{
					for(iThread = 0; iThread < numThread; ++iThread)
					{
						if(!arrThread[iThread]->IsRunning())
							break;
					}
					if(iThread >= numThread)
					{
						Sleep(100);
						continue;
					}
					break;
				}
				if(arrThread[iThread]->IsStarted())
				{
					arrThread[iThread]->WaitForThread();
					arrThread[iThread]->Stop();
				}
					
				arrThread[iThread]->Init(psphelper, lx * fSizeX, ly * fSizeY, fSizeX, fSizeY);
				arrThread[iThread]->Start();
				while (!arrThread[iThread]->IsRunning()) Yield();

				progress.Step((ly * m_nWidth + lx) * 100 / (m_nWidth * m_nHeight));
			}
			else
				m_pGameExporter->ExportSurfaceTexture(psphelper->m_pakFile, psphelper->m_sCtcFilename, lx * fSizeX, ly * fSizeY, fSizeX, fSizeY);
		}
	}

	if(bMT)
	{
		bool bDone = false;
		while(!bDone)
		{
			bDone = true;
			for(iThread = 0; iThread < numThread; ++iThread)
			{
				if(arrThread[iThread]->IsRunning())
				{
					bDone = false;
					break;
				}
				if(arrThread[iThread]->IsStarted())
				{
					arrThread[iThread]->WaitForThread();
					arrThread[iThread]->Stop();
				}
			}

			Sleep(100);
		}

		for(iThread = 0; iThread < numThread; ++iThread)
			delete arrThread[iThread];

		GetIEditor()->ShowStatusText(true);
	}
	return true;
}

void CSegmentExporter::UpdateAndCloseSegmentPacksForWrite()
{
	if(!m_bMultiPak)
		return;

	int nSize = m_segPak.size();

	for(int i = 0; i < nSize; i++)
	{
		SSegmentPakHelper *psphelper = &m_segPak[i];
		
		m_pGameExporter->CloseLevelPack(*psphelper, false);

		if(m_bExportMap)
		{
			m_pGameExporter->OpenLevelPack(*psphelper, true);
			uint32 dwCRC = gEnv->pCryPak->ComputeCRC(psphelper->m_sCtcFilename);
			m_pGameExporter->CloseLevelPack(*psphelper, true);

			m_pGameExporter->OpenLevelPack(*psphelper, false);
			psphelper->m_pakFile.GetArchive()->UpdateFileCRC(psphelper->m_sCtcFilename,dwCRC);
			m_pGameExporter->CloseLevelPack(*psphelper, false);
		}

		m_pGameExporter->OpenLevelPack(*psphelper, true);
	}
}

bool CSegmentExporter::ExportMap()
{

	CLogFile::WriteLine("Exporting terrain texture.");
	GetIEditor()->SetStatusText("Exporting terrain texture (Generating)...");

	CHeightmap* pHeightMap = GetIEditor()->GetHeightmap();
	int nMaxTilesNum = GetIEditor()->Get3DEngine()->GetTerrainSize()/
		GetIEditor()->Get3DEngine()->GetTerrainTextureNodeSizeMeters();

//	pHeightMap->GetTerrainGrid()->InitSectorGrid(nMaxTilesNum);		// release the editor textures on the terrain

	DWORD startTime = GetTickCount();

	ExportSurfaceTexture();

	CLogFile::WriteLine("Update terrain texture file...");
	pHeightMap->GetRGBLayer()->CleanupCache();
	pHeightMap->ClearModSectors();

	CLogFile::FormatLine( "Terrain Texture Exported in %u seconds.",(GetTickCount()-startTime)/1000 );
	return true;
}

bool CSegmentExporter::ExportHeightMap(bool bForce)
{
	auto psw = GetInternalSWDoc();
	assert(psw);

	ESaveFlag sflag = bForce ? SF_ForceSaveAll : SF_RegularSave;

	CString sFileBlockName;
	int nIndex = 0;

	CSegmentDataAggr &segAggr = psw->SegmentAggr();

	for (int ly = 0; ly < m_nHeight; ++ly)
	{
		for (int lx = 0; lx < m_nWidth; ++lx)
		{
			nIndex = lx + ly * m_nWidth;
			SSegmentPakHelper *psphelper = &m_segPak[nIndex];

			TWorldCoords wc(TLocalCoords(lx, ly));
			TSegmentData *pSeg = segAggr.GetSegmentData(wc, false);
			assert(pSeg);

			pSeg->SaveDataForEngine(sflag, pSeg->m_wc, -1, VT_CURRENT);

			for(ESDBTypeIterator eBlockType = SDB_ENGINEDATA_BEGIN; eBlockType != SDB_ENGINEDATA_END; ++eBlockType)
			{
				if(eBlockType == SDB_EngRGBData)
					continue; // skip surface texture

				sFileBlockName.Format( "%s%s", psphelper->m_sBlockPath, GetFileBlockName(eBlockType) );

				TSegDataBlock *pb = pSeg->GetBlock(eBlockType, false);

				uint32 nBlocks = pb->GetMemoryBlockCount();
				for(uint i = 0; i < nBlocks; ++i)
				{
					SNamedMemoryBlock &block = pb->GetNamedMemoryBlock(i);

					string sFilename = sFileBlockName.GetBuffer();
					if(!block.name.empty())
						sFilename = string().Format( "%s%s", sFilename, block.name.c_str() );
			
					if(!PathUtil::GetFile(sFilename).empty())
					{
						psphelper->m_pakFile.GetArchive()->UpdateFile(sFilename, block.mem.GetBuffer(), block.mem.GetSize());
					}
				}
			}
		}
	}

	return true;
}

// Exports per-segment navigation data to corresponding .bai files
void CSegmentExporter::ExportNavigation()
{
	CFile file;
	CMemoryBlock mem;

	for(int ly = 0; ly < m_nHeight; ++ly)
	{
		for(int lx = 0; lx < m_nWidth; ++lx)
		{
			const int nIndex = lx + ly * m_nWidth;
			SSegmentPakHelper *psphelper = &m_segPak[nIndex];

			TLocalCoords lc(lx, ly);
			AABB segmentBoundingBox = CSegmentedWorldDoc::CalcWorldBounds(lc);

			// export MNM
			stack_string navigationPathName;
			navigationPathName.Format("%smnmnav.bai", psphelper->m_sBlockPath);
#ifdef SEG_WORLD
			gEnv->pAISystem->GetNavigationSystem()->SaveToFile(navigationPathName.c_str(), segmentBoundingBox);
#endif

			if(file.Open(navigationPathName.c_str(), CFile::modeRead))
			{
				mem.Allocate(file.GetLength());
				file.Read(mem.GetBuffer(), file.GetLength());
				file.Close();

				if(psphelper->m_pakFile.UpdateFile(navigationPathName, mem))
					DeleteFile(navigationPathName);
			}

			// export Areas(AI Paths, AI Territory, ...)
			stack_string areasName;
			areasName.Format("%sareas.bai", psphelper->m_sBlockPath);
#ifdef SEG_WORLD
			GetIEditor()->GetGameEngine()->GetNavigation()->WriteAreasIntoFile(areasName.c_str(), segmentBoundingBox);
#endif

			if(file.Open(areasName.c_str(), CFile::modeRead))
			{
				mem.Allocate(file.GetLength());
				file.Read(mem.GetBuffer(), file.GetLength());
				file.Close();

				if(psphelper->m_pakFile.UpdateFile(areasName, mem))
					DeleteFile(areasName);
			}
		}
	}
}

void CSegmentExporter::ExportSegments(CGameExporter *pge, bool bSurfaceTexture)
{
	assert(pge);
	m_pGameExporter = pge;
	
	auto& pSW = GetIEditor()->GetSegmentedWorldDoc();
	
	m_bExportMap = GetIEditor()->GetTerrainManager()->GetLayerCount() && bSurfaceTexture;
	m_bMultiPak = CSegmentedWorldManager::IsLevelInMultiPack();
	pSW.GetSizeInSegments((UINT &)m_nWidth, (UINT &)m_nHeight);
	assert(m_nWidth && m_nHeight);

	m_segPak.resize(m_nWidth * m_nHeight);

	OpenSegmentPacksForWrite();

	if(m_bExportMap)
		ExportMap();

	if(!GetIEditor()->GetSegmentedWorldManager()->IsMapConverting())
	{
		ExportHeightMap();
	
		ExportNavigation();
	}

	UpdateAndCloseSegmentPacksForWrite();
}

void CSegmentExporter::UpdateLevelInfo(XmlNodeRef levelFile)
{
	auto& pSW = GetIEditor()->GetSegmentedWorldDoc();

	Vec2i worldMin, worldMax;
	pSW.GetWorldBounds(&worldMin, &worldMax);
	int segmentSizeMeters = pSW.GetSegmentSizeInMeters();
	levelFile->setAttr( "SegmentedWorld", 1 );
	levelFile->setAttr( "SegmentedWorldSizeMeters", segmentSizeMeters );
	levelFile->setAttr( "SegmentedWorldMinX", worldMin.x );
	levelFile->setAttr( "SegmentedWorldMinY", worldMin.y );
	levelFile->setAttr( "SegmentedWorldMaxX", worldMax.x );
	levelFile->setAttr( "SegmentedWorldMaxY", worldMax.y );
	if(((CCryEditApp*)AfxGetApp())->IsInSWBatchMode())
		levelFile->setAttr( "EntitiesUseGUIDs", false);
	else
		levelFile->setAttr( "EntitiesUseGUIDs", gEnv->pEntitySystem->EntitiesUseGUIDs() );
}
