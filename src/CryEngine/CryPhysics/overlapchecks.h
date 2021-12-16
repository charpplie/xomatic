#ifndef overlapchecks_h
#define overlapchecks_h

#define OVERLAP_PROC SPU_INDIRECT(RB_Intersect(LLL)) SPU_INDIRECT_TAG(overlap_check)
#define OVERLAP_PROC_ROPE_RWI SPU_INDIRECT(Rope(LLL),RWI(LLL)) SPU_INDIRECT_TAG(overlap_check)
typedef int (*overlap_check)(const primitive*,const primitive*,class COverlapChecker*) SPU_INDIRECT_TAG(overlap_check);

OVERLAP_PROC int default_overlap_check(const primitive*, const primitive*, class COverlapChecker*);
OVERLAP_PROC int box_box_overlap_check(const box *box1, const box *box2, class COverlapChecker*);
OVERLAP_PROC int box_heightfield_overlap_check(const box *pbox, const heightfield *phf, class COverlapChecker *pOverlapper=0);
OVERLAP_PROC int heightfield_box_overlap_check(const heightfield *phf, const box *pbox, class COverlapChecker *pOverlapper=0);
						 int box_voxgrid_overlap_check(const box *pbox, const voxelgrid *phf, class COverlapChecker *pOverlapper=0);
						 int voxgrid_box_overlap_check(const voxelgrid *phf, const box *pbox, class COverlapChecker *pOverlapper=0);
OVERLAP_PROC int box_tri_overlap_check(const box *pbox, const triangle *ptri, class COverlapChecker *pOverlapper=0);
OVERLAP_PROC int tri_box_overlap_check(const triangle *ptri, const box *pbox, class COverlapChecker *pOverlapper=0);
OVERLAP_PROC_ROPE_RWI int box_ray_overlap_check(const box *pbox, const ray *pray, class COverlapChecker *pOverlapper=SPU_LOCAL_PTR((COverlapChecker*)0));
						 int ray_box_overlap_check(const ray *pray, const box *pbox, class COverlapChecker *pOverlapper=0);
						 int box_sphere_overlap_check(const box *pbox, const sphere *psph, class COverlapChecker *pOverlapper=0);
						 int sphere_box_overlap_check(const sphere *psph, const box *pbox, class COverlapChecker *pOverlapper=0);
						 int tri_sphere_overlap_check(const triangle *ptri, const sphere *psph, class COverlapChecker *pOverlapper=0);
						 int sphere_tri_overlap_check(const sphere *psph, const triangle *ptri, class COverlapChecker *pOverlapper=0);
						 int heightfield_sphere_overlap_check(const heightfield *phf, const sphere *psph, class COverlapChecker *pOverlapper=0);
						 int sphere_heightfield_overlap_check(const sphere *psph, const heightfield *phf, class COverlapChecker *pOverlapper=0);
						 int sphere_sphere_overlap_check(const sphere *psph1, const sphere *psph2, class COverlapChecker *pOverlapper=0);

quotientf tri_sphere_dist2(const triangle *ptri, const sphere *psph, int &bFace);

class COverlapChecker {
public:

#if !defined(__SPU__)
  // ChrisR: causes run-time relocation because of required static initializing code in global g_idata struct
	COverlapChecker() { Init(); }
#endif 

	void Init() { iPrevCode = -1; }
	ILINE int Check(int type1,int type2, primitive *prim1,primitive *prim2) {
#if defined(ENABLE_SPU_FUNCTABLE_DEBUG) && defined(__SPU__)
		if (! ((type1 >=0 && type1<NPRIMS) && (type2 >=0 && type2<NPRIMS)) )
		{
			printf("COverlapChecker::table[%d][%d] index out of bounds \n", type1, type2);
			snPause();
		}
		overlap_check func = table[type1][type2];
		if (func == (overlap_check)SPU_FUNCTABLE_DEBUG_TAG) {
			char token[64]; const char* func_name = name_table[type1][type2];
			memcpy(token, func_name, min(strlen(func_name),sizeof(token))); token[sizeof(token)-1] = '\0';
			printf("COverlapChecker::table[%d][%d] (%s): Function not mapped to spu!\n", type1, type2, token);
			snPause();
		}
		return func(prim1,prim2,this);
#else
		return table[type1][type2](prim1,prim2,this);
#endif
	}
	ILINE int CheckExists(int type1,int type2) {
		return table[type1][type2]!=default_overlap_check;
	}

	/*class COverlapCheckerInit {
	public:
		COverlapCheckerInit();
	};*/
	static overlap_check table[NPRIMS][NPRIMS];
#if defined(ENABLE_SPU_FUNCTABLE_DEBUG)
	static const char* name_table[NPRIMS][NPRIMS];
#endif

	//static COverlapCheckerInit init;

	int iPrevCode;
	Matrix33 Basis21;
	Matrix33 Basis21abs;
};
//extern COverlapChecker g_Overlapper;

#endif
