#include "stdafx.h"																		// precompiled header
#include "SimpleIndexedMesh.h"												//



bool CSimpleIndexedMesh::Allocate( const uint32 dwTriangleCount, const uint32 dwPosCount, const uint32 dwNormalCount, const uint32 dwUVCount )
{
	FreeData();

	try
	{
		if(dwTriangleCount) m_pFaces = new CObjFace[dwTriangleCount];
		if(dwPosCount) m_pVerts = new CObjVert[dwPosCount];
		if(dwUVCount) m_pCoors = new CObjCoor[dwUVCount];
		if(dwNormalCount) m_pNorms = new CObjNorm[dwNormalCount];

		m_FaceCount=dwTriangleCount;m_VertCount=dwPosCount;m_CoorCount=dwUVCount;m_NormCount=dwNormalCount;
	}
	catch( CMemoryException * )
	{
		FreeData();

		MessageBox(0,"can't allocate memory","CSimpleIndexedMesh::Allocate 1",MB_OK);
		return false;
	}

	return true;
}




uint32 CSimpleIndexedMesh::GetNormalCount() const
{
	return m_NormCount;
}


bool CSimpleIndexedMesh::AllocateNormals( const uint32 dwNormalCount )
{
	delete m_pNorms;m_pNorms=0;

	try
	{
		if(dwNormalCount) m_pNorms = new CObjNorm[dwNormalCount];

		m_NormCount=dwNormalCount;
	}
	catch( CMemoryException * )
	{
		FreeData();

		MessageBox(0,"can't allocate memory","CSimpleIndexedMesh::AllocateNormals",MB_OK);
		return false;
	}

	return true;
}





void CSimpleIndexedMesh::SetPos( const uint32 dwIndex, const Vec3 &rValue )
{
	assert(dwIndex<m_VertCount);
	m_pVerts[dwIndex] = CObjVert(rValue.x,rValue.y,rValue.z);
}

Vec3 CSimpleIndexedMesh::GetPos( const uint32 dwIndex ) const
{
	assert(dwIndex<m_VertCount);

	CObjVert &rRef = m_pVerts[dwIndex];

	return Vec3(rRef.x,rRef.y,rRef.z);
}


void CSimpleIndexedMesh::SetNormal( const uint32 dwIndex, const Vec3 &rValue )
{
	assert(dwIndex<m_NormCount);
/*
	assert(rValue.x>=-1 && rValue.x<=1);		// bad input ? seems default max OBJ exporter does that
	assert(rValue.y>=-1 && rValue.y<=1);
	assert(rValue.z>=-1 && rValue.z<=1);
*/
	Vec3 vNorm = rValue.GetNormalizedSafe();

	m_pNorms[dwIndex] = CObjNorm(vNorm.x,vNorm.y,vNorm.z);
}

Vec3 CSimpleIndexedMesh::GetNormal( const uint32 dwIndex ) const
{
	assert(dwIndex<m_NormCount);
	
	CObjNorm &rRef = m_pNorms[dwIndex];
	
	return Vec3(rRef.x,rRef.y,rRef.z);
}


void CSimpleIndexedMesh::SetUV( const uint32 dwIndex, const Vec2 &rValue )
{
	assert(dwIndex<m_CoorCount);
	m_pCoors[dwIndex] = CObjCoor(rValue.x,rValue.y);
}





void CSimpleIndexedMesh::SetTriangle( const uint32 dwIndex, SMeshOutputTriangle &rTri )
{
	assert(dwIndex<m_FaceCount);
	CObjFace &rOut = m_pFaces[dwIndex];

	rOut.shader_id = rTri.m_dwMaterialId;

	for(uint32 dwI=0;dwI<3;++dwI)
	{
		rOut.v[dwI] = rTri.m_PosIndex[dwI];
		rOut.t[dwI] = rTri.m_UVIndex[dwI];
		rOut.n[dwI] = rTri.m_NormalIndex[dwI];
	}
}


uint32 CSimpleIndexedMesh::GetTriangleCount() const
{
	return m_FaceCount;
}

void CSimpleIndexedMesh::GetPosIndices( const uint32 dwTri, uint32 dwPosIndex[3] ) const
{
	CObjFace &rFace = m_pFaces[dwTri];

	dwPosIndex[0] = rFace.v[0];
	dwPosIndex[1] = rFace.v[1];
	dwPosIndex[2] = rFace.v[2];
}

