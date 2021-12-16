#ifndef __ANIMATIONEXPORTSOURCE_H__
#define __ANIMATIONEXPORTSOURCE_H__

#include "Export/IExportSource.h"

#include <xsi_ref.h>
#include <xsi_actionsource.h>

class AnimationExportSource : public IExportSource
{
public:
	AnimationExportSource(XSI::ActionSource& source, XSI::CRefArray& roots);

	// IExportSource
	virtual void GetMetaData(SExportMetaData& metaData) const;
	virtual std::string GetDCCFileName() const;
	virtual std::string GetExportDirectory() const;
	virtual void ReadGeometryFiles(IExportContext* context, IGeometryFileData* geometryFileData);
	virtual bool ReadMaterials(IExportContext* context, const IGeometryFileData* geometryFileData, IMaterialData* materialData);
	virtual void ReadModels(const IGeometryFileData* geometryFileData, int geometryFileIndex, IModelData* modelData);
	virtual void ReadSkinning(IExportContext* context, ISkinningData* skinningData, const IModelData* modelData, int modelIndex, ISkeletonData* skeletonData);
	virtual bool ReadSkeleton(const IGeometryFileData* geometryFileData, int geometryFileIndex, const IModelData* modelData, int modelIndex, const IMaterialData* materialData, ISkeletonData* skeletonData);
	virtual int GetAnimationCount() const;
	virtual std::string GetAnimationName(const IGeometryFileData* geometryFileData, int geometryFileIndex, int animationIndex) const;
	virtual void GetAnimationTimeSpan(float& start, float& stop, int animationIndex) const;
	virtual void ReadAnimationFlags(IExportContext* context, IAnimationData* animationData, const IGeometryFileData* geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex) const;
	virtual IAnimationData * ReadAnimation(IExportContext* context, const IGeometryFileData* geometryFileData, const IModelData* modelData, int modelIndex, const ISkeletonData* skeletonData, int animationIndex, float fps) const;
	virtual bool ReadGeometry(IExportContext* context, IGeometryData* geometry, const IModelData* modelData, const IMaterialData* materialData, int modelIndex);
	virtual bool ReadGeometryMaterialData(IExportContext* context, IGeometryMaterialData* geometryMaterialData, const IModelData* modelData, const IMaterialData* materialData, int modelIndex) const;
	virtual bool ReadBoneGeometry(IExportContext* context, IGeometryData* geometry, ISkeletonData* skeletonData, int boneIndex, const IMaterialData* materialData);
	virtual bool ReadBoneGeometryMaterialData(IExportContext* context, IGeometryMaterialData* geometryMaterialData, ISkeletonData* skeletonData, int boneIndex, const IMaterialData* materialData) const;
	virtual void ReadMorphs(IExportContext* context, IMorphData* morphData, const IModelData* modelData, int modelIndex);
	virtual bool ReadMorphGeometry(IExportContext* context, IGeometryData* geometry, const IModelData* modelData, int modelIndex, const IMorphData* morphData, int morphIndex, const IMaterialData* materialData);
	virtual bool HasValidPosController(const IModelData* modelData, int modelIndex) const;
	virtual bool HasValidRotController(const IModelData* modelData, int modelIndex) const;
	virtual bool HasValidSclController(const IModelData* modelData, int modelIndex) const;

private:
	XSI::ActionSource& m_source;
	XSI::CRefArray& m_roots;
	XSI::CRefArray m_heldReferences;
};

#endif //__ANIMATIONEXPORTSOURCE_H__
