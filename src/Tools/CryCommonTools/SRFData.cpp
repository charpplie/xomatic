#include "stdafx.h"
#include "srfdata.h"
#include <math.h>						// fabs()
#include "float.h"					// FLT_MAX
#include "BitmapDilation.h"	// CBitmapDilation

//#define ZLIB_DLL
//#define _WINDOWS
#include "zlib.h"						// compression through zlib


CSRFData::CSRFData() :m_dwWidth(0), m_dwHeight(0), m_dwHorizonCount(0), m_dwLoadedFileVersion(0), m_bNormalsAreInWS(true)
{
}

CSRFData::~CSRFData()
{
	FreeData();
}


void CSRFData::FreeData()
{
	m_SurfaceWorldNormals.FreeData();
	m_SurfaceDiffuse.FreeData();
	m_SurfaceSpecular.FreeData();
	m_SurfaceGloss.FreeData();
	m_SurfaceTransparency.FreeData();
	m_SurfaceHighPos.FreeData();
	m_SurfaceHorizon.FreeData();
	m_SurfaceTriangleId.FreeData();
	m_SurfaceHighMatId.FreeData();
	m_SurfaceAALevel.FreeData();
	m_SourceLowPoly.clear();
	m_sProperties="";

	// don't clear - settings should remain when recomputing
	// m_KeyValuePairs.clear();
}


void CSRFData::Init( const uint32 dwWidth, const uint32 dwHeight )
{
	assert(dwWidth);
	assert(dwHeight);

	FreeData();

	m_dwWidth=dwWidth;
	m_dwHeight=dwHeight;
}


bool CSRFData::AllocSurfaceWorldNormals()
{
	assert(m_dwWidth!=0 && m_dwHeight!=0);				// Init() wasn't called

	SNormal16 null;

	return m_SurfaceWorldNormals.Alloc(m_dwWidth,m_dwHeight,&null);
}

bool CSRFData::AllocAALevel()
{
	assert(m_dwWidth!=0 && m_dwHeight!=0);				// Init() wasn't called

	uint8 null=0;

	return m_SurfaceAALevel.Alloc(m_dwWidth,m_dwHeight,&null);
}

bool CSRFData::AllocSurfaceDiffuse()
{
	assert(m_dwWidth!=0 && m_dwHeight!=0);				// Init() wasn't called

	SRGB8 null;

	return m_SurfaceDiffuse.Alloc(m_dwWidth,m_dwHeight,&null);
}

bool CSRFData::AllocSurfaceHighPolyMaterialId()
{
	assert(m_dwWidth!=0 && m_dwHeight!=0);				// Init() wasn't called

	uint8 null=0;

	return m_SurfaceHighMatId.Alloc(m_dwWidth,m_dwHeight,&null);
}

bool CSRFData::AllocSurfaceSpecular()
{
	assert(m_dwWidth!=0 && m_dwHeight!=0);				// Init() wasn't called

	SRGB8 null;

	return m_SurfaceSpecular.Alloc(m_dwWidth,m_dwHeight,&null);
}

bool CSRFData::AllocSurfaceGloss()
{
	assert(m_dwWidth!=0 && m_dwHeight!=0);				// Init() wasn't called

	float null=0;

	return m_SurfaceGloss.Alloc(m_dwWidth,m_dwHeight,&null);
}

bool CSRFData::AllocSurfaceTransparency()
{
	assert(m_dwWidth!=0 && m_dwHeight!=0);				// Init() wasn't called

	uint8 null=0;

	return m_SurfaceTransparency.Alloc(m_dwWidth,m_dwHeight,&null);
}


bool CSRFData::AllocSurfaceHighPos()
{
	assert(m_dwWidth!=0 && m_dwHeight!=0);				// Init() wasn't called

	Vec3 null(0,0,0);

	return m_SurfaceHighPos.Alloc(m_dwWidth,m_dwHeight,&null);
}

bool CSRFData::AllocSurfaceHorizon( const uint32 dwHorizonCount )
{
	assert(m_dwWidth!=0 && m_dwHeight!=0);				// Init() wasn't called
	assert(dwHorizonCount);

	m_dwHorizonCount=dwHorizonCount;

	uint8 null=0;

	return m_SurfaceHorizon.Alloc(m_dwWidth*m_dwHorizonCount,m_dwHeight,&null);
}







void CSRFData::SetWorldHighPolyNormal( const uint32 dwX, const uint32 dwY, const Vec3 &vNormal )
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceWorldNormals.GetPointer());

	m_SurfaceWorldNormals.Set(dwX,dwY,SNormal16(vNormal));
}

void CSRFData::SetDiffuse( const uint32 dwX, const uint32 dwY, const float vColor[3] )
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceDiffuse.GetPointer());

	SRGB8 col(vColor);

	m_SurfaceDiffuse.Set(dwX,dwY,col);
}

void CSRFData::SetSpecular( const uint32 dwX, const uint32 dwY, const float vColor[3] )
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceSpecular.GetPointer());	
	
	SRGB8 col(vColor);

	m_SurfaceSpecular.Set(dwX,dwY,col);
}

void CSRFData::SetAALevel( const uint32 dwX, const uint32 dwY, const uint8 AALevel )
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceAALevel.GetPointer());

	m_SurfaceAALevel.Set(dwX,dwY,AALevel);
}


void CSRFData::SetGloss( const uint32 dwX, const uint32 dwY, const float fValue )
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceGloss.GetPointer());

	assert(fValue>=0);

	m_SurfaceGloss.Set(dwX,dwY,fValue);
}

void CSRFData::SetTransparency( const uint32 dwX, const uint32 dwY, const float fValue )
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceTransparency.GetPointer());

	assert(fValue>=0 && fValue<=1);

	m_SurfaceTransparency.Set(dwX,dwY,static_cast<uint8>(fValue*255.0f+0.5f));
}

void CSRFData::SetHighPos( const uint32 dwX, const uint32 dwY, const Vec3 &vRelPos )
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceHighPos.GetPointer());

	m_SurfaceHighPos.Set(dwX,dwY,vRelPos);
}

void CSRFData::SetHorizon( const uint32 dwX, const uint32 dwY, const uint32 dwHorizonId, const float &fAngle )
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(dwHorizonId<m_dwHorizonCount);
	assert(m_SurfaceHorizon.GetPointer());


	m_SurfaceHorizon.Set(dwX*m_dwHorizonCount+dwHorizonId,dwY,static_cast<uint8>(fAngle/(gf_PI*0.5f)*255.0f+0.5f));
}

bool CSRFData::IsHorizonValidAt( const uint32 dwX, const uint32 dwY )
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceHorizon.GetPointer());

	uint8 *p = &m_SurfaceHorizon.GetRef(dwX*m_dwHorizonCount,dwY);

	for(uint32 dwI=0;dwI<m_dwHorizonCount;++dwI)
		if(*p++ != 0)
			return true;

	return false; 
}


void CSRFData::SetHighPolyMaterialId( const uint32 dwX, const uint32 dwY, const uint8 MaterialId )
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceHighMatId.GetPointer());
	assert(MaterialId);													// don't call for 0 (no surface hit)

	m_SurfaceHighMatId.Set(dwX,dwY,MaterialId);
}

bool CSRFData::IsThereHighPolyMaterialId() const
{
	return m_SurfaceHighMatId.GetPointer()!=0;
}

uint8 CSRFData::GetHighPolyMaterialId( const uint32 dwX, const uint32 dwY ) const
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceHighMatId.GetPointer());

	return m_SurfaceHighMatId.GetRef(dwX,dwY);
}

void CSRFData::SetLowPolyTriangleId( const uint32 dwX, const uint32 dwY, const uint32 dwTriangleId )
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceTriangleId.GetPointer());

	m_SurfaceTriangleId.Set(dwX,dwY,dwTriangleId);
}

