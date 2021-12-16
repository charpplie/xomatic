
#include "StdAfx.h"
#include "SegmentLevelMerger.h"
#include "Utilities.h"
#include "CryEditDoc.h"
#include "SegmentedWorldManager.h"
#include "InternalCommon.h"
#include "SegmentedWorldDoc.h"
#include "Util/PakFile.h"
#include "DataUpgrade.h"
#include "Objects/ObjectLayerManager.h"

using namespace sw;

bool CSegmentLevelMerger::ValidateInput()
{
	if (!CFileUtil::PathExists(m_strDestPath) || !CFileUtil::PathExists(m_strSrcPath))
	{
		SWLog("Destination or source level not exists");
		return false;
	}

	// if a SW level is opened check if it conflicts with this operation
	if (GetIEditor()->GetDocument()->IsDocumentReady() && GetInternalSWDoc()) 
	{
		auto  pSWDoc = GetInternalSWDoc();
		CString strCurLevel = Path::GamePathToFullPath(GetIEditor()->GetLevelFolder());
		Path::ConvertBackSlashToSlash(strCurLevel);

		Recti rectInUse;
		rectInUse.Min.set(pSWDoc->m_wcOfs.wx, pSWDoc->m_wcOfs.wy);
		rectInUse.Max = rectInUse.Min + Vec2i(pSWDoc->GetWidth(),pSWDoc->GetHeight()) - Vec2i(1,1);

		if (0==m_strDestPath.CompareNoCase(strCurLevel) && GetRectDest().Intersects(rectInUse))
		{
			SWLog("You cannot merge to the dest segment(s) that have been opened for editing");
			return false;
		}

		if (0==m_strSrcPath.CompareNoCase(strCurLevel) && GetRectSrc().Intersects(rectInUse))
		{
			SWLog("You cannot merge from the src segment(s) that have been opened for editing");
			return false;
		}
	}

	// Check level.pak availability
	//ICryPak* pIPak = GetISystem()->GetIPak();
	CPakFile srcPak, destPak;
	CString strSrcPackFile = m_strSrcPath + "/"SW_LevelPakFile;
	CString strDestPackFile = m_strDestPath + "/"SW_LevelPakFile;

	// close read-only archive(in CGameEngine::LoadLevel) to be able to re-open as read-write below
	GetISystem()->GetIPak()->ClosePack(strDestPackFile);

	if (!srcPak.OpenForRead(strSrcPackFile)	// should be read-only
		|| !destPak.Open(strDestPackFile, true))
	{
		SWLog("Failed to open level.pak while merging levels");
		return false;
	}
	srcPak.Close();
	destPak.Close();

	// Check data structure version
	if (CDataUpgrade::Get().DetectSWLevelVersion(m_strDestPath) != CSegmentedWorldManager::GetSWDataStructureVersion()
		|| CDataUpgrade::Get().DetectSWLevelVersion(m_strSrcPath) != CSegmentedWorldManager::GetSWDataStructureVersion())
	{
		SWLog("Destination or source level is not in new format, please upgrade the level(s) first");
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// Check HeightMap UnitSize, WaterLevel, TextureSize
	bool bMergeCoverCTC = true;
	{
		XmlNodeRef xmlDest = XmlHelpers::LoadXmlFromFile( m_strDestPath + "/" + GetWorldDataFileName(WDB_LEVELGENERAL) );
		XmlNodeRef xmlSrc = XmlHelpers::LoadXmlFromFile( m_strSrcPath + "/" + GetWorldDataFileName(WDB_LEVELGENERAL) );
		if (!(xmlDest && xmlDest->isTag("Level") && (xmlDest = xmlDest->findChild("Heightmap"))
			&& xmlSrc && xmlSrc->isTag("Level") && (xmlSrc = xmlSrc->findChild("Heightmap"))
			))
		{
			SWLog("Destination or source level seems to be broken! Merging aborted!");
			return false;
		}

		int nUnitSizeDest = 0, nUnitSizeSrc = 0;
		if (!(xmlDest->getAttr("UnitSize",nUnitSizeDest)
			&& xmlSrc->getAttr("UnitSize",nUnitSizeSrc)
			&& nUnitSizeDest == nUnitSizeSrc)
			)
		{
			SWLog("Destination and source level are having different HeightMap UnitSize! Merging aborted!");
			return false;
		}

		float nWaterLevelDest =0, nWaterLevelSrc = 0;
		if (!(xmlDest->getAttr("WaterLevel",nWaterLevelDest) && xmlSrc->getAttr("WaterLevel",nWaterLevelSrc)
			&& nWaterLevelDest == nWaterLevelSrc))
		{
			int nAnswer = CSegmentedWorldManager::SWMsgBoxQuestion("Warning: Destination and source level are having different WaterLevel, some part of terrain may going to be under water.\n"
				"Continue?");
			if (nAnswer != IDYES)
				return false;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Check if any dest seg folders already exist
	//		the world allows holes, so width and height of world are useless
	// (we must do the whole check before copying, because we cannot rollback if anything conflicted)

	for (int dy = 0; dy < m_vSize.y; ++dy)
	{
		for (int dx = 0; dx < m_vSize.x; ++dx)
		{
			char tmpPath[MAX_PATH];
			_snprintf(tmpPath, 
				MAX_PATH, 
				"%s/" SW_EditorDataDirPath SW_SegmentDirPattern, 
				m_strDestPath, 
				m_vDestTopLeft.x + dx, 
				m_vDestTopLeft.y + dy);
			if (CFileUtil::PathExists(tmpPath))
			{
				SWLog("Dest segment(s) already exists");
				return false;
			}
		}
	}

	return true;
}

void CSegmentLevelMerger::CacheXmlArray( XmlArray& arrMainLayerFragments, XmlArray& arrSegInfoFragments )
{
	//////////////////////////////////////////////////////////////////////////
	// Merge Main Layers (before copying)
	// it's impossible to skip a certain file during the next step, so we have to keep the data
	// in memory and write it to file later.

	arrMainLayerFragments.reserve(m_vSize.x * m_vSize.y);
	arrSegInfoFragments.reserve(m_vSize.x * m_vSize.y);
	for (int dy = 0; dy < m_vSize.y; ++dy)
	{
		for (int dx = 0; dx < m_vSize.x; ++dx)
		{
			CString srcSegDir;
			srcSegDir.Format(SW_SegmentDirPattern, m_vSrcTopLeft.x + dx, m_vSrcTopLeft.y + dy);

			uint64 uFilesize;
			CString strFileSrc;
			strFileSrc.Format(m_strMainLayerPattern, m_strSrcPath, srcSegDir);
			if (CFileUtil::FileExists(strFileSrc) && CFileUtil::GetDiskFileSize(strFileSrc,uFilesize) && uFilesize > 0)
			{
				XmlNodeRef xmlResult = XmlHelpers::LoadXmlFromFile(strFileSrc);
				if (xmlResult && xmlResult->getChildCount() > 0)
					arrMainLayerFragments.push_back(std::make_pair(CPoint(m_vDestTopLeft.x + dx, m_vDestTopLeft.y + dy), xmlResult));
			}

			strFileSrc.Format(m_strSegInfoPattern, m_strSrcPath, srcSegDir);
			if (CFileUtil::FileExists(strFileSrc) && CFileUtil::GetDiskFileSize(strFileSrc,uFilesize) && uFilesize > 0)
			{
				XmlNodeRef xmlResult = XmlHelpers::LoadXmlFromFile(strFileSrc);
				if (xmlResult && xmlResult->getChildCount() > 0)
					arrSegInfoFragments.push_back(std::make_pair(CPoint(m_vDestTopLeft.x + dx, m_vDestTopLeft.y + dy), xmlResult));
			}
		}
	}
}

void CSegmentLevelMerger::CopyExtobjFolder()
{
	CString srcExtobjRoot = m_strSrcPath + "/" SW_ExternalObjectDirPath;
	CString destExtobjRoot = m_strDestPath + "/" SW_ExternalObjectDirPath;

	if (CFileUtil::PathExists(Path::RemoveBackslash(srcExtobjRoot)))
	{
		CFileUtil::CreatePath(destExtobjRoot);
		CFileUtil::CopyTree(srcExtobjRoot, destExtobjRoot);
	}

	if (m_progressBar)
	{
		m_progressBar->Step( 100 * m_nCurStep++ / m_nTotoalSteps );
	}
}

void CSegmentLevelMerger::CopySegeditorFolders()
{
	CString srcEditorSegsRoot = m_strSrcPath + "/"SW_EditorDataDirPath;
	CString destEditorSegsRoot = m_strDestPath + "/"SW_EditorDataDirPath;

	//////////////////////////////////////////////////////////////////////////
	// Copy seg_editor folders
	for (int dy = 0; dy < m_vSize.y; ++dy)
	{
		for (int dx = 0; dx < m_vSize.x; ++dx)
		{
			CString destSegDir, srcSegDir;
			destSegDir.Format(SW_SegmentDirPattern,m_vDestTopLeft.x + dx, m_vDestTopLeft.y + dy);
			srcSegDir.Format(SW_SegmentDirPattern, m_vSrcTopLeft.x + dx, m_vSrcTopLeft.y + dy);
			if (CFileUtil::PathExists(srcEditorSegsRoot + srcSegDir))	// hole(s) are allowed so we have to check each directory
			{
				CFileUtil::CreatePath(destEditorSegsRoot+destSegDir+"/");
				CFileUtil::CopyTree( srcEditorSegsRoot+srcSegDir + "/", destEditorSegsRoot+destSegDir + "/");
			}

			if (m_progressBar)
			{
				m_progressBar->Step( 100 * m_nCurStep++ / m_nTotoalSteps );
			}
		}
	}
}

void CSegmentLevelMerger::CopySegFolders()
{
	CString srcSegsRoot = m_strSrcPath + "/"SW_SegmentsDirPath;
	CString destSegsRoot = m_strDestPath + "/"SW_SegmentsDirPath;
	//////////////////////////////////////////////////////////////////////////
	// Copy seg folders
	for (int dy = 0; dy < m_vSize.y; ++dy)
	{
		for (int dx = 0; dx < m_vSize.x; ++dx)
		{
			CString destSegDir, srcSegDir;
			destSegDir.Format(SW_SegmentDirPattern, m_vDestTopLeft.x + dx, m_vDestTopLeft.y + dy);
			srcSegDir.Format(SW_SegmentDirPattern, m_vSrcTopLeft.x + dx, m_vSrcTopLeft.y + dy);
			if (CFileUtil::PathExists(srcSegsRoot + srcSegDir))	// hole(s) are allowed so we have to check each directory
			{
				CFileUtil::CreatePath(destSegsRoot+destSegDir+"/");
				CFileUtil::CopyTree( srcSegsRoot+srcSegDir + "/", destSegsRoot+destSegDir + "/");
			}
			// update offset info for each segment that will be used for global positions in visarea
			CPakFile segPak;
			CString pakFile, offsetFile;
			XmlNodeRef offsetXML;
			pakFile.Format("%s/"SW_SegmentsDirPath SW_SegmentDirPattern"/%s", m_strDestPath, m_vDestTopLeft.x + dx, m_vDestTopLeft.y + dy, SW_SegPakFile);
			offsetFile.Format("%s/"SW_SegmentsDirPath SW_SegmentDirPattern"/offsetinfo.xml", m_strDestPath, m_vDestTopLeft.x + dx, m_vDestTopLeft.y + dy);
			if(gEnv->pCryPak->OpenPack(pakFile))
			{
				Vec2 vOff(0, 0);
				offsetXML = XmlHelpers::LoadXmlFromFile(offsetFile);

				if(offsetXML)
					offsetXML->getAttr("vOff", vOff);
				else
					offsetXML = XmlHelpers::CreateXmlNode("OffsetInfo");

				vOff += GetDiffSrcToDest();
				offsetXML->setAttr("vOff", vOff);

				gEnv->pCryPak->ClosePack(pakFile);
			}
			if(segPak.Open(pakFile, true))
			{
				XmlString xmlString = offsetXML->getXML();
				segPak.GetArchive()->UpdateFile(offsetFile,(void *)xmlString.data(), xmlString.size());
				segPak.Close();
			}

			if (m_progressBar)
			{
				m_progressBar->Step(100 * m_nCurStep++ / m_nTotoalSteps);
			}
		}
	}
}

void CSegmentLevelMerger::MergeLayerInfoFile( XmlNodeRef& defaultLayer, std::vector<GUID>& otherMainLayerGUIDs, TGuidXmlNodeMap& layermapping )
{
	//////////////////////////////////////////////////////////////////////////
	// Merge SW_LayerInfoFile
	{
		XmlNodeRef resultLayerInfo = XmlHelpers::CreateXmlNode("Layers");
		XmlNodeRef srcLayerInfos[SourceData_count];
		srcLayerInfos[SourceData_Dest] = XmlHelpers::LoadXmlFromFile(m_strDestPath + "/" SW_LayerInfoFile);
		srcLayerInfos[SourceData_Src] = XmlHelpers::LoadXmlFromFile(m_strSrcPath + "/" SW_LayerInfoFile);

		m_nTotoalSteps += 0 - m_nProgStageEstimate + srcLayerInfos[SourceData_Dest]->getChildCount() + srcLayerInfos[SourceData_Src]->getChildCount();

		size_t nLayerIdx = 1;	// layer index starts from 1
		for (size_t iRoot = 0; iRoot < SourceData_count; ++iRoot)
		{
			XmlNodeRef layerinfo = srcLayerInfos[iRoot];
			assert(layerinfo->isTag("Layers"));
			for (size_t ii = 0; ii < layerinfo->getChildCount(); ++ii)
			{
				if(m_progressBar)
				{
					m_progressBar->Step(100 * m_nCurStep++ / m_nTotoalSteps);
				}

				XmlNodeRef nodeLayer = layerinfo->getChild(ii);
				assert(nodeLayer->isTag("Layer"));
				// we ignored layer id here because that will be re sorted

				XmlNodeRef nodeLayerAttr = nodeLayer->getChild(0);
				assert(nodeLayerAttr->isTag("LayerAttribs"));


				GUID guid;
				nodeLayerAttr->getAttr("GUID",guid);
				if (layermapping.find(guid) != layermapping.end())
					continue;					// same layer exists

				XmlNodeRef nodeLayerNew = XmlHelpers::CreateXmlNode("Layer");
				nodeLayerNew->setAttr("ID",nLayerIdx++);
				nodeLayerNew->addChild(nodeLayerAttr);

				// handle main layers
				if (0 == stricmp(nodeLayerAttr->getAttr("FullName"), "Main"))	
				{
					if (!defaultLayer)
						defaultLayer = nodeLayerNew;
					else	// even more Main layers
					{
						otherMainLayerGUIDs.push_back(guid);
						continue;							// skip
					}
				}

				resultLayerInfo->addChild(nodeLayerNew);

				// fill layer mapping
				layermapping.insert(std::make_pair(guid,nodeLayerNew));
			}
		}
		assert(resultLayerInfo->getChildCount());	// at least one layer!
		if (!defaultLayer && resultLayerInfo->getChildCount())
			defaultLayer = resultLayerInfo->getChild(0);	// if no main layer pick any layer that comes first.
		XmlHelpers::SaveXmlNode(resultLayerInfo, m_strDestPath + "/" SW_LayerInfoFile);
	}
	assert(otherMainLayerGUIDs.size() <= 1); // there should be only one other Main layer
}

void CSegmentLevelMerger::UpdateGUIDForMainLayerFiles(const XmlArray& arrMainLayerFragments, const GUID& guidDefMainLayer )
{
	//////////////////////////////////////////////////////////////////////////
	// Replace Main.lyr files in dest folder

	for (size_t ii = 0; ii < arrMainLayerFragments.size(); ++ii)
	{
		XmlNodeRef xml = arrMainLayerFragments[ii].second;
		for (int jj = 0; jj < xml->getChildCount(); ++jj)
			xml->getChild(jj)->setAttr("LayerGUID", guidDefMainLayer);	// put all objects to default layer

		CString destSegDir;
		const CPoint& wc = arrMainLayerFragments[ii].first;
		destSegDir.Format(SW_SegmentDirPattern, wc.x, wc.y);

		CString strFile;
		strFile.Format(m_strMainLayerPattern, m_strDestPath, destSegDir);
		CFileUtil::DeleteFile(strFile);
		XmlHelpers::SaveXmlNode(xml, strFile);
	}
}

void CSegmentLevelMerger::UpdateGUIDForSegInfoFiles(const XmlArray& arrSegInfoFragments, const std::vector<GUID>& otherMainLayerGUIDs, const GUID& guidDefMainLayer)
{
	//////////////////////////////////////////////////////////////////////////
	// Replace segmentinfo.xml files in dest folder

	for (size_t i = 0; i < arrSegInfoFragments.size(); ++i)
	{
		XmlNodeRef xml = arrSegInfoFragments[i].second;
		assert(xml);
		for (int j = 0; j < xml->getChildCount(); ++j)
		{
			XmlNodeRef nodeSeg = xml->getChild(j);
			assert(nodeSeg->isTag("Segment"));

			GUID guidLayerInSeg;
			nodeSeg->getAttr("LayerGUID", guidLayerInSeg);

			// this is an obsoleted main layer, replace the guid with the new main layer guid
			if (!otherMainLayerGUIDs.empty() && otherMainLayerGUIDs[0] == guidLayerInSeg)
			{
				guidLayerInSeg = guidDefMainLayer;
				nodeSeg->setAttr("LayerGUID", guidLayerInSeg);
			}
			
			// update world coordinate according to the offset
			Vec2 wcSrc;
			nodeSeg->getAttr("wc", wcSrc);
			nodeSeg->setAttr("wc", Vec2(GetDiffSrcToDest().x + wcSrc.x, GetDiffSrcToDest().y + wcSrc.y));
		}

		CString destSegDir;
		const CPoint& wc = arrSegInfoFragments[i].first;
		destSegDir.Format(SW_SegmentDirPattern, wc.x, wc.y);

		CString strFile;
		strFile.Format(m_strSegInfoPattern, m_strDestPath, destSegDir);
		CFileUtil::DeleteFile(strFile);
		XmlHelpers::SaveXmlNode(xml, strFile);
	}
}

void CSegmentLevelMerger::MergeSegmentInfoFile(const std::vector<GUID>& otherMainLayerGUIDs, const TGuidXmlNodeMap& layermapping, const GUID& guidDefMainLayer )
{
	//////////////////////////////////////////////////////////////////////////
	// Merge SW_SegmentInfoFile
	{
		XmlNodeRef srcSegInfos[SourceData_count];
		srcSegInfos[SourceData_Dest] = XmlHelpers::LoadXmlFromFile(m_strDestPath + "/" SW_SegmentInfoFile);
		srcSegInfos[SourceData_Src] = XmlHelpers::LoadXmlFromFile(m_strSrcPath + "/" SW_SegmentInfoFile);

		srcSegInfos[SourceData_Src] = srcSegInfos[SourceData_Src]->findChild("SegLayerMappings");
		srcSegInfos[SourceData_Dest] = srcSegInfos[SourceData_Dest]->findChild("SegLayerMappings");
		assert(srcSegInfos[SourceData_Src] && srcSegInfos[SourceData_Dest]);

		m_nTotoalSteps += 0 - m_nProgStageEstimate + srcSegInfos[SourceData_Dest]->getChildCount() + srcSegInfos[SourceData_Src]->getChildCount();

		XmlNodeRef resultSegInfo = XmlHelpers::CreateXmlNode("SegLayerMappings");
		// get those seginfo in range, and fill dest file
		for (size_t idxRoot = 0; idxRoot < SourceData_count; ++idxRoot)
		{
			XmlNodeRef seginfo = srcSegInfos[idxRoot];
			for (size_t ii = 0; ii < seginfo->getChildCount(); ++ii)
			{
				XmlNodeRef nodeSeg = seginfo->getChild(ii);
				assert(nodeSeg->isTag("Segment"));

				GUID guidLayerInSeg;
				nodeSeg->getAttr("LayerGUID",guidLayerInSeg);

				// this is an obsoleted main layer, replace the guid with the new main layer guid
				if ( !otherMainLayerGUIDs.empty() && otherMainLayerGUIDs[0] == guidLayerInSeg)
				{
					guidLayerInSeg = guidDefMainLayer;
					nodeSeg->setAttr("LayerGUID", guidLayerInSeg);
				}

				auto iterLayer = layermapping.find(guidLayerInSeg);
				if (iterLayer == layermapping.end())
					continue;		// if no such layer in layerinfo, we should not put the mapping in segmentinfo.

				XmlNodeRef layerNode = iterLayer->second;

				if (idxRoot == SourceData_Src)
				{
					// skip not needed and offset positions of those needed
					Vec2 tmpwc;
					nodeSeg->getAttr("wc",tmpwc);
					Vec2i wcSrc((int)tmpwc.x, (int)tmpwc.y);

					if (!GetRectSrc().InRect(wcSrc))
						continue;															// skip if not in range

					nodeSeg->setAttr("wc", Vec2(GetDiffSrcToDest().x + wcSrc.x, GetDiffSrcToDest().y + wcSrc.y));
				}


				// Layer id and name should be changed to new id and new name
				nodeSeg->setAttr("LayerID", layerNode->getAttr("ID"));
				nodeSeg->setAttr("LayerName", layerNode->getChild(0)->getAttr("Name"));
				resultSegInfo->addChild(nodeSeg);

				if (m_progressBar)
				{
					m_progressBar->Step(100 * m_nCurStep++ / m_nTotoalSteps);
				}
			}
		}

		XmlNodeRef xmlFileWrapper = XmlHelpers::CreateXmlNode("SegmentInfo");
		xmlFileWrapper->addChild(resultSegInfo);
		XmlHelpers::SaveXmlNode(xmlFileWrapper, m_strDestPath + "/" SW_SegmentInfoFile);
	}
}

void CSegmentLevelMerger::MergeObjectinfoXml(const XmlNodeRef& defaultLayer, TGuidXmlNodeMap& layermapping, TGuidList& lstGuidNeedToCopy )
{
	//////////////////////////////////////////////////////////////////////////
	// merge objectinfo.xml (external objects)

	{
		typedef std::set<GUID, guid_less_predicate> TGuidSet;
		TGuidSet	guidsetObjs;		// external objects may cross several segments, so merging by segs may cause duplicated objects.
		XmlNodeRef resultObjInfo = XmlHelpers::CreateXmlNode("Objects");
		XmlNodeRef srcObjInfos[SourceData_count];
		srcObjInfos[SourceData_Dest] = XmlHelpers::LoadXmlFromFile(m_strDestPath + "/" SW_ObjectInfoFile);
		srcObjInfos[SourceData_Src] = XmlHelpers::LoadXmlFromFile(m_strSrcPath + "/" SW_ObjectInfoFile);

		m_nTotoalSteps += 0 - m_nProgStageEstimate + srcObjInfos[SourceData_Dest]->getChildCount() + srcObjInfos[SourceData_Src]->getChildCount();

		for (size_t idxRoot = 0; idxRoot < SourceData_count; ++idxRoot)
		{
			XmlNodeRef objinfo = srcObjInfos[idxRoot];
			assert(objinfo->isTag("Objects"));
			for(size_t ii = 0; ii < objinfo->getChildCount(); ++ii)
			{
				XmlNodeRef nodeObj = objinfo->getChild(ii);
				assert(nodeObj->isTag("Object"));

				GUID guidObj;
				nodeObj->getAttr("GUID", guidObj);
				if ( !guidsetObjs.insert(guidObj).second )	// we already have this object
					continue;

				GUID guidLayer;
				nodeObj->getAttr("LayerGUID", guidLayer);

				XmlNodeRef layerNode = layermapping[guidLayer];
				if (!layerNode)		
					layerNode = defaultLayer;			// if no such layer in layermap, use default layer
				if (!layerNode)
					continue;							// if still no, leave it

				if (idxRoot == SourceData_Src)
				{
					// skip not needed and offset positions of those needed
					Recti rectObj;
					Vec2 tmpwc;
					nodeObj->getAttr("wcBoxMin", tmpwc);
					rectObj.Min.set((int)tmpwc.x, (int)tmpwc.y);
					nodeObj->getAttr("wcBoxMax", tmpwc);
					rectObj.Max.set((int)tmpwc.x, (int)tmpwc.y);

					nodeObj->getAttr("wc", tmpwc);
					Vec2i mainwc((int)tmpwc.x, (int)tmpwc.y);

					if (!rectObj.Intersects(GetRectSrc()) && !GetRectSrc().InRect(mainwc))
						continue;															// skip if not in range

					nodeObj->setAttr("wc", Vec2(GetDiffSrcToDest().x + mainwc.x, GetDiffSrcToDest().y + mainwc.y));

					nodeObj->setAttr("wcBoxMin", Vec2(GetDiffSrcToDest().x + rectObj.Min.x, GetDiffSrcToDest().y + rectObj.Min.y) );
					nodeObj->setAttr("wcBoxMax", Vec2(GetDiffSrcToDest().x + rectObj.Max.x, GetDiffSrcToDest().y + rectObj.Max.y) );

					lstGuidNeedToCopy.push_back(guidObj);
				}

				nodeObj->setAttr("LayerID", layerNode->getAttr("ID"));
				nodeObj->setAttr("LayerName", layerNode->getChild(0)->getAttr("Name"));

				resultObjInfo->addChild(nodeObj);

				if (m_progressBar)
				{
					m_progressBar->Step(100 * m_nCurStep++ / m_nTotoalSteps);
				}
			}
		}
		XmlHelpers::SaveXmlNode(resultObjInfo, m_strDestPath + "/" SW_ObjectInfoFile);
	}
}

void CSegmentLevelMerger::CopyExternalObjectFiles( const TGuidList& lstGuidNeedToCopy )
{
	//////////////////////////////////////////////////////////////////////////
	// copy external object files
	{
		m_nTotoalSteps += 0 - m_nProgStageEstimate + lstGuidNeedToCopy.size();

		for (auto iter = lstGuidNeedToCopy.begin(); iter != lstGuidNeedToCopy.end(); ++iter)
		{
			CString strSrcObjFile = m_strSrcPath + "/"SW_ExternalObjectDirName"/" + GuidUtil::ToString(*iter);
			CString strDestObjFile = m_strDestPath + "/"SW_ExternalObjectDirName"/" + GuidUtil::ToString(*iter);
			CFileUtil::CopyFile( strSrcObjFile, strDestObjFile, true);

			if (m_progressBar)
			{
				m_progressBar->Step(100 * m_nCurStep++ / m_nTotoalSteps);
			}
		}
	}
}

void CSegmentLevelMerger::MergeGlobalTerrainLayersData()
{
	//////////////////////////////////////////////////////////////////////////
	// merge global terrain layers data
	{
		XmlNodeRef xmlResultST = XmlHelpers::CreateXmlNode("SurfaceTypes");
		XmlNodeRef xmlResultTL = XmlHelpers::CreateXmlNode("Layers");
		XmlNodeRef xmlSourcesRoot[SourceData_count];
		xmlSourcesRoot[SourceData_Dest] = XmlHelpers::LoadXmlFromFile(m_strDestPath + "/" + GetWorldDataFileName(WDB_TERRAIN_LAYERS));
		xmlSourcesRoot[SourceData_Src] = XmlHelpers::LoadXmlFromFile(m_strSrcPath + "/" + GetWorldDataFileName(WDB_TERRAIN_LAYERS));
		assert(xmlSourcesRoot[SourceData_Src]->isTag(GetWorldDataBlockName(WDB_TERRAIN_LAYERS)));
		assert(xmlSourcesRoot[SourceData_Dest]->isTag(GetWorldDataBlockName(WDB_TERRAIN_LAYERS)));

		// Process Surface Types
		{
			typedef std::set<string> TStringSet;
			TStringSet setSurfaceTypeName;

			XmlNodeRef xmlSources[SourceData_count];
			xmlSources[SourceData_Dest] = xmlSourcesRoot[SourceData_Dest]->findChild("SurfaceTypes");
			xmlSources[SourceData_Src] = xmlSourcesRoot[SourceData_Src]->findChild("SurfaceTypes");

			m_nTotoalSteps += 0 - m_nProgStageEstimate + xmlSources[SourceData_Dest]->getChildCount() + xmlSources[SourceData_Src]->getChildCount();

			uint32 idDataNew = 0;
			for (size_t idxRoot = 0; idxRoot < SourceData_count; ++idxRoot)
			{
				XmlNodeRef xmlDataset = xmlSources[idxRoot];
				for(size_t ii = 0; ii < xmlDataset->getChildCount(); ++ii)
				{
					XmlNodeRef xmlData = xmlDataset->getChild(ii);
					assert(xmlData->isTag("SurfaceType"));

					CString strKey;
					xmlData->getAttr("Name",strKey);

					if (!setSurfaceTypeName.insert(string((const char*)strKey)).second)
						continue;

					xmlData->setAttr("SurfaceTypeID", idDataNew++);
					xmlResultST->addChild(xmlData);

					if (m_progressBar)
					{
						m_progressBar->Step(100 * m_nCurStep++ / m_nTotoalSteps);
					}
				}
			}
		}

		// Process Terrain Layers
		{
			typedef uint32 TBitfield[256/32];				// 255 is the biggest layerId that we can have
			typedef std::set<GUID, guid_less_predicate> TGuidSet;
			TGuidSet setGuids;
			TBitfield bitsLayerId = {};

			XmlNodeRef xmlSources[SourceData_count];
			xmlSources[SourceData_Dest] = xmlSourcesRoot[SourceData_Dest]->findChild("Layers");
			xmlSources[SourceData_Src] = xmlSourcesRoot[SourceData_Src]->findChild("Layers");

			m_nTotoalSteps += 0 - m_nProgStageEstimate + xmlSources[SourceData_Dest]->getChildCount() + xmlSources[SourceData_Src]->getChildCount();

			uint32 idDataNew = 0;
			for (size_t idxRoot = 0; idxRoot < SourceData_count; ++idxRoot)
			{
				XmlNodeRef xmlDataset = xmlSources[idxRoot];
				for(size_t ii = 0; ii < xmlDataset->getChildCount(); ++ii)
				{
					XmlNodeRef xmlData = xmlDataset->getChild(ii);
					assert(xmlData->isTag("Layer"));

					GUID guidKey;
					xmlData->getAttr("GUID",guidKey);
					if (!setGuids.insert(guidKey).second)
						continue;

					uint32 idLayer;
					xmlData->getAttr("LayerId", idLayer);
					if (bitsLayerId[idLayer])		// layerId already taken, find a new one
					{
						uint32 idLayerNew;
						size_t jj = 0;
						for (; jj < _countof(bitsLayerId); ++jj )
						{
							uint32 input = bitsLayerId[jj];
							if (input == 0xFFffFFff)
								continue;

							uint32 idx = 0;
							while (input & (1<<idx));
							idLayerNew = jj*32 + idx;
							break;
						}
						if (jj >= _countof(bitsLayerId))
						{
							SWLog("Total terrain layer exceeded the maximum 256!");
							break;
						}
						xmlData->setAttr("LayerId", idLayerNew);
					}
					xmlResultTL->addChild(xmlData);

					if (m_progressBar)
					{
						m_progressBar->Step(100 * m_nCurStep++ / m_nTotoalSteps);
					}
				}
			}
		}

		XmlNodeRef resultFileWrapper = XmlHelpers::CreateXmlNode(GetWorldDataBlockName(WDB_TERRAIN_LAYERS));
		resultFileWrapper->addChild(xmlResultST);
		resultFileWrapper->addChild(xmlResultTL);
		XmlHelpers::SaveXmlNode(resultFileWrapper, m_strDestPath + "/" + GetWorldDataFileName(WDB_TERRAIN_LAYERS));
	}
}

void CSegmentLevelMerger::MergeGlobalVegetationData()
{
	//////////////////////////////////////////////////////////////////////////
	{
		typedef std::set<GUID, guid_less_predicate> TGuidSet;
		TGuidSet	guidsetObjs;
		// merge global vegetation data
		{
			XmlNodeRef xmlResult = XmlHelpers::CreateXmlNode("Objects");
			XmlNodeRef xmlSources[SourceData_count];
			xmlSources[SourceData_Dest] = XmlHelpers::LoadXmlFromFile(m_strDestPath + "/" + GetWorldDataFileName(WDB_VEGETATION));
			xmlSources[SourceData_Src] = XmlHelpers::LoadXmlFromFile(m_strSrcPath + "/" + GetWorldDataFileName(WDB_VEGETATION));
			xmlSources[SourceData_Dest] = xmlSources[SourceData_Dest]->getChild(0)->getChild(0);
			xmlSources[SourceData_Src] = xmlSources[SourceData_Src]->getChild(0)->getChild(0);

			m_nTotoalSteps += 0 - m_nProgStageEstimate + xmlSources[SourceData_Dest]->getChildCount() + xmlSources[SourceData_Src]->getChildCount();

			uint32 idDataNew = 0;
			for (size_t idxRoot = 0; idxRoot < SourceData_count; ++idxRoot)
			{
				XmlNodeRef xmlDataset = xmlSources[idxRoot];
				assert(xmlDataset->isTag("Objects"));
				for(size_t ii = 0; ii < xmlDataset->getChildCount(); ++ii)
				{
					XmlNodeRef xmlData = xmlDataset->getChild(ii);
					assert(xmlData->isTag("Object"));

					GUID guidData;
					xmlData->getAttr("GUID",guidData);

					if (!guidsetObjs.insert(guidData).second) // already have same guid
						continue;

					// regenerate id
					xmlData->setAttr("Id",idDataNew++);
					xmlResult->addChild(xmlData);

					if (m_progressBar)
					{
						m_progressBar->Step(100 * m_nCurStep++ / m_nTotoalSteps);
					}
				}
			}

			XmlNodeRef resultFileWrapper = XmlHelpers::CreateXmlNode(GetWorldDataBlockName(WDB_VEGETATION));
			resultFileWrapper->newChild("VegetationMap")->addChild(xmlResult);
			XmlHelpers::SaveXmlNode(resultFileWrapper, m_strDestPath + "/" + GetWorldDataFileName(WDB_VEGETATION));
		}

		// remap vegetation object id in every instance
		{
			// the id is never used for loading, and instances are mapped to VegObject by GUID, so we don't need to remap at all.
		}
	}
}

void CSegmentLevelMerger::MergeLevelPak()
{
	XmlNodeRef xmlResultMission = NULL, xmlResultLevelData = NULL, xmlResultLevelInfo = NULL;
	CString srcMissionFile, destMissionFile, srcLevelDataFile, destLevelDataFile, destLevelInfoFile;
	srcMissionFile.Format("%s/mission_mission0.xml", m_strSrcPath);
	destMissionFile.Format("%s/mission_mission0.xml", m_strDestPath);
	srcLevelDataFile.Format("%s/leveldata.xml", m_strSrcPath);
	destLevelDataFile.Format("%s/leveldata.xml", m_strDestPath);
	destLevelInfoFile.Format("%s/"SW_SegmentedLevelInfo, m_strDestPath);

	if(gEnv->pCryPak->OpenPack(m_strSrcPath + "/"SW_LevelPakFile) && gEnv->pCryPak->OpenPack(m_strDestPath + "/"SW_LevelPakFile))
	{
		// process global entities
		{
			XmlNodeRef xmlSrcMission = XmlHelpers::LoadXmlFromFile(srcMissionFile);
			xmlResultMission = XmlHelpers::LoadXmlFromFile(destMissionFile);

			if(xmlSrcMission && xmlResultMission)
			{
				XmlNodeRef srcObjRoot = xmlSrcMission->findChild("Objects");
				XmlNodeRef destObjRoot = xmlResultMission->findChild("Objects");
				if(srcObjRoot && destObjRoot)
				{
					for(int i = 0; i < srcObjRoot->getChildCount(); i++)
					{
						XmlNodeRef obj = srcObjRoot->getChild(i);

						Vec2 tmpwc;
						obj->getAttr("CoordInSW",tmpwc);
						Vec2i wcSrc((int)tmpwc.x, (int)tmpwc.y);

						if (!GetRectSrc().InRect(wcSrc))
							continue;

						tmpwc += GetDiffSrcToDest();
						obj->setAttr("CoordInSW",tmpwc);

						destObjRoot->addChild(obj);
					}
				}
			}
		}

		// process surface types 
		{
			std::set<string> surfaceTypeNames;

			XmlNodeRef xmlSourcesRoot[SourceData_count];
			xmlSourcesRoot[SourceData_Src] = XmlHelpers::LoadXmlFromFile(srcLevelDataFile);
			xmlSourcesRoot[SourceData_Dest] = XmlHelpers::LoadXmlFromFile(destLevelDataFile);
			xmlResultLevelData = XmlHelpers::LoadXmlFromFile(destLevelDataFile);

			if(xmlSourcesRoot[SourceData_Src] && xmlSourcesRoot[SourceData_Dest])
			{
				XmlNodeRef xmlSources[SourceData_count];
				xmlSources[SourceData_Src] = xmlSourcesRoot[SourceData_Src]->findChild("SurfaceTypes");
				xmlSources[SourceData_Dest] = xmlSourcesRoot[SourceData_Dest]->findChild("SurfaceTypes");

				uint32 idDataNew = 0;
				if(xmlSources[SourceData_Src] && xmlSources[SourceData_Dest])
				{
					XmlNodeRef xmlResultST = XmlHelpers::CreateXmlNode("SurfaceTypes");
					for(int j = 0; j < SourceData_count; j++)
					{
						XmlNodeRef xmlDataset = xmlSources[j];
						for(int i = 0; i < xmlDataset->getChildCount(); i++)
						{
							XmlNodeRef xmlData = xmlDataset->getChild(i);
							assert(xmlData->isTag("SurfaceType"));

							CString stName;
							xmlData->getAttr("Name", stName);

							if (!surfaceTypeNames.insert(string((const char*)stName)).second)
								continue;

							xmlData->setAttr("SurfaceTypeID", idDataNew++);
							xmlResultST->addChild(xmlData);
						}
					}
					xmlResultLevelData->removeChild(xmlResultLevelData->findChild("SurfaceTypes"));
					xmlResultLevelData->addChild(xmlResultST);
				}
			}
		}
		 
		// process world bounding box
		if(auto pSWDoc = GetInternalSWDoc())
		{
			Recti rcBoundsDest = pSWDoc->RcWorldBounds();

			rcBoundsDest.DoUnite(GetRectDest());	// just extend it with the merged area

			// update current world boundary
			GetInternalSWDoc()->RcWorldBounds() = Recti(rcBoundsDest.Min.x, rcBoundsDest.Min.y, rcBoundsDest.Max.x, rcBoundsDest.Max.y);

			// update world boundary to dest level pak
			xmlResultLevelInfo = XmlHelpers::LoadXmlFromFile(destLevelInfoFile);
			if(xmlResultLevelInfo)
			{
				xmlResultLevelInfo->setAttr( "SegmentedWorldMinX", rcBoundsDest.Min.x );
				xmlResultLevelInfo->setAttr( "SegmentedWorldMinY", rcBoundsDest.Min.y );
				xmlResultLevelInfo->setAttr( "SegmentedWorldMaxX", rcBoundsDest.Max.x );
				xmlResultLevelInfo->setAttr( "SegmentedWorldMaxY", rcBoundsDest.Max.y );
			}
		}

		gEnv->pCryPak->ClosePack(m_strDestPath + "/"SW_LevelPakFile);
		gEnv->pCryPak->ClosePack(m_strSrcPath + "/"SW_LevelPakFile);
	}

	CPakFile destPak;
	if(destPak.Open(m_strDestPath + "/"SW_LevelPakFile, true))
	{
		XmlString xmlString;
		if(xmlResultMission)
		{
			xmlString = xmlResultMission->getXML();
			destPak.GetArchive()->UpdateFile(destMissionFile, (void *)xmlString.data(), xmlString.size());
		}
		if(xmlResultLevelData)
		{
			xmlString = xmlResultLevelData->getXML();
			destPak.GetArchive()->UpdateFile(destLevelDataFile, (void *)xmlString.data(), xmlString.size());
		}
		if(xmlResultLevelInfo)
		{
			xmlString = xmlResultLevelInfo->getXML();
			destPak.GetArchive()->UpdateFile(destLevelInfoFile, (void *)xmlString.data(), xmlString.size());
		}
		destPak.Close();
	}
}

void CSegmentLevelMerger::Process( bool bWithGuiReport )
{
	enum
	{
		nProgressStageCount = 9
	};

	if (bWithGuiReport)
	{
		m_nTotoalSteps = nProgressStageCount * (m_vSize.x * m_vSize.y) +1;

		m_progressBar.reset(new CWaitProgress(_T("Merging Segments ...")));
		m_progressBar->Start();
		m_progressBar->Step(100 * m_nCurStep++ / m_nTotoalSteps);
	}

	m_strMainLayerPattern = CString("%s/"SW_EditorDataDirPath"%s/"LAYER_PATH"Main.lyr");
	m_strSegInfoPattern = CString("%s/"SW_EditorDataDirPath"%s/"SW_SegmentInfoFile);

	XmlArray arrMainLayerFragments;	// keep world coord, because holes are allowed
	XmlArray arrSegInfoFragments;
	CacheXmlArray(arrMainLayerFragments, arrSegInfoFragments);
	CopyExtobjFolder();
	CopySegeditorFolders();
	CopySegFolders();

	XmlNodeRef defaultLayer;
	std::vector<GUID> otherMainLayerGUIDs;
	TGuidXmlNodeMap layermapping;
	MergeLayerInfoFile(defaultLayer, otherMainLayerGUIDs, layermapping);

	GUID guidDefMainLayer;
	assert(0 == stricmp(defaultLayer->findChild("LayerAttribs")->getAttr("FullName"), "Main"));
	bool bRetval = defaultLayer->findChild("LayerAttribs")->getAttr("GUID", guidDefMainLayer);
	assert(bRetval);
	UpdateGUIDForMainLayerFiles(arrMainLayerFragments, guidDefMainLayer);
	UpdateGUIDForSegInfoFiles(arrSegInfoFragments, otherMainLayerGUIDs, guidDefMainLayer);

	MergeGlobalTerrainLayersData();
	MergeGlobalVegetationData();
	MergeLevelPak();

	if (bWithGuiReport)
	{
		m_progressBar->Stop();
	}
}