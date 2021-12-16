#include "StdAfx.h"
#include "mayaIncludes.h"
#include "mayaUtilities.h"
#include "mayaAnimUtilities.h"
#include "mayaError.h"
#include "GeometryExportSource.h"
#include "sharedExport.h"
#include "attribute.h"
#include "ExportFunctions.h"
#include "materialExport.h"
#include "PathHelpers.h"

#include "Export/ISkeletonData.h"
#include "Export/IModelData.h"
#include "Export/IAnimationData.h"
#include "Export/IGeometryFileData.h"
#include "Export/IGeometryData.h"
#include "Export/IGeometryMaterialData.h"
#include "Export/IMaterialData.h"
#include "Export/IExportContext.h"
#include "Export/ISkinningData.h"
#include "Export/AnimationData.h"
#include "Export/IMorphData.h"

void GeometryExportSource::GetMetaData(SExportMetaData& metaData) const
{
	ExportFunctions::GetMetaData(metaData);
}

std::string GeometryExportSource::GetDCCFileName() const
{
	MFileIO fio;
	MString currentFilename = fio.currentFile();
	return std::string(currentFilename.asChar());
}

std::string GeometryExportSource::GetExportDirectory() const
{
	std::string path = currentExporterOptions.customFilePath;

	if (path.empty())
	{
		path = currentExporterOptions.customName;
	}

	if (path.empty())
	{
		path = PathHelpers::GetDirectory(GetDCCFileName());
	}
	else if (PathHelpers::IsRelative(path))
	{
		path = PathHelpers::Join(PathHelpers::GetDirectory(GetDCCFileName()), path);
		SharedCleanupPath( path );
	}

	return path;
}

namespace
{
	enum eNodeType 
	{
		eGROUP_NODE_TYPE = 0,
		eHELPER_NODE_TYPE,
		eBRANCH_NODE_TYPE,
		eUNKNOWN_NODE_TYPE = 666,
	};

	enum eTokenLocation
	{
		ePREFIX_TOKEN = 0,
		ePOSTFIX_TOKEN,
		eUNKNOWN_TOKEN_LOCATION = 666,
	};

	struct sNodeType
	{
		eNodeType type;
		eTokenLocation location;
		const char * token;
	};
	
	sNodeType nodeTypes[] = 
	{
		{
			eGROUP_NODE_TYPE, ePOSTFIX_TOKEN, "_group"
		},
		{
			eHELPER_NODE_TYPE, ePOSTFIX_TOKEN, "_helper"
		},
		{
			eBRANCH_NODE_TYPE, ePREFIX_TOKEN, "branch"
		},
	};

	eNodeType GetNodeType(const MDagPath &nodepath)
	{
		MString nodename = nodepath.partialPathName();
		const char * nodestr = nodename.asChar();
		for (int idx = 0; idx < ARRAYSIZE(nodeTypes); idx++ )
		{
			const char * token = nodeTypes[idx].token;
			switch ( nodeTypes[idx].location )
			{
				case ePREFIX_TOKEN :
				{
					if ( strstr(nodestr,token) == nodestr )
					{
						return nodeTypes[idx].type;
					}
					break;
				}
				
				case ePOSTFIX_TOKEN :
				{
					const char * nodetoken = strrchr(nodestr,'_');
					if ( nodetoken && stricmp(nodestr,token) == 0 )
					{
						return nodeTypes[idx].type;
					}
					break;
				}
			}
		}
		return eUNKNOWN_NODE_TYPE;
	}

	bool isNodeOfType(const MDagPath &nodepath, eNodeType type)
	{
		MString nodename = nodepath.partialPathName();
		const char * nodestr = nodename.asChar();
		const char * typetoken = NULL;
		eTokenLocation tokenlocation = eUNKNOWN_TOKEN_LOCATION;
		for (int idx = 0; idx < ARRAYSIZE(nodeTypes); idx++ )
		{
			if ( type == nodeTypes[idx].type )
			{
				typetoken = nodeTypes[idx].token;
				tokenlocation = nodeTypes[idx].location;
				break;
			}
		}
		if ( typetoken )
		{
			switch ( tokenlocation )
			{
				case ePOSTFIX_TOKEN :
				{
					const char * nodetoken = strrchr(nodestr,'_');
					if ( nodetoken && stricmp(typetoken,nodetoken) == 0 )
					{
						return true;
					}
					break;
				}
				case ePREFIX_TOKEN :
				{
					if ( strstr(nodestr,typetoken) == nodestr )
					{
						return true;
					}
					break;
				}
			}
		}
		return false;
	}

	void GatherRecursiveModelNodes( const MDagPath &nodePath, MDagPathArray &outModelNodes, const MFn::Type filterType )
	{
		MStatus status;
		MFnDagNode dagNode( nodePath, &status );
		
		if( status == MS::kFailure || !nodePath.isValid() || dagNode.isIntermediateObject() )
			return;
		if( isNodeOfType(nodePath,eGROUP_NODE_TYPE) )
			return;

		if( filterType == MFn::kInvalid || nodePath.apiType() == filterType )
			outModelNodes.append( nodePath );

		int childCount = nodePath.childCount();
		for( int childIndex = 0;childIndex < childCount;childIndex++ )
		{
			MDagPath childPath;
			MFnDagNode(nodePath.child( childIndex )).getPath( childPath );
			GatherRecursiveModelNodes( childPath, outModelNodes, filterType );
		}
	}

	void GatherModelNodes( const MDagPath &modelPath, MDagPathArray &outModelNodes, const MFn::Type filterType = MFn::kInvalid )
	{
		int childCount = modelPath.childCount();
		for( int childIndex = 0;childIndex < childCount;childIndex++ )
		{
			MDagPath childPath;
			MFnDagNode(modelPath.child( childIndex )).getPath( childPath );
			GatherRecursiveModelNodes( childPath, outModelNodes, filterType );
		}
	}

