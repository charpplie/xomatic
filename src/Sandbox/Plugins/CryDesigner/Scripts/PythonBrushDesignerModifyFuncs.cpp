#include "StdAfx.h"
#include "Util/BoostPythonHelpers.h"
#include "PythonBrushDesignerBasic.h"
#include "Objects/DesignerBrushObject.h"
#include "Core/BrushDesigner.h"
#include "Tools/BrushDesignerOffsetTool.h"
#include "Tools/BrushDesignerExtrudeTool.h"
#include "Tools/BrushDesignerMoveTool.h"
#include "Tools/BrushDesignerSubdivisionTool.h"

namespace BPython
{
	void PyDesignerOffset( float fScale, pSPyWrappedProperty bPyCreatBridgeEdges )
	{
		if( bPyCreatBridgeEdges->type != SPyWrappedProperty::eType_Bool )
			throw std::logic_error("bCreateBridgeEdges is invalid data type.");

		BUtil::SMainContext mc(GetContext());
		if( mc.pSelected->IsEmpty() )
			throw std::logic_error("There are no selected elements.");
		for( int i = 0, iCount(mc.pSelected->GetSize()); i < iCount; ++i )
		{
			if( !mc.pSelected->Get(i).IsFace() || !mc.pSelected->Get(i).m_pRegion )
				continue;

			CBrushRegion::RegionPtr pScaledRegion = mc.pSelected->Get(i).m_pRegion->Clone();
			pScaledRegion->Scale((BrushFloat)fScale,true);

			CBrushDesignerOffsetTool::ApplyOffset(mc.pDesigner, pScaledRegion, mc.pSelected->Get(i).m_pRegion, bPyCreatBridgeEdges->property.boolValue);
		}

		mc.pSelected->Clear();
		mc.pDesigner->ResetDB(BUtil::eDBRF_ALL);
		UpdateBrush(mc);
	}

	void PyDesignerExtrude( float fHeight, float fScale )
	{
		if( std::abs(fHeight) < kDesignerEpsilon )
			throw std::logic_error("height is too close to 0.");

		BUtil::SMainContext mc(GetContext());
		if( mc.pSelected->IsEmpty() )
			throw std::logic_error("There are no selected elements.");

		for( int i = 0, iCount(mc.pSelected->GetSize()); i < iCount; ++i )
		{
			if( !mc.pSelected->Get(i).IsFace() || !mc.pSelected->Get(i).m_pRegion )
				continue;
			CBrushRegion::RegionPtr pRegion = mc.pDesigner->QueryEquivalentRegion(mc.pSelected->Get(i).m_pRegion);
			if( pRegion )
				CBrushDesignerExtrudeTool::Extrude( mc, pRegion, fHeight, fScale );
		}

		mc.pSelected->Clear();
		mc.pDesigner->ResetDB(BUtil::eDBRF_ALL);
		UpdateBrush(mc);
	}

	void PyDesignerTranslate( pSPyWrappedProperty vOffset )
	{
		if( vOffset->type != SPyWrappedProperty::eType_Vec3 )
			throw std::logic_error("offset should be vec3 type.");
		BUtil::SMainContext mc(GetContext());
		BrushMatrix34 tm = BrushMatrix34::CreateTranslationMat(FromSVecToBrushVec3(vOffset->property.vecValue));
		CBrushDesignerMoveTool::Transform(mc,tm,s_bdpc.bMoveTogether);		
		UpdateSelection(mc);
		UpdateBrush(mc);
	}

	void PyDesignerRotate( pSPyWrappedProperty vRotation )
	{
		if( vRotation->type != SPyWrappedProperty::eType_Vec3 )
			throw std::logic_error("rotation should be vec3 type.");
		BUtil::SMainContext mc(GetContext());
		BrushVec3 vRot = FromSVecToBrushVec3(vRotation->property.vecValue);
		BrushMatrix34 tm = BrushMatrix34::CreateRotationXYZ(Ang3_tpl<BrushFloat>(DEG2RAD(vRot.x),DEG2RAD(vRot.y),DEG2RAD(vRot.z)));
		CBrushDesignerMoveTool::Transform(mc,tm,s_bdpc.bMoveTogether);
		UpdateSelection(mc);
		UpdateBrush(mc);
	}

	void PyDesignerScale( pSPyWrappedProperty vScale )
	{
		if( vScale->type != SPyWrappedProperty::eType_Vec3 )
			throw std::logic_error("scale should be vec3 type.");
		BUtil::SMainContext mc(GetContext());
		BrushMatrix34 tm = BrushMatrix34::CreateScale(FromSVecToBrushVec3(vScale->property.vecValue));
		CBrushDesignerMoveTool::Transform(mc,tm,s_bdpc.bMoveTogether);
		UpdateSelection(mc);
		UpdateBrush(mc);
	}

	void PyDesignerSubdivide( int nLevel )
	{
		if( nLevel < 0 || nLevel >= BUtil::kMaximumSubdivisionLevel )
			throw std::logic_error("a level should be between 0 and 4.");
		
		CBrushDesignerSubdivisionTool::Subdivide(nLevel,BPython::s_bdpc.bAutomaticUpdateMesh);
	}
}

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerOffset,
										   designer,
										   offset,
										   "Adds scaled faces inside the selected faces.",
										   "designer.offset(float scale, bool bCreateBridgeEdges)" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerExtrude,
										   designer,
										   extrude,
										   "Extrudes the selected faces by the specified height in a normal direction of each selected region after scaling it by the specified scale.",
										   "designer.extrude(float height, float scale)" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerTranslate,
										   designer,
										   translate,
										   "Translates the selected elements by the specified offset.",
										   "designer.translate((float offset_x,float offset_y,float offset_z))" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerRotate,
										   designer,
										   rotate,
										   "Rotates the selected elements by the specified degree offset.",
										   "designer.rotate((float rotation_x,float rotation_y,float rotation_z))" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerScale,
										   designer,
										   scale,
										   "Scales the selected elements by the specified offset.",
										   "designer.scale((float scale_x,float scale_y,float scale_z))" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerSubdivide,
										   designer,
										   subdivide,
										   "Subdivides the selected designer objects.",
										   "designer.subdivide(int nLevel)" );