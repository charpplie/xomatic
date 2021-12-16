
//include headers
/////////////////////////////////////////////////////////////////
#include	"stdafx.h"

#define DEBUG_PLANES			1

//////////////////////////////////////////////////////////////////////////
const bool CCamera::IntersectAABB(const CAABBd &aAABBox) const
{
	ftype fDot1,fVal;
	CPlanef *pFrusPtr=(CPlanef *)&m_Planes[0]; 
	  		
	//move to world space
	vector3f vMins=aAABBox.m_vMins+aAABBox.m_vWorldSpacePos;
	vector3f vMaxs=aAABBox.m_vMaxs+aAABBox.m_vWorldSpacePos;

	for (int k=0;k<FRUSTUM_PLANES;k++,pFrusPtr++) // loop while there are active planes
	{				
		fDot1=pFrusPtr->m_fDist;		
		
		fVal=pFrusPtr->m_vNormal.x;
		if (fVal<=0) 			
			fDot1+=fVal*vMins.x; 							
		else 		
			fDot1+=fVal*vMaxs.x;							
		
		fVal=pFrusPtr->m_vNormal.y;
		if (fVal<=0)
			fDot1+=fVal*vMins.y; 
		else 			
			fDot1+=fVal*vMaxs.y;
				
		fVal=pFrusPtr->m_vNormal.z;
		if (fVal<=0) 
			fDot1+=fVal*vMins.z; 
		else 
			fDot1+=fVal*vMaxs.z;		
				
		if (fDot1<0) 
			return (false);				  		
	} 
	
	// The AABB is either partially or fully visible
	return (true);	
}


/////////////////////////////////////////////////////////////////
const bool CCamera::IntersectAABB(const CAABBd &aAABBox,unsigned int dwInClipMask,unsigned int &dwOutClipMask) const
{
	ftype fDot1,fDot2,fVal;
	CPlanef *pFrusPtr=(CPlanef *)&m_Planes[0]; 
	  	
	unsigned int mk	= 1;
	dwOutClipMask		= 0;					// init outclip mask

	//move to world space
	vector3f vMins=aAABBox.m_vMins+aAABBox.m_vWorldSpacePos;
	vector3f vMaxs=aAABBox.m_vMaxs+aAABBox.m_vWorldSpacePos;

	while (mk<=dwInClipMask) // loop while there are active planes
	{				
		if (dwInClipMask & mk) //if clip plane is active
		{	
			fDot1=fDot2=pFrusPtr->m_fDist;		
		  
			fVal=pFrusPtr->m_vNormal.x;
			if (fVal<=0) 
			{
				fDot1+=fVal*vMins.x; 
				fDot2+=fVal*vMaxs.x;
			}			
			else 
			{
				fDot1+=fVal*vMaxs.x;
				fDot2+=fVal*vMins.x;
			}
		  
			fVal=pFrusPtr->m_vNormal.y;
			if (fVal<=0)
			{
				fDot1+=fVal*vMins.y; 
				fDot2+=fVal*vMaxs.y;
			}			
			else 
			{
				fDot1+=fVal*vMaxs.y;
				fDot2+=fVal*vMins.y;
			}
					
			fVal=pFrusPtr->m_vNormal.z;
			if (fVal<=0) 
			{
				fDot1+=fVal*vMins.z; 
				fDot2+=fVal*vMaxs.z;
			}			
			else 
			{
				fDot1+=fVal*vMaxs.z;		
				fDot2+=fVal*vMins.z;
			}
					
			if (fDot1<0) 
				return (false);		
		  
			if (fDot2<0) //intersetcs, min or max is positive and max or min is negative				
				dwOutClipMask |= mk;				
		}

		mk+=mk;			// mk = (1<<iter)
		pFrusPtr++;	// next plane

	} //mk<=dwInClipMask
	
	// The AABB is either partially or fully visible
	return (true);	
}

