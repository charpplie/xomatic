//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "ModifierUtils.h"
#include "AssetWriter.h"
#include "ISkinningInfo.h"
#include "ISkeleton.h"
#include "IBoneArray.h"
#include "IBone.h"
#include "IProgressMonitor.h"
#include "IErrorReporter.h"
#include "IChunkList.h"
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

AssetWriter::AssetWriter(FILE* f, IErrorReporter* pErrorReporter, IProgressMonitor* pProgressMonitor)
:	f(f),
	pErrorReporter(pErrorReporter),
	pProgressMonitor(pProgressMonitor)
{
}

// arrVertexMap maps compacted vertices -> original Max vertices (through the split vertices)
bool AssetWriter::SaveBoneInfo(ISkeleton* pSkeleton, ISkinningInfo* pSkinningInfo, const Matrix34& objectTransform, const std::string& sNodeName)
{
	size_t n_vert = pSkinningInfo->GetNumVertices();
	for(size_t k=0;k<n_vert;k++)
	{
		pProgressMonitor->UpdateProgress(100*k/n_vert,"\"%s\" Writing bone weights", sNodeName.c_str());

		int nLinks = pSkinningInfo->GetNumBonesForVertex(k);

		//=================
		// Write nLinks
		//=================
		std::vector<CryLink> arrLinks;

		// the minimal weight that the link must have to be exported
		const float fMinWeight = 0.01f;

		// this is the max number of weights per vertex - the extra ewights are clamped
		const unsigned nMaxLinksPerVertex = 5;

		// the total weight of the links that are exported (for renormalization)
		float fTotalWeight = 0;

		for (int j = 0;j < nLinks; j++)
		{
			CryLink link;
			ISkinningInfo::VertexBoneLinkInfo info;
			Vec3 point;
			pSkinningInfo->GetVertex(k, point);
			Vec3 worldVertex = objectTransform.TransformPoint(point);
			pSkinningInfo->GetBoneLinkInfoForVertex(k, j, worldVertex, info);

			link.BoneID = info.nBoneID;
			link.Blending	= info.fBlendingWeight;
			if (link.Blending < fMinWeight)
				continue;

			link.offset = info.offset;

			fTotalWeight += link.Blending;
			arrLinks.push_back(link);
		}

		if (fTotalWeight < 0.5)
		{
			char szDebug [1024];
			sprintf (szDebug, "%d links {", nLinks);
			unsigned i;
			for (i = 0; i < arrLinks.size(); ++i)
			{
				if (i)
					strcat (szDebug, ";");
				CryLink& l = arrLinks[i];
				std::string sBonename = "#OUT OF RANGE#";
				if (l.BoneID >= 0 && l.BoneID < pSkeleton->GetBones()->Count())
					sBonename = pSkeleton->GetBones()->Get(l.BoneID)->GetBoneName().c_str();
				sprintf (szDebug+strlen(szDebug), "v=(%g,%g,%g) w=%g b=%s", l.offset.x, l.offset.y, l.offset.z, l.Blending, sBonename.c_str());
			}
			Vec3 point;
			pSkinningInfo->GetVertex(k, point);
			sprintf (szDebug+strlen(szDebug), "} vertex={%g,%g,%g}", point.x, point.y, point.z);

			char szMessage[1024+400];
			sprintf (szMessage, "Cannot export vertex %d because all link weights sum up to only %.2f. They should sum up to 1. Please renormalize and/or reassign the vertices.\nDebug info: %s", k, fTotalWeight, szDebug);
			this->pErrorReporter->Report(IErrorReporter::Error, szMessage);
			return true;
		}

		for (j = 0; j < (int)arrLinks.size(); ++j)
			arrLinks[j].Blending /= fTotalWeight;

		std::sort (arrLinks.begin(), arrLinks.end(), CryLinkOrderByBlending());

		if (arrLinks.size() > nMaxLinksPerVertex)
			arrLinks.resize (nMaxLinksPerVertex);

		// write out the links: save the number of links, and the links themselves then
		nLinks = arrLinks.size();
		int res=fwrite(&nLinks, sizeof(nLinks), 1, f);
		if (res !=1)
			return true;
		res = fwrite(&arrLinks[0], sizeof(arrLinks[0]), arrLinks.size(), f);
		if (res != arrLinks.size())
			return true;
	}

	// Print out any errors encountered.
	if (ISkinningInfo::Error* pError = pSkinningInfo->GetError())
	{
		this->pErrorReporter->Report(IErrorReporter::Warning, std::string("Error while processing skinning info: ") + pError->sDescription);
	}
	else if (pSkinningInfo->GetNumBoneOutOfRangeWarnings() > 0)
	{
		std::ostringstream message;
		message << "The following " << pSkinningInfo->GetNumBoneOutOfRangeWarnings() << " vertex links are to bones that are not exported\n";
		int nNumLinksToPrint = min(pSkinningInfo->GetNumBoneOutOfRangeWarnings(), 25);
		for (int nWarning = 0; nWarning < nNumLinksToPrint; ++nWarning)
		{
			ISkinningInfo::BoneOutOfRangeWarning warning;
			pSkinningInfo->GetBoneOutOfRangeWarning(nWarning, warning);
			message << "  Vertex: " << std::setiosflags(std::ios_base::left) << std::setw(12) << warning.nVertex;
			message << "Bone: \"" << warning.sBoneName << "\"\n";
		}
		if (nNumLinksToPrint < pSkinningInfo->GetNumBoneOutOfRangeWarnings())
			message << "<" << pSkinningInfo->GetNumBoneOutOfRangeWarnings() - nNumLinksToPrint << " more>\n";

		this->pErrorReporter->Report(IErrorReporter::Warning, message.str());
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
// Saves the chunk containing initial poses of each bone
bool AssetWriter::SaveBoneInitialPos(ISkinningInfo* pSkinningInfo, ISkeleton* pSkeleton, int nChunkIdMesh, IChunkList* pChunkList)
{	
	if (pSkeleton->GetBones()->Count() == 0)
		return true;
	std::vector<Matrix34> arrBoneInitPos;
	arrBoneInitPos.resize (pSkeleton->GetBones()->Count());
	if (CalculateBoneInitPos(pSkinningInfo, pSkeleton, &arrBoneInitPos[0]) == 0)
		return true;

	fpos_t fpos;
	fgetpos (f, &fpos);

	BONEINITIALPOS_CHUNK_DESC_0001 ChunkHeader;
	ChunkHeader.nChunkIdMesh = nChunkIdMesh;
	ChunkHeader.numBones = pSkeleton->GetBones()->Count();

	// append
	CHUNK_HEADER ch_ent;
	ch_ent.FileOffset = (int)fpos;
	ch_ent.ChunkType  = ChunkType_BoneInitialPos;
	ch_ent.ChunkVersion = ChunkHeader.VERSION;
	pChunkList->Append(NULL, &ch_ent, true);

	if (1 != fwrite (&ChunkHeader,sizeof(ChunkHeader),1,f))
		return true; // error

	// convert Matrix3 into 4x3 Cry matrices to put into the chunk
	for (unsigned i = 0; i < arrBoneInitPos.size(); ++i)
	{
		SBoneInitPosMatrix mtx;
		for (int x = 0; x < 4; ++x)
		{
			Vec3 column = arrBoneInitPos[i].GetColumn(x);
			for (int y = 0; y < 3; ++y)
				mtx[x][y]= column[y];
		}
		if (1 != fwrite (&mtx, sizeof(mtx), 1, f))
			return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
// Calculates the initial position of each bone from arrBones, in context of
// physique modifier in pNode
// Returns the number of bone transformations got from physique, or 0 if not successful
// PARAMETERS:
//  pSkinNode    - the node that contains the skin
//  pBoneInitPos - [IN] Must be the buffer of at least arrBones.Count() elements
//                [OUT] the initial position of the bone
//
#include "ISkinningInfo.h"
unsigned AssetWriter::CalculateBoneInitPos(ISkinningInfo* pSkinningInfo, ISkeleton* pSkeleton, Matrix34* pBoneInitPos)
{
	unsigned nResult = 0;
	for (int nBone = 0; nBone < pSkeleton->GetBones()->Count(); ++nBone)
	{
		Matrix34 cryInitMatrix;
		IBone* pBone = pSkeleton->GetBones()->Get(nBone);
		Matrix3 tmInit;
		if (!pSkinningInfo->GetBoneInitialPosition(pBone, cryInitMatrix))
		{	
			// get the current position of the node as the initial
			Matrix34 cryTmInit;
			pBone->GetTransform(cryTmInit);
			CryToMaxMatrix(tmInit, cryTmInit);

			// now find the first parent that has initial pose matrix MI, and current matrix MX and 
			// set tmInit = tmInit * (MI / MX)
			IBone* pParent = pBone->GetParent();
			while (pParent)
			{
				Matrix34 cryParentInitMatrix;
				if (pSkinningInfo->GetBoneInitialPosition(pParent, cryParentInitMatrix))
				{
					Matrix3 tmParentInitial;
					CryToMaxMatrix(tmParentInitial, cryParentInitMatrix);
					Matrix34 cryParentCurrentTransform;
					pParent->GetTransform(cryParentCurrentTransform);
					Matrix3 tmParentNow;
					CryToMaxMatrix(tmParentNow, cryParentCurrentTransform);
					tmInit = (tmInit * Inverse (tmParentNow)) * tmParentInitial;
					break;
					//Matrix34 cryParentCurrentTransform;
					//pParent->GetTransform(cryParentCurrentTransform);
					//Matrix3 tmParentNow;
					//CryToMaxMatrix(tmParentNow, cryParentCurrentTransform);
					//MaxToCryMatrix(cryParentCurrentTransform, Inverse (tmParentNow));
					//tmInit = (tmInit * cryParentCurrentTransform) * cryParentInitMatrix;
					//break;
				}
				pParent = pParent->GetParent();
			}
		}
		else
		{
			CryToMaxMatrix(tmInit, cryInitMatrix);
			++nResult ;
		}
		MaxToCryMatrix(pBoneInitPos[nBone], tmInit);
	}
	return nResult;
}
