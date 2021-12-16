#include "StdAfx.h"
#include "Util/BoostPythonHelpers.h"
#include "Objects/DesignerBrushObject.h"
#include "Tools/BrushDesignerEditTool.h"
#include "Tools/BrushDesignerSelectTool.h"
#include "Tools/BrushDesignerSelectGrowTool.h"
#include "Tools/BrushDesignerInvertSelectionTool.h"
#include "Tools/BrushDesignerSelectAllNoneTool.h"
#include "Tools/BrushDesignerSelectConnectedTool.h"
#include "Tools/BrushDesignerLoopSelectionTool.h"
#include "Tools/BrushDesignerRingSelectionTool.h"
#include "Tools/BrushDesignerSelectAllNoneTool.h"
#include "Core/BrushDesignerElementManager.h"
#include "PythonBrushDesignerBasic.h"

namespace BPython
{
	CBrushDesignerEditTool* GetDesignerEditTool()
	{
		CEditTool* pEditTool = GetIEditor()->GetEditTool();
		if( pEditTool->IsKindOf(RUNTIME_CLASS(CBrushDesignerEditTool)) )
			return (CBrushDesignerEditTool*)pEditTool;
		return NULL;
	}

	void PyDesignerSelect( ElementID nElementKeyID )
	{
		DesignerElementsPtr pElements = s_bdpc.FindElements(nElementKeyID);
		if( pElements == NULL )
			throw std::logic_error("The element doesn't exist.");
		BUtil::SMainContext mc(GetContext());
		mc.pSelected->Add(*pElements);
		UpdateSelection(mc);
	}

	void PyDesignerDeselect( ElementID nElementKeyID )
	{
		DesignerElementsPtr pElements = s_bdpc.FindElements(nElementKeyID);
		if( pElements == NULL )
			throw std::logic_error("The element doesn't exist.");
		BUtil::SMainContext mc(GetContext());
		mc.pSelected->Erase(*pElements);
		UpdateSelection(mc);
	}

	void PyDesignerSetSelection( ElementID nElementKeyID )
	{
		DesignerElementsPtr pElements = s_bdpc.FindElements(nElementKeyID);
		if( pElements == NULL )
			throw std::logic_error("The element doesn't exist.");
		BUtil::SMainContext mc(GetContext());
		mc.pSelected->Clear();
		mc.pSelected->Set(*pElements);
		UpdateSelection(mc);
	}

	void PyDesignerSelectAllVertices()
	{
		BUtil::SMainContext mc(GetContext());
		CBrushDesignerSelectAllNoneTool::SelectAllVertices(mc.pObject,mc.pDesigner);
		UpdateSelection(mc);
		if( GetDesignerEditTool() )
			GetDesignerEditTool()->SetDesignerMode(BUtil::eDesigner_Select_Vertex);
	}

	void PyDesignerSelectAllEdges()
	{
		BUtil::SMainContext mc(GetContext());
		CBrushDesignerSelectAllNoneTool::SelectAllEdges(mc.pObject,mc.pDesigner);
		UpdateSelection(mc);
		if( GetDesignerEditTool() )
			GetDesignerEditTool()->SetDesignerMode(BUtil::eDesigner_Select_Edge);
	}

	void PyDesignerSelectAllFaces()
	{
		BUtil::SMainContext mc(GetContext());
		CBrushDesignerSelectAllNoneTool::SelectAllFaces(mc.pObject,mc.pDesigner);
		UpdateSelection(mc);
		if( GetDesignerEditTool() )
			GetDesignerEditTool()->SetDesignerMode(BUtil::eDesigner_Select_Face);
	}

	void PyDesignerDeselectAllVertices()
	{
		BUtil::SMainContext mc(GetContext());
		CBrushDesignerSelectAllNoneTool::DeselectAllVertices();
		UpdateSelection(mc);
	}

	void PyDesignerDeselectAllEdges()
	{
		BUtil::SMainContext mc(GetContext());
		CBrushDesignerSelectAllNoneTool::DeselectAllEdges();
		UpdateSelection(mc);
	}

	void PyDesignerDeselectAllFaces()
	{
		BUtil::SMainContext mc(GetContext());
		CBrushDesignerSelectAllNoneTool::DeselectAllFaces();
		UpdateSelection(mc);
	}

	void PyDesignerDeselectAll()
	{
		BUtil::SMainContext mc(GetContext());
		mc.pSelected->Clear();
		UpdateSelection(mc);
	}

	void PyDesignerGrowSelection()
	{
		BUtil::SMainContext mc(GetContext());
		CBrushDesignerSelectGrowTool::GrowSelection(mc);
		UpdateSelection(mc);
	}