	int GetMaterialIndex( const IMaterialData *materialData, MDagPath &nodePath, int polygonIndex )
	{
		MStatus status;
		int materialIndex = -1;

		if ( !(nodePath.hasFn(MFn::kMesh) && !nodePath.hasFn(MFn::kTransform)) )
			return materialIndex;

		MFnMesh nodeMesh(nodePath,&status);

		int meshInstanceNum = 0;
		if( nodePath.isInstanced() )
			meshInstanceNum = nodePath.instanceNumber();
		MObjectArray shaderArray;
		MIntArray shaderIndexArray;
		status = nodeMesh.getConnectedShaders(meshInstanceNum,shaderArray,shaderIndexArray);

		MObject shadingGroup;
		int shaderindex = shaderIndexArray[polygonIndex];
		if ( shaderindex == -1 )
		{
			shadingGroup = getPolygonShaderUsingMEL(nodePath,polygonIndex);
		}
		else
		{
			shadingGroup = shaderArray[shaderindex];
		}
		
		if ( shadingGroup.isNull() )
			return materialIndex;

		MObject surfaceshader;
		followPlug( shadingGroup, "surfaceShader", surfaceshader, MFn::kInvalid );
		MString shaderName = MFnDependencyNode( surfaceshader ).name(&status);
		
		int numMaterials = materialData->GetMaterialCount();
		for( int mIndex = 0;mIndex<numMaterials;mIndex++ )
		{
			std::string *materialHandle = static_cast<std::string*>(materialData->GetHandle( mIndex ));
			if( strcmp( materialHandle->c_str(), shaderName.asChar() ) == 0 )
			{
				materialIndex = mIndex;//materialData->GetID( mIndex );
				break;
			}
		}
		return materialIndex;
	}

	void RecurseHierarchyAndListModels(mayaNamePool *namePool, IModelData* modelData, MDagPath dagPath, int parentIndex)
	{
		int modelIndex = parentIndex;
		MString nodeName = dagPath.fullPathName().asChar();

		std::string modelName;
		removeNamespace( dagPath.partialPathName().asChar(), modelName );

		if ( dagPath.hasFn(MFn::kJoint) && dagPath.hasFn(MFn::kTransform) && strstr(dagPath.partialPathName().asChar(),"_joint"))
		{
			void *dagPathString = namePool->getPointer( dagPath.fullPathName().asChar() );
			SHelperData data;
			data.m_eHelperType = SHelperData::eHelperType_Dummy;

			float jointRadius = 0.0f;
			jointRadius = getFloatAttributeValue( dagPath.node(),"radi" ) / 100.0f;

			data.m_boundBoxMin[0] = data.m_boundBoxMin[1] = data.m_boundBoxMin[2] = -jointRadius;
			data.m_boundBoxMax[0] = data.m_boundBoxMax[1] = data.m_boundBoxMax[2] = jointRadius;
			
			std::string propertiesString = getNodeProperties( dagPath );
			
			modelIndex = modelData->AddModel( dagPathString, modelName.c_str(), parentIndex, false, data, propertiesString );

			float pos[3], scale[3], rot[3];
			getNodePosScaleRotation(dagPath, pos, scale, rot, true);
			modelData->SetTranslationRotationScale(modelIndex, pos, rot, scale);
		}
		else if ( dagPath.hasFn(MFn::kDagNode)&& dagPath.hasFn(MFn::kTransform) && !strstr(dagPath.partialPathName().asChar(),"cryexportnode") )
		{
			if ( isNodeOfType(dagPath,eHELPER_NODE_TYPE) )
			{
				void *dagPathString = namePool->getPointer( dagPath.fullPathName().asChar() );
				SHelperData data;
				data.m_eHelperType = SHelperData::eHelperType_Dummy;

				data.m_boundBoxMin[0] = -0.01f;
				data.m_boundBoxMin[1] = -0.01f;
				data.m_boundBoxMin[2] = -0.01f;

				data.m_boundBoxMax[0] = 0.01f;
				data.m_boundBoxMax[1] = 0.01f;
				data.m_boundBoxMax[2] = 0.01f;

				std::string propertiesString = getNodeProperties( dagPath );
				modelIndex = modelData->AddModel( dagPathString, modelName.c_str(), -1, false, data, propertiesString );

				float pos[3], scale[3], rot[3];
				pos[0] = pos[1] = pos[2] = 0.0f;
				scale[0] = scale[1] = scale[2] = 1.0f;
				rot[0] = rot[1] = rot[2] = 0.0f;
				modelData->SetTranslationRotationScale(modelIndex, pos, rot, scale);
			}

			if ( isNodeOfType(dagPath,eBRANCH_NODE_TYPE) )
			{
				void *dagPathString = namePool->getPointer( dagPath.fullPathName().asChar() );
				SHelperData data;
				data.m_eHelperType = SHelperData::eHelperType_Dummy;

				data.m_boundBoxMin[0] = -0.01f;
				data.m_boundBoxMin[1] = -0.01f;
				data.m_boundBoxMin[2] = -0.01f;

				data.m_boundBoxMax[0] = 0.01f;
				data.m_boundBoxMax[1] = 0.01f;
				data.m_boundBoxMax[2] = 0.01f;

				std::string propertiesString = getNodeProperties( dagPath );
				modelIndex = modelData->AddModel( dagPathString, modelName.c_str(), -1, false, data, propertiesString );

				float pos[3], scale[3], rot[3];
				getNodePosScaleRotation( dagPath, pos, scale, rot);
				float localPos[3];
				MDagPath shapeDagPath = dagPath;
				shapeDagPath.extendToShapeDirectlyBelow(0);
				getFloat3AttributeValue( shapeDagPath.node(), "localPosition", localPos[0], localPos[1], localPos[2] );
				pos[0] += localPos[0];
				pos[1] += localPos[1];
				pos[2] += localPos[2];
				modelData->SetTranslationRotationScale(modelIndex, pos, rot, scale);
			}

			if ( isNodeOfType(dagPath,eGROUP_NODE_TYPE) ) 
			{
				void *dagPathString = namePool->getPointer( dagPath.fullPathName().asChar() );
				SHelperData data;

				std::string propertiesString = getNodeProperties( dagPath );
				modelIndex = modelData->AddModel( dagPathString, modelName.c_str(), parentIndex, true, data, propertiesString );

				float pos[3], scale[3], rot[3];
				getNodePosScaleRotation( dagPath, pos, scale, rot);
				modelData->SetTranslationRotationScale(modelIndex, pos, rot, scale);
			}
		}

		// Recurse to the node's children.
		int childCount = dagPath.childCount();
		for( int childIndex = 0;childIndex < childCount;childIndex++ )
		{
			dagPath.push( dagPath.child( childIndex ) );
			RecurseHierarchyAndListModels( namePool, modelData, dagPath, modelIndex );
			dagPath.pop(1);
		}
	}

