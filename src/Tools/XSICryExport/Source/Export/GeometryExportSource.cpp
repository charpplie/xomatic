#include "StdAfx.h"
#include "GeometryExportSource.h"
#include "Export/ISkeletonData.h"
#include "Export/IModelData.h"
#include "Export/IAnimationData.h"
#include "Export/IGeometryFileData.h"
#include "Export/IGeometryData.h"
#include "Export/IMaterialData.h"
#include "Export/IExportContext.h"
#include "Export/ISkinningData.h"
#include "Export/IMorphData.h"
#include "Export/IGeometryMaterialData.h"
#include "ExportNodeHelpers.h"
#include "CRefHelpers.h"
#include "PathHelpers.h"
#include "MaterialHelpers.h"
#include "MeshHelpers.h"
#include "SkinningHelpers.h"
#include "Decompose.h"
#include "ModelHelpers.h"
#include "UserPropertyHelpers.h"
#include "Export/Export.h"
#include "Export/AnimationData.h"

#include <cctype>

#include "Properties.h"

#include "Cry_Math.h"

GeometryExportSource::GeometryExportSource(const XSI::CRefArray& exportNodes)
: m_exportNodes(exportNodes)
{
}

//////////////////////////////////////////////////////////////////////////
void GeometryExportSource::GetMetaData(SExportMetaData& metaData) const
{
	Export::GetMetaData(metaData);
}

//////////////////////////////////////////////////////////////////////////
std::string GeometryExportSource::GetDCCFileName() const
{
	XSI::Application app;
	XSI::Scene scn = app.GetActiveProject().GetActiveScene();
	XSI::CString fullFilename = scn.GetParameterValue(L"Filename");
	return std::string(fullFilename.GetAsciiString());
}

//////////////////////////////////////////////////////////////////////////
std::string GeometryExportSource::GetExportDirectory() const
{
	std::string exportPath = GetExportPath().GetAsciiString();

	if (exportPath.empty())
	{
		exportPath = PathHelpers::GetDirectory(GetDCCFileName());
	}

	return exportPath;
}

//////////////////////////////////////////////////////////////////////////
void GeometryExportSource::ReadGeometryFiles(IExportContext* context, IGeometryFileData* geometryFileData)
{
	for (int exportNodeIndex = 0, exportNodeCount = m_exportNodes.GetCount(); exportNodeIndex < exportNodeCount; ++exportNodeIndex)
	{
		XSI::SceneItem object = m_exportNodes[exportNodeIndex];

		if (ExportNodeHelpers::IsCryExportNode(object))
		{
			std::string geometryFileName = ExportNodeHelpers::GetGeometryFileName(object);

			XSI::CRef ref = (XSI::CRef)(XSI::X3DObject&)object;
			m_heldReferences.Add(ref); // Stop refs becoming invalid.

			// Add the export node.

			IGeometryFileData::SProperties props;
			props.bDoNotMerge = !ExportNodeHelpers::IsMergeGeometry(object);
			props.filetypeInt = ExportNodeHelpers::GetContentTypeInt(object);

			geometryFileData->AddGeometryFile(CRefHelpers::ToPointer(ref), geometryFileName.c_str(), props);
		}
	}
}

namespace
{
	SHelperData GetHelperData(const XSI::X3DObject& object)
	{
		SHelperData helperData;
		helperData.m_eHelperType = SHelperData::eHelperType_UNKNOWN;

		XSI::siClassID const id = object.GetClassID();

		if (id == XSI::siNullID)
		{
			float const size = object.GetParameterValue(XSI::CString(L"size"));

			if (size <= 0)
			{
				helperData.m_eHelperType = SHelperData::eHelperType_Point;
			}
			else
			{
				helperData.m_eHelperType = SHelperData::eHelperType_Dummy;
				helperData.m_boundBoxMin[0] = -size * 0.5f;
				helperData.m_boundBoxMin[1] = -size * 0.5f;
				helperData.m_boundBoxMin[2] = -size * 0.5f;
				helperData.m_boundBoxMax[0] = +size * 0.5f;
				helperData.m_boundBoxMax[1] = +size * 0.5f;
				helperData.m_boundBoxMax[2] = +size * 0.5f;
			}
		}

		return helperData;
	}

	bool HasMesh(const XSI::X3DObject& object)
	{
		// Check whether there is any geometry associated with this node.
		XSI::PolygonMesh const mesh = object.GetActivePrimitive().GetGeometry();
		return mesh.IsValid();
	}

	bool IsHelperObject(const XSI::X3DObject& object)
	{
		SHelperData const helperData = GetHelperData(object);
		return (helperData.m_eHelperType != SHelperData::eHelperType_UNKNOWN);
	}

