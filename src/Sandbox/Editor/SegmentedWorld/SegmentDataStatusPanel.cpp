#include "StdAfx.h"
#include "SegmentDataStatusPanel.h"
#include "InternalCommon.h"
#include "GameEngine.h"
#include "Objects/ObjectLayerManager.h"
#include <ISourceControl.h>

using namespace sw;

string GetStatusText(int nWDBState, const string &strLockedBy)
{
	string strText;

	//switch (nWDBState)
	//{
	//case sw::WDBState_Default:
	//	strText.Format("Not Locked");
	//	break;
	//case sw::WDBState_Missing:
	//	strText.Format("Missing");
	//	break;
	//case sw::WDBState_LockedByMe:
	//	strText.Format("Locked By me %s", strLockedBy.c_str());
	//	break;
	//case sw::WDBState_LockedByOther:
	//	strText.Format("Locked By %s", strLockedBy.c_str());
	//	break;
	//case sw::WDBState_Modified:
	//	strText.Format("Modified [Lock: %s]", strLockedBy.c_str());
	//	break;
	//case sw::WDBState_OutOfDate:
	//	strText.Format("OUT OF DATE [Lock: %s]", strLockedBy.c_str());
	//	break;
	//case sw::WDBState_Conflicted:
	//	strText.Format("CONFLICTED [Lock: %s(]", strLockedBy.c_str());
	//	break;
	//case sw::WDBState_Unknown:
	//default:
	//	strText.Format("Unknown");
	//	break;
	//}

	return strText;
}

//////////////////////////////////////////////////////////////////////////
// CSegmentTerrainDataRecord

CSegmentTerrainDataRecord::CSegmentTerrainDataRecord(int row, int col)
{
	CString levelpath = GetIEditor()->GetGameEngine()->GetLevelPath();
	for(int i = SDB_SOURCEDATA_BEGIN; i < SDB_SOURCEDATA_END; i++)
	{
		m_filename[i].Format("%s\\" SW_EditorDataDirPath SW_SegmentDirPattern "\\%s%s", levelpath, row, col, GetSegDataBlockName((ESDBType)i), SW_SegmentFileExtension);
	}
}

void CSegmentTerrainDataRecord::CreateItems()
{
	SetName("TerrainData");
	CreateStdItems();
}

uint32 CSegmentTerrainDataRecord::GetFileSCMAttributes()
{
	uint32 nFileAttr = 0xffff;
	ISourceControl *pSourceControl = GetIEditor()->GetSourceControl();

	for(int i = SDB_SOURCEDATA_BEGIN; i < SDB_SOURCEDATA_END; i++)
	{
		if(CFileUtil::FileExists(m_filename[i]))
			nFileAttr &= pSourceControl->GetFileAttributes(m_filename[i]);
	}
	return nFileAttr;
}

void CSegmentTerrainDataRecord::GetFilename(std::vector<CString> &filenames) const
{
	for(int i = SDB_SOURCEDATA_BEGIN; i < SDB_SOURCEDATA_END; i++)
	{
		if(CFileUtil::FileExists(m_filename[i]))
			filenames.push_back(m_filename[i]);
	}
}

//////////////////////////////////////////////////////////////////////////
// CSegmentLayerRecord

CSegmentLayerDataRecord::CSegmentLayerDataRecord(int row, int col)
{
	CString levelpath = GetIEditor()->GetGameEngine()->GetLevelPath();
	m_metadata.Format("%s\\" SW_EditorDataDirPath SW_SegmentDirPattern "\\%s", levelpath, row, col, SW_SegmentInfoFile);
}

void CSegmentLayerDataRecord::InitFilename(std::vector<CString> &layerNames, int row, int col)
{
	CString filename;
	CString levelpath = GetIEditor()->GetGameEngine()->GetLevelPath();
	for(int i = 0; i < layerNames.size(); i++)
	{
		filename.Format("%s\\" SW_EditorDataDirPath SW_SegmentDirPattern "\\%s%s%s", levelpath, row, col, LAYER_PATH, layerNames[i], LAYER_FILE_EXTENSION);
		m_filename.push_back(filename);
	}
}

void CSegmentLayerDataRecord::CreateItems()
{
	SetName("ObjectLayerData");
	CreateStdItems();
}