void CSimpleIndexedMesh::SetNormalIndices( const uint32 dwTri, const uint32 dwNormalIndex[3] )
{
	assert(dwTri<m_FaceCount);
	CObjFace &rFace = m_pFaces[dwTri];

	rFace.n[0] = dwNormalIndex[0];
	rFace.n[1] = dwNormalIndex[1];
	rFace.n[2] = dwNormalIndex[2];
}

void CSimpleIndexedMesh::GetNormalIndices( const uint32 dwTri, uint32 dwNormalIndex[3] ) const
{
	assert(dwTri<m_FaceCount);
	CObjFace &rFace = m_pFaces[dwTri];

	dwNormalIndex[0] = rFace.n[0];
	dwNormalIndex[1] = rFace.n[1];
	dwNormalIndex[2] = rFace.n[2];
}


uint32 CSimpleIndexedMesh::GetUVCount() const
{
	return m_CoorCount;
}

CSimpleIndexedMesh::CSimpleIndexedMesh()
{
	m_pFaces=0;m_pVerts=0;m_pCoors=0;m_pNorms=0;m_pMaterials=0;
	m_FaceCount=0;m_VertCount=0;m_CoorCount=0;m_NormCount=0;m_MaterialCount=0;
}


void CSimpleIndexedMesh::FreeData()
{
	delete [] m_pFaces;
	delete [] m_pVerts;
	delete [] m_pCoors;
	delete [] m_pNorms;
	delete [] m_pMaterials;

	m_pFaces=0;m_pVerts=0;m_pCoors=0;m_pNorms=0;m_pMaterials=0;
	m_FaceCount=0;m_VertCount=0;m_CoorCount=0;m_NormCount=0;m_MaterialCount=0;
}


CSimpleIndexedMesh::~CSimpleIndexedMesh()
{
	FreeData();
}



