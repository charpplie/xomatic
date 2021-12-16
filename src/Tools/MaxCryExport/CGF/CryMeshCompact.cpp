#include "stdafx.h"
#include <vector>
#include "CryMeshCompact.h"

// Proxy class describing properties of the texture coordinates and faces of CryEngine
struct CryTexMeshProxy
{
	enum {g_numCoords = 2};
	enum {g_numFaceVerts = 3};
	typedef CryTexFace Face;
	typedef CryUV Vert;

	static int FaceVertex (const Face& face, int nIndex)
	{
		assert (&face.t1 == &face.t0 + 1 && &face.t2 == &face.t1+1);
		return (&face.t0)[nIndex];
	}

	static int& FaceVertex (Face& face, int nIndex)
	{
		assert (&face.t1 == &face.t0 + 1 && &face.t2 == &face.t1+1);
		return (&face.t0)[nIndex];
	}

	static float VertCoord (const Vert& vert, int nIndex)
	{
		assert (&vert.v == &vert.u+1);
		assert (nIndex >= 0 && nIndex <2);
		return (&vert.u)[nIndex];
	}
};

// Proxy class describing properties of the mesh coordinates and faces of CryEngine
struct CryMeshProxy
{
	enum {g_numCoords = 3};
	enum {g_numFaceVerts = 3};
	typedef CryFace Face;
	typedef CryVertex Vert;

	static int FaceVertex (const CryFace& face, int nIndex)
	{
		assert (&face.v1 == &face.v0 + 1 && &face.v2 == &face.v1+1);
		return (&face.v0)[nIndex];
	}

	static int& FaceVertex (CryFace& face, int nIndex)
	{
		assert (&face.v1 == &face.v0 + 1 && &face.v2 == &face.v1+1);
		return (&face.v0)[nIndex];
	}

	static float VertCoord (const Vert& vert, int nIndex)
	{
		return vert.p[nIndex];
	}
};

struct CryMeshMapProxy: public CryMeshProxy
{
	enum {g_numCoords = 1};
	typedef int Vert;
  
	static int VertCoord (Vert vert, int)
	{
		return vert;
	}
};

// Utility class that
template <class MeshProxy>
class TMeshProxyUtility: public MeshProxy
{
public:
	typedef typename MeshProxy::Vert Vert;
	typedef typename MeshProxy::Face Face;
	typedef typename std::vector<Vert> VertArray;
	typedef typename std::vector<Face> FaceArray;

	// validates if the indices of the cry UVs/verts are not out of range
	static bool ValidateCryMesh (const Face* pFaces, unsigned numFaces, unsigned numVerts)
	{
		for (unsigned nFace = 0; nFace < numFaces; ++nFace)
		{
			const Face& face = pFaces[nFace];
			for (unsigned i = 0; i < 3; ++i)
				if (FaceVertex(face,i) <0 || (unsigned)FaceVertex(face,i) >= numVerts)
					return false;
		}
		return true;
	}

	// retrieves the raw UV coordinates per each tex vertex
	// the output array receives 3 * numFaces texture vertices resolved from the array of tex faces and UVs
	static void GetRawVerts(const Face* pFaces, unsigned numFaces, const Vert*pVerts, VertArray& arrRawVerts)
	{
		arrRawVerts.resize (numFaces * g_numCoords);
		for (unsigned nFace = 0; nFace < numFaces; ++nFace)
		{
			const Face& face = pFaces[nFace];
			for (unsigned i = 0; i < g_numCoords; ++i)
        arrRawVerts[nFace*g_numCoords+i] = pVerts[FaceVertex(face,i)];
		}
	}

	// compares two raw UVs arrays.
	// returns true if they are equal
	static bool EqualRawVerts (const VertArray& arrA, const VertArray& arrB)
	{
		unsigned nSize = arrA.size();
		if (nSize != arrB.size())
			return false;
		for (unsigned i = 0; i < nSize; ++i)
		{
			for (unsigned j = 0; j < g_numCoords; ++j)
				if (VertCoord (arrA[i],j) != VertCoord(arrB[i],j))
					return false;
		}
		return true;
	}


