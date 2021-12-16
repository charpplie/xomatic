#include "stdafx.h"													// for precompiled headers (has to be in first place)
#include "OBJFileFormat.h"									// COBJFileFormat
#include "../ASCIIFile.h"										// CASCIIFile

#include <map>															// STL map<>





#define SMALL_FLOAT 1e-12

#define LEFTCOMPARE(aa)							if(strncmp((const char *)p,aa,strlen(aa))==0)

// constructor
COBJFileFormat::COBJFileFormat( const bool bRecreateNormals ) 
	:m_bRecreateNormals(bRecreateNormals), m_bHasTex(false)
{
}


// destructor
COBJFileFormat::~COBJFileFormat()
{
	FreeData();
}


void COBJFileFormat::FreeData()
{
	std::vector<COBJMaterial *>::iterator it;

	for(it=m_Materials.begin();it!=m_Materials.end();++it)
		delete (*it);

	m_Materials.clear();
}


bool IsWhitespace( const char c )
{
	return c==8 || c==32;
}

// check if there is more than just whitespace
bool IsEndOfLine( unsigned char * _p )
{
	unsigned char *p=_p;

	while(IsWhitespace(*p))
		p++;										// jump over whitespace

	return *p==0 || *p==10 || *p==13;
}

// read an integer from memory and increse the pointer
void GotoNextLine( unsigned char * &p )
{
	while(*p!=10 && *p!=13 && *p!=0)p++;					// jump over content

	while(*p==10 || *p==13)p++;										// jump over return
}

// jump over float/int
void GotoNextNumber( unsigned char * &p )
{
	while(IsWhitespace(*p))p++;										// jump over whitespace

	while((*p>='0' && *p<='9') 
		|| *p=='-' || *p=='.')p++;									// jump over float

	while(IsWhitespace(*p))p++;										// jump over whitespace
}


// inefficient?
void GetLineAsString( unsigned char * &p, string &outStr )
{
	outStr="";

	while(IsWhitespace(*p))p++;						// jump over whitespace

	unsigned char *pStart = p;
	unsigned char *pEnd = p;

	while(*p!=10 && *p!=13 && *p!=0)
	{
		if(*p!=' ')pEnd=p+1;								// trim right ' '
		p++;
	}

	while(pStart<pEnd)
		outStr += *pStart++;								// could be done more efficient
}

//
uint32 GetDWORD( unsigned char * &p )
{
	uint32 ret=0;

	while(IsWhitespace(*p))p++;										// jump over whitespace

	while(*p>='0' && *p<='9')
	{
		ret = ret*10 + (*p-'0');p++;
	}
	
	return ret;
}


//
int GetInt( unsigned char * &p )
{
	uint32 ret=0;
	bool bNeg=false;

	while(IsWhitespace(*p))p++;										// jump over whitespace

	if(*p=='-')
	{
		bNeg=true;p++;
	}

	while(*p>='0' && *p<='9')
	{
		ret = ret*10 + (*p-'0');p++;
	}

	if(bNeg)return -(int)ret;
		return ret;
}


// returns 0.0f if there is no float
float GetFloat( unsigned char * &p )
{
	float ret=0;
	bool bNeg=false;

	while(IsWhitespace(*p))p++;										// jump over whitespace

	if(*p=='-')
	{
		bNeg=true;p++;
	}

	float fRet=0;

	while(*p>='0' && *p<='9')
	{
		fRet = fRet*10.0f + (float)(*p-'0');
		p++;
	}

	if(*p=='.')
	{
		p++;
		float Q = 0.1f;

		while(*p>='0' && *p<='9')
		{
			fRet += (float)(*p-'0') * Q;

			p++;Q*=0.1f;
		}
	}

	if(bNeg)
		fRet = -fRet;

	return fRet;
}



uint32 _CalcIndex( int indwValue, uint32 dwNum )
{
	if(indwValue<0)return (uint32)(dwNum+indwValue);
		else return (DWORD)(indwValue-1);
}