bool CSimpleIndexedMesh::ExportAsOBJ( const char *inszOBJPathName, const bool bWavefrontLikeLayout )
{
	MarkDuplicatedMaterialNames();	// is necessary because material IDs may use the same Material

	char szMATPathName[1024];

	// build .mtl filename
	{
		int len=(int)strlen(inszOBJPathName);

		if(len>=1024)return false;			// filename too long
		if(len<=3)return false;			// filename too short

		strcpy(szMATPathName,inszOBJPathName);
		szMATPathName[len-3]='m';
		szMATPathName[len-2]='t';
		szMATPathName[len-1]='l';
	}
	
	// write material description
	{
		FILE *hOut=fopen(szMATPathName,"w");		if(!hOut)return false;

		fprintf(hOut,"# SimpleIndexedMesh::ExportAsOBJ()\n#\n");
		if(!m_pMaterials)
		{
/*
			Ns = Phong specular component. Ranges from 0 to 1000. (I've seen various statements about this range (see below))
			Kd = Diffuse color weighted by the diffuse coefficient. (1,1,1) is white
			Ka = Ambient color weighted by the ambient coefficient. (1,1,1) is white
			Ks = Specular color weighted by the specular coefficient. (1,1,1) is white
			d = Dissolve factor (pseudo-transparency). Values are from 0-1. 0 is completely transparent, 1 is opaque.
			Ni = Refraction index. Values range from 1 upwards. A value of 1 will cause no refraction. A higher value implies refraction.
			illum = (0, 1, or 2) 0 to disable lighting, 1 for ambient & diffuse only (specular color set to black), 2 for full lighting (see below)
			sharpness = ? (see below)
			map_Kd = Diffuse color texture map.
			map_Ks = Specular color texture map.
			map_Ka = Ambient color texture map.
			map_Bump = Bump texture map.
			map_d = Opacity texture map.
			refl = reflection type and filename (?)
*/

			fprintf(hOut,"#\n");
			fprintf(hOut,"newmtl MatName\n");
			fprintf(hOut,"Ka  0.4 0.7 0.4\n");
			fprintf(hOut,"Kd  0.4 0.7 0.4\n");
			fprintf(hOut,"Ks  0.5 0.6 0.5\n");
			fprintf(hOut,"d  1\n");	
			fprintf(hOut,"Ns  0\n");
			fprintf(hOut,"illum 2\n");
			fprintf(hOut,"map_Kd texture.tga\n");
			fprintf(hOut,"#\n");
		}
		else
		{
			for(uint32 i=0;i<m_MaterialCount;i++)
			if((m_pMaterials[i].m_dwFlags&1) == 0)				// not marked as duplicate name
			{
				fprintf(hOut,"#\n");
				fprintf(hOut,"newmtl %s\n",m_pMaterials[i].m_sMaterialName.c_str());
				fprintf(hOut,"Ka  %.3f %.3f %.3f\n",m_pMaterials[i].m_AmbientColor[0],m_pMaterials[i].m_AmbientColor[1],m_pMaterials[i].m_AmbientColor[2]);
				fprintf(hOut,"Kd  %.3f %.3f %.3f\n",m_pMaterials[i].m_DiffuseColor[0],m_pMaterials[i].m_DiffuseColor[1],m_pMaterials[i].m_DiffuseColor[2]);
				fprintf(hOut,"Ks  %.3f %.3f %.3f\n",m_pMaterials[i].m_SpecularColor[0]*m_pMaterials[i].m_fShiniStrength,
																						m_pMaterials[i].m_SpecularColor[1]*m_pMaterials[i].m_fShiniStrength,
																						m_pMaterials[i].m_SpecularColor[2]*m_pMaterials[i].m_fShiniStrength);
				fprintf(hOut,"d  %.3f\n",m_pMaterials[i].m_fTransparency);
				fprintf(hOut,"Ns  %.3f\n",m_pMaterials[i].m_fShininess);
				fprintf(hOut,"illum 2\n");
				if(!m_pMaterials[i].m_sDiffuseTexturePath.empty())fprintf(hOut,"map_Kd %s\n",RemovePath(m_pMaterials[i].m_sDiffuseTexturePath.c_str()));
				if(!m_pMaterials[i].m_sBumpTexturePath.empty())fprintf(hOut,"bump %s\n",RemovePath(m_pMaterials[i].m_sBumpTexturePath.c_str()));
				fprintf(hOut,"#\n");
			}
		}

		fprintf(hOut,"#\n# EOF\n");
		fclose(hOut);
	}

	FILE *hOut=fopen(inszOBJPathName,"w");		if(!hOut)return false;

	fprintf(hOut,"# SimpleIndexedMesh::ExportAsOBJ()\n#\n");
	fprintf(hOut,"mtllib ./%s\n\n",RemovePath(szMATPathName));

	for(uint32 v=0;v<m_VertCount;v++)
	{
		if(bWavefrontLikeLayout)
			fprintf(hOut,"v %.4f %.2f %.2f\n",m_pVerts[v].x,m_pVerts[v].z,-m_pVerts[v].y);
		 else
			fprintf(hOut,"v %.4f %.2f %.2f\n",m_pVerts[v].x,m_pVerts[v].y,m_pVerts[v].z);
	}
	if(m_VertCount)fprintf(hOut,"# %d vertices\n\n",m_VertCount);

	for(uint32 c=0;c<m_CoorCount;c++)
	{
		// 1.0f- because texture coordinates in MAX have (left,bottom) as origin 
//		fprintf(hOut,"vt %.4f %.4f 0\n",m_pCoors[c].s,1.0f-m_pCoors[c].t);
		fprintf(hOut,"vt %.4f %.4f 0\n",m_pCoors[c].s,m_pCoors[c].t);
	}
	if(m_CoorCount)fprintf(hOut,"# %d texture vertices\n\n",m_CoorCount);

	for(uint32 n=0;n<m_NormCount;n++)
	{
		if(bWavefrontLikeLayout)
			fprintf(hOut,"vn %.3f %.3f %.3f\n",m_pNorms[n].x,m_pNorms[n].z,-m_pNorms[n].y);
		 else
			fprintf(hOut,"vn %.3f %.3f %.3f\n",m_pNorms[n].x,m_pNorms[n].y,m_pNorms[n].z);
	}
	if(m_NormCount)fprintf(hOut,"# %d vertex normals\n\n",m_NormCount);

	uint32 dwPassCount = m_MaterialCount ? m_MaterialCount : 1;		// n passses for n materials or 1 pass for no materials

	for(uint32 dwCurrentMat=0;dwCurrentMat<dwPassCount;++dwCurrentMat)
	{
		if(m_MaterialCount)
			fprintf(hOut,"usemtl %s\n",m_pMaterials[dwCurrentMat].m_sMaterialName.c_str());
			else
			fprintf(hOut,"usemtl MatName\n");

		for(uint32 f=0;f<m_FaceCount;f++)
		{
			if(m_MaterialCount) if(dwCurrentMat!=m_pFaces[f].shader_id)continue;

			fprintf(hOut,"f ");

			for(int d=0;d<3;d++)
			{
				assert(m_pVerts);
				
				fprintf(hOut,"%d",m_pFaces[f].v[d]+1);
				if(m_pCoors)fprintf(hOut,"/%d",m_pFaces[f].t[d]+1);
				if(m_pNorms)fprintf(hOut,"/%d",m_pFaces[f].n[d]+1);

				fprintf(hOut," ");
			}
			fprintf(hOut,"\n");
		}
	}

	fprintf(hOut,"# %d faces\n\n",m_FaceCount);

	fclose(hOut);
	return true;
}