	void RecurseHierarchyAndFindModels(std::set<wstring>& modelSet, const XSI::X3DObject& object)
	{
		if (HasMesh(object) || IsHelperObject(object))
		{
			modelSet.insert(object.GetRef().GetAsText().GetWideString());
		}

		// Recurse to the node's children.
		for (int childIndex = 0, childCount = object.GetChildren().GetCount(); childIndex < childCount; ++childIndex)
			RecurseHierarchyAndFindModels(modelSet, XSI::X3DObject(object.GetChildren().GetItem(childIndex)));
	}

	void RecurseHierarchyAndFindBones(std::set<wstring>& modelSet, const XSI::X3DObject& object)
	{
		modelSet.insert(object.GetRef().GetAsText().GetWideString());

		// Recurse to the node's children.
		for (int childIndex = 0, childCount = object.GetChildren().GetCount(); childIndex < childCount; ++childIndex)
			RecurseHierarchyAndFindBones(modelSet, XSI::X3DObject(object.GetChildren().GetItem(childIndex)));
	}

	void RecurseHierarchyAndListModels(IModelData* modelData, const std::set<wstring>& modelSet, const XSI::X3DObject& object, int parentIndex, XSI::CRefArray& heldReferences)
	{
		int modelIndex = parentIndex;

		// Check if it's a node with mesh or a helper node.
		if (modelSet.find(object.GetRef().GetAsText().GetWideString()) != modelSet.end())
		{
			UserPropertyHelpers::UserPropertyReader propertyReader(object);
			std::string propertiesString = propertyReader.GetUserPropertyString();

			bool const bGeometry = HasMesh(object);

			SHelperData helperData;
			if (!bGeometry)
			{
				helperData = GetHelperData(object);
			}

			XSI::CRef ref = object.GetRef();
			heldReferences.Add(ref);
			modelIndex = modelData->AddModel(CRefHelpers::ToPointer(ref), object.GetName().GetAsciiString(), parentIndex, bGeometry, helperData, propertiesString);

			// TODO: Set transform;
			XSI::MATH::CMatrix4 matrix = object.GetKinematics().GetLocal().GetTransform().GetMatrix4();

			decomp::HMatrix initTMFloat = {
				float(matrix.GetValue(0, 0)), float(matrix.GetValue(1, 0)), float(matrix.GetValue(2, 0)), float(matrix.GetValue(3, 0)),
				float(matrix.GetValue(0, 1)), float(matrix.GetValue(1, 1)), float(matrix.GetValue(2, 1)), float(matrix.GetValue(3, 1)),
				float(matrix.GetValue(0, 2)), float(matrix.GetValue(1, 2)), float(matrix.GetValue(2, 2)), float(matrix.GetValue(3, 2)),
				float(matrix.GetValue(0, 3)), float(matrix.GetValue(1, 3)), float(matrix.GetValue(2, 3)), float(matrix.GetValue(3, 3))};

			
			decomp::AffineParts parts;
			decomp::decomp_affine(initTMFloat, &parts);
			Quat rotation = *(Quat*)&parts.q;
			Ang3 angles(rotation);
			float const translationVec[3] = {parts.t.x, parts.t.y, parts.t.z};
			float const rotationVec[3] = {angles.x, angles.y, angles.z};
			float const scalingVec[3] = {parts.k.x, parts.k.y, parts.k.z};
			modelData->SetTranslationRotationScale(modelIndex, translationVec, rotationVec, scalingVec);
		}

		// Recurse to the node's children.
		for (int childIndex = 0, childCount = object.GetChildren().GetCount(); childIndex < childCount; ++childIndex)
			RecurseHierarchyAndListModels(modelData, modelSet, XSI::X3DObject(object.GetChildren().GetItem(childIndex)), modelIndex, heldReferences);
	}
}

void GeometryExportSource::ReadModels(const IGeometryFileData* geometryFileData, int geometryFileIndex, IModelData* modelData)
{
	XSI::X3DObject geometryFileObject(CRefHelpers::ToRef(geometryFileData->GetGeometryFileHandle(geometryFileIndex)));

	std::set<wstring> modelSet;
	RecurseHierarchyAndFindModels(modelSet, geometryFileObject);

	// Find any models that are referenced as bones.
	std::set<wstring> boneSet;
	for (std::set<wstring>::const_iterator modelPos = modelSet.begin(), modelEnd = modelSet.end(); modelPos != modelEnd; ++modelPos)
	{
		XSI::CRef modelRef;
		modelRef.Set((*modelPos).c_str());
		XSI::X3DObject model = modelRef;

		// Process only the objects with meshes
		if (HasMesh(model))
		{
			std::set<XSI::CRef> skeletonRoots = SkinningHelpers::GetSkeletonRoots(model);

			for (std::set<XSI::CRef>::iterator rootPos = skeletonRoots.begin(), rootEnd = skeletonRoots.end(); rootPos != rootEnd; ++rootPos)
				RecurseHierarchyAndFindBones(boneSet, XSI::X3DObject(*rootPos));
		}
	}

	// Remove any bones from the list of models to export.
	for (std::set<wstring>::const_iterator bonePos = boneSet.begin(), boneEnd = boneSet.end(); bonePos != boneEnd; ++bonePos)
		modelSet.erase(*bonePos);

	RecurseHierarchyAndListModels(modelData, modelSet, geometryFileObject, -1, m_heldReferences);
}

