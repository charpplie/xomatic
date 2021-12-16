#include "StdAfx.h"
#include "Util/BoostPythonHelpers.h"
#include "PythonBrushDesignerBasic.h"
#include "Objects/DesignerBrushObject.h"
#include "Tools/BrushDesignerStairTool.h"
#include "Core/BrushPrimitive.h"

namespace BPython
{
	std::vector<CDesignerBrushObject*> GetSelectedDesignerObjects()
	{
		std::vector<CDesignerBrushObject*> designerObjects; 
		CSelectionGroup* pGroup = GetIEditor()->GetObjectManager()->GetSelection();
		for( int i = 0, iCount(pGroup->GetCount()); i < iCount; ++i )
		{
			CBaseObject* pBaseObject = pGroup->GetObject(i);
			if( pBaseObject->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
				designerObjects.push_back((CDesignerBrushObject*)pBaseObject);
		}

		return designerObjects;
	}

	DesignerElementsPtr AddRegionsToDesignerObjects( const std::vector<CBrushRegion::RegionPtr>& regions )
	{
		std::vector<CDesignerBrushObject*> designerObjects = GetSelectedDesignerObjects();
		if( designerObjects.empty() )
			throw std::logic_error("At least a designer object has to be selected.");

		for( int k = 0, iRegionCount(regions.size()); k < iRegionCount; ++k )
			designerObjects[0]->GetDesigner()->AddRegion(regions[k],CBrushDesigner::eOpType_Add);

		DesignerElementsPtr pElements = new CBrushDesignerElementManager;	
		for( int i = 0, iCount(regions.size()); i < iCount; ++i )
			pElements->Add(SDesignerElement(designerObjects[0],regions[i]));

		UpdateBrush(GetContext());

		return pElements;
	}

	ElementID PyDesignerAddBox(pSPyWrappedProperty vPyPos, pSPyWrappedProperty vPySize)
	{
		if( vPyPos->type != SPyWrappedProperty::eType_Vec3 )
			throw std::logic_error("pos is invalid data type.");

		if( vPySize->type != SPyWrappedProperty::eType_Vec3 )
			throw std::logic_error("size is invalid data type.");

		BrushVec3 vPos = FromSVecToBrushVec3(vPyPos->property.vecValue);
		BrushVec3 vSize = FromSVecToBrushVec3(vPySize->property.vecValue);
		CBrushPrimitive bp;
		std::vector<CBrushRegion::RegionPtr> regions;
		bp.CreateBox( vPos, vPos+vSize, &regions );
		
		return s_bdpc.RegisterElements(AddRegionsToDesignerObjects(regions));
	}

	ElementID PyDesignerAddSphere(pSPyWrappedProperty vPyCenter, float radius, int numSides)
	{
		if( vPyCenter->type != SPyWrappedProperty::eType_Vec3 )
			throw std::logic_error("center is invalid data type.");

		if( radius <= 0 )
			throw std::logic_error("radius is more than 0.");

		if( numSides < 3 )
			throw std::logic_error("numSides is more than 3.");

		CBrushPrimitive bp;
		std::vector<CBrushRegion::RegionPtr> regions;
		bp.CreateSphere( FromSVecToBrushVec3(vPyCenter->property.vecValue), radius, numSides, &regions );

		return s_bdpc.RegisterElements(AddRegionsToDesignerObjects(regions));
	}

	ElementID PyDesignerAddCylinder( pSPyWrappedProperty vPyBottomCenter, float radius, float height, int numSides )
	{
		if( vPyBottomCenter->type != SPyWrappedProperty::eType_Vec3 )
			throw std::logic_error("center is invalid data type.");

		if( radius <= 0 )
			throw std::logic_error("radius is more than 0.");

		if( height <= 0 )
			throw std::logic_error("height is more than 0.");

		if( numSides < 3 )
			throw std::logic_error("numSides is more than 3.");

		BrushVec3 vBottomCenter = FromSVecToBrushVec3(vPyBottomCenter->property.vecValue);
		BrushVec3 vMin = vBottomCenter - BrushVec3(BrushFloat(radius),(BrushFloat)radius,0);
		BrushVec3 vMax = vBottomCenter + BrushVec3(BrushFloat(radius),(BrushFloat)radius,(BrushFloat)height);

		CBrushPrimitive bp;
		std::vector<CBrushRegion::RegionPtr> regions;
		bp.CreateCylinder(vMin, vMax, numSides, &regions);

		return s_bdpc.RegisterElements(AddRegionsToDesignerObjects(regions));
	}

	ElementID PyDesignerAddCone( pSPyWrappedProperty vPyBottomCenter, float radius, float height, int numSides )
	{
		if( vPyBottomCenter->type != SPyWrappedProperty::eType_Vec3 )
			throw std::logic_error("center is invalid data type.");

		if( radius <= 0 )
			throw std::logic_error("radius is more than 0.");

		if( height <= 0 )
			throw std::logic_error("height is more than 0.");

		if( numSides < 3 )
			throw std::logic_error("numSides is more than 3.");

		BrushVec3 vBottomCenter = FromSVecToBrushVec3(vPyBottomCenter->property.vecValue);
		BrushVec3 vMin = vBottomCenter - BrushVec3(BrushFloat(radius),(BrushFloat)radius,0);
		BrushVec3 vMax = vBottomCenter + BrushVec3(BrushFloat(radius),(BrushFloat)radius,(BrushFloat)height);

		CBrushPrimitive bp;
		std::vector<CBrushRegion::RegionPtr> regions;
		bp.CreateCone(vMin, vMax, numSides, &regions);

		return s_bdpc.RegisterElements(AddRegionsToDesignerObjects(regions));
	}

	ElementID PyDesignerAddRectangle( pSPyWrappedProperty vPyMin, pSPyWrappedProperty vPyMax )
	{
		if( vPyMin->type != SPyWrappedProperty::eType_Vec3 )
			throw std::logic_error("min is invalid data type.");

		if( vPyMax->type != SPyWrappedProperty::eType_Vec3 )
			throw std::logic_error("max is invalid data type.");

		BrushVec3 vMin = FromSVecToBrushVec3(vPyMin->property.vecValue);
		BrushVec3 vMax = FromSVecToBrushVec3(vPyMax->property.vecValue);

		CBrushPrimitive bp;
		std::vector<CBrushRegion::RegionPtr> regions;
		bp.CreateRectangle(vMin,vMax,&regions);

		return s_bdpc.RegisterElements(AddRegionsToDesignerObjects(regions));
	}

	ElementID PyDesignerAddDisc( pSPyWrappedProperty vPyCenter, float radius, int numSides )
	{
		if( vPyCenter->type != SPyWrappedProperty::eType_Vec3 )
			throw std::logic_error("center is invalid data type.");

		if( radius <= 0 )
			throw std::logic_error("radius is more than 0.");

		if( numSides < 3 )
			throw std::logic_error("numSides is more than 3.");

		BrushVec3 vCenter = FromSVecToBrushVec3(vPyCenter->property.vecValue);
		BrushVec3 vMin = vCenter - BrushVec3(BrushFloat(radius),BrushFloat(radius),0);
		BrushVec3 vMax = vCenter + BrushVec3(BrushFloat(radius),BrushFloat(radius),0);

		CBrushPrimitive bp;
		std::vector<CBrushRegion::RegionPtr> regions;
		bp.CreateDisc(vMin,vMax,numSides,&regions);

		return s_bdpc.RegisterElements(AddRegionsToDesignerObjects(regions));
	}

	ElementID PyDesignerAddStair( pSPyWrappedProperty vPyBoxMin, pSPyWrappedProperty vPyBoxMax, float fStepRise, pSPyWrappedProperty bPyMirrored, pSPyWrappedProperty bPyRotationBy90Degree )
	{
		if( vPyBoxMin->type != SPyWrappedProperty::eType_Vec3 )
			throw std::logic_error("boxmin is invalid data type.");

		if( vPyBoxMax->type != SPyWrappedProperty::eType_Vec3 )
			throw std::logic_error("boxmax is invalid data type.");

		if( fStepRise <= 0 )
			throw std::logic_error("steprise is more than 0.");

		if( bPyMirrored->type != SPyWrappedProperty::eType_Bool )
			throw std::logic_error("bMirrored should be bool type");

		if( bPyRotationBy90Degree->type != SPyWrappedProperty::eType_Bool )
			throw std::logic_error("bRotationBy90Degree should be bool type");

		BrushVec3 vBoxMin = FromSVecToBrushVec3(vPyBoxMin->property.vecValue);
		BrushVec3 vBoxMax = FromSVecToBrushVec3(vPyBoxMax->property.vecValue);
		bool bMirrored = bPyMirrored->property.boolValue;
		bool bRotationBy90Degree = bPyRotationBy90Degree->property.boolValue;
		BrushVec3 vStartPos(vBoxMin);
		BrushVec3 vEndPos(vBoxMax.x,vBoxMax.y,vBoxMin.z);
		BrushFloat fBoxWidth = std::abs(vBoxMin.x-vBoxMax.x);
		BrushFloat fBoxDepth = std::abs(vBoxMin.y-vBoxMax.y);
		BrushFloat fBoxHeight = std::abs(vBoxMin.z-vBoxMax.z);
		BrushPlane floorPlane(BrushVec3(0,0,1), -vStartPos.Dot(BrushVec3(0,0,1)));
		bool bXDirection = fBoxWidth >= fBoxDepth;
		CBrushDesignerStairTool::SOutputParameterForStairCreation output;

		CBrushDesignerStairTool::CreateStair(vStartPos,vEndPos,fBoxWidth,fBoxDepth,fBoxHeight,floorPlane,fStepRise,bXDirection,bMirrored,bRotationBy90Degree,NULL,output);
		return s_bdpc.RegisterElements(AddRegionsToDesignerObjects(output.regions));
	}

	struct SAddPolygonContext
	{
		SAddPolygonContext() : bInProgress(false),bAddOutside(true)
		{
		}

		void Clear()
		{
			verticesInside.clear();
			verticesOutside.clear();
			insidevertexSet.clear();
		}

		typedef std::vector<BrushVec3> VertexList;
		std::vector<VertexList> insidevertexSet;
		VertexList verticesOutside;
		VertexList verticesInside;
		bool bInProgress;
		bool bAddOutside;
	};

	SAddPolygonContext s_apc;

	void PyDesignerStartPolygonAddition()
	{
		if( s_apc.bInProgress )
			throw std::logic_error("You need to finish an another polygon addition which has been already started.");

		s_apc.bInProgress = true;
		s_apc.bAddOutside = true;
	}

	ElementID PyDesignerFinishPolygonAddition()
	{
		s_apc.bInProgress = false;

		if( s_apc.verticesOutside.size() < 3 )
			throw std::logic_error("You're trying to add an invalid polygon");

		std::vector<BrushVec3> vList;
		std::vector<BUtil::SEdge> eList;

		for( int i = 0, iVertexCountOutside(s_apc.verticesOutside.size()); i < iVertexCountOutside; ++i )
		{
			vList.push_back(s_apc.verticesOutside[i]);
			eList.push_back(BUtil::SEdge(i,(i+1)%iVertexCountOutside));
		}

		if( !s_apc.verticesInside.empty() )
			s_apc.insidevertexSet.push_back(s_apc.verticesInside);
		
		for( int i = 0, iVertexSetCountInside(s_apc.insidevertexSet.size()); i < iVertexSetCountInside; ++i )
		{	
			SAddPolygonContext::VertexList& verticesInside(s_apc.insidevertexSet[i]);
			int nOffset = vList.size();
			for( int k = 0, iVertexCountInside(verticesInside.size()); k < iVertexCountInside; ++k )
			{
				vList.push_back(verticesInside[k]);
				eList.push_back(BUtil::SEdge(nOffset+k,nOffset+((k+1)%iVertexCountInside)));
			}
		}

		std::vector<CBrushRegion::RegionPtr> regions;
		regions.push_back(new CBrushRegion(vList,eList));
		s_apc.Clear();
		return s_bdpc.RegisterElements(AddRegionsToDesignerObjects(regions));
	}

	void PyDesignerAddVertexToPolygon( pSPyWrappedProperty vPyVertex )
	{
		if( vPyVertex->type != SPyWrappedProperty::eType_Vec3 )
			throw std::logic_error("vertex is invalid data type.");

		if( s_apc.bAddOutside )
			s_apc.verticesOutside.push_back(FromSVecToBrushVec3(vPyVertex->property.vecValue));
		else
			s_apc.verticesInside.push_back(FromSVecToBrushVec3(vPyVertex->property.vecValue));
	}

	void PyDesignerStartToAddAnotherHole()
	{
		if( s_apc.bAddOutside )
			s_apc.bAddOutside = false;
		else if( !s_apc.verticesInside.empty() )
			s_apc.insidevertexSet.push_back(s_apc.verticesInside);
		s_apc.verticesInside.clear();
	}
}

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerAddBox, 
										   designer, 
										   add_box, 
										   "Adds a box in a designer object.",
										   "int designer.add_box((float pos_x,float pos_y,float pos_z),(float width,float depth,float height))" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerAddSphere, 
										   designer, 
										   add_sphere, 
										   "Adds a sphere in a designer object.",
										   "int designer.add_sphere((float center_x,float center_y,float center_z),float radius,int numSides)" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerAddCylinder, 
										   designer, 
										   add_cylinder, 
										   "Adds a cylinder in a designer object. Returns id which points out added polygons.",
										   "int designer.add_cylinder((float bottomcenter_x,float bottomcenter_y,float bottomcenter_z),float radius,float height,int numSides)" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerAddCone, 
										   designer, 
										   add_cone, 
										   "Adds a cone in a desigher object. Returns id which points out added polygons.",
										   "int designer.add_cone((float bottomcenter_x,float bottomcenter_y,float bottomcenter_z),float radius,float height,int numSides)" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerAddRectangle, 
										   designer, 
										   add_rectangle, 
										   "Adds a rectangle in a desigher object. Returns id which points out added polygons.",
										   "int designer.add_rectangle((float min_x,float min_y,float min_z),(float max_x,float max_y,float max_z))" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerAddDisc, 
										   designer, 
										   add_disc, 
										   "Adds a disc in a desigher object. Returns id which points out added polygons.",
										   "int designer.add_disc((float center_x,float center_y,float center_z),float radius,float numSides)" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerAddStair, 
										   designer, 
										   add_stair, 
										   "Adds a stair in a desigher object. Returns id which points out added polygons.",
										   "int designer.add_stair((float boxmin_x,float boxmin_y,float boxmin_z),(float boxmax_x,float boxmax_y,float boxmax_z),float stepRise,bool bMirror,bool bRotationBy90Degree)" ); 

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerStartPolygonAddition, 
										   designer, 
										   start_polygon_addition, 
										   "Starts a polygon addition.",
										   "designer.start_polygon_addition()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerFinishPolygonAddition, 
										   designer, 
										   finish_polygon_addition, 
										   "Finishes a polygon addition. Returns id which points out added polygons.",
										   "int designer.finish_polygon_addition()" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerAddVertexToPolygon, 
										   designer, 
										   add_vertex_to_polygon, 
										   "Adds a vertex to a polygon.",
										   "designer.add_vertex_to_polygon((float x,float y,float z))" );

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE( BPython::PyDesignerStartToAddAnotherHole, 
										   designer,
										   start_to_add_another_hole,
										   "Starts to add an another hole to a polygon",
										   "designer.start_to_add_another_hole()" );