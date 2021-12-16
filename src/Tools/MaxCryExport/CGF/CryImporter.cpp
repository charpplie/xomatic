//===========================================================================*\
//	Part of Crytek Character Studio and Object Export Plug-in
//
//  Copyright: © Cuneyt Ozdas 2000, 2001
//				 cuneyt@cuneytozdas.com
//===========================================================================*/

#include "stdafx.h"
#include "CryImporter.h"

static CryImporterClassDesc CryImporterDesc;
ClassDesc2* GetCryImporterDesc() { return &CryImporterDesc; }

extern void Matrix3_to_CryMatrix2(float cm[4][4], Matrix3 &m3);
extern void CryMatrix_to_Matrix3(Matrix3 &m3, float cm[4][4]);


//--- CryImporter -------------------------------------------------------
CryImporter::CryImporter()
{
	ImportGeom		= true;
	ImportHelpers	= true;
	ImportLights	= true;
	DeleteScene		= false;
}

CryImporter::~CryImporter() 
{
}

void CryImporter::ShowAbout(HWND hWnd)
{			
}

INT_PTR CALLBACK CryImporterOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) 
{
	static CryImporter *imp = NULL;
	WORD cmd	= HIWORD(wParam); // notification code 
	WORD id		= LOWORD(wParam);         

	switch(message) 
	{
		case WM_INITDIALOG:
			imp = (CryImporter *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));
			CheckRadioButton(hWnd,IDC_MERGE_SCENE, IDC_REPLACE_SCENE, imp->DeleteScene?IDC_REPLACE_SCENE:IDC_MERGE_SCENE);
			CheckDlgButton(hWnd, IDC_IMPORT_MESH	,imp->ImportGeom);
			CheckDlgButton(hWnd, IDC_IMPORT_HELPER	,imp->ImportHelpers);
			CheckDlgButton(hWnd, IDC_IMPORT_LIGHT	,imp->ImportLights);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case IDOK:
				imp->ImportGeom		= IsDlgButtonChecked(hWnd, IDC_IMPORT_MESH) ? true : false;
				imp->ImportHelpers	= IsDlgButtonChecked(hWnd, IDC_IMPORT_HELPER)? true : false;
				imp->ImportLights	= IsDlgButtonChecked(hWnd, IDC_IMPORT_LIGHT)? true : false;
				imp->DeleteScene	= IsDlgButtonChecked(hWnd, IDC_REPLACE_SCENE)? true : false;

				EndDialog(hWnd, IDOK);
				break;

			case IDCANCEL:
				EndDialog(hWnd, IDCANCEL);
				break;
			}
			break;


		case WM_CLOSE:
			//EndDialog(hWnd, 0);
			return TRUE;
	}
	return FALSE;
}

int CryImporter::DoImport(const TCHAR *filename,ImpInterface *ii, Interface *gi, BOOL suppressPrompts)
{
	SupressPrompts = suppressPrompts ? true : false;

	iip				= ii;
	ip				= gi;
	ImportGeom		= true;
	ImportHelpers	= true;
	ImportLights	= true;
	DeleteScene		= false;
	
	if(!SupressPrompts)
	{
		int res=DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_IMPORT_PANEL), GetActiveWindow(), CryImporterOptionsDlgProc, (LPARAM)this);
		if(res==IDCANCEL) return TRUE;
	}

	//Open File
	f=fopen(filename,"rb");
	if(!f)
	{
		if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Error opening cgf file",MB_OK);
		return FALSE;
	}

	//Read the Header
	int res=fread(&FileHeader,sizeof(FileHeader),1,f);
	if(res!=1)
	{
		if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Error reading cgf file",MB_OK);
		return FALSE;
	}

	//check the file signature & type
	if(strcmp(FileHeader.Signature,FILE_SIGNATURE) || FileHeader.FileType != FileType_Geom)
	{
		if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","File is not a cgf File",MB_OK);
		return FALSE;
	}

	//check the file version
	int chunk_header_size=0;
	if(FileHeader.Version == 0x0744)
	{
		chunk_header_size = sizeof(CHUNK_HEADER);
	}
	else if(FileHeader.Version == 0x0623)
	{
		chunk_header_size = sizeof(CHUNK_HEADER_0623);
	}
	else
	{
		if(!SupressPrompts) MessageBox(GetActiveWindow(),"Obsolete file version","This is an old cgf file. This version can not be imported",MB_OK);
		return FALSE;
	}

	//Read the Chunk Table
	ChunkList.Reset();
	ChunkTable.ZeroCount();
	fseek(f,FileHeader.ChunkTableOffset,SEEK_SET);
	int n_chunks;
	res=fread(&n_chunks,sizeof(n_chunks),1,f);
	if(res!=1)
	{
		if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read chunk table",MB_OK);
		return FALSE;
	}
	for(int i=0;i<n_chunks;i++)
	{
		CHUNK_HEADER ch;
		res=fread(&ch,chunk_header_size,1,f);
		if(res!=1)
		{
			if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read chunk table",MB_OK);
			return FALSE;
		}

		if(FileHeader.Version == 0x0623) ch.ChunkID = -1;
		ChunkTable.Append(1,&ch);
	}

	if(DeleteScene) iip->NewScene();
	
	//Read the Chunks

	if(ImportGeom)
	{
		if(!ReadGeomNameListChunks())	return FALSE;	//for versions < 0744
		if(!ReadMaterialListChunks())	return FALSE;	//for versions < 0744
		if(!ReadMtlChunks())			return FALSE;	//for versions >= 0744
		if(!ReadMeshChunks())			return FALSE;
	}

	if(ImportHelpers)
		if(!ReadHelperChunks())			return FALSE;
	
	if(ImportLights)
		if(!ReadLightChunks())			return FALSE;

	if(!ReadNodeChunks())				return FALSE;	//for version >= 0744
	
	if(!ReadScenePropChunks())			return FALSE;

	if(FileHeader.Version>=0x0744 && ImportGeom)
	{
		if(!BuildNodeHierarchy())		return FALSE;
		if(!BuildMtlHierarchy())		return FALSE;
	}


	ChunkTable.ZeroCount();
	GeomNames.ZeroCount();
	Materials.ZeroCount();
	ip = NULL;
	iip= NULL;
	fclose(f);f=NULL;
	return TRUE;
}

