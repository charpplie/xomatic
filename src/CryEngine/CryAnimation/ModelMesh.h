//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:ModelSkeleton.cpp
//  Implementation of Skeleton class
//
//	History:
//	January 12, 2005: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////

#ifndef _CRY_SKIN_HDR_
#define _CRY_SKIN_HDR_

#include <CGFContent.h>


class CCharacterModel;
class CModelJoint;
class CSkinInstance;
class CCharInstance;
class CAttachment;
class CMorphing;

struct BoneID 
{
	/*uint16*/uint8 idx0,idx1,idx2,idx3;
	void GetMemoryUsage(ICrySizer *pSizer) const{}
};

struct AttSkinVertex
{
	Vec3 wpos0;			//vertex-position of model.1 
	Vec3 wpos1;			//vertex-position of model.2 
	Vec3 wpos2;			//vertex-position of model.3 
	uint8 boneIDs[4];
	f32 weights[4];
	ColorB color;   //index for blend-array
  uint32 org_vertex_index; 
	void FromExtSkinVertex(const ExtSkinVertex& skin);
	void GetMemoryUsage(ICrySizer *pSizer ) const{}
};

struct MorphTargetVertex
{
	uint16 m_nVertexId;			//vertex index in the original (mesh) array of vertices
	union {
		struct {
			uint8 m_MTVertexX;      // the target point of the morph target
			uint8 m_MTVertexY;      // the target point of the morph target
			uint8 m_MTVertexZ;      // the target point of the morph target
			uint8 m_fBalanceLR;   // Vertex Left balance multiplier.
		};
		uint8 m_Data[4];
	};
	void GetMemoryUsage(ICrySizer *pSizer ) const {}
};

//////////////////////////////////////////////////////////////////////////
class CMorphTarget : public _i_reference_target_t
{
public:

	uint32 m_MeshID;	//currently not used
	string m_name;		// Morph target name.

	Vec3 m_MTNegBasis;
	Vec3 m_MTExtensions;
	//--------------------------------------------------------------------

	// Morph vertices
	DynArray<MorphTargetVertex> m_vertices;

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject( m_vertices );
	}
};

//////////////////////////////////////////////////////////////////////////
//                    This is the skinning class:                       //
//       it skins vertices & tangents and applys morph-targets          // 
//////////////////////////////////////////////////////////////////////////
class CModelMesh 
{
public:
	CModelMesh():
			m_PosStride(0),
			m_UVStride(0),
			m_ColorStride(0),
			m_SkinStride(0),
			m_ShapeStride(0),
			m_TangentStride(0),
			m_BinormalStride(0),
			m_Indices(0),
			m_pIndices(0),
			m_iThreadMeshAccessCounter(0),
			m_pPositions(0),
			m_pSkinningInfo(0),
			m_pMorphingInfo(0),
			m_pUVs(0),
			m_pColors(0),
			m_pTangents(0),
			m_pBinormals(0),
			m_nLOD(0),
			m_pModel(0),
			m_FrameIdVertexArray(-1),
			m_FrameIdSkinInit(-1),
			m_numExtVertices(0),
			m_numExtTriangles(0)
	{
		for(int i=0;i<VERTEX_REGS_NUM;i++)
			VertexRegs[i] = Vec4(0.0f,0.0f,0.0f,0.0f);
	}

	~CModelMesh(){}

	CloseInfo FindClosestPointOnMesh( const Vec3& RMWPosition, bool bUseCenterPointDist=true); 

	void InitSkinningExtSW (CMorphing* pMorphing, f32* pShapeDeform,uint32 nResetMode, int nLOD);

	float ComputeExtent(GeomQuery& geo, EGeomForm eForm);
	void GetRandomPos(CCharInstance* pInstance, RandomPos& ran, GeomQuery& geo, EGeomForm eForm);

	void DrawWireframe( CMorphing* pMorphing, QuatTS* parrNewSkinQuat, f32* pShapeDeform,uint32 nResetMode, int nLOD,const Matrix34& rRenderMat34);
	void DrawWireframeStatic( CMorphing* pMorphing, const Matrix34& m34, int nLOD, uint32 color);