uint16 CSRFData::GetLowPolyTriangleId( const uint32 dwX, const uint32 dwY ) const
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceTriangleId.GetPointer());

	return m_SurfaceTriangleId.GetRef(dwX,dwY);
}


uint16 *CSRFData::GetLowPolyTriangleIdBuffer() const
{
	assert(m_SurfaceTriangleId.GetPointer());

	return m_SurfaceTriangleId.GetPointer();
}


Vec3 CSRFData::GetHighPolyNormal( const uint32 dwX, const uint32 dwY ) const
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceWorldNormals.GetPointer());

	return m_SurfaceWorldNormals.GetRef(dwX,dwY).GetDecompressed();
}


void CSRFData::GetNearestXY( const uint32 dwInX, const uint32 dwInY, uint32 &dwOutX, uint32 &dwOutY ) const
{
	assert(dwInX<m_dwWidth);
	assert(dwInY<m_dwHeight);
	
	if(!m_SurfaceIndirection.GetPointer())
	{
		dwOutX = dwInX;
		dwOutY = dwInY;
		return;
	}

	SIndirectionXY rel = m_SurfaceIndirection.GetRef(dwInX,dwInY);

	dwOutX = (dwInX + rel.x) % m_dwWidth;
	dwOutY = (dwInY + rel.y) % m_dwHeight;
}



bool CSRFData::IsThereDiffuse() const
{
	return m_SurfaceDiffuse.GetPointer()!=0;
}

void CSRFData::GetDiffuse( const uint32 dwX, const uint32 dwY, float outValue[3] ) const
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceDiffuse.GetPointer());

	m_SurfaceDiffuse.GetRef(dwX,dwY).GetDecompressed(outValue);
}


void CSRFData::GetSpecular( const uint32 dwX, const uint32 dwY, float outValue[3] ) const
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceSpecular.GetPointer());

	m_SurfaceSpecular.GetRef(dwX,dwY).GetDecompressed(outValue);
}


float CSRFData::GetGloss( const uint32 dwX, const uint32 dwY ) const
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceGloss.GetPointer());

	return m_SurfaceGloss.GetRef(dwX,dwY);
}


uint8 CSRFData::GetAALevel( const uint32 dwX, const uint32 dwY ) const
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceAALevel.GetPointer());

	return m_SurfaceAALevel.GetRef(dwX,dwY);
}

float CSRFData::GetTransparency( const uint32 dwX, const uint32 dwY ) const
{
	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);
	assert(m_SurfaceTransparency.GetPointer());

	return m_SurfaceTransparency.GetRef(dwX,dwY)/255.0f;
}

// correct tringle area = CalcAreaMul2()/2
// triangle area: http://geometryalgorithms.com/Archive/algorithm_0101/
static float CalcAreaMul2( const float fU0, const float fV0, const float fU1, const float fV1, const float fU2, const float fV2 )
{
	float fdU[2] = { fU1-fU0, fU2-fU0 };
	float fdV[2] = { fV1-fV0, fV2-fV0 };

	return fdU[0]*fdV[1] - fdU[1]*fdV[0];
}

uint16 CSRFData::CalcLowTriangle( const uint32 dwX, const uint32 dwY, Vec3 &vBaryCorrsOut ) const
{
	assert(m_SurfaceTriangleId.GetPointer());

	uint32 dwNearestX,dwNearestY;
	GetNearestXY(dwX,dwY,dwNearestX,dwNearestY);

	uint32 dwTriangleId = (uint32)m_SurfaceTriangleId.GetRef(dwNearestX,dwNearestY);

	if(dwTriangleId==0xffff)			// invalid?
		return dwTriangleId;

	const SSourceTriangle &rTri = m_SourceLowPoly[dwTriangleId];

	float fU=GetU(dwX);
	float fV=GetV(dwY);

	float fWholeTriArea = CalcAreaMul2( rTri.m_Vertex[0].m_fU,rTri.m_Vertex[0].m_fV,
																			rTri.m_Vertex[1].m_fU,rTri.m_Vertex[1].m_fV, 
																			rTri.m_Vertex[2].m_fU,rTri.m_Vertex[2].m_fV );

	if(fabs(fWholeTriArea)<0.000001f)
		return 0xffff;									// invalid

	float fTriArea0 =			CalcAreaMul2( fU,fV,
																			rTri.m_Vertex[1].m_fU,rTri.m_Vertex[1].m_fV, 
																			rTri.m_Vertex[2].m_fU,rTri.m_Vertex[2].m_fV );
	float fTriArea1 =			CalcAreaMul2( rTri.m_Vertex[0].m_fU,rTri.m_Vertex[0].m_fV,
																			fU,fV, 
																			rTri.m_Vertex[2].m_fU,rTri.m_Vertex[2].m_fV );
	vBaryCorrsOut[0] = fTriArea0/fWholeTriArea;
	vBaryCorrsOut[1] = fTriArea1/fWholeTriArea;

	// clipping
	{
		float sub=vBaryCorrsOut.x+vBaryCorrsOut.y-1.0f;
		if(sub>0.0f)
		{
			sub*=0.5f;
			vBaryCorrsOut.x-=sub;
			vBaryCorrsOut.y-=sub;
		}
		if(vBaryCorrsOut.x<0.0f)vBaryCorrsOut.x=0.0f;
		if(vBaryCorrsOut.y<0.0f)vBaryCorrsOut.y=0.0f;
	}

	vBaryCorrsOut.z = 1.0f-vBaryCorrsOut.x-vBaryCorrsOut.y;

	return dwTriangleId;
}



bool CSRFData::CalcLowPos( const uint32 dwX, const uint32 dwY, Vec3 &outValue ) const
{
	assert(m_SurfaceTriangleId.GetPointer());

	Vec3 vBary; 

	uint16 wTriangle = CalcLowTriangle(dwX,dwY,vBary);

	if(wTriangle==0xffff)			// invalid
		return false;

	const SSourceTriangle &rTri = m_SourceLowPoly[wTriangle];

	outValue = rTri.m_Vertex[0].m_Pos*vBary.x + rTri.m_Vertex[1].m_Pos*vBary.y + rTri.m_Vertex[2].m_Pos*vBary.z;

	return true;
}

bool CSRFData::CalcLowNormal( const uint32 dwX, const uint32 dwY, Vec3 &outTangN ) const
{
	assert(m_SurfaceTriangleId.GetPointer());

	Vec3 vBary; 

	uint16 wTriangle = CalcLowTriangle(dwX,dwY,vBary);

	if(wTriangle==0xffff)			// invalid
		return false;

	const SSourceTriangle &rTri = m_SourceLowPoly[wTriangle];

	Vec3 vP0,vP1,vP2;
	
	vP0 = rTri.m_Vertex[0].m_Pos;
	vP1 = rTri.m_Vertex[1].m_Pos;
	vP2 = rTri.m_Vertex[2].m_Pos;

	outTangN = vP0*vBary.x + vP1*vBary.y + vP2*vBary.z;

	return true;
}

bool CSRFData::CalcLowTangentBase( const uint32 dwX, const uint32 dwY, Vec3 &outTangU, Vec3 &outTangV, Vec3 &outTangN,
	const bool bPerVertexOrthogonalizedTS ) const
{
	assert(m_SurfaceTriangleId.GetPointer());

	Vec3 vBary; 

	uint16 wTriangle = CalcLowTriangle(dwX,dwY,vBary);

	if(wTriangle==0xffff)			// invalid
		return false;

	const SSourceTriangle &rTri = m_SourceLowPoly[wTriangle];

	Vec3 vU[3], vV[3], vN[3];

	for(int i=0;i<3;++i)
	{
		vU[i] = rTri.m_Vertex[i].m_TangentU.GetDecompressed();
		vV[i] = rTri.m_Vertex[i].m_TangentV.GetDecompressed();
		vN[i] = rTri.m_Vertex[i].m_TangentN.GetDecompressed();
	}

	if(bPerVertexOrthogonalizedTS)
	for(int i=0;i<3;++i)
	{
		Vec3 vMiddle = (vU[i]+vV[i]).GetNormalized();
		vU[i] = (vMiddle+(vMiddle%vN[i])).GetNormalized();
		vV[i] = (vMiddle+(vN[i]%vMiddle)).GetNormalized();
	}

	outTangU = vU[0]*vBary.x + vU[1]*vBary.y + vU[2]*vBary.z;
	outTangV = vV[0]*vBary.x + vV[1]*vBary.y + vV[2]*vBary.z;
	outTangN = vN[0]*vBary.x + vN[1]*vBary.y + vN[2]*vBary.z;

	return true;
}



