

#ifndef CAABB_H
#define CAABB_H

#if _MSC_VER > 1000
# pragma once
#endif
 
//////////////////////////////////////////////////////////////////////
template<class T> class CAABB
{
public:

	CAABB()
	{
		m_vMins.SetMax();
		m_vMaxs.SetMin();
		m_vWorldSpacePos.Clear(); //lets put it in the center
	}

	void	SetPos(const vector3<T>	&vPos)
	{
		m_vWorldSpacePos=vPos;
	}

	void	CheckDims(const vector3<T>	&vMins,const vector3<T>	&vMaxs)
	{
		m_vMins.CheckMin(vMins);
		m_vMaxs.CheckMax(vMaxs);
	}
	
	void	SetDims(const vector3<T>	&vMins,const vector3<T>	&vMaxs)
	{
		//world space center of the bbox. The objects are supposed
		//to be centered at 0,0,0 so this is actually the world space center
		//in object space
		m_vCenter=(vMaxs+vMins)*0.5; 
		
		m_vMins=vMins; //-m_vCenter; 
		m_vMaxs=vMaxs; //-m_vCenter; 

		m_vExt=m_vMaxs-m_vMins;		
		m_vHalfDiagonal=m_vExt-m_vCenter;
	}

	vector3<T>	m_vMins;
	vector3<T>	m_vMaxs; 
	vector3<T>	m_vWorldSpacePos; 

	//store for subsequent operations
	vector3<T>	m_vExt;
	vector3<T>	m_vCenter; 
	vector3<T>	m_vHalfDiagonal;
};

//////////////////////////////////////////////////////////////////////
typedef CAABB<float> CAABBf;
typedef CAABB<double> CAABBd;

//////////////////////////////////////////////////////////////////////
bool SphereIntersectBBox(const vector3f &vSphereOrigin,float fRadius,const CAABBd &AABB);
bool Overlap(const CAABBd &AABB1,const CAABBd &AABB2);

//////////////////////////////////////////////////////////////////////////
typedef std::vector<CAABBd> AABBVec;
typedef AABBVec::iterator AABBVecIt;

#endif