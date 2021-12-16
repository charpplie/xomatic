#include "StdAfx.h"
#include "INITGUID.H"
#include "GridMapTool.h"
#include "Viewport.h"
#include "SegmentDataStatusPanel.h"
#include "SegmentedWorldManager.h"
#include "Utilities.h"

//////////////////////////////////////////////////////////////////////////
SegmentSelectorType CSegmentSelectTool::m_currentSelectorType = eSelectorEdit;
CSegmentSelector CSegmentSelectTool::m_selector[eSelectorCount];

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CSegmentSelectTool,CEditTool)

//////////////////////////////////////////////////////////////////////////
CSegmentSelectTool::CSegmentSelectTool()
{
	m_panel = 0;

	m_selector[eSelectorEdit].type = eSelectorEdit;
	m_selector[eSelectorCheck].type = eSelectorCheck;
	m_selector[eSelectorMerge].type = eSelectorMerge;
	m_pSelector = &m_selector[eSelectorEdit];
	m_prevSelector = 0;

	m_pointerPos = Vec3(ZERO);
	m_segmentSize = 0;
	m_mouseHoveredArea = Recti(0, 0, 0, 0);
	m_selectedArea = Recti(0, 0, 0, 0);
	m_worldBoundary = Recti(0, 0, 0, 0);
}

CSegmentSelectTool::~CSegmentSelectTool()
{
}

void CSegmentSelectTool::SetExnernalUIPanel(CSegmentDataStatusPanel *pPanel)
{
	assert(pPanel);
	m_panel = pPanel;
}

void CSegmentSelectTool::Display(DisplayContext &dc)
{
	if(!m_segmentSize)
		return;

	IRenderAuxGeom *renderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
	SAuxGeomRenderFlags oldFlags = renderAuxGeom->GetRenderFlags();
	SAuxGeomRenderFlags renderFlags(oldFlags);

	{
		renderFlags.SetDepthTestFlag(e_DepthTestOff);
		renderAuxGeom->SetRenderFlags(renderFlags);

		dc.SetColor(1, 1, 1, 0.8f);
		dc.DrawSolidBox(Vec3(m_selectedArea.Min.x, m_selectedArea.Min.y, 0), Vec3(m_selectedArea.Max.x, m_selectedArea.Max.y, 0));

		dc.SetColor(0, 0, 1, 1);
		dc.DrawWireBox(Vec3(m_worldBoundary.Min.x, m_worldBoundary.Min.y, 0), Vec3(m_worldBoundary.Max.x, m_worldBoundary.Max.y, 0));

		dc.SetColor(0, 1, 0, 1);
		ISegmentedWorldDoc& swdoc = GetIEditor()->GetSegmentedWorldDoc();
		int iEditSegX = 0, iEditSegY = 0;
		UINT iEditSegW = 0, iEditSegH = 0;
		swdoc.GetOffsetInSegments(iEditSegX, iEditSegY);
		swdoc.GetSizeInSegments(iEditSegW, iEditSegH);
		dc.DrawWireBox(Vec3(iEditSegX * m_segmentSize, iEditSegY * m_segmentSize, 0), Vec3((iEditSegX + iEditSegW) * m_segmentSize, (iEditSegY + iEditSegH) * m_segmentSize, 0));

		dc.SetColor(1, 1, 0, 1);
		RectList::iterator it = m_removedAreas.begin();
		for(; it != m_removedAreas.end(); ++it)
		{
			dc.DrawWireBox(Vec3(it->Min.x, it->Min.y, 0), Vec3(it->Max.x, it->Max.y, 0));
		}
		
		dc.SetColor(1, 1, 1, 0.8f);
		dc.DrawWireBox(Vec3(m_mouseHoveredArea.Min.x, m_mouseHoveredArea.Min.y, 0), Vec3(m_mouseHoveredArea.Max.x, m_mouseHoveredArea.Max.y, 0));
	}

	renderAuxGeom->SetRenderFlags(oldFlags);
}

bool CSegmentSelectTool::MouseCallback(CViewport *view,EMouseEvent event,CPoint &point,int flags)
{
	bool bCollideWithTerrain = true;
	m_pointerPos = view->ViewToWorld(point, &bCollideWithTerrain, true);

	if(event == eMouseMove)
	{
		m_mouseHoveredArea = GetSelRectMt();
	}
	else if(event == eMouseLDown)
	{
		m_selectedArea = GetSelRectMt();
	}
	else if(event == eMouseLDblClick)
	{
		if(m_currentSelectorType == eSelectorEdit)
		{
			MoveToSelected();
		}
		else if(m_currentSelectorType == eSelectorMerge)
		{
			MergeToSelected();
			UpdateWorldBound();
		}
	}

	return true;
}

