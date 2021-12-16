#ifndef geoman_h
#define geoman_h
#pragma once

const int GEOM_CHUNK_SZ = 64;
const int PHYS_GEOM_VER = 1;

class CTriMesh;
struct SCrackGeom {
	int id;
	CTriMesh *pGeom;
	int idmat;
	Vec3 pt0;
	Matrix33 Rc;
	float maxedge,rmaxedge;
	float ry3;
	vector2df pt3;
	Vec3 vtx[3];
};

struct phys_geometry_serialize {
	int dummy0;
	Vec3 Ibody;
	quaternionf q;
	Vec3 origin;
	float V;
	int nRefCount;
	int surface_idx;
	int dummy1;
	int nMats;

	AUTO_STRUCT_INFO
};

class CGeomManager : public IGeomManager {
public:
	CGeomManager() { InitGeoman(); }
	~CGeomManager() { ShutDownGeoman(); }

	void InitGeoman();
	void ShutDownGeoman();

	VIRTUAL IGeometry *CreateMesh(strided_pointer<const Vec3> pVertices,strided_pointer<unsigned short> pIndices,char *pMats,int *pForeignIdx,int nTris, 
		int flags, float approx_tolerance=0.05f, int nMinTrisPerNode=2,int nMaxTrisPerNode=4, float favorAABB=1.0f) 
	{
		SBVTreeParams params;
		params.nMinTrisPerNode = nMinTrisPerNode; params.nMaxTrisPerNode = nMaxTrisPerNode;
		params.favorAABB = favorAABB;
		return CreateMesh(pVertices,pIndices,pMats,pForeignIdx, nTris, flags & ~mesh_VoxelGrid, approx_tolerance, &params);
	}
	VIRTUAL IGeometry *CreateMesh(strided_pointer<const Vec3> pVertices,strided_pointer<unsigned short> pIndices,char *pMats,int *pForeignIdx,int nTris, 
		int flags, float approx_tolerance, SMeshBVParams *pParams);
	VIRTUAL IGeometry *CreatePrimitive(int type, const primitives::primitive *pprim);
	VIRTUAL void DestroyGeometry(IGeometry *pGeom);

	VIRTUAL phys_geometry *RegisterGeometry(IGeometry *pGeom,int defSurfaceIdx=0, int *pMatMapping=0,int nMats=0);
	VIRTUAL int AddRefGeometry(phys_geometry *pgeom);
	VIRTUAL int UnregisterGeometry(phys_geometry *pgeom);
	VIRTUAL void SetGeomMatMapping(phys_geometry *pgeom, int *pMatMapping, int nMats);

	VIRTUAL void SaveGeometry(CMemStream &stm, IGeometry *pGeom);
	VIRTUAL IGeometry *LoadGeometry(CMemStream &stm, strided_pointer<const Vec3> pVertices, strided_pointer<unsigned short> pIndices, char *pIds);
	VIRTUAL void SavePhysGeometry(CMemStream &stm, phys_geometry *pgeom);
	VIRTUAL phys_geometry *LoadPhysGeometry(CMemStream &stm, strided_pointer<const Vec3> pVertices, strided_pointer<unsigned short> pIndices, char *pIds);
	VIRTUAL IGeometry *CloneGeometry(IGeometry *pGeom);

	VIRTUAL ITetrLattice *CreateTetrLattice(const Vec3 *pt,int npt, const int *pTets,int nTets);
	VIRTUAL int RegisterCrack(IGeometry *pGeom, Vec3 *pVtx, int idmat);
	VIRTUAL void UnregisterCrack(int id);
	VIRTUAL IGeometry *GetCrackGeom(const Vec3 *pt,int idmat, geom_world_data *pgwd);
	VIRTUAL void UnregisterAllCracks(void (*OnRemoveGeom)(IGeometry *pGeom)=0) {
		for(int i=0;i<m_nCracks;i++) {
			if (OnRemoveGeom) OnRemoveGeom((IGeometry*)m_pCracks[i].pGeom);
			((IGeometry*)m_pCracks[i].pGeom)->Release();
		} m_nCracks=m_idCrack=0;
	}

	VIRTUAL IBreakableGrid2d *GenerateBreakableGrid(vector2df *ptsrc,int npt, const vector2di &nCells, int bStaticBorder, int seed=-1);

	phys_geometry *GetFreeGeomSlot();
	virtual IPhysicalWorld *GetIWorld() { return 0; }

	phys_geometry **m_pGeoms;
	int m_nGeomChunks,m_nGeomsInLastChunk;
	phys_geometry *m_pFreeGeom;
	int m_lockGeoman;

	SCrackGeom *m_pCracks;
	int m_nCracks;
	int m_idCrack;
	float m_kCrackScale,m_kCrackSkew;
	int m_sizeExtGeoms;
};

#endif