namespace
{
	void ListMaterials(IMaterialData* materialData, XSI::Library& library, int parentIndex, XSI::CRefArray& heldReferences)
	{
		XSI::CRefArray materials = library.GetItems();
		std::string matLibraryName = library.GetName().GetAsciiString();
		if (matLibraryName.empty() )
		{
			matLibraryName = "UnknownMatLibrary";
		}

		for (int materialIndex = 0, materialCount = materials.GetCount(); materialIndex < materialCount; ++materialIndex)
		{
			XSI::Material material = materials[materialIndex];

			MaterialHelpers::MaterialInfo materialInfo = MaterialHelpers::GetInfo(material);

			heldReferences.Add(materials[materialIndex]);
			materialData->AddMaterial(
				matLibraryName.c_str(), 
				materialInfo.id,
				CRefHelpers::ToPointer(materials[materialIndex]), 
				(string("__phys")+materialInfo.physicalize).c_str());
		}
	}

	void ListMaterialLibraries(IMaterialData* materialData, XSI::CRefArray& heldReferences)
	{
		XSI::Application app;
		XSI::CRefArray libraries = app.GetActiveProject().GetActiveScene().GetMaterialLibraries();
		for (int libraryIndex = 0, libraryCount = libraries.GetCount(); libraryIndex < libraryCount; ++libraryIndex)
		{
			XSI::Library library = libraries[libraryIndex];
			//heldReferences.Add(libraries[libraryIndex]);
			//int materialIndex = materialData->AddMaterial(library.GetName().GetAsciiString(), -1, /*-1,*/ CRefHelpers::ToPointer(libraries[libraryIndex]), "");
			ListMaterials(materialData, library, -1, heldReferences);
		}
	}
}

bool GeometryExportSource::ReadMaterials(IExportContext* context, const IGeometryFileData* const geometryFileData, IMaterialData* materialData)
{
	ListMaterialLibraries(materialData, m_heldReferences);
	return true;
}

void GeometryExportSource::ReadSkinning(IExportContext* context, ISkinningData* skinningData, const IModelData* const modelData, int modelIndex, ISkeletonData* skeletonData)
{
	// Build a table mapping bone names to indices.
	std::map<string, int> boneNameIndexMap;
	for (int boneIndex = 0, boneCount = skeletonData->GetBoneCount(); boneIndex < boneCount; ++boneIndex)
	{
		XSI::CRef boneRef = CRefHelpers::ToRef(skeletonData->GetBoneHandle(boneIndex));
		boneNameIndexMap.insert(std::make_pair(boneRef.GetAsText().GetAsciiString(), boneIndex));
	}

	// Read the weights from the model.
	XSI::CRef model = CRefHelpers::ToRef(modelData->GetModelHandle(modelIndex));
	std::map<int, std::map<int, double> > vertexWeights = SkinningHelpers::GetVertexWeights(model, boneNameIndexMap);

	// Count the vertices.
	int maxVertexIndex = -1;
	for (std::map<int, std::map<int, double> >::const_iterator vertexPos = vertexWeights.begin(), vertexEnd = vertexWeights.end(); vertexPos != vertexEnd; ++vertexPos)
		maxVertexIndex = (std::max)(maxVertexIndex, (*vertexPos).first);
	int vertexCount = maxVertexIndex + 1;
	skinningData->SetVertexCount(vertexCount);

	// Register all the weights.
	for (std::map<int, std::map<int, double> >::const_iterator vertexPos = vertexWeights.begin(), vertexEnd = vertexWeights.end(); vertexPos != vertexEnd; ++vertexPos)
	{
		int vertexIndex = (*vertexPos).first;
		const std::map<int, double>& weightsForVertexMap = (*vertexPos).second;

		for (std::map<int, double>::const_iterator weightPos = weightsForVertexMap.begin(), weightEnd = weightsForVertexMap.end(); weightPos != weightEnd; ++weightPos)
		{
			int boneIndex = (*weightPos).first;
			double weight = (*weightPos).second;

			skinningData->AddWeight(vertexIndex, boneIndex, float(weight));
		}
	}
}