//============================================================================================================================
// ChunkType_GeomNameList
//============================================================================================================================
BOOL CryImporter::ReadGeomNameListChunks()
{
	int res;
	int n_chunks = ChunkTable.Count();
	for(int i=0;i<n_chunks;i++)
	{
		CHUNK_HEADER &ch = ChunkTable[i];
		if(ch.ChunkType != ChunkType_GeomNameList) continue;

		switch(ch.ChunkVersion)
		{
		case 0x0201:
			{
				fseek(f,ch.FileOffset,SEEK_SET);
				GEOMNAMELIST_CHUNK_DESC_0201 gc;
				res=fread(&gc,sizeof(gc),1,f);
				if(res!=1)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read geom name list chunk",MB_OK);
					return FALSE;
				}

				for(int j=0;j<gc.nEntities;j++)
				{
					NAME_ENTITY ne;
					res=fread(&ne,sizeof(ne),1,f);
					if(res!=1)
					{
						if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read geom name list chunk",MB_OK);
						return FALSE;
					}

					GeomNames.Append(1,&ne);
				}
			}
			break;
		
		default:
			if(!SupressPrompts)
			{
				int res=MessageBox(GetActiveWindow(),"Error","Unknown chunk version for ChunkType_GeomNameList detected",MB_OK);
				return FALSE;
			}
		}
	}
	return TRUE;
}


