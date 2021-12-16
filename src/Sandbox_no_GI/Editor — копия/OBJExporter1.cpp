// OBJExporter1.cpp: implementation of the COBJExporter class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "OBJExporter1.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

COBJExporter::COBJExporter()
{
	m_curObject = -1;
	m_curSubMesh = -1;
}

COBJExporter::~COBJExporter()
{

}


bool COBJExporter::StartNewObject(const char * pName)
{
	CExpObject newObject;
	strcpy(newObject.m_name, pName);
	//newObject.m_isMtl=false;
	m_objects.push_back(newObject);
	m_curObject++;
	m_curSubMesh = -1;

	//StartNewSubMesh();
	return true;
}


void COBJExporter::StartNewSubMesh()
{
	CExpSubMesh newSubMesh;
	newSubMesh.m_isMtl=false;
	if(m_curObject > -1)
	{
		m_objects[m_curObject].m_vSubMeshes.push_back(newSubMesh);
	}
	m_curSubMesh++;
}


char * COBJExporter::MakeRelativePath(char * outPath, const char * pMainFileName, const char * pFileName)
{
	//*
	const char * ch = strrchr(pMainFileName, '\\');
	if(ch)
	{
		if(strlen(pFileName)>ch-pMainFileName && !strnicmp(pMainFileName, pFileName, ch-pMainFileName))
		{
			strcpy(outPath, pFileName + (ch-pMainFileName)+1);
			return outPath;
		}
	}
	/**/
	strcpy(outPath, pFileName);
	return outPath;
}