namespace
{
	struct JointParameterEntry
	{
		JointParameterEntry(ISkeletonData::Axis axis, ISkeletonData::Limit limit, float localOffset): axis(axis), limit(limit), localOffset(localOffset) {}
		ISkeletonData::Axis axis;
		ISkeletonData::Limit limit;
		float localOffset;
	};

	void RecurseAndListBones(ISkeletonData* skeletonData, XSI::X3DObject& bone, int parentIndex, const IMaterialData* materialData, XSI::CRefArray& heldReferences)
	{
		// Add the bone.
		XSI::CRef boneRef = bone.GetRef();
		heldReferences.Add(boneRef);
		int boneIndex = skeletonData->AddBone(CRefHelpers::ToPointer(boneRef), ModelHelpers::GetModelColladaName(bone).c_str(), parentIndex);

		// Check whether the bone has geometry.
		MeshHelpers::MeshAccessor meshAccessor(bone);
		bool hasGeometry = meshAccessor.IsValid();
		skeletonData->SetHasGeometry(boneIndex, hasGeometry);

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

		// Check for phys mesh nodes.
		XSI::Application application;
		string boneName = bone.GetName().GetAsciiString();
		string physBoneName = boneName + "_Phys";
		XSI::CStringArray emptyArray;
		XSI::CString physBoneNameXSI;
		physBoneNameXSI.PutAsciiString(physBoneName.c_str());
		XSI::CRefArray physBones = application.GetActiveSceneRoot().FindChildren(physBoneNameXSI, L"", emptyArray, true);
		skeletonData->SetPhysicalized(boneIndex, physBones.GetCount() > 0);

		string physParentFrameBoneName = boneName + "_Phys_ParentFrame";
		XSI::CString physParentFrameBoneNameXSI;
		physParentFrameBoneNameXSI.PutAsciiString(physParentFrameBoneName.c_str());
		XSI::CRefArray physParentFrameBones = application.GetActiveSceneRoot().FindChildren(physParentFrameBoneNameXSI, L"", emptyArray, true);
		XSI::CRef physParentFrameBoneRef = (physParentFrameBones.GetCount() ? physParentFrameBones[0] : XSI::CRef());
		int physParentFrameBonesCount = physParentFrameBones.GetCount();
		if (physParentFrameBoneRef.IsValid())
		{
			XSI::X3DObject parentFrameBone(physParentFrameBoneRef);
			string name = parentFrameBone.GetName().GetAsciiString();
			XSI::CRef physParentFrameParentBoneRef = parentFrameBone.GetParent();
			XSI::CRef physParentFrameParentBone3DRef = parentFrameBone.GetParent3DObject();
			int a = 1;
			a = a;
		}
		XSI::CRef physParentFrameParentBoneRef = (physParentFrameBoneRef.IsValid() ? XSI::X3DObject(physParentFrameBoneRef).GetParent() : XSI::CRef());
		if (physParentFrameBoneRef && physParentFrameParentBoneRef)
		{
			XSI::X3DObject parentFrameBone(physParentFrameBoneRef);
			XSI::X3DObject parentFrameParentBone(physParentFrameParentBoneRef);

			XSI::MATH::CMatrix4 parentFrameTM = parentFrameBone.GetKinematics().GetGlobal().GetTransform().GetMatrix4();
			XSI::MATH::CMatrix4 parentFrameParentTM = parentFrameParentBone.GetKinematics().GetGlobal().GetTransform().GetMatrix4();
			XSI::MATH::CMatrix4 parentFrameParentTMInverse;
			parentFrameParentTMInverse.Invert(parentFrameParentTM);

			XSI::MATH::CMatrix4 parentFrameRelativeTM;
			parentFrameRelativeTM.Mul(parentFrameParentTMInverse, parentFrameTM);

			decomp::HMatrix parentFrameRelativeTMFloat = {
				float(parentFrameRelativeTM.GetValue(0, 0)), float(parentFrameRelativeTM.GetValue(1, 0)), float(parentFrameRelativeTM.GetValue(2, 0)), float(parentFrameRelativeTM.GetValue(3, 0)),
				float(parentFrameRelativeTM.GetValue(0, 1)), float(parentFrameRelativeTM.GetValue(1, 1)), float(parentFrameRelativeTM.GetValue(2, 1)), float(parentFrameRelativeTM.GetValue(3, 1)),
				float(parentFrameRelativeTM.GetValue(0, 2)), float(parentFrameRelativeTM.GetValue(1, 2)), float(parentFrameRelativeTM.GetValue(2, 2)), float(parentFrameRelativeTM.GetValue(3, 2)),
				float(parentFrameRelativeTM.GetValue(0, 3)), float(parentFrameRelativeTM.GetValue(1, 3)), float(parentFrameRelativeTM.GetValue(2, 3)), float(parentFrameRelativeTM.GetValue(3, 3))};

			decomp::AffineParts parts;
			decomp::decomp_affine(parentFrameRelativeTMFloat, &parts);
			Quat rotation = *(Quat*)&parts.q;
			Ang3 angles(rotation);
			float parentFrameTranslationVec[3] = {parts.t.x, parts.t.y, parts.t.z};
			float parentFrameRotationVec[3] = {angles.x, angles.y, angles.z};
			float parentFrameScalingVec[3] = {parts.k.x, parts.k.y, parts.k.z};

			skeletonData->SetParentFrameTranslation(boneIndex, translationVec);
			skeletonData->SetParentFrameRotation(boneIndex, rotationVec);
			skeletonData->SetParentFrameScale(boneIndex, scalingVec);
		}

		// Read joint limits.
		{
			XSI::CRefArray parameters = bone.GetKinematics().GetLocal().GetParameters();

			// Since XSI limits are relative to the current node position, whereas the engine
			// expects them relative to the parent, we have to subtract the local euler rotations
			// from the limits.
			XSI::KinematicState kineState = bone.GetKinematics().GetLocal();
			XSI::MATH::CTransformation transform = kineState.GetTransform();
			float offsets[] = {float(transform.GetRotX()), float(transform.GetRotY()), float(transform.GetRotZ())};

			typedef std::map<std::string, JointParameterEntry> NameJointParameterMap;
			NameJointParameterMap nameJointParameterMap;
			nameJointParameterMap.insert(std::make_pair("rotxminlimit", JointParameterEntry(ISkeletonData::AxisX, ISkeletonData::LimitMin, offsets[0])));
			nameJointParameterMap.insert(std::make_pair("rotyminlimit", JointParameterEntry(ISkeletonData::AxisY, ISkeletonData::LimitMin, offsets[1])));
			nameJointParameterMap.insert(std::make_pair("rotzminlimit", JointParameterEntry(ISkeletonData::AxisZ, ISkeletonData::LimitMin, offsets[2])));
			nameJointParameterMap.insert(std::make_pair("rotxmaxlimit", JointParameterEntry(ISkeletonData::AxisX, ISkeletonData::LimitMax, offsets[0])));
			nameJointParameterMap.insert(std::make_pair("rotymaxlimit", JointParameterEntry(ISkeletonData::AxisY, ISkeletonData::LimitMax, offsets[1])));
			nameJointParameterMap.insert(std::make_pair("rotzmaxlimit", JointParameterEntry(ISkeletonData::AxisZ, ISkeletonData::LimitMax, offsets[2])));

			for (int paramIndex = 0, paramCount = parameters.GetCount(); paramIndex < paramCount; ++paramIndex)
			{
				XSI::Parameter parameter = parameters[paramIndex];
				std::string paramName = parameter.GetScriptName().GetAsciiString();
				std::transform(paramName.begin(), paramName.end(), paramName.begin(), std::tolower);
				NameJointParameterMap::iterator jointEntryPos = nameJointParameterMap.find(paramName);
				if (jointEntryPos != nameJointParameterMap.end())
				{
					float value = (float)parameter.GetValue(0.0f);
					const JointParameterEntry& jointEntry = (*jointEntryPos).second;
					value += jointEntry.localOffset;
					skeletonData->SetLimit(boneIndex, jointEntry.axis, jointEntry.limit, value);
				}
			}
		}

		// Read other joint parameters from the user properties.
		{
			// Read the user properties for this node.
			UserPropertyHelpers::UserPropertyReader propertyReader(bone);

			// Define the parameters to read from the user properties and what functions to use to set them in the skeleton data.
			char dampingParamName[] = ".damping";
			char springAngleParamName[] = ".springangle";
			enum {PARAM_COUNT = 3};
			char springTensionParamName[] = ".springtension";
			char* paramNames[PARAM_COUNT] = {dampingParamName, springAngleParamName, springTensionParamName};
			typedef void (ISkeletonData::*JointParamSetter)(int boneIndex, ISkeletonData::Axis axis, float value);
			JointParamSetter setters[PARAM_COUNT] = {&ISkeletonData::SetSpringTension, &ISkeletonData::SetSpringAngle, &ISkeletonData::SetAxisDamping};

			// Loop through all the axes.
			for (ISkeletonData::Axis axis = ISkeletonData::Axis(0); int(axis) < 3; axis = ISkeletonData::Axis(axis + 1))
			{
				// Loop through all the parameters we have defined.
				for (int param = 0; param < PARAM_COUNT; ++param)
				{
					// Rewrite the first char of the param in the param name to show the current axis.
					char* paramName = paramNames[param];
					paramName[0] = 'x' + param;

					// Look up the user property corresponding to this param.
					float value;
					if (propertyReader.GetPropertyByRef(paramName, value))
					{
						// Set the property in the skeleton data.
						(skeletonData->*(setters[param]))(boneIndex, axis, value);
					}
				}
			}
		}

		// Recurse to the node's children.
		for (int childIndex = 0, childCount = bone.GetChildren().GetCount(); childIndex < childCount; ++childIndex)
			RecurseAndListBones(skeletonData, XSI::X3DObject(bone.GetChildren().GetItem(childIndex)), boneIndex, materialData, heldReferences);
	}
}