//============================================================================================================================
// ChunkType_Mtl
//============================================================================================================================
BOOL CryImporter::ReadMtlChunks()
{
	int res;
	int n_chunks = ChunkTable.Count();
	for(int i=0;i<n_chunks;i++)
	{
		CHUNK_HEADER &ch = ChunkTable[i];
		if(ch.ChunkType != ChunkType_Mtl) continue;

		switch(ch.ChunkVersion)
		{
		case 0x0744:
			{
				fseek(f,ch.FileOffset,SEEK_SET);
				MTL_CHUNK_DESC_0746 mc;
				res=fread(&mc,sizeof(mc),1,f);
				if(res!=1)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read material chunk",MB_OK);
					return FALSE;
				}


				switch(mc.MtlType)
				{

				case MTL_STANDARD:
					{
						//create the material
						StdMat *mat=NewDefaultStdMat();
						mat->SetName(mc.name);
						mat->SetAmbient(  Color(mc.col_a.r/255.0, mc.col_a.g/255.0, mc.col_a.b/255.0),0); 
						mat->SetDiffuse(  Color(mc.col_d.r/255.0, mc.col_d.g/255.0, mc.col_d.b/255.0),0);
						mat->SetSpecular( Color(mc.col_s.r/255.0, mc.col_s.g/255.0, mc.col_s.b/255.0),0);
						mat->SetDynamicsProperty(0, 0, DYN_BOUNCE			,mc.Dyn_Bounce);
						mat->SetDynamicsProperty(0, 0, DYN_STATIC_FRICTION  ,mc.Dyn_StaticFriction);
						mat->SetDynamicsProperty(0, 0, DYN_SLIDING_FRICTION ,mc.Dyn_SlidingFriction);
						
						if(strlen(mc.tex_d.name))
						{
							BitmapTex *tex=NewDefaultBitmapTex();
							TextureMap3 &m=mc.tex_d;
							tex->SetMapName(mc.tex_d.name);
							StdUVGen *stduv=GetTextureStdUVGen(tex);
							if(stduv)
							{
								stduv->SetUOffs	(m.uoff_val,0);
								stduv->SetUScl	(m.uscl_val,0);
								stduv->SetUAng	(m.urot_val,0);
								stduv->SetVOffs	(m.voff_val,0);
								stduv->SetVScl	(m.vscl_val,0);
								stduv->SetVAng	(m.vrot_val,0);
								stduv->SetWAng	(m.wrot_val,0);
								stduv->SetFlag	(U_WRAP,	m.utile);
								stduv->SetFlag	(V_WRAP,	m.vtile);
								stduv->SetFlag	(U_MIRROR,	m.umirror);
								stduv->SetFlag	(V_MIRROR,	m.vmirror);

							}
							mat->SetSubTexmap(ID_DI,tex);
						}

						if(strlen(mc.tex_b.name))
						{
							BitmapTex *tex=NewDefaultBitmapTex();
							TextureMap3 &m=mc.tex_b;
							tex->SetMapName(mc.tex_b.name);
							StdUVGen *stduv=GetTextureStdUVGen(tex);
							if(stduv)
							{
								stduv->SetUOffs	(m.uoff_val,0);
								stduv->SetUScl	(m.uscl_val,0);
								stduv->SetUAng	(m.urot_val,0);
								stduv->SetVOffs	(m.voff_val,0);
								stduv->SetVScl	(m.vscl_val,0);
								stduv->SetVAng	(m.vrot_val,0);
								stduv->SetWAng	(m.wrot_val,0);
								stduv->SetFlag	(U_WRAP,	m.utile);
								stduv->SetFlag	(V_WRAP,	m.vtile);
								stduv->SetFlag	(U_MIRROR,	m.umirror);
								stduv->SetFlag	(V_MIRROR,	m.vmirror);
							}
							mat->SetSubTexmap(ID_BU,tex);
						}

						if(strlen(mc.tex_o.name))
						{
							BitmapTex *tex=NewDefaultBitmapTex();
							TextureMap3 &m=mc.tex_o;
							tex->SetMapName(mc.tex_o.name);
							StdUVGen *stduv=GetTextureStdUVGen(tex);
							if(stduv)
							{
								stduv->SetUOffs	(m.uoff_val,0);
								stduv->SetUScl	(m.uscl_val,0);
								stduv->SetUAng	(m.urot_val,0);
								stduv->SetVOffs	(m.voff_val,0);
								stduv->SetVScl	(m.vscl_val,0);
								stduv->SetVAng	(m.vrot_val,0);
								stduv->SetWAng	(m.wrot_val,0);
								stduv->SetFlag	(U_WRAP,	m.utile);
								stduv->SetFlag	(V_WRAP,	m.vtile);
								stduv->SetFlag	(U_MIRROR,	m.umirror);
								stduv->SetFlag	(V_MIRROR,	m.vmirror);
							}
							mat->SetSubTexmap(ID_OP,tex);
						}

						ChunkList.Append(mat,&mc.chdr,false);
					}
					break;

				case MTL_MULTI:
					{
						MultiMtl *mat=NewDefaultMultiMtl();
						mat->SetName(mc.name);
						mat->SetNumSubMtls(mc.nChildren);

						ChunkList.Append(mat,&mc.chdr,false);
					}
					break;

				case MTL_2SIDED:
					break;

				default:
					{
						int res=MessageBox(GetActiveWindow(),"Error","Unknown material type encountered",MB_OK);
						return FALSE;
					}
				}
			}
			break;
		
		default:
			if(!SupressPrompts)
			{
				int res=MessageBox(GetActiveWindow(),"Error","Unknown chunk version for ChunkType_MtlList detected",MB_OK);
				return FALSE;
			}
		}
	}
	return TRUE;
}

//============================================================================================================================
// ChunkType_MtlList
//============================================================================================================================
BOOL CryImporter::ReadMaterialListChunks()
{
	int res;
	int n_chunks = ChunkTable.Count();
	for(int i=0;i<n_chunks;i++)
	{
		CHUNK_HEADER &ch = ChunkTable[i];
		if(ch.ChunkType != ChunkType_MtlList) continue;

		switch(ch.ChunkVersion)
		{
		case 0x0201:
			{
				fseek(f,ch.FileOffset,SEEK_SET);
				MTLLIST_CHUNK_DESC_0201 mc;
				res=fread(&mc,sizeof(mc),1,f);
				if(res!=1)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read material list chunk",MB_OK);
					return FALSE;
				}

				for(int j=0;j<mc.nEntities;j++)
				{
					MAT_ENTITY me;
					res=fread(&me,sizeof(me),1,f);
					if(res!=1)
					{
						if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read material list chunk",MB_OK);
						return FALSE;
					}

					if(me.IsStdMat)
					{
						//create the material
						StdMat *mat=NewDefaultStdMat();
						mat->SetName(me.name);
						mat->SetAmbient(  Color(me.col_a.r/255.0, me.col_a.g/255.0, me.col_a.b/255.0),0); 
						mat->SetDiffuse(  Color(me.col_d.r/255.0, me.col_d.g/255.0, me.col_d.b/255.0),0);
						mat->SetSpecular( Color(me.col_s.r/255.0, me.col_s.g/255.0, me.col_s.b/255.0),0);
						mat->SetDynamicsProperty(0, 0, DYN_BOUNCE			,me.Dyn_Bounce);
						mat->SetDynamicsProperty(0, 0, DYN_STATIC_FRICTION  ,me.Dyn_StaticFriction);
						mat->SetDynamicsProperty(0, 0, DYN_SLIDING_FRICTION ,me.Dyn_SlidingFriction);
						if(*me.map_d.name)
						{
							BitmapTex *tex=NewDefaultBitmapTex();
							tex->SetMapName(me.map_d.name);
							mat->SetSubTexmap(ID_DI,tex);
						}
						if(*me.map_b.name)
						{
							BitmapTex *tex=NewDefaultBitmapTex();
							tex->SetMapName(me.map_b.name);
							mat->SetSubTexmap(ID_BU,tex);
						}
						if(*me.map_o.name)
						{
							BitmapTex *tex=NewDefaultBitmapTex();
							tex->SetMapName(me.map_o.name);
							mat->SetSubTexmap(ID_OP,tex);
						}
						
						Mtl *m=(Mtl *)mat;
						Materials.Append(1,&m);
					}
					else
					{
						MultiMtl *mat=NewDefaultMultiMtl();
						mat->SetName(me.name);
						Mtl *m=(Mtl *)mat;
						Materials.Append(1,&m);
					}
				}
			}
			break;
		
		default:
			if(!SupressPrompts)
			{
				int res=MessageBox(GetActiveWindow(),"Error","Unknown chunk version for ChunkType_MtlList detected",MB_OK);
				return FALSE;
			}
		}
	}
	return TRUE;
}