Vec3 CSRFData::GetHighPos( const uint32 dwX, const uint32 dwY ) const
{
	assert(m_SurfaceHighPos.GetPointer());

	return m_SurfaceHighPos.GetRef(dwX,dwY);
}

void CSRFData::CalcDisplacementRange( float &outMin, float &outMax, const bool bNPatch, const bool bQuadric ) const
{
	outMax=-FLT_MAX;
	outMin=FLT_MAX;
		
	if(!m_SurfaceHighPos.GetPointer())
		return;

	for(uint32 dwY=0;dwY<m_SurfaceHighPos.GetHeight();++dwY)
	for(uint32 dwX=0;dwX<m_SurfaceHighPos.GetWidth();++dwX)
	{
		float fDisp = CalcDisplacementDist(dwX,dwY,bNPatch,bQuadric);

		if(fDisp!=FLT_MAX)
		{
			outMax=max(outMax,fDisp);
			outMin=min(outMin,fDisp);
		}
	}
}


float CSRFData::CalcDisplacementDist( const uint32 dwX, const uint32 dwY, const bool bNPatch, const bool bQuadric ) const
{
	assert(m_SurfaceHighPos.GetPointer());

	SNormal16 null;

	if(m_SurfaceWorldNormals.GetRef(dwX,dwY)==null)			// displacement only valid if highpolynormal is valid
		return FLT_MAX;

	Vec3 vLowPos,vHighPos,vNormal;

	if(bNPatch)
	{	
		if(!CalcLowPNTrianglePosNormal(dwX,dwY,bQuadric,vLowPos,vNormal))
			return FLT_MAX;
	}
	else
	{
		if(!CalcLowPos(dwX,dwY,vLowPos))
			return FLT_MAX;

		bool bRet=CalcLowNormal(dwX,dwY,vNormal);			assert(bRet);
	}

	vHighPos = GetHighPos(dwX,dwY);

	Vec3 vRelDist= vHighPos-vLowPos;

	return vNormal*vRelDist;
}






void CSRFData::AllocSurfaceIndirection( const uint32 dwPixelCount )
{
	if(m_dwWidth==0 || m_dwHeight==0)
		return;		// not work needed

	int iRadius = (int)dwPixelCount;

	m_SurfaceIndirection.Alloc(m_dwWidth,m_dwHeight);

	// more then 9 needs to be done through multiple calls
	for(;iRadius>0;iRadius-=9)
	{
		CBitmapDilation dilation(m_dwWidth,m_dwHeight);

		SNormal16 null;

		for(uint32 dwY=0;dwY<m_dwHeight;++dwY)
			for(uint32 dwX=0;dwX<m_dwWidth;++dwX)
			{
				SNormal16 Val;

				m_SurfaceWorldNormals.Get(dwX,dwY,Val);

				if(!(Val==null))
					dilation.SetValid(dwX,dwY);
			}

			dilation.SetDilationRadius(min(9,iRadius));

			for(uint32 dwDY=0;dwDY<m_dwHeight;++dwDY)			// dest
				for(uint32 dwDX=0;dwDX<m_dwWidth;++dwDX)		// dest
				{
					uint32 dwSX,dwSY;													// source

					if(dilation.GetBestSampleBorder(dwDX,dwDY,dwSX,dwSY))
						m_SurfaceIndirection.Set(dwDX,dwDY,SIndirectionXY((int)dwSX-(int)dwDX,(int)dwSY-(int)dwDY));
				}
	}
/*
	// horizon might be sampled on different areas
	if(m_SurfaceHorizon.IsValid())
	{
		int iRadius = (int)dwPixelCount;

		// more then 9 needs to be done through multiple calls
		for(;iRadius>0;iRadius-=9)
		{
			CBitmapDilation dilation(m_dwWidth,m_dwHeight);

			for(uint32 dwY=0;dwY<m_dwHeight;++dwY)
				for(uint32 dwX=0;dwX<m_dwWidth;++dwX)
					if(IsHorizonValidAt(dwX,dwY))
						dilation.SetValid(dwX,dwY);

			dilation.SetDilationRadius(min(9,iRadius));

			for(uint32 dwDY=0;dwDY<m_dwHeight;++dwDY)		// dest
				for(uint32 dwDX=0;dwDX<m_dwWidth;++dwDX)		// dest
				{
					uint32 dwSX,dwSY;													// source

					dilation.GetBestSampleBorder(dwDX,dwDY,dwSX,dwSY);

					uint8 *pSrc = &m_SurfaceHorizon.GetRef(dwSX*m_dwHorizonCount,dwSY);
					uint8 *pDst = &m_SurfaceHorizon.GetRef(dwDX*m_dwHorizonCount,dwDY);

					memcpy(pDst,pSrc,m_dwHorizonCount);
				}
		}
	}
*/
}

bool CSRFData::AllocSourceLowPoly( const uint32 dwTriangleCount )
{
	assert(dwTriangleCount);

	m_SourceLowPoly.clear();
	m_SurfaceTriangleId.FreeData();

	if(dwTriangleCount>=0xffff-1)							// m_SurfaceTriangleId is uint16
		return false;

	m_SourceLowPoly.resize(dwTriangleCount);

	const uint16 invalid=0xffff;

	m_SurfaceTriangleId.Alloc(m_dwWidth,m_dwHeight,&invalid);

	return true;
}


uint32 CSRFData::CalcAALevelSum() const
{
	assert(m_SurfaceAALevel.GetPointer());
	uint32 dwRet=0;

	for(uint32 dwY=0;dwY<m_dwHeight;++dwY)
	for(uint32 dwX=0;dwX<m_dwWidth;++dwX)
		dwRet += (uint32)m_SurfaceAALevel.GetRef(dwX,dwY);

	return dwRet;
}

uint32 CSRFData::CalcUsedAreaSize() const
{
	assert(m_SurfaceTriangleId.GetPointer());
	uint32 dwRet=0;

	const uint16 invalid=0xffff;

	for(uint32 dwY=0;dwY<m_dwHeight;++dwY)
	for(uint32 dwX=0;dwX<m_dwWidth;++dwX)
	{
		uint16 wIndex = m_SurfaceTriangleId.GetRef(dwX,dwY);

		if(wIndex!=invalid)
			++dwRet;
	}
	

	return dwRet;
}


void CSRFData::SetLowPolyVertex( const uint32 dwTriangle, const uint32 dwIndex, const Vec3 &vPos,
	const Vec3 &vTanU, const Vec3 &vTanV, const Vec3 &vTanN, const float fU, const float fV )
{
	assert(dwTriangle<m_SourceLowPoly.size());			// forgot to call AllocSourceLowPoly() or index out of range
	assert(dwIndex<3);

	SSourceTriangle &rTri = m_SourceLowPoly[dwTriangle];
	SSourceVertex &rVert = rTri.m_Vertex[dwIndex];

	rVert.m_Pos=vPos;
	rVert.m_TangentU=SNormal16(vTanU);
	rVert.m_TangentV=SNormal16(vTanV);
	rVert.m_TangentN=SNormal16(vTanN);
	rVert.m_fU=fU;
	rVert.m_fV=fV;
}

