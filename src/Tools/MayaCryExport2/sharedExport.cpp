#include "StdAfx.h"
#include "sharedExport.h"
#include "mayaUtilities.h"
#include "attribute.h"

#include "PathHelpers.h"

// Remove any '/../'s from the path.
void SharedCleanupPath( std::string &path )
{
	if( PathHelpers::IsRelative(path) ) // Only try to clean up absolute paths
		return;
	const char *pathPtr = path.c_str();
	int pathLength = strlen(pathPtr);
	int slashPos = 0;
	while( slashPos < pathLength-2 )
	{
		if( (pathPtr[slashPos+0] == '/' || pathPtr[slashPos+0] == '\\') && pathPtr[slashPos+1] == '.' && pathPtr[slashPos+2] == '.' )
		{
			int prevSlashPos = slashPos-1;
			while( prevSlashPos >= 0 && pathPtr[prevSlashPos] != '/' && pathPtr[prevSlashPos] != '\\' ) prevSlashPos--;
			int afterSlashPos = slashPos+1;
			while( afterSlashPos < pathLength && pathPtr[afterSlashPos] != '/' && pathPtr[afterSlashPos] != '\\' ) afterSlashPos++;
			if( prevSlashPos >= 0 && afterSlashPos < pathLength )
			{
				std::string newPath = path.substr( 0, prevSlashPos );
				newPath.append( path.substr( afterSlashPos, pathLength ) );
				path = newPath;
				pathPtr = path.c_str();
				pathLength = strlen(pathPtr);
				slashPos = prevSlashPos;
			}
			else // Can't remove
			{
				slashPos++;
			}
		}
		else
		{
			slashPos++;
		}
	}
}

std::string GetGeometryFileName(MDagPath dagPath)
{
	MFnDagNode dagNode(dagPath);
	std::string dagNodeName;
	removeNamespace( dagNode.name().asChar(), dagNodeName );

	const char prefix[] = "cryexportnode";
	char lowerCaseNamePrefix[sizeof(prefix) / sizeof(prefix[0])];
	strncpy(lowerCaseNamePrefix, dagNodeName.c_str(), strlen(prefix));
	lowerCaseNamePrefix[strlen(prefix)] = 0;
	std::string modelName;
	if (_stricmp(lowerCaseNamePrefix, prefix) == 0)
		modelName = dagNodeName.c_str() + strlen(lowerCaseNamePrefix);
	if (modelName.size() >= 1 && modelName[0] == '_')
		modelName = modelName.substr(1);
	return modelName;
}

bool BuildSelectionList( MSelectionList &outputSelectionList )
{
	outputSelectionList.clear();

	// Get a list of all top level nodes
	gatherTopLevelNodes( outputSelectionList );

	return true;
}

bool GetNodePathFromName( std::string &nodePathString, MDagPath &outNodePath )
{
	MString nodePathMString;
	nodePathMString.set( nodePathString.c_str() );

	MSelectionList sList;
	MGlobal::getSelectionListByName( nodePathMString, sList );
	if( sList.length() == 1 )
	{
		sList.getDagPath( 0, outNodePath );
		return true;
	}
	return false;
}

/*IGeometryFileData::ContentType GetContentTypeForExportNode(IExportContext* context, MDagPath dagPath)
{
	MStatus status;
	IGeometryFileData::ContentType contentType = IGeometryFileData::ContentType_CGF;
	MPlug plug;
	plug = MFnDependencyNode( dagPath.node() ).findPlug( "exportType", &status );

	MFnEnumAttribute enumAttribute( plug.attribute(), &status );
	int iVal;
	status = plug.getValue( iVal );
	MString enumString = enumAttribute.fieldName( iVal );

	if( strstr( enumString.asChar(), ".CGF" ) )
		contentType = IGeometryFileData::ContentType_CGF;
	else if( strstr( enumString.asChar(), ".CHR" ) )
		contentType = IGeometryFileData::ContentType_CHR;
	else if( strstr( enumString.asChar(), ".CGA" ) )
		contentType = IGeometryFileData::ContentType_CGA;
	else if( strstr( enumString.asChar(), ".ANM" ) )
		contentType = IGeometryFileData::ContentType_ANM;

	return contentType;
}*/

