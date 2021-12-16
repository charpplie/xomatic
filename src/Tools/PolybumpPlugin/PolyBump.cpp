

//////////////////////////////////////////////////////////////////////
//
//	Crytek SuperFramework Source code
//	
//	File:PolyBump.cpp
//  Description: Creates a poly bump with 2 models (low and high resolution) as input 
//
//	History:
//	-March 15,2001:Created by Marco Corbetta
//  -July 13,2001: Rewritten from scratch by Marco Corbetta
//  -2/21/2002 modified to work in 3DStudio max as a thread (MM)
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"

#include <vector>																			// std::vector
#include <list>																				// std::list
#include <map>																				// std::map
#include "TGA.h"																			// PIX_LoadTGA32
//#include "list.h"																			// list2
#include "SimpleIndexedMesh.h"												// CSimpleIndexedMesh	
#include "PolyBump.h"																	// 
#include "PolyBumpWorkerThread.h"											// CPolyBumpWorkerThread
#include "CopyProtection.h"														// g_szSerialNo
#include "KeyGen.h"																		// SimpleCheck() SerialNo
#include "MAC_Address.h"															// NetbiosHelper_CheckCRC
#include "Crytek_Logo.h"															// Crytek Logo
#include "Cry_Math.h"																	//
#include "ChooseIntersection.h"												// Choose..Intersection
#include "SRFData.h"																	// CSRFData
#include "ResourceCompilerHelper.h"										// CResourceCompilerHelper
#include "TriangleInputProxy.h"												// CTriangleInputProxy
#include <omp.h>																			// OpenMP




typedef unsigned char UBYTE;


struct SSamplePointContext
{
	// constructor
	SSamplePointContext( CPbHighMesh *pHighMesh, CPbHighMesh *pCageMesh ) 
		:m_pHighMesh(pHighMesh), m_pCageMesh(pCageMesh), m_nAngleMaxCount(0), m_nHorizonTilt(5)
	{
		assert(pHighMesh);
	}

	CPbHighMesh *			m_pHighMesh;					//!<
	CPbHighMesh *			m_pCageMesh;					//!<
	float							m_fLowPolyObjectSize;	//!<
	uint32						m_nAngleMaxCount;			//!< defines the used array elements in m_fHorizon 0=no additional ray shooting..31
	uint32						m_nHorizonTilt;				//!< Quality of the horizon angle value (count = 1<<m_nHorizonTilt)
};



struct SSamplePointData
{
	// constructor
	SSamplePointData( SSamplePointContext &rContext, CPbLowTri *pTriLow ) 
		:m_pTriLow(pTriLow), m_rContext(rContext)
	{
		m_dwHighPolyTri=0xffffffff;
		m_vSumHighPolyPos=Vec3(0,0,0);
		m_vSumHighPolyNormal=Vec3(0,0,0);
		//		m_vSumDiffuseColor=Vec3(0,0,0);

		for(uint32 dwI=0;dwI<rContext.m_nAngleMaxCount;++dwI)
			m_fSumHorizonAngle[dwI]=0;

//		hitListHighPoly.reserve(160*64);		 //todo
	}

	//
	void GetHighPolyPosAndNormal( Vec3 &vPos, Vec3 &vNormal ) const
	{
		vPos=m_vSumHighPolyPos;
		vNormal=m_vSumHighPolyNormal;
	}
	//
	void SetHighPolyPosAndNormal( const Vec3 &vPos, const Vec3 &vNormal )
	{
		assert(vNormal.len()<=1.0f);
		m_vSumHighPolyPos+=vPos;
		m_vSumHighPolyNormal+=vNormal;
	}
	//
	/*	void SetDiffuseColor( const Vec3 &vCol )
	{
	m_vSumDiffuseColor+=vCol;
	}
	*/
	//
	void SetHorizon( const uint32 dwAngleNo, const float fAngle )
	{
		assert(fAngle>=0 && fAngle<=(gf_PI/2));
		assert(dwAngleNo>=0 && dwAngleNo<=m_rContext.m_nAngleMaxCount);

		m_fSumHorizonAngle[dwAngleNo]+=fAngle;
	}

	//
	float GetHorizon( const uint32 dwAngleNo )
	{
		return m_fSumHorizonAngle[dwAngleNo];
	}

	SSamplePointContext &		m_rContext; 
	uint32									m_dwHighPolyTri;			//!< 0xffffffff if not set yet
	CPbLowTri *							m_pTriLow;						//!<

//	CIntInfoHighList hitListHighPoly;					

private: // ------------------------------------------------------------

	//	Vec3									m_vSumDiffuseColor;		//!< (0..1,0..1,0..1)
	//	float									m_fSumSpecularColor[3];	//!< (0..1,0..1,0..1)
	//	float									m_fSumGloss;							//!< 0..1
	//	float									m_fSumTransparency;			//!< 0..1
	Vec3									m_vSumHighPolyPos;			//!<
	Vec3									m_vSumHighPolyNormal;		//!<
	float									m_fSumHorizonAngle[32];			//!< [0.. m_nAngleMaxCount-1] 0..(gf_PI/2)
		
};







CPbHighMesh::CPbHighMesh() :m_nNumTris(0)
#ifndef USE_MEMORY_BLOCKS
	,m_pMesh(0)
#endif // USE_MEMORY_BLOCKS
{
}


CPbHighMesh::~CPbHighMesh()
{
	FreeData();
}

uint32 CPbHighMesh::GetFaceCount() const
{
	return m_nNumTris;
}


void CPbHighMesh::FreeData()
{
	m_nNumTris=0;
	m_HPRaster.DeInit();

#ifdef 	USE_MEMORY_BLOCKS	//win32 memory optimization
	m_MeshBlocks.clear();
	m_MeshBlocks.resize(0);
#else
	if(m_pMesh)
	{
		delete [] m_pMesh;
		m_pMesh=NULL;
	}    
#endif //USE_MEMORY_BLOCKS
}


// calculate the size of the bounding sphere
float CPbLowMesh::CalcBoundingVolume( Vec3 &outMin, Vec3 &outMax )
{
	Vec3 minbox(FLT_MAX,FLT_MAX,FLT_MAX),maxbox(-FLT_MAX,-FLT_MAX,-FLT_MAX);

	// calculate bounding box
	for(int i=0;i!=m_nNumTris;i++)
		m_pMesh[i].ExtendMinMax(minbox,maxbox);

	// calculate midpoint
	Vec3 vMidPoint=(minbox+maxbox)*0.5f;
	float Radius2=0.0;

	// calculate radius
	for(int i=0;i!=m_nNumTris;i++)
			m_pMesh[i].ExtendSphere(vMidPoint,Radius2);

	outMin=minbox;
	outMax=maxbox;

	return (float)sqrt(Radius2);
}


