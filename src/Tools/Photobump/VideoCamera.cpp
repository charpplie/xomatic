

#include "StdAfx.h"
#include "video.h"
#include "photoimage.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

//////////////////////////////////////////////////////////////////////////
void CVideo::MyPerspective(double fovx, double aspect, double zNear, double zFar)
{
   // This code is based off the MESA source for gluPerspective
   // *NOTE* This assumes GL_PROJECTION is the current matrix

   double xmin, xmax, ymin, ymax;
   double m[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

   xmax = zNear * tan(fovx * M_PI / 360.0);
   xmin = -xmax;
   
   ymin = (xmin / aspect)*aspect;
   ymax = (xmax / aspect)*aspect;

	 #define M(row,col)  m[col*4+row]

   // Set up the projection matrix
   M(0,0) = (2.0 * zNear) / (xmax - xmin);
   M(1,1) = (2.0 * zNear) / (ymax - ymin);
   M(2,2) = -(zFar + zNear) / (zFar - zNear);

   M(0,2) = (xmax + xmin) / (xmax - xmin);
   M(1,2) = (ymax + ymin) / (ymax - ymin);
   M(3,2) = -1.0;

   M(2,3) = -(2.0 * zFar * zNear) / (zFar - zNear);

	 #undef M	
   
   // Add to current matrix
   glMultMatrixd(m);
}

//////////////////////////////////////////////////////////////////////////
void MyPerspective2(double fovx, double aspect, double zNear, double zFar)
{
   // This code is based off the MESA source for gluPerspective
   // *NOTE* This assumes GL_PROJECTION is the current matrix

   double xmin, xmax, ymin, ymax;
   double m[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

   xmax = zNear * tan(fovx * M_PI / 360.0);
   xmin = -xmax;
   
   ymin = (xmin / aspect);
   ymax = (xmax / aspect);

	 #define M(row,col)  m[col*4+row]

   // Set up the projection matrix
   M(0,0) = (2.0 * zNear) / (xmax - xmin);
   M(1,1) = (2.0 * zNear) / (ymax - ymin);
   M(2,2) = -(zFar + zNear) / (zFar - zNear);

   M(0,2) = (xmax + xmin) / (xmax - xmin);
   M(1,2) = (ymax + ymin) / (ymax - ymin);
   M(3,2) = -1.0;

   M(2,3) = -(2.0 * zFar * zNear) / (zFar - zNear);

	 #undef M	
   
   // Add to current matrix
   glMultMatrixd(m);
}

/////////////////////////////////////////////////////////////////
void CVideo::SetCamera(CCamera *pCamera,bool bUseGLMatrix,bool bUseGLViewport)
{ 

	pCamera->m_bProjectSet=pCamera->m_bUnprojectSet=false;

	//set projection matrix
	int nViewport[4];
	if (!bUseGLViewport)
	{			
		glGetIntegerv(GL_VIEWPORT,nViewport);
		glViewport(pCamera->m_nViewport[0],pCamera->m_nViewport[1],pCamera->m_nViewport[2],pCamera->m_nViewport[3]);	
	}

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();	
	if (pCamera->m_bCustomPerspective)
	{	
	//#ifdef FORWARD_MOTION
		if (!pCamera->m_bLoadedFromBojou)
			MyPerspective2(RAD2DEG(pCamera->m_fFov),(ftype)(pCamera->m_nWidth)/(ftype)(pCamera->m_nHeight),pCamera->m_fZMin,pCamera->m_fZMax);
		//#else
		else
			MyPerspective(RAD2DEG(pCamera->m_fFov),(ftype)(pCamera->m_nWidth)/(ftype)(pCamera->m_nHeight),pCamera->m_fZMin,pCamera->m_fZMax);	
		//#endif
	}
	else
		// camera.fov is for horizontal -> GL needs it vertical
		// projection.ratio is height/width -> GL needs width/height
		gluPerspective(pCamera->m_fFov/TO_RAD*pCamera->m_fProjectionRatio,1.0/pCamera->m_fProjectionRatio,pCamera->m_fZMin,pCamera->m_fZMax);		

	//get proj matrix	
	glGetDoublev(GL_PROJECTION_MATRIX,m_Camera.m_dGLProjMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX,pCamera->m_dGLProjMatrix);

	//set view matrix
	glMatrixMode(GL_MODELVIEW); 
	glLoadIdentity();	
			
	if (bUseGLMatrix)
	{	
		if (!pCamera->m_nPivot)
		{		
			if (!pCamera->m_bLoadedFromBojou)
			{			
//#ifdef FORWARD_MOTION			
				glRotated(pCamera->m_vAngles.x,1,0,0);	//pitch	
				glRotated(pCamera->m_vAngles.z,0,1,0);	//roll
				glRotated(pCamera->m_vAngles.y,0,0,1);	//yaw					
				glTranslated(-pCamera->m_vOrigin.x,-pCamera->m_vOrigin.y,-pCamera->m_vOrigin.z);	
			}
			else
			{			
//#else
				glRotated(pCamera->m_vAngles.x,1,0,0);	//pitch	
				glRotated(pCamera->m_vAngles.z,0,1,0);	//roll
				glRotated(pCamera->m_vAngles.y,0,0,1);	//yaw					
				glTranslated(-pCamera->m_vOrigin.x,-pCamera->m_vOrigin.y,-(pCamera->m_vOrigin.z+pCamera->m_fOffset));	
			}
//#endif
		}		
		else	
		{	
			// Position the camera
			glTranslated(-pCamera->m_vOrigin.x,-pCamera->m_vOrigin.y,-pCamera->m_vOrigin.z);
			
			// Rotate the camera
			glRotated(pCamera->m_vAngles.x, 1.0, 0.0, 0.0 ); 
			glRotated(pCamera->m_vAngles.z, 0.0, 1.0, 0.0 ); 
		}		

		//get view matrix	
		glGetDoublev(GL_MODELVIEW_MATRIX,m_Camera.m_dGLViewMatrix);
		glGetDoublev(GL_MODELVIEW_MATRIX,pCamera->m_dGLViewMatrix);
	}
	else
	{	
		//ASSERT(0);
		//glMultMatrixf(&pCamera->m_OrigMatrix.m_Tvalues[0][0]);
		//glRotatef(180,1.0, 0.0,0.0);
		//glLoadMatrixf(&pCamera->m_Matrix.m_Tvalues[0][0]);
		//glTranslated(-pCamera->m_vOrigin.x,-pCamera->m_vOrigin.y,-pCamera->m_vOrigin.z);	

		CMatrixf mat;
		mat.Identity();		

//#ifdef FORWARD_MOTION			
		if (!pCamera->m_bLoadedFromBojou)
		{
			mat.MultMatrixf(&pCamera->m_OrigMatrix.m_Tvalues[0][0]);				
			mat.TranslateMatrix(vector3f(-pCamera->m_vOrigin.x,-pCamera->m_vOrigin.y,-pCamera->m_vOrigin.z));
		}
		else
//#else
		{		
			mat.RotateMatrixf(180.0,1.0,0,0);//rot+=1.0;		
			mat.MultMatrixf(&pCamera->m_OrigMatrix.m_Tvalues[0][0]);				
			//mat.RotateMatrixf(180.0,0,0,1.0);
			//mat.RotateMatrixf(180.0,0,1.0,0);
			mat.TranslateMatrix(vector3f(-pCamera->m_vOrigin.x,-pCamera->m_vOrigin.y,-pCamera->m_vOrigin.z));
			//mat.TranslateMatrix(vector3f(pCamera->m_vOrigin.x,pCamera->m_vOrigin.y,pCamera->m_vOrigin.z));		
		}
//#endif

		glMultMatrixd(&mat.m_Tvalues[0][0]);

		//get view matrix	
		memcpy(m_Camera.m_dGLViewMatrix,&mat.m_Tvalues[0][0],sizeof(CMatrixf));
		memcpy(pCamera->m_dGLViewMatrix,&mat.m_Tvalues[0][0],sizeof(CMatrixf));
	}

	//set this camera for later use
	memcpy(&m_Camera,pCamera,sizeof(CCamera));
      
	if (!bUseGLViewport)
	{					
		glViewport(nViewport[0],nViewport[1],nViewport[2],nViewport[3]);	
	}
	
	glGetIntegerv(GL_VIEWPORT,m_Camera.m_nViewport);

	//set back the matrix for the camera and calc frustum planes
	//if (bUseGLMatrix)
	pCamera->SetCameraMatrix(m_Camera.m_dGLProjMatrix,m_Camera.m_dGLViewMatrix);
}

/*
 * Transform a point (column vector) by a 4x4 matrix.  I.e.  out = m * in
 * Input:  m - the 4x4 matrix
 *         in - the 4x1 vector
 * Output:  out - the resulting 4x1 vector.
 */
static void
transform_point(double out[4], const double m[16], const double in[4])
{
#define M(row,col)  m[col*4+row]
   out[0] =
      M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0, 3) * in[3];
   out[1] =
      M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1, 3) * in[3];
   out[2] =
      M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2, 3) * in[3];
   out[3] =
      M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3, 3) * in[3];
