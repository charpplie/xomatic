#include "StdAfx.h"
#include "MeshHelpers.h"
#include <xsi_primitive.h>

MeshHelpers::MeshAccessor::MeshAccessor(XSI::X3DObject& object)
:	object(object),
	mesh(object.GetActivePrimitive().GetGeometry()),
	accessor(mesh.GetGeometryAccessor())
{
}

bool MeshHelpers::MeshAccessor::IsValid()
{
	return this->accessor.IsValid();
}

XSI::CDoubleArray MeshHelpers::MeshAccessor::GetPositions()
{
	// polygon vertex positions
	XSI::CDoubleArray vtxPosArray;
	this->accessor.GetVertexPositions(vtxPosArray);

	return vtxPosArray;
}

XSI::CFloatArray MeshHelpers::MeshAccessor::GetNormals()
{
	// polygon node normals
	XSI::CFloatArray nodeArray;
	this->accessor.GetNodeNormals(nodeArray);

	return nodeArray;
}

XSI::CFloatArray MeshHelpers::MeshAccessor::GetUVs()
{
	// gets all UVs on the mesh
	XSI::CRefArray uvs = this->accessor.GetUVs( );

	// works with the first UV set on the geometry
	// UV values are stored as a flat list of float values grouped in
	// triplets (i.e. UVW), first triplet being the values at node 0, etc...
	XSI::ClusterProperty uvProp = uvs[ 0 ];
	XSI::CFloatArray uvValues;
	uvProp.GetValues( uvValues );

	return uvValues;
}

XSI::CFloatArray MeshHelpers::MeshAccessor::GetColours()
{
	// gets all UVs on the mesh
	XSI::CRefArray colours = this->accessor.GetVertexColors();

	// works with the first colours set on the geometry
	XSI::ClusterProperty colourProp = colours[ 0 ];
	XSI::CFloatArray colourValues;
	colourProp.GetValues( colourValues );

	return colourValues;
}

XSI::CLongArray MeshHelpers::MeshAccessor::GetVertexIndices()
{
    // retrieve all node indices per triangle on the geometry
    // nodes are grouped in triplets, first triplet being the nodes for
    // triangle 0, etc...
	XSI::CLongArray triVertexIds;
    this->accessor.GetTriangleVertexIndices( triVertexIds );
	return triVertexIds;
}

XSI::CLongArray MeshHelpers::MeshAccessor::GetNodeIndices()
{
    // retrieve all node indices per triangle on the geometry
    // nodes are grouped in triplets, first triplet being the nodes for
    // triangle 0, etc...
    XSI::CLongArray triNodeIds;
    this->accessor.GetTriangleNodeIndices( triNodeIds );
	return triNodeIds;
}

int MeshHelpers::MeshAccessor::GetTriangleCount()
{
	return this->accessor.GetTriangleCount();
}

XSI::CRefArray MeshHelpers::MeshAccessor::GetMaterials()
{
	return this->accessor.GetMaterials();
}

XSI::CLongArray MeshHelpers::MeshAccessor::GetPolygonMaterialIndices()
{
	XSI::CLongArray indices;
	this->accessor.GetPolygonMaterialIndices(indices);
	return indices;
}

XSI::CLongArray MeshHelpers::MeshAccessor::GetPolygonTriangleIndices()
{
	XSI::CLongArray indices;
	this->accessor.GetPolygonTriangleIndices(indices);
	return indices;
}
