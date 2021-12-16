#include "StdAfx.h"
#include "mayaIncludes.h"
#include "mayaUtilities.h"
#include "mayaAnimUtilities.h"
#include "AnimationExportSource.h"
#include "sharedExport.h"
#include "attribute.h"
#include "MayaCryExport.h"
#include "ExportFunctions.h"

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
		
void AnimationExportSource::GetMetaData(SExportMetaData& metaData) const
{
	ExportFunctions::GetMetaData(metaData);
}

std::string AnimationExportSource::GetDCCFileName() const
{
	MFileIO fio;
	MString currentFilename = fio.currentFile();
	return std::string(currentFilename.asChar());
}

std::string AnimationExportSource::GetExportDirectory() const
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
	void GetAnimationRange( int &firstFrame, int &lastFrame )
	{
		firstFrame = currentExporterOptions.animStart;
		lastFrame = currentExporterOptions.animEnd;
	}
}

void AnimationExportSource::ReadGeometryFiles(IExportContext* context, IGeometryFileData* geometryFileData)
{
	MDagPath skeletonRoot = findNodeInScene( currentExporterOptions.animRoot.c_str(),FNBN_FULLNAMEONLY );

	if( skeletonRoot.isValid() )
	{
		std::string const name = GetAnimationName(NULL,0,0);

		IGeometryFileData::SProperties props;
		props.filetypeInt = CRY_FILE_TYPE_CAF;
		props.bDoNotMerge = false;
		geometryFileData->AddGeometryFile( NULL, name.c_str(), props );
	}
	else
	{
		Log( context, IExportContext::MessageSeverity_Error, "Root bone `%s` could not be found.", currentExporterOptions.animRoot.c_str() );
	}
}

void AnimationExportSource::ReadModels(const IGeometryFileData* geometryFileData, int geometryFileIndex, IModelData* modelData)
{
	char *properties = "";
	SHelperData data;
	int modelIndex = modelData->AddModel(0, geometryFileData->GetGeometryFileName(geometryFileIndex), -1, false, data, properties);
	float translation[] = {0.0f, 0.0f, 0.0f};
	float rotation[] = {0.0f, 0.0f, 0.0f};
	float scale[] = {1.0f, 1.0f, 1.0f};
	modelData->SetTranslationRotationScale(modelIndex,translation,rotation,scale);
}

bool AnimationExportSource::ReadMaterials(IExportContext* context, const IGeometryFileData* const geometryFileData, IMaterialData* materialData)
{
	return true;
}

void AnimationExportSource::ReadSkinning(IExportContext* context, ISkinningData* skinningData, const IModelData* const modelData, int modelIndex, ISkeletonData* skeletonData)
{
}

bool AnimationExportSource::ReadSkeleton(const IGeometryFileData* const geometryFileData, int geometryFileIndex, const IModelData* const modelData, int modelIndex, const IMaterialData* materialData, ISkeletonData* skeletonData)
{
	bool ret = false;

	std::string skelPathString = currentExporterOptions.animRoot;
	MDagPath skelPath;
	if( GetNodePathFromName( skelPathString, skelPath ) )
	{
		ret = ReadSkeletonAndListBones( &m_namePool, skelPath, geometryFileData, geometryFileIndex, modelData, modelIndex, materialData, skeletonData);
	}

	return ret;
}

int AnimationExportSource::GetAnimationCount() const
{
	return 1; // Only support 1 animation at a time.
}

std::string AnimationExportSource::GetAnimationName(const IGeometryFileData* geometryFileData, int geometryFileIndex, int animationIndex) const
{
	assert(animationIndex == 0); // Only support 1 animation at a time.

	return PathHelpers::GetFilename( currentExporterOptions.customName );
}

void AnimationExportSource::GetAnimationTimeSpan(float& start, float& stop, int animationIndex) const
{
	assert(animationIndex == 0); // Only support 1 animation at a time.
	
	int firstFrame, lastFrame;
	GetAnimationRange( firstFrame, lastFrame );

	start = getTimeFromFrame( firstFrame );
	stop = getTimeFromFrame( lastFrame );
}

void AnimationExportSource::ReadAnimationFlags(IExportContext* context, IAnimationData* animationData, const IGeometryFileData* const geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex) const
{
}