	struct MeshData
	{
		MPointArray positionArray;
		MFloatVectorArray normalArray;
		MStringArray uvSetNames;
		MFloatArray uArray,vArray;
		MStringArray colourSetNames;
		MColorArray colourArray;

		struct vertex
		{
			vertex() {}
			vertex( const int inP, const int inN, const int inT, const int inC ) : p(inP), n(inN), t(inT), c(inC) { }
			int p,n,t,c;
		};

		struct triangle
		{
			triangle() {}
			triangle( const int inMtlID ) : mtlID(inMtlID) { } 
			int mtlID;
			MeshData::vertex vtx[3];
		};
		std::vector<MeshData::triangle> triangles;
	};

	bool ReadMeshData( IExportContext* context, const IMaterialData *materialData, MDagPath nodePath, const MMatrix &inverseOrigin, MeshData &outMeshData )
	{
		int i;
		MStatus status;
		MString nodeName = nodePath.fullPathName().asChar();
		MSpace::Space space = MSpace::kWorld;
		MFnMesh nodeMesh(nodePath,&status);

		if ( status == MS::kSuccess && nodePath.hasFn(MFn::kMesh) )	
		{
			// Export the mesh data
			double exportScale = 0.01f;
			nodeMesh.syncObject();
			int defaultUVIndex = -1;
			int defaultColourIndex = -1;

			// Get the position array
			nodeMesh.getPoints( outMeshData.positionArray, space );
			for( i=0;i<outMeshData.positionArray.length();i++ )
				outMeshData.positionArray[i] = (outMeshData.positionArray[i] * exportScale) * inverseOrigin;

			// Get the normal array
			nodeMesh.getNormals(outMeshData.normalArray,space);

			// Export the UV sets
			nodeMesh.getUVSetNames( outMeshData.uvSetNames );	
			int numUVSets = outMeshData.uvSetNames.length();
			if( numUVSets > 0 )
				nodeMesh.getUVs(outMeshData.uArray,outMeshData.vArray,&outMeshData.uvSetNames[0]); // Only bother with the first uv set as that what max seems to do.

			// Export the colour arrays
			nodeMesh.getColorSetNames( outMeshData.colourSetNames );
			int numColourSets = outMeshData.colourSetNames.length();
			if( numColourSets > 0 )
				nodeMesh.getFaceVertexColors( outMeshData.colourArray,&(outMeshData.colourSetNames[0]) ); // Only bother with the first colour set as thats all max seems to bother with.
		
			bool meshUVError = false;
			MItMeshPolygon polygonIt( nodePath, MObject::kNullObj );
			for( ;!polygonIt.isDone(); polygonIt.next() ) 
			{
				int colourIndex, vertexIndex, normalIndex, uvIndex;
				int vertexCount = polygonIt.polygonVertexCount();

				int materialIndex = GetMaterialIndex( materialData, nodePath, polygonIt.index() );
				std::map< int, int > polygonVertexIndices;
				for( int v = 0; v < vertexCount; v++)
					polygonVertexIndices.insert( std::make_pair( polygonIt.vertexIndex(v), v ) );

				int defaultUVIndex = -1;
				int numTriangles;
				polygonIt.numTriangles( numTriangles );
				for( int t = 0;t<numTriangles;t++ )
				{
					MPointArray points;
					MIntArray vertexList;
					polygonIt.getTriangle( t, points, vertexList, space );

					MeshData::triangle tri( materialIndex );
					for( int v=0;v<3;v++ ) // Vertex count is always 3
					{
						int vIndex = polygonVertexIndices.find( vertexList[v] )->second;
						vertexIndex = polygonIt.vertexIndex( vIndex );
						normalIndex = polygonIt.normalIndex( vIndex );
						if( outMeshData.uvSetNames.length() > 0 )
							polygonIt.getUVIndex(vIndex,uvIndex,&outMeshData.uvSetNames[0]);
						else
							uvIndex = -1;
						if( outMeshData.colourSetNames.length() > 0 )
							nodeMesh.getFaceVertexColorIndex(polygonIt.index(),vIndex,colourIndex,&(outMeshData.colourSetNames[0]));
						else
							colourIndex = -1;

						// Make sure we have a valid uvIndex. Create a default if needed.
						if( uvIndex < 0 || uvIndex >= outMeshData.uArray.length() )
						{
							if( defaultUVIndex < 0 )
							{
								defaultUVIndex = outMeshData.uArray.length();
								outMeshData.uArray.append( 0.0f );
								outMeshData.vArray.append( 0.0f );
							}
							uvIndex = defaultUVIndex;
							meshUVError = true;
						}

						tri.vtx[v] = MeshData::vertex( vertexIndex, normalIndex, uvIndex, colourIndex );
					}
					outMeshData.triangles.push_back(tri);
				}
			}
			if( meshUVError )
				Log( context, IExportContext::MessageSeverity_Error, "Some vertices in mesh `%s` have no uv's.", nodeName.asChar() );
		}

		return true;
	}