//============================================================================================================================
// ChunkType_Mesh
//============================================================================================================================
BOOL CryImporter::ReadMeshChunks()
{
	int res;
	int n_chunks = ChunkTable.Count();
	for(int i=0;i<n_chunks;i++)
	{
		CHUNK_HEADER &ch = ChunkTable[i];
		if(ch.ChunkType != ChunkType_Mesh) continue;

		switch(ch.ChunkVersion)
		{
		case 0x0623:
			{
				fseek(f,ch.FileOffset,SEEK_SET);
				MESH_CHUNK_DESC_0623 mc;
				res=fread(&mc,sizeof(mc),1,f);
				if(res!=1)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read mesh chunk",MB_OK);
					return FALSE;
				}

				//read  prop str
				char *prop_str=NULL;
				if(mc.PropStrLen)
				{
					prop_str = (char*)calloc(mc.PropStrLen+1, 1);
					res=fread(prop_str,mc.PropStrLen,1,f);
					if(res!=1)
					{
						if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read prop str of mesh chunk",MB_OK);
						return FALSE;
					}
				}

				//read vertices
				CryVertex *verts=(CryVertex*)calloc(sizeof(CryVertex),mc.nVerts);
				res=fread(verts,sizeof(CryVertex),mc.nVerts,f);
				if(res!=mc.nVerts)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read vertices of mesh chunk",MB_OK);
					return FALSE;
				}

				//read faces
				CryFace *faces=(CryFace*)calloc(sizeof(CryFace),mc.nFaces);
				res=fread(faces,sizeof(CryFace),mc.nFaces,f);
				if(res!=mc.nFaces)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read faces of mesh chunk",MB_OK);
					return FALSE;
				}

				//read tverts
				CryUV *tverts=NULL;
				if(mc.nTVerts)
				{
					tverts=(CryUV *)calloc(sizeof(CryUV),mc.nTVerts);
					res=fread(tverts,sizeof(CryUV),mc.nTVerts,f);
					if(res!=mc.nTVerts)
					{
						if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read texture vertices of mesh chunk",MB_OK);
						return FALSE;
					}
				}

				//read tfaces
				CryTexFace *tfaces=NULL;
				if(tverts && (mc.nTVerts!= mc.nVerts))
				{
					tfaces=(CryTexFace*)calloc(sizeof(CryTexFace),mc.nFaces);
					res=fread(tfaces,sizeof(CryTexFace),mc.nFaces,f);
					if(res!=mc.nFaces)
					{
						if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read texture faces of mesh chunk",MB_OK);
						return FALSE;
					}
				}

				ImpNode *node = iip->CreateNode();
				if (node) 
				{
					//NAME
					node->SetName(GeomNames[mc.GeomID].name);

					//TRI OBJECT
					TriObject *tri = CreateNewTriObject();
					node->Reference(tri);
					Mesh &mesh=tri->mesh;
					
					int i;
					//TM
					Point3 center(0.0f,0.0f,0.0f);
					for(i=0;i<mc.nVerts;i++) center += Point3(verts[i].p.x, verts[i].p.y, verts[i].p.z);
					center /= float(mc.nVerts);
					Matrix3 tm;
					tm.IdentityMatrix();
					tm.SetTrans(center);
					node->SetTransform(0,tm);
					
					//FACE AND VERTEX
					mesh.setNumFaces(mc.nFaces);
					mesh.setNumVerts(mc.nVerts);
					for(i=0;i<mc.nVerts;i++)	mesh.setVert(i,Point3(verts[i].p.x, verts[i].p.y, verts[i].p.z)-center);
					for(i=0;i<mc.nFaces;i++)
					{
						mesh.faces[i].setVerts(faces[i].v0,faces[i].v1,faces[i].v2);
						mesh.faces[i].smGroup	= faces[i].SmGroup;
						mesh.faces[i].setEdgeVisFlags(EDGE_VIS,EDGE_VIS,EDGE_VIS);
						mesh.faces[i].setMatID(faces[i].MatID);
					}

					//TEXTURE COORDS
					if(tverts)
					{
						mesh.setNumTVerts(mc.nTVerts);
						mesh.setNumTVFaces(mc.nFaces, FALSE);
						for(int i=0;i<mc.nTVerts;i++)		mesh.tVerts[i] = Point3(tverts[i].u, tverts[i].v, 0.0f);

						if(!tfaces) //build identical face topology
							for(int i=0;i<mc.nFaces;i++)	mesh.tvFace[i].setTVerts(mesh.faces[i].v);
						else
							for(int i=0;i<mc.nFaces;i++)	mesh.tvFace[i].setTVerts(tfaces[i].t0,tfaces[i].t1,tfaces[i].t2);
					}

					//MATERIAL & PROP_STR
					INode *inode = node->GetINode();
					if(inode && Materials.Count() && (mc.MatID!=-1))
					{
						inode->SetMtl(Materials[mc.MatID]);
					}
					
					if(inode && prop_str) inode->SetUserPropBuffer(prop_str);
					

					//DONE
					iip->AddNodeToScene(node);
				}

				if(tverts)	free(tverts);
				if(tfaces)	free(tfaces);
				if(prop_str)free(prop_str);
				if(verts)	free(verts);
				if(faces)	free(faces);
			}
			break;

		case 0x0744:
			{
				fseek(f,ch.FileOffset,SEEK_SET);
				MESH_CHUNK_DESC_0744 mc;
				res=fread(&mc,sizeof(mc),1,f);
				if(res!=1)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read mesh chunk",MB_OK);
					return FALSE;
				}

				//read vertices
				CryVertex *verts=(CryVertex*)calloc(sizeof(CryVertex),mc.nVerts);
				res=fread(verts,sizeof(CryVertex),mc.nVerts,f);
				if(res!=mc.nVerts)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read vertices of mesh chunk",MB_OK);
					return FALSE;
				}

				//read faces
				CryFace *faces=(CryFace*)calloc(sizeof(CryFace),mc.nFaces);
				res=fread(faces,sizeof(CryFace),mc.nFaces,f);
				if(res!=mc.nFaces)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read faces of mesh chunk",MB_OK);
					return FALSE;
				}

				//read tverts
				CryUV *tverts=NULL;
				if(mc.nTVerts)
				{
					tverts=(CryUV *)calloc(sizeof(CryUV),mc.nTVerts);
					res=fread(tverts,sizeof(CryUV),mc.nTVerts,f);
					if(res!=mc.nTVerts)
					{
						if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read texture vertices of mesh chunk",MB_OK);
						return FALSE;
					}
				}

				//read tfaces
				CryTexFace *tfaces=NULL;
				if(tverts && (mc.nTVerts!= mc.nVerts))
				{
					tfaces=(CryTexFace*)calloc(sizeof(CryTexFace),mc.nFaces);
					res=fread(tfaces,sizeof(CryTexFace),mc.nFaces,f);
					if(res!=mc.nFaces)
					{
						if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read texture faces of mesh chunk",MB_OK);
						return FALSE;
					}
				}

				//TRI OBJECT
				TriObject *tri = CreateNewTriObject();
				Mesh &mesh=tri->mesh;
				
				//FACE AND VERTEX
				mesh.setNumFaces(mc.nFaces);
				mesh.setNumVerts(mc.nVerts);

				for(i=0;i<mc.nVerts;i++)	mesh.setVert(i,Point3(verts[i].p.x, verts[i].p.y, verts[i].p.z));
				for(i=0;i<mc.nFaces;i++)
				{
					mesh.faces[i].setVerts(faces[i].v0,faces[i].v1,faces[i].v2);
					mesh.faces[i].smGroup	= faces[i].SmGroup;
					mesh.faces[i].setEdgeVisFlags(EDGE_VIS,EDGE_VIS,EDGE_VIS);
					mesh.faces[i].setMatID(faces[i].MatID);
				}

				//TEXTURE COORDS
				if(tverts)
				{
					mesh.setNumTVerts(mc.nTVerts);
					mesh.setNumTVFaces(mc.nFaces, FALSE);
					for(int i=0;i<mc.nTVerts;i++)		mesh.tVerts[i] = Point3(tverts[i].u, tverts[i].v, 0.0f);

					if(!tfaces) //build identical face topology
						for(int i=0;i<mc.nFaces;i++)	mesh.tvFace[i].setTVerts(mesh.faces[i].v);
					else
						for(int i=0;i<mc.nFaces;i++)	mesh.tvFace[i].setTVerts(tfaces[i].t0,tfaces[i].t1,tfaces[i].t2);
				}

				//done
				ChunkList.Append(tri,&mc.chdr,false);

				if(tverts)	free(tverts);
				if(tfaces)	free(tfaces);
				if(verts)	free(verts);
				if(faces)	free(faces);
			}
			break;


		default:
			if(!SupressPrompts)
			{
				int res=MessageBox(GetActiveWindow(),"Error","Unknown chunk version for ChunkType_Mesh detected",MB_OK);
				return FALSE;
			}
		}
	}
	return TRUE;
}

