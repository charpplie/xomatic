#include "StdAfx.h"
#include "Util/BoostPythonHelpers.h"
#include "PythonBrushDesignerBasic.h"
#include "Objects/DesignerBrushObject.h"
#include "Tools/BrushDesignerWeldTool.h"
#include "Tools/BrushDesignerFillSpaceTool.h"
#include "Tools/BrushDesignerFlipTool.h"
#include "Tools/BrushDesignerMergeTool.h"
#include "Tools/BrushDesignerSeparateTool.h"
#include "Tools/BrushDesignerRemoveTool.h"
#include "Tools/BrushDesignerCopyTool.h"
#include "Tools/BrushDesignerRemoveDoubles.h"
#include "Tools/BrushDesignerSelectTool.h"
#include "Tools/BrushDesignerEditTool.h"
#include "Core/BrushDesigner.h"
#include "Core/BaseBrush.h"

namespace BPython
{
	ElementID PyDesignerFlip()
	{
		BUtil::SMainContext mc = GetContext();
		if( mc.pSelected->IsEmpty() )
			throw std::logic_error("There are no selected elements.");

		CBrushDesignerElementManager flippedElements;
		CBrushDesignerFlipTool::FlipRegions(mc,flippedElements);
		UpdateBrush(mc);
		return s_bdpc.RegisterElements(new CBrushDesignerElementManager(flippedElements));
	}

	void PyDesignerMergeFaces()
	{
		BUtil::SMainContext mc = GetContext();
		if( mc.pSelected->IsEmpty() )
			throw std::logic_error("There are no selected elements.");

		CBrushDesignerMergeTool::MergeRegions(mc);
		mc.pSelected->Clear();
		UpdateBrush(mc);
	}

	void PyDesignerSeparate()
	{
		BUtil::SMainContext mc = GetContext();
		if( mc.pSelected->IsEmpty() )
			throw std::logic_error("There are no selected elements.");

		CBrushDesignerSeparateTool::Separate(mc);
		UpdateBrush(mc);
	}

	void PyDesignerRemove()
	{
		BUtil::SMainContext mc = GetContext();
		if( mc.pSelected->IsEmpty() )
			throw std::logic_error("There are no selected elements.");

		CBrushDesignerRemoveTool::RemoveSelectedElements(mc,false);
		mc.pSelected->Clear();
		mc.pDesigner->ResetDB(BUtil::eDBRF_ALL);
		UpdateBrush(mc);
	}

	ElementID PyDesignerCopy()
	{
		BUtil::SMainContext mc = GetContext();
		if( mc.pSelected->IsEmpty() )
			throw std::logic_error("There are no selected elements.");

		CBrushDesignerElementManager copiedElements;
		CBrushDesignerCopyTool::Copy(mc,&copiedElements);
		UpdateBrush(mc);

		return s_bdpc.RegisterElements(new CBrushDesignerElementManager(copiedElements));
	}

	void PyDesignerRemoveDoubles( float fDistance )
	{
		BUtil::SMainContext mc = GetContext();
		if( mc.pSelected->IsEmpty() )
			throw std::logic_error("There are no selected elements.");

		CBrushDesignerRemoveDoublesTool::RemoveDoubles(mc,fDistance);
		mc.pSelected->Clear();
		mc.pDesigner->ResetDB(BUtil::eDBRF_ALL);
		UpdateBrush(mc);
	}

	void PyDesignerWeld()
	{
		BUtil::SMainContext mc = GetContext();
		if( mc.pSelected->IsEmpty() )
			throw std::logic_error("There are no selected elements.");

		std::vector<BrushVec3> vertices;
		for( int i = 0, iSelectedElementCount(mc.pSelected->GetSize()); i < iSelectedElementCount; ++i )
		{
			if( mc.pSelected->Get(i).IsVertex() )
				vertices.push_back(mc.pSelected->Get(i).GetVertex());
		}

		if( vertices.size() != 2 )
			throw std::logic_error("Only two vertices need to be selected.");

		CBrushDesignerWeldTool::Weld(mc,vertices[0],vertices[1]);
		mc.pSelected->Clear();
		mc.pDesigner->ResetDB(BUtil::eDBRF_ALL);
		UpdateBrush(mc);
	}
}

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerFlip,
										   designer,
										   flip,
										   "Flips the selected faces. Returns nID which points out fliped polygons.",
										   "designer.flip()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerCopy,
										   designer,
										   copy,
										   "Copies the selected faces. Returns nID which points out copied polygons.",
										   "designer.copy()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerMergeFaces,
										   designer,
										   merge_faces,
										   "Merges the selected faces.",
										   "designer.merge_faces()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerRemove,
										   designer,
										   remove,
										   "Removes the selected faces or edges.",
										   "designer.remove()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerSeparate,
										   designer,
										   separate,
										   "Separates the selected faces to a new designer object.",
										   "designer.separate()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerRemoveDoubles,
										   designer,
										   remove_doubles,
										   "Removes doubles from the select elements within the specified distance.",
										   "designer.remove_doubles( float distance )" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerWeld,
										   designer,
										   weld,
										   "Puts the first selected vertex together into the second selected vertex.",
										   "designer.weld()" );