bool GeometryExportSource::ReadSkeleton(const IGeometryFileData* const geometryFileData, int geometryFileIndex, const IModelData* const modelData, int modelIndex, const IMaterialData* materialData, ISkeletonData* skeletonData)
{
	XSI::CRef model = CRefHelpers::ToRef(modelData->GetModelHandle(modelIndex));

	std::set<XSI::CRef> skeletonRoots = SkinningHelpers::GetSkeletonRoots(model);

	for (std::set<XSI::CRef>::iterator rootPos = skeletonRoots.begin(), rootEnd = skeletonRoots.end(); rootPos != rootEnd; ++rootPos)
	{
		RecurseAndListBones(skeletonData, XSI::X3DObject(*rootPos), -1, materialData, m_heldReferences);
	}

	return !skeletonRoots.empty();
}

int GeometryExportSource::GetAnimationCount() const
{
	return 0;
}

std::string GeometryExportSource::GetAnimationName(const IGeometryFileData* geometryFileData, int geometryFileIndex, int animationIndex) const
{
	return std::string("default");
}

void GeometryExportSource::GetAnimationTimeSpan(float& start, float& stop, int animationIndex) const
{
	// Get the current project
	XSI::Application app;
	XSI::Project prj = app.GetActiveProject();

	// The PlayControl property set is stored with scene data under the project
	XSI::CRefArray proplist = prj.GetProperties();
	XSI::Property playctrl( proplist.GetItem(L"Play Control") );

	start = (float)playctrl.GetParameter( L"In" ).GetValue() / (float)playctrl.GetParameter( L"Rate" ).GetValue();
	stop = (float)playctrl.GetParameter( L"Out" ).GetValue() / (float)playctrl.GetParameter( L"Rate" ).GetValue();
}

