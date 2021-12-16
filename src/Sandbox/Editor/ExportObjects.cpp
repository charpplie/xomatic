#include "stdafx.h"
#include "Objects/SelectionGroup.h"
#include "Objects/BaseObject.h"
#include "Brush/SolidBrushObject.h"
#include "Geometry/EdGeometry.h"
#include "IIndexedMesh.h"
#include "OBJExporter1.h"
#include "I3DEngine.h"
#include "Objects/Entity.h"
#include "Material/Material.h"
#include "ExportObjects.h"

namespace
{


void SetTexture(char * outName, const IRenderShaderResources * pRes, int slot)
{
	SEfResTexture * pTex = pRes->GetTexture(slot);
	if(pTex)
	{
		GetCurrentDirectory(MAX_PATH, outName);
		strcat(outName, "\\");
		strcat(outName, Path::GamePathToFullPath(pTex->m_Name.c_str()));
	}
}


void AddMaterialToModel(COBJExporter & cModel, CBaseObject * obj, CMaterial * pMtl)
{
	if(pMtl)
	{
		CExpMtl mtl;
		strcpy(mtl.m_name, pMtl->GetFullName());
		
		IMaterial * matInfo = pMtl->GetMatInfo();
		IRenderShaderResources * pRes = matInfo->GetShaderItem().m_pShaderResources;
		if(!pRes)
			return;
		mtl.dif  = pRes->GetDiffuseColor();
		mtl.spec = pRes->GetSpecularColor();
		mtl.trans= pRes->GetOpacity();
		mtl.shin = pRes->GetSpecularShininess()/255.0f;

		SetTexture(mtl.m_map_Kd,   pRes, EFTT_DIFFUSE);
		SetTexture(mtl.m_map_Ks,   pRes, EFTT_SPECULAR);
		SetTexture(mtl.m_map_Ns,   pRes, EFTT_GLOSS);
		SetTexture(mtl.m_map_d,    pRes, EFTT_OPACITY);
		SetTexture(mtl.m_map_bump, pRes, EFTT_BUMP);
		SetTexture(mtl.m_map_decal,pRes, EFTT_DECAL_OVERLAY);
		SetTexture(mtl.m_map_disp, pRes, EFTT_BUMP_HEIGHT);

		cModel.AddMtl(mtl);
	}
	else
	{
		CExpMtl mtl;
		strcpy(mtl.m_name, obj->GetName());
		cModel.AddMtl(mtl);
	}
}



void AddMeshToModel(COBJExporter & cModel, CBaseObject * obj, IIndexedMesh * pMesh)
{
	if(!pMesh)
		return;
	cModel.StartNewObject(obj->GetName());

	CMaterial * pMtl = obj->GetRenderMaterial();

	IIndexedMesh::SMeshDesc meshDesc;
	pMesh->GetMesh(meshDesc);

	const Matrix34 &tm = obj->GetWorldTM();
	Matrix34 invTm = tm.GetInverted();

	for(int v=0; v<meshDesc.m_nVertCount; v++)
	{
		CVector3D vec;
		Vec3 tmp = tm.TransformPoint(meshDesc.m_pVerts[v]);
		vec.fX = tmp.x*100;
		vec.fY = tmp.y*100;
		vec.fZ = tmp.z*100;
		cModel.AddVertex(vec);

		Vec3 norm = invTm.TransformPoint(meshDesc.m_pNorms[v]);
		norm = norm.normalize();
		vec.fX = norm.x;
		vec.fY = norm.y;
		vec.fZ = norm.z;
		cModel.AddNormal(vec);
	}

	for(int v=0; v<meshDesc.m_nCoorCount; v++)
	{
		CTexCoord2D tc;
		tc.fU = meshDesc.m_pTexCoord[v].s;
		tc.fV = meshDesc.m_pTexCoord[v].t;
		cModel.AddTexCoord(tc);
	}

	if(pMtl && pMtl->IsMultiSubMaterial() && pMesh->GetSubSetCount()
		&& !(pMesh->GetSubSetCount()==1 && pMesh->GetSubSet(0).nNumIndices==0))
	{
		for( int i=0; i<pMesh->GetSubSetCount(); i++)
		{
			cModel.StartNewSubMesh();
			SMeshSubset & sms = pMesh->GetSubSet(i);

			int nTris = sms.nNumIndices/3;
			for(int f=0; f<nTris; f++)
			{
				CFace face;
				face.iVertexIndices[0]=meshDesc.m_pIndices[sms.nFirstIndexId + f*3+0];
				face.iVertexIndices[1]=meshDesc.m_pIndices[sms.nFirstIndexId + f*3+1];
				face.iVertexIndices[2]=meshDesc.m_pIndices[sms.nFirstIndexId + f*3+2];

				face.iTexCoordIndices[0]=meshDesc.m_pIndices[sms.nFirstIndexId + f*3+0];
				face.iTexCoordIndices[1]=meshDesc.m_pIndices[sms.nFirstIndexId + f*3+1];
				face.iTexCoordIndices[2]=meshDesc.m_pIndices[sms.nFirstIndexId + f*3+2];

				cModel.AddFace(face);
			}

			CMaterial * pSubMtl = 0;
			if(sms.nMatID < pMtl->GetSubMaterialCount())
				pSubMtl = pMtl->GetSubMaterial(sms.nMatID);
			AddMaterialToModel(cModel, obj, pSubMtl);
		}
	}
	else
	{
		cModel.StartNewSubMesh();
		if (meshDesc.m_nFaceCount == 0 && meshDesc.m_nIndexCount != 0 && meshDesc.m_pIndices != 0)
		{
			int nTris = meshDesc.m_nIndexCount/3;
			for(int f=0; f<nTris; f++)
			{
				CFace face;
				face.iVertexIndices[0]=meshDesc.m_pIndices[f*3+0];
				face.iVertexIndices[1]=meshDesc.m_pIndices[f*3+1];
				face.iVertexIndices[2]=meshDesc.m_pIndices[f*3+2];

				face.iTexCoordIndices[0]=meshDesc.m_pIndices[f*3+0];
				face.iTexCoordIndices[1]=meshDesc.m_pIndices[f*3+1];
				face.iTexCoordIndices[2]=meshDesc.m_pIndices[f*3+2];

				cModel.AddFace(face);
			}
		}
		else
		{
			for(int f=0; f<meshDesc.m_nFaceCount; f++)
			{
				CFace face;
				face.iVertexIndices[0]=meshDesc.m_pFaces[f].v[0];
				face.iVertexIndices[1]=meshDesc.m_pFaces[f].v[1];
				face.iVertexIndices[2]=meshDesc.m_pFaces[f].v[2];

				face.iTexCoordIndices[0]=meshDesc.m_pFaces[f].t[0];
				face.iTexCoordIndices[1]=meshDesc.m_pFaces[f].t[1];
				face.iTexCoordIndices[2]=meshDesc.m_pFaces[f].t[2];

				cModel.AddFace(face);
			}
		}

		AddMaterialToModel(cModel, obj, pMtl);
	}
}


bool ExportObject( COBJExporter & cModel, CBaseObject *obj )
{
	if(obj->GetType() == OBJTYPE_GROUP)
	{
		for(int i=0; i<obj->GetChildCount(); i++)
			if(!ExportObject( cModel, obj->GetChild(i) ))
				return false;
		return true;
	}

	CEdGeometry * pEdGeom = obj->GetGeometry();
	IIndexedMesh * pMesh=0;

	if(obj->GetType() == OBJTYPE_SOLID)
	{
		CSolidBrushObject *sobj = (CSolidBrushObject*)obj;
		pMesh = GetIEditor()->Get3DEngine()->CreateIndexedMesh();
		// Using not optimized mesh
		if (sobj->GetBrush())
			sobj->GetBrush()->GenerateIndexMesh(pMesh);
	}

	if(!pMesh && pEdGeom)
		pMesh = pEdGeom->GetIndexedMesh();

	if(!pMesh && obj->GetType() == OBJTYPE_ENTITY)
	{
		CEntity * pEnt = (CEntity *)obj;
		IEntity * pIEnt = pEnt->GetIEntity();
		if(pIEnt)
		{
			IStatObj *pStatObj = pIEnt->GetStatObj(0);
			if(pStatObj)
			{
				if (pStatObj && pStatObj->GetParentObject())
					pStatObj = pStatObj->GetParentObject();

				if(pStatObj->GetSubObjectCount())
				{
					for(int i = 0; i<pStatObj->GetSubObjectCount(); i++)
					{
						IStatObj::SSubObject * pSubObj = pStatObj->GetSubObject(i);
						if(pSubObj && pSubObj->nType==STATIC_SUB_OBJECT_MESH && pSubObj->pStatObj)
						{
							pMesh = pSubObj->pStatObj->GetIndexedMesh(true);
						}
					}
				}
				else
				{
					pMesh = pStatObj->GetIndexedMesh(true);
				}
			}
		}
	}

	if(!pMesh)
	{
		CLogFile::WriteLine("Error while getting the mesh from object!");
		char out[256];
		sprintf(out, "Critical error during exporting! \nObject: %s.", (const char*)obj->GetName() );
		AfxMessageBox(out);
		return false;
	}

	AddMeshToModel(cModel, obj, pMesh);
	return true;
}

}




bool CExportObjects::Export( const char *pszFileName )
{
	COBJExporter cModel;

	CSelectionGroup *sel = GetIEditor()->GetSelection();
	for(int i=0; i<sel->GetCount(); i++)
	{
		if(!ExportObject( cModel, sel->GetObject(i) ))
			return false;
	}

	// Write the model into the file
	if (!cModel.WriteOBJ(pszFileName))
	{
		CLogFile::WriteLine("Error while exporting part of the heightmap as OBJ model !");
		AfxMessageBox("Crititcal error during exporting !");
		return false;
	}

	return true;
}