/*
void COBJFileFormat::_LoadOBJ_ComputeSize_WithZBrushDetection( CLineReader &in, uint32 &dwTriangleCount, uint32 &dwPosCount,
	uint32 &dwNormalCount, uint32 &dwUVCount, bool &bSmoothingGroupsSpecified )
{
	const uint32 dwMaxLineLength=1024;
	unsigned char szLine[dwMaxLineLength];

	dwNormalCount=0;
	bSmoothingGroupsSpecified=false;

	// typical header:
	// # File exported by ZBrush version 2.0
	// # www.zbrush.com
	// #Vertex Count 14388
	// #UV Vertex Count 84156
	// #Face Count 28052
	// #Auto scale x=0.022858 y=0.022858 z=0.022858
	// #Auto offset x=-0.013276 y=1.221337 z=0.041479
	// mtllib lo.mtl

	if(in.GetLine(szLine,dwMaxLineLength))	// ignore first line
	if(in.GetLine(szLine,dwMaxLineLength))	// ignore second line
	{
		for(uint32 dwLine=0;dwLine<10;++dwLine)
		{
			if(!in.GetLine(szLine,dwMaxLineLength))
				break;

			unsigned char *p = szLine;

			LEFTCOMPARE("#Vertex Count ")
			{
				p+=strlen("#Vertex Count ");

				dwPosCount = GetInt(p);
			}
			else LEFTCOMPARE("#UV Vertex Count ")
			{
				p+=strlen("#UV Vertex Count ");

				dwUVCount = GetInt(p);
			}
			else LEFTCOMPARE("#Face Count ")
			{
				p+=strlen("#Face Count ");

				dwTriangleCount = GetInt(p)*2;		// *2 because ZBrush saves quads - PROBLEM - actually not always
			}
		}
	}


	if(dwTriangleCount)
	if(dwPosCount)
	if(dwUVCount)
	{
		OutputDebugString("ZBrush header detected - 1 pass invoked - fast\n");
		return;
	}


	in.Restart();

	OutputDebugString("no ZBrush header detected - 2 pass invoked\n");

	// load whole file and ComputeSize manually
	_LoadOBJ_ComputeSize(in,dwTriangleCount,dwPosCount,dwNormalCount,dwUVCount,bSmoothingGroupsSpecified);
}
*/


void COBJFileFormat::_LoadOBJ_ComputeSize( CLineReader &in, uint32 &dwTriangleCount, uint32 &dwPosCount,
	uint32 &dwNormalCount, uint32 &dwUVCount, bool &bSmoothingGroupsSpecified )
{
	dwTriangleCount=0;
	dwPosCount=0;
	dwNormalCount=0;
	dwUVCount=0;
	bSmoothingGroupsSpecified=false;

	uint32	dwVertexMapSize=0;																				// size of VertexMap

	MessageAdd("LoadOBJ: Enter Pass #1");

	// Pass #1: get the data count (for faster loading with less memory trashing)
	{
		const uint32 dwMaxLineLength=1024;
		unsigned char szLine[dwMaxLineLength];

		while(in.GetLine(szLine,dwMaxLineLength))
		{
			unsigned char *p = szLine;

			LEFTCOMPARE("v ")													// vertex position
			{
				dwPosCount++;
			}
			else
			LEFTCOMPARE("vt ")												// vertex texture coordinates (optional)
			{
				dwUVCount++;
			}
			else
			LEFTCOMPARE("vn ")												// vertex normals (optional)
			{
				dwNormalCount++;
			}
			else
			LEFTCOMPARE("s ")													// smoothing groups
			{
				bSmoothingGroupsSpecified=true;
			}
			else
			LEFTCOMPARE("f ")													// convex polygon vertex No assignment
			{
				p+=2;

				uint32 i=0;

				while(!IsEndOfLine(p))
				{
					// jump over one vertex
					for(;;)
					{			
						GetInt(p);
						if(*p=='/')
						{
							p++;
							continue;
						}
						break;
					}

					++i;
				}

				dwTriangleCount+=i-2;			// 3 vertices -> 1 triangle, 4 vertices -> 2 triangles
			}
		}
	}		// Pass #1 end
}




