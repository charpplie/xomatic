#include "StdAfx.h"
#include "BrushDesignerSubdivisionTool.h"
#include "Core/BrushDesigner.h"
#include "Core/BrushDesignerEdgesSharpnessManager.h"
#include "Core/BrushDesignerElementManager.h"
#include "Tools/BrushDesignerEditTool.h"
#include "Objects/DesignerBrushObject.h"
#include "IBaseToolPanel.h"

namespace
{
	IBaseToolPanel* s_pSubdivisionPanel = NULL;
}

void CBrushDesignerSubdivisionTool::Enter()
{
	__super::Enter();
	GetDesigner()->ClearExcludedEdgesInDrawing();
	m_SelectedEdgesAsEnter.clear();
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	for( int i = 0, iSize(pSelected->GetSize()); i < iSize; ++i )
	{
		const SDesignerElement& element = pSelected->Get(i);
		if( element.IsEdge() )
		{
			GetDesigner()->AddExcludedEdgeInDrawing(element.GetEdge());
			m_SelectedEdgesAsEnter.push_back(element.GetEdge());
		}
		else if( element.IsFace() && element.m_pRegion )
		{
			for( int k = 0, iEdgeCount(element.m_pRegion->GetEdgeSize()); k < iEdgeCount; ++k )
			{
				const BrushEdge3D& edge = element.m_pRegion->GetEdge(k);
				GetDesigner()->AddExcludedEdgeInDrawing(edge);
				m_SelectedEdgesAsEnter.push_back(edge);
			}
		}
	}
	pSelected->Clear();
}

void CBrushDesignerSubdivisionTool::Leave()
{
	__super::Leave();
	m_HighlightedSharpEdges.clear();
	GetDesigner()->ClearExcludedEdgesInDrawing();
}

void CBrushDesignerSubdivisionTool::BeginEditParams()
{
	if( !s_pSubdivisionPanel )
		s_pSubdivisionPanel = CreateSubdivisionToolPanel(this,(void*)GetPanelIndex());			
}

void CBrushDesignerSubdivisionTool::EndEditParams()
{
	if( s_pSubdivisionPanel )
	{
		s_pSubdivisionPanel->DestroyPanel();
		s_pSubdivisionPanel = NULL;
	}
}

void CBrushDesignerSubdivisionTool::Subdivide( int nLevel, bool bUpdateBrush )
{
	CSelectionGroup* pSelection = GetIEditor()->GetSelection();
	for( int i = 0, iCount(pSelection->GetCount()); i < iCount; ++i )
	{
		CBaseObject* pObj = pSelection->GetObject(i);
		if( !pObj->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
			continue;
		CDesignerBrushObject* pDesignerObj = (CDesignerBrushObject*)pObj;
		pDesignerObj->GetDesigner()->SetSubdivisionLevel(nLevel);
		if( bUpdateBrush )
			pDesignerObj->UpdateBrush();
	}
}

void CBrushDesignerSubdivisionTool::HighlightEdgeGroup( const char* edgeGroupName )
{
	CBrushDesignerEdgeSharpnessManager* pEdgeSharpnessMgr = GetDesigner()->GetEdgeSharpnessMgr();
	BUtil::SEdgeSharpness* pEdgeSharpness = pEdgeSharpnessMgr->FindEdgeSharpness(edgeGroupName);
	if( pEdgeSharpness == NULL )
	{
		m_HighlightedSharpEdges.clear();
		GetDesigner()->ClearExcludedEdgesInDrawing();
		return;
	}
	m_HighlightedSharpEdges = pEdgeSharpness->edges;
	for( int i = 0, iSize(m_HighlightedSharpEdges.size()); i < iSize; ++i )	
		GetDesigner()->AddExcludedEdgeInDrawing(m_HighlightedSharpEdges[i]);	
}

void CBrushDesignerSubdivisionTool::AddNewEdgeTag()
{
	CBrushDesignerEdgeSharpnessManager* pEdgeSharpnessMgr = GetDesigner()->GetEdgeSharpnessMgr();
	string newEdgeGroupName = pEdgeSharpnessMgr->GenerateValidName();
	pEdgeSharpnessMgr->AddEdges(newEdgeGroupName,m_SelectedEdgesAsEnter,1);
	HighlightEdgeGroup(newEdgeGroupName);
	m_SelectedEdgesAsEnter.clear();
	UpdateBrush();
}

void CBrushDesignerSubdivisionTool::DeleteEdgeTag( const char* name )
{
	if( name == NULL )
		return;
	CBrushDesignerEdgeSharpnessManager* pEdgeSharpnessMgr = GetDesigner()->GetEdgeSharpnessMgr();
	pEdgeSharpnessMgr->RemoveEdgeSharpness(name);
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();		
	pSelected->Clear();
	UpdateBrush();
}

void CBrushDesignerSubdivisionTool::Display( DisplayContext &dc )
{
	dc.SetLineWidth(7);
	dc.SetColor(ColorB(150,255,50,255));
	for( int i = 0, iCount(m_HighlightedSharpEdges.size()); i < iCount; ++i )
		dc.DrawLine(m_HighlightedSharpEdges[i].m_v[0],m_HighlightedSharpEdges[i].m_v[1]);
	dc.DepthTestOn();

	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	dc.SetColor(BUtil::kSelectedColor);
	dc.SetLineWidth(BUtil::kChosenLineThickness);
	for( int i = 0, iCount(m_SelectedEdgesAsEnter.size()); i < iCount; ++i )
		dc.DrawLine(m_SelectedEdgesAsEnter[i].m_v[0],m_SelectedEdgesAsEnter[i].m_v[1]);
}