#undef M
}

/*
 * Perform a 4x4 matrix multiplication  (product = a x b).
 * Input:  a, b - matrices to multiply
 * Output:  product - product of a and b
 */
static void
matmul(GLdouble * product, const GLdouble * a, const GLdouble * b)
{
   // This matmul was contributed by Thomas Malik 
   GLdouble temp[16];
   GLint i;

#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define T(row,col)  temp[(col<<2)+row]

   // i-te Zeile 
   for (i = 0; i < 4; i++) {
      T(i, 0) =
	 A(i, 0) * B(0, 0) + A(i, 1) * B(1, 0) + A(i, 2) * B(2, 0) + A(i,
								       3) *
	 B(3, 0);
      T(i, 1) =
	 A(i, 0) * B(0, 1) + A(i, 1) * B(1, 1) + A(i, 2) * B(2, 1) + A(i,
								       3) *
	 B(3, 1);
      T(i, 2) =
	 A(i, 0) * B(0, 2) + A(i, 1) * B(1, 2) + A(i, 2) * B(2, 2) + A(i,
								       3) *
	 B(3, 2);
      T(i, 3) =
	 A(i, 0) * B(0, 3) + A(i, 1) * B(1, 3) + A(i, 2) * B(2, 3) + A(i,
								       3) *
	 B(3, 3);
   }

#undef A
#undef B
#undef T
   memcpy(product, temp, 16 * sizeof(GLdouble));
}

