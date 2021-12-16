#pragma once

class CRenderMeshMerger : public Cry3DEngineBase
{
public:
	CRenderMeshMerger(void);
	~CRenderMeshMerger(void);

	struct SDecalClipInfo { Vec3 vPos; float fRadius; Vec3 vProjDir; };

	struct SMergeInfo
	{
		const char *sMeshType;
		const char *sMeshName;
		bool bCompactVertBuffer;
		bool bPrintDebugMessages;
		bool bMakeNewMaterial;
		bool bMergeToOneRenderMesh;
		bool bPlaceInstancePositionIntoVertexNormal;
		IMaterial *pUseMaterial; // Force to use this material
		
		SDecalClipInfo * pDecalClipInfo;
		AABB *pClipCellBox;
    Vec3 vResultOffset; // this offset will be subtracted from output vertex positions
		
		SMergeInfo() : 
			bCompactVertBuffer(false),
			bPrintDebugMessages(false),
			bMakeNewMaterial(true),
			sMeshType(""),sMeshName(""),pDecalClipInfo(0),pClipCellBox(0),pUseMaterial(0),
			bPlaceInstancePositionIntoVertexNormal(0), vResultOffset(0,0,0) {}
	};

  IRenderMesh * MergeRenderMeshes(SRenderMeshInfoInput *pRMIArray, int nRMICount,PodArray<SRenderMeshInfoOutput> &outRenderMeshes,SMergeInfo &info );

public:
	struct SMergedChunk
	{
		CRenderChunk rChunk;
		IMaterial *pMaterial;
	};

protected:
	PodArray<SRenderMeshInfoInput> m_lstRMIChunks;
	PodArray<SVF_P3S_C4B_T2S> m_lstVerts;
	PodArray<SPipTangents> m_lstTangBasises;
	PodArray<uint32> m_lstIndices;
	PodArray<SMergedChunk> m_lstChunks;

	PodArray<SVF_P3S_C4B_T2S> m_lstNewVerts;
	PodArray<SPipTangents> m_lstNewTangBasises;
	PodArray<uint16> m_lstNewIndices;
	PodArray<SMergedChunk> m_lstNewChunks;
	
	PodArray<SMergedChunk> m_lstChunksMergedTemp;

	int m_nTotalVertexCount;
	int m_nTotalIndexCount;

	void MakeRenderMeshInfoListOfAllChunks( SRenderMeshInfoInput * pRMIArray, int nRMICount, SMergeInfo &info );
	void MakeListOfAllCRenderChunks( SMergeInfo &info );
	void IsChunkValid(CRenderChunk & Ch, PodArray<SVF_P3S_C4B_T2S> & lstVerts, PodArray<uint32> & lstIndices);

  void ClipByAABB( SMergeInfo &info );
	void ClipDecals( SMergeInfo &info );
	void CompactVertices( SMergeInfo &info );
	void TryMergingChunks( SMergeInfo &info );

	bool ClipTriangle(int nStartIdxId, Plane * pPlanes, int nPlanesNum);

	static int Cmp_Materials(IMaterial *pMat1, IMaterial *pMat2);
	static int Cmp_RenderChunks_(const void* v1, const void* v2);
	static int Cmp_RenderChunksInfo(const void* v1, const void* v2);
};
