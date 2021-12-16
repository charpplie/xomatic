#include "StdAfx.h"
#include "AnimationExportSource.h"
#include "Export/ISkeletonData.h"
#include "Export/IModelData.h"
#include "Export/IAnimationData.h"
#include "Export/IGeometryFileData.h"
#include "Export/IGeometryData.h"
#include "Export/IMaterialData.h"
#include "Export/IExportContext.h"
#include "Export/ISkinningData.h"
#include "Export/Export.h"
#include "Export/AnimationData.h"

#include "PathHelpers.h"
#include "CRefHelpers.h"
#include "Decompose.h"
#include "ModelHelpers.h"

#include <cmath>

#include "Properties.h"

#include "Cry_Math.h"

AnimationExportSource::AnimationExportSource(XSI::ActionSource& source, XSI::CRefArray& roots)
: m_source(source), m_roots(roots)
{
}

//////////////////////////////////////////////////////////////////////////
void AnimationExportSource::GetMetaData(SExportMetaData& metaData) const
{
	Export::GetMetaData(metaData);
}

std::string AnimationExportSource::GetDCCFileName() const
{
	XSI::Application app;
	XSI::Scene scn = app.GetActiveProject().GetActiveScene();
	XSI::CString fullFilename = scn.GetParameterValue(L"Filename");
	return std::string(fullFilename.GetAsciiString());
}

std::string AnimationExportSource::GetExportDirectory() const
{
	std::string exportPath = GetExportPath().GetAsciiString();

	if (exportPath.empty())
	{
		exportPath = PathHelpers::GetDirectory(GetDCCFileName());
	}

	return exportPath;
}

void AnimationExportSource::ReadGeometryFiles(IExportContext* context, IGeometryFileData* geometryFileData)
{
	IGeometryFileData::SProperties props;
	props.filetypeInt = CRY_FILE_TYPE_CAF;
	props.bDoNotMerge = false;

	// Register a dummy geometry file.
	string const sourceName = m_source.GetName().GetAsciiString();
	geometryFileData->AddGeometryFile(0, sourceName.c_str(), props);
}

void AnimationExportSource::ReadModels(const IGeometryFileData* geometryFileData, int geometryFileIndex, IModelData* modelData)
{
	// Register a dummy model.
	int const modelIndex = modelData->AddModel(0, geometryFileData->GetGeometryFileName(geometryFileIndex), -1, false, SHelperData(), "");
	float const translation[] = {0.0f, 0.0f, 0.0f};
	float const rotation[] = {0.0f, 0.0f, 0.0f};
	float const scale[] = {1.0f, 1.0f, 1.0f};
	modelData->SetTranslationRotationScale(modelIndex, translation, rotation, scale);

}

bool AnimationExportSource::ReadMaterials(IExportContext* context, const IGeometryFileData* const geometryFileData, IMaterialData* materialData)
{
	// Ignore materials for animations.
	return true;
}

void AnimationExportSource::ReadSkinning(IExportContext* context, ISkinningData* skinningData, const IModelData* const modelData, int modelIndex, ISkeletonData* skeletonData)
{
}

namespace
{
	void RecurseAndListBones(ISkeletonData* skeletonData, XSI::X3DObject& bone, int parentIndex, const IMaterialData* materialData, XSI::CRefArray& heldReferences)
	{
		// Get the name of the bone.

		// Add the bone.
		XSI::CRef boneRef = bone.GetRef();
		heldReferences.Add(boneRef);
		int boneIndex = skeletonData->AddBone(CRefHelpers::ToPointer(boneRef), ModelHelpers::GetModelColladaName(bone).c_str(), parentIndex);

		// Set the initial position. In Max we have to ask the skin modifier for the initial position, but other code in XSI
		// just seems to use the current position of the bone, so let's do that for now.
		XSI::MATH::CMatrix4 initTM = bone.GetKinematics().GetGlobal().GetTransform().GetMatrix4();
		decomp::HMatrix initTMFloat = {
			float(initTM.GetValue(0, 0)), float(initTM.GetValue(1, 0)), float(initTM.GetValue(2, 0)), float(initTM.GetValue(3, 0)),
			float(initTM.GetValue(0, 1)), float(initTM.GetValue(1, 1)), float(initTM.GetValue(2, 1)), float(initTM.GetValue(3, 1)),
			float(initTM.GetValue(0, 2)), float(initTM.GetValue(1, 2)), float(initTM.GetValue(2, 2)), float(initTM.GetValue(3, 2)),
			float(initTM.GetValue(0, 3)), float(initTM.GetValue(1, 3)), float(initTM.GetValue(2, 3)), float(initTM.GetValue(3, 3))};

		decomp::AffineParts parts;
		decomp::decomp_affine(initTMFloat, &parts);
		Quat rotation = *(Quat*)&parts.q;
		Ang3 angles(rotation);
		float translationVec[3] = {parts.t.x, parts.t.y, parts.t.z};
		float rotationVec[3] = {angles.x, angles.y, angles.z};
		float scalingVec[3] = {parts.k.x, parts.k.y, parts.k.z};

		skeletonData->SetTranslation(boneIndex, translationVec);
		skeletonData->SetRotation(boneIndex, rotationVec);
		skeletonData->SetScale(boneIndex, scalingVec);

		// Recurse to the node's children.
		for (int childIndex = 0, childCount = bone.GetChildren().GetCount(); childIndex < childCount; ++childIndex)
			RecurseAndListBones(skeletonData, XSI::X3DObject(bone.GetChildren().GetItem(childIndex)), boneIndex, materialData, heldReferences);
	}
}

