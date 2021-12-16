

//include headers
/////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "CPlane.h"

//////////////////////////////////////////////////////////////////////////
bool CPlanef::RayIntersect(const vector3f &vRayOrigin, const vector3f &vRayDir,ftype &fTime) const
{
	ftype fDenom=m_vNormal * vRayDir;
	if (fDenom==0)
		return (false);

	ftype	fNumer=m_vNormal*vRayOrigin;

	fTime=-((fNumer - m_fDist) / fDenom); 
	//fTime=-((fNumer + m_fDist) / fDenom); 

	return (true);
}

//////////////////////////////////////////////////////////////////////////
bool CPlanef::LineStabsPlane(const vector3f &v1, const vector3f &v2,vector3f &vRes) const
{
	ftype fDist1=DistFromPlane(v1);
	ftype fDist2=DistFromPlane(v2);

	if (fDist1*fDist2>=0)
		return (false); // doesn't cross

	ftype fDot=fDist1/(fDist1-fDist2);			

	//find intersection
	for (int j=0;j<3;j++)
	{	
		// avoid round off error 
		if (m_vNormal[j]==1.0)
			vRes[j]=m_fDist;
		else 
		if (m_vNormal[j]==-1.0)
			vRes[j]=-m_fDist;
		else
			vRes[j]=v1[j]+fDot*(v2[j]-v1[j]);
	} //j
	
	//vRes=v1+fDot*(v2-v1); 				
	return (true);
}

//////////////////////////////////////////////////////////////////////////
//! set normal and dist for this plane and  then calculate plane type
void	CPlanef::Set(const vector3f &vNormal,ftype fDist)
{	
	m_vNormal = vNormal; 
	m_fDist = fDist;	
}

//! calc the plane giving 3 points
//! returns false for degenerated triangles
//////////////////////////////////////////////////////////////////////////
bool	CPlanef::CalcPlane(const vector3f &p0,const vector3f &p1,const vector3f &p2)
{
	m_vNormal = (p0-p1)^(p2-p1);
	ftype fLength = m_vNormal.Length();
	if (fLength<0.001f)
	{
		m_vNormal.Clear();
		return (false);
	}

	ftype fInv=1.0/fLength; 
	m_vNormal.x*=fInv; m_vNormal.y*=fInv; m_vNormal.z*=fInv;	  
	// NOTE: I calculate the d component of the plane equation as -d
	m_fDist = (p0*m_vNormal);	
	return (true);
}

//! tell which side of the plane the box is
//////////////////////////////////////////////////////////////////////////
int CPlanef::CalcBoxSide(const vector3f &vMins, const vector3f &vMaxs) const
{
	vector3f Corners[2];

	for (int i=0;i<3;i++)
	{
		if (m_vNormal[i] < 0)
		{
			Corners[0][i] = vMins[i];
			Corners[1][i] = vMaxs[i];
		}
		else
		{
			Corners[1][i] = vMins[i];
			Corners[0][i] = vMaxs[i];
		}
	} //i

	ftype fDist1 = (m_vNormal*Corners[0]) - m_fDist;
	ftype fDist2 = (m_vNormal*Corners[1]) - m_fDist;

	bool bFront=(fDist1>=0);

	if (fDist2<0)
	{
	  if (bFront)
	    return (SIDE_CROSS);

	  return (SIDE_BACK);
	}
		
	return (SIDE_FRONT);
}

//! invert plane's equation
//////////////////////////////////////////////////////////////////////////
void	CPlanef::Invert()
{
	m_vNormal = -m_vNormal;
	m_fDist = -m_fDist; 	
}

//////////////////////////////////////////////////////////////////////////
void	CPlanef::Normalize()
{
	ftype fMag;
	fMag=m_vNormal.Length();		
	m_vNormal.x=m_vNormal.x / fMag;
	m_vNormal.y=m_vNormal.y / fMag;
	m_vNormal.z=m_vNormal.z / fMag;
	m_fDist = m_fDist / fMag;
}

//! snap a plane to the nearest grid
//////////////////////////////////////////////////////////////////////////
void	CPlanef::Snap()
{
	for (int i=0 ; i<3 ; i++)
	{
	  if (fabs(m_vNormal[i] - 1) < NORMAL_EPSILON)
	  {
	    m_vNormal.Clear();			
	    m_vNormal[i] = 1;
	    break;
	  }
	  
	  if (fabs(m_vNormal[i]- -1) < NORMAL_EPSILON )
	  {
	    m_vNormal.Clear();
	    m_vNormal[i] = -1;
	    break;
	  }
	}
	
	ftype fNewDist=(ftype)floor(m_fDist+0.05f);	
	if (fabs(m_fDist-fNewDist) < DIST_EPSILON) 
	  m_fDist = fNewDist;
}

//////////////////////////////////////////////////////////////////////////
vector3f CPlanef::Closest(const vector3f &vPoint)
{
	ftype	t=DistFromPlane(vPoint);
	return (vPoint - m_vNormal * t);
}