//the renderer is responsable to call this function 'cos
//the view matrix is obtained from the OpenGL/DX Matrix
//this function computes also frustum planes
//plane normals are pointing inside	
/////////////////////////////////////////////////////////////////
void	CCamera::SetCameraMatrix(ftype *pProjMatrix,ftype *pViewMatrix)
{
	memcpy(m_dGLProjMatrix,pProjMatrix,sizeof(ftype)*16);
	memcpy(m_dGLViewMatrix,pViewMatrix,sizeof(ftype)*16);
	//combine the two matrices		
	m_Matrix.Identity();
	m_Matrix.MultMatrixf(pProjMatrix);
	m_Matrix.MultMatrixf(pViewMatrix);

	vector3f vNormal;

	// RIGHT clipping plane
	vNormal.x = m_Matrix.cell(3)-m_Matrix.cell(0);
	vNormal.y = m_Matrix.cell(7)-m_Matrix.cell(4);
	vNormal.z = m_Matrix.cell(11)-m_Matrix.cell(8);
	m_Planes[3].Set(vNormal,m_Matrix.cell(15)-m_Matrix.cell(12));		

	// LEFT clipping plane
	vNormal.x = m_Matrix.cell(3)+m_Matrix.cell(0);
	vNormal.y = m_Matrix.cell(7)+m_Matrix.cell(4);
	vNormal.z = m_Matrix.cell(11)+m_Matrix.cell(8);
	m_Planes[2].Set(vNormal,m_Matrix.cell(15)+m_Matrix.cell(12));		

	// BOTTOM clipping plane
	vNormal.x = m_Matrix.cell(3)+m_Matrix.cell(1);
	vNormal.y = m_Matrix.cell(7)+m_Matrix.cell(5);
	vNormal.z = m_Matrix.cell(11)+m_Matrix.cell(9);
	m_Planes[4].Set(vNormal,m_Matrix.cell(15)+m_Matrix.cell(13));		

	// TOP clipping plane
	vNormal.x = m_Matrix.cell(3)-m_Matrix.cell(1);
	vNormal.y = m_Matrix.cell(7)-m_Matrix.cell(5);
	vNormal.z = m_Matrix.cell(11)-m_Matrix.cell(9);
	m_Planes[5].Set(vNormal,m_Matrix.cell(15)-m_Matrix.cell(13));		

	// FAR clipping plane
	vNormal.x = m_Matrix.cell(3)-m_Matrix.cell(2);
	vNormal.y = m_Matrix.cell(7)-m_Matrix.cell(6);
	vNormal.z = m_Matrix.cell(11)-m_Matrix.cell(10);
	m_Planes[1].Set(vNormal,m_Matrix.cell(15)-m_Matrix.cell(14));		

	// NEAR clipping plane
	vNormal.x = m_Matrix.cell(3)+m_Matrix.cell(2);
	vNormal.y = m_Matrix.cell(7)+m_Matrix.cell(6);
	vNormal.z = m_Matrix.cell(11)+m_Matrix.cell(10);
	m_Planes[0].Set(vNormal,m_Matrix.cell(15)+m_Matrix.cell(14));		

	//they dont really need to be normalized - its just to
	//simplify debugging
#ifdef DEBUG_PLANES
for (int k=0;k<FRUSTUM_PLANES;k++)
	m_Planes[k].Normalize();
#endif
}

/////////////////////////////////////////////////////////////////
const bool CCamera::IsPointVisible(const vector3f &vPos,unsigned int dwInClipMask) const
{
	CPlanef *pFrusPtr=(CPlanef *)&m_Planes[0]; 
	  	
	unsigned int mk	= 1;
	ftype fDist;
	
	while (mk<=dwInClipMask) // loop while there are active planes
	{				
		if (dwInClipMask & mk) //if clip plane is active
		{	
			fDist=pFrusPtr->m_vNormal*vPos+pFrusPtr->m_fDist;
			if (fDist<0)
				return (false);
		}

		mk+=mk;			// mk = (1<<iter)
		pFrusPtr++;	// next plane

	} //mk<=dwInClipMask

	return (true);
}

/////////////////////////////////////////////////////////////////
void	CCamera::SetPos(const vector3f &vPos)
{
	m_vOldPos=m_vOrigin;
	m_vOrigin=vPos;
}

/////////////////////////////////////////////////////////////////
const vector3f CCamera::GetPos(bool bWithOffset) const
{
	if (bWithOffset)
	{
		vector3f vPos=m_vOrigin;
		vPos.z+=m_fOffset;
		return (vPos);
	}
	return (m_vOrigin);
}

