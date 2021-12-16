

#ifndef PLANE_H
#define PLANE_H

#if _MSC_VER > 1000
# pragma once
#endif

//////////////////////////////////////////////////////////////////////
#include "vector3.h"

//////////////////////////////////////////////////////////////////////
#define	NORMAL_EPSILON	0.0001f
#define	DIST_EPSILON	0.01f

//////////////////////////////////////////////////////////////////////
template<class T> class CPlane
{
public:
	 
	CPlane() 
	{		
		m_vNormal.Clear();				
		m_fDist=0;
	};

	//! Initialize a plane giving normal and dist
	CPlane(const vector3f &vNormal,T fDist)
	{
		Set(vNormal,fDist);
	}

	CPlane(const vector3f &vNormal,const vector3f &vOrigin)
	{
		ftype fDist=vOrigin*vNormal;
		Set(vNormal,fDist);
	}

	vector3f Closest(const vector3f &vPoint);
	
	//! set normal and dist for this plane 
	void	Set(const vector3f &vNormal,T fDist);

	//! calc the plane giving 3 points
	//! returns false for degenrated triangles
	bool	CalcPlane(const vector3f &p0,const vector3f &p1,const vector3f &p2);

	//! tell which side of the plane the box is
  int CalcBoxSide(const vector3f &vMins, const vector3f &vMaxs) const;

	//! invert plane's equation
	void	Invert();

	void	Normalize();

	//! check for equality between two planes
	inline  friend	bool operator ==(const CPlane<T> &p1, const CPlane<T> &p2)
  {
    if (Ffabs(p1.m_vNormal.x-p2.m_vNormal.x)>NORMAL_EPSILON) 
      return (false);
    if (Ffabs(p1.m_vNormal.y-p2.m_vNormal.y)>NORMAL_EPSILON) 
      return (false);
    if (Ffabs(p1.m_vNormal.z-p2.m_vNormal.z)>NORMAL_EPSILON) 
      return (false);
    
    if (Ffabs(p1.m_fDist-p2.m_fDist)<DIST_EPSILON) 
      return(true);
    return (false);
  }

	//! snap a plane to the nearest grid
	void	Snap();

	//! calc the distance from the plane to point p
	inline T DistFromPlane(const vector3f &vPoint)
	{		
		return (m_vNormal*vPoint-m_fDist);
	}

	//! same as above for const members
	T	DistFromPlane(const vector3f &vPoint) const
	{		
		return (m_vNormal*vPoint-m_fDist);
	}

	bool RayIntersect(const vector3f &vRayOrigin, const vector3f &vRayDir,T &fTime) const;
	bool LineStabsPlane(const vector3f &v1, const vector3f &v2,vector3f &vRes) const;

//protected: 
	
	vector3f	m_vNormal;
	T						m_fDist;	
};


//////////////////////////////////////////////////////////////////////
typedef	CPlane<ftype> CPlanef;

#endif //plane