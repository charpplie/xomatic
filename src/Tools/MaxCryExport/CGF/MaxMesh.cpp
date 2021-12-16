//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MaxMesh.h"
#include "CryMeshCompact.h"
#include "IErrorReporter.h"
#include "VNormal.h"
#include "MeshUtils.h"
#include "MaxSkinningInfo.h"
#include "ModifierUtils.h"
#include "MaxMorphData.h"
#include "MaxMaterial.h"
#include "VertexColors.h"

#define MAX_MAT_ID 64

struct VVertex
{
	Point3 pos;
	Point3 norm;
	DWORD smooth;
	int index;
	bool init;
	VVertex *next;

	VVertex() { smooth=0; index=0; next=NULL; init=false; pos=norm=Point3(0,0,0);}
	~VVertex() { delete next; }

	VVertex( int i,Point3 &p,Point3 &n,DWORD sm )
	{
		pos = p;
		norm = n;
		smooth = sm;
		index = i;
		init = true;
		next = 0;
	}

	void AddVertex( int i,Point3 &p,Point3 &n,DWORD sm )
	{
		if (!(sm&smooth) && init)
		{
			if (next)
				next->AddVertex(i,p,n,sm);
			else {
				next = new VVertex(i,p,n,sm);
			}
		} 
		else {
			pos = p;
			norm = n;
			smooth = sm;
			index = i;
			init = true;
		}
	}
	VVertex& GetVertex( DWORD sm )
	{
		if (smooth&sm || !next) return *this;
		else return next->GetVertex(sm);	
	}
};

MaxMesh::MaxMesh(IErrorReporter* pErrorReporter, INode* pMaxNode, Mesh* pMaxMesh, MaxMaterial* pMaterial, const Matrix3& transform, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, float fMorphMinOffset)
:	transform(transform),
	bHasVertexColours(false),
	bHasVertexAlpha(false),
	pMaxNode(pMaxNode),
	pSkinningInfo(0),
	pMorphData(0)
{
	this->ReadMesh(pErrorReporter, pMaxMesh, pMaterial, bGenerateDefaultUVs);
	this->CreateSkinningInfo(bAllowBlending, BoneList);
	this->CreateMorphData(fMorphMinOffset);
}

MaxMesh::~MaxMesh()
{
	delete this->pSkinningInfo;
}

int MaxMesh::UVCount()
{
	return int(this->uvs.size());
}

CryUV* MaxMesh::GetUVs()
{
	return &this->uvs[0];
}

int MaxMesh::TextureFaceCount()
{
	return int(this->textureFaces.size());
}

CryTexFace* MaxMesh::GetTextureFaces()
{
	return &this->textureFaces[0];
}

int MaxMesh::VertexCount()
{
	return int(this->vertices.size());
}

CryVertex* MaxMesh::GetVertices()
{
	return &this->vertices[0];
}

int MaxMesh::FaceCount()
{
	return int(this->faces.size());
}

CryFace* MaxMesh::GetFaces()
{
	return &this->faces[0];
}

bool MaxMesh::HasVertexColours()
{
	return this->bHasVertexColours;
}

CryIRGB* MaxMesh::GetVertexColours()
{
	return &this->vertexColours[0];
}

bool MaxMesh::HasVertexAlpha()
{
	return this->bHasVertexAlpha;
}

unsigned char* MaxMesh::GetVertexAlpha()
{
	return &this->vertexAlpha[0];
}

ISkinningInfo* MaxMesh::GetSkinningInfo()
{
	return this->pSkinningInfo;
}

IMorphData* MaxMesh::GetMorphData()
{
	return this->pMorphData;
}

Point3* MaxMesh::GetVertsForBoneSavingRoutines()
{
	return &this->vertsForBoneSavingRoutines[0];
}

int* MaxMesh::GetVertsForBoneSavingRoutinesMap()
{
	return &this->vertsForBoneSavingRoutinesMap[0];
}

int MaxMesh::GetVertexForBoneSavingRoutinesMapSize()
{
	return int(this->vertsForBoneSavingRoutinesMap.size());
}

int* MaxMesh::GetVertMap()
{
	return &this->vertMap[0];
}

int MaxMesh::GetVertMapSize()
{
	return int(this->vertMap.size());
}