bool CSRFData::CalcLowPolyUVOrientation( const uint32 dwX, const uint32 dwY ) const
{
	uint16 wTriangle = GetLowPolyTriangleId(dwX,dwY);

	if(wTriangle==0xffff)			// invalid
		return false;

	const SSourceTriangle &rTri = m_SourceLowPoly[wTriangle];

	float fU=GetU(dwX);
	float fV=GetV(dwY);

	float fWholeTriArea = CalcAreaMul2( rTri.m_Vertex[0].m_fU,rTri.m_Vertex[0].m_fV,
																			rTri.m_Vertex[1].m_fU,rTri.m_Vertex[1].m_fV, 
																			rTri.m_Vertex[2].m_fU,rTri.m_Vertex[2].m_fV );

	return fWholeTriArea>0.0f;
}


void CSRFData::SetLowPolyTriangle( const uint32 dwTriangle, const uint32 dwMaterialId  )
{
	assert(dwTriangle<m_SourceLowPoly.size());

	SSourceTriangle &rTri = m_SourceLowPoly[dwTriangle];

	rTri.m_dwMaterialId=dwMaterialId;
}


float CSRFData::GetU( const uint32 dwX ) const
{
	return dwX/(float)m_dwWidth;
}

float CSRFData::GetV( const uint32 dwY ) const
{
	return dwY/(float)m_dwHeight;
}


float CSRFData::CalcHorizon( const uint32 dwX, const uint32 dwY, const float fAngle ) const
{
	// todo - put in real implementation

	uint8 *pHorizon = m_SurfaceHorizon.GetPointer(dwX*m_dwHorizonCount,dwY);

	return pHorizon[0]*(1.0f/255.0f);
}



void CSRFData::CalcUnocccludedAreaDirection( const uint32 dwX, const uint32 dwY, Vec3 &vDirectionOut, const float fBrighter ) const
{
	assert(m_SurfaceHorizon.GetPointer());
	assert(m_dwHorizonCount);

	if(GetLowPolyTriangleId(dwX,dwY)==0xffff)
	{
		vDirectionOut=Vec3(0,0,0);		// there is no surface info
		return;
	}

	float fSkyArea=0.0f;

	uint8 *pHorizon = m_SurfaceHorizon.GetPointer(dwX*m_dwHorizonCount,dwY);

	Vec3 vHighPolyBaseA, vHighPolyBaseB, vHighPolyBaseN;

	CalcHighPolyBase(dwX,dwY,vHighPolyBaseA,vHighPolyBaseB,vHighPolyBaseN);

	vDirectionOut=Vec3(0,0,0);

	for(uint32 dwI=0;dwI<m_dwHorizonCount;++dwI)
	{
		float fAngle=dwI*((gf_PI*2.0f)/m_dwHorizonCount);
		float s=sinf(fAngle), c=cosf(fAngle);
		float tiltanglepart = pHorizon[dwI]*(1.0f/255.0f);

//		const uint32 iQuality=7;			// should be quite enough

		// calculate the average normal - simple algorithm but this way it was easy to implement
		float fWedgeArea=(1.0f/(float)m_dwHorizonCount)*(tiltanglepart*tiltanglepart);
//		float tiltdeltaanglepart=0.5f/(float)(1<<iQuality);

		fSkyArea+=fWedgeArea;

		// /\
		// --
		// centroid y of a isosceles triangle is at 2/3 of it's height
		float fAverageAngle = tiltanglepart*(gf_PI*0.5f) * 1.0f/3.0f;
		float tilt=sinf(fAverageAngle);
		float rndx,rndy,rndz;

		rndx=s*tilt; rndy=c*tilt; rndz=(float)sqrt(1.0f-tilt);

		Vec3 vDir = vHighPolyBaseA*rndx + vHighPolyBaseB*rndy + vHighPolyBaseN*rndz;

		vDirectionOut += vDir*fWedgeArea;

		/*
		for(float fPart=0;fPart<tiltanglepart;fPart+=tiltdeltaanglepart)
		{
			assert(fPart<=gf_PI_DIV_2+0.001f);		// fPart 0..gf_PI_DIV_2
			float tilt=sinf(fPart*gf_PI_DIV_2);
			float rndx,rndy,rndz;

			rndx=s*tilt; rndy=c*tilt; rndz=(float)sqrt(1.0f-tilt);

			Vec3 vDir;

			vDir = vHighPolyBaseA*rndx + vHighPolyBaseB*rndy + vHighPolyBaseN*rndz;

			vDirectionOut += vDir*fWedgeArea;		// stronger average normal (more realistic look)
		}
		*/
	}

	// correct the average normal (no surface hit -> unit length normal)
	vDirectionOut.NormalizeSafe();

	float fMul = max(vHighPolyBaseN*vDirectionOut,0.0f)				// the more the normal is rotated - the more is the length reduced - less specular
							* (1.0f-(1.0f-fSkyArea)*(1.0f-fSkyArea));		// better looking - not physical correct 

	vDirectionOut*=fMul;


	// make the vector less small =>brighter result (inter reflections might brighten up the object)

	float fAddLength= (1-fMul)*fBrighter;	

	assert(fAddLength>=0.0f && fAddLength<=1.0f);

	vDirectionOut += fAddLength*vHighPolyBaseN;		// result can not be longer than 1
}

void CSRFData::CalcHighPolyBase( const uint32 dwX, const uint32 dwY, Vec3 &outBaseA, Vec3 &outBaseB, Vec3 &outBaseN ) const
{
	assert(m_dwHorizonCount);

	outBaseN = GetHighPolyNormal(dwX,dwY);

	Vec3 vBaseA,vBaseB;

	Matrix33 m33 = Matrix33::CreateOrthogonalBase(outBaseN);
	vBaseA = m33.GetColumn(1);
	vBaseB = m33.GetColumn(2);

	float fRandomRotation = CalcPseudoRandom(dwX,dwY) * gf_PI / (float)m_dwHorizonCount;

	float s=sinf(fRandomRotation),c=cos(fRandomRotation);

	outBaseA = vBaseA*c-vBaseB*s;
	outBaseB = vBaseB*c+vBaseA*s;
}


Vec3 CSRFData::CalcTangentHighPolyNormal( const uint32 dwX, const uint32 dwY, const bool bPerVertexOrthogonalizedTS ) const
{
	assert(m_bNormalsAreInWS);

	Vec3 vRetWorld = GetHighPolyNormal(dwX,dwY);

	return TransformDirectionIntoTangentSpace(dwX,dwY,vRetWorld,bPerVertexOrthogonalizedTS);
}


float CSRFData::CalcAccessibilityValue( const uint32 dwX, const uint32 dwY ) const
{
	assert(m_SurfaceHorizon.GetPointer());
	assert(m_dwHorizonCount);

	float fSkyArea=0.0f;

	assert(dwX<m_dwWidth);
	assert(dwY<m_dwHeight);

	uint8 *pHorizon = m_SurfaceHorizon.GetPointer(dwX*m_dwHorizonCount,dwY);

	for(uint32 dwI=0;dwI<m_dwHorizonCount;++dwI)
	{
		float tiltanglepart = pHorizon[dwI]*(1.0f/255.0f);

		float fWedgeArea=(1.0f/(float)m_dwHorizonCount)*(tiltanglepart*tiltanglepart);

		fSkyArea+=fWedgeArea;
	}

	return fSkyArea;
}

uint32 CSRFData::GetWidth() const
{
	return m_dwWidth;
}

uint32 CSRFData::GetHeight() const
{
	return m_dwHeight;
}

uint32 CSRFData::GetHorizonCount() const
{
	return m_dwHorizonCount;
}


