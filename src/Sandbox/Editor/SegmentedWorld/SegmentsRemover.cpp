
#include "StdAfx.h"
#include "SegmentsRemover.h"
#include "Utilities.h"
#include "CryEditDoc.h"
#include "SegmentedWorldManager.h"
#include "InternalCommon.h"
#include "DataUpgrade.h"
#include "SegmentedWorldDoc.h"

using namespace sw;

bool CSegmentsRemover::ValidateInput(const CString& strLevelPath, const std::vector<CPoint>& wcs)
{
	if (!CFileUtil::PathExists(strLevelPath))
	{
		SWLog("Level not exists!");
		return false;
	}

	if (GetIEditor()->GetDocument()->IsDocumentReady() && GetIEditor()->IsValidSegmentedWorldDoc()) // if a SW level is opened check if it conflicts with this operation
	{
		CString strCurLevel = Path::GamePathToFullPath(GetIEditor()->GetLevelFolder());
		Path::ConvertBackSlashToSlash(strCurLevel);

		if (0==strLevelPath.CompareNoCase(strCurLevel))
		{
			Recti rcLoaded = GetIEditor()->GetSegmentedWorldDoc().GetLoadedRect();
			for(int i = 0; i < wcs.size(); i++)
			{
				if (rcLoaded.InRect(Vec2i(wcs[i].x, wcs[i].y)))
				{
					SWLog("You cannot remove segments from a level which are currently opened for editing");
					return false;
				}
			}
		}
	}

	// Check data structure version
	if (CDataUpgrade::Get().DetectSWLevelVersion(strLevelPath) != CSegmentedWorldManager::GetSWDataStructureVersion())
	{
		CSegmentedWorldManager::SWMsgBox("The level is not in new format, please upgrade it first");
		return false;
	}

	return true;
}

void CSegmentsRemover::Process()
{
	RemoveFolders();

	std::set<GUID, guid_less_predicate> remainingLayers;
	UpdateSegmentInfoFile(remainingLayers);
	UpdateLayerInfoFile(remainingLayers);
}

void CSegmentsRemover::RemoveFolders()
{
	for	(std::vector<CPoint>::const_iterator iter = m_wcs.begin(); iter != m_wcs.end(); ++iter )
	{
		const CPoint& wc = *iter;
		// remove seg_editor folder
		{
			CString strSegPath;
			strSegPath.Format("%s/" SW_SegmentsDirPath SW_SegmentDirPattern "/", m_strLevelPath, wc.x, wc.y);
			Path::ConvertBackSlashToSlash(strSegPath);
			CFileUtil::Deltree(strSegPath, true);
		}

		// remove seg folder
		{
			CString strSegPath;
			strSegPath.Format("%s/" SW_EditorDataDirPath SW_SegmentDirPattern "/", m_strLevelPath, wc.x, wc.y);
			Path::ConvertBackSlashToSlash(strSegPath);
			CFileUtil::Deltree(strSegPath, true);
		}
	}
}

void CSegmentsRemover::UpdateSegmentInfoFile(std::set<GUID, guid_less_predicate>& remainingLayers)
{
	CString strFilename = m_strLevelPath + "/" SW_SegmentInfoFile;
	XmlNodeRef xmlSegInfo;
	xmlSegInfo = XmlHelpers::LoadXmlFromFile(strFilename);


	XmlNodeRef seglayers = xmlSegInfo->findChild("SegLayerMappings");
	for (size_t ii = 0; ii < seglayers->getChildCount(); ++ii)
	{
		XmlNodeRef nodeSeg = seglayers->getChild(ii);
		assert(nodeSeg->isTag("Segment"));

		Vec2 curwc;
		nodeSeg->getAttr("wc",curwc);
		if (std::find(m_wcs.begin(), m_wcs.end(), CPoint(curwc.x,curwc.y)) != m_wcs.end())
		{
			// remove segments
			seglayers->deleteChildAt(ii--);
		}
		else
		{
			// update remainings & recalculate world bounds
			GUID guidLayer;
			nodeSeg->getAttr("LayerGUID",guidLayer);
			remainingLayers.insert(guidLayer);			// meant to have many equal values

			Vec2 curwc;
			nodeSeg->getAttr("wc",curwc);
		}
	}

	enum{ RECT_INVALID_VALUE = 0x7fFFffFF };
	static const Recti _invalid_rect(RECT_INVALID_VALUE, RECT_INVALID_VALUE, RECT_INVALID_VALUE, RECT_INVALID_VALUE);
	Recti worldBounds = _invalid_rect;

	CString strSegs;
	strSegs.Format("%s/" SW_EditorDataDirPath "*.*" , m_strLevelPath);
	Path::ConvertBackSlashToSlash(strSegs);

	__finddata64_t		fd;
	intptr_t hFind = 0;
	if ((hFind = _findfirst64(strSegs, &fd)) != -1)
	{
		do
		{
			if (!(fd.attrib & _A_SUBDIR))
				continue;

			CString name = fd.name;

			int x,y;
			if (2 != sscanf(name, SW_SegmentDirPattern, &x, &y))
				continue;

			Recti rcCur(x, y, x + 1, y + 1);

			// if not initialized give it a initial value
			if ( worldBounds.IsEqual(_invalid_rect) )
				worldBounds = rcCur;
			else
				worldBounds.DoUnite(rcCur);

		} while(!_findnext64(hFind, &fd));

		_findclose(hFind);
	}

	if (!worldBounds.IsEqual(_invalid_rect))
	{
		worldBounds.Max.x -= 1;
		worldBounds.Max.y -= 1;

		if(GetIEditor()->GetSegmentedWorldManager()->GetDocI())
			GetIEditor()->GetSegmentedWorldManager()->GetDocI()->RcWorldBounds() = worldBounds;
	}

	CFileUtil::DeleteFile(strFilename);
	XmlHelpers::SaveXmlNode(xmlSegInfo, strFilename);
}

void CSegmentsRemover::UpdateLayerInfoFile(const std::set<GUID, guid_less_predicate>& remainingLayers )
{
	CString strFilename = m_strLevelPath + "/" SW_LayerInfoFile;

	XmlNodeRef xmlLayerInfo;
	xmlLayerInfo = XmlHelpers::LoadXmlFromFile(strFilename);
	assert(xmlLayerInfo->isTag("Layers"));

	for (size_t ii=0; ii < xmlLayerInfo->getChildCount(); ++ii)
	{
		XmlNodeRef nodeLayer = xmlLayerInfo->getChild(ii)->getChild(0);
		if (!nodeLayer)
			continue;
		assert(nodeLayer->isTag("LayerAttribs"));
		GUID guidLayer;
		nodeLayer->getAttr("GUID",guidLayer);

		// remove layers that are no longer needed
		if (remainingLayers.end() == remainingLayers.find(guidLayer))
			xmlLayerInfo->deleteChildAt(ii--);
	}

	CFileUtil::DeleteFile(strFilename);
	XmlHelpers::SaveXmlNode(xmlLayerInfo, strFilename);
}