void MaxMesh::ReadMesh(IErrorReporter* pErrorReporter, Mesh* pMesh, MaxMaterial* pMaterial, bool bGenerateDefaultUVs)
{
	// maps split smooth groups mesh vertex indices --> original mesh indices
	std::vector<int> arrVertexMapSmooth;
	Mesh outMesh;
	SplitSmoothGroups (*pMesh, outMesh, arrVertexMapSmooth, false, pMesh->verts);
	pMesh = &outMesh;

	if (bGenerateDefaultUVs && pMesh->getNumTVerts() == 0)
		GenerateDefaultUVs(*pMesh);

	int nVerts = pMesh->getNumVerts();
	int nFaces = pMesh->getNumFaces();
	int nTVerts = pMesh->getNumTVerts();
	int nCVerts = pMesh->getNumVertCol();
	int i;

	Face		*Faces	  = pMesh->faces;
	Point3		*Verts	  = pMesh->verts;
	TVFace		*TexFaces = pMesh->tvFace;
	UVVert		*TexVerts = pMesh->tVerts;
	VertColor	*ColVerts = pMesh->vertCol;
	TVFace		*ColFaces = pMesh->vcFace;

	if(nTVerts)
	{
		//UpdateProgress(0,"\"%s\" Preparing texture vertices", name);

		// copy all texture vertices into this array arrUVs
		this->uvs.resize(nTVerts);

		for (i=0; i<nTVerts; i++) 
		{
			CryUV& tv = this->uvs[i];
			tv.u = TexVerts[i].x;
			tv.v = TexVerts[i].y;
		}

		//UpdateProgress(10,"\"%s\" Preparing texture faces", name);

		// copy all texture faces into this array arrTexFaces
		this->textureFaces.resize(nFaces);
		for (i=0; i<nFaces; i++) 
		{
			CryTexFace& mf = this->textureFaces[i];
			mf.t0			= TexFaces[i].t[0];
			mf.t1			= TexFaces[i].t[1];
			mf.t2			= TexFaces[i].t[2];
		}

		// validate and optimize the info
		//UpdateProgress(20, "\"%s\" Cleaning up texture coordinates", name);
		if (!ValidateCryUVs(&this->textureFaces[0], this->textureFaces.size(), this->uvs.size()))
			return;

		this->uvs.resize(CompactCryUVs(&this->textureFaces[0], this->textureFaces.size(), &this->uvs[0], this->uvs.size()));
	}
	nTVerts = this->uvs.size();

	// fill the vertices in
	this->vertices.resize(nVerts);
	pMesh->buildNormals();
	for(i=0;i<nVerts;i++)
	{
		Point3 v = this->transform.PointTransform(Verts[i]);
		Point3 n = pMesh->getNormal(i);

		CryVertex& vert = this->vertices[i];
		vert.p.x=v.x; 
		vert.p.y=v.y; 
		vert.p.z=v.z;

		vert.n.x=n.x;
		vert.n.y=n.y;
		vert.n.z=n.z;
	}

	this->faces.resize(nFaces);

	for (i=0; i<nFaces; i++) 
	{
		CryFace& mf = this->faces[i];
		mf.v0			= Faces[i].v[0];
		mf.v1			= Faces[i].v[1];	
		mf.v2			= Faces[i].v[2];

		mf.SmGroup		= Faces[i].smGroup;
		mf.MatID = Faces[i].getMatID();

		if (mf.MatID < 0 || mf.MatID >= MAX_MAT_ID)
		{
			char s[1024];
			sprintf(s, "Mesh in Node %s have invalid MatID=%d, MatID must from 0 to 63\r\nMatID will be set to 0", this->pMaxNode->GetName(), mf.MatID);
			pErrorReporter->Report(IErrorReporter::Warning, s);
			mf.MatID = 0;
		}
	}

	//UpdateProgress(50, "\"%s\" Cleaning up coordinates", name);
	if (!ValidateCryVerts(&this->faces[0], this->faces.size(), this->vertices.size()))
		return;

	// maps splitted vertices to compacted vertices
	// compacts (detects and throws away the unreferred vertices)
	CompactCryVertMap (&this->faces[0], this->faces.size(), nVerts,  this->vertMap);

	// construct the vertex map and vertex array for passing to bone binding saving routines
	// this->vertsForBoneSavingRoutinesMap maps compacted vertices -> original Max vertices (through the split vertices)
	// vertices corresponding to the compacted vertex array
	{
		this->vertsForBoneSavingRoutinesMap.resize(this->vertMap.size());
		this->vertsForBoneSavingRoutines.resize(this->vertMap.size());
		for (unsigned i = 0; i < this->vertMap.size(); ++i)
		{
			this->vertsForBoneSavingRoutines[i] = Verts[this->vertMap[i]];
			this->vertsForBoneSavingRoutinesMap[i] = arrVertexMapSmooth[this->vertMap[i]];
		}
	}

	PrepareVertexColours(pMesh);

	this->bHasVertexColours = (pMesh->mapSupport(0) && (pMesh->getNumVertCol() > 0));
	this->bHasVertexAlpha = pMesh->mapSupport(MAP_ALPHA) && nVerts > 0;

	bool bMultiMat = false;
	if (pMaterial && pMaterial->IsMultiMaterial())
		bMultiMat = true;

	for (int i = 0; i < this->FaceCount(); i++)
	{
		CryFace& mf = this->GetFaces()[i];

		if (!bMultiMat)
			mf.MatID = 0;
	}
}

