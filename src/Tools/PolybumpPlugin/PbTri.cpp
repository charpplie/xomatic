#include "stdafx.h"
#include <float.h>												// FLT_MAX
#include "PbTri.h"												// CPbTri
#include "PolyBump.h"											// CPbMesh
#include "intersect_triangle.h"						// intersect_triangle()
#include "Cry_Math.h"											// Vec3



float	CalcTriArea( const Vec3 &p0, const Vec3 &p1, const Vec3 &p2 )
{
	// http://www.flipcode.com/geometry/issue10.shtml
	// ABC = (1/2)*||(B–A)×(C–A)||

	return( ((p1-p0)^(p2-p0)).len() *0.5f );
}





// extend bounding box
void CPbBaseTri::ExtendMinMax( Vec3 &inoutMin, Vec3 &inoutMax ) const
{
	assert(this);

	for(int i=0;i<3;i++)
	{
		inoutMin.x=min(inoutMin.x,m_Verts[i].x);inoutMax.x=max(inoutMax.x,m_Verts[i].x);
		inoutMin.y=min(inoutMin.y,m_Verts[i].y);inoutMax.y=max(inoutMax.y,m_Verts[i].y);
		inoutMin.z=min(inoutMin.z,m_Verts[i].z);inoutMax.z=max(inoutMax.z,m_Verts[i].z);
	}
}


void CPbBaseTri::ExtendSphere( Vec3 invMid, float &outfRadius2 )
{
	assert(this);

	float len2;

	len2=(invMid-m_Verts[0]).len2();	if(outfRadius2<len2)outfRadius2=len2;
	len2=(invMid-m_Verts[1]).len2();	if(outfRadius2<len2)outfRadius2=len2;
	len2=(invMid-m_Verts[2]).len2();	if(outfRadius2<len2)outfRadius2=len2;
}









//get the interpolated normal using barycentric coords (for the high poly mesh)
//////////////////////////////////////////////////////////////////////
void CPbHighTri::CalcBarycentricCoordsFromPoint( const Vec3 &point, Vec3 &outvNormal, bool inbDoClipping )
{
  // first sub tri
  float a1=CalcTriArea(point,m_Verts[1],m_Verts[2]);
  // second sub tri  
  float a2=CalcTriArea(m_Verts[0],point,m_Verts[2]);

	float fArea3d=CalcTriArea(m_Verts[0],m_Verts[1],m_Verts[2]);				// area size in world space

	// to prevent crash
	if(fArea3d==0.0f)
	{ 
		// slow but should be rare
		outvNormal=ComputeTriangleNormal().GetNormalizedSafe();

		assert(outvNormal.len2()<=1.0f);
		return;
	}

  // calculate alpha beta and gamma components as area ratio
  float alpha=a1/fArea3d;
  float beta=a2/fArea3d;
	
	if(inbDoClipping)	// clip the barycentric coordinates inside the triangle
	{
		float sub=alpha+beta-1.0f;
		if(sub>0.0f)
		{
			sub*=0.5f;
			alpha-=sub;
			beta-=sub;
		}
		if(alpha<0.0f)alpha=0.0f;
		if(beta<0.0f)beta=0.0f;
	}

  // since the sum is always 1 'cause the point is inside the polygon,
  // gamma can be calculated in this way...
  float gamma=1.0f-alpha-beta;

  // interpolate normal vector
  outvNormal= alpha*m_VertsNormal[0].GetDecompressed() + beta*m_VertsNormal[1].GetDecompressed() + gamma*m_VertsNormal[2].GetDecompressed();

	outvNormal.Normalize();

	assert(outvNormal.len()<=1.0f);
}



// calculate the barycentric coords from texture coords (in the low poly model)
// normalize normal after calling this function
//////////////////////////////////////////////////////////////////////
void CPbLowTri::CalcBarycentricCoordsFromUV( float s, float t, Vec3 &point, Vec3 &normal,
	bool inbDoClipping ) const
{
	float fBar[3];		// barycentric coordinates

	CalcBarycentrics(s,t,fBar,inbDoClipping);

  //interpolate position and normal
  point=  fBar[0]*m_Verts[0]
				+ fBar[1]*m_Verts[1]
				+ fBar[2]*m_Verts[2];

  normal= fBar[0]*m_VertsNormal[0].GetDecompressed()
				+ fBar[1]*m_VertsNormal[1].GetDecompressed()
				+ fBar[2]*m_VertsNormal[2].GetDecompressed();

	// normal is not normalized because we need it's length for renormalizing in tangentspace
}


// helper method
void CPbLowTri::CalcBarycentrics( float inS, float inT, float outCoor[3], bool inbDoClipping ) const
{
	float ftemptriX[3],ftemptriY[3];  

  // calculate the area of first sub triangle
  memcpy(ftemptriX,m_fS,sizeof(float)*3);
  memcpy(ftemptriY,m_fT,sizeof(float)*3);

  ftemptriX[0]=inS;ftemptriY[0]=inT;
	float a1=CalcUVArea(ftemptriX,ftemptriY);

  // calculate the area of second sub triangle
  memcpy(ftemptriX,m_fS,sizeof(float)*3);
  memcpy(ftemptriY,m_fT,sizeof(float)*3);

  ftemptriX[1]=inS;ftemptriY[1]=inT;
  float a2=CalcUVArea(ftemptriX,ftemptriY);  

	// calculate alpha beta and gamma components as area ratio
  float alpha=a1/m_fUVArea;
  float beta=a2/m_fUVArea;
	
	if(inbDoClipping)
	{
		float sub=alpha+beta-1.0f;
		if(sub>0.0f)
		{
			sub*=0.5f;
			alpha-=sub;
			beta-=sub;
		}
		if(alpha<0.0f)alpha=0.0f;
		if(beta<0.0f)beta=0.0f;
	}

	outCoor[0]=alpha;
	outCoor[1]=beta;
  outCoor[2]=1.0f-alpha-beta;
}






float CPbBaseTri::CalcIntersectionFromDir( const Vec3 &invStart, const Vec3 &invDir ) 
{
	assert(!_isnan(invDir.x));
	assert(!_isnan(invDir.y));
	assert(!_isnan(invDir.z));

	double orig[3]={invStart.x,invStart.y,invStart.z};
	double dir[3]={ invDir.x,invDir.y,invDir.z };
	double vert1[3]={ m_Verts[0].x, m_Verts[0].y, m_Verts[0].z };
	double vert2[3]={ m_Verts[1].x, m_Verts[1].y, m_Verts[1].z };
	double vert3[3]={ m_Verts[2].x, m_Verts[2].y, m_Verts[2].z };
	double t,u,v;

	if(!intersect_triangle(orig,dir,vert1,vert2,vert3,&t,&u,&v))
		return FLT_MAX;

	return (float)t;
}










void CPbLowTri::RefreshInternals()
{
	m_fUVArea=CalcUVArea(m_fS,m_fT);														// area size in texture

	if(m_fUVArea==0.0f)
		m_fUVArea=0.001f;																					// to prevent crash

	assert(sizeof(SCompressedNormal)==sizeof(short)*3);
	assert(sizeof(m_VertsNormal)==3*sizeof(SCompressedNormal));
	assert(sizeof(CPbBaseTri)==sizeof(Vec3)*3+sizeof(m_VertsNormal)+2);
	assert(sizeof(CPbBaseTri)==sizeof(CPbHighTri));
}

Vec3 CPbBaseTri::ComputeTriangleNormal() const
{
	return (m_Verts[1]-m_Verts[0])%(m_Verts[2]-m_Verts[0]);	//vector cross-product	
}
