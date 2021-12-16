#include "StdAfx.h"

#include "utils.h"
#include "primitives.h"
#include "overlapchecks.h"
#include "intersectionchecks.h"
#include "unprojectionchecks.h"
#include "bvtree.h"
#include "geometry.h"
#include "singleboxtree.h"
#include "aabbtree.h"
#include "obbtree.h"
#include "trimesh.h"
#include "heightfieldbv.h"
#include "heightfieldgeom.h"
#include "voxelbv.h"
#include "voxelgeom.h"
#include "physicalplaceholder.h"
#include "rigidbody.h"
#include "physicalentity.h"
#include "softentity.h"
#include "ropeentity.h"
#include "geoman.h"
#include "physicalworld.h"

SPU_LOCAL SPU_DOMAIN_LOCAL CHeightfield *g_pHFbuf[MAX_PHYS_THREADS+1];
#define g_pHF g_pHFbuf[iCaller]

//#define SUPP_DYNAMIC_JOB_DATA_NUM
#define MAX_JOB_DATA_COUNT 4

#ifdef __SPU__
#include <cell/dma.h>
#include <SPU/SPU.h>
#endif

uint32 g_JobAllocSize = 0;

#define MAX_JOB_SIZE_INTERSECT (48*1024)
#define MAX_JOB_SIZE_SOLVER (210*1024)
#define MAX_JOB_SIZE_CLOTH (80*1024)
#define MAX_JOB_SIZE_ROPE (70*1024)

//need a cache bypassing version of writing to a volatile int ptr
ILINE void SignalOutput(volatile int* pWritten, const int val)
{
#ifdef __SPU__
	struct SIntVal
	{
		int valBuf[4];
		//set the value into the slot matching last 4 bits of EA, return LS to dma from
		ILINE const uint32 SetValue(const uint32 ea, const int val)
		{
			const uint32 slot = (ea & 15) / 4;
			valBuf[slot] = val;
			return (uint32)&valBuf[slot];
		}
	} _ALIGN(16);

	SIntVal intVal;	
	const uint32 eaAddr		= (uint32)pWritten;
	const uint32 lsAddr		= intVal.SetValue((uint32)pWritten, val);
	si_wrch(MFC_TagID, si_from_uint(MEM_TRANSFER_DMA_TAG_BASE));
	si_wrch(MFC_LSA, si_from_uint(lsAddr));
	si_wrch(MFC_EAL, si_from_uint(eaAddr));
	si_wrch(MFC_Size, si_from_uint(4));
	si_wrch(MFC_Cmd, si_from_uint(MFC_CMD_WORD(0,0,MFC_PUT_CMD))) ;
	memtransfer_sync(0);
#else
	*pWritten = val;
#endif
}

char *g_jobName[4] = { "RB_Intersect", "RB_Solver", "Cloth", "Rope" };

float getHFPatchHeight(int ix,int iy)	{
	int iCaller = get_iCaller();
	const CHeightfieldBV& hbv = g_pHF->m_Tree;
	const vector2di& patchStart = hbv.m_PatchStart;
	return g_pHF->m_pVertices[ix-patchStart.x+(hbv.m_PatchSize.x+1)*(iy-patchStart.y)].z;
}

unsigned char getHFPatchSurfType(int ix,int iy)	{
	int iCaller = get_iCaller();
	const CHeightfieldBV& hbv = g_pHF->m_Tree;
	const vector2di& patchStart = hbv.m_PatchStart;
	return g_pHF->m_pIds[ix-patchStart.x+(hbv.m_PatchSize.x+1)*(iy-patchStart.y)];
}

#if defined(PS3)
#ifndef __SPU__
	extern uint32 g_ForceStopSPUs;
//	#define SUPPORT_DEBUG_SPU
#endif
#include <IJobManSPU.h>

SPU_LOCAL float g_sintab[SINCOSTABSZ+1]=
{
	0.000000f, 0.003068f, 0.006136f, 0.009204f,0.012272f, 0.015339f, 0.018407f, 0.021474f,
	0.024541f, 0.027608f, 0.030675f, 0.033741f,0.036807f, 0.039873f, 0.042938f, 0.046003f,
	0.049068f, 0.052132f, 0.055195f, 0.058258f,0.061321f, 0.064383f, 0.067444f, 0.070505f,
	0.073565f, 0.076624f, 0.079682f, 0.082740f,0.085797f, 0.088854f, 0.091909f, 0.094963f,
	0.098017f, 0.101070f, 0.104122f, 0.107172f,0.110222f, 0.113271f, 0.116319f, 0.119365f,
	0.122411f, 0.125455f, 0.128498f, 0.131540f,0.134581f, 0.137620f, 0.140658f, 0.143695f,
	0.146730f, 0.149765f, 0.152797f, 0.155828f,0.158858f, 0.161886f, 0.164913f, 0.167938f,
	0.170962f, 0.173984f, 0.177004f, 0.180023f,0.183040f, 0.186055f, 0.189069f, 0.192080f,
	0.195090f, 0.198098f, 0.201105f, 0.204109f,0.207111f, 0.210112f, 0.213110f, 0.216107f,
	0.219101f, 0.222094f, 0.225084f, 0.228072f,0.231058f, 0.234042f, 0.237024f, 0.240003f,
	0.242980f, 0.245955f, 0.248928f, 0.251898f,0.254866f, 0.257831f, 0.260794f, 0.263755f,
	0.266713f, 0.269668f, 0.272621f, 0.275572f,0.278520f, 0.281465f, 0.284408f, 0.287347f,
	0.290285f, 0.293219f, 0.296151f, 0.299080f,0.302006f, 0.304929f, 0.307850f, 0.310767f,
	0.313682f, 0.316593f, 0.319502f, 0.322408f,0.325310f, 0.328210f, 0.331106f, 0.334000f,
	0.336890f, 0.339777f, 0.342661f, 0.345541f,0.348419f, 0.351293f, 0.354164f, 0.357031f,
	0.359895f, 0.362756f, 0.365613f, 0.368467f,0.371317f, 0.374164f, 0.377007f, 0.379847f,
	0.382683f, 0.385516f, 0.388345f, 0.391170f,0.393992f, 0.396810f, 0.399624f, 0.402435f,
	0.405241f, 0.408044f, 0.410843f, 0.413638f,0.416430f, 0.419217f, 0.422000f, 0.424780f,
	0.427555f, 0.430327f, 0.433094f, 0.435857f,0.438616f, 0.441371f, 0.444122f, 0.446869f,
	0.449611f, 0.452350f, 0.455084f, 0.457813f,0.460539f, 0.463260f, 0.465977f, 0.468689f,
	0.471397f, 0.474100f, 0.476799f, 0.479494f,0.482184f, 0.484869f, 0.487550f, 0.490227f,
	0.492898f, 0.495565f, 0.498228f, 0.500885f,0.503538f, 0.506187f, 0.508830f, 0.511469f,
	0.514103f, 0.516732f, 0.519356f, 0.521975f,0.524590f, 0.527199f, 0.529804f, 0.532403f,
	0.534998f, 0.537587f, 0.540172f, 0.542751f,0.545325f, 0.547894f, 0.550458f, 0.553017f,
	0.555570f, 0.558119f, 0.560662f, 0.563199f,0.565732f, 0.568259f, 0.570781f, 0.573297f,
	0.575808f, 0.578314f, 0.580814f, 0.583309f,0.585798f, 0.588282f, 0.590760f, 0.593232f,
	0.595699f, 0.598161f, 0.600617f, 0.603067f,0.605511f, 0.607950f, 0.610383f, 0.612810f,
	0.615232f, 0.617647f, 0.620057f, 0.622461f,0.624860f, 0.627252f, 0.629638f, 0.632019f,
	0.634393f, 0.636762f, 0.639124f, 0.641481f,0.643832f, 0.646176f, 0.648514f, 0.650847f,
	0.653173f, 0.655493f, 0.657807f, 0.660114f,0.662416f, 0.664711f, 0.667000f, 0.669283f,
	0.671559f, 0.673829f, 0.676093f, 0.678350f,0.680601f, 0.682846f, 0.685084f, 0.687315f,
	0.689541f, 0.691759f, 0.693971f, 0.696177f,0.698376f, 0.700569f, 0.702755f, 0.704934f,
	0.707107f, 0.709273f, 0.711432f, 0.713585f,0.715731f, 0.717870f, 0.720002f, 0.722128f,
	0.724247f, 0.726359f, 0.728464f, 0.730563f,0.732654f, 0.734739f, 0.736817f, 0.738887f,
	0.740951f, 0.743008f, 0.745058f, 0.747101f,0.749136f, 0.751165f, 0.753187f, 0.755201f,
	0.757209f, 0.759209f, 0.761202f, 0.763188f,0.765167f, 0.767139f, 0.769103f, 0.771061f,
	0.773010f, 0.774953f, 0.776888f, 0.778816f,0.780737f, 0.782651f, 0.784557f, 0.786455f,
	0.788346f, 0.790230f, 0.792107f, 0.793975f,0.795837f, 0.797691f, 0.799537f, 0.801376f,
	0.803208f, 0.805031f, 0.806848f, 0.808656f,0.810457f, 0.812251f, 0.814036f, 0.815814f,
	0.817585f, 0.819348f, 0.821102f, 0.822850f,0.824589f, 0.826321f, 0.828045f, 0.829761f,
	0.831470f, 0.833170f, 0.834863f, 0.836548f,0.838225f, 0.839894f, 0.841555f, 0.843208f,
	0.844853f, 0.846491f, 0.848120f, 0.849742f,0.851355f, 0.852961f, 0.854558f, 0.856147f,
	0.857729f, 0.859302f, 0.860867f, 0.862424f,0.863973f, 0.865514f, 0.867046f, 0.868571f,
	0.870087f, 0.871595f, 0.873095f, 0.874587f,0.876070f, 0.877545f, 0.879012f, 0.880471f,
	0.881921f, 0.883363f, 0.884797f, 0.886222f,0.887640f, 0.889048f, 0.890449f, 0.891841f,
	0.893224f, 0.894599f, 0.895966f, 0.897325f,0.898674f, 0.900016f, 0.901349f, 0.902673f,
	0.903989f, 0.905297f, 0.906596f, 0.907886f,0.909168f, 0.910441f, 0.911706f, 0.912962f,
	0.914210f, 0.915449f, 0.916679f, 0.917901f,0.919114f, 0.920318f, 0.921514f, 0.922701f,
	0.923880f, 0.925049f, 0.926210f, 0.927363f,0.928506f, 0.929641f, 0.930767f, 0.931884f,
	0.932993f, 0.934093f, 0.935184f, 0.936266f,0.937339f, 0.938403f, 0.939459f, 0.940506f,
	0.941544f, 0.942573f, 0.943593f, 0.944605f,0.945607f, 0.946601f, 0.947586f, 0.948561f,
	0.949528f, 0.950486f, 0.951435f, 0.952375f,0.953306f, 0.954228f, 0.955141f, 0.956045f,
	0.956940f, 0.957826f, 0.958703f, 0.959572f,0.960431f, 0.961280f, 0.962121f, 0.962953f,
	0.963776f, 0.964590f, 0.965394f, 0.966190f,0.966976f, 0.967754f, 0.968522f, 0.969281f,
	0.970031f, 0.970772f, 0.971504f, 0.972227f,0.972940f, 0.973644f, 0.974339f, 0.975025f,
	0.975702f, 0.976370f, 0.977028f, 0.977677f,0.978317f, 0.978948f, 0.979570f, 0.980182f,
	0.980785f, 0.981379f, 0.981964f, 0.982539f,0.983105f, 0.983662f, 0.984210f, 0.984748f,
	0.985278f, 0.985797f, 0.986308f, 0.986809f,0.987301f, 0.987784f, 0.988258f, 0.988722f,
	0.989176f, 0.989622f, 0.990058f, 0.990485f,0.990903f, 0.991311f, 0.991710f, 0.992099f,
	0.992480f, 0.992850f, 0.993212f, 0.993564f,0.993907f, 0.994240f, 0.994565f, 0.994879f,
	0.995185f, 0.995481f, 0.995767f, 0.996045f,0.996313f, 0.996571f, 0.996820f, 0.997060f,
	0.997290f, 0.997511f, 0.997723f, 0.997925f,0.998118f, 0.998302f, 0.998475f, 0.998640f,
	0.998795f, 0.998941f, 0.999078f, 0.999205f,0.999322f, 0.999431f, 0.999529f, 0.999619f,
	0.999699f, 0.999769f, 0.999831f, 0.999882f,0.999925f, 0.999958f, 0.999981f, 0.999995f,
	1.000000f
};
#else//PS3
	#define sys_timer_subusleep_inldb16cyc(ms)
