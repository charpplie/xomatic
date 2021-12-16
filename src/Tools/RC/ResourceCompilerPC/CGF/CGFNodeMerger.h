#ifndef __CGFNODEMERGER_H__
#define __CGFNODEMERGER_H__

class CContentCGF;
class CMesh;
struct CMaterialCGF;
struct CNodeCGF;

namespace CGFNodeMerger
{
	bool SetupMeshSubsets(CContentCGF* pCGF, CMesh &mesh,CMaterialCGF *pMaterialCGF);
	bool MergeNodes(CContentCGF* pCGF, std::vector<CNodeCGF*> nodes, string& errorMessage,CMesh *pOutMesh);
};

#endif //__CGFNODEMERGER_H__
