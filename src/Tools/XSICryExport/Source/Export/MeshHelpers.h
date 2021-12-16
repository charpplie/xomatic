#ifndef __MESHHELPERS_H__
#define __MESHHELPERS_H__

#include <xsi_x3dobject.h>
#include <xsi_polygonmesh.h>
#include <xsi_geometryaccessor.h>

namespace MeshHelpers
{
	class MeshAccessor
	{
	public:
		MeshAccessor(XSI::X3DObject& object);

		bool IsValid();

		XSI::CDoubleArray GetPositions();
		XSI::CFloatArray GetNormals();
		XSI::CFloatArray GetUVs();
		XSI::CFloatArray GetColours();

		XSI::CLongArray GetVertexIndices();
		XSI::CLongArray GetNodeIndices();

		XSI::CRefArray GetMaterials();
		XSI::CLongArray GetPolygonMaterialIndices();
		XSI::CLongArray GetPolygonTriangleIndices();

		int GetTriangleCount();

		XSI::X3DObject object;
		XSI::PolygonMesh mesh;
		XSI::CGeometryAccessor accessor;
	};
}

#endif //__MESHHELPERS_H__