bool COBJFileFormat::_LoadOBJ_LoadData( CLineReader &in, const bool bSmoothingGroupsSpecified, ITriangleMeshOutputProxy &rOutput )
{
	std::vector<uint32> SmoothingGroups;

	if(bSmoothingGroupsSpecified)
		SmoothingGroups.resize(rOutput.GetTriangleCount(),0xffffffff);

	bool bRet=true;																										// return value

	do																																	// goto replacement
	{
		MessageAdd("LoadOBJ: Enter Pass #2");

		// Pass #2: load data
		{
			uint32 NumVPos=0,NumVTex=0,NumVNorm=0,NumTri=0;
			COBJMaterial *pCurrentMaterial=0;

			bool bTextureThere=false,bNormalsThere=false;										// what find out what is the meaning of %d %d/%d or %d/%d/%d
			bool bFaceMode=false;																						// facemode / vertex mode
			uint32 dwCurrentSmoothingGroup=0xffffffff;

			const uint32 dwMaxLineLength=1024;
			unsigned char szLine[dwMaxLineLength];

			while(in.GetLine(szLine,dwMaxLineLength))
			{
				unsigned char *p = szLine;

				LEFTCOMPARE("v ")										// vertex position
				{
					if(bFaceMode)											// next object (from facemode to vertexmode
					{ 
						bTextureThere=false;bNormalsThere=false;bFaceMode=false; 
					}

					p+=2;
					Vec3 vec;

					vec.x=GetFloat(p);vec.y=GetFloat(p);vec.z=GetFloat(p);

					rOutput.SetPos(NumVPos++,vec);
				}
				else
				LEFTCOMPARE("vt ")									// vertex texture coordinates (optional)
				{
					p+=3;
					Vec2 vec;

//					vec.x=GetFloat(p);vec.y=1.0f-GetFloat(p);			// 1.0f- because texture coordinates in MAX have (left,bottom) as origin 
					vec.x=GetFloat(p);vec.y=GetFloat(p);

					// check for reasonable texture coords
					assert(vec.x>-100.0f);
					assert(vec.y>-100.0f);
					assert(vec.x<100.0f);
					assert(vec.y<100.0f);

					bTextureThere=true;
					rOutput.SetUV(NumVTex++,vec);				
				}
				else
				LEFTCOMPARE("vn ")									// vertex normals (optional)
				{
					p+=3;
					Vec3 vec;
		
					vec.x=GetFloat(p);vec.y=GetFloat(p);vec.z=GetFloat(p);

					bNormalsThere=true;
					rOutput.SetNormal(NumVNorm++,vec);
				}
				else
				LEFTCOMPARE("usemtl ")							// convex polygon material
				{
					string sMatName;
					p+=7;

					GetLineAsString(p,sMatName);

					pCurrentMaterial = FindOrCreateNewMaterial(sMatName.c_str());
					assert(pCurrentMaterial);
				}
				else
				LEFTCOMPARE("s ")										// smoothing groups
				{
					p+=2;

					dwCurrentSmoothingGroup=GetDWORD(p);
				}
				else
				LEFTCOMPARE("f ")										// convex polygon vertex No assignment
				{
					uint32 dwIndex_Pos[3];
					uint32 dwIndex_Tex[3];
					uint32 dwIndex_Norm[3];

					bFaceMode=true;
					p+=2;

					int i=0;

					while(!IsEndOfLine(p))																						// as many vertices are there
					{
						dwIndex_Pos[2]=_CalcIndex(GetInt(p),NumVPos);

						if(bTextureThere)
						{
							assert(*p=='/');
							while(*p=='/')p++;
							dwIndex_Tex[2]=_CalcIndex(GetInt(p),NumVTex);
						}
						else dwIndex_Tex[2]=0xffffffff;															// unassiged

						if(bNormalsThere)
						{
							assert(*p=='/');
							while(*p=='/')p++;
							dwIndex_Norm[2]=_CalcIndex(GetInt(p),NumVNorm);
						}
						else dwIndex_Norm[2]=0xffffffff;														// unassiged

						if(i>=2)
						{
							SMeshOutputTriangle tri;

							for(uint32 dwP=0;dwP<3;++dwP)
							{
								tri.m_PosIndex[dwP]=dwIndex_Pos[dwP];
								tri.m_NormalIndex[dwP]=dwIndex_Norm[dwP];
								tri.m_UVIndex[dwP]=dwIndex_Tex[dwP];
							}

							if(!pCurrentMaterial)
								pCurrentMaterial = FindOrCreateNewMaterial("default");

							tri.m_dwMaterialId = pCurrentMaterial->m_dwId;

							// store the smoothing group information
							if(bSmoothingGroupsSpecified)
								SmoothingGroups[NumTri]=dwCurrentSmoothingGroup;
							
							// store a new triangle
							rOutput.SetTriangle(NumTri++,tri);
						}

						// triangle fan
						if(i==0)
						{
							dwIndex_Pos[0]=dwIndex_Pos[2];
							dwIndex_Tex[0]=dwIndex_Tex[2];
							dwIndex_Norm[0]=dwIndex_Norm[2];						
						}
						else
						{
							dwIndex_Pos[1]=dwIndex_Pos[2];
							dwIndex_Tex[1]=dwIndex_Tex[2];
							dwIndex_Norm[1]=dwIndex_Norm[2];						
						}

						i++;
					}		// for i
				}			// "f "
			}
		}		// Pass #2 end

		// add materials
		{
			std::vector<COBJMaterial *>::iterator it;
			uint32 dwI=0;

			rOutput.AllocateMaterials( (uint32)m_Materials.size() );

			for(it=m_Materials.begin();it!=m_Materials.end();++it,++dwI)
			{
				COBJMaterial &rMat = *(*it);

				rOutput.SetMaterial(dwI,rMat);
			}
		}

		// ************************************************************************************

		bool bRecreateNormals = rOutput.GetNormalCount()==0 && m_bRecreateNormals;

		MessageAdd("LoadOBJ normal recreation: %s",bRecreateNormals?"true":"false");
		
		// recreate normal information
		if(bRecreateNormals)
			bRet=_RecreateNormals(rOutput,SmoothingGroups);

	} while(false);																										// goto replacement

	return bRet;
}