bool COBJExporter::WriteOBJ(const char * pszFileName)
{
	//////////////////////////////////////////////////////////////////////
	// Write the OBJ file to disk
	//////////////////////////////////////////////////////////////////////

	FILE *hFile = NULL;
	VectorIt itVec;
	FaceIt itFace;
	TexCoordIt itTexCoord;
	CString szMTL;
	char ppszBuffer[3][32];

	ASSERT(pszFileName);

	CLogFile::FormatLine("Exporting OBJ file to '%s'", pszFileName);

	// Open the file
	hFile = fopen(pszFileName, "w");

	if (!hFile)
	{
		CLogFile::FormatLine("Error while opening file '%s' !", pszFileName);
		ASSERT(hFile);
		return false;
	}

	// Write header
	fprintf(hFile, "# Object file exported by Sandbox\n");
	fprintf(hFile, "# Attention: while import to 3DS Max Unify checkbox for normals must be unchecked.\n");
	fprintf(hFile, "#\n");

	// Create MTL library filename
	szMTL = pszFileName;
	szMTL = Path::ReplaceExtension( szMTL,"mtl" );
	szMTL = Path::GetFile(szMTL);

	// Write material library import statement
	fprintf(hFile, "mtllib %s\n", (const char*)szMTL);
	fprintf(hFile, "#\n");

	for(int i=0; i<=m_curObject; i++)
	{
		//fprintf(hFile, "g\n");
		fprintf(hFile, "g %s\n", m_objects[i].m_name); //For XSI
		fprintf(hFile, "# object %s\n", m_objects[i].m_name);
		fprintf(hFile, "#\n");

		// Write all vertices
		for (itVec=m_objects[i].m_vVertices.begin(); itVec!=m_objects[i].m_vVertices.end(); itVec++)
		{
			fprintf(hFile, "v  %s %s %s\n", 
			StripZero(ppszBuffer[0], (* itVec).fX),
			StripZero(ppszBuffer[1], (* itVec).fY), 
			StripZero(ppszBuffer[2], (* itVec).fZ));
		}
		fprintf(hFile, "# %i vertices\n\n", m_objects[i].m_vVertices.size());

		// Write all texture coordinates
		for (itTexCoord=m_objects[i].m_vTexCoords.begin(); itTexCoord!=m_objects[i].m_vTexCoords.end(); itTexCoord++)
		{
			fprintf(hFile, "vt  %s %s 0\n",
			StripZero(ppszBuffer[0], (* itTexCoord).fU),
			StripZero(ppszBuffer[1], (* itTexCoord).fV));
		}
		fprintf(hFile, "# %i texture vertices\n\n", m_objects[i].m_vTexCoords.size());


		// Write all vertices
		for (itVec=m_objects[i].m_vNormals.begin(); itVec!=m_objects[i].m_vNormals.end(); itVec++)
		{
			fprintf(hFile, "vn  %s %s %s\n", 
			StripZero(ppszBuffer[0], (* itVec).fX),
			StripZero(ppszBuffer[1], (* itVec).fY), 
			StripZero(ppszBuffer[2], (* itVec).fZ));
		}
		fprintf(hFile, "# %i vertex normals\n\n", m_objects[i].m_vNormals.size());


		//if(m_objects[i].m_isMtl)
		//	fprintf(hFile, "usemtl %s\n", m_objects[i].m_mtl.m_name);
		//fprintf(hFile, "s 0\n");

		for(int j=0; j<m_objects[i].m_vSubMeshes.size(); j++)
		{
			if(m_objects[i].m_vSubMeshes[j].m_isMtl)
				fprintf(hFile, "usemtl %s\n", m_objects[i].m_vSubMeshes[j].m_mtl.m_name);
			fprintf(hFile, "s %d\n", j);

			// Write all faces, convert the indices to one based indices
			for (itFace=m_objects[i].m_vSubMeshes[j].m_vFaces.begin(); itFace!=m_objects[i].m_vSubMeshes[j].m_vFaces.end(); itFace++)
			{
				/*
				fprintf(hFile, "f %i/%i %i/%i %i/%i\n",
					(* itFace).iVertexIndices[0] + 1, (* itFace).iTexCoordIndices[0] + 1,
					(* itFace).iVertexIndices[1] + 1, (* itFace).iTexCoordIndices[1] + 1,
					(* itFace).iVertexIndices[2] + 1, (* itFace).iTexCoordIndices[2] + 1);
					*/
				fprintf(hFile, "f %i/%i/%i %i/%i/%i %i/%i/%i\n",
					(* itFace).iVertexIndices[0] -m_objects[i].m_vVertices.size(), (* itFace).iTexCoordIndices[0] -m_objects[i].m_vTexCoords.size(), (* itFace).iVertexIndices[0] -m_objects[i].m_vNormals.size(),
					(* itFace).iVertexIndices[1] -m_objects[i].m_vVertices.size(), (* itFace).iTexCoordIndices[1] -m_objects[i].m_vTexCoords.size(), (* itFace).iVertexIndices[1] -m_objects[i].m_vNormals.size(),
					(* itFace).iVertexIndices[2] -m_objects[i].m_vVertices.size(), (* itFace).iTexCoordIndices[2] -m_objects[i].m_vTexCoords.size(), (* itFace).iVertexIndices[2] -m_objects[i].m_vNormals.size());

			}
			fprintf(hFile, "# %i faces\n\n", m_objects[i].m_vSubMeshes[j].m_vFaces.size());
		}
	}

	fprintf(hFile, "g\n");
	fclose(hFile);

	// Create MTL library filename
	szMTL = Path::ReplaceExtension( pszFileName,"mtl" );


	// Open the material file
	hFile = fopen(szMTL, "w");

	if (!hFile)
	{
		CLogFile::FormatLine("Error while opening file '%s' !", (const char*)szMTL);
		ASSERT(hFile);
		return false;
	}

	// Write header
	fprintf(hFile, "# Material file exported by CryEdit\n\n");

	char relPath[MAX_PATH];

	for(int i=0; i<=m_curObject; i++)
	{
		for(int j=0; j<m_objects[i].m_vSubMeshes.size(); j++)
		{
			if(m_objects[i].m_vSubMeshes[j].m_isMtl)
			{
				// Write terrain material
				CExpMtl & mtl = m_objects[i].m_vSubMeshes[j].m_mtl;
				fprintf(hFile, "newmtl %s\n", mtl.m_name);
				fprintf(hFile, "Ka %s %s %s\n",
					StripZero(ppszBuffer[0], mtl.dif.r),
					StripZero(ppszBuffer[1], mtl.dif.g),
					StripZero(ppszBuffer[2], mtl.dif.b));
				fprintf(hFile, "Kd %s %s %s\n",
					StripZero(ppszBuffer[0], mtl.dif.r),
					StripZero(ppszBuffer[1], mtl.dif.g),
					StripZero(ppszBuffer[2], mtl.dif.b));
				fprintf(hFile, "Ks %s %s %s\n",
					StripZero(ppszBuffer[0], mtl.spec.r),
					StripZero(ppszBuffer[1], mtl.spec.g),
					StripZero(ppszBuffer[2], mtl.spec.b));
				fprintf(hFile, "d %s\n", StripZero(ppszBuffer[0], mtl.trans));
				fprintf(hFile, "Tr %s\n", StripZero(ppszBuffer[0], mtl.trans));
				fprintf(hFile, "Ns %s\n", StripZero(ppszBuffer[0], mtl.shin));
				if(strlen(mtl.m_map_Kd))
					fprintf(hFile, "map_Kd %s\n", MakeRelativePath(relPath, pszFileName, mtl.m_map_Kd));
				if(strlen(mtl.m_map_Ks))
					fprintf(hFile, "map_Ks %s\n", MakeRelativePath(relPath, pszFileName, mtl.m_map_Ks));
				if(strlen(mtl.m_map_Ns))
					fprintf(hFile, "map_Ns %s\n", MakeRelativePath(relPath, pszFileName, mtl.m_map_Ns));
				if(strlen(mtl.m_map_d))
					fprintf(hFile, "map_d %s\n", MakeRelativePath(relPath, pszFileName, mtl.m_map_d));
				if(strlen(mtl.m_map_bump))
					fprintf(hFile, "bump %s\n", MakeRelativePath(relPath, pszFileName, mtl.m_map_bump));
				if(strlen(mtl.m_map_decal))
					fprintf(hFile, "decal %s\n", MakeRelativePath(relPath, pszFileName, mtl.m_map_decal));
				if(strlen(mtl.m_map_disp))
					fprintf(hFile, "disp %s\n", MakeRelativePath(relPath, pszFileName, mtl.m_map_disp));
				fprintf(hFile, "\n");
			}
		}
	}

	fclose(hFile);
	
	return true;
}

char * COBJExporter::StripZero(char pszOutputBuffer[32], float fNumber)
{
	////////////////////////////////////////////////////////////////////////
	// Convert a float into a string representation and remove all 
	// uneccessary zeroes and the decimal dot if it is not needed
	////////////////////////////////////////////////////////////////////////

	long i;

	sprintf(pszOutputBuffer, "%f", fNumber);

	for (i=strlen(pszOutputBuffer) - 1; i>=0; i--)
	{
		if (pszOutputBuffer[i] == '0')
			pszOutputBuffer[i] = '\0';
		else if (pszOutputBuffer[i] == '.')
		{
			pszOutputBuffer[i] = '\0';
			break;
		}
		else
			break;
	}

	return pszOutputBuffer;
}