	bool WriteMeshData( IExportContext* context, IGeometryData* geometry, MeshData &meshData )
	{
		int i, v;
		const int indexOffsetGeomPositions = geometry->GetNumberOfPositions();
		const int indexOffsetGeomNormals = geometry->GetNumberOfNormals();
		const int indexOffsetGeomTextureCoordinates = geometry->GetNumberOfTextureCoordinates();
		const int indexOffsetGeomColours = geometry->GetNumberOfVertexColours();

		for( i=0;i<meshData.positionArray.length();i++ )
			geometry->AddPosition( (float)(meshData.positionArray[i].x), (float)(meshData.positionArray[i].y), (float)(meshData.positionArray[i].z) );
		for( i=0;i<meshData.normalArray.length();i++ )
			geometry->AddNormal( meshData.normalArray[i].x, meshData.normalArray[i].y, meshData.normalArray[i].z );
		for( i=0;i<meshData.uArray.length();i++ )
			geometry->AddTextureCoordinate( meshData.uArray[i], meshData.vArray[i] );
		for( i=0;i<meshData.colourArray.length();i++ )
			geometry->AddVertexColour( meshData.colourArray[i].r, meshData.colourArray[i].g, meshData.colourArray[i].b, meshData.colourArray[i].a );


		for( i=0;i<meshData.triangles.size();i++ )
		{
			int indices[12];
			for( v=0;v<3;v++ )
			{
				indices[(v*4)+0] = meshData.triangles[i].vtx[v].p + indexOffsetGeomPositions;
				indices[(v*4)+1] = meshData.triangles[i].vtx[v].n + indexOffsetGeomNormals;
				indices[(v*4)+2] = meshData.triangles[i].vtx[v].t + indexOffsetGeomTextureCoordinates;
				indices[(v*4)+3] = meshData.triangles[i].vtx[v].c + indexOffsetGeomColours;
			}
			geometry->AddPolygon( indices, meshData.triangles[i].mtlID );
		}
		return true;
	}

	bool ReadGeometryDataForNode(IExportContext* context, IGeometryData* geometry, MDagPath nodePath, const IMaterialData *materialData, TMeshNameToGeomIndexMap &nameIndexMap, const MMatrix &inverseOrigin )
	{
		MStatus status;
		MString nodeName = nodePath.fullPathName().asChar();
		MSpace::Space space = MSpace::kWorld;
		MFnDagNode dagNode( nodePath, &status );

		if( dagNode.isIntermediateObject() )
			return true;

		MFnMesh nodeMesh(nodePath,&status);
		if ( status == MS::kSuccess && nodePath.hasFn(MFn::kMesh) && !nodePath.hasFn(MFn::kTransform) )	
		{
			MeshData meshData;
			ReadMeshData( context, materialData, nodePath, inverseOrigin, meshData );

			const int indexOffsetGeomPositions = geometry->GetNumberOfPositions();
			nameIndexMap.insert( std::make_pair(nodeName.asChar(), indexOffsetGeomPositions) );

			WriteMeshData( context, geometry, meshData );
		}

		return true;
	}

	bool ReadGeometryDataForNode(IExportContext* context, MDagPath nodePath, IGeometryMaterialData* geometryMaterialData, const IMaterialData* materialData)
	{
		bool foundInvalidIndex = false;

		if( MFnDagNode( nodePath ).isIntermediateObject() )
			return false;

		if ( nodePath.hasFn(MFn::kMesh) && !nodePath.hasFn(MFn::kTransform) )
		{
			MItMeshPolygon polygonIt( nodePath, MObject::kNullObj );
			for( ;!polygonIt.isDone(); polygonIt.next() ) 
			{
				int materialIndex = GetMaterialIndex( materialData, nodePath, polygonIt.index() );
				if ( materialIndex == -1 )
					foundInvalidIndex = true;
				
				geometryMaterialData->AddUsedMaterialIndex( materialIndex );
			}
		}

		if ( foundInvalidIndex )
			Log(context,IExportContext::MessageSeverity_Error,"Mesh '%s' contains polygons with an invalid material index", nodePath.partialPathName().asChar());

		return true;
	}

	std::string GetMaterialProperties( MObject surfaceshader )
	{
		bool foundPhysicalise = false;

		std::string properties = "";
		if( !surfaceshader.isNull() )
		{
			// The physics setting is pulled directly from the names in the enum control.
			std::string physicsSetting = getAttributeValue( surfaceshader, "physicalise" );
			if( physicsSetting.length() > 0 )
			{
				properties += "__phys" + physicsSetting;
				foundPhysicalise = true;
			}
		}

		if( !foundPhysicalise )
			properties = "__physNone";
		
		return properties;
	}

