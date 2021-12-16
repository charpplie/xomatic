#include "StdAfx.h"
#include "stdio.h"
// Need to include this here for the MFnPlugin header
#include <vector>

#include "mayaIncludes.h"
#include "MayaCryExport.h"
#include "ExportFunctions.h"
#include "GeometryExportSource.h"
#include "mayaError.h"
#include "attribute.h"

#include "Export/ExportStatusWindow.h"
#include "Export/IExportContext.h"
#include "Export/ColladaExportWriter.h"


// The global options class
cryExportOptions currentExporterOptions;

/////////////////////////////////////////////////////////////////////
// cryExportOptions functions
cryExportOptions::cryExportOptions( void )
{
	this->setDefaultOptions();
}

cryExportOptions::~cryExportOptions( void )
{
}

void cryExportOptions::setDefaultOptions( void )
{
	this->optionsString.assign("");
	this->customName.assign("");
	this->customFilePath.assign("");
	this->customMaterialName.assign("");
	this->exportType = kExportType_unknown;
	this->animStart = this->animEnd = 0;
	this->selectedMaterialsOnly = false;
}

bool cryExportOptions::parseOptionsString( const MString &optionsString )
{
	int i;
	this->setDefaultOptions();
	this->optionsString.assign( optionsString.asChar() );

	// Lets now do all of the option processing	
	if( optionsString.length() >0 )
	{
		MStringArray optionList;
		MStringArray theOption;
		optionsString.split( ';', optionList );

		for( i=0;i<optionList.length();++i )
		{
			theOption.clear();
			optionList[i].split( '=', theOption);
			if( theOption.length() > 1 )
			{
				if( theOption[0] == MString( "cryExportType" ) )
				{
					if( strIsSame( theOption[1].asChar(), "anim" ) )
						this->exportType = kExportType_anim;
					else if( strIsSame( theOption[1].asChar(), "material" ) )
						this->exportType = kExportType_material;
					else if( strIsSame( theOption[1].asChar(), "geom" ) )
						this->exportType = kExportType_geom;
					else
						this->exportType = kExportType_unknown;
				}
				else if( theOption[0] == MString( "cryExportCustomName" ) )
				{
					this->customName.assign( theOption[1].asChar() );
				}
				else if( theOption[0] == MString( "cryExportCustomFilePath" ) )
				{
					this->customFilePath.assign( theOption[1].asChar() );
				}
				else if( theOption[0] == MString( "cryExportCustomMaterialName" ) )
				{
					this->customMaterialName.assign( theOption[1].asChar() );
				}
				else if( theOption[0] == MString( "cryExportAnimStart" ) )
				{
					this->animStart = theOption[1].asInt();
				}
				else if( theOption[0] == MString( "cryExportAnimEnd" ) )
				{
					this->animEnd = theOption[1].asInt();
				}
				else if( theOption[0] == MString( "cryExportAnimRoot" ) )
				{
					this->animRoot = theOption[1].asChar();
				}
				else if( theOption[0] == MString( "crySelectedMaterialsOnly" ) )
				{
					this->selectedMaterialsOnly = (bool)(theOption[1].asInt());
				}
				else
					outputErrorMessage( PRI_LOW, "Unknown option `%s`.", theOption[0].asChar() );
			}
		}
	}

	return true;
}

///////////////////////////////////////////////////
// MayaCryExport Functions
MStatus MayaCryExport::writer( const MFileObject &fileObject, const MString &options, MPxFileTranslator::FileAccessMode mode )
{
	bool exportedOK = true;

	currentExporterOptions.parseOptionsString( options );

	try
	{
		ExportFunctions::Export( currentExporterOptions );
	}
	catch ( int i )
	{
		i = 0; // Use it to stop the unreferenced warning
		exportedOK = false;
	}

	if( exportedOK )
	{
		return MS::kSuccess;
	}

	return MS::kFailure;
}


//=================================================

cryMayaCheckDegradedFaces::cryMayaCheckDegradedFaces()
{
}

cryMayaCheckDegradedFaces::~cryMayaCheckDegradedFaces()
{
}

MStatus cryMayaCheckDegradedFaces::doIt( const MArgList& args )
{
	MString inNodeName;
	inNodeName = args.asString(0);

	MDagPath node;
	MSelectionList sList;
	MStatus status;

	MGlobal::getSelectionListByName( inNodeName, sList );
	if ( (status = sList.getDagPath( 0, node )) != MStatus::kSuccess )
		return status;

	status = DegradedFacesIterateNodes(node);

	if ( status == MStatus::kFailure )
	{
		MessageBox(0,"Degenerate Faces were found and the vertex positions were printed to the maya output window.","Degenerate Faces Found",0);
	}

	status = VertexSharingPointCheckIterateNodes(node);
	if ( status == MStatus::kFailure )
	{
		MessageBox(0,"Some vertices were found to have the same location, this may prevent the asset from exporting. The positions were printed in the maya output window.","Vertices sharing the same location found",0);
	}

	return status;
}