void MaxMesh::PrepareVertexColours(Mesh* pMesh)
{
	VertexColors::GetVertexColorsForMesh(this->vertexColours, *pMesh);
	VertexColors::GetVertexAlphasForMesh(this->vertexAlpha, *pMesh);
}

// Generate new mesh according to smoothing groups.
// Duplicate vertices if normals not shared.
void MaxMesh::SplitSmoothGroups(Mesh& mesh, Mesh& outMesh, std::vector<int>& arrVertexMap, BOOL negate, Point3* pActualVerts)
{
	if (!pActualVerts)
		pActualVerts = mesh.verts;

	Tab<VNormal> vnorms;
	MeshUtils::CalcVertexNormals( vnorms,&mesh,true,negate, pActualVerts);

	int i,j;
	int nVerts = mesh.getNumVerts();
	int nFaces = mesh.getNumFaces();
	Face		*Faces	  = mesh.faces;
	Point3		*Verts	  = pActualVerts;

	std::vector<Face> newFaces;
	std::vector<Point3> newVerts;
	std::vector<Point3> newNormals;
	std::vector<VVertex> vverts;

	newFaces.resize(nFaces);
	newVerts.resize(nVerts);
	newNormals.resize(nVerts);
	arrVertexMap.resize(nVerts);

	// Inistialize vertices.
	vverts.resize(nVerts);
	for (i = 0; i < nVerts; ++i)
	{
		newVerts[i] = Verts[i];
		arrVertexMap[i] = i;
		vverts[i] = VVertex();
		newNormals[i] = vnorms[i].norm;
	}

	for (i = 0; i < nFaces; i++) 
	{
		newFaces[i] = Faces[i];
		for (j = 0; j < 3; j++)
		{
			int v = Faces[i].v[j];
			Point3 vn = vnorms[v].GetNormal( Faces[i].smGroup );
			vverts[v].AddVertex( v,Verts[v],vn,Faces[i].smGroup );
		}
	}

	for (i = 0; i < nVerts; i++)
	{
		newNormals[vverts[i].index] = vverts[i].norm;
		for (VVertex *vv = vverts[i].next; vv != 0; vv = vv->next)
		{
			int index = vv->index;
			int newIndex = newVerts.size();
			vv->index = newIndex;
			newVerts.push_back(Verts[index]);
			newNormals.push_back(vv->norm);

			arrVertexMap.push_back(index);
		}
	}

	for (i = 0; i < nFaces; i++) 
	{
		for (j = 0; j < 3; j++)
		{
			int v = newFaces[i].v[j];
			int newIndex = vverts[v].GetVertex( Faces[i].smGroup ).index;
			newFaces[i].v[j] = newIndex;
		}
	}

	outMesh.DeepCopy( &mesh,ALL_CHANNELS );

	// Copy new verts.
	outMesh.setNumVerts( newVerts.size() );
	for (i = 0; i < (int)newVerts.size(); i++)
	{
		outMesh.setVert( i,newVerts[i] );
		outMesh.setNormal( i,newNormals[i] );
	}

	// Copy new faces
	outMesh.setNumFaces( newFaces.size(), TRUE);
	for (i = 0; i < (int)newFaces.size(); i++)
	{
		outMesh.faces[i] = newFaces[i];
	}
}