bool AnimationExportSource::ReadSkeleton(const IGeometryFileData* const geometryFileData, int geometryFileIndex, const IModelData* const modelData, int modelIndex, const IMaterialData* materialData, ISkeletonData* skeletonData)
{
	for (int rootIndex = 0, rootCount = m_roots.GetCount(); rootIndex < rootCount; ++rootIndex)
	{
		XSI::X3DObject root(m_roots[rootIndex]);
		RecurseAndListBones(skeletonData, root, -1, materialData, m_heldReferences);
	}

	return true;
}

int AnimationExportSource::GetAnimationCount() const
{
	return 1;
}

std::string AnimationExportSource::GetAnimationName(const IGeometryFileData* geometryFileData, int geometryFileIndex, int animationIndex) const
{
	return std::string(m_source.GetName().GetAsciiString());
}

void AnimationExportSource::GetAnimationTimeSpan(float& start, float& stop, int animationIndex) const
{
	XSI::CParameterRefArray& parameters = m_source.GetParameters();
	double timeStart = m_source.GetParameter(L"AnimStart").GetValue();
	double timeEnd = m_source.GetParameter(L"AnimEnd").GetValue();

	start = float(timeStart);
	stop = float(timeEnd);
}

void AnimationExportSource::ReadAnimationFlags(IExportContext* context, IAnimationData* animationData, const IGeometryFileData* const geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex) const
{
}