void CPbHighMesh::CalcBoundingVolume( Vec3 &outMin, Vec3 &outMax )
{
	outMin = Vec3(FLT_MAX,FLT_MAX,FLT_MAX);
	outMax = Vec3(-FLT_MAX,-FLT_MAX,-FLT_MAX);

#ifdef	USE_MEMORY_BLOCKS	//win32 memory optimization

	std::vector<TPbHighTriBlock>::const_iterator it,end=m_MeshBlocks.end();
	
	for(it=m_MeshBlocks.begin();it!=end;++it)
	{
		const TPbHighTriBlock &rBlock = *it;

		TPbHighTriBlock::const_iterator it2,end2=rBlock.end();

		for(it2=rBlock.begin();it2!=end2;++it2)
		{
			const CPbHighTri &rTri = *it2;

			rTri.ExtendMinMax(outMin,outMax);
		}
	}

#else // USE_MEMORY_BLOCKS
	
	for(int ctTriNum=0;ctTriNum!=m_nNumTris;ctTriNum++)
		m_pMesh[ctTriNum].ExtendMinMax(outMin,outMax);

#endif // USE_MEMORY_BLOCKS
}






















// * Build index for tangent space (tangent vector, binormal vector) and build a new index value for every
//   - vertex index
//   - material id
//   - normal index (build from smoothing groups)
//   - texture index
//
// * Calc tangent and binormal per index, sum indices together
//
// * normalize result