//============================================================================================================================
// ChunkType_Helper
//============================================================================================================================
BOOL CryImporter::ReadHelperChunks()
{
	int res;
	int n_chunks = ChunkTable.Count();
	for(int i=0;i<n_chunks;i++)
	{
		CHUNK_HEADER &ch = ChunkTable[i];
		if(ch.ChunkType != ChunkType_Helper) continue;

		switch(ch.ChunkVersion)
		{
		case HELPER_CHUNK_DESC_0744::VERSION:
			{
				fseek(f,ch.FileOffset,SEEK_SET);
				HELPER_CHUNK_DESC_0744 hc;
				res=fread(&hc,sizeof(hc),1,f);
				if(res!=1)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read helper chunk",MB_OK);
					return FALSE;
				}

				//OBJECT
				switch(hc.type)
				{
				case HP_POINT:
					{
						//HelperObject *point = (HelperObject *)iip->Create(HELPER_CLASS_ID, Class_ID(POINTHELP_CLASS_ID, 0));
						HelperObject *point = (HelperObject *)ip->CreateInstance(HELPER_CLASS_ID, Class_ID(POINTHELP_CLASS_ID, 0));
						if(!point)
						{
							if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not create point object",MB_OK);
							//return FALSE;
							continue;
						}
						ChunkList.Append(point, &hc.chdr,false);
					}
					break;

				case HP_DUMMY:
					{
						DummyObject *dummy=new DummyObject();
						
						Point3 size(hc.size.x,hc.size.y,hc.size.z);
						size = size/2;
						dummy->SetBox(Box3(-size,size));
						ChunkList.Append(dummy, &hc.chdr,false);
					}
					break;


				default:
					{
						int res=MessageBox(GetActiveWindow(),"Error","Unknown Helper object type in helper chunk",MB_OK);
						return FALSE;
					}
				};
			}
			break;
		
		default:
			if(!SupressPrompts)
			{
				int res=MessageBox(GetActiveWindow(),"Error","Unknown chunk version for ChunkType_Helper detected",MB_OK);
				return FALSE;
			}
		}
	}
	return TRUE;
}
//============================================================================================================================
// ChunkType_Light
//============================================================================================================================
BOOL CryImporter::ReadLightChunks()
{
	int res;
	int n_chunks = ChunkTable.Count();
	for(int i=0;i<n_chunks;i++)
	{
		CHUNK_HEADER &ch = ChunkTable[i];
		if(ch.ChunkType != ChunkType_Light) continue;

		switch(ch.ChunkVersion)
		{
		case 0x0351:
			{
				fseek(f,ch.FileOffset,SEEK_SET);
				LIGHT_CHUNK_DESC_0351 lc;
				res=fread(&lc,sizeof(lc),1,f);
				if(res!=1)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read light chunk",MB_OK);
					return FALSE;
				}

				int lType = (lc.type==LT_OMNI)?OMNI_LIGHT : (lc.type==LT_SPOT) ? FSPOT_LIGHT : (lc.type==LT_DIRECT) ? DIR_LIGHT : -1;
				if(lType == -1)
				{
					iip->SetAmbient(0, Point3(lc.color.r/255.0f, lc.color.g/255.0f,lc.color.b/255.0f));
					continue;
				}

				ImpNode *node = iip->CreateNode();
				
				//NAME
				node->SetName(lc.name);
				
				//TM
				Matrix3 tm;
				CryMatrix_to_Matrix3(tm,lc.tm);
				tm.ValidateFlags();
				node->SetTransform(0,tm);
				
				//OBJECT
				GenLight *lt=iip->CreateLightObject(lType);
				lt->SetUseLight(lc.on);
				lt->Enable(lc.on);
				lt->SetHotspot(0,lc.hotsize);
				lt->SetFallsize(0,lc.fallsize);
				lt->SetAtten(0,ATTEN1_START,lc.nearAttenStart);
				lt->SetAtten(0,ATTEN1_END,lc.nearAttenEnd);
				lt->SetAtten(0,ATTEN_START,lc.attenStart);
				lt->SetAtten(0,ATTEN_END,lc.attenEnd);
				lt->SetRGBColor(0,Point3(lc.color.r/255.0f, lc.color.g/255.0f,lc.color.b/255.0f));
				lt->SetIntensity(0,lc.intens);
				lt->SetUseAtten(lc.useAtten);
				lt->SetUseAttenNear(lc.useNearAtten);
				lt->SetShadow(lc.shadow);
				node->Reference(lt);

				//DONE
				iip->AddNodeToScene(node);
			}
			break;

		case 0x0744:
			{
				fseek(f,ch.FileOffset,SEEK_SET);
				LIGHT_CHUNK_DESC_0744 lc;
				res=fread(&lc,sizeof(lc),1,f);
				if(res!=1)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read light chunk",MB_OK);
					return FALSE;
				}

				int lType = (lc.type==LT_OMNI)?OMNI_LIGHT : (lc.type==LT_SPOT) ? FSPOT_LIGHT : (lc.type==LT_DIRECT) ? DIR_LIGHT : -1;
				if(lType == -1)
				{
					iip->SetAmbient(0, Point3(lc.color.r/255.0f, lc.color.g/255.0f,lc.color.b/255.0f));
					continue;
				}

				//OBJECT
				GenLight *lt=iip->CreateLightObject(lType);
				lt->SetUseLight(lc.on);
				lt->Enable(lc.on);
				lt->SetHotspot(0,lc.hotsize);
				lt->SetFallsize(0,lc.fallsize);
				lt->SetAtten(0,ATTEN1_START,lc.nearAttenStart);
				lt->SetAtten(0,ATTEN1_END,lc.nearAttenEnd);
				lt->SetAtten(0,ATTEN_START,lc.attenStart);
				lt->SetAtten(0,ATTEN_END,lc.attenEnd);
				lt->SetRGBColor(0,Point3(lc.color.r/255.0f, lc.color.g/255.0f,lc.color.b/255.0f));
				lt->SetIntensity(0,lc.intens);
				lt->SetUseAtten(lc.useAtten);
				lt->SetUseAttenNear(lc.useNearAtten);
				lt->SetShadow(lc.shadow);

				ChunkList.Append(lt,&lc.chdr,false);
			}
			break;
		
		default:
			if(!SupressPrompts)
			{
				int res=MessageBox(GetActiveWindow(),"Error","Unknown chunk version for ChunkType_Light detected",MB_OK);
				return FALSE;
			}
		}
	}
	return TRUE;
}

