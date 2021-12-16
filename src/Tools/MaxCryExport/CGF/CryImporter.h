//===========================================================================*\
//	Part of Crytek Character Studio and Object Export Plug-in
//
//  Copyright: © Cuneyt Ozdas 2000, 2001
//				 cuneyt@cuneytozdas.com
//===========================================================================*/

#ifndef __CRYIMPORTER__H
#define __CRYIMPORTER__H

#include "resource.h"
#include "namelist.h"

StdUVGen *GetTextureStdUVGen(Texmap* tex);

extern TCHAR *GetString(int id);
extern HINSTANCE hInstance;
ClassDesc2* GetCryImporterDesc();

#define CRYIMPORTER_CLASS_ID	Class_ID(0x50b91324, 0x53d0a2da)

class CryImporter : public SceneImport 
{
public:
	static HWND		hParams;
	ImpInterface	*iip;
	Interface		*ip;
	bool			SupressPrompts;
	bool			ImportGeom;
	bool			ImportLights;
	bool			ImportHelpers;
	bool			DeleteScene;
	
	FILE			*f;
	FILE_HEADER		FileHeader;

	Tab<CHUNK_HEADER>	ChunkTable;	//read from the file
	Tab<NAME_ENTITY>	GeomNames;
	Tab<Mtl*>			Materials;
	CHUNK_LIST			ChunkList;	//filled by each chunk read: used for chunk-matching

	int				ExtCount()						{ return 1;}
	const TCHAR *	Ext(int n)						{ return _T("cgf"); }
	const TCHAR *	LongDesc()						{ return _T("Crytek Geometry File");}
	const TCHAR *	ShortDesc()						{ return _T("Crytek");}
	const TCHAR *	AuthorName()					{ return _T("Cuneyt Ozdas");}
	const TCHAR *	CopyrightMessage()				{ return _T("Copyright 2001 Cuneyt Ozdas"); }
	const TCHAR *	OtherMessage1()					{ return _T("http://www.crytek.com");}
	const TCHAR *	OtherMessage2()					{ return _T("http://www.crymod.com");}
	unsigned int	Version()						{ return 100;}
	
	void			ShowAbout(HWND hWnd);		
	int				DoImport(const TCHAR *name,ImpInterface *i,Interface *gi, BOOL suppressPrompts=FALSE);	// Import file
	BOOL			ReadGeomNameListChunks();
	BOOL			ReadMaterialListChunks();
	BOOL			ReadMeshChunks();
	BOOL			ReadHelperChunks();
	BOOL			ReadLightChunks();
	BOOL			ReadScenePropChunks();
	BOOL			ReadNodeChunks();
	BOOL			ReadMtlChunks();
	BOOL			BuildMtlHierarchy();
	BOOL			BuildNodeHierarchy();
	
	//Constructor/Destructor

	CryImporter();
	~CryImporter();	
	

};

class CryImporterClassDesc:public ClassDesc2 
{
	public:
	int 			IsPublic()						{ return TRUE; }
	void *			Create(BOOL loading = FALSE)	{ return new CryImporter(); }
	const TCHAR *	ClassName()						{ return _T("CryTekImporter"); }
	SClass_ID		SuperClassID()					{ return SCENE_IMPORT_CLASS_ID; }
	Class_ID		ClassID()						{ return CRYIMPORTER_CLASS_ID; }
	const TCHAR* 	Category()						{ return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName()					{ return _T("CryImporter"); }		// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance()						{ return hInstance; }				// returns owning module handle

};


#endif // __CRYIMPORTER__H
