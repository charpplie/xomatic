#include "stdafx.h"
#include "PbTri.h"												// CPbHighTri

/*
// calculate mesh-ray intersection
//////////////////////////////////////////////////////////////////////
uint32 ChooseNearestAcceptableIntersection( CIntInfoHighList &inIntersections, const Vec3 &invStart, const Vec3 &invEnd, 
																											const Vec3 &refnormal, Vec3 &respoint )
{
  // find the nearest intersection point with a similar normal
//  CPbHighTri *besttri=NULL;
	uint32 besttri = 0xffffffff;
  float bestdist=3.402823466e+38F;											// maximum possible value

	float mid=(invStart-invEnd).len()*0.5f;

	CIntInfoHighIt i, end=inIntersections.end();

	// find best hit with a good normal
	for(i=inIntersections.begin();i!=end;++i)
  {
		CIntersInfo<CPbHighTri> &ci=(*i);

		float dist=(float)(fabs(ci.m_fDist-mid));

    if(dist<bestdist)
    {
			float factor=ci.m_pTri->ComputeTriangleNormal()*refnormal;

			if(factor>0)
			{
				besttri=ci.m_dwObjectIndex;
				bestdist=dist;
				respoint=ci.m_Point;
			}
    }
  } //i

  return besttri;
}
*/


	
/*
// calculate mesh-ray intersection
//////////////////////////////////////////////////////////////////////
uint32 ChooseNearestIntersection( CIntInfoHighList &inIntersections, const Vec3 &invStart, const Vec3 &invEnd, 
																						const Vec3 &refnormal, Vec3 &respoint )
{
  // find the nearest intersection point with a similar normal
//  CPbHighTri *besttri=NULL;
  uint32 besttri=0xffffffff;
  float bestdist=3.402823466e+38F;											// maximum possible value

	float mid2=(invStart-invEnd).len()*0.5f;

	CIntInfoHighIt i, end=inIntersections.end();

	// find best hit with a good normal
  for(i=inIntersections.begin();i!=end;++i)
  {
		CIntersInfo<CPbHighTri> &ci=(*i);

		float dist = fabs(ci.m_fDist-mid2);

    if(dist<bestdist)
    {
			float factor=ci.m_pTri->ComputeTriangleNormal()*refnormal;

			if(factor>0)
			{
				besttri=ci.m_dwObjectIndex;
				bestdist=dist;
				respoint=ci.m_Point;
			}
    }
  } //i

  return besttri;
}



//
// refnormal has to be normalized
uint32 ChooseLatestIntersection( CIntInfoHighList &inIntersections, const Vec3 &invStart, const Vec3 &invEnd, 
																					 const Vec3 &refnormal, Vec3 &respoint )
{
  //find the latest intersection point with a similar normal
  uint32 besttri=0xffffffff;
  float bestdist=-1.0f;        

	CIntInfoHighIt i, end=inIntersections.end();

	// find best hit with a good normal
	for(i=inIntersections.begin();i!=end;++i)
  {
		CIntersInfo<CPbHighTri> &ci=(*i);

		assert(ci.m_fDist>=0.0f);

		if(ci.m_fDist>bestdist)
    {
			float factor=ci.m_pTri->ComputeTriangleNormal()*refnormal;

			if(factor>0)
			{
				besttri=ci.m_dwObjectIndex;
				bestdist=ci.m_fDist;
				respoint=ci.m_Point;                    
			}
    }
  } //i

  return besttri;
}
*/