// projection du point (objx,objy,obz) sur l'ecran (winx,winy,winz) 
//////////////////////////////////////////////////////////////////////////
int CVideo::MyProject(double objx, double objy, double objz,
	   //const double model[16], const double proj[16],const GLint viewport[4],
		 CCamera *pCam,
	   double * winx, double * winy, double * winz,bool bCache)
{
   // matrice de transformation 
   double in[4], out[4];
	 double	m[16],*matptr;

   // initilise la matrice et le vecteur a transformer 
   in[0] = objx;
   in[1] = objy;
   in[2] = objz;
   in[3] = 1.0;
   //transform_point(out, model, in);
   //transform_point(in, proj, out);

   //transform_point(out, pCam->m_dGLViewMatrix, in);
   //transform_point(in, pCam->m_dGLProjMatrix, out);

	 if (!bCache || (!pCam->m_bProjectSet))
	 {
		matmul(m,pCam->m_dGLProjMatrix,pCam->m_dGLViewMatrix);				
		memcpy(pCam->m_dProject,m,sizeof(double)*16);		
		pCam->m_bProjectSet=bCache;		
	 }
	 
	 matptr=pCam->m_dProject;

	 transform_point(out, matptr, in);

   // d'ou le resultat normalise entre -1 et 1 
   if (out[3] == 0.0)
      return 0;

   out[0] /= out[3];
   out[1] /= out[3];
   out[2] /= out[3];

   // en coordonnees ecran 
   *winx = pCam->m_nViewport[0] + (1 + out[0]) * pCam->m_nViewport[2] / 2;
   *winy = pCam->m_nViewport[1] + (1 + out[1]) * pCam->m_nViewport[3] / 2;
   // entre 0 et 1 suivant z 
   *winz = (1 + out[2]) / 2;
   return 1;
}

/*
 * Compute inverse of 4x4 transformation matrix.
 * Code contributed by Jacques Leroy jle@star.be
 * Return GL_TRUE for success, GL_FALSE for failure (singular matrix)
 */