	void PyDesignerSelectConnectedFaces()
	{
		BUtil::SMainContext mc(GetContext());
		CBrushDesignerSelectConnectedTool::SelectConnectedRegions(mc);
		UpdateSelection(mc);
	}

	void PyDesignerLoopSelection()
	{
		BUtil::SMainContext mc(GetContext());
		CBrushDesignerLoopSelectionTool::LoopSelection(mc);
		UpdateSelection(mc);
	}

	void PyDesignerRingSelection()
	{
		BUtil::SMainContext mc(GetContext());
		CBrushDesignerRingSelectionTool::RingSelection(mc);
		UpdateSelection(mc);
	}

	void PyDesignerInvertSelection()
	{
		BUtil::SMainContext mc(GetContext());
		CBrushDesignerInvertSelectionTool::InvertSelection(mc);
		UpdateSelection(mc);
	}

	void PyDesignerSelectVertexMode()
	{
		if(GetDesignerEditTool())
			GetDesignerEditTool()->SetDesignerMode(BUtil::eDesigner_Select_Vertex);
	}

	void PyDesignerSelectEdgeMode()
	{
		if(GetDesignerEditTool())
			GetDesignerEditTool()->SetDesignerMode(BUtil::eDesigner_Select_Edge);
	}

	void PyDesignerSelectFaceMode()
	{
		if(GetDesignerEditTool())
			GetDesignerEditTool()->SetDesignerMode(BUtil::eDesigner_Select_Face);
	}

	void PyDesignerSelectPivotMode()
	{
		if(GetDesignerEditTool())
			GetDesignerEditTool()->SetDesignerMode(BUtil::eDesigner_Pivot);
	}
}

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerSelect,
										   designer, 
										   select, 
										   "Selects elements with nID.",
										   "designer.select( int nID )" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerDeselect,
										   designer, 
										   deselect, 
										   "Deselects elements with nID.",
										   "designer.deselect( int nID )" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerSetSelection,
										   designer,
										   set_selection,
										   "Deselect the existing selections and sets the new selection.",
										   "designer.set_selection( int nID )" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerSelectAllVertices,
										   designer, 
										   select_all_vertices, 
										   "Selects all vertices.",
										   "designer.select_all_vertices()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerSelectAllEdges,
										   designer, 
										   select_all_edges, 
										   "Selects all edges.",
										   "designer.select_all_edges()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerSelectAllFaces,
										   designer, 
										   select_all_faces, 
										   "Selects all faces.",
										   "designer.select_all_faces()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerDeselectAllVertices,
										   designer,
										   deselect_all_vertices,
										   "Deselects all vertices.",
										   "designer.deselect_all_vertices()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerDeselectAllEdges,
										   designer,
										   deselect_all_edges,
										   "Deselects all edges.",
										   "designer.deselect_all_edges()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerDeselectAllFaces,
										   designer,
										   deselect_all_faces,
										   "Deselects all faces.",
										   "designer.deselect_all_faces()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerDeselectAll,
										   designer,
										   deselect_all,
										   "Deselects all elements.",
										   "designer.deselect_all()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerSelectConnectedFaces,
										   designer,
										   select_connected_faces,
										   "Selects all connected faces with the selected elements.",
										   "designer.select_connected_faces()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerGrowSelection,
										   designer,
										   grow_selection,
										   "Grows selection",
										   "designer.grow_selection()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerLoopSelection,
										   designer,
										   loop_selection,
										   "Selects a loop of edges that are connected in a end to end.",
										   "designer.loop_selection()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerRingSelection,
										   designer,
										   ring_selection,
										   "Selects a sequence of edges that are not connected, but on opposite sides to each other continuing along a face loop.",
										   "designer.ring_selection()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerInvertSelection,
										   designer,
										   invert_selection,
										   "Selects all components that are not selected, and deselect currently selected components.",
										   "designer.invert_selection()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerSelectVertexMode,
										   designer,
										   select_vertexmode,
										   "Select Vertex Mode of CryDesigner",
										   "designer.select_vertexmode()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerSelectEdgeMode,
										   designer,
										   select_edgemode,
										   "Select Edge Mode of CryDesigner",
										   "designer.select_edgemode()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerSelectFaceMode,
										   designer,
										   select_facemode,
										   "Select Face Mode of CryDesigner",
										   "designer.select_facemode()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerSelectPivotMode,
										   designer,
										   select_pivotmode,
										   "Select Pivot Mode of CryDesigner",
										   "designer.select_pivotmode()");