	// deletes unused UVs from the array and compacts it;
	// renumbers the indices in the tex faces
	// returns the number of UVs in the compacted array
	static unsigned CompactCryVerts (Face* pFaces, unsigned numFaces, Vert*pVerts, unsigned numVerts)
	{
		assert (ValidateCryMesh(pFaces, numFaces, numVerts));

#ifdef _DEBUG
		// prepare the raw UVs for assertion at the end of the procedure
		VertArray arrOldRawVerts;
		GetRawVerts(pFaces, numFaces, pVerts, arrOldRawVerts);
#endif

		// UV usage: false if unused, true if used
		std::vector<bool> arrUsage;
		arrUsage.resize (numVerts, false);

		for (unsigned nFace = 0; nFace < numFaces; ++nFace)
		{
			Face& face = pFaces[nFace];
			for (unsigned v = 0; v < g_numFaceVerts; ++v)
				arrUsage[FaceVertex(face,v)] = true;
		}

		std::vector<int> arrOldToNewVerts;
		arrOldToNewVerts.resize (numVerts);
		unsigned numNewVerts = 0;

		for (unsigned nVert = 0; nVert < numVerts; ++nVert)
		{
			if (arrUsage[nVert])
			{
				// the Vert is used
				arrOldToNewVerts[nVert] = numNewVerts;
				pVerts[numNewVerts] = pVerts[nVert];
				++numNewVerts;
			}
#ifdef _DEBUG
			else
				arrOldToNewVerts[nVert] = -1;
#endif
		}

		for (unsigned nTexFace = 0; nTexFace < numFaces; ++nTexFace)
		{
			Face& face = pFaces[nTexFace];
			for (unsigned v = 0; v < g_numFaceVerts; ++v)
				FaceVertex(face,v) = arrOldToNewVerts[FaceVertex(face,v)];
		}

#ifdef _DEBUG
		// prepare the raw UVs for assertion at the end of the procedure
		VertArray arrNewRawVerts;
		GetRawVerts(pFaces, numFaces, pVerts, arrNewRawVerts);
		assert (EqualRawVerts(arrNewRawVerts, arrOldRawVerts));
#endif

		return numNewVerts;
	}
};


// validates if the indices of the cry UVs are not out of range
bool ValidateCryUVs (const CryTexFace* pFaces, unsigned numFaces, unsigned numUVs)
{
	return TMeshProxyUtility<CryTexMeshProxy>::ValidateCryMesh(pFaces, numFaces, numUVs);
}

// validates if the indices of the cry vertices are not out of range
bool ValidateCryVerts (const CryFace* pFaces, unsigned numFaces, unsigned numVerts)
{
	return TMeshProxyUtility<CryMeshProxy>::ValidateCryMesh(pFaces, numFaces, numVerts);
}


// deletes unused UVs from the array and compacts it;
// renumbers the indices in the tex faces
// returns the number of UVs in the compacted array
unsigned CompactCryUVs (CryTexFace* pFaces, unsigned numFaces, CryUV*pUVs, unsigned numUVs)
{
	return TMeshProxyUtility<CryTexMeshProxy>::CompactCryVerts (pFaces, numFaces, pUVs, numUVs);
}

// deletes unused vertices from the array and compacts it;
// renumbers the indices in the faces
// returns the number of verts in the compacted array
unsigned CompactCryVerts (CryFace* pFaces, unsigned numFaces, CryVertex*pVerts, unsigned numVerts)
{
	return TMeshProxyUtility<CryMeshProxy>::CompactCryVerts (pFaces, numFaces, pVerts, numVerts);
}


// construct compact map for the given vertices: 
// also modifies the faces to refer to the new indices.
// THe output array of ints (arrVerts) maps the new vertices to old, unused vertices are not referred to
void CompactCryVertMap (CryFace* pFaces, unsigned numFaces, unsigned numVertices, std::vector<int>& arrVerts)
{
	arrVerts.resize (numVertices);
	for (unsigned i = 0; i < numVertices; ++i)
		arrVerts[i] = i;

	arrVerts.resize (TMeshProxyUtility<CryMeshMapProxy>::CompactCryVerts (pFaces, numFaces, &arrVerts[0], numVertices));
}