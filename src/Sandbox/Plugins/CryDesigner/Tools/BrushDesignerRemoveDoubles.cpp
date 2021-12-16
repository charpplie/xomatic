#include "StdAfx.h"
#include "BrushDesignerRemoveDoubles.h"
#include "BrushDesignerSelectTool.h"
#include "Core/BrushDesigner.h"
#include "BrushDesignerEditTool.h"
#include "IBaseToolPanel.h"

namespace 
{
	IRemoveDoubleToolPanel* s_pRemoveDoubleToolPanel = NULL;
}

void CBrushDesignerRemoveDoublesTool::BeginEditParams()
{
	if( !s_pRemoveDoubleToolPanel )
		s_pRemoveDoubleToolPanel = CreateRemoveDoubleToolPanel(this,(void*)GetPanelIndex());
}

void CBrushDesignerRemoveDoublesTool::EndEditParams()
{
	if( s_pRemoveDoubleToolPanel )
	{
		s_pRemoveDoubleToolPanel->DestroyPanel();
		s_pRemoveDoubleToolPanel = NULL;
	}
}

bool HasVertexInList( const std::vector<BrushVec3>& vList, const BrushVec3& vPos )
{
	for( int i = 0, iVListCount(vList.size()); i < iVListCount; ++i )
	{
		if( vList[i].IsEquivalent(vPos,kDesignerEpsilon) )
			return true;
	}
	return false;
}

void CBrushDesignerRemoveDoublesTool::Enter()
{
	CBrushDesignerBaseTool::Enter();
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();
	if( !pSelected->IsEmpty() )
	{
		CUndo undo("Designer : Remove Doubles");
		GetDesigner()->RecordUndo("Designer : Remove Doubles",GetBaseObject());

		RemoveDoubles(GetMainContext(),(float)s_pRemoveDoubleToolPanel->GetDistance());
		
		pSelected->Clear();
		CreateMirroredRegions(GetDesigner());
		UpdateBrush();
		Sync();
			
		GetEditTool()->GoToSelectDesignerMode();
	}
}

void CBrushDesignerRemoveDoublesTool::RemoveDoubles( BUtil::SMainContext& mc, float fDistance )
{
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	std::vector<BrushVec3> uniqueVertices;
	for( int i = 0, iSelectedElementCount(pSelected->GetSize()); i < iSelectedElementCount; ++i )
	{
		for( int k = 0, iVertexCount((*pSelected)[i].m_Vertices.size()); k < iVertexCount; ++k )
		{
			if( !HasVertexInList(uniqueVertices,(*pSelected)[i].m_Vertices[k]) )
				uniqueVertices.push_back((*pSelected)[i].m_Vertices[k]);
		}
	}

	int iVertexCount(uniqueVertices.size());
	BrushVec3 vTargetPos;
	std::set<int> alreadyUsedVertices;	

	while( alreadyUsedVertices.size() < iVertexCount )
	{
		for( int i = iVertexCount-1; i >= 0; --i )
		{
			if( alreadyUsedVertices.find(i) == alreadyUsedVertices.end() )
			{
				vTargetPos = uniqueVertices[i];
				alreadyUsedVertices.insert(i);
				break;
			}
		}

		for( int i = 0; i < iVertexCount; ++i )
		{
			if( alreadyUsedVertices.find(i) != alreadyUsedVertices.end() )
				continue;
			if( vTargetPos.GetDistance(uniqueVertices[i]) < fDistance )
			{
				Weld(mc,uniqueVertices[i],vTargetPos);
				alreadyUsedVertices.insert(i);
			}
		}
	}
}