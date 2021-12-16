#ifndef intersectionchecks_h
#define intersectionchecks_h

#define INTERS_PROC SPU_INDIRECT(RB_Intersect(LLL)) SPU_INDIRECT_TAG(intersection_check)
typedef int (*intersection_check)(const primitive*,const primitive*,prim_inters*) SPU_INDIRECT_TAG(intersection_check);

#if !defined(__SPU__)
INTERS_PROC int default_intersection(const primitive*,const primitive*, prim_inters *pinters);
#else
#define default_intersection NULL
#endif 

INTERS_PROC int tri_tri_intersection(const triangle *ptri1, const triangle *ptri2, prim_inters *pinters);
INTERS_PROC int tri_box_intersection(const triangle *ptri, const box *pbox, prim_inters *pinters);
INTERS_PROC int box_tri_intersection(const box *pbox, const triangle *ptri, prim_inters *pinters);
INTERS_PROC int tri_cylinder_intersection(const triangle *ptri, const cylinder *pcyl, prim_inters *pinters);
INTERS_PROC int cylinder_tri_intersection(const cylinder *pcyl, const triangle *ptri, prim_inters *pinters);
INTERS_PROC int tri_sphere_intersection(const triangle *ptri, const sphere *psphere, prim_inters *pinters);
INTERS_PROC int sphere_tri_intersection(const sphere *psphere, const triangle *ptri, prim_inters *pinters);
INTERS_PROC int tri_capsule_intersection(const triangle *ptri, const capsule *pcaps, prim_inters *pinters);
INTERS_PROC int capsule_tri_intersection(const capsule *pcyl, const triangle *ptri, prim_inters *pinters);
						int tri_ray_intersection(const triangle *ptri, const ray *pray, prim_inters *pinters);
SPU_INDIRECT(RWI(LLL)) int ray_tri_intersection(const ray *pray, const triangle *ptri, prim_inters *pinters);
						int tri_plane_intersection(const triangle *ptri, const plane *pplane, prim_inters *pinters);
						int plane_tri_intersection(const plane *pplane, const triangle *ptri, prim_inters *pinters);
						int box_box_intersection(const box *pbox1, const box *pbox2, prim_inters *pinters);
						int box_cylinder_intersection(const box *pbox, const cylinder *pcyl, prim_inters *pinters);
						int cylinder_box_intersection(const cylinder *pcyl, const box *pbox, prim_inters *pinters);
						int box_sphere_intersection(const box *pbox, const sphere *psphere, prim_inters *pinters);
						int sphere_box_intersection(const sphere *psphere, const box *pbox, prim_inters *pinters);
						int box_capsule_intersection(const box *pbox, const capsule *pcapsule, prim_inters *pinters);
						int capsule_box_intersection(const capsule *pcaps, const box *pbox, prim_inters *pinters);
SPU_INDIRECT(RWI(LLL)) int box_ray_intersection(const box *pbox, const ray *pray, prim_inters *pinters);
						int ray_box_intersection(const ray *pray, const box *pbox, prim_inters *pinters);
						int box_plane_intersection(const box *pbox, const plane *pplane, prim_inters *pinters);
						int plane_box_intersection(const plane *pplane, const box *pbox, prim_inters *pinters);
						int cylinder_cylinder_intersection(const cylinder *pcyl1, const cylinder *pcyl2, prim_inters *pinters);
						int cylinder_sphere_intersection(const cylinder *pcyl, const sphere *psphere, prim_inters *pinters);
						int sphere_cylinder_intersection(const sphere *psphere, const cylinder *pcyl, prim_inters *pinters);
						int cylinder_capsule_intersection(const cylinder *pcyl, const capsule *pcaps, prim_inters *pinters);
						int capsule_cylinder_intersection(const capsule *pcaps, const cylinder *pcyl, prim_inters *pinters);
						int cylinder_ray_intersection(const cylinder *pcyl, const ray *pray, prim_inters *pinters);
						int ray_cylinder_intersection(const ray *pray, const cylinder *pcyl, prim_inters *pinters);
						int cylinder_plane_intersection(const cylinder *pcyl, const plane *pplane, prim_inters *pinters);
						int plane_cylinder_intersection(const plane *pplane, const cylinder *pcyl, prim_inters *pinters);
						int sphere_sphere_intersection(const sphere *psphere1, const sphere *psphere2, prim_inters *pinters);
						int sphere_ray_intersection(const sphere *psphere, const ray *pray, prim_inters *pinters);
						int ray_sphere_intersection(const ray *pray, const sphere *psphere, prim_inters *pinters);
						int capsule_capsule_intersection(const capsule *pcaps1, const capsule *pcaps2, prim_inters *pinters);
						int capsule_ray_intersection(const capsule *pcaps, const ray *pray, prim_inters *pinters);
						int ray_capsule_intersection(const ray *pray, const capsule *pcaps, prim_inters *pinters);
						int sphere_plane_intersection(const sphere *psphere, const plane *pplane, prim_inters *pinters);
						int plane_sphere_intersection(const plane *pplane, const sphere *psphere, prim_inters *pinters);
						int ray_plane_intersection(const ray *pray, const plane *pplane, prim_inters *pinters);
						int plane_ray_intersection(const plane *pplane, const ray *pray, prim_inters *pinters);
						int capsule_sphere_intersection(const cylinder *pcaps, const sphere *psphere, prim_inters *pinters);
						int sphere_capsule_intersection(const sphere *psphere, const capsule *pcaps, prim_inters *pinters);


class CIntersectionChecker {
public:
	//CIntersectionChecker();
	ILINE int Check(int type1,int type2, const primitive* prim1,const primitive *prim2, prim_inters* pinters) {
#if defined(ENABLE_SPU_FUNCTABLE_DEBUG) && defined(__SPU__)
		if (! ((type1 >=0 && type1<NPRIMS) && (type2 >=0 && type2<NPRIMS)) )
		{
			printf("CIntersectionChecker::table[%d][%d] index out of bounds \n", type1, type2);
			snPause();
		}
		intersection_check func = table[type1][type2];
		if (func == (intersection_check)SPU_FUNCTABLE_DEBUG_TAG) {
			char token[64]; const char* func_name = name_table[type1][type2];
			memcpy(token, func_name, min(strlen(func_name),sizeof(token))); token[sizeof(token)-1] = '\0';
			printf("CIntersectionChecker::table[%d][%d] (%s): Function not mapped to spu!\n", type1, type2, token);
			snPause();
		}
		return func(prim1,prim2, pinters);
#else
		return table[type1][type2](prim1,prim2, pinters);
#endif
	}
	ILINE int CheckExists(int type1,int type2) {
		return table[type1][type2]!=default_intersection;
	}
	static intersection_check table[NPRIMS][NPRIMS];
#if defined(ENABLE_SPU_FUNCTABLE_DEBUG)
	static const char* name_table[NPRIMS][NPRIMS];
#endif
};
extern CIntersectionChecker g_Intersector;

#endif