Vec3 CSRFData::TransformDirectionIntoTangentSpace( const uint32 dwX, const uint32 dwY, const Vec3 vWorldDir, const bool bPerVertexOrthogonalizedTS ) const
{
	Vec3 vTangentU, vTangentV, vTangentN;

	if(CalcLowTangentBase(dwX,dwY,vTangentU,vTangentV,vTangentN,bPerVertexOrthogonalizedTS))
	{
		// build transform matrix
		Matrix33 mTangentSpace;

		mTangentSpace.SetFromVectors(vTangentV,-vTangentU,vTangentN);	// blue(bright means to viewer), green(bright means down,dark means up), red(bright means right, dark means left)

		Matrix33 mInvTangentSpace=mTangentSpace;
		mInvTangentSpace.Invert();

		// transform vector
		float fLength=vWorldDir.GetLength();		if(fLength>1.0f)fLength=1.0f;

		Vec3 vTangDirOut = mInvTangentSpace*vWorldDir;
		vTangDirOut.NormalizeSafe();
		vTangDirOut*=fLength;

		return vTangDirOut;
	}

	return vWorldDir;
}



// based on "Curved PN Triangles paper"
// return the Trueform/PN Triangle generated position (used for displacement mapping)
bool CSRFData::CalcLowPNTrianglePosNormal( const uint32 dwX, const uint32 dwY, const bool bQuadratic, Vec3 &outPos, Vec3 &outNormal ) const
{
	assert(m_SurfaceTriangleId.GetPointer());

	Vec3 _vBary;		// barycentric coordiantes

	uint16 wTriangle = CalcLowTriangle(dwX,dwY,_vBary);

	if(wTriangle==0xffff)			// invalid
		return false;

	Vec3 vBary = Vec3(_vBary[1],_vBary[2],_vBary[0]);		// barycentric coordiantes lie this code needs it

	const SSourceTriangle &rTri = m_SourceLowPoly[wTriangle];

	Vec3 b300=rTri.m_Vertex[0].m_Pos,
			 b030=rTri.m_Vertex[1].m_Pos,
			 b003=rTri.m_Vertex[2].m_Pos;
	Vec3 n200=rTri.m_Vertex[0].m_TangentN.GetDecompressed(),
			 n020=rTri.m_Vertex[1].m_TangentN.GetDecompressed(),
			 n002=rTri.m_Vertex[2].m_TangentN.GetDecompressed();

	Vec3 p12=b030-b300;
	Vec3 p23=b003-b030;
	Vec3 p31=b300-b003;

	float v12= 2*(p12*(n200+n020)) / (p12*p12);
	float v23= 2*(p23*(n002+n020)) / (p23*p23);
	float v31= 2*(p31*(n200+n002)) / (p31*p31);

	Vec3 n110=n200+n020-v12*(b030-b300);
	Vec3 n011=n020+n002-v23*(b003-b030);
	Vec3 n101=n002+n200-v31*(b300-b003);

	n110.Normalize();
	n011.Normalize();
	n101.Normalize();

	float w12=(b030-b300)*n200;
	float w21=(b300-b030)*n020;
	float w23=(b003-b030)*n020;
	float w32=(b030-b003)*n002;
	float w31=(b300-b003)*n002;
	float w13=(b003-b300)*n200;

	Vec3 b210=(2*b300+b030-w12*n200)/3.0f;
	Vec3 b120=(2*b030+b300-w21*n020)/3.0f;
	Vec3 b021=(2*b030+b003-w23*n020)/3.0f;
	Vec3 b012=(2*b003+b030-w32*n002)/3.0f;
	Vec3 b102=(2*b003+b300-w31*n002)/3.0f;
	Vec3 b201=(2*b300+b003-w13*n200)/3.0f;

	Vec3 E=(b210+b120+b021+b012+b102+b210)/6;
	Vec3 V=(b300+b030+b003)/3;
	Vec3 b111=E+(E-V)/2;

	outPos = b300*(vBary[2]*vBary[2]*vBary[2]) + b030*(vBary[0]*vBary[0]*vBary[0]) + b003*(vBary[1]*vBary[1]*vBary[1])
				+ 3*b210*(vBary[2]*vBary[2]*vBary[0]) + 3*b120*(vBary[0]*vBary[0]*vBary[2]) + 3*b201*(vBary[2]*vBary[2]*vBary[1])
				+ 3*b021*(vBary[0]*vBary[0]*vBary[1]) + 3*b102*(vBary[1]*vBary[1]*vBary[2]) + 3*b012*(vBary[1]*vBary[1]*vBary[0])
				+ 6*b111*(vBary[0]*vBary[1]*vBary[2]);

	if(bQuadratic)
	{
		outNormal = n200*(vBary[2]*vBary[2]) + n020*(vBary[0]*vBary[0]) + n002*(vBary[1]*vBary[1])
		+ n110*(vBary[2]*vBary[0]) + n011*(vBary[0]*vBary[1]) + n101*(vBary[2]*vBary[1]);
	}
	else
	{
		outNormal = n200*_vBary[0] + n020*_vBary[1] + n002*_vBary[2];
	}

	return true;
}


void CSRFData::SetPropertiesString( const char *szProp )
{
	m_sProperties=szProp;
}

const char *CSRFData::GetPropertiesString() const
{
	return m_sProperties.c_str();
}



bool _WriteWithChunkId( gzFile hnd, uint32 dwChunkId, const std::vector<CSRFData::SSourceTriangle> &rTriangles )
{
	if(rTriangles.empty())
		return true;

	// chunkid
	if(!gzwrite(hnd,&dwChunkId,sizeof(uint32)))
		return false;		// error

	// size
	uint32 dwUncompressedSize = ((uint32)rTriangles.size())*sizeof(CSRFData::SSourceTriangle)+sizeof(uint32);		// bitmap+sizeof(dwTriangleCount)
	if(!gzwrite(hnd,&dwUncompressedSize,sizeof(uint32)))
		return false;		// error

	// triangle Count
	uint32 dwTriangleCount = (uint32)rTriangles.size();
	if(!gzwrite(hnd,&dwTriangleCount,sizeof(uint32)))
		return false;		// error

	// data
	std::vector<CSRFData::SSourceTriangle>::const_iterator it;

	for(it=rTriangles.begin();it!=rTriangles.end();++it)
	{
		const CSRFData::SSourceTriangle &rTri = *it;

		if(!gzwrite(hnd,(voidp)&rTri,sizeof(CSRFData::SSourceTriangle)))
			return false;		// error
	}


	return true;		// no error
}



bool _ReadWithoutChunkId( gzFile hnd, std::vector<CSRFData::SSourceTriangle> &rTriangles )
{
	// size
	uint32 dwUncompressedSize;
	if(!gzread(hnd,&dwUncompressedSize,sizeof(uint32)))
		return false;		// error

	// size
	uint32 dwTriangleCount;
	if(!gzread(hnd,&dwTriangleCount,sizeof(uint32)))
		return false;		// error

	assert(dwTriangleCount);	// chunk should not be present in that case

	rTriangles.clear();
	rTriangles.resize(dwTriangleCount);

	// data
	if(!gzread(hnd,&rTriangles[0],dwTriangleCount*sizeof(CSRFData::SSourceTriangle)))
		return false;		// error

	return false;
}


template <class T>
bool _WriteWithChunkId( gzFile hnd, uint32 dwChunkId, CSimpleBitmap<T> &rBitmap )
{
	void *pPtr=rBitmap.GetPointer();

	if(!pPtr)
		return true;		// no error

	// chunkid
	if(!gzwrite(hnd,&dwChunkId,sizeof(uint32)))
		return false;		// error

	// size
	uint32 dwUncompressedSize=rBitmap.CalcBitmapSize()+4+4;		// bitmap+sizeof(width)+sizeof(height)
	if(!gzwrite(hnd,&dwUncompressedSize,sizeof(uint32)))
		return false;		// error

	// width
	uint32 dwWidth=rBitmap.GetWidth();
	if(!gzwrite(hnd,&dwWidth,sizeof(uint32)))
		return false;		// error

	// height
	uint32 dwHeight=rBitmap.GetHeight();
	if(!gzwrite(hnd,&dwHeight,sizeof(uint32)))
		return false;		// error

	// data
	if(!gzwrite(hnd,rBitmap.GetPointer(),rBitmap.CalcBitmapSize()))
		return false;		// error

	return true;		// no error
}