bool CSegmentSelectTool::OnKeyUp(CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags)
{
	if(nChar == VK_ESCAPE)
	{
		RestoreSelectorType();
	}

	return true;
}

Vec2i CSegmentSelectTool::GetSelSize()
{
	if(m_currentSelectorType == eSelectorMerge)
	{
		return m_pSelector->mergeSize;
	}

	return Vec2i(m_pSelector->selSize, m_pSelector->selSize);
}

Recti CSegmentSelectTool::GetSelRectMt()
{
	Vec3 ptWorldPosMt = m_pointerPos;
	float fHalfSizeMt = m_segmentSize * 0.5f;
	Vec2i selSizeMt = GetSelSize() * m_segmentSize;
	Vec2i halfSelSizeMt(selSizeMt.x / 2, selSizeMt.y / 2);
	Vec2i ptCenterMt;

	bool bOdd = GetSelSize().x % 2;
	if(!bOdd)
	{
		ptWorldPosMt.x += (ptWorldPosMt.x < 0) ? -fHalfSizeMt : fHalfSizeMt;
		ptWorldPosMt.y += (ptWorldPosMt.y < 0) ? -fHalfSizeMt : fHalfSizeMt;
	}

	ptCenterMt.x = (int)(ptWorldPosMt.x / m_segmentSize) * m_segmentSize;
	ptCenterMt.y = (int)(ptWorldPosMt.y / m_segmentSize) * m_segmentSize;
	if(bOdd)
	{
		ptCenterMt.x += (ptWorldPosMt.x < 0) ? -fHalfSizeMt : fHalfSizeMt;
		ptCenterMt.y += (ptWorldPosMt.y < 0) ? -fHalfSizeMt : fHalfSizeMt;
	}

	return Recti(ptCenterMt.x - halfSelSizeMt.x, ptCenterMt.y - halfSelSizeMt.y, ptCenterMt.x + halfSelSizeMt.x, ptCenterMt.y + halfSelSizeMt.y);
}

void CSegmentSelectTool::MoveToSelected()
{
	bool result = false;

	Recti rcWorld = m_selectedArea / m_segmentSize;
	if(rcWorld.IsEmpty())
		return;

	if(GetIEditor()->IsValidSegmentedWorldDoc())
	{
		auto& doc = GetIEditor()->GetSegmentedWorldDoc();
		Vec2i min;
		Vec2i max;
		doc.GetWorldBounds(&min, &max);
		if (rcWorld.Intersects(Recti(min, max)))
		{
			result = doc.MoveTo(rcWorld.Min.x, rcWorld.Min.y, rcWorld.GetWidth(), rcWorld.GetHeight());

			if(result)
				SetStatusText("Segments Successfully Loaded");
			else
				SetStatusText("Failed To Open Selected Segments");
		}
		else 
		{
			SetStatusText("Selected area has no Segments, Select area with Segments!");
		}
	}
}

void CSegmentSelectTool::PrepareForMerge()
{
	if(m_worldName.IsEmpty())
	{
		AfxMessageBox("Please open a segmented world level before merge!", MB_OK | MB_ICONERROR);
		return;
	}

	SetStatusText("Please select a segmented level for merge");
	
	if(!OpenWorldInteranl(m_mergeWorldName))
		return;

	if(!strcmpi(m_mergeWorldName, m_worldName))
	{
		AfxMessageBox("Cannot merge to self!", MB_OK | MB_ICONERROR);
		return;
	}

	Recti rc;
	CString levelpath = Path::GetGameFolder() + "\\Levels\\" + m_mergeWorldName + "\\";
	if (!GetIEditor()->GetSegmentedWorldManager()->GetWorldBounds(levelpath, rc))
	{
		AfxMessageBox("Failed to get world bounds from segmentinfo.xml!", MB_OK | MB_ICONERROR);
		return;
	}
	SetSelectorType(eSelectorMerge);
	m_pSelector->mergeSize = Vec2i(rc.GetWidth() + 1, rc.GetHeight() + 1);

	SetStatusText("Double click on the empty area to merge");
}

void CSegmentSelectTool::MergeToSelected()
{
	auto pSWMgr = GetIEditor()->GetSegmentedWorldManager();
	if(!pSWMgr->MergeLevel(m_worldName, m_mergeWorldName,
		Vec2i(m_selectedArea.Min.x / m_segmentSize, m_selectedArea.Min.y / m_segmentSize),
		Vec2i(ZERO),
		m_pSelector->mergeSize)
		)
	{
		SetStatusText("Failed To Merge Segments");
		return;
	}

	SetStatusText("Segments Successfully Merged");
	RestoreSelectorType();
}