#endif

#ifdef __SPU__
SPU_LOCAL SPU_DOMAIN_LOCAL geom_contact *g_Contacts;
SPU_LOCAL int g_nTotContacts,g_maxContacts,g_BrdPtBufPos,g_nAreas,g_nAreaPt;
#endif

#ifdef USE_PHYS_JOBS

struct used_geom {
	used_geom() {}
	used_geom(CGeometry *_pgeom,int _ioffs) { pgeom=_pgeom; ioffs=_ioffs; }
	CGeometry *pgeom;
	int ioffs;
};

#if defined(PS3) 
#if defined(PHYS_JOB_SIMULATION)
struct CryEventSimple {
	CryEventSimple() { set=0; }
	void Set() { AtomicAdd(&set,1); };
	int Wait(int delay=0) { while(!set) Sleep(0); AtomicAdd(&set,-set); return 1; }
	volatile int set;
};
#else
struct CryEventSimple {
	void Set() {};
	int Wait(int delay=0) { return 1; }
};
#endif
#else
#define CryEventSimple CryEvent
#endif

CryEventSimple g_someJobDone;

CGeometry *LoadGeomFromMem(CMemStream &stm);

#ifndef __SPU__
struct SJobData {
	int bWorking;
	volatile int written;
	int read;
	int szin,szin0;
	int szinMax[4];
	char *bufin;
	int batchStart;
	int szout;
	char *bufout;
	void *pent0last;
	CMemStream stmAux;
	used_geom *pUsedGeoms;
	int nUsedGeoms,nUsedGeomsAlloc;
	int ipass;
#if defined(PHYS_JOB_SIMULATION) && !defined(PS3)
	CryEvent start;
	volatile int bStop;
#endif
	SJobData() : stmAux(false) {}
//	template<class dtype> int queue_data(const dtype& data) { *(dtype*)(bufin+szin)=data; return szin+=sizeof(dtype); }
//	void queue_data(char *pdata, int sz) {	memcpy(bufin+szin, pdata, sz); bufin+=sz; }
	int has_geom(CGeometry *pGeom) { 
		for(int i=nUsedGeoms-1;i>=0;i--) if (pUsedGeoms[i].pgeom==pGeom)
			return pUsedGeoms[i].ioffs;
		return 0;
	}
} _ALIGN(128);

#ifdef SUPP_DYNAMIC_JOB_DATA_NUM
SJobData *g_job=0;
#else
static SJobData g_job[MAX_JOB_DATA_COUNT];
#endif
int g_ijob,g_nJobs=0;
int g_maxJobSize[4];

int ReportJobContacts(char *bufout);
CBVTree *LoadBVTreeFromMem(CMemStream &stm, CTriMesh *pMesh);

void SaveBVTreeToMem(CBVTree *pTree, CMemStream &stm);
void SaveGeomToMem(CGeometry *pGeom, CMemStream &stm, int bForRays);
int WaitForJobFinish(int&,int);
CMemStream &GetJobAuxStream() { return g_job[g_ijob].stmAux; }
int ReadDelayedSolverResults(CMemStream &stm, entity_contact **&pContactsOut,RigidBody **&pBodiesOut);

int GeomOffsInJobBuf(CGeometry *pGeom, int ijob) { return g_job[ijob].has_geom(pGeom); }
void SaveGeomToJobBuf(CMemStream &stm, CGeometry *pGeom, int ioffs, int ijob, int bForRays=0)
{
	stm.Write(ioffs);
	if (!ioffs)	{
		if (g_job[ijob].nUsedGeoms==g_job[ijob].nUsedGeomsAlloc)
			ReallocateList(g_job[ijob].pUsedGeoms, g_job[ijob].nUsedGeoms, g_job[ijob].nUsedGeomsAlloc+=1024);
		g_job[ijob].pUsedGeoms[g_job[ijob].nUsedGeoms++] = used_geom(pGeom,g_job[ijob].szin+stm.m_iPos);
		SaveGeomToMem(pGeom, stm, bForRays);
	} 
}