//============================================================================================================================
// ChunkType_Node
//============================================================================================================================
BOOL CryImporter::ReadNodeChunks()
{
	int res;
	int n_chunks = ChunkTable.Count();
	for(int i=0;i<n_chunks;i++)
	{
		CHUNK_HEADER &ch = ChunkTable[i];
		if(ch.ChunkType != ChunkType_Node) continue;

		switch(ch.ChunkVersion)
		{
		case 0x0823:
			{
				fseek(f,ch.FileOffset,SEEK_SET);
				NODE_CHUNK_DESC_0823 nc;
				res=fread(&nc,sizeof(nc),1,f);
				if(res!=1)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read node chunk",MB_OK);
					return FALSE;
				}

				//read  prop str
				char *prop_str=NULL;
				if(nc.PropStrLen)
				{
					prop_str = (char*)calloc(nc.PropStrLen+1, 1);
					res=fread(prop_str,nc.PropStrLen,1,f);
					if(res!=1)
					{
						if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read prop str of node chunk",MB_OK);
						if(prop_str) free(prop_str); 
						prop_str=NULL;
						return FALSE;
					}
				}
				
				if(nc.ObjectID != -1)
				{
					Object *obj=(Object *)ChunkList.GetPtr(nc.ObjectID, ChunkType_ANY);
					if(obj)
					{
						//create node
						ImpNode *node	= iip->CreateNode();
						INode *inode	= node->GetINode();
						
						//NAME
						node->SetName(nc.name);

						//Group
						inode->SetGroupHead(nc.IsGroupHead);
						inode->SetGroupMember(nc.IsGroupMember);
						
						//TM
						Matrix3 tm;
						CryMatrix_to_Matrix3(tm,nc.tm);
						tm.ValidateFlags();
						node->SetTransform(0,tm);

						//object
						node->Reference(obj);

						//propstr
						if(inode && prop_str) inode->SetUserPropBuffer(prop_str);

						//material
						if(inode && nc.MatID!=-1)
						{
							Mtl *mtl=(Mtl *)ChunkList.GetPtr(nc.MatID, ChunkType_Mtl);
							if(mtl) inode->SetMtl(mtl);
						}
	
						//DONE
						iip->AddNodeToScene(node);

						ChunkList.Append(inode,&nc.chdr,false);
						if(prop_str) free(prop_str); 
						prop_str=NULL;

					}
				}
				else
				{
					int res=MessageBox(GetActiveWindow(),"Error","a node without an object is encountered ",MB_OK);
					if(prop_str) free(prop_str);
					prop_str=NULL;
					return FALSE;
				}
			}
			break;
		
		default:
			if(!SupressPrompts)
			{
				int res=MessageBox(GetActiveWindow(),"Error","Unknown chunk version for ChunkType_Node detected",MB_OK);
				return FALSE;
			}
		}
	}
	return TRUE;
}