IAnimationData * AnimationExportSource::ReadAnimation(IExportContext* context, const IGeometryFileData* const geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex, float fps) const
{
	int frameIndex, boneIndex, axis;
	int firstFrame, lastFrame;

	if( !skeletonData )
	{
		Log( context, IExportContext::MessageSeverity_Error, "No skeleton when reading the animation." );
		return NULL;
	}

	GetAnimationRange( firstFrame, lastFrame );

	IAnimationData *animationData = new AnimationData(skeletonData->GetBoneCount(), fps, getTimeFromFrame( firstFrame ));

	int frameCount = lastFrame - firstFrame + 1;
	animationData->SetFrameCount(frameCount);

	int boneCount = skeletonData->GetBoneCount();

	std::vector<MDagPath> bonePath(boneCount);
	std::vector<bool> transformSpace(boneCount);
	std::fill_n(transformSpace.begin(), boneCount, false);
	for( boneIndex = 0; boneIndex < boneCount; boneIndex++ )
	{
		std::string *bonePathString = static_cast<std::string*>(skeletonData->GetBoneHandle(boneIndex));
		if( GetNodePathFromName( *bonePathString, bonePath[boneIndex] ) )
		{
			int parentIndex = skeletonData->GetBoneParentIndex(boneIndex);
			MDagPath parentPath;
			if( parentIndex >= 0 )
			{
				std::string *parentPathString = static_cast<std::string*>(skeletonData->GetBoneHandle(boneIndex));
				GetNodePathFromName( *parentPathString, parentPath );
			}

			transformSpace[boneIndex] = false;
			if( !parentPath.isValid() && bonePath[boneIndex].length() > 1 )
			{
				parentPath = bonePath[boneIndex];
				parentPath.pop();
			}

			if( parentPath.isValid() && parentPath.hasFn( MFn::kJoint ) )
				transformSpace[boneIndex] = true;
		}
	}

	MTime initialTime = MAnimControl::currentTime();

	for( frameIndex = 0; frameIndex < frameCount; frameIndex++ )
	{
		MTime time;
		time.setUnit( time.uiUnit() );
		time.setValue( firstFrame + frameIndex );

		MAnimControl::setCurrentTime(time);

		for( boneIndex = 0; boneIndex < boneCount; boneIndex++ )
		{
			float translation[3], rotation[3], scale[3];
			getNodePosScaleRotation( bonePath[boneIndex], translation, scale, rotation, transformSpace[boneIndex]);

			// Convert the frame data
			// Round scaling
			for( axis = 0; axis < 3; axis++ )
			{
				scale[axis] = floorf(scale[axis] * 1000.0f + 0.5f) / 1000.0f;
			}
			// Rotation to degrees
			for( axis = 0; axis < 3; axis++ )
				rotation[axis] = RAD2DEG( rotation[axis] );

			// Store the information in the animation data.
			animationData->SetFrameData( boneIndex, frameIndex, translation, rotation, scale );
		}
	}

	MAnimControl::setCurrentTime( initialTime );

	return animationData;
}

bool AnimationExportSource::ReadGeometry(IExportContext* context, IGeometryData* geometry, const IModelData* const modelData, const IMaterialData* materialData, int modelIndex)
{
	return false;
}

bool AnimationExportSource::ReadGeometryMaterialData(IExportContext* context, IGeometryMaterialData* geometryMaterialData, const IModelData* const modelData, const IMaterialData* materialData, int modelIndex) const
{
	return false;
}

bool AnimationExportSource::ReadBoneGeometry(IExportContext* context, IGeometryData* geometry, ISkeletonData* skeletonData, int boneIndex, const IMaterialData* materialData)
{
	return false;
}

bool AnimationExportSource::ReadBoneGeometryMaterialData(IExportContext* context, IGeometryMaterialData* geometryMaterialData, ISkeletonData* skeletonData, int boneIndex, const IMaterialData* materialData) const
{
	return false;
}

void AnimationExportSource::ReadMorphs(IExportContext* context, IMorphData* morphData, const IModelData* const modelData, int modelIndex)
{
}

bool AnimationExportSource::ReadMorphGeometry(IExportContext* context, IGeometryData* geometry, const IModelData* const modelData, int modelIndex, const IMorphData* const morphData, int morphIndex, const IMaterialData* materialData)
{
	assert(0);
	return false;
}

bool AnimationExportSource::HasValidPosController(const IModelData* modelData, int modelIndex) const
{
	assert(0);
	return false;
}

bool AnimationExportSource::HasValidRotController(const IModelData* modelData, int modelIndex) const
{
	assert(0);
	return false;
}

bool AnimationExportSource::HasValidSclController(const IModelData* modelData, int modelIndex) const
{
	assert(0);
	return false;
}