int CGeometry::IntersectQueued(IGeometry *piCollider, geom_world_data *pdata1,geom_world_data *pdata2, 
															 intersection_params *pip, geom_contact *&pcontacts, void *pent0,void *pent1,int ipart0,int ipart1)
{
	bool bQueue = true;
#ifndef PHYS_JOB_SIMULATION
	bQueue = InvokeJobOnSPU("RB_Intersect");
#endif
	if (!bQueue || !g_nJobs || pip->bSweepTest || max(pdata1->v.len2(),pdata2->v.len2())>sqr(3.0f))
		return Intersect(piCollider, pdata1,pdata2, pip, pcontacts);

	CGeometry *pCollider = (CGeometry*)piCollider;
	ReadLock lock0(pCollider->m_lockUpdate);
  ReadLock lock1(this->m_lockUpdate); 

	int i,ihf,imesh,ijob,ioffs0,ioffs1,size,sizehdr,size0,size1;
	int itype[2] = { GetType(), pCollider->GetType() };
	bool bKeepPrevContacts = pip->bKeepPrevContacts;
	if (iszero(itype[0]-GEOM_VOXELGRID)|iszero(itype[1]-GEOM_VOXELGRID))
		return Intersect(piCollider, pdata1,pdata2, pip, pcontacts);
	if (ihf = iszero(itype[0]-GEOM_HEIGHTFIELD) | iszero(itype[1]-GEOM_HEIGHTFIELD)*2) {
		geometry_under_test gtest[2];
		geom_world_data *pdata[2] = { pdata1,pdata2 };
		for(i=0;i<2;i++) {
			gtest[i].offset = pdata[i]->offset;
			gtest[i].R = pdata[i]->R;
			gtest[i].scale = pdata[i]->scale;
			gtest[i].rscale = 1.0f/pdata[i]->scale;
			gtest[i].v = pdata[i]->v;
		}
		for(i=0;i<2;i++) {
			gtest[i].offset_rel = ((gtest[i].offset-gtest[i^1].offset)*gtest[i^1].R)*gtest[i^1].rscale;
			(gtest[i].R_rel = gtest[i^1].R.T()) *= gtest[i].R;
			gtest[i].scale_rel = gtest[i].scale*gtest[i^1].rscale;
			gtest[i].rscale_rel = gtest[i^1].scale*gtest[i].rscale;
			gtest[i].sweepstep = 0;
			gtest[i].bStopIntersection = 1;
		}
		if (pip->bSweepTest) {
			gtest[0].sweepstep = pcontacts[0].vel = gtest[0].v.len();
			gtest[0].sweepdir = gtest[0].v/gtest[0].sweepstep;
			gtest[0].sweepstep *= pip->time_interval;
			gtest[0].sweepdir_loc = gtest[0].sweepdir*gtest[0].R;
			gtest[0].sweepstep_loc = gtest[0].sweepstep*gtest[0].rscale;
		}	
		if (ihf & 1 && !PrepareForIntersectionTest(gtest+0, pCollider,gtest+1, true))
			return 0;
		if (ihf & 2 && !pCollider->PrepareForIntersectionTest(gtest+1, this,gtest+0, true))
			return 0;
	}
	if (ihf | (imesh = iszero(itype[0]-GEOM_TRIMESH) | iszero(itype[1]-GEOM_TRIMESH)*2)) {
		int nIdxTri=0,nFeatures=0,nIds=0,nUsedNodes=0,nNodeTris=0,nVtx=0,nTris=0;
		CTriMesh *pMesh[2] = { (CTriMesh*)this, (CTriMesh*)pCollider };
		for(i=0;i<2;i++) if ((ihf|imesh)&1<<i) {
			nNodeTris = pMesh[i]->GetBVTree()->MaxPrimsInNode();
			nIdxTri += pMesh[i]->m_nMaxVertexValency+nNodeTris;
			nFeatures += pMesh[i]->m_nMaxVertexValency;
			nIds += max(pMesh[i]->m_nMaxVertexValency,nNodeTris);
			nVtx = max(nVtx, pMesh[i]->m_nVertices);
			nTris = max(nTris, pMesh[i]->m_nTris);
			ijob = pMesh[i]->m_pTree->GetType();
			if (iszero(ijob-BVT_OBB)|iszero(ijob-BVT_AABB))
				nUsedNodes += (((CAABBTree*)pMesh[i]->m_pTree)->m_nNodes-1>>5)+1;
		}
		if (max(max(max(max(max(nIdxTri-128,nFeatures-32),nIds-128),nUsedNodes-240),nVtx-(256*32)),nTris-(400*32))>0)
			return Intersect(piCollider, pdata1,pdata2, pip, pcontacts);
	}

	CMemStream stmSizer;

	SaveGeomToMem(this, stmSizer, 0); size0=stmSizer.m_iPos; stmSizer.m_iPos=0;
	SaveGeomToMem(pCollider, stmSizer, 0); size1=stmSizer.m_iPos; 
	sizehdr = sizeof(int)*2+sizeof(geom_world_data)*2+sizeof(intersection_params)+(sizeof(void*)+sizeof(int))*2;
	if (sizehdr+size0+size1 > g_maxJobSize[0])
		return Intersect(piCollider, pdata1,pdata2, pip, pcontacts);

	if (!g_job[ijob=g_ijob].bWorking && pent0==g_job[ijob].pent0last) {
		ioffs0 = (ihf & 1) ? 0 : g_job[ijob].has_geom(this);
		ioffs1 = (ihf & 2) ? 0 : g_job[ijob].has_geom(pCollider);
		size = sizehdr + (size0 & ioffs0-1>>31) + (size1 & ioffs1-1>>31);
		if (g_job[ijob].szin+size > g_job[ijob].szinMax[0]) {
			pip->bKeepPrevContacts = false;
			goto findempty;
		}
		pip->bKeepPrevContacts = true;
	}	else {
		findempty:
		const int nJobs = g_nJobs;
		for(ijob=0; ijob<nJobs && g_job[ijob].bWorking; ijob++);
		for(i=ijob+1;i<nJobs;i++) if (!g_job[i].bWorking && g_job[i].szinMax[0]-g_job[i].szin > g_job[ijob].szinMax[0]-g_job[ijob].szin)
			ijob = i;
		if (ijob>=nJobs || 
			 ((ioffs0 = (ihf & 1) ? 0 : g_job[ijob].has_geom(this)),
			  (ioffs1 = (ihf & 2) ? 0 : g_job[ijob].has_geom(pCollider)),
			  (size = sizehdr + (size0 & ioffs0-1>>31) + (size1 & ioffs1-1>>31)),
			  g_job[ijob].szin+size > g_job[ijob].szinMax[0])) 
		{
			if ((ijob = WaitForJobFinish(*(int*)0,0))<0) {
				pip->bKeepPrevContacts = bKeepPrevContacts;
				return Intersect(piCollider, pdata1,pdata2, pip, pcontacts);
      }
			ioffs0=ioffs1 = 0; size = sizehdr+size0+size1;
		}
		pip->bKeepPrevContacts = false;
	}

	g_job[g_ijob=ijob].pent0last = pent0;
	CMemStream stm(g_job[ijob].bufin+g_job[ijob].szin, g_job[ijob].szinMax[0]-g_job[ijob].szin, false);

	SaveGeomToJobBuf(stm,this,ioffs0,ijob);
	SaveGeomToJobBuf(stm,pCollider,ioffs1,ijob);

	stm.Write(pdata1, sizeof(geom_world_data));
	stm.Write(pdata2, sizeof(geom_world_data));
	stm.Write(pip, sizeof(intersection_params));
	stm.Write(pent0); stm.Write(ipart0);
	stm.Write(pent1); stm.Write(ipart1);

	g_job[ijob].szin += stm.m_iPos;
	g_job[ijob].ipass = 0;

	pip->bKeepPrevContacts = bKeepPrevContacts;

	return 0;
}
#endif//__SPU__

#if defined(PHYS_JOB_SIMULATION) || defined(__SPU__)
  char g_JobBufIntersect[MAX_PHYS_THREADS+1][MAX_JOB_SIZE_INTERSECT] JOB_LOCAL("RB_Intersect") _ALIGN(128);
  char g_JobBufSolver[MAX_PHYS_THREADS+1][MAX_JOB_SIZE_SOLVER] JOB_LOCAL("RB_Solver") _ALIGN(128);
  char g_JobBufCloth[MAX_PHYS_THREADS+1][MAX_JOB_SIZE_CLOTH] JOB_LOCAL("Cloth") _ALIGN(128);
	char g_JobBufRope[MAX_PHYS_THREADS+1][MAX_JOB_SIZE_ROPE] JOB_LOCAL("Rope") _ALIGN(128);