void CSegmentSelectTool::RemoveSelected()
{
	if(m_currentSelectorType != eSelectorEdit)
		return;
	
	Recti rcSelected = m_selectedArea / m_segmentSize;
	if(rcSelected.IsEmpty())
		return;

	sw::EProceedMode eRemove = sw::PROCEED_ASK;
	eRemove = sw::AskForProceeding("Are you sure you want to delete selected segments?");
	if(eRemove != sw::PROCEED_YES)
		return;

	std::vector<CPoint> pts;
	int w = rcSelected.GetWidth();
	int h = rcSelected.GetHeight();
	for (int dy = 0; dy < h; ++dy)
	{
		for (int dx = 0; dx < w; ++dx)
		{
			CPoint pt(rcSelected.Min.x + dx, rcSelected.Min.y + dy);
			pts.push_back(pt);

		}
	}

	bool bRemoved = CSegmentedWorldManager::RemoveSegments(pts, m_worldName);
	if(bRemoved)
	{
		Recti rc;
		for(int i = 0; i < pts.size(); i++)
		{
			const CPoint &pt = pts[i];
			rc = Recti(pt.x, pt.y, pt.x + 1, pt.y + 1) * m_segmentSize;
			if(rc.Intersects(m_worldBoundary))
				m_removedAreas.push_back(rc);
		}
		SetStatusText("Segments Successfully Removed");
	}
	else
		SetStatusText("Failed To Removed Segments");
}

bool CSegmentSelectTool::OpenWorldInteranl(CString &outWorldName)
{
	CString docFilename;
	CString currentLevelsPath = Path::GetExecutableParentDirectory() + "\\" + Path::GetGameFolder() + "\\Levels";
	if(!CFileUtil::SelectFile("Segmented Cry Files (*.scry)|*.scry", currentLevelsPath, docFilename))
		return false;

	// Levels outside of Game/Levels won't be opened
	if (0 != docFilename.Left(currentLevelsPath.GetLength()).CompareNoCase(currentLevelsPath))
		return false;

	CString strRelative = docFilename.Mid(currentLevelsPath.GetLength()+1);
	outWorldName = strRelative.Left(strRelative.FindOneOf("/\\"));

	return true;
}

void CSegmentSelectTool::UpdateWorldBound()
{
	// get world bound from pak file
	Recti rcBound;
	CString levelpath = Path::GetGameFolder() + "\\Levels\\" + m_worldName + "\\";
	if(!GetIEditor()->GetSegmentedWorldManager()->GetWorldBounds(levelpath, rcBound))
	{
		SetStatusText("Failed to get world bounds from level pak");
		return;
	}
	rcBound.Max.x += 1;
	rcBound.Max.y += 1;

	// early return
	Recti newWorldBoundary = rcBound * m_segmentSize;
	if(newWorldBoundary.IsEqual(m_worldBoundary))
		return;

	// update removed areas
	m_removedAreas.clear();
	CSegmentedWorldManager::GetMissingSegmentsInRect(levelpath, rcBound, m_removedAreas, m_segmentSize);

	// update world boundary
	m_worldBoundary = newWorldBoundary;

	// update segment entries
	if(m_panel)
		m_panel->UpdateEntries(rcBound);
}

void CSegmentSelectTool::SetSelectorType(SegmentSelectorType type)
{
	m_prevSelector = m_pSelector;
	m_currentSelectorType = type;
	m_pSelector = &m_selector[type];
}

void CSegmentSelectTool::RestoreSelectorType()
{
	if(m_prevSelector)
	{
		m_pSelector = m_prevSelector;
		m_currentSelectorType = m_prevSelector->type;
		m_prevSelector = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
// Class description.
//////////////////////////////////////////////////////////////////////////
class CSegmentSelectTool_ClassDesc : public CRefCountClassDesc
{
	//! This method returns an Editor defined GUID describing the class this plugin class is associated with.
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_EDITTOOL; }

	//! Return the GUID of the class created by plugin.
	virtual REFGUID ClassID() 
	{
		return SEGMENT_SELECT_TOOL_GUID;
	}

	//! This method returns the human readable name of the class.
	virtual const char* ClassName() { return "EditTool.SegmentSelect"; };

	//! This method returns Category of this class, Category is specifing where this plugin class fits best in
	//! create panel.
	virtual const char* Category() { return "SegmentedWorld"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CSegmentSelectTool); }
	//////////////////////////////////////////////////////////////////////////
};

//////////////////////////////////////////////////////////////////////////
void CSegmentSelectTool::RegisterTool( CRegistrationContext &rc )
{
	rc.pClassFactory->RegisterClass(new CSegmentSelectTool_ClassDesc);
}