// string
bool _WriteWithChunkId( gzFile hnd, uint32 dwChunkId, string &rString )
{
	if(rString.empty())
		return true;		// no error

	// chunkid
	if(!gzwrite(hnd,&dwChunkId,sizeof(uint32)))
		return false;		// error

	// size
	uint32 dwUncompressedSize=(uint32)rString.length()+1;		// string + 0 termination
	if(!gzwrite(hnd,&dwUncompressedSize,sizeof(uint32)))
		return false;		// error

	// data
	// Version 1 was if(!gzwrite(hnd,(const voidp)rString.c_str(),dwUncompressedSize-1))
	if(!gzwrite(hnd,(const voidp)rString.c_str(),dwUncompressedSize))
		return false;		// error

	return true;		// no error
}

// string
bool _ReadWithoutChunkId( gzFile hnd, const uint32 dwVersion, string &rString )
{
	// size
	uint32 dwUncompressedSize;
	if(!gzread(hnd,&dwUncompressedSize,sizeof(uint32)))
		return false;		// error

	char *szTxt = new char[dwUncompressedSize];

	uint32 dwSizeToLoad=dwUncompressedSize;

	// for backward compability
	if(dwVersion<=1)
		--dwSizeToLoad;

	if(!gzread(hnd,szTxt,dwSizeToLoad))
	{
		delete szTxt;
		rString="";
		return false;		// error
	}

	// zero termination
	szTxt[dwUncompressedSize-1]=0;

	rString=szTxt;

	delete szTxt;
	return true;
}




// string,string
bool _WriteWithChunkId( gzFile hnd, uint32 dwChunkId, const string &rStringKey, const string &rStringValue )
{
	// chunkid
	if(!gzwrite(hnd,&dwChunkId,sizeof(uint32)))
		return false;		// error

	// size
	uint32 dwUncompressedSize=(uint32)rStringKey.length()+(uint32)rStringValue.length()+2;		// string + 0 termination + string + 0 termination

	if(!gzwrite(hnd,&dwUncompressedSize,sizeof(uint32)))
		return false;		// error

	// data
	if(!gzwrite(hnd,(const voidp)rStringKey.c_str(),(uint32)rStringKey.length()+1))
		return false;		// error

	if(!gzwrite(hnd,(const voidp)rStringValue.c_str(),(uint32)rStringValue.length()+1))
		return false;		// error

	return true;		// no error
}



// string,string
bool _ReadWithoutChunkId( gzFile hnd, string &rStringKey, string &rStringValue )
{
	// size
	uint32 dwUncompressedSize;
	if(!gzread(hnd,&dwUncompressedSize,sizeof(uint32)))
		return false;		// error

	char *szTxt = new char[dwUncompressedSize];

	uint32 dwSizeToLoad=dwUncompressedSize;

	if(!gzread(hnd,szTxt,dwSizeToLoad))
	{
		delete szTxt;
		rStringKey="";rStringValue="";
		return false;		// error
	}

	rStringKey=szTxt;
	rStringValue=&szTxt[strlen(szTxt)+1];
	
	delete szTxt;
	return true;
}




template <class T>
bool _ReadWithoutChunkId( gzFile hnd, CSimpleBitmap<T> &rBitmap )
{
	// size
	uint32 dwUncompressedSize;
	if(!gzread(hnd,&dwUncompressedSize,sizeof(uint32)))
		return false;		// error

	// width
	uint32 dwWidth;
	if(!gzread(hnd,&dwWidth,sizeof(uint32)))
		return false;		// error

	// height
	uint32 dwHeight;
	if(!gzread(hnd,&dwHeight,sizeof(uint32)))
		return false;		// error

	if(!rBitmap.Alloc(dwWidth,dwHeight))
		return false;

	if(dwUncompressedSize!=rBitmap.CalcBitmapSize()+4+4)		// bitmap+sizeof(width)+sizeof(height)
		return false;		// error	

	// data
	if(!gzread(hnd,rBitmap.GetPointer(),rBitmap.CalcBitmapSize()))
		return false;		// error

	return true;
}

bool CSRFData::Load( const char *szFilename )
{
	gzFile hnd = gzopen(szFilename,"rb");

	if(!hnd)
		return false;

	// header
	uint32 dwFileID;
	if(!gzread(hnd,&dwFileID,sizeof(uint32)))
		return false;		// error

	if(dwFileID!='srf_')
		return false;

	uint32 dwMagicWord;
	if(!gzread(hnd,&dwMagicWord,sizeof(uint32)))
		return false;		// error

	if(dwMagicWord!=0x02652343)
		return false;

	if(!gzread(hnd,&m_dwLoadedFileVersion,sizeof(uint32)))
		return false;		// error

	if(m_dwLoadedFileVersion!=0x1 && m_dwLoadedFileVersion!=0x2)
		return false;

	// header
	{
		if(!gzread(hnd,&m_dwWidth,sizeof(uint32)))
			return false;		// error
		if(!gzread(hnd,&m_dwHeight,sizeof(uint32)))
			return false;		// error
		if(!gzread(hnd,&m_dwHorizonCount,sizeof(uint32)))
			return false;		// error

		uint32 dwReserved;

		if(!gzread(hnd,&dwReserved,sizeof(uint32)))
			return false;		// error
		if(!gzread(hnd,&dwReserved,sizeof(uint32)))
			return false;		// error
		if(!gzread(hnd,&dwReserved,sizeof(uint32)))
			return false;		// error
	}

	for(;;)
	{
		// chunk id
		uint32 dwChunkId='srf_';
		if(!gzread(hnd,&dwChunkId,sizeof(uint32)))
			return false;		// error

/*	// debugging
		char str[80];

		sprintf(str,"\nLast dwChunkId: %c%c%c%c\n",((char *)&dwChunkId)[3],((char *)&dwChunkId)[2],((char *)&dwChunkId)[1],((char *)&dwChunkId)[0]);
		OutputDebugString(str);
*/

				 if(dwChunkId=='wnrm')_ReadWithoutChunkId(hnd,m_SurfaceWorldNormals);
		else if(dwChunkId=='diff')_ReadWithoutChunkId(hnd,m_SurfaceDiffuse);
		else if(dwChunkId=='spec')_ReadWithoutChunkId(hnd,m_SurfaceSpecular);
		else if(dwChunkId=='glos')_ReadWithoutChunkId(hnd,m_SurfaceGloss);
		else if(dwChunkId=='trns')_ReadWithoutChunkId(hnd,m_SurfaceTransparency);
		else if(dwChunkId=='hpos')_ReadWithoutChunkId(hnd,m_SurfaceHighPos);
		else if(dwChunkId=='trin')_ReadWithoutChunkId(hnd,m_SurfaceTriangleId);
		else if(dwChunkId=='horz')_ReadWithoutChunkId(hnd,m_SurfaceHorizon);
		else if(dwChunkId=='mesh')_ReadWithoutChunkId(hnd,m_SourceLowPoly);
		else if(dwChunkId=='hmat')_ReadWithoutChunkId(hnd,m_SurfaceHighMatId);
		else if(dwChunkId=='prop')_ReadWithoutChunkId(hnd,m_dwLoadedFileVersion,m_sProperties);
		else if(dwChunkId=='keyv')
		{
			string sKey, sValue;

			if(_ReadWithoutChunkId(hnd,sKey,sValue))
				m_KeyValuePairs[sKey]=sValue;
		}
		else if(dwChunkId=='end_')break;			// last chunk
		else
		{
			// unknown chunk id
			assert(0);						// if you load a newer file with old code in DEBUG - or internal error

			// size
			uint32 dwUncompressedSize;
			if(!gzread(hnd,&dwUncompressedSize,sizeof(uint32)))
				return false;		// error
			
			// jump over it
			gzseek(hnd,dwUncompressedSize,SEEK_CUR);
		}
	}

	gzclose(hnd);

	return true;
}