#else
	char g_JobBufIntersect[MAX_PHYS_THREADS+1][1];
	char g_JobBufSolver[MAX_PHYS_THREADS+1][1];
	char g_JobBufCloth[MAX_PHYS_THREADS+1][1];
	char g_JobBufRope[MAX_PHYS_THREADS+1][1];
#endif

int ReportJobContacts(char *bufout)
{
	int iCaller = get_iCaller();
	if (!g_nTotContacts)
		return 0;
	char *pbuf = SPU_LOCAL_PTR((char*)(&g_idata[iCaller]));
	geom_contact *pcont;
	int i,sz,ncont,sztot=0;
	INT_PTR pAreaBuf,pAreaPtBuf,pAreaPrimBuf0,pAreaPrimBuf1,pAreaFeatureBuf0,pAreaFeatureBuf1,pBrdPtBuf,pBrdiTriBuf;
	*(int*)pbuf = ncont=g_nTotContacts; sztot += sizeof(int)*2;
	memmove(pbuf+sztot, g_ContPent[0], sz=ncont*sizeof(g_ContPent[0][0])); sztot+=sz;
	memmove(pbuf+sztot, g_ContPent[1], sz); sztot+=sz;
	memmove(pbuf+sztot, g_ContiPart, sz=ncont*sizeof(g_ContiPart[0])); sztot+=sz;
	memmove(pcont=(geom_contact*)(pbuf+sztot), g_Contacts, sz=ncont*sizeof(geom_contact)); sztot+=sz;
	memmove(pbuf+sztot, g_AreaBuf, sz=g_nAreas*sizeof(geom_contact_area)); pAreaBuf=(INT_PTR)pbuf+sztot; sztot+=sz;
	memmove(pbuf+sztot, g_AreaPtBuf, sz=g_nAreaPt*sizeof(Vec3)); pAreaPtBuf=(INT_PTR)bufout+sztot; sztot+=sz;
	memmove(pbuf+sztot, g_AreaPrimBuf0, sz=g_nAreaPt*sizeof(int)); pAreaPrimBuf0=(INT_PTR)bufout+sztot; sztot+=sz;
	memmove(pbuf+sztot, g_AreaFeatureBuf0, sz); pAreaFeatureBuf0=(INT_PTR)bufout+sztot; sztot+=sz;
	memmove(pbuf+sztot, g_AreaPrimBuf1, sz); pAreaPrimBuf1=(INT_PTR)bufout+sztot; sztot+=sz;
	memmove(pbuf+sztot, g_AreaFeatureBuf1, sz); pAreaFeatureBuf1=(INT_PTR)bufout+sztot; sztot+=sz;
	memmove(pbuf+sztot, g_BrdPtBuf, (sz=g_BrdPtBufPos)*sizeof(Vec3)); pBrdPtBuf=(INT_PTR)bufout+sztot; sztot+=sz*sizeof(Vec3);
	memmove(pbuf+sztot, g_BrdiTriBuf, sz*=sizeof(int)*2); pBrdiTriBuf=(INT_PTR)bufout+sztot; sztot+=sz;
	for(i=0;i<ncont;i++) {
		if (pcont[i].parea) {
			int *g_AreaPrimBuf0d,*g_AreaPrimBuf1d,*g_AreaFeatureBuf0d,*g_AreaFeatureBuf1d;
			if (pcont[i].parea->piPrim[0]<pcont[i].parea->piPrim[1]) {
				g_AreaPrimBuf0d=g_AreaPrimBuf0; g_AreaPrimBuf1d=g_AreaPrimBuf1; g_AreaFeatureBuf0d=g_AreaFeatureBuf0; g_AreaFeatureBuf1d=g_AreaFeatureBuf1;
			}	else {
				g_AreaPrimBuf0d=g_AreaPrimBuf1; g_AreaPrimBuf1d=g_AreaPrimBuf0; g_AreaFeatureBuf0d=g_AreaFeatureBuf1; g_AreaFeatureBuf1d=g_AreaFeatureBuf0;
			}
			pcont[i].parea = (geom_contact_area*)((INT_PTR)pcont[i].parea+pAreaBuf-(INT_PTR)g_AreaBuf);
			pcont[i].parea->piPrim[0] = (int*)((INT_PTR)pcont[i].parea->piPrim[0]+pAreaPrimBuf0-(INT_PTR)g_AreaPrimBuf0d);
			pcont[i].parea->piPrim[1] = (int*)((INT_PTR)pcont[i].parea->piPrim[1]+pAreaPrimBuf1-(INT_PTR)g_AreaPrimBuf1d);
			pcont[i].parea->piFeature[0] = (int*)((INT_PTR)pcont[i].parea->piFeature[0]+pAreaFeatureBuf0-(INT_PTR)g_AreaFeatureBuf0d);
			pcont[i].parea->piFeature[1] = (int*)((INT_PTR)pcont[i].parea->piFeature[1]+pAreaFeatureBuf1-(INT_PTR)g_AreaFeatureBuf1d);
			pcont[i].parea->pt = (Vec3*)((INT_PTR)pcont[i].parea->pt+pAreaPtBuf-(INT_PTR)g_AreaPtBuf);
			pcont[i].parea = (geom_contact_area*)((INT_PTR)pcont[i].parea-(INT_PTR)pbuf+(INT_PTR)bufout);
		}
		pcont[i].ptborder = (Vec3*)((INT_PTR)pcont[i].ptborder + 
			(pcont[i].ptborder==&g_Contacts[i].pt || pcont[i].ptborder==&g_Contacts[i].center ?
			 (INT_PTR)pcont-(INT_PTR)g_Contacts+(INT_PTR)bufout-(INT_PTR)pbuf : 
			(g_Contacts[i].parea && pcont[i].ptborder==g_Contacts[i].parea->pt ? 
			 pAreaPtBuf-(INT_PTR)g_AreaPtBuf	:
			 pBrdPtBuf-(INT_PTR)g_BrdPtBuf)));
		pcont[i].idxborder = (int(*)[2])((INT_PTR)pcont[i].idxborder+pBrdiTriBuf-(INT_PTR)g_BrdiTriBuf);
	}
	sztot = (sztot + 127) & ~127;//align for transfer speed
	((int*)pbuf)[1] = sztot;
	memtransfer_to_main(bufout,pbuf,sztot,0/*id*/);
	memtransfer_sync(0/*id*/);
	return sztot;
}

extern geom_world_data defgwd;
extern intersection_params defip;

extern COverlapChecker g_Overlapper;
CGeometry *LoadGeomFromMemBufOffs(CMemStream &stm)
{
	int ioffs=stm.Read<int>(), ipos0=stm.m_iPos;
	stm.m_iPos += ioffs-stm.m_iPos & -ioffs>>31;
	CGeometry *pGeom = LoadGeomFromMem(stm); 
	stm.m_iPos += ipos0-stm.m_iPos & -ioffs>>31;
	return pGeom;
}