static GLboolean
invert_matrix(const GLdouble * m, GLdouble * out)
{
// NB. OpenGL Matrices are COLUMN major. 
#define SWAP_ROWS(a, b) { GLdouble *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(c)*4+(r)]

   GLdouble wtmp[4][8];
   GLdouble m0, m1, m2, m3, s;
   GLdouble *r0, *r1, *r2, *r3;

   r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

   r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
      r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
      r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
      r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
      r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
      r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
      r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
      r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
      r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
      r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
      r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
      r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

   /* choose pivot - or die */
   if (fabs(r3[0]) > fabs(r2[0]))
      SWAP_ROWS(r3, r2);
   if (fabs(r2[0]) > fabs(r1[0]))
      SWAP_ROWS(r2, r1);
   if (fabs(r1[0]) > fabs(r0[0]))
      SWAP_ROWS(r1, r0);
   if (0.0 == r0[0])
      return GL_FALSE;

   /* eliminate first variable     */
   m1 = r1[0] / r0[0];
   m2 = r2[0] / r0[0];
   m3 = r3[0] / r0[0];
   s = r0[1];
   r1[1] -= m1 * s;
   r2[1] -= m2 * s;
   r3[1] -= m3 * s;
   s = r0[2];
   r1[2] -= m1 * s;
   r2[2] -= m2 * s;
   r3[2] -= m3 * s;
   s = r0[3];
   r1[3] -= m1 * s;
   r2[3] -= m2 * s;
   r3[3] -= m3 * s;
   s = r0[4];
   if (s != 0.0) {
      r1[4] -= m1 * s;
      r2[4] -= m2 * s;
      r3[4] -= m3 * s;
   }
   s = r0[5];
   if (s != 0.0) {
      r1[5] -= m1 * s;
      r2[5] -= m2 * s;
      r3[5] -= m3 * s;
   }
   s = r0[6];
   if (s != 0.0) {
      r1[6] -= m1 * s;
      r2[6] -= m2 * s;
      r3[6] -= m3 * s;
   }
   s = r0[7];
   if (s != 0.0) {
      r1[7] -= m1 * s;
      r2[7] -= m2 * s;
      r3[7] -= m3 * s;
   }

   /* choose pivot - or die */
   if (fabs(r3[1]) > fabs(r2[1]))
      SWAP_ROWS(r3, r2);
   if (fabs(r2[1]) > fabs(r1[1]))
      SWAP_ROWS(r2, r1);
   if (0.0 == r1[1])
      return GL_FALSE;

   /* eliminate second variable */
   m2 = r2[1] / r1[1];
   m3 = r3[1] / r1[1];
   r2[2] -= m2 * r1[2];
   r3[2] -= m3 * r1[2];
   r2[3] -= m2 * r1[3];
   r3[3] -= m3 * r1[3];
   s = r1[4];
   if (0.0 != s) {
      r2[4] -= m2 * s;
      r3[4] -= m3 * s;
   }
   s = r1[5];
   if (0.0 != s) {
      r2[5] -= m2 * s;
      r3[5] -= m3 * s;
   }
   s = r1[6];
   if (0.0 != s) {
      r2[6] -= m2 * s;
      r3[6] -= m3 * s;
   }
   s = r1[7];
   if (0.0 != s) {
      r2[7] -= m2 * s;
      r3[7] -= m3 * s;
   }

   /* choose pivot - or die */
   if (fabs(r3[2]) > fabs(r2[2]))
      SWAP_ROWS(r3, r2);
   if (0.0 == r2[2])
      return GL_FALSE;

   /* eliminate third variable */
   m3 = r3[2] / r2[2];
   r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
      r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];

   /* last check */
   if (0.0 == r3[3])
      return GL_FALSE;

   s = 1.0 / r3[3];		/* now back substitute row 3 */
   r3[4] *= s;
   r3[5] *= s;
   r3[6] *= s;
   r3[7] *= s;

   m2 = r2[3];			/* now back substitute row 2 */
   s = 1.0 / r2[2];
   r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
      r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
   m1 = r1[3];
   r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
      r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
   m0 = r0[3];
   r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
      r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

   m1 = r1[2];			/* now back substitute row 1 */
   s = 1.0 / r1[1];
   r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
      r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
   m0 = r0[2];
   r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
      r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

   m0 = r0[1];			/* now back substitute row 0 */
   s = 1.0 / r0[0];
   r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
      r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

   MAT(out, 0, 0) = r0[4];
   MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
   MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
   MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
   MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
   MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
   MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
   MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
   MAT(out, 3, 3) = r3[7];

   return GL_TRUE;