bool CSRFData::Save( const char *szFilename )
{
	gzFile hnd = gzopen(szFilename,"wb");

	if(!hnd)
		return false;

	// header
	uint32 dwFileID='srf_';
	if(!gzwrite(hnd,&dwFileID,sizeof(uint32)))
		return false;		// error
	
	uint32 dwMagicWord=0x02652343;
	if(!gzwrite(hnd,&dwMagicWord,sizeof(uint32)))
		return false;		// error

	uint32 dwVersion=m_dwActualFileVersion;
	if(!gzwrite(hnd,&dwVersion,sizeof(uint32)))
		return false;		// error

	// header
	{
		if(!gzwrite(hnd,&m_dwWidth,sizeof(uint32)))
			return false;		// error
		if(!gzwrite(hnd,&m_dwHeight,sizeof(uint32)))
			return false;		// error
		if(!gzwrite(hnd,&m_dwHorizonCount,sizeof(uint32)))
			return false;		// error

		uint32 dwReserved=0;

		if(!gzwrite(hnd,&dwReserved,sizeof(uint32)))
			return false;		// error
		if(!gzwrite(hnd,&dwReserved,sizeof(uint32)))
			return false;		// error
		if(!gzwrite(hnd,&dwReserved,sizeof(uint32)))
			return false;		// error
	}


	// data
	_WriteWithChunkId(hnd,'prop',m_sProperties);
	_WriteWithChunkId(hnd,'wnrm',m_SurfaceWorldNormals);
	_WriteWithChunkId(hnd,'diff',m_SurfaceDiffuse);
	_WriteWithChunkId(hnd,'spec',m_SurfaceSpecular);
	_WriteWithChunkId(hnd,'glos',m_SurfaceGloss);
	_WriteWithChunkId(hnd,'trns',m_SurfaceTransparency);
	_WriteWithChunkId(hnd,'hpos',m_SurfaceHighPos);
	_WriteWithChunkId(hnd,'trin',m_SurfaceTriangleId);
	_WriteWithChunkId(hnd,'horz',m_SurfaceHorizon);
	_WriteWithChunkId(hnd,'mesh',m_SourceLowPoly);
	_WriteWithChunkId(hnd,'hmat',m_SurfaceHighMatId);

	{
		std::map<string,string>::const_iterator it,end=m_KeyValuePairs.end();

		for(it=m_KeyValuePairs.begin();it!=end;++it)
			_WriteWithChunkId(hnd,'keyv',it->first,it->second);
	}

	// end chunk
	uint32 dwEndChunk='end_';
	if(!gzwrite(hnd,&dwEndChunk,sizeof(uint32)))
		return false;		// error

	gzclose(hnd);

	return true;
}


char *CSRFData::GetInfoString( const uint32 _dwLine ) const
{
	uint32 dwLine=_dwLine;
	static char szBuffer[4096];
/*
	if(!m_KeyValuePairs.empty())
	{
		if(dwLine==0)		// seperator line
		{
			strcpy(szBuffer,"KeyValue Pairs:");
			return szBuffer;
		}
		else --dwLine;

		std::map<string,string>::const_iterator it=m_KeyValuePairs.begin(), end=m_KeyValuePairs.end();

		for(;it!=end;--dwLine,++it)
		{
			if(dwLine==0)
			{
				sprintf(szBuffer,"  '%s'='%s'",it->first.c_str(),it->second.c_str());
				return szBuffer;
			}
		}

		if(dwLine==0)		// separator line
		{
			strcpy(szBuffer,"");
			return szBuffer;
		}
		else --dwLine;
	}
*/

	switch(dwLine)
	{
		case  0:	sprintf(szBuffer,"Extent: %dx%d Horizons: %d",m_dwWidth,m_dwHeight,m_dwHorizonCount);break;
		case  1:	sprintf(szBuffer,"Normals: %d Bytes uncompressed",m_SurfaceWorldNormals.CalcBitmapSize());break;
		case  2:	sprintf(szBuffer,"Diffuse: %d Bytes uncompressed",m_SurfaceDiffuse.CalcBitmapSize());break;
		case  3:	sprintf(szBuffer,"Specular: %d Bytes uncompressed",m_SurfaceSpecular.CalcBitmapSize());break;
		case  4:	sprintf(szBuffer,"Gloss: %d Bytes uncompressed",m_SurfaceGloss.CalcBitmapSize());break;
		case  5:	sprintf(szBuffer,"Transparency: %d Bytes uncompressed",m_SurfaceTransparency.CalcBitmapSize());break;
		case  6:	sprintf(szBuffer,"HighPos: %d Bytes uncompressed",m_SurfaceHighPos.CalcBitmapSize());break;
		case  7:	sprintf(szBuffer,"TriangleId: %d Bytes uncompressed",m_SurfaceTriangleId.CalcBitmapSize());break;
		case  8:	sprintf(szBuffer,"Horizon: %d Bytes uncompressed",m_SurfaceHorizon.CalcBitmapSize());break;
		case  9:	sprintf(szBuffer,"HighPolyMatId: %d Bytes uncompressed",m_SurfaceHighMatId.CalcBitmapSize());break;
		case 10:	sprintf(szBuffer,"LowPoly: %d Bytes uncompressed",m_SourceLowPoly.size()*sizeof(SSourceTriangle));break;
		case 11:	
			{
				float fMinDisp,fMaxDisp;

				CalcDisplacementRange(fMinDisp,fMaxDisp,false,false);

				if(fMinDisp!=FLT_MAX && fMaxDisp!=-FLT_MAX)
					sprintf(szBuffer,"Displace (bNPatch=bQuadric=false): min=%.2f max=%.2f",fMinDisp,fMaxDisp);
				else
					sprintf(szBuffer,"Displace (bNPatch=bQuadric=false)");
			}
			break;
		case 12:	sprintf(szBuffer,"Version: File=%d Loader=%d",m_dwLoadedFileVersion,m_dwActualFileVersion);break;
		case 13:	sprintf(szBuffer,"Properties: '%s'",GetPropertiesString());break;
		default:
			return 0;
			break;
	}

	return szBuffer;
}


// produced belivable random numbers for a give x and y coordinate
// tested with external test app
// Return:
//   0..1 (8bit quality)
float CSRFData::CalcPseudoRandom( const uint32 x, const uint32 y )
{
	uint32 a = x&0xff;
	uint32 b = y&0xff;

	uint32 ret = ((a+71)*(b+43));

	ret *= ((a+13)*(b+91));

	return (((ret>>16)^ret) & 0xff)*(1.0f/255.0f);
}



bool CSRFData::GetValue( const char *szKey, char *szValue, const uint32 dwMaxSize ) const
{
	assert(szKey);
	assert(*szKey!=0);

	std::map<string,string>::const_iterator it = m_KeyValuePairs.find(szKey);

	if(it!=m_KeyValuePairs.end())
	if(it->second.size()<dwMaxSize-1)
	{
		strcpy(szValue,it->second.c_str());
   	return true;
	}

	return false;
}


bool CSRFData::GetValue( const char *szKey, float &fValue ) const
{
	char str[256];

	if(!GetValue(szKey,str,256))
		return false;

	fValue = (float)atof(str);
	return true;
}