#if !defined(CRYCG_CM)
SPU_ENTRY(RB_Intersect)
#endif
void IntersectJobProc(char *bufin,int szin, char *bufout,int szout, volatile int *written)
{
	SPU_DOMAIN_LOCAL CGeometry *pGeom[2];
	SPU_DOMAIN_LOCAL geom_world_data *pgwd[2];
	SPU_DOMAIN_LOCAL intersection_params *pip;
	SPU_DOMAIN_LOCAL CPhysicalEntity *pent[2];
	SPU_DOMAIN_LOCAL geom_contact *pcont;
	int i,ipart[2],ncont,sz;
	int iCaller = get_iCaller_int();
	//align transfer for speed, bypass cache
	memtransfer_from_main(&g_JobBufIntersect[iCaller], bufin, (szin+127)&~127, 0);
	//initialize structures as it is done via static variables on ppu, use stack as they do not occupy any space
#ifdef __SPU__
	InitGeometryGlobals now;
	InitTriMeshGlobals initTriMeshGlobals;
	InitHeightfieldGlobals initHeightfieldGlobals;
	g_Contacts = g_idata[0].Contacts;
	g_maxContacts = sizeof(g_idata[0].Contacts)/sizeof(geom_contact);
#endif
	memtransfer_sync(0);
	CMemStream stm(&g_JobBufIntersect[iCaller],szin,false);

	g_nTotContacts = 0;
	//initialize here since we do not support job global initializers
	//g_idata[iCaller].Overlapper.iPrevCode = -1;
	g_Overlapper.iPrevCode = -1;
	new (&defgwd) geom_world_data();
	new (&defip) intersection_params();

	while(*(int*)(stm.m_pBuf+stm.m_iPos)!=-1){
		for(i=0;i<2;i++)
			pGeom[i] = LoadGeomFromMemBufOffs(stm);
		pgwd[0] = SPU_LOCAL_PTR((geom_world_data*)(stm.m_pBuf+stm.m_iPos)); 
    stm.m_iPos += sizeof(geom_world_data);
		pgwd[1] = SPU_LOCAL_PTR((geom_world_data*)(stm.m_pBuf+stm.m_iPos)); 
    stm.m_iPos += sizeof(geom_world_data);
    pip = SPU_LOCAL_PTR((intersection_params*)(stm.m_pBuf+stm.m_iPos)); 
    stm.m_iPos += sizeof(intersection_params);
		pip->bThreadSafe = 1;
		if (!pip->bKeepPrevContacts) {
			if ((*written+(sz=ReportJobContacts(bufout+*written)))>=szout)
				for(*written=(*written+sz)|1<<29,g_someJobDone.Set(); *written; )
#if defined(PHYS_JOB_SIMULATION) && defined(PS3) && !defined(__SPU__)
					WaitForJobFinish(i,0)
#endif
					;
			else if (sz>0) {
				*written += sz; 
				g_someJobDone.Set();
			}
		}
		stm.Read(pent[0]); stm.Read(ipart[0]);
    MEMSTREAM_DEBUG_ASSERT(pent[0]);
		stm.Read(pent[1]); stm.Read(ipart[1]);
    MEMSTREAM_DEBUG_ASSERT(pent[1]);

    MEMSTREAM_DEBUG_ASSERT(pGeom[0]);    
 		ncont = pGeom[0]->IsAPrimitive() ? 
 			((CPrimitive*)pGeom[0])->CPrimitive::Intersect(pGeom[1], pgwd[0],pgwd[1], pip, pcont) :
 			pGeom[0]->CGeometry::Intersect(pGeom[1], pgwd[0],pgwd[1], pip, pcont);
    //ncont = pGeom[0]->Intersect(pGeom[1], pgwd[0], pgwd[1], pip, pcont);
		for(; ncont>0; ncont--) for(i=0;i<2;i++) {
			g_ContPent[i][g_nTotContacts-ncont] = pent[i];
			g_ContiPart[g_nTotContacts-ncont][i] = ipart[i];
		}
	}
	const int written_cur = *written;//spare 1 volatile lookup
	SignalOutput(written, (written_cur + ReportJobContacts(bufout+written_cur)) | 1<<30);
	g_someJobDone.Set();
}

#ifdef USE_SOLVER_JOB
void InvokeDelayedContactSolver(CMemStream &stm);
#if !defined(CRYCG_CM)
SPU_ENTRY(RB_Solver)
#endif
void SolverJobProc(char *bufin,int szin, volatile int *written)
{
	int iCaller = get_iCaller_int();
	//align transfer for speed, bypass cache
	memtransfer_from_main(&g_JobBufSolver[iCaller], bufin, (szin+127)&~127, 1/*id*/);
	memtransfer_sync(1/*id*/);
	CMemStream stm(&g_JobBufSolver[iCaller],szin,false);

	while(stm.m_iPos<szin) 
		InvokeDelayedContactSolver(stm);

	memtransfer_to_main(bufin, &g_JobBufSolver[iCaller], (szin+127)&~127,1/*id*/);
	memtransfer_sync(1/*id*/);
	SignalOutput(written, szin|1<<30);
	g_someJobDone.Set();
}
#endif


#ifdef USE_CLOTH_JOB
JOB_LOCAL("Cloth") geom_contact g_ClothContacts[4];
#if !defined(CRYCG_CM)
SPU_ENTRY(Cloth)
#endif
void ClothJobProc(char *bufin,int szin, volatile int *written)
{
	int iCaller = get_iCaller_int();
	//align transfer for speed, bypass cache
	memtransfer_from_main(&g_JobBufCloth[iCaller], bufin, (szin+127)&~127, 2/*id*/);
	memtransfer_sync(2/*id*/);
	CMemStream stm(&g_JobBufCloth[iCaller],szin,false);

#ifdef __SPU__
	g_Contacts = g_ClothContacts;
	g_maxContacts = sizeof(g_ClothContacts)/sizeof(g_ClothContacts[0]);
#endif

	while(stm.m_iPos<szin) {
		CPhysicalEntity *pent = stm.Read<CPhysicalEntity*>();
		CSoftEntity::JobProc(stm);
	}

	memtransfer_to_main(bufin, &g_JobBufCloth[iCaller], (szin+127)&~127,2/*id*/);
	memtransfer_sync(2/*id*/);
	SignalOutput(written, szin|1<<30);
	g_someJobDone.Set();
}
#endif

#ifdef USE_ROPE_JOB
#if !defined(CRYCG_CM)
SPU_ENTRY(Rope)
#endif
void RopeJobProc(char *bufin,int szin, volatile int *written)
{
	int iCaller = get_iCaller_int();
	//align transfer for speed, bypass cache
	memtransfer_from_main(&g_JobBufRope[iCaller], bufin, (szin+127)&~127, 2/*id*/);
	memtransfer_sync(2/*id*/);
	CMemStream stm(&g_JobBufRope[iCaller],szin,false);
#ifdef __SPU__
	g_Overlapper.iPrevCode = -1;
	g_nTotContacts = 0;

	InitGeometryGlobals now;
	InitTriMeshGlobals initTriMeshGlobals;
	InitHeightfieldGlobals initHeightfieldGlobals;

	g_Contacts = g_idata[0].Contacts;
	g_maxContacts = sizeof(g_idata[0].Contacts)/sizeof(geom_contact);

	new (&defgwd) geom_world_data();
	new (&defip) intersection_params();
#endif

	while(stm.m_iPos<szin) {
		/*CPhysicalEntity *pent = */stm.Read<CPhysicalEntity*>();
		CRopeEntity::JobProc(stm);
	}

	memtransfer_to_main(bufin, &g_JobBufRope[iCaller], (szin+127)&~127,2/*id*/);
	memtransfer_sync(2/*id*/);
	SignalOutput(written, szin|1<<30);
	g_someJobDone.Set();
}
#endif

#if defined(PS3) && !defined(__SPU__)
	DECLARE_SPU_JOB("RB_Intersect", TIntersectJob)
#ifdef USE_SOLVER_JOB
	DECLARE_SPU_JOB("RB_Solver", TSolverJob)
#endif
#ifdef USE_CLOTH_JOB
	DECLARE_SPU_JOB("Cloth", TClothJob)
#endif
#ifdef USE_ROPE_JOB
	DECLARE_SPU_JOB("Rope", TRopeJob)
#endif
#endif


SPU_NO_INLINE void SaveBVTreeToMem(CBVTree *pTree, CMemStream &stm)
{
  MEMSTREAM_DEBUG_ASSERT(pTree);
	int itype = pTree->GetType();
	stm.Write(itype);
	switch (itype) {
		case BVT_OBB: {	COBBTree *pOBBTree = (COBBTree*)pTree;
			stm.Write(pOBBTree, sizeof(COBBTree));
			stm.Write(pOBBTree->m_pNodes, pOBBTree->m_nNodes*sizeof(OBBnode));
			stm.Write(pOBBTree->m_pTri2Node, pOBBTree->m_pMesh->m_nTris*sizeof(pOBBTree->m_pTri2Node[0]));
     
		} break;
		case BVT_AABB: { CAABBTree *pAABBTree = (CAABBTree*)pTree;
			stm.Write(pAABBTree, sizeof(CAABBTree));
			stm.Write(pAABBTree->m_pNodes, pAABBTree->m_nNodes*sizeof(AABBnode));
			stm.Write(pAABBTree->m_pTri2Node, ((pAABBTree->m_pMesh->m_nTris-1>>5-pAABBTree->m_nBitsLog)+1)*sizeof(int));	
		} break;
		case BVT_SINGLEBOX: 
			stm.Write(pTree, sizeof(CSingleBoxTree));
			break;
		case BVT_HEIGHTFIELD: { CHeightfieldBV *pHfBV = (CHeightfieldBV*)pTree;
			stm.Write(pHfBV->m_pUsedTriMap, ((pHfBV->m_pMesh->m_nTris-1>>5)+1)*sizeof(int));
		} break;
		case BVT_VOXEL: { voxelgrid *pvx = ((CVoxelBV*)pTree)->m_pgrid;
			stm.Write(pvx->pCellTris, sizeof(int)*(pvx->size.GetVolume()+1));
			stm.Write(pvx->pTriBuf, sizeof(int)*pvx->pCellTris[pvx->size.GetVolume()]);
		}	break;
	}
#if defined(MEMSTREAM_DEBUG)
  int tag = MEMSTREAM_DEBUG_TAG;
  stm.Write(&tag, sizeof(int));
#endif 
}