#undef MAT
#undef SWAP_ROWS
}

//////////////////////////////////////////////////////////////////////////
int CVideo::MyUnproject(double winx, double winy, double winz,
	      //const double modelMatrix[16],
	      //const double projMatrix[16],
	      //const GLint viewport[4],
	      //GLclampd nearZ, GLclampd farZ,
				CCamera *pCam,
	      double * objx, double * objy, double * objz,
				bool bCache)
{
   // matrice de transformation 
   double m[16], A[16],*matptr;
   double in[4], out[4];
   double z = winz; //nearZ + winz * (farZ - nearZ);

   // transformation coordonnees normalisees entre -1 et 1 
   in[0] = (winx - pCam->m_nViewport[0]) * 2 / pCam->m_nViewport[2] - 1.0;
   in[1] = (winy - pCam->m_nViewport[1]) * 2 / pCam->m_nViewport[3] - 1.0;
   in[2] = 2.0 * z - 1.0;
   in[3] = 1.0f; //clipw;

   // calcul transformation inverse 
	 if (!bCache || (!pCam->m_bUnprojectSet))
	 {
		//matmul(A, projMatrix, modelMatrix);
		//invert_matrix(A, m);
		matmul(A,pCam->m_dGLProjMatrix,pCam->m_dGLViewMatrix);
		invert_matrix(A, m);		
		memcpy(pCam->m_dUnproject,m,sizeof(double)*16);		
		pCam->m_bUnprojectSet=bCache;		
	 }
	 
	matptr=pCam->m_dUnproject;
	 
   // d'ou les coordonnees objets 
   transform_point(out, matptr, in);
   if (out[3] == 0.0)
      return 0;
   *objx = out[0] / out[3];
   *objy = out[1] / out[3];
   *objz = out[2] / out[3];
   //*objw = out[3];
   return 1;
}

//////////////////////////////////////////////////////////////////////
void CVideo::ProjectToScreen(const vector3f &vSource,vector3f &vDest,CCamera *pCam,bool bCache)
{
	double dx,dy,dz;	
	//if (!pCam)
	//	pCam=&m_Camera;

  MyProject(vSource.x,vSource.y,vSource.z,
              pCam, 
							&dx,&dy,&dz,bCache);

	/*
  MyProject(vSource.x,vSource.y,vSource.z,
              pCam->m_dGLViewMatrix,
              pCam->m_dGLProjMatrix,
              pCam->m_nViewport,
              &dx,&dy,&dz);
	*/

	/*
  gluProject(vSource.x,vSource.y,vSource.z,
              pCam->m_dGLViewMatrix,
              pCam->m_dGLProjMatrix,
              pCam->m_nViewport,
              &dx,&dy,&dz);
	*/
	vDest.x=(ftype)(dx);
	vDest.y=(ftype)((pCam->m_nViewport[3])-dy);
	vDest.z=(ftype)(dz);
}

//////////////////////////////////////////////////////////////////////
void CVideo::UnProjectFromScreen(const vector3f &vSource,vector3f &vDest,CCamera *pCam,bool bCache)
{
	double dx,dy,dz;

	if (!pCam)
		pCam=&m_Camera;

	vector3f vTemp=vSource;
	vTemp.y=(pCam->m_nViewport[3]-vSource.y);

  MyUnproject(vTemp.x,vTemp.y,vTemp.z,
								pCam,
                &dx,&dy,&dz,bCache);		

	/*
  MyUnproject(vTemp.x,vTemp.y,vTemp.z,
								pCam->m_dGLViewMatrix,
								pCam->m_dGLProjMatrix,								
								pCam->m_nViewport,
								pCam->m_fZMin,pCam->m_fZMax,
                &dx,&dy,&dz);		
	*/

	/*
  gluUnProject(vSource.x,vSource.y,vSource.z,
								pCam->m_dGLViewMatrix,
								pCam->m_dGLProjMatrix,
								pCam->m_nViewport,
                     &dx,&dy,&dz);		
	*/


	vDest.x=(ftype)(dx);vDest.y=(ftype)(dy);vDest.z=(ftype)(dz);

  //CheckError("Unproject from screen");  
}