IAnimationData * AnimationExportSource::ReadAnimation(IExportContext* context, const IGeometryFileData* const geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex, float fps) const
{
	XSI::CParameterRefArray& parameters = m_source.GetParameters();
	double timeStart = m_source.GetParameter(L"AnimStart").GetValue();
	IAnimationData *animationData = new AnimationData(skeletonData->GetBoneCount(), fps, 
																										float(timeStart));

	// Create a mapping between bone names and indices.
	std::map<string, int> boneNameIDMap;
	for (int boneIndex = 0, boneCount = skeletonData->GetBoneCount(); boneIndex < boneCount; ++boneIndex)
	{
		XSI::CRef boneRef = CRefHelpers::ToRef(skeletonData->GetBoneHandle(boneIndex));
		XSI::X3DObject bone = boneRef;
		string boneName = bone.GetName().GetAsciiString();
		boneNameIDMap.insert(std::make_pair(boneName, boneIndex));
	}

	// Find the start and end frames of the animation - sample at 30fps.
	double timeEnd = m_source.GetParameter(L"AnimEnd").GetValue();
	int firstFrame = int(timeStart * fps + 0.5f);
	int lastFrame = int(timeEnd * fps + 0.5f);
	int frameCount = lastFrame - firstFrame + 1;
	animationData->SetFrameCount(frameCount);

	std::map<std::pair<int, int>, std::vector<float> > dataMap;
	for (int boneIndex = 0, boneCount = skeletonData->GetBoneCount(); boneIndex < boneCount; ++boneIndex)
	{
		XSI::CRef boneRef = CRefHelpers::ToRef(skeletonData->GetBoneHandle(boneIndex));
		XSI::X3DObject bone = boneRef;

		// Set the initial position. In Max we have to ask the skin modifier for the initial position, but other code in XSI
		// just seems to use the current position of the bone, so let's do that for now.
		XSI::MATH::CMatrix4 initTM = bone.GetKinematics().GetLocal().GetTransform().GetMatrix4();
		decomp::HMatrix initTMFloat = {
			float(initTM.GetValue(0, 0)), float(initTM.GetValue(1, 0)), float(initTM.GetValue(2, 0)), float(initTM.GetValue(3, 0)),
			float(initTM.GetValue(0, 1)), float(initTM.GetValue(1, 1)), float(initTM.GetValue(2, 1)), float(initTM.GetValue(3, 1)),
			float(initTM.GetValue(0, 2)), float(initTM.GetValue(1, 2)), float(initTM.GetValue(2, 2)), float(initTM.GetValue(3, 2)),
			float(initTM.GetValue(0, 3)), float(initTM.GetValue(1, 3)), float(initTM.GetValue(2, 3)), float(initTM.GetValue(3, 3))};

		decomp::AffineParts parts;
		decomp::decomp_affine(initTMFloat, &parts);
		Quat rotation = *(Quat*)&parts.q;
		Ang3 angles(rotation);
		float translationVec[3] = {parts.t.x, parts.t.y, parts.t.z};
		float rotationVec[3] = {angles.x, angles.y, angles.z};
		float scalingVec[3] = {parts.k.x, parts.k.y, parts.k.z};

		std::vector<float>& translationArray = dataMap[std::make_pair(boneIndex, 0)];
		translationArray.resize(frameCount * 3);
		for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex)
			translationArray[frameIndex * 3 + 0] = translationVec[0], translationArray[frameIndex * 3 + 1] = translationVec[1], translationArray[frameIndex * 3 + 2] = translationVec[2];

		std::vector<float>& rotationArray = dataMap[std::make_pair(boneIndex, 1)];
		rotationArray.resize(frameCount * 3);
		for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex)
			rotationArray[frameIndex * 3 + 0] = rotationVec[0], rotationArray[frameIndex * 3 + 1] = rotationVec[1], rotationArray[frameIndex * 3 + 2] = rotationVec[2];

		// Convert rotation values to degrees.
		for (int frameIndex = firstFrame; frameIndex <= lastFrame; ++frameIndex)
		{
			for (int axis = 0; axis < 3; ++axis)
				rotationArray[(frameIndex - firstFrame) * 3 + axis] *= 180.0f / 3.14159f;
		}


		std::vector<float>& scalingArray = dataMap[std::make_pair(boneIndex, 2)];
		scalingArray.resize(frameCount * 3);
		for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex)
			scalingArray[frameIndex * 3 + 0] = scalingVec[0], scalingArray[frameIndex * 3 + 1] = scalingVec[1], scalingArray[frameIndex * 3 + 2] = scalingVec[2];
	}

	// For each bone sample the transform at each frame.
	XSI::CRefArray items = m_source.GetItems();

	// Create a mapping between parameter names and indices.
	std::map<string, std::pair<int, int> > parameterNameMap;
	parameterNameMap.insert(std::make_pair("posx", std::make_pair(0, 0)));
	parameterNameMap.insert(std::make_pair("posy", std::make_pair(0, 1)));
	parameterNameMap.insert(std::make_pair("posz", std::make_pair(0, 2)));
	parameterNameMap.insert(std::make_pair("rotx", std::make_pair(1, 0)));
	parameterNameMap.insert(std::make_pair("roty", std::make_pair(1, 1)));
	parameterNameMap.insert(std::make_pair("rotz", std::make_pair(1, 2)));
	parameterNameMap.insert(std::make_pair("sclx", std::make_pair(2, 0)));
	parameterNameMap.insert(std::make_pair("scly", std::make_pair(2, 1)));
	parameterNameMap.insert(std::make_pair("sclz", std::make_pair(2, 2)));

	for (int itemIndex = 0, itemCount = items.GetCount(); itemIndex < itemCount; ++itemIndex)
	{
		XSI::AnimationSourceItem item = items[itemIndex];

		// Parse the target to determine what parameter is being controlled.
		string target = item.GetTarget().GetAsciiString();
		size_t pos = 0;
		size_t modelNameStart = pos;
		pos = (pos != string::npos ? target.find('.', pos) : pos);
		size_t modelNameEnd = pos;
		pos = (pos != string::npos ? pos + 1 : pos);
		size_t kineParamStart = pos;
		pos = (pos != string::npos ? target.find('.', pos) : pos);
		size_t kineParamEnd = pos;
		pos = (pos != string::npos ? pos + 1 : pos);
		size_t localParamStart = pos;
		pos = (pos != string::npos ? target.find('.', pos) : pos);
		size_t localParamEnd = pos;
		pos = (pos != string::npos ? pos + 1 : pos);
		size_t paramStart = pos;
		size_t paramEnd = string::npos;
		string modelName = target.substr(modelNameStart, modelNameEnd - modelNameStart);
		string kineParam = target.substr(kineParamStart, kineParamEnd - kineParamStart);
		string localParam = target.substr(localParamStart, localParamEnd - localParamStart);
		string param = target.substr(paramStart, paramEnd - paramStart);
		std::map<string, int>::iterator boneNameIDPos = boneNameIDMap.find(modelName);
		int boneIndex = (boneNameIDPos != boneNameIDMap.end() ? (*boneNameIDPos).second : -1);
		bool isKineParam = kineParam == "kine";
		bool isLocalParam = localParam == "local";
		std::map<string, std::pair<int, int> >::iterator paramNamePos = parameterNameMap.find(param);
		int paramArrayIndex = (paramNamePos != parameterNameMap.end() ? (*paramNamePos).second.first : -1);
		int paramArrayOffset = (paramNamePos != parameterNameMap.end() ? (*paramNamePos).second.second : -1);
		int paramArrayStride = 3;

		// If we parsed the target correctly, then sample the curve into the desired array.
		if (boneIndex >= 0 && isKineParam && isLocalParam && paramArrayIndex >= 0)
		{
			float* paramArray = &dataMap[std::make_pair(boneIndex, paramArrayIndex)][0] + paramArrayOffset;
			for (int frame = firstFrame; frame <= lastFrame; ++frame)
			{
				XSI::CRef ref = item.GetSource();
				double value = 0.0;
				if (ref.IsA(XSI::siFCurveID))
				{
					XSI::FCurve datasrc(ref);
					value = datasrc.Eval(double(frame));
				}
				else if (ref.IsA(XSI::siStaticSourceID))
				{
					XSI::StaticSource datasrc(ref);
					value = datasrc.GetValue();
				}
				paramArray[(frame - firstFrame) * paramArrayStride] = float(value);
			}
		}
	}

	for (int boneIndex = 0, boneCount = skeletonData->GetBoneCount(); boneIndex < boneCount; ++boneIndex)
	{
		std::vector<float>& translationArray = dataMap[std::make_pair(boneIndex, 0)];
		std::vector<float>& rotationArray = dataMap[std::make_pair(boneIndex, 1)];
		std::vector<float>& scalingArray = dataMap[std::make_pair(boneIndex, 2)];

		// Round scale values to nearest 0.001.
		for (int frameIndex = firstFrame; frameIndex <= lastFrame; ++frameIndex)
		{
			for (int axis = 0; axis < 3; ++axis)
			{
				float& value = scalingArray[(frameIndex - firstFrame) * 3 + axis];
				value = std::floor(value * 1000.0f + 0.5f) / 1000.0f;
			}
		}

		// Store the information in the animation data.
		for (int frameIndex = firstFrame; frameIndex <= lastFrame; ++frameIndex)
		{
			animationData->SetFrameData(boneIndex, frameIndex - firstFrame,
				&translationArray[3 * (frameIndex - firstFrame)],
				&rotationArray[3 * (frameIndex - firstFrame)],
				&scalingArray[3 * (frameIndex - firstFrame)]);
		}
	}

	return animationData;
}

bool AnimationExportSource::ReadGeometry(IExportContext* context, IGeometryData* geometry, const IModelData* const modelData, const IMaterialData* const materialData, int modelIndex)
{
	return false;
}

bool AnimationExportSource::ReadGeometryMaterialData(IExportContext* context, IGeometryMaterialData* geometryMaterialData, const IModelData* const modelData, const IMaterialData* const materialData, int modelIndex) const
{
	return false;
}

bool AnimationExportSource::ReadBoneGeometry(IExportContext* context, IGeometryData* geometry, ISkeletonData* skeletonData, int boneIndex, const IMaterialData* const materialData)
{
	return false;
}

bool AnimationExportSource::ReadBoneGeometryMaterialData(IExportContext* context, IGeometryMaterialData* geometryMaterialData, ISkeletonData* skeletonData, int boneIndex, const IMaterialData* const materialData) const
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