// transform the indexed mesh in a convenient format
//////////////////////////////////////////////////////////////////////
void CPbHighMesh::GetNormalsAndClearBaseVectors( const CSimpleIndexedMesh &mesh )
{
	assert(mesh.m_VertCount!=0);			if(mesh.m_VertCount==0)return;

	CObjFace *pTface=mesh.m_pFaces;

	m_nNumTris+=mesh.m_FaceCount;

#ifdef USE_MEMORY_BLOCKS	// win32 opptimization

	uint32 dwCurrentBlock = (uint32)m_MeshBlocks.size();

	if(dwCurrentBlock)
	if(m_MeshBlocks[dwCurrentBlock-1].size()!=g_nObjPerBlock)
		dwCurrentBlock--;

	uint32 dwNewBlockCount = (GetFaceCount()+g_nObjPerBlock-1)/g_nObjPerBlock;

	m_MeshBlocks.resize(dwNewBlockCount);

	CPbHighTri *pTri=0;														// no block allocated
	uint32 dwBlockSizeLeft=0;											// 0 as there is no block allocated

	uint32 dwTrianglesToAdd = mesh.m_FaceCount;
	uint32 ctTriNum=0;

	for(uint32 dwBlock=dwCurrentBlock;dwBlock<dwNewBlockCount;++dwBlock)
	{
		TPbHighTriBlock &rBlock = m_MeshBlocks[dwBlock];

		uint32 dwOldBlockSize = (uint32)rBlock.size();

		uint32 dwNewBlockSize = MIN(dwOldBlockSize+dwTrianglesToAdd,g_nObjPerBlock);

		dwTrianglesToAdd -= dwNewBlockSize-dwOldBlockSize;

		rBlock.resize(dwNewBlockSize);

		assert(dwNewBlockSize);

		TPbHighTriBlock::iterator it2,end2=rBlock.end();

		for(it2=rBlock.begin()+dwOldBlockSize;it2!=end2;++it2,++ctTriNum)
		{
			CPbHighTri *pTri = &(*it2);

#else // USE_MEMORY_BLOCKS

	m_pMesh=new CPbHighTri[mesh.m_FaceCount];

	if(!m_pMesh)
	{
		MessageBox(0,"","low mem 3",MB_OK);
		return;
	}

	// copy data and build triangle data structures (area,area3d,plane)
	{
		// for every triangle
		for(uint32 ctTriNum = 0; ctTriNum<mesh.m_FaceCount; ctTriNum++)
		{	
			CPbHighTri *pTri = &m_pMesh[ctTriNum];

#endif // USE_MEMORY_BLOCKS

			// for every vertex of the triangle
			for(int j=0;j<3;j++)
			{
				assert(pTface->v[j]>=0);
				
				CObjVert *pTvert = &mesh.m_pVerts[pTface->v[j]];

				pTri->m_Verts[j].x = pTvert->x;
				pTri->m_Verts[j].y = pTvert->y;
				pTri->m_Verts[j].z = pTvert->z;
/*
				if(mesh->m_pCoors)
				{
					CObjCoor *pTcoord = &mesh->m_pCoors[pTface->t[j]];	// uv mapping supplied
	  			pTri->m_fS[j]=pTcoord->s;
	  			pTri->m_fT[j]=pTcoord->t;		
				}
				else
				{
	  			pTri->m_fS[j]=0.0f;											// no uv mapping supplied
	  			pTri->m_fT[j]=0.0f;		
				}
*/
				if(mesh.m_pNorms)												// normals supplied
				{
					CObjNorm *pNorms=&mesh.m_pNorms[pTface->n[j]];
					
					assert(pNorms->x>=-1 && pNorms->x<=1);
					assert(pNorms->y>=-1 && pNorms->y<=1);
					assert(pNorms->z>=-1 && pNorms->z<=1);
					
					pTri->m_VertsNormal[j]=SCompressedNormal(Vec3(pNorms->x,pNorms->y,pNorms->z));
				}
				else																			// no normals supplied
					pTri->m_VertsNormal[j]=SCompressedNormal(Vec3(0,0,0));
			} //j

			assert((unsigned)(pTface->shader_id)<256);
			pTri->m_TriMaterialID = (unsigned char)(pTface->shader_id);

			if(!mesh.m_pNorms)
			{
				pTri->m_VertsNormal[0]=pTri->m_VertsNormal[1] = pTri->m_VertsNormal[2] = pTri->ComputeTriangleNormal().GetNormalizedSafe();
			}

			++pTface;
		} //k	
	}

#ifdef USE_MEMORY_BLOCKS	// win32 opptimization
	assert(dwTrianglesToAdd==0);
#endif // USE_MEMORY_BLOCKS
}



// transform the indexed mesh in a convenient format
//////////////////////////////////////////////////////////////////////
void CPbLowMesh::GetNormalsAndClearBaseVectors( CSimpleIndexedMesh &mesh )
{
	assert(mesh.m_VertCount!=0);			if(mesh.m_VertCount==0)return;

	m_pMesh=new CPbLowTri[mesh.m_FaceCount];  

	if(!m_pMesh)
	{
		MessageBox(0,"","low mem 4",MB_OK);
		return;
	}

	m_nNumTris=mesh.m_FaceCount;

	// copy data and build triangle data structures (area,area3d,plane)
	{
		CObjNorm *pNorms=0;
		CPbLowTri *pTri=m_pMesh;
		CObjFace *pTface=mesh.m_pFaces;

		// for every triangle
		for(uint32 k=0;k<mesh.m_FaceCount;k++,pTface++,pTri++)
		{	
			// for every vertex of the triangle
			for(uint32 j=0;j<3;j++)
			{
				assert(pTface->v[j]>=0);
				
				CObjVert *pTvert = &mesh.m_pVerts[pTface->v[j]];

				pTri->m_Verts[j].x = pTvert->x;
				pTri->m_Verts[j].y = pTvert->y;
				pTri->m_Verts[j].z = pTvert->z;

				if(mesh.m_pCoors)
				{
					CObjCoor *pTcoord = &mesh.m_pCoors[pTface->t[j]];	// uv mapping supplied
	  			pTri->m_fS[j]=pTcoord->s;
	  			pTri->m_fT[j]=pTcoord->t;		
				}
				else
				{
	  			pTri->m_fS[j]=0.0f;											// no uv mapping supplied
	  			pTri->m_fT[j]=0.0f;		
				}

				if(mesh.m_pNorms)												// normals supplied
				{
					pNorms=&mesh.m_pNorms[pTface->n[j]];
					pTri->m_VertsNormal[j] = SCompressedNormal(Vec3(pNorms->x,pNorms->y,pNorms->z));
				}
				else																			// no normals supplied
					pTri->m_VertsNormal[j] = SCompressedNormal(Vec3(0,0,0));

				pTri->m_vTangent[j]=Vec3(0,0,0);					// clear tangent base vectors
				pTri->m_vBinormal[j]=Vec3(0,0,0);
				pTri->m_vTNormal[j]=Vec3(0,0,0);
			} //j

			pTri->RefreshInternals();										// this is neccessray to get rayshooting working
		} //k	
	}
}







void CPbLowMesh::BuildNormalsAndBaseVectors( CSimpleIndexedMesh &mesh )
{
	GetNormalsAndClearBaseVectors(mesh);

	CTangentSpaceCalculation<CTriangleInputProxy> tangents;
	CTriangleInputProxy Input(mesh);

	tangents.CalculateTangentSpace(Input);

	// store the calculated data
	{
		CPbLowTri *pTri=m_pMesh;
		CObjFace *pTface=mesh.m_pFaces;

		// for every triangle
		for(uint32 k=0;k<(uint32)mesh.m_FaceCount;k++,pTri++)
		{	
			uint32 dwBaseIndx[3];

			tangents.GetTriangleBaseIndices(k,dwBaseIndx);

			assert((unsigned)(mesh.m_pFaces[k].shader_id)<256);
			pTri->m_TriMaterialID = (unsigned char)(mesh.m_pFaces[k].shader_id);

			// for every vertex of the triangle
			for(int j=0;j<3;j++)
			{
				float vU[3],vV[3],vN[3];

				tangents.GetBase(dwBaseIndx[j],vU,vV,vN);

				pTri->m_vBinormal[j]=Vec3(vU[0],vU[1],vU[2]);
				pTri->m_vTangent[j]=Vec3(vV[0],vV[1],vV[2]);
				pTri->m_vTNormal[j]=Vec3(vN[0],vN[1],vN[2]);
			}
		}
	}
}














void CalcHorizon( SSamplePointData &rSamplePointData, const Vec3 &vBaseA, const Vec3 &vBaseB, const Vec3 &vBaseN, const Vec3 &vBasePos )
{
	Vec3	vFrom = vBasePos + vBaseN*rSamplePointData.m_rContext.m_fLowPolyObjectSize*0.002f;		// slight offset from the surface

	// hemisphere lighting implemented with optimized algorithm (fast, good approximation)
	//
	// algorithm works like this:
	//
	//  To sample the whole hemisphere by dividing it into wedges (nAngleMaxCount).
	//  For every wedge a shadow horizon is searched (binary search, nTiltMaxCount times => resolution:2^nTiltMaxCount) 
	float fSkyArea=0.0f;		
	float walker=0.0f;
	int nTiltMaxCount=rSamplePointData.m_rContext.m_nHorizonTilt;
	const float walkerstep=gf_PI2/(float)rSamplePointData.m_rContext.m_nAngleMaxCount;

	for(uint32 w=0;w<rSamplePointData.m_rContext.m_nAngleMaxCount;w++,walker+=walkerstep)
	{
		float s=sinf(walker),c=cosf(walker);
		float tiltanglepart=0.25f*gf_PI;			// start at 45 degree
		float deltapart=tiltanglepart*0.5f;		// start with 45/2 degree steps and half them every iteration

		for(int t=0;t<nTiltMaxCount;t++)
		{
			float tilt=sinf(tiltanglepart);
			float rndx,rndy,rndz;

			rndx=s*tilt; rndy=c*tilt; rndz=(float)sqrt(1.0f-tilt);

			Vec3 vDir=vBaseA*rndx + vBaseB*rndy + vBaseN*rndz;

			if(!rSamplePointData.m_rContext.m_pHighMesh->CalcIntersection(vFrom,vFrom+vDir*rSamplePointData.m_rContext.m_fLowPolyObjectSize,rSamplePointData.m_dwHighPolyTri))
				tiltanglepart+=deltapart;
				else
				tiltanglepart-=deltapart;

			deltapart*=0.5f;
		}

		rSamplePointData.SetHorizon(w,tiltanglepart);
	}


/*
	// hemisphere lighting implemented with simple Monte-Carlo sampling (slow but correct)
	int nCount=0;
	const int nMaxCount=64*6;
	for(int i=0;i<nMaxCount;i++)		// nMaxCount samples
	{
		Vec3 vTo;

		// get ray in a sphere
		do
		{
			vTo=Vec3((rand()%1024)*(1.0f/512.0f)-1.0f,
								(rand()%1024)*(1.0f/512.0f)-1.0f,
								(rand()%1024)*(1.0f/512.0f)-1.0f);

		} while(vTo.Length2()>1.0f);

		if(vTo*highresnorm<0.0f)vTo*=-1.0f;			// wrong side of hermisphere, flip it

		// get ray on a sphere
		vTo.Normalize();

		vTo=vFrom+vTo*infRayScale;

		if(!rContext.m_pHighMesh->CalcIntersection(vFrom,vTo))
			nCount++;
	}
	*outpAccessability=(float)nCount/(float)nMaxCount;
*/
}


// /param inpCageMesh might be 0
// /param outpAccessability 0 or pointer to a float, will be in the range [0..1]
// /param outOccDirection 0 if is outpAccessability 0 as well or pointer to the Vec3
bool GetSamplePoint( SSamplePointData &rSamplePointData, float infRayLength,
										 float infS, float infT, CPolyBumpWorkerThread *inpData, bool inbDoCLipping )
{
	Vec3 lowresnorm,mid,highresnorm,respoint;

	uint32 dwWidth=inpData->m_Properties.GetOutputWidth();
	uint32 dwHeight=inpData->m_Properties.GetOutputHeight();

	float deltamapx=1.0f/dwWidth;
	float deltamapy=1.0f/dwHeight;

	//get interpolated point and normal using barycentric coords from texel's uv
	rSamplePointData.m_pTriLow->CalcBarycentricCoordsFromUV( infS,infT,respoint,lowresnorm,inbDoCLipping);

	// normalize lowresnorm
	{
		float fL2 = lowresnorm.GetLengthSquared();

		if(fL2<=0)
			return false;

		lowresnorm/=sqrtf(fL2);
	}

	mid=respoint;

	float fMinDist=-infRayLength, fMaxDist=infRayLength;

	// fill in hits with the cage, update fMinDist and fMaxDist
	if(rSamplePointData.m_rContext.m_pCageMesh)
	{
		Vec3 vEnd=mid+lowresnorm*fMaxDist;
		Vec3 vBegin=mid+lowresnorm*fMinDist;

		rSamplePointData.m_rContext.m_pCageMesh->GatherRayHitsHP_Nearest2Sides(vBegin,vEnd,fMinDist,fMaxDist);
		//
/*		static CIntInfoHighList hitListCage;				hitListCage.clear();		hitListCage.reserve(64);

		rSamplePointData.m_rContext.m_pCageMesh->GatherRayHitsHP(vBegin,vEnd,hitListCage);

		CIntInfoHighList::const_iterator i, end=hitListCage.end();

		for(i=hitListCage.begin();i!=end;i++)
		{
			float fDistFromMid = (*i).m_fDist - infRayLength;

			if(fDistFromMid>0)
			{
				if(fDistFromMid<fMaxDist)
					fMaxDist=fDistFromMid;
			}
			else
			{
				if(fDistFromMid>fMinDist)
					fMinDist=fDistFromMid;
			}
		}
*/
	}


	// m.m. optimization (I shoot only one ray - because both rays have the same direction,
	// the orientation doesn't matter )
	Vec3 vEnd=mid+lowresnorm*fMaxDist;
	Vec3 vBegin=mid+lowresnorm*fMinDist;

/*
	CIntInfoHighList hitListHighPoly;				hitListHighPoly.reserve(64);

	// fill in hits with the high poly
	rSamplePointData.m_rContext.m_pHighMesh->GatherRayHitsHP(vBegin,vEnd,hitListHighPoly);

	uint32 cb2=0xffffffff;

	switch(inpData->m_Properties.m_nHitMode)
	{	
		// nearest hit
		case 0: cb2=ChooseNearestIntersection(hitListHighPoly,vBegin,vEnd,lowresnorm,respoint);break;
		// latest hit
		case 1: cb2=ChooseLatestIntersection(hitListHighPoly,vBegin,vEnd,lowresnorm,respoint);break;

		default:
			assert(0);
	}
*/

	Vec3 vDir=vEnd-vBegin;
	{
		float fLength=vDir.GetLength();
		assert(fLength!=0.0f);		// otherwise float exception will occur
		vDir/=fLength;									// normalize
	}

	CIntersInfo<CPbHighTri> Hit;

	switch(inpData->m_Properties.m_nHitMode)
	{	
		case 0: 		// nearest hit
			Hit = rSamplePointData.m_rContext.m_pHighMesh->GatherRayHitsHP_Nearest(vBegin,vEnd,lowresnorm);
			break;

		case 1:			// latest hit
			Hit = rSamplePointData.m_rContext.m_pHighMesh->GatherRayHitsHP_Latest(vBegin,vEnd,lowresnorm);
			break;

		default:
			assert(0);
	}

	//	if(!Hit.m_pTri) 
//		Hit = ChooseNearestAcceptableIntersection(rSamplePointData.hitListHighPoly,vBegin,vEnd,lowresnorm,respoint);

	if(!Hit.m_pTri) 
		return false;																														// no triangle at this texel

	uint32 cb=Hit.m_dwObjectIndex;

//	respoint = Hit.m_Point;
	respoint = Hit.ComputeIntersectionPoint(vBegin,vDir);

//	CPbHighTri *cbPtr = rSamplePointData.m_rContext.m_pHighMesh->GetTrianglePtr(Hit.m_dwObjectIndex);
	CPbHighTri *cbPtr = Hit.m_pTri;

	assert(cbPtr);

	cbPtr->CalcBarycentricCoordsFromPoint(respoint,highresnorm,false);

	rSamplePointData.m_dwHighPolyTri=cb;
	rSamplePointData.SetHighPolyPosAndNormal(respoint,highresnorm);

/*
	static const Vec3 ColTable[]=
	{
//		Vec3(0,0,0),
		Vec3(0,0,1),
		Vec3(0,1,0),
		Vec3(0,1,1),
		Vec3(1,0,0),
		Vec3(1,0,1),
		Vec3(1,1,0),
		Vec3(1,1,1)
	};

	rContext.SetDiffuseColor( ColTable[ cbPtr->m_TriMaterialID%(sizeof(ColTable)/sizeof(Vec3)) ] );
*/

	return true;
}



static bool Difference( const Vec3 inA, const Vec3 inB, const float infTreshold )
{
	float dist2=(inA-inB).len2();

	return dist2>=infTreshold;
}



static bool DoHereAntialias( CSRFData &rSurface, uint32 x, uint32 y, uint32 dwWidth, uint32 dwHeight, CPbLowMesh *pLowMesh, const float fAntialiasTreshold )
{
	Vec3 vNormal,vNext;
	
	vNormal = rSurface.GetHighPolyNormal(x,y);

	if(x!=0)																														// left
	{
		vNext = rSurface.GetHighPolyNormal(x-1,y);
		
		if(Difference(vNormal,vNext,fAntialiasTreshold))
			return true;
	}

	if(y!=0)																														// top
	{
		vNext = rSurface.GetHighPolyNormal(x,y-1);

		if(Difference(vNormal,vNext,fAntialiasTreshold))
			return true;
	}

	if(x!=dwWidth-1)																										// right
	{
		vNext = rSurface.GetHighPolyNormal(x+1,y);

		if(Difference(vNormal,vNext,fAntialiasTreshold))
			return true;
	}

	if(y!=dwHeight-1)																										// down
	{
		vNext = rSurface.GetHighPolyNormal(x,y+1);

		if(Difference(vNormal,vNext,fAntialiasTreshold))
			return true;
	}

	return false;
}

// is this texel on a border? - then do centroid sampling
bool IsBorderPixel( const uint32 dwX, const uint32 dwY, const CSRFData &rSRFData )
{
	if(dwX!=0)if(rSRFData.GetLowPolyTriangleId(dwX-1,dwY)==0xffff)return true;
	if(dwY!=0)if(rSRFData.GetLowPolyTriangleId(dwX,dwY-1)==0xffff)return true;
	if(dwX!=rSRFData.GetWidth()-1)if(rSRFData.GetLowPolyTriangleId(dwX+1,dwY)==0xffff)return true;
	if(dwY!=rSRFData.GetHeight()-1)if(rSRFData.GetLowPolyTriangleId(dwX,dwY+1)==0xffff)return true;

	return false;
}





//create a polybump with 2 meshes as input
//////////////////////////////////////////////////////////////////////
bool CreateBumpMap( CPolyBumpWorkerThread *inpData )
{
	CSRFData &rSRFData = inpData->m_SRFData;

	inpData->DebugLog("CreateBumpMap() started --------------------------------");

	try
	{
		{
			std::string str = inpData->m_HighMeshData.BuildRayAccelerator(inpData->m_Properties.m_bOutputDebugInfo);

			inpData->DebugLog("build high poly 4 (%s)",str.c_str());
		}
  }
	catch (CMemoryException* )
	{
		rSRFData.FreeData();

		MessageBox(0,"can't allocate memory","RayAccelerator mesh ",MB_OK);
		return false;
	}



	bool bEvaluationVersion=false;

	inpData->m_bUserHasStopped=false;

	// check input
	{
		CSimpleIndexedMesh &m1=inpData->GetLowMesh();

		assert(m1.GetUVCount());		// needs UV assignment

		assert(m1.m_VertCount!=0);
	}

	uint32 dwHighPolyFaceCount = inpData->m_HighMeshData.GetFaceCount();

	uint32 dwWidth = rSRFData.GetWidth();
	uint32 dwHeight = rSRFData.GetHeight();

	{
		uint32 m,d,y;
		UBYTE HardwareID[4];

		if(!CCrytekKeyGen::FurtherCheck(g_szSerialNo,"PB",m,d,y,HardwareID)											// Check serial number PB=PolyBump
		|| !NetbiosHelper_CheckCRC(HardwareID))
		{
			char str2[2048];

			sprintf(str2,"Unregistered Version (HardwareID dependent SerialNo is missing or wrong)\n\n"\
					"This trial version is is limited to %dx%d texture size and max. %d triangles\n\n"\
					"Check the AboutBox for information about getting the registered version.",TRIALVERSION_MAXTEXTURESIZE,TRIALVERSION_MAXTEXTURESIZE,TRIALVERSION_MAXHIGHPOLYCOUNT);
			MessageBox(0,str2,"PolyBumpPlugin",MB_OK);

			if(dwWidth>TRIALVERSION_MAXTEXTURESIZE || dwHeight>TRIALVERSION_MAXTEXTURESIZE || dwHighPolyFaceCount>TRIALVERSION_MAXHIGHPOLYCOUNT)
				return false;

			bEvaluationVersion=true;
		}
	}

	rSRFData.AllocSurfaceHighPos();	// allocate space for the displacement map
	
  // allocate space for the normal map
	if(!rSRFData.AllocSurfaceWorldNormals())
	{
		MessageBox(0,"","low mem 8a",MB_OK);
		return false;
	}

	// allocate space for the diffuse map
	if(!rSRFData.AllocSurfaceHighPolyMaterialId())
	{
		MessageBox(0,"","low mem 8b",MB_OK);
		return false;
	}

/*
	// allocate space for the diffuse map
	if(!rSRFData.AllocSurfaceDiffuse())
	{
		MessageBox(0,"","low mem 8c",MB_OK);
		return false;
	}
*/

	// -------------------------------------------------------------------------------

  inpData->m_nImageSizeX=dwWidth;
  inpData->m_nImageSizeY=dwHeight;
	float deltamapx=1.0f/dwWidth;
	float deltamapy=1.0f/dwHeight;

	CPbLowMesh *pLowMesh=new CPbLowMesh;

	inpData->DebugLog("build low poly 1");

	int iMaterialId = inpData->m_Properties.GetLowPolyMaterialId(inpData->GetLowMesh());

	// build low poly data structure + accelerator structure
	{
		CSimpleIndexedMesh &m1 = inpData->GetLowMesh();

		assert(m1.m_VertCount!=0);

		pLowMesh->BuildNormalsAndBaseVectors(m1);

		inpData->DebugLog("build low poly 2 (triangle count: %d)",pLowMesh->m_nNumTris);

		pLowMesh->BuildRayAccelerator(inpData->m_Properties.m_bOutputDebugInfo);

		m1.FreeData();
	}

	// set lowpolymesh into rSRFData
	{
		rSRFData.AllocSourceLowPoly(pLowMesh->m_nNumTris);

		for(uint32 dwTri=0;dwTri<pLowMesh->m_nNumTris;++dwTri)
		{
			CPbLowTri &rTri = pLowMesh->m_pMesh[dwTri];

			for(uint32 dwI=0;dwI<3;++dwI)		// all 3 vertices of the triangle
			{	
				rSRFData.SetLowPolyVertex(dwTri,dwI,
					rTri.m_Verts[dwI],																																		// pos
					rTri.m_vTangent[dwI],rTri.m_vBinormal[dwI],rTri.m_VertsNormal[dwI].GetDecompressed(),	// tangents    (todo: check if m_vTangent and m_vBinormal should not swap places)
					rTri.m_fS[dwI],rTri.m_fT[dwI]);																												// UV
			}
		}
	}


	// build cage data structure + accelerator structure
	CPbHighMesh *pCageMesh=0;
	{
		CSimpleIndexedMesh &m3 = inpData->GetCageMesh();

		if(m3.m_VertCount)
		{
			pCageMesh=new CPbHighMesh();

			inpData->DebugLog("build cage 1");

			pCageMesh->GetNormalsAndClearBaseVectors(m3);

			inpData->DebugLog("build cage poly 2 (triangle count: %d)",pCageMesh->m_nNumTris);

			pCageMesh->BuildRayAccelerator(inpData->m_Properties.m_bOutputDebugInfo);

			m3.FreeData();
		}
	}

	if(!inpData->ThreadPollMessages(0,"Init")) 
		inpData->m_bUserHasStopped=true;

	// draw tris pointer to keep track of the low mesh triangles
	if(!inpData->m_bUserHasStopped)
		inpData->DrawTrisPointers(rSRFData.GetLowPolyTriangleIdBuffer(),pLowMesh,iMaterialId);

	SSamplePointContext SamplePointContext(&inpData->m_HighMeshData,pCageMesh);

	Vec3 vMin,vMax;
	
	SamplePointContext.m_fLowPolyObjectSize = pLowMesh->CalcBoundingVolume(vMin,vMax);

	float rayscale=inpData->m_Properties.m_fRayLength*SamplePointContext.m_fLowPolyObjectSize*2.0f;	// *2.0f because diameter not radius should be used

	//  for good results (2^nTiltMaxCount) should be nearly the same as (nAngleMaxCount*3.14)
	switch(inpData->m_Properties.m_iHorizonMode)
	{
		case 0:		break;		// none
		case 1:		SamplePointContext.m_nAngleMaxCount=6;SamplePointContext.m_nHorizonTilt=4;break;
		case 2:		SamplePointContext.m_nAngleMaxCount=12;SamplePointContext.m_nHorizonTilt=5;break;
		case 3:		SamplePointContext.m_nAngleMaxCount=16;SamplePointContext.m_nHorizonTilt=6;break;
		default: assert(0);
	}
/*
	{
		char str[256];

		sprintf(str,"m_nAngleMaxCount=%d, m_nHorizonTilt=%d\n",SamplePointContext.m_nAngleMaxCount,SamplePointContext.m_nHorizonTilt);
		OutputDebugString(str);
	}
*/
	// allocate space for the accessibity map
	if(SamplePointContext.m_nAngleMaxCount)
		rSRFData.AllocSurfaceHorizon(SamplePointContext.m_nAngleMaxCount);

	{
		uint32 m,d,y;
		UBYTE HardwareID[4];

		if(!CCrytekKeyGen::FurtherCheck(g_szSerialNo,"PB",m,d,y,HardwareID)											// Check serial number PB=PolyBump
		|| !NetbiosHelper_CheckCRC(HardwareID))
		{
			if(dwWidth>TRIALVERSION_MAXTEXTURESIZE || dwHeight>TRIALVERSION_MAXTEXTURESIZE || dwHighPolyFaceCount>TRIALVERSION_MAXHIGHPOLYCOUNT)
				inpData->m_bUserHasStopped=true;
		}
	}


	if(!inpData->m_bUserHasStopped)
		if(!inpData->ThreadPollMessages(1,"Build")) inpData->m_bUserHasStopped=true;

	if(inpData->m_Properties.m_iAntiAliasingMode)
		rSRFData.AllocAALevel();

	inpData->DebugLog("build start calculation");

	volatile bool bStop=false;

	if(!inpData->m_bUserHasStopped)
	if(inpData->m_Properties.DoNonAAStep())
	{
		uint32	dwFullProgress = rSRFData.CalcUsedAreaSize();	// to calculate the progress
		uint32	dwCurrentProgress = 0;

		// todo: dwCurrentProgress
		#pragma omp parallel for schedule(dynamic, 1), shared(bStop)
		for(int32 y=0;y<dwHeight;y++)
		{
			if(bStop)
				continue;

			float mapt = (y+0.5f)*deltamapy;											// correct half pixel

			inpData->SetProgressState(y,CPolybumpProgress::EPPS_InProgress);

			if(omp_get_thread_num() == 0)
			{
				const char *szInfoText=	inpData->m_Properties.m_iAntiAliasingMode==0 ? "Pass 1/1" : "Pass 1/2";

				if(!inpData->ThreadPollMessages((dwCurrentProgress*100.0f)/(float)dwFullProgress,szInfoText))
				{ 
					inpData->m_bUserHasStopped=true;
					bStop=true;continue;
				}
			}
			
			float maps=deltamapx*0.5f;																					// correct half pixel
			for(uint32 x=0;x<dwWidth;x++,maps+=deltamapx)
			{    
				uint16 wLowPolyTriangle = rSRFData.GetLowPolyTriangleId(x,y);

				if(wLowPolyTriangle==0xffff)
					continue;													 // no tri at this texel

				dwCurrentProgress++;																									// next texel (for progress)

				SSamplePointData SamplePointData(SamplePointContext,&pLowMesh->m_pMesh[wLowPolyTriangle]);

				bool bDoClipping=IsBorderPixel(x,y,rSRFData);				// is this texel on a border? - then do centroid sampling

				if(!GetSamplePoint(SamplePointData,rayscale,maps,mapt,inpData,bDoClipping))
					continue;

				{
					Vec3 vPos,vNormal;	//,vDiffuse = SamplePointContext.GetDiffuse();

					SamplePointData.GetHighPolyPosAndNormal(vPos,vNormal);

					rSRFData.SetHighPos(x,y,vPos);
					rSRFData.SetWorldHighPolyNormal(x,y,vNormal);

					CPbHighTri *pHighPolyTri = inpData->m_HighMeshData.GetTrianglePtr(SamplePointData.m_dwHighPolyTri);

					if(pHighPolyTri)
						rSRFData.SetHighPolyMaterialId(x,y,pHighPolyTri->m_TriMaterialID+1);

//					rSRFData.SetDiffuse(x,y,vDiffuse);
				}

				// then get the interpolated normal on the high poly mesh using barycentric coords
				// calculate one accessibility pixel
				if(SamplePointContext.m_nAngleMaxCount)							// !=0 means horizon should be calculated
				{
					Vec3 vBaseA,vBaseB,vBaseN,vPos;

					rSRFData.CalcHighPolyBase(x,y,vBaseA,vBaseB,vBaseN);
					vPos=rSRFData.GetHighPos(x,y);

					CalcHorizon(SamplePointData,vBaseA,vBaseB,vBaseN,vPos);
				}

				for(uint32 dwI=0;dwI<SamplePointContext.m_nAngleMaxCount;++dwI)
					rSRFData.SetHorizon(x,y,dwI,SamplePointData.GetHorizon(dwI));
			} //x

			inpData->SetProgressState(y,inpData->m_Properties.m_iAntiAliasingMode ? CPolybumpProgress::EPPS_DoneP1 : CPolybumpProgress::EPPS_DoneFinal);
		} //y
	}


	if(!inpData->m_bUserHasStopped)
	if(inpData->m_Properties.m_iAntiAliasingMode)
	{
		float fAntialiasStep,fAntialiasTreshold;

		inpData->m_Properties.GetAntialiasingParam(fAntialiasStep,fAntialiasTreshold);

		for(uint32 y=0;y<dwHeight;y++)
		for(uint32 x=0;x<dwWidth;x++)
		{    
			if(rSRFData.GetLowPolyTriangleId(x,y)==0xffff)
				continue;																					// no tri at this texel

			if(DoHereAntialias(rSRFData,x,y,dwWidth,dwHeight,pLowMesh,fAntialiasTreshold))
				rSRFData.SetAALevel(x,y,1);
		}

		uint32	dwFullProgress = rSRFData.CalcAALevelSum();	// to calculate the progress
		uint32	dwCurrentProgress = 0;

		// adaptive Anti aliasing

		#pragma omp parallel for schedule(dynamic, 1) shared(bStop)
		for(int32 y=0;y<dwHeight;y++)
		{    
			if(bStop)
				continue;

			float mapt = (y+0.5f)*deltamapy;											// correct half pixel

			inpData->SetProgressState(y,CPolybumpProgress::EPPS_InProgress);

			if(omp_get_thread_num() == 0)
			{
				const char *szInfoText=inpData->m_Properties.DoNonAAStep() ? "Pass 2/2" : "Pass 1/1";

				if(!inpData->ThreadPollMessages((dwCurrentProgress*100.0f)/(float)dwFullProgress,szInfoText))
				{ 
					inpData->m_bUserHasStopped=true;
					bStop=true;continue;
//					break; 
				}
			}

			float maps=deltamapx*0.5f;																					// correct half pixel
			for(uint32 x=0;x<dwWidth;x++,maps+=deltamapx)
			{    
				uint8 ucAALevel=rSRFData.GetAALevel(x,y);

				if(!ucAALevel)
					continue;																								// no antialiasing at this texel

				uint16 wLowPolyTriangle = rSRFData.GetLowPolyTriangleId(x,y);

				assert(wLowPolyTriangle!=0xffff);													// no tri at this texel

				dwCurrentProgress+=(uint32)ucAALevel;											// next texel (for progress)

				bool bDoClipping=IsBorderPixel(x,y,rSRFData);				// is this texel on a border? - then do centroid sampling

				SSamplePointData SamplePointData(SamplePointContext,&pLowMesh->m_pMesh[wLowPolyTriangle]);

				float stepherex=fAntialiasStep*deltamapx, stepherey=fAntialiasStep*deltamapy;

				float fAntialiasingSampleCount=0.0f;			// in float to avoid int->float cast

				// all non horizon data
				{
					for(float suby=(fAntialiasStep*0.5f-0.5f)*deltamapy;suby<=0.5f*deltamapy;suby+=stepherey)
					for(float subx=(fAntialiasStep*0.5f-0.5f)*deltamapx;subx<=0.5f*deltamapx;subx+=stepherex)
					{
						float jitterx=0.0f,jittery=0.0f;

						if(inpData->m_Properties.DoAAJittering())
						{
							jitterx=((rand()%1024)*(1.0f/1024.0f)-0.5f)*stepherex;
							jittery=((rand()%1024)*(1.0f/1024.0f)-0.5f)*stepherey;
						}

						if(!GetSamplePoint(SamplePointData,rayscale,maps+subx+jitterx,mapt+suby+jittery,inpData,bDoClipping))
							continue;

						fAntialiasingSampleCount+=1.0f;
					}
				}

				if(fAntialiasingSampleCount)
				{
					float fAntialiasSampleWeight=1.0f/fAntialiasingSampleCount;
					
					{
						Vec3 vPos,vNormal;	//,vDiffuse = SamplePointContext.GetDiffuse();

						SamplePointData.GetHighPolyPosAndNormal(vPos,vNormal);

						rSRFData.SetHighPos(x,y,vPos*fAntialiasSampleWeight);
						rSRFData.SetWorldHighPolyNormal(x,y,vNormal*fAntialiasSampleWeight);
//						rSRFData.SetDiffuse(x,y,vDiffuse*fAntialiasSampleWeight);
					}
				}
			} //x

			inpData->SetProgressState(y,CPolybumpProgress::EPPS_DoneFinal);
		} //y
	}

  // clean up
  delete pLowMesh;
  inpData->m_HighMeshData.FreeData();
	delete pCageMesh;

	return true;
}












// ray casting, gather unsorted hits
void CPbLowMesh::GatherRayHits( const Vec3 invStart, const Vec3 invEnd, CIntInfoLowList &outIntersections )
{
	Vec3 vDir=invEnd-invStart;

	float fLength=vDir.GetLength();
	
	vDir.Normalize();	// Baustelle normalize?

	CEveryObjectOnlyOnceSTLVector<CPbLowMesh,uint32,CPbLowTri> sTriangleSink(*this,invStart,vDir,fLength,outIntersections);			// Baustelle - static may be faster

	m_LPRaster.GatherRayHitsTo(invStart,invEnd,sTriangleSink);
}



// ray casting, nearest
void CPbHighMesh::GatherRayHitsHP_Nearest2Sides( const Vec3 invStart, const Vec3 invEnd, float &outMin, float &outMax  )
{
	Vec3 vDir=invEnd-invStart;

	float fLength=vDir.GetLength();

	assert(fLength!=0.0f);		// otherwise float exception will occur

	vDir/=fLength;									// normalize

	CEveryObjectOnlyOnce_Nearest2Sides<CPbHighMesh,uint32,CPbHighTri> sTriangleSink(*this,invStart,vDir,fLength);			// Baustelle - static may be faster

	m_HPRaster.GatherRayHitsTo(invStart,invEnd,sTriangleSink);

	outMin = sTriangleSink.m_fBestMin;
	outMax = sTriangleSink.m_fBestMax;
}


// ray casting, nearest
CIntersInfo<CPbHighTri> CPbHighMesh::GatherRayHitsHP_Nearest( const Vec3 invStart, const Vec3 invEnd, const Vec3 vLowPolyNormal )
{
	Vec3 vDir=invEnd-invStart;

	float fLength=vDir.GetLength();

	assert(fLength!=0.0f);		// otherwise float exception will occur

	vDir/=fLength;									// normalize

	CEveryObjectOnlyOnce_Nearest<CPbHighMesh,uint32,CPbHighTri> sTriangleSink(*this,invStart,vDir,fLength,vLowPolyNormal);			// Baustelle - static may be faster

	m_HPRaster.GatherRayHitsTo(invStart,invEnd,sTriangleSink);

	return sTriangleSink.m_FoundIntersection;
}


// ray casting, latest
CIntersInfo<CPbHighTri> CPbHighMesh::GatherRayHitsHP_Latest( const Vec3 invStart, const Vec3 invEnd, const Vec3 vLowPolyNormal )
{
	Vec3 vDir=invEnd-invStart;

	float fLength=vDir.GetLength();

	assert(fLength!=0.0f);		// otherwise float exception will occur

	vDir/=fLength;									// normalize

	CEveryObjectOnlyOnce_Latest<CPbHighMesh,uint32,CPbHighTri> sTriangleSink(*this,invStart,vDir,fLength,vLowPolyNormal);			// Baustelle - static may be faster

	m_HPRaster.GatherRayHitsTo(invStart,invEnd,sTriangleSink);

	return sTriangleSink.m_FoundIntersection;
}

// ray casting, gather unsorted hits
void CPbHighMesh::GatherRayHitsHP( const Vec3 invStart, const Vec3 invEnd, CIntInfoHighList &outIntersections )
{
	Vec3 vDir=invEnd-invStart;

	float fLength=vDir.GetLength();

	assert(fLength!=0.0f);		// otherwise float exception will occur
	
	vDir/=fLength;									// normalize

	CEveryObjectOnlyOnceSTLVector<CPbHighMesh,uint32,CPbHighTri> sTriangleSink(*this,invStart,vDir,fLength,outIntersections);			// Baustelle - static may be faster

	m_HPRaster.GatherRayHitsTo(invStart,invEnd,sTriangleSink);
}


// raytracing, test if there is a hit or not
bool CPbHighMesh::CalcIntersection( const Vec3 invStart, const Vec3 invEnd, const uint32 indwIgnoreObject )
{
	Vec3 vDir=invEnd-invStart;

	float fLength=vDir.GetLength();

	vDir/=fLength;									// normalize

	CEveryObjectOnlyOnceBool<CPbHighMesh,uint32,CPbHighTri> sTriangleSink(*this,invStart,vDir,fLength,indwIgnoreObject);

	m_HPRaster.GatherRayHitsTo(invStart,invEnd,sTriangleSink);

	return sTriangleSink.GetResult();
}


std::string CPbHighMesh::BuildRayAccelerator( const bool inbDebug )
{
	Vec3 vMin,vMax;

	CalcBoundingVolume(vMin,vMax);

	m_HPRaster.Init(vMin,vMax,m_nNumTris,USE_RASTERCUBE_ACCELERATOR_MAGNIFIER);

	for(uint32 dwPass=1;dwPass<=2;++dwPass)
	{
#ifdef USE_MEMORY_BLOCKS	//win32 memory optimization
		std::vector<TPbHighTriBlock>::iterator it,end=m_MeshBlocks.end();
		uint32 i=0;
		
		for(it=m_MeshBlocks.begin();it!=end;++it)
		{
			TPbHighTriBlock &rBlock = *it;

			TPbHighTriBlock::iterator it2,end2=rBlock.end();

			for(it2=rBlock.begin();it2!=end2;++it2,++i)
			{
				CPbHighTri &rTri = *it2;
#else
		{
			for(uint32 i=0;i<m_nNumTris;i++)
			{
				CPbHighTri &rTri = m_pMesh[i];
#endif //USE_MEMORY_BLOCKS

				CVector3D Verts[3];

				Verts[0]=rTri.m_Verts[0];
				Verts[1]=rTri.m_Verts[1];
				Verts[2]=rTri.m_Verts[2];

				m_HPRaster.PutInTriangle(Verts,i); //fix pTri
			}
		}

		if(dwPass==1)
			m_HPRaster.PreProcess(inbDebug);
	}

	m_HPRaster.Test();


//	m_LPRaster.Compress();  deactivated because it seems there is still a bug


	char str[256];
	uint32 iSize[3];
	uint32 byteSize = (uint32)m_HPRaster.CalcMemoryConsumption(iSize);

	sprintf(str,"Acceleration scheme: RasterCube %dx%dx%d ~%d bytes",iSize[0],iSize[1],iSize[2],byteSize);

	std::string sRet=str;
/*
	if(inbDebug)
		sRet+=TestRasterCube();
*/
	return sRet;
}



std::string CPbLowMesh::BuildRayAccelerator( const bool inbDebug )
{
	Vec3 vMin,vMax;

	CalcBoundingVolume(vMin,vMax);

	m_LPRaster.Init(vMin,vMax,m_nNumTris,USE_RASTERCUBE_ACCELERATOR_MAGNIFIER);

	for(uint32 dwPass=1;dwPass<=2;++dwPass)
	{
		for(uint32 i=0;i<m_nNumTris;i++)
		{
			CVector3D Verts[3];

			Verts[0]=m_pMesh[i].m_Verts[0];
			Verts[1]=m_pMesh[i].m_Verts[1];
			Verts[2]=m_pMesh[i].m_Verts[2];

			m_LPRaster.PutInTriangle(Verts,i);
		}

		if(dwPass==1)
			m_LPRaster.PreProcess(inbDebug);
	}

	m_LPRaster.Test();

//	m_Raster.Compress();  deactivated because it seems there is still a bug


	char str[256];
	uint32 iSize[3];
	uint32 byteSize = (uint32)m_LPRaster.CalcMemoryConsumption(iSize);

	sprintf(str,"Acceleration scheme: RasterCube %dx%dx%d ~%d bytes",iSize[0],iSize[1],iSize[2],byteSize);

	std::string sRet=str;
/*
	if(inbDebug)
		sRet+=TestRasterCube();
*/
	return sRet;
}