//============================================================================================================================
// ChunkType_SceneProps
//============================================================================================================================
BOOL CryImporter::ReadScenePropChunks()
{
	int res;
	int n_chunks = ChunkTable.Count();
	for(int i=0;i<n_chunks;i++)
	{
		CHUNK_HEADER &ch = ChunkTable[i];
		if(ch.ChunkType != ChunkType_SceneProps) continue;

		switch(ch.ChunkVersion)
		{
		case 0x0201:
			{
				fseek(f,ch.FileOffset,SEEK_SET);
				SCENEPROPS_CHUNK_DESC_0201 sc;
				res=fread(&sc,sizeof(sc),1,f);
				if(res!=1)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read SceneProps chunk",MB_OK);
					return FALSE;
				}

				int numProps = sc.nProps;
				for (int i=0; i<numProps; i++) 
				{
					SCENEPROP_ENTITY item;
					res=fread(&item,sizeof(item),1,f);
					if(res!=1)
					{
						if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read SceneProps chunk",MB_OK);
						return FALSE;
					}

					wchar_t nameBuf[256],valueBuf[256];
					swprintf(nameBuf,  L"%hs", item.name);
					swprintf(valueBuf, L"%hs", item.value);
					
					PROPSPEC ps;
					ps.ulKind	= PRSPEC_LPWSTR;
					ps.lpwstr	= nameBuf;
					PROPVARIANT pv;
					pv.vt		= VT_LPWSTR;
					pv.pwszVal	= valueBuf;
					ip->AddProperty(PROPSET_USERDEFINED,&ps,&pv);
				
				}
			}
			break;

		case 0x0744:
			break;

		default:
			if(!SupressPrompts)
			{
				int res=MessageBox(GetActiveWindow(),"Error","Unknown chunk version for ChunkType_SceneProps detected",MB_OK);
				return FALSE;
			}
		}
	}
	return TRUE;
}

