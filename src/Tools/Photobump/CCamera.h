
#ifndef CAMERA_H
#define CAMERA_H

#if _MSC_VER > 1000
# pragma once
#endif

#include	"CMatrix.h"
#include	"CPlane.h"
#include	"CAABB.h"
//////////////////////////////////////////////////////////////////////

#define FRUSTUM_PLANES		6
#define MASK_ALL_PLANES		1|2|4|8|16|32

#define DEFAULT_ZMAX	1024.0
#define DEFAULT_ZMIN	0.25
//#define DEFAULT_FOV		M_PI/2.0
#define DEFAULT_FOV		(DEG2RAD(60.0))

//////////////////////////////////////////////////////////////////////
class CCamera 
{
	
public:
	CCamera()
	{
		m_vOrigin.Clear(); //in the center of the world
		m_vAngles.Clear();
		m_vOldPos.Clear();
		m_fRotation=0;
		m_Matrix.Identity();		
		m_OrigMatrix.Identity();
		m_fOffset=0;
		m_fSpeed=1.0;
		m_nPivot=0;
		m_nWidth=m_nHeight=0;
		m_bCustomPerspective=false;
		m_bLoadedFromBojou=false;

		m_fFocalLenght=0;
		m_vPrincipalPoint.Clear();
		m_nFrame=-1;
		
		memset(m_dGLViewMatrix,0,sizeof(double)*16);		
		memset(m_dGLProjMatrix,0,sizeof(double)*16);		
		m_bProjectSet=m_bUnprojectSet=false;
	}

	const bool IntersectAABB(const CAABBd &aAABBox,unsigned int dwInClipMask,unsigned int &dwOutClipMask) const;
	const bool IntersectAABB(const CAABBd &aAABBox) const;
	
	const bool IsPointVisible(const vector3f &vPos,unsigned int dwInClipMask) const;

	//the renderer is responsable to call this function 'cos
	//the view matrix is obtained from the OpenGL/DX Matrix
	//this function computes also frustum planes
	//plane normals are pointing inside	
	void	SetCameraMatrix(double *pProjMatrix,double *pViewMatrix);

	const	vector3f	GetPos(bool bWithOffset=false) const;	
	void			SetPos(const vector3f &vPos);

	const	vector3f	GetAngles() const;
	void			SetAngles(const vector3f &vAngles);

	void	SetQuatRotation(const vector3f &vQuat,ftype fW);

	void	Init(int nWidth,int nHeight,ftype fFova=DEFAULT_FOV,ftype fZMAX=DEFAULT_ZMAX,ftype fZMIN=DEFAULT_ZMIN);

	bool	IsSphereVisible(const vector3f &center,ftype radius);

	vector3f GetForwardVector() const;
	vector3f GetUpVector() const;
	vector3f GetRightVector() const;

	void			GetRay(ftype fX,ftype fY,int w,int h,vector3f &v1,vector3f &v2);

	CPlanef		m_Planes[FRUSTUM_PLANES];
	vector3f	m_vOrigin;
	vector3f	m_vOldPos;
	CMatrixf	m_Matrix,m_OrigMatrix;

	ftype			m_fOffset;
	ftype			m_fSpeed;
	
	int				m_nPivot;
	int				m_nWidth,m_nHeight;
	int				m_nFrame;

	bool			m_bCustomPerspective;
	bool			m_bLoadedFromBojou;

	// to match quaternion and glRotate form
	vector3f  m_vAngles;
	ftype			m_fRotation;
	ftype			m_fZMin;
	ftype			m_fZMax;
	ftype			m_fFov;
	ftype			m_fProjectionRatio;

	ftype     m_fFocalLenght;
	vector3f	m_vPrincipalPoint;

	// for projection/unprojection	
	double		m_dGLProjMatrix[16],m_dGLViewMatrix[16];
	int				m_nViewport[4];
	double		m_dUnproject[16],m_dProject[16];
	bool			m_bProjectSet,m_bUnprojectSet;
};

typedef std::vector<CCamera *>	lstCameras;
typedef lstCameras::iterator		lstCamerasIt;

#endif