MStatus cryMayaCheckDegradedFaces::VertexSharingPointCheckIterateNodes(MDagPath node)
{
	MStatus status = MStatus::kSuccess;
	MSpace::Space space = MSpace::kTransform;
	MFnMesh nodeMesh(node,&status);

	if ( status != MS::kSuccess && node.childCount() == 0 )
	{
		return status;
	}

	if ( status == MS::kSuccess && node.hasFn(MFn::kMesh) && node.hasFn(MFn::kTransform) )
	{
		// Get the position array
		MPointArray positionArray;
		nodeMesh.getPoints( positionArray, space );
		int numPositions = positionArray.length();

		for ( int run1idx = 0; run1idx < numPositions; run1idx++ )
		{
			for ( int run2idx = run1idx+1; run2idx < numPositions; run2idx++ )
			{
				MVector diff = positionArray[run1idx] - positionArray[run2idx];
				if ( diff.length() == 0.0f ) 
				{
					printf("2 vertices are sharing the same location: ('%f','%f','%f')\n",positionArray[run1idx].x,positionArray[run1idx].y,positionArray[run1idx].z);
					fflush(0);
					status = MStatus::kFailure;
				}
			}
		}
	}
	else
	{
		status = MStatus::kSuccess;
	}


	// Recurse to the node's children.
	int childCount = node.childCount();
	for( int childIndex = 0;childIndex < childCount;childIndex++ )
	{
		node.push( node.child( childIndex ) );
		MStatus tempstatus = VertexSharingPointCheckIterateNodes(node);
		
		if ( status == MStatus::kSuccess )
			status = tempstatus;

		node.pop(1);
	}

	return status;
}

MStatus cryMayaCheckDegradedFaces::DegradedFacesIterateNodes(MDagPath node)
{
	const float threshold = 0.0001f;
	MStatus status = MStatus::kSuccess;
	int t;
	MSpace::Space space = MSpace::kTransform;
	MFnDagNode dagNode( node, &status );

	if ( status != MS::kSuccess && node.childCount() == 0 )
	{
		return status;
	}

	if ( status == MS::kSuccess && node.hasFn(MFn::kMesh) && node.hasFn(MFn::kTransform) )
	{
		MItMeshPolygon polygonIt( node, MObject::kNullObj );
		for( ;!polygonIt.isDone(); polygonIt.next() ) 
		{
			int numTriangles;
			polygonIt.numTriangles( numTriangles );
			for( t = 0;t<numTriangles;t++ )
			{
				MPointArray points;
				MIntArray vertexList;
				polygonIt.getTriangle( t, points, vertexList, space );

				MVector avec = points[1] - points[0]; 
				MVector bvec = points[2] - points[0];
				MVector cvec = points[1] - points[2];

				float a = (float)avec.length();
				float b = (float)bvec.length();
				float c = (float)cvec.length();

				float ja = a + b + c; 
				float jb = a + b - c;
				float jc = b + c - a;
				float jd = c + a - b;
				float s = ja * jb * jc * jd;
				float d = s/16; 
				float area = sqrt(d);
				if ( area < threshold )
				{
					printf("Degenerate face: v0('%f','%f','%f') v1('%f','%f','%f') v2('%f','%f','%f')\n", points[0].x, points[0].y, points[0].z, points[1].x, points[1].y, points[1].z, points[2].x, points[2].y, points[2].z );
					fflush(0);
					status = MStatus::kFailure;
				}
			}
		}
	}
	else
	{
		status = MStatus::kSuccess;
	}


	// Recurse to the node's children.
	int childCount = node.childCount();
	for( int childIndex = 0;childIndex < childCount;childIndex++ )
	{
		node.push( node.child( childIndex ) );
		MStatus tempstatus = DegradedFacesIterateNodes(node);
		
		if ( status == MStatus::kSuccess )
			status = tempstatus;

		node.pop(1);
	}

	return status;
}

bool cryMayaCheckDegradedFaces::isUndoable() const
{
	return false;
}

void* cryMayaCheckDegradedFaces::creator()
{
	return new cryMayaCheckDegradedFaces();
}