	void ReadSkinningDataForNode(IExportContext* context, MDagPath nodePath, ISkinningData* skinningData, MObjectArray boneIndexMap, TMeshNameToGeomIndexMap &nameIndexMap )
	{
		MStatus status;
		int i, j, numInfs, numGeoms, index;
		MFnDagNode dagNode( nodePath );

		if( dagNode.isIntermediateObject() )
			return;

		if ( nodePath.hasFn(MFn::kMesh) && !nodePath.hasFn(MFn::kTransform) )
		{
			MItDependencyNodes iter( MFn::kSkinClusterFilter );
			for( ; !iter.isDone(); iter.next() ) 
			{
				MObject object = iter.item();

				// For each skinCluster node, get the list of influence objects
				MFnSkinCluster skinCluster(object);
				MDagPathArray infs;
				numInfs = skinCluster.influenceObjects(infs, &status);

				numGeoms = skinCluster.numOutputConnections();
				for(i = 0; i < numGeoms; ++i) 
				{
					index = skinCluster.indexForOutputConnection(i,&status);

					MDagPath skinPath;
					status = skinCluster.getPathAtIndex( index, skinPath );

					if( !(skinPath == nodePath) )
						continue;

					// The mesh is affected by this cluster
					MItGeometry gIter(skinPath);
					std::string skinMeshName = skinPath.fullPathName().asChar();
					int meshBaseVertexIndex = nameIndexMap[skinMeshName];
					skinningData->SetVertexCount( meshBaseVertexIndex + gIter.exactCount() );

					for( ; !gIter.isDone(); gIter.next() ) 
					{
						MObject comp = gIter.currentItem(&status);

						MFloatArray weights;
						unsigned int infCount;
						status = skinCluster.getWeights(skinPath,comp,weights,infCount);
						float totalWeight = 0.0f;
						int totalNumInfs = 0;
						if( infCount > 0 )
						{
							for (j = 0; j < infCount ; ++j )
							{
								if( weights[j] > 0.0f )
								{
									int boneIndex = -1;
									// TODO: This is slow. Needs speeding up with some sort of hash table etc.
									for( int k = 0;k<boneIndexMap.length();k++ )
									{
										if( boneIndexMap[k] == infs[j].node() )
										{
											boneIndex = k;
											break;
										}
									}
									if( boneIndex >= 0 )
									{
										skinningData->AddWeight( meshBaseVertexIndex + gIter.index(), boneIndex, weights[j] );
										totalWeight += weights[j];
										totalNumInfs++;
									}
								}
							}
						}
						if( fabsf( totalWeight - 1.0f) > 0.01f )
						{
							Log( context, IExportContext::MessageSeverity_Error, "Vertex weights don't add up to 1. (%f)", totalWeight );
						}
						const int CMaxInfs = 4;
						if( totalNumInfs > CMaxInfs )
						{
							Log( context, IExportContext::MessageSeverity_Error, "Too many (%d) infs on a vertex, Max %d.", totalNumInfs, CMaxInfs );
						}
					}
				}
			}
		}
	}

	enum eControllers
	{
		CONTROLLER_TRANSLATE = 0,
		CONTROLLER_ROTATE,
		CONTROLLER_SCALE,
	};

	const char * controllers[] = {"translate","rotate","scale"};