	void DrawTangents (		const Matrix34& rRenderMat34 );
	void DrawBinormals (	const Matrix34& rRenderMat34 );
	void DrawNormals (		const Matrix34& rRenderMat34 );

	size_t SizeOfModelMesh(ICrySizer *pSizer) const;
	void GetMemoryUsage(ICrySizer *pSizer) const;
	void DrawDebugInfo(CCharInstance* pInstance, int nLOD,const Matrix34& rRenderMat34, int DebugMode, IMaterial* & pMaterial,  CRenderObject *pObj, const SRendParams& RendParams);

	// returns the index of the morph target, in the indexation of the array of morph targets
	int FindMorphTarget (const char* szMorphTargetName)
	{
		int numMophs = m_morphTargets.size();
		for (int i=0; i<numMophs; ++i)
			if (!stricmp(m_morphTargets[i]->m_name.c_str(), szMorphTargetName))
				return i;
		return -1;
	}
	
	void ExportModel();

	Vec3 GetSkinnedExtVertex2( QuatTS* parrNewSkinQuat, uint32 i);
	void LockFullRenderMesh(int lod);
	void UnlockFullRenderMesh(int lod);

	inline ExtSkinVertex GetSkinVertex(int pos)
	{
		if (m_pIndices)
			pos = m_pIndices[pos];
		return GetSkinVertexNoInd(pos);
	}
	ExtSkinVertex GetSkinVertexNoInd(int pos);

	void SkinningExtSW(CCharInstance* pInstance, CAttachment* pAttachment, int nLOD, int nList);
	void SkinningForDecals(CSkinInstance* pInstance, int bone);
	void GetSingleSkinnedVertex(CCharInstance* pInstance, int nV, int nList, Vec3* pVert, Vec3* pNorm, bool bLockUnlock = true);
	uint32 GetVertextCount() const;

private:

	Vec3 GetSkinnedExtNormal(CSkinInstance* pInstance,uint32 i);
	void VertexShader_SBS(QuatTS* parrNewSkinQuat, uint32 i); 
	void VertexShader_LBS(QuatTS* parrNewSkinQuat, uint32 i); 


public:
	//////////////////////////////////////////////////////////////////////////
	// Member variables.
	//////////////////////////////////////////////////////////////////////////

	// For render mesh locks
	int32 m_PosStride;
	int32 m_UVStride;
	int32 m_ColorStride;
	int32 m_SkinStride;
	int32 m_ShapeStride;
	int32 m_TangentStride;
	int32 m_BinormalStride;
	int32 m_Indices;

	unsigned short int * m_pIndices;
	volatile int m_iThreadMeshAccessCounter;
	byte * m_pPositions;
	byte * m_pSkinningInfo;
	byte * m_pMorphingInfo;
	byte * m_pUVs;
	byte * m_pColors;
	byte * m_pTangents;
	byte * m_pBinormals;

	uint32 m_nLOD;
	CCharacterModel* m_pModel;
	static const uint32 VERTEX_REGS_NUM = 8;
	Vec4 VertexRegs[VERTEX_REGS_NUM];											//the blend-parameter for shape deformation as they appear in the constat registers


	int m_FrameIdVertexArray;						// to avoid skinning a mesh multiple times a frame
	int m_FrameIdSkinInit;							// to avoid skinning a mesh multiple times a frame
	uint32 m_numExtVertices;
	uint32 m_numExtTriangles;

	//DynArray<PhysicalProxy> m_arrPhyBoneMeshes;
	DynArray< _smart_ptr<CMorphTarget> > m_morphTargets;
	DynArray<IntSkinVertex> m_arrIntVertices;			//internal vertices (just position and links)
	DynArray<BoneID> m_arrExtBoneIDs;					//this is a list with the global BoneIDs
	DynArray<AttSkinVertex> m_arrExtVerticesCached;
};

#endif