void GeometryExportSource::ReadAnimationFlags(IExportContext* context, IAnimationData* animationData, const IGeometryFileData* const geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex) const
{
	// empty
}

struct SAnimFrameData
{
	Vec3 position;
	Vec3 rotation;
	Vec3 scale;
};

IAnimationData * GeometryExportSource::ReadAnimation(IExportContext* context, const IGeometryFileData* const geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex, float fps) const
{
	if( !modelData )
		return NULL;

	// Get the current project
	XSI::Application app;
	XSI::Project prj = app.GetActiveProject();
	// The PlayControl property set is stored with scene data under the project
	XSI::CRefArray proplist = prj.GetProperties();
	XSI::Property playctrl( proplist.GetItem(L"Play Control") );
	float start = playctrl.GetParameter( L"In" ).GetValue();
	float stop = playctrl.GetParameter( L"Out" ).GetValue();

	// get the sample step for timeline animation
	XSI::Property prop = app.GetActiveSceneRoot().GetProperties().GetItem( L"CryPropertiesUI" );
	// get step, first check if we have a valid parameter, if not, step is 2
	const int step = prop.GetParameter( L"ANMSampleStep" ).IsValid() ? prop.GetParameterValue( L"ANMSampleStep" ) : 2;
	int frameCount = ( stop - start ) / step + 1;
	IAnimationData* animationData = new NonSkeletalAnimationData( modelData->GetModelCount() );

	if( !animationData )
	{
		return NULL;
	}

	for( uint32 i = 0, iCount = modelData->GetModelCount(); i < iCount; ++i )
	{
		XSI::X3DObject obj = CRefHelpers::ToRef( modelData->GetModelHandle( i ) );

		// set up the key (frame) count
		animationData->SetFrameCountPos( i, frameCount );
		animationData->SetFrameCountRot( i, frameCount );
		animationData->SetFrameCountScl( i, frameCount );

		// if we have an invalid object, then skip this one
		if( !obj.IsValid() )
		{
			continue;
		}

		// start the frame counting
		int nFrame = 0;

		for( uint32 j = start; j <= stop; j += step )
		{
			XSI::CRefArray animParams = obj.GetAnimatedParameters();
			SAnimFrameData frameData;

			// clear up the frame data to zero
			memset( &frameData, 0, sizeof( frameData ) );

			for( uint32 k = 0, kCount = animParams.GetCount(); k < kCount; ++k )
			{
				XSI::Parameter	param = animParams[k];
				XSI::CString		sParamName = param.GetScriptName();

				//
				// check and evaluate position key
				//

				if( sParamName == L"posx" )
					frameData.position.x = param.GetValue( j );

				if( sParamName == L"posy" )
					frameData.position.y = param.GetValue( j );

				if( sParamName == L"posz" )
					frameData.position.z = param.GetValue( j );

				//
				// check and evaluate rotation key
				//

				if( sParamName == L"rotx" )
					frameData.rotation.x = param.GetValue( j );

				if( sParamName == L"roty" )
					frameData.rotation.y = param.GetValue( j );

				if( sParamName == L"rotz" )
					frameData.rotation.z = param.GetValue( j );

				//
				// check and evaluate scale key
				//

				if( sParamName == L"sclx" )
					frameData.scale.x = param.GetValue( j );

				if( sParamName == L"scly" )
					frameData.scale.y = param.GetValue( j );

				if( sParamName == L"sclz" )
					frameData.scale.z = param.GetValue( j );
			}

			// set the animation data
			IAnimationData::TCB tcb;
			IAnimationData::Ease ease;

			// compute time in seconds based on frame and fps, check fps for zero
			float time = (float) j / ( fps ? fps : 1.0f );

			animationData->SetFrameTimePos( i, nFrame, time );
			animationData->SetFrameTimeRot( i, nFrame, time );
			animationData->SetFrameTimeScl( i, nFrame, time );
			animationData->SetFrameDataPos( i, nFrame, &frameData.position.x );
			animationData->SetFrameDataRot( i, nFrame, &frameData.rotation.x );
			animationData->SetFrameDataScl( i, nFrame, &frameData.scale.x );

			// TODO: maybe we dont need this for XSI, we only have bezier here, no TCB
			animationData->SetFrameTCBPos( i, nFrame, tcb );
			animationData->SetFrameEaseInOutPos( i, nFrame, ease );
			animationData->SetFrameTCBRot( i, nFrame, tcb );
			animationData->SetFrameEaseInOutRot( i, nFrame, ease );
			animationData->SetFrameTCBScl( i, nFrame, tcb );
			animationData->SetFrameEaseInOutScl( i, nFrame, ease );

			// advance a frame
			++nFrame;
		}
	}

	return animationData;
}