// load the .OBJ file format (Alias Wavefront)
bool COBJFileFormat::LoadOBJ( const char *inszFileName, ITriangleMeshOutputProxy &rOutput )
{
	if(*inszFileName==0)
		return false;

//	m_Progress.CreateThreadderHWND();		// if use the progress window we need to close it as well (e.g. class with destructor)

	bool bRet=false;																// return value

	MessageAdd("LoadOBJ: start loading '%s'",inszFileName);

	do																									// goto replacement
	{
		// open the file ----------------------------------------
		CLineReader	in(inszFileName);

		if(!in.IsValid())
		{
			ErrorAdd("COBJFileFormat::LoadOBJ '%s' failed",inszFileName);break;	
		}

		// read the data -----------------------------------------

		// material library
		// for each object
		// {
		//		vertex position
		//		vertex texture coordinates (optional)
		//		vertex normals (optional)
		//		face material and vertex No assignment
		// }

		// load .MTL file
		{
			uint32 len = (uint32)strlen(inszFileName);

			if(len>=4 && inszFileName[len-4]=='.')
			{
				string sMTLName = inszFileName;		
				
				sMTLName.resize(sMTLName.size()-3);		sMTLName+="MTL";
				
				bRet=_LoadMTL(sMTLName.c_str());			// todo: MTL file name should come from the .OBJ file
			}
			else
			{
				bRet=false;
				MessageAdd("Can't load material file for OBJ file '%s'",inszFileName);
			}
		}
		uint32 dwFaces,NumVPos,NumVNorm,NumVTex;
		bool bSmoothingGroupsSpecified;

		// load .OBJ (and recreate normals)
		if(bRet)
		{
//			_LoadOBJ_ComputeSize_WithZBrushDetection(in,dwFaces,NumVPos,NumVNorm,NumVTex,bSmoothingGroupsSpecified);
			_LoadOBJ_ComputeSize(in,dwFaces,NumVPos,NumVNorm,NumVTex,bSmoothingGroupsSpecified);

			MessageAdd("%d vertices, %d texture coordinates, %d normals, %d dwFaces", NumVPos,NumVTex,NumVNorm,dwFaces);

			{
				char str[256];

				sprintf(str,"Allocate (tri:%d pos:%d uv:%d norm:%d)\n",
					dwFaces,NumVPos,NumVTex,NumVNorm);

				OutputDebugString(str);
			}

			// reserve the needed amount (for faster loading with less memory trashing)
			rOutput.Allocate(dwFaces,NumVPos,NumVNorm,NumVTex);

			m_bHasTex = NumVTex!=0;
		}

		if(bRet)
		{
			in.Restart();
			bRet=_LoadOBJ_LoadData(in,bSmoothingGroupsSpecified,rOutput);
		}

	} while(false);																			// goto replacement


	// close the file ---------------------------------------- 
	if(!bRet)
		FreeData();

	if(bRet)
		MessageAdd("LoadOBJ: '%s' successfully loaded",inszFileName);

	return bRet;
}




































