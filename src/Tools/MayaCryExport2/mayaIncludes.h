/******************************************************************************
** Andrew Rayson - Januray 2005
** Includes for maya headers and some defines
******************************************************************************/

#ifndef __MAYAINCLUDES_H__ 
#define __MAYAINCLUDES_H__

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#endif

#include <vector>
#include <string>

#include <maya/MPxCommand.h>

#include <maya/MStatus.h>
#include <maya/MGlobal.h>
#include <maya/MCommandResult.h>
#include <maya/MArgList.h>


#include <maya/MFnStringData.h>

#include <maya/MObjectArray.h>
#include <maya/MIntArray.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MFloatVectorArray.h> 
#include <maya/MVector.h>
#include <maya/MColorArray.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MMatrix.h>
#include <maya/MFloatMatrix.h>
#include <maya/MQuaternion.h>
#include <maya/MEulerRotation.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MDistance.h>

#include <maya/MFn.h>
#include <maya/MFnSet.h>
#include <maya/MFnPartition.h>
#include <maya/MFnTransform.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnNurbsSurface.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MFnLight.h>
#include <maya/MFnSpotLight.h>
#include <maya/MFnNonAmbientLight.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnWeightGeometryFilter.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnPhongShader.h>

#include <maya/MItDag.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItSelectionList.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItGeometry.h>

#include <maya/MFileIO.h>
#include <maya/MFileObject.h>
#include <maya/MPxFileTranslator.h>

#include <maya/MTime.h>
#include <maya/MAnimControl.h>
#include <maya/MAnimUtil.h>
#include <maya/MFnIkJoint.h>

#include <maya/M3dView.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MDrawData.h>
#include <maya/MMaterial.h>
#include <maya/MFnCamera.h>

#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnTypedAttribute.h>

#include <maya/MImage.h>

#endif // __MAYAINCLUDES_H__