uint32 CSegmentLayerDataRecord::GetFileSCMAttributes()
{
	uint32 nFileAttr = 0xffff;
	ISourceControl *pSourceControl = GetIEditor()->GetSourceControl();

	nFileAttr &= pSourceControl->GetFileAttributes(m_metadata);
	for(int i = 0; i < m_filename.size(); i++)
	{
		if(CFileUtil::FileExists(m_filename[i]))
			nFileAttr &= pSourceControl->GetFileAttributes(m_filename[i]);
	}
	return nFileAttr;
}

void CSegmentLayerDataRecord::GetFilename(std::vector<CString> &filenames) const
{
	filenames.push_back(m_metadata);
	for(int i = 0; i < m_filename.size(); i++)
	{
		if(CFileUtil::FileExists(m_filename[i]))
			filenames.push_back(m_filename[i]);
	}
}

//////////////////////////////////////////////////////////////////////////
// CSegmentDataRecord

CSegmentDataRecord::CSegmentDataRecord(int row, int col)
: m_row(row)
, m_col(col)
{
	CString levelpath = GetIEditor()->GetGameEngine()->GetLevelPath();
	m_filename.Format("%s\\" SW_EditorDataDirPath SW_SegmentDirPattern "\\%s", levelpath, row, col, SW_SegmentMainFile);
	m_child.clear();
}

void CSegmentDataRecord::CreateItems()
{
	CString segname;
	segname.Format(SW_SegmentDirPattern, m_row, m_col);
	SetName(segname);
	CreateStdItems();
}

uint32 CSegmentDataRecord::GetFileSCMAttributes()
{
	ISourceControl *pSourceControl = GetIEditor()->GetSourceControl();

	return pSourceControl->GetFileAttributes(m_filename);
}

void CSegmentDataRecord::GetFilename(std::vector<CString> &filenames) const
{
	CString path = Path::GetPath(m_filename);
	filenames.push_back(path);
}

void CSegmentDataRecord::AddChild(CSLBaseItemRecord *pChild)
{
	if(!pChild)
		return;

	m_child.push_back(pChild);
}

void CSegmentDataRecord::AddRecordToTree(CTreeCtrlReport *pTree)
{
	if(!pTree)
		return;

	CreateItems();
	pTree->AddTreeRecord(this, 0);

	for(int i = 0; i < m_child.size(); i++)
	{
		m_child[i]->CreateItems();
		pTree->AddTreeRecord(m_child[i], this);
	}
}

//////////////////////////////////////////////////////////////////////////
// CSegmentDataStatusPanel

CSegmentDataStatusPanel::CSegmentDataStatusPanel(CWnd *pParent)
: CSLDataPanel(CSegmentDataStatusPanel::IDD, pParent)
{
	Create(IDD, pParent);
}

BOOL CSegmentDataStatusPanel::OnInitDialog()
{
	CSLDataPanel::OnInitDialog();

	return TRUE;
}

void CSegmentDataStatusPanel::UpdateEntries(Recti &rcWorld)
{
	m_tree.BeginUpdate();
	m_tree.DeleteAllItems();
	
	CString levelpath = GetIEditor()->GetGameEngine()->GetLevelPath();
	CString layerinfo;
	layerinfo.Format("%s\\%s", levelpath, SW_LayerInfoFile);

	std::vector<CString> layerNames;
	XmlNodeRef xml = XmlHelpers::LoadXmlFromFile(layerinfo);
	if(xml && xml->isTag("Layers"))
	{
		for(int i = 0; i < xml->getChildCount(); i++)
		{
			XmlNodeRef layer = xml->getChild(i);
			assert(layer && layer->isTag("Layer"));

			XmlNodeRef attribs = layer->getChild(0);
			assert(attribs && attribs->isTag("LayerAttribs"));

			const char *pName = attribs->getAttr("Name");
			layerNames.push_back(pName);
		}
	}

	for(int col = rcWorld.Min.y; col < rcWorld.Max.y; col++)
		for(int row = rcWorld.Min.x; row < rcWorld.Max.x; row++)
		{
			CSegmentDataRecord *pRecord = new CSegmentDataRecord(row, col);
			pRecord->AddChild(new CSegmentTerrainDataRecord(row, col));

			CSegmentLayerDataRecord *pLayerDataRecord = new CSegmentLayerDataRecord(row, col);
			pLayerDataRecord->InitFilename(layerNames, row, col);
			pRecord->AddChild(pLayerDataRecord);

			pRecord->AddRecordToTree(&m_tree);
		}

	m_tree.EndUpdate();
	m_tree.Populate();
}