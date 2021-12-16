#ifndef __STATICOBJECTCOMPILER_H__
#define __STATICOBJECTCOMPILER_H__

class CPhysicsInterface;
class CContentCGF;
class CMesh;
struct CMaterialCGF;

#define MAX_NUM_LODS 6

class CStaticObjectCompiler
{
public:
	CStaticObjectCompiler(CPhysicsInterface *pPhysicsInterface);
	~CStaticObjectCompiler();

	void SetSplitLods( bool bSplit ) { m_bSplitLODs = bSplit; }

	CContentCGF* MakeCompiledCGF( CContentCGF *pCGF );

public:
	CContentCGF* m_pLODs[MAX_NUM_LODS];

private:
//	const bool CalculateSHCoefficientsForMesh(CMesh *pMesh, CContentCGF *pCGF, const CMaterialCGF* cpMat);
	CContentCGF* ProcessCompiledCGF( CContentCGF *pCGF );

	void AnalyzeSharedMeshes( CContentCGF *pCGF );
	bool CompileMeshes( CContentCGF *pCGF );

	void SplitLODs( CContentCGF *pCGF );
	CContentCGF* MakeLOD( int nLodNum,CContentCGF *pCGF );

	bool Physicalize(CContentCGF *pCompilerCGF, CContentCGF *pSrcCGF);
	void AnalyzeFoliage(CContentCGF *pCGF, struct CNodeCGF *pNodeCGF);
	void PrepareSkinData(CNodeCGF *pNode, const Matrix34 &mtxSkelToMesh, CNodeCGF *pNodeSkel, float r, bool bSwapEndian);
	bool ValidateBoundingBoxes( CContentCGF *pCGF );

	bool MakeMergedCGF( CContentCGF *pCompiledCGF,CContentCGF *pCGF );

private:
	CPhysicsInterface *m_pPhysicsInterface;
	bool m_bSplitLODs;

};

#endif //__STATICOBJECTCOMPILER_H__