namespace
{
	bool ReadGeometryFor3DObject(XSI::X3DObject& object, IGeometryData* geometry, const IMaterialData* const materialData)
	{
		MeshHelpers::MeshAccessor accessor(object);

		if (accessor.IsValid())
		{
			// Read positions.
			XSI::CDoubleArray positions = accessor.GetPositions();
			for (int i = 0, count = positions.GetCount(); i < count; i += 3)
				geometry->AddPosition(float(positions[i + 0]), float(positions[i + 1]), float(positions[i + 2]));

			// Read normals.
			XSI::CFloatArray normals = accessor.GetNormals();
			for (int i = 0, count = normals.GetCount(); i < count; i += 3)
				geometry->AddNormal(normals[i + 0], normals[i + 1], normals[i + 2]);

			// Read uvs.
			XSI::CFloatArray uvs = accessor.GetUVs();
			for (int i = 0, count = uvs.GetCount(); i < count; i += 3) // Stride is 3 (uvw)
				geometry->AddTextureCoordinate(uvs[i + 0], uvs[i + 1]);

			// Read colours.
			XSI::CFloatArray colours = accessor.GetColours();
			for (int i = 0, count = colours.GetCount(); i < count; i += 4) // TODO: Is this stride correct?
				geometry->AddVertexColour(colours[i + 0], colours[i + 1], colours[i + 2], colours[i + 3]);

			// Read triangles.
			XSI::CLongArray vertexIndices = accessor.GetVertexIndices();
			XSI::CLongArray nodeIndices = accessor.GetNodeIndices();
			XSI::CLongArray polygonMaterialIndices = accessor.GetPolygonMaterialIndices();
			XSI::CLongArray trianglePolygonIndices = accessor.GetPolygonTriangleIndices();
			XSI::CRefArray materials = accessor.GetMaterials();
			for (int triangleIndex = 0, triangleCount = accessor.GetTriangleCount(); triangleIndex < triangleCount; ++triangleIndex)
			{
				int indices[12] = {
					vertexIndices[triangleIndex * 3 + 0], nodeIndices[triangleIndex * 3 + 0], nodeIndices[triangleIndex * 3 + 0], nodeIndices[triangleIndex * 3 + 0],
					vertexIndices[triangleIndex * 3 + 1], nodeIndices[triangleIndex * 3 + 1], nodeIndices[triangleIndex * 3 + 1], nodeIndices[triangleIndex * 3 + 1],
					vertexIndices[triangleIndex * 3 + 2], nodeIndices[triangleIndex * 3 + 2], nodeIndices[triangleIndex * 3 + 2], nodeIndices[triangleIndex * 3 + 2]
				};

				// To find the material we need to do a linear search through the known materials. This is because multiple CRefs can refer to
				// the same object in XSI, so comparing pointer values is insufficient to check for identity. I don't know of any way to do
				// a proper ordering either, so we are left with a linear search. Should be ok though, only 10s of materials.
				int polygonIndex = trianglePolygonIndices[triangleIndex];
				int localMaterialIndex = polygonMaterialIndices[polygonIndex];
				XSI::CRef materialRef = materials[localMaterialIndex];
				int materialIndex = -1;
				for (int i = 0, count = materialData->GetMaterialCount(); i < count; ++i)
				{
					XSI::CRef ref = CRefHelpers::ToRef(materialData->GetHandle(i));
					if (ref == materialRef)
						materialIndex = i;
				}
				geometry->AddPolygon(indices, materialIndex);
			}
		}

		return accessor.IsValid();
	}
}