	bool hasValidAnimationController(const IModelData* modelData, int modelIndex, const int controller)
	{
		std::string *dagPathString = static_cast<std::string*>(modelData->GetModelHandle(modelIndex));
		MDagPath dagPath;
		if( GetNodePathFromName( *dagPathString, dagPath ) )
		{
			MPlugArray animatedControllers;
			if ( MAnimUtil::isAnimated(dagPath) && MAnimUtil::findAnimatedPlugs(dagPath,animatedControllers) )
			{
				for (int idx = 0; idx < animatedControllers.length(); idx++ )
				{
					MString ctrlstr = animatedControllers[idx].name();
					char controllerName[256];
					_snprintf(controllerName, sizeof(controllerName), "%s", ctrlstr.asChar());
					if ( strstr(controllerName, controllers[controller]) )
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	void getAnimationPlugs(const MDagPath& node, const int controller, MPlugArray &plugs)
	{
		MPlugArray animatedControllers;
		if ( MAnimUtil::isAnimated(node) && MAnimUtil::findAnimatedPlugs(node,animatedControllers) )
		{
			for (int idx = 0; idx < animatedControllers.length(); idx++ )
			{
				MString ctrlstr = animatedControllers[idx].name();
				char controllerName[256];
				_snprintf(controllerName, sizeof(controllerName), "%s", ctrlstr.asChar());
				if ( strstr(controllerName, controllers[controller]) )
				{
					plugs.append(animatedControllers[idx]);
				}
			}
		}
	}
	
	bool populateControllerKeyData(const MDagPath & dagPath, const IModelData* modelData, int modelIndex, const int controller, std::vector<float> & keydata, int & keycount )
	{
		bool ret = false;
		keycount = 0;
		if ( hasValidAnimationController(modelData,modelIndex,controller) )
		{
			MPlugArray plugs;
			MObjectArray anims;
			getAnimationPlugs(dagPath,controller,plugs);
			if ( MAnimUtil::findAnimation(plugs[0],anims) ) 
			{
				int firstFrame, lastFrame;
				GetAnimationRange( firstFrame, lastFrame, currentExporterOptions);
				for( int frameidx = firstFrame; frameidx <= lastFrame; frameidx++ )
				{
					MTime time;
					float translate[3], rotate[3], scale[3];

					time.setUnit( time.uiUnit() );
					time.setValue( firstFrame + frameidx );
					float timeValue = (float)time.as( MTime::kSeconds );

					MAnimControl::setCurrentTime(time);
					getNodePosScaleRotation( dagPath, translate, scale, rotate, false);

					switch (controller)
					{
						case CONTROLLER_TRANSLATE : keydata.push_back(translate[0]);
													keydata.push_back(translate[1]);
													keydata.push_back(translate[2]);
							break;
						case CONTROLLER_ROTATE :	keydata.push_back(rotate[0]);
													keydata.push_back(rotate[1]);
													keydata.push_back(rotate[2]);
							break;
						case CONTROLLER_SCALE :		keydata.push_back(scale[0]);
													keydata.push_back(scale[1]);
													keydata.push_back(scale[2]);
							break;
					}
					
					keydata.push_back(timeValue);
					keycount++;
					ret = true;
				}
			}
		}
		return ret;
	}

	bool AddRootMaterial( mayaNamePool *namePool, IExportContext* context, const std::string &rootMaterialName, const MObjectArray &surfaceShaders, IMaterialData* materialData )
	{
		MStatus status;

		int subMaterialID = 0;
		for( int i = 0;i<surfaceShaders.length();i++ )
		{
			MObject surfaceshader = surfaceShaders[i];
			MString shaderName = MFnDependencyNode( surfaceshader ).name(&status);

			void *dagPathString = namePool->getPointer( shaderName.asChar() );
			std::string properties = GetMaterialProperties( surfaceshader );
			int index = materialData->AddMaterial( rootMaterialName.c_str(), subMaterialID, dagPathString, properties.c_str() );
			subMaterialID++;
		}

		const int MaxSubMaterials = 32;
		if( subMaterialID > MaxSubMaterials )
		{
			Log( context, IExportContext::MessageSeverity_Error, "Too many sub materials in group `%s`. There's %d, Max %d.", rootMaterialName.c_str(), subMaterialID, MaxSubMaterials );
		}

		return true;
	}
}

void GeometryExportSource::ReadGeometryFiles(IExportContext* context, IGeometryFileData* geometryFileData)
{
	// Make sure the name to index map is cleared.
	m_meshNameToGeomIndexMap.clear();

	AddGeometryFiles( &m_namePool, context, geometryFileData );
}

void GeometryExportSource::ReadModels(const IGeometryFileData* geometryFileData, int geometryFileIndex, IModelData* modelData)
{
	std::string *dagPathString = static_cast<std::string*>(geometryFileData->GetGeometryFileHandle(geometryFileIndex));
	MDagPath dagPath;
	if( GetNodePathFromName( *dagPathString, dagPath ) )
	{
		RecurseHierarchyAndListModels( &m_namePool, modelData, dagPath, -1 );
	}
}

bool GeometryExportSource::ReadMaterials(IExportContext* context, const IGeometryFileData* const geometryFileData, IMaterialData* materialData)
{
	bool ret = true;

	MayaCryMaterials cryMaterials;
	buildMaterialArray( cryMaterials );

	for( int i = 0;i<cryMaterials.rootMaterials.size();i++ )
	{
		if( !AddRootMaterial( &m_namePool, context, cryMaterials.rootMaterials[i]->materialName, cryMaterials.rootMaterials[i]->surfaceShaders, materialData ) )
			ret = false;
	}

	return ret;
}

void GeometryExportSource::ReadSkinning(IExportContext* context, ISkinningData* skinningData, const IModelData* const modelData, int modelIndex, ISkeletonData* skeletonData)
{
	if ( modelData->GetHelperData(modelIndex).m_eHelperType == SHelperData::eHelperType_Dummy )
	{
		return;
	}

	// Build the bone index map
	MObjectArray boneIndexMap;
	int boneCount = skeletonData->GetBoneCount();
	for (int boneIndex = 0; boneIndex < boneCount; boneIndex++ )
	{
		std::string *bonePathString = static_cast<std::string*>(skeletonData->GetBoneHandle(boneIndex));
		MDagPath bonePath;
		if( GetNodePathFromName( *bonePathString, bonePath ) )
			boneIndexMap.append( bonePath.node() );
	}

	// Iterate over the mesh nodes and get the skinning data
	std::string *dagPathString = static_cast<std::string*>(modelData->GetModelHandle(modelIndex));
	MDagPath dagPath;
	if( GetNodePathFromName( *dagPathString, dagPath ) )
	{
		MDagPathArray modelMeshNodes;
		GatherModelNodes( dagPath, modelMeshNodes, MFn::kMesh );
		for( int i = 0;i<modelMeshNodes.length();i++ )
		{
			ReadSkinningDataForNode(context, modelMeshNodes[i], skinningData, boneIndexMap, m_meshNameToGeomIndexMap );
		}
	}
}


bool GeometryExportSource::ReadSkeleton(const IGeometryFileData* const geometryFileData, int geometryFileIndex, const IModelData* const modelData, int modelIndex, const IMaterialData* materialData, ISkeletonData* skeletonData)
{
	bool ret = false;
	MDagPath nodePath, skeletonRoot;
	
	std::string *nodePathString = static_cast<std::string*>(geometryFileData->GetGeometryFileHandle(geometryFileIndex));
	if( GetNodePathFromName( *nodePathString, nodePath ) )
	{
		if( FindSkeletonRootFromExportNode( nodePath, skeletonRoot ) )
		{
			ret = ReadSkeletonAndListBones( &m_namePool, skeletonRoot, geometryFileData, geometryFileIndex, modelData, modelIndex, materialData, skeletonData);
		}
	}
	return ret;
}

int GeometryExportSource::GetAnimationCount() const
{
	return 0;
}

std::string GeometryExportSource::GetAnimationName(const IGeometryFileData* geometryFileData, int geometryFileIndex, int animationIndex) const
{
	assert(animationIndex == 0); 

	return PathHelpers::RemoveExtension(PathHelpers::GetFilename(GetDCCFileName()));
}

void GeometryExportSource::GetAnimationTimeSpan(float& start, float& stop, int animationIndex) const
{
	assert(animationIndex == 0); // Only support 1 animation at a time.

	int firstFrame, lastFrame;
	GetAnimationRange( firstFrame, lastFrame, currentExporterOptions );

	start = getTimeFromFrame( firstFrame );
	stop = getTimeFromFrame( lastFrame );
}

void GeometryExportSource::ReadAnimationFlags(IExportContext* context, IAnimationData* animationData, const IGeometryFileData* const geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex) const
{
	assert(0);
}

IAnimationData * GeometryExportSource::ReadAnimation(IExportContext* context, const IGeometryFileData* const geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex, float fps) const
{
	int modelCount = modelData->GetModelCount();
	IAnimationData *animationData = new NonSkeletalAnimationData(modelCount);

	MTime currentTime = MAnimControl::currentTime();
	
	for (int modelIndex = 0; modelIndex < modelCount; ++modelIndex)
	{
		std::string *dagPathString = static_cast<std::string*>(modelData->GetModelHandle(modelIndex));
		MDagPath dagPath;
		if( GetNodePathFromName( *dagPathString, dagPath ) )
		{
			const int step = 4;
			const int timeidx = step - 1;
			int translatekeyscount = -1;		
			int rotatekeyscount = -1;
			int scalekeyscount = -1;

			std::vector<float> translateArray;
			std::vector<float> rotateArray;
			std::vector<float> scaleArray;

			bool translatekeys = populateControllerKeyData(dagPath,modelData,modelIndex,CONTROLLER_TRANSLATE,translateArray,translatekeyscount);
			bool rotatekeys = populateControllerKeyData(dagPath,modelData,modelIndex,CONTROLLER_ROTATE,rotateArray,rotatekeyscount);
			bool scalekeys = populateControllerKeyData(dagPath,modelData,modelIndex,CONTROLLER_SCALE,scaleArray,scalekeyscount);

			if ( translatekeys && translatekeyscount > 0 )
			{
				animationData->SetFrameCountPos(modelIndex,translatekeyscount);
				for ( int transidx = 0; transidx < translatekeyscount; ++transidx )
				{
					IAnimationData::TCB tcb;
					IAnimationData::Ease ease;

					tcb.bias = 0.0f;
					tcb.continuity = 0.0f;
					tcb.tension = 0.0f;
					ease.in = 0.0f;
					ease.out = 0.0f;

					animationData->SetFrameTCBPos(modelIndex,transidx,tcb);
					animationData->SetFrameEaseInOutPos(modelIndex,transidx,ease);
					animationData->SetFrameTimePos(modelIndex,transidx,translateArray[(transidx*step)+timeidx]);
					animationData->SetFrameDataPos(modelIndex,transidx,&translateArray[transidx*step]);
				}
			}

			if ( rotatekeys && rotatekeyscount > 0 )
			{
				animationData->SetFrameCountRot(modelIndex,rotatekeyscount);
				for ( int rotateidx = 0; rotateidx < rotatekeyscount; ++rotateidx )
				{
					// Rotation to degrees
					for( int axis = 0; axis < 3; axis++ )
					{
						rotateArray[(rotateidx * step) + axis] = RAD2DEG( rotateArray[(rotateidx * step) + axis] );
					}

					IAnimationData::TCB tcb;
					IAnimationData::Ease ease;

					tcb.bias = 0.0f;
					tcb.continuity = 0.0f;
					tcb.tension = 0.0f;
					ease.in = 0.0f;
					ease.out = 0.0f;

					animationData->SetFrameTCBRot(modelIndex,rotateidx,tcb);
					animationData->SetFrameEaseInOutRot(modelIndex,rotateidx,ease);
					animationData->SetFrameTimeRot(modelIndex,rotateidx,rotateArray[(rotateidx*step)+timeidx]);
					animationData->SetFrameDataRot(modelIndex,rotateidx,&rotateArray[rotateidx*step]);
				}
			}

			if ( scalekeyscount && scalekeyscount > 0 ) 
			{
				animationData->SetFrameCountScl(modelIndex,scalekeyscount);
				for ( int scaleidx = 0; scaleidx < scalekeyscount; ++scaleidx )
				{
					// Round scaling
					for( int axis = 0; axis < 3; axis++ )
					{
						float& value = scaleArray[(scaleidx * step) + axis];
						value = floorf(value * 1000.0f + 0.5f) / 1000.0f;
					}

					IAnimationData::TCB tcb;
					IAnimationData::Ease ease;

					tcb.bias = 0.0f;
					tcb.continuity = 0.0f;
					tcb.tension = 0.0f;
					ease.in = 0.0f;
					ease.out = 0.0f;

					animationData->SetFrameTCBScl(modelIndex,scaleidx,tcb);
					animationData->SetFrameEaseInOutScl(modelIndex,scaleidx,ease);
					animationData->SetFrameTimeScl(modelIndex,scaleidx,scaleArray[(scaleidx*step)+timeidx]);
					animationData->SetFrameDataScl(modelIndex,scaleidx,&scaleArray[scaleidx*step]);
				}
			}
		}
	}

	MAnimControl::setCurrentTime(currentTime);
	return animationData;
}

bool GeometryExportSource::ReadGeometry(IExportContext* context, IGeometryData* geometry, const IModelData* const modelData, const IMaterialData* materialData, int modelIndex)
{
	if ( modelData->GetHelperData(modelIndex).m_eHelperType == SHelperData::eHelperType_Dummy )
	{
		return true;
	}

	std::string *dagPathString = static_cast<std::string*>(modelData->GetModelHandle(modelIndex));
	MDagPath dagPath;
	if( GetNodePathFromName( *dagPathString, dagPath ) )
	{
		MMatrix inverseOrigin = dagPath.inclusiveMatrixInverse();
		MDagPathArray modelMeshNodes;
		GatherModelNodes( dagPath, modelMeshNodes, MFn::kMesh );
		for( int i = 0;i<modelMeshNodes.length();i++ )
		{
			ReadGeometryDataForNode( context, geometry, modelMeshNodes[i], materialData, m_meshNameToGeomIndexMap, inverseOrigin );
		}
	}

	return true;
}

bool GeometryExportSource::ReadGeometryMaterialData(IExportContext* context, IGeometryMaterialData* geometryMaterialData, const IModelData* const modelData, const IMaterialData* materialData, int modelIndex) const
{
	int i;
	std::string *dagPathString = static_cast<std::string*>(modelData->GetModelHandle(modelIndex));
	MDagPath dagPath;
	if( GetNodePathFromName( *dagPathString, dagPath ) )
	{
		MDagPathArray modelMeshNodes;
		GatherModelNodes( dagPath, modelMeshNodes, MFn::kMesh );

		for( i = 0;i<modelMeshNodes.length();i++ )
		{
			ReadGeometryDataForNode(context, modelMeshNodes[i], geometryMaterialData, materialData);
		}
	}

	return true;
}

bool GeometryExportSource::ReadBoneGeometry(IExportContext* context, IGeometryData* geometry, ISkeletonData* skeletonData, int boneIndex, const IMaterialData* materialData)
{
	std::string *dagPathString = static_cast<std::string*>(skeletonData->GetBoneHandle(boneIndex));
	MDagPath dagPath;
	if( GetNodePathFromName( *dagPathString, dagPath ) )
	{
		MDagPath boneMeshPath = GetMeshForBone( dagPath );
		if( boneMeshPath.isValid() )
		{
			// Export the bone geom in the bones local space.
			MMatrix inverseOrigin = dagPath.inclusiveMatrixInverse();
			return ReadGeometryDataForNode( context, geometry, boneMeshPath, materialData, m_meshNameToGeomIndexMap, inverseOrigin );
		}
	}

	return false;
}

bool GeometryExportSource::ReadBoneGeometryMaterialData(IExportContext* context, IGeometryMaterialData* geometryMaterialData, ISkeletonData* skeletonData, int boneIndex, const IMaterialData* materialData) const
{
	std::string *dagPathString = static_cast<std::string*>(skeletonData->GetBoneHandle(boneIndex));
	MDagPath dagPath;
	if( GetNodePathFromName( *dagPathString, dagPath ) )
	{
		MDagPath boneMeshPath = GetMeshForBone( dagPath );
		if( boneMeshPath.isValid() )
		{
			boneMeshPath.extendToShapeDirectlyBelow(0);
			return ReadGeometryDataForNode(context, boneMeshPath, geometryMaterialData, materialData);
		}
	}

	return false;
}

void GeometryExportSource::ReadMorphs(IExportContext* context, IMorphData* morphData, const IModelData* const modelData, int modelIndex)
{
	int i, j;
	MStatus status;
	std::string *dagPathString = static_cast<std::string*>(modelData->GetModelHandle(modelIndex));
	MDagPath dagPath;
	if( GetNodePathFromName( *dagPathString, dagPath ) )
	{
		MDagPathArray modelMeshNodes;
		GatherModelNodes( dagPath, modelMeshNodes, MFn::kMesh );
		if( modelMeshNodes.length() > 0 )
		{
			MObject blendObject = getBlendControllerFromBase( modelMeshNodes[0] );
			if( blendObject != MObject::kNullObj )
			{
				if( modelMeshNodes.length() > 1 )
					Log( context, IExportContext::MessageSeverity_Error, "A Blend shape model can only have one mesh." );

				MFnBlendShapeDeformer blend( blendObject );
				MObjectArray baseObjects;
				blend.getBaseObjects( baseObjects );

				if( baseObjects.length() > 0 )
				{
					int numTargets = blend.numWeights();
					for( i = 0;i<numTargets;i++ )
					{
						MObjectArray targetObjects;
						blend.getTargets( baseObjects[0], i, targetObjects );

						for( j = 0;j<targetObjects.length();j++ )
						{
							MDagPath targetPath = MDagPath::getAPathTo( targetObjects[j] );
							MDagPath targetParentPath = targetPath;
							status = targetParentPath.pop();
							if( status == MS::kSuccess && targetParentPath.length() > 0 && targetParentPath.hasFn( MFn::kTransform ) )
								targetPath = targetParentPath;

							MString targetName = MFnDagNode(targetPath).name();
							void *dagPathString = m_namePool.getPointer( targetName.asChar() );
							morphData->AddMorph( dagPathString, targetName.asChar() );
						}
					}
				}
			}
		}
	}
}

bool GeometryExportSource::ReadMorphGeometry(IExportContext* context, IGeometryData* geometry, const IModelData* const modelData, int modelIndex, const IMorphData* const morphData, int morphIndex, const IMaterialData* materialData)
{
	std::string *modelPathString = static_cast<std::string*>(modelData->GetModelHandle(modelIndex));
	std::string *targetPathString = static_cast<std::string*>(morphData->GetMorphHandle(morphIndex));
	MDagPath modelPath, targetPath;
	if( GetNodePathFromName( *modelPathString, modelPath ) && GetNodePathFromName( *targetPathString, targetPath ) )
	{
		MMatrix inverseOrigin = modelPath.inclusiveMatrixInverse();
		MObject blendObject = getBlendControllerFromTarget( targetPath );
		
		if( blendObject != MObject::kNullObj )
		{
			MFnBlendShapeDeformer blend( blendObject );
			
			MObjectArray baseObjects;
			blend.getBaseObjects( baseObjects );
			
			if( baseObjects.length() == 1 )
			{
				MObject baseObject = baseObjects[0];
				MDagPath basePath;
				MFnDagNode(baseObject).getPath( basePath );

				MeshData baseMeshData, targetMeshData;
				ReadMeshData( context, materialData, basePath, inverseOrigin, baseMeshData );
				ReadMeshData( context, materialData, targetPath, inverseOrigin, targetMeshData );

				baseMeshData.positionArray.copy( targetMeshData.positionArray );

				// NOTE : The offset of the geometry positions should always be 0 as we currently only allow 1 mesh per model for morphs

				WriteMeshData( context, geometry, baseMeshData );
			}
			else
			{
				Log( context, IExportContext::MessageSeverity_Error, "A Blend shape has the wrong number of base objects (%d). It should have 1.", baseObjects.length() );
			}
		}
	}
	
	return true;
}

bool GeometryExportSource::HasValidPosController(const IModelData* modelData, int modelIndex) const
{
	return hasValidAnimationController(modelData,modelIndex,CONTROLLER_TRANSLATE);
}

bool GeometryExportSource::HasValidRotController(const IModelData* modelData, int modelIndex) const
{
	return hasValidAnimationController(modelData,modelIndex,CONTROLLER_ROTATE);
}

bool GeometryExportSource::HasValidSclController(const IModelData* modelData, int modelIndex) const
{
	return hasValidAnimationController(modelData,modelIndex,CONTROLLER_SCALE);
}