//============================================================================================================================
// BuildNodeHierarchy
//============================================================================================================================
BOOL CryImporter::BuildNodeHierarchy()
{
	int res;
	int n_chunks = ChunkTable.Count();
	for(int i=0;i<n_chunks;i++)
	{
		CHUNK_HEADER &ch = ChunkTable[i];
		if(ch.ChunkType != ChunkType_Node) continue;
		if(ch.ChunkVersion != 0x0823) continue;

		//read the node info
		fseek(f,ch.FileOffset,SEEK_SET);
		NODE_CHUNK_DESC_0823 nc;
		res=fread(&nc,sizeof(nc),1,f);
		if(res!=1)
		{
			if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read node chunk",MB_OK);
			return FALSE;
		}

		if(nc.nChildren)
		{
			INode *node = (INode *)ChunkList.GetPtr(nc.chdr.ChunkID,ChunkType_Node);
			assert(node);
			if(!node) continue;
			
			//skip proplen if any
			if(nc.PropStrLen) fseek(f,nc.PropStrLen,SEEK_CUR);

			//read children chunk ID's
			for(int j=0;j<nc.nChildren;j++)
			{
				int id;
				res=fread(&id,sizeof(id),1,f);
				if(res!=1)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read node children",MB_OK);
					return FALSE;
				}
				if(id==-1) continue;

				INode *child = (INode *)ChunkList.GetPtr(id,ChunkType_Node);
				if(child) node->AttachChild(child,TRUE);
			}

			//update bounding box of group head
			if(nc.IsGroupHead)
			{
				Object *obj = node->GetObjectRef();
				if (obj) 
				{
					if ( obj->ClassID()== Class_ID(DUMMY_CLASS_ID,0)) 
					{
						DummyObject *dum = (DummyObject *)obj;
						dum->SetValidity(NEVER);
						//ip->RedrawViews(ip->GetTime());
					}
				}
			}
		}


	}

	return TRUE;
}

//============================================================================================================================
// BuildMtlHierarchy
//============================================================================================================================
BOOL CryImporter::BuildMtlHierarchy()
{
	int res;
	int n_chunks = ChunkTable.Count();
	for(int i=0;i<n_chunks;i++)
	{
		CHUNK_HEADER &ch = ChunkTable[i];
		if(ch.ChunkType != ChunkType_Mtl) continue;
		if(ch.ChunkVersion != 0x0744) continue;

		//read the node info
		fseek(f,ch.FileOffset,SEEK_SET);
		MTL_CHUNK_DESC_0746 mc;
		res=fread(&mc,sizeof(mc),1,f);
		if(res!=1)
		{
			if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read mtl chunk",MB_OK);
			return FALSE;
		}

		if((mc.MtlType==MTL_MULTI || mc.MtlType==MTL_2SIDED) && mc.nChildren>0)
		{
			MultiMtl *mtl = (MultiMtl *)ChunkList.GetPtr(mc.chdr.ChunkID,ChunkType_Mtl);
			assert(mtl);
			if(!mtl) continue;
			
			//read children chunk ID's
			for(int j=0;j<mc.nChildren;j++)
			{
				int id;
				res=fread(&id,sizeof(id),1,f);
				if(res!=1)
				{
					if(!SupressPrompts) MessageBox(GetActiveWindow(),"Error","Can not read mtl children",MB_OK);
					return FALSE;
				}
				if(id==-1) continue;

				Mtl *child = (Mtl *)ChunkList.GetPtr(id,ChunkType_Mtl);
				if(child) mtl->SetSubMtlAndName(j,child,child->GetName());
			}
		}
	}

	return TRUE;
}