CBVTree *LoadBVTreeFromMem(CMemStream &stm, CTriMesh *pMesh)
{
	char *stmBuf = SPU_LOCAL_PTR(stm.m_pBuf);
  MEMSTREAM_DEBUG_ASSERT(pMesh);
	int itype = stm.Read<int>();
	CBVTree *pTree = (CBVTree*)(stmBuf+stm.m_iPos);
  MEMSTREAM_DEBUG_ASSERT(pTree);
	switch (itype) {
		case BVT_OBB: { COBBTree *pOBBTree = (COBBTree*)pTree;
			stm.m_iPos += sizeof(COBBTree);
			pOBBTree->m_pMesh = pMesh;
			pOBBTree->m_pNodes = (OBBnode*)(stmBuf+stm.m_iPos); stm.m_iPos += pOBBTree->m_nNodes*sizeof(OBBnode);
			pOBBTree->m_pTri2Node = (index_t*)(stmBuf+stm.m_iPos); stm.m_iPos += pMesh->m_nTris*sizeof(index_t);
		} break;
		case BVT_AABB: { CAABBTree *pAABBTree = (CAABBTree*)pTree; 
			stm.m_iPos += sizeof(CAABBTree);
			pAABBTree->m_pMesh = pMesh;
			pAABBTree->m_pNodes = (AABBnode*)(stmBuf+stm.m_iPos); stm.m_iPos += pAABBTree->m_nNodes*sizeof(AABBnode);
			pAABBTree->m_pTri2Node = (int*)(stmBuf+stm.m_iPos); stm.m_iPos += ((pMesh->m_nTris-1>>5-pAABBTree->m_nBitsLog)+1)*sizeof(int);	
		} break;
		case BVT_SINGLEBOX:
			((CSingleBoxTree*)pTree)->m_pGeom = pMesh;
			stm.m_iPos += sizeof(CSingleBoxTree);
			break;
		case BVT_HEIGHTFIELD: {	
			CHeightfieldBV *pHfBV = (CHeightfieldBV*)(pTree = &((CHeightfield*)pMesh)->m_Tree);
			pHfBV->m_pMesh = pMesh;
			pHfBV->m_phf = &((CHeightfield*)pMesh)->m_hf;
			pHfBV->m_pUsedTriMap = (unsigned int*)(stmBuf+stm.m_iPos); stm.m_iPos += ((pMesh->m_nTris-1>>5)+1)*sizeof(int);
		} break;
		case BVT_VOXEL: {	
			CVoxelBV *pVoxBV = (CVoxelBV*)(pTree = &((CVoxelGeom*)pMesh)->m_Tree);
			pVoxBV->m_pMesh = pMesh;
			pVoxBV->m_pgrid = &((CVoxelGeom*)pMesh)->m_grid;
			pVoxBV->m_pgrid->pCellTris = (int*)(stmBuf+stm.m_iPos); stm.m_iPos += sizeof(int)*(pVoxBV->m_pgrid->size.GetVolume()+1);
			pVoxBV->m_pgrid->pTriBuf = (int*)(stmBuf+stm.m_iPos); stm.m_iPos += sizeof(int)*pVoxBV->m_pgrid->pCellTris[pVoxBV->m_pgrid->size.GetVolume()];
			pVoxBV->m_pgrid->pVtx = pMesh->m_pVertices;
			pVoxBV->m_pgrid->pIndices = pMesh->m_pIndices;
			pVoxBV->m_pgrid->pNormals = pMesh->m_pNormals;
			pVoxBV->m_pgrid->pIds = pMesh->m_pIds;
		} break;
		default:
			return 0;
	}
#if defined(MEMSTREAM_DEBUG)
  int read_tag;
  stm.Read(read_tag);
  if (read_tag != MEMSTREAM_DEBUG_TAG) { snPause(); }
#endif 
	return pTree;
}


SPU_NO_INLINE void SaveGeomToMem(CGeometry *pGeom, CMemStream &stm, int bForRays)
{
	int itype=pGeom->GetType(), sz=sizeof(CTriMesh), i,j;
	stm.Write(itype);
	switch (itype) {
		case GEOM_VOXELGRID: 
      sz=sizeof(CVoxelGeom); 
      goto savemesh;
		case GEOM_HEIGHTFIELD: 
      sz=sizeof(CHeightfield);
		case GEOM_TRIMESH: {
			savemesh:
			CTriMesh *pMesh = (CTriMesh*)pGeom;
			stm.Write(pGeom, sz);
			stm.Write(pMesh->m_pIndices, pMesh->m_nTris*3*sizeof(pMesh->m_pIndices[0]));
			if (pMesh->m_pIds) {
				stm.Write((int)1);
				stm.Write(pMesh->m_pIds, ((pMesh->m_nTris-1&~3)+4)*sizeof(pMesh->m_pIds[0]));
			} else 
				stm.Write((int)0);
			stm.Write(pMesh->m_pVertices.data, pMesh->m_nVertices*pMesh->m_pVertices.iStride);
			stm.Write(pMesh->m_pNormals, pMesh->m_nTris*sizeof(pMesh->m_pNormals[0]));
			stm.Write(pMesh->m_pTopology, pMesh->m_nTris*sizeof(pMesh->m_pTopology[0]));
			if (itype==GEOM_TRIMESH && bForRays) {
				stm.Write(-1);
				for(i=0; i<pMesh->m_nHashPlanes; i++) {
					stm.Write(pMesh->m_pHashGrid[i], sizeof(index_t)*(j=pMesh->m_hashgrid[i].size.x*pMesh->m_hashgrid[i].size.y+1));
					stm.Write(pMesh->m_pHashData[i], sizeof(index_t)*pMesh->m_pHashGrid[i][j-1]);
				}
				stm.m_iPos += (pMesh->m_nTris+31>>5)*4; // space for used triangles array
			} else 
				SaveBVTreeToMem(pMesh->m_pTree, stm);
		} break;
		default: 
			sz = pGeom->GetSizeFast();
			stm.Write(&sz, sizeof(sz));
			stm.Write(pGeom, sz);
	}
#if defined(MEMSTREAM_DEBUG)
  int tag = MEMSTREAM_DEBUG_TAG;
  stm.Write(&tag, sizeof(int));
#endif
}

