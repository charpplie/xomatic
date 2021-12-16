struct CryTexFace;
struct CryUV;

// validates if the indices of the cry UVs are not out of range
extern bool ValidateCryUVs (const CryTexFace* pFaces, unsigned numFaces, unsigned numUVs);

// deletes unused UVs from the array and compacts it;
// renumbers the indices in the tex faces
// returns the number of UVs in the compacted array
extern unsigned CompactCryUVs (CryTexFace* pFaces, unsigned numFaces, CryUV*pUVs, unsigned numUVs);

// validates if the indices of the cry vertices are not out of range
extern bool ValidateCryVerts (const CryFace* pFaces, unsigned numFaces, unsigned numVerts);

// deletes unused vertices from the array and compacts it;
// renumbers the indices in the faces
// returns the number of verts in the compacted array
extern unsigned CompactCryVerts (CryFace* pFaces, unsigned numFaces, CryVertex*pVerts, unsigned numVerts);

// construct compact map for the given vertices: 
// also modifies the faces to refer to the new indices.
// THe output array of ints (arrVerts) maps the new vertices to old, unused vertices are not referred to
extern void CompactCryVertMap (CryFace* pFaces, unsigned numFaces, unsigned numVertices, std::vector<int>& arrVerts);