// Generate the default texture coordinates for the mesh
void MaxMesh::GenerateDefaultUVs(Mesh& mesh)
{
	unsigned numVerts = mesh.getNumVerts();
	mesh.setNumTVerts(numVerts);
	// make up the cubic coordinates
	for (unsigned nVert = 0; nVert < numVerts; ++nVert)
	{
		Point3& v = mesh.getVert(nVert);
		Point3	r (fabs(v.x), fabs(v.y), fabs(v.z));
		UVVert uv (0,0,0);
		if (r.x > 1e-3 || r.y > 1e-3 || r.z > 1e-3)
		{
			if (r.x > r.y)
			{
				if (r.x > r.z)
				{
					// X rules
					uv.x = v.y/r.x;
					uv.y = v.z/r.x;
					uv.z = v.x;
				}
				else
				{
					// Z rules
					uv.x = v.x / r.z;
					uv.y = v.y / r.z;
					uv.z = v.z;
				}
			}
			else
			{
				// r.x < r.y
				if (r.y > r.z)
				{
					// Y rules
					uv.x = v.x / r.y;
					uv.y = v.z / r.y;
					uv.z = v.y;
				}
				else
				{
					// Z rules
					uv.x = v.x / r.z;
					uv.y = v.y / r.z;
					uv.z = v.z;
				}
			}
		}
		// now the texture coordinates are in the range [-1,1]
		// we want normalized to 0..1 texture coordinates
		uv.x = (uv.x + 1)/2;
		uv.y = (uv.y + 1)/2;

		mesh.setTVert(nVert, uv);
	}

	unsigned numFaces = mesh.getNumFaces();
	mesh.setNumTVFaces (numFaces);
	TVFace* pTVFaces = mesh.tvFace;
	Face* pFaces = mesh.faces;

	for (unsigned nTVFace = 0; nTVFace < numFaces; ++nTVFace)
	{
		for (unsigned i = 0; i < 3; ++i)
			pTVFaces[nTVFace].t[i] = pFaces[nTVFace].v[i];
	}

	// clean up the face material indices: they have no meaning as long as we don't have UVs in Max
	for (unsigned nFace = 0; nFace < numFaces; ++nFace)
		mesh.setFaceMtlIndex(nFace, 0);
}

void MaxMesh::CreateSkinningInfo(bool bAllowBlending, NameList& BoneList)
{
	ISkinningInfoSource* pSkinningInfoSource = FindSkinningInfoSource(this->pMaxNode, BoneList, bAllowBlending);
	if (pSkinningInfoSource != 0)
		this->pSkinningInfo = new MaxSkinningInfo(pSkinningInfoSource, this->pMaxNode, &this->vertsForBoneSavingRoutines[0], &this->vertsForBoneSavingRoutinesMap[0], int(this->vertsForBoneSavingRoutinesMap.size()));
}

void MaxMesh::CreateMorphData(float fMorphMinOffset)
{
	MorphR3* pMorpher = FindMorpherModifier(this->pMaxNode);
	if (pMorpher != 0)
		this->pMorphData = new MaxMorphData(pMorpher, fMorphMinOffset);
}

MaxMesh* MaxMesh::Create(IErrorReporter* pErrorReporter, INode* pMaxNode, MaxMaterial* pMaterial, bool bGenerateDefaultUVs, bool bAllowBlending, NameList& BoneList, float fMorphMinOffset)
{
	MaxMesh* pMesh = 0;

	// TODO: I'm pretty sure some of these operations create temporary objects that need to be deleted.
	Object *der_obj;
	if (!pMaxNode || !(der_obj = pMaxNode->EvalWorldState(0).obj))
		return 0;

	Object* pBaseObject = GetObjectBeforeSkin(pMaxNode);

	TimeValue	time	= 0;
	Object* obj = der_obj->Eval(time).obj;
	if (!obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
		return 0;
	TriObject* tri = (TriObject*)obj->ConvertToType(time, Class_ID(TRIOBJ_CLASS_ID, 0));
	Mesh* mesh	= &tri->mesh;

	if (mesh != 0)
	{
		Matrix3 tm(1);

		Point3 pos = pMaxNode->GetObjOffsetPos();
		tm.PreTranslate(pos);
		Quat quat = pMaxNode->GetObjOffsetRot();
		PreRotateMatrix(tm, quat);
		ScaleValue scaleValue = pMaxNode->GetObjOffsetScale();
		ApplyScaling(tm, scaleValue);

		pMesh = new MaxMesh(pErrorReporter, pMaxNode, mesh, pMaterial, tm, bGenerateDefaultUVs, bAllowBlending, BoneList, fMorphMinOffset);
	}

	if(obj != tri) 
		tri->DeleteThis();

	return pMesh;
}
