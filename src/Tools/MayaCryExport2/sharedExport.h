#ifndef __SHAREDEXPORT_H__
#define __SHAREDEXPORT_H__

#include "Export/IExportContext.h"
#include "Export/IGeometryFileData.h"
#include "Export/ISkeletonData.h"
#include "Export/IMaterialData.h"
#include "Export/IModelData.h"
#include "PropertyHelpers.h"
#include "mayaIncludes.h"
#include "mayaNamePool.h"

void SharedCleanupPath( std::string &path );
std::string GetGeometryFileName(MDagPath dagPath);
bool BuildSelectionList( MSelectionList &outputSelectionList );
bool GetNodePathFromName( std::string &nodePathString, MDagPath &outNodePath );
void GatherExportNode( MDagPathArray &objectArray );
MDagPath GetMeshForBone( MDagPath bonePath );
//IGeometryFileData::ContentType GetContentTypeForExportNode(IExportContext* context, MDagPath dagPath);

void AddGeometryFiles( mayaNamePool *namePool, IExportContext* context, IGeometryFileData* geometryFileData);

bool FindSkeletonRootFromExportNode( MDagPath &nodePath, MDagPath &outSkeletonRoot );
void RecurseHierarchyAndListBones( mayaNamePool *namePool, ISkeletonData* skeletonData, MDagPath nodePath, int parentIndex, const IMaterialData* materialData );
bool ReadSkeletonAndListBones(mayaNamePool *namePool, const MDagPath &nodePath, const IGeometryFileData* geometryFileData, int geometryFileIndex, const IModelData* modelData, int modelIndex, const IMaterialData* materialData, ISkeletonData* skeletonData);

#endif // __SHAREDEXPORT_H__