// recreate the normal information (with smoothing groups)
bool COBJFileFormat::_RecreateNormals( ITriangleMeshOutputProxy &rOutput, const std::vector<uint32> SmoothingGroups )
{
	std::multimap<uint32,std::pair<uint32,uint32> > mmSharedMap;			// key=dwVertexNo value=std::pair<dwSmoothingGroup,dwNormalNo>

	uint32 dwTriangleCount = rOutput.GetTriangleCount();
	uint32 dwNormalsCount=0;

	// for every triangle
	for(uint32 dwTri=0;dwTri<dwTriangleCount;++dwTri)
	{
		uint32 dwTriSmoothingGroup;

		if(!SmoothingGroups.empty())
		{
			assert(dwTri<SmoothingGroups.size());
			dwTriSmoothingGroup = SmoothingGroups[dwTri];
		}
		else dwTriSmoothingGroup=0xffffffff;						// no smoothing groups -> soft edges!

		uint32 dwPosIndices[3];
		uint32 dwNormalIndices[3] = { 0xffffffff,0xffffffff,0xffffffff };

		rOutput.GetPosIndices(dwTri,dwPosIndices);

		// for every vertex of the triangle
		for(int i=0;i<3;i++)
		{
			std::multimap<uint32,std::pair<uint32,uint32> >::iterator itFind=mmSharedMap.find(dwPosIndices[i]);	

			// merge vertices
			// for every vertex with the same smoothing group and vertex number in my map
			while(itFind!=mmSharedMap.end())
			{
				if((*itFind).second.first==dwTriSmoothingGroup)
				{
					dwNormalIndices[i] = (*itFind).second.second;
					break;
				}

				++itFind;
				if((*itFind).first != dwPosIndices[i])
					break;
			}

			if(dwNormalIndices[i]==0xffffffff)																				// create the normal 
			{
				dwNormalIndices[i] = dwNormalsCount++;	

				mmSharedMap.insert(std::pair<uint32,std::pair<uint32,uint32> >(dwPosIndices[i],
					std::pair<uint32,uint32>(dwTriSmoothingGroup,dwNormalIndices[i])));
			}

			rOutput.SetNormalIndices(dwTri,dwNormalIndices);
		}
	}	// for every triangle

	// allocate normals **************************************

	if(!rOutput.AllocateNormals(dwNormalsCount))
		return false;

	// clear normals **************************************

	// for every normal
	for(uint32 dwN=0;dwN<dwNormalsCount;++dwN)
		rOutput.SetNormal(dwN,Vec3(0,0,0));

	// add the normals **************************************

	// for every triangle
	for(uint32 dwTri=0;dwTri<dwTriangleCount;++dwTri)
	{
		uint32 dwTriSmoothingGroup;

		if(!SmoothingGroups.empty())
		{
			assert(dwTri<SmoothingGroups.size());
			dwTriSmoothingGroup = SmoothingGroups[dwTri];
		}
		else dwTriSmoothingGroup=0xffffffff;						// no smoothing groups -> soft edges!

		Vec3 vTriNormal;
		uint32 dwPosIndices[3];
		
		rOutput.GetPosIndices(dwTri,dwPosIndices);

		// build triangle normal
		{
			Vec3 a = rOutput.GetPos(dwPosIndices[1])-rOutput.GetPos(dwPosIndices[0]);
			Vec3 b = rOutput.GetPos(dwPosIndices[2])-rOutput.GetPos(dwPosIndices[0]);

			vTriNormal = a.Cross(b).GetNormalized();
		}

		uint32 dwNormalIndices[3];

		rOutput.GetNormalIndices(dwTri,dwNormalIndices);

		// for every vertex of the triangle
		for(int i=0;i<3;i++)
		{
			if(dwTriSmoothingGroup==0)
				{ rOutput.SetNormal(dwNormalIndices[i],vTriNormal);continue; }

			std::multimap<uint32,std::pair<uint32,uint32> >::iterator itFind;
			
			itFind=mmSharedMap.find( dwPosIndices[i] );

			while(itFind!=mmSharedMap.end())
			{
				uint32 dwVertexSmoothingGroup=(*itFind).second.first;
				uint32 dwNormalIdx=(*itFind).second.second;
				const Vec3 vert = rOutput.GetNormal(dwNormalIdx);

				if((dwVertexSmoothingGroup & dwTriSmoothingGroup) !=0)				// only vertices with the same smoothing group
				{
					Vec3 vPos[3] = { rOutput.GetPos(dwPosIndices[0]), rOutput.GetPos(dwPosIndices[1]), rOutput.GetPos(dwPosIndices[2]) };

//					float fAngle = Vec3::CalcAngleBetween(	vPos[(i+2)%3]-vPos[i], vPos[(i+1)%3]-vPos[i] );
					float fAngle = acos( (vPos[(i+2)%3]-vPos[i]).GetNormalized() | (vPos[(i+1)%3]-vPos[i]).GetNormalized() );		// CalcAngleBetween

					rOutput.SetNormal(dwNormalIdx,vert + vTriNormal*fAngle);
				}

				++itFind;
				if((*itFind).first!=dwPosIndices[i])
					break;
			}
		}
	}	// for every triangle


	// normalize normals **************************************

	// for every normal
	for(uint32 dwN=0;dwN<dwNormalsCount;++dwN)
	{
		Vec3 vNorm = rOutput.GetNormal(dwN);

//		rOutput.SetNormal(dwN,vNorm.GetNormalized());
//		vNorm.Normalize();
		vNorm.NormalizeSafe();
		rOutput.SetNormal(dwN,vNorm);
	}

	return true;
}
















