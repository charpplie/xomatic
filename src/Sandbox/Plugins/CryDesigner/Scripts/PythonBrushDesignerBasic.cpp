#include "StdAfx.h"
#include "PythonBrushDesignerBasic.h"
#include "Tools/BrushDesignerEditTool.h"
#include "Objects/DesignerBrushObject.h"
#include "Tools/BrushDesignerSelectTool.h"

namespace BPython
{
	CBrushDesignerPythonContext s_bdpc;
	CBrushDesignerPythonContext s_bdpc_before_init;

	BUtil::SMainContext GetContext()
	{
		CSelectionGroup* pGroup = GetIEditor()->GetObjectManager()->GetSelection();
		CBaseObject* pObject = pGroup->GetObject(0);
		if( !pObject->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
			throw std::logic_error("The selected object isn't a designer object type");

		CEditTool* pEditTool = GetIEditor()->GetEditTool();
		if( pEditTool == NULL || !pEditTool->IsKindOf(RUNTIME_CLASS(CBrushDesignerEditTool)) )
			throw std::logic_error("The selected object isn't a designer object type");		
		
		CDesignerBrushObject* pDesignerObject = (CDesignerBrushObject*)pObject;
		CBrushDesignerEditTool* pDesignerTool = (CBrushDesignerEditTool*)pEditTool;

		BUtil::SMainContext mc;
		mc.pObject = pObject;
		mc.pDesigner = pDesignerObject->GetDesigner();
		mc.pBrush = pDesignerObject->GetBrush();
		mc.pSelected = pDesignerTool->GetSelectedElements();

		return mc;
	}

	void UpdateBrush(BUtil::SMainContext& mc, bool bForce)
	{
		if( bForce || s_bdpc.bAutomaticUpdateMesh )
			mc.pBrush->Update(mc.pObject,mc.pDesigner);
	}

	void UpdateSelection( BUtil::SMainContext& mc )
	{	
		CBrushDesignerSelectTool::ResetDesignerRejectedEdgeList(mc);
		CBrushDesignerSelectTool::UpdateSelectionMeshFromSelectedElementList(mc);
	}

	ElementID CBrushDesignerPythonContext::RegisterElements( DesignerElementsPtr pElements )
	{
		CBrushDesignerElementManager* pPureElements = pElements.get();
		pPureElements->AddRef();
		elementVariables.insert(pPureElements);
		return (ElementID)pPureElements;
	}

	DesignerElementsPtr CBrushDesignerPythonContext::FindElements( ElementID id )
	{
		std::set<CBrushDesignerElementManager*>::iterator ii = elementVariables.find((CBrushDesignerElementManager*)id);
		if( ii == elementVariables.end() )
			return NULL;
		return *ii;
	}

	void CBrushDesignerPythonContext::ClearElementVariables()
	{
		std::set<CBrushDesignerElementManager*>::iterator ii = elementVariables.begin();
		for( ; ii != elementVariables.end(); ++ii )
			(*ii)->Release();
		elementVariables.clear();
	}

	BrushVec3 FromSVecToBrushVec3( const SPyWrappedProperty::SVec& sVec ) 
	{
		return ToBrushVec3(Vec3(sVec.x, sVec.y, sVec.z));
	}

	void OutputPolygonPythonCreationCode( CBrushRegion* pRegion )
	{
		std::vector<CBrushRegion::RegionPtr> outerRegions;
		pRegion->GetSeparatedRegions( outerRegions, CBrushRegion::eSR_OuterHull );

		std::vector<CBrushRegion::RegionPtr> innerRegions;
		pRegion->GetSeparatedRegions( innerRegions, CBrushRegion::eSR_InnerHull );

		CString buffer;
		OutputDebugString("designer.start_polygon_addition()\n");

		std::vector<BrushVec3> vList;
		for( int i = 0, iRegionCount(outerRegions.size()); i < iRegionCount; ++i )
		{			
			outerRegions[i]->GetLinkedVertices(vList);
			for( int k = 0, iVListCount(vList.size()); k < iVListCount; ++k )
			{
				buffer.Format("designer.add_vertex_to_polygon((%f,%f,%f))\n",vList[k].x,vList[k].y,vList[k].z);
				OutputDebugString(buffer);
			}
		}

		if( !innerRegions.empty() )
		{
			for( int i = 0, iRegionCount(innerRegions.size()); i < iRegionCount; ++i )
			{
				innerRegions[i]->GetLinkedVertices(vList);
				buffer.Format("\ndesigner.start_to_add_another_hole()\n");
				OutputDebugString(buffer);
				for( int k = 0, iVListCount(vList.size()); k < iVListCount; ++k )
				{
					buffer.Format("designer.add_vertex_to_polygon((%f,%f,%f))\n",vList[k].x,vList[k].y,vList[k].z);
					OutputDebugString(buffer);
				}
			}
		}

		OutputDebugString("designer.finish_polygon_addition()\n");
	}

	void PyDesignerStart()
	{
		s_bdpc_before_init = s_bdpc;
		s_bdpc.Init();
	}

	void PyDesignerEnd()
	{
		s_bdpc.ClearElementVariables();
		s_bdpc = s_bdpc_before_init;
	}

	void PyDesignerSetEnv( pSPyWrappedProperty name, pSPyWrappedProperty value )
	{
		if( name->type != SPyWrappedProperty::eType_String )
			throw std::logic_error("Invalid name type.");

		if( name->stringValue == "move_together" || name->stringValue == "automatic_update_mesh" )
		{
			if( value->type != SPyWrappedProperty::eType_Bool)
				throw std::logic_error("the type of the value should be Bool.");
		}

		if( name->stringValue == "move_together" )
			s_bdpc.bMoveTogether = value->property.boolValue;
		else if( name->stringValue == "automatic_update_mesh")
			s_bdpc.bAutomaticUpdateMesh = value->property.boolValue;
		else
			throw std::logic_error("The name doesn't exist.");
	}

	void PyDesignerRecordUndo()
	{
		CUndo undo("Calls python scripts in a designer object");
		BUtil::SMainContext mc(GetContext());
		mc.pDesigner->RecordUndo("Calls python scripts in a designer object",mc.pObject);
	}

	void PyDesignerUpdateMesh()
	{
		UpdateBrush(GetContext(),true);
	}
}

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerStart,
										   designer,
										   start,
										   "Initializes all external variables used in the designer scripts.",
										   "designer.start()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerEnd,
										   designer,
										   end,
										   "Restores the designer context to the context used before initializing.",
										   "designer.end()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerSetEnv,
										   designer,
										   set_env,
										   "Sets environment variables.",
										   "designer.set_env( str name, [bool || int || float || str || (float,float,float)] paramValue )" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerRecordUndo,
										   designer,
										   record_undo,
										   "Records the current designer states to be undone.",
										   "designer.record_undo()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerUpdateMesh,
										   designer,
										   update_mesh,
										   "Updates mesh.",
										   "designer.update_mesh()" );