/////////////////////////////////////////////////////////////////
void	CCamera::SetAngles(const vector3f &vAngles) 
{		
	m_vAngles=vAngles;
	for (int k=0;k<3;k++)
	{
		if (m_vAngles[k]<0)
			m_vAngles[k]+=360;
		else
		if (m_vAngles[k]>360)
			m_vAngles[k]-=360;
	}
}

/////////////////////////////////////////////////////////////////
const vector3f CCamera::GetAngles() const
{
	return (m_vAngles);
}

/////////////////////////////////////////////////////////////////
void	CCamera::SetQuatRotation(const vector3f &vQuat,ftype fW)
{
	m_vAngles=vQuat;
	m_fRotation=m_vAngles.QuatToEuler(fW);		
}

/////////////////////////////////////////////////////////////////
void	CCamera::Init(int nWidth,int nHeight,ftype fFova/*=DEFAULT_FOV*/,ftype fZMAX/*=DEFAULT_ZMAX*/,ftype fZMIN/*=DEFAULT_ZMIN*/)
{	
	m_fFov = fFova;
	m_fZMax=fZMAX;	  
	m_fZMin=fZMIN; 	 		

	ftype fTw=(ftype)(nWidth);
	ftype	fTh=(ftype)(nHeight);

	m_nWidth=nWidth;
	m_nHeight=nHeight;

	m_nViewport[0]=0;
	m_nViewport[1]=0;
	m_nViewport[2]=nWidth;
	m_nViewport[3]=nHeight;		

	// projection ratio (1.0 for square pixels)
	m_fProjectionRatio = fTh/fTw;		
}

//////////////////////////////////////////////////////////////////////////
vector3f CCamera::GetForwardVector() const
{
	//calculate forward vector  
  vector3f vForward(m_Matrix.cell(2),m_Matrix.cell(6),m_Matrix.cell(10)); 
  vForward.Normalize();	
	return (vForward);
}

//////////////////////////////////////////////////////////////////////////
vector3f CCamera::GetUpVector() const
{
	//calculate up vector  
  vector3f vUp(m_Matrix.cell(1),m_Matrix.cell(5),m_Matrix.cell(9)); 
  vUp.Normalize();	
	return (vUp);
}

//////////////////////////////////////////////////////////////////////////
vector3f CCamera::GetRightVector() const
{
	//calculate right vector  
  //vector3f vRight(m_Matrix.cell(3),m_Matrix.cell(7),m_Matrix.cell(11)); 
	vector3f vRight(m_Matrix.cell(0),m_Matrix.cell(4),m_Matrix.cell(8)); 
  vRight.Normalize();	
	return (vRight);
}

//////////////////////////////////////////////////////////////////////////
bool	CCamera::IsSphereVisible(const vector3f &center,ftype radius)
{
	ftype dist;
	CPlanef *pFrusPtr=(CPlanef *)&m_Planes[0];
	
	for (int i=0;i<FRUSTUM_PLANES;i++,pFrusPtr++) 	
	{
	  dist=pFrusPtr->DistFromPlane(center);
	  if (dist>radius) 
	    return (false);
	}
	
	return (true);	
}

//////////////////////////////////////////////////////////////////////////
void CCamera::GetRay(ftype fX,ftype fY,int w,int h,vector3f &v1,vector3f &v2)
{
	vector3f vAngles=GetAngles();
	
	int nCentX=w>>1;
	int nCentY=h>>1;

	// calc angles step for both x & y field of view

	ftype fAngXStep=(RAD2DEG(m_fFov/2.0))/(ftype)(nCentX);
	// calc FovY and then calc half step
	ftype fAngYStep=((m_fFov/TO_RAD*m_fProjectionRatio)/2.0)/(ftype)(nCentY);
	
	// point is on the screen
	// point.x is left/right, angle.x is up/down
	// point.y is up/down, angle.y is roll - angle-z is left/right
	
	ftype fAngY=(ftype)(fY-nCentY)*fAngYStep; // up/down
	ftype fAngX=(ftype)(fX-nCentX)*fAngXStep; // left/right
	
	vAngles.x+=fAngY; // up/down
	vAngles.z+=fAngX; // left/right
			
	vector3f vDir=vAngles;
	vDir.VecFromCameraAngles();
	
	// origin
	v1=m_vOrigin;
	// end 
	v2=v1+vDir*m_fZMax;
}