bool GeometryExportSource::ReadGeometry(IExportContext* context, IGeometryData* geometry, const IModelData* const modelData, const IMaterialData* const materialData, int modelIndex)
{
	XSI::X3DObject object = CRefHelpers::ToRef(modelData->GetModelHandle(modelIndex));

	return ReadGeometryFor3DObject(object, geometry, materialData);
}

namespace
{
	bool ReadGeometryMaterialDataFor3DObject(XSI::X3DObject& object, IGeometryMaterialData* geometryMaterialData, const IMaterialData* const materialData)
	{
		MeshHelpers::MeshAccessor accessor(object);

		if (accessor.IsValid())
		{
			XSI::CRefArray materials = accessor.GetMaterials();
			for (int localMaterialIndex = 0, localMaterialCount = materials.GetCount(); localMaterialIndex < localMaterialCount; ++localMaterialIndex)
			{
				XSI::CRef materialRef = materials[localMaterialIndex];

				// To find the material we need to do a linear search through the known materials. This is because multiple CRefs can refer to
				// the same object in XSI, so comparing pointer values is insufficient to check for identity. I don't know of any way to do
				// a proper ordering either, so we are left with a linear search. Should be ok though, only 10s of materials.
				int materialIndex = -1;
				for (int i = 0, count = materialData->GetMaterialCount(); i < count; ++i)
				{
					XSI::CRef ref = CRefHelpers::ToRef(materialData->GetHandle(i));
					if (ref == materialRef)
						materialIndex = i;
				}

				if (materialIndex >= 0)
					geometryMaterialData->AddUsedMaterialIndex(materialIndex);
			}
		}

		return accessor.IsValid();
	}
}

bool GeometryExportSource::ReadGeometryMaterialData(IExportContext* context, IGeometryMaterialData* geometryMaterialData, const IModelData* const modelData, const IMaterialData* const materialData, int modelIndex) const
{
	XSI::X3DObject object = CRefHelpers::ToRef(modelData->GetModelHandle(modelIndex));

	return ReadGeometryMaterialDataFor3DObject(object, geometryMaterialData, materialData);
}

bool GeometryExportSource::ReadBoneGeometry(IExportContext* context, IGeometryData* geometry, ISkeletonData* skeletonData, int boneIndex, const IMaterialData* const materialData)
{
	XSI::X3DObject object = CRefHelpers::ToRef(skeletonData->GetBoneHandle(boneIndex));

	return ReadGeometryFor3DObject(object, geometry, materialData);
}

bool GeometryExportSource::ReadBoneGeometryMaterialData(IExportContext* context, IGeometryMaterialData* geometryMaterialData, ISkeletonData* skeletonData, int boneIndex, const IMaterialData* const materialData) const
{
	XSI::X3DObject object = CRefHelpers::ToRef(skeletonData->GetBoneHandle(boneIndex));

	return ReadGeometryMaterialDataFor3DObject(object, geometryMaterialData, materialData);
}

void GeometryExportSource::ReadMorphs(IExportContext* context, IMorphData* morphData, const IModelData* const modelData, int modelIndex)
{
}

bool GeometryExportSource::ReadMorphGeometry(IExportContext* context, IGeometryData* geometry, const IModelData* const modelData, int modelIndex, const IMorphData* const morphData, int morphIndex, const IMaterialData* materialData)
{
	return true;
}

bool GeometryExportSource::HasValidPosController(const IModelData* modelData, int modelIndex) const
{
	return true;
}

bool GeometryExportSource::HasValidRotController(const IModelData* modelData, int modelIndex) const
{
	return true;
}

bool GeometryExportSource::HasValidSclController(const IModelData* modelData, int modelIndex) const
{
	return true;
}