SPU_DEFAULT_TO_LOCAL(RWI)
CGeometry *LoadGeomFromMem(CMemStream &_stm)
{
	SPU_DOMAIN_LOCAL CMemStream &stm(SPU_LOCAL_REF(_stm));
	SPU_DOMAIN_LOCAL char *stmBuf = SPU_LOCAL_PTR(stm.m_pBuf);
	int itype=stm.Read<int>(), sz=sizeof(CTriMesh), i,j;
	CGeometry *pGeom = 0;
	switch (itype) {
		case GEOM_VOXELGRID: 
      sz=sizeof(CVoxelGeom);	
      goto loadmesh;
		case GEOM_HEIGHTFIELD: {
			CHeightfield *phf = (CHeightfield*)(stmBuf+stm.m_iPos);
			sz = sizeof(CHeightfield);
#ifndef __SPU__
			phf->m_hf.fpGetHeightCallback = getHFPatchHeight;
			phf->m_hf.fpGetSurfTypeCallback = getHFPatchSurfType;
#endif
			int iCaller = get_iCaller();
			g_pHF = phf;
		}
		case GEOM_TRIMESH: {
			loadmesh:
      CTriMesh *pMesh = (CTriMesh*)(stmBuf+stm.m_iPos);	stm.m_iPos += sz;
			pMesh->m_pIndices = (index_t*)(stmBuf+stm.m_iPos); stm.m_iPos += pMesh->m_nTris*3*sizeof(pMesh->m_pIndices[0]);
			stm.Read(i);
			pMesh->m_pIds = stmBuf+stm.m_iPos; stm.m_iPos += ((pMesh->m_nTris-1&~3)+4)*sizeof(pMesh->m_pIds[0])*i;
			pMesh->m_pIds = (char*)((INT_PTR)pMesh->m_pIds & -(INT_PTR)i);
			pMesh->m_pVertices.data = (Vec3*)(stmBuf+stm.m_iPos); stm.m_iPos += pMesh->m_nVertices*pMesh->m_pVertices.iStride;
			pMesh->m_pNormals = (Vec3*)(stmBuf+stm.m_iPos); stm.m_iPos += pMesh->m_nTris*sizeof(pMesh->m_pNormals[0]);
			pMesh->m_pTopology = (trinfo*)(stmBuf+stm.m_iPos); stm.m_iPos += pMesh->m_nTris*sizeof(pMesh->m_pTopology[0]);
			pMesh->m_pForeignIdx=0; pMesh->m_pVtxMap=0; pMesh->m_pMeshUpdate=0; 
			if (pMesh->m_pTree = LoadBVTreeFromMem(stm, pMesh))
				pMesh->m_nHashPlanes = 0;
			else {
				for(i=0; i<pMesh->m_nHashPlanes; i++) {
					pMesh->m_pHashGrid[i] = (index_t*)(stmBuf+stm.m_iPos);
					stm.m_iPos += sizeof(index_t)*(j=pMesh->m_hashgrid[i].size.x*pMesh->m_hashgrid[i].size.y+1);
					pMesh->m_pHashData[i] = (index_t*)(stmBuf+stm.m_iPos);
					stm.m_iPos += sizeof(index_t)*pMesh->m_pHashGrid[i][j-1];
				}
				pMesh->m_pUsedTriMap = (unsigned int*)(stmBuf+stm.m_iPos); 
				stm.m_iPos += (pMesh->m_nTris+31>>5)*4;
			}
			pGeom = pMesh;
		} break;
		default:
			sz = *((int*)(stmBuf+stm.m_iPos));
      stm.m_iPos += sizeof(int);
			pGeom = (CGeometry*)(stmBuf+stm.m_iPos); 
			stm.m_iPos += sz;
	}
#if defined(MEMSTREAM_DEBUG)
  int read_tag;
  stm.Read(read_tag);
  if (read_tag != MEMSTREAM_DEBUG_TAG) { snPause(); }
#endif 
	return pGeom;
}

#ifndef __SPU__
int *g_pAllGroupsFinished = 0;
int FindJobQueue(int sz, CMemStream &stm, int ipass)
{
	if (sz>g_maxJobSize[ipass])
		return -1;
#ifndef PHYS_JOB_SIMULATION
	if(!InvokeJobOnSPU("RB_Intersect"))
		return -1;
#endif
	int i,ijob;
	const int nJobs = g_nJobs;
	if (sz<0 && g_job[g_ijob].ipass==ipass)
		ijob = g_ijob;
	else {
		for(ijob=0; ijob<nJobs && (g_job[ijob].bWorking || g_job[ijob].szin>0 && g_job[ijob].ipass!=ipass); ijob++);
		for(i=ijob+1;i<nJobs;i++) 
			if (!g_job[i].bWorking && (g_job[i].szin==0 || g_job[i].ipass==ipass) && 
					g_job[i].szinMax[ipass]-g_job[i].szin > g_job[ijob].szinMax[ipass]-g_job[ijob].szin)
			ijob = i;
		if (ijob>=nJobs || g_job[ijob].szin+sz>g_job[ijob].szinMax[ipass])
			if ((ijob=WaitForJobFinish(*g_pAllGroupsFinished,0))<0)
				return -1;
	}
	stm.m_pBuf = g_job[ijob].bufin+g_job[ijob].szin;
	stm.m_iPos = 0;
	stm.m_nSize = g_job[ijob].szinMax[ipass]-g_job[ijob].szin;
	stm.bMeasureOnly = 0;
	g_job[ijob].szin += max(sz,0);
	g_job[ijob].ipass = ipass;
	return g_ijob=ijob;
}

void UpdateJobBuf(int ijob, int szAdd) { g_job[ijob].szin += szAdd; }


int WaitForJobFinish(int &bAllGroupsFinished,int bWait)
{
	int i,nworking,ijob;
	bool bInvokedOnSPU=false;
	if (&bAllGroupsFinished)
		g_pAllGroupsFinished = &bAllGroupsFinished;

	const int nJobs = g_nJobs;
	for(i=nworking=0; i<nJobs; nworking+=g_job[i++].bWorking) if (g_job[i].szin) {
#if defined(PS3) && !defined(__SPU__)
		bInvokedOnSPU = InvokeJobOnSPU(g_jobName[g_job[i].ipass]);
#endif
		*(int*)(g_job[i].bufin+g_job[i].szin) = -1;
		g_job[i].bWorking=1; g_job[i].szin0=g_job[i].szin; g_job[i].szin=0; 
		g_job[i].nUsedGeoms=0; g_job[i].pent0last=0; g_job[i].written=g_job[i].read=0; 
		g_job[i].stmAux.m_iPos = 0;
#if defined(PS3) && !defined(__SPU__)
		if (bInvokedOnSPU) {
			switch (g_job[i].ipass) {
				case 0: {	TIntersectJob job(g_job[i].bufin,g_job[i].szin0+4, g_job[i].bufout,g_job[i].szout, &g_job[i].written);
									job.SetCacheMode(NPPU::eCM_4);
									job.Run();
								} break;
#ifdef USE_SOLVER_JOB
				case 1: {	TSolverJob job(g_job[i].bufin,g_job[i].szin0, &g_job[i].written);
									job.SetCacheMode(NPPU::eCM_None);
									job.Run();
								} break;
#endif
#ifdef USE_CLOTH_JOB
				case 2: { TClothJob job(g_job[i].bufin,g_job[i].szin0, &g_job[i].written);
									job.SetCacheMode(NPPU::eCM_4);
									job.Run();
								} break;
#endif
#ifdef USE_ROPE_JOB
				case 3: { TRopeJob job(g_job[i].bufin,g_job[i].szin0, &g_job[i].written);
									job.SetCacheMode(NPPU::eCM_4);
									job.Run();
								} break;
#endif

			}
		}	else
#if defined(PHYS_JOB_SIMULATION)
		{
			switch (g_job[i].ipass) {
				case 0: IntersectJobProc(g_job[i].bufin,g_job[i].szin0+4, g_job[i].bufout,g_job[i].szout, &g_job[i].written); break;
				case 1:	SolverJobProc(g_job[i].bufin,g_job[i].szin0, &g_job[i].written); break;
				case 2: ClothJobProc(g_job[i].bufin,g_job[i].szin0, &g_job[i].written); break;
#ifdef USE_ROPE_JOB
				case 3: RopeJobProc(g_job[i].bufin,g_job[i].szin0, &g_job[i].written); break;
#endif
			}
		}
#endif//PHYS_JOB_SIMULATION
#endif//__SPU__
#if defined(PHYS_JOB_SIMULATION) && !defined(PS3)
		g_job[i].start.Set()
#endif
		;
	}
	if (!nworking)
		return -1;
#ifdef SUPPORT_DEBUG_SPU
	const int64 cWaitEnd = CryGetTicks() + 10*80*1024*1024;//1 sec on PS3
	bool spuStopped = false;
#endif
		do { 
#ifdef SUPPORT_DEBUG_SPU
			if(g_ForceStopSPUs && bInvokedOnSPU && !spuStopped && (CryGetTicks()>cWaitEnd))
			{
				for(uint32 i=1; i<GetIJobManSPU()->GetSPUsAllowed(); ++i)
				{
					NPPU::WriteSPUProbReg(i, NPPU::scPCSPURunCntl, 0);
					snRawSPUNotifySPUStopped(i);
					printf("physics timeout: stopped spu %d\n",i);
				}
				spuStopped = true;
			}
#endif
		if (bWait && !g_someJobDone.Wait(3000))
			i=i;
		int foundJob = 0;
		int usleepIter = 0;
		for(i=nworking=0,ijob=-1;i<nJobs;i++) if (g_job[i].written>g_job[i].read) {
			foundJob = 1;
			usleepIter=0;
			for(; g_job[i].read<(g_job[i].written & (1<<29)-1); ) {
				int sz;
				if (g_job[i].ipass==0) {
					// report back the results to pent0
					int offs = g_job[i].read;
					int ncont = *(int*)(g_job[i].bufout+offs); offs+=sizeof(int);
					sz = *(int*)(g_job[i].bufout+offs);	offs+=sizeof(int);
					CPhysicalEntity *pent0 = *(CPhysicalEntity**)(g_job[i].bufout+offs); offs+=ncont*sizeof(pent0);
					CPhysicalEntity **pents = (CPhysicalEntity**)(g_job[i].bufout+offs); offs+=ncont*sizeof(pents[0]);
					int (*iparts)[2] = (int(*)[2])(g_job[i].bufout+offs); offs+=ncont*sizeof(iparts[0]);
					geom_contact *pcontacts = (geom_contact*)(g_job[i].bufout+offs);
					pent0->DelayedIntersect(pcontacts,ncont, pents,iparts);
				} else if (g_job[i].ipass==1) {
					// report solver results
					CMemStream stm(g_job[i].bufin+g_job[i].read, g_job[i].szin0-g_job[i].read, false);
					int nContacts,nBodies,nEnts,iGroup;
					entity_contact **pContacts;
					RigidBody **pBodies;
					float dt,Ebefore,fixedDamping;
					nContacts = ReadDelayedSolverResults(stm, pContacts,pBodies);
					iGroup = g_pPhysWorlds[0]->ReadDelayedSolverResults(g_job[i].stmAux, dt,Ebefore,nEnts,fixedDamping, pContacts,pBodies);
					nBodies = InvokeContactSolver(dt,&g_pPhysWorlds[0]->m_vars, Ebefore, pContacts,nContacts,pBodies);
					g_pPhysWorlds[0]->ProcessIslandSolverResults(iGroup,-1,dt,Ebefore,nEnts,fixedDamping,bAllGroupsFinished,pContacts,nContacts,nBodies,
						get_iCaller_int(),0);
					sz = stm.m_iPos;
				}	else if (g_job[i].ipass==2 || g_job[i].ipass==3) {
					CMemStream stm(g_job[i].bufin+g_job[i].read, g_job[i].szin0-g_job[i].read, false);
					CPhysicalEntity *pent = stm.Read<CPhysicalEntity*>();
					pent->OnDelayedStep(stm);
					sz = stm.m_iPos;
				}
				g_job[i].read += sz;
			}
			if (g_job[i].written & 1<<30) {
				g_job[ijob=i].bWorking = 0;
				g_job[i].stmAux.m_iPos = 0;
			} else {
				if (g_job[i].written & 1<<29)
					g_job[i].written=g_job[i].read = 0;
				nworking++; i=-1; // resume the job if it still has something to do
			}
		}	else
			nworking += g_job[i].bWorking;
#ifndef PHYS_JOB_SIMULATION
		if(!foundJob)
		{
//			sys_timer_subusleep_inldb16cyc(16);//gives other hardware thread all instr.fetch cycles
//			++usleepIter;
//			if(usleepIter > 16)
				Sleep(0);//give other thread chance to run after some iterations
			foundJob = 0;
		}
#endif
	} while(ijob==-1 && nworking && bWait);
	return ijob;
}