void GatherExportNode( MDagPathArray &objectArray )
{
	MSelectionList selectionList;
	BuildSelectionList( selectionList );

	findNodesInList( "cryexportnode*", selectionList, objectArray, FNBN_IGNORECASE, MFn::kInvalid );
}

MDagPath GetMeshForBone( MDagPath bonePath )
{
	MDagPath boneMesh;
	// The bones geometry is the first direct child of the bone thats a mesh.
	// Iterate until we find a child thats a mesh.
	for( int childIndex = 0;childIndex<bonePath.childCount();childIndex++ )
	{
		MDagPath boneChildPath = bonePath;
		boneChildPath.push( bonePath.child( childIndex ) );
		if( boneChildPath.hasFn( MFn::kMesh ) )
		{
			boneMesh = boneChildPath;
			boneMesh.extendToShapeDirectlyBelow(0);
			break;
		}
		boneChildPath.pop(1);
	}
	return boneMesh;
}

void AddGeometryFiles( mayaNamePool *namePool, IExportContext* context, IGeometryFileData* geometryFileData)
{
	MDagPathArray objectArray;
	GatherExportNode( objectArray );

	int i;
	for( i = 0;i<objectArray.length();i++ )
	{
		std::string geometryFileName = GetGeometryFileName(objectArray[i]);
		MDagPath dagPath = objectArray[i];
		void *dagPathString = namePool->getPointer( dagPath.fullPathName().asChar() );

		//std::string properties = getNodeProperties( dagPath );
		//geometryFileData->AddGeometryFile(dagPathString, geometryFileName.c_str(), properties);
		IGeometryFileData::SProperties properties;

		std::string propertiesString = getNodeProperties( dagPath );
		std::string value;
		if( PropertyHelpers::GetPropertyValue(propertiesString, "fileType", value) )
		{
			properties.filetypeInt = ExportFileTypeHelpers::StringToCryFileType(value.c_str());
			properties.bDoNotMerge = PropertyHelpers::GetPropertyValue(propertiesString, "DoNotMerge", value);
			geometryFileData->AddGeometryFile(dagPathString, geometryFileName.c_str(), properties);
		}
		else
		{
			Log( context, IExportContext::MessageSeverity_Error, "Export node `%s` has no type.", MFnDagNode(dagPath.node()).name().asChar() );
		}
	}
}