bool CSRFData::GetValue( const char *szKey, int &iValue ) const
{
	char str[256];

	if(!GetValue(szKey,str,256))
		return false;

	iValue = atoi(str);
	return true;
}

void CSRFData::SetValue( const char *szKey, const char *szValue )
{
	assert(szKey);
	assert(*szKey!=0);
	assert(szValue);

	if(*szValue==0)
		m_KeyValuePairs.erase(szKey);
	 else
		m_KeyValuePairs[szKey]=szValue;
}


void CSRFData::SetValue( const char *szKey, const int iValue )
{
	assert(szKey);
	assert(*szKey!=0);

	char str[256];

	sprintf(str,"%d",iValue);

	m_KeyValuePairs[szKey]=str;
}


void CSRFData::SetValue( const char *szKey, const float fValue )
{
	assert(szKey);
	assert(*szKey!=0);

	char str[256];

	sprintf(str,"%f",fValue);

	m_KeyValuePairs[szKey]=str;
}





static uint32 Vec3ToColor( const Vec3 &v )
{
	assert(v.x>=0.0f && v.x<=1.0f);
	assert(v.y>=0.0f && v.y<=1.0f);
	assert(v.z>=0.0f && v.z<=1.0f);

	uint32 r = (uint32)(v.x*255.9999f);
	uint32 g = (uint32)(v.y*255.9999f);
	uint32 b = (uint32)(v.z*255.9999f);

	return (r<<16) | (g<<8) | b;
}

static uint32 Norm3ToColor( const Vec3 &v )
{
	return Vec3ToColor(v*0.5f+Vec3(0.5f,0.5f,0.5f));	
}


bool CSRFData::RequestPreview( const uint32 dwWidth, const uint32 dwHeight, const uint32 dwStartY, const uint32 dwEndY, 
	uint32 *pARGB, const uint32 dwPreviewId ) const
{
	uint32 dwId=0xffffffff;

	const uint32 dwInvalidCol = 0x105050;
	const uint32 dwErrorCol = 0xff0000;
/*
	if(m_SurfaceTriangleId.GetPointer())
		++dwId;
	if(dwPreviewId==dwId)
	{
		for(uint32 dwDstY=0;dwDstY<dwHeight;++dwDstY)
		{
			uint32 dwSrcY = (dwDstY*m_dwHeight)/dwHeight;

			for(uint32 dwDstX=0;dwDstX<dwWidth;++dwDstX)
			{
				uint32 dwSrcX = (dwDstX*m_dwWidth)/dwWidth;

				Vec3 vBary; 
				uint16 wTriangle = CalcLowTriangle(dwSrcX,dwSrcY,vBary);

				if(wTriangle==0xffff)		// invalid
				{
					*pARGB++ = dwInvalidCol;
					continue;
				}

				const SSourceTriangle &rTri = m_SourceLowPoly[wTriangle];

				if(rTri.m_dwMaterialId!=0xffffffff)
				{
					const uint32 dwColArray[] = { 0x0,0xff,0xff00,0xffff,0xff0000,0xff00ff,0xffff00 };	// distinct colors without white

					uint32 dwCol = dwColArray[rTri.m_dwMaterialId % (sizeof(dwColArray)/sizeof(uint32))];

					*pARGB++ = dwCol;
				}
				else *pARGB++ = 0xffffff;		// white in case the material was undefined
			}
		}
		return true;
	}
*/

	if(m_SurfaceTriangleId.GetPointer())
		++dwId;
	if(dwPreviewId==dwId || dwPreviewId==0xffffffff)	
	{
		for(uint32 dwDstY=0;dwDstY<dwHeight;++dwDstY)
		{
			uint32 dwSrcY = (dwDstY*m_dwHeight)/dwHeight;

			for(uint32 dwDstX=0;dwDstX<dwWidth;++dwDstX)
			{
				uint32 dwSrcX = (dwDstX*m_dwWidth)/dwWidth;

				Vec3 vBary; 
				if(CalcLowTriangle(dwSrcX,dwSrcY,vBary)==0xffff)		// invalid
				{
					*pARGB++ = dwInvalidCol;
					continue;
				}

				Vec3 vNormal = CalcTangentHighPolyNormal(dwSrcX,dwSrcY,false);

				uint32 dwCol = Norm3ToColor(vNormal);

				*pARGB++ = dwCol;
			}
		}
		return true;
	}

	if(m_SurfaceHorizon.GetPointer())
		++dwId;
	if(dwPreviewId==dwId)
	{
		for(uint32 dwDstY=0;dwDstY<dwHeight;++dwDstY)
		{
			uint32 dwSrcY = (dwDstY*m_dwHeight)/dwHeight;

			for(uint32 dwDstX=0;dwDstX<dwWidth;++dwDstX)
			{
				uint32 dwSrcX = (dwDstX*m_dwWidth)/dwWidth;

				Vec3 vBary; 
				if(CalcLowTriangle(dwSrcX,dwSrcY,vBary)==0xffff)		// invalid
				{
					*pARGB++ = dwInvalidCol;
					continue;
				}

				float fAcc=CalcAccessibilityValue(dwSrcX,dwSrcY);

				uint32 dwCol = Vec3ToColor(Vec3(fAcc,fAcc,fAcc));

				*pARGB++ = dwCol;
			}
		}
		return true;
	}


	if(m_SurfaceHighPos.GetPointer())
		++dwId;
	if(dwPreviewId==dwId)
	{
		float fMinDisp,fMaxDisp;

		CalcDisplacementRange(fMinDisp,fMaxDisp,false,false);

		float fDispScale = 1.0f/(fMaxDisp-fMinDisp);

		for(uint32 dwDstY=0;dwDstY<dwHeight;++dwDstY)
		{
			uint32 dwSrcY = (dwDstY*m_dwHeight)/dwHeight;

			for(uint32 dwDstX=0;dwDstX<dwWidth;++dwDstX)
			{
				uint32 dwSrcX = (dwDstX*m_dwWidth)/dwWidth;

				Vec3 vBary; 
				if(CalcLowTriangle(dwSrcX,dwSrcY,vBary)==0xffff)		// invalid
				{
					*pARGB++ = dwInvalidCol;
					continue;
				}

				float fDisp=CalcDisplacementDist(dwSrcX,dwSrcY,false,false);

				if(fDisp==FLT_MAX)
				{
					*pARGB++ = dwErrorCol;
					continue;
				}

				float fNormDisp = (fDisp-fMinDisp)*fDispScale;

				assert(fNormDisp>=0);
				assert(fNormDisp<=1.0f);

				uint32 dwCol = Vec3ToColor(Vec3(0,fNormDisp,0));

				*pARGB++ = dwCol;
			}
		}
		return true;
	}


	if(IsThereHighPolyMaterialId())
		++dwId;
	if(dwPreviewId==dwId)
	{
		static const Vec3 ColTable[]=
		{
			// R G B
			Vec3(0,0,0),
			Vec3(1,0,0),
			Vec3(0,1,0),
			Vec3(1,1,0),
			Vec3(0,0,1),
			Vec3(1,0,1),
			Vec3(0,1,1),
			Vec3(1,1,1)
		};

		for(uint32 dwDstY=0;dwDstY<dwHeight;++dwDstY)
		{
			uint32 dwSrcY = (dwDstY*m_dwHeight)/dwHeight;

			for(uint32 dwDstX=0;dwDstX<dwWidth;++dwDstX)
			{
				uint32 dwSrcX = (dwDstX*m_dwWidth)/dwWidth;

				uint8 matid = GetHighPolyMaterialId(dwSrcX,dwSrcY);

				if(matid==0)
					*pARGB++ = Vec3ToColor(Vec3(0.5f,0.5f,0.5f));
				 else
					*pARGB++ = Vec3ToColor(ColTable[(matid-1)%((sizeof(ColTable)/sizeof(Vec3)))]);
			}
		}
		return true;
	}

	return false;
}