bool CSimpleIndexedMesh::AllocateMaterials( const  uint32 dwCount )
{
	delete m_pMaterials;m_pMaterials=0;
	m_MaterialCount=dwCount;

	if(m_MaterialCount)
	{
		m_pMaterials = new CSimpleMaterial[dwCount];

		if(!m_pMaterials)
		{
			m_MaterialCount=0;
			return false;
		}
	}

	return true;
}


void CSimpleIndexedMesh::SetMaterial( const uint32 dwIndex, const COBJMaterial &rMat )
{
	assert(dwIndex<m_MaterialCount);

	CSimpleMaterial &rDest = m_pMaterials[dwIndex];

	rDest.m_sMaterialName = rMat.m_sName;
	rDest.m_AmbientColor[0] = rMat.m_Ka.x;
	rDest.m_AmbientColor[1] = rMat.m_Ka.y;
	rDest.m_AmbientColor[2] = rMat.m_Ka.z;
	rDest.m_DiffuseColor[0] = rMat.m_Kd.x;
	rDest.m_DiffuseColor[1] = rMat.m_Kd.y;
	rDest.m_DiffuseColor[2] = rMat.m_Kd.z;
	rDest.m_sBumpTexturePath = rMat.m_sBumpTexture;
	rDest.m_sDiffuseTexturePath = rMat.m_sDiffuseTexture;
}


void CSimpleIndexedMesh::Debug() const
{
	char str[80];

	OutputDebugString("CSimpleIndexedMesh Debug=\n{\n");

	for(uint32 v=0;v<m_VertCount;v++)
	{
		sprintf(str,"  %d. Pos:(%.2f,%.2f,%.2f)\n",v,m_pVerts[v].x,m_pVerts[v].y,m_pVerts[v].z);
		OutputDebugString(str);
	}

	for(uint32 c=0;c<m_CoorCount;c++)
	{
		sprintf(str,"  %d. UV:(%.2f,%.2f)\n",c,m_pCoors[c].s,m_pCoors[c].t);
		OutputDebugString(str);
	}

	for(uint32 n=0;n<m_NormCount;n++)
	{
		sprintf(str,"  %d. Norm:(%.2f,%.2f,%.2f)\n",n,m_pNorms[n].x,m_pNorms[n].y,m_pNorms[n].z);
		OutputDebugString(str);
	}

	for(uint32 f=0;f<m_FaceCount;f++)
	{
		sprintf(str,"  %d. Face %d/%d/%d %d/%d/%d %d/%d/%d M:%d\n",f,
																									m_pFaces[f].v[0]+1,m_pFaces[f].t[0]+1,m_pFaces[f].n[0]+1,
																									m_pFaces[f].v[1]+1,m_pFaces[f].t[1]+1,m_pFaces[f].n[1]+1,
																									m_pFaces[f].v[2]+1,m_pFaces[f].t[2]+1,m_pFaces[f].n[2]+1,m_pFaces[f].shader_id);
		OutputDebugString(str);
	}

	OutputDebugString("}\n");
}