void RecurseHierarchyAndListBones( mayaNamePool *namePool, ISkeletonData* skeletonData, MDagPath nodePath, int parentIndex, const IMaterialData* materialData, const bool isAnimationExport )
{
	if( !nodePath.hasFn( MFn::kJoint ) ) // If the current node is not a joint, stop the recursion here.
		return;

	MFnDagNode dagNode(nodePath);

	void *dagPathString = namePool->getPointer( nodePath.fullPathName().asChar() );
	std::string boneName;
	convertToGameName( dagNode.name().asChar(), boneName );
	int boneIndex = skeletonData->AddBone( dagPathString, boneName.c_str(), parentIndex );

	std::string nodeName = dagNode.name().asChar();

	// Check whether the bone has geometry.
	MFnMesh boneMesh;
	MDagPath boneMeshPath = GetMeshForBone( nodePath );
	bool hasMesh = false;
	if( !isAnimationExport && boneMeshPath.isValid() )
		hasMesh = true;
	skeletonData->SetHasGeometry(boneIndex, hasMesh);
	skeletonData->SetPhysicalized(boneIndex, hasMesh);

	float exportScale = 1.f;
	float translationVec[3] = { 0.0f, 0.0f, 0.0f };
	float rotationVec[3] = { 0.0f, 0.0f, 0.0f };
	float scalingVec[3] = { 1.0f, 1.0f, 1.0f };
	
	getNodePosScaleRotation( nodePath, translationVec, scalingVec, rotationVec );

	translationVec[0] *= exportScale;
	translationVec[1] *= exportScale;
	translationVec[2] *= exportScale;

	skeletonData->SetTranslation( boneIndex, translationVec );
	skeletonData->SetRotation( boneIndex, rotationVec );
	skeletonData->SetScale( boneIndex, scalingVec );

	// Check for phys mesh nodes.
	static const char* physBoneSuffix = "_Phys";
	static const char* physParentFrameBoneSuffix = "_PhysParentFrame";
	std::string physBoneName = nodeName + physBoneSuffix;
	std::string physParentFrameBoneName = nodeName + physParentFrameBoneSuffix;

	MDagPath physBone = findNodeInScene( physBoneName.c_str(), FNBN_FULLNAMEONLY, MFn::kInvalid );
	MDagPath physParentFrameBone = findNodeInScene( physParentFrameBoneName.c_str(), FNBN_FULLNAMEONLY, MFn::kInvalid );

	// Set the parent frame matrix if it exists.
	if( physParentFrameBone.isValid() )
	{
		getNodePosScaleRotation( physParentFrameBone, translationVec, scalingVec, rotationVec, true );
		translationVec[0] *= exportScale;
		translationVec[1] *= exportScale;
		translationVec[2] *= exportScale;	

		skeletonData->SetParentFrameTranslation(boneIndex, translationVec);
		skeletonData->SetParentFrameRotation(boneIndex, rotationVec);
		skeletonData->SetParentFrameScale(boneIndex, scalingVec);
	}

	// Bone limits etc
	if( hasMesh )
	{
		MFnTransform nodeTransform(nodePath);

		if( nodeTransform.isLimited( MFnTransform::kRotateMinX ) )
			skeletonData->SetLimit(boneIndex, ISkeletonData::AxisX, ISkeletonData::LimitMin, (float)nodeTransform.limitValue( MFnTransform::kRotateMinX ) );
		if( nodeTransform.isLimited( MFnTransform::kRotateMaxX ) )
			skeletonData->SetLimit(boneIndex, ISkeletonData::AxisX, ISkeletonData::LimitMax, (float)nodeTransform.limitValue( MFnTransform::kRotateMaxX ) );
		if( nodeTransform.isLimited( MFnTransform::kRotateMinY ) )
			skeletonData->SetLimit(boneIndex, ISkeletonData::AxisY, ISkeletonData::LimitMin, (float)nodeTransform.limitValue( MFnTransform::kRotateMinY ) );
		if( nodeTransform.isLimited( MFnTransform::kRotateMaxY ) )
			skeletonData->SetLimit(boneIndex, ISkeletonData::AxisY, ISkeletonData::LimitMax, (float)nodeTransform.limitValue( MFnTransform::kRotateMaxY ) );
		if( nodeTransform.isLimited( MFnTransform::kRotateMinZ ) )
			skeletonData->SetLimit(boneIndex, ISkeletonData::AxisZ, ISkeletonData::LimitMin, (float)nodeTransform.limitValue( MFnTransform::kRotateMinZ ) );
		if( nodeTransform.isLimited( MFnTransform::kRotateMaxZ ) )
			skeletonData->SetLimit(boneIndex, ISkeletonData::AxisZ, ISkeletonData::LimitMax, (float)nodeTransform.limitValue( MFnTransform::kRotateMaxZ ) );

		if( nodePath.hasFn( MFn::kJoint ) )
		{
			float vX, vY, vZ;

			getFloat3AttributeValue( nodePath.node(), "spring", vX, vY, vZ );
			skeletonData->SetSpringAngle(boneIndex, ISkeletonData::AxisX, vX );
			skeletonData->SetSpringAngle(boneIndex, ISkeletonData::AxisY, vY );
			skeletonData->SetSpringAngle(boneIndex, ISkeletonData::AxisZ, vZ );

			getFloat3AttributeValue( nodePath.node(), "springTension", vX, vY, vZ );
			skeletonData->SetSpringTension(boneIndex, ISkeletonData::AxisX, vX );
			skeletonData->SetSpringTension(boneIndex, ISkeletonData::AxisY, vY );
			skeletonData->SetSpringTension(boneIndex, ISkeletonData::AxisZ, vZ );

			getFloat3AttributeValue( nodePath.node(), "damping", vX, vY, vZ );
			skeletonData->SetAxisDamping(boneIndex, ISkeletonData::AxisX, vX );
			skeletonData->SetAxisDamping(boneIndex, ISkeletonData::AxisY, vY );
			skeletonData->SetAxisDamping(boneIndex, ISkeletonData::AxisZ, vZ );
		}
	}

	// Recurse to the node's children.
	int childCount = nodePath.childCount();
	for( int childIndex = 0;childIndex < childCount;childIndex++ )
	{
		nodePath.push( nodePath.child( childIndex ) );
		RecurseHierarchyAndListBones( namePool, skeletonData, nodePath, boneIndex, materialData, isAnimationExport );
		nodePath.pop(1);
	}
}

