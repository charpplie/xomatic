#ifndef aabbtree_h
#define aabbtree_h

struct AABBnode {
	unsigned int ichild;
	unsigned char minx;
	unsigned char maxx;
	unsigned char miny;
	unsigned char maxy;
	unsigned char minz;
	unsigned char maxz;
	unsigned char ntris;
	unsigned char bSingleColl;

	AUTO_STRUCT_INFO
};

struct AABBnodeV0_PS3 {
	unsigned int minx7_maxx7_miny7_maxy7;
	unsigned int minz7_maxz7_ichild15;
	unsigned int ntris6_bSingleColl1;

	AUTO_STRUCT_INFO_LOCAL
};
struct AABBnodeV0_PC {
	unsigned int minx : 7;
	unsigned int maxx : 7;
	unsigned int miny : 7;
	unsigned int maxy : 7;
	unsigned int minz : 7;
	unsigned int maxz : 7;
	unsigned int ichild : 15;
	unsigned int ntris : 6;
	unsigned int bSingleColl : 1;

	AUTO_STRUCT_INFO_LOCAL
};	

class CTriMesh;

class CAABBTree : public CBVTree {
public:
	CAABBTree() {}
	virtual ~CAABBTree() { 
		if (m_pNodes) delete[] m_pNodes; m_pNodes=0; 
		if (m_pTri2Node) delete[] m_pTri2Node; m_pTri2Node=0; 
	}
	virtual int GetType()  { return BVT_AABB; }

	float Build(CGeometry *pMesh);
	virtual void SetGeomConvex();

	void SetParams(int nMinTrisPerNode,int nMaxTrisPerNode, float skipdim, const Matrix33 &Basis);
	float BuildNode(int iNode, int iTriStart,int nTris, Vec3 center,Vec3 size, int nDepth);

	SPU_INDIRECT(RB_Intersect(LLLL))
	virtual int PrepareForIntersectionTest(geometry_under_test *pGTest, CGeometry *pCollider,geometry_under_test *pGTestColl);
	SPU_INDIRECT(RB_Intersect(LL))
	virtual void CleanupAfterIntersectionTest(geometry_under_test *pGTest);
	SPU_INDIRECT(RB_Intersect(LL))
	virtual void GetBBox(box *pbox); 
	SPU_INDIRECT(RB_Intersect(L))
	virtual int MaxPrimsInNode() { return m_nMaxTrisInNode; }
	SPU_INDIRECT(RB_Intersect(LL))
	virtual void GetNodeBV(BV *&pBV, int iNode=0, int iCaller=0);
	SPU_INDIRECT(RB_Intersect(LLL))
	virtual void GetNodeBV(BV *&pBV, const Vec3 &sweepdir,float sweepstep, int iNode=0, int iCaller=0);
	SPU_INDIRECT(RB_Intersect(LLLL))
	virtual void GetNodeBV(const Matrix33 &Rw,const Vec3 &offsw,float scalew, BV *&pBV, int iNode=0, int iCaller=0);
	SPU_INDIRECT(RB_Intersect(LLLLL))
	virtual void GetNodeBV(const Matrix33 &Rw,const Vec3 &offsw,float scalew, BV *&pBV, const Vec3 &sweepdir,float sweepstep, int iNode=0, int iCaller=0) {}
	SPU_INDIRECT(RB_Intersect(LL))
	virtual float SplitPriority(const BV* pBV);
	SPU_INDIRECT(RB_Intersect(LLLLLL))
	virtual void GetNodeChildrenBVs(const Matrix33 &Rw,const Vec3 &offsw,float scalew, const BV *pBV_parent, BV *&pBV_child1,BV *&pBV_child2, int iCaller=0);
	SPU_INDIRECT(RB_Intersect(LLLL))
	virtual void GetNodeChildrenBVs(const BV *pBV_parent, BV *&pBV_child1,BV *&pBV_child2, int iCaller=0);
	SPU_INDIRECT(RB_Intersect(LLLLL))
	virtual void GetNodeChildrenBVs(const BV *pBV_parent, const Vec3 &sweepdir,float sweepstep, BV *&pBV_child1,BV *&pBV_child2, int iCaller=0);
	SPU_INDIRECT(RB_Intersect(L))
	virtual void ReleaseLastBVs(int iCaller);
	SPU_INDIRECT(RB_Intersect(L))
	virtual void ReleaseLastSweptBVs(int iCaller);
	SPU_INDIRECT(RB_Intersect(LLLL))
	virtual int GetNodeContents(int iNode, BV *pBVCollider,int bColliderUsed,int bColliderLocal, 
		geometry_under_test *pGTest,geometry_under_test *pGTestOp);
	SPU_INDIRECT(RB_Intersect(LL))
	virtual int GetNodeContentsIdx(int iNode, int &iStartPrim) { iStartPrim = m_pNodes[iNode].ichild; return m_pNodes[iNode].ntris; }
	SPU_INDIRECT(RB_Intersect(LL))
	virtual void MarkUsedTriangle(int itri, geometry_under_test *pGTest);
	SPU_INDIRECT(RB_Intersect(L))
	virtual float GetMaxSkipDim() { return m_maxSkipDim/max(max(m_size.x,m_size.y),m_size.z); }
	void GetRootNodeDim(Vec3 &center,Vec3 &size)
	{
		Vec3 ptmin,ptmax;
		ptmin.x = m_pNodes[0].minx*m_size.x*(2.0f/128);
		ptmax.x = (m_pNodes[0].maxx+1)*m_size.x*(2.0f/128);
		ptmin.y = m_pNodes[0].miny*m_size.y*(2.0f/128);
		ptmax.y = (m_pNodes[0].maxy+1)*m_size.y*(2.0f/128);
		ptmin.z = m_pNodes[0].minz*m_size.z*(2.0f/128);
		ptmax.z = (m_pNodes[0].maxz+1)*m_size.z*(2.0f/128);
		center = m_center+((ptmax+ptmin)*0.5f-m_size)*m_Basis;
		size = (ptmax-ptmin)*0.5f;
	}

	virtual void GetMemoryStatistics(ICrySizer *pSizer);
	virtual void Save(CMemStream &stm);
	virtual void Load(CMemStream &stm, CGeometry *pGeom);

	CTriMesh *m_pMesh;
	AABBnode *m_pNodes;
	int m_nNodes,m_nNodesAlloc;
	Vec3 m_center;
	Vec3 m_size;
	Matrix33 m_Basis;
	int m_bOriented;
	int *m_pTri2Node,m_nBitsLog;
	int m_nMaxTrisPerNode,m_nMinTrisPerNode;
	int m_nMaxTrisInNode;
	float m_maxSkipDim;
};

#endif