#if defined(PHYS_JOB_SIMULATION) && !defined(PS3)
struct SFakeSPUTask : public IThreadTask {
	SFakeSPUTask(SJobData *pjob,int idx) { m_job=pjob; m_idx=idx+1; }
	virtual void OnUpdate() {
		if (!m_job)
			return;
		MarkAsPhysWorkerThread(&m_idx);
		while(!m_job->bStop)	{
			m_job->start.Wait();
			if (!m_job->bStop)
				if (m_job->ipass==0)
					IntersectJobProc(m_job->bufin,m_job->szin0+4, m_job->bufout,m_job->szout, &m_job->written);
#ifdef USE_SOLVER_JOB
				else if (m_job->ipass==1)
					SolverJobProc(m_job->bufin,m_job->szin0, &m_job->written);
#endif
#ifdef USE_CLOTH_JOB
				else if (m_job->ipass==2)
					ClothJobProc(m_job->bufin,m_job->szin0, &m_job->written);
#endif
#ifdef USE_ROPE_JOB
				else if (m_job->ipass==3)
					RopeJobProc(m_job->bufin,m_job->szin0, &m_job->written);
#endif

		}
		GetISystem()->GetIThreadTaskManager()->UnregisterTask(this);
		g_someJobDone.Set();
		m_job = 0;
	}
	virtual void Stop() { m_job->bStop=1; m_job->start.Set(); }
	virtual SThreadTaskInfo* GetTaskInfo() { return &m_TaskInfo; }
	SJobData *m_job;
	int m_idx;
	SThreadTaskInfo m_TaskInfo;
};
#endif

void CreateJobThreads(int nJobs)
{
	if (g_nJobs==nJobs)
		return;
	int i;
	SThreadTaskParams ttp;
	ttp.name = "PhysicsSPUThread";
	ttp.nFlags = THREAD_TASK_BLOCKING;
	ttp.nPriorityOff = 1;
	g_maxJobSize[0] = MAX_JOB_SIZE_INTERSECT;
	g_maxJobSize[1] = MAX_JOB_SIZE_SOLVER;
	g_maxJobSize[2] = MAX_JOB_SIZE_CLOTH;
	g_maxJobSize[3] = MAX_JOB_SIZE_ROPE;
#ifdef SUPP_DYNAMIC_JOB_DATA_NUM
	if (g_job) {
#if defined(PHYS_JOB_SIMULATION) && !defined(PS3)
		for(i=0; i<g_nJobs; i++) {
			g_job[i].bStop = 1;	g_job[i].start.Set();
			g_someJobDone.Wait();
		}
#endif
		delete[] g_job;
	}
	g_job = (g_nJobs=nJobs)>0 ? new SJobData[nJobs] : 0;
#else
	g_nJobs = min(nJobs, MAX_JOB_DATA_COUNT);
#endif
	for(i=0; i<g_nJobs; i++) {
		g_job[i].szin = 0;
		g_job[i].written=g_job[i].read=g_job[i].bWorking = 0;
		g_job[i].szinMax[0] = g_maxJobSize[0];
		g_job[i].szinMax[1] = g_maxJobSize[1];
		g_job[i].szinMax[2] = g_maxJobSize[2];
		g_job[i].szinMax[3] = g_maxJobSize[3];
		uint32 allocSize = max(max(g_maxJobSize[0],g_maxJobSize[1]),g_maxJobSize[2]);
		g_job[i].bufin = (char*)CryModuleMemalign(allocSize, 128);
		g_JobAllocSize += allocSize;
		g_job[i].szinMax[0]-=4;
		g_job[i].szout=32768;
		allocSize = g_job[i].szout;
		g_job[i].bufout = (char*)CryModuleMemalign(allocSize, 128);
		g_JobAllocSize += allocSize;
		g_job[i].szout-=10000;
		g_job[i].nUsedGeoms = 0;
		g_job[i].pUsedGeoms = new used_geom[g_job[i].nUsedGeomsAlloc=1024];
		g_job[i].pent0last = 0;
		*(int*)g_job[i].bufin = -1;
#if defined(PHYS_JOB_SIMULATION) && !defined(PS3)
		g_job[i].bStop = 0;
		GetISystem()->GetIThreadTaskManager()->RegisterTask(new SFakeSPUTask(g_job+i,i), ttp);
#endif
		g_JobAllocSize += sizeof(SJobData);
	}
	g_ijob = 0;
}

namespace PhysicsSPUBuffer
{
	void GetMemoryUsage( ICrySizer *pSizer ) 
	{
		uint32 allocSize = max(max(g_maxJobSize[0],g_maxJobSize[1]),g_maxJobSize[2]);
		for( int i=0; i<g_nJobs; i++) 
		{
			pSizer->AddObject( g_job[i].bufin, allocSize );
			pSizer->AddObject( g_job[i].bufout, 32768 );			 
			pSizer->AddObject( g_job[i].pUsedGeoms, sizeof(used_geom) * 1024 );
		}
	}
}
#endif//__SPU__

#else
int WaitForJobFinish(int&,int) { return -1; }
void CreateJobThreads(int) {}
CMemStream &GetJobAuxStream() { static CMemStream dummy; return dummy; }
#endif

// ropes and cloth: split into CheckCollisions(checkParts) and Solve() functions; upload the entire object after checkParts if filled

#undef SUPP_DYNAMIC_JOB_DATA_NUM
#undef MAX_JOB_DATA_COUNT