bool FindSkeletonRootFromExportNode( MDagPath &nodePath, MDagPath &outSkeletonRoot )
{
	MStatus status;
	int i, numInfs, numGeoms, index;
	
	// Find a mesh from the root node
	MDagPath meshPath = findNodeInHierarchy( NULL, nodePath, 0, MFn::kMesh );
	if( !meshPath.isValid() )
		return false;

	MItDependencyNodes iter( MFn::kSkinClusterFilter );
	for( ; !iter.isDone(); iter.next() ) 
	{
		MObject object = iter.item();

		// For each skinCluster node, get the list of influence objects
		MFnSkinCluster skinCluster(object);
		MDagPathArray infs;
		numInfs = skinCluster.influenceObjects(infs, &status);

		if( infs.length() == 0 )
			continue;

		// Loop through the geometries affected by this cluster
		numGeoms = skinCluster.numOutputConnections();
		for(i = 0; i < numGeoms; ++i) 
		{
			index = skinCluster.indexForOutputConnection(i,&status);

			MDagPath skinPath;
			status = skinCluster.getPathAtIndex( index, skinPath );

			if( !(skinPath == meshPath) )
				continue;

			// We now know this skin cluster affects the node we are interested in.
			MDagPath boneNode = infs[0];
			while( boneNode.length() > 0 )
			{
				MDagPath parentNode = boneNode;
				parentNode.pop( 1 );
				if( parentNode.hasFn(MFn::kJoint) )
					boneNode = parentNode;
				else
					break;
			}
			if( boneNode.isValid() )
			{
				outSkeletonRoot = boneNode;
				return true;
			}
		}
	}
	return false;
}

bool ReadSkeletonAndListBones(mayaNamePool *namePool, const MDagPath &nodePath, const IGeometryFileData* const geometryFileData, int geometryFileIndex, const IModelData* const modelData, int modelIndex, const IMaterialData* materialData, ISkeletonData* skeletonData)
{
	bool nodeHasSkin = false;
	MStatus status;

	IGeometryFileData::SProperties prop = geometryFileData->GetProperties( geometryFileIndex );
	if( prop.filetypeInt == CRY_FILE_TYPE_CHR || prop.filetypeInt == CRY_FILE_TYPE_ANM || prop.filetypeInt == CRY_FILE_TYPE_CAF )
	{
		if( nodePath.isValid() )
		{
			bool isAnimationExport = true;
			if( prop.filetypeInt == CRY_FILE_TYPE_CHR )
				isAnimationExport = false;
			RecurseHierarchyAndListBones( namePool, skeletonData, nodePath, -1, materialData, isAnimationExport );
			nodeHasSkin = true;
		}
	}

	return nodeHasSkin;
}