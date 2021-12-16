#include "stdafx.h"

#include "Utility.h"
#include "ModifierUtils.h"
#include "MorpherUtils.h"
#include "IMorphData.h"
#include "IMorphChannel.h"
#include "IMorphChannelArray.h"
#include "ISourceMesh.h"

inline Vec3& operator << (Vec3& left, Point3& right)
{
	left.x = right.x;
	left.y = right.y;
	left.z = right.z;
	return left;
}

// returns false if it's ok
// arrVertexMap maps compacted vertices -> original Max vertices (through the split vertices)
// nChunkIdMesh - the chunk id of the mesh of pNode that is to be exported or already has been exported
// tm - with this matrix, all the target vertices will be transformed (the original Max vertices were transformed with this matrix before being exported)
bool CSExportUtility::SaveMorphTargets (FILE* f, ISourceMesh* pMesh, const Matrix34& tm, int nVertexMapSize, int* pVertexMap, int nChunkIdMesh, const Vec3& morphOffset)
{
	IMorphData* pMorphData = pMesh->GetMorphData();
	if (!pMorphData)
		return false;

	int nNumChannnels = pMorphData->GetMorphChannels()->Count();
	for (int nChannel = 0; nChannel < nNumChannnels; ++nChannel)
	{
		IMorphChannel* pMorphChannel = pMorphData->GetMorphChannels()->Get(nChannel);
		if (SaveMorphTarget (f, tm, pMorphData, pMorphChannel, nVertexMapSize, pVertexMap, nChunkIdMesh, morphOffset))
			return true;
	}
	return false;
}

// arrVertexMap maps compacted vertices -> original Max vertices (through the split vertices)
// returns error bit
bool CSExportUtility::SaveMorphTarget (FILE* f, const Matrix34& tm, IMorphData* pMorphData, IMorphChannel* pMorphChannel, int nVertexMapSize, int* pVertexMap, int nChunkIdMesh, const Vec3& morphOffset)
{
	float fMinOffsetSqr = pMorphData->GetMinOffset();
	fMinOffsetSqr *= fMinOffsetSqr;

	int nModifiedVertex;
	unsigned numMorphVertices = 0;

	BitArray arrModifiedMaxVerts (pMorphChannel->GetNumPoints());
	arrModifiedMaxVerts.ClearAll();

	// set the bits for the vertices modified in Max indexation
	for (nModifiedVertex = 0; nModifiedVertex < pMorphChannel->GetNumPoints(); ++nModifiedVertex)
	{
		float fDeltaLen = pMorphChannel->GetDelta(nModifiedVertex).GetLengthSquared();
		if (fDeltaLen >= fMinOffsetSqr)
		{
			arrModifiedMaxVerts.Set(nModifiedVertex);
			++numMorphVertices;
		}
	}

	if (!numMorphVertices)
	{
		m_skippedMorphTargetNames.push_back(pMorphChannel->GetName());
		return false;
	}

	// add the chunk to the chunk list, already with the file offset info
	fpos_t fpos;
	fgetpos(f,&fpos);
	CHUNK_HEADER ch_ent;
	ch_ent.FileOffset = (int)fpos;
	ch_ent.ChunkType  = ChunkType_MeshMorphTarget;
	ch_ent.ChunkVersion = MESHMORPHTARGET_CHUNK_DESC_0001::VERSION;
	ChunkList.Append(NULL, &ch_ent, true);

	// write the header of the chunk directly into the file
	MESHMORPHTARGET_CHUNK_DESC_0001 ChunkHeader;
	ChunkHeader.nChunkIdMesh = nChunkIdMesh;
	ChunkHeader.numMorphVertices = 0;

	// calculate the number of the vertices modified in Cry indexation (some of Max-indexed vertices may not be present,
	// or some may be present twice)
	for (nModifiedVertex = 0; nModifiedVertex < nVertexMapSize; ++nModifiedVertex)
	{
		if (arrModifiedMaxVerts[pVertexMap[nModifiedVertex]])
			++ChunkHeader.numMorphVertices;			
	}

	// write the header finally
	if (1 != fwrite (&ChunkHeader, sizeof(ChunkHeader), 1, f))
		return true;

	// for each modified vertex (in Cry indexation) write the corresponding structure
	for (nModifiedVertex = 0; nModifiedVertex < nVertexMapSize; ++nModifiedVertex)
	{
		if (arrModifiedMaxVerts[pVertexMap[nModifiedVertex]])
		{
			SMeshMorphTargetVertex MorphVertex;
			MorphVertex.nVertexId = nModifiedVertex;
			MorphVertex.ptVertex = tm.TransformPoint(pMorphChannel->GetPosition(pVertexMap[nModifiedVertex]) + morphOffset);
			if (1 != fwrite (&MorphVertex, sizeof(MorphVertex), 1, f))
				return true;
		}
	}
	// now write the name of the morph target; in the future, padding may be needed for proper alignment
	if (1 != fwrite (pMorphChannel->GetName().c_str(), pMorphChannel->GetName().size() + 1, 1, f))
		return true;

	return false; // all OK
}