bool COBJFileFormat::_LoadMTL( const char *inszFileName )
{
	CASCIIFile file;

	if(!file.IO_LoadASCIIFile(inszFileName))
	{
		MessageAdd("COBJFileFormat::LoadMTL no .MTL file found ('%s')",inszFileName);
		return true;		// no file, nothing to load
	}

	char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char fname[_MAX_FNAME];
  char ext[_MAX_EXT];

	_splitpath(inszFileName,drive,dir,fname,ext);

	unsigned char *p=(unsigned char *)file.GetDataPtr();
	COBJMaterial *pCurrentMaterial=0;
	Vec3 vec;

	while(*p)
	{
		while(*p>0 && *p<=' ')p++;		// ignore whitspace

		LEFTCOMPARE("newmtl ")		// material name
		{
			p+=7;
			string sMatName;
			GetLineAsString(p,sMatName);
			pCurrentMaterial = FindOrCreateNewMaterial(sMatName.c_str());
		}
		else
		if(pCurrentMaterial)			// proceed only with a material pointer
		LEFTCOMPARE("Ka ")				// ambient color
		{
			p+=3;
			vec.x=GetFloat(p);vec.y=GetFloat(p);vec.z=GetFloat(p);
			pCurrentMaterial->m_Ka = vec;
		}
		else
		LEFTCOMPARE("Kd ")				// diffuse color
		{
			p+=3;
			vec.x=GetFloat(p);vec.y=GetFloat(p);vec.z=GetFloat(p);
			pCurrentMaterial->m_Kd = vec;
		}
		else
		LEFTCOMPARE("Ks ")				// specular color
		{
			p+=3;
			vec.x=GetFloat(p);vec.y=GetFloat(p);vec.z=GetFloat(p);
			pCurrentMaterial->m_Ks = vec;
		}
		else
		LEFTCOMPARE("Kss ")				// subsurface color (non OBJ standard)
		{
			p+=4;
			vec.x=GetFloat(p);vec.y=GetFloat(p);vec.z=GetFloat(p);
			pCurrentMaterial->m_Kss = vec;
		}
		else
		LEFTCOMPARE("d ")					//
		{
		}
		else
		LEFTCOMPARE("Ns ")				//  specular highlight
		{
		}
		else
		LEFTCOMPARE("illum ")			//
		{
		}
		else
		LEFTCOMPARE("map_Kd ")		//
		{
			p+=7;
			string sTexName;
			GetLineAsString(p,sTexName);

			char path_buffer[_MAX_PATH];
			_makepath(path_buffer,drive,dir,sTexName.c_str(),0);
		}
		else
		LEFTCOMPARE("bump ")			//
		{
			p+=5;
			string sTexName;
			GetLineAsString(p,sTexName);

			char path_buffer[_MAX_PATH];
			_makepath(path_buffer,drive,dir,sTexName.c_str(),0);
		}
		else
		LEFTCOMPARE("occdir ")			//
		{
			p+=7;
			string sTexName;
			GetLineAsString(p,sTexName);

			char path_buffer[_MAX_PATH];
			_makepath(path_buffer,drive,dir,sTexName.c_str(),0);
		}

		GotoNextLine(p);
	}

	return true;
}




COBJMaterial *COBJFileFormat::FindOrCreateNewMaterial( const char *insName )
{
	assert(insName);

	std::vector<COBJMaterial *>::iterator it;

	for(it=m_Materials.begin();it!=m_Materials.end();++it)
		if(strcmp((*it)->m_sName.c_str(),insName)==0)
			return *it;
//			assert(0);			// material is already existing

	COBJMaterial *ret=new COBJMaterial(insName,(uint32)m_Materials.size());

	m_Materials.push_back(ret);

	return ret;
}


void COBJFileFormat::MessageAdd( const char *szStr, ... )
{
	// todo
}


void COBJFileFormat::ErrorAdd( const char *szStr, ... )
{
	// todo
	assert(0);
}


/*
uint32 COBJFileFormat::GetMaterialCount() const
{
	return (uint32)m_Materials.size();
}

const COBJMaterial *COBJFileFormat::GetMaterials() const
{
	return (const COBJMaterial *)&m_Materials[0];
}
*/
