#ifndef singleboxtree_h
#define singleboxtree_h

class CSingleBoxTree : public CBVTree {
public:
	CSingleBoxTree() { m_nPrims=1; m_bIsConvex=0; }
	SPU_INDIRECT(RB_Intersect(L))
	virtual int GetType() { return BVT_SINGLEBOX; }
	virtual float Build(CGeometry *pGeom);
	virtual void SetGeomConvex() { m_bIsConvex = 1; }
	void SetBox(box *pbox);
	SPU_INDIRECT(RB_Intersect(LLLL))
	virtual int PrepareForIntersectionTest(geometry_under_test *pGTest, CGeometry *pCollider,geometry_under_test *pGTestColl);
	SPU_INDIRECT(RB_Intersect(LL))
	virtual void GetBBox(box *pbox);
	SPU_INDIRECT(RB_Intersect(L))
	virtual int MaxPrimsInNode() { return m_nPrims; }
	SPU_INDIRECT(RB_Intersect(LL),Rope(LL))
	virtual void GetNodeBV(BV *&pBV, int iNode=0, int iCaller=0);
	SPU_INDIRECT(RB_Intersect(LLL),Rope(LLL))
	virtual void GetNodeBV(BV *&pBV, const Vec3 &sweepdir,float sweepstep, int iNode=0, int iCaller=0);
	SPU_INDIRECT(RB_Intersect(LLLL),Rope(LLLL))
	virtual void GetNodeBV(const Matrix33 &Rw,const Vec3 &offsw,float scalew, BV *&pBV, int iNode=0, int iCaller=0);
	SPU_INDIRECT(RB_Intersect(LLLLL),Rope(LLLLL))
	virtual void GetNodeBV(const Matrix33 &Rw,const Vec3 &offsw,float scalew, BV *&pBV, const Vec3 &sweepdir,float sweepstep, int iNode=0, int iCaller=0);
	SPU_INDIRECT(RB_Intersect(LLLL))
	virtual int GetNodeContents(int iNode, BV *pBVCollider,int bColliderUsed,int bColliderLocal, 
		geometry_under_test *pGTest,geometry_under_test *pGTestOp);
	SPU_INDIRECT(RB_Intersect(LL))
	virtual int GetNodeContentsIdx(int iNode, int &iStartPrim) { iStartPrim=0; return m_nPrims; }
	SPU_INDIRECT(RB_Intersect(LL))
	virtual void MarkUsedTriangle(int itri, geometry_under_test *pGTest);

	virtual void GetMemoryStatistics(ICrySizer *pSizer);
	virtual void Save(CMemStream &stm);
	virtual void Load(CMemStream &stm, CGeometry *pGeom);

	CGeometry *m_pGeom;
	box m_Box;
	int m_nPrims;
	int m_bIsConvex;
};

#endif